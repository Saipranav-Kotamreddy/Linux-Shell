#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "Whitespace.h"

#define CMDLINE_MAX 512

struct singleCommand{
	char program[32];
	char arguments[16][32];
	int pipeInput;
	int outputFileDescriptor;
};

pid_t runCommand(struct singleCommand cmd, char** args, int pipeList[], int pipeStart, int pipeListCount){
	pid_t pid;
	pid = fork();
	if(pid==0){
		if(cmd.pipeInput!= STDIN_FILENO){
			dup2(cmd.pipeInput, STDIN_FILENO);
		}
		if(cmd.outputFileDescriptor != STDOUT_FILENO){
			dup2(cmd.outputFileDescriptor, STDOUT_FILENO);
		}
		for(int i=pipeStart; i<pipeListCount; i++){
			close(pipeList[i]);
		}
		execvp(cmd.program, args);
		fprintf(stderr, "Error: command not found\n");
		exit(1);
	}
	if(cmd.pipeInput!= STDIN_FILENO){
		close(cmd.pipeInput);
	}
	if(cmd.outputFileDescriptor!= STDOUT_FILENO){
		close(cmd.outputFileDescriptor);
	}
	return pid;
}

int commandSplit(char* standardDupCommand, char* commandList[]){
	int commandCount=1;
	if(standardDupCommand[0]=='|' || standardDupCommand[strlen(standardDupCommand)-1]=='|'){
			fprintf(stderr, "Error: missing command\n");
			return 0;
	}
	char* pipeCommand=strtok(standardDupCommand, "|");
	strcpy(commandList[commandCount-1],pipeCommand);
	while((pipeCommand= strtok(NULL, "|"))!=NULL){
		commandCount++;
		strcpy(commandList[commandCount-1],pipeCommand);
	}
	return commandCount;
}

int parser(struct singleCommand* cmd, char* inputCommand){
	char* program = strtok(inputCommand, " ");
	memcpy(cmd->program, program, sizeof(cmd->program));
	memcpy(cmd->arguments[0], program, sizeof(cmd->arguments[0]));
	int argCount=1;
	int nextRedirectNoTrunc=0;
	int nextRedirectTrunc=0;
	//If the output descriptor is ever not -1 and the parser is still going, thats an error
	cmd->outputFileDescriptor=STDOUT_FILENO;

	if(program==NULL || !strcmp(program, ">") || !strcmp(program, ">>")){
		return -3;
	}

	while((program= strtok(NULL, " "))!=NULL){
		if(argCount==17){
			return argCount;
		}
		if(nextRedirectTrunc){
			nextRedirectTrunc=2;
			int fd = open(program, O_WRONLY | O_CREAT| O_TRUNC, 0644);
			if(fd==-1){
				return -2;
			}
			cmd->outputFileDescriptor=fd;
			continue;
		}
		if(nextRedirectNoTrunc){
			nextRedirectNoTrunc=2;
			int fd = open(program, O_WRONLY | O_CREAT | O_APPEND, 0644);
			if(fd==-1){
				return -2;
			}
			cmd->outputFileDescriptor=fd;
			continue;
		}
		if(!strcmp(program, ">")){
			nextRedirectTrunc=1;
			continue;
		}
		if(!strcmp(program, ">>")){
			nextRedirectNoTrunc=1;
			continue;
		}
		memcpy(cmd->arguments[argCount], program, sizeof(cmd->arguments[argCount]));
		argCount++;
	}
	
	if(nextRedirectTrunc==1 || nextRedirectNoTrunc==1){
		return -1;
	}
	return argCount;
}

int multiParser(int commandCount, char* commandList[], struct singleCommand parsedCommandList[],char* argList[commandCount][16], int pipeList[]){
	int pipePosition=0;
	int lastCommand=0;
	int pipeEnds[2];
	int argCount=0;
	for(int i=0; i<commandCount; i++){
		parsedCommandList[i].pipeInput=STDIN_FILENO;
		if(i!=0){
			parsedCommandList[i].pipeInput=pipeEnds[0];
		}
		if(i==commandCount-1){
			lastCommand=1;
		}
		argCount = parser(&parsedCommandList[i], commandList[i]);

		if(argCount==17){	
			fprintf(stderr, "Error: too many process arguments\n");
			return 1;
		}
	
		if(argCount==-1){
			fprintf(stderr, "Error: no output file\n");
			return 1;
		}

		if(argCount==-2){
			fprintf(stderr, "Error: cannot open output file\n");
			return 1;
		}

		if(argCount==-3){
			fprintf(stderr, "Error: missing command\n");
			return 1;
		}

		for(int j=0; j<argCount; j++){
			argList[i][j]=parsedCommandList[i].arguments[j];	
		}
		argList[i][argCount]=NULL;	
		if(!lastCommand){
			if(parsedCommandList[i].outputFileDescriptor!=STDOUT_FILENO){
				fprintf(stderr, "Error: mislocated output redirection\n");
				return 1;
			}
			pipe(pipeEnds);
			parsedCommandList[i].outputFileDescriptor=pipeEnds[1];
			pipeList[pipePosition]=pipeEnds[0];
			pipeList[pipePosition+1]=pipeEnds[1];
			pipePosition=pipePosition+2;
		}
	}
	return 0;
}

void pwd(char* cmd){
	char cwd[CMDLINE_MAX];
	getcwd(cwd, sizeof(cwd));
	fprintf(stdout, "%s\n", cwd);
	int status=0;
	fprintf(stderr, "+ completed '%s' [%d]\n",cmd, status);
}

void cd(char* cmd, char* argList[1][16]){
	int status;
	if(chdir(argList[0][1])==-1){
		fprintf(stderr, "Error: cannot cd into directory\n");
		status=1;
	}
	else{
		status=0;
	}
	fprintf(stderr, "+ completed '%s' [%d]\n",cmd, status);
}

void normalCommand(char* cmd, int commandCount, struct singleCommand parsedCommandList[commandCount], char* argList[commandCount][16], int pipeList[commandCount]){
	int status;
	int statusArray[commandCount];
	pid_t pidList[commandCount];
	int pipeStart=0;
	int pipeEnd=(commandCount-1)*2;
	for(int i=0; i<commandCount; i++){
		pidList[i] = runCommand(parsedCommandList[i],argList[i], pipeList, pipeStart, pipeEnd);
		pipeStart=pipeStart+2;
	}
	for(int j=0; j<commandCount; j++){
		waitpid(pidList[j], &status, 0);
		statusArray[j]=status;
	}
	fprintf(stderr, "+ completed '%s' ",cmd);
	for(int k=0; k<commandCount-1; k++){
		fprintf(stderr, "[%d]", WEXITSTATUS(statusArray[k]));
	}
	fprintf(stderr, "[%d]\n", WEXITSTATUS(statusArray[commandCount-1]));
}	

int main(void)
{
        char cmd[CMDLINE_MAX];

        while (1) {
                char *nl;

                /* Print prompt */
                printf("sshell@ucd$ ");
                fflush(stdout);

                /* Get command line */
                fgets(cmd, CMDLINE_MAX, stdin);

                /* Print command line if stdin is not provided by terminal */
                if (!isatty(STDIN_FILENO)) {
                        printf("%s", cmd);
                        fflush(stdout);
                }

                /* Remove trailing newline from command line */
                nl = strchr(cmd, '\n');
                if (nl)
                        *nl = '\0';

                /* Builtin command */
                if (!strcmp(cmd, "exit")) {
                        fprintf(stderr, "Bye...\n");
			fprintf(stderr, "+ completed '%s' [0]\n",cmd);
                        break;
                }

		if(!strcmp(cmd, "pwd")){
			pwd(cmd);
			continue;
		}
                /* Regular command */
		int errorFlag=0;
		int commandCount=1;	
		char dupCommand[CMDLINE_MAX];
		memcpy(dupCommand, cmd, sizeof(cmd));
		char** commandList=malloc(sizeof(char*)*4);	
		char* standardDupCommand = noSpace(dupCommand);
		
		for(int i=0; i<4; i++){
			commandList[i]= malloc(CMDLINE_MAX);
		}

		commandCount = commandSplit(standardDupCommand, commandList);
		free(standardDupCommand);
		if(commandCount==0){
			continue;
		}

		struct singleCommand parsedCommandList[commandCount];
		char* argList[commandCount][16];
		int pipeList[6];
		
		errorFlag=multiParser(commandCount, commandList, parsedCommandList, argList, pipeList);

		for(int i=0; i<4; i++){
			free(commandList[i]);
		}
		free(commandList);
		if(errorFlag==1){
			continue;
		}


		if(!strcmp(parsedCommandList[0].program, "cd")){
			cd(cmd, argList);
			continue;
		}	
		else{
			normalCommand(cmd, commandCount, parsedCommandList, argList, pipeList);
		}
        }

        return EXIT_SUCCESS;
}

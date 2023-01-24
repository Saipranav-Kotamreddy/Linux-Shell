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
	/*printf("Pipe Input: %d\n", cmd.pipeInput);
	printf("Pipe Output: %d\n", cmd.outputFileDescriptor);
	printf("Arg 1: %s\n", args[1]);*/
	pid = fork();
	if(pid==0){
		if(cmd.pipeInput!= STDIN_FILENO){
			dup2(cmd.pipeInput, STDIN_FILENO);
			//close(cmd.pipeInput);
		}
		if(cmd.outputFileDescriptor != STDOUT_FILENO){
			dup2(cmd.outputFileDescriptor, STDOUT_FILENO);
			//close(cmd.outputFileDescriptor);
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

int main(void)
{
        char cmd[CMDLINE_MAX];
	char cwd[CMDLINE_MAX];

        while (1) {
                char *nl;
		int status;
                //int retval;

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

                /* Regular command */
                //retval = system(cmd);
		int commandCount=1;
		
		char dupCommand[CMDLINE_MAX];
		memcpy(dupCommand, cmd, sizeof(cmd));
		char* commandList[4];	
		char* standardDupCommand = noSpace(dupCommand);
		
		if(standardDupCommand[0]=='|' || standardDupCommand[strlen(standardDupCommand)-1]=='|'){
				fprintf(stderr, "Error: missing command\n");
				continue;
		}

		char* pipeCommand=strtok(standardDupCommand, "|");
		commandList[commandCount-1]=pipeCommand;
		while((pipeCommand= strtok(NULL, "|"))!=NULL){
			commandCount++;
			commandList[commandCount-1]=pipeCommand;
		}
		struct singleCommand parsedCommandList[commandCount];
		char* argList[commandCount][16];
		int pipeList[8];
		int pipeListCount=0;
		int lastCommand=0;
		int pipeEnds[2];
		int argCount=0;
		int errorFlag=0;
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
				errorFlag=1;
				break;
			}
			
			if(argCount==-1){
				fprintf(stderr, "Error: no output file\n");
				errorFlag=1;
				break;
			}

			if(argCount==-2){
				fprintf(stderr, "Error: cannot open output file\n");
				errorFlag=1;
				break;
			}

			if(argCount==-3){
				fprintf(stderr, "Error: missing command\n");
				errorFlag=1;
				break;
			}


			for(int j=0; j<argCount; j++){
				argList[i][j]=parsedCommandList[i].arguments[j];	
			}
			argList[i][argCount]=NULL;	
			if(!lastCommand){
				if(parsedCommandList[i].outputFileDescriptor!=STDOUT_FILENO){
					fprintf(stderr, "Error: mislocated output redirection\n");
					errorFlag=1;
					break;
				}
				pipe(pipeEnds);
				parsedCommandList[i].outputFileDescriptor=pipeEnds[1];
				pipeList[pipeListCount]=pipeEnds[0];
				pipeList[pipeListCount+1]=pipeEnds[1];
				pipeListCount=pipeListCount+2;
			}
		}
		free(standardDupCommand);

		if(errorFlag==1){
			continue;
		}
		
		int statusArray[commandCount];
		if(!strcmp(parsedCommandList[0].program, "pwd")){
			getcwd(cwd, sizeof(cwd));
			fprintf(stdout, "%s\n", cwd);
			statusArray[0]=0;
			fprintf(stderr, "+ completed '%s' [0]\n",cmd);
			continue;
		}
		
		else if(!strcmp(parsedCommandList[0].program, "cd")){
			if(chdir(argList[0][1])==-1){
				fprintf(stderr, "Error: cannot cd into directory\n");
				statusArray[0]=1;
			}
			else{
				statusArray[0]=0;
			}
			fprintf(stderr, "+ completed '%s' [%d]\n",cmd, statusArray[0]);
			continue;
		}
		
		else{
			pid_t pidList[commandCount];
			int pipeStart=0;
			for(int i=0; i<commandCount; i++){
				pidList[i] = runCommand(parsedCommandList[i],argList[i], pipeList, pipeStart, pipeListCount);
				pipeStart=pipeStart+2;
			}
			for(int j=0; j<commandCount; j++){
			//for(int j=commandCount-1; j>=0; j--){
				waitpid(pidList[j], &status, 0);
				statusArray[j]=status;
				//printf("Finished command %s\n", parsedCommandList[j].program);
			}
		}
		fprintf(stderr, "+ completed '%s' ",cmd);
		for(int k=0; k<commandCount-1; k++){
			fprintf(stderr, "[%d]", WEXITSTATUS(statusArray[k]));
		}
		fprintf(stderr, "[%d]\n", WEXITSTATUS(statusArray[commandCount-1]));
        }

        return EXIT_SUCCESS;
}

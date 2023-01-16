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


pid_t runCommand(struct singleCommand cmd, char** args){
	pid_t pid;
	/*printf("Pipe Input: %d\n", cmd.pipeInput);
	printf("Pipe Output: %d\n", cmd.outputFileDescriptor);
	printf("Arg 1: %s\n", args[1]);*/
	pid = fork();
	if(pid==0){
		if(cmd.pipeInput!= STDIN_FILENO){
			dup2(cmd.pipeInput, STDIN_FILENO);
			close(cmd.pipeInput);
		}
		if(cmd.outputFileDescriptor != STDOUT_FILENO){
			dup2(cmd.outputFileDescriptor, STDOUT_FILENO);
			close(cmd.outputFileDescriptor);
		}
		execvp(cmd.program, args);
		perror("Error");
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
	int nextRedirect=0;
	//If the output descriptor is ever not -1 and the parser is still going, thats an error
	cmd->outputFileDescriptor=STDOUT_FILENO;

	while((program= strtok(NULL, " "))!=NULL){
		if(nextRedirect){
			int fd = open(program, O_WRONLY | O_CREAT| O_TRUNC, 0644);
			cmd->outputFileDescriptor=fd;
			continue;
		}
		if(!strcmp(program, ">")){
			nextRedirect=1;
			continue;
		}
		memcpy(cmd->arguments[argCount], program, sizeof(cmd->arguments[argCount]));
		argCount++;
	}
	//cmd->arguments[argCount]=NULL;
	//char* args[argCount+1];	
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
                printf("sshell$ ");
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
                        break;
                }

                /* Regular command */
                //retval = system(cmd);
		int commandCount=1;
		
		char dupCommand[CMDLINE_MAX];
		memcpy(dupCommand, cmd, sizeof(cmd));
		char* commandList[4];	
		char* standardDupCommand = noSpace(dupCommand);

		char* pipeCommand=strtok(standardDupCommand, "|");
		commandList[commandCount-1]=pipeCommand;
		while((pipeCommand= strtok(NULL, "|"))!=NULL){
			commandCount++;
			commandList[commandCount-1]=pipeCommand;;
		}
		struct singleCommand parsedCommandList[commandCount];
		char* argList[commandCount][16];
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
			for(int j=0; j<argCount; j++){
				argList[i][j]=parsedCommandList[i].arguments[j];	
			}
			argList[i][argCount]=NULL;	
			if(!lastCommand){
				pipe(pipeEnds);
				parsedCommandList[i].outputFileDescriptor=pipeEnds[1];
			}
		}
		

		int statusArray[commandCount];
		if(!strcmp(parsedCommandList[0].program, "pwd")){
			getcwd(cwd, sizeof(cwd));
			fprintf(stdout, "%s\n", cwd);
			statusArray[0]=0;
		}
		
		else if(!strcmp(parsedCommandList[0].program, "cd")){
			printf("%s\n", argList[0][1]);
			if(chdir(argList[1][1])==-1){
				fprintf(stdout, "Error in changing directory\n");
			}
			else{
				statusArray[0]=0;
			}
		}
		
		else{
			pid_t pidList[commandCount];
			for(int i=0; i<commandCount; i++){
				pidList[i] = runCommand(parsedCommandList[i],argList[i]);
			}
			for(int j=0; j<commandCount; j++){
				waitpid(pidList[j], &status, 0);
				statusArray[j]=status;
			}
		}
		fprintf(stdout, "+ completed '%s': ",cmd);
		for(int k=0; k<commandCount-1; k++){
			fprintf(stdout, "[%d]", WEXITSTATUS(statusArray[k]));
		}
		fprintf(stdout, "[%d]\n", WEXITSTATUS(statusArray[commandCount-1]));
        }

        return EXIT_SUCCESS;
}

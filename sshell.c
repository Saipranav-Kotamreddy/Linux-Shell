#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "Whitespace.h"

//Macros for all predefined limits
#define CMDLINE_MAX 512
#define ARGUMENT_MAX_COUNT 16
#define ARGUMENT_MAX_LENGTH 32
#define MAX_PIPE_COMMANDS 4

//Error Terms for all errors checked by handler
enum{
	NO_ERR,
	ERR_TOO_MANY_ARGS,
	ERR_MISS_CMD,
	ERR_NO_OUTPUT,
	ERR_OPEN_FAILED,
	ERR_MISPLACE_REDIRECT,
	ERR_MISPLACE_BACKGROUND
};

//Struct carrying all information relevant per command
struct singleCommand{
	char program[ARGUMENT_MAX_LENGTH];
	char arguments[ARGUMENT_MAX_COUNT][ARGUMENT_MAX_LENGTH];
	int pipeInput;
	int outputFileDescriptor;
};

//Running fork-wait-exec on non-built-in command
pid_t runCommand(struct singleCommand cmd, char** args, int pipeList[], int pipeStart, int pipeListCount){
	pid_t pid;
	pid = fork();
	if(pid==0){
		//Duplicating pipes to input/output if necessary, closing all pipes in this child
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
	//Closing pipes sequentially in parent
	if(cmd.pipeInput!= STDIN_FILENO){
		close(cmd.pipeInput);
	}
	if(cmd.outputFileDescriptor!= STDOUT_FILENO){
		close(cmd.outputFileDescriptor);
	}
	return pid;
}

//Splitting the full command line into sets of commands based on pipe symbol, '|'
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
//Handles all errors from parsing a single command
int errorHandler(int errorVal){
	if(errorVal==NO_ERR){
		return 0;
	}
	if(errorVal==ERR_TOO_MANY_ARGS){
		fprintf(stderr, "Error: too many process arguments\n");
		return 1;
	}

	if(errorVal==ERR_NO_OUTPUT){
		fprintf(stderr, "Error: no output file\n");
		return 1;
	}
	if(errorVal==ERR_OPEN_FAILED){
		fprintf(stderr, "Error: cannot open output file\n");
		return 1;
	}
	if(errorVal==ERR_MISS_CMD){
		fprintf(stderr, "Error: missing command\n");
		return 1;
	}
	return 1;
}

//Parses a single command
int parser(struct singleCommand* cmd, char* inputCommand, int* argCount){
	char* program = strtok(inputCommand, " ");
	memcpy(cmd->program, program, sizeof(cmd->program));
	memcpy(cmd->arguments[0], program, sizeof(cmd->arguments[0]));
	int nextRedirectNoTrunc=0;
	int nextRedirectTrunc=0;
	//Sets default output to shell output
	cmd->outputFileDescriptor=STDOUT_FILENO;

	//Half of missing command check: If no command exists, or the program starts with a redirect assume command is missing
	if(program==NULL || !strcmp(program, ">") || !strcmp(program, ">>")){
		return ERR_MISS_CMD;
	}

	while((program= strtok(NULL, " "))!=NULL){
		if(*argCount==(ARGUMENT_MAX_COUNT)){
			return ERR_TOO_MANY_ARGS;
		}
		//Set redirectFlag to 2 to register the new output location occurred
		if(nextRedirectTrunc){
			nextRedirectTrunc=2;
			int fd = open(program, O_WRONLY | O_CREAT| O_TRUNC, 0644);
			if(fd==-1){
				return ERR_OPEN_FAILED;
			}
			cmd->outputFileDescriptor=fd;
			continue;
		}
		if(nextRedirectNoTrunc){
			nextRedirectNoTrunc=2;
			int fd = open(program, O_WRONLY | O_CREAT | O_APPEND, 0644);
			if(fd==-1){
				return ERR_OPEN_FAILED;
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
		memcpy(cmd->arguments[*argCount], program, sizeof(cmd->arguments[*argCount]));
		(*argCount)++;
	}
	//If symbol to redirect exists but no arguments after, missing redirect error occurs
	if(nextRedirectTrunc==1 || nextRedirectNoTrunc==1){
		return ERR_NO_OUTPUT;
	}
	return NO_ERR;
}
//Parses the total command, sets up pipes between commands
int multiParser(int commandCount, char* commandList[], struct singleCommand parsedCommandList[],char* argList[commandCount][ARGUMENT_MAX_COUNT], int pipeList[]){
	int pipePosition=0;
	int lastCommand=0;
	int pipeEnds[2];
	int argCount=1;
	int errorVal=0;
	for(int i=0; i<commandCount; i++){
		argCount=1;
		parsedCommandList[i].pipeInput=STDIN_FILENO;
		if(i!=0){
			parsedCommandList[i].pipeInput=pipeEnds[0];
		}
		if(i==commandCount-1){
			lastCommand=1;
		}

		errorVal = parser(&parsedCommandList[i], commandList[i], &argCount);

		int errorFlag = errorHandler(errorVal);
		if(errorFlag){
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
			//Initilalizes and stores pipes
			if(!pipe(pipeEnds)){
				fprintf(stderr, "Pipe failed");
				exit(EXIT_FAILURE);	
			}
			parsedCommandList[i].outputFileDescriptor=pipeEnds[1];
			pipeList[pipePosition]=pipeEnds[0];
			pipeList[pipePosition+1]=pipeEnds[1];
			pipePosition=pipePosition+2;
		}
	}
	return 0;
}
//Built in command pwd
void pwd(char* cmd){
	char cwd[CMDLINE_MAX];
	int status;
	if(!getcwd(cwd, sizeof(cwd))){
		fprintf(stderr, "Get cwd failed");
		status=1;	
	}
	else{
		fprintf(stdout, "%s\n", cwd);
		status=0;
	}
	fprintf(stderr, "+ completed '%s' [%d]\n",cmd, status);
}
//Built in command cd
void cd(char* cmd, char* argList[1][ARGUMENT_MAX_COUNT]){
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
//Running a normal, non built-in command
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
		if(!fgets(cmd, CMDLINE_MAX, stdin)){	
			fprintf(stderr, "Error: cannot cd into directory\n");
			exit(EXIT_FAILURE);
		};

		/* Print command line if stdin is not provided by terminal */
		if (!isatty(STDIN_FILENO)) {
			printf("%s", cmd);
			fflush(stdout);
		}

		/* Remove trailing newline from command line */
		nl = strchr(cmd, '\n');
		if (nl)
			*nl = '\0';

		// Builtin commands w/o arguments
		if (!strcmp(cmd, "exit")) {
			fprintf(stderr, "Bye...\n");
			fprintf(stderr, "+ completed '%s' [0]\n",cmd);
			break;
		}

		if(!strcmp(cmd, "pwd")){
			pwd(cmd);
			continue;
		}
		// Commands with arguments
		int errorFlag=0;
		int backgroundFlag=0;
		int commandCount=1;
		//dupCommand and standardDupCommand are used to respace the command line to be parsed
		char dupCommand[CMDLINE_MAX];
		memcpy(dupCommand, cmd, sizeof(cmd));
		char** commandList=malloc(sizeof(char*)*MAX_PIPE_COMMANDS);
		char* standardDupCommand = clean(dupCommand);
		int commandLength = strlen(standardDupCommand);

		for(int pos=0; pos<commandLength; pos++){
			if(standardDupCommand[pos]=='&'){
				if(pos==commandLength-1){
					backgroundFlag=1;
					standardDupCommand[pos]='\0';
					break;
				}
				else{
					backgroundFlag=ERR_MISPLACE_BACKGROUND;
					break;
				}
			}
		}	

		if(backgroundFlag==ERR_MISPLACE_BACKGROUND){
			fprintf(stderr, "Error: mislocated background sign\n");
			continue;
		}

		for(int i=0; i<MAX_PIPE_COMMANDS; i++){
			commandList[i]= malloc(CMDLINE_MAX);
		}

		commandCount = commandSplit(standardDupCommand, commandList);
		free(standardDupCommand);
		if(commandCount==0){
			continue;
		}

		struct singleCommand parsedCommandList[commandCount];
		char* argList[commandCount][ARGUMENT_MAX_COUNT];
		int pipeList[((MAX_PIPE_COMMANDS-1)*2)];

		errorFlag=multiParser(commandCount, commandList, parsedCommandList, argList, pipeList);

		for(int i=0; i<MAX_PIPE_COMMANDS; i++){
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
			if(backgroundFlag==0){
				normalCommand(cmd, commandCount, parsedCommandList, argList, pipeList);
			}
			else{
				
				pid_t pid;
				pid = fork();
				if(pid==0){
					normalCommand(cmd, commandCount, parsedCommandList, argList, pipeList);
					exit(1);
				}
				else{
					for(int i=0; i<((commandCount-1)*2); i++){
						close(pipeList[i]);
					}
				}
			}
		}
	}
	return EXIT_SUCCESS;
}

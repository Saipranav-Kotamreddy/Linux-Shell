#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>


#define CMDLINE_MAX 512

struct singleCommand{
	char program[32];
	char arguments[16][32];
	int outputFile;
};


int runCommand(struct singleCommand cmd, char** args){
	pid_t pid;
	pid = fork();
	int status;
	if(pid==0){
		execvp(cmd.program, args);
		perror("Error");
		exit(1);
	}
	else{
		waitpid(pid, &status, 0);
	}
	return status;
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
		struct singleCommand command;
		char* program = strtok(cmd, " ");
		memcpy(command.program, program, sizeof(command.program));
		memcpy(command.arguments[0], program, sizeof(command.arguments[0]));
		int argCount=1;
		while((program= strtok(NULL, " "))!=NULL){
			memcpy(command.arguments[argCount], program, sizeof(command.arguments[argCount]));
			argCount++;
		}
		char* args[argCount+1];
		for(int i=0; i<argCount; i++){
			args[i]=command.arguments[i];	
		}
		args[argCount]=NULL;	
		
		
		if(!strcmp(command.program, "pwd")){
			getcwd(cwd, sizeof(cwd));
			fprintf(stdout, "%s\n", cwd);
			status=0;
		}
		
		else if(!strcmp(command.program, "cd")){
			printf("%s\n", args[1]);
			if(chdir(args[1])==-1){
				fprintf(stdout, "Error in changing directory\n");
			}
		}
		
		else{
			status = runCommand(command,args);
		}
		fprintf(stdout, "Return status value for '%s': %d\n",cmd, status);
        }

        return EXIT_SUCCESS;
}

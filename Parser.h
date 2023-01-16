#define CMDLINE_MAX 512

typedef struct singleCommand{
	char program[32];
	char arguments[16][32];
	int outputFile;
} Better;

Better* Parser(Better* command, int argc, char** argv[]){
    int b = -1;
    int t = 0;
    int k = 0;
    int cmd_index = 0;
    int arg_index = 0;

    for(int i = 0; i < argc; i++){
        // reset index
        if(argv[i] == "|"){
            b = i;
            t = 0;
        }
        //checks if command is valid and doesn't take arguments 
        if(strcmp(argv[b+1], "pwd") == 0 || strcmp(argv[b+1], "ls") == 0){
            command->program[cmd_index] = argv[i];
            cmd_index++;
        } 
        //checks if command is valid and takes arguments 
        if(strcmp(argv[b+1], "date") == 0) {
            command->program[cmd_index] = argv[i];
            cmd_index++;
            t=1;
        }
        //checks if expecting parameters and if index not currently set to actual cmd
        if(t == 1 && argv[i] != command->program[cmd_index-1]){
            for(int l = 0; l < strlen(argv[i]); l++){
                command->arguments[k][l] = argv[i][l];
            }
            k++;
        }

    }
    return command;
}

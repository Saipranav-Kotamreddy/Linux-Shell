#include <string.h>
#include <stdio.h>
#include <stdlib.h>



char* clean(char* str){
	char* line = malloc(1025 * sizeof(char));
	memset(line, 0, 1025);
	char whitespace[] = " \t\n\r\v";

	char* token = strtok(str, whitespace);

	while (token != NULL){
		char* partialLine = malloc(1025 * sizeof(char));
		memset(partialLine, 0, 1025);

		int flag = 0;
		int start = 0;
		int i;
		for (i = 0; token[i] != 0; ++i){
			if (token[i] == '>') {
				if (!flag) {
					strncat(partialLine, token + start, i - start);
					if (i > 0){
						strcat(partialLine, " >");
					}
					else{
						strcat(partialLine, ">");
					}
					flag = 1;
				}
				else {
					strcat(partialLine, ">");
				}
				start = i + 1;
			}
			else {
				if (flag) {
					strcat(partialLine, " ");
					start = i;
					flag = 0;
				}
			}
	 	}
		strncat(partialLine, token + start, i - start);

		if (partialLine[0] == '|') {
			int len = strlen(line);
			if (len > 0){
				line[len - 1] = 0;
			}
			strncat(line, partialLine, 1024);
		}
		else {
			strncat(line, partialLine, 1024);
			strcat(line, " ");
		}
		token = strtok(NULL, whitespace);
		free(partialLine);
	}

	int len = strlen(line);
	if (len > 0 && line[len - 1] != '|'){
		line[len - 1] = 0;
	}

	return line;
}

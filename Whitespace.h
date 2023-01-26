#include <string.h>
#include <stdio.h>
#include <stdlib.h>



char*
clean(char* str) 
{
    char* line = malloc(1025 * sizeof(char));
    memset(line, 0, 1025);
    char whitespace[] = " \t\n\r\v";

    // "echo hello > test.txt|grep hello"

    char* token = strtok(str, whitespace);
    
    while (token != NULL)
    {
        char* temp = malloc(1025 * sizeof(char));
        memset(temp, 0, 1025);

        int flag = 0;
        int start = 0;
        int i;
        for (i = 0; token[i] != 0; ++i)
        {
            if (token[i] == '>') {
                if (!flag) {
                    strncat(temp, token + start, i - start);
                    if (i > 0)
                        strcat(temp, " >");
                    else
                        strcat(temp, ">");
                    
                    flag = 1;
                } else {
                    strcat(temp, ">");
                }
            } else {
                if (flag) {
                    strcat(temp, " ");
                    start = i;
                    flag = 0;
                }
            }
        }
        strncat(temp, token + start, i - start);

        if (temp[0] == '|') {
            int n = strlen(line);
            if (n > 0)
                line[n - 1] = 0;
            
            strncat(line, temp, 1024);
        } else {
            strncat(line, temp, 1024);
            strcat(line, " ");
        }

        token = strtok(NULL, whitespace);
    }

    int n = strlen(line);
    if (n > 0)
        line[n - 1] = 0;

    return line;
}

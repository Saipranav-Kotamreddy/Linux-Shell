#include <stdio.h>
#include <string.h>

char *noSpace(char* str_1);



char *noSpace(char* str_1){
  int L = strlen(str_1);
    char* str_new = malloc(1024);
    char str_temp[1024]= "";
    int x = 0;
    int y= 0;
    for(int i = 0; i < L; i++) {
        if(str_temp[x] == ' ' && str_1[i] == ' '){
            continue;
          }
        else { 
            str_temp[x] = str_1[i];
           // printf("%c", str_temp[x]);
            str_new[y] = str_temp[x];
            y++;
          }

    }

    return str_new;
    
}

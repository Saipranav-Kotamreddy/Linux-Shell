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

    //printf("%s\n", line);
    return line;
}









//  #include <stdio.h>
//  #include <string.h>

//  char *noSpace(char* str_1);


// char *noSpace(char* str_1){
//   int L = strlen(str_1);
//   char* str_new = malloc(1024);
//   memset(str_new, '\0', 1024);
//   int x = 0;
//   int y= 0;
//   for(int i = 0; i < L; i++) {
//     // check for '>' or '>>'
//     if (str_1[i] == '>') {
//       if (i > 0 && str_1[i - 1] != ' ') {
//         str_new[y] = ' ';
//         y++;
//       }
//       str_new[y] = str_1[i];
//       y++;
//       if (i < L - 1 && str_1[i + 1] != ' ') {
//         str_new[y] = ' ';
//         y++;
//       }
//     }
//     // check for '|'
//     else if (str_1[i] == '|') {
//       str_new[y] = str_1[i];
//       y++;
//     }
//     // check for whitespace
//     else if (str_1[i] == ' ') {
//       if (y > 0 && str_new[y - 1] != ' ') {
//         str_new[y] = ' ';
//         y++;
//       }
//     }
//     // all other characters
//     else {
//       str_new[y] = str_1[i];
//       y++;
//     }
//   }
//   return str_new; 
// }









// #include <stdio.h>
// #include <string.h>

// char *noSpace(char* str_1);

// char *noSpace(char* str_1){
//   int L = strlen(str_1);
//     char* str_new = malloc(1024);
//     memset(str_new, '\0', 1024);
//     char str_temp[1024]= "";
//     int x = 0;
//     int y= 0;
//     for(int i = 0; i < L; i++) {
//         if(str_1[i] == '>' || str_1[i] == '|'){
//           if(str_1[i] == '>'){
//             if((str_1[i-1] != ' ') || (str_1[i+1] != ' ')){
//               str_temp[x] = ' ';
//               x++;
//             }
//           }
//           str_temp[x] = str_1[i];
//           x++;
//           if(str_1[i] == '>'){
//             if((str_1[i-1] != ' ') || (str_1[i+1] != ' ')){
//               str_temp[x] = ' ';
//               x++;
//             }
//           }
//         }
//         else if(str_temp[x] == ' ' && str_1[i] == ' '){
//             continue;
//           }
//         else { 
//             str_temp[x] = str_1[i];
//            // printf("%c", str_temp[x]);
//             str_new[y] = str_temp[x];
//             y++;
//           }

//     }

//     //printf("%s\n", str_new);
//     return str_new; 
// }







// #include <stdio.h>
// #include <string.h>

// char *noSpace(char* str_1);



// char *noSpace(char* str_1){
//   int L = strlen(str_1);
//     char* str_new = malloc(1024);
//     memset(str_new, '\0', 1024);
//     char str_temp[1024]= "";
//     int x = 0;
//     int y= 0;
//     for(int i = 0; i < L; i++) {
//         if(str_temp[x] == ' ' && str_1[i] == ' '){
//             continue;
//           }
//         else { 
//             str_temp[x] = str_1[i];
//            // printf("%c", str_temp[x]);
//             str_new[y] = str_temp[x];
//             y++;
//           }

//     }

//     //printf("%s\n", str_new);
//     return str_new; 
// }

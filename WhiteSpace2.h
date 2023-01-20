//Updated Whitespace function

#include <stdio.h>
#include <string.h>

char *noSpace(char* str);



char *noSpace(char* str)



int l = strlen(str);
  char *new_str = malloc(sizeof(char) * 1024);
  // char temp;
  int x = 0;

  for (int i = 0; i < l; i++) {

    if (str[i] == ' ')
      continue;

    else {
      if (str[i] != '>' && str[i] != ' ') {
        new_str[x] = str[i];
        x++;
        continue;
      } else {
        // add a space at the start and at least 1 '>'
        new_str[x] = ' ';
        x++;
        new_str[x] = '>';
        x++;
        if (str[i + 1] != '>') { // if only 1 > print
          new_str[x] = ' ';
          x++;
          continue;
        }
        new_str[x] = '>';
        x++;
        new_str[x] = ' ';
        x++;
        i += 2;
      }
    }
  }

  printf("'%s'", new_str);
  return 0;
}

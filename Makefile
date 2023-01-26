all: sshell

sshell: sshell.c Whitespace.h
	gcc -Wall -Werror -Wextra -g -o sshell sshell.c

clean:
	rm -f sshell

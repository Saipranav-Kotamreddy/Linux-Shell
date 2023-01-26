all: sshell

sshell: sshell.c Whitespace.h
	gcc -Wall -Werror -Wextra -O2 -o sshell sshell.c

clean:
	rm -f sshell

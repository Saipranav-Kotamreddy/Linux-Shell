all: sshell

sshell: sshell.c
	gcc -Wall -Werror -Wextra -g -o sshell sshell.c

clean:
	rm -f sshell

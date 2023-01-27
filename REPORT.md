# SSHELL: Simple Shell

## Summary

The sshell program is a command-line interface that accepts various inputs 
from the user. These inputs include commands, arguments, redirects and 
command pipelines. The program then executes these inputs as jobs.

## Implementation

The implementation of `sshell` can be broken down into 2 steps:

1. Parsing the command line for what needs to be run
2. Executing the command line job

### Parsing Method

The first thing the program does is take in the command line from the user.
This command line is read using fgets and stored as a string. The first
thing checked is if the user input matches the exit or pwd builtin commands,
as neither can have arguments, so they would make up the whole command line.

If the command line matches neither keyword, the string is standardized to
have specific spacing using a helper function called `clean()`. The function
`clean()` removes unnecessary whitespace from the user's input command line. 
First, the function allocates memory for a new line string and sets it to 
all null using `memset`. This string then goes through the original
command and copies it, except changes multiple spaces between arguments
to just one space and puts spaces around '>' and '>>', while removing
spaces aroudn the '|' character. This is done to assist later parsingh
Tere is an edgecase where `clean` fails; if the command line is the 
max length, but the '>' or '>>' symbols have no spaces around them, then 
command line overflows. 

This cleaned string is now parsed for the background job symbol; if 
it is at the end then the background job flag is set. However, if 
it is in a different position, then an error is returned.

Next, the parser splits the command line based on '|' to get each individual
command in the pipeline separately. This split also checks to make sure there
is not a missing command before or after a pipe symbol, which is an error. The
individual commands are stored in an array, while their count is stored in an
integer variable. These are then passed to another parser, `multiParser()`,
which handles pipe file descriptors, as well as storing each commands arguments
in an array of `char**`. This is due to `execvp()` not accepting the 
`char[ARGUMENT_MAX_COUNT][ARGUMENT_MAX_LENGTH]` array, so it was converted to
be usable to `execvp()`. `multiparser()` then calls `parser()` on each command.

`parser()` breaks down each command into the program and arguments.
A struct type called singleCommand stores all necessary info per command.
The first argument is taken as the program name and set to the program
field in the struct, as well as the first argument. The parser then collects
all the arguments by delimiting on whitespace. If a 17th argument is read in 
a single command, an error for too many arguments is returned. `parser()`
also handles file redirects, changing how the file is opened/written to
based on if the redirect is '>' or '>>'. If either symbol is encountered,
a flag is set that the next token is the file to rediret to. If there is
no token after either symbol, an error is returned for missing file. An
error is also returned if the file corresponding to the token is not openable.
This file's filedescriptor is stored in singleCommand's output variable.
Finally, if the command is missing a first argument or starts with a redirect,
a missing command error is returned. Otherwise, `NO_ERR` is returned.

Any error returned by parser is checked via an error handler function, which
if an error is found, prints the correct message and terminates the parsing.
It then displays the shell input again. The `multiparser()` loops through each
command split based on '|' and if a command is not the last command, sets
that command's output to the pipe write end and copies it to the singleCommand
struct's output variable. Any command that is not the first command has its
input set to the read end of the pipe created in the last command. If a
command tries to redirect to both a file and a pipe output, a mislocated
output redirection error is returned. The pipe file descriptors are also
stored into an array called pipeList. Assuming that no errors occurred, the
program now has an array of singleCommands with all the information necessary
to execute each program, the arguments of each program, and a list of pipes.

### Command Execution

The first thing checked after parsing is if the command is 'cd', as that
is run as a builtin command using the `chdir` library function. Otherwise,
the program checks if `backgroundFlag` was set or not. If it was set, then
the program execution is run in a child so the parent can continue to accept
commands from the user. Otherwise the program runs in the parent. The reason
the size of the argList is hard coded to 1 in cd is it can only accept one
argument, which is the path to the new directory. If the directory
change fails, an error is returned.

Normal commands are run in `normalCommand`, which calls `runCommand()` on each
command. `runCommand()`  only contains `fork()` and `execvp()`, as if 
`wait()` was included, the pipes would be unable to run concurrently. Instead 
the pids are stored in an array, where after every command is run, the 
program loops until every child process formed by the `fork()` and `execvp()` 
terminates. 

In the parent side of `runCommand()`, the file descriptors corresponding to the 
input and output of that command are closed if they do not point to the shell, 
in order to ensure the parent does not have any remaining connections to pipes
by the end. In each child, if the input or output do not make to `STDIN` or 
`STDOUT` respectively, the correct input and output are `dup2()` onto those 
positions in the fd table. The child then closes every pipe connection that 
was cloned when the child was made to prevent connections to later pipes, using
pipeList and a start point to only close unclosed pipe fds. 

Since all connections to each pipe besides the commands whose input and output
are the pipe were closed, if the reader process finishes, it closes its read 
pipe end, which causes the writer process to also terminate allowing for all
processes in the pipeline to close.

The command is then run using `execvp()`, passing the program and arguments.
If the program does not exist, an error is returned. In both cases, the child
terminates normally, returning a '1' status in case of command not found.
This status is grabbed using waitpid and stored into a status array, which
will be displayed in the completion message. This completion message is
displayed after all child processes terminate. The message uses the original
command line given, which is why all parsing and standardization was done on a
copy of the command.

If the command is run without `backgroundFlag` set, it then displays the
shell input once again. In the case of if a background job is running, exit
checks if any children of the parent exist, as the children are the background
jobs, so if any exist then a background job is running. The function to check
if any children exist using `waitpid()` was gotten from stack overflow and cited
within the code above the function call. This check was done instead of
managing active children pids in order to avoid tracking completion and
needing information from the child termination to the parents. That is why all
outputs and errors are done in the child. This does cause a slight issue where
the completion message can show up next to the shell input, but the user can 
still input into the shell, after which the display will fix itself for the next
command.

### Memory Management

The only times dynamic memory allocation is used is for the standardization of
the string, as it is done in a function, and to store the command list. In both
cases, the variable becomes unnecessary after parsing through it, so it
is free'd after it is parsed through, resulting in no memory leaks. The amount
of memory allocated is based on the maximum command line length and number
of piped commands defined in the specifications and stored in macros.

All other variables have their memory allocated by the OS, so they clear
automatically.

### Makefile

The makefile checks if the C file or Whitespace header function were updated,
as those are the only 2 files used to compile the code. The makefile creates
no intermediary object files, as there is only one C file, and compiles
only with no errors or warnings.

## Testing

The code was tested on all inputs provided on the project specifications,
with additional tests mixing functionality, such as piping and redirecting
together. These cases were ran on both our `sshell` and `sshell_ref`
and compared using `diff` to analyze discrepancies and correct for them.













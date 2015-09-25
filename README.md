# custom-shell
An implementation of custom unix-like shell in C with capabilities to execute commands, piping, IO redirection, executing scripts and handling of background execution of processes.

To build and execute:
Type "make" to build and execute the shell.

Example commands:
1. Type ls to check if the list of files/dir's are correctly displayed in the current directory.
2. Type ls | wc or similar command to check piping (Multi-piping of commands is not supported).
3. < <file-name> wc or wc < <file-name> for input redirection.
4. ls > <file-name> for output redirection.
5. ls & for starting a process in the background.
6. ./script.myshell to execute the script in the custom shell

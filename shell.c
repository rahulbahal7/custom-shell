/* See Chapter 5 of Advanced UNIX Programming:  http://www.basepath.com/aup/
 *   for further related examples of systems programming.  (That home page
 *   has pointers to download this chapter free.
 *
 * Copyright (c) Gene Cooperman, 2006; May be freely copied as long as this
 *   copyright notice remains.  There is no warranty.
 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>


#define MAXLINE 200  /* This is how we declare constants in C */
#define MAXARGS 20
#define STDIN 0
#define STDOUT 1

bool is_pipe = false;
bool is_redirect = false;
bool background = false;

/* In C, "static" means not visible outside of file.  This is different
 * from the usage of "static" in Java.
 * Note that end_ptr is an output parameter.
 */
static char * getword(char * begin, char **end_ptr) {
    char * end = begin;
    while ( *begin == ' ' )
        begin++;  /* Get rid of leading spaces. */
    end = begin;
    while ( *end != '\0' && *end != '\n' && *end != ' ')
      end++;  /* Keep going. */
    if ( end == begin)
        return NULL;  /* if no more words, return NULL */
    *end = '\0';  /* else put string terminator at end of this word. */
    *end_ptr = end;
    if (begin[0] == '$') { /* if this is a variable to be expanded */
        begin = getenv(begin+1); /* begin+1, to skip past '$' */
	if (begin == NULL) {
	    perror("getenv");
	    begin = "UNDEFINED";
        }
    }
    return begin; /* This word is now a null-terminated string.  return it. */
}

/* In C, "int" is used instead of "bool", and "0" means false, any
 * non-zero number (traditionally "1") means true.
 */
/* argc is _count_ of args (*argcp == argc); argv is array of arg _values_*/
static void getargs(char cmd[], int *argcp, char *argv[]) {
    char *cmdp = cmd;
    char *end;
    int i = 0;

    /* fgets creates null-terminated string. stdin is pre-defined C constant
     *   for standard intput.  feof(stdin) tests for file:end-of-file.
     */
    if (fgets(cmd, MAXLINE, stdin) == NULL && feof(stdin)) {
      printf("Couldn't read from standard input. End of file? Exiting ...\n");
        exit(1);  /* any non-zero value for exit means failure. */
    }
    // BUG 1: Fixed Comments in commands
    while ((cmdp = getword(cmdp, &end)) != NULL && *cmdp != '#') {
      if (*cmdp == '|') is_pipe = true;
      
      if (*cmdp == '<' || *cmdp == '>') is_redirect = true;
      //if (*cmdp == '&') background = true;
      
      argv[i++] = cmdp;
      cmdp = end + 1;
      /* end is output param */
      /* getword converts word into null-terminated string */
      /* "end" brings us only to the '\0' at end of string */
    }
    argv[i] = NULL; /* Create additional null word at end for safety. */
    *argcp = i;
}

// Parse arguments for piping
static void parse_pipe(char *left[], char *right[], char *argv[]) {
  int a, b;
  for (a= 0; *argv[a] != '|'; a++) left[a]= argv[a];
  left[a++] = '\0';
  for (b =0; *argv[a] != '|' , argv[a] != NULL; a++, b++) right[b]= argv[a];
  right[b] = '\0';
}

// To handle piping
int handle_pipe(char *argv[]) {
    int pipe_fd[2];       /* 'man pipe' says its arg is this type */
    int fd;               /* 'man dup' says its arg is this type */
    pid_t child1, child2; /* 'man fork' says it returns type 'pid_t' */
    char *left[MAXARGS], *right[MAXARGS];
    parse_pipe(left, right, argv);
    fflush(stdout);  /* Force printing to complete, before continuing. */
    if ( -1 == pipe(pipe_fd) ) perror("pipe");
    child1 = fork();
    /* child1 > 0 implies that we're still the parent. */
    if (child1 > 0) child2 = fork();
    if (child1 == 0) { /* if we are child1, do:  "ls | ..." */
      if ( -1 == close(STDOUT) ) perror("close");  /* close  */
        fd = dup(pipe_fd[1]); /* set up empty STDOUT to be pipe_fd[1] */
        if ( -1 == fd ) perror("dup");
        if ( fd != STDOUT ) fprintf(stderr, "Pipe output not at STDOUT.\n");
        close(pipe_fd[0]); /* never used by child1 */
        close(pipe_fd[1]); /* not needed any more */
        if ( -1 == execvp(left[0], left) ) perror("execvp");
    } else if (child2 == 0) { /* if we are child2, do:  "... | wc" */
        if ( -1 == close(STDIN) ) perror("close");  /* close  */
        fd = dup(pipe_fd[0]); /* set up empty STDIN to be pipe_fd[0] */
        if ( -1 == fd ) perror("dup");
        if ( fd != STDIN ) fprintf(stderr, "Pipe input not at STDIN.\n");
        close(pipe_fd[0]); /* not needed any more */
        close(pipe_fd[1]); /* never used by child2 */
        if ( -1 == execvp(right[0], right) ) perror("execvp");
    } else { /* else we're parent */
        int status;
        /* Close parent copy of pipes;
         * In particular, if pipe_fd[1] not closed, child2 will hang
         *   forever waiting since parent could also write to pipe_fd[1]
         */
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        if ( -1 == waitpid(child1, &status, 0) ) perror("waitpid");
        /* Optionally, check return status.  This is what main() returns. */
        if (WIFEXITED(status) == 0)
            printf("child1 returned w/ error code %d\n", WEXITSTATUS(status));
        if ( -1 == waitpid(child2, &status, 0) ) perror("waitpid");
        /* Optionally, check return status.  This is what main() returns. */
        if (WIFEXITED(status) == 0)
            printf("child2 returned w/ error code %d\n", WEXITSTATUS(status));
    }
    return 0;  /* returning 0 from main() means success. */
}


// Method for input-output redirection
static void execute_redirect(char *argv[])
{
	char *command[MAXARGS];
	int i, count=0, tokens=0, input, output;
	for(i=0; argv[i] != NULL; i++){
		if(*argv[i] == '<' || *argv[i] == '>')
			i++;
		else{
			command[tokens]=argv[i];
			tokens++;
		}
	}
	command[tokens]=NULL;

	while(argv[count]!=NULL){
	if(*argv[count] == '<'){
		input = open(argv[count+1], O_RDONLY);
		pid_t pid = fork();
		if(pid == 0){
			close(STDIN_FILENO);
			dup2(input, 0);
			execvp(command[0], command);
		}
		wait(NULL);
	}
	else if(*argv[count] == '>'){
		output = open(argv[count+1], O_WRONLY | O_CREAT , S_IRUSR | S_IWUSR);
		if(output == -1)
			perror("open for reading");
		pid_t pid = fork();
		if(pid == 0){
			close(STDOUT_FILENO);
			dup2(output,1);
			execvp(command[0], command);
		}
	}
	wait(NULL);
	count++;
	}
}

// Method to execute process in the background
static void execute_background(int argc, char *argv[]) {
	pid_t chilpid;
	chilpid = fork();
	argv[argc-1]=NULL;
	if(chilpid == 0){
		if(-1 == execvp(argv[0], argv)){
			perror("execvp");
			printf("  (could'nt find command)\n");
		}
	}
	return;
}

// Main execute method
static void execute(int argc, char *argv[]) {
  if (is_pipe) {
    handle_pipe(argv);
    is_pipe =false;
  } 

    else if(is_redirect){
    execute_redirect(argv);
    is_redirect=false;
    }

else {
    pid_t childpid; /* child process ID */
    childpid = fork();
    

    if (childpid == -1) { /* in parent (returned error) */
      perror("fork"); /* perror => print error string of last system call */
        printf("  (failed to execute command)\n");
    }
    
    if (childpid == 0) { /* child:  in child, childpid was set to 0 */
        /* Executes command in argv[0];  It searches for that file in
	 *  the directories specified by the environment variable PATH.
         */
      if (-1 == execvp(argv[0], argv)) {
          perror("execvp");
          printf("  (couldn't find command)\n");
        }
	/* NOT REACHED unless error occurred */
        exit(1);
    } else /* parent:  in parent, childpid was set to pid of child process */
        waitpid(childpid, NULL, 0);  /* wait until child process finishes */
  }
  return;
}



// BUG 3: Return and end the child process when user presses Ctrl-C
void handle_signal(int signum) {
  return;
}

int main(int argc, char *argv[]) {
  // BUG 3: call handle_signal when SIGINT(Ctrl-C) is received
  signal(SIGINT, handle_signal);
  char cmd[MAXLINE];
  char *childargv[MAXARGS];
  int childargc;
  /*
    BUG 2: Fixed bug to run scripts from command line
    Redirected STDIN stream to the script
    file provided as command line arguement
  */
  if (argc > 1) {
    fclose(stdin);
    stdin = fopen(argv[1], "r");
  }
  while (1) {
    // When executing script files no need to print this
    if (argc < 2)
      printf("%% "); /* printf uses %d, %s, %x, etc.  See 'man 3 printf' */
    fflush(stdout); /* flush from output buffer to terminal itself */
    getargs(cmd, &childargc, childargv);
    /* childargc and childargv are
       output args; on input they have garbage, but getargs sets them. */
    /* Check first for built-in commands. */

    // To handle background process
    if(childargc > 0 && *childargv[childargc-1] == '&'){
	execute_background(childargc, childargv);
    }
    else if ( childargc > 0 && strcmp(childargv[0], "exit") == 0 )
      exit(0);
    else if ( childargc > 0 && strcmp(childargv[0], "logout") == 0 )
      exit(0);
    else
      execute(childargc, childargv);
  }
  /* NOT REACHED */
}

/*
 * SP project1_phase1
 * myshell.c
 * Purpose: Build a structrue of myshell program that prints prompt, gets a command line from user, and excute it.
 * @version 1.0 2022/04/14
 * @author Wonbin Choi
*/

/* $begin shellmain */
#include "myshell.h"
#include<errno.h>
#define MAXARGS   128

/* Function prototypes */
void eval(char *cmdline); //evaluate comand line
int parseline(char *buf, char **argv); //pasre command line and divide it by the rule that user made. 
int builtin_command(char **argv); // check command is builtin_command.

int main() 
{
    char cmdline[MAXLINE]; /* Command line */

    while (1) {
	/* Read */
	printf("CSE4100-SP-P#1-myshell>> ");//print prompt                   
	fgets(cmdline, MAXLINE, stdin); //get command line
	if (feof(stdin))
	    exit(0);

	/* Evaluate */
	eval(cmdline);//evaluate command line
    } 
}
/* $end shellmain */
  
/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline) 
{
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */
    
    strcpy(buf, cmdline);
    bg = parseline(buf, argv); //parse command line and check if execute commnad in background
    if (argv[0] == NULL)  
		return;   /* Ignore empty lines */
    if (!builtin_command(argv)) { //quit -> exit(0), & -> ignore, cd->process by chdir other -> run
		if ((pid = Fork()) == 0) {//in child
        	if (execvp(argv[0], argv) < 0) {	//
            	printf("%s: Command not found.\n", argv[0]);
            	exit(0);
			}
        }

	/* Parent waits for foreground job to terminate */
	if (!bg){ 
	    int status;
		if (waitpid(pid, &status, 0) < 0)//wait until child process terminated
			unix_error("waitfg: waitpid error");
	}
	else//when there is backgrount process!
	    printf("%d %s", pid, cmdline);
    }
    return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv) 
{
    if (!strcmp(argv[0], "exit")) /* quit command */
		exit(0);  
    if (!strcmp(argv[0], "&"))    /* Ignore singleton & */
		return 1;
	if (!strcmp(argv[0], "cd")) {/* chanege directory */
		chdir(argv[1]);
		return 1;
	}
    return 0;                     /* Not a builtin command */
}
/* $end eval */

/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
int parseline(char *buf, char **argv) 
{
    char *delim;         /* Points to first space delimiter */
    int argc;            /* Number of args */
    int bg;              /* Background job? */
    char* delim2;		/* used to check " and ' */

    buf[strlen(buf)-1] = ' ';  /* Replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
		buf++;

    /* Build the argv list */
    argc = 0;
	while ((delim = strchr(buf, ' '))) {
		argv[argc++] = buf;
		*delim = '\0';
		buf = delim + 1;

		while (*buf) {
			while (*buf == ' ') /* Ignore spaces */
				buf++;
			if (*buf == '\'') {/* if ' -> search another ' */
				argv[argc++] = buf + 1;
				delim2 = strchr(buf + 1, '\'');
				*delim2 = '\0';
				buf = delim2 + 1;
			}
			else if (*buf == '\"') {/* if " -> search another " */
				argv[argc++] = buf + 1;
				delim2 = strchr(buf + 1, '\"');
				*delim2 = '\0';
				buf = delim2 + 1;
			}
			else {
				break;
			}
		}
	}

    argv[argc] = NULL;
    
    if (argc == 0)  /* Ignore blank line */
		return 1;

    /* Should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0)
		argv[--argc] = NULL;

    return bg;
}
/* $end parseline */



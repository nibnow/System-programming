/*
 * SP project1_phase2
 * myshell.c
 * Purpose: Update myshell to Start by creating a new process for each command in the pipeline and making the parent wait for the last command.
 * @version 1.0 2022/04/14
 * @author Wonbin Choi
*/

/* $begin shellmain */
#include "myshell.h"
#include<errno.h>
#define MAXARGS   128

/* Function prototypes */
void eval(char* cmdline); //evaluate comand line
int parseline(char* buf, char** argv); //pasre command line and divide it by the rule that user made. 
int builtin_command(char** argv); // check command is builtin_command.

void pipe_eval(char *cmdline); //if command line has | -> evaluate by this function.
void close_all_pipe(); // close all pipe.

int pipe_num = 0;
int fd[2];//for pipe
int fd2;//to store fd in parent from child to send it again next child input
int pipe_last = 0;//if last command(divided by |)

int main() 
{
    char cmdline[MAXLINE]; /* Command line */

    while (1) {
	/* Read */
	printf("CSE4110-SP-P#1-myshell>> ");//print prompt  
	fgets(cmdline, MAXLINE, stdin); //get command line
	if (feof(stdin))
	    exit(0);

	if (strchr(cmdline, '|')) {
		pipe_num = 1;
		pipe_eval(cmdline);
	}	
	/* Evaluate */
	else eval(cmdline);//evaluate command line
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
    if (!builtin_command(argv)) { //quit -> exit(0), & -> ignore, cd -> process by chdir other -> run
		if ((pid = Fork()) == 0) {
			if (pipe_num && !pipe_last)//command with pipe, but no last command
			{
				dup2(fd[1], 1);//link fd[1] and STDOUT_FILENO
			}
			if (pipe_num > 1)//command is not first. this command input is from previous command
			{
				dup2(fd2, 0);//link fd2 and STDIN_FILENO
			}
			close_all_pipe;
        	if (execvp(argv[0], argv) < 0) {
           		printf("%s: Command not found.\n", argv[0]);
            	exit(0);
			}
		}

	/* Parent waits for foreground job to terminate */
	if (!bg){ 
	    int status;
		if (waitpid(pid, &status, 0) < 0)
			unix_error("waitfg: waitpid error");
		if (pipe_num && !pipe_last)//pipe and no last
		{
			fd2 = dup(fd[0]);//store fd from child
			close(fd[1]);
			close(fd[0]);
		}
		if (pipe_last)//if last
		{
			close_all_pipe();
		}
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
	
	buf[(strlen(buf)-1)] = ' ';	/* Replace trailing '\n' with space */
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

void pipe_eval(char *cmdline)
{
	char *part_cmd;//커맨드를 자른 부분
	char *temp_cmd;//커맨드 위치를 잠시 저장
	char *pipe_cmd = (char*)malloc(20*sizeof(char));//커맨드 자른 부분에 " "을 더해줘서 넘길 것.

	pipe(fd);//pipe 생성

	if (temp_cmd = strchr(cmdline, '|')) {//파이프가 존재
		part_cmd = strtok(cmdline, "|");//파이프 단위로 자름
		strcat(pipe_cmd, part_cmd);
		strcat(pipe_cmd, " ");
		cmdline = temp_cmd + 1;
	}
	else {//마지막인 경우
		pipe_last = 1;
		eval(cmdline);

		pipe_num = 0;
		pipe_last = 0;
		free(pipe_cmd);
		return;
	}

	eval(pipe_cmd);

	pipe_num++;

	free(pipe_cmd);

	pipe_eval(cmdline);
}

void close_all_pipe()
{
	close(fd[0]);
	close(fd[1]);
	close(fd2);
}

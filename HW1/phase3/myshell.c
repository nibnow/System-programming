/*
 * SP project1_phase3
 * myshell.c
 * Purpose: Update myshell to be enable to process background job.
 * @version 1.0 2022/04/14
 * @author Wonbin Choi
*/

/* $begin shellmain */
#include "myshell.h"
#include <errno.h>
#define MAXARGS   128

/* Function prototypes */
void eval(char* cmdline); //evaluate comand line
int parseline(char* buf, char** argv); //pasre command line and divide it by the rule that user made. 
int builtin_command(char** argv); // check command is builtin_command.

void pipe_eval(char* cmdline); //if command line has | -> evaluate by this function.
void close_all_pipe(); // close all pipe.

int pipe_num = 0;
int fd[2];//for pipe
int fd2;//to store fd in parent from child to send it again next child input
int pipe_last = 0;//if last command(divided by |)

void sigint_handler(int sig);//각 signal들 처리를 위한 handler
void sigtstp_handler(int sig);
void sigchld_handler(int sig);

int job_num= 0;
typedef struct job *job_pointer;
typedef struct job {//background running 혹은 stopped된 job들을 나타내기 위한 구조체
	int job_ID;
	int job_pid;
	int job_pgid;
	char job_state[20];
	char job_name[100];
	job_pointer next;
} job;
int job_index = 0;
job_pointer job_list[100];//job들 관리를 위한 자료구조

int fg_pgid = 0;

void delete_job(int pid);//joblist에서 삭제
void add_job(int pid, char *cmdline);//job list에 추가

int main() 
{
    char cmdline[MAXLINE]; /* Command line */

	job_list[0] = (job_pointer)malloc(sizeof(job));//job list 처음 생성(헤더 역할)
	job_list[0]->next = NULL;
	job_index = 1;

	pid_t pid;
	int is_bg = 0;//백그라운드 여부
	char *part_cmdline;//command line 일부 저장

	if (signal(SIGINT, sigint_handler) == SIG_ERR)
		unix_error("sigint error\n");
	if (signal(SIGTSTP,sigtstp_handler) == SIG_ERR)
		unix_error("sigtstp error\n");
	if (signal(SIGCHLD, sigchld_handler))
		unix_error("sigchld error\n");

    while (1) {
		/* Read */
		printf("CSE4110-SP-P#1-myshell %d>> ", getpid());//print prompt 
		fgets(cmdline, MAXLINE, stdin);
		if (feof(stdin))
		    exit(0);

		is_bg = 0;
		cmdline[strlen(cmdline)-1] = ' ';// \n -> " "
		if (strchr(cmdline, '&'))	//& 있는지 확인
			is_bg = 1;
		if (is_bg) {
			part_cmdline = strtok(cmdline, "&");//있으면 잘라줌
			strcat(part_cmdline, " ");
		}

	    char *argv[MAXARGS]; /* Argument list execve() */
	    char buf[MAXLINE];   /* Holds modified command line */
		int bg;
 	    strcpy(buf, cmdline);
	    bg = parseline(buf, argv);

		if (!builtin_command(argv)) {//builtin 여부 확인
			if ((pid = Fork()) == 0) {
				setpgid(0, 0);//그룹아이디 설정, 파이프인지 백그라운드인지에 따라 4가지
				if (strchr(cmdline, '|')) {
					pipe_num = 1;
					if (!is_bg) {
						fg_pgid = getpgrp();
						pipe_eval(cmdline); 
					}
					else{
						pipe_eval(part_cmdline);
					}
				}	
				/* Evaluate */
				else {
					if (!is_bg) {
						fg_pgid = getpgrp();
						eval(cmdline);
					}
					else {
						eval(part_cmdline);
					}
				}
				exit(0);
    		}
			if (!is_bg) {		
		  	  int status;
				setpgid(pid, 0);//자식의 그룹아이디를 자식으로
				fg_pgid = pid;
				if (waitpid(pid, &status, WUNTRACED) < 0) {//멈추거나 종료 기다려줌
					unix_error("waitfg: waitpid error");
				}
				if (WIFSTOPPED(status)) {//멈춘거면 job_list에 추가
						add_job(pid, cmdline);
				}
				fg_pgid = 0;
			}
			else {//백그라운드면 job_list에 추가
				job_pointer cur;
				cur = job_list[0];
				while (cur->next != NULL)
					cur = cur->next;
				job_list[job_index] = (job_pointer)malloc(sizeof(job));
				job_list[job_index]->job_ID = job_index;
				job_list[job_index]->job_pid = pid;		
				setpgid(pid, 0);

				job_list[job_index]->job_pgid = pid;
				strcpy(job_list[job_index]->job_state, "Running");
				strcpy(job_list[job_index]->job_name, cmdline);
				job_list[job_index]->next = NULL;

				cur->next = job_list[job_index++];

				job_num++;

		  	 	printf("%d %s\n", pid, cmdline);
			}
		}
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
    if (!builtin_command(argv)) { //quit -> exit(0), & -> ignore, cd -> process by chdir other -> run; except job command
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
        	if (execvp(argv[0], argv) < 0) {	//ex) /bin/ls ls -al &
           		printf("%s: Command not found.\n", argv[0]);
            	exit(0);
			}
		}
	/* Parent waits for foreground job to terminate */
		if (!bg){ 
		    int status;
			pid_t pid_returned;
			while ((pid_returned = waitpid(pid, &status, WUNTRACED)) > 0) {//정지 종료를 기다림
				if (WIFEXITED(status) || WIFSIGNALED(status)) {//종료시 탈출, 정지시 다시 실행하면 다시 기다림
					break;
				}
			}
			if (pipe_num && !pipe_last)
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
		{
		}
    }
    return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv) 
{
    if (argv[0] == NULL) {//그냥 엔터쳤을때
		return 2;
	}
    if (!strcmp(argv[0], "exit")) /* quit command */
		exit(0);  
    if (!strcmp(argv[0], "&"))    /* Ignore singleton & */
		return 1;
	if (!strcmp(argv[0], "cd")) {/* chanege directory */
		chdir(argv[1]);
		return 2;
	}
	if (!strcmp(argv[0], "jobs")) {//job_list 출력
		job_pointer cur;
		cur = job_list[0]->next;
		while (cur != NULL)
		{
			printf("[%d] %s %s\n", cur->job_ID, cur->job_state, cur->job_name, cur->job_pid);
			cur = cur->next;
		}
		return 2;
	}
	if (!strcmp(argv[0], "bg")) {//stopped -> background running
		job_pointer cur;
		cur = job_list[0]->next;
		int pid = cur->job_pid;
		if (argv[1][0] == '%') {
			int jid = atoi(&argv[1][1]);
			while (cur->job_ID != jid) {
				cur = cur->next;
				if (!cur)
					break;
				pid = cur->job_pid;
			}
		}
		else {	
			pid = atoi(argv[1]);
			while (cur->job_pid != pid) {
				cur = cur->next;
				if (!cur)
					break;
			}
		}
		if (cur) {
			strcpy(cur->job_state, "Running");
			printf("[%d] %s %s\n", cur->job_ID, cur->job_state, cur->job_name);
			kill(-1*pid, SIGCONT);
		}
		else
			printf("No such job\n");
		
		return 2;
	}
	if (!strcmp(argv[0], "fg")) {//stopped, background running -> foreground running
		job_pointer cur;
		cur = job_list[0]->next;
		int pid = cur->job_pid;
		int status;
		if (argv[1][0] == '%') {
			int jid = atoi(&argv[1][1]);
			while (cur->job_ID != jid) {
				cur = cur->next;
				if (!cur)
					break;
				pid = cur->job_pid;
			}
		}
		else {	
			pid = atoi(argv[1]);
			while (cur->job_pid != pid) {
				cur = cur->next;
				if (!cur)
					break;
			}
		}
		if (cur) {
			if (!strcmp(cur->job_state, "Suspended")) {
				strcpy(cur->job_state, "Running");
				kill(-1*pid, SIGCONT);
			}
			fg_pgid = cur->job_pgid;;
			printf("[%d] %s %s\n", cur->job_ID, cur->job_state, cur->job_name);
			delete_job(pid);
			if ((waitpid(pid, &status, WUNTRACED)) < 0){
				unix_error("waitfg: waitpid error");
			}
			if (WIFSTOPPED(status)) {
				add_job(pid, cur->job_name);
			}
			fg_pgid = 0;
		}
		else
			printf("No such job\n");
		return 2;
	}
	if (!strcmp(argv[0], "kill")) {
		job_pointer cur, pre;
		cur = job_list[0]->next;
		pre = job_list[0];
		int pid;
		if (argv[1][0] == '%') {
			int jid = atoi(&argv[1][1]);
			pid = cur->job_pid;
			while (cur->job_ID != jid) {
				cur = cur->next;
				pre = pre->next;
				if (!cur)
					break;
				pid = cur->job_pid;
			}
		}
		else {
			pid = atoi(argv[1]);
			while (cur->job_pid != pid) {
				cur = cur->next;
				pre = pre->next;
				if (!cur)
					break;
			}
		}
		if (cur) {
			kill((-1)*pid, SIGKILL);
		}
		else
			printf("No such job\n");

		return 2;
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
	char *delim2;		/* used to check " and ' */
	
	buf[(strlen(buf)-1)] = ' ';
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
				delim2 = strchr(buf+1, '\'');
				*delim2 = '\0';
				buf = delim2 + 1;
			}
			else if (*buf == '\"') {/* if " -> search another " */
				argv[argc++] = buf + 1;
				delim2 = strchr(buf+1, '\"');
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
//	free(pipe_cmd);
}

void close_all_pipe()
{
	close(fd[0]);
	close(fd[1]);
	close(fd2);
}

void sigint_handler (int sig)
{
	int olderrno = errno;

	Sio_puts("\n");
	
	if (fg_pgid)
		kill((-1*fg_pgid), SIGKILL);

	errno = olderrno;
	
	return;
}

void sigtstp_handler (int sig)
{
	int olderrno = errno;

	Sio_puts("\n");
	
	if (fg_pgid) {
		kill((-1*fg_pgid), SIGSTOP);
	}

	errno = olderrno;

	return;
}

void sigchld_handler (int sig)
{
	int olderrno = errno;
	int status;
	sigset_t mask_all, prev_one;
	pid_t pid;

	sigfillset(&mask_all);
	while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
		if (WIFSTOPPED(status)) {
		}
		else {
			sigprocmask(SIG_BLOCK, &mask_all, &prev_one);
			delete_job(pid);
			sigprocmask(SIG_SETMASK, &prev_one, NULL);
		}
	}
	if (errno != ECHILD && errno != 0) {
		Sio_error("waitpid error\n");
	}
	errno = olderrno;
}

void delete_job(int pid)
{
	job_pointer cur, pre;
	cur = job_list[0]->next;
	pre = job_list[0];

	while (cur->job_pid != pid) {
		cur = cur->next;
		pre = pre->next;
		if (!cur)
			break;
	}

	if (cur) {
		pre->next = cur->next;
		free(cur);
	}
}

void add_job(int pid, char *cmdline)
{
	job_pointer cur;
	cur = job_list[0];
	while (cur->next != NULL)
		cur = cur->next;
	job_list[job_index] = (job_pointer)malloc(sizeof(job));	
	job_list[job_index]->job_ID = job_index;
	job_list[job_index]->job_pid = pid;

	job_list[job_index]->job_pgid = pid;
	strcpy(job_list[job_index]->job_state, "Suspended");
	strcpy(job_list[job_index]->job_name, cmdline);
	job_list[job_index]->next = NULL;

	cur->next = job_list[job_index++];

	job_num++;
	printf("%d %s", pid, cmdline);
}

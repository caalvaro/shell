#ifndef SHELL_H_INCLUDED
#define SHELL_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include "linked_list/linked_list.h"

#define MAXARGS   128
#define	MAXLINE	 8192  /* Max text line length */
#define MAXJOBS  64

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);
void handler(int sig);
Job *create_job(pid_t pgid, int jid, char *command_line);
void print_jobs();
Job *find_job_by_jid(int jid);
Job *find_job_by_pgid(int pgid);
Job *get_job(char *id);
void jobs_command();
void fg_command(char **argv);
void bg_command(char **argv);
void kill_command(char **argv);
void cd_command(char **argv);
void remove_job_by_jid(int jid);
void remove_job_by_pgid(int pgid);
void set_job_status(Job *job, char *new_status);

#endif

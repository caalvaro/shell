#ifndef JOBS_H_INCLUDED
#define JOBS_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include "linked_list/linked_list.h"
#include "shell.h"

Job *create_job(pid_t pgid, int jid, char *command_line);
void print_jobs();
Job *find_job_by_jid(int jid);
Job *find_job_by_pgid(int pgid);
Job *get_job(char *id);
void remove_job_by_jid(int jid);
void remove_job_by_pgid(int pgid);
void set_job_status(Job *job, char *new_status);
void add_job(Job *job);

#endif

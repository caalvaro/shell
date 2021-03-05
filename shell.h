#ifndef SHELL_H_INCLUDED
#define SHELL_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include "jobs.h"
#include "builtin_commands.h"

#define MAXARGS   128
#define	MAXLINE	 8192  /* Max text line length */

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);
void handler(int sig);

#endif

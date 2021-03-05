#ifndef BUILTIN_COMMANDS_H_INCLUDED
#define BUILTIN_COMMANDS_H_INCLUDED

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include "linked_list/linked_list.h"
#include "jobs.h"

void jobs_command();
void fg_command(char **argv);
void bg_command(char **argv);
void kill_command(char **argv);
void cd_command(char **argv);

#endif

#ifndef LINKED_LIST_H_INCLUDED
#define LINKED_LIST_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>

typedef struct _LIST_NODE LIST_NODE;
typedef struct _LIST_HEAD LIST_HEAD;

typedef struct job {
    pid_t pgid;
    int jid;
    char *command_line;
    char *status;
} Job;

struct _LIST_NODE {
    Job *job;
    LIST_NODE *previous_node;
    LIST_NODE *next_node;
};

struct _LIST_HEAD {
    int list_size;
    LIST_NODE* first_node;
    LIST_NODE* last_node;
};

LIST_NODE* create_node(Job *job);
LIST_HEAD* create_head();
void append_node(LIST_HEAD* list_head, LIST_NODE* node);
void remove_node(LIST_HEAD* list_head, LIST_NODE* node);
LIST_NODE* get_node(LIST_HEAD* list_head, int id);

#endif

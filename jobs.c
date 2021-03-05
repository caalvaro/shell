#include "jobs.h"

// variáveis globais inicializadas em main.c
extern char **environment;
extern pid_t foreground_process; // pid do processo que está em foreground
extern int jobs_counter;

extern LIST_HEAD *jobs_list; // lista de jobs ativos

Job *create_job(pid_t pgid, int jid, char *command_line) {
  Job *job = malloc(sizeof(job));
  char *job_cmd = malloc(sizeof(char) * MAXLINE);
  strcpy(job_cmd, command_line);

  job->pgid = pgid;
  job->jid = jid;
  job->command_line = job_cmd;
  job->status = "Running";

  return job;
}

void print_jobs() {
  LIST_NODE *job_node = jobs_list->first_node;

  while (job_node != NULL) {
    printf("[%d] %s: %s\n", job_node->job->jid, job_node->job->status, job_node->job->command_line);
    job_node = job_node->next_node;
  }
}

Job *find_job_by_jid(int jid) {
  LIST_NODE *job_node = jobs_list->first_node;

  while (job_node != NULL) {
    if (job_node->job->jid == jid) {
      return job_node->job;
    }
    job_node = job_node->next_node;
  }

  printf("jobs: não há job com JID %d.\n", jid);

  return NULL;
}

Job *find_job_by_pgid(int pgid) {
  LIST_NODE *job_node = jobs_list->first_node;

  while (job_node != NULL) {
    if (job_node->job->pgid == pgid) {
      return job_node->job;
    }
    job_node = job_node->next_node;
  }

  printf("jobs: não há job com PGID %d.\n", pgid);

  return NULL;
}

Job *get_job(char *id) {
  Job *job;
  // recebe um argumento JID ou PID
  if (id[0] == '%') {
    int jid = atoi(id + 1); // pega só os caracteres após o primeiro

    job = find_job_by_jid(jid);
  }
  else {
    int pgid = atoi(id);

    job = find_job_by_pgid(pgid);
  }

  return job;
}

void remove_job_by_jid(int jid) {
  Job *job = find_job_by_jid(jid);

  if (job == NULL) {
    printf("Erro: Não foi possível remover o job\n");
    return;
  }

  remove_node(jobs_list, get_node(jobs_list, job->jid));
}

void remove_job_by_pgid(int pgid) {
  Job *job = find_job_by_pgid(pgid);

  if (job == NULL) {
    printf("Erro: Não foi possível remover o job\n");
    return;
  }

  remove_node(jobs_list, get_node(jobs_list, job->jid));
}

void set_job_status(Job *job, char *new_status) {
  if (job == NULL) {
    printf("Erro: não foi possível alterar o status do job\n");
    return;
  }

  job->status = new_status;
}

void add_job(Job *job) {
  append_node(jobs_list, create_node(job));
}

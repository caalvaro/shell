#include "builtin_commands.h"

// variáveis globais inicializadas em main.c
extern char **environment;
extern pid_t foreground_process; // pid do processo que está em foreground
extern int jobs_counter;

extern LIST_HEAD *jobs_list; // lista de jobs ativos

// Comando interno jobs da shell
void jobs_command() {
  LIST_NODE *job_node = jobs_list->first_node;

  if (job_node == NULL) {
    printf("jobs: não há jobs ativos no momento.\n");
    return;
  }

  while (job_node != NULL) {
    int status;
    int child_result = waitpid(job_node->job->pgid, &status, WNOHANG | WUNTRACED);

    // analisa a situação dos processos ativos
    if (WIFSTOPPED(status)) { // se o processo estiver parado, seta o status como Stopped
      set_job_status(job_node->job, "Stopped");
    }
    else if (strcmp(job_node->job->status, "Done") == 0) { // se o processo já tiver sido terminado e consultado, remove da lista
      remove_job_by_jid(job_node->job->jid);
    }
    else if (child_result) { // verifica se o processo filho terminou
      set_job_status(job_node->job, "Done");
    }

    job_node = job_node->next_node;
  }

  print_jobs();
}

// Comando interno fg da shell
void fg_command(char **argv) {
  Job *job;

  if (argv[1] == NULL) {
    printf("fg: Insira um job id\n");
    return;
  }

  job = get_job(argv[1]); // resgata o job pelo pid ou jid
  if (job == NULL) {
    return;
  }

  foreground_process = job->pgid;

  // envia um sinal pro processo continuar
  kill(job->pgid, SIGCONT);

  set_job_status(job, "Running");

  printf("Job colocado em foreground\n");

  // espera o processo terminar
  waitpid(job->pgid, NULL, WUNTRACED); // uso do WUNTRACED para a função retornar quando enviarmos SIGTSTP pelo teclado
}

// Comando interno bg da shell
void bg_command(char **argv) {
  Job *job;

  if (argv[1] == NULL) {
    printf("bg: Insira um job id\n");
    return;
  }

  job = get_job(argv[1]); // resgata o job pelo pid ou jid
  if (job == NULL) {
    return;
  }

  // envia um sinal pro processo continuar
  kill(job->pgid, SIGCONT);
  kill(job->pgid, SIGTTIN); // sinaliza que o processo filho não pode usar a entrada padrão enquanto estiver em background

  set_job_status(job, "Running");
  printf("Job executando\n");
  foreground_process = 0;
  // não aguarda pelo término da execução do job, retorna para shell
}

// Comando interno kill da shell
void kill_command(char **argv) {
  Job *job;

  if (argv[1] == NULL) {
    printf("kill: Insira um job id\n");
    return;
  }

  job = get_job(argv[1]); // resgata o job pelo pid ou jid
  if (job == NULL) {
    return;
  }

  // envia um sinal pro processo ser finalizado
  kill(-job->pgid, SIGKILL);

  remove_job_by_jid(job->jid);

  printf("\n%d: %s killed.\n", job->pgid, job->command_line);
  foreground_process = 0;
}

// Comando interno cd da shell
void cd_command(char **argv) {
  if (chdir(argv[1]) != 0) {
    printf("cd: error\n");
  }
  else {
    printf("Diretório alterado para %s\n", argv[1]);
  }
}

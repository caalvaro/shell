#include "shell.h"

char **environment;
pid_t foreground_process; // pid do processo que está em foreground
int jobs_counter;

LIST_HEAD *jobs_list; // lista de jobs ativos

int main(int argc, char **argv, char **envp) {
  char command_line[MAXLINE];

  foreground_process = 0; // pid do processo que está em foreground
  jobs_counter = 1;

  // handlers de sinal
  signal(SIGINT, handler);
  signal(SIGTSTP, handler);

  environment = envp;
  jobs_list = create_head();

  while (1) {
    printf("\nshell> ");
    fgets(command_line, MAXLINE, stdin);

    if (feof(stdin))
      exit(0);

    eval(command_line);
  }
}

// executa o comando passado para a shell
void eval(char *cmdline) {
  char *argv[MAXARGS];    // lista de argumentos para ser construida pela função parseline e passada para execve()
  char buffer[MAXLINE];   // guarda a linha de comando modificada
  int background;         // o processo irá rodar em background?
  pid_t pid;              // pid do processo filho

  strcpy(buffer, cmdline);
  background = parseline(buffer, argv); // constrói o argv e retorna e deve rodar em background

  if (argv[0] == NULL)
    return;   // ignora linhas vazias

  if (!builtin_command(argv)) {
    if ((pid = fork()) == 0) {   // executado pelo processo filho
      if (execve(argv[0], argv, environment) < 0) {
        printf("%s: Comando não encontrado.\n", argv[0]);
        exit(0);
      }
    }
    else { // executado pelo processo pai
      if (access(argv[0], X_OK) != 0) { // não cria o job se o arquivo não foi encontrado
        waitpid(pid, NULL, 0);
        return;
      }

      append_node(jobs_list, create_node(create_job(pid, jobs_counter++, cmdline)));

      // espera o processo filho se estiver em foreground
      if (!background) {
        foreground_process = pid;

        if (waitpid(pid, NULL, WUNTRACED) < 0) // uso do WUNTRACED para quando o filho for interrompido, a função retorna
          printf("waitfg: waitpid error");

        remove_job_by_pgid(pid);
        foreground_process = 0;
      }
      else {
        foreground_process = 0;
        kill(pid, SIGTTIN); // sinaliza que o processo filho não pode usar a entrada padrão enquanto estiver em background
        printf("background: %d %s", pid, cmdline);
      }
    }
  }
  return;
}

// executa os comandos internos da shell, se existirem
int builtin_command(char **argv) {
  if (!strcmp(argv[0], "quit"))    /* quit command */
    exit(0);

  if (!strcmp(argv[0], "&"))       /* Ignore singleton & */
    return 1;

  if (!strcmp(argv[0], "cd")) {    /* run change directory command */
    cd_command(argv);
    return 1;
  }

  if (!strcmp(argv[0], "jobs")) {  /* run jobs command */
    jobs_command();
    return 1;
  }

  if (!strcmp(argv[0], "fg")) {    /* run fg command */
    fg_command(argv);
    return 1;
  }

  if (!strcmp(argv[0], "bg")) {    /* run bg command */
    bg_command(argv);
    return 1;
  }

  if (!strcmp(argv[0], "kill")) {  /* run kill command */
    kill_command(argv);
    return 1;
  }

  return 0; /* Not a builtin command */
}

// analisa o comando passado e constrói a lista de argumentos
int parseline(char *buffer, char **argv) {
  char *delimiter;                    // aponta pro primeiro delimitador de espaço
  int argc;                           // quantidade de argumentos
  int run_in_background;              // o processo vai rodar em background?

  buffer[strlen(buffer)-1] = ' ';      // troca o '\n' de trailing para espaço
  while (*buffer && (*buffer == ' '))  // ignora espaços
    buffer++;

  // constrói o argv
  argc = 0;
  while ((delimiter = strchr(buffer, ' '))) {
    argv[argc++] = buffer;
    *delimiter = '\0';
    buffer = delimiter + 1;
    while (*buffer && (*buffer == ' ')) // ignora espaços
      buffer++;
  }
  argv[argc] = NULL;

  if (argc == 0)  // ignora linhas em branco
    return 1;

  // o processo deve ser rodado em background?
  if ((run_in_background = (*argv[argc-1] == '&')) != 0)
    argv[--argc] = NULL;

  return run_in_background;
}

// gerencia os sinais SIGINT e SIGTSTP recebidos pelo teclado
void handler(int sig) {
  if (foreground_process) {
    Job *job = find_job_by_pgid(foreground_process);
    pid_t pid = foreground_process;
    foreground_process = 0; // seta o processo em foreground para zero, para ignorar imediatamente o próximo sinal

    if (sig == SIGTSTP) {
      set_job_status(job, "Stopped");
      printf("\n%d: %s stopped.\n", job->pgid, job->command_line);
    }
    else if (sig == SIGINT) {
      remove_job_by_jid(job->jid);
      printf("\n%d: %s killed.\n", job->pgid, job->command_line);
    }

    kill(pid, sig);
  }
}

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
  kill(job->pgid, SIGKILL);

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

#include "shell.h"

// variáveis globais inicializadas em main.c
extern char **environment;
extern pid_t foreground_process; // pid do processo que está em foreground
extern int jobs_counter;

extern LIST_HEAD *jobs_list; // lista de jobs ativos

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
    if (access(argv[0], X_OK) != 0) { // se não é possível acessar o arquivo passado, retorna
      printf("%s: Comando não encontrado.\n", argv[0]);
      return;
    }

    if ((pid = fork()) == 0) {   // executado pelo processo filho
      execve(argv[0], argv, environment);
    }
    else { // executado pelo processo pai
      add_job(create_job(pid, jobs_counter++, cmdline)); // adiciona o job na lista de jobs

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

#include "shell.h"

// variáveis globais compartilhadas entre arquivos
char **environment = NULL;
pid_t foreground_process = 0; // pid do processo que está em foreground
int jobs_counter = 1;

LIST_HEAD *jobs_list = NULL; // lista de jobs ativos

int main(int argc, char **argv, char **envp) {
  char command_line[MAXLINE];

  // handlers de sinal
  signal(SIGINT, handler);
  signal(SIGTSTP, handler);

  // inicializa variáveis globais
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

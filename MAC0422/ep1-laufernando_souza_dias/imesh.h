/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

EP1 - Simulador de Processos usando POSIX Threads
MAC0422 - Sistemas Operacionais

Professor: Daniel Macedo Batista
Aluno: Laufernando Souza Dias

Entregue em 23/03/2026

Uso: ./imesh
Comandos suportados:

• /bin/ls -1aF --color=never
• /bin/top -b -n 1 -p 1
• ./ep1 <argumentos do EP1>
• pwd
• date +%s
• kill -<sinal> <pid>

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#ifndef IMESH_H
#define IMESH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/wait.h>
#include <time.h>

#include <readline/readline.h>
#include <readline/history.h>

#define MAX_INPUT 512
#define MAX_PATH 2048

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

// Protótipos

void cmd_pwd(void);
void cmd_date(void);
void cmd_kill(pid_t pid, int signal);
char* get_prompt(void);
int process_command(char *cmd);

#endif


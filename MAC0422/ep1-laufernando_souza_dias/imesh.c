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

#include "imesh.h"

// Implementação dos 3 comandos internos com syscalls

// syscall: getcwd
void cmd_pwd() {
    char *buf;

    // Usando alocação dinâmica para pegar o diretório atual
    // Se buf é NULL, getcwd deve alocar memória usando malloc em alguns sistemas
    buf = getcwd(NULL, 0); 
    if (buf != NULL) {
        printf("%s\n", buf);
        free(buf); // A chamada é responsável por liberar a memória (caller)
    } else {
        perror("getcwd error (dynamic allocation)");
    }
}

// syscall: time
void cmd_date() {
    // Invoca time() com NULL, o que retorna o tempo atual desde Epoch
    time_t seconds_since_epoch = time(NULL);
    if (seconds_since_epoch == (time_t)-1) {
        perror("time error");
    }

    printf("%ld\n", (long)seconds_since_epoch);
}

// syscall: kill
void cmd_kill(pid_t pid, int signal) {
    if ( pid <= 0 || !( 1 <= signal && signal <= 31) ) {
        printf("invalid input error\n");
    } else {
        int result = kill(pid, signal);
        if (result == 0) {
            printf("signal %d sent to process %d\n", signal, pid);
        } 
        else {
            printf("error sending signal: verify the pid and permissions\n");
        }
    }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

// Obtém o prompt a ser exibido em determinado instante; retorna ponteiro para a string do prompt
char* get_prompt() {
    static char prompt[MAX_PATH];
    uid_t uid;
    char hostname[128];
    char cwd[MAX_PATH];

    // Obtém o nome do usuário
    uid = getuid();
    struct passwd *pwd = getpwuid(uid);

    // Obtém o nome da máquina e do diretório
    gethostname(hostname, sizeof(hostname));
    getcwd(cwd, sizeof(cwd));

    hostname[sizeof(hostname) - 1] = '\0';
    cwd[sizeof(cwd) - 1] = '\0';
    
    snprintf(prompt, sizeof(prompt)+MAX_PATH, "[%s@%s:%s]$ ", pwd->pw_name, hostname, cwd);
    return prompt;
} 

/*
Aqui se processa o comando pedido pelo usuário: para comandos externos (onde pode usar execve()) 
primeiro, e comandos internos (os 3 implementados com syscalls) pwd, date e kill.
*/
int process_command(char *cmd) {    
    if (cmd == NULL || strlen(cmd) == 0) {
        return 0;
    }
    
    // Tokenização do comando para separar os argumentos
    char cmd_copy[MAX_PATH];
    strncpy(cmd_copy, cmd, MAX_PATH - 1);
    cmd_copy[MAX_PATH - 1] = '\0';

    char *args[MAX_INPUT];
    char *saveptr;

    char *token = strtok_r(cmd_copy, " \t", &saveptr);

    int i = 0;

    while (token != NULL && i < MAX_INPUT - 1) {
        args[i++] = token;
        token = strtok_r(NULL, " \t", &saveptr);
    }

    args[i] = NULL;

    // Verifica novamente se há argumentos
    if (i == 0) {
        return 1;
    }
    
    // Verifica comando exit
    if (strcmp(args[0], "exit") == 0 && i == 1) {
        return 0;
    }

    char *program_path;

    // Comandos externos implementados com execve() substituindo um processo filho após fork()
    switch (i) {
        case 1:
            if (strcmp(args[0], "pwd") == 0) {
                cmd_pwd();
                return 1;
            }
            break;
        case 2:
            if (strcmp(args[0], "date") == 0 &&
                    strcmp(args[1], "+%s") == 0) {
            
                cmd_date();
                return 1;
            }
            break;
        case 3:
            if (strcmp(args[0], "/bin/ls") == 0 &&
                strcmp(args[1], "-1aF") == 0 &&
                strcmp(args[2], "--color=never") == 0) {

                program_path = "/usr/bin/ls";

                pid_t pid = fork();
                int status;
                char *envp[] = {NULL};

                if (pid < 0) {
                    perror("fork failed");
                    return 1;
                } 

                else if (pid == 0) {
                    
                    if (execve(program_path, args, envp) == -1) {
                        perror("execve failed");
                        return 1;
                    }
                }

                else waitpid(pid, &status, 0);
                return 1;
            }
            else if (strcmp(args[0], "kill") == 0 &&
                    args[1] != NULL &&
                    args[2] != NULL ) {

                pid_t pid_to_kill = atoi(args[2]);
                int signal = -1 * (atoi(args[1]));
                cmd_kill(pid_to_kill, signal);
                return 1;
            }
            break;
        case 4:
            if (strcmp(args[0], "./ep1") == 0) {
                program_path = "./ep1";

                pid_t pid = fork();
                int status;
                char *envp[] = {NULL};

                if (pid < 0) {
                    perror("fork failed");
                    return 1;
                } 

                else if (pid == 0) {
                    char *envp[] = {NULL};

                    if (execve(program_path, args, envp) == -1) {
                        perror("execve failed");
                        return 1;
                    }
                }

                else waitpid(pid, &status, 0);
                return 1;
            } 
            break;
        case 6:
            if (strcmp(args[0], "/bin/top") == 0 &&
                strcmp(args[1], "-b") == 0 &&
                strcmp(args[2], "-n") == 0 &&
                strcmp(args[3], "1") == 0 &&
                strcmp(args[4], "-p") == 0 &&
                strcmp(args[5], "1") == 0) {

                program_path = "/usr/bin/top";

                pid_t pid = fork();
                int status;
                char *envp[] = {NULL};

                if (pid < 0) {
                    perror("fork failed");
                    return 1;
                } 

                else if (pid == 0) {

                    if (execve(program_path, args, envp) == -1) {
                        perror("execve failed");
                        return 1;
                    }
                }

                else waitpid(pid, &status, 0);
                return 1;
            }
            break;
    }

    printf("invalid command\n");
    return 1;
}

// Só trabalha com readline e history, gerenciamento de recursos do shell.
int main() {
    char *input;
    
    // Inicializa a biblioteca readline
    using_history();
    
    while (1) {
        // Usa readline para obter entrada com histórico e edição
        input = readline(get_prompt());
        
        // Sai se Ctrl+D for pressionado
        if (input == NULL) {
            printf("\n");
            break;
        }
        
        // Adiciona ao histórico se entrada não é vazia
        if (strlen(input) > 0) {
            add_history(input);
        }
        
        // Processamento dos 6 comandos do shell
        int process_status = process_command(input);
        free(input);

        // Ao parar de processar por entrada nula ou comando 'exit'
        if (process_status == 0) break;
    }

    // Limpa recursos do readline
    clear_history();

    return 0;
}
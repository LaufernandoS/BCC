/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

EP1 - Simulador de Processos usando POSIX Threads
MAC0422 - Sistemas Operacionais

Professor: Daniel Macedo Batista
Aluno: Laufernando Souza Dias

Entregue em 23/03/2026

Uso: ./ep1 <escalonador> <trace.txt> <saida.txt>
<escalonador> - 1 pra SJF, 2 pra RR, 3 pra PRIORITY

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
\
#ifndef EP1_H
#define EP1_H

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <pthread.h>
#include <time.h>

#define MAX_PROCESSES 50            // Definição da proposta do EP
#define MAX_RUNNING_TIME 120        // Definição da proposta do EP
#define MAX_NAME_LEN 32             // Tamanho máximo para nome de processo
#define TICK 10L                    // Tick do simulador para passagem de tempo em sleep
#define QUANTUM 2                   // Quantum para os escalonadores preemptivos
#define QUANTUM_MS (QUANTUM * 1000L)
#define MAX_PRIORITY 5              // 5 classes de prioridade para o escalonador
#define BUFFER_SIZE 128             // Limita o tamanho do nome de arquivo trace

// Tipos de escalonadores
typedef enum {
    SJF = 1,                        // Shortest Job First (Mais Curto Primeiro)
    RR = 2,                         // Round-Robin (Chaveamento Circular)
    PRIORITY = 3                    // Escalonamento com Prioridade
} SchedulerType;

// Tipos de estado de um processo
typedef enum { NEW, READY, RUNNING, BLOCKED, FINISHED } ProcessState;

// Estrutura para representar um processo
typedef struct {
    char name[MAX_NAME_LEN];
    int deadline;                   // prazo final
    int t0;                         // t0 - tempo de chegada                        [[[em segundos]]]
    int dt;                         // dt - duração total                           [[[em segundos]]]
    int process_idx;                // guarda o índice do processo no trace
    pthread_t thread;               // thread do processo
    pthread_mutex_t process_mutex;  // mutex protege acesso ao processo
    pthread_cond_t process_cond;    // variável de condição do processo
    long remaining_time;            // tempo restante de execução                   [[[em milissegundos]]]
    long tf;                        // tf - tempo de finalização                    [[[em milissegundos]]]
    long tr;                        // tr - tempo de execução (tempo de relógio)    [[[em milissegundos]]]
    ProcessState status;            // indica o status, se novo, pronto, rodando, bloqueado ou finalizado
    int fulfilled;                  // 1 = cumpriu o deadline; 0 = não cumpriu
    int core;                       // indica se, e onde (core) o processo está escalonado (rodando); 
                                    // -1 caso não esteja rodando
    long time_in_current_core;      // trackeia o tempo no core atual (preemptivos) [[[em milissegundos]]]
    long last_update;               // tempo da última atualização                  [[[em milissegundos]]]
    int should_run;                 // flag do escalonador passada para o processo quando deve rodar
} Process;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

// Estruturas para trabalhar com as filas

typedef struct Node {
    int process_idx;
    struct Node* next;
} Node;

typedef struct Queue {
    Node* front;
    Node* rear;
} Queue;

typedef struct PriorityQueue {
    Queue queues[MAX_PRIORITY];
} PriorityQueue;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

// Variáveis Globais

extern Process processes[MAX_PROCESSES];
extern SchedulerType scheduler_type;
extern int num_processes;
extern long slice;    
extern long current_time;
extern int preemption_count;
extern int available_cores;
extern Process **cores_availability;
extern struct timespec start_time;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

// Protótipos de Funções

long get_current_time_ms(void);
int ms_to_seconds(long ms);

void init_queue(Queue* q);
int  is_empty(Queue* q);
void init_priority_queue(PriorityQueue* pq);
int is_empty_priority(PriorityQueue* pq);
static Node* make_node(int process_idx);
void enqueue(Queue* q, int process_idx);
void enqueue_sjf(Queue* q, int process_idx);
int dequeue(Queue* q);
int dequeue_priority(PriorityQueue* pq);
int assign_priority_class(int deadline_diff);

static int process_arrived(int i);
void update_queue(Queue* q);
void update_priority_queue(PriorityQueue* pq);

static void assign_to_core(int process_idx, int core_idx);

void schedule_sjf(Queue* q);
void schedule_rr(Queue* q);
void schedule_priority(PriorityQueue* pq);

int all_processes_finished(void);
void update_running_processes(void);
void dispatch_processes(void);

void* process_thread(void* arg);
void* scheduler_thread(void* arg);

int load_trace(const char* filename);
void save_simulation(const char* filename);

#endif

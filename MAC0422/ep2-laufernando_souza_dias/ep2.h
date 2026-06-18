/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

EP2 - Simulador de Corridas Madison usando Barreira de Sincronização
MAC0422 - Sistemas Operacionais

Professor: Daniel Macedo Batista
Aluno: Laufernando Souza Dias

Entregue em 29/04/2026

Uso: ./ep2 <nº voltas> <metragem do velódromo> <nº de equipes> \
           <abordagem: i|e> [-debug] 

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#ifndef EP2_H
#define EP2_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdatomic.h>
#include <sys/wait.h>
#include <pthread.h>
#include <time.h>

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define BUFFER_SIZE 32
#define MAX_TEAMS 1249
#define EXTERNAL_TRACK 9
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

// Estruturas

typedef enum { NAIVE, EFFICIENT } Approach;

typedef enum { RACING, RECOVERING, FINISHED } RiderState;

// Armazena posição de ciclista
typedef struct {
    int x;
    int y;
} Position;

// Estrutura para representar um ciclista
typedef struct {
    int team;                       // número da equipe, de 1 a k inclusivo
    char rider;                     // diferencia corredores da mesma equipe
    Position pos;                   // posição na matriz do velódromo
    pthread_t thread;               // thread do ciclista
    atomic_int lap;                 // volta atual da equipe;
                                    // corredor lê e escreve, entidade central só lê
    atomic_int finished_flag;       // flag atômica para indicar que finalizou
    RiderState status;              // status do corredor
    int speed;                      // velocidade pretendida em km/h
    int quarter;                    // acompanha quantos quartos de metro andou a cada 60 ms;
                                    // 1 se a 15 km/h, 2 se a 30 km/h, 4 se a 60 km/h
    int swap_flag;                  // flag para necessidade de troca de posições
    int laps_won;                   // número de voltas vencidas pelo corredor
                                    // só a entidade central mexe !!!
    long last_lap_time;             // tempo de finalização da última volta [[[em milissegundos]]]
                                    // só a entidade central mexe !!!
} Cyclist;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Variáveis Globais

extern Approach approach;                  
extern Cyclist* cyclists;                  
extern pthread_mutex_t velodrome_mutex;    
extern pthread_mutex_t* column_mutex;    
extern pthread_barrier_t time_barrier;     
extern int debug;                      
extern atomic_int running_debug;       
extern long current_time;              
extern int cyclists_count;                 
extern int race_finished;              
extern Cyclist *** velodrome;              
extern int control_lap;    
extern int n, d, k;         

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Protótipos de Funções

// Função para converter milissegundos para minutos
double ms_to_minutes(long ms);
// Cálculo do módulo (evita acesso a índice negativo com resto)
int mod(int a, int b);
// Embaralha - sorteio da posição inicial e ordem de desempate
void shuffle_teams(int *arr, int n);
// Sorteia as posições iniciais dos ciclistas na simulação
void draw_start_positions(void);
// Sorteia e retorna nova velocidade de acordo com as probabilidades
int draw_new_speed(Cyclist* cyclist);
// Pega um ciclista correndo e troca pelo companheiro de equipe
void swap_cyclists(Cyclist* racing, Cyclist* recovering);

// Calcula e retorna contagem de ciclistas numa mesma faixa (coluna)
int cyclists_in_column(int column);
// Tentativa de avançar na mesma pista
void try_forward(Cyclist* c);
// Tentativa de ultrapassagem em pista externa
void try_overtaking(Cyclist* c);
// Tentativa de ir para pista mais interna
void try_internal(Cyclist* c);

// Imprime as posições, ao término de uma volta, em stdout
void print_lap_rel(int lap, int** lap_ranking);
// Imprime relatório final pós última volta, em stdout
void print_final_rel(int** lap_ranking);

// Imprime debug em stderr - Renderiza a pista em sentido anti-horário
void print_debug(void);
/* Atualiza o ranking em lap_ranking; retorna o número da volta
 se tiver sido completada, 0 caso contrário */ 
int update_ranking(int* teams, int* current_lap, int** lap_ranking);

// Função de rotina da thread de um ciclista
void* cyclist_thread(void* arg);
// Função de rotina da thread de controle
void* control_thread(void* arg);

// Inicializa estruturas e threads dos ciclistas
void init_cyclists(void);

#endif

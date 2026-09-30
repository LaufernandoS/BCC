/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

EP3 - Simulação de Algoritmos de Substituição de Página 
MAC0422 - Sistemas Operacionais

Professor: Daniel Macedo Batista
Aluno: Laufernando Souza Dias

Entregue em 06/05/2026

Uso: ./ep3 <nome de arquivo pgm de memória física> \ 
            <nome de arquivo pgm de memória virtual> \
            <número do algoritmo de substituição de página>

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#ifndef EP3_H
#define EP3_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define BUFFER_SIZE 128
#define PAGE_SIZE 4096              // em bytes
#define MAX_ADDRESS_SPACES 65536    // em bytes
#define MEM_FRAMES 32
#define VIRTUAL_PAGES 64
#define PAGES_OFFSET 16

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Estruturas para trabalhar com as filas

typedef struct Node {
    void* data;
    struct Node* next;
} Node;

typedef struct Queue {
    Node* front;
    Node* rear;
} Queue;

// Estruturas

typedef enum { 
    NRU = 1, 
    SecondChance = 2, 
    LRU2 = 3
} PageReplacementAlgorithm;

// Entrada típica da tabela de páginas
typedef struct page_table_entry {
    bool bitR;
    bool bitM;
    bool present;                                       // bit presente/ausente da memória física                                          
    uint8_t page_frame_addr;                            // Endereço no quadro de páginas
} page_table_entry;

// Entrada típica do quadro de páginas
typedef struct page_frame_entry {
    int pid;                                            // PIDs de processo responsável
    uint8_t page_table_addr;                            // Endereço na tabela de páginas
} page_frame_entry;

typedef struct VirtualMemory {
    page_table_entry* page_table;                       // Tabela de páginas como sequência de entradas
} VirtualMemory;

typedef struct PhysicalMemory {
    page_frame_entry* page_frame;                       // Sequência de quadros de páginas
} PhysicalMemory;

 // Mapeia o número do endereço na tabela de páginas
typedef struct MMU {
    long accesses;                                      // controla a quantidade de acessos a memória
    long page_hits, page_faults;                        // controla a quantidade de page hits e page faults
    Queue* page_queue;                                  // fila - usado no segunda chance
    bool** lru_matrix;                                  // matriz - usado no LRU 2ª versão
} MMU;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Variáveis Globais

extern PageReplacementAlgorithm page_replacement_algorithm;
extern char* virt_mem_filename;                                
extern char* phys_mem_filename;                

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Protótipos de Funções

// Inicializa uma fila implementada com lista encadeada
void init_queue(Queue* q);
// Verifica se uma fila está vazia ou não e retorna 1 em caso afirmativo, 0 em caso negativo
int is_empty(Queue* q);
// Cria um nó alocando memória para o mesmo; retorna o nó, ou NULL em caso de falha
static Node* make_node(void* entry);
// Cria um ponteiro para uma entrada em alguma tabela/quadro e enfileira
void enqueue(Queue* q, void* entry);
// Remove da cabeça e retorna ponteiro para a entrada retirada da fila
void* dequeue(Queue* q);
// Libera a memória de uma fila existente
void free_queue(Queue* q);

/*
Seleciona quadro a ser substituído baseado na página menos usada recentemente usando
os bits R e M. Retorna ponteiro para o quadro modificado.
*/ 
page_frame_entry* nru_page(VirtualMemory* virt_mem, PhysicalMemory* phys_mem, 
    int virt_mem_idx);
/*
Seleciona quadro a ser substituído baseado em página não usada mais próxima na fila da MMU.
Retorna ponteiro para o quadro modificado.
*/ 
page_frame_entry* second_chance_page(VirtualMemory* virt_mem, PhysicalMemory* phys_mem, MMU* mmu, 
    int virt_mem_idx);
/*
Seleciona quadro a ser substituído baseado em página pouco utilizada recentemente usando
uma matriz com bits R. Retorna ponteiro para o quadro modificado.
*/ 
page_frame_entry* lru2_page(VirtualMemory* virt_mem, PhysicalMemory* phys_mem, MMU* mmu, 
    int virt_mem_idx);

// Função auxiliar que reescreve o conteúdo dos arquivos pgm após fault
void rewrite_pgms(char* phys_mem_pgm_f, char* virt_mem_pgm_f, 
    VirtualMemory* virt_mem, PhysicalMemory* phys_mem);
// Atualiza estruturas de dados quando um processo tenta acessar página presente
void page_hit_handler(VirtualMemory* virt_mem, PhysicalMemory* phys_mem, MMU* mmu, 
    char mode, int virt_mem_idx);
// Atualiza estruturas de dados quando um processo tenta acessar página ausente
void page_fault_handler(VirtualMemory* virt_mem, PhysicalMemory* phys_mem, MMU* mmu, 
    char mode, int virt_mem_idx);
// Gerencia as tentativas de acesso a memória baseado em pid, modo de acesso, e endereço 
void access_handler(int pid, char mode, int addr, 
    VirtualMemory* virt_mem, PhysicalMemory* phys_mem, MMU* mmu);

// Imprime todo o conteúdo da memória virtual
void print_virtual(void);
// Imprime todo o conteúdo da memória física
void print_physical(void);

/*
Faz o parsing inicial dos dados dos arquivos de memória para a execução do programa.
Retorna 0 em caso de falha, 1 em caso de sucesso.
*/
int pgms_parser(char* phys_mem_pgm_f, char* virt_mem_pgm_f, VirtualMemory* virt_mem, PhysicalMemory* phys_mem);
/*
Faz o parsing dos comandos do usuário para interação com o programa.
Retorna 0 caso tente encerrar interação, 1 para continuidade.
*/
int command_parser(char *cmd, VirtualMemory* virt_mem, PhysicalMemory* phys_mem, MMU* mmu);

// Inicializa estruturas chave da MMU
void init_mmu(MMU* mmu);
// Libera memória das estruturas chave da MMU
void free_mmu(MMU* mmu);

#endif
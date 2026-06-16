/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

EP1 - Simulador de Processos usando POSIX Threads
MAC0422 - Sistemas Operacionais

Professor: Daniel Macedo Batista
Aluno: Laufernando Souza Dias

Entregue em 23/03/2026

Uso: ./ep1 <escalonador> <trace.txt> <saida.txt>
<escalonador> - 1 pra SJF, 2 pra RR, 3 pra PRIORITY

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "ep1.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

// Variáveis Globais

Process   processes[MAX_PROCESSES];
SchedulerType scheduler_type;
int       num_processes    = 0;    // 
long      slice            = 2;    // 2 ms como slice para processo "queimar" CPU
long      current_time     = 0;    // 
int       preemption_count = 0;    //
int       available_cores;         //   
Process **cores_availability;      // Vetor de ponteiros (usado p/ acessar valor de core)
struct timespec start_time;        // Estrutura para simular relógio do sistema

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

// Utilitários de tempo

// Função para obter o tempo atual em milissegundos
long get_current_time_ms() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec - start_time.tv_sec) * 1000 + (now.tv_nsec - start_time.tv_nsec) / 1000000;
}

// Função para converter milissegundos para segundos
int ms_to_seconds(long ms) {
    return ms / 1000;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

// Operadores para trabalhar com as filas

// Inicialização e verificação se está vazia

void init_queue(Queue* q)  { 
    q->front = NULL; 
    q->rear = NULL;
}

int  is_empty(Queue* q) { return q->front == NULL; }

void init_priority_queue(PriorityQueue* pq) {
    for (int i = 0; i < MAX_PRIORITY; i++)
        init_queue(&pq->queues[i]);
}

int is_empty_priority(PriorityQueue* pq) {
    for (int i = MAX_PRIORITY - 1; i >= 0; i--)
        if (!is_empty(&pq->queues[i])) return 0;
    return 1;
}

// Aloca e retorna um novo nó, ou NULL em falha
static Node* make_node(int process_idx) {
    Node* n = malloc(sizeof(Node));
    if (n) { n->process_idx = process_idx; n->next = NULL; }
    return n;
}

// Adiciona um processo ao final da fila
void enqueue(Queue* q, int process_idx) {
    // Cria nó e aloca a memória
    Node* newNode = make_node(process_idx);
    if (newNode == NULL) return;

    // Se estiver vazia, o processo é a cabeça e cauda
    if (is_empty(q)) {
        q->front = newNode;
        q->rear = newNode;
    } else {
        // Se não, é alocado ao final da fila
        q->rear->next = newNode;
        q->rear = newNode;
    }
}

// Inserção ordenada por dt (menor primeiro) para SJF
void enqueue_sjf(Queue* q, int process_idx) {
    Node* newNode = make_node(process_idx);
    if (!newNode) return;

    int new_dt = processes[process_idx].dt;

    if (is_empty(q) || new_dt < processes[q->front->process_idx].dt) {
        newNode->next  = q->front;
        q->front = newNode;
        if (q->rear == NULL) q->rear = newNode;
        return;
    }

    // Olha dos dts do nó seguinte ao nó atual
    Node* currentNode = q->front;
    while (currentNode->next && processes[currentNode->next->process_idx].dt <= new_dt)
        currentNode = currentNode->next;

    // Ajusta os ponteiros
    newNode->next   = currentNode->next;
    currentNode->next = newNode;
    if (!newNode->next) q->rear = newNode;
}

// Remove o processo da cabeça da fila e retorna seu índice 
int dequeue(Queue* q) {
    if (is_empty(q)) return -1;
    
    // Armazena a cabeça para poder liberar a memória dele posteriormente
    Node* temp = q->front;
    int deq_process_idx = temp->process_idx;
    // Move o ponteiro de frente para o segundo
    q->front = temp->next;
    // Atualiza se a fila ficar vazia
    if (q->front == NULL) q->rear = NULL;

    free(temp);
    return deq_process_idx;
}

// Remove o elemento mais próximo da primeira classe de não vazia
int dequeue_priority(PriorityQueue* pq) {
    for (int i = MAX_PRIORITY - 1; i >= 0; i--) {
        if (!is_empty(&pq->queues[i])) {
            return dequeue(&pq->queues[i]);
        }
    }
    return -1;
}

// Dado a distância para o deadline de um processo, define a prioridade
int assign_priority_class(int deadline_diff) {
    if (deadline_diff <  0)  return 0;
    if (deadline_diff <  5)  return MAX_PRIORITY - 1;
    if (deadline_diff < 10)  return MAX_PRIORITY - 2;
    if (deadline_diff < 15)  return MAX_PRIORITY - 3;
    return                          MAX_PRIORITY - 4;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

// Admissão de novos processos às filas

// Condição de chegada compartilhada pelos três escalonadores
static int process_arrived(int i) {
    return processes[i].status == NEW
        && processes[i].t0 <= ms_to_seconds(current_time);
}

// Atualiza a fila para a chegada de novos processos
void update_queue(Queue* q) {
    for (int i = 0; i < num_processes; i++) {
        if (!process_arrived(i)) continue;
        processes[i].status = READY;
        if (scheduler_type == SJF) enqueue_sjf(q, i);
        else                       enqueue(q, i);
    }
}

// Atualiza a fila de prioridades para novos processos 
void update_priority_queue(PriorityQueue* pq) {
    for (int i = 0; i < num_processes; i++) {
        if (!process_arrived(i)) continue;
        processes[i].status = READY;
        // Seleciona a fila correta em termos de prioridade
        int diff     = processes[i].deadline
                     - ms_to_seconds(processes[i].remaining_time)
                     - ms_to_seconds(current_time);
        enqueue(&pq->queues[assign_priority_class(diff)], i);
    }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

// Alocação de processo em core livre (comum aos três escalonadores)

static void assign_to_core(int process_idx, int core_idx) {
    Process* p            = &processes[process_idx];
    p->core               = core_idx;
    p->status             = RUNNING;
    p->time_in_current_core = 0;
    p->last_update        = get_current_time_ms();
    cores_availability[core_idx] = p;
    // Faz a atribuição de thread sobre uma cpu física
    cpu_set_t cp;
    CPU_ZERO(&cp);
    CPU_SET(core_idx, &cp);
    pthread_setaffinity_np(processes[process_idx].thread, sizeof(cpu_set_t), &cp);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

/*
Escalonadores 

Fica a cargo das funções de escalonamento DECIDIR qual é o próximo processo, constituindo a 
terceira da quarta etapa do loop da thread de escalonamento.
*/

// SJF
/*
    Ordena de acordo com o que tem menor dt, seleciona, e só deve decidir novamente quando houver 
    finalizado um processo atualmente rodando, por isso tem função própria para inserção em fila: 
    seguindo a ideia do princípio de indução (transitividade), dada uma fila ordenada e a chegada 
    de um novo processo, somente encaixá-lo na posição correta mantém a ordenação satisfeita; o caso 
    base é quando a fila está vazia e um novo processo vira cabeça e cauda.
*/
void schedule_sjf(Queue* q) {
    for (int core_idx = 0; core_idx < available_cores && !is_empty(q); core_idx++) {
        if (cores_availability[core_idx] != NULL) continue;
        int idx = dequeue(q);
        if (idx != -1) assign_to_core(idx, core_idx);
    }
}

// RR
/*
    Segue a fila de processos de maneira circular, ou seja, tira um processo e joga ele no final 
    (queue e dequeue simples). A preempção deve acontecer de modo a olhar para o tempo consumido em 
    cada core por determinado processo e comparar com o quantum.
*/
void schedule_rr(Queue* q) {
    for (int core_idx = 0; core_idx < available_cores; core_idx++) {
        if (cores_availability[core_idx] == NULL) {
            int idx = dequeue(q);
            if (idx != -1) assign_to_core(idx, core_idx);
            continue;
        }

        // Verifica necessidade de preempção
        Process* prev_process = cores_availability[core_idx];
        if (prev_process->time_in_current_core < QUANTUM_MS || is_empty(q))
            continue;

        int idx = dequeue(q);
        if (idx == -1) continue;
        // Coloca o preemptado de volta na fila
        prev_process->core                 = -1;
        prev_process->status               = READY;
        prev_process->time_in_current_core = 0;
        enqueue(q, prev_process->process_idx);
        
        // Atualiza com novo processo 
        assign_to_core(idx, core_idx);
        preemption_count++;
    }
}

// Priority
/*
    Criada com classes de prioridade (como de 0 a 4, inclusivo), similar ao round-robin,
    mas com foco em cumprir o máximo de deadlines ao verificar a distância para tal.

    Classe 4: prazo apertado, com 0 <= deadline - current_time - remaining_time  < 5
    Classe 3: prazo menos apertado, com 5 <= deadline - current_time - remaining_time  < 10
    Classe 2: prazo ainda mais flexível, 10 <= deadline - current_time - remaining_time < 15
    Classe 1: prazo muito flexível, 15 <= deadline - current_time - remaining_time
    Classe 0: processos que já não podem mais ter o deadline cumprido
*/
void schedule_priority(PriorityQueue* pq) {
    for (int core_idx = 0; core_idx < available_cores; core_idx++) {
        if (cores_availability[core_idx] == NULL) {
            int idx = dequeue_priority(pq);
            if (idx != -1) assign_to_core(idx, core_idx);
            continue;
        }

        // Verifica necessidade de preempção
        Process* prev_process = cores_availability[core_idx];
        if (prev_process->time_in_current_core < QUANTUM_MS || is_empty_priority(pq))
            continue;

        int idx = dequeue_priority(pq);
        if (idx == -1) continue;
        // Coloca o preemptado de volta na fila
        prev_process->core                 = -1;
        prev_process->status               = READY;
        prev_process->time_in_current_core = 0;
        // Seleciona a fila correta em termos de prioridade
        int diff     = prev_process->deadline
                     - ms_to_seconds(prev_process->remaining_time)
                     - ms_to_seconds(current_time);
        enqueue(&pq->queues[assign_priority_class(diff)], prev_process->process_idx);

        assign_to_core(idx, core_idx);
        preemption_count++;
    }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

// Funções auxiliares p/ escalonamento

// Verifica se os processos finalizaram
int all_processes_finished() {
    for (int i = 0; i < num_processes; i++) {
        if (processes[i].status != FINISHED)
            return 0;
    }
    return 1;
}

// Atualiza variáveis dos processos em execução
void update_running_processes() {
    for (int i = 0; i < available_cores; i++) {
        // Verifica o processo no core escolhido
        Process* p = cores_availability[i];

        if (p != NULL) {
            // Proteção contra leituras de estado do mutex
            pthread_mutex_lock(&p->process_mutex);

            long elapsed              = get_current_time_ms() - p->last_update;
            p->remaining_time         -= elapsed;
            p->time_in_current_core   += elapsed;
            p->tr                     += elapsed;
            p->last_update            = get_current_time_ms();

            if (p->remaining_time <= 0) {
                p->status             = FINISHED;
                p->tf                 = current_time;
                p->fulfilled          = ( (int) p->tf / 1000 <= p->deadline);

                // Libera o core
                p->core               = -1;
                cores_availability[i] = NULL;
            }

            pthread_mutex_unlock(&p->process_mutex);
        }
    }
}

// Sinaliza para processos que devem rodar ou encerrar
void dispatch_processes() {
    for (int i = 0; i < num_processes; i++) {
        Process* p = &processes[i];
        // Processo rodando sai do wait, processo finalizado precisa de sinalização
        if (p->status != RUNNING && p->status != FINISHED) continue;

        pthread_mutex_lock(&p->process_mutex);
        p->should_run = 1;
        pthread_cond_signal(&p->process_cond);
        pthread_mutex_unlock(&p->process_mutex);
    }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

// Threads

// Função que executa thread de processo (operário)
void* process_thread(void* arg) {
    Process* p = (Process*) arg;

    pthread_mutex_lock(&p->process_mutex);
    while (1) {
        // Sempre tem que verificar se o escalonador libera rodar
        while (!p->should_run) {
            pthread_cond_wait(&p->process_cond, &p->process_mutex);
        }

        // status protegido por mutex (só chega aqui se já houve despache)
        if (p->status == FINISHED) {
            pthread_mutex_unlock(&p->process_mutex);
            return NULL;
        }

        // should_run protegido por mutex (só chega aqui se já houve despache)
        p->should_run = 0;
        pthread_mutex_unlock(&p->process_mutex);

        // Busy loop controlado com 10 milissegundos
        long start = get_current_time_ms();
        volatile int alpha = 0;
        while (get_current_time_ms() - start < slice) {
            // Sai do loop se foi preemptado
            pthread_mutex_lock(&p->process_mutex);
            int still_running = (p->status == RUNNING);
            pthread_mutex_unlock(&p->process_mutex);
            if (!still_running) break;
            for (int i = 0; i < 100; i++) alpha = i;
        }

        pthread_mutex_lock(&p->process_mutex);
    }
}

// Função que executa thread do escalonador (despachante)
void* scheduler_thread(void* arg) {
    // Inicialização das filas
    Queue *process_queue = NULL;
    PriorityQueue *priority_queue = NULL;

    if (scheduler_type == SJF || scheduler_type == RR) {
        process_queue = malloc(sizeof(Queue));
        init_queue(process_queue);
    }
    else if (scheduler_type == PRIORITY) {
        priority_queue = malloc(sizeof(PriorityQueue));
        init_priority_queue(priority_queue);
    }

    while (!all_processes_finished()) {
        current_time = get_current_time_ms();

        // Com essa ordem estrita de aquisição de recursos, evitamos deadlocks mais facilmente:

        // 1. Atualiza execução de quem está rodando
        update_running_processes();
        // 2. Admite chegada de novos processos
        if (scheduler_type != PRIORITY) update_queue(process_queue);
        else update_priority_queue(priority_queue);
        // 3. Escalona com a rotina adequada
        switch (scheduler_type) {
            case SJF:      schedule_sjf(process_queue);       break;
            case RR:       schedule_rr(process_queue);        break;
            case PRIORITY: schedule_priority(priority_queue); break;
        }
        // 4. "Executar" processos (sinalizar threads)
        dispatch_processes();

        // Nanosleep para precisão, gasta um tick
        // Assim o escalonador não corre e deixa o processo queimar CPU
        nanosleep(&(struct timespec){0, TICK * 1000000}, NULL);
    }

    free(process_queue);
    free(priority_queue);
    return NULL;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

// I/O

int load_trace(const char* filename) {
    FILE *input_file;
    char buffer[BUFFER_SIZE];
    
    // Leitura da entrada com os processos
    input_file = fopen(filename, "r");
    if (input_file == NULL) {
        perror("error opening trace file");
        return 0;
    }

    // Laço para ler os processos e armazenar os dados
    while (fgets(buffer, BUFFER_SIZE, input_file) != NULL && num_processes < MAX_PROCESSES) {
        Process* p = &processes[num_processes];

        sscanf(buffer, "%s %d %d %d", p->name, &p->deadline, &p->t0, &p->dt);

        p->process_idx          = num_processes;
        p->remaining_time       = (long)p->dt * 1000;
        p->tr                   = 0;
        p->tf                   = 0;
        p->time_in_current_core = 0;
        p->core                 = -1;
        p->status               = NEW;
        p->fulfilled            = 0;
        p->should_run           = 0;

        pthread_mutex_init(&p->process_mutex, NULL);
        pthread_cond_init(&p->process_cond,  NULL);

        num_processes++;
    }

    return 1;
}

void save_simulation(const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("error creating simulation file");
        return;
    }
    
    for (int i = 0; i < num_processes; i++) {
        int tr_sec = ms_to_seconds(processes[i].tr);
        int tf_sec = ms_to_seconds(processes[i].tf);
        fprintf(file, "%d %s %d %d\n", 
                processes[i].fulfilled,
                processes[i].name,
                tf_sec,
                tr_sec);
    }
    
    // Linha extra com o número de preempções
    fprintf(file, "%d\n", preemption_count);
    fclose(file);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

int main(int argc, char *argv[]) {
    if (argc != 4) {
        perror("invalid number of arguments\n use './ep1 <scheduler> <input_file> <output_file>'");
        return 1;
    }

    scheduler_type = atoi(argv[1]);
    if (scheduler_type < SJF || scheduler_type > PRIORITY) {
        printf("invalid scheduler; use 1 for SJF, 2 for RR, 3 for Priority.\n");
        return 1;
    }
    
    // Carrega o arquivo de trace
    if (!load_trace(argv[2])) {
        printf("error loading trace file.\n");
        return 1;
    }

    // Pega o número de núcleos disponíveis
    int total_cores = (int) sysconf(_SC_NPROCESSORS_CONF);
    // if (total_cores >= 2) total_cores /= 2; // Descomentar p/ testar com metade dos núcleos
    // "Prepara os cores" a serem ocupados
    available_cores = total_cores;
    cores_availability = calloc(available_cores, sizeof(Process*));

    // Seta o tempo de início
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    // Cria a thread do escalonador
    pthread_t scheduler;
    if (pthread_create(&scheduler, NULL, scheduler_thread, NULL) != 0) {
        perror("error creating the scheduler thread");
        return 1;
    }
    // Cria threads para cada processo
    for (int i = 0; i < num_processes; i++) {
        if (pthread_create(&processes[i].thread, NULL, process_thread, &processes[i]) != 0) {
            perror("error creating a process thread");
            return 1;
        }
    }

    // Aguarda a thread do escalonador terminar
    pthread_join(scheduler, NULL);
    // Aguarda todas as threads de processos terminarem
    for (int i = 0; i < num_processes; i++) {
        pthread_join(processes[i].thread, NULL);
    }

    // Salva os resultados
    save_simulation(argv[3]);
    
    return 0;
}

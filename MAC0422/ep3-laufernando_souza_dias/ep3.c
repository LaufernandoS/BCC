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

#include "ep3.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Variáveis Globais

PageReplacementAlgorithm page_replacement_algorithm;    // Reconhece o algoritmo a utilizar
char* virt_mem_filename;                                // Nome do PGM da memória virtual
char* phys_mem_filename;                                // Nome do PGM da memória física

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Operadores para trabalhar com as filas de páginas

void init_queue(Queue* q)  { 
    q->front = NULL; 
    q->rear = NULL;
}

int is_empty(Queue* q) { return q->front == NULL; }

static Node* make_node(void* entry) {
    Node* n = malloc(sizeof(Node));
    if (n) { n->data = entry; n->next = NULL; }
    return n;
}

void enqueue(Queue* q, void* entry) {
    // Cria nó e aloca a memória
    Node* newNode = make_node(entry);
    if (newNode == NULL) return;

    // Se estiver vazia, é a cabeça e cauda
    if (is_empty(q)) {
        q->front = newNode;
        q->rear = newNode;
    } else {
        // Se não, é alocado ao final da fila
        q->rear->next = newNode;
        q->rear = newNode;
    }
}

void* dequeue(Queue* q) {
    if (is_empty(q)) return NULL;
    
    // Armazena a cabeça para poder liberar a memória dele posteriormente
    Node* temp = q->front;
    void* entry = temp->data;
    // Move o ponteiro de frente para o segundo
    q->front = temp->next;
    // Atualiza se a fila ficar vazia
    if (q->front == NULL) q->rear = NULL;

    free(temp);
    return (void*) entry;
}

void free_queue(Queue* q) {
    Node* temp;
    while (q->front != NULL) {
        temp = q->front;
        q->front = q->front->next;
        free(temp);
    }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Algoritmos de Substituição

page_frame_entry* nru_page(VirtualMemory* virt_mem, PhysicalMemory* phys_mem, int virt_mem_idx) {
    int class_counter = 0;
    // Escolhe um ponto aleatório da tabela para começar a busca
    int x = rand() % VIRTUAL_PAGES;
    int page_ptr = x;
    int sub_page_frame_addr = -1;
 
    while (1) {
        if (virt_mem->page_table[page_ptr].present == true) {
            if (class_counter == 0) {
                if (virt_mem->page_table[page_ptr].bitR == false 
                    && virt_mem->page_table[page_ptr].bitM == false) {
                    sub_page_frame_addr = virt_mem->page_table[page_ptr].page_frame_addr;
                    break;
                }
            }
            else if (class_counter == 1) {
                if (virt_mem->page_table[page_ptr].bitR == false 
                    && virt_mem->page_table[page_ptr].bitM == true) {
                    sub_page_frame_addr = virt_mem->page_table[page_ptr].page_frame_addr;
                    break;
                }
            }
            else if (class_counter == 2) {
                if (virt_mem->page_table[page_ptr].bitR == true 
                    && virt_mem->page_table[page_ptr].bitM == false) {
                    sub_page_frame_addr = virt_mem->page_table[page_ptr].page_frame_addr;
                    break;
                }
            }
            else if (class_counter == 3) {
                if (virt_mem->page_table[page_ptr].bitR == true 
                    && virt_mem->page_table[page_ptr].bitM == true) {
                    sub_page_frame_addr = virt_mem->page_table[page_ptr].page_frame_addr;
                    break;
                }
            }
            else break; // class_counter == 4: nenhuma vítima encontrada (não deveria ocorrer)
        }
 
        page_ptr = (page_ptr + 1) % VIRTUAL_PAGES;
        if (page_ptr == x) {
            class_counter++;
            continue; // evita reprocessar x como primeiro da nova classe
        }
    }
 
    if (sub_page_frame_addr != -1) {
        uint8_t old_table_addr = phys_mem->page_frame[sub_page_frame_addr].page_table_addr;
 
        // Atualiza a vítima na tabela virtual
        virt_mem->page_table[old_table_addr].bitR = false;
        virt_mem->page_table[old_table_addr].bitM = false;
        virt_mem->page_table[old_table_addr].present = false;
        virt_mem->page_table[old_table_addr].page_frame_addr = 255;
 
        // Atualiza o quadro físico com os dados da página entrante
        phys_mem->page_frame[sub_page_frame_addr].pid = virt_mem_idx / PAGES_OFFSET * 64;
        phys_mem->page_frame[sub_page_frame_addr].page_table_addr = (uint8_t) virt_mem_idx;
 
        return &phys_mem->page_frame[sub_page_frame_addr];
    }
 
    fprintf(stderr, "ERROR: nru substitution failed\n");
    return NULL;
}
 
page_frame_entry* second_chance_page(VirtualMemory* virt_mem, PhysicalMemory* phys_mem, MMU* mmu, int virt_mem_idx) {
    // Inicialização lazy: se a fila estiver vazia mas há páginas presentes,
    // enfileira os quadros ocupados na ordem de 0 a MEM_FRAMES-1
    if (is_empty(mmu->page_queue)) {
        for (int i = 0; i < MEM_FRAMES; i++) {
            if (virt_mem->page_table[phys_mem->page_frame[i].page_table_addr].present == true)
                enqueue(mmu->page_queue, &phys_mem->page_frame[i]);
        }
    }
 
    page_frame_entry* entry = (page_frame_entry*) dequeue(mmu->page_queue);
 
    if (entry == NULL) {
        fprintf(stderr, "ERROR: empty queue after lazy init\n");
        return NULL;
    }
 
    // Percorre a fila com segunda chance às páginas com bitR == true
    while (virt_mem->page_table[entry->page_table_addr].bitR == true) {
        virt_mem->page_table[entry->page_table_addr].bitR = false;
        enqueue(mmu->page_queue, entry);
        entry = (page_frame_entry*) dequeue(mmu->page_queue);
        if (entry == NULL) {
            fprintf(stderr, "ERROR: queue exhausted during second chance\n");
            return NULL;
        }
    }
 
    uint8_t old_table_addr = entry->page_table_addr;
 
    // Atualiza a vítima na tabela virtual
    virt_mem->page_table[old_table_addr].bitR = false;
    virt_mem->page_table[old_table_addr].bitM = false;
    virt_mem->page_table[old_table_addr].present = false;
    virt_mem->page_table[old_table_addr].page_frame_addr = 255;
 
    // Atualiza o quadro físico com os dados da página entrante
    entry->pid = virt_mem_idx / PAGES_OFFSET * 64;
    entry->page_table_addr = (uint8_t) virt_mem_idx;
 
    return entry;
}
 
page_frame_entry* lru2_page(VirtualMemory* virt_mem, PhysicalMemory* phys_mem, MMU* mmu, int virt_mem_idx) {
    int victim_idx = -1;
 
    for (int i = 0; i < MEM_FRAMES; i++) {
        if (victim_idx == -1) { victim_idx = i; continue; }
        // Comparação: menor valor binário = LRU
        for (int j = 0; j < MEM_FRAMES; j++) {
            if (mmu->lru_matrix[i][j] < mmu->lru_matrix[victim_idx][j]) {
                victim_idx = i;
                break;
            }
            if (mmu->lru_matrix[i][j] > mmu->lru_matrix[victim_idx][j]) {
                break;
            }
            // Se iguais, continua para a próxima coluna
        }
    }
    
    if (victim_idx == -1) {
        fprintf(stderr, "ERROR: lru2 substitution failed\n");
        return NULL;
    }
    page_frame_entry* entry = &phys_mem->page_frame[victim_idx];
 
    uint8_t old_table_addr = entry->page_table_addr;
 
    // Atualiza a vítima na tabela virtual
    virt_mem->page_table[old_table_addr].bitR = false;
    virt_mem->page_table[old_table_addr].bitM = false;
    virt_mem->page_table[old_table_addr].present = false;
    virt_mem->page_table[old_table_addr].page_frame_addr = 255;
 
    // Atualiza o quadro físico com os dados da página entrante
    entry->pid = virt_mem_idx / PAGES_OFFSET * 64;
    entry->page_table_addr = (uint8_t) virt_mem_idx;
 
    return entry;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void rewrite_pgms(char* phys_mem_pgm_f, char* virt_mem_pgm_f, VirtualMemory* virt_mem, PhysicalMemory* phys_mem) {
    // Memória Virtual
    FILE* virt_mem_f = fopen(virt_mem_pgm_f, "w");
    if (virt_mem_f == NULL) {
        perror("error opening virtual memory pgm file");
        return;
    }

    fprintf(virt_mem_f, "P2\n");
    fprintf(virt_mem_f, "1 64\n");
    fprintf(virt_mem_f, "255\n");

    for (int i = 0; i < VIRTUAL_PAGES; i++) 
        fprintf(virt_mem_f, "%hhu\n", virt_mem->page_table[(VIRTUAL_PAGES - 1) - i].page_frame_addr);
    fprintf(virt_mem_f, "\n");

    fclose(virt_mem_f);

    // Memória Física
    FILE* phys_mem_f = fopen(phys_mem_pgm_f, "w");
    if (phys_mem_f == NULL) {
        perror("error opening physical memory pgm file");
        return;
    }

    fprintf(phys_mem_f, "P2\n");
    fprintf(phys_mem_f, "1 32\n");
    fprintf(phys_mem_f, "255\n");

    for (int i = 0; i < MEM_FRAMES; i++) 
        fprintf(phys_mem_f, "%d\n", phys_mem->page_frame[(MEM_FRAMES - 1) - i].pid);
    fprintf(phys_mem_f, "\n");

    fclose(phys_mem_f);
}

// MMU handlers

void page_hit_handler(VirtualMemory* virt_mem, PhysicalMemory* phys_mem, MMU* mmu, char mode, int virt_mem_idx) {
    mmu->page_hits++;
    printf("hit");
 
    // Atualiza bits R e M conforme o modo de acesso
    if (mode == 'L') {
        virt_mem->page_table[virt_mem_idx].bitR = true;
    }
    else if (mode == 'E') {
        virt_mem->page_table[virt_mem_idx].bitR = true;
        virt_mem->page_table[virt_mem_idx].bitM = true;
    }
 
    switch (page_replacement_algorithm) {
        case NRU:
            // Atualização dos bits já realizada acima
            break;
        case SecondChance:
            // Hit não move a página na fila; o bitR=true já foi setado acima.
            // O algoritmo usará esse bit quando a página chegar ao front da fila.
            break;
        case LRU2:
            // Seta toda a linha como 1, depois zera toda a coluna
            uint8_t frame_idx = virt_mem->page_table[virt_mem_idx].page_frame_addr;
            for (int i = 0; i < MEM_FRAMES; i++) mmu->lru_matrix[frame_idx][i] = true;
            for (int i = 0; i < MEM_FRAMES; i++) mmu->lru_matrix[i][frame_idx] = false;
            break;
    }
}
 
void page_fault_handler(VirtualMemory* virt_mem, PhysicalMemory* phys_mem, MMU* mmu, char mode, int virt_mem_idx) {
    mmu->page_faults++;
    printf("fault");
 
    page_frame_entry* sub_frame = NULL;
 
    switch (page_replacement_algorithm) {
        case NRU:
            sub_frame = nru_page(virt_mem, phys_mem, virt_mem_idx);
            break;
        case SecondChance:
            sub_frame = second_chance_page(virt_mem, phys_mem, mmu, virt_mem_idx);
            break;
        case LRU2:
            sub_frame = lru2_page(virt_mem, phys_mem, mmu, virt_mem_idx);
            break;
    }
 
    if (sub_frame == NULL) return;
 
    // Obtém o índice do quadro liberado via aritmética de ponteiros
    int frame_idx = (int)(sub_frame - phys_mem->page_frame);
 
    // Atualiza a página entrante na tabela virtual
    virt_mem->page_table[virt_mem_idx].present = true;
    virt_mem->page_table[virt_mem_idx].page_frame_addr = (uint8_t) frame_idx;
 
    // Atualiza bits R e M da página entrante conforme o modo de acesso
    if (mode == 'L') {
        virt_mem->page_table[virt_mem_idx].bitR = true;
        virt_mem->page_table[virt_mem_idx].bitM = false;
    }
    else if (mode == 'E') {
        virt_mem->page_table[virt_mem_idx].bitR = true;
        virt_mem->page_table[virt_mem_idx].bitM = true;
    }
 
    // No Segunda Chance, enfileira a página recém-carregada
    if (page_replacement_algorithm == SecondChance)
        enqueue(mmu->page_queue, sub_frame);
 
    // No LRU2, atualiza a matriz para a página recém-carregada
    if (page_replacement_algorithm == LRU2) {
        for (int i = 0; i < MEM_FRAMES; i++) mmu->lru_matrix[frame_idx][i] = true;
        for (int i = 0; i < MEM_FRAMES; i++) mmu->lru_matrix[i][frame_idx] = false;
    }
}

void access_handler(int pid, char mode, int addr, VirtualMemory* virt_mem, PhysicalMemory* phys_mem, MMU* mmu) {
    int page_ptr = (addr / PAGE_SIZE);
    /*
    SEMPRE considerar que são 4 processos ocupando faixas de 16 páginas:
    PID 0 as páginas de 0 a 15; PID 64 as de 16 a 31;
    PID 128 as de 32 a 47; PID 192 as de 48 a 63.

    Ou seja, quando o comando passado é 
    <PID> L|E <endereço>
    ,o endereço se refere a um dos 65536 endereços que ele tem.
    Encontra a página correta com base no endereço e armazena em page_ptr.
    */
    switch(pid) {
        case 0:
            break;
        case 64:
            page_ptr += 16;
            break;
        case 128:
            page_ptr += 32;
            break;
        case 192:
            page_ptr += 48;
            break;
    }

    if (virt_mem->page_table[page_ptr].present == true) 
        page_hit_handler(virt_mem, phys_mem, mmu, mode, page_ptr);
    else {
        page_fault_handler(virt_mem, phys_mem, mmu, mode, page_ptr);
        rewrite_pgms(phys_mem_filename, virt_mem_filename, virt_mem, phys_mem);
    } 

    mmu->accesses++;
    // Zera os bitR
    if (mmu->accesses % 100 == 0) {
        for (int i = 0; i < VIRTUAL_PAGES; i++) virt_mem->page_table[i].bitR = false;
        if (page_replacement_algorithm == LRU2) {
            for (int i = 0; i < MEM_FRAMES; i++)
                for (int j = 0; j < MEM_FRAMES; j++)
                    mmu->lru_matrix[i][j] = false;
        }
    }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Impressões de conteúdo da memória 

void print_virtual() {
    FILE* f = fopen(virt_mem_filename, "r");
    if (f == NULL) { perror("..."); return; }
    char buffer[BUFFER_SIZE];
    
    while (fgets(buffer, BUFFER_SIZE, f) != NULL)
        printf("%s", buffer);
    fclose(f);
}

void print_physical() {
    FILE* f = fopen(phys_mem_filename, "r");
    if (f == NULL) { perror("..."); return; }
    char buffer[BUFFER_SIZE];

    while (fgets(buffer, BUFFER_SIZE, f) != NULL)
        printf("%s", buffer);
    fclose(f);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Parsers

int pgms_parser(char* phys_mem_pgm_f, char* virt_mem_pgm_f, VirtualMemory* virt_mem, PhysicalMemory* phys_mem) {
    char buffer[BUFFER_SIZE];

    // Memória Virtual

    FILE* virt_mem_f = fopen(virt_mem_pgm_f, "r");
    if (virt_mem_f == NULL) {
        perror("error opening virtual memory pgm file");
        return 0;
    }

    int i = 0;  // linhas totais lidas
    int j = 0;  // linhas de dados lidas (começa após o cabeçalho)

    while (fgets(buffer, BUFFER_SIZE, virt_mem_f) != NULL) {
        // Ignora linhas sem dados 
        if (buffer[0] == '\n' || buffer[0] == '\r' || buffer[0] == '\0')
            continue;
        if (i < 3) {
            i++;
            continue;
        }

        // Guarda dado na ordem inversa: j=0 => índice 63, j=63 => índice 0
        if (j < VIRTUAL_PAGES) {
            int idx = (VIRTUAL_PAGES - 1) - j;
            sscanf(buffer, "%hhu", &virt_mem->page_table[idx].page_frame_addr);
            virt_mem->page_table[idx].present = (virt_mem->page_table[idx].page_frame_addr != 255);
            virt_mem->page_table[idx].bitR    = false;
            virt_mem->page_table[idx].bitM    = false;
            j++;
        }

        i++;
    }

    fclose(virt_mem_f);

    // Memória Física

    FILE* phys_mem_f = fopen(phys_mem_pgm_f, "r");
    if (phys_mem_f == NULL) {
        perror("error opening physical memory pgm file");
        return 0;
    }

    i = 0;
    j = 0;

    while (fgets(buffer, BUFFER_SIZE, phys_mem_f) != NULL) {
        // Ignora linhas sem dados
        if (buffer[0] == '\n' || buffer[0] == '\r' || buffer[0] == '\0')
            continue;
        if (i < 3) {
            i++;
            continue;
        }

        // Guarda dado na ordem inversa: j=0 => índice 31, j=31 => índice 0
        if (j < MEM_FRAMES) {
            int idx = (MEM_FRAMES - 1) - j;
            sscanf(buffer, "%d", &phys_mem->page_frame[idx].pid);
            j++;
        }

        i++;
    }

    fclose(phys_mem_f);

    for (int i = 0; i < VIRTUAL_PAGES; i++) {
        if (virt_mem->page_table[i].present) {
            uint8_t frame = virt_mem->page_table[i].page_frame_addr;
            phys_mem->page_frame[frame].page_table_addr = (uint8_t) i;
        }
    }

    return 1;
}

int command_parser(char *cmd, VirtualMemory* virt_mem, PhysicalMemory* phys_mem, MMU* mmu) {
    if (cmd == NULL) return 0;      // EOF -> sair
    if (strlen(cmd) == 0) return 1; // linha vazia -> ignorar

    // Remove newline do fgets
    cmd[strcspn(cmd, "\n")] = '\0';
    
    // Tokenização do comando para separar os argumentos
    char cmd_copy[BUFFER_SIZE];
    strncpy(cmd_copy, cmd, BUFFER_SIZE - 1);
    cmd_copy[BUFFER_SIZE - 1] = '\0';

    char *args[BUFFER_SIZE];
    char *saveptr;

    char *token = strtok_r(cmd_copy, " \t", &saveptr);

    int i = 0;

    while (token != NULL && i < BUFFER_SIZE - 1) {
        args[i++] = token;
        token = strtok_r(NULL, " \t", &saveptr);
    }

    args[i] = NULL;

    // Verifica novamente se há argumentos
    if (i == 0) {
        return 1;
    }
    
    // Verifica comando 'sair'
    if (strcmp(args[0], "sair") == 0 && i == 1) {
        printf("Page faults: %d\n", mmu->page_faults);
        printf("Page hits: %d\n", mmu->page_hits);
        return 0;
    }

    switch (i) {
        case 2:
            if (strcmp(args[0], "imprime") == 0 &&
                strcmp(args[1], "virtual") == 0) {
                    print_virtual();
                    printf("\n");
                    return 1;
                }
            else if (strcmp(args[0], "imprime") == 0 &&
                     strcmp(args[1], "fisica") == 0) {
                        print_physical();
                        printf("\n");
                        return 1;
                     }
            break;
        case 3:
            if (strcmp(args[1], "L") == 0 ||
                strcmp(args[1], "E") == 0) {
                    int pid = atoi(args[0]);
                    char mode = args[1][0];
                    int addr = atoi(args[2]);

                    if (pid != 0 && pid != 64 && pid != 128 && pid != 192) {
                        fprintf(stderr, "ERROR: unexpected pid");
                        return 1;
                    }

                    if (addr >= MAX_ADDRESS_SPACES) {
                        fprintf(stderr, "ERROR: valid address space is between 0 and %d", MAX_ADDRESS_SPACES - 1);
                        return 1;
                    }

                    access_handler(pid, mode, addr, virt_mem, phys_mem, mmu);
                    printf("\n");
                    return 1;
                }
            break;
    }

    fprintf(stderr, "ERROR: invalid command");
    return 1;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Inicialização e liberação da MMU

void init_mmu(MMU* mmu) {
    mmu->accesses = 0;
    mmu->page_hits = 0;
    mmu->page_faults = 0;
    switch(page_replacement_algorithm) {
        case(SecondChance): 
            mmu->page_queue = (Queue*)malloc(sizeof(Queue)); 
            init_queue(mmu->page_queue);
            break; 
        case(LRU2):
            mmu->lru_matrix = malloc(MEM_FRAMES * sizeof(bool*));
            for (int i = 0; i < MEM_FRAMES; i++) 
                mmu->lru_matrix[i] = calloc(MEM_FRAMES, sizeof(bool));
            break;
    }
}

void free_mmu(MMU* mmu) {
    switch(page_replacement_algorithm) {
        case(SecondChance):
            free_queue(mmu->page_queue);
            free(mmu->page_queue);
            break; 
        case(LRU2):
            for (int i = 0; i < MEM_FRAMES; i++) 
                free(mmu->lru_matrix[i]);
            free(mmu->lru_matrix);
            break;
    }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "use: %s <arg1> <arg2> <arg3>\n", argv[0]);
        return 1;
    }

    VirtualMemory virt_mem;         // Declara a memória virtual
    PhysicalMemory phys_mem;        // Declara a memória física
    MMU mmu;                        // Declara a MMU

    virt_mem.page_table = calloc(VIRTUAL_PAGES, sizeof(page_table_entry));
    phys_mem.page_frame = calloc(MEM_FRAMES, sizeof(page_frame_entry));

    virt_mem_filename = argv[2];
    phys_mem_filename = argv[1];

    if (pgms_parser(phys_mem_filename, virt_mem_filename, &virt_mem, &phys_mem) != 1) {
        fprintf(stderr, "ERROR: pgm files with incorrect syntax\n");
        free(virt_mem.page_table);
        free(phys_mem.page_frame);

        return 1;
    }

    int n = atoi(argv[3]);
    page_replacement_algorithm = n;

    if (page_replacement_algorithm < NRU || page_replacement_algorithm > LRU2) {
        fprintf(stderr, "ERROR: invalid page replacement algorithm; use 1 for NRU, \
            2 for second chance, 3 for LRU using matrix.\n");
        return 1;
    }

    init_mmu(&mmu);

    // Interação com usuário via command_parser()
    char input[BUFFER_SIZE];
    int process_status;
    
    // Padrão do prompt
    printf("> ");
    while(fgets(input, BUFFER_SIZE, stdin) != NULL) {

        // Sai se Ctrl+D for pressionado
        if (input == NULL) {
            printf("\n");
            break;
        }

        process_status = command_parser(input, &virt_mem, &phys_mem, &mmu);
        // Ao parar de processar por entrada nula ou comando 'sair'
        if (process_status == 0) break;
        printf("> ");
    }

    // Libera memória alocada
    free(virt_mem.page_table);
    free(phys_mem.page_frame);
    free_mmu(&mmu);

    return 0;
}
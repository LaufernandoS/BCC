/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

EP2 - Simulador de Corridas Madison usando Barreira de Sincronização
MAC0422 - Sistemas Operacionais

Professor: Daniel Macedo Batista
Aluno: Laufernando Souza Dias

Entregue em 29/04/2026

Uso: ./ep2 <nº voltas> <metragem do velódromo> <nº de equipes> \
           <abordagem: i|e> [-debug] 

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "ep2.h"

// Variáveis Globais

Approach approach;                  // abordagem escolhida
Cyclist* cyclists;                  // vetor com ciclistas
pthread_mutex_t velodrome_mutex;    // Mutex para velódromo 
                                    // apenas para abordagem ingênua!
pthread_mutex_t* column_mutex;      // Mutexes para as colunas
                                    // apenas para abordagem eficiente!
pthread_barrier_t time_barrier;     // barreira de sincronização
int debug = 0;                      // flag de debug
atomic_int running_debug = 0;       // variável atômica para permitir 
                                    // execução de debug
long current_time = 0;              // relógio da simulação, sofre
                                    // acréscimos de 60 [[[em milissegundos]]]
int cyclists_count;                 // contador de ciclistas em prova
int race_finished = 0;              // flag de sinalização de que a 
                                    // corrida acabou
Cyclist *** velodrome;              // Matriz do velódromo, 
                                    // velodrome[i][j] == NULL <=> posição vazia,
                                    // se não, posição ocupada pelo ciclista do ponteiro
int control_lap = 0;                // volta atual para o controle da thread central
int n = 10, d = 100, k = 5;         // valores default

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

double ms_to_minutes(long ms) {
    return (double) ms / 60000;
}

int mod(int a, int b)
{
    int r = a % b;
    return r < 0 ? (r + b) : r;
}

void shuffle_teams(int *arr, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);

        int tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}

void draw_start_positions() {
    /* 
    Como a volta começa zerada, e o posicionamento começará em
    índice 0, considere como volta completa quando o ciclista passar
    por 'd - 1'; já começa a corrida fazendo lap++ ao passar de 0 para 
    d - 1 (linha de chegada, é a mesma que a largada).
    */
    int picked_cyclists = 0;

    // Prepara a permutação
    int teams[k];
    for (int i = 0; i < k; i++) {
        teams[i] = i + 1;
    }

    shuffle_teams(teams, k);
    // Ciclistas 'a'
    int team_ptr = 0;
    for (int column = 0; column < d && picked_cyclists < k; column++) {
        for (int track = 0; track < 5 && picked_cyclists < k; track++) {

            int team = teams[team_ptr];
            int base = 2 * team - 2;

            if (velodrome[track][column] == NULL) {

                cyclists[base].pos.x = track;
                cyclists[base].pos.y = column;
                velodrome[track][column] = &cyclists[base];

                picked_cyclists++;
                team_ptr++;
            }
        }
    }

    // Ciclistas 'b'
    // Reembaralha
    shuffle_teams(teams, k);
    team_ptr = 0;
    for (int column = 0; column < d && picked_cyclists < 2 * k; column += 2) {

        int team = teams[team_ptr];
        int base = 2 * team - 2;

        if (velodrome[EXTERNAL_TRACK][column] == NULL) {

            cyclists[base + 1].pos.x = EXTERNAL_TRACK;
            cyclists[base + 1].pos.y = column;
            velodrome[EXTERNAL_TRACK][column] = &cyclists[base + 1];

            picked_cyclists++;
            team_ptr++;
        }
    }
} 

int draw_new_speed(Cyclist* cyclist) {
    int x = rand() % 5;
    // 40% de ser 60, 60% de ser 30
    if (cyclist->speed == 60) return ((x < 2) + 1) * 30;
    // 80% de ser 60, 20% de ser 30
    else return ((x < 4) + 1) * 30;                      
}

void swap_cyclists(Cyclist* racing, Cyclist* recovering) {
    // Verificação defensiva — se dados inconsistentes, aborta silenciosamente
    if (racing->status == FINISHED || recovering->status == FINISHED)
        return;
    if (racing->pos.y != recovering->pos.y) return;
    if (velodrome[racing->pos.x][racing->pos.y] != racing) return;
    if (velodrome[recovering->pos.x][recovering->pos.y] != recovering) return;

    velodrome[racing->pos.x][racing->pos.y] = recovering;
    velodrome[recovering->pos.x][recovering->pos.y] = racing;

    Position tmp_pos = racing->pos;
    racing->pos = recovering->pos;
    recovering->pos = tmp_pos;
    
    // Troca campos relevantes
    int tmp_speed = racing->speed;
    racing->speed = recovering->speed;
    recovering->speed = tmp_speed;

    int tmp_quarter = racing->quarter;
    racing->quarter = recovering->quarter;
    recovering->quarter = tmp_quarter;

    // Troca status
    racing->status = RECOVERING;
    recovering->status = RACING;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

// Funções de Movimento

// Contagem de ciclistas numa mesma faixa (coluna)
int cyclists_in_column(int column) {
    int cyclists_in_column = 0;
    for (int track = 0; track < 9; track++)
        if (velodrome[track][column] != NULL) cyclists_in_column++;
    return cyclists_in_column;
}

void try_forward(Cyclist* c) {
    int next_column = mod((c->pos.y - 1), d);
    if (velodrome[c->pos.x][next_column] == NULL) {
        velodrome[c->pos.x][c->pos.y] = NULL;

        velodrome[c->pos.x][next_column] = c;
        c->pos.y = next_column;
    }
}

void try_overtaking(Cyclist* c) {
    int next_column = mod((c->pos.y - 1), d);
    int cyclists_next_column = cyclists_in_column(next_column);
    if (cyclists_next_column < 9) {
        velodrome[c->pos.x][c->pos.y] = NULL;

        velodrome[cyclists_next_column][next_column] = c;
        c->pos.x = cyclists_next_column;
        c->pos.y = next_column;
    }
}

void try_internal(Cyclist* c) {
    int curr_column = c->pos.y;
    for (int track = c->pos.x - 1; track >= 0; track--) {
        if (velodrome[track][curr_column] == NULL) {
            velodrome[c->pos.x][c->pos.y] = NULL;
            velodrome[track][curr_column] = c;
            c->pos.x = track;
            break;
        }
    }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

// Impressões

void print_lap_rel(int lap, int** lap_ranking) {
    printf("Lap %d ranking: \n", lap - 1);
    for (int i = 0; i < k; i++)
        printf("Pos. %d: team %d\n", i + 1, lap_ranking[lap - 1][i]);
    printf("\n");
}

void print_final_rel(int** lap_ranking) {
    printf("Final ranking: \n");
    for (int i = 0; i < k; i++) {
        int base = 2* (lap_ranking[n - 1][i]) - 2;

        if (base < 0) {
            printf("error in lap rankings\n");
            return;
        }

        int curr_team = cyclists[base].team;
        double last_lap_time_min = ms_to_minutes(cyclists[2*curr_team - 2].last_lap_time);
        int laps_won = cyclists[2*curr_team - 2].laps_won;

        printf("Pos. %d: team %d - time: %.2fmin - laps won: %d\n", i + 1, 
                curr_team, last_lap_time_min, laps_won);
    }
    printf("\n");
}

void print_debug() {
    for (int track = 9; track >= 0; track--) {
        for (int column = 0; column < d; column++) {
            if (velodrome[track][column] == NULL) fprintf(stderr, ". ");
            else {
                Cyclist* c = velodrome[track][column];
                fprintf(stderr, "%d%c", c->team, c->rider);
            }
        }
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");
}

int update_ranking(int* teams, int* current_lap, int** lap_ranking) {
    /* 
    São n linhas (voltas), k colunas (equipes)
    lap_ranking[i][j] = posição do time j na volta i + 1.

    current_lap verifica as voltas na thread de controle
    */
    int completed_lap = 0;

    for (int team_ptr = 0; team_ptr < k; team_ptr++) {
        int team = teams[team_ptr];
        int base = 2 * team - 2;

        int lap_a = atomic_load(&cyclists[base].lap);
        int lap_b = atomic_load(&cyclists[base + 1].lap);

        if (lap_a > current_lap[base] ||
            lap_b > current_lap[base + 1]) {
                
            int new_lap = MAX(lap_a, lap_b);
            current_lap[base] = new_lap;
            current_lap[base+1] = new_lap;
            // Primeiro verifica se vale a pena processar  

            if (new_lap < 2) continue;

            // Atualiza o tempo de finalização da volta
            cyclists[base].last_lap_time = current_time;
            cyclists[base + 1].last_lap_time = current_time;
            
            int *p = lap_ranking[new_lap - 2];
            int *end = p + k;

            // Encaixa o time na colocação daquela volta
            while (p < end) {
                if (*p == 0) {
                    *p = team;
                    // Vencedor da volta
                    if (p == lap_ranking[new_lap - 2]) {
                        cyclists[base].laps_won++;
                        cyclists[base + 1].laps_won++;
                    }
                    // Chegou no último colocado da volta
                    if (p == end - 1)
                        return completed_lap = new_lap - 1;
                    break;
                }
                p++;
            }
        }
    }

    return completed_lap;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

// Threads

void* cyclist_thread(void* arg) {
    Cyclist* c = (Cyclist*) arg;
    Cyclist* teammate;

    if (c->rider == 'a') teammate = &cyclists[2*c->team - 1];
    else teammate = &cyclists[2*c->team - 2];

    while(1) {
        if (debug)
            while (atomic_load(&running_debug) == 1) continue;
        
        if (c->status != FINISHED) {
            Position prev_pos = c->pos;
            int prev_lap = atomic_load(&c->lap);

            int curr_column = c->pos.y;
            int req_column = mod((c->pos.y - 1), d);
            int first, second;
            
            if (curr_column != 0) {
                first = MIN(curr_column, req_column);
                second = MAX(curr_column, req_column);
            }
            else {
                first = MAX(curr_column, req_column);
                second = MIN(curr_column, req_column);
            }

            // Eficiente pega o mutex da coluna atual e da seguinte
            if (approach == EFFICIENT) {
                pthread_mutex_lock(&column_mutex[first]);
                pthread_mutex_lock(&column_mutex[second]);
            }
            else pthread_mutex_lock(&velodrome_mutex);
    
            // Contabiliza o quanto andou em 60 ms
            if (c->speed == 15) c->quarter += 1;
            if (c->speed == 30) c->quarter += 2;
            // 1: tenta avançar na pista atual 
            if (c->speed == 60 || c->quarter % 4 == 0) {
                try_forward(c);
                if (c->speed != 60) c->quarter = 0;
            }
            
            // 2: tenta fazer ultrapassagem, avançando p/ próxima coluna
            if (c->speed == 60 && c->pos.y == prev_pos.y) try_overtaking(c);

            // 3: quem corre tenta descer para pista mais interna
            if (c->status == RACING) try_internal(c);
    
            // 4: mais veloz lida com o não avanço
            if (c->speed == 60 && c->pos.y == prev_pos.y) c->quarter += 2;
            
            // 5: lida com mudança de volta e sorteia nova velocidade 
            // As duas condições garantem que roda esse if somente uma vez
            if (c->status == RACING) {
                if (c->pos.y == d-1 && prev_pos.y != d-1) {
                    atomic_fetch_add(&c->lap, 1);
                    atomic_fetch_add(&teammate->lap, 1);
                    c->speed = draw_new_speed(c);
                    
                    if (atomic_load(&c->lap) > n) {
                        c->status = FINISHED;
                        teammate->status = FINISHED;
    
                        velodrome[c->pos.x][c->pos.y] = NULL;
    
                        atomic_fetch_add(&c->finished_flag, 1);
                        atomic_fetch_add(&teammate->finished_flag, 1);
                    }
                    else if (atomic_load(&c->lap) % 5 == 0) c->swap_flag = 1;
                }
                // Quem corre é responsável por chamar a troca
                if (c->swap_flag == 1 && c->pos.y == teammate->pos.y) {
                    swap_cyclists(c, teammate);
                    c->swap_flag = 0;
                }
            }

            if (approach == EFFICIENT) {
                pthread_mutex_unlock(&column_mutex[first]);
                pthread_mutex_unlock(&column_mutex[second]);
            }
            else pthread_mutex_unlock(&velodrome_mutex);
        }

        // Garante que retirou o marcador de posição
        else if (velodrome[c->pos.x][c->pos.y] != NULL)
            velodrome[c->pos.x][c->pos.y] = NULL;

        // Sincronização 1
        pthread_barrier_wait(&time_barrier);

        // Sincronização 2
        pthread_barrier_wait(&time_barrier);

        // Se o controle mudou a flag da corrida
        if (race_finished == 1) break;
    }

    return NULL;
}

void* control_thread(void* arg) {
    int start_count = cyclists_count;
    
    int control_lap = 0;
    int *current_lap = calloc(start_count, sizeof(int));  // trackeia em qual volta está cada ciclista
    int **lap_ranking = malloc(n * sizeof(int*));         // n vetores para o ranqueamento em uma volta (n voltas)
    for (int i = 0; i < n; i++) {
        lap_ranking[i] = calloc(k, sizeof(int));
    }

    // Prepara uma permutação
    int teams[k];
    for (int i = 0; i < k; i++) {
        teams[i] = i + 1;
    }
    // Se houver empate, a ordem de iteração na atualização de ranking já é aleatória!
    shuffle_teams(teams, k);

    pthread_barrier_init(&time_barrier, NULL, start_count + 1);

    while (1) {
        // 1: ciclistas andam 1 passo de forma concorrente 
        // 2: avança o relógio em 60ms;
        current_time += 60;

        // Sincronização 1
        pthread_barrier_wait(&time_barrier);

        if (debug) {
            atomic_fetch_add(&running_debug, 1);
            print_debug();
            atomic_fetch_sub(&running_debug, 1);
        }

        // 3: marca as threads de ciclistas que precisam ser destruı́das
        for (int i = 0; i < 2*k; i++) {
            if (cyclists[i].status == FINISHED && atomic_load(&cyclists[i].finished_flag)) {
                atomic_fetch_sub(&cyclists[i].finished_flag, 1);
                cyclists_count--;
            }
        }

        // 4: imprima as informações na tela; se terminou de preencher uma volta, imprima-a
        control_lap = update_ranking(teams, current_lap, lap_ranking);
        if (control_lap != 0 && !debug)
            print_lap_rel(control_lap, lap_ranking);

        if (cyclists_count == 0) {
            race_finished = 1;
        }

        // Sincronização 2
        pthread_barrier_wait(&time_barrier);

        if (race_finished == 1) break;
    }

    pthread_barrier_destroy(&time_barrier);
    for (int i = 0; i < 2*k; i++) pthread_join(cyclists[i].thread, NULL);

    print_final_rel(lap_ranking);
    free(current_lap);
    for (int i = 0; i < n; i++) free(lap_ranking[i]);
    free(lap_ranking);

    return NULL;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

void init_cyclists() {
    /* 
    Os times vão de 1 a k (inclusivo)
    Ou seja... time 1 = {ciclista 0, ciclista 1},
               time 2 = {ciclista 2, ciclista 3},
    ...até     time k = {ciclista 2k-2, ciclista 2k-1},
    */
    for (int i = 0; i < 2*k; i++) {
        cyclists[i].quarter = 0;
        cyclists[i].swap_flag = 0;
        cyclists[i].lap = 0;
        cyclists[i].finished_flag = 0;
        cyclists[i].pos.x = -1; cyclists[i].pos.y = -1;
        cyclists[i].laps_won = 0;

        // Define time (1 a k) e diferenciador de corredor
        cyclists[i].team = i / 2 + 1;
        if (i % 2 == 0) {
            cyclists[i].rider = 'a';
            cyclists[i].speed = 30;
            cyclists[i].status = RACING;
        }
        else {
            cyclists[i].rider = 'b';
            cyclists[i].speed = 15;
            cyclists[i].status = RECOVERING;
        } 
    }

    // Define posições iniciais aleatoriamente
    draw_start_positions();
    usleep(1000);

    // Cria as threads
    for (int i = 0; i < 2*k; i++) {
        pthread_create(&cyclists[i].thread, NULL, cyclist_thread, &cyclists[i]);
    }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

int main(int argc, char *argv[]) {
    char line[BUFFER_SIZE];

    if (argc < 5) {
        printf("use: %s <arg1> <arg2> <arg3> <arg4> [-debug]\n", argv[0]);
        return 1;
    }

    if (argc == 6 && strcmp(argv[5], "-debug") == 0) {
        debug = 1;
    }

    // seed para random, pode ser alterada
    srand(time(NULL)); 

    // Seta os argumentos da linha de comando
    n = atoi(argv[1]);
    d = atoi(argv[2]);
    k = atoi(argv[3]);

    if (strcmp(argv[3], "i") == 0) approach = NAIVE;
    else if (strcmp(argv[3], "e") == 0) approach = EFFICIENT;

    // Inicializa o velódromo
    velodrome = malloc(10 * sizeof(Cyclist**));
    for (int i = 0; i < 10; i++) {
        velodrome[i] = malloc(d * sizeof(Cyclist*));
        memset(velodrome[i], 0, d * sizeof(Cyclist*));
    }

    // Inicializa os ciclistas
    cyclists_count = 2*k;
    cyclists = malloc(cyclists_count * sizeof(Cyclist));

    // Inicializa mutexes
    column_mutex = malloc(d * sizeof(pthread_mutex_t));
    for (int i = 0; i < d; i++) pthread_mutex_init(&column_mutex[i], NULL);
    pthread_mutex_init(&velodrome_mutex, NULL);

    // Inicializa a thread controle de acordo com abordagem
    pthread_t control;
    pthread_create(&control, NULL, control_thread, NULL);
    // Inicializa os ciclistas e suas threads
    init_cyclists();

    // Aguarda a thread do controle terminar
    pthread_join(control, NULL);

    // Libera memória de velódromo, ciclistas, e mutexes
    for (int i = 0; i < 10; i++) free(velodrome[i]);
    free(velodrome);
    free(cyclists);
    free(column_mutex);

    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

#define MAX_COMMAND_LENGTH 1024
#define MAX_ARGS 6  // pathname + até 5 argumentos

// Estrutura para nó da lista ligada
typedef struct process_node {
    pid_t pid;
    time_t start_time;
    time_t end_time;
    int finished;  // 0 = em execução, 1 = terminado
    struct process_node* next;
} process_node_t;

// Estrutura de dados compartilhada entre threads
typedef struct {
    process_node_t* head;      // Cabeça da lista ligada
    int numChildren;           // Número de processos em execução
    int exit_requested;        // Flag para solicitar término
    pthread_mutex_t mutex;     // Mutex para proteção da lista
    pthread_cond_t cond;       // Condição para sincronização
} shared_data_t;

shared_data_t shared_data;

// Função para ler uma linha do stdin
char* read_line() {
    char* line = NULL;
    size_t size = 0;
    ssize_t read;
    
    read = getline(&line, &size, stdin);
    if (read == -1) {
        free(line);
        return NULL;
    }
    
    return line;
}

// Função para adicionar processo à lista ligada
void add_process(pid_t pid, time_t start_time) {
    pthread_mutex_lock(&shared_data.mutex);
    
    // Cria novo nó
    process_node_t* new_node = (process_node_t*)malloc(sizeof(process_node_t));
    if (new_node == NULL) {
        fprintf(stderr, "Erro: Falha ao alocar memória\n");
        pthread_mutex_unlock(&shared_data.mutex);
        return;
    }
    
    new_node->pid = pid;
    new_node->start_time = start_time;
    new_node->end_time = 0;
    new_node->finished = 0;
    new_node->next = shared_data.head;
    
    // Insere no início da lista
    shared_data.head = new_node;
    shared_data.numChildren++;
    
    pthread_mutex_unlock(&shared_data.mutex);
}

// Função para marcar processo como terminado
void mark_process_finished(pid_t pid, time_t end_time) {
    pthread_mutex_lock(&shared_data.mutex);
    
    process_node_t* current = shared_data.head;
    while (current != NULL) {
        if (current->pid == pid && !current->finished) {
            current->end_time = end_time;
            current->finished = 1;
            shared_data.numChildren--;
            break;
        }
        current = current->next;
    }
    
    pthread_mutex_unlock(&shared_data.mutex);
}

// Função para obter todos os processos terminados e imprimir
void print_all_processes() {
    pthread_mutex_lock(&shared_data.mutex);
    
    printf("\n=== Histórico de Processos ===\n");
    process_node_t* current = shared_data.head;
    while (current != NULL) {
        if (current->finished) {
            double execution_time = difftime(current->end_time, current->start_time);
            printf("pid=%d, tempo=%ld segundos (%.2f segundos)\n", 
                   current->pid, 
                   (long)execution_time,
                   execution_time);
        }
        current = current->next;
    }
    printf("==============================\n\n");
    
    pthread_mutex_unlock(&shared_data.mutex);
}

// Função para liberar a memória da lista ligada
void free_process_list() {
    pthread_mutex_lock(&shared_data.mutex);
    
    process_node_t* current = shared_data.head;
    while (current != NULL) {
        process_node_t* next = current->next;
        free(current);
        current = next;
    }
    shared_data.head = NULL;
    
    pthread_mutex_unlock(&shared_data.mutex);
}

// Tarefa monitora (thread)
void* monitor_thread(void* arg) {
    pid_t pid;
    int status;
    time_t end_time;
    (void)arg;  // Para evitar warning de parâmetro não utilizado
    
    while (1) {
        pthread_mutex_lock(&shared_data.mutex);
        int has_children = (shared_data.numChildren > 0);
        int should_exit = shared_data.exit_requested;
        pthread_mutex_unlock(&shared_data.mutex);
        
        // Se não há processos filhos e exit foi solicitado, termina
        if (should_exit && !has_children) {
            break;
        }
        
        // Se não há processos filhos, espera 1 segundo
        if (!has_children) {
            sleep(1);
            continue;
        }
        
        // Há processos filhos, espera por um deles terminar
        pid = wait(&status);
        
        if (pid > 0) {
            end_time = time(NULL);
            
            int exit_code;
            if (WIFEXITED(status)) {
                exit_code = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                exit_code = WTERMSIG(status);
                fprintf(stderr, "[%d] Terminado por sinal %d\n", pid, exit_code);
            } else {
                exit_code = -1;
            }
            
            mark_process_finished(pid, end_time);
            
            // Imprime mensagem de término (exceto durante exit)
            pthread_mutex_lock(&shared_data.mutex);
            int exiting = shared_data.exit_requested;
            pthread_mutex_unlock(&shared_data.mutex);
            
            if (!exiting) {
                process_node_t* current;
                pthread_mutex_lock(&shared_data.mutex);
                current = shared_data.head;
                while (current != NULL) {
                    if (current->pid == pid && current->finished) {
                        double exec_time = difftime(current->end_time, current->start_time);
                        printf("[%d] terminou com código %d (tempo: %.2f segundos)\n", 
                               pid, exit_code, exec_time);
                        break;
                    }
                    current = current->next;
                }
                pthread_mutex_unlock(&shared_data.mutex);
                fflush(stdout);
            }
        } else if (pid == -1 && errno != EINTR) {
            perror("wait");
            break;
        }
    }
    
    return NULL;
}

// Função para executar um comando pathname com argumentos
int execute_pathname(char* pathname, char* args[]) {
    time_t start_time;
    pid_t pid;
    
    // Verifica se o número de argumentos excede o limite
    int arg_count = 0;
    while (args[arg_count] != NULL) {
        arg_count++;
    }
    
    if (arg_count > MAX_ARGS) {
        fprintf(stderr, "Erro: Máximo de %d argumentos permitidos\n", MAX_ARGS - 1);
        return -1;
    }
    
    // Verifica se o arquivo executável existe e é acessível
    if (access(pathname, X_OK) != 0) {
        fprintf(stderr, "Erro: Não foi possível executar '%s' - %s\n", 
                pathname, strerror(errno));
        return -1;
    }
    
    // Fork para criar processo filho
    pid = fork();
    
    if (pid == -1) {
        fprintf(stderr, "Erro: Falha ao criar processo filho - %s\n", strerror(errno));
        return -1;
    } else if (pid == 0) {
        // Processo filho - executa o programa
        execv(pathname, args);
        
        // Se chegar aqui, execv falhou
        fprintf(stderr, "Erro: Falha ao executar '%s' - %s\n", pathname, strerror(errno));
        exit(127);
    } else {
        // Processo pai - registra o processo
        start_time = time(NULL);
        add_process(pid, start_time);
        printf("[%d] %s", pid, pathname);
        
        // Imprime os argumentos se houver
        if (arg_count > 1) {
            printf(" ");
            for (int i = 1; i < arg_count; i++) {
                printf("%s ", args[i]);
            }
        }
        printf("\n");
        fflush(stdout);
        return 0;
    }
}

int main(void) {
    pthread_t monitor;
    char* line;
    char* args[MAX_ARGS + 1];  // +1 para o NULL
    char* pathname;
    int should_exit = 0;
    
    printf("=== cpd-terminal (Etapa 1 e 2) ===\n");
    printf("Comandos disponíveis:\n");
    printf("  - pathname [arg1 arg2 ...] - Executa programa em background (max 5 args)\n");
    printf("  - exit - Termina o programa aguardando todos os processos\n");
    printf("================================\n\n");
    
    // Inicializa estrutura de dados compartilhada
    shared_data.head = NULL;
    shared_data.numChildren = 0;
    shared_data.exit_requested = 0;
    pthread_mutex_init(&shared_data.mutex, NULL);
    pthread_cond_init(&shared_data.cond, NULL);
    
    // Cria thread monitora
    if (pthread_create(&monitor, NULL, monitor_thread, NULL) != 0) {
        perror("pthread_create");
        exit(1);
    }
    
    // Loop principal
    while (!should_exit) {
        printf("cpd-terminal> ");
        fflush(stdout);
        
        // Lê linha de comando
        line = read_line();
        if (line == NULL) {
            clearerr(stdin);
            continue;
        }
        
        // Remove newline se existir
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        // Se linha vazia, continua
        if (strlen(line) == 0) {
            free(line);
            continue;
        }
        
        // Comando exit
        if (strcmp(line, "exit") == 0) {
            should_exit = 1;
            free(line);
            break;
        }
        
        // Processa comando pathname
        // Divide a linha em argumentos (usando espaços como separadores)
        int arg_count = 0;
        char* token = strtok(line, " ");
        while (token != NULL && arg_count < MAX_ARGS) {
            args[arg_count++] = token;
            token = strtok(NULL, " ");
        }
        args[arg_count] = NULL;
        
        if (arg_count > 0) {
            pathname = args[0];
            execute_pathname(pathname, args);
        }
        
        free(line);
    }
    
    // Exit solicitado - informa thread monitora
    printf("\nexit\n");
    printf("Aguardando término de todos os processos filhos...\n");
    
    pthread_mutex_lock(&shared_data.mutex);
    shared_data.exit_requested = 1;
    pthread_mutex_unlock(&shared_data.mutex);
    
    // Aguarda thread monitora terminar
    pthread_join(monitor, NULL);
    
    // Imprime informações completas de todos os processos
    print_all_processes();
    
    // Libera memória
    free_process_list();
    pthread_mutex_destroy(&shared_data.mutex);
    pthread_cond_destroy(&shared_data.cond);
    
    printf("cpd-terminal terminado.\n");
    
    return 0;
}
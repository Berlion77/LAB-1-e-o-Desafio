#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_PRIORITY 5
#define MIN_PRIORITY 0
#define MAX_ID_LEN 100

// Estrutura para uma tarefa
typedef struct task {
    char id[MAX_ID_LEN];
    int priority;
    time_t creation_time;  // Para ordenar por data de criação
    struct task *next;
} task_t;

// Estrutura para o gestor de tarefas
typedef struct {
    task_t *priorities[MAX_PRIORITY + 1];  // Array de listas (0 a 5)
} task_manager_t;

// Inicializar o gestor de tarefas
void init_task_manager(task_manager_t *manager) {
    for (int i = 0; i <= MAX_PRIORITY; i++) {
        manager->priorities[i] = NULL;
    }
}

// Função para verificar se o ID já existe em qualquer prioridade
int id_exists(task_manager_t *manager, const char *id) {
    for (int p = 0; p <= MAX_PRIORITY; p++) {
        task_t *current = manager->priorities[p];
        while (current != NULL) {
            if (strcmp(current->id, id) == 0) {
                return 1;  // ID encontrado
            }
            current = current->next;
        }
    }
    return 0;  // ID não encontrado
}

// Comando: new <prioridade> <id-nova-tarefa>
void new_task(task_manager_t *manager, int priority, const char *id) {
    // Validar prioridade
    if (priority < MIN_PRIORITY || priority > MAX_PRIORITY) {
        printf("ERRO: Prioridade inválida. Deve estar entre %d e %d.\n", 
               MIN_PRIORITY, MAX_PRIORITY);
        return;
    }
    
    // Verificar se ID já existe
    if (id_exists(manager, id)) {
        printf("ERRO: Já existe uma tarefa com o ID '%s'.\n", id);
        return;
    }
    
    // Criar nova tarefa
    task_t *new_task = (task_t*)malloc(sizeof(task_t));
    if (new_task == NULL) {
        printf("ERRO: Falha ao alocar memória.\n");
        return;
    }
    
    strcpy(new_task->id, id);
    new_task->priority = priority;
    new_task->creation_time = time(NULL);  // Data/hora atual
    new_task->next = NULL;
    
    // Inserir na lista apropriada (manter ordenado por data - mais recentes primeiro)
    task_t **current = &(manager->priorities[priority]);
    
    // Inserir no início (para manter mais recentes primeiro)
    new_task->next = *current;
    *current = new_task;
    
    printf("Tarefa '%s' criada com prioridade %d.\n", id, priority);
}

// Função auxiliar para listar tarefas de uma prioridade específica
void list_priority(task_t *list, int target_priority, int show_priority) {
    task_t *current = list;
    while (current != NULL) {
        if (show_priority) {
            printf("  [%d] %s\n", current->priority, current->id);
        } else {
            printf("  %s\n", current->id);
        }
        current = current->next;
    }
}

// Comando: list <prioridade>
void list_tasks(task_manager_t *manager, int min_priority) {
    if (min_priority < MIN_PRIORITY || min_priority > MAX_PRIORITY) {
        printf("ERRO: Prioridade inválida. Deve estar entre %d e %d.\n", 
               MIN_PRIORITY, MAX_PRIORITY);
        return;
    }
    
    int found = 0;
    printf("Tarefas com prioridade >= %d:\n", min_priority);
    
    // Listar das prioridades mais altas para as mais baixas
    for (int p = MAX_PRIORITY; p >= min_priority; p--) {
        if (manager->priorities[p] != NULL) {
            printf("Prioridade %d:\n", p);
            list_priority(manager->priorities[p], p, 0);
            found = 1;
        }
    }
    
    if (!found) {
        printf("  Nenhuma tarefa encontrada.\n");
    }
}

// Comando: complete <id-tarefa>
void complete_task(task_manager_t *manager, const char *id) {
    // Procurar em todas as listas de prioridade
    for (int p = 0; p <= MAX_PRIORITY; p++) {
        task_t **current = &(manager->priorities[p]);
        
        while (*current != NULL) {
            if (strcmp((*current)->id, id) == 0) {
                // Tarefa encontrada - remover da lista
                task_t *to_remove = *current;
                *current = (*current)->next;
                free(to_remove);
                printf("Tarefa '%s' concluída e removida.\n", id);
                return;
            }
            current = &((*current)->next);
        }
    }
    
    // Se chegou aqui, a tarefa não foi encontrada
    printf("TAREFA INEXISTENTE\n");
}

// Função para liberar toda a memória
void cleanup(task_manager_t *manager) {
    for (int p = 0; p <= MAX_PRIORITY; p++) {
        task_t *current = manager->priorities[p];
        while (current != NULL) {
            task_t *temp = current;
            current = current->next;
            free(temp);
        }
        manager->priorities[p] = NULL;
    }
}

// Função para processar comandos interativamente
void process_commands(task_manager_t *manager) {
    char line[256];
    char command[20];
    
    printf("\n=== Gestor de Tarefas Pessoais ===\n");
    printf("Comandos disponíveis:\n");
    printf("  new <prioridade> <id>\n");
    printf("  list <prioridade>\n");
    printf("  complete <id>\n");
    printf("  exit\n\n");
    
    while (1) {
        printf("> ");
        if (fgets(line, sizeof(line), stdin) == NULL) {
            break;
        }
        
        // Remover newline
        line[strcspn(line, "\n")] = 0;
        
        // Parse do comando
        sscanf(line, "%s", command);
        
        if (strcmp(command, "new") == 0) {
            int priority;
            char id[MAX_ID_LEN];
            if (sscanf(line, "%*s %d %s", &priority, id) == 2) {
                new_task(manager, priority, id);
            } else {
                printf("Uso: new <prioridade> <id>\n");
            }
        }
        else if (strcmp(command, "list") == 0) {
            int priority;
            if (sscanf(line, "%*s %d", &priority) == 1) {
                list_tasks(manager, priority);
            } else {
                printf("Uso: list <prioridade>\n");
            }
        }
        else if (strcmp(command, "complete") == 0) {
            char id[MAX_ID_LEN];
            if (sscanf(line, "%*s %s", id) == 1) {
                complete_task(manager, id);
            } else {
                printf("Uso: complete <id>\n");
            }
        }
        else if (strcmp(command, "exit") == 0) {
            printf("A encerrar o gestor de tarefas...\n");
            break;
        }
        else if (strlen(command) > 0) {
            printf("Comando desconhecido. Comandos: new, list, complete, exit\n");
        }
    }
}

int main() {
    task_manager_t manager;
    init_task_manager(&manager);
    
    process_commands(&manager);
    
    cleanup(&manager);
    return 0;
}
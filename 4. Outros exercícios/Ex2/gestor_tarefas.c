#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct tarefa {
    char id[50];
    int prioridade;
    int timestamp; // para ordenar por criação
    struct tarefa *next;
} Tarefa;

Tarefa* criar_tarefa(Tarefa *lista[], char *id, int prioridade, int tempo) {
    Tarefa *novo = (Tarefa*) malloc(sizeof(Tarefa));
    strcpy(novo->id, id);
    novo->prioridade = prioridade;
    novo->timestamp = tempo;

    // Inserir no início da lista da prioridade
    novo->next = lista[prioridade];
    lista[prioridade] = novo;

    return novo;
}

void listar(Tarefa *lista[], int prioridade_min) {
    printf("Tarefas (prioridade >= %d):\n", prioridade_min);

    for (int p = 5; p >= prioridade_min; p--) { // prioridade mais alta primeiro
        Tarefa *atual = lista[p];
        while (atual != NULL) {
            printf("ID: %s | Prioridade: %d | Timestamp: %d\n",
                   atual->id, atual->prioridade, atual->timestamp);
            atual = atual->next;
        }
    }
}

int completar(Tarefa *lista[], char *id) {
    for (int p = 0; p <= 5; p++) { // percorrer todas as listas
        Tarefa *atual = lista[p];
        Tarefa *ant = NULL;

        while (atual != NULL) {
            if (strcmp(atual->id, id) == 0) {
                if (ant == NULL) {
                    lista[p] = atual->next;
                } else {
                    ant->next = atual->next;
                }
                free(atual);
                return 1; // sucesso
            }
            ant = atual;
            atual = atual->next;
        }
    }

    printf("TAREFA INEXISTENTE\n");
    return 0; // não encontrada
}

int main() {
    Tarefa *listas[6] = {NULL}; // 6 níveis de prioridade
    int timestamp = 0; // para controlar ordem de criação

    char comando[20], id[50];
    int prioridade;

    while (1) {
        printf("Comando: ");
        if (scanf("%s", comando) != 1) break;

        if (strcmp(comando, "new") == 0) {
            scanf("%d %s", &prioridade, id);
            if (prioridade < 0 || prioridade > 5) {
                printf("Prioridade inválida!\n");
                continue;
            }
            criar_tarefa(listas, id, prioridade, timestamp++);
        } else if (strcmp(comando, "list") == 0) {
            scanf("%d", &prioridade);
            if (prioridade < 0 || prioridade > 5) prioridade = 0;
            listar(listas, prioridade);
        } else if (strcmp(comando, "complete") == 0) {
            scanf("%s", id);
            completar(listas, id);
        } else {
            printf("Comando desconhecido!\n");
        }
    }

    return 0;
}
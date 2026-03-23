#include <stdio.h>
#include <stdlib.h>

typedef struct process {
    int pid;
    double start_time;
    double end_time;
    struct process *next;
} process_t;

void update_terminated_process(process_t *list, int pid, double end_time) {
    process_t *current = list;
    
    while (current != NULL) {
        if (current->pid == pid) {
            current->end_time = end_time;
            printf("Processo %d terminado em %.2f\n", pid, end_time);
            return;
        }
        current = current->next;
    }
    
    printf("Aviso: Processo %d não encontrado na lista\n", pid);
}

// Função auxiliar para criar um novo processo
process_t* create_process(int pid, double start_time) {
    process_t *new_process = (process_t*)malloc(sizeof(process_t));
    new_process->pid = pid;
    new_process->start_time = start_time;
    new_process->end_time = -1;
    new_process->next = NULL;
    return new_process;
}

// Função para imprimir a lista
void print_process_list(process_t *list) {
    process_t *current = list;
    while (current != NULL) {
        printf("PID: %d, Start: %.2f, End: %.2f\n", 
               current->pid, current->start_time, current->end_time);
        current = current->next;
    }
}

int main() {
    // Criar uma lista de exemplo
    process_t *head = create_process(1001, 0.0);
    head->next = create_process(1002, 1.5);
    head->next->next = create_process(1003, 3.2);
    
    printf("Antes da atualização:\n");
    print_process_list(head);
    
    // Atualizar o processo com PID 1002
    update_terminated_process(head, 1002, 5.8);
    
    printf("\nDepois da atualização:\n");
    print_process_list(head);
    
    // Liberar memória
    process_t *current = head;
    while (current != NULL) {
        process_t *temp = current;
        current = current->next;
        free(temp);
    }
    
    return 0;
}
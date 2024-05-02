#include <stdio.h>
#include <stdlib.h>
#include "queue.h"
#include "ppos.h"

// operating system check
#if defined(_WIN32) || (!defined(__unix__) && !defined(__unix) && (!defined(__APPLE__) || !defined(__MACH__)))
#warning Este codigo foi planejado para ambientes UNIX (LInux, *BSD, MacOS). A compilacao e execucao em outros ambientes e responsabilidade do usuario.
#endif

#define NUM_PRODUTORES 3
#define NUM_CONSUMIDORES 2
#define TAM_BUFFER 5

/*
 * Buffer
 * Note que essas funções não controlam o sistema produtor-consumidor.
 * Essa é a função dos semáforos.
 */

typedef struct Buffer {
    int array[TAM_BUFFER];
    int i;
} Buffer;

void bufferInit(Buffer* buffer) {
    buffer->i = 0;
}

void bufferPlace(Buffer* buffer, int item) {
    buffer->array[buffer->i] = item;
    ++(buffer->i);
}

int bufferRetrieve(Buffer* buffer) {
    int j;
    int item = buffer->array[0];

    for (j = 1; j < buffer->i; j++) {
        buffer->array[j-1] = buffer->array[j];
    }
    
    --(buffer->i);
    return item;
}

/*
 * Programa principal
 */

task_t produtor[NUM_PRODUTORES];
task_t consumidor[NUM_CONSUMIDORES];

semaphore_t sBuffer;
semaphore_t sItem;
semaphore_t sVaga;

Buffer b;

void taskProdutor(void* id) {
    int* tid = (int*)id;
    int num = *tid - 2;
    int item = 0;
    
    while (1) {
        task_sleep(1);
        item = random() % 100;
        
        sem_down(&sVaga);
        sem_down(&sBuffer);
        
        bufferPlace(&b, item);
        
        sem_up(&sBuffer);
        sem_up(&sItem);
        
        printf("p%d produziu %d\n", num, item);
    }
    
    task_exit(0);
}

void taskConsumidor(void* id) {
    int* tid = (int*)id;
    int num = *tid - 2 - NUM_PRODUTORES;
    int item;
    
    while (1) {
        sem_down(&sItem);
        sem_down(&sBuffer);
        
        item = bufferRetrieve(&b);
        
        sem_up(&sBuffer);
        sem_up(&sVaga);
        
        printf("                    c%d consumiu %d\n", num, item);
        task_sleep(1);
    }
    
    task_exit(0);
}

int main (int argc, char *argv[]) {
    int i;
    
    ppos_init();
    
    bufferInit(&b);
    
    sem_create(&sBuffer, 1);
    sem_create(&sItem, 0);
    sem_create(&sVaga, 5);
    
    for (i = 0; i < NUM_PRODUTORES; ++i) {
        task_create(&(produtor[i]), taskProdutor, &(produtor[i].id));
    }
    
    for (i = 0; i < NUM_CONSUMIDORES; ++i) {
        task_create(&(consumidor[i]), taskConsumidor, &(consumidor[i].id));
    }
    
    task_exit(0);
    exit(0);
}

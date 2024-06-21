#include "ppos_disk.h" //   gcc -o main ppos_disk.c disk.c ppos.h pingpong-disco1.c ppos-core-aux.c libppos_static.a -lrt   COMANDO
#include <stdio.h>     //   gcc -o main disk.c ppos.h pingpong-disco1.c ppos-core-aux.c libppos_static.a -lrt
#include <stdlib.h>
#include <signal.h>

// Variáveis globais do gerenciador de disco
int num_blocks;               // Número de blocos do disco
int block_size;               // Tamanho de cada bloco em bytes
int disk_signal_received = 0; // Sinal do disco recebido
semaphore_t disk_semaphore;   // Semáforo para controle de acesso ao disco
task_t *disk_manager_task;    // Tarefa do gerente de disco
task_t *task_queue = NULL;
; // Tarefa de fila

// Prototipo das funções
void diskDriverBody(void *args);
void disk_signal_handler(int signal);

int disk_mgr_init(int *numBlocks, int *blockSize)
{

    // Inicializa o disco
    int result = disk_cmd(DISK_CMD_INIT, 0, 0);
    if (result < 0)
    {
        return -1; // Erro na inicialização do disco
    }

    // Consulta o tamanho do disco em blocos
    result = disk_cmd(DISK_CMD_DISKSIZE, 0, 0);
    if (result < 0)
    {
        return -1; // Erro ao consultar o tamanho do disco
    }
    *numBlocks = result;

    // Consulta o tamanho de cada bloco do disco em bytes
    result = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);
    if (result < 0)
    {
        return -1; // Erro ao consultar o tamanho do bloco
    }
    *blockSize = result;

    disk_manager_task = (task_t *)malloc(sizeof(task_t));
    if (!disk_manager_task)
    {
        return -1; // Erro ao alocar memória para o gerente de disco
    }

    // Inicializa o semáforo de acesso ao disco
    sem_create(&disk_semaphore, 1);

    // Cria a tarefa do gerente de disco
    task_create(disk_manager_task, diskDriverBody, NULL);

    // Configura o manipulador de sinal para SIGUSR1
    signal(SIGUSR1, disk_signal_handler);

    return 0; // Sucesso
}

int disk_block_read(int block, void *buffer)
{
    task_t *operation = (task_t *)malloc(sizeof(task_t));
    if (!operation)
        return -1; // Erro ao alocar memória para a operação

    operation->disk.type = DISK_CMD_READ;
    operation->disk.block = block;
    operation->disk.buffer = buffer;

    // Obtém o semáforo de acesso ao disco
    sem_down(&disk_semaphore);

    // Adiciona a operação à fila de operações do disco
    
    queue_append((queue_t **)&task_queue, (queue_t *)operation);
    // Se o gerente de disco está dormindo, acorda-o
    task_resume(disk_manager_task);

    // Libera o semáforo de acesso ao disco
    task_yield();

    return 0;
}

int disk_block_write(int block, void *buffer)
{
    task_t *operation = (task_t *)malloc(sizeof(task_t));
    if (!operation)
        return -1; // Erro ao alocar memória para a operação

    operation->disk.type = DISK_CMD_WRITE;
    operation->disk.block = block;
    operation->disk.buffer = buffer;

    // Obtém o semáforo de acesso ao disco
    sem_down(&disk_semaphore);

    // Adiciona a operação à fila de operações do disco
    queue_append((queue_t **)&task_queue, (queue_t *)operation);

    // Se o gerente de disco está dormindo, acorda-o
    task_resume(disk_manager_task);

    // Libera o semáforo de acesso ao disco
    task_yield();

    return 0;
}

void diskDriverBody(void *args)
{
    while (1)
    {
        // Obtém o semáforo de acesso ao disco
        sem_down(&disk_semaphore);
        // Verifica se foi acordado devido a um sinal do disco
        if (disk_signal_received || queue_size((queue_t *)task_queue) > 0)
        {
            disk_signal_received = 0;
            if (task_queue)
            {
                task_t *operation = task_queue;

                // Solicita ao disco a operação de L/E
                if (operation->disk.type == DISK_CMD_WRITE || operation->disk.type == DISK_CMD_READ)
                {
                    disk_cmd(operation->disk.type, operation->disk.block, operation->disk.buffer);
                }

                // Acorda a tarefa cujo pedido foi atendido
                task_resume(operation);

                // Libera a memória alocada para a operação
                // Remove a operação da fila e libera a memória alocada para a operação
                queue_remove((queue_t **)&task_queue, (queue_t *)operation);
                free(operation);

                task_sleep(1);
            }
        }

        // Libera o semáforo de acesso ao disco
        sem_up(&disk_semaphore);

        // Suspende a tarefa corrente
        task_yield();
    }

    free(disk_manager_task); // Tarefa do gerente de disco
    free(task_queue);
}

void disk_signal_handler(int signal)
{
    disk_signal_received = 1;
    task_resume(disk_manager_task); // Acorda a tarefa gerente de disco
}

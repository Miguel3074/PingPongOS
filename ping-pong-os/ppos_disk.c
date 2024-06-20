#include "ppos_disk.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

// Variáveis globais do gerenciador de disco
int num_blocks;               // Número de blocos do disco
int block_size;               // Tamanho de cada bloco em bytes
int disk_signal_received = 0; // Sinal do disco recebido
semaphore_t disk_semaphore;   // Semáforo para controle de acesso ao disco
task_t *disk_manager_task;     // Tarefa do gerente de disco

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

    // Inicializa o semáforo de acesso ao disco
    sem_create(&disk_semaphore, 1);

    // Cria a tarefa do gerente de disco
    task_create(disk_manager_task, diskDriverBody, NULL); // TALVEZ PROBLEMA AQUI E PRECISE CRIAR UMA TASK PARA ISSO
    
    // Configura o manipulador de sinal para SIGUSR1
    signal(SIGUSR1, disk_signal_handler);

    return 0; // Sucesso
}

int disk_block_read(int block, void *buffer)
{

    disk_t *operation = (disk_t *)malloc(sizeof(disk_t));
    if (!operation)
        return -1; // Erro ao alocar memória para a operação

    operation->type = DISK_CMD_READ;
    operation->block = block;
    operation->buffer = buffer;

    if (!disk_manager_task)
    {
        disk_manager_task->disk = *operation;
    }
    else
    {
        disk_manager_task->next->disk = *operation;
    }

    // Obtém o semáforo de acesso ao disco
    sem_down(&disk_semaphore);

    // Adiciona a operação à fila de operações do disco
    queue_append((queue_t **)&disk_manager_task, (queue_t *)operation);

    // Se o gerente de disco está dormindo, acorda-o
    if (disk_manager_task->state == 's')
    {
        task_resume(disk_manager_task);
    }

    // Libera o semáforo de acesso ao disco
    sem_up(&disk_semaphore);

    task_yield();

    return 0;
}

int disk_block_write(int block, void *buffer)
{
    disk_t *operation = (disk_t *)malloc(sizeof(disk_t));
    if (!operation)
        return -1; // Erro ao alocar memória para a operação

    operation->type = DISK_CMD_WRITE;
    operation->block = block;
    operation->buffer = buffer;

    if (!disk_manager_task)
    {
        disk_manager_task->disk = *operation;
    }
    else
    {
        disk_manager_task->next->disk = *operation;
    }

    // Obtém o semáforo de acesso ao disco
    sem_down(&disk_semaphore);

    // Adiciona a operação à fila de operações do disco
    queue_append((queue_t **)&disk_manager_task, (queue_t *)operation);

    // Se o gerente de disco está dormindo, acorda-o
    if (disk_manager_task->state == 's')
    {
        task_resume(disk_manager_task);
    }

    // Libera o semáforo de acesso ao disco
    sem_up(&disk_semaphore);

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
        if (disk_signal_received || 1 == 1)
        {
            disk_signal_received = 0;
            // Verifica se o disco está livre e há pedidos de E/S na fila       && disk_manager_task->next != NULL
            if (disk_cmd(DISK_CMD_STATUS, 0, 0) == 1 )
            {
                
                // Escolhe na fila o pedido a ser atendido (usando FCFS)
                task_t *next_op = (task_t *)malloc(sizeof(task_t));
                next_op = disk_manager_task->next;
                 printf("ENTROU NO BODY0\n %d",next_op->disk.block);
                // Solicita ao disco a operação de L/E
                if (next_op->disk.type == DISK_CMD_WRITE || next_op->disk.type == DISK_CMD_READ)
                {
                    printf("ENTROU NO BODY\n");
                    disk_cmd(next_op->disk.type, next_op->disk.block, next_op->disk.buffer);
                }
                printf("ENTROU NO BODY1\n");
                // Acorda a tarefa cujo pedido foi atendido
                task_resume(next_op);
                printf("ENTROU NO BODY2\n");
                // Libera a memória alocada para a operação
                free(next_op);
            }
        }

        // Libera o semáforo de acesso ao disco
        sem_up(&disk_semaphore);

        // Suspende a tarefa corrente
        task_yield();
    }
}

void disk_signal_handler(int signal)
{
    disk_signal_received = 1;
    task_resume(disk_manager_task); // Acorda a tarefa gerente de disco
}

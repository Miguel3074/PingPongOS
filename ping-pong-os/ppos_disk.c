#include "ppos_disk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Para usar memcpy

int disk_signal = 0;              // Indica se foi gerado um sinal do disco
int disk_free = 1;                // Indica se o disco está livre para operações
task_t *task_current = NULL;      // Tarefa corrente
semaphore_t disk_semaphore;       // Semáforo para controle de acesso ao disco
mqueue_t disk_operations_queue;   // Fila de operações do disco
task_t disk_manager_task; // Declarado como variável global

// gcc -o main ppos_disk.c disk.c ppos.h pingpong-disco1.c ppos-core-aux.c libppos_static.a -lrt      DEVO RODAR
// gcc -o main pingpong-disco1.c disk.c ppos.h ppos-core-aux.c libppos_static.a -lrt      TESTE 1


void diskDriverBody(void *args);


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

    // Inicializa o semáforo de acesso ao disco
    sem_create(&disk_semaphore, 1);

    // Inicializa a fila de operações do disco
    mqueue_create(&disk_operations_queue, sizeof(disk_t));

    // Cria a tarefa do gerente de disco
    task_create(&disk_manager_task, diskDriverBody, NULL);

    return 0; // Sucesso
}

int disk_block_read(int block, void *buffer)
{
    // Obtém o semáforo de acesso ao disco
    sem_down(&disk_semaphore);

    // Cria uma estrutura para representar a operação de leitura
    disk_t operation;
    operation.type = DISK_CMD_READ;
    operation.block = block;
    operation.buffer = buffer;
    operation.task = task_current;

    // Adiciona a operação à fila de operações do disco
    mqueue_send(&disk_operations_queue, &operation);

    // Se o gerente de disco estiver dormindo, acorda-o
    if (task_state(&disk_manager_task) == SUSPENDED)
    {
        task_resume(&disk_manager_task);
    }

    // Libera o semáforo de acesso ao disco
    sem_up(&disk_semaphore);

    // Suspende a tarefa corrente
    task_suspend(task_current, NULL);

    return 0;
}

int disk_block_write(int block, void *buffer)
{
    // Obtém o semáforo de acesso ao disco
    sem_down(&disk_semaphore);

    // Cria uma estrutura para representar a operação de escrita
    disk_t operation;
    operation.type = DISK_CMD_WRITE;
    operation.block = block;
    operation.buffer = buffer;
    operation.task = task_current;

    // Adiciona a operação à fila de operações do disco
    mqueue_send(&disk_operations_queue, &operation);

    // Se o gerente de disco estiver dormindo, acorda-o
    if (task_state(&disk_manager_task) == SUSPENDED)
    {
        task_resume(&disk_manager_task);
    }

    // Libera o semáforo de acesso ao disco
    sem_up(&disk_semaphore);

    // Suspende a tarefa corrente
    task_suspend(task_current, NULL);

    return 0;
}

void diskDriverBody(void *args)
{
    while (1)
    {
        // Obtém o semáforo de acesso ao disco
        sem_down(&disk_semaphore);

        // Verifica se foi acordado devido a um sinal do disco
        if (disk_signal)
        {
            disk_signal = 0;
            // Acorda a tarefa cujo pedido foi atendido
            disk_t completed_op;
            mqueue_recv(&disk_operations_queue, &completed_op);
            task_resume(completed_op.task);
        }

        // Verifica se o disco está livre e há pedidos na fila
        if (disk_free && disk_operations_queue.countMessages > 0)
        {
            // Escolhe na fila o pedido a ser atendido (usando FCFS)
            disk_t next_op;
            mqueue_recv(&disk_operations_queue, &next_op);

            // Solicita ao disco a operação de E/S
            if (next_op.type == DISK_CMD_READ)
            {
                disk_cmd(DISK_CMD_READ, next_op.block, next_op.buffer);
            }
            else if (next_op.type == DISK_CMD_WRITE)
            {
                disk_cmd(DISK_CMD_WRITE, next_op.block, next_op.buffer);
            }

            // Define o disco como ocupado
            disk_free = 0;
        }

        // Libera o semáforo de acesso ao disco
        sem_up(&disk_semaphore);

        // Suspende a tarefa corrente (retorna ao dispatcher)
        task_suspend(&disk_manager_task, NULL);
    }
}

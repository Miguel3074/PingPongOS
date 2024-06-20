#include "ppos_disk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Para usar memcpy

int disk_signal = 0;            // Indica se foi gerado um sinal do disco
task_t *task_current = NULL;    // Tarefa corrente
semaphore_t disk_semaphore;     // Semáforo para controle de acesso ao disco
mqueue_t disk_operations_queue; // Fila de operações do disco
task_t disk_manager_task;       // Declarado como variável global

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
    mqueue_create(&disk_operations_queue, sizeof(disk_t), 256);

    // Cria a tarefa do gerente de disco
    task_create(&disk_manager_task, diskDriverBody, NULL);

    return 0; // Sucesso
}

int disk_block_read(int block, void *buffer)
{
    // Inicializa a estrutura de operação
    disk_t operation;
    memset(&operation, 0, sizeof(disk_t));
    operation.type = DISK_CMD_READ;
    operation.block = block;
    operation.buffer = buffer;
    operation.task = task_current;

    // Obtém o semáforo de acesso ao disco
    sem_down(&disk_semaphore);

    // Adiciona a operação à fila de operações do disco
    if (mqueue_send(&disk_operations_queue, &operation) != 0)
    {
        printf("disk_block_read: Erro ao enviar operação de leitura para a fila\n");
        sem_up(&disk_semaphore);
        return -1;
    }

    // Libera o semáforo de acesso ao disco
    sem_up(&disk_semaphore);

    return 0;
}

int disk_block_write(int block, void *buffer)
{
    // Cria uma estrutura para representar a operação de escrita
    disk_t operation;
    operation.type = DISK_CMD_WRITE;
    operation.block = block;
    operation.buffer = buffer;
    operation.task = task_current;

    // Obtém o semáforo de acesso ao disco
    sem_down(&disk_semaphore);

    // Adiciona a operação à fila de operações do disco
    mqueue_send(&disk_operations_queue, &operation);

    // Libera o semáforo de acesso ao disco
    sem_up(&disk_semaphore);

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

            // Processa a operação concluída
            disk_t completed_op;
            mqueue_recv(&disk_operations_queue, &completed_op);
            task_resume(completed_op.task);
            printf("ENTROU AQUI\n");
        }

        // Verifica se o disco está livre e há pedidos na fila
        if (disk_operations_queue.countMessages > 0)
        {
            // Escolhe na fila o pedido a ser atendido (usando FCFS)
            disk_t next_op;
            mqueue_recv(&disk_operations_queue, &next_op);

            // Solicita ao disco a operação de L/E
          
            disk_cmd(next_op.type, next_op.block, next_op.buffer);

            disk_signal = 1;
        }

        // Libera o semáforo de acesso ao disco
        sem_up(&disk_semaphore);

        task_yield();
    }
}

#include "ppos_disk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Para usar memcpy

int disk_signal = 0;              // Indica se foi gerado um sinal do disco
int disk_free = 1;                // Indica se o disco está livre para operações
int disk_sleeping = 0;            // Indica se o gerente de disco está dormindo
task_t *task_current = NULL;      // Tarefa corrente
semaphore_t disk_semaphore;       // Semáforo para controle de acesso ao disco
task_t disk_manager_task;         // Tarefa do gerente de disco
mqueue_t disk_operations_queue;   // Fila de operações do disco

// Estrutura para representar as operações de disco na fila
typedef struct {
    int type;         // Tipo da operação (leitura ou escrita)
    int block;        // Número do bloco a ser lido ou escrito
    void *buffer;     // Buffer de dados para leitura ou escrita
    task_t *task;     // Tarefa associada à operação
} DiskOperation;

void diskDriverBody(void *args);

// Função principal
int main()
{
    // Inicializa o semáforo de acesso ao disco
    sem_create(&disk_semaphore, 1);

    // Inicializa a fila de operações do disco com capacidade para 10 operações de tamanho sizeof(DiskOperation)
    mqueue_create(&disk_operations_queue, 10, sizeof(DiskOperation));

    // Cria a tarefa do gerente de disco
    task_create(&disk_manager_task, diskDriverBody, NULL);

    int num_blocks; // Variável para armazenar o número de blocos do disco
    int block_size; // Variável para armazenar o tamanho de cada bloco do disco

    // Chama a função para inicializar o gerente de disco
    int result = disk_mgr_init(&num_blocks, &block_size);

    // Verifica se a inicialização foi bem-sucedida
    if (result == 0)
    {
        printf("Disco inicializado com sucesso!\n");
        printf("Número de blocos: %d\n", num_blocks);
        printf("Tamanho de cada bloco: %d bytes\n", block_size);
    }
    else
    {
        printf("Erro ao inicializar o disco.\n");
    }

    return 0;
}

// Função para inicializar o gerente de disco
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

    return 0; // Sucesso
}

int disk_block_read(int block, void *buffer)
{
    // Obtém o semáforo de acesso ao disco
    sem_down(&disk_semaphore);

    // Cria uma estrutura para representar a operação de leitura
    DiskOperation operation;
    operation.type = DISK_CMD_READ;
    operation.block = block;
    operation.buffer = buffer;
    operation.task = task_current;

    // Adiciona a operação à fila de operações do disco
    mqueue_send(&disk_operations_queue, &operation);

    // Se o gerente de disco estiver dormindo, acorda-o
    if (disk_sleeping)
    {
        disk_sleeping = 0;
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
    DiskOperation operation;
    operation.type = DISK_CMD_WRITE;
    operation.block = block;
    operation.buffer = buffer;
    operation.task = task_current;

    // Adiciona a operação à fila de operações do disco
    mqueue_send(&disk_operations_queue, &operation);

    // Se o gerente de disco estiver dormindo, acorda-o
    if (disk_sleeping)
    {
        disk_sleeping = 0;
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
            DiskOperation completed_op;
            mqueue_recv(&disk_operations_queue, &completed_op);
            task_resume(completed_op.task);
        }

        // Verifica se o disco está livre e há pedidos na fila
        if (disk_free && disk_operations_queue.countMessages > 0)
        {
            // Escolhe na fila o pedido a ser atendido (usando FCFS)
            DiskOperation next_op;
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
        }

        // Libera o semáforo de acesso ao disco
        sem_up(&disk_semaphore);

        // Suspende a tarefa corrente (retorna ao dispatcher)
        task_suspend(&disk_manager_task, NULL);
    }
}

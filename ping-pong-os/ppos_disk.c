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
task_t *disk_wait_queue = NULL; // Fila de tarefas esperando pelo disco
;                               // Tarefa de fila

enum SchedulingAlgorithm
{
    FCFS,
    SSTF,
    CSCAN
};

enum SchedulingAlgorithm current_algorithm = FCFS; // Algoritmo de escalonamento escolhido

// Prototipo das funções
void diskDriverBody(void *args);
void disk_signal_handler(int signal);

void schedule_fcfs(task_t **task_queue, int current_head_position, int *blocks_traversed);

void schedule_sstf(task_t **task_queue, int current_head_position, int *blocks_traversed);

void schedule_cscan(task_t **task_queue, int current_head_position, int *blocks_traversed, int num_blocks);

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
    int current_head_position = 0;
    int blocks_traversed = 0;
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
                // switch (current_algorithm) {
                //     case FCFS:
                //         schedule_fcfs(&task_queue, current_head_position, &blocks_traversed);
                //         break;
                //     case SSTF:
                //         schedule_sstf(&task_queue, current_head_position, &blocks_traversed);
                //         break;
                //     case CSCAN:
                //         schedule_cscan(&task_queue, current_head_position, &blocks_traversed, num_blocks);
                //         break;
                // }
                task_t *operation = task_queue;

                // Solicita ao disco a operação de L/E
                if (operation->disk.type == DISK_CMD_WRITE || operation->disk.type == DISK_CMD_READ)
                {
                    disk_cmd(operation->disk.type, operation->disk.block, operation->disk.buffer);
                }

                // Acorda a tarefa cujo pedido foi atendido
                task_resume(operation);

                // Remove a operação da fila e libera a memória alocada para a operação
                queue_remove((queue_t **)&task_queue, (queue_t *)operation);
                free(operation);

                task_sleep(1);
            }
        }

        // Libera o semáforo de acesso ao disco
        sem_up(&disk_semaphore);

        // Suspende a tarefa corrente
        task_suspend(disk_manager_task, &disk_wait_queue);
    }

    free(disk_manager_task); // Tarefa do gerente de disco
    free(task_queue);
}

void disk_signal_handler(int signal)
{
    disk_signal_received = 1;
    task_resume(disk_manager_task); // Acorda a tarefa gerente de disco
}

void schedule_fcfs(task_t **task_queue, int current_head_position, int *blocks_traversed)
{
    // Não é necessário reordenar a fila de operações, pois já está na ordem de chegada
    task_t *task = *task_queue;

    while (task)
    {
        int distance = abs(task->disk.block - current_head_position);
        *blocks_traversed += distance;
        current_head_position = task->disk.block;

        task = task->next;
    }
}
void schedule_sstf(task_t **task_queue, int current_head_position, int *blocks_traversed)
{
    task_t *sorted_queue = NULL;
    while (*task_queue)
    {
        task_t *closest_task = NULL;
        task_t *prev_task = NULL;
        task_t *task = *task_queue;
        task_t *prev = NULL;

        while (task)
        {
            if (!closest_task || abs(task->disk.block - current_head_position) < abs(closest_task->disk.block - current_head_position))
            {
                closest_task = task;
                prev_task = prev;
            }
            prev = task;
            task = task->next;
        }

        int distance = abs(closest_task->disk.block - current_head_position);
        *blocks_traversed += distance;
        current_head_position = closest_task->disk.block;

        // Remove closest_task from task_queue
        if (prev_task)
        {
            prev_task->next = closest_task->next;
        }
        else
        {
            *task_queue = closest_task->next;
        }
        // Append closest_task to sorted_queue
        closest_task->next = NULL;
        queue_append((queue_t **)&sorted_queue, (queue_t *)closest_task);
    }
    *task_queue = sorted_queue;
}

void schedule_cscan(task_t **task_queue, int current_head_position, int *blocks_traversed, int num_blocks)
{
    task_t *sorted_queue = NULL;

    // Ordena a fila de operações pela posição dos blocos
    while (*task_queue)
    {
        task_t *min_task = NULL;
        task_t *prev_task = NULL;
        task_t *task = *task_queue;
        task_t *prev = NULL;

        while (task)
        {
            if (!min_task || task->disk.block < min_task->disk.block)
            {
                min_task = task;
                prev_task = prev;
            }
            prev = task;
            task = task->next;
        }

        // Remove min_task da task_queue
        if (prev_task)
        {
            prev_task->next = min_task->next;
        }
        else
        {
            *task_queue = min_task->next;
        }

        // Append min_task ao sorted_queue
        min_task->next = NULL;
        queue_append((queue_t **)&sorted_queue, (queue_t *)min_task);
    }

    task_t *task = sorted_queue;
    int wrapped_around = 0;

    while (task)
    {
        if (task->disk.block >= current_head_position || wrapped_around)
        {
            int distance = abs(task->disk.block - current_head_position);
            *blocks_traversed += distance;
            current_head_position = task->disk.block;
        }
        task = task->next;

        // Verifica se precisamos fazer a volta circular
        if (!task && !wrapped_around)
        {
            *blocks_traversed += (num_blocks - current_head_position);
            current_head_position = 0;
            task = sorted_queue;
            wrapped_around = 1;
        }
    }
}

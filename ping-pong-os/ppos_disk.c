#include "ppos_disk.h" //   gcc -o main ppos_disk.c disk.c ppos.h pingpong-disco1.c ppos-core-aux.c libppos_static.a -lrt   COMANDO
#include <stdio.h>     //   gcc -o main disk.c ppos.h pingpong-disco1.c ppos-core-aux.c libppos_static.a -lrt
#include <stdlib.h>
#include <signal.h>

// Variáveis globais do gerenciador de disco
int num_blocks;               // Número de blocos do disco
int block_size;               // Tamanho de cada bloco em bytes
int disk_signal_received = 0; // Sinal do disco recebido
task_t *disk_manager_task;    // Tarefa do gerente de disco
disk_t localDisk;
int current_head_position = 0; // Posição atual do cabeçote do disco
int blocks_traversed = 0;      // Número de blocos traversados
semaphore_t disk_semaphore;

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

disk_queue *schedule_fcfs(disk_queue *task_aux, int current_head_position, int *blocks_traversed);

disk_queue *schedule_sstf(disk_queue *task_aux, int current_head_position, int *blocks_traversed);

disk_queue *schedule_cscan(disk_queue *task_aux, int current_head_position, int *blocks_traversed, int num_blocks);

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
    if (sem_create(&disk_semaphore, 1) < 0)
    {
        return -1; // Erro ao criar o semáforo
    }

    // Cria a tarefa do gerente de disco
    task_create(disk_manager_task, diskDriverBody, NULL);

    // Configura o manipulador de sinal para SIGUSR1
    signal(SIGUSR1, disk_signal_handler);

    return 0; // Sucesso
}

int disk_block_read(int block, void *buffer)
{
    disk_queue *operation = (disk_queue *)malloc(sizeof(disk_queue));

    operation->next = NULL;
    operation->type = DISK_CMD_READ;
    operation->block = block;
    operation->buffer = buffer;
    // Obtém o semáforo de acesso ao disco
    sem_down(&disk_semaphore);

    // Adiciona a operação à fila de operações do disco
    queue_append((queue_t **)&localDisk.task_queue, (queue_t *)operation);

    // Se o gerente de disco está dormindo, acorda-o
    task_resume(disk_manager_task);

    // Libera o semáforo de acesso ao disco
    task_yield();
    return 0;
}

int disk_block_write(int block, void *buffer)
{
    disk_queue *operation = (disk_queue *)malloc(sizeof(disk_queue));

    operation->next = NULL;
    operation->type = DISK_CMD_WRITE;
    operation->block = block;
    operation->buffer = buffer;

    // Obtém o semáforo de acesso ao disco
    sem_down(&disk_semaphore);

    // Adiciona a operação à fila de operações do disco
    queue_append((queue_t **)&localDisk.task_queue, (queue_t *)operation);

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
        sem_down(&disk_semaphore);

        // Se foi acordado devido a um sinal do disco
        if (disk_signal_received)
        {
            disk_signal_received = 0;
            printf("Disk signal\n");
            // disk_t *operation = &localDisk.task_queue;
            // task_resume(operation);

            // // Remove a operação da fila e libera a memória alocada para a operação
            // queue_remove((queue_t **)&localDisk.task_queue, (queue_t *)operation);
            // free(operation);
        }

        switch (current_algorithm)
        {
        case FCFS:
            (localDisk.task_queue) = schedule_fcfs(localDisk.task_queue, current_head_position, &blocks_traversed);
            break;
        case SSTF:
            (localDisk.task_queue) = schedule_sstf(localDisk.task_queue, current_head_position, &blocks_traversed);
            break;
        case CSCAN:
            (localDisk.task_queue) = schedule_cscan(localDisk.task_queue, current_head_position, &blocks_traversed, num_blocks);
            break;
        default:
            // Algoritmo de escalonamento inválido
            break;
        }

        // Consulta o status do disco
        int disk_status = disk_cmd(DISK_CMD_STATUS, 0, 0);
        // Se o disco estiver livre e houver pedidos de E/S na fila
        if (disk_status == DISK_STATUS_IDLE && localDisk.task_queue)
        {

            // Solicita ao disco a operação de E/S, usando disk_cmd()
            if (localDisk.task_queue->type == DISK_CMD_WRITE || localDisk.task_queue->type == DISK_CMD_READ)
            {
                disk_cmd(localDisk.task_queue->type, localDisk.task_queue->block, localDisk.task_queue->buffer);

                // Atualiza a posição do cabeçote após a operação
                current_head_position = localDisk.task_queue->block;
                queue_remove((queue_t **)&localDisk.task_queue, (queue_t *)localDisk.task_queue);
            }
        }

        task_sleep(1);
        sem_up(&disk_semaphore);
        // Suspende a tarefa corrente (retorna ao dispatcher)
        task_yield();
    }
}

void disk_signal_handler(int signal)
{
    disk_signal_received = 1;
    task_resume(disk_manager_task); // Acorda a tarefa gerente de disco
}

disk_queue *schedule_fcfs(disk_queue *task_aux, int current_head_position, int *blocks_traversed)
{
    return task_aux->next;
}

disk_queue *schedule_sstf(disk_queue *task_aux, int current_head_position, int *blocks_traversed)
{
    // SSTF - Shortest Seek Time First
    if (!task_aux)
        return NULL;

    disk_queue *shortest_seek = task_aux;
    disk_queue *iterator = task_aux->next;
    int min_seek = abs(shortest_seek->block - current_head_position);

    while (iterator != task_aux)
    {
        int seek_time = abs(iterator->block - current_head_position);
        if (seek_time < min_seek)
        {
            min_seek = seek_time;
            shortest_seek = iterator;
        }
        iterator = iterator->next;
    }

    if (shortest_seek != task_aux)
    {
        queue_remove((queue_t **)&task_aux, (queue_t *)shortest_seek);
        queue_append((queue_t **)&task_aux, (queue_t *)shortest_seek);
    }

    return task_aux;
}

disk_queue *schedule_cscan(disk_queue *task_aux, int current_head_position, int *blocks_traversed, int num_blocks)
{
    // CSCAN - Circular SCAN
    if (!task_aux)
        return NULL;

    disk_queue *start = task_aux;
    disk_queue *iterator = task_aux->next;

    while (iterator != start)
    {
        if (iterator->block < start->block)
            start = iterator;
        iterator = iterator->next;
    }

    iterator = start;

    do
    {
        if (iterator->block >= current_head_position)
        {
            queue_remove((queue_t **)&task_aux, (queue_t *)iterator);
            queue_append((queue_t **)&task_aux, (queue_t *)iterator);
            break;
        }
        iterator = iterator->next;
    } while (iterator != start);

    return task_aux;
}

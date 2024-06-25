#include "ppos_disk.h" //   gcc -o main ppos_disk.c disk.c ppos.h pingpong-disco1.c ppos-core-aux.c libppos_static.a -lrt   COMANDO
#include <stdio.h>     //   gcc -o main disk.c ppos.h pingpong-disco1.c ppos-core-aux.c libppos_static.a -lrt
#include <stdlib.h>
#include <limits.h>
#include <signal.h>
#include "ppos-core-globals.h"

// Variáveis globais do gerenciador de disco
int num_blocks;                 // Número de blocos do disco
int block_size;                 // Tamanho de cada bloco em bytes
int disk_signal_received = 0;   // Sinal do disco recebido
semaphore_t disk_semaphore;     // Semáforo para controle de acesso ao disco
task_t *disk_manager_task;      // Tarefa do gerente de disco
task_t *disk_wait_queue = NULL; // Fila de tarefas esperando pelo disco
int current_head_position = 0;  // Posição atual do cabeçote do disco
int blocks_traversed = 0;       // Número de blocos traversados

mqueue_t task_queue;

enum SchedulingAlgorithm
{
    FCFS,
    SSTF,
    CSCAN
};

void disk_scheduler_sstf();

void disk_scheduler_cscan();

int compare_disk_operations(const void *a, const void *b);

enum SchedulingAlgorithm current_algorithm = FCFS; // Algoritmo de escalonamento escolhido

// Prototipo das funções
void diskDriverBody(void *args);
void disk_signal_handler(int signal);

// FUNÇOES escalonador de requisições de acesso ao disco

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

    if (mqueue_create(&task_queue, 256, sizeof(task_t)) == -1)
        printf("ERRO AO CRIAR");

    // Cria a tarefa do gerente de disco
    task_create(disk_manager_task, diskDriverBody, NULL);

    // Configura o manipulador de sinal para SIGUSR1
    signal(SIGUSR1, disk_signal_handler);

    return 0; // Sucesso
}

int disk_block_read(int block, void *buffer)
{
    // Obtém o semáforo de acesso ao disco
    sem_down(&disk_semaphore);

    // Adiciona a operação à fila de operações do disco

    task_t taskOp;
    taskOp.disk.type = DISK_CMD_READ;
    taskOp.disk.block = block;
    taskOp.disk.buffer = buffer;

    if (mqueue_send(&task_queue, &taskOp) == -1)
        printf("ERRO READ!\n");

    // Se o gerente de disco está dormindo, acorda-o
    task_resume(disk_manager_task);

    task_yield();

    return 0;
}

int disk_block_write(int block, void *buffer)
{

    // Obtém o semáforo de acesso ao disco
    sem_down(&disk_semaphore);

    // Adiciona a operação à fila de operações do disco
    task_t taskOp;
    taskOp.disk.type = DISK_CMD_WRITE;
    taskOp.disk.block = block;
    taskOp.disk.buffer = buffer;

    if (mqueue_send(&task_queue, &taskOp) == -1)
        printf("ERRO WRITE!\n");

    // Se o gerente de disco está dormindo, acorda-o
    task_resume(disk_manager_task);

    task_yield();

    return 0;
}

void diskDriverBody(void *args)
{
    while (1)
    {
        // Se foi acordado devido a um sinal do disco
        sem_down(&disk_semaphore);

        if (disk_signal_received)
        {
            // Resetar o sinal do disco
            disk_signal_received = 0;

            // Acordar a tarefa cujo pedido foi atendido
            if (disk_wait_queue != NULL)
            {
                task_t *waiting_task = disk_wait_queue;  // Tarefa que estava esperando pelo disco
                disk_wait_queue = disk_wait_queue->next; // Remove a tarefa da fila de espera
                task_resume(waiting_task);               // Acorda a tarefa
            }
        }

        switch (current_algorithm)
        {
        case SSTF:
            disk_scheduler_sstf();
            break;
        case CSCAN:
            disk_scheduler_cscan();
            break;
        default:
        if (disk_cmd(DISK_CMD_STATUS, 0, 0) == DISK_STATUS_IDLE && mqueue_msgs(&task_queue) > 0)
        {
            // Escolhe na fila o pedido a ser atendido, usando FCFS
            task_t operation;

            if (mqueue_recv(&task_queue, &operation) == -1)
                printf("ERRO RECV!\n");

            // Solicita ao disco a operação de E/S, usando disk_cmd()
            if (operation.disk.type == DISK_CMD_WRITE || operation.disk.type == DISK_CMD_READ)
            {

                disk_cmd(operation.disk.type, operation.disk.block, operation.disk.buffer);

                // Atualiza a posição do cabeçote após a operação
                current_head_position = operation.disk.block;
            }
        }
            break;
        }
        // Se o disco estiver livre e houver pedidos de E/S na fila
        
        task_sleep(1);

        sem_up(&disk_semaphore);

        // Suspende a tarefa corrente (retorna ao dispatcher)

        task_suspend(disk_manager_task, &disk_wait_queue);
    }
}

void disk_signal_handler(int signal)
{
    disk_signal_received = 1;
    task_resume(disk_manager_task); // Acorda a tarefa gerente de disco
}

void disk_scheduler_sstf()
{
    // Verifica se há pedidos na fila de operações do disco
    if (mqueue_msgs(&task_queue) > 0)
    {
        // Inicializa a menor distância como um valor grande para comparação
        int menor_distancia = INT_MAX;
        task_t operation, closest_operation;

        // Itera sobre os pedidos na fila para encontrar o mais próximo
        mqueue_t temp_queue;
        mqueue_create(&temp_queue, 256, sizeof(task_t));
        while (mqueue_msgs(&task_queue) > 0)
        {
            mqueue_recv(&task_queue, &operation);
            int distancia = abs(current_head_position - operation.disk.block);
            if (distancia < menor_distancia)
            {
                menor_distancia = distancia;
                closest_operation = operation;
            }
            mqueue_send(&temp_queue, &operation);
        }

        // Restaura a fila original
        while (mqueue_msgs(&temp_queue) > 0)
        {
            mqueue_recv(&temp_queue, &operation);
            mqueue_send(&task_queue, &operation);
        }
        mqueue_destroy(&temp_queue);

        // Realiza a operação mais próxima encontrada (SSTF)
        if (closest_operation.disk.type == DISK_CMD_WRITE || closest_operation.disk.type == DISK_CMD_READ)
        {
            disk_cmd(closest_operation.disk.type, closest_operation.disk.block, closest_operation.disk.buffer);
            current_head_position = closest_operation.disk.block; // Atualiza a posição do cabeçote após a operação
        }
    }
}

void disk_scheduler_cscan()
{
    // Verifica se há pedidos na fila de operações do disco
    if (mqueue_msgs(&task_queue) > 0)
    {
        // Ordena a fila de operações por ordem crescente de blocos
        task_t operations[mqueue_msgs(&task_queue)];
        int i = 0;
        while (mqueue_msgs(&task_queue) > 0)
        {
            mqueue_recv(&task_queue, &operations[i]);
            i++;
        }
        qsort(operations, i, sizeof(task_t), compare_disk_operations);

        // Encontra o próximo pedido na fila para atender (CSCAN)
        task_t next_operation;
        for (int j = 0; j < i; j++)
        {
            if (operations[j].disk.block >= current_head_position)
            {
                next_operation = operations[j];
                break;
            }
        }

        // Se não encontrou na parte superior, seleciona o primeiro da fila
        if (next_operation.disk.type == -1)
            next_operation = operations[0];

        // Realiza a operação encontrada (CSCAN)
        if (next_operation.disk.type == DISK_CMD_WRITE || next_operation.disk.type == DISK_CMD_READ)
        {
            disk_cmd(next_operation.disk.type, next_operation.disk.block, next_operation.disk.buffer);
            current_head_position = next_operation.disk.block; // Atualiza a posição do cabeçote após a operação
        }
    }
}

int compare_disk_operations(const void *a, const void *b)
{
    task_t *op1 = (task_t *)a;
    task_t *op2 = (task_t *)b;

    // Comparação por blocos
    if (op1->disk.block < op2->disk.block)
        return -1;
    else if (op1->disk.block > op2->disk.block)
        return 1;
    else
        return 0;
}

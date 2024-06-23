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
int current_head_position = 0;  // Posição atual do cabeçote do disco
int blocks_traversed = 0;       // Número de blocos traversados

enum SchedulingAlgorithm
{
    FCFS,
    SSTF,
    CSCAN
};

enum SchedulingAlgorithm current_algorithm = CSCAN; // Algoritmo de escalonamento escolhido

// Prototipo das funções
void diskDriverBody(void *args);
void disk_signal_handler(int signal);

void schedule_fcfs(task_t **task_queue, int *current_head_position, int *blocks_traversed);

void schedule_sstf(task_t **task_queue, int *current_head_position, int *blocks_traversed);

void schedule_cscan(task_t **task_queue, int *current_head_position, int *blocks_traversed, int num_blocks);

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
    operation->next = NULL;
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
    operation->next = NULL;
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
        sem_down(&disk_semaphore);

        // Se foi acordado devido a um sinal do disco
        if (disk_signal_received)
        {
            // Resetar o sinal do disco
            disk_signal_received = 0;

            // Acorda a tarefa cujo pedido foi atendido
            if (task_queue)
            {
                task_t *operation = task_queue;
                task_resume(operation);

                // Remove a operação da fila e libera a memória alocada para a operação
                queue_remove((queue_t **)&task_queue, (queue_t *)operation);
                free(operation);
            }
        }

        switch (current_algorithm)
        {
        case FCFS:
            schedule_fcfs(&task_queue, &current_head_position, &blocks_traversed);
            break;
        case SSTF:
            schedule_sstf(&task_queue, &current_head_position, &blocks_traversed);
            break;
        case CSCAN:
            schedule_cscan(&task_queue, &current_head_position, &blocks_traversed, num_blocks);
            break;
        default:
            // Algoritmo de escalonamento inválido
            break;
        }

        // Consulta o status do disco
        int disk_status = disk_cmd(DISK_CMD_STATUS, 0, 0);

        // Se o disco estiver livre e houver pedidos de E/S na fila
        if (disk_status == DISK_STATUS_IDLE && task_queue != NULL)
        {
            // Escolhe na fila o pedido a ser atendido, usando FCFS
            task_t *operation = task_queue;

            // Solicita ao disco a operação de E/S, usando disk_cmd()
            if (operation->disk.type == DISK_CMD_WRITE || operation->disk.type == DISK_CMD_READ)
            {
                disk_cmd(operation->disk.type, operation->disk.block, operation->disk.buffer);

                // Atualiza a posição do cabeçote após a operação
                current_head_position = operation->disk.block;
            }
        }

        task_sleep(10);
        sem_up(&disk_semaphore);
        // Suspende a tarefa corrente (retorna ao dispatcher)
        task_suspend(disk_manager_task, &disk_wait_queue);
    }

    // Liberação de recursos (não deveria ser alcançada devido ao loop infinito)
    free(disk_manager_task);
    free(task_queue);
}

void disk_signal_handler(int signal)
{
    disk_signal_received = 1;
    task_resume(disk_manager_task); // Acorda a tarefa gerente de disco
}

void schedule_fcfs(task_t **task_queue, int *current_head_position, int *blocks_traversed)
{
    // Verifica se há tarefas na fila
    if (*task_queue == NULL)
    {
        return;
    }

    // A primeira tarefa na fila é a próxima a ser atendida
    task_t *operation = *task_queue;

    // Calcula o número de blocos a serem percorridos para atender a operação
    *blocks_traversed += abs(operation->disk.block - *current_head_position);

    // Atualiza a posição atual do cabeçote
    *current_head_position = operation->disk.block;
}
void schedule_sstf(task_t **task_queue, int *current_head_position, int *blocks_traversed)
{
    // Verifica se há tarefas na fila
    if (*task_queue == NULL)
    {
        return;
    }

    task_t *closest_task = NULL;
    int closest_distance = num_blocks; // Inicializa com a maior distância possível

    task_t *operation = *task_queue;
    task_t *task = operation;

    do
    {
        int distance = abs(task->disk.block - *current_head_position);
        if (distance < closest_distance)
        {
            closest_distance = distance;
            closest_task = task;
        }
        task = task->next;
    } while (task != operation);

    if (closest_task)
    {
        // Atualiza a posição do cabeçote e os blocos percorridos
        *blocks_traversed += closest_distance;
        *current_head_position = closest_task->disk.block;

        // Remove a tarefa mais próxima da fila
        queue_remove((queue_t **)&task_queue, (queue_t *)closest_task);
        free(closest_task); // Libera a memória alocada para a tarefa
    }

    // Calcula o número de blocos a serem percorridos para atender a operação
    *blocks_traversed += abs(operation->disk.block - *current_head_position);
}

void schedule_cscan(task_t **task_queue, int *current_head_position, int *blocks_traversed, int num_blocks)
{
    if (*task_queue == NULL)
    {
        return;
    }

    // Ordena a fila de tarefas com base na posição do bloco
    task_t *current = *task_queue;
    while (current->next != NULL)
    {
        task_t *next = current->next;
        while (next != NULL)
        {
            if (current->disk.block > next->disk.block)
            {
                int temp_block = current->disk.block;
                current->disk.block = next->disk.block;
                next->disk.block = temp_block;
            }
            next = next->next;
        }
        current = current->next;
    }

    // Encontra a primeira tarefa na fila cujo bloco é maior que a posição atual do cabeçote
    current = *task_queue;
    while (current != NULL && current->disk.block <= *current_head_position)
    {
        current = current->next;
    }

    // Se não há tarefas na direção atual, move o cabeçote para o início do disco
    if (current == NULL)
    {
        *blocks_traversed += num_blocks - *current_head_position;
        *current_head_position = 0;
    }
    else
    {
        // Calcula o número de blocos a serem percorridos até a última tarefa na direção atual
        *blocks_traversed += current->disk.block - *current_head_position;
        *current_head_position = current->disk.block;
    }

    // Atualiza a posição do cabeçote e os blocos percorridos para atender às tarefas na direção oposta
    while (current != NULL)
    {
        *blocks_traversed += abs(current->disk.block - *current_head_position);
        *current_head_position = current->disk.block;
        current = current->next;
    }

    // Remove as tarefas atendidas da fila
    while (*task_queue != NULL && (*task_queue)->disk.block <= *current_head_position)
    {
        task_t *temp = *task_queue;
        *task_queue = (*task_queue)->next;
        free(temp);
    }
}

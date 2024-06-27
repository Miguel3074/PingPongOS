#include "ppos_disk.h" //   gcc -o main ppos_disk.c disk.c ppos.h pingpong-disco1.c ppos-core-aux.c libppos_static.a -lrt   COMANDO
#include <stdio.h>     //   gcc -o main disk.c ppos.h pingpong-disco1.c ppos-core-aux.c libppos_static.a -lrt
#include <stdlib.h>
#include <limits.h>
#include "ppos-core-globals.h"
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

enum SchedulingAlgorithm current_algorithm = CSCAN; // Algoritmo de escalonamento escolhido

// Prototipo das funções
void diskDriverBody(void *args);
void disk_signal_handler(int signal);

disk_queue *schedule_fcfs(disk_queue *task_aux, int current_head_position);

disk_queue *schedule_sstf(disk_queue *task_aux, int current_head_position);

disk_queue *schedule_cscan(disk_queue *task_aux, int current_head_position, int num_blocks);

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

    operation->task = taskExec;
    operation->next = NULL;
    operation->type = DISK_CMD_READ;
    operation->block = block;
    operation->buffer = buffer;

    // Obtém o semáforo de acesso ao disco
    sem_down(&disk_semaphore);

    // Adiciona a operação à fila de operações do disco
    queue_append((queue_t **)&localDisk.task_queue, (queue_t *)operation);

    sem_up(&disk_semaphore);

    // Se o gerente de disco está dormindo, acorda-o
    task_resume(disk_manager_task);

    task_suspend(taskExec, &localDisk.disk_wait_queue);
    // Libera o semáforo de acesso ao disco
    task_yield();

    return 0;
}

int disk_block_write(int block, void *buffer)
{
    disk_queue *operation = (disk_queue *)malloc(sizeof(disk_queue));

    operation->task = taskExec;
    operation->next = NULL;
    operation->type = DISK_CMD_WRITE;
    operation->block = block;
    operation->buffer = buffer;

    // Obtém o semáforo de acesso ao disco
    sem_down(&disk_semaphore);

    // Adiciona a operação à fila de operações do disco
    queue_append((queue_t **)&localDisk.task_queue, (queue_t *)operation);

    // Libera o semáforo de acesso ao disco
    sem_up(&disk_semaphore);

    // Se o gerente de disco está dormindo, acorda-o
    task_resume(disk_manager_task);

    task_suspend(taskExec, &localDisk.disk_wait_queue);

    task_yield();

    return 0;
}

void diskDriverBody(void *args)
{

    while (1)
    {
        sem_down(&disk_semaphore);
        if (readyQueue == taskMain)
        {
            readyQueue = readyQueue->next;
        }
        // Se foi acordado devido a um sinal do disco
        if (disk_signal_received)
        {
            disk_signal_received = 0;
            // acorda a tarefa cujo pedido foi atendido
            if (localDisk.task_queue)
            {
                task_resume(localDisk.task_queue->task);
                queue_remove((queue_t **)&localDisk.task_queue, (queue_t *)localDisk.task_queue);
            }
        }

        // Consulta o status do disco
        int disk_status = disk_cmd(DISK_CMD_STATUS, 0, 0);
        // Se o disco estiver livre e houver pedidos de E/S na fila
        if (disk_status == DISK_STATUS_IDLE && localDisk.task_queue)
        {
            switch (current_algorithm)
            {
            case FCFS:
                (localDisk.task_queue) = schedule_fcfs(localDisk.task_queue, current_head_position);
                break;
            case SSTF:
                (localDisk.task_queue) = schedule_sstf(localDisk.task_queue, current_head_position);
                break;
            case CSCAN:
                (localDisk.task_queue) = schedule_cscan(localDisk.task_queue, current_head_position, num_blocks);
                break;
            }

            // Solicita ao disco a operação de E/S, usando disk_cmd()
            if (localDisk.task_queue->type == DISK_CMD_WRITE || localDisk.task_queue->type == DISK_CMD_READ)
            {
                disk_cmd(localDisk.task_queue->type, localDisk.task_queue->block, localDisk.task_queue->buffer);

                // Atualiza a posição do cabeçote após a operação
                current_head_position = localDisk.task_queue->block;

                printf("Blocos percorridos: %d\n", blocks_traversed); // Exibe o número de blocos percorridos
            }
        }

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

disk_queue *schedule_fcfs(disk_queue *task_aux, int current_head_position)
{
    if (task_aux == NULL)
    {
        return NULL; // Se a fila estiver vazia, retorna NULL
    }

    // Calcula a distância entre a posição atual do cabeçote e o bloco da próxima operação

    blocks_traversed += abs(task_aux->block - current_head_position);

    return task_aux; // Retorna a primeira tarefa para processamento
}

disk_queue *schedule_sstf(disk_queue *task_aux, int current_head_position)
{
    if (task_aux == NULL)
    {
        return NULL; // Se a fila estiver vazia, retorna NULL
    }

    disk_queue *selected_task = task_aux;
    disk_queue *temp_task = task_aux->next;
    int shortest_distance = abs(task_aux->block - current_head_position);

    // Percorre a lista circular de tarefas procurando pela tarefa mais próxima
    while (temp_task != task_aux)
    {
        int distance = abs(temp_task->block - current_head_position);

        // Se encontrou uma tarefa mais próxima, atualiza a tarefa selecionada e a menor distância
        if (distance < shortest_distance)
        {
            selected_task = temp_task;
            shortest_distance = distance;
        }

        temp_task = temp_task->next;
    }

    blocks_traversed += abs(task_aux->block - current_head_position);

    return selected_task; // Retorna a tarefa mais próxima em termos de distância de busca
}

disk_queue *schedule_cscan(disk_queue *task_aux, int current_head_position, int num_blocks)
{
    if (task_aux == NULL)
    {
        return NULL; // Se a fila estiver vazia, retorna NULL
    }

    disk_queue *selected_task = task_aux;
    disk_queue *temp_task = task_aux->next;
    int min_block = num_blocks;
    int closest_cscan_distance = num_blocks * 2;
    int forward_tracks = 0;

    // Percorre a lista circular de tarefas procurando pela tarefa com a menor distância no sentido crescente
    while (temp_task != task_aux)
    {
        int distance = abs(temp_task->block - current_head_position);

        // Se encontrou uma tarefa com menor distância no sentido crescente, atualiza a tarefa selecionada
        if (distance < closest_cscan_distance)
        {
            closest_cscan_distance = distance;
            selected_task = temp_task;
            forward_tracks++;
        }

        temp_task = temp_task->next;
    }

    // Se não houver tarefas no sentido crescente, faz uma passagem completa no sentido inverso
    if (forward_tracks == 0)
    {
        // Reinicia a distância para encontrar a menor distância no sentido inverso
        closest_cscan_distance = num_blocks * 2;

        temp_task = task_aux->next;

        while (temp_task != task_aux)
        {
            int distance = abs(current_head_position - temp_task->block);

            // Se encontrou uma tarefa com menor distância no sentido inverso, atualiza a tarefa selecionada
            if (distance < closest_cscan_distance)
            {
                closest_cscan_distance = distance;
                selected_task = temp_task;
            }

            temp_task = temp_task->next;
        }
    }

    printf("%d\n",closest_cscan_distance);
    // Atualiza o número de blocos percorridos com a distância percorrida pela operação selecionada
    blocks_traversed += closest_cscan_distance;

    return selected_task; // Retorna a tarefa com a menor distância no sentido crescente no CSCAN
}

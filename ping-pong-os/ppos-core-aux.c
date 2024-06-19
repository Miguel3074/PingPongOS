#include "ppos.h"
#include "ppos-core-globals.h"

// ****************************************************************************
// Coloque aqui as suas modificações, p.ex. includes, defines variáveis,
// estruturas e funções
#include <sys/time.h>
#include <signal.h>

#define QUANTUM 20

struct sigaction action;
struct itimerval timer;
int time = 0;

void task_set_eet(task_t *task, int eet)
{
    if (task == NULL)
    {
        taskExec->timeExpected = eet;
    }
    else
    {
        task->timeExpected = eet;
        task->timeRemaining = eet;
    }
}

int task_get_eet(task_t *task)
{
    if (task == NULL)
        return taskExec->timeExpected;
    return task->timeExpected;
}

int task_get_ret(task_t *task)
{
    if (task == NULL)
    {
        return taskExec->timeRemaining;
    }
    return task->timeRemaining;
}

task_t *scheduler()
{
    task_t *task_return = readyQueue;
    int i;
    task_t *aux;

    if (readyQueue != NULL)
    {
        if (countTasks > 1)
            aux = readyQueue->next;
        else
            aux = readyQueue;

        i = 0;
        while (i < countTasks)
        {
            if (!(aux == taskMain || aux == taskDisp))
            {
                if (task_return == readyQueue || task_get_ret(task_return) > task_get_ret(aux))
                {
                    task_return = aux;
                }
            }
            aux = aux->next;
            i++;
        }
        return task_return;
    }
    return readyQueue;
}

void GerenciadorTempo(int signum)
{
    systemTime++;
    if (taskExec != taskMain && taskExec != taskDisp)
    {
        taskExec->running_time++;
        taskExec->timeRemaining--;
        taskExec->quantum--;
    }
    if (taskExec->quantum == 0)
    {
        task_yield();
    }
}

void temporizador()
{
    // timer.c

    // registra a ação para o sinal de timer SIGALRM
    action.sa_handler = GerenciadorTempo;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGALRM, &action, 0) < 0)
    {
        perror("Erro em sigaction: ");
        exit(1);
    }

    // ajusta valores do temporizador
    timer.it_value.tv_usec = 1000;
    timer.it_value.tv_sec = 0;
    timer.it_interval.tv_usec = 1000;
    timer.it_interval.tv_sec = 0;

    // arma o temporizador ITIMER_REAL (vide man setitimer)
    if (setitimer(ITIMER_REAL, &timer, 0) < 0)
    {
        perror("Erro em setitimer: ");
        exit(1);
    }
}

// ****************************************************************************

void before_ppos_init()
{
    // put your customization here
    printf("\nPPOS STARTED\n");
#ifdef DEBUG
    printf("\ninit - BEFORE");
#endif
    systemTime = 0;
    temporizador();
}

void after_ppos_init()
{
    // put your customization here
#ifdef DEBUG
    printf("\ninit - AFTER");
#endif
}

void before_task_create(task_t *task)
{
    // put your customization here
#ifdef DEBUG
    printf("\ntask_create - BEFORE - [%d]", task->id);
#endif
}

void after_task_create(task_t *task)
{
    // put your customization here
#ifdef DEBUG
    printf("\ntask_create - AFTER - [%d]", task->id);
#endif
    if (task != NULL)
    {
        task->execution_time = 0;
        task->activations = 0;
        task->quantum = QUANTUM;
        task_set_eet(task, 99999);
    }
}

void before_task_exit()
{
    // put your customization here
#ifdef DEBUG
    printf("\ntask_exit - BEFORE - [%d]", taskExec->id);
#endif
}

void after_task_exit()
{
    // put your customization here
    taskExec->execution_time = systime() - taskExec->execution_time;
    time = systime();
#ifdef DEBUG
    printf("\ntask_exit - AFTER- [%d]", taskExec->id);
#endif
    printf("\nTask %d exit: Execution time: %d ms; Processor time: %d ms; %d activations\n",
           taskExec->id,
           taskExec->execution_time,
           taskExec->running_time,
           taskExec->activations);
}

void before_task_switch(task_t *task)
{
    // put your customization here
#ifdef DEBUG
    printf("\ntask_switch - BEFORE - [%d -> %d]", taskExec->id, task->id);
#endif
    task->activations++;
}

void after_task_switch(task_t *task)
{
    // put your customization here
    task->quantum = QUANTUM;
#ifdef DEBUG
    printf("\ntask_switch - AFTER - [%d -> %d]", taskExec->id, task->id);
#endif
}

void before_task_yield()
{
    // put your customization here
#ifdef DEBUG
    printf("\ntask_yield - BEFORE - [%d]", taskExec->id);
#endif
}
void after_task_yield()
{
    // put your customization here
#ifdef DEBUG
    printf("\ntask_yield - AFTER - [%d]", taskExec->id);
#endif
}

void before_task_suspend(task_t *task)
{
    // put your customization here
#ifdef DEBUG
    printf("\ntask_suspend - BEFORE - [%d]", task->id);
#endif
}

void after_task_suspend(task_t *task)
{
    // put your customization here
#ifdef DEBUG
    printf("\ntask_suspend - AFTER - [%d]", task->id);
#endif
}

void before_task_resume(task_t *task)
{
    // put your customization here
#ifdef DEBUG
    printf("\ntask_resume - BEFORE - [%d]", task->id);
#endif
}

void after_task_resume(task_t *task)
{
    // put your customization here
#ifdef DEBUG
    printf("\ntask_resume - AFTER - [%d]", task->id);
#endif
}

void before_task_sleep()
{
    // put your customization here
#ifdef DEBUG
    printf("\ntask_sleep - BEFORE - [%d]", taskExec->id);
#endif
}

void after_task_sleep()
{
    // put your customization here
#ifdef DEBUG
    printf("\ntask_sleep - AFTER - [%d]", taskExec->id);
#endif
}

int before_task_join(task_t *task)
{
    // put your customization here
#ifdef DEBUG
    printf("\ntask_join - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_task_join(task_t *task)
{
    // put your customization here
#ifdef DEBUG
    printf("\ntask_join - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_create(semaphore_t *s, int value)
{
    // put your customization here
#ifdef DEBUG
    printf("\nsem_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_create(semaphore_t *s, int value)
{
    // put your customization here
#ifdef DEBUG
    printf("\nsem_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_down(semaphore_t *s)
{
    // put your customization here
#ifdef DEBUG
    printf("\nsem_down - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_down(semaphore_t *s)
{
    // put your customization here
#ifdef DEBUG
    printf("\nsem_down - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_up(semaphore_t *s)
{
    // put your customization here
#ifdef DEBUG
    printf("\nsem_up - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_up(semaphore_t *s)
{
    // put your customization here
#ifdef DEBUG
    printf("\nsem_up - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_destroy(semaphore_t *s)
{
    // put your customization here
#ifdef DEBUG
    printf("\nsem_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_destroy(semaphore_t *s)
{
    // put your customization here
#ifdef DEBUG
    printf("\nsem_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_create(mutex_t *m)
{
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_create(mutex_t *m)
{
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_lock(mutex_t *m)
{
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_lock - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_lock(mutex_t *m)
{
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_lock - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_unlock(mutex_t *m)
{
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_unlock - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_unlock(mutex_t *m)
{
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_unlock - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_destroy(mutex_t *m)
{
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_destroy(mutex_t *m)
{
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_create(barrier_t *b, int N)
{
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_create(barrier_t *b, int N)
{
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_join(barrier_t *b)
{
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_join - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_join(barrier_t *b)
{
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_join - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_destroy(barrier_t *b)
{
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_destroy(barrier_t *b)
{
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_create(mqueue_t *queue, int max, int size)
{
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_create(mqueue_t *queue, int max, int size)
{
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_send(mqueue_t *queue, void *msg)
{
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_send - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_send(mqueue_t *queue, void *msg)
{
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_send - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_recv(mqueue_t *queue, void *msg)
{
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_recv - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_recv(mqueue_t *queue, void *msg)
{
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_recv - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_destroy(mqueue_t *queue)
{
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_destroy(mqueue_t *queue)
{
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_msgs(mqueue_t *queue)
{
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_msgs - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_msgs(mqueue_t *queue)
{
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_msgs - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

// Cria um semáforo com valor inicial "value"
int sem_create(semaphore_t *s, int value) {
    if (s == NULL)
        return -1;

    s->value = value;
    s->queue = NULL;
    s->active = 1;

    return 0;
}

// Requisita o semáforo
int sem_down(semaphore_t *s) {
    if (s == NULL || !s->active)
        return -1;

    s->value--;

    if (s->value < 0) {
        task_t *task = taskExec;
        task->state = 's'; // Suspended state
        queue_append((queue_t **)&s->queue, (queue_t *)task);
        task_yield(); // Suspends the current task
    }

    return 0;
}

// Libera o semáforo
int sem_up(semaphore_t *s) {
    if (s == NULL || !s->active)
        return -1;

    s->value++;

    if (s->value <= 0) {
        task_t *task = (task_t *)queue_remove((queue_t **)&s->queue, (queue_t *)s->queue);
        if (task != NULL)
            task->state = 'r'; // Ready state
    }

    return 0;
}

// Destroi o semáforo, liberando as tarefas bloqueadas
int sem_destroy(semaphore_t *s) {
    if (s == NULL)
        return -1;

    s->active = 0;

    while (s->queue != NULL) {
        task_t *task = (task_t *)queue_remove((queue_t **)&s->queue, (queue_t *)s->queue);
        if (task != NULL)
            task->state = 'r'; // Ready state
    }

    return 0;
}

// Implementação do corpo das funções de filas de mensagens

// Cria uma fila para até max mensagens de size bytes cada
int mqueue_create(mqueue_t *queue, int max, int size) {
    if (queue == NULL)
        return -1;

    queue->content = malloc(max * size);
    if (queue->content == NULL)
        return -1;

    queue->messageSize = size;
    queue->maxMessages = max;
    queue->countMessages = 0;

    sem_create(&queue->sBuffer, 1);
    sem_create(&queue->sItem, 0);
    sem_create(&queue->sVaga, max);

    queue->active = 1;

    return 0;
}

// Envia uma mensagem para a fila
int mqueue_send(mqueue_t *queue, void *msg) {
    if (queue == NULL || msg == NULL || !queue->active)
        return -1;

    sem_down(&queue->sVaga);
    sem_down(&queue->sBuffer);

    // Copy the message to the queue
    void *destination = (char *)queue->content + (queue->countMessages * queue->messageSize);
    memcpy(destination, msg, queue->messageSize);

    queue->countMessages++;

    sem_up(&queue->sBuffer);
    sem_up(&queue->sItem);

    return 0;
}

// Recebe uma mensagem da fila
int mqueue_recv(mqueue_t *queue, void *msg) {
    if (queue == NULL || msg == NULL || !queue->active)
        return -1;

    sem_down(&queue->sItem);
    sem_down(&queue->sBuffer);

    // Copy the message from the queue
    void *source = (char *)queue->content + ((queue->countMessages - 1) * queue->messageSize);
    memcpy(msg, source, queue->messageSize);

    queue->countMessages--;

    sem_up(&queue->sBuffer);
    sem_up(&queue->sVaga);

    return 0;
}

// Destroi a fila, liberando as tarefas bloqueadas
int mqueue_destroy(mqueue_t *queue) {
    if (queue == NULL)
        return -1;
}  

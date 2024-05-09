// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.2 -- Outubro de 2016

#include <stdio.h>
#include <stdlib.h>

// Interface do núcleo para as aplicações
#ifndef __PPOS__
#define __PPOS__

// macros importantes ==========================================================

// habilita compatibilidade POSIX no MacOS X (para ucontext.h)
#define _XOPEN_SOURCE 600

// este código deve ser compilado em sistemas UNIX-like
#if defined(_WIN32) || (!defined(__unix__) && !defined(__unix) && (!defined(__APPLE__) || !defined(__MACH__)))
#warning Este codigo foi planejado para ambientes UNIX (LInux, *BSD, MacOS). A compilacao e execucao em outros ambientes e responsabilidade do usuario.
#endif

// otimizações podem atrapalhar código que manipula contexto
#ifdef __OPTIMIZE__
#error "Please do not use optimizations (-O1, -O2, ...)"
#endif

#include "ppos_data.h"		// estruturas de dados necessárias

// funções gerais ==============================================================

/*
*** IMPORTANTE ***
As funções before_* e after_* devem ser implementadas nos respectivos arquivos *.inc. 
Ex. before_ppos_init() e after_ppos_init() devem ser implementados no arquivo ppos_init.inc

As funções before_* são chamadas na primeira linha de código da função.
Nao existe nenhuma variavel ou estrutura de dados inicializada antes que essa funcao eh chamada.

As funcoes after_* são chamadas apos todas as variaveis e estruturas de dados do PPOS 
estarem alocadas/desalocadas e inicializadas dentro do contexto e dos objetivos
individuais de cada função. 
Essas funcoes nao sao necessariamente o ultimo comando a ser executado dentro do contexto
da funcao base.
Ex. a função after_task_create()) eh chamada após todas as variaveis e estruturas de dados
da tarefa estarem alocadas e devidademente inicializadas. Após o retorno dessa funcao, a 
nova tarefa é incluída na fila de tarefas prontas.
*/

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init ();
void before_ppos_init ();
void after_ppos_init (); // Após o retorno dessa funcao, o PPOS troca imediatamente 
                         // o contexto para o despachante de tarefas.

// gerência de tarefas =========================================================

// Cria uma nova tarefa. Retorna um ID> 0 ou erro.
int task_create (task_t *task,			// descritor da nova tarefa
                 void (*start_func)(void *),	// funcao corpo da tarefa
                 void *arg) ;			// argumentos para a tarefa
void before_task_create (task_t *task );
void after_task_create (task_t *task );  // Após o retorno dessa funcao, a nova tarefa é incluída na
                                         // fila de tarefas prontas.

// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit (int exitCode) ;
void before_task_exit ();
void after_task_exit (); // Após o retorno dessa funcao, eh realiada a troca de contexto para 
                         // a tarefa do dispatcher ou a tarefa main()

// alterna a execução para a tarefa indicada
int task_switch (task_t *task) ;
void before_task_switch ( task_t *task ) ;
void after_task_switch ( task_t *task ) ; // Após o retorno dessa funcao, eh realiada a troca de 
                                          // contexto [swapcontext()] para a tarefa indicada no parâmetro


// retorna o identificador da tarefa corrente (main deve ser 0)
int task_id () ;

// operações de escalonamento ==================================================

// libera o processador para a próxima tarefa, retornando à fila de tarefas
// prontas ("ready queue")
void task_yield () ;
void before_task_yield () ;
void after_task_yield () ;

// suspende/bloqueia uma tarefa, colocando-a na lista queue
void task_suspend( task_t *task, task_t **queue ) ;
void before_task_suspend( task_t *task ) ;
void after_task_suspend( task_t *task ) ;

// resume a execução de uma tarefa suspensa
void task_resume(task_t *task) ;
void before_task_resume(task_t *task) ;
void after_task_resume(task_t *task) ;

// define a prioridade estática de uma tarefa (ou a tarefa atual)
void task_setprio (task_t *task, int prio) ;

// retorna a prioridade estática de uma tarefa (ou a tarefa atual)
int task_getprio (task_t *task) ;

// retorna a proxima tarefa a ser executada conforme a politica de escalonamento
task_t * scheduler() ;

// operações de gestão do tempo ================================================

// suspende a tarefa corrente por t milissegundos
// essa função só funciona se o relógio do sistema estiver implementado
void task_sleep (int t) ;
void before_task_sleep () ;
void after_task_sleep () ;

// retorna o valor atual do relógio do sistema (em milisegundos)
unsigned int systime () ;

// operações de sincronização ==================================================

// a tarefa corrente aguarda o encerramento de outra task
int task_join (task_t *task) ;
int before_task_join (task_t *task) ;
int after_task_join (task_t *task) ;

// operações de IPC ============================================================

// semáforos

// cria um semáforo com valor inicial "value"
int sem_create (semaphore_t *s, int value) ;
int before_sem_create (semaphore_t *s, int value) ;
int after_sem_create (semaphore_t *s, int value) ;

// requisita o semáforo
int sem_down (semaphore_t *s) ;
int before_sem_down (semaphore_t *s) ;
int after_sem_down (semaphore_t *s) ;

// libera o semáforo
int sem_up (semaphore_t *s) ;
int before_sem_up (semaphore_t *s) ;
int after_sem_up (semaphore_t *s) ;

// destroi o semáforo, liberando as tarefas bloqueadas
int sem_destroy (semaphore_t *s) ;
int before_sem_destroy (semaphore_t *s) ;
int after_sem_destroy (semaphore_t *s) ;

// mutexes

// Inicializa um mutex (sempre inicialmente livre)
int mutex_create (mutex_t *m) ;
int before_mutex_create (mutex_t *m) ;
int after_mutex_create (mutex_t *m) ;

// Solicita um mutex
int mutex_lock (mutex_t *m) ;
int before_mutex_lock (mutex_t *m) ;
int after_mutex_lock (mutex_t *m) ;

// Libera um mutex
int mutex_unlock (mutex_t *m) ;
int before_mutex_unlock (mutex_t *m) ;
int after_mutex_unlock (mutex_t *m) ;

// Destrói um mutex
int mutex_destroy (mutex_t *m) ;
int before_mutex_destroy (mutex_t *m) ;
int after_mutex_destroy (mutex_t *m) ;

// barreiras

// Inicializa uma barreira
int barrier_create (barrier_t *b, int N) ;
int before_barrier_create (barrier_t *b, int N) ;
int after_barrier_create (barrier_t *b, int N) ;

// Chega a uma barreira
int barrier_join (barrier_t *b) ;
int before_barrier_join (barrier_t *b) ;
int after_barrier_join (barrier_t *b) ;

// Destrói uma barreira
int barrier_destroy (barrier_t *b) ;
int before_barrier_destroy (barrier_t *b) ;
int after_barrier_destroy (barrier_t *b) ;

// filas de mensagens

// cria uma fila para até max mensagens de size bytes cada
int mqueue_create (mqueue_t *queue, int max, int size) ;
int before_mqueue_create (mqueue_t *queue, int max, int size) ;
int after_mqueue_create (mqueue_t *queue, int max, int size) ;

// envia uma mensagem para a fila
int mqueue_send (mqueue_t *queue, void *msg) ;
int before_mqueue_send (mqueue_t *queue, void *msg) ;
int after_mqueue_send (mqueue_t *queue, void *msg) ;

// recebe uma mensagem da fila
int mqueue_recv (mqueue_t *queue, void *msg) ;
int before_mqueue_recv (mqueue_t *queue, void *msg) ;
int after_mqueue_recv (mqueue_t *queue, void *msg) ;

// destroi a fila, liberando as tarefas bloqueadas
int mqueue_destroy (mqueue_t *queue) ;
int before_mqueue_destroy (mqueue_t *queue) ;
int after_mqueue_destroy (mqueue_t *queue) ;

// informa o número de mensagens atualmente na fila
int mqueue_msgs (mqueue_t *queue) ;
int before_mqueue_msgs (mqueue_t *queue) ;
int after_mqueue_msgs (mqueue_t *queue) ;

// funcao para debug. imprime os campos da estrutura task_t
void print_tcb( task_t* task );

//==============================================================================

// Redefinir funcoes POSIX "proibidas" como "FORBIDDEN" (gera erro ao compilar)

// POSIX threads
#define pthread_create			FORBIDDEN
#define pthread_join			FORBIDDEN
#define pthread_exit			FORBIDDEN

// POSIX semaphores
#define sem_init			FORBIDDEN
#define sem_post			FORBIDDEN
#define sem_wait			FORBIDDEN
#define sem_trywait			FORBIDDEN

// POSIX message queues
#define mq_open				FORBIDDEN
#define mq_send				FORBIDDEN
#define mq_receive			FORBIDDEN
#define mq_close			FORBIDDEN

// POSIX condvars
#define pthread_cond_init		FORBIDDEN
#define pthread_cond_wait		FORBIDDEN
#define pthread_cond_signal		FORBIDDEN
#define pthread_cond_timedwait		FORBIDDEN

// POSIX barriers
#define pthread_barrier_init		FORBIDDEN
#define pthread_barrier_wait		FORBIDDEN
#define pthread_barrier_destroy		FORBIDDEN

// POSIX mutexes
#define pthread_mutex_init		FORBIDDEN
#define pthread_mutex_lock		FORBIDDEN
#define pthread_mutex_unlock		FORBIDDEN
#define pthread_mutex_timedlock		FORBIDDEN
#define pthread_mutex_trylock		FORBIDDEN
#define pthread_mutex_destroy		FORBIDDEN

// POSIX RW-locks
#define pthread_rwlock_init		FORBIDDEN
#define pthread_rwlock_rdlock		FORBIDDEN
#define pthread_rwlock_wrlock		FORBIDDEN
#define pthread_rwlock_unlock		FORBIDDEN
#define pthread_rwlock_tryrdlock	FORBIDDEN
#define pthread_rwlock_tryrwlock	FORBIDDEN
#define pthread_rwlock_timedrdlock	FORBIDDEN
#define pthread_rwlock_timedrwlock	FORBIDDEN
#define pthread_rwlock_destroy		FORBIDDEN

// POSIX spinlocks
#define pthread_spin_init		FORBIDDEN
#define pthread_spin_lock		FORBIDDEN
#define pthread_spin_unlock		FORBIDDEN
#define pthread_spin_trylock		FORBIDDEN
#define pthread_spin_destroy		FORBIDDEN


//==============================================================================
// Constantes, defines, macros do PingPong OS

#define PPOS_TASK_STATE_NEW        'N'
#define PPOS_TASK_STATE_READY      'R'
#define PPOS_TASK_STATE_EXECUTING  'X'
#define PPOS_TASK_STATE_SUSPENDED  'S'
#define PPOS_TASK_STATE_TERMINATED 'T'

#define STACKSIZE              32768

#define PRINT_READY_QUEUE      queue_print ("Ready Queue", (queue_t*)readyQueue, (void*)&print_tcb );
#define PPOS_PREEMPT_ENABLE    preemption = 1;
#define PPOS_PREEMPT_DISABLE   preemption = 0;
#define PPOS_IS_PREEMPT_ACTIVE (preemption == 1)

#endif

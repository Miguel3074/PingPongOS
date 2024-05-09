#ifndef __PPOS_CORE_GLOBALS__
#define __PPOS_CORE_GLOBALS__

// Declaracao das variaveis globais usadas no PPOS
extern task_t* taskMain;  // Representa a funcao the main()
extern task_t* taskExec; // Ponteiro para a TCB da tarefa em execucao
extern task_t* taskDisp;  // Ponteiro para a tarefa de escalonamento (dispatcher)
extern task_t* freeTask; // Ponteiro para a tarefa que terminou
extern task_t* readyQueue; // Ponteiro para a fila de tarefas prontas
extern task_t* sleepQueue; // Ponteiro para a fila de tarefas dormindo
extern long nextid;        // Valor do proximo ID a ser usado pelo task_create()
extern long countTasks;    // Total de tarefas de usuario
extern unsigned char preemption; // indica se pode haver preempcao no momento. 
                                // Valor 1 indica que a preempcao esta habilido, 
                                // qualquer outro valor indica desabilitado
extern unsigned int systemTime; // conta o tempo global do sistema, em ticks do relogio

#endif
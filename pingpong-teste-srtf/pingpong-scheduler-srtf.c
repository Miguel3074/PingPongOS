// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.1 -- Julho de 2016

// Teste do escalonador por prioridades dinâmicas

#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"
#include "ppos-core-globals.h"

//task_t Pang, Peng, Ping, Pong, Pung ;

#define USER_TASKS_MAX 5
task_t user_tasks[USER_TASKS_MAX];
char user_tasks_names[USER_TASKS_MAX][15];
//int user_tasks_execution_time[USER_TASKS_MAX] = {100, 200, 300, 400, 500, 600, 700, 800, 900, 1000}; // cenario 1
//int user_tasks_execution_time[USER_TASKS_MAX] = {1000, 900, 800, 700, 600, 500, 400, 300, 200, 100}; // cenario 2
int user_tasks_execution_time[USER_TASKS_MAX] = {30, 50, 70, 90, 110, 130, 150, 170, 190, 210}; // cenario 3
//int user_tasks_execution_time[USER_TASKS_MAX] = {210, 190, 170, 150, 130, 110, 90, 70, 50, 30}; // cenario 4

int one_tick = 0;

int new_tasks_count = 0;
char new_task_name[15];
task_t new_user_tasks[50];
int last_created_task = 0;



// corpo das threads
void Body (void * arg)
{
  int i ;
  int end_time = one_tick * task_get_eet(NULL);
  int last_printed_line = 0;

  printf ("[%d]\t%s: inicio (tempo de execucao %d)\n", systime(), (char *) arg, task_get_eet(NULL)) ;
  last_printed_line = systime();

  // o campo taskExec->running_time indica o tempo que a tarefa executou ate o momento
  // se for o caso, esse campo pode ser trocado conforme a implementacao de cada equipe
  // o que importa eh esse loop sair somente se a tarefa realmente executou o X tempo que
  // foi indicado como seu tempo de execucao
  while (taskExec->running_time < task_get_eet(NULL)) {
    end_time--;
    if ((last_printed_line+5) <= systime()) {
      printf ("[%d]\t%s: interacao %d\t\t%d\n", systime(), (char *) arg, end_time, taskExec->running_time) ;
      last_printed_line = systime();
    }

    if ((last_created_task != systime()) && (systime()%100) == 0) {
      last_created_task = systime();
      // cria uma tarefa com prioridade mais alta
      sprintf(new_task_name, "NEWTask[%2d]", new_tasks_count);
      printf("Criando NOVA tarefa: %s\n", new_task_name);
      task_create (&new_user_tasks[new_tasks_count], Body, &new_task_name) ;
      task_set_eet(&new_user_tasks[new_tasks_count], 15);
      new_tasks_count++;
      task_yield();
    }
  }
  printf ("[%d]\t%s: fim\n", systime(), (char *) arg) ;
  task_exit (0) ;
}

int main (int argc, char *argv[])
{
  int i, aux_time;
  
  printf ("main: inicio\n");

  ppos_init () ;

  // waiting for the first microsecond
  while (systime() <= 0) ;
  // estimate how many iterations is a microsecond
  aux_time = systime() + 1;
  while (systime() < aux_time)
    one_tick++;
  // adjusting value
  //one_tick = (one_tick*90)/100;
  printf("Loop iterations to microseconds = %d\n", one_tick);


  // creating tasks
  for (i=0; i<USER_TASKS_MAX; i++) {
    sprintf(&user_tasks_names[i][0], "UTask[%2d]", i);
    printf("Criando tarefa: %s\n", &user_tasks_names[i]);
    task_create (&user_tasks[i], Body, &user_tasks_names[i]) ;
    task_set_eet(&user_tasks[i], user_tasks_execution_time[i]);
  }

  task_yield () ;

  printf ("main: fim\n");
  exit (0) ;
}

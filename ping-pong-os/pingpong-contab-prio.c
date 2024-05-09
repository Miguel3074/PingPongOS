// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.1 -- Julho de 2016

// Teste da contabilização - tarefas com prioridades distintas

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> 
#include "ppos.h"

#define WORKLOAD 40000

task_t Pang, Peng, Ping, Pong, Pung ;

// simula um processamento pesado
int hardwork (int n)
{
   int i, j, soma ;

   soma = 0 ;
   for (i=0; i<n; i++)
      for (j=0; j<n; j++)
         soma += j ;
   return (soma) ;
}

// corpo das threads
void Body (void * arg)
{
   printf ("%s: inicio em %4d ms (prio: %d)\n", (char *) arg,
           systime(), task_getprio(NULL)) ;
   hardwork (WORKLOAD) ;
   printf ("%s: fim    em %4d ms\n", (char *) arg, systime()) ;
   task_exit (0) ;
}

int main (int argc, char *argv[])
{
   printf ("main: inicio\n");

   ppos_init () ;

   srand(time(NULL));

   task_create (&Pang, Body, "    Pang") ;
   task_setprio (&Pang, 0);
   sleep(rand() % 3);
   
   task_create (&Peng, Body, "        Peng") ;
   task_setprio (&Peng, -2);
   sleep(rand() % 3);
   
   task_create (&Ping, Body, "            Ping") ;
   task_setprio (&Ping, -4);
   sleep(rand() % 3);
   
   task_create (&Pong, Body, "                Pong") ;
   task_setprio (&Pong, -6);
   sleep(rand() % 3);
   
   task_create (&Pung, Body, "                    Pung") ;
   task_setprio (&Pung, -8);
   sleep(rand() % 3);
   
   task_join(&Pung);
   task_join(&Pong);
   task_join(&Ping);
   task_join(&Peng);
   task_join(&Pang);

   printf ("main: fim\n");
   exit (0);
}

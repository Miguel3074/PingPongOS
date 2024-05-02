// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.1 -- Julho de 2016

// Teste da preempção por tempo - teste de stress com muitas threads

#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"

#define WORKLOAD 5000
#define NUMTASKS 500

task_t task[NUMTASKS] ;

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
   int i ;

   for (i=0; i<3; i++)
   {
      printf ("task %3d: i=%d\n", task_id (), i) ;
      hardwork (WORKLOAD) ;
   }
   printf ("task %3d: fim\n", task_id ()) ;
   task_exit (0) ;
}

int main (int argc, char *argv[])
{
   int i ;
   printf ("main: inicio\n");

   ppos_init () ;

   for (i=0; i<NUMTASKS; i++)
   {
      printf ("Creating task %d\n", i+2) ;
      task_create (&task[i], Body, NULL) ;
   }

   //task_yield () ;
   for (i=0; i<NUMTASKS; i++)
      task_join(&task[i]);

   printf ("main: fim\n");
   exit (0);
}

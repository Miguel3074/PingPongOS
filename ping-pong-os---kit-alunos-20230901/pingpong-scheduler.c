// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.1 -- Julho de 2016

// Teste do escalonador por prioridades dinâmicas

#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"

task_t Pang, Peng, Ping, Pong, Pung ;

// corpo das threads
void Body (void * arg)
{
  int i ;

  printf ("%s: inicio (prioridade %d)\n", (char *) arg, task_getprio(NULL)) ;

  for (i=0; i<10; i++)
  {
    printf ("%s: %d\n", (char *) arg, i) ;
    task_yield ();
  }
  printf ("%s: fim\n", (char *) arg) ;
  task_exit (0) ;
}

int main (int argc, char *argv[])
{
  printf ("main: inicio\n");

  ppos_init () ;

  task_create (&Pang, Body, "    Pang") ;
  task_setprio (&Pang, 0);

  task_create (&Peng, Body, "        Peng") ;
  task_setprio (&Peng, 2);

  task_create (&Ping, Body, "            Ping") ;
  task_setprio (&Ping, 4);

  task_create (&Pong, Body, "                Pong") ;
  task_setprio (&Pong, 6);

  task_create (&Pung, Body, "                    Pung") ;
  task_setprio (&Pung, 8);

  task_join(&Pang);
  task_join(&Peng);
  task_join(&Ping);
  task_join(&Ping);
  task_join(&Pung);

  printf ("main: fim\n");
  exit (0) ;
}

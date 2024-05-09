#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ppos.h"

// operating system check
#if defined(_WIN32) || (!defined(__unix__) && !defined(__unix) && (!defined(__APPLE__) || !defined(__MACH__)))
#warning Este codigo foi planejado para ambientes UNIX (LInux, *BSD, MacOS). A compilacao e execucao em outros ambientes e responsabilidade do usuario.
#endif

#define NUMTASKS 30
#define NUMSTEPS 10000000

task_t task[NUMTASKS] ;
semaphore_t  s ;
long int soma = 0 ;

void taskBody(char *id)
{
   int i ;

   for (i=0; i< NUMSTEPS; i++)
   {
      sem_down (&s) ;
      //printf("\n%s\t\t\tSETP = %d\t\tSOMA = %d", id, i, soma);
      //fflush(stdout);
      soma += 1 ;
      sem_up (&s) ;
   }

   task_exit (0) ;
}

int main (int argc, char *argv[])
{
   int i ;
   
   printf ("Main INICIO\n") ;

   ppos_init () ;

   sem_create (&s, 1) ;

   printf ("%d tarefas somando %d vezes cada, aguarde...\n",
           NUMTASKS, NUMSTEPS) ;

   for (i=0; i<NUMTASKS; i++) {
     char *task_name = malloc(20);
     memset(task_name, 0, 20);
     sprintf(task_name, "Task %d", i);
     printf("\nCreating %s", task_name);
     task_create (&task[i], taskBody, task_name);
   }
   fflush(stdout);

   for (i=0; i<NUMTASKS; i++)
     task_join (&task[i]) ;

   sem_destroy (&s) ;

   if (soma == (NUMTASKS*NUMSTEPS))
     printf ("Soma deu %ld, valor correto!\n", soma) ;
   else
     printf ("Soma deu %ld, mas deveria ser %d!\n",
             soma, NUMTASKS*NUMSTEPS) ;

   task_exit (0) ;

   exit (0) ;
}

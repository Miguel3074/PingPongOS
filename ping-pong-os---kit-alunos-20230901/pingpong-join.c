#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"

// operating system check
#if defined(_WIN32) || (!defined(__unix__) && !defined(__unix) && (!defined(__APPLE__) || !defined(__MACH__)))
#warning Este codigo foi planejado para ambientes UNIX (LInux, *BSD, MacOS). A compilacao e execucao em outros ambientes e responsabilidade do usuario.
#endif

task_t Pang, Peng, Ping, Pong, Pung ;

int Limite = 3 ;

void Body (void * arg)
{
   int i,j,max ;

   max = Limite ;
   Limite += 2 ;

   for (i=0; i<max; i++)
   {
      printf ("%s %d\n", (char *) arg, i) ;
      for (j=0; j<10000000; j++) ;
   }
   printf ("%s FIM\n", (char *) arg) ;
   task_exit (max) ;
}

int main (int argc, char *argv[])
{
   int i,j, ret ;
   
   ppos_init () ;

   printf ("Main INICIO\n");

   task_create (&Pang, Body, "    Pang") ;
   task_create (&Peng, Body, "        Peng") ;
   task_create (&Ping, Body, "            Ping") ;
   task_create (&Pong, Body, "                Pong") ;
   task_create (&Pung, Body, "                    Pung") ;

   for (i=0; i<2; i++)
   {
      printf ("Main %d\n", i) ;
      for (j=0; j<10000000; j++) ;
   }

   printf ("Main espera Pang...\n") ;
   ret = task_join (&Pang) ;
   printf ("Pang encerrou com exit code %d\n", ret) ;

   printf ("Main espera Peng...\n") ;
   ret = task_join (&Peng) ;
   printf ("Peng encerrou com exit code %d\n", ret) ;

   printf ("Main espera Ping...\n") ;
   ret = task_join (&Ping) ;
   printf ("Ping encerrou com exit code %d\n", ret) ;

   printf ("Main espera Pong...\n") ;
   ret = task_join (&Pong) ;
   printf ("Pong encerrou com exit code %d\n", ret) ;

   printf ("Main espera Pung...\n") ;
   ret = task_join (&Pung) ;
   printf ("Pung encerrou com exit code %d\n", ret) ;

   printf ("Main FIM\n");

   task_exit (0) ;

   exit (0) ;
}

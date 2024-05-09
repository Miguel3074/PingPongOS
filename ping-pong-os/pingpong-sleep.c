#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "ppos.h"

// operating system check
#if defined(_WIN32) || (!defined(__unix__) && !defined(__unix) && (!defined(__APPLE__) || !defined(__MACH__)))
#warning Este codigo foi planejado para ambientes UNIX (LInux, *BSD, MacOS). A compilacao e execucao em outros ambientes e responsabilidade do usuario.
#endif

task_t Pang, Peng, Ping, Pong, Pung ;

void Body (void * arg)
{
   int i, sleepTime ;

   for (i=0; i<10; i++)
   {
      sleepTime = random() % 5 ;
      printf ("%5d ms: %s %d (dorme %ds)\n",
              systime(), (char *) arg, i, sleepTime) ;
      task_sleep (sleepTime) ;
   }
   printf ("%5d ms: %s FIM\n", systime(), (char *) arg) ;
   task_exit (0) ;
}

int main (int argc, char *argv[])
{
   ppos_init ();

   printf ("%5d ms: Main INICIO\n", systime()) ;

   task_create (&Pang, Body, "    Pang") ;
   task_create (&Peng, Body, "        Peng") ;
   task_create (&Ping, Body, "            Ping") ;
   task_create (&Pong, Body, "                Pong") ;
   task_create (&Pung, Body, "                    Pung") ;

   printf ("%5d ms: Main espera Pang...\n", systime()) ;
   task_join (&Pang) ;
   printf ("%5d ms: Pang acabou\n", systime()) ;

   printf ("%5d ms: Main espera Peng...\n", systime()) ;
   task_join (&Peng) ;
   printf ("%5d ms: Peng acabou\n", systime()) ;

   printf ("%5d ms: Main espera Ping...\n", systime()) ;
   task_join (&Ping) ;
   printf ("%5d ms: Ping acabou\n", systime()) ;

   printf ("%5d ms: Main espera Pong...\n", systime()) ;
   task_join (&Pong) ;
   printf ("%5d ms: Pong acabou\n", systime()) ;

   printf ("%5d ms: Main espera Pung...\n", systime()) ;
   task_join (&Pung) ;
   printf ("%5d ms: Pung acabou\n", systime()) ;

   printf ("%5d ms: Main FIM\n", systime()) ;
   task_exit (0) ;

   exit (0) ;
}

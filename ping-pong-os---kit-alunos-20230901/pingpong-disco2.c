// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.1 -- Julho de 2016

// Teste das operações de acesso a disco com múltiplas threads
// fazendo operações de leitura e escrita em disco simultâneas.

#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"
#include "ppos_disk.h"

#define NUMTASKS  16

// tarefas
task_t mover[NUMTASKS] ;	// tarefas movedoras de blocos
int numblocks ;			// numero de blocos no disco
int blocksize ;			// tamanho de cada bloco (bytes)

// corpo das threads "mover"
void moverBody (void * arg)
{
  long myNumber ;
  int i, j, block_orig, block_dest ;
  char *buffer1, *buffer2 ;
  int blocksPerTask = (numblocks/NUMTASKS/2) ;

  buffer1 = malloc (blocksize) ;
  buffer2 = malloc (blocksize) ;

  // define os blocos iniciais
  myNumber = (long) arg ;
  block_orig = myNumber * blocksPerTask ;
  block_dest = numblocks - 1 - (myNumber * blocksPerTask) ;

  printf ("T%02d movendo %2d blocos entre bloco %3d e bloco %3d\n",
          task_id(), blocksPerTask, block_orig, block_dest) ;

  // move blocksPerTask blocos
  for (i = 0 ; i < blocksPerTask; i++)
  {
    // le o bloco b1 do disco
    printf ("T%02d vai ler bloco %3d\n", task_id(), block_orig) ;
    if (disk_block_read (block_orig, buffer1) == 0)
      printf ("T%02d leu bloco %3d\n", task_id(), block_orig) ;
    else
      printf ("T%02d erro ao ler bloco %3d\n", task_id(), block_orig) ;

    // le o bloco b2 do disco
    printf ("T%02d vai ler bloco %3d\n", task_id(), block_dest) ;
    if (disk_block_read (block_dest, buffer2) == 0)
      printf ("T%02d leu bloco %3d\n", task_id(), block_dest) ;
    else
      printf ("T%02d erro ao ler bloco %3d\n", task_id(), block_dest) ;

    // mostra o conteudo do bloco b1
    printf ("T%02d bloco %3d tem: [", task_id(), block_orig) ;
    for (j = 0; j < blocksize; j++)
      printf ("%c", buffer1[j]) ;
    printf ("]\n") ;

    // mostra o conteudo do bloco b2
    printf ("T%02d bloco %3d tem: [", task_id(), block_dest) ;
    for (j = 0; j < blocksize; j++)
      printf ("%c", buffer2[j]) ;
    printf ("]\n") ;

    // escreve o bloco b1 no disco
    printf ("T%02d vai escrever bloco %3d\n", task_id(), block_dest) ;
    if (disk_block_write (block_dest, buffer1) == 0)
      printf ("T%02d escreveu bloco %3d\n", task_id(), block_dest) ;
    else
      printf ("T%02d erro ao escrever bloco %3d\n", task_id(), block_dest) ;

    // escreve o bloco b2 no disco
    printf ("T%02d vai escrever bloco %3d\n", task_id(), block_orig) ;
    if (disk_block_write (block_orig, buffer2) == 0)
      printf ("T%02d escreveu bloco %3d\n", task_id(), block_orig) ;
    else
      printf ("T%02d erro ao escrever bloco %3d\n", task_id(), block_orig) ;

    // define os proximos blocos
    block_orig++ ;
    block_dest-- ;
  }
  printf ("T%02d terminou\n", task_id()) ;
  free (buffer1) ;
  free (buffer2) ;
  task_exit (0) ;
}

int main (int argc, char *argv[])
{
  long i ;

  printf ("main: inicio\n") ;

  // inicializa o sistema operacional
  ppos_init () ;

  // inicializa o gerente de disco
  if (disk_mgr_init (&numblocks, &blocksize) < 0)
  {
    printf ("Erro na abertura do disco\n") ;
    exit (1) ;
  }
  printf ("Disco contem %d blocos de %d bytes cada\n", numblocks, blocksize) ;

  // cria as tarefas
  for (i = 0; i < NUMTASKS; i++)
   task_create (&mover[i], moverBody, (long*) i) ;

  // aguarda todas as tarefas encerrarem
  for (i = 0; i < NUMTASKS; i++)
   task_join (&mover[i]) ;

  // encerra a thread main
  printf ("main: fim\n") ;
  task_exit (0) ;

  exit(0) ;
}

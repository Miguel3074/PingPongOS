// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.2 -- Agosto de 2020

// Teste das operações de acesso a disco com uma única thread (main),
// que lê todos os blocos do disco e escreve blocos com caracteres aleatórios

#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"
#include "ppos_disk.h"

int numblocks ;			// numero de blocos no disco
int blocksize ;			// tamanho de cada bloco (bytes)

int main (int argc, char *argv[])
{
  int i, j ;
  unsigned char c, *buffer ;

  printf ("%5d ms: main inicio\n", systime() ) ;

  // inicializa o sistema operacional
  ppos_init () ;

  // inicializa o gerente de disco
  if (disk_mgr_init (&numblocks, &blocksize) < 0)
  {
    printf ("Erro na abertura do disco\n") ;
    exit (1) ;
  }
  else
    printf ("%5d ms: disco contem %d blocos de %d bytes cada\n",
            systime(), numblocks, blocksize) ;

  // aloca o buffer para ler blocos do disco
  buffer = malloc (blocksize) ;
  if (! buffer)
  {
    perror ("malloc") ;
    exit (1) ;
  }

  // lê e imprime todos os blocos do disco, um a um
  for (i=0; i<numblocks; i++)
  {
    // lê o bloco i do disco no buffer
    printf ("%5d ms: lendo bloco %d\n", systime(), i) ;
    if (disk_block_read (i, buffer))
      printf ("Erro!\n") ;

    // mostra o conteudo do buffer
    printf ("%5d ms: buffer: [", systime()) ;
    for (j = 0; j < blocksize; j++)
      printf ("%c", buffer[j]) ;
    printf ("]\n") ;
  }

  // inicia gerador de números aleatórios
  srandom (systime()) ;

  // lê e imprime todos os blocos do disco, um a um
  for (i=0; i<numblocks; i++)
  {

    // preenche o buffer com um caractere aleatório
    c = 32 + random () % 48 ;
    for (j = 0; j < blocksize; j++)
      buffer[j] = c ;

    // escreve o buffer no bloco i do disco
    printf ("%5d ms: escrevendo bloco %d com caracteres \"%c\"\n",
            systime(), i, c) ;
    if (disk_block_write (i, buffer))
      printf ("Erro!\n") ;
  }

  // encerra a thread main
  printf ("%5d ms: main fim\n", systime()) ;
  task_exit (0) ;

  exit(0) ;
}

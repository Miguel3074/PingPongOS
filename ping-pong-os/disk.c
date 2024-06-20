// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.2 -- Junho de 2019

// Este código simula a operação e interface de um disco rígido de computador.
// O "hardware" do disco simulado oferece as operações descritas no arquivo
// disk.h.
//
// As operações de leitura e escrita de blocos de dados são atendidas
// de forma assíncrona, ou seja: o disco responde à requisição imediatamente,
// mas a operação de E/S em si demora um pouco mais; quando ela for completada,
// o disco irá informar isso através de um sinal SIGUSR1 (simulando a interrução
// de hardware que ocorre em um disco real).
//
// O conteúdo do disco simulado é armazenado em um arquivo no sistema operacional
// subjacente, sendo portanto preservado de uma execução para outra.
//
// Atencao: deve ser usado o flag de ligacao -lrt, para ligar com a 
// biblioteca POSIX de tempo real, pois o disco simulado usa timers POSIX.

// operating system check
#if defined(_WIN32) || (!defined(__unix__) && !defined(__unix) && (!defined(__APPLE__) || !defined(__MACH__)))
#warning Este codigo foi planejado para ambientes UNIX (LInux, *BSD, MacOS). A compilacao e execucao em outros ambientes e responsabilidade do usuario.
#endif

#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include "disk.h"

// parâmetros de operação do disco simulado
#define DISK_NAME       "disk.dat"	// arquivo com o conteúdo do disco
#define DISK_BLOCK_SIZE  64		// tamanho de cada bloco, em bytes
#define DISK_DELAY_MIN   30		// atraso minimo, em milisegundos
#define DISK_DELAY_MAX  300		// atraso maximo, em milisegundos

//#define DEBUG_DISK 1			// para depurar a operação do disco

/**********************************************************************/

// estrutura com os dados internos do disco (estado inicial desconhecido)
typedef struct {
  int status ;			// estado do disco
  char *filename ;		// nome do arquivo que simula o disco
  int fd ;			// descritor do arquivo que simula o disco
  int numblocks ;		// numero de blocos do disco
  int blocksize ;		// tamanho dos blocos em bytes
  char *buffer ;		// buffer da proxima operacao (read/write)
  int prev_block ;		// bloco da ultima operacao
  int next_block ;		// bloco da proxima operacao
  int delay_min, delay_max ;	// tempos de acesso mínimo e máximo
  timer_t           timer ;	// timer que simula o tempo de acesso
  struct itimerspec delay ;	// struct do timer de tempo de acesso
  struct sigevent   sigev ;	// evento associado ao timer
  struct sigaction  signal ;	// tratador de sinal do timer
} disk_t ;

// should be static to avoid clash with "disk" variables in other files
static disk_t disk ;			// hard disk structure

/**********************************************************************/

// arma o timer que simula o tempo de acesso ao disco;
// ao disparar, ele gera um sinal SIGIO
static void disk_settimer ()
{
  int time_ms ;

  // tempo no intervalo [DISK_DELAY_MIN ... DISK_DELAY_MAX], proporcional a
  // distancia entre o proximo bloco a ler (next_block) e a ultima leitura
  // (prev_block), somado a um pequeno fator aleatorio
  time_ms = abs (disk.next_block - disk.prev_block)
          * (disk.delay_max - disk.delay_min) / disk.numblocks
          + disk.delay_min
          + random () % (disk.delay_max - disk.delay_min) / 10 ;

  #ifdef DEBUG_DISK
  printf ("DISK: [%d->%d, %d]\n", disk.prev_block, disk.next_block, time_ms) ;
  #endif

  // primeiro disparo, em nano-segundos,
  disk.delay.it_value.tv_nsec = time_ms * 1000000 ;

  // primeiro disparo, em segundos
  disk.delay.it_value.tv_sec  = time_ms / 1000 ;

  // proximos disparos nao ocorrem (disparo unico)
  disk.delay.it_interval.tv_nsec = 0 ;
  disk.delay.it_interval.tv_sec  = 0 ;

  // arma o timer
  if (timer_settime(disk.timer, 0, &disk.delay, NULL) == -1)
  {
     perror("DISK:");
     exit(1);
  }
  #ifdef DEBUG_DISK
  printf ("DISK: timer is set\n") ;
  #endif
}

/**********************************************************************/

// trata o sinal SIGIO do timer que simula o tempo de acesso ao disco
static void disk_sighandle (int sig)
{
  #ifdef DEBUG_DISK
  printf ("DISK: signal %d received\n", sig) ;
  #endif

  // verificar qual a operacao pendente e realiza-la
  switch (disk.status)
  {
    case DISK_STATUS_READ:
      // faz a leitura previamente agendada
      lseek (disk.fd, disk.next_block * disk.blocksize, SEEK_SET) ;
      read  (disk.fd, disk.buffer, disk.blocksize) ;
      break ;

    case DISK_STATUS_WRITE:
      // faz a escrita previamente agendada
      lseek (disk.fd, disk.next_block * disk.blocksize, SEEK_SET) ;
      write (disk.fd, disk.buffer, disk.blocksize) ;
      break ;

    default:
      // erro: estado desconhecido
      perror("DISK: unknown disk state");
      exit(1);
  }

  // guarda numero de bloco da ultima operacao
  disk.prev_block = disk.next_block ;

  // disco se torna ocioso novamente
  disk.status = DISK_STATUS_IDLE ;

  // gerar um sinal SIGUSR1 para o "kernel" do usuario
  raise (SIGUSR1) ;
}

/**********************************************************************/

// inicializa o disco virtual
// retorno: 0 (sucesso) ou -1 (erro)
static int disk_init ()
{
  // o disco jah foi inicializado ?
  if ( disk.status != DISK_STATUS_UNKNOWN )
    return -1 ;

  // estado atual do disco
  disk.status = DISK_STATUS_IDLE ;
  disk.next_block = disk.prev_block = 0 ;

  // abre o arquivo no disco (leitura/escrita, sincrono)
  disk.filename = DISK_NAME ;
  disk.fd = open (disk.filename, O_RDWR|O_SYNC) ;
  if (disk.fd < 0)
  {
    perror("DISK: " DISK_NAME);
    exit (1) ;
  }

  // define seu tamanho em blocos
  disk.blocksize = DISK_BLOCK_SIZE ;
  disk.numblocks = lseek (disk.fd, 0, SEEK_END) / disk.blocksize ;

  // ajusta atrasos mínimo e máximo de acesso no disco
  disk.delay_min = DISK_DELAY_MIN ;
  disk.delay_max = DISK_DELAY_MAX ;

  // associa SIGIO do timer ao handle apropriado
  disk.signal.sa_handler = disk_sighandle ;
  sigemptyset (&disk.signal.sa_mask);
  disk.signal.sa_flags = 0;
  sigaction (SIGIO, &disk.signal, 0);

  // cria o timer que simula o tempo de acesso ao disco
  disk.sigev.sigev_notify = SIGEV_SIGNAL;
  disk.sigev.sigev_signo = SIGIO;
  if (timer_create(CLOCK_REALTIME, &disk.sigev, &disk.timer) == -1)
  {
    perror("DISK:");
    exit (1) ;
  }

  #ifdef DEBUG_DISK
  printf ("DISK: initialized\n") ;
  #endif

  return 0 ;
}

/**********************************************************************/

// funcao que implementa a interface de acesso ao disco em baixo nivel
int disk_cmd (int cmd, int block, void *buffer)
{
  #ifdef DEBUG_DISK
  printf ("DISK: received command %d\n", cmd) ;
  #endif

  switch (cmd)
  {
    // inicializa o disco
    case DISK_CMD_INIT:
      return (disk_init ()) ;

    // solicita status do disco
    case DISK_CMD_STATUS:
      return (disk.status) ;

    // solicita tamanho do disco
    case DISK_CMD_DISKSIZE:
      if (disk.status == DISK_STATUS_UNKNOWN)
        return -1 ;
      return (disk.numblocks) ;

    // solicita tamanho de bloco
    case DISK_CMD_BLOCKSIZE:
      if (disk.status == DISK_STATUS_UNKNOWN)
        return -1 ;
      return (disk.blocksize) ;

    // solicita atraso mínimo
    case DISK_CMD_DELAYMIN:
      if (disk.status == DISK_STATUS_UNKNOWN)
        return -1 ;
      return (disk.delay_min) ;

    // solicita atraso máximo
    case DISK_CMD_DELAYMAX:
      if (disk.status == DISK_STATUS_UNKNOWN)
        return -1 ;
      return (disk.delay_max) ;

    // solicita operação de leitura ou de escrita
    case DISK_CMD_READ:
    case DISK_CMD_WRITE:
      if (disk.status != DISK_STATUS_IDLE)
        return -1 ;
      if ( !buffer )
        return -1 ;
      if ( block < 0 || block >= disk.numblocks)
        return -1 ;

      // registra que ha uma operacao pendente
      disk.buffer = buffer ;
      disk.next_block = block ;
      if (cmd == DISK_CMD_READ)
        disk.status = DISK_STATUS_READ ;
      else
        disk.status = DISK_STATUS_WRITE ;

      // arma o timer que simula o atraso do disco
      disk_settimer () ;

      return 0 ;

    default:
      return -1 ;
  }
}

/**********************************************************************/

// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.2 -- Julho de 2017

#include <stdio.h>
#include <signal.h>
#include "ppos_data.h"
#include "ppos.h"
#include "disk.h"
#include "queue.h"

// interface do gerente de disco rígido (block device driver)

#ifndef __DISK_MGR__
#define __DISK_MGR__

// estruturas de dados e rotinas de inicializacao e acesso
// a um dispositivo de entrada/saida orientado a blocos,
// tipicamente um disco rigido.

// estrutura que representa um disco no sistema operacional

// Definição da estrutura disk_t
typedef struct disk_t
{
  struct disk_queue *task_queue;
  task_t *disk_wait_queue; // Fila de tarefas esperando pelo disco
} disk_t;

typedef struct disk_queue
{
  struct disk_queue *prev;
  struct disk_queue *next;
  task_t *task; // Tarefa associada à operação de disco
  int type;     // Tipo da operação (leitura ou escrita)
  int block;    // Número do bloco a ser lido ou escrito
  void *buffer; // Buffer de dados para leitura ou escrita
} disk_queue;

// inicializacao do gerente de disco
// retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init(int *numBlocks, int *blockSize);

// leitura de um bloco, do disco para o buffer
int disk_block_read(int block, void *buffer);

// escrita de um bloco, do buffer para o disco
int disk_block_write(int block, void *buffer);

void disk_signal_handler(int signal);

#endif

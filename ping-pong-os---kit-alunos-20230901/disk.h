// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.2 -- Junho de 2019

// Código que simula um disco rígido - especificação da interface do disco

// Atencao: deve ser usado o flag de ligacao -lrt, para ligar com a 
// biblioteca POSIX de tempo real, pois o disco simulado usa timers POSIX.

#ifndef __DISK__
#define __DISK__

// operações oferecidas pelo disco
#define DISK_CMD_INIT		0	// inicializacao do disco
#define DISK_CMD_READ		1	// leitura de bloco do disco
#define DISK_CMD_WRITE		2	// escrita de bloco do disco
#define DISK_CMD_STATUS		3	// consulta status do disco
#define DISK_CMD_DISKSIZE	4	// consulta tamanho do disco em blocos
#define DISK_CMD_BLOCKSIZE	5	// consulta tamanho de bloco em bytes
#define DISK_CMD_DELAYMIN	6	// consulta tempo resposta mínimo (ms)
#define DISK_CMD_DELAYMAX	7	// consulta tempo resposta máximo (ms)

// estados internos do disco
#define DISK_STATUS_UNKNOWN	0	// disco não inicializado
#define DISK_STATUS_IDLE	1	// disco livre
#define DISK_STATUS_READ	2	// disco ocupado fazendo leitura
#define DISK_STATUS_WRITE	3	// disco ocupado fazendo escrita

// No caso de operacoes assincronas, a chamada apenas "agenda" os pedidos de
// operacao e retorna imediatamente. Quando a operacao solicitada for concluida,
// o disco ira gerar um sinal SIGUSR1, que deve ser recebido e tratado pelo
// gerenciador de discos, para acordar a tarefa que solicitou a operação.

int disk_cmd (int cmd, int block, void *buffer) ;

// Exemplos de uso:

// inicializa um disco (operacao sincrona)
// int disk_cmd (DISK_CMD_INIT, 0, 0) ;
// result < 0: erro
// result = 0: disco corretamente inicializado

// consulta status do disco (operacao sincrona)
// int disk_cmd (DISK_CMD_STATUS, 0, 0) ;
// result = 0: erro (disco não inicializado ou inexistente)
// result = 1: disco livre (disponível para comandos de leitura/escrita)
// result = 2: disco ocupado realizando leitura (disco nao disponivel)
// result = 3: disco ocupado realizando escrita (disco nao disponivel)

// consulta tamanho do disco (operacao sincrona)
// int disk_cmd (DISK_CMD_DISKSIZE, 0, 0) ;
// result <  0: erro
// result >= 0: tamanho do disco (em blocos)

// consulta tamanho de cada bloco (operacao sincrona)
// int disk_cmd (DISK_CMD_BLOCKSIZE, 0, 0) ;
// result <  0: erro
// result >= 0: tamanho de cada bloco (em bytes)

// consulta tempo de resposta mínimo (operacao sincrona)
// int disk_cmd (DISK_CMD_DELAYMIN, 0, 0) ;
// result <  0: erro
// result >= 0: tempo de resposta mínimo do disco (em ms)

// consulta tempo de resposta máximo (operacao sincrona)
// int disk_cmd (DISK_CMD_DELAYMAX, 0, 0) ;
// result <  0: erro
// result >= 0: tempo de resposta máximo do disco (em ms)

// agenda a leitura de um bloco de disco (operacao assincrona)
// int disk_cmd (DISK_CMD_READ, int block, void *buffer) ;
// result < 0: erro
// result = 0: ok (leitura agendada, sinal SIGUSR1 serah gerado ao completar)

// agenda a escrita de um bloco de disco (operacao assincrona)
// int disk_cmd (DISK_CMD_WRITE, int block, void *buffer) ;
// result < 0: erro
// result = 0: ok (escrita agendada, sinal SIGUSR1 serah gerado ao completar)

#endif

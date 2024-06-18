#include "ppos_disk.h" 

// Fila de operações de disco
disk_operation_t *disk_queue = NULL;

// Semáforos para gerenciar acesso ao disco
semaphore_t disk_access;
semaphore_t disk_manager;

int main() {
    int num_blocks;   // Variável para armazenar o número de blocos do disco
    int block_size;   // Variável para armazenar o tamanho de cada bloco do disco

    // Chama a função para inicializar o gerente de disco
    int result = disk_mgr_init(&num_blocks, &block_size);

    // Verifica se a inicialização foi bem-sucedida
    if (result == 0) {
        printf("Disco inicializado com sucesso!\n");
        printf("Número de blocos: %d\n", num_blocks);
        printf("Tamanho de cada bloco: %d bytes\n", block_size);
    } else {
        printf("Erro ao inicializar o disco.\n");
    }

    return 0;
}



// Função para inicializar o gerente de disco
int disk_mgr_init(int *numBlocks, int *blockSize) {
    // Inicializa o disco
    int result = disk_cmd(DISK_CMD_INIT, 0, 0);
    if (result < 0) {
        return -1; // Erro na inicialização do disco
    }

    // Consulta o tamanho do disco em blocos
    result = disk_cmd(DISK_CMD_DISKSIZE, 0, 0);
    if (result < 0) {
        return -1; // Erro ao consultar o tamanho do disco
    }
    *numBlocks = result;

    // Consulta o tamanho de cada bloco do disco em bytes
    result = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);
    if (result < 0) {
        return -1; // Erro ao consultar o tamanho do bloco
    }
    *blockSize = result;

    return 0; // Sucesso
}


int disk_block_read(int block, void *buffer)
{
    // obtém o semáforo de acesso ao disco
 
   // inclui o pedido na fila_disco
 
   //if (gerente de disco está dormindo)
   //{
      // acorda o gerente de disco (põe ele na fila de prontas)
   //}
 
   // libera semáforo de acesso ao disco
 
   // suspende a tarefa corrente (retorna ao dispatcher)
    return 0;
}

int disk_block_write(int block, void *buffer)
{
    return 0;
}

void diskDriverBody (void * args)
{
   //while (true) 
   //{
      // obtém o semáforo de acesso ao disco
 
      // se foi acordado devido a um sinal do disco
      //if (disco gerou um sinal)
      //{
         // acorda a tarefa cujo pedido foi atendido
      //}
 
      // se o disco estiver livre e houver pedidos de E/S na fila
      //if (disco_livre && (fila_disco != NULL))
      //{
         // escolhe na fila o pedido a ser atendido, usando FCFS
         // solicita ao disco a operação de E/S, usando disk_cmd()
      //}
 
      // libera o semáforo de acesso ao disco
 
      // suspende a tarefa corrente (retorna ao dispatcher)
   //}
}
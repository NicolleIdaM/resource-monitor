#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "../include/monitor.h"

void gerar_carga_io()
{
    const char *filename = "teste_io_temporario.txt";
    const int tamanho_bloco = 4096;
    const int qtde_bloco = 500;

    int arquivo = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if(arquivo == -1){
        perror("Erro ao criar arquivo de teste IO");
        return;
    }

    char *buffer = malloc(tamanho_bloco);
    if(buffer == 0){
        perror("Erro ao alocar buffer");
        close(arquivo);
        return;
    }

    for(int i = 0; i < tamanho_bloco; i++){
        buffer[i] = 'A' + (i % 26);
    }

    for(int i = 0; i < qtde_bloco; i++){
        write(arquivo, buffer, tamanho_bloco);
    }

    free(buffer);
    close(arquivo);

    arquivo = open(filename, O_RDONLY);
    buffer = malloc(tamanho_bloco);

    if(arquivo != -1 && buffer != 0){
        for(int i = 0; i < qtde_bloco; i++){
            read(arquivo, buffer, tamanho_bloco);
        }
        free(buffer);
        close(arquivo);
    }
    unlink(filename);
}

int main()
{
    printf("TESTE DE I/O\n");

    metricas_io_t io;
    pid_t pid = getpid();
    int testes_funcionando = 0;

    /*Teste 1 - Leitura Básica*/
    printf("\n TESTE DE LEITURA BÁSICA");
    if(get_metricas_io(pid, &io) == 0){
        printf("Teste de leitura funcionando");
        testes_funcionando++;
    }else{
        printf("Teste de leitura falhou");
    }
}
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
    printf("\nTESTE DE LEITURA BÁSICA");
    if(get_metricas_io(pid, &io) == 0){
        printf("\n  Teste de leitura funcionando\n");
        testes_funcionando++;
    }else{
        printf("\n  Teste de leitura falhou\n");
    }

    /*Teste 2 - Detecção de I/O*/
    printf("\nTESTE DE DETECÇÃO DE I/O");
    metricas_io_t antes, depois;

    get_metricas_io(pid, &antes);
    gerar_carga_io();
    get_metricas_io(pid, &depois);

    if(depois.bytes_escritos > antes.bytes_escritos){
        printf("\n  Teste de detecção funcionando (Bytes Escritos: %lu)\n", depois.bytes_escritos - antes.bytes_escritos);
        testes_funcionando++;
    }else{
        printf("\n  Teste de detecção falhou\n");
    }

    /*Teste 3 - Multiplas Leituras*/
    printf("\nTESTE DE MULTIPLAS LEITURAS");
    int leituras_realizadas = 0;
    for(int i = 0; i < 3; i++){
        if (get_metricas_io(pid, &io) == 0){
            leituras_realizadas++;
        }
        sleep(1);
    }

    if(leituras_realizadas == 3){
        printf("\n  Leituras realizadas: %d/3\n", leituras_realizadas);
        testes_funcionando++;
    }else{
        printf("\n  Teste de multiplas leituras falhou\n");
    }

    /*Resultado Final*/
    printf("\nRESULTADO: %d/3 passaram\n", testes_funcionando);
    if(testes_funcionando == 3){
        printf("\nTodos os testes funcionaram\n");
    }else{              
        printf("\nAlgo não funcionou! Verefique o programa!\n");
    }
}
#include "../include/monitor.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

int get_metricas_io(pid_t pid, metricas_io_t *metricas)
{
    if (metricas == NULL){
        errno = EINVAL;
        return -1;
    }
    
    metricas -> bytes_lidos = 0;
    metricas -> bytes_escritos = 0;
    metricas -> chamadas_lidas = 0;
    metricas -> chamadas_escritas = 0;
    metricas -> operacoes_lidas_disco = 0;
    metricas -> operacoes_escritas_disco = 0;
    metricas -> bytes_lidos_disco = 0;
    metricas -> bytes_escritos_disco = 0;


    char caminho[256];
    snprintf(caminho, sizeof(caminho), "/proc/%d/io", pid);

    FILE *arquivo = fopen(caminho, "r");
    if(arquivo == NULL){
        perror("Erro ao abrir arquivo io");
        return -1;
    }

    char linha[256];
    int campos_lidos = 0;
    
    while(fgets(linha, sizeof(linha), arquivo)){
        if (strstr(linha, "rchar:")){
            if (sscanf(linha, "rchar: %lu", &metricas->bytes_lidos) == 1) campos_lidos++;
        } else if (strstr(linha, "wchar:")){
            if (sscanf(linha, "wchar: %lu", &metricas->bytes_escritos) == 1) campos_lidos++;
        } else if (strstr(linha, "syscr:")){
            if (sscanf(linha, "syscr: %lu", &metricas->chamadas_lidas) == 1) campos_lidos++;
        } else if (strstr(linha, "syscw:")){
            if (sscanf(linha, "syscw: %lu", &metricas->chamadas_escritas) == 1) campos_lidos++;
        } else if (strstr(linha, "read_bytes")){
            if (sscanf(linha, "read_bytes: %lu", &metricas->bytes_lidos_disco) == 1) campos_lidos++;
        } else if (strstr(linha, "write_bytes")){
            if (sscanf(linha, "write_bytes: %lu", &metricas->bytes_escritos_disco) == 1) campos_lidos++;
        }
    }

    fclose(arquivo);

    const unsigned long tamanho_bloco = 4096;
    if(metricas -> bytes_lidos_disco > 0){
        metricas -> operacoes_lidas_disco = metricas -> bytes_lidos_disco / tamanho_bloco;
        if(metricas -> bytes_lidos_disco % tamanho_bloco != 0){
            metricas -> operacoes_lidas_disco++;
        }
    }

    if(metricas -> bytes_escritos_disco > 0){
        metricas -> operacoes_escritas_disco = metricas -> bytes_escritos_disco / tamanho_bloco;
        if(metricas -> bytes_escritos_disco % tamanho_bloco != 0){
            metricas -> operacoes_escritas_disco++;
        }
    }
    
    if (campos_lidos < 2) {
        fprintf(stderr, "Erro: dados de I/O insuficientes (lidos %d/4 campos principais)\n", campos_lidos);
        errno = EIO;
        return -1;
    }

    return 0;
}

int get_operacoes_disco(pid_t pid, unsigned long* ler_operacoes, unsigned long* escrever_operacoes, unsigned long* ler_bytes, unsigned long* escrever_bytes){
    if(ler_operacoes == NULL){
        errno = EINVAL;
        return -1;
    }

    if(escrever_operacoes == NULL){
        errno = EINVAL;
        return -1;
    }

    if(ler_bytes == NULL){
        errno = EINVAL;
        return -1;
    }

    if(escrever_bytes == NULL){
        errno = EINVAL;
        return -1;
    }

    metricas_io_t metricas;
    if(get_metricas_io(pid, &metricas) == 0){
        *ler_operacoes = metricas.operacoes_lidas_disco;
        *escrever_operacoes = metricas.operacoes_escritas_disco;
        *ler_bytes = metricas.bytes_lidos_disco;
        *escrever_bytes = metricas.bytes_escritos_disco;
        return 0;
    }

    return -1;
}
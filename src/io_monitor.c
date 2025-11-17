#include "../include/monitor.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

/*
 * Obtém métricas de I/O para um processo específico.
 * Lê dados do arquivo /proc/[pid]/io do kernel.
 */
int get_metricas_io(pid_t pid, metricas_io_t *metricas)
{
    if (metricas == NULL){
        errno = EINVAL;
        return -1;
    }
    
    /* Inicializa métricas com zeros para caso de erro */
    metricas -> bytes_lidos = 0;
    metricas -> bytes_escritos = 0;
    metricas -> chamadas_lidas = 0;
    metricas -> chamadas_escritas = 0;
    metricas -> operacoes_lidas_disco = 0;
    metricas -> operacoes_escritas_disco = 0;
    metricas -> bytes_lidos_disco = 0;
    metricas -> bytes_escritos_disco = 0;

    /* Abre arquivo de I/O do processo no procfs */
    char caminho[256];
    snprintf(caminho, sizeof(caminho), "/proc/%d/io", pid);

    FILE *arquivo = fopen(caminho, "r");
    if(arquivo == NULL){
        perror("Erro ao abrir arquivo io");
        return -1;
    }

    char linha[256];
    int campos_lidos = 0;
    
    /* Parse do arquivo linha por linha procurando pelos campos específicos */
    while(fgets(linha, sizeof(linha), arquivo)){
        /* rchar: bytes lidos (inclui cache) */
        if (strstr(linha, "rchar:")){
            if (sscanf(linha, "rchar: %lu", &metricas->bytes_lidos) == 1) campos_lidos++;
        } 
        /* wchar: bytes escritos (inclui cache) */
        else if (strstr(linha, "wchar:")){
            if (sscanf(linha, "wchar: %lu", &metricas->bytes_escritos) == 1) campos_lidos++;
        } 
        /* syscr: chamadas de sistema de leitura */
        else if (strstr(linha, "syscr:")){
            if (sscanf(linha, "syscr: %lu", &metricas->chamadas_lidas) == 1) campos_lidos++;
        } 
        /* syscw: chamadas de sistema de escrita */
        else if (strstr(linha, "syscw:")){
            if (sscanf(linha, "syscw: %lu", &metricas->chamadas_escritas) == 1) campos_lidos++;
        } 
        /* read_bytes: bytes realmente lidos do disco (exclui cache) */
        else if (strstr(linha, "read_bytes")){
            if (sscanf(linha, "read_bytes: %lu", &metricas->bytes_lidos_disco) == 1) campos_lidos++;
        } 
        /* write_bytes: bytes realmente escritos no disco */
        else if (strstr(linha, "write_bytes")){
            if (sscanf(linha, "write_bytes: %lu", &metricas->bytes_escritos_disco) == 1) campos_lidos++;
        }
    }

    fclose(arquivo);

    /* Calcula operações de disco baseado em bytes lidos/escritos */
    const unsigned long tamanho_bloco = 4096; /* Tamanho típico de bloco em sistemas Linux */
    if(metricas -> bytes_lidos_disco > 0){
        metricas -> operacoes_lidas_disco = metricas -> bytes_lidos_disco / tamanho_bloco;
        /* Arredonda para cima se houver resto */
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
    
    /* Verifica se leu dados suficientes para considerar válido */
    if (campos_lidos < 2) {
        fprintf(stderr, "Erro: dados de I/O insuficientes (lidos %d/4 campos principais)\n", campos_lidos);
        errno = EIO;
        return -1;
    }

    return 0;
}

/*
 * Função auxiliar para obter apenas operações e bytes de disco.
 * Encapsula get_metricas_io para uma interface mais simples.
 */
int get_operacoes_disco(pid_t pid, unsigned long* ler_operacoes, unsigned long* escrever_operacoes, 
                       unsigned long* ler_bytes, unsigned long* escrever_bytes){
    /* Validação de parâmetros */
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

    /* Obtém métricas completas e extrai os campos desejados */
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
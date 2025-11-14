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
    
    metricas->bytes_lidos = 0;
    metricas->bytes_escritos = 0;
    metricas->chamadas_lidas = 0;
    metricas->chamadas_escritas = 0;

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
        }
    }

    fclose(arquivo);
    
    if (campos_lidos < 2) {
        fprintf(stderr, "Erro: dados de I/O insuficientes (lidos %d/4 campos principais)\n", campos_lidos);
        errno = EIO;
        return -1;
    }

    return 0;
}
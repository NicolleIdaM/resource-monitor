#include "../include/monitor.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

int get_metricas_memoria(pid_t pid, metricas_memoria_t *metricas){
    if (metricas == NULL){
        errno = EINVAL;
        return -1;
    }
    
    char caminho[256];
    snprintf(caminho, sizeof(caminho), "/proc/%d/statm", pid);

    FILE *arquivo = fopen(caminho, "r");
    if(arquivo == NULL){
        perror("Erro ao abrir arquivo statm");
        return -1;
    }

    unsigned long size, resident, share, text, lib, data, dt;
    int leitura = fscanf(arquivo, "%lu %lu %lu %lu %lu %lu %lu", 
                        &size, &resident, &share, &text, &lib, &data, &dt);
    fclose(arquivo);

    if(leitura != 7){
        fprintf(stderr, "Erro ao ler estatísticas do processo (lidos %d/7 campos)\n", leitura);
        errno = EIO;
        return -1;
    }

    long tamanho_pagina = sysconf(_SC_PAGESIZE);
    if (tamanho_pagina == -1) {
        perror("Erro ao obter tamanho da página");
        return -1;
    }

    metricas->RAM = resident * tamanho_pagina;
    metricas->MV = size * tamanho_pagina;
    metricas->MTD = text * tamanho_pagina;

    return 0;
}
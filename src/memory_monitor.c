#include "../include/monitor.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

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

    snprintf(caminho, sizeof(caminho), "/proc/%d/stat", pid);
    arquivo = fopen(caminho, "r");
    if(arquivo == NULL){
        perror("Erro ao abrir arquivo stat para page faults");
        metricas->page_faults_minor = 0;
        metricas->page_faults_major = 0;
        metricas->swap = 0;
        return 0;
    }

    char linha[1024];
    if(fgets(linha, sizeof(linha), arquivo)){
        char *token = strtok(linha, " ");
        int campo = 1;
        while(token != NULL){
            if(campo == 10){
                metricas->page_faults_minor = strtoul(token, NULL, 10);
            } else if(campo == 12){
                metricas->page_faults_major = strtoul(token, NULL, 10);
            }
            token = strtok(NULL, " ");
            campo++;
        }
    }
    fclose(arquivo);

    snprintf(caminho, sizeof(caminho), "/proc/%d/status", pid);
    arquivo = fopen(caminho, "r");
    if(arquivo != NULL){
        char linha[256];
        while(fgets(linha, sizeof(linha), arquivo)){
            if(strstr(linha, "VmSwap:")){
                sscanf(linha, "VmSwap: %lu kB", &metricas->swap);
                metricas->swap *= 1024;
                break;
            }
        }
        fclose(arquivo);
    } else {
        metricas->swap = 0;
    }

    return 0;
}
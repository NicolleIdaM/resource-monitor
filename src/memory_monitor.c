#include "../include/monitor.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

/*
 * Obtém métricas de memória para um processo específico.
 * Combina dados de /proc/[pid]/statm, /proc/[pid]/stat e /proc/[pid]/status.
 */
int get_metricas_memoria(pid_t pid, metricas_memoria_t *metricas){
    if (metricas == NULL){
        errno = EINVAL;
        return -1;
    }
    
    /* Inicializa swap com 0 (caso não seja encontrado) */
    metricas->swap = 0;

    /* Lê informações básicas de memória do arquivo statm */
    char caminho[256];
    snprintf(caminho, sizeof(caminho), "/proc/%d/statm", pid);

    FILE *arquivo = fopen(caminho, "r");
    if(arquivo == NULL){
        perror("Erro ao abrir arquivo statm");
        return -1;
    }

    /* Campos do statm: size, resident, share, text, lib, data, dt */
    unsigned long size, resident, share, text, lib, data, dt;
    int leitura = fscanf(arquivo, "%lu %lu %lu %lu %lu %lu %lu", 
    &size, &resident, &share, &text, &lib, &data, &dt);
    fclose(arquivo);

    if(leitura != 7){
        fprintf(stderr, "Erro ao ler estatísticas do processo (lidos %d/7 campos)\n", leitura);
        errno = EIO;
        return -1;
    }

    /* Obtém tamanho da página do sistema para converter páginas em bytes */
    long tamanho_pagina = sysconf(_SC_PAGESIZE);
    if (tamanho_pagina == -1) {
        perror("Erro ao obter tamanho da página");
        return -1;
    }

    /* Converte páginas para bytes e preenche a estrutura */
    metricas -> RAM = resident * tamanho_pagina;      /* Memória residente (RAM usada) */
    metricas -> MV = size * tamanho_pagina;           /* Tamanho total da memória virtual */
    metricas -> MTD = text * tamanho_pagina;          /* Memória de texto (código) */

    /* Lê page faults do arquivo stat do processo */
    snprintf(caminho, sizeof(caminho), "/proc/%d/stat", pid);
    arquivo = fopen(caminho, "r");
    if(arquivo == NULL){
        perror("Erro ao abrir arquivo stat para page faults");
        /* Se não conseguir abrir, inicializa com zeros */
        metricas -> falha_pag_menor = 0;
        metricas -> falha_pag_maior = 0;
    } else {
        char linha[1024];
        if(fgets(linha, sizeof(linha), arquivo)){
            char *token = strtok(linha, " ");
            int campo = 1;
            /* Parse manual do arquivo stat para encontrar campos específicos */
            while(token != NULL){
                /* Campo 10: page faults menores (sem I/O) */
                if(campo == 10){
                    metricas -> falha_pag_menor = strtoul(token, NULL, 10);
                } 
                /* Campo 12: page faults maiores (com I/O) */
                else if(campo == 12){
                    metricas -> falha_pag_maior = strtoul(token, NULL, 10);
                }
                token = strtok(NULL, " ");
                campo++;
            }
        }
        fclose(arquivo);
    }

    /* Lê uso de swap do arquivo status */
    snprintf(caminho, sizeof(caminho), "/proc/%d/status", pid);
    arquivo = fopen(caminho, "r");
    if(arquivo != NULL){
        char linha[256];
        while(fgets(linha, sizeof(linha), arquivo)){
            /* Procura pela linha VmSwap no arquivo status */
            if(strstr(linha, "VmSwap:")){
                unsigned long swap_kb;
                if (sscanf(linha, "VmSwap: %lu kB", &swap_kb) == 1) {
                    /* Converte KB para bytes */
                    metricas->swap = swap_kb * 1024;
                }
                break;
            }
        }
        fclose(arquivo);
    }

    return 0;
}
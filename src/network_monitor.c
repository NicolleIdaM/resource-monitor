#include "../include/monitor.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>

int get_metricas_rede(pid_t pid, metricas_rede_t *metricas) {
    if (metricas == NULL) {
        errno = EINVAL;
        return -1;
    }

    metricas->bytes_recebidos = 0;
    metricas->bytes_enviados = 0;
    metricas->pacotes_recebidos = 0;
    metricas->pacotes_enviados = 0;
    metricas->conexoes_ativas = 0;

    char caminho[256];
    snprintf(caminho, sizeof(caminho), "/proc/%d/net/dev", pid);
    
    FILE *arquivo = fopen(caminho, "r");
    if (arquivo != NULL) {
        char linha[512];
        int linha_num = 0;
        
        while (fgets(linha, sizeof(linha), arquivo)) {
            linha_num++;
            if (linha_num <= 2) continue;
            
            char interface[32];
            unsigned long long bytes_recv, packets_recv, bytes_sent, packets_sent;
            
            if (sscanf(linha, "%31[^:]: %llu %llu %*u %*u %*u %*u %*u %*u %llu %llu",
                      interface, &bytes_recv, &packets_recv, &bytes_sent, &packets_sent) >= 4) {
                
                if (strcmp(interface, "lo") != 0) {
                    metricas->bytes_recebidos += bytes_recv;
                    metricas->pacotes_recebidos += packets_recv;
                    metricas->bytes_enviados += bytes_sent;
                    metricas->pacotes_enviados += packets_sent;
                }
            }
        }
        fclose(arquivo);
    }

    return 0;
}
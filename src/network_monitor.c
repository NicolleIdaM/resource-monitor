#include "../include/monitor.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>

/*
 * Obtém métricas de rede para um processo específico.
 * Combina dados de /proc/[pid]/net/dev e /proc/[pid]/net/tcp.
 */
int get_metricas_rede(pid_t pid, metricas_rede_t *metricas) {
    if (metricas == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Inicializa métricas com zeros */
    metricas->bytes_recebidos = 0;
    metricas->bytes_enviados = 0;
    metricas->pacotes_recebidos = 0;
    metricas->pacotes_enviados = 0;
    metricas->conexoes_ativas = 0;

    /* Tenta ler estatísticas de rede específicas do processo */
    char caminho[256];
    snprintf(caminho, sizeof(caminho), "/proc/%d/net/dev", pid);
    
    FILE *arquivo = fopen(caminho, "r");
    if (arquivo != NULL) {
        char linha[512];
        int linha_num = 0;
        
        /* Parse do arquivo /proc/[pid]/net/dev */
        while (fgets(linha, sizeof(linha), arquivo)) {
            linha_num++;
            if (linha_num <= 2) continue; /* Pula cabeçalhos */
            
            char interface[32];
            unsigned long long bytes_recv, packets_recv, bytes_sent, packets_sent;
            
            /* Formato: interface: bytes_recv packets_recv ... bytes_sent packets_sent ... */
            if (sscanf(linha, "%31[^:]: %llu %llu %*u %*u %*u %*u %*u %*u %llu %llu",
                      interface, &bytes_recv, &packets_recv, &bytes_sent, &packets_sent) >= 4) {
                
                /* Ignora interface loopback para estatísticas de rede real */
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

    /* Conta conexões TCP ativas do processo */
    snprintf(caminho, sizeof(caminho), "/proc/%d/net/tcp", pid);
    arquivo = fopen(caminho, "r");
    if (arquivo != NULL) {
        char linha[512];
        int conexoes = 0;
        
        /* Cada linha (exceto cabeçalho) representa uma conexão TCP */
        while (fgets(linha, sizeof(linha), arquivo)) {
            if (strlen(linha) > 10 && linha[0] != ' ' && linha[0] != 's') {
                conexoes++;
            }
        }
        fclose(arquivo);
        
        metricas->conexoes_ativas = conexoes;
    }

    /* Fallback: se não encontrou dados específicos do processo, usa dados globais */
    if (metricas->bytes_recebidos == 0 && metricas->bytes_enviados == 0) {
        arquivo = fopen("/proc/net/dev", "r");
        if (arquivo != NULL) {
            char linha[512];
            int linha_num = 0;
            
            /* Lê estatísticas de rede globais do sistema */
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
    }

    return 0;
}
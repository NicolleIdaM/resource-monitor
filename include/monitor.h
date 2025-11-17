#ifndef MONITOR_H
#define MONITOR_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h> 

/*
 * Estrutura para métricas de CPU:
 * - Tempo em modo usuário e sistema
 * - Percentual de uso de CPU
 * - Context switches e número de threads
 */
typedef struct{
    unsigned long tempo_usuario;
    unsigned long tempo_sistema;
    double porcentagem_cpu;
    unsigned long context_switches;
    unsigned long threads;
}metricas_cpu_t;

/*
 * Estrutura para métricas de memória:
 * - RAM, Memória Virtual, Memória Total Disponível
 * - Page faults (menores e maiores)
 * - Uso de swap
 */
typedef struct{
    unsigned long RAM;
    unsigned long MV;
    unsigned long MTD;
    unsigned long falha_pag_menor;
    unsigned long falha_pag_maior;
    unsigned long swap;
}metricas_memoria_t;

/*
 * Estrutura para métricas de I/O:
 * - Bytes e chamadas de leitura/escrita
 * - Operações de disco (leitura/escrita)
 */
typedef struct{
    unsigned long bytes_lidos;
    unsigned long bytes_escritos;
    unsigned long chamadas_lidas;
    unsigned long chamadas_escritas;
    unsigned long operacoes_lidas_disco;
    unsigned long operacoes_escritas_disco;
    unsigned long bytes_lidos_disco;
    unsigned long bytes_escritos_disco;
}metricas_io_t;

/*
 * Estrutura para métricas de rede:
 * - Bytes e pacotes recebidos/enviados
 * - Conexões ativas
 */
typedef struct{
    unsigned long bytes_recebidos;
    unsigned long bytes_enviados;
    unsigned long pacotes_recebidos;
    unsigned long pacotes_enviados;
    unsigned long conexoes_ativas;
}metricas_rede_t;

/* Funções para obter métricas de recursos do processo */
int get_metricas_cpu(pid_t pid, metricas_cpu_t* metricas);
int get_metricas_memoria(pid_t pid, metricas_memoria_t* metricas);
int get_metricas_io(pid_t pid, metricas_io_t* metricas);
int get_metricas_rede(pid_t pid, metricas_rede_t* metricas);

/* Função de experimento para medir overhead do monitoramento */
void experimento_overhead(void);

/* Monitora processo com intervalo e número de iterações */
void monitorar_processo(pid_t pid, int intervalo, int iteracoes);

/* Reseta estado interno da CPU para cálculos delta */
void resetar_estado_cpu(void);

#endif
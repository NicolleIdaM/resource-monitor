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

typedef struct{
    unsigned long tempo_usuario;
    unsigned long tempo_sistema;
    double porcentagem_cpu;
    unsigned long context_switches;
    unsigned long threads;
}metricas_cpu_t;

typedef struct{
    unsigned long RAM;
    unsigned long MV;
    unsigned long MTD;
    unsigned long page_faults_minor;
    unsigned long page_faults_major;
    unsigned long swap;
}metricas_memoria_t;

typedef struct{
    unsigned long bytes_lidos;
    unsigned long bytes_escritos;
    unsigned long chamadas_lidas;
    unsigned long chamadas_escritas;
}metricas_io_t;

typedef struct{
    unsigned long bytes_recebidos;
    unsigned long bytes_enviados;
    unsigned long pacotes_recebidos;
    unsigned long pacotes_enviados;
    unsigned long conexoes_ativas;
}metricas_rede_t;

int get_metricas_cpu(pid_t pid, metricas_cpu_t* metricas);
int get_metricas_memoria(pid_t pid, metricas_memoria_t* metricas);
int get_metricas_io(pid_t pid, metricas_io_t* metricas);
int get_metricas_rede(pid_t pid, metricas_rede_t* metricas);
void monitorar_processo(pid_t pid, int intervalo, int iteracoes);
void resetar_estado_cpu(void);

#endif
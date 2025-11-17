#include "../include/monitor.h"
#include <math.h>
#include <sys/time.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static struct{
    unsigned long ultimo_tempo_total;
    unsigned long ultimo_tempo_processo;
    unsigned long ultimo_context_switches;
    int primeira_chamada;
}estado_cpu = {0, 0, 0, 1};

void experimento_overhead() {
    printf("\n=== EXPERIMENTO 1: OVERHEAD DE MONITORAMENTO ===\n");
    
    pid_t pid = getpid();
    metricas_cpu_t cpu;
    const int iteracoes = 10000;
    const int tamanho_workload = 1000;
    
    for (int i = 0; i < 1000; i++) {
        volatile double x = sqrt(i);
        (void)x;
    }
    
    struct timespec inicio, fim;
    clock_gettime(CLOCK_MONOTONIC, &inicio);
    
    for (int i = 0; i < iteracoes; i++) {
        volatile double resultado = 0;
        for (int j = 0; j < tamanho_workload; j++) {
            resultado += sqrt(i + j);
        }
        (void)resultado;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &fim);
    double tempo_sem_monitor = (fim.tv_sec - inicio.tv_sec) + 
                              (fim.tv_nsec - inicio.tv_nsec) / 1e9;
    
    clock_gettime(CLOCK_MONOTONIC, &inicio);
    
    for (int i = 0; i < iteracoes; i++) {
        volatile double resultado = 0;
        for (int j = 0; j < tamanho_workload; j++) {
            resultado += sqrt(i + j);
        }
        (void)resultado;
        
        if (i % 100 == 0) {
            get_metricas_cpu(pid, &cpu);
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &fim);
    double tempo_com_monitor = (fim.tv_sec - inicio.tv_sec) + (fim.tv_nsec - inicio.tv_nsec) / 1e9;
    
    double overhead = 0;
    if (tempo_sem_monitor > 0.0001) {
        overhead = ((tempo_com_monitor - tempo_sem_monitor) / tempo_sem_monitor) * 100;
        if (overhead > 1000) overhead = 1000;
    }
    
    printf("Tempo sem monitor: %.6f segundos\n", tempo_sem_monitor);
    printf("Tempo com monitor: %.6f segundos\n", tempo_com_monitor);
    printf("Overhead: %.2f%%\n", overhead);
    printf("Latência de sampling: ~%.6f ms\n", (tempo_com_monitor - tempo_sem_monitor) * 1000 / iteracoes);
}
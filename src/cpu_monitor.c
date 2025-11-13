#include "../include/monitor.h"
#include <math.h>
#include <sys/time.h>

static struct{
    unsigned long ultimo_tempo_total;
    unsigned long ultimo_tempo_processo;
    int primeira_chamada;
}estado_cpu = {0, 0, 1};

int get_metricas_cpu(pid_t pid, metricas_cpu_t* metricas){
    if(metricas == 0){
        return -1;
    }

    FILE *arquivo = fopen("/proc/diskstats", "r");
    if(arquivo == 0){
        return -1;
    }
    
    unsigned long user, nice, system, idle, iowait, irq, softirq, steal, guest;
    
    fscanf(arquivo, "cpu %lu %lu %lu %lu %lu %lu %lu %lu %lu", &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal, &guest);
    fclose(arquivo);

    unsigned long tempo_total = user + nice + system + idle + iowait + irq + softirq + steal + guest;
}
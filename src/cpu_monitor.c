#include "../include/monitor.h"
#include <math.h>
#include <sys/time.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static struct{
    unsigned long ultimo_tempo_total;
    unsigned long ultimo_tempo_processo;
    unsigned long ultimo_context_switches;
    int primeira_chamada;
}estado_cpu = {0, 0, 0, 1};

int get_metricas_cpu(pid_t pid, metricas_cpu_t* metricas){
    if(metricas == NULL){
        errno = EINVAL;
        return -1;
    }

    FILE *arquivo = fopen("/proc/stat", "r");
    if(arquivo == NULL){
        perror("Erro ao abrir /proc/stat");
        return -1;
    }
    
    unsigned long user, nice, system, idle, iowait, irq, softirq;
    int leitura = fscanf(arquivo, "cpu %lu %lu %lu %lu %lu %lu %lu", 
           &user, &nice, &system, &idle, &iowait, &irq, &softirq);
    fclose(arquivo);

    if(leitura != 7){
        fprintf(stderr, "Erro ao ler estatísticas de CPU (lidos %d/7 campos)\n", leitura);
        errno = EIO;
        return -1;
    }

    unsigned long tempo_total = user + nice + system + idle + iowait + irq + softirq;

    char caminho[256];
    snprintf(caminho, sizeof(caminho), "/proc/%d/stat", pid);

    FILE *arquivo_processo = fopen(caminho, "r");
    if(arquivo_processo == NULL){
        perror("Erro ao abrir stat do processo");
        return -1;
    }

    unsigned long tempo_usuario, tempo_sistema;
    char comando[256];
    char estado;
    int pid_lido;
    unsigned long minflt, majflt, utime, stime, cutime, cstime, num_threads;
    long rss;
    
    leitura = fscanf(arquivo_processo, 
           "%d %s %c %*d %*d %*d %*d %*d %*u %lu %lu %*u %*u %lu %lu %ld %*d %*d %*d %*d %*u %lu", 
           &pid_lido, comando, &estado, &minflt, &majflt, &utime, &stime, &rss, &num_threads);
    fclose(arquivo_processo);

    if(leitura != 9){
        fprintf(stderr, "Erro ao ler estatísticas do processo (lidos %d/9 campos)\n", leitura);
        errno = EIO;
        return -1;
    }

    tempo_usuario = utime;
    tempo_sistema = stime;
    unsigned long tempo_processo = tempo_usuario + tempo_sistema;

    arquivo = fopen("/proc/stat", "r");
    unsigned long context_switches = 0;
    if(arquivo){
        char linha[256];
        while(fgets(linha, sizeof(linha), arquivo)){
            if(strstr(linha, "ctxt")){
                sscanf(linha, "ctxt %lu", &context_switches);
                break;
            }
        }
        fclose(arquivo);
    }

    if(estado_cpu.primeira_chamada == 0){
        unsigned long total_delta = tempo_total - estado_cpu.ultimo_tempo_total;
        unsigned long processo_delta = tempo_processo - estado_cpu.ultimo_tempo_processo;

        if(total_delta > 0){
            metricas->porcentagem_cpu = ((double)processo_delta / (double)total_delta) * 100.0;
            if(metricas->porcentagem_cpu > 100.0){
                metricas->porcentagem_cpu = 100.0;
            }
        } else {
            metricas->porcentagem_cpu = 0.0;
        }
    } else {
        metricas->porcentagem_cpu = 0.0;
        estado_cpu.primeira_chamada = 0;
    }

    metricas->tempo_usuario = tempo_usuario;
    metricas->tempo_sistema = tempo_sistema;
    metricas->threads = num_threads;
    
    if(estado_cpu.primeira_chamada == 0){
        metricas->context_switches = context_switches - estado_cpu.ultimo_context_switches;
    } else {
        metricas->context_switches = 0;
    }

    estado_cpu.ultimo_tempo_total = tempo_total;
    estado_cpu.ultimo_tempo_processo = tempo_processo;
    estado_cpu.ultimo_context_switches = context_switches;

    return 0;
}

void resetar_estado_cpu() {
    estado_cpu.primeira_chamada = 1;
    estado_cpu.ultimo_tempo_total = 0;
    estado_cpu.ultimo_tempo_processo = 0;
    estado_cpu.ultimo_context_switches = 0;
}
#include "../include/monitor.h"
#include <math.h>
#include <sys/time.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/*
 * Estado global para cálculo delta de CPU.
 * Mantém valores anteriores para calcular diferenças entre leituras.
 */
static struct{
    unsigned long ultimo_tempo_total;      /* Tempo total de CPU do sistema na última leitura */
    unsigned long ultimo_tempo_processo;   /* Tempo de CPU do processo na última leitura */
    unsigned long ultimo_context_switches; /* Context switches do sistema na última leitura */
    int primeira_chamada;                  /* Flag para identificar primeira chamada (1 = primeira) */
}estado_cpu = {0, 0, 0, 1};

/*
 * Obtém métricas de CPU para um processo específico.
 * Calcula percentual de CPU baseado em diferenças entre leituras consecutivas.
 */
int get_metricas_cpu(pid_t pid, metricas_cpu_t* metricas){
    if(metricas == NULL){
        errno = EINVAL;
        return -1;
    }

    /* Lê estatísticas globais de CPU do sistema a partir de /proc/stat */
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

    /* Calcula tempo total de CPU (soma todos os modos de execução) */
    unsigned long tempo_total = user + nice + system + idle + iowait + irq + softirq;

    /* Lê estatísticas específicas do processo a partir de /proc/[pid]/stat */
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
    unsigned long pag_menores, pag_maiores, qtde_threads;

    /* Parse do arquivo stat do processo - campos específicos:
     * %d: PID, %s: comando, %c: estado, %*d: pular campos, 
     * %lu: page faults menores, %lu: page faults maiores,
     * %lu: tempo usuário, %lu: tempo sistema, %lu: quantidade de threads */
    leitura = fscanf(arquivo_processo, 
    "%d %s %c %*d %*d %*d %*d %*d %*u %lu %lu %*u %*u %lu %lu %*d %*d %*d %*d %lu", 
    &pid_lido, comando, &estado, &pag_menores, &pag_maiores, &tempo_usuario, &tempo_sistema, &qtde_threads);
    fclose(arquivo_processo);

    if(leitura != 8){
    fprintf(stderr, "Erro ao ler estatísticas do processo (lidos %d/8 campos)\n", leitura);
        errno = EIO;
        return -1;
    }

    /* Tempo total de CPU usado pelo processo (usuário + sistema) */
    unsigned long tempo_processo = tempo_usuario + tempo_sistema;

    /* Lê contexto switches do sistema a partir de /proc/stat */
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

    /* Calcula percentual de CPU (exceto na primeira chamada quando não há dados anteriores) */
    if(estado_cpu.primeira_chamada == 0){
        /* Calcula diferenças desde a última medição */
        unsigned long total_delta = tempo_total - estado_cpu.ultimo_tempo_total;
        unsigned long processo_delta = tempo_processo - estado_cpu.ultimo_tempo_processo;

        if(total_delta > 0){
            /* Percentual = (tempo do processo / tempo total do sistema) * 100 */
            metricas -> porcentagem_cpu = ((double)processo_delta / (double)total_delta) * 100.0;
            /* Limita a 100% para evitar valores absurdos */
            if(metricas -> porcentagem_cpu > 100.0){
                metricas -> porcentagem_cpu = 100.0;
            }
        } else {
            metricas -> porcentagem_cpu = 0.0;
        }
        /* Context switches desde última medição */
        metricas -> context_switches = context_switches - estado_cpu.ultimo_context_switches;
    } else {
        /* Primeira chamada: não há dados anteriores para calcular delta */
        metricas -> porcentagem_cpu = 0.0;
        metricas -> context_switches = 0;
        estado_cpu.primeira_chamada = 0; /* Marca que a primeira chamada foi feita */
    }

    /* Preenche estrutura com valores absolutos (não dependem de delta) */
    metricas -> tempo_usuario = tempo_usuario;
    metricas -> tempo_sistema = tempo_sistema;
    metricas -> threads = qtde_threads;

    /* Atualiza estado para próxima chamada */
    estado_cpu.ultimo_tempo_total = tempo_total;
    estado_cpu.ultimo_tempo_processo = tempo_processo;
    estado_cpu.ultimo_context_switches = context_switches;

    return 0;
}

/*
 * Experimento para medir overhead do monitoramento de CPU.
 * Compara tempo de execução com e sem chamadas de monitoramento.
 */
void experimento_overhead() {
    printf("\n=== EXPERIMENTO 1: OVERHEAD DE MONITORAMENTO ===\n");
    
    pid_t pid = getpid();
    metricas_cpu_t cpu;
    const int iteracoes = 10000;
    const int workload_size = 1000;
    
    /* Warm-up: executa algumas iterações para aquecer cache da CPU */
    for (int i = 0; i < 1000; i++) {
        volatile double x = sqrt(i);
        (void)x; /* Evita warning de variável não utilizada */
    }
    
    /* Mede tempo SEM monitoramento */
    struct timespec inicio, fim;
    clock_gettime(CLOCK_MONOTONIC, &inicio);
    
    for (int i = 0; i < iteracoes; i++) {
        volatile double resultado = 0;
        /* Gera carga de CPU com cálculos matemáticos */
        for (int j = 0; j < workload_size; j++) {
            resultado += sqrt(i + j);
        }
        (void)resultado;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &fim);
    double tempo_sem_monitor = (fim.tv_sec - inicio.tv_sec) + (fim.tv_nsec - inicio.tv_nsec) / 1e9;
    
    /* Mede tempo COM monitoramento (chamadas a cada 100 iterações) */
    clock_gettime(CLOCK_MONOTONIC, &inicio);
    
    for (int i = 0; i < iteracoes; i++) {
        volatile double resultado = 0;
        for (int j = 0; j < workload_size; j++) {
            resultado += sqrt(i + j);
        }
        (void)resultado;
        
        /* Simula monitoramento periódico (a cada 100 iterações) */
        if (i % 100 == 0) {
            get_metricas_cpu(pid, &cpu);
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &fim);
    double tempo_com_monitor = (fim.tv_sec - inicio.tv_sec) + (fim.tv_nsec - inicio.tv_nsec) / 1e9;
    
    /* Calcula overhead percentual */
    double overhead = 0;
    if (tempo_sem_monitor > 0.0001) {
        overhead = ((tempo_com_monitor - tempo_sem_monitor) / tempo_sem_monitor) * 100;
        if (overhead > 1000) overhead = 1000; /* Limita overhead máximo para 1000% */
    }
    
    printf("Tempo sem monitor: %.6f segundos\n", tempo_sem_monitor);
    printf("Tempo com monitor: %.6f segundos\n", tempo_com_monitor);
    printf("Overhead: %.2f%%\n", overhead);
    printf("Latência de sampling: ~%.6f ms\n", (tempo_com_monitor - tempo_sem_monitor) * 1000 / iteracoes);
}
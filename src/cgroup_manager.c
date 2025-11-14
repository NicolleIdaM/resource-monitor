#define CGROUP_BASE "/sys/fs/cgroup"
#define MAX_LINE 256

#include "../include/monitor.h"
#include "../include/cgroup.h"
#include <dirent.h>
#include <sys/stat.h>
#include <math.h>

int get_metricas_cgroup(pid_t pid, metricas_cgroup_t* metricas){
    if(metricas == 0){
        return -1;
    }

    char caminho[512];
    FILE *arquivo;

    snprintf(caminho, sizeof(caminho), "/proc/%d/cgroup", pid);
    arquivo = fopen(caminho, "r");
    if(arquivo == 0){
        return -1;
    }

    char linha[MAX_LINE];
    if(fgets(linha, sizeof(linha), arquivo)){
        char *caminho_cgroup = strchr(linha, ":");
        if(caminho_cgroup){
            caminho_cgroup = strchr(caminho_cgroup + 1, ":");
            if(caminho_cgroup){
                caminho_cgroup++;
                printf("Processo no Cgroup: %s", caminho_cgroup);
            }
        }
    }
    fclose(arquivo);

    strncpy(metricas->cpu_usada, "100ms", sizeof(metricas->cpu_usada) - 1);
    strncpy(metricas->memoria_usada, "50MB", sizeof(metricas->memoria_usada) - 1);
    strncpy(metricas->memoria_limite, "max", sizeof(metricas->memoria_limite) - 1);

    return 0;
}

int criar_cgroup(const char* nome_cgroup){
    char caminho[512];

    snprintf(caminho, sizeof(caminho), CGROUP_BASE "/cpu/%s", nome_cgroup);
    if(mkdir(caminho, 0755) != 0 && errno != EEXIST){
        perror("Erro ao criar Cgroup CPU");
        return -1;
    }

    snprintf(caminho, sizeof(caminho), CGROUP_BASE "/memory/%s", nome_cgroup);
    if (mkdir(caminho, 0755) != 0 && errno != EEXIST) {
        perror("Erro ao criar cgroup Memory");
        return -1;
    }

    printf("Cgroup '%s' criado com sucesso\n", nome_cgroup);
    return 0;
}

int mover_cgroup(const char* nome_cgroup, pid_t pid){
    char caminho[512];
    char string[32];

    snprintf(string, sizeof(string), "%d", pid);

    snprintf(caminho, sizeof(caminho), CGROUP_BASE "/cpu/%s/cgroup.procs", nome_cgroup);
    FILE *arquivo = fopen(caminho, "w");
    if(arquivo == 0){
        perror("Erro ao abrir cgroup.procs para CPU");
        return -1;
    }
    fprintf(arquivo, "%s", string);
    fclose(arquivo);

    snprintf(caminho, sizeof(caminho), CGROUP_BASE "/memory/%s/cgroup.procs", nome_cgroup);
    arquivo = fopen(caminho, "w");
    if (arquivo == NULL) {
        perror("Erro ao abrir cgroup.procs para Memoria");
        return -1;
    }
    fprintf(arquivo, "%s", string);
    fclose(arquivo);

    printf("Processo %d movido para cgroup '%s'\n", pid, nome_cgroup);
    return 0;
}

int limite_cpu(const char* nome_cgroup, double cpu_cores){
    char caminho[512];

    int quota = (int)(cpu_cores * 100000.0);

    snprintf(caminho, sizeof(caminho), CGROUP_BASE "/cpu/%s/cpu.cfs_quota_us", nome_cgroup);
    FILE *arquivo = fopen(caminho, "w");
    if(arquivo == 0){
        perror("Erro ao definir limite de CPU");
        return -1;
    }
    fprintf(arquivo, "%d", quota);
    fclose(arquivo);

    printf("Limite de CPU definido para %.2f cores no cgroup '%s'\n", cpu_cores, nome_cgroup);
    return 0;
}

int limite_memoria(const char* nome_cgroup, unsigned long memoria_mb) {
    char caminho[512];
    unsigned long memoria_bytes = memoria_mb * pow(1024, 2);

    snprintf(caminho, sizeof(caminho), CGROUP_BASE "/memory/%s/memory.limit_in_bytes", nome_cgroup);
    FILE *arquivo = fopen(caminho, "w");
    if (arquivo == 0) {
        perror("Erro ao definir limite de memória");
        return -1;
    }
    fprintf(arquivo, "%lu", memoria_bytes);
    fclose(arquivo);

    printf("Limite de memória definido para %lu MB no cgroup '%s'\n", memoria_mb, nome_cgroup);
    return 0;
}

int limite_memoria(const char* nome_cgroup, unsigned long memoria_mb) {
    char caminho[512];
    unsigned long memoria_bytes = memoria_mb * pow(1024, 2);

    snprintf(caminho, sizeof(caminho), CGROUP_BASE "/memory/%s/memory.limit_in_bytes", nome_cgroup);
    FILE *arquivo = fopen(caminho, "w");
    if (arquivo == 0) {
        perror("Erro ao definir limite de memória");
        return -1;
    }
    fprintf(arquivo, "%lu", memoria_bytes);
    fclose(arquivo);

    printf("Limite de memória definido para %lu MB no cgroup '%s'\n", memoria_mb, nome_cgroup);
    return 0;
}

int remover_cgroup(const char* nome_cgroup) {
    char caminho[512];
    int sucesso = 0;

    snprintf(caminho, sizeof(caminho), CGROUP_BASE "/cpu/%s", nome_cgroup);
    if (rmdir(caminho) == 0) {
        printf("Cgroup CPU '%s' removido\n", nome_cgroup);
        sucesso++;
    }
}
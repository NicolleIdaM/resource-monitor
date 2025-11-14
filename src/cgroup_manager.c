#define CGROUP_BASE "/sys/fs/cgroup"
#define MAX_LINE 256

#include "../include/monitor.h"
#include "../include/cgroup.h"
#include <dirent.h>
#include <sys/stat.h>

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
}
#include "../include/monitor.h"
#include <sys/stat.h>
#include <fcntl.h>

int get_metricas_memoria(pid_t pid, metricas_memoria_t *metricas){
    if (metricas == 0){
        return -1;
    }
    
    char caminho[256];
    snprintf(caminho, sizeof(caminho), "/proc/%d/status", pid);

    FILE *arquivo = fopen(caminho, "r");
    if(arquivo == 0){
        perror("Erro ao abrir arquivo status");
        return -1;
    }
}
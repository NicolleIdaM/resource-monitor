#include "../include/monitor.h"
#include <sys/stat.h>
#include <fcntl.h>

int get_metricas_io(pid_t pid, metricas_io_t *metricas)
{
    if (metricas == NULL){
        return -1;
    }

    char caminho_io[256];
    snprintf(caminho_io, sizeof(caminho_io), "/proc/%d/io", pid);

    FILE *arquivo = fopen(caminho_io, "r");
    if(arquivo == NULL){
        perror("Erro ao abrir arquivo io");
        return -1;
    }
}
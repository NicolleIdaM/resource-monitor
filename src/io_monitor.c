#include "../include/monitor.h"
#include <sys/stat.h>
#include <fcntl.h>

int get_metricas_io(pid_t pid, metricas_io_t *metricas)
{
    if (metricas == NULL){
        return -1;
    }

    char caminho[256];
    snprintf(caminho, sizeof(caminho), "/proc/%d/io", pid);

    FILE *arquivo = fopen(caminho, "r");
    if(arquivo == NULL){
        perror("Erro ao abrir arquivo io");
        return -1;
    }

    char linha[256];
    while(fgets(linha, sizeof(linha), arquivo)){
        if (strstr(linha, "rchar:")){
            sscanf(linha, "rchar: %lu", &metricas->bytes_lidos);
        }else if (strstr(linha, "wchar:")){
            sscanf(linha, "wchar: %lu", &metricas->bytes_escritos);}
        else if (strstr(linha, "syscr:")){
            sscanf(linha, "syscr: %lu", &metricas->chamadas_lidas);
        }else if (strstr(linha, "syscw:")){
            sscanf(linha, "syscw: %lu", &metricas->chamadas_escritas);
        }
    }

    fclose(arquivo);
    return 0;
}
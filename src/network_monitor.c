#include "../include/monitor.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>

int get_metricas_rede(pid_t pid, metricas_rede_t *metricas) {
    if (metricas == NULL) {
        errno = EINVAL;
        return -1;
    }

    metricas->bytes_recebidos = 0;
    metricas->bytes_enviados = 0;
    metricas->pacotes_recebidos = 0;
    metricas->pacotes_enviados = 0;
    metricas->conexoes_ativas = 0;

    return 0;
}
#ifndef MONITOR_H
#define MONITOR_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>

typedef struct{
    unsigned long tempo_usuario;
    unsigned long tempo_sistema;
    double porcentagem_cpu;
}metricas_cpu;

typedef struct{
    unsigned long RAM;
    unsigned long MV;
    unsigned long MTD;
}metricas_memoria;

typedef struct{
    unsigned long bytes_lidos;
    unsigned long bytes_escritos;
    unsigned long chamadas_lidas;
    unsigned long chamadas_escritas;
}metricas_io;

typedef struct{
    int pid_namespace;
    int usuario_namespace;
    int filesystem_namespace;
    int net_namespace;
    int hostname_namespace;
    int ipc_namespace;
}metricas_namespace;

typedef struct {
    char cpu_usada[64];
    char memoria_usada[64];
    char memoria_limite[64];
} metricas_cgroup;

#endif
#ifndef CGROUP_H
#define CGROUP_H

#include <sys/types.h>

typedef struct{
    char cpu_usada[64];
    char memoria_usada[64];
    char memoria_usada[64];
}metricas_cgroup_t;

int get_metricas_cgroup(pid_t pid, metricas_cgroup_t* metricas);
int criar_cgroup(const char* nome_cgroup);
int mover_cgroup(const char* nome_cgroup, pid_t pid);
int limite_cpu(const char* nome_cgroup, double cpu_cores);
int limite_memoria(const char* nome_cgroup, unsigned long memoria_mb);
int remover_cgroup(const char* nome_cgroup);
void listar_cgroups();

#endif
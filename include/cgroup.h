#ifndef CGROUP_H
#define CGROUP_H

#include <sys/types.h>

typedef struct {
    char cpu_usada[64];
    char memoria_usada[64];
    char memoria_limite[64];
    char cgroup_version[16];
} metricas_cgroup_t;

int get_metricas_cgroup(pid_t pid, metricas_cgroup_t* metricas);
int criar_cgroup(const char* nome_cgroup);
int mover_cgroup(const char* nome_cgroup, pid_t pid);
int remover_cgroup(const char* nome_cgroup);
void listar_cgroups();

int limite_cpu(const char* nome_cgroup, double cpu_cores);
int limite_memoria(const char* nome_cgroup, unsigned long memoria_mb);
int limite_io(const char* nome_cgroup, unsigned long ler_bytes_segudos, unsigned long escrever_bytes_segundo);

int detectar_cgroup_version(void);
int criar_cgroup_v2(const char* nome_cgroup);
int limite_cpu_v2(const char* nome_cgroup, double cpu_cores);
int limite_memoria_v2(const char* nome_cgroup, unsigned long memoria_mb);

#endif
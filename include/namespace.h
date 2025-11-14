#ifndef NAMESPACE_H
#define NAMESPACE_H

#include <sys/types.h>

typedef struct{
    unsigned long pid_namespace;
    unsigned long usuario_namespace;
    unsigned long filesystem_namespace;
    unsigned long net_namespace;
    unsigned long hostname_namespace;
    unsigned long ipc_namespace;
}metricas_namespace_t;

int get_infos_namespace(pid_t pid, metricas_namespace_t* ns_infos);
int procurar_processo(const char* ns_tipo, const char* ns_id);
int comparar_namespace(pid_t pid1, pid_t pid2);
void listar_namespaces();
char* obter_tipo_namespace(const char* ns_link);

#endif
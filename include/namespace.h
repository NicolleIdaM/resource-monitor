#ifndef NAMESPACE_H
#define NAMESPACE_H

#include <sys/types.h>

/*
 * Estrutura para armazenar IDs de namespaces de processo.
 * Cada campo representa o inode do namespace correspondente.
 */
typedef struct{
    unsigned long pid_namespace;       /* Namespace de PID */
    unsigned long usuario_namespace;   /* Namespace de usuário */
    unsigned long filesystem_namespace;/* Namespace de filesystem (mount) */
    unsigned long net_namespace;       /* Namespace de rede */
    unsigned long hostname_namespace;  /* Namespace de hostname (UTS) */
    unsigned long ipc_namespace;       /* Namespace de IPC */
}metricas_namespace_t;

/* Obtém informações de namespaces de um processo */
int get_infos_namespace(pid_t pid, metricas_namespace_t* ns_infos);

/* Procura processos que estão em um namespace específico */
int procurar_processo(const char* ns_tipo, const char* ns_id);

/* Compara namespaces de dois processos */
int comparar_namespace(pid_t pid1, pid_t pid2);

/* Executa experimento de isolamento via namespaces */
void experimento_isolamento_namespace(void);

/* Lista namespaces de processos importantes (init, atual, etc.) */
void listar_namespaces();

/* Obtém tipo de namespace a partir de um link simbólico */
char* obter_tipo_namespace(const char* ns_link);

#endif
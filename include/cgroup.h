#ifndef CGROUP_H
#define CGROUP_H

#include <sys/types.h>

/*
 * Estrutura para armazenar métricas de uso de recursos via cgroup.
 * Inclui uso de CPU, memória, limite de memória e versão do cgroup.
 */
typedef struct {
    char cpu_usada[64];          /* Uso de CPU em segundos ou percentual */
    char memoria_usada[64];      /* Memória atualmente usada pelo processo */
    char memoria_limite[64];     /* Limite máximo de memória permitido */
    char cgroup_version[16];     /* Versão do cgroup (v1, v2 ou none) */
} metricas_cgroup_t;

/* Funções para coletar métricas de cgroup */
int get_metricas_cgroup(pid_t pid, metricas_cgroup_t* metricas);
int get_metricas_cgroup_v1(pid_t pid, metricas_cgroup_t* metricas);
int get_metricas_cgroup_v2(pid_t pid, metricas_cgroup_t* metricas);
int detectar_cgroup_version(void);

/* Funções para gerenciar cgroups (criação, movimentação, limites) */
int criar_cgroup(const char* nome_cgroup);
int mover_cgroup(const char* nome_cgroup, pid_t pid);
int limite_cpu(const char* nome_cgroup, double cpu_cores);
int limite_memoria(const char* nome_cgroup, unsigned long memoria_mb);
int limite_io(const char* nome_cgroup, unsigned long ler_bytes_segudos, unsigned long escrever_bytes_segundo);

/* Versões específicas para cgroup v2 */
int criar_cgroup_v2(const char* nome_cgroup);
int limite_cpu_v2(const char* nome_cgroup, double cpu_cores);
int limite_memoria_v2(const char* nome_cgroup, unsigned long memoria_mb);

/* Funções para monitorar e limitar I/O (blkio) */
int get_metricas_blkio(const char* nome_cgroup, unsigned long* bytes_lidos, unsigned long* bytes_escritos);
int limite_blkio_v1(const char* nome_cgroup, unsigned long ler_blkIo, unsigned long escrever_blkIo);

/* Remoção e listagem de cgroups */
int remover_cgroup(const char* nome_cgroup);
void listar_cgroups();

/* Move processo de volta para o cgroup raiz */
int mover_para_root(pid_t pid);

/* Funções de experimentos para testar limites de recursos */
void experimento_throttling_cpu(void);
void experimento_limite_memoria(void);
void experimento_limite_io(void);
void executar_experimentos(void);

/* Formata bytes em string legível (KB, MB, GB) */
void formatar_memoria(char* buffer, size_t buffer_size, unsigned long bytes);

#endif
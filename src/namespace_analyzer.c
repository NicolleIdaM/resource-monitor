#define _DEFAULT_SOURCE

#include "../include/monitor.h"
#include "../include/namespace.h"
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int get_infos_namespace(pid_t pid, metricas_namespace_t* metricas){
    if(metricas == NULL || pid <= 0){
        errno = EINVAL;
        return -1;
    }

    char caminho[256];
    struct stat numero_inode;
    int sucesso = 0;

    metricas->pid_namespace = 0;
    metricas->usuario_namespace = 0;
    metricas->filesystem_namespace = 0;
    metricas->net_namespace = 0;
    metricas->hostname_namespace = 0;
    metricas->ipc_namespace = 0;

    snprintf(caminho, sizeof(caminho), "/proc/%d/ns/pid", pid);
    if(stat(caminho, &numero_inode) == 0){
        metricas->pid_namespace = numero_inode.st_ino;
        sucesso++;
    } else {
        perror("Erro ao acessar pid namespace");
    }

    snprintf(caminho, sizeof(caminho), "/proc/%d/ns/user", pid);
    if(stat(caminho, &numero_inode) == 0){
        metricas->usuario_namespace = numero_inode.st_ino;
        sucesso++;
    } else {
        perror("Erro ao acessar user namespace");
    }

    snprintf(caminho, sizeof(caminho), "/proc/%d/ns/mnt", pid);
    if(stat(caminho, &numero_inode) == 0){
        metricas->filesystem_namespace = numero_inode.st_ino;
        sucesso++;
    } else {
        perror("Erro ao acessar mnt namespace");
    }

    snprintf(caminho, sizeof(caminho), "/proc/%d/ns/net", pid);
    if(stat(caminho, &numero_inode) == 0){
        metricas->net_namespace = numero_inode.st_ino;
        sucesso++;
    } else {
        perror("Erro ao acessar net namespace");
    }

    snprintf(caminho, sizeof(caminho), "/proc/%d/ns/uts", pid);
    if(stat(caminho, &numero_inode) == 0){
        metricas->hostname_namespace = numero_inode.st_ino;
        sucesso++;
    } else {
        perror("Erro ao acessar uts namespace");
    }

    snprintf(caminho, sizeof(caminho), "/proc/%d/ns/ipc", pid);
    if(stat(caminho, &numero_inode) == 0){
        metricas->ipc_namespace = numero_inode.st_ino;
        sucesso++;
    } else {
        perror("Erro ao acessar ipc namespace");
    }

    if (sucesso == 0) {
        fprintf(stderr, "Erro: não foi possível ler nenhum namespace para PID %d\n", pid);
        errno = ENOENT;
        return -1;
    }

    printf("Sucesso: lidos %d/6 namespaces para PID %d\n", sucesso, pid);
    return 0;
}

int comparar_namespace(pid_t pid1, pid_t pid2){
    if (pid1 <= 0 || pid2 <= 0) {
        errno = EINVAL;
        return -1;
    }

    metricas_namespace_t namespace1, namespace2;

    if(get_infos_namespace(pid1, &namespace1) == -1){
        fprintf(stderr, "Erro ao obter namespaces para PID %d\n", pid1);
        return -1;
    }

    if(get_infos_namespace(pid2, &namespace2) == -1){
        fprintf(stderr, "Erro ao obter namespaces para PID %d\n", pid2);
        return -1;
    }

    int diferentes = 0;
    
    if(namespace1.pid_namespace != namespace2.pid_namespace){
        printf("  PID Namespace diferente: %lu vs %lu\n", 
               namespace1.pid_namespace, namespace2.pid_namespace);
        diferentes++;
    }
    
    if (namespace1.usuario_namespace != namespace2.usuario_namespace){
        printf("  User Namespace diferente: %lu vs %lu\n", 
               namespace1.usuario_namespace, namespace2.usuario_namespace);
        diferentes++;
    }
    
    if (namespace1.filesystem_namespace != namespace2.filesystem_namespace){
        printf("  Mount Namespace diferente: %lu vs %lu\n", 
               namespace1.filesystem_namespace, namespace2.filesystem_namespace);
        diferentes++;
    }
    
    if (namespace1.net_namespace != namespace2.net_namespace){
        printf("  Network Namespace diferente: %lu vs %lu\n", 
               namespace1.net_namespace, namespace2.net_namespace);
        diferentes++;
    }
    
    if (namespace1.hostname_namespace != namespace2.hostname_namespace){
        printf("  UTS Namespace diferente: %lu vs %lu\n", 
               namespace1.hostname_namespace, namespace2.hostname_namespace);
        diferentes++;
    }
    
    if (namespace1.ipc_namespace != namespace2.ipc_namespace){
        printf("  IPC Namespace diferente: %lu vs %lu\n", 
               namespace1.ipc_namespace, namespace2.ipc_namespace);
        diferentes++;
    }

    printf("Total de namespaces diferentes: %d/6\n", diferentes);
    return diferentes;
}

int procurar_processo(const char* ns_tipo, const char* ns_id){
    if (ns_tipo == NULL || ns_id == NULL) {
        errno = EINVAL;
        return -1;
    }

    DIR *dir;
    struct dirent *entrada_diretorio;
    char caminho[512];
    char namespace_caminho[512];
    
    char *endptr;
    unsigned long ns_inode = strtoul(ns_id, &endptr, 10);
    if (endptr == ns_id || *endptr != '\0') {
        fprintf(stderr, "Erro: ID de namespace inválido: %s\n", ns_id);
        errno = EINVAL;
        return -1;
    }

    dir = opendir("/proc");
    if(dir == NULL){
        perror("Erro ao abrir o diretorio /proc");
        return -1;
    }

    printf("Processos no namespace %s [%s]:\n", ns_tipo, ns_id);
    int processos_encontrados = 0;

    while((entrada_diretorio = readdir(dir)) != NULL){
        if(entrada_diretorio->d_type == DT_DIR){
            int pid = atoi(entrada_diretorio->d_name);
            if(pid > 0){
                snprintf(namespace_caminho, sizeof(namespace_caminho), 
                        "/proc/%s/ns/%s", entrada_diretorio->d_name, ns_tipo);
                
                struct stat numero_inode;
                if(stat(namespace_caminho, &numero_inode) == 0){
                    if(numero_inode.st_ino == ns_inode){
                        snprintf(caminho, sizeof(caminho), "/proc/%s/comm", entrada_diretorio->d_name);
                        
                        FILE *arquivo_comando = fopen(caminho, "r");
                        char comando[256] = "desconhecido";
                        if(arquivo_comando){
                            if (fgets(comando, sizeof(comando), arquivo_comando) != NULL) {
                                comando[strcspn(comando, "\n")] = 0;
                            }
                            fclose(arquivo_comando);
                        } else {
                            perror("Erro ao ler comando do processo");
                        }
                        
                        printf("  PID: %s, Comando: %s\n", entrada_diretorio->d_name, comando);
                        processos_encontrados++;
                    }
                }
            }
        }
    }

    closedir(dir);
    
    if (processos_encontrados == 0) {
        printf("  Nenhum processo encontrado neste namespace.\n");
    } else {
        printf("  Total de processos encontrados: %d\n", processos_encontrados);
    }
    
    return processos_encontrados;
}

void listar_namespaces(){
    pid_t pids[] = {1, getpid(), 0};
    const char* nomes[] = {"init (PID 1)", "Processo Atual", ""};
    int sucesso_total = 0;

    for(int i = 0; pids[i] != 0; i++){
        metricas_namespace_t metricas;
        if(get_infos_namespace(pids[i], &metricas) == 0){
            printf("Namespaces do %s:\n", nomes[i]);
            printf("  PID Namespace: %lu\n", (unsigned long)metricas.pid_namespace);
            printf("  Usuario Namespace: %lu\n", (unsigned long)metricas.usuario_namespace);
            printf("  Filesystem Namespace: %lu\n", (unsigned long)metricas.filesystem_namespace);
            printf("  Network Namespace: %lu\n", (unsigned long)metricas.net_namespace);
            printf("  Hostname Namespace: %lu\n", (unsigned long)metricas.hostname_namespace);
            printf("  IPC Namespace: %lu\n", (unsigned long)metricas.ipc_namespace);
            printf("\n");
            sucesso_total++;
        }else{
            printf("Erro ao obter informacoes do namespace para o PID %d\n", pids[i]);
        }
    }
    
    if (sucesso_total == 0) {
        printf("Nenhum namespace pôde ser listado.\n");
    }
}

char* obter_tipo_namespace(const char* ns_link){
    if (ns_link == NULL) {
        return "desconhecido";
    }
    
    static char tipo[32];
    const char *prefixo = strrchr(ns_link, '/');
    if(prefixo){
        strncpy(tipo, prefixo + 1, sizeof(tipo) - 1);
        tipo[sizeof(tipo) - 1] = '\0';
        return tipo;
    }
    
    strncpy(tipo, "desconhecido", sizeof(tipo) - 1);
    return tipo;
}
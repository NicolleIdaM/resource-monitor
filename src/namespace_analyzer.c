#define _DEFAULT_SOURCE

#include "../include/monitor.h"
#include "../include/namespace.h"
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

int get_infos_namespace(pid_t pid, metricas_namespace_t* metricas){
    if(metricas == 0 && pid <= 0){
        return -1;
    }

    char caminho[256];
    struct stat numero_inode;

    snprintf(caminho, sizeof(caminho), "/proc/%d/ns/pid", pid);
    if(stat(caminho, &numero_inode) == 0){
        metricas -> pid_namespace = numero_inode.st_ino;
    }else{
        return -1;
    }

    snprintf(caminho, sizeof(caminho), "/proc/%d/ns/user", pid);
    if(stat(caminho, &numero_inode) == 0){
        metricas -> usuario_namespace = numero_inode.st_ino;
    }else{
        return -1;
    }

    snprintf(caminho, sizeof(caminho), "/proc/%d/ns/mnt", pid);
    if(stat(caminho, &numero_inode) == 0){
        metricas -> filesystem_namespace = numero_inode.st_ino;
    }else{
        return -1;
    }

    snprintf(caminho, sizeof(caminho), "/proc/%d/ns/net", pid);
    if(stat(caminho, &numero_inode) == 0){
        metricas -> net_namespace = numero_inode.st_ino;
    }else{
        return -1;
    }

    snprintf(caminho, sizeof(caminho), "/proc/%d/ns/uts", pid);
    if(stat(caminho, &numero_inode) == 0){
        metricas -> hostname_namespace = numero_inode.st_ino;
    }else{
        return -1;
    }

    snprintf(caminho, sizeof(caminho), "/proc/%d/ns/ipc", pid);
    if(stat(caminho, &numero_inode) == 0){
        metricas -> ipc_namespace = numero_inode.st_ino;
    }else{
        return -1;
    }

    return 0;
}

int comparar_namespace(pid_t pid1, pid_t pid2){
    metricas_namespace_t namespace1, namespace2;

    if(get_infos_namespace(pid1, &namespace1) == -1){
        return -1;
    }

    if(get_infos_namespace(pid2, &namespace2) == -1){
        return -1;
    }

    int diferentes = 0;
    if(namespace1.pid_namespace != namespace2.pid_namespace){
        diferentes++;
    }
    if (namespace1.usuario_namespace != namespace2.usuario_namespace){
        diferentes++;
    }
    if (namespace1.filesystem_namespace != namespace2.filesystem_namespace){
        diferentes++;
    }
    if (namespace1.net_namespace != namespace2.net_namespace){
        diferentes++;
    }
    if (namespace1.hostname_namespace != namespace2.hostname_namespace){
        diferentes++;
    }
    if (namespace1.ipc_namespace != namespace2.ipc_namespace){
        diferentes++;
    }

    return diferentes;
}

int procurar_processo(const char* ns_tipo, const char* ns_id){
    DIR *dir;
    struct dirent *entrada_diretorio;
    char caminho[512];  // Corrigido o typo "caminhpo"
    char namespace_caminho[512];
    
    unsigned long ns_inode = strtoul(ns_id, NULL, 10);

    dir = opendir("/proc");
    if(dir == 0){
        perror("Erro ao abrir o diretorio /proc");
        return -1;
    }

    printf("Processos no namespace %s [%s]:\n", ns_tipo, ns_id);

    while((entrada_diretorio = readdir(dir)) != NULL){
        if(entrada_diretorio->d_type == DT_DIR && atoi(entrada_diretorio->d_name) > 0){
            snprintf(namespace_caminho, sizeof(namespace_caminho), 
                    "/proc/%s/ns/%s", entrada_diretorio->d_name, ns_tipo);
            
            struct stat numero_inode;
            if(stat(namespace_caminho, &numero_inode) == 0){
                if(numero_inode.st_ino == ns_inode){
                    snprintf(caminho, sizeof(caminho), "/proc/%s/comm", entrada_diretorio->d_name);
                    
                    FILE *arquivo_comando = fopen(caminho, "r");
                    char comando[256] = "desconhecido";
                    if(arquivo_comando){
                        fgets(comando, sizeof(comando), arquivo_comando);
                        comando[strcspn(comando, "\n")] = 0;
                        fclose(arquivo_comando);
                    }
                    
                    printf("  PID: %s, Comando: %s\n", entrada_diretorio->d_name, comando);
                }
            }
        }
    }

    closedir(dir);
    return 0;
}

void listar_namespaces(){
    pid_t pids[] = {1, getpid(), 0};
    const char* nomes[] = {"init (PID 1)", "Processo Atual", ""};

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
        }else{
            printf("Erro ao obter informacoes do namespace para o PID %d\n", pids[i]);
        }
    }
}

char* obter_tipo_namespace(const char* ns_link){
    static char tipo[32];
    const char *prefixo = strrchr(ns_link, '/');
    if(prefixo){
        strncpy(tipo, prefixo + 1, sizeof(tipo) - 1);
        tipo[sizeof(tipo) - 1] = '\0';
        return tipo;
    }
    return "desconhecido";
}
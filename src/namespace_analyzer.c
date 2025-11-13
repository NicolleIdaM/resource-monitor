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
    char caminhpo[512];
    char namespace_caminho[512];
    char armazenar_links[1024];
    ssize_t armazenar_tamanho;

    unsigned long ns_inode = strtoul(ns_id, NULL, 10);

    dir = opendir("/proc");
    if(dir == 0){
        perror("Erro ao abrir o diretorio /proc");
        return -1;
    }

    printf("Processos no namespace %s [%s]:\n", ns_tipo, ns_id);

    while((entrada_diretorio = readdir(dir)) != 0){
        if(entrada_diretorio -> d_type == DT_DIR && atoi(entrada_diretorio -> d_name) > 0){
            
        }
    }
}
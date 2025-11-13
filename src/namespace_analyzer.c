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
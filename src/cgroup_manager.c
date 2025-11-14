#define _GNU_SOURCE
#include "../include/monitor.h"
#include "../include/cgroup.h"
#include <dirent.h>
#include <sys/stat.h>
#include <math.h>
#include <unistd.h>

int detectar_cgroup_version(void) {
    struct stat st;
    
    if (stat("/sys/fs/cgroup/cgroup.controllers", &st) == 0) {
        return 2;
    }
    
    if (stat("/sys/fs/cgroup/cpu", &st) == 0) {
        return 1;
    }
    
    return 0;
}

int criar_cgroup(const char* nome_cgroup) {
    int version = detectar_cgroup_version();
    
    if (version == 2) {
        return criar_cgroup_v2(nome_cgroup);
    } else if (version == 1) {
        char caminho[512];

        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/cpu/%s", nome_cgroup);
        if (mkdir(caminho, 0755) != 0 && errno != EEXIST) {
            perror("Erro ao criar Cgroup CPU");
            return -1;
        }

        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/memory/%s", nome_cgroup);
        if (mkdir(caminho, 0755) != 0 && errno != EEXIST) {
            perror("Erro ao criar cgroup Memory");
            return -1;
        }

        printf("Cgroup v1 '%s' criado com sucesso\n", nome_cgroup);
        return 0;
    } else {
        printf("Sistema não suporta cgroups\n");
        return -1;
    }
}

int criar_cgroup_v2(const char* nome_cgroup) {
    char caminho[512];
    int version = detectar_cgroup_version();
    
    if (version != 2) {
        printf("Sistema não usa cgroup v2\n");
        return -1;
    }
    
    snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/%s", nome_cgroup);
    
    if (mkdir(caminho, 0755) != 0 && errno != EEXIST) {
        perror("Erro ao criar cgroup v2");
        return -1;
    }
    
    char controladores[512];
    snprintf(controladores, sizeof(controladores), "%s/cgroup.subtree_control", caminho);
    
    FILE *arquivo = fopen(controladores, "w");
    if (arquivo) {
        fprintf(arquivo, "+cpu +memory +io");
        fclose(arquivo);
    }
    
    printf("Cgroup v2 '%s' criado com sucesso\n", nome_cgroup);
    return 0;
}

int mover_cgroup(const char* nome_cgroup, pid_t pid) {
    int version = detectar_cgroup_version();
    char caminho[512];
    char pid_str[32];

    snprintf(pid_str, sizeof(pid_str), "%d", pid);

    if (version == 2) {
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/%s/cgroup.procs", nome_cgroup);
        FILE *arquivo = fopen(caminho, "w");
        if (arquivo == NULL) {
            perror("Erro ao abrir cgroup.procs para cgroup v2");
            return -1;
        }
        fprintf(arquivo, "%s", pid_str);
        fclose(arquivo);
    } else if (version == 1) {
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/cpu/%s/cgroup.procs", nome_cgroup);
        FILE *arquivo = fopen(caminho, "w");
        if (arquivo == NULL) {
            perror("Erro ao abrir cgroup.procs para CPU");
            return -1;
        }
        fprintf(arquivo, "%s", pid_str);
        fclose(arquivo);

        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/memory/%s/cgroup.procs", nome_cgroup);
        arquivo = fopen(caminho, "w");
        if (arquivo == NULL) {
            perror("Erro ao abrir cgroup.procs para Memoria");
            return -1;
        }
        fprintf(arquivo, "%s", pid_str);
        fclose(arquivo);
    } else {
        printf("cgroups não disponíveis\n");
        return -1;
    }

    printf("Processo %d movido para cgroup '%s'\n", pid, nome_cgroup);
    return 0;
}

int limite_cpu(const char* nome_cgroup, double cpu_cores) {
    int version = detectar_cgroup_version();
    
    if (version == 2) {
        return limite_cpu_v2(nome_cgroup, cpu_cores);
    } else if (version == 1) {
        return limite_cpu_v1(nome_cgroup, cpu_cores);
    } else {
        printf("cgroups não disponíveis\n");
        return -1;
    }
}

int limite_cpu_v1(const char* nome_cgroup, double cpu_cores) {
    char caminho[512];
    int quota = (int)(cpu_cores * 100000.0);

    snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/cpu/%s/cpu.cfs_quota_us", nome_cgroup);
    FILE *arquivo = fopen(caminho, "w");
    if (arquivo == NULL) {
        perror("Erro ao definir limite de CPU v1");
        return -1;
    }
    fprintf(arquivo, "%d", quota);
    fclose(arquivo);

    snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/cpu/%s/cpu.cfs_period_us", nome_cgroup);
    arquivo = fopen(caminho, "w");
    if (arquivo) {
        fprintf(arquivo, "100000");
        fclose(arquivo);
    }

    printf("Limite de CPU v1 definido para %.2f cores no cgroup '%s'\n", cpu_cores, nome_cgroup);
    return 0;
}

int limite_cpu_v2(const char* nome_cgroup, double cpu_cores) {
    char caminho[512];
    int version = detectar_cgroup_version();
    
    if (version != 2) {
        printf("Sistema não usa cgroup v2\n");
        return -1;
    }
    
    unsigned int peso = (unsigned int)(cpu_cores * 100.0);
    if (peso < 1) peso = 1;
    if (peso > 10000) peso = 10000;
    
    snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/%s/cpu.weight", nome_cgroup);
    FILE *arquivo = fopen(caminho, "w");
    if (arquivo == NULL) {
        perror("Erro ao definir limite de CPU v2");
        return -1;
    }
    fprintf(arquivo, "%u", peso);
    fclose(arquivo);
    
    if (cpu_cores < 1.0) {
        unsigned int max = (unsigned int)(cpu_cores * 100000.0);
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/%s/cpu.max", nome_cgroup);
        arquivo = fopen(caminho, "w");
        if (arquivo) {
            fprintf(arquivo, "%u 100000", max);
            fclose(arquivo);
        }
    }
    
    printf("Limite de CPU v2 definido para %.2f cores (peso: %u) no cgroup '%s'\n", 
           cpu_cores, peso, nome_cgroup);
    return 0;
}

int limite_memoria(const char* nome_cgroup, unsigned long memoria_mb) {
    int version = detectar_cgroup_version();
    
    if (version == 2) {
        return limite_memoria_v2(nome_cgroup, memoria_mb);
    } else if (version == 1) {
        return limite_memoria_v1(nome_cgroup, memoria_mb);
    } else {
        printf("cgroups não disponíveis\n");
        return -1;
    }
}

int limite_memoria_v1(const char* nome_cgroup, unsigned long memoria_mb) {
    char caminho[512];
    unsigned long memoria_bytes = memoria_mb * 1024 * 1024;

    snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/memory/%s/memory.limit_in_bytes", nome_cgroup);
    FILE *arquivo = fopen(caminho, "w");
    if (arquivo == NULL) {
        perror("Erro ao definir limite de memória v1");
        return -1;
    }
    fprintf(arquivo, "%lu", memoria_bytes);
    fclose(arquivo);

    printf("Limite de memória v1 definido para %lu MB no cgroup '%s'\n", memoria_mb, nome_cgroup);
    return 0;
}

int limite_memoria_v2(const char* nome_cgroup, unsigned long memoria_mb) {
    char caminho[512];
    int version = detectar_cgroup_version();
    
    if (version != 2) {
        printf("Sistema não usa cgroup v2\n");
        return -1;
    }
    
    unsigned long memoria_bytes = memoria_mb * 1024 * 1024;
    
    snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/%s/memory.max", nome_cgroup);
    FILE *arquivo = fopen(caminho, "w");
    if (arquivo == NULL) {
        perror("Erro ao definir limite de memória v2");
        return -1;
    }
    fprintf(arquivo, "%lu", memoria_bytes);
    fclose(arquivo);
    
    snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/%s/memory.swap.max", nome_cgroup);
    arquivo = fopen(caminho, "w");
    if (arquivo) {
        fprintf(arquivo, "%lu", memoria_bytes);
        fclose(arquivo);
    }
    
    printf("Limite de memória v2 definido para %lu MB no cgroup '%s'\n", 
           memoria_mb, nome_cgroup);
    return 0;
}

int limite_io(const char* nome_cgroup, unsigned long bytes_leitura_por_segundo, unsigned long bytes_escrita_por_segundo) {
    char caminho[512];
    int version = detectar_cgroup_version();
    
    if (version == 2) {
        if (bytes_leitura_por_segundo > 0 || bytes_escrita_por_segundo > 0) {
            snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/%s/io.max", nome_cgroup);
            FILE *arquivo = fopen(caminho, "w");
            if (arquivo) {
                if (bytes_leitura_por_segundo > 0 && bytes_escrita_por_segundo > 0) {
                    fprintf(arquivo, "8:0 rbps=%lu wbps=%lu", bytes_leitura_por_segundo, bytes_escrita_por_segundo);
                } else if (bytes_leitura_por_segundo > 0) {
                    fprintf(arquivo, "8:0 rbps=%lu", bytes_leitura_por_segundo);
                } else if (bytes_escrita_por_segundo > 0) {
                    fprintf(arquivo, "8:0 wbps=%lu", bytes_escrita_por_segundo);
                }
                fclose(arquivo);
            }
        }
    } else if (version == 1) {
        if (bytes_leitura_por_segundo > 0) {
            snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/blkio/%s/blkio.throttle.read_bps_device", nome_cgroup);
            FILE *arquivo = fopen(caminho, "w");
            if (arquivo) {
                fprintf(arquivo, "8:0 %lu", bytes_leitura_por_segundo);
                fclose(arquivo);
            }
        }
        
        if (bytes_escrita_por_segundo > 0) {
            snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/blkio/%s/blkio.throttle.write_bps_device", nome_cgroup);
            FILE *arquivo = fopen(caminho, "w");
            if (arquivo) {
                fprintf(arquivo, "8:0 %lu", bytes_escrita_por_segundo);
                fclose(arquivo);
            }
        }
    } else {
        printf("cgroup não disponível para limitação de I/O\n");
        return -1;
    }
    
    printf("Limites de I/O definidos - Leitura: %lu B/s, Escrita: %lu B/s no cgroup '%s'\n",
           bytes_leitura_por_segundo, bytes_escrita_por_segundo, nome_cgroup);
    return 0;
}

int remover_cgroup(const char* nome_cgroup) {
    char caminho[512];
    int sucesso = 0;
    int version = detectar_cgroup_version();

    if (version == 2) {
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/%s", nome_cgroup);
        if (rmdir(caminho) == 0) {
            printf("Cgroup v2 '%s' removido\n", nome_cgroup);
            sucesso = 1;
        }
    } else if (version == 1) {
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/cpu/%s", nome_cgroup);
        if (rmdir(caminho) == 0) {
            printf("Cgroup CPU '%s' removido\n", nome_cgroup);
            sucesso++;
        }

        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/memory/%s", nome_cgroup);
        if (rmdir(caminho) == 0) {
            printf("Cgroup Memory '%s' removido\n", nome_cgroup);
            sucesso++;
        }
    }

    if ((version == 1 && sucesso == 2) || (version == 2 && sucesso == 1)) {
        printf("Cgroup removido com sucesso\n");
        return 0;
    } else {
        printf("Apenas %d componentes do cgroup foram removidos\n", sucesso);
        return -1;
    }
}

void listar_cgroups() {
    DIR *dir;
    struct dirent *entrada_diretorio;
    int version = detectar_cgroup_version();

    printf("Versão do cgroup detectada: %d\n", version);

    if (version == 2) {
        printf("\nCgroups v2:\n");
        dir = opendir("/sys/fs/cgroup");
        if (dir) {
            while ((entrada_diretorio = readdir(dir)) != NULL) {
                if (entrada_diretorio->d_type == DT_DIR && 
                    entrada_diretorio->d_name[0] != '.' &&
                    strcmp(entrada_diretorio->d_name, "..") != 0) {
                    printf("  %s\n", entrada_diretorio->d_name);
                }
            }
            closedir(dir);
        }
    } else if (version == 1) {
        printf("\nCgroups de CPU:\n");
        dir = opendir("/sys/fs/cgroup/cpu");
        if (dir) {
            while ((entrada_diretorio = readdir(dir)) != NULL) {
                if (entrada_diretorio->d_type == DT_DIR && 
                    entrada_diretorio->d_name[0] != '.' &&
                    strcmp(entrada_diretorio->d_name, "..") != 0) {
                    printf("  %s\n", entrada_diretorio->d_name);
                }
            }
            closedir(dir);
        }

        printf("\nCgroups de Memória:\n");
        dir = opendir("/sys/fs/cgroup/memory");
        if (dir) {
            while ((entrada_diretorio = readdir(dir)) != NULL) {
                if (entrada_diretorio->d_type == DT_DIR && 
                    entrada_diretorio->d_name[0] != '.' &&
                    strcmp(entrada_diretorio->d_name, "..") != 0) {
                    printf("  %s\n", entrada_diretorio->d_name);
                }
            }
            closedir(dir);
        }
    } else {
        printf("cgroups não disponíveis\n");
    }
}

int get_metricas_cgroup(pid_t pid, metricas_cgroup_t* metricas) {
    if (metricas == NULL) {
        return -1;
    }

    int version = detectar_cgroup_version();
    char caminho[512];
    FILE *arquivo;

    if (version == 2) {
        strncpy(metricas->cgroup_version, "v2", sizeof(metricas->cgroup_version) - 1);
    } else if (version == 1) {
        strncpy(metricas->cgroup_version, "v1", sizeof(metricas->cgroup_version) - 1);
    } else {
        strncpy(metricas->cgroup_version, "none", sizeof(metricas->cgroup_version) - 1);
    }

    if (version == 2) {
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/cpu.stat");
    } else {
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/cpu/user.slice/cpuacct.usage");
    }
    
    arquivo = fopen(caminho, "r");
    if (arquivo) {
        unsigned long cpu_usage;
        if (version == 2) {
            char linha[256];
            while (fgets(linha, sizeof(linha), arquivo)) {
                if (strstr(linha, "usage_usec")) {
                    sscanf(linha, "usage_usec %lu", &cpu_usage);
                    break;
                }
            }
        } else {
            fscanf(arquivo, "%lu", &cpu_usage);
        }
        fclose(arquivo);
        snprintf(metricas->cpu_usada, sizeof(metricas->cpu_usada), "%luns", cpu_usage);
    } else {
        strncpy(metricas->cpu_usada, "N/A", sizeof(metricas->cpu_usada) - 1);
    }

    if (version == 2) {
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/memory.current");
    } else {
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/memory/user.slice/memory.usage_in_bytes");
    }
    
    arquivo = fopen(caminho, "r");
    if (arquivo) {
        unsigned long mem_usage;
        fscanf(arquivo, "%lu", &mem_usage);
        fclose(arquivo);
        snprintf(metricas->memoria_usada, sizeof(metricas->memoria_usada), "%luMB", mem_usage / (1024 * 1024));
    } else {
        strncpy(metricas->memoria_usada, "N/A", sizeof(metricas->memoria_usada) - 1);
    }

    strncpy(metricas->memoria_limite, "max", sizeof(metricas->memoria_limite) - 1);

    return 0;
}
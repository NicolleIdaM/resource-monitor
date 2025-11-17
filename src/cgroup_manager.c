#define _GNU_SOURCE
#include "../include/monitor.h"
#include "../include/cgroup.h"
#include <dirent.h>
#include <sys/stat.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/*****************************/
/*      MÉTRICAS CGROUP      */
/*****************************/
int get_metricas_cgroup(pid_t pid, metricas_cgroup_t* metricas) {
    (void)pid;
    
    if (metricas == NULL)  {
        errno = EINVAL;
        return -1;
    }

    int version = detectar_cgroup_version();
    char caminho[512];
    FILE *arquivo;

    strncpy(metricas->cpu_usada, "N/A", sizeof(metricas->cpu_usada) - 1);
    strncpy(metricas->memoria_usada, "N/A", sizeof(metricas->memoria_usada) - 1);
    strncpy(metricas->memoria_limite, "max", sizeof(metricas->memoria_limite) - 1);

    if (version == 2) {
        strncpy(metricas->cgroup_version, "v2", sizeof(metricas->cgroup_version) - 1);
    } else if (version == 1) {
        strncpy(metricas->cgroup_version, "v1", sizeof(metricas->cgroup_version) - 1);
    } else {
        strncpy(metricas->cgroup_version, "none", sizeof(metricas->cgroup_version) - 1);
        return 0;
    }

    if (version == 2) {
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/cpu.stat");
        arquivo = fopen(caminho, "r");
        if (arquivo) {
            unsigned long cpu_usage = 0;
            char linha[256];
            while (fgets(linha, sizeof(linha), arquivo)) {
                if (strstr(linha, "usage_usec")) {
                    if (sscanf(linha, "usage_usec %lu", &cpu_usage) == 1) {
                        snprintf(metricas->cpu_usada, sizeof(metricas->cpu_usada), "%luns", cpu_usage * 1000);
                    }
                    break;
                }
            }
            fclose(arquivo);
        }
    } else {
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/cpu/user.slice/cpuacct.usage");
        arquivo = fopen(caminho, "r");
        if (arquivo) {
            unsigned long cpu_usage;
            if (fscanf(arquivo, "%lu", &cpu_usage) == 1) {
                snprintf(metricas->cpu_usada, sizeof(metricas->cpu_usada), "%luns", cpu_usage);
            }
            fclose(arquivo);
        }
    }

    if (version == 2) {
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/memory.current");
    } else {
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/memory/user.slice/memory.usage_in_bytes");
    }
    
    arquivo = fopen(caminho, "r");
    if (arquivo) {
        unsigned long mem_usage;
        if (fscanf(arquivo, "%lu", &mem_usage) == 1) {
            snprintf(metricas->memoria_usada, sizeof(metricas->memoria_usada), "%luMB", mem_usage / (1024 * 1024));
        }
        fclose(arquivo);
    }

    return 0;
}

/*****************************/
/*       VERSÃO CGROUP       */
/*****************************/
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

/******************************/
/*          CGROUP V1         */
/******************************/
int criar_cgroup(const char* nome_cgroup) {
    if (nome_cgroup == NULL || strlen(nome_cgroup) == 0) {
        errno = EINVAL;
        return -1;
    }

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
        errno = ENOSYS;
        return -1;
    }
}

int mover_cgroup(const char* nome_cgroup, pid_t pid) {
    if (nome_cgroup == NULL || strlen(nome_cgroup) == 0 || pid <= 0) {
        errno = EINVAL;
        return -1;
    }

    int version = detectar_cgroup_version();
    char caminho[512];
    char pid_str[32];

    snprintf(pid_str, sizeof(pid_str), "%d", pid);

    if (version == 2) {
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/%s/cgroup.procs", nome_cgroup);
        if (escrever_arquivo(caminho, "%s", pid_str) != 0) {
            perror("Erro ao mover processo para cgroup v2");
            return -1;
        }
    } else if (version == 1) {
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/cpu/%s/cgroup.procs", nome_cgroup);
        if (escrever_arquivo(caminho, "%s", pid_str) != 0) {
            perror("Erro ao mover processo para cgroup CPU v1");
            return -1;
        }

        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/memory/%s/cgroup.procs", nome_cgroup);
        if (escrever_arquivo(caminho, "%s", pid_str) != 0) {
            perror("Erro ao mover processo para cgroup Memory v1");
            return -1;
        }
    } else {
        printf("cgroups não disponíveis\n");
        errno = ENOSYS;
        return -1;
    }

    printf("Processo %d movido para cgroup '%s'\n", pid, nome_cgroup);
    return 0;
}

/******************************/
/*   CGROUP V1 - LIMITAÇÕES   */
/******************************/
int limite_cpu_v1(const char* nome_cgroup, double cpu_cores) {
    if (nome_cgroup == NULL || cpu_cores < 0) {
        errno = EINVAL;
        return -1;
    }

    char caminho[512];
    int quota = (int)(cpu_cores * 100000.0);

    snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/cpu/%s/cpu.cfs_quota_us", nome_cgroup);
    if (escrever_arquivo(caminho, "%d", quota) != 0) {
        perror("Erro ao definir limite de CPU v1");
        return -1;
    }

    snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/cpu/%s/cpu.cfs_period_us", nome_cgroup);
    if (escrever_arquivo(caminho, "100000") != 0) {
        printf("Aviso: não foi possível configurar período de CPU\n");
    }

    printf("Limite de CPU v1 definido para %.2f cores no cgroup '%s'\n", cpu_cores, nome_cgroup);
    return 0;
}

int limite_memoria_v1(const char* nome_cgroup, unsigned long memoria_mb) {
    if (nome_cgroup == NULL) {
        errno = EINVAL;
        return -1;
    }

    char caminho[512];
    unsigned long memoria_bytes = memoria_mb * 1024 * 1024;

    snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/memory/%s/memory.limit_in_bytes", nome_cgroup);
    if (escrever_arquivo(caminho, "%lu", memoria_bytes) != 0) {
        perror("Erro ao definir limite de memória v1");
        return -1;
    }

    printf("Limite de memória v1 definido para %lu MB no cgroup '%s'\n", memoria_mb, nome_cgroup);
    return 0;
}

/******************************/
/*          CGROUP V2         */
/******************************/
int criar_cgroup_v2(const char* nome_cgroup) {
    if (nome_cgroup == NULL || strlen(nome_cgroup) == 0) {
        errno = EINVAL;
        return -1;
    }

    char caminho[512];
    int version = detectar_cgroup_version();
    
    if (version != 2) {
        printf("Sistema não usa cgroup v2\n");
        errno = ENOSYS;
        return -1;
    }
    
    snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/%s", nome_cgroup);
    
    if (mkdir(caminho, 0755) != 0 && errno != EEXIST) {
        perror("Erro ao criar cgroup v2");
        return -1;
    }
    
    char controladores[512];
    snprintf(controladores, sizeof(controladores), "%s/cgroup.subtree_control", caminho);
    
    if (escrever_arquivo(controladores, "+cpu +memory +io") != 0) {
        printf("Aviso: não foi possível habilitar todos os controladores\n");
    }
    
    printf("Cgroup v2 '%s' criado com sucesso\n", nome_cgroup);
    return 0;
}

/******************************/
/*   CGROUP V2 - LIMITAÇÕES   */
/******************************/
int limite_cpu_v2(const char* nome_cgroup, double cpu_cores) {
    if (nome_cgroup == NULL || cpu_cores < 0) {
        errno = EINVAL;
        return -1;
    }

    char caminho[512];
    int version = detectar_cgroup_version();
    
    if (version != 2) {
        printf("Sistema não usa cgroup v2\n");
        errno = ENOSYS;
        return -1;
    }

    unsigned int peso = (unsigned int)(cpu_cores * 100.0);
    if (peso < 1) peso = 1;
    if (peso > 10000) peso = 10000;
    
    snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/%s/cpu.weight", nome_cgroup);
    if (escrever_arquivo(caminho, "%u", peso) != 0) {
        perror("Erro ao definir peso de CPU v2");
        return -1;
    }
    
    if (cpu_cores < 1.0) {
        unsigned int max = (unsigned int)(cpu_cores * 100000.0);
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/%s/cpu.max", nome_cgroup);
        if (escrever_arquivo(caminho, "%u 100000", max) != 0) {
            printf("Aviso: não foi possível configurar máximo de CPU\n");
        }
    }
    
    printf("Limite de CPU v2 definido para %.2f cores (peso: %u) no cgroup '%s'\n", 
           cpu_cores, peso, nome_cgroup);
    return 0;
}

int limite_memoria_v2(const char* nome_cgroup, unsigned long memoria_mb) {
    if (nome_cgroup == NULL) {
        errno = EINVAL;
        return -1;
    }

    char caminho[512];
    int version = detectar_cgroup_version();
    
    if (version != 2) {
        printf("Sistema não usa cgroup v2\n");
        errno = ENOSYS;
        return -1;
    }
    
    unsigned long memoria_bytes = memoria_mb * 1024 * 1024;
    
    snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/%s/memory.max", nome_cgroup);
    if (escrever_arquivo(caminho, "%lu", memoria_bytes) != 0) {
        perror("Erro ao definir limite de memória v2");
        return -1;
    }
    
    snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/%s/memory.swap.max", nome_cgroup);
    if (escrever_arquivo(caminho, "%lu", memoria_bytes) != 0) {
        printf("Aviso: não foi possível configurar limite de swap\n");
    }
    
    printf("Limite de memória v2 definido para %lu MB no cgroup '%s'\n", 
           memoria_mb, nome_cgroup);
    return 0;
}

/******************************/
/*        BLOQUEIO I/O        */
/******************************/
int get_metricas_blkio(const char* nome_cgroup, unsigned long* bytes_lidos, unsigned long* bytes_escritos) {
    if (nome_cgroup == NULL || bytes_lidos == NULL || bytes_escritos == NULL) {
        errno = EINVAL;
        return -1;
    }

    *bytes_lidos = 0;
    *bytes_escritos = 0;

    int version = detectar_cgroup_version();
    char caminho[512];
    FILE *arquivo;

    if (version == 2) {
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/%s/io.stat", nome_cgroup);
        arquivo = fopen(caminho, "r");
        if (arquivo != NULL) {
            char linha[256];
            while (fgets(linha, sizeof(linha), arquivo)) {
                unsigned long rbytes, wbytes;
                if (sscanf(linha, "%*s rbytes=%lu wbytes=%lu", &rbytes, &wbytes) == 2) {
                    *bytes_lidos = rbytes;
                    *bytes_escritos = wbytes;
                    break;
                }
            }
            fclose(arquivo);
        }
    } else if (version == 1) {
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/blkio/%s/blkio.io_service_bytes", nome_cgroup);
        arquivo = fopen(caminho, "r");
        if (arquivo != NULL) {
            char linha[256];
            while (fgets(linha, sizeof(linha), arquivo)) {
                if (strstr(linha, "Read")) {
                    sscanf(linha, "%*s %*s %lu", bytes_lidos);
                } else if (strstr(linha, "Write")) {
                    sscanf(linha, "%*s %*s %lu", bytes_escritos);
                }
            }
            fclose(arquivo);
        }
    }
    
    return 0;
}

int limite_blkio_v1(const char* nome_cgroup, unsigned long ler_blkIo, unsigned long escrever_blkIo) {
    if (nome_cgroup == NULL) {
        errno = EINVAL;
        return -1;
    }

    char caminho[512];

    snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/blkio/%s", nome_cgroup);
    if (mkdir(caminho, 0755) != 0 && errno != EEXIST) {
        perror("Erro ao criar cgroup BlkIO");
        return -1;
    }

    if (ler_blkIo > 0) {
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/blkio/%s/blkio.throttle.read_bps_device", nome_cgroup);
        if (escrever_arquivo(caminho, "8:0 %lu", ler_blkIo) != 0) {
            printf("Aviso: não foi possível configurar limite de leitura BlkIO\n");
        }
    }

    if (escrever_blkIo > 0) {
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/blkio/%s/blkio.throttle.write_bps_device", nome_cgroup);
        if (escrever_arquivo(caminho, "8:0 %lu", escrever_blkIo) != 0) {
            printf("Aviso: não foi possível configurar limite de escrita BlkIO\n");
        }
    }

    printf("Limites BlkIO v1 definidos - Leitura: %lu B/s, Escrita: %lu B/s no cgroup '%s'\n", 
           ler_blkIo, escrever_blkIo, nome_cgroup);
    return 0;
}

/******************************/
/*     LIMITAÇÕES GLOBAIS     */
/******************************/
int limite_cpu(const char* nome_cgroup, double cpu_cores) {
    if (nome_cgroup == NULL || cpu_cores < 0) {
        errno = EINVAL;
        return -1;
    }

    int version = detectar_cgroup_version();
    
    if (version == 2) {
        return limite_cpu_v2(nome_cgroup, cpu_cores);
    } else if (version == 1) {
        return limite_cpu_v1(nome_cgroup, cpu_cores);
    } else {
        printf("cgroups não disponíveis\n");
        errno = ENOSYS;
        return -1;
    }
}

int limite_memoria(const char* nome_cgroup, unsigned long memoria_mb) {
    if (nome_cgroup == NULL) {
        errno = EINVAL;
        return -1;
    }

    int version = detectar_cgroup_version();
    
    if (version == 2) {
        return limite_memoria_v2(nome_cgroup, memoria_mb);
    } else if (version == 1) {
        return limite_memoria_v1(nome_cgroup, memoria_mb);
    } else {
        printf("cgroups não disponíveis\n");
        errno = ENOSYS;
        return -1;
    }
}

/******************************/
/*     ARQUIVO DE ESCRITA     */
/******************************/
static int escrever_arquivo(const char* caminho, const char* formato, ...) {
    if (caminho == NULL || formato == NULL) {
        errno = EINVAL;
        return -1;
    }

    FILE *arquivo = fopen(caminho, "w");
    if (arquivo == NULL) {
        perror("Erro ao abrir arquivo para escrita");
        return -1;
    }
    
    va_list args;
    va_start(args, formato);
    int resultado = vfprintf(arquivo, formato, args);
    va_end(args);
    
    fclose(arquivo);
    
    if (resultado < 0) {
        perror("Erro ao escrever no arquivo");
        return -1;
    }
    
    return 0;
}

int limite_io(const char* nome_cgroup, unsigned long bytes_leitura_por_segundo, unsigned long bytes_escrita_por_segundo) {
    if (nome_cgroup == NULL) {
        errno = EINVAL;
        return -1;
    }

    char caminho[512];
    int version = detectar_cgroup_version();
    
    if (version == 2) {
        if (bytes_leitura_por_segundo > 0 || bytes_escrita_por_segundo > 0) {
            snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/%s/io.max", nome_cgroup);
            FILE *arquivo = fopen(caminho, "w");
            if (arquivo == NULL) {
                perror("Erro ao abrir arquivo de limite de I/O v2");
                return -1;
            }
            
            int escrito = 0;
            if (bytes_leitura_por_segundo > 0 && bytes_escrita_por_segundo > 0) {
                escrito = fprintf(arquivo, "8:0 rbps=%lu wbps=%lu", bytes_leitura_por_segundo, bytes_escrita_por_segundo);
            } else if (bytes_leitura_por_segundo > 0) {
                escrito = fprintf(arquivo, "8:0 rbps=%lu", bytes_leitura_por_segundo);
            } else if (bytes_escrita_por_segundo > 0) {
                escrito = fprintf(arquivo, "8:0 wbps=%lu", bytes_escrita_por_segundo);
            }
            
            fclose(arquivo);
            
            if (escrito < 0) {
                perror("Erro ao escrever limites de I/O v2");
                return -1;
            }
        }
    } else if (version == 1) {
        if (bytes_leitura_por_segundo > 0) {
            snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/blkio/%s/blkio.throttle.read_bps_device", nome_cgroup);
            if (escrever_arquivo(caminho, "8:0 %lu", bytes_leitura_por_segundo) != 0) {
                printf("Aviso: não foi possível configurar limite de leitura I/O\n");
            }
        }
        
        if (bytes_escrita_por_segundo > 0) {
            snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/blkio/%s/blkio.throttle.write_bps_device", nome_cgroup);
            if (escrever_arquivo(caminho, "8:0 %lu", bytes_escrita_por_segundo) != 0) {
                printf("Aviso: não foi possível configurar limite de escrita I/O\n");
            }
        }
    } else {
        printf("cgroup não disponível para limitação de I/O\n");
        errno = ENOSYS;
        return -1;
    }
    
    printf("Limites de I/O definidos - Leitura: %lu B/s, Escrita: %lu B/s no cgroup '%s'\n",
           bytes_leitura_por_segundo, bytes_escrita_por_segundo, nome_cgroup);
    return 0;
}

/******************************/
/*       REMOVER CGROUP       */
/******************************/
int remover_cgroup(const char* nome_cgroup) {
    if (nome_cgroup == NULL) {
        errno = EINVAL;
        return -1;
    }

    char caminho[512];
    int sucesso = 0;
    int version = detectar_cgroup_version();

    if (version == 2) {
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/%s", nome_cgroup);
        if (rmdir(caminho) == 0) {
            printf("Cgroup v2 '%s' removido\n", nome_cgroup);
            sucesso = 1;
        } else {
            perror("Erro ao remover cgroup v2");
        }
    } else if (version == 1) {
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/cpu/%s", nome_cgroup);
        if (rmdir(caminho) == 0) {
            printf("Cgroup CPU '%s' removido\n", nome_cgroup);
            sucesso++;
        } else {
            perror("Erro ao remover cgroup CPU");
        }

        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/memory/%s", nome_cgroup);
        if (rmdir(caminho) == 0) {
            printf("Cgroup Memory '%s' removido\n", nome_cgroup);
            sucesso++;
        } else {
            perror("Erro ao remover cgroup Memory");
        }
    } else {
        printf("cgroups não disponíveis\n");
        errno = ENOSYS;
        return -1;
    }

    if ((version == 1 && sucesso == 2) || (version == 2 && sucesso == 1)) {
        printf("Cgroup removido com sucesso\n");
        return 0;
    } else {
        printf("Apenas %d componentes do cgroup foram removidos\n", sucesso);
        return -1;
    }
}

/*******************************/
/*        LISTAR CGROUP        */
/*******************************/
void listar_cgroups() {
    DIR *dir;
    struct dirent *entrada_diretorio;
    int version = detectar_cgroup_version();

    printf("Versão do cgroup detectada: %d\n", version);

    if (version == 2) {
        printf("\nCgroups v2:\n");
        dir = opendir("/sys/fs/cgroup");
        if (dir) {
            int count = 0;
            while ((entrada_diretorio = readdir(dir)) != NULL) {
                if (entrada_diretorio->d_type == DT_DIR && 
                    entrada_diretorio->d_name[0] != '.' &&
                    strcmp(entrada_diretorio->d_name, "..") != 0) {
                    printf("  %s\n", entrada_diretorio->d_name);
                    count++;
                }
            }
            closedir(dir);
            printf("Total: %d cgroups\n", count);
        } else {
            perror("Erro ao abrir diretório de cgroups v2");
        }
    } else if (version == 1) {
        int count_cpu = 0, count_memory = 0;

        printf("\nCgroups de CPU:\n");
        dir = opendir("/sys/fs/cgroup/cpu");
        if (dir) {
            while ((entrada_diretorio = readdir(dir)) != NULL) {
                if (entrada_diretorio->d_type == DT_DIR && 
                    entrada_diretorio->d_name[0] != '.' &&
                    strcmp(entrada_diretorio->d_name, "..") != 0) {
                    printf("  %s\n", entrada_diretorio->d_name);
                    count_cpu++;
                }
            }
            closedir(dir);
            printf("Total CPU: %d cgroups\n", count_cpu);
        } else {
            perror("Erro ao abrir diretório de cgroups CPU");
        }

        printf("\nCgroups de Memória:\n");
        dir = opendir("/sys/fs/cgroup/memory");
        if (dir) {
            while ((entrada_diretorio = readdir(dir)) != NULL) {
                if (entrada_diretorio->d_type == DT_DIR && 
                    entrada_diretorio->d_name[0] != '.' &&
                    strcmp(entrada_diretorio->d_name, "..") != 0) {
                    printf("  %s\n", entrada_diretorio->d_name);
                    count_memory++;
                }
            }
            closedir(dir);
            printf("Total Memory: %d cgroups\n", count_memory);
        } else {
            perror("Erro ao abrir diretório de cgroups Memory");
        }
    } else {
        printf("cgroups não disponíveis\n");
    }
}

/******************************/
/*        EXPERIMENTOS        */
/******************************/
void experimento_throttling_cpu() {
    printf("\n=== EXPERIMENTO 3: THROTTLING DE CPU ===\n");
    
    if (geteuid() != 0) {
        printf("AVISO: Este experimento requer privilégios de root.\n");
        printf("Execute com: sudo ./resource-monitor -e\n");
        return;
    }

    const char* cgroup_name = "teste_cpu_exp";
    double limites[] = {0.25, 0.5, 1.0, 2.0};
    int num_limites = sizeof(limites) / sizeof(limites[0]);
    
    if (criar_cgroup(cgroup_name) != 0) {
        printf("Erro ao criar cgroup para experimento\n");
        return;
    }

    if (mover_cgroup(cgroup_name, getpid()) != 0) {
        printf("Erro ao mover processo\n");
        return;
    }
    
    printf("Limite\tCPU%% Medido\tDesvio\t\tThroughput\n");
    printf("------\t----------\t------\t\t----------\n");
    
    for (int i = 0; i < num_limites; i++) {
        if (limite_cpu(cgroup_name, limites[i]) != 0) {
            printf("Erro ao aplicar limite de CPU\n");
            continue;
        }
        
        sleep(1);
        
        clock_t inicio = clock();
        unsigned long iteracoes = 0;
        metricas_cpu_t cpu;
        double cpu_total = 0;
        int medicoes = 0;

        while ((clock() - inicio) < (CLOCKS_PER_SEC * 3)) {
            volatile double resultado = 0;
            for (int j = 0; j < 10000; j++) {
                resultado += sqrt(j) * sin(j * 0.01);
            }
            (void)resultado;
            iteracoes++;
            
            if (iteracoes % 1000 == 0 && get_metricas_cpu(getpid(), &cpu) == 0) {
                cpu_total += cpu.porcentagem_cpu;
                medicoes++;
            }
        }
        
        printf("%.2f\t%.1f%%\t\t%.1f%%\t\t%.0f iter/s\n", limites[i], cpu_medio, desvio, throughput);
    }
    
    mover_cgroup("", getpid());
    remover_cgroup(cgroup_name);
}
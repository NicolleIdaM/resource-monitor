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

/*
 * Função auxiliar para escrever em arquivos do cgroup com formatação.
 * Similar ao fprintf, mas abre/fecha o arquivo automaticamente.
 */
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

/*
 * Detecta qual versão do cgroup está em uso no sistema.
 * Retorna 1 para cgroup v1, 2 para v2, ou 0 se não suportado.
 */
int detectar_cgroup_version(void) {
    struct stat st;
    
    /* cgroup v2: verifica se existe cgroup.controllers */
    if (stat("/sys/fs/cgroup/cgroup.controllers", &st) == 0) {
        return 2;
    }
    
    /* cgroup v1: verifica se existe diretório cpu */
    if (stat("/sys/fs/cgroup/cpu", &st) == 0) {
        return 1;
    }
    
    return 0;
}

/*
 * Cria um novo cgroup com o nome especificado.
 * Suporta ambas as versões do cgroup automaticamente.
 */
int criar_cgroup(const char* nome_cgroup) {
    if (nome_cgroup == NULL || strlen(nome_cgroup) == 0) {
        errno = EINVAL;
        return -1;
    }

    /* Cgroups geralmente requerem privilégios de root */
    if (geteuid() != 0) {
        printf("Aviso: Cgroups requerem privilégios de root. Execute com 'sudo'\n");
        errno = EACCES;
        return -1;
    }

    int version = detectar_cgroup_version();
    
    if (version == 2) {
        return criar_cgroup_v2(nome_cgroup);
    } else if (version == 1) {
        char caminho[512];

        /* Cria diretório do cgroup para CPU (v1) */
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/cpu/%s", nome_cgroup);
        if (mkdir(caminho, 0755) != 0 && errno != EEXIST) {
            perror("Erro ao criar Cgroup CPU");
            return -1;
        }

        /* Cria diretório do cgroup para Memory (v1) */
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

/*
 * Cria um cgroup específico para a versão 2.
 * Habilita controladores de CPU, memória e I/O automaticamente.
 */
int criar_cgroup_v2(const char* nome_cgroup) {
    if (nome_cgroup == NULL || strlen(nome_cgroup) == 0) {
        errno = EINVAL;
        return -1;
    }

    char caminho[512];
    char controladores[512];
    int version = detectar_cgroup_version();
    
    if (version != 2) {
        printf("Sistema não usa cgroup v2\n");
        errno = ENOSYS;
        return -1;
    }
    
    /* Cria diretório do cgroup v2 */
    size_t escrito = snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/%s", nome_cgroup);
    if (escrito >= sizeof(caminho)) {
        printf("Erro: nome do cgroup muito longo\n");
        errno = ENAMETOOLONG;
        return -1;
    }
    
    if (mkdir(caminho, 0755) != 0 && errno != EEXIST) {
        perror("Erro ao criar cgroup v2");
        return -1;
    }
    
    /* Habilita controladores no cgroup (CPU, memória, I/O) */
    escrito = snprintf(controladores, sizeof(controladores), "%s/cgroup.subtree_control", caminho);
    if (escrito >= sizeof(controladores)) {
        printf("Aviso: caminho muito longo para controladores\n");
        return 0;
    }
    
    if (escrever_arquivo(controladores, "+cpu +memory +io") != 0) {
        printf("Aviso: não foi possível habilitar todos os controladores\n");
    }
    
    printf("Cgroup v2 '%s' criado com sucesso\n", nome_cgroup);
    return 0;
}

/*
 * Move um processo para um cgroup específico.
 * Suporta ambas as versões do cgroup automaticamente.
 */
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
        /* Move processo para cgroup v2 */
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/%s/cgroup.procs", nome_cgroup);
        if (escrever_arquivo(caminho, "%s", pid_str) != 0) {
            perror("Erro ao mover processo para cgroup v2");
            return -1;
        }
    } else if (version == 1) {
        /* Move processo para cgroup CPU v1 */
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/cpu/%s/cgroup.procs", nome_cgroup);
        if (escrever_arquivo(caminho, "%s", pid_str) != 0) {
            perror("Erro ao mover processo para cgroup CPU v1");
            return -1;
        }

        /* Move processo para cgroup Memory v1 */
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

/*
 * Define limite de CPU para cgroup v1 usando CFS (Completely Fair Scheduler).
 * cpu_cores: número de cores de CPU (ex: 0.5 = meio core, 2.0 = dois cores)
 */
int limite_cpu_v1(const char* nome_cgroup, double cpu_cores) {
    if (nome_cgroup == NULL || cpu_cores < 0) {
        errno = EINVAL;
        return -1;
    }

    char caminho[512];
    /* Converte cores para microseconds (100ms = 100000µs) */
    int quota = (int)(cpu_cores * 100000.0);

    /* Define quota de CPU (tempo máximo que pode usar em cada período) */
    snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/cpu/%s/cpu.cfs_quota_us", nome_cgroup);
    if (escrever_arquivo(caminho, "%d", quota) != 0) {
        perror("Erro ao definir limite de CPU v1");
        return -1;
    }

    /* Define período padrão de 100ms */
    snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/cpu/%s/cpu.cfs_period_us", nome_cgroup);
    if (escrever_arquivo(caminho, "100000") != 0) {
        printf("Aviso: não foi possível configurar período de CPU\n");
    }

    printf("Limite de CPU v1 definido para %.2f cores no cgroup '%s'\n", cpu_cores, nome_cgroup);
    return 0;
}

/*
 * Define limite de CPU para cgroup v2 usando peso e limites máximos.
 * cpu_cores: número de cores de CPU (usa peso para alocação proporcional)
 */
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

    /* Calcula peso (1-10000), onde 100 = 1 core padrão */
    unsigned int peso = (unsigned int)(cpu_cores * 100.0);
    if (peso < 1) peso = 1;
    if (peso > 10000) peso = 10000;
    
    /* Define peso de CPU (alocação proporcional) */
    snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/%s/cpu.weight", nome_cgroup);
    if (escrever_arquivo(caminho, "%u", peso) != 0) {
        perror("Erro ao definir peso de CPU v2");
        return -1;
    }
    
    /* Para limites rígidos (< 1 core), define máximo absoluto */
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

/*
 * Define limite de CPU (versão genérica que detecta automaticamente a versão)
 */
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

/*
 * Define limite de memória para cgroup v1
 * memoria_mb: limite máximo de memória em megabytes
 */
int limite_memoria_v1(const char* nome_cgroup, unsigned long memoria_mb) {
    if (nome_cgroup == NULL) {
        errno = EINVAL;
        return -1;
    }

    char caminho[512];
    unsigned long memoria_bytes = memoria_mb * 1024 * 1024;

    /* Define limite máximo de memória em bytes */
    snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/memory/%s/memory.limit_in_bytes", nome_cgroup);
    if (escrever_arquivo(caminho, "%lu", memoria_bytes) != 0) {
        perror("Erro ao definir limite de memória v1");
        return -1;
    }

    printf("Limite de memória v1 definido para %lu MB no cgroup '%s'\n", memoria_mb, nome_cgroup);
    return 0;
}

/*
 * Define limite de memória para cgroup v2
 * memoria_mb: limite máximo de memória em megabytes
 */
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
    
    /* Define limite máximo de memória */
    snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/%s/memory.max", nome_cgroup);
    if (escrever_arquivo(caminho, "%lu", memoria_bytes) != 0) {
        perror("Erro ao definir limite de memória v2");
        return -1;
    }
    
    /* Define limite de swap (igual ao de memória para evitar uso excessivo) */
    snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/%s/memory.swap.max", nome_cgroup);
    if (escrever_arquivo(caminho, "%lu", memoria_bytes) != 0) {
        printf("Aviso: não foi possível configurar limite de swap\n");
    }
    
    printf("Limite de memória v2 definido para %lu MB no cgroup '%s'\n", 
           memoria_mb, nome_cgroup);
    return 0;
}

/*
 * Define limite de memória (versão genérica)
 */
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

/*
 * Define limites de I/O (leitura e escrita) para o cgroup
 * bytes_leitura_por_segundo: throughput máximo de leitura
 * bytes_escrita_por_segundo: throughput máximo de escrita
 */
int limite_io(const char* nome_cgroup, unsigned long bytes_leitura_por_segundo, unsigned long bytes_escrita_por_segundo) {
    if (nome_cgroup == NULL) {
        errno = EINVAL;
        return -1;
    }

    char caminho[512];
    int version = detectar_cgroup_version();
    
    if (version == 2) {
        /* cgroup v2: usa io.max para limites absolutos */
        if (bytes_leitura_por_segundo > 0 || bytes_escrita_por_segundo > 0) {
            printf("Configurando limites de I/O para cgroup v2...\n");
            
            snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/%s/io.max", nome_cgroup);
            FILE *arquivo = fopen(caminho, "w");
            if (arquivo != NULL) {
                int escrito = 0;
                /* 8:0 representa o dispositivo principal (sda) */
                if (bytes_leitura_por_segundo > 0 && bytes_escrita_por_segundo > 0) {
                    escrito = fprintf(arquivo, "8:0 rbps=%lu wbps=%lu", 
                                     bytes_leitura_por_segundo, bytes_escrita_por_segundo);
                } else if (bytes_leitura_por_segundo > 0) {
                    escrito = fprintf(arquivo, "8:0 rbps=%lu", bytes_leitura_por_segundo);
                } else if (bytes_escrita_por_segundo > 0) {
                    escrito = fprintf(arquivo, "8:0 wbps=%lu", bytes_escrita_por_segundo);
                }
                fclose(arquivo);
                
                if (escrito > 0) {
                    printf("Limites de I/O v2 configurados via io.max\n");
                    return 0;
                }
            }
            
            /* Método alternativo se io.max falhar */
            printf("Tentando método alternativo para I/O...\n");
            snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/%s/io.weight", nome_cgroup);
            if (escrever_arquivo(caminho, "default 100") == 0) {
                printf("Peso de I/O configurado\n");
            }
            
            printf("Aviso: Limitação de I/O completa pode requerer configuração adicional do sistema\n");
            printf("Considere verificar: /sys/fs/cgroup/io.max e permissões do sistema\n");
        }
    } else if (version == 1) {
        /* cgroup v1: usa blkio.throttle para limites */
        if (bytes_leitura_por_segundo > 0) {
            snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/blkio/%s/blkio.throttle.read_bps_device", nome_cgroup);
            if (escrever_arquivo(caminho, "8:0 %lu", bytes_leitura_por_segundo) != 0) {
                printf("Aviso: não foi possível configurar limite de leitura I/O\n");
            } else {
                printf("Limite de leitura I/O configurado: %lu B/s\n", bytes_leitura_por_segundo);
            }
        }
        
        if (bytes_escrita_por_segundo > 0) {
            snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/blkio/%s/blkio.throttle.write_bps_device", nome_cgroup);
            if (escrever_arquivo(caminho, "8:0 %lu", bytes_escrita_por_segundo) != 0) {
                printf("Aviso: não foi possível configurar limite de escrita I/O\n");
            } else {
                printf("Limite de escrita I/O configurado: %lu B/s\n", bytes_escrita_por_segundo);
            }
        }
        
        if (bytes_leitura_por_segundo > 0 || bytes_escrita_por_segundo > 0) {
            return 0;
        }
    } else {
        printf("cgroup não disponível para limitação de I/O\n");
        errno = ENOSYS;
        return -1;
    }
    
    printf("Limites de I/O - Leitura: %lu B/s, Escrita: %lu B/s no cgroup '%s'\n",
           bytes_leitura_por_segundo, bytes_escrita_por_segundo, nome_cgroup);
    return 0;
}

/*
 * Remove um cgroup e move todos os processos de volta para o root
 */
int remover_cgroup(const char* nome_cgroup) {
    if (nome_cgroup == NULL) {
        errno = EINVAL;
        return -1;
    }

    char caminho[512];
    int sucesso = 0;
    int version = detectar_cgroup_version();

    if (version == 2) {
        /* cgroup v2: move processos para root antes de remover */
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/%s/cgroup.procs", nome_cgroup);
        FILE *arquivo = fopen(caminho, "r");
        if (arquivo) {
            char pid_str[32];
            while (fgets(pid_str, sizeof(pid_str), arquivo)) {
                pid_t pid = atoi(pid_str);
                if (pid > 0) {
                    mover_para_root(pid);
                }
            }
            fclose(arquivo);
        }

        usleep(100000); /* Espera processos se moverem */

        /* Remove diretório do cgroup */
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/%s", nome_cgroup);
        if (rmdir(caminho) == 0) {
            printf("Cgroup v2 '%s' removido\n", nome_cgroup);
            sucesso = 1;
        } else {
            if (errno == EBUSY) {
                printf("Aviso: Cgroup '%s' ainda está em uso. Tente novamente.\n", nome_cgroup);
            } else {
                perror("Erro ao remover cgroup v2");
            }
        }
    } else if (version == 1) {
        /* cgroup v1: remove de todos os controladores */
        char* controladores[] = {"cpu", "memory", "blkio"};
        int num_controladores = sizeof(controladores) / sizeof(controladores[0]);
        
        /* Move processos para root em todos os controladores */
        for (int i = 0; i < num_controladores; i++) {
            snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/%s/%s/cgroup.procs", 
                     controladores[i], nome_cgroup);
            FILE *arquivo = fopen(caminho, "r");
            if (arquivo) {
                char pid_str[32];
                while (fgets(pid_str, sizeof(pid_str), arquivo)) {
                    pid_t pid = atoi(pid_str);
                    if (pid > 0) {
                        mover_para_root(pid);
                    }
                }
                fclose(arquivo);
            }
            
            usleep(50000); /* Pequena espera entre controladores */
        }

        /* Remove diretórios de todos os controladores */
        for (int i = 0; i < num_controladores; i++) {
            snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/%s/%s", 
                     controladores[i], nome_cgroup);
            if (rmdir(caminho) == 0) {
                printf("Cgroup %s '%s' removido\n", controladores[i], nome_cgroup);
                sucesso++;
            } else {
                if (errno != ENOENT) {
                    perror("Erro ao remover cgroup");
                }
            }
        }
    } else {
        printf("cgroups não disponíveis\n");
        errno = ENOSYS;
        return -1;
    }

    if ((version == 1 && sucesso >= 1) || (version == 2 && sucesso == 1)) {
        printf("Cgroup removido com sucesso\n");
        return 0;
    } else {
        printf("Apenas %d componentes do cgroup foram removidos\n", sucesso);
        return -1;
    }
}

/*
 * Lista todos os cgroups existentes no sistema
 */
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
                /* Ignora diretórios especiais (. e ..) */
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

        /* Lista cgroups de CPU */
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

        /* Lista cgroups de Memória */
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

/*
 * Obtém métricas de uso do cgroup para um processo específico
 */
int get_metricas_cgroup(pid_t pid, metricas_cgroup_t* metricas) {
    if (metricas == NULL || pid <= 0) {
        errno = EINVAL;
        return -1;
    }

    /* Valores padrão caso não consiga obter métricas */
    strncpy(metricas->cpu_usada, "N/A", sizeof(metricas->cpu_usada) - 1);
    strncpy(metricas->memoria_usada, "N/A", sizeof(metricas->memoria_usada) - 1);
    strncpy(metricas->memoria_limite, "N/A", sizeof(metricas->memoria_limite) - 1);
    
    int version = detectar_cgroup_version();
    
    if (version == 2) {
        strncpy(metricas->cgroup_version, "v2", sizeof(metricas->cgroup_version) - 1);
        return get_metricas_cgroup_v2(pid, metricas);
    } else if (version == 1) {
        strncpy(metricas->cgroup_version, "v1", sizeof(metricas->cgroup_version) - 1);
        return get_metricas_cgroup_v1(pid, metricas);
    } else {
        strncpy(metricas->cgroup_version, "none", sizeof(metricas->cgroup_version) - 1);
        return 0;
    }
}

/*
 * Obtém métricas de cgroup para a versão 1
 */
int get_metricas_cgroup_v1(pid_t pid, metricas_cgroup_t* metricas) {
    char caminho[512];
    char linha[256];
    FILE *arquivo;
    
    /* Lê arquivo cgroup do processo para descobrir seu cgroup */
    snprintf(caminho, sizeof(caminho), "/proc/%d/cgroup", pid);
    arquivo = fopen(caminho, "r");
    if (arquivo == NULL) {
        return 0;
    }
    
    char cgroup_path[128] = "";
    int cgroup_encontrado = 0;
    
    /* Parse do arquivo cgroup para encontrar o path do cgroup */
    while (fgets(linha, sizeof(linha), arquivo)) {
        if (strstr(linha, "cpu,") || strstr(linha, "cpu:")) {
            char *path = strchr(linha, ':');
            if (path && (path = strchr(path + 1, ':'))) {
                size_t len = strlen(path + 1);
                if (len > 0) {
                    size_t copy_len = len;
                    if (path[1] == '\n') copy_len = 0;
                    else if (copy_len > sizeof(cgroup_path) - 1) 
                        copy_len = sizeof(cgroup_path) - 1;
                    
                    if (copy_len > 0) {
                        strncpy(cgroup_path, path + 1, copy_len);
                        cgroup_path[copy_len] = '\0';
                        char *newline = strchr(cgroup_path, '\n');
                        if (newline) *newline = '\0';
                        cgroup_encontrado = 1;
                    }
                }
            }
            break;
        }
    }
    fclose(arquivo);
    
    /* Se não encontrou cgroup específico, usa root */
    if (!cgroup_encontrado || strlen(cgroup_path) > 100) {
        strcpy(cgroup_path, "");
    }
    
    /* Obtém uso de CPU */
    if (strlen(cgroup_path) == 0) {
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/cpu/cpuacct.usage");
    } else {
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/cpu%s/cpuacct.usage", cgroup_path);
    }
    
    arquivo = fopen(caminho, "r");
    if (arquivo != NULL) {
        unsigned long long cpu_usage;
        if (fscanf(arquivo, "%llu", &cpu_usage) == 1) {
            /* Converte nanosegundos para segundos */
            double cpu_seconds = cpu_usage / 1e9;
            snprintf(metricas->cpu_usada, sizeof(metricas->cpu_usada), "%.2fs", cpu_seconds);
        }
        fclose(arquivo);
    }
    
    /* Obtém uso de memória */
    if (strlen(cgroup_path) == 0) {
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/memory/memory.usage_in_bytes");
    } else {
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/memory%s/memory.usage_in_bytes", cgroup_path);
    }
    
    arquivo = fopen(caminho, "r");
    if (arquivo != NULL) {
        unsigned long memory_usage;
        if (fscanf(arquivo, "%lu", &memory_usage) == 1) {
            formatar_memoria(metricas->memoria_usada, sizeof(metricas->memoria_usada), memory_usage);
        }
        fclose(arquivo);
    }
    
    /* Obtém limite de memória */
    if (strlen(cgroup_path) == 0) {
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/memory/memory.limit_in_bytes");
    } else {
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/memory%s/memory.limit_in_bytes", cgroup_path);
    }
    
    arquivo = fopen(caminho, "r");
    if (arquivo != NULL) {
        unsigned long memory_limit;
        if (fscanf(arquivo, "%lu", &memory_limit) == 1) {
            if (memory_limit == 0x7FFFFFFFFFFFFFFF) {
                /* Valor especial que representa limite ilimitado */
                strncpy(metricas->memoria_limite, "ilimitado", sizeof(metricas->memoria_limite) - 1);
            } else {
                formatar_memoria(metricas->memoria_limite, sizeof(metricas->memoria_limite), memory_limit);
            }
        }
        fclose(arquivo);
    }
    
    return 0;
}

/*
 * Obtém métricas de cgroup para a versão 2
 */
int get_metricas_cgroup_v2(pid_t pid, metricas_cgroup_t* metricas) {
    char caminho[512];
    char linha[256];
    FILE *arquivo;
    
    /* Lê arquivo cgroup do processo */
    snprintf(caminho, sizeof(caminho), "/proc/%d/cgroup", pid);
    arquivo = fopen(caminho, "r");
    if (arquivo == NULL) {
        return 0;
    }
    
    char cgroup_path[128] = "";
    int cgroup_encontrado = 0;
    
    /* Parse para encontrar cgroup v2 (indicado por "0::") */
    while (fgets(linha, sizeof(linha), arquivo)) {
        if (strstr(linha, "0::")) {
            char *path = strchr(linha, ':');
            if (path) {
                path = strchr(path + 1, ':');
                if (path) {
                    size_t len = strlen(path + 1);
                    if (len > 0 && len < sizeof(cgroup_path)) {
                        strncpy(cgroup_path, path + 1, len);
                        cgroup_path[len] = '\0';
                        char *newline = strchr(cgroup_path, '\n');
                        if (newline) *newline = '\0';
                        cgroup_encontrado = 1;
                    }
                }
            }
            break;
        }
    }
    fclose(arquivo);
    
    /* Se não encontrou, assume cgroup raiz */
    if (!cgroup_encontrado || strlen(cgroup_path) > 100) {
        strcpy(cgroup_path, "/");
    }
    
    /* Obtém estatísticas de CPU */
    snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup%s/cpu.stat", cgroup_path);
    arquivo = fopen(caminho, "r");
    if (arquivo != NULL) {
        unsigned long long user_usage = 0, system_usage = 0;
        while (fgets(linha, sizeof(linha), arquivo)) {
            if (strstr(linha, "user_usec")) {
                sscanf(linha, "user_usec %llu", &user_usage);
            } else if (strstr(linha, "system_usec")) {
                sscanf(linha, "system_usec %llu", &system_usage);
            }
        }
        fclose(arquivo);
        
        if (user_usage > 0 || system_usage > 0) {
            /* Converte microsegundos para segundos */
            double total_seconds = (user_usage + system_usage) / 1e6;
            snprintf(metricas->cpu_usada, sizeof(metricas->cpu_usada), "%.2fs", total_seconds);
        }
    }
    
    /* Obtém uso atual de memória */
    snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup%s/memory.current", cgroup_path);
    arquivo = fopen(caminho, "r");
    if (arquivo != NULL) {
        unsigned long memory_usage;
        if (fscanf(arquivo, "%lu", &memory_usage) == 1) {
            formatar_memoria(metricas->memoria_usada, sizeof(metricas->memoria_usada), memory_usage);
        }
        fclose(arquivo);
    }
    
    /* Obtém limite de memória */
    snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup%s/memory.max", cgroup_path);
    arquivo = fopen(caminho, "r");
    if (arquivo != NULL) {
        char max_buffer[64];
        if (fgets(max_buffer, sizeof(max_buffer), arquivo) != NULL) {
            max_buffer[strcspn(max_buffer, "\n")] = 0;
            
            if (strcmp(max_buffer, "max") == 0) {
                /* "max" significa memória ilimitada */
                strncpy(metricas->memoria_limite, "ilimitado", sizeof(metricas->memoria_limite) - 1);
            } else {
                unsigned long memory_limit = strtoul(max_buffer, NULL, 10);
                formatar_memoria(metricas->memoria_limite, sizeof(metricas->memoria_limite), memory_limit);
            }
        }
        fclose(arquivo);
    }
    
    return 0;
}

/*
 * Define limites de I/O para cgroup v1 (função específica)
 */
int limite_blkio_v1(const char* nome_cgroup, unsigned long ler_blkIo, unsigned long escrever_blkIo) {
    if (nome_cgroup == NULL) {
        errno = EINVAL;
        return -1;
    }

    char caminho[512];

    /* Cria cgroup blkio se não existir */
    snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/blkio/%s", nome_cgroup);
    if (mkdir(caminho, 0755) != 0 && errno != EEXIST) {
        perror("Erro ao criar cgroup BlkIO");
        return -1;
    }

    /* Define limite de leitura */
    if (ler_blkIo > 0) {
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/blkio/%s/blkio.throttle.read_bps_device", nome_cgroup);
        if (escrever_arquivo(caminho, "8:0 %lu", ler_blkIo) != 0) {
            printf("Aviso: não foi possível configurar limite de leitura BlkIO\n");
        }
    }

    /* Define limite de escrita */
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

/*
 * Obtém métricas de I/O de um cgroup
 */
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
        /* cgroup v2: lê de io.stat */
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
        /* cgroup v1: lê de blkio.io_service_bytes */
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

/*
 * Experimento para testar throttling de CPU com cgroups.
 * Aplica diferentes limites e mede o desempenho real.
 */
void experimento_throttling_cpu() {
    printf("\n=== EXPERIMENTO 3: THROTTLING DE CPU ===\n");
    
    const char* cgroup_name = "teste_cpu_exp";
    double limites[] = {0.25, 0.5, 1.0, 2.0};
    int num_limites = sizeof(limites) / sizeof(limites[0]);
    int version = detectar_cgroup_version();
    
    /* Cria cgroup para o experimento */
    if (criar_cgroup(cgroup_name) != 0) {
        printf("Erro ao criar cgroup para experimento\n");
        return;
    }
    
    /* Move processo atual para o cgroup */
    if (mover_cgroup(cgroup_name, getpid()) != 0) {
        printf("Erro ao mover processo\n");
        return;
    }
    
    printf("Limite\tCPU%% Medido\tDesvio\t\tThroughput\tStatus\n");
    printf("------\t----------\t------\t\t----------\t------\n");
    
    /* Testa cada limite de CPU */
    for (int i = 0; i < num_limites; i++) {
        if (limite_cpu(cgroup_name, limites[i]) != 0) {
            printf("Erro ao aplicar limite de CPU\n");
            continue;
        }
        
        sleep(1); /* Espera estabilização */
        
        clock_t inicio = clock();
        unsigned long iteracoes = 0;
        metricas_cpu_t cpu;
        double cpu_total = 0;
        int medicoes = 0;
        
        /* Executa carga de CPU por 3 segundos */
        while ((clock() - inicio) < (CLOCKS_PER_SEC * 3)) {
            volatile double resultado = 0;
            /* Gera carga de CPU com cálculos matemáticos intensivos */
            for (int j = 0; j < 100000; j++) { 
                resultado += sqrt(j) * tan(j * 0.001) * log(j + 1) * cos(j * 0.01);
            }
            (void)resultado;
            iteracoes++;
            
            /* Coleta métricas periodicamente */
            if (iteracoes % 200 == 0 && get_metricas_cpu(getpid(), &cpu) == 0) {
                cpu_total += cpu.porcentagem_cpu;
                medicoes++;
            }
        }
        
        /* Calcula métricas de desempenho */
        double cpu_medio = medicoes > 0 ? cpu_total / medicoes : 0;
        double throughput = (double)iteracoes / 3.0;
        
        double cpu_esperado = limites[i] * 100.0;
        double desvio = 0.0;
        char status[32];
        
        /* Análise de resultados baseada na versão do cgroup */
        if (version == 2) {
            /* Cgroup v2 usa peso, então a precisão é diferente */
            if (cpu_medio > 0.1 && cpu_esperado > 0.1) {
                double ratio = cpu_medio / cpu_esperado;
                if (ratio > 1.3) {
                    desvio = (ratio - 1.3) * 100;
                    strcpy(status, "ALTO");
                } else if (ratio < 0.7) {
                    desvio = (0.7 - ratio) * 100;
                    strcpy(status, "BAIXO");
                } else {
                    desvio = fabs(ratio - 1.0) * 100;
                    strcpy(status, "OK");
                }
            } else {
                strcpy(status, "N/A");
            }
        } else {
            /* Cgroup v1 tem limites mais rígidos */
            if (cpu_esperado > 0.1) {
                desvio = fabs(cpu_medio - cpu_esperado) / cpu_esperado * 100.0;
                if (desvio < 15.0) strcpy(status, "PRECISO");
                else if (desvio < 30.0) strcpy(status, "ACEITÁVEL");
                else strcpy(status, "IMPRECISO");
            } else {
                strcpy(status, "N/A");
            }
        }
        
        printf("%.2f\t%.1f%%\t\t%.1f%%\t\t%.0f iter/s\t%s\n", limites[i], cpu_medio, desvio, throughput, status);
    }
    
    /* Limpeza: move processo de volta para root e remove cgroup */
    mover_para_root(getpid());
    remover_cgroup(cgroup_name);
    
    if (version == 2) {
        printf("\nNota: CGroup v2 usa 'peso' em vez de limites rígidos.\n");
        printf("Desvios maiores são normais. Faixa aceitável: 70%%-130%% do limite.\n");
    }
}

/*
 * Experimento para testar limitação de memória com cgroups.
 * Tenta alocar memória incrementalmente até atingir o limite.
 */
void experimento_limite_memoria() {
    printf("\n=== EXPERIMENTO 4: LIMITAÇÃO DE MEMÓRIA ===\n");
    
    const char* cgroup_name = "teste_mem_exp";
    unsigned long limite_mb = 100;
    
    /* Verifica privilégios de root */
    if (geteuid() != 0) {
        printf("AVISO: Este experimento requer privilégios de root.\n");
        printf("Execute com: sudo ./resource-monitor -e\n");
        return;
    }
    
    /* Configura cgroup com limite de memória */
    if (criar_cgroup(cgroup_name) != 0) {
        printf("Erro ao criar cgroup para experimento\n");
        return;
    }
    
    if (mover_cgroup(cgroup_name, getpid()) != 0) {
        printf("Erro ao mover processo\n");
        return;
    }
    
    if (limite_memoria(cgroup_name, limite_mb) != 0) {
        printf("Erro ao aplicar limite de memória\n");
        return;
    }
    
    printf("Tentando alocar memória incrementalmente (limite: %lu MB)...\n", limite_mb);
    
    size_t bloco_size = 10 * 1024 * 1024; /* 10MB por bloco */
    char** blocos = NULL;
    int num_blocos = 0;
    size_t total_alocado = 0;
    int falha_ocorrida = 0;
    
    /* Loop de alocação incremental */
    while (total_alocado < limite_mb * 1024 * 1024 && !falha_ocorrida) {
        char* bloco = malloc(bloco_size);
        if (bloco == NULL) {
            printf("Falha de alocação em %zu MB\n", total_alocado / (1024 * 1024));
            falha_ocorrida = 1;
            break;
        }
        
        /* Acessa a memória para forçar alocação real */
        memset(bloco, 0xAA, bloco_size / 10);
        
        /* Expande array de blocos */
        char** novo_array = realloc(blocos, (num_blocos + 1) * sizeof(char*));
        if (novo_array == NULL) {
            printf("Falha ao expandir array em %zu MB\n", total_alocado / (1024 * 1024));
            free(bloco);
            falha_ocorrida = 1;
            break;
        }
        
        blocos = novo_array;
        blocos[num_blocos++] = bloco;
        total_alocado += bloco_size;
        
        printf("Alocado: %zu MB\n", total_alocado / (1024 * 1024));
        
        /* Mostra métricas atuais do cgroup */
        metricas_cgroup_t metrics;
        if (get_metricas_cgroup(getpid(), &metrics) == 0) {
            printf("  Uso atual: %s, Limite: %s\n", metrics.memoria_usada, metrics.memoria_limite);
        }
        
        sleep(1);
    }
    
    printf("Máximo alocado: %zu MB\n", total_alocado / (1024 * 1024));
    
    /* Limpeza: libera toda a memória alocada */
    if (blocos != NULL) {
        for (int i = 0; i < num_blocos; i++) {
            if (blocos[i] != NULL) {
                free(blocos[i]);
            }
        }
        free(blocos);
    }
    
    /* Move processo de volta e remove cgroup */
    mover_para_root(getpid());
    remover_cgroup(cgroup_name);
    
    printf("Comportamento: %s\n", falha_ocorrida ? "Falha de alocação antes do limite" : "Limite atingido sem OOM killer");
}

/*
 * Formata bytes em string legível (B, KB, MB, GB)
 */
void formatar_memoria(char* buffer, size_t buffer_size, unsigned long bytes) {
    if (bytes > 1024 * 1024) {
        snprintf(buffer, buffer_size, "%.1fMB", bytes / (1024.0 * 1024.0));
    } else if (bytes > 1024) {
        snprintf(buffer, buffer_size, "%.1fKB", bytes / 1024.0);
    } else {
        snprintf(buffer, buffer_size, "%luB", bytes);
    }
}

/*
 * Experimento para testar limitação de I/O com cgroups.
 * Aplica diferentes limites e mede throughput e latência.
 */
void experimento_limite_io() {
    printf("\n=== EXPERIMENTO 5: LIMITAÇÃO DE I/O ===\n");
    
    const char* cgroup_name = "teste_io_exp";
    unsigned long limites_bps[] = {1024 * 1024, 512 * 1024, 256 * 1024}; /* 1MB/s, 512KB/s, 256KB/s */
    int num_limites = sizeof(limites_bps) / sizeof(limites_bps[0]);
    
    /* Configura cgroup para experimento */
    if (criar_cgroup(cgroup_name) != 0) {
        printf("Erro ao criar cgroup para experimento\n");
        return;
    }
    
    if (mover_cgroup(cgroup_name, getpid()) != 0) {
        printf("Erro ao mover processo\n");
        return;
    }
    
    printf("Limite\tThroughput Medido\tLatência\tTempo Execução\n");
    printf("------\t----------------\t--------\t-------------\n");
    
    /* Testa cada limite de I/O */
    for (int i = 0; i < num_limites; i++) {
        if (limite_io(cgroup_name, limites_bps[i], limites_bps[i]) != 0) {
            printf("Erro ao aplicar limite de I/O\n");
            continue;
        }
        
        sleep(1); /* Espera estabilização */
        
        const char* test_file = "io_test_file.dat";
        const size_t block_size = 4096;
        const int num_blocks = 1000;
        
        clock_t inicio = clock();
        metricas_io_t io_inicio, io_fim;
        
        /* Mede I/O inicial */
        get_metricas_io(getpid(), &io_inicio);
        
        /* Executa teste de escrita */
        FILE* file = fopen(test_file, "w");
        if (file) {
            char buffer[block_size];
            memset(buffer, 'X', block_size);
            
            for (int j = 0; j < num_blocks; j++) {
                fwrite(buffer, 1, block_size, file);
                fflush(file); /* Força escrita imediata */
            }
            fclose(file);
        }
        
        /* Mede I/O final e tempo */
        get_metricas_io(getpid(), &io_fim);
        clock_t fim = clock();
        
        /* Calcula métricas */
        double tempo_execucao = ((double)(fim - inicio)) / CLOCKS_PER_SEC;
        unsigned long bytes_escritos = io_fim.bytes_escritos - io_inicio.bytes_escritos;
        double throughput = bytes_escritos / tempo_execucao;
        double latencia_media = tempo_execucao * 1000 / num_blocks;
        
        printf("%lu B/s\t%.0f B/s (%.1f%%)\t%.2f ms\t\t%.2f s\n", limites_bps[i], throughput, (throughput / limites_bps[i]) * 100,latencia_media, tempo_execucao);
        
        /* Remove arquivo de teste */
        unlink(test_file);
    }
    
    /* Limpeza */
    mover_cgroup("", getpid());
    remover_cgroup(cgroup_name);
}

/*
 * Move um processo de volta para o cgroup raiz
 */
int mover_para_root(pid_t pid) {
    int version = detectar_cgroup_version();
    char caminho[512];
    char pid_str[32];
    
    snprintf(pid_str, sizeof(pid_str), "%d", pid);
    
    if (version == 2) {
        /* cgroup v2: move para cgroup raiz */
        snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/cgroup.procs");
        return escrever_arquivo(caminho, "%s", pid_str);
    } else if (version == 1) {
        /* cgroup v1: move para root em todos os controladores */
        char* controladores[] = {"cpu", "memory", "blkio"};
        int sucesso = 0;
        
        for (int i = 0; i < 3; i++) {
            snprintf(caminho, sizeof(caminho), "/sys/fs/cgroup/%s/cgroup.procs", controladores[i]);
            if (escrever_arquivo(caminho, "%s", pid_str) == 0) {
                sucesso++;
            }
        }
        if (sucesso > 0) {
    printf("Processo %d movido com sucesso para cgroup root\n", pid);
        return 0;
    } else {
        printf("Falha ao mover processo %d para cgroup root\n", pid);
        return -1;
    }
    }
    
    return -1;
}
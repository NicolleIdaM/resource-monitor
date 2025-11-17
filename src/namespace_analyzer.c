#include "../include/monitor.h"
#include "../include/namespace.h"
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sched.h>

/*
 * Obtém informações de namespaces de um processo específico.
 * Lê os inodes dos links simbólicos em /proc/[pid]/ns/ para identificar namespaces.
 */
int get_infos_namespace(pid_t pid, metricas_namespace_t* metricas){
    if(metricas == NULL || pid <= 0){
        errno = EINVAL;
        return -1;
    }

    char caminho[256];
    struct stat numero_inode;
    int sucesso = 0;

    /* Inicializa todos os namespaces com 0 */
    metricas->pid_namespace = 0;
    metricas->usuario_namespace = 0;
    metricas->filesystem_namespace = 0;
    metricas->net_namespace = 0;
    metricas->hostname_namespace = 0;
    metricas->ipc_namespace = 0;

    /* Lê namespace de PID */
    snprintf(caminho, sizeof(caminho), "/proc/%d/ns/pid", pid);
    if(stat(caminho, &numero_inode) == 0){
        metricas->pid_namespace = numero_inode.st_ino;
        sucesso++;
    } else {
        perror("Erro ao acessar pid namespace");
    }

    /* Lê namespace de usuário */
    snprintf(caminho, sizeof(caminho), "/proc/%d/ns/user", pid);
    if(stat(caminho, &numero_inode) == 0){
        metricas->usuario_namespace = numero_inode.st_ino;
        sucesso++;
    } else {
        perror("Erro ao acessar user namespace");
    }

    /* Lê namespace de filesystem (mount) */
    snprintf(caminho, sizeof(caminho), "/proc/%d/ns/mnt", pid);
    if(stat(caminho, &numero_inode) == 0){
        metricas->filesystem_namespace = numero_inode.st_ino;
        sucesso++;
    } else {
        perror("Erro ao acessar mnt namespace");
    }

    /* Lê namespace de rede */
    snprintf(caminho, sizeof(caminho), "/proc/%d/ns/net", pid);
    if(stat(caminho, &numero_inode) == 0){
        metricas->net_namespace = numero_inode.st_ino;
        sucesso++;
    } else {
        perror("Erro ao acessar net namespace");
    }

    /* Lê namespace de hostname (UTS) */
    snprintf(caminho, sizeof(caminho), "/proc/%d/ns/uts", pid);
    if(stat(caminho, &numero_inode) == 0){
        metricas->hostname_namespace = numero_inode.st_ino;
        sucesso++;
    } else {
        perror("Erro ao acessar uts namespace");
    }

    /* Lê namespace de IPC */
    snprintf(caminho, sizeof(caminho), "/proc/%d/ns/ipc", pid);
    if(stat(caminho, &numero_inode) == 0){
        metricas->ipc_namespace = numero_inode.st_ino;
        sucesso++;
    } else {
        perror("Erro ao acessar ipc namespace");
    }

    /* Verifica se conseguiu ler algum namespace */
    if (sucesso == 0) {
        fprintf(stderr, "Erro: não foi possível ler nenhum namespace para PID %d\n", pid);
        errno = ENOENT;
        return -1;
    }

    printf("Sucesso: lidos %d/6 namespaces para PID %d\n", sucesso, pid);
    return 0;
}

/*
 * Compara os namespaces de dois processos.
 * Retorna o número de namespaces que são diferentes.
 */
int comparar_namespace(pid_t pid1, pid_t pid2){
    if (pid1 <= 0 || pid2 <= 0) {
        errno = EINVAL;
        return -1;
    }

    metricas_namespace_t namespace1, namespace2;

    /* Obtém namespaces do primeiro processo */
    if(get_infos_namespace(pid1, &namespace1) == -1){
        fprintf(stderr, "Erro ao obter namespaces para PID %d\n", pid1);
        return -1;
    }

    /* Obtém namespaces do segundo processo */
    if(get_infos_namespace(pid2, &namespace2) == -1){
        fprintf(stderr, "Erro ao obter namespaces para PID %d\n", pid2);
        return -1;
    }

    int diferentes = 0;
    
    /* Compara cada namespace individualmente */
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

/*
 * Procura todos os processos que estão em um namespace específico.
 * ns_tipo: tipo de namespace (pid, user, net, etc)
 * ns_id: inode do namespace a ser procurado
 */
int procurar_processo(const char* ns_tipo, const char* ns_id){
    if (ns_tipo == NULL || ns_id == NULL) {
        errno = EINVAL;
        return -1;
    }

    DIR *dir;
    struct dirent *entrada_diretorio;
    char caminho[512];
    char namespace_caminho[512];
    
    /* Converte string do ID para número */
    char *endptr;
    unsigned long ns_inode = strtoul(ns_id, &endptr, 10);
    if (endptr == ns_id || *endptr != '\0') {
        fprintf(stderr, "Erro: ID de namespace inválido: %s\n", ns_id);
        errno = EINVAL;
        return -1;
    }

    /* Abre diretório /proc para listar todos os processos */
    dir = opendir("/proc");
    if(dir == NULL){
        perror("Erro ao abrir o diretorio /proc");
        return -1;
    }

    printf("Processos no namespace %s [%s]:\n", ns_tipo, ns_id);
    int processos_encontrados = 0;

    /* Itera por todas as entradas do diretório /proc */
    while((entrada_diretorio = readdir(dir)) != NULL){
        /* Filtra apenas diretórios que são PIDs numéricos */
        if(entrada_diretorio->d_type == DT_DIR){
            int pid = atoi(entrada_diretorio->d_name);
            if(pid > 0){
                /* Constrói caminho para o namespace específico do processo */
                snprintf(namespace_caminho, sizeof(namespace_caminho), 
                        "/proc/%s/ns/%s", entrada_diretorio->d_name, ns_tipo);
                
                /* Lê inode do namespace */
                struct stat numero_inode;
                if(stat(namespace_caminho, &numero_inode) == 0){
                    /* Verifica se o inode corresponde ao procurado */
                    if(numero_inode.st_ino == ns_inode){
                        /* Obtém nome do comando do processo */
                        snprintf(caminho, sizeof(caminho), "/proc/%s/comm", entrada_diretorio->d_name);
                        
                        FILE *arquivo_comando = fopen(caminho, "r");
                        char comando[256] = "desconhecido";
                        if(arquivo_comando){
                            if (fgets(comando, sizeof(comando), arquivo_comando) != NULL) {
                                /* Remove newline do final */
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

/*
 * Lista namespaces de processos importantes (init, processo atual)
 */
void listar_namespaces(){
    /* PIDs a serem analisados: init (1), processo atual, e terminador (0) */
    pid_t pids[] = {1, getpid(), 0};
    const char* nomes[] = {"init (PID 1)", "Processo Atual", ""};
    int sucesso_total = 0;

    /* Itera pelos PIDs e mostra seus namespaces */
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

/*
 * Extrai o tipo de namespace a partir de um link simbólico.
 * Exemplo: "/proc/123/ns/pid" -> "pid"
 */
char* obter_tipo_namespace(const char* ns_link){
    if (ns_link == NULL) {
        return "desconhecido";
    }
    
    static char tipo[32];
    const char *prefixo = strrchr(ns_link, '/');
    if(prefixo){
        /* Copia o texto após a última barra */
        strncpy(tipo, prefixo + 1, sizeof(tipo) - 1);
        tipo[sizeof(tipo) - 1] = '\0';
        return tipo;
    }
    
    strncpy(tipo, "desconhecido", sizeof(tipo) - 1);
    return tipo;
}

/*
 * Mede o overhead de criação de namespaces usando unshare().
 * Testa a criação de cada tipo de namespace individualmente.
 */
void medir_overhead_namespaces() {
    printf("\n=== MEDIÇÃO REAL DE OVERHEAD DE CRIAÇÃO ===\n");
    
    struct timeval inicio, fim;
    const char* nomes[] = {"PID", "Network", "Mount", "UTS", "IPC", "User"};
    int flags[] = {CLONE_NEWPID, CLONE_NEWNET, CLONE_NEWNS, CLONE_NEWUTS, CLONE_NEWIPC, CLONE_NEWUSER};
    const int NUM_TIPOS = 6;
    const int NUM_TESTES = 2; /* Número de testes por namespace para média */
    
    /* Testa cada tipo de namespace */
    for (int i = 0; i < NUM_TIPOS; i++) {
        long total_micros = 0;
        int testes_validos = 0;
        
        /* Executa múltiplos testes para obter uma média */
        for (int teste = 0; teste < NUM_TESTES; teste++) {
            gettimeofday(&inicio, NULL);
            pid_t pid = fork();
            
            if (pid == 0) {
                /* Processo filho: tenta criar o namespace */
                if (unshare(flags[i]) == -1) {
                    _exit(1); /* Falha na criação */
                }
                _exit(0); /* Sucesso */
            } else if (pid > 0) {
                /* Processo pai: espera filho terminar e mede tempo */
                int status;
                waitpid(pid, &status, 0);
                gettimeofday(&fim, NULL);
                
                /* Só conta se o filho terminou com sucesso */
                if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                    long micros = (fim.tv_sec - inicio.tv_sec) * 1000000L + 
                                 (fim.tv_usec - inicio.tv_usec);
                    total_micros += micros;
                    testes_validos++;
                }
            }
            usleep(50000); /* Pequena pausa entre testes */
        }
        
        /* Mostra resultado médio */
        if (testes_validos > 0) {
            printf("  %s namespace: %ld µs\n", nomes[i], total_micros / testes_validos);
        } else {
            printf("  %s namespace: N/A (sem permissão)\n", nomes[i]);
        }
    }
}

/*
 * Experimento completo de análise de namespaces.
 * Compara namespaces entre processos e mede overhead de criação.
 */
void experimento_isolamento_namespace() {
    printf("\n=== EXPERIMENTO 2: ISOLAMENTO VIA NAMESPACES ===\n");
    
    /* PIDs a serem comparados: processo pai e processo atual */
    pid_t pids[] = {getppid(), getpid()};
    const char* nomes[] = {"processo pai", "processo atual"};
    int num_pids = sizeof(pids) / sizeof(pids[0]);
    
    printf("Comparação de namespaces entre processos:\n");
    /* Compara cada par de processos */
    for (int i = 0; i < num_pids - 1; i++) {
        for (int j = i + 1; j < num_pids; j++) {
            printf("\n%s (PID %d) vs %s (PID %d):\n", nomes[i], pids[i], nomes[j], pids[j]);
            int diferentes = comparar_namespace(pids[i], pids[j]);
            if (diferentes >= 0) {
                printf("Namespaces diferentes: %d/6\n", diferentes);
            }
        }
    }
    
    /* Mede overhead de criação de namespaces */
    medir_overhead_namespaces(); 
    
    /* Lista processos no mesmo namespace do processo atual */
    printf("\nProcessos por namespace no sistema:\n");
    metricas_namespace_t current_ns;
    if (get_infos_namespace(getpid(), &current_ns) == 0) {
        char ns_id[32];
        snprintf(ns_id, sizeof(ns_id), "%lu", current_ns.pid_namespace);
        printf("Processos no mesmo PID namespace: ");
        procurar_processo("pid", ns_id);
    }
}
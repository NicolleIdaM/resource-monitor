#include "../include/monitor.h"
#include "../include/namespace.h"
#include "../include/cgroup.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

void experimento_overhead(void);
void experimento_isolamento_namespace(void);
void experimento_throttling_cpu(void);
void experimento_limite_memoria(void);
void experimento_limite_io(void);

void mostrar_uso(const char* nome_programa) {
    printf("Uso: %s [OPÇÕES]\n", nome_programa);
    printf("\nOPÇÕES:\n");
    printf("  -p, --pid PID          Monitorar processo específico (padrão: processo atual)\n");
    printf("  -i, --intervalo MS     Intervalo de monitoramento em milissegundos (padrão: 1000)\n");
    printf("  -n, --iteracoes N      Número de iterações de monitoramento (padrão: 10)\n");
    printf("  -m, --monitorar        Modo de monitoramento contínuo\n");
    printf("  -e, --experimentos     Executar todos os experimentos obrigatórios\n");
    printf("  -c, --cgroup AÇÃO      Gerenciar cgroups [listar|criar NOME|remover NOME]\n");
    printf("  -s, --namespace AÇÃO   Analisar namespaces [listar|comparar PID1,PID2|procurar TIPO,ID]\n");
    printf("  -h, --help             Mostrar esta ajuda\n");
    printf("\nEXEMPLOS:\n");
    printf("  %s -m -p 1234 -i 500          # Monitorar PID 1234 a cada 500ms\n", nome_programa);
    printf("  %s -e                         # Executar todos os experimentos\n", nome_programa);
    printf("  %s -c listar                  # Listar cgroups existentes\n", nome_programa);
    printf("  %s -s listar                  # Listar namespaces do sistema\n", nome_programa);
    printf("  %s -s comparar 1,1234         # Comparar namespaces dos PIDs 1 e 1234\n", nome_programa);
}

void monitorar_processo(pid_t pid, int intervalo, int iteracoes) {
    printf("Iniciando monitoramento do PID %d\n", pid);
    printf("Intervalo: %d ms, Iterações: %d\n\n", intervalo, iteracoes);
    
    printf("%-8s %-8s %-12s %-10s %-12s %-10s %-8s\n", "Tempo", "CPU%", "Memória", "Swap", "IO Leitura", "IO Escrita", "Threads");
    printf("%-8s %-8s %-12s %-10s %-12s %-10s %-8s\n", "(s)", "", "(MB)", "(MB)", "(MB)", "(MB)", "");
    
    for (int i = 0; i < iteracoes; i++) {
        metricas_cpu_t cpu;
        metricas_memoria_t memoria;
        metricas_io_t io;
        
        if (get_metricas_cpu(pid, &cpu) == 0 &&
            get_metricas_memoria(pid, &memoria) == 0 &&
            get_metricas_io(pid, &io) == 0) {
            
            printf("%-8d %-8.1f %-12.1f %-10.1f %-12.1f %-10.1f %-8lu\n",
                   i * intervalo / 1000,
                   cpu.porcentagem_cpu,
                   memoria.RAM / (1024.0 * 1024.0),
                   memoria.swap / (1024.0 * 1024.0),
                   io.bytes_lidos_disco / (1024.0 * 1024.0),
                   io.bytes_escritos_disco / (1024.0 * 1024.0),
                   cpu.threads);
        } else {
            printf("Erro ao coletar métricas na iteração %d\n", i);
        }
        
        usleep(intervalo * 1000);
    }
}

void modo_monitoramento_detalhado(pid_t pid, int intervalo, int iteracoes) {
    printf("=== MODO DE MONITORAMENTO DETALHADO ===\n");
    printf("PID: %d, Intervalo: %d ms, Iterações: %d\n\n", pid, intervalo, iteracoes);
    
    for (int i = 0; i < iteracoes; i++) {
        printf("--- Iteração %d ---\n", i + 1);
        
        metricas_cpu_t cpu;
        if (get_metricas_cpu(pid, &cpu) == 0) {
            printf("CPU: %.1f%% (User: %lu ticks, System: %lu ticks)\n",
                   cpu.porcentagem_cpu, cpu.tempo_usuario, cpu.tempo_sistema);
            printf("Context Switches: %lu, Threads: %lu\n",
                   cpu.context_switches, cpu.threads);
        } else {
            printf("CPU: Erro ao obter métricas\n");
        }
        
        metricas_memoria_t memoria;
        if (get_metricas_memoria(pid, &memoria) == 0) {
            printf("Memória: RAM=%.1fMB, Virtual=%.1fMB, Swap=%.1fMB\n",
                   memoria.RAM / (1024.0 * 1024.0),
                   memoria.MV / (1024.0 * 1024.0),
                   memoria.swap / (1024.0 * 1024.0));
            printf("Page Faults: Menores=%lu, Maiores=%lu\n",
                   memoria.falha_pag_menor, memoria.falha_pag_maior);
        } else {
            printf("Memória: Erro ao obter métricas\n");
        }
        
        metricas_io_t io;
        if (get_metricas_io(pid, &io) == 0) {
            printf("I/O: Leitura=%luMB, Escrita=%luMB\n",
                   io.bytes_lidos_disco / (1024 * 1024),
                   io.bytes_escritos_disco / (1024 * 1024));
            printf("Syscalls: Read=%lu, Write=%lu\n",
                   io.chamadas_lidas, io.chamadas_escritas);
        } else {
            printf("I/O: Erro ao obter métricas\n");
        }
        
        printf("\n");
        usleep(intervalo * 1000);
    }
}

void executar_experimentos() {
    printf("=== EXECUTANDO EXPERIMENTOS OBRIGATÓRIOS ===\n");
    printf("============================================\n\n");
    
    experimento_overhead();
    printf("\n");
    
    experimento_isolamento_namespace();
    printf("\n");
    
    experimento_throttling_cpu();
    printf("\n");
    
    experimento_limite_memoria();
    printf("\n");
    
    experimento_limite_io();
    
    printf("============================================\n");
    printf ("EXPERIMENTOS CONCLUÍDOS\n");
}

void processar_comando_cgroup(const char* acao) {
    if (strcmp(acao, "listar") == 0) {
        listar_cgroups();
    } else if (strncmp(acao, "criar ", 6) == 0) {
        const char* nome = acao + 6;
        if (criar_cgroup(nome) == 0) {
            printf("Cgroup '%s' criado com sucesso\n", nome);
        }
    } else if (strncmp(acao, "remover ", 8) == 0) {
        const char* nome = acao + 8;
        if (remover_cgroup(nome) == 0) {
            printf("Cgroup '%s' removido com sucesso\n", nome);
        }
    } else {
        printf("Ação de cgroup inválida. Use: listar, criar NOME, ou remover NOME\n");
    }
}

void processar_comando_namespace(const char* acao) {
    if (strcmp(acao, "listar") == 0) {
        listar_namespaces();
    } else if (strncmp(acao, "comparar", 8) == 0) {
        char* acao_copy = strdup(acao);
        char* token = strtok(acao_copy, " ");
        
        if (token != NULL && strcmp(token, "comparar") == 0) {
            token = strtok(NULL, ",");
            if (token != NULL) {
                char* pid1_str = token;
                char* pid2_str = strtok(NULL, ",");
                
                if (pid1_str && pid2_str) {
                    pid_t pid1 = atoi(pid1_str);
                    pid_t pid2 = atoi(pid2_str);
                    if (pid1 > 0 && pid2 > 0) {
                        comparar_namespace(pid1, pid2);
                    } else {
                        printf("PIDs inválidos: %s, %s\n", pid1_str, pid2_str);
                    }
                } else {
                    printf("Formato inválido. Use: -s 'comparar PID1,PID2'\n");
                }
            } else {
                printf("Formato inválido. Use: -s 'comparar PID1,PID2'\n");
            }
        }
        free(acao_copy);
    } else if (strncmp(acao, "procurar", 8) == 0) {
        char* acao_copy = strdup(acao);
        char* token = strtok(acao_copy, " ");
        
        if (token != NULL && strcmp(token, "procurar") == 0) {
            token = strtok(NULL, ",");
            if (token != NULL) {
                char* tipo = token;
                char* id = strtok(NULL, ",");
                
                if (tipo && id) {
                    procurar_processo(tipo, id);
                } else {
                    printf("Formato inválido. Use: -s 'procurar TIPO,ID'\n");
                }
            } else {
                printf("Formato inválido. Use: -s 'procurar TIPO,ID'\n");
            }
        }
        free(acao_copy);
    } else {
        printf("Ação de namespace inválida. Use: listar, 'comparar PID1,PID2', ou 'procurar TIPO,ID'\n");
    }
}

int main(int argc, char *argv[]) {
    pid_t pid = getpid();
    int intervalo = 1000;
    int iteracoes = 10;
    int modo_monitorar = 0;
    int modo_detalhado = 0;
    int executar_exps = 0;
    char* comando_cgroup = NULL;
    char* comando_namespace = NULL;
    
    static struct option opcoes_longa[] = {
        {"pid", required_argument, 0, 'p'},
        {"intervalo", required_argument, 0, 'i'},
        {"iteracoes", required_argument, 0, 'n'},
        {"monitorar", no_argument, 0, 'm'},
        {"detalhado", no_argument, 0, 'd'},
        {"experimentos", no_argument, 0, 'e'},
        {"cgroup", required_argument, 0, 'c'},
        {"namespace", required_argument, 0, 's'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int opcao;
    int indice_opcao = 0;
    
    while ((opcao = getopt_long(argc, argv, "p:i:n:mdehc:s:", opcoes_longa, &indice_opcao)) != -1) {
        switch (opcao) {
            case 'p':
                pid = atoi(optarg);
                if (pid <= 0) {
                    printf("Erro: PID inválido: %s\n", optarg);
                    return 1;
                }
                break;
            case 'i':
                intervalo = atoi(optarg);
                if (intervalo <= 0) {
                    printf("Erro: Intervalo inválido: %s\n", optarg);
                    return 1;
                }
                break;
            case 'n':
                iteracoes = atoi(optarg);
                if (iteracoes <= 0) {
                    printf("Erro: Número de iterações inválido: %s\n", optarg);
                    return 1;
                }
                break;
            case 'm':
                modo_monitorar = 1;
                break;
            case 'd':
                modo_detalhado = 1;
                break;
            case 'e':
                executar_exps = 1;
                break;
            case 'c':
                comando_cgroup = optarg;
                break;
            case 's':
                comando_namespace = optarg;
                break;
            case 'h':
                mostrar_uso(argv[0]);
                return 0;
            default:
                mostrar_uso(argv[0]);
                return 1;
        }
    }
    
    printf("=== RESOURCE MONITOR ===\n");
    printf("Sistema de profiling e análise de recursos\n\n");
    
    if (comando_cgroup) {
        processar_comando_cgroup(comando_cgroup);
        return 0;
    }
    
    if (comando_namespace) {
        processar_comando_namespace(comando_namespace);
        return 0;
    }
    
    if (executar_exps) {
        executar_experimentos();
        return 0;
    }
    
    if (modo_monitorar) {
        if (modo_detalhado) {
            modo_monitoramento_detalhado(pid, intervalo, iteracoes);
        } else {
            monitorar_processo(pid, intervalo, iteracoes);
        }
        return 0;
    }
    
    printf("Selecione uma opção:\n");
    printf("1. Monitoramento em tempo real\n");
    printf("2. Monitoramento detalhado\n");
    printf("3. Executar experimentos obrigatórios\n");
    printf("4. Analisar namespaces\n");
    printf("5. Gerenciar cgroups\n");
    printf("6. Informações do sistema\n");
    printf("0. Sair\n");
    
    printf("\nOpção: ");
    char opcao_interativa[10];
    if (fgets(opcao_interativa, sizeof(opcao_interativa), stdin)) {
        int escolha = atoi(opcao_interativa);
        switch (escolha) {
            case 1:
                monitorar_processo(pid, intervalo, iteracoes);
                break;
            case 2:
                modo_monitoramento_detalhado(pid, intervalo, iteracoes);
                break;
            case 3:
                executar_experimentos();
                break;
            case 4:
                listar_namespaces();
                break;
            case 5:
                listar_cgroups();
                break;
            case 6:
                printf("\n=== INFORMAÇÕES DO SISTEMA ===\n");
                printf("PID atual: %d\n", getpid());
                printf("Versão do cgroup: %d\n", detectar_cgroup_version());
                
                metricas_cpu_t cpu;
                metricas_memoria_t memoria;
                if (get_metricas_cpu(getpid(), &cpu) == 0) {
                    printf("CPU do processo: %.1f%%\n", cpu.porcentagem_cpu);
                }
                if (get_metricas_memoria(getpid(), &memoria) == 0) {
                    printf("Memória do processo: %.1f MB\n", memoria.RAM / (1024.0 * 1024.0));
                }
                break;
            case 0:
                printf("Saindo...\n");
                break;
            default:
                printf("Opção inválida\n");
                break;
        }
    }
    
    return 0;
}
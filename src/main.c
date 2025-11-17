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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include "../include/monitor.h"

/*
 * Gera carga de memória para testes.
 * Aloca, preenche e libera blocos de memória.
 */
void gerar_carga_memoria() {
    const size_t tamanho_bloco = 1024 * 1024; /* 1MB por bloco */
    const int qtde_bloco = 5;

    /* Aloca array de ponteiros para os blocos */
    char** blocos = malloc(qtde_bloco * sizeof(char*));
    if (blocos == NULL) {
        perror("Erro ao alocar array de blocos");
        return;
    }

    printf("  Alocando %d blocos de %zu bytes cada\n", qtde_bloco, tamanho_bloco);

    /* Aloca e preenche cada bloco */
    for(int i = 0; i < qtde_bloco; i++) {
        blocos[i] = malloc(tamanho_bloco);
        if(blocos[i]) {
            /* Preenche bloco com padrão para forçar alocação real */
            memset(blocos[i], i % 256, tamanho_bloco);
            printf("  Alocado bloco %d/%d\n", i + 1, qtde_bloco);
        } else {
            printf("  Falha ao alocar bloco %d/%d\n", i + 1, qtde_bloco);
            break;
        }
        sleep(1);
    }

    /* Libera toda a memória alocada */
    printf("  Liberando memória...\n");
    for(int i = 0; i < qtde_bloco; i++) {
        if(blocos[i]) {
            free(blocos[i]);
        }
    }
    free(blocos);
    printf("  Memória liberada\n");
}

/*
 * Programa de teste para monitor de memória.
 * Testa leitura básica, detecção de uso de memória e múltiplas leituras.
 */
int main() {
    printf("TESTE DE MEMÓRIA\n");

    metricas_memoria_t memoria;
    pid_t pid = getpid();
    int testes_funcionando = 0;

    /* Teste 1 - Leitura Básica */
    printf("\nTESTE DE LEITURA BÁSICA\n");
    if(get_metricas_memoria(pid, &memoria) == 0) {
        printf("  Teste de leitura funcionando (RAM: %luKB, Virtual: %luKB)\n", 
               memoria.RAM / 1024, memoria.MV / 1024);
        testes_funcionando++;
    } else {
        printf("  Teste de leitura falhou\n");
    }

    /* Teste 2 - Detecção de Uso de Memória */
    printf("\nTESTE DE DETECÇÃO DE USO DE MEMÓRIA\n");
    metricas_memoria_t antes, durante;

    /* Mede memória antes da alocação */
    get_metricas_memoria(pid, &antes);

    /* Aloca e mantém alocado 4MB de memória */
    char* bloco1 = malloc(2 * 1024 * 1024);
    char* bloco2 = malloc(2 * 1024 * 1024);
    if(bloco1 && bloco2) memset(bloco1, 1, 2 * 1024 * 1024);

    /* Mede memória durante a alocação */
    get_metricas_memoria(pid, &durante);

    /* Verifica se detectou aumento no uso de memória */
    long diferenca = durante.RAM - antes.RAM;
    if(diferenca > 1024 * 1024) {
        printf("    Teste de detecção funcionando (+%ldKB)\n", diferenca / 1024);
        testes_funcionando++;
    } else {
        printf("    Teste de detecção falhou (+%ldKB)\n", diferenca / 1024);
    }

    /* Libera memória alocada para o teste */
    free(bloco1);
    free(bloco2);

    /* Teste 3 - Múltiplas Leituras */
    printf("\nTESTE DE MÚLTIPLAS LEITURAS\n");
    int leituras_realizadas = 0;
    for(int i = 0; i < 3; i++) {
        if(get_metricas_memoria(pid, &memoria) == 0) {
            leituras_realizadas++;
            printf("  Leitura %d: RAM=%luKB\n", i + 1, memoria.RAM / 1024);
        } else {
            printf("  Falha na leitura %d\n", i + 1);
        }
        sleep(1);
    }

    if(leituras_realizadas == 3) {
        printf("  Leituras realizadas: %d/3 - Sucesso\n", leituras_realizadas);
        testes_funcionando++;
    } else {
        printf("  Teste de múltiplas leituras falhou (%d/3)\n", leituras_realizadas);
    }

    /* Resultado Final */
    printf("\nRESULTADO: %d/3 testes passaram\n", testes_funcionando);
    if(testes_funcionando == 3) {
        printf("Todos os testes funcionaram\n");
    } else {              
        printf("Alguns testes falharam! Verifique o programa!\n");
    }

    return 0;
}
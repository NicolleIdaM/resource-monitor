#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include "../include/monitor.h"

void gerar_carga_memoria() {
    const size_t tamanho_bloco = 1024 * 1024;
    const int qtde_bloco = 5;

    char** blocos = malloc(qtde_bloco * sizeof(char*));
    if (blocos == NULL) {
        perror("Erro ao alocar array de blocos");
        return;
    }

    printf("  Alocando %d blocos de %zu bytes cada\n", qtde_bloco, tamanho_bloco);

    for(int i = 0; i < qtde_bloco; i++) {
        blocos[i] = malloc(tamanho_bloco);
        if(blocos[i]) {
            memset(blocos[i], i % 256, tamanho_bloco);
            printf("  Alocado bloco %d/%d\n", i + 1, qtde_bloco);
        } else {
            printf("  Falha ao alocar bloco %d/%d\n", i + 1, qtde_bloco);
            break;
        }
        sleep(1);
    }

    printf("  Liberando memória...\n");
    for(int i = 0; i < qtde_bloco; i++) {
        if(blocos[i]) {
            free(blocos[i]);
        }
    }
    free(blocos);
    printf("  Memória liberada\n");
}

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

    get_metricas_memoria(pid, &antes);

    // Alocar e manter alocado
    char* bloco1 = malloc(2 * 1024 * 1024);
    char* bloco2 = malloc(2 * 1024 * 1024);
    if(bloco1 && bloco2) memset(bloco1, 1, 2 * 1024 * 1024);

    get_metricas_memoria(pid, &durante);

    long diferenca = durante.RAM - antes.RAM;
    if(diferenca > 1024 * 1024) {
        printf("    Teste de detecção funcionando (+%ldKB)\n", diferenca / 1024);
        testes_funcionando++;
    } else {
        printf("    Teste de detecção falhou (+%ldKB)\n", diferenca / 1024);
    }

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
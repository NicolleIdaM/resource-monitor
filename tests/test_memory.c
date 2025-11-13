#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "../include/monitor.h"
#include <math.h>

void gerar_carga_memoria(){
    const size_t tamanho_bloco = pow(1024, 2);
    const int qtde_bloco = 10;

    char** blocos = malloc(qtde_bloco * sizeof(char*));

    for(int i = 0; i < qtde_bloco; i++){
        blocos[i] = malloc(tamanho_bloco);
        if(blocos[i]){
            memset(blocos[i], i % 256, tamanho_bloco);
            printf("\n  Alocado bloco %d/%d", i + 1, qtde_bloco);
        }
        sleep(1);
    }

    for(int i = 0; i < qtde_bloco; i++){
        free(blocos[i]);
    }
    free(blocos);
}

int main(){
    printf("TESTE DE MEMÓRIA\n");

    metricas_memoria_t memoria;
    pid_t pid = getpid();
    int testes_funcionando = 0;

    /*Teste 1 - Leitura Básica*/
    printf("\nTESTE DE LEITURA BÁSICA");
    if(get_metricas_memoria(pid, &memoria) == 0){
        printf("\n  Teste de leitura funcionando (RAM: %luKB)\n", memoria.RAM / 1024);
        testes_funcionando++;
    }else{
        printf("\n  Teste de leitura falhou\n");
    }

    /*Teste 2 - Detecção de Uso de Memória*/
    printf("\nTESTE DE DETECÇÃO DE USO DE MEMÓRIA");
    metricas_memoria_t antes, depois;

    get_metricas_memoria(pid, &antes);
    gerar_carga_memoria();
    get_metricas_memoria(pid, &depois);

    if(depois.RAM < antes.RAM + (50 * 1024 * 1024)){
        printf("\n  Teste de detecção funcionando (RAM Estável)\n");
        testes_funcionando++;
    }else{
        printf("\n  Teste de detecção falhou (possível vazamento)\n");
    }

    /*Teste 3 - Multiplas Leituras*/
    printf("\nTESTE DE MULTIPLAS LEITURAS");
    int leituras_realizadas = 0;
    for(int i = 0; i < 3; i++){
        if(get_metricas_memoria(pid, &memoria) == 0){
            leituras_realizadas++;
        }
        sleep(1);
    }

    if(leituras_realizadas == 3){
        printf("\n  Leituras realizadas: %d/3\n", leituras_realizadas);
        testes_funcionando++;
    }else{
        printf("\n  Teste de multiplas leituras falhou\n");
    }

    /*Resultado Final*/
    printf("\nRESULTADO: %d/3 passaram\n", testes_funcionando);
    if(testes_funcionando == 3){
        printf("\nTodos os testes funcionaram\n");
    }else{              
        printf("\nAlgo não funcionou! Verefique o programa!\n");
    }

    return 0;
}
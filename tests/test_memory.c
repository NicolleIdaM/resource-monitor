#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/monitor.h"

void gerar_carga_memoria(){
    const size_t tamanho_bloco = pow(1024, 2);
    const int qtde_bloco = 50;

    char** blocos = malloc(qtde_bloco * sizeof(char*));

    for(int i = 0; i < qtde_bloco; i++){
        blocos[i] = malloc(tamanho_bloco);
        if(blocos[i]){
            memset(blocos[i], i % 256, tamanho_bloco);
            printf("Alocado bloco %d/%d\n", i + 1, qtde_bloco);
        }
        usleep(100000);
    }

    sleep(2);

    for(int i = 0; i < qtde_bloco; i++){
        free(blocos[i]);
    }
    free(blocos);
}
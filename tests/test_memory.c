#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/monitor.h"

void gerar_carga_memoria(){
    const size_t tamanho_bloco = pow(1024, 2);
    const int qtde_bloco = 50;

    char** blocos = malloc(qtde_bloco * sizeof(char*));
}
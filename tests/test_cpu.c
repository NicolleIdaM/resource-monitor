#include <stdio.h>;
#include <unistd.h>;
#include <math.h>;

/*Gerar Carga de CPU*/
void gerar_carga_cpu(){
    for(int i = 0; i < 1000000; i++){
        sqrt(i * 1.5);
    }
}
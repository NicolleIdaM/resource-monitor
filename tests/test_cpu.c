#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include "../include/monitor.h"
#include <sys/types.h>

/*Gerar Carga de CPU*/
void gerar_carga_cpu(){
    volatile double resultado = 0;
    const long iteracoes = 3000000;

    for(int i = 0; i < iteracoes; i++){
        resultado += sqrt(i) * (i + 1) * sin(i * 0.01);
        if(i % (iteracoes / 10) == 0){
            printf("Progresso: %d%%\n", (int)((i * 100)/iteracoes));
        }
    }
}

int main(){
    printf("TESTE CPU\n");

    metricas_cpu_t cpu;
    pid_t pid = getpid();
    int testes_funcionando = 0;

    get_metricas_cpu(pid, &cpu);
    sleep(1);

    /*Teste 1 - Leitura Basica*/
    printf("\nTESTE DE LEITURA BASICA\n");
    if(get_metricas_cpu(pid, &cpu) == 0){
        printf("Teste de leitura funcionando (CPU: %.1f%%)\n", cpu.porcentagem_cpu);
        testes_funcionando++;
    }else{
        printf("Teste de leitura falhou\n");
    }

    /*Teste 2 - Detecao de carga*/
    printf("\nTESTE DE DETECAO DE CARGA\n");
    metricas_cpu_t antes, durante;

    get_metricas_cpu(pid, &antes);
    gerar_carga_cpu();
    get_metricas_cpu(pid, &durante);
    
    if(durante.porcentagem_cpu > 1.0){
        printf("Teste de detecao funcionando (CPU: %.1f%%)\n", durante.porcentagem_cpu);
        testes_funcionando++;
    }else{
        printf("Teste de detecao falhou (CPU: %.1f%%)\n", durante.porcentagem_cpu);
    }

    /*Teste 3 - Multiplas Leituras*/
    printf("\nTESTE DE MULTIPLAS LEITURAS\n");
    int leituras_realizadas = 0;
    for(int i = 0; i < 3; i++){
        if(get_metricas_cpu(pid, &cpu) == 0){
            leituras_realizadas++;
        }
        sleep(1);
    }

    if(leituras_realizadas == 3){
        printf("Leituras Realizadas: %d/3\n", leituras_realizadas);
        testes_funcionando++;
    }else{
        printf("Teste de multiplas leituras falhou\n");
    }

    /*Resultado Final*/
    printf("\nRESULTADO: %d/3 passaram\n", testes_funcionando);
    if(testes_funcionando == 3){
        printf("\nTodos os testes funcionaram\n");
    }else{
        printf("\nAlguns testes falharam\n");
    }
    
    return 0;
}
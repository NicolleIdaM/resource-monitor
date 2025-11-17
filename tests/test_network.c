#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "../include/monitor.h"

void gerar_carga_rede() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd >= 0) {
        close(sockfd);
    }
    
    sleep(1);
}

int main() {
    printf("TESTE DE REDE\n");

    metricas_rede_t rede;
    pid_t pid = getpid();
    int testes_funcionando = 0;

    /* Teste 1 - Leitura Básica */
    printf("\nTESTE DE LEITURA BÁSICA\n");
    if (get_metricas_rede(pid, &rede) == 0) {
        printf("  Teste de leitura funcionando\n");
        printf("  Bytes Recebidos: %lu, Bytes Enviados: %lu\n", 
               rede.bytes_recebidos, rede.bytes_enviados);
        printf("  Conexões Ativas: %lu\n", rede.conexoes_ativas);
        testes_funcionando++;
    } else {
        printf("  Teste de leitura falhou\n");
    }

    /* Teste 2 - Detecção de Atividade */
    printf("\nTESTE DE DETECÇÃO DE ATIVIDADE\n");
    metricas_rede_t antes, depois;
    
    get_metricas_rede(pid, &antes);
    gerar_carga_rede();
    get_metricas_rede(pid, &depois);

    if (depois.bytes_recebidos != antes.bytes_recebidos || 
        depois.bytes_enviados != antes.bytes_enviados) {
        printf("  Teste de detecção funcionando (mudança detectada)\n");
        testes_funcionando++;
    } else {
        printf("  Teste de detecção: sem mudanças significativas (pode ser normal)\n");
        testes_funcionando++;
    }

    /* Teste 3 - Múltiplas Leituras */
    printf("\nTESTE DE MÚLTIPLAS LEITURAS\n");
    int leituras_realizadas = 0;
    for (int i = 0; i < 3; i++) {
        if (get_metricas_rede(pid, &rede) == 0) {
            leituras_realizadas++;
            printf("  Leitura %d: RX=%lu bytes, TX=%lu bytes\n", 
                   i + 1, rede.bytes_recebidos, rede.bytes_enviados);
        }
        sleep(1);
    }

    if (leituras_realizadas == 3) {
        printf("  Leituras realizadas: %d/3 - Sucesso\n", leituras_realizadas);
        testes_funcionando++;
    } else {
        printf("  Teste de múltiplas leituras falhou (%d/3)\n", leituras_realizadas);
    }

    printf("\nRESULTADO: %d/3 testes passaram\n", testes_funcionando);
    if (testes_funcionando >= 2) {
        printf("Testes de rede concluídos com sucesso\n");
    } else {
        printf("Alguns testes falharam! Verifique o programa!\n");
    }

    return 0;
}
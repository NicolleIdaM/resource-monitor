#include "../include/monitor.h"
#include <math.h>
#include <sys/time.h>

static struct{
    unsigned long ultimo_tempo_total;
    unsigned long ultimo_tempo_processo;
    int primeira_chamada;
}estado_cpu = {0, 0, 1};
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
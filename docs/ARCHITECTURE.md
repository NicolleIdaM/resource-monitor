# Estrutura do Projeto
```bash
resource-monitor/
├── include/                    # Headers (.h)
│   ├── monitor.h               # Métricas gerais
│   ├── namespace.h             # Funções de namespace
│   └── cgroup.h                # Funções de cgroup
├── src/                        # Código fonte (.c)
│   ├── output/                 # Binários compilados
│   │   └── *.o                 # Objetos compilados
│   ├── main.c                  # Programa principal
│   ├── cpu_monitor.c           # Monitor de CPU
│   ├── memory_monitor.c        # Monitor de memória
│   ├── io_monitor.c            # Monitor de I/O
│   ├── network_monitor.c       # Monitor de rede
│   ├── namespace_analyzer.c    # Analisador de namespaces
│   └── cgroup_manager.c        # Gerenciador de cgroups
├── tests/                      # Testes unitários
│   ├── output/                 # Testes compilados
│   │   ├── test_cpu            # Teste de CPU
│   │   ├── test_memory         # Teste de memória
│   │   ├── test_io             # Teste de I/O
│   │   └── test_network        # Teste de rede
│   ├── test_cpu.c
│   ├── test_memory.c
│   ├── test_io.c
│   └── test_network.c
└── scripts/                    # Scripts auxiliares
    └── visualize.py            # Dashboard Python
```

# Arquitetura do Sistema de Monitoramento

## Visão Geral

O Resource Monitor é um sistema de profiling e análise que utiliza primitivas do kernel Linux (namespaces e cgroups) para monitorar, limitar e analisar o uso de recursos por processos e containers.

## Componentes Principais

### 1. Resource Profiler
**Arquivos:** `src/cpu_monitor.c`, `src/memory_monitor.c`, `src/io_monitor.c`, `src/network_monitor.c`

**Funcionalidades:**
- Coleta métricas de CPU (user time, system time, context switches)
- Monitora uso de memória (RSS, VSZ, page faults)
- Analisa I/O (bytes lidos/escritos, operações de disco)
- Monitora rede (bytes rx/tx, pacotes, conexões)
- Exporta dados em CSV/JSON para análise

**Interfaces:**
- `/proc/[pid]/stat` - Estatísticas de processo
- `/proc/[pid]/io` - Métricas de I/O
- `/proc/[pid]/net/dev` - Estatísticas de rede
- `/proc/[pid]/status` - Status de memória

### 2. Namespace Analyzer
**Arquivo:** `src/namespace_analyzer.c`

**Funcionalidades:**
- Lista todos os namespaces ativos no sistema
- Mapeia processos por namespace
- Compara namespaces entre processos
- Mede overhead de criação de namespaces
- Gera relatórios de isolamento

**Interfaces:**
- `/proc/[pid]/ns/` - Namespaces do processo
- Syscalls: `setns()`, `unshare()`
- `/proc/[pid]/status` (NSpid field)

### 3. Control Group Manager
**Arquivo:** `src/cgroup_manager.c`

**Funcionalidades:**
- Lê métricas de cgroups (CPU, Memory, BlkIO)
- Cria e remove cgroups experimentais
- Aplica limites de recursos (CPU, memória, I/O)
- Move processos entre cgroups
- Mede precisão de throttling

**Interfaces:**
- `/sys/fs/cgroup/` - Hierarquia de cgroups
- `cpu.cfs_quota_us`, `memory.limit_in_bytes`
- `cgroup.procs` - Gerenciamento de processos
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
- CPU Monitor: Coleta tempo de usuário/sistema, porcentagem de CPU, context switches e threads
- Memory Monitor: Monitora RAM, memória virtual, page faults e swap
- I/O Monitor: Analisa bytes lidos/escritos, syscalls e operações de disco
- Network Monitor: Monitora bytes recebidos/enviados, pacotes e conexões ativas

**Interfaces:**
- `/proc/[pid]/stat` - Estatísticas de processo
- `/proc/[pid]/statm` - Uso de memória
- `/proc/[pid]/io` - Métricas de I/O
- `/proc/[pid]/net/dev` - Estatísticas de rede
- `/proc/[pid]/status` - Status de memória

### 2. Namespace Analyzer
**Arquivo:** `src/namespace_analyzer.c`

**Funcionalidades:**
- Obtém informações de todos os namespaces de um processo (PID, User, Mount, Network, UTS, IPC)
- Compara namespaces entre processos diferentes
- Procura processos em um namespace específico
- Lista namespaces do sistema (init, processo atual)
- Mede overhead de criação de namespaces

**Interfaces:**
- `/proc/[pid]/ns/` - Links simbólicos para namespaces
- Syscalls: `setns()`, `unshare()`
- `stat()` para obter inodes dos namespaces

### 3. Control Group Manager
**Arquivo:** `src/cgroup_manager.c`

**Funcionalidades:**
- Detecta automaticamente versão do cgroup (v1/v2)
- Cria e remove cgroups experimentais
- Aplica limites de CPU, memória e I/O
- Move processos entre cgroups
- Coleta métricas de uso de cgroups
- Implementa experimentos de throttling

**Interfaces:**
- `/sys/fs/cgroup/` - Hierarquia de cgroups
- `cpu.cfs_quota_us`, `memory.limit_in_bytes` V1
- `cpu.weight`, `memory.max` V2
- `cgroup.procs` - Gerenciamento de processos
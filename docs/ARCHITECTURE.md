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
# Resource Monitor - Sistema de Profiling e Análise de Recursos

## Descrição do Projeto

Sistema de profiling e análise que permite monitorar, limitar e analisar o uso de recursos por processos e containers, explorando as primitivas do kernel Linux que tornam a containerização possível.

O sistema oferece funcionalidades completas para:
- **Monitoramento em tempo real** de CPU, memória, I/O e rede
- **Análise de namespaces** do Linux para isolamento de processos
- **Gerenciamento de cgroups** para limitação de recursos
- **Experimentos automatizados** que demonstram os conceitos de containerização
- **Interface visual** para acompanhamento de métricas do sistema

## Requisitos e Dependências

### Sistema Operacional
- **VirtualBox** (para usar Linux em ambiente virtualizado)
- **Ubuntu 24.04 LTS** ou superior
- Kernel Linux com suporte a namespaces e cgroups

### Linguagens e Compiladores
- **C** (linguagem principal)
- **Python 3** (para interface visual)
- **GCC/G++** (compilador C)
- **Make** (sistema de build)

### Bibliotecas e Dependências
```bash
# Bibliotecas C essenciais
sudo apt update
sudo apt install build-essential gcc g++ make

# Bibliotecas para desenvolvimento Python
sudo apt install python3 python3-pip python3-tk

# Bibliotecas Python para interface gráfica
pip3 install matplotlib psutil
```

# Instruções de Compilação
## Compilação dos Testes
```bash
# Remova arquivo de built
make clean
make

## Compilar todos os teste juntos
make test-all

## Compilar teste separadamente
make test-cpu
make test-memory
make test-io
```

# Instruções de Uso
## Execução Básica
```bash
# Executar o monitor principal
./resource-monitor

# Mostrar ajuda
./resource-monitor --help
```

## Modo de Monitoramento
```bash
# Monitorar processo específico
./resource-monitor -p 1234 -i 500 -n 20

# Monitoramento contínuo do processo atual
./resource-monitor -m -i 1000

# Monitoramento detalhado
./resource-monitor -m -d -p 5678
```

## Análise de Namespaces
```bash
# Listar namespaces do sistema
.resource-monitor -s listar

# Comparar namespaces de dois processos
./resource-monitor -s "comparar 1,1234"

# Procurar processos em um namespace específico
./resource-monitor -s "procurar pid,4026531836"
```

## Gerenciamento de Cgroups
```bash
# Listar cgroups existentes
sudo .resource-monitor -c listar

# Criar novo cgroup
sudo ./resource-monitor -c "criar meu-grupo"

# Remover cgroup
sudo ./resource-monitor -c "remover meu-grupo"
```

## Execução de Experimentos
```bash
# Executar todos os experimentos obrigatórios
sudo ./resource-monitor -e

# Experimentos individuais (via menu interativo)
./resource-monitor
# depois selecione opção 3
```

## Interface Visual
```bash
# Executar dashboard visual (requer Python)
python3 scripts/visualize.py

# Ou navegue até o diretório scripts e execute:
cd scripts
python3 visualize.py
```

# Exemplos de Uso
## Exemplo 1: Monitoramento de Processo Web
```bash
# Encontrar PID do servidor web
pgrep nginx
# Suponha que retorne 8842

# Monitorar a cada 2 segundos por 30 iterações
./resource-monitor -p 8842 -i 2000 -n 30 -m
```

## Exemplo 2: Análise de Container Docker
```bash
# Encontrar PIDs do container
docker inspect --format '{{.State.Pid}}' meu-container
# Suponha que retorne 14523

# Analisar namespaces do container
./resource-monitor -s "comparar 1,14523"

# Monitorar recursos do container
./resource-monitor -p 14523 -m -i 1000
```

## Exemplo 3: Limitação de Recursos com Cgroups
```bash
# Criar cgroup para aplicação
sudo ./resource-monitor -c "criar app-limite"

# Mover processo para cgroup (substitua PID)
sudo ./resource-monitor -c "mover app-limite 5678"

# Aplicar limites (via código ou manualmente)
# Limitar CPU para 0.5 cores
echo "50000" > /sys/fs/cgroup/cpu/app-limite/cpu.cfs_quota_us

# Limitar memória para 100MB
echo "100000000" > /sys/fs/cgroup/memory/app-limite/memory.limit_in_bytes
```

## Exemplo 4: Dashboard em Tempo Real
```bash
# Terminal 1 - Executar aplicação pesada
./test_cpu &

# Terminal 2 - Iniciar monitoramento visual
python3 scripts/visualize.py

# Terminal 3 - Monitorar via CLI
./resource-monitor -p $(pgrep test_cpu) -m -i 500
```

# Autores e Contribuição
**Nicolle Ida Muller**  
    • Desenvolveu o projeto integralmente de forma independente  
    • Implementou todos os módulos de monitoramento  
    • Criou o sistema de namespaces e cgroups  
    • Desenvolveu a interface visual em Python  
    • Realizou testes e validações do sistema
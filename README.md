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
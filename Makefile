CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -I./include -D_DEFAULT_SOURCE -D_GNU_SOURCE
LDFLAGS = -lm
TARGET = resource-monitor

# Diretórios
SRC_DIR = src
TEST_DIR = tests
SRC_OUTPUT_DIR = $(SRC_DIR)/output
TEST_OUTPUT_DIR = $(TEST_DIR)/output
BIN_DIR = .

# Fontes principais
SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/cpu_monitor.c $(SRC_DIR)/memory_monitor.c \
          $(SRC_DIR)/io_monitor.c $(SRC_DIR)/namespace_analyzer.c \
          $(SRC_DIR)/cgroup_manager.c $(SRC_DIR)/network_monitor.c

# Objetos principais
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(SRC_OUTPUT_DIR)/%.o)

# Fontes de teste
TEST_SOURCES = $(TEST_DIR)/test_cpu.c $(TEST_DIR)/test_memory.c \
               $(TEST_DIR)/test_io.c $(TEST_DIR)/test_network.c

# Executáveis de teste
TEST_TARGETS = $(TEST_OUTPUT_DIR)/test-cpu $(TEST_OUTPUT_DIR)/test-memory \
               $(TEST_OUTPUT_DIR)/test-io $(TEST_OUTPUT_DIR)/test-network

# Criar diretórios necessários
$(shell mkdir -p $(SRC_OUTPUT_DIR) $(TEST_OUTPUT_DIR))

# Alvo principal (fica no diretório raiz)
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $^ $(LDFLAGS)
	@echo "Executável principal criado: $(TARGET)"

# Regra para objetos principais (em src/output/)
$(SRC_OUTPUT_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compilado: $@"

# Testes (em tests/output/)
$(TEST_OUTPUT_DIR)/test-cpu: $(TEST_DIR)/test_cpu.c $(SRC_OUTPUT_DIR)/cpu_monitor.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_OUTPUT_DIR)/test-memory: $(TEST_DIR)/test_memory.c $(SRC_OUTPUT_DIR)/memory_monitor.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_OUTPUT_DIR)/test-io: $(TEST_DIR)/test_io.c $(SRC_OUTPUT_DIR)/io_monitor.o
	$(CC) $(CFLAGS) -o $@ $^

$(TEST_OUTPUT_DIR)/test-network: $(TEST_DIR)/test_network.c $(SRC_OUTPUT_DIR)/network_monitor.o
	$(CC) $(CFLAGS) -o $@ $^

# Alvos de teste
test-cpu: $(TEST_OUTPUT_DIR)/test-cpu
	@echo "=== EXECUTANDO TESTE CPU ==="
	@$(TEST_OUTPUT_DIR)/test-cpu || echo "Teste CPU falhou"

test-memory: $(TEST_OUTPUT_DIR)/test-memory
	@echo "=== EXECUTANDO TESTE MEMORY ==="
	@$(TEST_OUTPUT_DIR)/test-memory || echo "Teste Memory falhou"

test-io: $(TEST_OUTPUT_DIR)/test-io
	@echo "=== EXECUTANDO TESTE IO ==="
	@$(TEST_OUTPUT_DIR)/test-io || echo "Teste IO falhou"

test-network: $(TEST_OUTPUT_DIR)/test-network
	@echo "=== EXECUTANDO TESTE NETWORK ==="
	@$(TEST_OUTPUT_DIR)/test-network || echo "Teste Network falhou"

test-all: test-cpu test-memory test-io test-network
	@echo "====================================="
	@echo "=== TODOS OS TESTES CONCLUÍDOS ==="

# Limpeza
clean:
	rm -rf $(SRC_OUTPUT_DIR) $(TEST_OUTPUT_DIR)
	@echo "Arquivos de build removidos"

clean-all: clean
	rm -f $(TARGET) *~ core
	@echo "Limpeza completa realizada"

# Informação
info:
	@echo "=== ESTRUTURA DO PROJETO ==="
	@echo "Objetos em: $(SRC_OUTPUT_DIR)"
	@echo "Testes em: $(TEST_OUTPUT_DIR)"
	@echo "Executável principal: $(TARGET)"
	@echo "Fontes: $(words $(SOURCES)) arquivos"
	@echo "Testes: $(words $(TEST_SOURCES)) arquivos"

# Alvos phony
.PHONY: clean clean-all test-all test-cpu test-memory test-io test-network info
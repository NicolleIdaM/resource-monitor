CC = gcc
CFLAGS = -Wall -Wextra -std=c2x -g -I./include
TARGET = resource-monitor
SOURCES = src/main.c src/cpu_monitor.c src/memory_monitor.c src/io_monitor.c \
          src/namespace_analyzer.c src/cgroup_manager.c

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

test-cpu: tests/test_cpu.c src/cpu_monitor.c
	$(CC) $(CFLAGS) -o test-cpu $^ -lm

test-memory: tests/test_memory.c src/memory_monitor.c
	$(CC) $(CFLAGS) -o test-memory $^ -lm

test-io: tests/test_io.c src/io_monitor.c
	$(CC) $(CFLAGS) -o test-io $^

test-all: test-cpu test-memory test-io
	@echo "=== EXECUTANDO TESTES AUTOMÁTICOS ==="
	@./test-cpu
	@./test-memory
	@./test-io

clean:
	rm -f $(TARGET) test-cpu test-memory test-io

.PHONY: clean test-all
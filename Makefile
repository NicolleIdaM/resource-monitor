CC = gcc
CFLAGS = -Wall -Wextra -std=c2x -g -I./include
TARGET = resource-monitor
SOURCES = src/main.c src/cpu_monitor.c src/memory_monitor.c src/io_monitor.c \
          src/namespace_analyzer.c src/cgroup_manager.c src/export_csv.c

#Executável
$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

#Testes individuais
test-cpu: tests/test_cpu.c src/cpu_monitor.c
	$(CC) $(CFLAGS) -o test-cpu $^

test-memory: tests/test_memory.c src/memory_monitor.c
	$(CC) $(CFLAGS) -o test-memory $^

test-io: tests/test_io.c src/io_monitor.c
	$(CC) $(CFLAGS) -o test-io $^

#Verifica memory leaks
memcheck: $(TARGET)
	valgrind --leak-check=full --track-origins=yes ./$(TARGET) $$(pidof bash)

#Limpeza
clean:
	rm -f $(TARGET) test-cpu test-memory test-io

.PHONY: clean memcheck
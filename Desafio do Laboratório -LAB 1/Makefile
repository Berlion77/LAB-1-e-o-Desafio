# Compilador
CC = gcc
CFLAGS = -Wall -Wextra -pthread
LDFLAGS = -pthread

# Nome do executável principal
TARGET = cpd-terminal

# Programa de teste
FIBONACCI = fibonacci

# Arquivos fonte
SOURCES = cpd-terminal.c
FIB_SOURCES = fibonacci.c

# Arquivos objeto
OBJECTS = $(SOURCES:.c=.o)
FIB_OBJECTS = $(FIB_SOURCES:.c=.o)

# Regra padrão
all: $(TARGET) $(FIBONACCI)

# Regra para criar o executável principal
$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJECTS)

# Regra para criar o programa fibonacci
$(FIBONACCI): $(FIB_OBJECTS)
	$(CC) -o $(FIBONACCI) $(FIB_OBJECTS)

# Regra para compilar arquivos .c em .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Regra para limpar arquivos gerados
clean:
	rm -f $(OBJECTS) $(FIB_OBJECTS) $(TARGET) $(FIBONACCI)

# Regra para limpar e recompilar
rebuild: clean all

# Regra para executar
run: $(TARGET)
	./$(TARGET)

# Declaração de phony targets
.PHONY: all clean rebuild run
TARGET = pingpongos

SRC = ppos-core-aux.c pingpong-scheduler-srtf.c

LIB = libppos_static.a

CC = gcc

CFLAGS = -o $(TARGET)

all: build run

build:
	$(CC) $(CFLAGS) $(SRC) $(LIB)

run: build
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all build run clean


# Compilar e Executar: make
# Somente Compilar: make build
# Somente Executar: make run
# Limpar: make clean
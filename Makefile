#pratical mode:
# all:
# 	gcc -Wall -c common.c
# 	gcc -Wall client.c common.o -o client
# 	gcc -Wall server.c common.o -o server

# Compilador e flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I./src
# Nomes dos executáveis
CLIENT = client
SERVER = server
# Pasta de saída
BIN_DIR = bin
# Arquivos fonte
CLIENT_SRC = src/client.c src/common.c
SERVER_SRC = src/server.c src/common.c
# Regra padrão (executada com 'make')
all: create_bin $(BIN_DIR)/$(CLIENT) $(BIN_DIR)/$(SERVER)
# Cria a pasta bin/
create_bin:
	mkdir -p $(BIN_DIR)
# Compila o cliente
$(BIN_DIR)/$(CLIENT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $@ $^
# Compila o servidor
$(BIN_DIR)/$(SERVER): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $@ $^
# Limpa os binários
clean:
	rm -rf $(BIN_DIR)
.PHONY: all create_bin clean
#pratical mode:
# all:
# 	gcc -Wall -c common.c
# 	gcc -Wall client.c common.o -o client
# 	gcc -Wall server.c common.o -o server
# Compilador e flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I./src
LDFLAGS = -lm -lpthread  # Added math and pthread libraries

# Nomes dos executáveis
CLIENT = client
SERVER = server

# Pasta de saída
BIN_DIR = bin

# Arquivos fonte
CLIENT_SRC = src/client.c src/common.c
SERVER_SRC = src/server.c src/common.c

# Regra padrão (executada com 'make')
all: clean_old create_bin $(BIN_DIR)/$(CLIENT) $(BIN_DIR)/$(SERVER)

# Remove os executáveis antigos se existirem
clean_old:
	rm -f $(BIN_DIR)/$(CLIENT) $(BIN_DIR)/$(SERVER)

# Cria a pasta bin/
create_bin: clean_old
	mkdir -p $(BIN_DIR)

# Compila o cliente (sem pthread/math, se não for necessário)
$(BIN_DIR)/$(CLIENT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $@ $^

# Compila o servidor (com pthread e math)
$(BIN_DIR)/$(SERVER): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Limpa os binários
clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean_old create_bin clean
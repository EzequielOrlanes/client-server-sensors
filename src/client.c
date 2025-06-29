#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <stdbool.h>
#include "common.h"
#include <pthread.h>
#include <sys/select.h>  
#include <unistd.h>      

#define BUFSZ 500
typedef enum {
    MSG_START,
    MSG_CLOSED,
    MSG_BET,
    MSG_CASHOUT,
    MSG_MULTIPLIER,
    MSG_EXPLODE,
    MSG_PAYOUT,
    MSG_PROFIT,
    MSG_BYE,
    MSG_ERROR
} MessageType;

typedef struct {
    int type;
    int player_id;
    float value;
    float player_profit;
    float house_profit;
    char message[BUFSZ];
} GameMessage;

void usage(int argc, char **argv) {
    printf("usage: %s <server IP> <server port> -nick <apelido>\n", argv[0]);
    printf("example: %s 127.0.0.1 51511 -nick Flip\n", argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    //  Verificar argumentos
    if (argc != 5) {
        if(argc > 5){
        printf("Error: Invalid number of arguments\n");
        exit(EXIT_FAILURE);
        };
        if(strcmp(argv[3], "-nick") != 0){
        printf("Error: Expected '-nick' argument\n ");
        exit(EXIT_FAILURE);
        }; 
        printf("Error: Invalid number of arguments\n");
        exit(EXIT_FAILURE);  
    }
    // Verificar tamanho do apelido
    if (strlen(argv[4]) > 13) {
        printf("Error: Nickname too long (max 13)\n");
        exit(EXIT_FAILURE);
    }
    // Configurar endereço do servidor
    struct sockaddr_storage storage;
    if (0 != addrparse(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }
    // Criar socket e conectar
    int sock = socket(storage.ss_family, SOCK_STREAM, 0);
    if (sock == -1) {
        logexit("socket");
    }
    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (connect(sock, addr, sizeof(storage)) != 0) {
        logexit("connect");
    }
    printf("Conectado ao servidor.\n");
    // Enviar nickname
    GameMessage msg = {0};
    msg.type = MSG_START;
    strncpy(msg.message, argv[4], BUFSZ - 1);
    send(sock, &msg, sizeof(GameMessage), 0);
    bool running = true;
    bool can_bet = false;
    bool can_cashout = false;
    bool has_bet = false;
    bool flag_cashout = false;
while (running) {
    if (recv(sock, &msg, sizeof(GameMessage), 0) <= 0) {
        printf("Conexão com o servidor perdida.\n");
        break;
    } 
    else{
    switch (msg.type) {
        case MSG_START:
            printf("\n%s\n", msg.message);
            can_bet = true;
            can_cashout = false;
            has_bet = false;
            break;
        case MSG_CLOSED:
            printf("\n%s\n", msg.message);
            can_bet = false; // Desativa novas apostas
            if (has_bet) {   // Só mostra cashout se apostou
                printf("\nDigite [C] para sacar\n");
                can_cashout = true;
                continue;
            }
            break;
        case MSG_BET:
            printf("\n%s\n", msg.message);
            break;
        case MSG_MULTIPLIER:
            printf("\nMultiplicador atual: %.2fx\n", msg.value);
            can_cashout = true;
            if (flag_cashout == true){
                break;
            } else {
                  goto q;
            }
            break;
        case MSG_CASHOUT:
            printf("\n%s\n", msg.message);
            can_cashout = false;
            break;
        case MSG_EXPLODE:
            printf("\n%s\n", msg.message);
            can_cashout = false;
            continue;
        case MSG_PAYOUT:
            printf("\n%s\n", msg.message);
            continue;
        case MSG_PROFIT:
            printf("\n%s\n", msg.message);
            continue;
        case MSG_BYE:
            printf("\n%s\n", msg.message);
            running = false;
            break;
        case MSG_ERROR:
            printf("\n%s\n", msg.message);
            break;
        default:
            continue;
    }
}
    // Processar entrada do usuário APENAS se for o momento certo
    if (can_bet && has_bet == false ) {
        printf("\n$");
        fflush(stdout);
        char input[BUFSZ];
        if (fgets(input, BUFSZ, stdin) == NULL) {
            break;
        }
        input[strcspn(input, "\n")] = '\0';
        
        if (strcmp(input, "Q") == 0) {
            msg.type = MSG_BYE;
            send(sock, &msg, sizeof(GameMessage), 0);
            running = false;
            break;
        } 
        else {
            float bet = atof(input);
            if (bet <= 0) {
                printf("Error: Valor de aposta inválido\n");
            } else {
                msg.type = MSG_BET;
                msg.value = bet;
                has_bet = true;
                send(sock, &msg, sizeof(GameMessage), 0);
            }
        }
    }
    q:
    if (can_cashout && can_bet == false) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    // Timeout = 0 (não espera)
    struct timeval timeout = {0, 0};
    // Verifica se tem algo para ler
    if (select(1, &fds, NULL, NULL, &timeout) > 0) {
        char input[BUFSZ];
        if (fgets(input, BUFSZ, stdin) == NULL) continue;
        input[strcspn(input, "\n")] = '\0';  // Remove o \n
        
        if (strcmp(input, "C") == 0 || strcmp(input, "c") == 0) {
            msg.type = MSG_CASHOUT;
            send(sock, &msg, sizeof(GameMessage), 0);
        }
        else if (strcmp(input, "Q") == 0 || strcmp(input, "q") == 0) {
            msg.type = MSG_BYE;
            send(sock, &msg, sizeof(GameMessage), 0);
            running = false;
            flag_cashout = true;
            break;
        }
        // else {
        //     printf("Comando inválido. Use [C] ou [Q].\n");
        // }
    }
    
    }
}
    close(sock);
    return 0;
}
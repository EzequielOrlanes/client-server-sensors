#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include "common.h"

#define BUFSZ 500
#define B127CKLOG 1
#define MSG_SIZE 256

typedef enum {
    MSG_REQUEST,
    MSG_RESPONSE,
    MSG_RESULT,
    MSG_PLAY_AGAIN_REQUEST,
    MSG_PLAY_AGAIN_RESPONSE,
    MSG_ERROR,
    MSG_END
} MessageType;

typedef struct {
    int type;
    int client_action;
    int server_action;
    int result;
    int client_wins;
    int server_wins;
    char message[MSG_SIZE];
} GameMessage;


int determine_result(int client_action, int server_action) {
    if (client_action == server_action) return -1;
    switch (client_action) {
        case 0: return (server_action == 2 || server_action == 3) ? 1 : 0;
        case 1: return (server_action == 0 || server_action == 4) ? 1 : 0;
        case 2: return (server_action == 1 || server_action == 3) ? 1 : 0;
        case 3: return (server_action == 1 || server_action == 4) ? 1 : 0;
        case 4: return (server_action == 0 || server_action == 2) ? 1 : 0;
        default: return 0;
    }
}

void send_message(int sock, GameMessage *msg) {
    send(sock, msg, sizeof(GameMessage), 0);
}

int receive_message(int sock, GameMessage *msg) {
    return recv(sock, msg, sizeof(GameMessage), 0);
   
}

void print_action(int action, char *buf) {
    const char *actions[] = {
        "Nuclear Attack", "Intercept Attack", "Cyber Attack",
        "Drone Strike", "Bio Attack"
    };
    if (action >= 0 && action <= 4) strcpy(buf, actions[action]);
    else strcpy(buf, "Invalid");
}

void usage(int argc, char **argv){
    printf("usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}



int main(int argc, char *argv[]) {


 if (argc < 3)
    {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage))
    {
        usage(argc, argv);
    }

    int sock;
    sock = socket(storage.ss_family, SOCK_STREAM, 0);
    if (sock == -1)
    {
        logexit("socket");
    }

    int enable = 1;
    if (0 != setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)))
    {
        logexit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != bind(sock, addr, sizeof(storage))){
        logexit("bind");
    }

    if (0 != listen(sock, 10)){
        logexit("listen");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);

// init:

    printf("bound to %s, waiting connections\n", addrstr);
    struct sockaddr_storage cstorage;
    struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
    socklen_t caddrlen = sizeof(cstorage);
    int sock = accept(sock, caddr, &caddrlen);
    if (sock == -1)
    {
        logexit("accept");
    }

    char caddrstr[BUFSZ];
    addrtostr(caddr, caddrstr, BUFSZ);
    printf("[log] connection from %s\n", caddrstr);
    // char ip_version[10];
    int client_wins = 0, server_wins = 0;


    while (1) {
        printf("Entrou aqui Server");
//  goto init;
        GameMessage msg = {0};
        // Solicita jogada
        msg.type = MSG_REQUEST;
        send_message(sock, &msg);

        // Recebe jogada
        if (receive_message(sock, &msg) <= 0) printf("UI");
        if (msg.client_action < 0 || msg.client_action > 4) {
            msg.type = MSG_ERROR;
            snprintf(msg.message, MSG_SIZE, "Por favor, selecione um valor de 0 a 4.");
            send_message(sock, &msg);
            continue;
        }

        int server_action = rand() % 5;
        int result = determine_result(msg.client_action, server_action);

        msg.type = MSG_RESULT;
        msg.server_action = server_action;
        msg.result = result;

        char client_str[32], server_str[32];
        print_action(msg.client_action, client_str);
        print_action(server_action, server_str);

        if (result == 1) {
            snprintf(msg.message, MSG_SIZE, "Você escolheu: %s\nServidor escolheu: %s\nResultado: Vitória!", client_str, server_str);
            client_wins++;
        } else if (result == 0) {
            snprintf(msg.message, MSG_SIZE, "Você escolheu: %s\nServidor escolheu: %s\nResultado: Derrota!", client_str, server_str);
            server_wins++;
        } else {
            snprintf(msg.message, MSG_SIZE, "Você escolheu: %s\nServidor escolheu: %s\nResultado: Empate!", client_str, server_str);
        }

        msg.client_wins = client_wins;
        msg.server_wins = server_wins;
        send_message(sock, &msg);

        if (result == -1) continue;

        // Jogar novamente?
        msg.type = MSG_PLAY_AGAIN_REQUEST;
        send_message(sock, &msg);

        if (receive_message(sock, &msg) <= 0) printf("receu");
        if (msg.type != MSG_PLAY_AGAIN_RESPONSE || (msg.result != 0 && msg.result != 1)) {
            msg.type = MSG_ERROR;
            snprintf(msg.message, MSG_SIZE, "Por favor, digite 1 para jogar novamente ou 0 para encerrar.");
            send_message(sock, &msg);
            continue;
        }

        if (msg.result == 0) {
            msg.type = MSG_END;
            snprintf(msg.message, MSG_SIZE, "Fim de jogo!\nPlacar final: Você %d x %d Servidor\nObrigado por jogar!", client_wins, server_wins);
            msg.client_wins = client_wins;
            msg.server_wins = server_wins;
            send_message(sock, &msg);
            // break;
        }
    }

    // close(sock);
    printf("Cliente desconectado.\n");
    // return 0;
}

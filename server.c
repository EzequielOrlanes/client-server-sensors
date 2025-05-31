#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "common.h"

#define BUFSZ 500
#define MSG_SIZE 256
#define EQUAL 100

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
    if (client_action == server_action) return EQUAL;
    switch (client_action) {
        case 0: return (server_action == 2 || server_action == 3) ? 1 : 0;
        case 1: return (server_action == 0 || server_action == 4) ? 1 : 0;
        case 2: return (server_action == 1 || server_action == 3) ? 1 : 0;
        case 3: return (server_action == 1 || server_action == 4) ? 1 : 0;
        case 4: return (server_action == 0 || server_action == 2) ? 1 : 0;
    }
    return 0;
}

void print_action(int action, char *buf) {
    char * possible_actions[] = {
        "Nuclear Attack", 
        "Intercept Attack",
        "Cyber Attack",
        "Drone Strike",
        "Bio Attack"
    };
    if (action >= 0 && action <= 4){
        strcpy(buf, possible_actions[action]);
    } else {
            strcpy(buf, "Invalid");
      }
}


void usage(int argc, char **argv){
    printf("usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc < 3){
        usage(argc, argv);
    }
    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage)){
        usage(argc, argv);
    }

    int sock_connect;
    sock_connect = socket(storage.ss_family, SOCK_STREAM, 0);
    int opt = 0;
    setsockopt(sock_connect, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt)); 
    if (sock_connect == -1) logexit("socket");

    int enable = 1;
    if (0 != setsockopt(sock_connect, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) logexit("setsockopt");

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != bind(sock_connect, addr, sizeof(storage))) logexit("bind");
    if (0 != listen(sock_connect, 10)) logexit("listen");

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    char protocol[10]; // "IPv4" ou "IPv6"
    int port = 0;

    if (addr->sa_family == AF_INET) {
        // IPv4
        struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;
        port = ntohs(addr4->sin_port);
        strcpy(protocol, "IPv4");
    } else if (addr->sa_family == AF_INET6) {
        // IPv6
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr;
        port = ntohs(addr6->sin6_port);
        strcpy(protocol, "IPv6");
    } else {
        strcpy(protocol, "Unknown");
    }

    printf("Servido iniciado em modo %s, na porta %d aguardando conexão...\n", protocol, port);
    struct sockaddr_storage cstorage;
    struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
    socklen_t caddrlen = sizeof(cstorage);
    int sock = accept(sock_connect, caddr, &caddrlen);
    if (sock == -1){
        logexit("accept");
    }
    printf("Cliente conectado.\n");
    printf("Apresentando as opções para o cliente.\n");
    int client_wins = 0; 
    int server_wins = 0;

    while (1) {
        GameMessage msg = {0};
        // Solicita jogada
        msg.type = MSG_REQUEST;
        send(sock, &msg, sizeof(GameMessage), 0);
        // Recebe jogada
        if ((recv(sock, &msg, sizeof(GameMessage), 0)) <= 0) break;
        fprintf(stderr, "Cliente escolheu %d.\n", msg.client_action);
        if (msg.client_action < 0 || msg.client_action > 4) {
            printf("Erro: opção inválida de jogada.\n");
            msg.type = MSG_ERROR;
            snprintf(msg.message, MSG_SIZE, "Por favor, selecione um valor de 0 a 4.\n");
            send(sock, &msg, sizeof(GameMessage), 0);
            continue;
        }

        //select random number between 0 and 5 as server action.
        int server_action = rand() % 5;
        printf("Servidor escolheu aleatoriamente %d\n", server_action);
        int result = determine_result(msg.client_action, server_action);

        msg.type = MSG_RESULT;
        msg.server_action = server_action;
        msg.result = result;

        char client_str[32], server_str[32];
        print_action(msg.client_action, client_str);
        print_action(server_action, server_str);
        if (result == 1) {
            snprintf(msg.message, MSG_SIZE,"Você escolheu: %s\n Servidor escolheu: %s\n Resultado: Vitória!\n", client_str, server_str);
            client_wins++;
        } else if (result == 0) {
            snprintf(msg.message, MSG_SIZE,"Você escolheu: %s\n Servidor escolheu: %s\n Resultado: Derrota!\n", client_str, server_str);
            server_wins++;
        } else if (result == EQUAL){
            snprintf(msg.message, MSG_SIZE,"Você escolheu: %s\n Servidor escolheu: %s\n Resultado: Empate!\n", client_str, server_str);
        }

        if(result == EQUAL) printf("Empate.\n");

        msg.client_wins = client_wins;
        msg.server_wins = server_wins;
        fprintf(stderr, "Placar atualizado: Cliente %d x %d Servidor\n", client_wins, server_wins);
        printf("Perguntando se o cliente deseja jogar novamente.\n");
        send(sock, &msg, sizeof(GameMessage), 0);

        if (result == -1) continue;

        // Plays again option choose. 
        msg.type = MSG_PLAY_AGAIN_REQUEST;
        send(sock, &msg, sizeof(GameMessage), 0);


        if ((recv(sock, &msg, sizeof(GameMessage), 0)) <= 0) break;
        printf("Solicitando ao cliente mais uma escolha.\n");

        if (msg.type != MSG_PLAY_AGAIN_RESPONSE || (msg.result != 0 && msg.result != 1)) {
            msg.type = MSG_ERROR;
            snprintf(msg.message, MSG_SIZE, "Por favor, digite 1 para jogar novamente ou 0 para encerrar.");
            send(sock, &msg, sizeof(GameMessage), 0);
            continue;
        }

        if (msg.result == 0) {
            printf("Cliente não deseja jogar novamente.\n");
            printf("Enviando placar final.\n");
            msg.type = MSG_END;
            snprintf(msg.message, MSG_SIZE,"Fim de jogo!\n Placar final: Você %d x %d Servidor\n Obrigado por jogar!", client_wins, server_wins);
            msg.client_wins = client_wins;
            msg.server_wins = server_wins;
            send(sock, &msg, sizeof(GameMessage), 0);
            break;
        }
    }
    close(sock);
    printf("Cliente desconectado.\n");
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include "common.h"
#include <stdbool.h> //bool true or false
#include "common.h"

#define MSG_SIZE 256
#define BUFSZ 500

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


void send_message(int sock, GameMessage *msg) {
    send(sock, msg, sizeof(GameMessage), 0);
}

int receive_message(int sock, GameMessage *msg) {
    return recv(sock, msg, sizeof(GameMessage), 0);
}

void print_menu() {
    printf("Escolha sua jogada: \n");
    printf("0 - Nuclear Attack \n");
    printf("1 - Intercept Attack \n");
    printf("2 - Cyber Attack \n");
    printf("3 - Drone Strike \n");
    printf("4 - Bio Attack \n");
    printf("\n");
    printf("$ ");
    fflush(stdout); //limpa o buffer
}

void usage(int argc, char **argv) {
	printf("usage: %s <server IP> <server port>\n", argv[0]);
	printf("example: %s 127.0.0.1 51511\n", argv[0]);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    //Define adress and check param to init client.
	if (argc < 3) usage(argc, argv);
	struct sockaddr_storage storage;
	if (0 != addrparse(argv[1], argv[2], &storage)) usage(argc, argv);
    //Create socket and establish connection with server.
	int sock;
	sock = socket(storage.ss_family, SOCK_STREAM, 0);
	if (sock == -1) logexit("socket");
	struct sockaddr *addr = (struct sockaddr *)(&storage);
	if (0 != connect(sock, addr, sizeof(storage))) logexit("connect");
    printf("Conectado ao servidor.\n");

    while (1) {
        GameMessage msg = {0};
        if (receive_message(sock, &msg) <= 0) break;
        if (msg.type == MSG_REQUEST) {
            print_menu();
            int escolha;
            scanf("%d", &escolha);
            msg.type = MSG_RESPONSE;
            msg.client_action = escolha;
            send_message(sock, &msg);
        }
        else if (msg.type == MSG_RESULT) {
            printf("\n %s \n", msg.message);
        }
        else if (msg.type == MSG_PLAY_AGAIN_REQUEST) {
            printf("Deseja jogar novamente? \n1 - Sim \n0 - Nao \n");
            int again;
            printf("\n");
            printf("$ ");
            scanf("%d", &again);
            printf("\n");
            msg.type = MSG_PLAY_AGAIN_RESPONSE;
            msg.result = again;
            send_message(sock, &msg);
        }
        else if (msg.type == MSG_ERROR) {
            printf("\n %s \n", msg.message);
        }
        else if (msg.type == MSG_END) {
            printf("\n %s \n", msg.message);
            break;
        }
    }
    close(sock);
    return 0;
}


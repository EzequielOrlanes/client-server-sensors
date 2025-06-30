#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include "common.h"
#include <math.h> 

#define BUFSZ 500
#define MAX_PLAYERS 10
#define BET_TIME 10
#define MULTIPLIER_INTERVAL 1 //segundos

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

typedef struct {
    int id;
    int sock;
    pthread_t thread;
    char nickname[14];
    float bet;
    float profit;
    bool has_bet;
    bool has_cashed_out;
    bool active;
} Player;
//variaveis que precisam ser globais.
Player players[MAX_PLAYERS];
int num_players = 0;
pthread_mutex_t players_mutex = PTHREAD_MUTEX_INITIALIZER;
float current_multiplier = 1.00;
float explosion_point = 0.0;
float total_bets = 0.0;
int num_betting_players = 0;
bool game_active = false;
bool accepting_bets = false;
float house_profit = 0.0;
bool exploded_yet = false;
pthread_mutex_t game_mutex = PTHREAD_MUTEX_INITIALIZER;

//funções de comunicação com o cliente. 
void broadcast_message(GameMessage *msg, bool include_inactive) {
    pthread_mutex_lock(&players_mutex);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players[i].active || !include_inactive) {
            send(players[i].sock, msg, sizeof(GameMessage), 0);
        }
    }
    pthread_mutex_unlock(&players_mutex);
}

void send_message(int sock, GameMessage *msg) {
    send(sock, msg, sizeof(GameMessage), 0);
}

void log_event(const char *event, int player_id, float multiplier, float explosion, 
               int num_players, float total_bets, float bet, float payout, 
               float player_profit, float house_profit) {
    printf("event=%s", event);
    if (player_id != 0) printf(" | id=%d", player_id);
    if (multiplier != 0.0f) printf(" | m=%.2f", multiplier);
    if (explosion != 0.0f) printf(" | me=%.2f", explosion);
    if (num_players != 0) printf(" | N=%d", num_players);
    if (total_bets != 0.0f) printf(" | V=%.2f", total_bets);
    if (bet != 0.0f) printf(" | bet=%.2f", bet);
    if (payout != 0.0f) printf(" | payout=%.2f", payout);
    if (player_profit != 0.0f) printf(" | player_profit=%.2f", player_profit);
    if (house_profit != 0.0f) printf(" | house_profit=%.2f", house_profit);
    printf("\n\n");
}


void calculate_explosion_point() {
    pthread_mutex_lock(&game_mutex);
    explosion_point = sqrtf(1 + num_betting_players + 0.01 * total_bets);
    pthread_mutex_unlock(&game_mutex);
    log_event("closed", 0, 0.0, explosion_point, num_betting_players, total_bets, 
              0.0, 0.0, 0.0, 0.0);
}

void *game_loop(void *arg) {
    // Contagem regressiva para apostas
    GameMessage msg = {0};
    msg.type = MSG_START;
    broadcast_message(&msg, false);
    log_event("start", 0, 0.0, 0.0, num_betting_players, 0.0, 0.0, 0.0, 0.0, 0.0);
    for (int i = BET_TIME; i > 0; i--) {
        sleep(1);
        pthread_mutex_lock(&game_mutex);
        if (!accepting_bets) {
            pthread_mutex_unlock(&game_mutex);
            break;
        }
        pthread_mutex_unlock(&game_mutex);
    }
    pthread_mutex_lock(&game_mutex);
    accepting_bets = false;
    pthread_mutex_unlock(&game_mutex);
    msg.type = MSG_CLOSED;
    snprintf(msg.message, BUFSZ, "Apostas encerradas! Não é mais possível apostar nesta rodada.");    
    broadcast_message(&msg, false);
    calculate_explosion_point();
    // Loop do multiplicador
    current_multiplier = 1.00;
    bool exploded = false;
    while (exploded == false) {
        sleep(MULTIPLIER_INTERVAL);
        pthread_mutex_lock(&game_mutex);
        if (game_active == false) {
            pthread_mutex_unlock(&game_mutex);
            break;
        }
        current_multiplier += 0.01;
        if (current_multiplier >= explosion_point) {
            exploded = true;
            game_active = false;
        }
        msg.type = MSG_MULTIPLIER;
        msg.value = current_multiplier;
        snprintf(msg.message, BUFSZ, "Multiplicador atual: %.2fx", current_multiplier);
        broadcast_message(&msg, false);
        log_event("multiplier", 0, current_multiplier, explosion_point, num_betting_players, 
                  total_bets, 0.0, 0.0, 0.0, house_profit);
        pthread_mutex_unlock(&game_mutex);
    }

 if(exploded == true) {
    pthread_mutex_lock(&game_mutex);
    pthread_mutex_lock(&players_mutex);
    int cont = 0;
        for (int i = 0; i < MAX_PLAYERS; i++) {
            float total_losses = 0.0;
            // Apenas processa se o jogador estiver ativo
            if (players[i].active == false) {
                players[i].has_bet = false;
                players[i].has_cashed_out = false;
                continue;
            }
            // Lógica para jogadores que perderam
            if (players[i].has_bet && players[i].has_cashed_out == false ) {
                cont++;
                players[i].profit -= players[i].bet;
                for (int i = 0; i < num_betting_players; i++) {
                    if (players[i].has_bet && !players[i].has_cashed_out && players[i].active) {
                    total_losses += players[i].bet;
                    }
                }
            }
                house_profit += total_losses; 
                msg.type = MSG_EXPLODE;
                msg.player_id = players[i].id;
                msg.value = current_multiplier;
                msg.player_profit = players[i].profit;
                snprintf(msg.message, BUFSZ, "Você perdeu R$ %.2f.\nTente novamente na próxima rodada! Aviãozinho tá pagando :)", players[i].bet);
                send_message(players[i].sock, &msg);
                log_event("explode", players[i].id, current_multiplier, explosion_point,num_betting_players, total_bets, players[i].bet, 0.0, players[i].profit, 0);
                msg.type = MSG_PROFIT;
                snprintf(msg.message, BUFSZ, "Profit atual: R$ %.2f", players[i].profit);
                send_message(players[i].sock, &msg);

        }
        msg.type = MSG_PROFIT;
        msg.player_id = 0; // Indica a casa
        msg.house_profit = house_profit;
        snprintf(msg.message, BUFSZ, "Profit da casa: R$ %.2f", house_profit);
        broadcast_message(&msg, false);
        log_event("profit", 0, 0.0, 0.0, 0, 0.0, 0.0, 0.0, 0.0, house_profit);
        game_active = true;
        exploded_yet = true;
        pthread_mutex_unlock(&players_mutex);
        pthread_mutex_unlock(&game_mutex);
    }

    // Preparar próxima rodada
    if (game_active == true && exploded_yet == true) {
        pthread_mutex_lock(&game_mutex);
        total_bets = 0.0;
        num_betting_players = 0;
        accepting_bets = true;
        pthread_t game_thread;
        pthread_create(&game_thread, NULL, game_loop, NULL);
        pthread_detach(game_thread);
        pthread_mutex_unlock(&game_mutex);
    }else{
        accepting_bets = false;
    }
    return NULL;
}

void *handle_client(void *data) {
    Player *player = (Player *)data;
    GameMessage msg = {0};
    // Enviar mensagem de boas-vindas

    msg.type = MSG_START;
    if (accepting_bets) {
        snprintf(msg.message, BUFSZ, "Rodada aberta! Digite o valor da aposta ou digite [Q] para sair (%d segundos restantes)", BET_TIME);
    } else if (game_active) {
        snprintf(msg.message, BUFSZ, "Apostas encerradas! Não é mais possível apostar nesta rodada.");
    } else {
        snprintf(msg.message, BUFSZ, "Aguardando início de nova rodada...");
    }
    send_message(player->sock, &msg);
    while (1) {
        if (recv(player->sock, &msg, sizeof(GameMessage), 0) <= 0) break;
        if (msg.type == MSG_BET && accepting_bets) {
            if (msg.value <= 0) {
                msg.type = MSG_ERROR;
                snprintf(msg.message, BUFSZ, "Error: Invalid bet value");
                send_message(player->sock, &msg);
                continue;
            }
            pthread_mutex_lock(&players_mutex);
            player->bet = msg.value;
            player->has_bet = true;
            total_bets += msg.value;
            num_betting_players++;
            msg.type = MSG_BET;
            pthread_mutex_unlock(&players_mutex);
            snprintf(msg.message, BUFSZ, "Aposta recebida: R$ %.2f", msg.value);
            send_message(player->sock, &msg);
            log_event("bet", player->id, 0.0, 0.0, num_betting_players, total_bets, 
                      msg.value, 0.0, 0.0, 0.0);
            
        } 
        //msg.type == MSG_CASHOUT
        else if (game_active && player->has_bet && player->has_cashed_out == false) {
            pthread_mutex_lock(&players_mutex);
            float payout = player->bet * current_multiplier;
            player->profit += (payout - player->bet);
            player->has_cashed_out = true;
            house_profit -= payout;
            msg.type = MSG_CASHOUT;
            msg.value = current_multiplier;
            snprintf(msg.message, BUFSZ, "Você sacou em %.2fx", current_multiplier);
            send_message(player->sock, &msg);
            log_event("cashout", player->id, current_multiplier, explosion_point, 
                      num_betting_players, total_bets, player->bet, 0.0, 0.0, 0.0);
            msg.type = MSG_PAYOUT;
            msg.value = payout;
            snprintf(msg.message, BUFSZ, "Você ganhou R$ %.2f!", payout);
            send_message(player->sock, &msg);
            log_event("payout", player->id, 0.0, 0.0, 0, 0.0, 0.0, payout, 0.0, house_profit);
            msg.type = MSG_PROFIT;
            msg.player_profit = player->profit;
            snprintf(msg.message, BUFSZ, "Profit atual: R$ %.2f", player->profit);
            send_message(player->sock, &msg);
            log_event("profit", player->id, 0.0, 0.0, 0, 0.0, 0.0, 0.0, 
                      player->profit, house_profit);
            pthread_mutex_unlock(&players_mutex);
        } 
        else if (msg.type == MSG_BYE) {
            pthread_mutex_lock(&players_mutex);
            player->active = false;
            pthread_mutex_unlock(&players_mutex);
            msg.type = MSG_BYE;
            snprintf(msg.message, BUFSZ, "Aposte com responsabilidade. Volte logo, %s!", player->nickname);
            send_message(player->sock, &msg);
            log_event("bye", player->id, 0.0, 0.0, 0, 0.0, 0.0, 0.0, 0.0, house_profit);
            printf("Encerrando servidor.");
            return 0;
        } 
        else {
            msg.type = MSG_ERROR;
            snprintf(msg.message, BUFSZ, "Error: Invalid command");
            send_message(player->sock, &msg);
        }
    }
    close(player->sock);
    pthread_mutex_lock(&players_mutex);
    player->active = false;
    num_players--;
    pthread_mutex_unlock(&players_mutex);
    return NULL;
}

void usage(int argc, char **argv) {
    printf("usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        usage(argc, argv);
    }
    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }
    int sock_connect = socket(storage.ss_family, SOCK_STREAM, 0);
    if (sock_connect == -1) logexit("socket");
    // Permitir reuso do endereço
    int enable = 1;
    if (setsockopt(sock_connect, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) != 0) logexit("setsockopt");
    // Configuração para IPv6 
    if (storage.ss_family == AF_INET6) {
        int opt = 0;
        if (setsockopt(sock_connect, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt)) != 0) {
            logexit("setsockopt IPV6_V6ONLY");
        }
    }
    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (bind(sock_connect, addr, sizeof(storage)) != 0) {
        logexit("bind");
    }
    if (listen(sock_connect, 10) != 0) {
        logexit("listen");
    }
    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        players[i].active = false;
        players[i].id = i + 1;
    }
    // Iniciar jogo
    accepting_bets = true;
    game_active = true;
    pthread_t game_thread;
    pthread_create(&game_thread, NULL, game_loop, NULL);
    pthread_detach(game_thread);
    
    // Loop principal para aceitar conexões
    while (1) {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);
        
        int csock = accept(sock_connect, caddr, &caddrlen);
        if (csock == -1) {
            perror("accept");
            continue;
        }
        // Receber nickname do cliente
        GameMessage msg;
        if (recv(csock, &msg, sizeof(GameMessage), 0) <= 0) {
            close(csock);
            continue;
        }
        // Verificar se há espaço para novo jogador
        pthread_mutex_lock(&players_mutex);
        if (num_players >= MAX_PLAYERS) {
            msg.type = MSG_ERROR;
            snprintf(msg.message, BUFSZ, "Servidor cheio. Tente novamente mais tarde.");
            send(csock, &msg, sizeof(GameMessage), 0);
            close(csock);
            pthread_mutex_unlock(&players_mutex);
            continue;
        }
        // Encontrar slot livre
        int player_index = -1;
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (!players[i].active) {
                player_index = i;
                break;
            }
        }
        if (player_index == -1) {
            msg.type = MSG_ERROR;
            snprintf(msg.message, BUFSZ, "Erro interno do servidor.");
            send(csock, &msg, sizeof(GameMessage), 0);
            close(csock);
            pthread_mutex_unlock(&players_mutex);
            continue;
        }
        // Configurar novo jogador
        players[player_index].sock = csock;
        players[player_index].active = true;
        players[player_index].has_bet = false;
        players[player_index].has_cashed_out = false;
        players[player_index].profit = 0.0;
        strncpy(players[player_index].nickname, msg.message, 13);
        players[player_index].nickname[13] = '\0';
        num_players++;
        pthread_mutex_unlock(&players_mutex);
        // Criar thread para o cliente
        pthread_create(&players[player_index].thread, NULL, handle_client, &players[player_index]);
        pthread_detach(players[player_index].thread);
    }
    close(sock_connect);
    return 0;
}
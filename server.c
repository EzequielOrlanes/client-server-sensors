#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h> 

#include <sys/socket.h>
#include <sys/types.h>

#define BUFSZ 500
#define TAM 100
#define MSG_SIZE 256

#define INS_REQ "INS_REQ"
#define REM_REQ "REM_REQ"
#define CH_REQ "CH_REQ"
#define SEN_REQ "SEN_REQ"
#define SEN_RES "SEN_RES"
#define VAL_REQ "VAL_REQ"
#define VAL_RES "VAL_RES"
#define ERROR "ERROR"

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
        int type; // Tipo da mensagem
        int client_action;
        int server_action;
        int result;
        int client_wins;
        int server_wins;
        char message[MSG_SIZE];
} GameMessage;
        



void usage(int argc, char **argv)
{   //It's necessary use command 'make' before type commands below.
    printf("usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

struct sensor
{
    int sensor_id;
    int current;
    int tension;
    int efficiency;
    int potency;
};

struct sensor sensors[TAM];

void remove_sensor(int id)
{
    sensors[id].sensor_id = -1;
    sensors[id].current = '\0';
    sensors[id].efficiency = '\0';
    sensors[id].potency = '\0';
    sensors[id].tension = '\0';
}

int result_potency(int tension, int current)
{
    int result;
    result = (tension * current);
    return result;
}

int aleatory(){
    srand(time(NULL));
    return (rand() % 6);
}

int main(int argc, char **argv)
{

    if (argc < 3)
    {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage))
    {
        usage(argc, argv);
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1)
    {
        logexit("socket");
    }

    int enable = 1;
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)))
    {
        logexit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != bind(s, addr, sizeof(storage)))
    {
        logexit("bind");
    }

    if (0 != listen(s, 10))
    {
        logexit("listen");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
init:

    printf("bound to %s, waiting connections\n", addrstr);
    struct sockaddr_storage cstorage;
    struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
    socklen_t caddrlen = sizeof(cstorage);
    int csock = accept(s, caddr, &caddrlen);
    if (csock == -1)
    {
        logexit("accept");
    }

    char caddrstr[BUFSZ];
    addrtostr(caddr, caddrstr, BUFSZ);
    printf("[log] connection from %s\n", caddrstr);

    while (1)
    {
        char buf[BUFSZ];
        memset(buf, 0, BUFSZ);
        size_t count = recv(csock, buf, BUFSZ, 0);
        if (count == 0)
        {
            close(csock);
            goto init;
        }
        if (count == -1)
        {
            perror("Recv");
            close(csock);
            exit(EXIT_FAILURE);
        }

        char *action;
        char *sensor_id;
        char *current;
        char *tension;
        char *efficiency;
        action = strtok(buf, " ");
        sensor_id = strtok(NULL, " ");
        current = strtok(NULL, " ");
        tension = strtok(NULL, " ");
        efficiency = strtok(NULL, " ");

        // 1)Ligar o sensor
        if ((strcmp(action, INS_REQ) == 0))
        {
            int sensor_ids = strtol(sensor_id, NULL, 10);
            int current_s = strtol(current, NULL, 10);
            int tension_s = strtol(tension, NULL, 10);
            int efficiency_s = strtol(efficiency, NULL, 10);
            sensors[0].sensor_id = '\0';

            int cont;
            for (int i = 0; i < TAM; i++)
            {
                if (sensor_ids == sensors[i].sensor_id && sensors[i].sensor_id != '\0')
                {
                    cont++;
                }
            }

            if (cont > 0)
            {
                memset(buf, 0, BUFSZ);
                sprintf(buf, "sensor already exists");
                count = send(csock, buf, strlen(buf) + 1, 0);
                if (count != strlen(buf) + 1)
                {
                    logexit("send");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    cont = 0;
                }
            }
            else
            {
                sensors[sensor_ids].sensor_id = sensor_ids;
                sensors[sensor_ids].current = current_s;
                sensors[sensor_ids].tension = tension_s;
                sensors[sensor_ids].efficiency = efficiency_s;
                sensors[sensor_ids].potency = result_potency(tension_s, current_s);

                memset(buf, 0, BUFSZ);
                sprintf(buf, "sucessful installation");
                count = send(csock, buf, strlen(buf) + 1, 0);
                if (count != strlen(buf) + 1)
                {
                    logexit("send");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    printf("%s %d %d %d %d\n", INS_REQ, sensor_ids, current_s, tension_s, efficiency_s);
                    continue;
                    memset(buf, 0, BUFSZ);
                }
            }
        }

        // 2) Remove sensor
        if (strcmp(action, REM_REQ) == 0)
        {
            int sensor_ids = strtol(sensor_id, NULL, 10);
            sensors[0].sensor_id = '\0';

            int cont = 0;
            for (int i = 0; i < TAM; i++)
            {
                if (sensor_ids == sensors[i].sensor_id)
                {
                    cont++;
                }
            }

            if (cont <= 0)
            {
                memset(buf, 0, BUFSZ);
                sprintf(buf, "sensor not installed");
                count = send(csock, buf, strlen(buf) + 1, 0);
            }
            else
            {
                remove_sensor(sensor_ids);
                memset(buf, 0, BUFSZ);
                sprintf(buf, "successful removal");
                count = send(csock, buf, strlen(buf) + 1, 0);
                if (count != strlen(buf) + 1)
                {
                    logexit("send");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    printf("%s %d\n", REM_REQ, sensor_ids);
                    continue;
                }
            }
        }

        // 3) Change sensor
        if (strcmp(action, CH_REQ) == 0)
        {
            int sensor_ids = strtol(sensor_id, NULL, 10);
            int current_s = strtol(current, NULL, 10);
            int tension_s = strtol(tension, NULL, 10);
            int efficiency_s = strtol(efficiency, NULL, 10);

            int cont = 0;
            for (int i = 0; i < TAM; i++)
            {
                if (sensor_ids == sensors[i].sensor_id)
                {
                    cont++;
                }
            }
            if (cont == 0)
            {
                memset(buf, 0, BUFSZ);
                sprintf(buf, "sensor not installed");
                count = send(csock, buf, strlen(buf) + 1, 0);
                if (count != strlen(buf) + 1)
                {
                    logexit("send");
                    exit(EXIT_FAILURE);
                    continue;
                }
            }
            else
            {
                sensors[sensor_ids].sensor_id = sensor_ids;
                sensors[sensor_ids].current = current_s;
                sensors[sensor_ids].tension = tension_s;
                sensors[sensor_ids].efficiency = efficiency_s;
                sensors[sensor_ids].potency = result_potency(tension_s, current_s);

                memset(buf, 0, BUFSZ);
                sprintf(buf, "successful change\n");
                count = send(csock, buf, strlen(buf) + 1, 0);
                if (count != strlen(buf) + 1)
                {
                    logexit("send");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    printf("%s %d %d %d %d\n", CH_REQ, sensor_ids, sensors[sensor_ids].current, sensors[sensor_ids].tension, sensors[sensor_ids].efficiency);
                    continue;
                }
            }
        }

        // 4)Show Value
        if ((strcmp(action, SEN_REQ) == 0))
        {
            int sensor_ids = strtol(sensor_id, NULL, 10);

            int cont = 0;
            for (int i = 0; i <= TAM; i++)
            {
                if (sensor_ids == sensors[i].sensor_id)
                {
                    cont++;
                }
            }
            if (cont == 0)
            {
                memset(buf, 0, BUFSZ);
                sprintf(buf, "sensor not installed");
                count = send(csock, buf, strlen(buf) + 1, 0);
                if (count != strlen(buf) + 1)
                {
                    logexit("send");
                    exit(EXIT_FAILURE);
                    continue;
                }
            }
            else
            {
                memset(buf, 0, BUFSZ);
                sprintf(buf, "sensor %d: %d %d", sensor_ids, sensors[sensor_ids].potency, sensors[sensor_ids].efficiency);
                count = send(csock, buf, strlen(buf) + 1, 0);
                if (count != strlen(buf) + 1)
                {
                    logexit("send");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    printf("%s %d\n", SEN_REQ, sensor_ids);
                    continue;
                }
            }
        }

        // 5)Show Values
        if (strcmp(action, VAL_REQ) == 0)
        {
            int cont = 0;
            int aux[cont];
            for (int i = 0; i < sizeof(sensors) / sizeof(sensors[0]); i++)
            {
                if (sensors[i].sensor_id != 0 && sensors[i].tension != 0)
                {
                    aux[cont] = sensors[i].sensor_id;
                    cont++;
                }
            }
            if (cont > 0)
            {
                memset(buf, 0, BUFSZ);
                for (int i = 0; i < cont; i++)
                {
                    sprintf(buf + strlen(buf), "%d (%d %d)", sensors[aux[i]].sensor_id, sensors[aux[i]].potency, sensors[aux[i]].efficiency);
                }

                char s[9] = "sensors:";
                char *buffer = strcat(s, buf);
                count = send(csock, buffer, strlen(buffer) + 1, 0);
                if (count != strlen(buffer) + 1)
                {
                    logexit("send");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    printf("%s\n", VAL_REQ);
                }
            }
            else
            {
                memset(buf, 0, BUFSZ);
                sprintf(buf, "no sensors");
                count = send(csock, buf, strlen(buf) + 1, 0);
                if (count != strlen(buf) + 1)
                {
                    logexit("send");
                    exit(EXIT_FAILURE);
                }
            }
        }

        // kill
        if (strncmp(action, "kill", 4) == 0)
        {
            memset(buf, 0, BUFSZ);
            sprintf(buf, "kill\n");
            count = send(csock, buf, strlen(buf) + 1, 0);
            if (count != strlen(buf) + 1)
            {
                logexit("send");
            }
            else
            {
                break;
            }
        }
    }
    close(s);
}
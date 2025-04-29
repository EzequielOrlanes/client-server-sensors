#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define INS_REQ "INS_REQ"
#define REM_REQ "REM_REQ"
#define CH_REQ "CH_REQ"
#define SEN_REQ "SEN_REQ"
#define SEN_RES "SEN_RES"
#define VAL_REQ "VAL_REQ"
#define VAL_RES "VAL_RES"

#define BUFSZ 500

void usage(int argc, char **argv) {
	printf("usage: %s <server IP> <server port>\n", argv[0]);
	printf("example: %s 127.0.0.1 51511\n", argv[0]);
	exit(EXIT_FAILURE);
}


int main(int argc, char **argv) {

   	char *action;
	char *param;
	char *sensor_id;
	char *current;
	char *tension;
	char *efficiency;

	if (argc < 3) {
		usage(argc, argv);
	}

	struct sockaddr_storage storage;
	if (0 != addrparse(argv[1], argv[2], &storage)) {
		usage(argc, argv);
	}

	int s;
	s = socket(storage.ss_family, SOCK_STREAM, 0);
	if (s == -1) {
		logexit("socket");
	}
	struct sockaddr *addr = (struct sockaddr *)(&storage);
	if (0 != connect(s, addr, sizeof(storage))) {
		logexit("connect");
	}

	char addrstr[BUFSZ];
	addrtostr(addr, addrstr, BUFSZ);

	printf("connected to %s\n", addrstr);
	unsigned total = 0;

	while(1) {
	char buf[BUFSZ];
	memset(buf, 0, BUFSZ);
	fgets(buf,BUFSZ, stdin);
	char buffer_aux[BUFSZ];
	memset(buffer_aux, 0, BUFSZ);
	strcpy(buffer_aux, buf);
	action = strtok(buffer_aux," ");
	param = strtok(NULL," ");
  	sensor_id = strtok(NULL," ");
  	current = strtok(NULL," ");
	tension = strtok(NULL," ");
	efficiency = strtok(NULL," ");
	int show_flag = 0;

	if((strncmp(action, "kill", 4) != 0) && (strncmp(action, "install", 7) != 0)
 	&& (strncmp(action, "remove", 6) != 0) && 
	(strncmp(action, "show", 4) != 0) && (strncmp(action, "change", 6) != 0) ){
	exit(EXIT_FAILURE);
	close(s);
}


		if(strncmp(action, "kill", 4) == 0){ 
      	if(send(s, action, 5, 0) == -1){
        perror("Send");
        return -1;
		} 
		else{
		close(s);
		exit(EXIT_FAILURE);
		}
	}

			if((strncmp(action, "install",7) == 0) && (strncmp(param, "file", 4) == 0)){
  				FILE *file = fopen((sensor_id), "r");
  				if(file == NULL){
				printf("arquivo failed");
  				} else{
				char data[BUFSZ];
				fread(data, sizeof(char), sizeof(data), file);
				char buffer_aux[BUFSZ];
				memset(buffer_aux, 0, BUFSZ);
				strcpy(buffer_aux, data);
				sensor_id = strtok(buffer_aux, "\n");
  				current = strtok(NULL, "\n");
				tension = strtok(NULL,"\n");
				efficiency = strtok(NULL,"\n");

				int sensor_ids = strtol(sensor_id, NULL, 10); 
				int current_s = strtol(current, NULL, 10); 
    			int tension_s = strtol(tension, NULL, 10); 
    			int efficiency_s = strtol(efficiency, NULL, 10); 

			if((sensor_ids < 0 )||(current_s < 0 || current_s > 10) || (tension_s < 0 || tension_s > 150) || (efficiency_s < 0 || efficiency_s > 100)){
    			printf("invalid sensor\n");
				continue;
    		} 
				else { 
				memset(buf, 0, BUFSZ);
				sprintf(buf,"%s %s %s %s %s",INS_REQ, sensor_id, current, tension, efficiency);
				if(send(s, buf, sizeof(buf), 0 )== -1){
				perror("Send");
				return -1;
					} 
				}
  				fclose(file);
				}
  				
				}

		if((strcmp(action, "install") == 0) && (strcmp(param, "param") == 0)){
			int sensor_ids = strtol(sensor_id, NULL, 10); 
    		int current_s = strtol(current, NULL, 10); 
    		int tension_s = strtol(tension, NULL, 10); 
    		int efficiency_s = strtol(efficiency, NULL, 10); 
			if((sensor_ids < 0 )||(current_s < 0 || current_s > 10) || (tension_s < 0 || tension_s > 150) || (efficiency_s < 0 || efficiency_s > 100)){
    		printf("invalid sensor\n");
			continue;
    		} else { 
			memset(buf, 0, BUFSZ);
			sprintf(buf,"%s %s %s %s %s",INS_REQ, sensor_id, current, tension, efficiency);
			if(send(s, buf, sizeof(buf), 0 )== -1){
			perror("Send");
			return -1;
			} 
		}
	}

	if((strcmp(action, "change")==0) && (strcmp(param,"param")==0)){
		int sensor_ids = strtol(sensor_id, NULL, 10); 
    	int current_s = strtol(current, NULL, 10); 
    	int tension_s = strtol(tension, NULL, 10); 
    	int efficiency_s = strtol(efficiency, NULL, 10); 
			if((sensor_ids < 0 )||(current_s < 0 || current_s > 10) || (tension_s < 0 || tension_s > 150) || (efficiency_s < 0 || efficiency_s > 100)){
    			printf("invalid sensor\n");
				continue;
    		} 
			else{
			memset(buf, 0, BUFSZ);
			sprintf(buf,"%s %s %s %s %s",CH_REQ, sensor_id, current, tension, efficiency);
				if(send(s, buf, sizeof(buf), 0 )== -1){
				perror("Send");
				return -1;
				} 
			}
	}

		if((strncmp(action, "remove", 6) == 0)){
			memset(buf, 0, BUFSZ);
			sprintf(buf,"%s %s", REM_REQ, param);
				if(send(s, buf, sizeof(buf), 0 )== -1){
				perror("Send");
				return -1;
				}
		}


		if((strncmp(action, "show", 4)==0) && (strncmp(param,"values",6)==0)){
			show_flag++;
			memset(buf, 0, BUFSZ);
			sprintf(buf,"%s", VAL_REQ);
			if(send(s, VAL_REQ, sizeof(VAL_REQ)+1, 0 )== -1){
				perror("Send");
				return -1;
			}
		}


		if((strncmp(action, "show", 4)==0) && (strncmp(param, "value",5)==0) && (show_flag==0)){
			memset(buf, 0, BUFSZ);
			sprintf(buf,"%s %s", SEN_REQ, sensor_id);
			if(send(s, buf, sizeof(buf), 0 )== -1){
				perror("Send");
				return -1;
			}
		}

		int count = recv(s, buf + total, BUFSZ - total, 0);
		if (count == 0) {
			break;
			exit(EXIT_SUCCESS);
		}
		if(count == -1){
        	perror("Recv");
        	exit(EXIT_FAILURE);
      	}
		puts(buf);
		memset(buf, 0, BUFSZ);
	}
	close(s);
    exit(EXIT_SUCCESS);
}
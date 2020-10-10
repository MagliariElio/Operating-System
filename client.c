#include "lib.h"


char*nome_client;
int sent;
pthread_t thread_padre;
void *gestione_comunicazione(void *argument);

int main(int argc, char *argv[]){
	struct sockaddr_in server, client;
	long sd;
	
	printf("\n\t\t\t*************************\n\t\t\t*\t  CLIENT\t*\n\t\t\t*************************\n\n");
	printf("Inserisci il tuo nome: ");
	if(scanf("%ms", &nome_client) == EOF){
		free(nome_client);
		exit(-1);
	}
	nome_client[strlen(nome_client)] = '\0';
	
	if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		printf("error init socket\n");	
		exit(EXIT_FAILURE);
	}
	
	client.sin_family = AF_INET;
	client.sin_addr.s_addr = htonl(INADDR_ANY);
	client.sin_port = htons(0);
	
	if(bind(sd, (struct sockaddr *)&client, sizeof(client)) == -1){
		printf("error bind\n");
		exit(EXIT_FAILURE);
	}

	char *IP;
	IP = (char *)malloc(14);
	strcpy(IP, "127.0.0.1");
	IP[strlen(IP)] = '\0';
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(IP);
	server.sin_port = htons(PORT);
	
	if(connect(sd, (struct sockaddr *)&server, sizeof(server)) == -1){
		printf("error connect\n");
		printf("error code %d\n", errno);
		exit(EXIT_FAILURE);
	}
	
	printf("Connesso al server - Digita 'quit' per interrompere la connessione\n\n");
	pthread_t thread;
	thread_padre = pthread_self();
	
	send(sd, nome_client, strlen(nome_client), 0);

	
	pthread_create(&thread, NULL, (void *)gestione_comunicazione, (void *)sd);
	
	sent = 0;
	char *message_input, *message_input_send;
	while(1){
		reset:	
			message_input = (char *)malloc(MAX_READER);
			if(fgets(message_input, MAX_READER, stdin) == NULL){
				free(message_input);
				goto reset;
			}
			
			message_input[strlen(message_input)-1] = '\0';
			message_input_send = (char *)malloc(MAX_READER);
			sprintf(message_input_send, "%s: %s", nome_client, message_input);
			
			if(strcmp(message_input, "quit") == 0 && sent == 0){
				printf("\t** Chiusura del canale di comunicazione **\n");
				close(sd);
				free(message_input);
				free(message_input_send);
				pthread_kill(thread, SIGINT);
				return 0;
			}
			
			if(sent == 1){
				send(sd, message_input_send, strlen(message_input_send), 0);
			}else{
				send(sd, message_input, strlen(message_input), 0);
			}
			
			if(strcmp(message_input, "quit") == 0){
				sent = 0;
			}
			
			free(message_input);
			free(message_input_send);
	}
	return 0;
}

void *gestione_comunicazione(void *argument){
	long sd, c;
 	sd = (long) argument;
	char *message_server;
	
	while(1){
		message_server = (char *)malloc(MAX_READER);
		c = recv(sd, message_server, MAX_READER, 0);
		
		if(c == 0){
			free(message_server);
			pthread_kill(thread_padre, SIGKILL);
			pthread_exit(NULL);
		}
		message_server[c] = '\0';
		
		if(strcmp(message_server, "     *** Digita per parlare! ***\n") == 0){
			sent = 1;
		}
		
		if(strcmp(message_server, "***exItnOw***") == 0){
			sent = 0;
			free(message_server);
			send(sd, "***exItnOw***", strlen("***exItnOw***"), 0);
			continue;
		}

		printf("%s\n", message_server);
		free(message_server);
	}
	exit(0);
}

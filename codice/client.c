#include "lib.h"


char*nome_client;
int sent;
long sd;
pthread_t thread_padre;
pthread_t thread;
void *gestione_comunicazione(void *argument);



void exit_thread(){
	pthread_exit(NULL);
}


void gestione_uscita(){
	
	if(sent == 0){
		printf("\t*** \e[1mChiusura del canale di comunicazione\e[22m ***\n\n\n");
		send(sd, "quit", strlen("quit"), 0);
		close(sd);
		if(thread == pthread_self()){
			pthread_kill(thread_padre, SIGUSR1);
		}
		else{
			pthread_kill(thread, SIGUSR1);
		}
		pthread_exit(NULL);
	}
	
	sent = 0;
	char *close_chat;
	close_chat = (char *)malloc(MAX_READER);
	sprintf(close_chat, "%s: %s", nome_client, "quit");
	send(sd, close_chat, strlen(close_chat), 0);
	printf("\033[2J\033[H");
}


int main(){
	struct sockaddr_in server, client;
	
	signal(SIGINT, gestione_uscita);
	signal(SIGUSR1, exit_thread);
	
	printf("\n\t\t\e[34m\e[1m@@@@@@   @@       @@   @@@@@@   @@@@  @@   @@@@@@\e[22m\e[39m\n");
 	printf("\t\t\e[34m\e[1m@@       @@       @@   @@       @@ @@ @@     @@\e[22m\e[39m\n");
 	printf("\t\t\e[34m\e[1m@@       @@       @@   @@@@@@   @@   @@@     @@\e[22m\e[39m\n");
 	printf("\t\t\e[34m\e[1m@@       @@       @@   @@       @@    @@     @@\e[22m\e[39m\n");
 	printf("\t\t\e[34m\e[1m@@@@@@   @@@@@@   @@   @@@@@@   @@    @@     @@\e[22m\e[39m\n");
	
	printf("\n\033[40m\033[1;33mInserisci il tuo nome: \033[0m");
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

	
	printf("Connesso al server - Digita '\e[1mquit\e[22m' per interrompere la connessione\n\n");
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
				printf("\t*** \e[1mChiusura del canale di comunicazione\e[22m ***\n");
				close(sd);
				free(message_input);
				free(message_input_send);
				pthread_kill(thread, SIGINT);
				printf("\033[2J\033[H");
				return 0;
			}
			
			if(sent == 1){
				send(sd, message_input_send, strlen(message_input_send), 0);
			}
			else{
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
			pthread_kill(thread_padre, SIGUSR1);
			pthread_exit(NULL);
		}
		message_server[c] = '\0';
		
		if(strcmp(message_server, "\n\t*** \e[91m\e[1mDigita per parlare!\e[22m\e[39m ***\n") == 0){
			printf("%s\n", message_server);
			sent = 1;
			free(message_server);
			continue;
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

#include "lib.h"

#define PENDING 10

int fd[PENDING];													//mantiene tutti i socket client descriptor
void *identificazione(void *argument);								//identifica il client
void client_online(void *argument);									//stampa i client disponibili online
int cerca_client(int, char *, int);									//cerca il client selezionato
void comunicazione(int client1, long client2, long i);				//mette in comunicazione i due client



struct tabella{
	char *nome_client;
	char *indirizzo_ip;
	int disponibile;		//-1 non attivo, 1 disponibile, 0 occupato
	pthread_t id_thread;
};

struct value{
	int dsc;
	int numero;
	pthread_t testa;
};




struct tabella **tabella_thread;
pthread_mutex_t *sem;
int passaggio[3];
int entry, exit_client;




void client_exit_attesa(int signo){
	pthread_exit(NULL);
}

void dealloca_memoria(int i){
	
	pthread_mutex_lock(sem);
	free(tabella_thread[i]->nome_client);
	free(tabella_thread[i]->indirizzo_ip);
	tabella_thread[i]->disponibile = -1;
	fd[i] = -1;
	pthread_mutex_unlock(sem);
	
	pthread_exit(NULL);
}

void cambia_linea(int signo, siginfo_t *a, void* b){
	comunicazione(-1, -1, -1);
	return;
}


void *listen_client(void *argument){
	struct value *head_listen = (struct value *) argument;
	char *ric_attesa;
	int cl, id_l = head_listen->numero;
	
	
	while(cl >= 0){
		ric_attesa = (char *)malloc(MAX_READER);
		cl = recv(head_listen->dsc, ric_attesa, MAX_READER, 0);
		ric_attesa[cl] = '\0';
	
		exit_client = 1;
		while(entry == 0);
		
		if(cl == 0){
 			pthread_kill(head_listen->testa, SIGUSR1);
			printf("Client[%d] si è disconnesso dal server\n", id_l);
			
			free(head_listen);
			free(ric_attesa);
			dealloca_memoria(id_l);
			
			return((void *)0);
		}
		exit_client = 0;
	}
	
	pthread_exit(NULL);
}




int main(){
    struct sockaddr_in server, client[PENDING];
    int sd;
	long id;
    socklen_t addrlen;
	pthread_t thread[PENDING];
	
	printf("\n\t\t*************************\n\t\t*\t  SERVER\t*\n\t\t*************************\n\n");
	
    
    if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){
        printf("Errore init socket\n");
        exit(EXIT_FAILURE);
    }
    
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
    

    if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))<0){
    	printf("Error setsockopt(SO_REUSEADDR)");
    	printf("Error code: %d\n", errno);
    	exit(EXIT_FAILURE);
    } 
	
	
    if( (bind(sd, (struct sockaddr *)&server, sizeof(server)))== -1){
        printf("Error binding\n");
        printf("Error code: %d\n", errno);
        exit(EXIT_FAILURE);
    }
    

    if( listen(sd, PENDING) == -1){
		printf("error listen\n");
		printf("error code %d\n", errno);
		exit(EXIT_FAILURE);
	}
	
	printf("Server in ascolto sulla porta %d\n", PORT);
	

	addrlen = sizeof(client[0]);
	sem = malloc(sizeof(pthread_mutex_t *));
	tabella_thread = malloc(PENDING*sizeof(struct tabella *));
	
	for(int x=0; x<PENDING; x++){
		tabella_thread[x] = malloc(sizeof(struct tabella *));
		fd[x] = -1;
	}

	id = 0;
	int ct;
	while(1){
		while(1){
			pthread_mutex_lock(sem);
			ct = fd[id];
			pthread_mutex_unlock(sem);
			
			if(ct == -1){					//trovato un posto per un client
				break;
			}
			id = (id+1)%PENDING;
		}
		

		while((ct = accept(sd, (struct sockaddr *)&client[id], &addrlen)) == -1 );
		
		pthread_mutex_lock(sem);
		fd[id] = ct;
		tabella_thread[id]->indirizzo_ip = (char *)malloc(strlen(inet_ntoa(client[id].sin_addr)));
		strcpy(tabella_thread[id]->indirizzo_ip, inet_ntoa(client[id].sin_addr));
		printf("\nConnesione aperta sul server\n");
		printf("Ip Client[%ld]: %s\n\n", id, tabella_thread[id]->indirizzo_ip);
		pthread_mutex_unlock(sem);
		
		pthread_create(&thread[id], NULL, (void *)identificazione, (void *)id);
		id = (id+1)%PENDING;
	}

	printf("Chiusura della connessione\n");

    close(sd);
    return 0;
}

void *identificazione(void *argument){
	
	sigset_t set;
	struct sigaction act;
	sigfillset(&set);
	act.sa_sigaction = cambia_linea;
	act.sa_mask = set;
	act.sa_flags = 0;
	sigaction(SIGCHLD, &act, NULL);
	
	long i = (long) argument;					//indice del thread
	int c;
	
	char *nome;
	nome = (char *)malloc(MAX_READER);
	c = recv(fd[i], nome, MAX_READER, 0);
	if(c == 0){
		free(nome);
		fd[i] = -1;
		pthread_exit(NULL);
	}
	nome[c] = '\0';
	
	pthread_mutex_lock(sem);
	tabella_thread[i]->nome_client = (char *)malloc(MAX_READER);
	strcpy(tabella_thread[i]->nome_client, nome);
	tabella_thread[i]->nome_client[strlen(tabella_thread[i]->nome_client)] = '\0';
	tabella_thread[i]->disponibile = 1;
	tabella_thread[i]->id_thread = pthread_self();
	pthread_mutex_unlock(sem);
	
	free(nome);
	
	
	client_online((void *)i);
	return((void *)0);
}

void client_online(void *argument){
	long i = (long)argument;
	int c, dsc, count;
	
	pthread_mutex_lock(sem);
	dsc = fd[i];
	pthread_mutex_unlock(sem);
	
	
	char *stampa;
	restart_list:
	
	nanosleep((const struct timespec[]){{0, 5000000L}}, NULL);
	send(dsc, "\n   --- Elenco delle persone connesse ---", strlen("\n   --- Elenco delle persone connesse ---"), 0);

	count = 0;
	for(int j=0; j<PENDING; j++){
		
		count++;
		pthread_mutex_lock(sem);
		c = tabella_thread[j]->disponibile;
		pthread_mutex_unlock(sem);
		
		if(c == 1){
			count--;
			stampa = (char *)malloc(MAX_READER);
			
			pthread_mutex_lock(sem);
			sprintf(stampa, " - %s        Indirizzo IP: %s", tabella_thread[j]->nome_client, tabella_thread[j]->indirizzo_ip);
			pthread_mutex_unlock(sem);
			
			nanosleep((const struct timespec[]){{0, 50000000L}}, NULL);
			send(dsc, stampa, strlen(stampa), 0);
			
			free(stampa);
		}
	}
	
	if(count == PENDING-1){
 		signal(SIGUSR1, client_exit_attesa);
		struct value *head;
		head = malloc(sizeof(struct value *));
		pthread_t thread_ric_attesa;
		
		head->dsc = dsc;
		head->numero = (int) i;
		head->testa = pthread_self();

  		pthread_create(&thread_ric_attesa, NULL, (void *)listen_client, (void *)head);
		
		int x = 0;
		exit_client = 0;
		while(1){
			if(i!=x){
				
				entry = 0;
				
				pthread_mutex_lock(sem);
				c = tabella_thread[x]->disponibile;
				pthread_mutex_unlock(sem);
				
				if(c == 1){
  					pthread_kill(thread_ric_attesa, SIGUSR1);
					free(head);
					goto restart_list;
				}
				
				entry = 1;
				while(exit_client == 1);
			}
			x = (x+1)%PENDING;
		}
	}
	
	char *seleziona;
	while(1){
		nanosleep((const struct timespec[]){{0, 5000000L}}, NULL);
		send(dsc, "   --- Digita il nome della persona da contattare ---", strlen("   --- Digita il nome della persona da contattare ---"), 0);
		nanosleep((const struct timespec[]){{0, 5000000L}}, NULL);
		send(dsc, "   *** Digita 'aggiorna' per aggiornare la lista  ***", strlen("   *** Digita 'aggiorna' per aggiornare la lista  ***"), 0);
		
		seleziona = (char *)malloc(MAX_READER);
		
		c = recv(dsc, seleziona, MAX_READER, 0);
		seleziona[c] = '\0';
		
		if(c == 0 || strcmp(seleziona, "quit") == 0){
			printf("Client[%ld] si è disconnesso dal server\n", i);
			free(seleziona);
			nanosleep((const struct timespec[]){{0, 50000000L}}, NULL);
			
			dealloca_memoria(i);
			
			pthread_exit(NULL);
		}
		
		if(strcmp(seleziona, "aggiorna") == 0){
			free(seleziona);
			goto restart_list;
		}
		
		if(cerca_client(i, seleziona, dsc) == 0){
			goto restart_list;
		}
	}
}


int cerca_client(int i, char *seleziona, int descpritor){
	
	char *risposta, **cerca;
	int dsc[2], c;
	
	dsc[0] = descpritor;
	
	cerca = (char **)malloc(3*MAX_READER);
	
	for(long j=0; j<PENDING; j++){
			
		if(i!=j){
			cerca[0] = (char *)malloc(MAX_READER);
			
			pthread_mutex_lock(sem);
			if(tabella_thread[j]->nome_client == NULL){
				pthread_mutex_unlock(sem);
				free(cerca[0]);
				continue;
			}
			strcpy(cerca[0], tabella_thread[j]->nome_client);
			pthread_mutex_unlock(sem);
			
			cerca[0][strlen(cerca[0])] = '\0';
			
			if(strcmp(seleziona, cerca[0]) == 0){
				free(seleziona);
				free(cerca[0]);
				
				send(dsc[0], "   --- Invio della richiesta, attendere... ---", strlen("   --- Invio della richiesta, attendere... ---"), 0);
				
				pthread_mutex_lock(sem);
				dsc[1] = fd[j];
				pthread_mutex_unlock(sem);
				
				cerca[1] = (char*)malloc(MAX_READER);
				
				pthread_mutex_lock(sem);
				sprintf(cerca[1], "\n   --- Hai una richiesta da %s ---", tabella_thread[i]->nome_client);
				pthread_mutex_unlock(sem);
				
				send(dsc[1], cerca[1], strlen(cerca[1]), 0);
				nanosleep((const struct timespec[]){{0, 500000L}}, NULL);
				free(cerca[1]);
				send(dsc[1], "   --- Digitare Si per accettare altrimenti digitare No ---", strlen("   --- Digitare Si per accettare altrimenti digitare No ---"), 0);
				
				while(1){
					risposta = (char *)malloc(MAX_READER);
					c = recv(dsc[1], risposta, MAX_READER, 0);
					
					if(c == 0 || strcmp(risposta, "quit") == 0){
						printf("Client[%ld] si è disconnesso dal server\n", j);
						free(risposta);
						
						dealloca_memoria(j);
						
						pthread_exit(NULL);
					}
					risposta[c] = '\0';
					
					
					if(strcmp(risposta, "Si") == 0 || strcmp(risposta, "si") == 0 || strcmp(risposta, "SI") == 0){
						free(risposta);
						
						passaggio[0] = dsc[1];
						passaggio[1] = dsc[0];
						passaggio[2] = j;
						
						cerca[1] = (char *)malloc(MAX_READER);
						send(dsc[0], "     ------------------------------------", strlen("     ------------------------------------"), 0);
						nanosleep((const struct timespec[]){{0, 500000L}}, NULL);
						
						pthread_mutex_lock(sem);
						sprintf(cerca[1], "   --- %s ha accettato la richiesta! ---", tabella_thread[j]->nome_client);
						tabella_thread[i]->disponibile = 0;
						tabella_thread[j]->disponibile = 0;
						pthread_mutex_unlock(sem);
						
						send(dsc[1], "     ------------------------------------", strlen("     ------------------------------------"), 0);
						send(dsc[0], cerca[1], strlen(cerca[1]), 0);
						free(cerca[1]);
						nanosleep((const struct timespec[]){{0, 500000L}}, NULL);
						send(dsc[1], "\n     *** Digita per parlare! ***\n", strlen("\n     *** Digita per parlare! ***\n"), 0);
						send(dsc[0], "\n     *** Digita per parlare! ***\n", strlen("\n     *** Digita per parlare! ***\n"), 0);
						
						pthread_mutex_lock(sem);
						pthread_kill(tabella_thread[j]->id_thread, SIGCHLD);
						pthread_mutex_unlock(sem);
						
						free(cerca);

						comunicazione(dsc[0], dsc[1], i);
						pthread_exit(NULL);
					}
					
					if(strcmp(risposta, "No") == 0 || strcmp(risposta, "no") == 0 || strcmp(risposta, "NO") == 0){
						free(risposta);
						free(cerca);
						risposta = (char *) malloc(MAX_READER);
						
						pthread_mutex_lock(sem);
						sprintf(risposta, "\n   --- %s ha rifiutato la richiesta! ---", tabella_thread[j]->nome_client);
						pthread_mutex_unlock(sem);
						
						send(dsc[0], risposta, strlen(risposta), 0);
						free(risposta);
						return 0;
					}
					
					send(dsc[1], "   --- Devi rispondere Si o No! ---", strlen("   --- Devi rispondere Si o No! ---"), 0);
					free(risposta);
				}
			}
			
			free(cerca[0]);
		}
	}
	send(dsc[0], "\n   --- Nome non trovato! ---\n", strlen("\n   --- Nome non trovato! ---\n"), 0);
	free(seleziona);
	return 1;
}



void comunicazione(int client1, long client2, long i){
	int c;
	

	if(client1 == -1 && client2 == -1 && i == -1){
		client1 = passaggio[0];
		client2 = passaggio[1];
		i = passaggio[2];
	}
	
	
	char *messaggio, *uscita;
	uscita = (char *)malloc(MAX_READER);
	
	pthread_mutex_lock(sem);
	sprintf(uscita, "%s: quit", tabella_thread[i]->nome_client);
	pthread_mutex_unlock(sem);

	while(1){
		messaggio = (char *)malloc(MAX_READER);
		c = recv(client1, messaggio, MAX_READER, 0);
		messaggio[c] = '\0';
		
		if(strcmp(messaggio, "***exItnOw***") == 0){
			free(messaggio);
			free(uscita);
			
			pthread_mutex_lock(sem);
			tabella_thread[i]->disponibile = 1;
			pthread_mutex_unlock(sem);
			
			client_online((void *)i);
			pthread_exit(NULL);
		}
		
		if(strcmp(messaggio, uscita) == 0 || c == 0){
			send(client1, "\n     ------------------------------------", strlen("\n     ------------------------------------"), 0);
			send(client2, "\n     ------------------------------------", strlen("\n     ------------------------------------"), 0);
			free(messaggio);
			messaggio = (char *)malloc(MAX_READER);
			nanosleep((const struct timespec[]){{0, 500000L}}, NULL);	
			
			pthread_mutex_lock(sem);
			sprintf(messaggio, "\n         %s ha abbandonato la conversazione\n", tabella_thread[i]->nome_client);
			pthread_mutex_unlock(sem);
			
			send(client2, messaggio, strlen(messaggio), 0);
			send(client1, messaggio, strlen(messaggio), 0);
			nanosleep((const struct timespec[]){{0, 500000L}}, NULL);
			send(client2, "***exItnOw***", strlen("***exItnOw***"), 0);
			free(messaggio);
			free(uscita);
			
			pthread_mutex_lock(sem);
			tabella_thread[i]->disponibile = 1;
			pthread_mutex_unlock(sem);
			
			client_online((void *)i);
			pthread_exit(NULL);
		}
		
		send(client2, messaggio, strlen(messaggio), 0);
		free(messaggio);
	}
	
	pthread_exit(NULL);
	
}

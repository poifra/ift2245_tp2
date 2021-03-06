#include "server_thread.h"
#include <stdio.h>

#include <netinet/in.h>
#include <netdb.h>

#include <strings.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <sys/select.h>
#include <time.h>

#include <stdbool.h>
#include <conf.h>
#include <pthread.h>

// Variable obtenue de /conf.c

unsigned int server_socket_fd;

// Structures de données pour l'algorithme du banquier.
// Pour les tableaux de 2 dimensions, utilisez des tableaux de pointeurs de tableaux.
//  ___
// |   |      _______________
// | *-----> | 5  | 7  | ...
// |___|
// |   |      ______________
// | *-----> | 12 | 23 | ...
// |___|
// |   |
// |   |
//
int *available;
int **max;
int **allocation;
int **need;

// Variable du journal.
// Nombre de requête acceptée (ACK envoyé en réponse à REQ)
unsigned int count_accepted = 0;

// Nombre de requête en attente (WAIT envoyé en réponse à REQ)
unsigned int count_on_wait = 0;

// Nombre de requête refusée (REFUSE envoyé en réponse à REQ)
unsigned int count_invalid = 0;

// Nombre de client qui se sont terminés correctement (ACC envoyé en réponse à END)
unsigned int count_dispatched = 0;

// Nombre total de requête (REQ) traités.
unsigned int request_processed = 0;

// Nombre de clients ayant envoyé le message END.
unsigned int clients_ended = 0;

// Nombre de clients. Nombre reçu du client lors de la requête BEGIN.
unsigned int num_clients;

int* client_sockets;

pthread_mutex_t banker_lock;

void error(const char *msg)
{
	perror(msg);
	exit(-1);
}

void
st_init_banquier ()
{
	//initialisation des structures de donnée
	int i, j;
	int count = num_server_threads;

	available = malloc(num_resources * sizeof(int));
	allocation = malloc(num_clients * sizeof(int*));
	max = malloc(num_clients * sizeof(int*));
	need = malloc(num_clients * sizeof(int*));
	if (available == NULL || allocation == NULL || max == NULL || need == NULL) {
		error("null pointer exception");
	}
	for (i = 0; i < num_resources; i++) {
		available[i] = *(available_resources + i);
	}

	for (j = 0; j < num_clients; j++) {
		allocation[j] = malloc(num_resources * sizeof(int));
		max[j] = malloc(num_resources * sizeof(int));
		need[j] = malloc(num_resources * sizeof(int));
		if (allocation[j] == NULL || max[j] == NULL || need[j] == NULL) {
			error("null pointer exception");
		}
		for (i = 0; i < num_resources; i++) {
			allocation[j][i] = 0;
			max[j][i] = 0;
			need[j][i] = 0;
		}

	}
}

int st_init_client(int cid, int32_t *client_max){
        int i;
        int response = 1;
        pthread_mutex_lock(&banker_lock);
        for(i = 0; i < num_resources && response; i++){
                if(client_max[i] > available[i])
                        response = 0;
                else {
                        max[cid][i] = client_max[i];
                        need[cid][i] = max[cid][i] - allocation[cid][i];
                }
        }
        pthread_mutex_unlock(&banker_lock);
        return response;
}

//kill the banker
int st_execute_banker(int cid, int *req){        
        int i;
        int valid = 1;
        int enough = 1;
        pthread_mutex_lock(&banker_lock);
        for(i = 0; i < num_resources && valid && enough; i++){
                valid = valid && (- req[i]) <= need[cid][i]//Le client ne demande pas plus que ce qu'il peut
                        && req[i] <= allocation[cid][i];//Le client n'essaie pas de libérer plus que ce qu'il possède
                enough = enough && (- req[i]) <= available[i];
        }
        if(!valid){
                pthread_mutex_unlock(&banker_lock);
                return -1;//refuse
        }
        else if(!enough) {
                pthread_mutex_unlock(&banker_lock);
                return 0;//wait
        } else {
                //calcul du nouvel état 
                for(i = 0; i < num_resources; i++){
                        available[i] += req[i];
                        allocation[cid][i] -= req[i];
                }
        }
        //vérification du nouvel état
        int j;
        int safe = true;
        int work[num_resources];
        int finished[num_clients];
        int done = false;
        //Work = available
        for(i = 0; i < num_resources; i++){
                work[i] = available[i];                
        }
        for(i = 0; i < num_clients; i++){
                finished[i] = false;                
        }
        while(safe && !done){
                safe = false;
                for(i = 0; i < num_clients; i++){
                        if(!finished[i]){
                                safe = true;
                                //safe = need[i] <= work                                
                                for(j = 0; j < num_resources && safe; j++){
                                        safe = safe && need[i%num_clients][j] <= work[j];
                                }                
                                if(safe){
                                        finished[i%num_clients] = true;
                                        //work += allocation[i]
                                        for(j = 0; j < num_resources; j++){
                                                work[j] += allocation[i%num_clients][j];
                                        }
                                        i = 0;
                                }
                        }
                }
                done = true;
                for(i = 0; i < num_clients; i++){
                        done = done && finished[i];
                }                                
        }
        if(!safe){
                //rollback
                for(i = 0; i < num_resources; i++){
                        available[i] -= req[i];
                        allocation[cid][i] += req[i];
                }
        }

        pthread_mutex_unlock(&banker_lock);
        return safe;
}

void st_free_resources(int cid){
        int i;
        for(i = 0; i < num_resources; i++){
                available[i] += allocation[cid][i];
                allocation[cid][i] = 0;
                need[cid][i] = 0;
        }        
}

void
st_process_request (server_thread *st, int socket_fd)
{
	int size = sizeof(int) * (num_resources + 2);//La taille maximale d'un message est 2 de plus que le nombre de ressources
	char data[size];
	int32_t *msg;
	int remaining = size;
	int rc;
	while (remaining)
	{
		rc = read(socket_fd, data + size - remaining, remaining);

		//	printf("read data:%p msg:%p send:%p sizeof(msg):%d remaining:%d rc:%d\n",data, &msg, data + size - remaining, size, remaining, rc);

		if (rc < 0) {
			error("server error on read");
		}
		remaining -= rc;
	}
	msg = (int32_t *) data;

	printf("le serveur reçoit %d %d %d %d %d sur le socket %d\n", msg[0], msg[1], msg[2], msg[3], msg[4], socket_fd);

	int32_t reponse[2];
	int waiting_time = 5;//temps d'attente en secondes.
	switch (msg[0])
	{
	case END:
		printf("END recu\n");
		clients_ended++;
		reponse[0] = ACK;
		reponse[1] = -1;
                st_free_resources(msg[1]);
		st->fini = 1;
		break;
	case REQ:
		request_processed++;
		printf("REQ recu\n");
		printf("msg[1] cid = %d\n", msg[1]);
                if(!(msg[1] < num_clients)){
                        error("invalid client id");
                }
		switch (st_execute_banker(msg[1], msg + 2)) {
		case 1:
                        count_accepted++;
                        printf("Le serveur accept la requête.\n");
			reponse[0] = ACK;
			reponse[1] = -1;
			break;
		case -1:
                        printf("Le serveur refuse la requête.\n");
                        count_invalid++;
			reponse[0] = REFUSE;
			reponse[1] = -1;
			break;
		case 0:
                        printf("Le serveur met en attente le cient.\n");
                        count_on_wait++;
			reponse[0] = WAIT;
			reponse[1] = waiting_time;
			break;
		}
		break;
	case BEGIN:
		printf("BEGIN recu\n");
		if (num_resources != msg[1]) {
			error("Invalid number of resources declared by client");
		} else {
                        num_clients = msg[2];
                        num_server_threads = num_clients;
                        reponse[0] = ACK;
                        reponse[1] = -1;
                }
		break;
	case INIT:
		printf("INIT recu\n");
		request_processed++;
                if(!(msg[1] < num_clients)){
                        error("invalid client id");
                } else {
                        switch(st_init_client(msg[1], msg+2)){
                        case 0:
                                reponse[0] = REFUSE;
                                reponse[1] = -1;
                                break;
                        case 1:
                                reponse[0] = ACK;
                                reponse[1] = -1;
                                break;
                                
                        }
                }
		
		break;
	default:
		printf("lolnope msg[0] is %d \n", msg[0]);
		break;
	}


	char *response_data;
	response_data = (char *) reponse;
	size = sizeof(int32_t) * 2;
	remaining = size;
	rc = 0;
	while (remaining) {

		rc = write(socket_fd, response_data + size - remaining, remaining);
		//	printf("write data:%p msg:%p send:%p sizeof(msg):%d remaining:%d rc:%d\n", data, &reponse, data + size - remaining, size, remaining, rc);

		if (rc < 0) {
			error("server error on write");
		}
		remaining -= rc;
	}

	printf("le serveur a répondu %d sur le socket %d\n", reponse[0], socket_fd);

}


void
st_signal ()
{
	//demande au clients de se terminer
	// TODO: Remplacer le contenu de cette fonction
	sleep(5);
	printf("signal\n");
	// TODO end
}


void st_wait_begin()
{
	struct sockaddr_in thread_addr;
	socklen_t socket_len = sizeof (thread_addr);
	int socket_fd = -1;
	int start = time (NULL);

	// Boucle jusqu'à ce que accept recoive la première connection.
	while (socket_fd < 0)
	{
		socket_fd =
		    accept (server_socket_fd, (struct sockaddr *) &thread_addr,
		            &socket_len);

		if ((time (NULL) - start) >= max_wait_time)
		{
			printf ("Time out on thread while waiting for begin.\n");
			pthread_exit (NULL);
		}
	}
	st_process_request(NULL, socket_fd); //on traite le begin

}
void *
st_code (void *param)
{
	server_thread *st = (server_thread *) param;
	st->fini = 0;
        
	struct sockaddr_in thread_addr;
	socklen_t socket_len = sizeof (thread_addr);
	int thread_socket_fd = -1;
	int start = time (NULL);
        
	// Boucle jusqu'à ce que accept recoive la première connection.
	while (thread_socket_fd < 0)
	{
		thread_socket_fd =
		    accept (server_socket_fd, (struct sockaddr *) &thread_addr,
		            &socket_len);
		if ((time (NULL) - start) >= max_wait_time)
		{
			fprintf (stderr, "Time out no connect on thread %0x.\n", st->pt_tid);
			pthread_exit (NULL);
			//TODO : flag fin thread
		}
	}

	// Boucle de traitement des requêtes
	// Boucle tant que pas de end
	while (!st->fini && (time (NULL) - start) <= max_wait_time) {
		st_process_request (st, thread_socket_fd);
	}


	if (!st->fini) {
		fprintf (stderr, "Time out no request on thread %d.\n", st->id);
	}
	printf("fin pt_tid %0x\n", st->pt_tid);
	shutdown(thread_socket_fd, SHUT_RDWR);
	close(thread_socket_fd);
	pthread_exit (NULL);

}
//
// Ouvre un socket pour le serveur. Vous devrez modifier ce code.
//
void
st_open_socket ()
{
	server_socket_fd = socket (AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (server_socket_fd < 0) {
		error ("ERROR opening socket");
	}

	struct sockaddr_in serv_addr;
	bzero ((char *) &serv_addr, sizeof (serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons (port_number);

	if (bind
	        (server_socket_fd, (struct sockaddr *) &serv_addr,
	         sizeof (serv_addr)) < 0) {
		error ("ERROR on binding");
	}

	listen (server_socket_fd, server_backlog_size);
	printf("listening on socket %d\n", server_socket_fd);
}


//
// Affiche les données recueillies lors de l'exécution du
// serveur.
// La branche else ne doit PAS être modifiée.
//
void
st_print_results (FILE * fd, bool verbose)
{
	if (fd == NULL) { fd = stdout; }
	if (verbose)
	{
		fprintf (fd, "\n---- Résultat du serveur ----\n");
		fprintf (fd, "Requêtes acceptées: %d\n", count_accepted);
		fprintf (fd, "Requêtes misent en attente: %d\n", count_on_wait);
		fprintf (fd, "Requêtes invalides: %d\n", count_invalid);
		fprintf (fd, "Clients : %d\n", count_dispatched);
		fprintf (fd, "Requêtes traitées: %d\n", request_processed);
	}
	else
	{
		fprintf (fd, "%d %d %d %d %d\n", count_accepted, count_on_wait,
		         count_invalid, count_dispatched, request_processed);
	}
}

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
void error(const char *msg)
{
	perror(msg);
	exit(-1);
}

void
st_init ()
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
                available[i] = *(available_resources+i);
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

	// TODO
	//https://en.wikipedia.org/wiki/Banker%27s_algorithm

	// Attend la connection d'un client et initialise les structures pour
	// l'algorithme du banquier.

	// END TODO
}


int st_execute_banker(int cid, int *req){        
        int i;
        int valid = 1;
        int enough = 1;
        for(i = 0; i < num_resources; i++){
                valid = valid && req[i] < need[cid][i];
                enough = enough && req[i] < available[i];
        }
        if(!valid){
                return -1;//refuse
        }
        else if(!enough) {
                return 0;//wait
        } else {
                //calcul du nouvel état 
                for(i = 0; i < num_resources; i++){
                        available[i] -= req[i];
                        allocation[cid][i] += req[i];
                }
        }
        //vérification du nouvel état
        int j;
        int safe = 1;
        int work[num_resources];
        for(i = 0; i < num_resources; i++){
                work[i] = available[i];
        }
        for(i = 0; i < num_clients && safe; i++){
                safe = 1;
                //safe = need[i] < work
                for(j = 0; j < num_resources; i++){
                        safe = safe && need[i][j] <= work[j];
                }
                if(safe){
                        //work += allocation[i]
                        for(j = 0; j < num_resources; j++){
                                work[j] += allocation[i][j];
                        }
                }
        }
        if(!safe){
                //rollback
                for(i = 0; i < num_resources; i++){
                        available[i] += req[i];
                        allocation[cid][i] -= req[i];
                }
        }

        return safe;
}

void
st_process_request (server_thread *st, int socket_fd)
{
	int size = sizeof(int) * (num_resources + 2);//La taille maximale d'un message est 2 de plus que le nombre de ressources
	char data[size];
	int *msg;
	int remaining = size;
	int rc;
	while (remaining)
	{
		rc = read(socket_fd, data + size - remaining, remaining);

		printf("read data:%p msg:%p send:%p sizeof(msg):%d remaining:%d rc:%d\n",
		       data, &msg, data + size - remaining, size, remaining, rc);

		if (rc < 0) {
			error("server error on read");
		}
		remaining -= rc;
	}
	msg = (int *) data;
	size = sizeof(int32_t) * 2;
	int32_t reponse[2];
	int waiting_time = 10;//temps d'attente en secondes.
	int close_socket = 0;
	switch (msg[0])
	{
	case END:
		printf("END recu\n");
		clients_ended++;
		reponse[0] = ACK;
		reponse[1] = -1;
		close_socket = 1;
		break;
	case REQ:
		request_processed++;
		printf("REQ recu\n");
		switch (st_execute_banker(msg[1], msg + 2)) {
		case 1:
			reponse[0] = ACK;
			reponse[1] = -1;
			break;
		case -1:
			reponse[0] = REFUSE;
			reponse[1] = -1;
			break;
		case 0:
			reponse[0] = WAIT;
			reponse[1] = waiting_time;
			break;
		}
		break;
	case BEGIN:
		printf("BEGIN recu\n");
		if (num_resources != msg[1]) {
			error("Invalid number of resources declared by client");
		}
		num_clients = msg[2];
		reponse[0] = ACK;
		reponse[1] = -1;
		break;
	case INIT:
		printf("INIT recu\n");
		request_processed++;
		reponse[0] = ACK;
		reponse[1] = -1;
		break;
	default:
		printf("lolnope msg[0] is %d \n", msg[0]);
		break;
	}


	char *response_data;
	response_data = (char *) reponse;
	remaining = size;
	rc = 0;
	while (remaining) {

		rc = write(socket_fd, data + size - remaining, remaining);
		printf("write data:%p msg:%p send:%p sizeof(msg):%d remaining:%d rc:%d\n", data, &reponse, data + size - remaining, remaining, rc);

		if (rc < 0) {
			error("server error on write");
		}
		remaining -= rc;
		printf("next round %d\n", remaining);
	}
	if (close_socket) {
		close(socket_fd);
	}

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


void *
st_code (void *param)
{
	server_thread *st = (server_thread *) param;

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
		if (thread_socket_fd > 0)
		{
			num_clients++;
			st_process_request(st, thread_socket_fd);
		}
		if ((time (NULL) - start) >= max_wait_time)
		{
			fprintf (stderr, "Time out on thread %d.\n", st->id);
			//	pthread_exit (NULL);
		}
	}


	// Boucle de traitement des requêtes
	// Boucle tant qu'il reste des clients qui n'ont pas envoyé END
	while (clients_ended < num_clients)
	{
		if ((time (NULL) - start) >= max_wait_time)
		{
			fprintf (stderr, "Time out on thread %d.\n", st->id);
			pthread_exit (NULL);
		}
		if (thread_socket_fd > 0)
		{
			st_process_request (st, thread_socket_fd);
			//	close (thread_socket_fd);
		}
		thread_socket_fd =
		    accept (server_socket_fd, (struct sockaddr *) &thread_addr,
		            &socket_len);
	}

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
		fprintf (fd, "Requêtes : %d\n", count_on_wait);
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

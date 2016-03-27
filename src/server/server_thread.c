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
	bool safe = false;
	
	available = malloc(num_resources * sizeof(int));
	allocation = malloc(num_resources * sizeof(int*));
	max = malloc(num_clients * sizeof(int*));
	need = malloc(num_clients * sizeof(int*));
	if (available == NULL || allocation == NULL || max == NULL || need == NULL) {
		error("null pointer exception");
	}
	for (i = 0; i < num_resources; i++) {
                available[i] = *(available_resources+i);
		allocation[i] = malloc(num_resources * sizeof(int));
		max[i] = malloc(num_resources * sizeof(int));
		need[i] = malloc(num_resources * sizeof(int));
		if (allocation[i] == NULL || max[i] == NULL || need[i] == NULL) {
			error("null pointer exception");
		}
		for (j = 0; j < num_clients; j++) {
			allocation[i][j] = 0;
			max[i][j] = 0;
			need[i][j] = 0;
		}
	}
        //	*/
	// TODO
	//https://en.wikipedia.org/wiki/Banker%27s_algorithm

	// Attend la connection d'un client et initialise les structures pour
	// l'algorithme du banquier.

	// END TODO
}

void
st_process_request (server_thread *st, int socket_fd)
{
	int size = sizeof(int) * 5;
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
	switch (msg[0])
	{
	case END:
		printf("END recu\n");
		clients_ended++;
		close(socket_fd);
		break;
	case REQ:
		request_processed++;
		printf("REQ recu\n");
		break;
	case BEGIN:
		printf("BEGIN recu\n");                
                if(num_resources != msg[1])
                        error("Invalid number of resources declared by client");
                num_clients = msg[2];                
		break;
	case INIT:
                printf("INIT recu\n");
		request_processed++;
		break;
	default:
		printf("lolnope msg[0] is %d \n",msg[0]);
		break;
	}

	size = sizeof(int32_t) * 2;
	int32_t reponse[2];
        char *response_data;
	reponse[0] = ACK;
	reponse[1] = -1;       
	response_data = (char *) reponse;
	remaining = size;
	rc = 0;
	while (remaining) {

		rc = write(socket_fd, data + size - remaining, remaining);
		printf("write data:%p msg:%p send:%p sizeof(msg):%d remaining:%d rc:%d\n",data, &reponse, data + size - remaining, remaining, rc);

		if (rc < 0) {
			error("server error on write");
		}
		remaining -= rc;
		printf("next round %d\n", remaining);
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
		perror ("ERROR opening socket");
	}

	struct sockaddr_in serv_addr;
	bzero ((char *) &serv_addr, sizeof (serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons (port_number);

	if (bind
	        (server_socket_fd, (struct sockaddr *) &serv_addr,
	         sizeof (serv_addr)) < 0) {
		perror ("ERROR on binding");
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

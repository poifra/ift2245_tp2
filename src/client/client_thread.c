#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <strings.h>
#include <string.h>

#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netdb.h>

#include "client_thread.h"

// Variable du journal.
// Nombre de requêtes (REQ envoyés)
unsigned int count = 0;

// Nombre de requête acceptée (ACK reçus en réponse à REQ)
unsigned int count_accepted = 0;

// Nombre de requête en attente (WAIT reçus en réponse à REQ)
unsigned int count_on_wait = 0;

// Nombre de requête refusée (REFUSE reçus en réponse à REQ)
unsigned int count_invalid = 0;

// Nombre de client qui se sont terminés correctement (ACC reçu en réponse à END)
unsigned int count_dispatched = 0;

// Nombre total de requêtes envoyées.
unsigned int request_sent = 0;

int *thread_running;

int getSocketDescriptor();
int send_request (int client_id, int request_id, int socket_fd, int32_t *message);

void error(const char *msg)
{
	perror(msg);
	exit(0);
}

void initThreadRunning()
{
	thread_running = malloc(num_clients * sizeof(int));
	for (int i = 0; i < num_clients; i++) {
		thread_running[i] = 0;
	}
}
// Vous devez modifier cette fonction pour faire l'envoie des requêtes
// Les ressources demandées par la requête doivent être choisit aléatoirement
// (sans dépasser le maximum pour le client). Les peuvent être positive ou négative.
// Assurez-vous que la dernière requête d'un client libère toute les ressources qu'il
// a jusqu'alors accumulées.
int
send_request (int client_id, int request_id, int socket_fd, int32_t *message)
{
	int32_t *msg;
	int size = sizeof(int32_t) * (num_resources+2);

	if (message == NULL)
	{
		msg = malloc(size);
		if (msg == NULL) {
			error("mourru");
		}
		msg[0] = REQ;
		msg[1] = client_id; 

		int32_t freeOrGet = 0;
		int32_t amount = 0;
		//choose random values to ask or release
		for(int i = 2; i < num_clients; i++)
		{
			freeOrGet = rand() % 2 ? -1 : 1;
			amount = (rand() % max_resources_per_client[client_id][i]);
			msg[i] = freeOrGet * amount;
		}
	}
	else {
		msg = message;
	}

	//printf("msg[0] client %d\n",msg[0]);

	char *data = (char *) msg;
	int remaining = size;
	int rc;
	while (remaining)
	{
		rc = write(socket_fd, data + size - remaining, remaining);
//		printf("client %d request %d write data socket %d :%p msg:%p send:%p sizeof(msg):%d remaining:%d rc:%d\n", client_id, request_id, socket_fd, data, &msg, data + size - remaining, size, remaining, rc);
		if (rc < 0) {
			error("client error on write");
		}
		remaining -= rc;
	}
	request_sent++;

	printf("le client envoie %d %d %d %d %d sur le socket %d\n", msg[0], msg[1], msg[2], msg[3], msg[4], socket_fd);

	size = 2 * sizeof(int32_t);
	int32_t *reponse = malloc(size);
	if (reponse == NULL) {
		error("pas de mémoire pour lire la réponse du serveur");
	}

	data = (char*) reponse;
	remaining = size;
	rc = 0;
	while (remaining)
	{
		rc = read(socket_fd, data + size - remaining, remaining);
//		printf("client %d request %d read data socket %d :%p msg:%p send:%p sizeof(msg):%d remaining:%d rc:%d\n", client_id, request_id, socket_fd,data, &reponse, data + size - remaining, size, remaining, rc);
		if (rc < 0) {
			printf("client %d requete %d va pas bien :(\n", client_id, request_id);
			error("client error on read");
		}
		remaining -= rc;
	}
	switch (reponse[0])
	{
	case REFUSE:
		count_invalid++;
		break;
	case ACK:
		if(msg[0]==END)
			count_dispatched++;
		else
			count_accepted++;
		break;
	case WAIT:
		count_on_wait++;
		break;
	default:
		printf("code de reponse inconnu %d\n", reponse[0]);
	}
	printf("le serveur a repondu %d sur le socket %d\n", reponse[0], socket_fd);
	return 0;
//	fprintf(stdout,"response: message_code %d replied to client %d request %d \n", reponse.message_code, reponse.clientId, reponse.reqId);

}

int connectServer()
{
	int socket_fd;
	struct sockaddr_in servAddr;

	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd < 0) {
		error("ERROR opening socket");
	}

	struct hostent *hst;
	hst = gethostbyname("localhost");
	if (hst == NULL) {
		error("no such host");
	}

	bzero((char *) &servAddr, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	bcopy((char *) hst->h_addr_list[0],
	      (char*) &servAddr.sin_addr.s_addr,
	      hst->h_length);
	servAddr.sin_port = htons(port_number);

	if (connect(socket_fd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
		error("error connecting");
	}
//	printf("socket descriptor gave socket %d\n", socket_fd);
	return socket_fd;
}

void *
ct_code (void *param)
{
	int socket_fd = connectServer();
	client_thread *ct = (client_thread *) param;

	for (unsigned int request_id = 0; request_id < num_request_per_client; request_id++)
	{
		printf("client %d sending request %d\n", ct->id, request_id);
		send_request (ct->id, request_id, socket_fd, NULL);
	}

	int32_t endRequest[5] = {END, -1, -1, -1, -1};
	endRequest[1] = ct->id;
	send_request(ct->id, num_request_per_client, socket_fd, endRequest);
	printf("thats all for client %d\n", ct->id);
	thread_running[ct->id] = 0;

	shutdown(socket_fd, 0);
	close(socket_fd);
	pthread_exit (NULL);

}


//
// Vous devez changer le contenu de cette fonction afin de régler le
// problème de synchronisation de la terminaison.
// Le client doit attendre que le serveur termine le traitement de chacune
// de ses requêtes avant de terminer l'exécution.
//
void
ct_wait_server ()
{
	int running;
	// TP2 TODO
	do {
		running = 0;
		for (int i = 0; i < num_clients; i++)
			if (thread_running[i]) {
				running++;
			}
		if (running) {
			sleep(1);
		}
	}
	while (running != 0);
	// TP2 TODO:END

}


void
ct_init (client_thread * ct)
{
	ct->id = count++;
	thread_running[ct->id] = 1;
	pthread_attr_init (&(ct->pt_attr));
	pthread_create (&(ct->pt_tid), &(ct->pt_attr), &ct_code, ct);
	pthread_detach (ct->pt_tid);
}

// Affiche les données recueillies lors de l'exécution du
// serveur.
// La branche else ne doit PAS être modifiée.
//
void
st_print_results (FILE * fd, bool verbose)
{
	if (fd == NULL) {
		fd = stdout;
	}
	if (verbose)
	{
		fprintf (fd, "\n---- Résultat du client ----\n");
		fprintf (fd, "Requêtes acceptées: %d\n", count_accepted);
		fprintf (fd, "Requêtes : %d\n", count_on_wait);
		fprintf (fd, "Requêtes invalides: %d\n", count_invalid);
		fprintf (fd, "Clients : %d\n", count_dispatched);
		fprintf (fd, "Requêtes envoyées: %d\n", request_sent);
	}
	else
	{
		fprintf (fd, "%d %d %d %d %d\n", count_accepted, count_on_wait,
		         count_invalid, count_dispatched, request_sent);
	} fprintf (fd, "%d %d %d %d %d\n", count_accepted, count_on_wait,
	           count_invalid, count_dispatched, request_sent);
}

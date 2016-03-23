#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <strings.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netdb.h>

#include "client_thread.h"


// Variable de configuration.
extern const int port_number;
extern const unsigned int num_clients;
extern const unsigned int num_resources;
extern const unsigned int num_request_per_client;
extern const unsigned int **max_resources_per_client;

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

int thread_running[NUM_CLIENTS];

void error(const char *msg)
{
	perror(msg);
	exit(0);
}

uint32_t* manageRequest(int clientId, int request_id, int socket_fd, uint32_t* toSend)
{
	uint32_t *reponse;
	return reponse;
}
// Vous devez modifier cette fonction pour faire l'envoie des requêtes
// Les ressources demandées par la requête doivent être choisit aléatoirement
// (sans dépasser le maximum pour le client). Les peuvent être positive ou négative.
// Assurez-vous que la dernière requête d'un client libère toute les ressources qu'il
// a jusqu'alors accumulées.
void
send_request (int client_id, int request_id, int socket_fd)
{

	message msg;
	msg.message_code = REQ;
	msg.clientId = client_id;
	msg.reqId = request_id;

	char *data = (char *) &msg;
	int remaining = sizeof(msg);
	int rc;
	while (remaining)
	{
		printf("write data:%p msg:%p send:%p sizeof(msg):%d remaining:%d rc:%d\n", data, &msg, data + sizeof(msg) - remaining, sizeof(msg), remaining, rc);
		rc = write(socket_fd, data + sizeof(msg) - remaining, remaining);
		if (rc < 0) {
			error("client error on write");
		}
		remaining -= rc;
	}
	request_sent++;

	message reponse;
	data = (char*) &reponse;
	remaining = sizeof(reponse);
	rc = 0;
	while (remaining)
	{
		rc = read(socket_fd, data + sizeof(reponse) - remaining, remaining);
		printf("read data:%p msg:%p send:%p sizeof(msg):%d remaining:%d rc:%d\n", data, &msg, data + sizeof(msg) - remaining, sizeof(msg), remaining, rc);
		if (rc < 0) {
			error("client request error on read");
		}
		remaining -= rc;
	}

//	fprintf(stdout,"response: message_code %d replied to client %d request %d \n", reponse.message_code, reponse.clientId, reponse.reqId);

}

int clientBegin(uint32_t *message) {
	int socket_fd = getSocketDescriptor();
	
	char *data = (char *) &message;
	int remaining = sizeof(message);
	int rc;
	while (remaining)
	{
		printf("write data:%p msg:%p send:%p sizeof(msg):%d remaining:%d rc:%d\n", data, &msg, data + sizeof(message) - remaining, sizeof(message), remaining, rc);
		rc = write(socket_fd, data + sizeof(message) - remaining, remaining);
		if (rc < 0) {
			error("client error on write");
		}
		remaining -= rc;
	}
	request_sent++;
	uint32_t *reponse = malloc(2*sizeof(uint32_t));
	if (reponse == NULL){
		error("failed to allocate memory to store reponse");
	}
	char *data = (char *) &msg;
	int remaining = sizeof(reponse);
	int rc;
	while (remaining)
	{
		rc = read(socket_fd, data + sizeof(reponse) - remaining, remaining);
		printf("read data:%p msg:%p send:%p sizeof(msg):%d remaining:%d rc:%d\n",data,&reponse,data + sizeof(reponse) - remaining,sizeof(reponse),remaining,rc);
		if (rc < 0) {
			error("server error on read");
		}
		remaining -= rc;
	}
	if ()


}

int getSocketDescriptor(){
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

	return socket_fd;
}
void *
ct_code (void *param)
{
	int socket_fd;
	client_thread *ct = (client_thread *) param;

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

	// TP2 TODO
	// Vous devez ici faire l'initialisation des petits clients (`INIT`).
	// TP2 TODO:END
	if (connect(socket_fd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
		error("error connecting");
	}
	int request_id = 0;
	for (unsigned int request_id = 0; request_id < num_request_per_client; request_id++)
	{
		printf("client %d sending request %d\n", ct->id, request_id);
		send_request (ct->id, request_id, socket_fd);
	}
	printf("thats all for client %d\n", ct->id);
	*ct->thread_running = 0;
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
	ct->thread_running  = &thread_running[ct->id];
}

void
ct_create_and_start (client_thread * ct)
{
	pthread_attr_init (&(ct->pt_attr));
	pthread_create (&(ct->pt_tid), &(ct->pt_attr), &ct_code, ct);
	pthread_detach (ct->pt_tid);
}

//
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

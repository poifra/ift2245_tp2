#include <stdlib.h>

#include "client_thread.h"
#include <sys/socket.h>

#include <stdint.h>

extern const unsigned int num_clients;
extern const unsigned int num_resources;

int send_request (int client_id, int request_id, int socket_fd,
		  int32_t * message);
void initThreadRunning ();
int connectServer ();

int
main (int argc, char *argv[argc + 1])
{

  initThreadRunning ();

  int32_t *theMotherOfAllRequests = malloc (sizeof (int32_t) * 5);
  if (theMotherOfAllRequests == NULL)
    {
      printf ("memory is kill\n");
      exit (-1);
    }

  theMotherOfAllRequests[0] = BEGIN;
  theMotherOfAllRequests[1] = num_resources;
  theMotherOfAllRequests[2] = num_clients;
  theMotherOfAllRequests[3] = -1;
  theMotherOfAllRequests[4] = -1;

  int begin_fd = connectServer ();
  if (send_request (0, 0, begin_fd, theMotherOfAllRequests) == -1)
    {
      printf ("The server said screw you\n");
      exit (-1);
    }

  shutdown (begin_fd, 0);
  close (begin_fd);

  client_thread client_threads[NUM_CLIENTS];

  for (unsigned int i = 0; i < num_clients; i++)
    {
      ct_init (&(client_threads[i]));
    }

  ct_wait_server ();

  // Affiche le journal.
  st_print_results (stdout, true);
  FILE *fp = fopen ("client_log", "w");
  if (fp == NULL)
    {
      fprintf (stderr, "Could not print log");
      return EXIT_FAILURE;
    }
  st_print_results (fp, false);
  fclose (fp);

  return EXIT_SUCCESS;
}

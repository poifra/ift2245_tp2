#include <stdlib.h>

#include "server_thread.h"
#include <pthread.h>

extern unsigned int num_server_threads;

int
main (int argc, char *argv[argc + 1])
{
  server_thread st[num_server_threads];

  // Ouvre un socket
  st_open_socket ();

  // Initialise le serveur.
  st_init ();

  // Part les fils d'exécution.
  for (unsigned int i = 0; i < num_server_threads; i++)
  {
    st[i].id = i;
    pthread_attr_init (&(st[i].pt_attr));
    pthread_create (&(st[i].pt_tid), &(st[i].pt_attr), &st_code, &(st[i]));
  }

  for (unsigned int i = 0; i < num_server_threads; i++)
    pthread_join (st[i].pt_tid, NULL);

  // Signale aux clients de se terminer.
  st_signal ();

  // Affiche le journal.
  st_print_results (stdout, true);
  FILE *fp = fopen("server_log", "w");
  if (fp == NULL)
  {
    fprintf(stderr, "Could not print log");
    return EXIT_FAILURE;
  }
  st_print_results (fp, false);
  fclose(fp);

  return EXIT_SUCCESS;
}

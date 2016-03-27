#ifndef CLIENTCONF_H
#define CLIENTCONF_H

// Client configuration
#define NUM_CLIENTS 5
#define NUM_RESOURCES 3

#ifndef NO_EXTERN
// Common configuration
extern const int port_number;
extern const unsigned int num_clients;
extern const unsigned int num_resources;
extern const unsigned int num_request_per_client;
extern const unsigned int max_resources_per_client[NUM_CLIENTS][NUM_RESOURCES];
#endif

#endif

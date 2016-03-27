#ifndef SERVERCONF_h
#define SERVERCONF_h

#define NUM_RESOURCES 3

#ifndef NO_EXTERN
// Listen port
extern const int port_number;

// Server configuration
extern const unsigned int num_resources;
extern unsigned int num_server_threads;
extern const unsigned int max_wait_time;
extern const unsigned int server_backlog_size;
extern const unsigned int available_resources[NUM_RESOURCES];
#endif

#endif

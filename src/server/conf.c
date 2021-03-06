#define NO_EXTERN
#include "conf.h"

// Listen port
const int port_number = 2016;

// Server configuration
#define NUM_RESOURCES 3
const unsigned int num_resources = NUM_RESOURCES;
unsigned int num_server_threads;
const unsigned int max_wait_time = 30;
const unsigned int server_backlog_size = 5;
const unsigned int available_resources[NUM_RESOURCES] = { 10, 5, 7 };

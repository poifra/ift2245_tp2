#define NO_EXTERN		// Prevents the definition of extern.
#include "conf.h"

// Common configuration
const int port_number = 2016;

// Client configuration
const unsigned int num_clients = NUM_CLIENTS;
const unsigned int num_resources = NUM_RESOURCES;
const unsigned int num_request_per_client = 5;
const unsigned int max_resources_per_client[NUM_CLIENTS][NUM_RESOURCES] = {
  {7, 5, 3},
  {3, 2, 2},
  {9, 1, 2},
  {2, 2, 2},
  {4, 3, 3},
/*	{42, 92, 10},
	{29, 29, 20},
	{91, 15, 2},
	{16, 7, 9},
	{23, 39, 41},*/

};

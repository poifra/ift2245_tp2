#ifndef PROTOCOL_H
#define PROTOCOL_H

#define NUM_CLIENTS 1
#define NUM_RESOURCES 3

typedef enum PROTOCOL_MESSAGE p_msg;
enum PROTOCOL_MESSAGE
{
  BEGIN,
  CLOSE,
  ACK,
  REQ,
  WAIT,
  END,
  REFUSE,
  INIT
};

typedef struct message message;
struct message {
	p_msg message_code;
	int clientId;
	int reqId;
	int *resourceRequest;
};

#endif

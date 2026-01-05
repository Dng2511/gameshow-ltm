#ifndef SSE_H
#define SSE_H

#include <sys/select.h>
#include <stdbool.h>

#define MAX_CLIENTS 100
#define BUFF_SIZE 1024

typedef struct {
    int client_sock;
    bool is_sse;
} Client;

extern Client clients[MAX_CLIENTS];
extern fd_set master_set;

void broadcast_message(const char *message, int sender_sock);
void remove_client(int client_sock);

#endif

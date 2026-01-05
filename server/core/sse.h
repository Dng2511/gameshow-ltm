#ifndef SSE_H
#define SSE_H

#include <sys/select.h>
#include <stdbool.h>
#include <json-c/json.h>

#define MAX_CLIENTS 100
#define BUFF_SIZE 4096

typedef struct {
    int client_sock;
    bool is_sse;
} Client;

extern Client clients[MAX_CLIENTS];
extern fd_set master_set;

void broadcast_message(const char *message, int sender_sock);
void broadcast_json_object(json_object *json_obj, int sender_sock);
void broadcast_json_object_to_clients(json_object *json_obj, int *client_socks, int num_clients);
void remove_client(int client_sock);

#endif

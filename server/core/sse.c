#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <json-c/json.h>
#include "sse.h"

void broadcast_message(const char *message, int sender_sock) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].client_sock != -1 && clients[i].is_sse && clients[i].client_sock != sender_sock) {
            char sse_message[BUFF_SIZE];
            snprintf(sse_message, sizeof(sse_message), "data: %s\n\n", message);
            send(clients[i].client_sock, sse_message, strlen(sse_message), 0);
        }
    }
}

void broadcast_json_object(json_object *json_obj, int sender_sock) {
    const char *json_str = json_object_to_json_string(json_obj);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].client_sock != -1 && clients[i].is_sse && clients[i].client_sock != sender_sock) {
            char sse_message[BUFF_SIZE];
            snprintf(sse_message, sizeof(sse_message), "data: %s\n\n", json_str);
            send(clients[i].client_sock, sse_message, strlen(sse_message), 0);
        }
    }
}

void broadcast_json_object_to_clients(json_object *json_obj, int *client_socks, int num_clients) {
    const char *json_str = json_object_to_json_string(json_obj);

    for (int i = 0; i < num_clients; i++) {
        int client_sock = client_socks[i];
        if (client_sock != -1) {
            char sse_message[BUFF_SIZE];
            snprintf(sse_message, sizeof(sse_message), "data: %s\n\n", json_str);
            send(client_sock, sse_message, strlen(sse_message), 0);
        }
    }
}

void remove_client(int client_sock) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].client_sock == client_sock) {
            close(clients[i].client_sock);
            clients[i].client_sock = -1;
            clients[i].is_sse = false;
            FD_CLR(client_sock, &master_set);
            printf("Client disconnected: %d\n", client_sock);
            break;
        }
    }
}

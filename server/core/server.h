#ifndef CORE_SERVER_H
#define CORE_SERVER_H
#include <netinet/in.h>

typedef struct ThreadArgs {
    int connfd;
    struct sockaddr_in cliaddr;
} ThreadArgs;

void handle_request(int client_sock);
void send_error_response(int client_sock, int status_code, const char *message);
int match_route(const char *route, const char *path, char *param);
void parse_request(const char *request, char *method, char *path);
void route_request(int client_sock, const char *request, const char *json);

#endif // CORE_SERVER_H
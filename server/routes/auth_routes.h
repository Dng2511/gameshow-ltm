#ifndef AUTH_ROUTER_H
#define AUTH_ROUTER_H

// Common utility functions
void handle_login(int client_sock, const char *request, const char *body);
void handle_register(int client_sock, const char *request, const char *body);
void handle_logout(int client_sock, const char *request, const char *body);
void test(int client_sock, const char *request, const char *body); 

#endif // AUTH_ROUTER_H
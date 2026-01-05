#ifndef API_ROUTER_H
#define API_ROUTER_H

void subcribe(int client_sock, const char *request, const char *body);
void get_data(int client_sock, const char *request, const char *body);
void send_message(int client_sock, const char *request, const char *body);
void choice(int client_sock, const char *request, const char *body);
void set_option(int client_sock, const char *request, const char *body);
void get_game_data(int client_sock, const char *request, const char *body);
void handle_choice(int client_sock, const char *request, const char *body);
void initialize_game(int client_sock, const char *request, const char *body);
void get_game_result(int client_sock, const char *request, const char *body);
void serve_image(int client_sock, const char *request, const char *body);

#endif
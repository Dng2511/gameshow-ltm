#ifndef ROOM_ROUTER_H
#define ROOM_ROUTER_H

// Common utility functions
void join_room(int client_sock, const char *request, const char *body);
void add_room(int client_sock, const char *request, const char *body);
void get_room_info(int client_sock, const char *request, const char *body);
void get_all_room_info(int client_sock, const char *request, const char *body);
void leave_room(int client_sock, const char *request, const char *body);
void disband_room(int client_sock, const char *request, const char *body);

#endif // ROOM_ROUTER_H
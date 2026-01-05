#ifndef UTILS_H
#define UTILS_H

// Common utility functions
void sendResponse(int client_sock, const char *response);
void sendError(int client_sock, const char *message, int error_code);
void send_cookie_response(int client_sock, const char *response, const char *username);
const char *extract_cookie(const char *request, const char *cookie_name);
int check_cookies(const char *request);
int get_user_id_from_request(const char *request);

#endif // UTILS_H
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <json-c/json.h>
#include "http_handler.h"
#include "sse.h"
#include "../features/room.h"
#include "../features/user.h"

extern Client clients[MAX_CLIENTS];
extern fd_set master_set;

void handle_http_request(int client_sock) {
    char buffer[BUFF_SIZE];
    int bytes_received = recv(client_sock, buffer, BUFF_SIZE - 1, 0);

    if (bytes_received <= 0) {
        printf("Client disconnected: %d\n", client_sock);
        remove_client(client_sock);
        return;
    }

    buffer[bytes_received] = '\0';

    if (strncmp(buffer, "OPTIONS", 7) == 0) {
        const char *response =
            "HTTP/1.1 204 No Content\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n"
            "Connection: keep-alive\r\n\r\n";
        send(client_sock, response, strlen(response), 0);
    }
    // SSE Subscription
    else if (strstr(buffer, "GET /api/subscribe")) {
        const char *sse_headers =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/event-stream\r\n"
            "Cache-Control: no-cache\r\n"
            "Connection: keep-alive\r\n"
            "Access-Control-Allow-Origin: *\r\n\r\n";
        send(client_sock, sse_headers, strlen(sse_headers), 0);

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].client_sock == client_sock) {
                clients[i].is_sse = true;
                printf("Client %d subscribed to SSE\n", client_sock);
                break;
            }
        }
    }
    // Handle GET /api/data
    else if (strncmp(buffer, "GET /api/data", 13) == 0) {
        struct json_object *json_response = json_object_new_object();
        json_object_object_add(json_response, "status", json_object_new_string("success"));
        json_object_object_add(json_response, "message", json_object_new_string("Hello, this is a JSON response"));

        const char *json_str = json_object_to_json_string(json_response);
        char response[BUFF_SIZE];
        snprintf(response, sizeof(response), 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Content-Length: %zu\r\n"
            "Connection: keep-alive\r\n\r\n%s",
            strlen(json_str), json_str);

        send(client_sock, response, strlen(response), 0);
        json_object_put(json_response);
    }
    else if (strncmp(buffer, "POST /api/message", 17) == 0) {
        char *json_start = strstr(buffer, "\r\n\r\n");
        if (json_start) {
            json_start += 4;
        } else {
            json_start = buffer;
        }

        struct json_object *json_request = json_tokener_parse(json_start);
        struct json_object *message_obj;

        const char *message_str = "No message received";
        if (json_request && json_object_object_get_ex(json_request, "message", &message_obj)) {
            message_str = json_object_get_string(message_obj);
        }

        broadcast_message(message_str, client_sock);

        struct json_object *json_response = json_object_new_object();
        json_object_object_add(json_response, "status", json_object_new_string("Message received"));
        json_object_object_add(json_response, "message", json_object_new_string(message_str));

        const char *json_str = json_object_to_json_string(json_response);
        char response[BUFF_SIZE];
        snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Content-Length: %zu\r\n"
            "Connection: keep-alive\r\n\r\n%s",
            strlen(json_str), json_str);

        send(client_sock, response, strlen(response), 0);

        json_object_put(json_request);
        json_object_put(json_response);
    }
    else if (strncmp(buffer, "POST /api/choice", 16) == 0) {
        // Similar logic for choice endpoint
        char *json_start = strstr(buffer, "\r\n\r\n");
        if (json_start) {
            json_start += 4;
        } else {
            json_start = buffer;
        }

        printf("Received JSON payload: %s\n", json_start);

        struct json_object *json_request = json_tokener_parse(json_start);
        struct json_object *choice_obj;

        const char *choice_str = "No choice received";
        if (json_request && json_object_object_get_ex(json_request, "choice", &choice_obj)) {
            choice_str = json_object_get_string(choice_obj);
        }

        struct json_object *json_response = json_object_new_object();
        json_object_object_add(json_response, "status", json_object_new_string("Choice received"));
        json_object_object_add(json_response, "choice", json_object_new_string(choice_str));

        const char *json_str = json_object_to_json_string(json_response);
        char response[BUFF_SIZE];
        snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Content-Length: %zu\r\n"
            "Connection: keep-alive\r\n\r\n%s",
            strlen(json_str), json_str);

        printf("Sending response: %s\n", response);
        send(client_sock, response, strlen(response), 0);

        json_object_put(json_request);
        json_object_put(json_response);
    }
    else {
        const char *response =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Access-Control-Allow-Origin: *\r\n\r\n"
            "Endpoint not found.";
        send(client_sock, response, strlen(response), 0);
    }
}

// Xử lý HTTP đơn giản cho một số endpoint mẫu:
// - Trả về CORS preflight cho OPTIONS
// - Xử lý SSE subscribe nếu client GET /api/subscribe
// - Các endpoint demo: GET /api/data, POST /api/message, POST /api/choice
// Hàm này là ví dụ xử lý HTTP; routing chính trong core/server.c

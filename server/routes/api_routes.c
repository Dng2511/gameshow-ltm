#include "api_routes.h"
#include "../core/sse.h"
#include "../features/game.h"
#include "../utils/utils.h"
#include "../middleware/cookies.h"
#include "../features/room.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <json-c/json.h>
#include <time.h>

extern Client clients[MAX_CLIENTS];
extern fd_set master_set;

void initialize_game(int client_sock, const char *request, const char *body) {
    printf("called initialize_game");
    struct json_object *json_request = json_tokener_parse(body);
    struct json_object *room_name_obj, *num_players_obj;
    const char *room_name = NULL;
    int num_players = 0;

    if (json_request && json_object_object_get_ex(json_request, "room_name", &room_name_obj) &&
        json_object_object_get_ex(json_request, "num_players", &num_players_obj)) {
        room_name = json_object_get_string(room_name_obj);
        num_players = json_object_get_int(num_players_obj);
    } else {
        sendError(client_sock, "Invalid request", 400);
        return;
    }

    Room *room = get_room_by_name(room_name);
    if (!room) {
        sendError(client_sock, "Waiting room not found", 500);
        return;
    }

    if (room->status == 1) {
        sendError(client_sock, "Room is already in-game", 400);
        return;
    }

    // if (!delete_room(room_name)) {
    //     sendError(client_sock, "Failed to delete existing room", 500);
    //     return;
    // }

    GameRoom *game_room = create_game_room(room_name, room->topic);
    if (!game_room) {
        sendError(client_sock, "Server is full", 500);
        return;
    }

    game_room->num_players = num_players;
    game_room->question_start_time = time(NULL); // Set the start time for the first question

    room->status = 1; // Set status to 1 (in-game)

    sendResponse(client_sock, "{\"status\":\"Game initialized\"}");
    // Create the broadcast JSON object
    struct json_object *broadcast_json = json_object_new_object();
    json_object_object_add(broadcast_json, "action", json_object_new_string("start"));
    json_object_object_add(broadcast_json, "room_name", json_object_new_string(room_name));

    // Broadcast the JSON object
    broadcast_json_object(broadcast_json, client_sock);

    json_object_put(broadcast_json);
}

void get_game_data(int client_sock, const char *request, const char *body) {
    printf("get_game_data called\n");
    struct json_object *json_request = json_tokener_parse(body);
    struct json_object *room_name_obj;
    struct json_object *username_obj;

    const char *room_name = NULL;
    const char *username = NULL;

    if (json_request && json_object_object_get_ex(json_request, "room_name", &room_name_obj) && json_object_object_get_ex(json_request, "username", &username_obj)) {
        room_name = json_object_get_string(room_name_obj);
        username = json_object_get_string(username_obj);
    } else {
        sendError(client_sock, "Invalid request", 400);
        return;
    }

    GameRoom *room = find_room(room_name);
    if (!room) {
        sendError(client_sock, "Room not found", 404);
        return;
    }

    int client_index = -1;
    for (int i = 0; i < room->num_players; i++) {
        if (strcmp(room->client_progress[i].username, username) == 0) {
            client_index = i;
            break;
        }
    }

    if (client_index == -1) {
        for (int i = 0; i < room->num_players; i++) {
            if (room->client_progress[i].username[0] == '\0') {
                strncpy(room->client_progress[i].username, username, 50);
                printf("New user assigned: %s\n", room->client_progress[i].username);
                client_index = i;
                break;
            }
        }
    }
    if (client_index == -1) {
        sendError(client_sock, "Server is full", 500);
        return;
    }

    int question_index = room->client_progress[client_index].current_question;

    if (question_index < 0 || question_index >= 10) {
        sendError(client_sock, "Invalid question index", 500);
        return;
    }

    struct json_object *json_response = json_object_new_object();
    json_object_object_add(json_response, "id", json_object_new_int(room->questions[question_index].id));
    json_object_object_add(json_response, "name1", json_object_new_string(room->questions[question_index].name1));
    json_object_object_add(json_response, "name2", json_object_new_string(room->questions[question_index].name2));
    json_object_object_add(json_response, "pic1", json_object_new_string(room->questions[question_index].pic1));
    json_object_object_add(json_response, "pic2", json_object_new_string(room->questions[question_index].pic2));
    json_object_object_add(json_response, "unit", json_object_new_string(room->questions[question_index].unit));
    json_object_object_add(json_response, "timestamp", json_object_new_int(room->question_start_time)); // Add timestamp

    struct json_object *clients_array = json_object_new_array();
        for (int i = 0; i < room->num_players; i++) {
            struct json_object *client_json = json_object_new_object();
            json_object_object_add(client_json, "username", json_object_new_string(room->client_progress[i].username));
            json_object_object_add(client_json, "score", json_object_new_int(room->client_progress[i].score));
            json_object_array_add(clients_array, client_json);
        }
    json_object_object_add(json_response, "clients", clients_array);

    // Create JSON array for used_powerup
    struct json_object *used_powerup_array = json_object_new_array();
    for (int i = 1; i < MAX_POWERUPS; i++) {
        json_object_array_add(used_powerup_array, json_object_new_int(room->client_progress[client_index].used_powerup[i]));
    }
    json_object_object_add(json_response, "used_powerup", used_powerup_array); // Add used_powerup array

    sendResponse(client_sock, json_object_to_json_string(json_response));
    json_object_put(json_response);
}

void handle_choice(int client_sock, const char *request, const char *body) {
    struct json_object *json_request = json_tokener_parse(body);
    struct json_object *choice_obj;
    struct json_object *room_name_obj;
    struct json_object *username_obj;
    struct json_object *remaining_time_obj;
    struct json_object *powerup_obj;
    int choice = 0;
    int remaining_time = 0;
    int powerup = 0;

    if (json_request && json_object_object_get_ex(json_request, "choice", &choice_obj)) {
        choice = json_object_get_int(choice_obj);
    }

    if (json_request && json_object_object_get_ex(json_request, "remaining_time", &remaining_time_obj)) {
        remaining_time = json_object_get_int(remaining_time_obj);
    }

    if (json_request && json_object_object_get_ex(json_request, "powerup", &powerup_obj)) {
        powerup = json_object_get_int(powerup_obj);
    }

    const char *room_name = NULL;
    const char *username = NULL;

    if (json_request && json_object_object_get_ex(json_request, "room_name", &room_name_obj) && json_object_object_get_ex(json_request, "username", &username_obj)) {
        room_name = json_object_get_string(room_name_obj);
        username = json_object_get_string(username_obj);
    } else {
        sendError(client_sock, "Invalid request", 400);
        return;
    }

    GameRoom *room = find_room(room_name);
    if (!room) {
        sendError(client_sock, "Room not found", 404);
        return;
    }

    int client_index = -1;
    for (int i = 0; i < room->num_players; i++) {
        if (strcmp(room->client_progress[i].username, username) == 0) {
            client_index = i;
            break;
        }
    }

    if (client_index == -1) {
        sendError(client_sock, "User not found", 500);
        return;
    }

    int question_index = room->client_progress[client_index].current_question;
    int base_score = (choice == room->questions[question_index].answer) ? 1000 : 0;
    int bonus = 0;
    const char *powerup_msg = "";

    if (base_score == 1000) {
        room->client_progress[client_index].streak++;
        if (powerup == 3) {
            bonus += 1000; // Power-up 3: +1000 score if correct
            powerup_msg = "+1000 score received";
        }
    } else {
        if (powerup == 2) {
            room->client_progress[client_index].streak = room->client_progress[client_index].streak; // Power-up 2: Protect streak if wrong
            powerup_msg = "Streak protected";
        } else {
            room->client_progress[client_index].streak = 0;
        }
    }

    bonus += (base_score == 1000) ? remaining_time * 10 * room->client_progress[client_index].streak : 0;
    int total_score = base_score + bonus;

    if (powerup == 1) {
        total_score *= 2; // Power-up 1: Double the total score
        powerup_msg = "Doubled score received";
    }

    room->client_progress[client_index].answered = 1;
    room->client_progress[client_index].score += total_score;
    room->client_progress[client_index].used_powerup[powerup] = 1;

    // Create the broadcast JSON object
    struct json_object *broadcast_json = json_object_new_object();
    json_object_object_add(broadcast_json, "action", json_object_new_string("an user answered"));
    json_object_object_add(broadcast_json, "room_name", json_object_new_string(room_name));

    // Broadcast the JSON object
    broadcast_json_object(broadcast_json, client_sock);

    json_object_put(broadcast_json);

    // Check if all clients have answered
    printf("---------\n");
    for (int i = 0; i < room->num_players; i++) {
        printf("%s:  %d\n", room->client_progress[i].username, room->client_progress[i].answered);
    }
    printf("current: %d\n", room->current_question_index);
    printf("---------\n");

    int all_answered = 1;
    for (int i = 0; i < room->num_players; i++) {
        if (room->client_progress[i].username[0] != '\0' && !room->client_progress[i].answered) {
            all_answered = 0;
            break;
        }
    }

    if (all_answered) {
        room->all_answered = 1;
        room->all_answered_time = time(NULL);
    }

    struct json_object *json_response = json_object_new_object();
    json_object_object_add(json_response, "base_score", json_object_new_int(base_score));
    json_object_object_add(json_response, "bonus", json_object_new_int(bonus));
    json_object_object_add(json_response, "total_score", json_object_new_int(total_score));
    json_object_object_add(json_response, "value1", json_object_new_int(room->questions[question_index].value1));
    json_object_object_add(json_response, "value2", json_object_new_int(room->questions[question_index].value2));
    json_object_object_add(json_response, "remaining_time", json_object_new_int(remaining_time));
    json_object_object_add(json_response, "streak", json_object_new_int(room->client_progress[client_index].streak));
    json_object_object_add(json_response, "powerup_msg", json_object_new_string(powerup_msg)); // Add powerup_msg to response

    sendResponse(client_sock, json_object_to_json_string(json_response));

    json_object_put(json_request);
    json_object_put(json_response);
}

void get_game_result(int client_sock, const char *request, const char *body) {
    struct json_object *json_request = json_tokener_parse(body);
    struct json_object *room_name_obj;
    const char *room_name = NULL;
    if (json_request && json_object_object_get_ex(json_request, "room_name", &room_name_obj)) {
        room_name = json_object_get_string(room_name_obj);
    } else {
        sendError(client_sock, "Invalid request", 400);
        return;
    }

    GameRoom *room = find_room(room_name);
    if (!room) {
        sendError(client_sock, "Room not found", 404);
        return;
    }

    struct json_object *json_response = json_object_new_array();

    for (int i = 0; i < room->num_players; i++) {
        if (room->client_progress[i].username[0] != '\0') {
            struct json_object *json_player = json_object_new_object();
            json_object_object_add(json_player, "username", json_object_new_string(room->client_progress[i].username));
            json_object_object_add(json_player, "score", json_object_new_int(room->client_progress[i].score));
            json_object_array_add(json_response, json_player);
        }
    }

    Room *waiting_room = get_room_by_name(room_name);
    if (waiting_room) {
        waiting_room->status = 0; // Set status to 0 (waiting) when game ends
    }

    sendResponse(client_sock, json_object_to_json_string(json_response));
    json_object_put(json_response);
}

void set_option(int client_sock, const char *request, const char *body) {
    const char *response =
        "HTTP/1.1 204 No Content\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Connection: keep-alive\r\n\r\n";
    send(client_sock, response, strlen(response), 0);
}

void subcribe(int client_sock, const char *request, const char *body) {
    // printf("Subscribing client: %d\n", client_sock);
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

void get_data(int client_sock, const char *request, const char *body) {
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

void send_message(int client_sock, const char *request, const char *body) {
    struct json_object *json_request = json_tokener_parse(body);
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

void choice(int client_sock, const char *request, const char *body) {
    // Similar logic for choice endpoint
    printf("Received JSON payload: %s\n", body);

    struct json_object *json_request = json_tokener_parse(body);
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

// Serve image files from server/database/ based on URL path /images/<relative_path>
// Security: prevent path traversal by rejecting '..' and leading '/'.
void serve_image(int client_sock, const char *request, const char *body) {
    // Parse request line to extract path
    char method[16], path[512];
    sscanf(request, "%s %s", method, path);

    // Expect GET
    if (strcmp(method, "GET") != 0) {
        sendError(client_sock, "Method Not Allowed", 405);
        return;
    }

    const char *prefix = "/images/";
    if (strncmp(path, prefix, strlen(prefix)) != 0) {
        sendError(client_sock, "Not Found", 404);
        return;
    }

    const char *rel = path + strlen(prefix); // relative path after /images/

    // Reject path traversal
    if (strstr(rel, "..") != NULL || rel[0] == '/' || rel[0] == '\0') {
        sendError(client_sock, "Invalid image path", 400);
        return;
    }

    // Build file path under server/database/
    char file_path[1024];
    snprintf(file_path, sizeof(file_path), "database/%s", rel);

    // Try to open file
    FILE *f = fopen(file_path, "rb");
    if (!f) {
        perror("fopen");
        sendError(client_sock, "Image not found", 404);
        return;
    }

    // Determine file size
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    // Guess content type from extension
    const char *ext = strrchr(rel, '.');
    const char *content_type = "application/octet-stream";
    if (ext) {
        if (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0) content_type = "image/jpeg";
        else if (strcasecmp(ext, ".png") == 0) content_type = "image/png";
        else if (strcasecmp(ext, ".gif") == 0) content_type = "image/gif";
        else if (strcasecmp(ext, ".webp") == 0) content_type = "image/webp";
    }

    // Send headers
    char header[512];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n\r\n",
        content_type, size);

    send(client_sock, header, header_len, 0);

    // Stream file in blocks
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        send(client_sock, buf, n, 0);
    }

    fclose(f);
    // Close the connection on server side; caller may also close
}
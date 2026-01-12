#include "room_routes.h"

#include "../features/user.h"
#include "../features/room.h"
#include "../core/sse.h"
#include "../utils/utils.h"

#include <json-c/json.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "../middleware/cookies.h"

void join_room(int client_sock, const char *request, const char *body) {
    // Locate the JSON body in the HTTP request
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

    Room *room = get_room_by_name(room_name);
    if (!room) {
        sendError(client_sock, "Room not found", 404);
        return;
    }

    if (room->status == 1) {
        sendError(client_sock, "Room is in-game", 400);
        return;
    }

    struct json_object *json_response = json_object_new_object();

    if ((username != NULL) && room_name && !check_user_in_room(room_name, username) && add_user_to_room(room_name, username)) {
        json_object_object_add(json_response, "status", json_object_new_string("success"));

        // Get the list of users in the room
        struct json_object *users_array = json_object_new_array();
        if (room) {
            UserNode *current = room->users;
            while (current != NULL) {
                struct json_object *user_obj = json_object_new_object();
                json_object_object_add(user_obj, "username", json_object_new_string(current->user->username));
                json_object_array_add(users_array, user_obj);
                current = current->next;
            }
        }
        json_object_object_add(json_response, "users", users_array);
        sendResponse(client_sock, json_object_to_json_string(json_response));
        // Create the broadcast JSON object
        struct json_object *broadcast_json = json_object_new_object();
        json_object_object_add(broadcast_json, "action", json_object_new_string("join"));
        json_object_object_add(broadcast_json, "username", json_object_new_string(username));
        json_object_object_add(broadcast_json, "room_name", json_object_new_string(room_name));

        // Broadcast the JSON object
        broadcast_json_object(broadcast_json, client_sock);

        json_object_put(broadcast_json);
    } else {
        json_object_object_add(json_response, "status", json_object_new_string("failure"));
        json_object_object_add(json_response, "message", json_object_new_string("Room not found or room is full"));
        sendError(client_sock, json_object_to_json_string(json_response), 500);
    }

    json_object_put(json_request);
    json_object_put(json_response);
}

void leave_room(int client_sock, const char *request, const char *body) {
    struct json_object *json_request = json_tokener_parse(body);
    struct json_object *room_name_obj;
    struct json_object *username_obj;

    const char *room_name = NULL;
    const char *username = NULL;

    // if (check_cookies(request)) {
    //     const char *session_id = extract_cookie(request, "session_id");
    //     username = validate_session(session_id);
    //     printf("Current User: %s", username);
    // }

    if (json_request && json_object_object_get_ex(json_request, "room_name", &room_name_obj) && json_object_object_get_ex(json_request, "username", &username_obj)) {
        room_name = json_object_get_string(room_name_obj);
        username = json_object_get_string(username_obj);
    } else {
        sendError(client_sock, "Invalid request", 400);
        return;
    }

    // if user who left is the host, disband the room instead

    Room *room = get_room_by_name(room_name);
    if (strcmp(username,room->host->username)==0) {
        disband_room(client_sock, request, body);
        return;
    }

    struct json_object *json_response = json_object_new_object();
    if (room_name && delete_user_from_room(room_name, username)) {

        json_object_object_add(json_response, "status", json_object_new_string("success"));
        json_object_object_add(json_response, "message", json_object_new_string("User left the room"));
        sendResponse(client_sock, json_object_to_json_string(json_response));
        // Create the broadcast JSON object
        struct json_object *broadcast_json = json_object_new_object();
        json_object_object_add(broadcast_json, "action", json_object_new_string("leave"));
        json_object_object_add(broadcast_json, "username", json_object_new_string(username));
        json_object_object_add(broadcast_json, "room_name", json_object_new_string(room_name));

        // Broadcast the JSON object
        broadcast_json_object(broadcast_json, client_sock);

        json_object_put(broadcast_json);
    } else {
        json_object_object_add(json_response, "status", json_object_new_string("failure"));
        json_object_object_add(json_response, "message", json_object_new_string("Room not found or user not in the room"));
        sendError(client_sock, "Invalid request", 500);
    }

    json_object_put(json_request);
    json_object_put(json_response);
}

void add_room(int client_sock, const char *request, const char *body) {
    struct json_object *json_request = json_tokener_parse(body);
    struct json_object *room_name_obj, *username_obj, *capacity_obj, *topic_obj;

    const char *room_name = NULL;
    const char *username = NULL;
    const char * topic = NULL;

    // if (check_cookies(request)) {
    //     const char *session_id = extract_cookie(request, "session_id");
    //     username = validate_session(session_id);
    //     printf("%s", username);
    // }

    int capacity = 0;

    struct json_object *num_questions_obj;
    int num_questions = 0;
    if (json_request && json_object_object_get_ex(json_request, "room_name", &room_name_obj) && 
        json_object_object_get_ex(json_request, "username", &username_obj) &&
        json_object_object_get_ex(json_request, "capacity", &capacity_obj) && 
        json_object_object_get_ex(json_request, "topic", &topic_obj)) {
        room_name = json_object_get_string(room_name_obj);
        username = json_object_get_string(username_obj);
        capacity = json_object_get_int(capacity_obj);
        topic = json_object_get_string(topic_obj);
        if (json_object_object_get_ex(json_request, "num_questions", &num_questions_obj)) {
            num_questions = json_object_get_int(num_questions_obj);
            if (num_questions < 1) num_questions = 1;
            if (num_questions > 20) num_questions = 20;
        } else {
            num_questions = 20;
        }
        // username = json_object_get_string(username_obj);
        if (capacity < 2) {
            capacity = 2;
        } else if (capacity > 10) {
            capacity = 10;
        }
    } else {
        sendError(client_sock, "Invalid request", 400);
        return;
    }

    if (get_room_by_name(room_name) != NULL) {
        sendError(client_sock, "Room already exists", 400);
        return;
    }

    struct json_object *json_response = json_object_new_object();
    if (room_name && create_room(room_name, capacity, topic, username)) {
        Room *r = get_room_by_name(room_name);
        if (r) r->num_questions = num_questions;
        json_object_object_add(json_response, "status", json_object_new_string("success"));
        json_object_object_add(json_response, "message", json_object_new_string("Room created successfully"));
        sendResponse(client_sock, json_object_to_json_string(json_response));
        // Create the broadcast JSON object
        struct json_object *broadcast_json = json_object_new_object();
        json_object_object_add(broadcast_json, "action", json_object_new_string("create"));
        json_object_object_add(broadcast_json, "room_name", json_object_new_string(room_name));
        json_object_object_add(broadcast_json, "capacity", json_object_new_int(capacity));
        json_object_object_add(broadcast_json, "topic", json_object_new_string(topic));
        json_object_object_add(broadcast_json, "host", json_object_new_string(username));
        json_object_object_add(broadcast_json, "room_status", json_object_new_int(0));

        // Add the array of players (initially only the host)
        struct json_object *players_array = json_object_new_array();
        struct json_object *player_obj = json_object_new_object();
        json_object_object_add(player_obj, "username", json_object_new_string(username));
        json_object_array_add(players_array, player_obj);
        json_object_object_add(broadcast_json, "users", players_array);
        // Broadcast the JSON object
        broadcast_json_object(broadcast_json, client_sock);

        json_object_put(broadcast_json);
    } else {
        json_object_object_add(json_response, "status", json_object_new_string("failure"));
        json_object_object_add(json_response, "message", json_object_new_string("Room creation failed"));
        sendError(client_sock, json_object_to_json_string(json_response), 500);
    }

    json_object_put(json_request);
    json_object_put(json_response);
}

void disband_room(int client_sock, const char *request, const char *body) {
    struct json_object *json_request = json_tokener_parse(body);
    struct json_object *room_name_obj;
    struct json_object *username_obj;

    const char *room_name = NULL;
    // const char *username = NULL;

    // if (check_cookies(request)) {
    //     const char *session_id = extract_cookie(request, "session_id");
    //     username = validate_session(session_id);
    //     printf("%s", username);
    // }

    if (json_request && json_object_object_get_ex(json_request, "room_name", &room_name_obj) && json_object_object_get_ex(json_request, "username", &username_obj)) {
        room_name = json_object_get_string(room_name_obj);
        // username = json_object_get_string(username_obj);
    } else {
        sendError(client_sock, "Invalid request", 400);
        return;
    }

    struct json_object *json_response = json_object_new_object();
    if (room_name && delete_room(room_name)) {
        json_object_object_add(json_response, "status", json_object_new_string("success"));
        json_object_object_add(json_response, "message", json_object_new_string("Room disbanded successfully"));
        sendResponse(client_sock, json_object_to_json_string(json_response));
        // Create the broadcast JSON object
        struct json_object *broadcast_json = json_object_new_object();
        json_object_object_add(broadcast_json, "action", json_object_new_string("disband"));
        json_object_object_add(broadcast_json, "room_name", json_object_new_string(room_name));

        // Broadcast the JSON object
        broadcast_json_object(broadcast_json, client_sock);

        json_object_put(broadcast_json);
    } else {
        json_object_object_add(json_response, "status", json_object_new_string("failure"));
        json_object_object_add(json_response, "message", json_object_new_string("Room not found"));
        sendError(client_sock, json_object_to_json_string(json_response),500);
    }

    json_object_put(json_request);
    json_object_put(json_response);
}

void get_room_info(int client_sock, const char *request, const char *body) {
    struct json_object *json_request = json_tokener_parse(body);
    struct json_object *room_name_obj;

    const char *room_name = NULL;
    if (json_request && json_object_object_get_ex(json_request, "room_name", &room_name_obj)) {
        room_name = json_object_get_string(room_name_obj);
    } else {
        sendError(client_sock, "Invalid request", 400);
        return;
    }

    struct json_object *json_response = json_object_new_object();
    Room *room = get_room_by_name(room_name);
    if (room) {
        json_object_object_add(json_response, "status", json_object_new_string("success"));
        json_object_object_add(json_response, "room_name", json_object_new_string(room->name));
        json_object_object_add(json_response, "capacity", json_object_new_int(room->capacity));
        json_object_object_add(json_response, "topic", json_object_new_string(room->topic));
        json_object_object_add(json_response, "host", json_object_new_string(room->host->username));

        // Retrieve the list of users in the room
        struct json_object *users_array = json_object_new_array();
        UserNode *current = room->users;

        while (current != NULL) {
            struct json_object *user_obj = json_object_new_object();
            json_object_object_add(user_obj, "username", json_object_new_string(current->user->username));
            json_object_array_add(users_array, user_obj);
            current = current->next;
        }
        
        json_object_object_add(json_response, "users", users_array);
        sendResponse(client_sock, json_object_to_json_string(json_response));
    } else {
        json_object_object_add(json_response, "status", json_object_new_string("failure"));
        json_object_object_add(json_response, "message", json_object_new_string("Room not found"));
        sendError(client_sock, json_object_to_json_string(json_response),500);
    }

    json_object_put(json_request);
    json_object_put(json_response);
}

void get_all_room_info(int client_sock, const char *request, const char *body) {
    struct json_object *json_response = json_object_new_object();
    struct json_object *rooms_array = json_object_new_array();

    Room all_rooms[500];
    int room_count = get_all_rooms(all_rooms);
    printf("%d\n", room_count);
    for (int i = 0; i < room_count; i++) {
        Room current = all_rooms[i];
        struct json_object *room_obj = json_object_new_object();
        json_object_object_add(room_obj, "room_name", json_object_new_string(current.name));
        json_object_object_add(room_obj, "capacity", json_object_new_int(current.capacity));
        json_object_object_add(room_obj, "topic", json_object_new_string(current.topic));
        json_object_object_add(room_obj, "host", json_object_new_string(current.host->username));
        json_object_object_add(room_obj, "status", json_object_new_int(current.status));

        struct json_object *users_array = json_object_new_array();
        UserNode *user_current = current.users;
        while (user_current != NULL) {
            struct json_object *user_obj = json_object_new_object();
            json_object_object_add(user_obj, "username", json_object_new_string(user_current->user->username));
            json_object_array_add(users_array, user_obj);
            user_current = user_current->next;
        }
        json_object_object_add(room_obj, "users", users_array);
        json_object_array_add(rooms_array, room_obj);
    }
    json_object_object_add(json_response, "rooms", rooms_array);
    sendResponse(client_sock, json_object_to_json_string(json_response));

    // free(all_rooms);
    json_object_put(json_response);
}
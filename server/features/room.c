#include "room.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "user.h"

#define MAX_ROOMS 500

Room rooms[MAX_ROOMS];
int room_count = 0;
pthread_mutex_t room_mutex = PTHREAD_MUTEX_INITIALIZER;

int countUserRoom(UserNode *users) {
    int count = 0;
    UserNode *current = users;
    while (current != NULL) {
        ++count;
        current = current->next;
    }
    return count;
}

// Create a new room, Curuser se la thang dau tien va la host
int create_room(const char *name, int capacity, const char *topic, const char *curUser) {
    pthread_mutex_lock(&room_mutex);
    if (room_count >= MAX_ROOMS) {
        pthread_mutex_unlock(&room_mutex);
        return 0; // Maximum number of rooms reached
    }

    Room *new_room = &rooms[room_count++];
    strncpy(new_room->name, name, sizeof(new_room->name) - 1);
    new_room->name[sizeof(new_room->name) - 1] = '\0';
    new_room->capacity = capacity;
    strncpy(new_room->topic, topic, sizeof(new_room->topic) - 1);
    new_room->topic[sizeof(new_room->topic) - 1] = '\0';
    User *cur = find_user(curUser);
    new_room->host = cur;
    new_room->status = 0;

    // Create a new user node for the host
    UserNode *new_user_node = malloc(sizeof(UserNode));
    if (new_user_node == NULL) {
        pthread_mutex_unlock(&room_mutex);
        return 0; // Memory allocation failed
    }
    new_user_node->user = cur;
    new_user_node->next = NULL;
    new_room->users = new_user_node;

    pthread_mutex_unlock(&room_mutex);
    return 1; // Room created successfully
}

// Delete a room -> WIP (cos the lam kieu tim id thay vi ten -> de tim nhung kho xoa)
int delete_room(const char *name) {
    pthread_mutex_lock(&room_mutex);
    for (int i = 0; i < room_count; ++i) {
        if (strcmp(rooms[i].name, name) == 0) {
            // Free all users in the room
            UserNode *current = rooms[i].users;
            while (current != NULL) {
                UserNode *temp = current;
                current = current->next;
                free(temp);
            }

            // Shift remaining rooms
            for (int j = i; j < room_count - 1; ++j) {
                rooms[j] = rooms[j + 1];
            }
            --room_count;
            pthread_mutex_unlock(&room_mutex);
            return 1; // Room deleted successfully
        }
    }
    pthread_mutex_unlock(&room_mutex);
    return 0; // Room not found
}

// Add a user to a room (cos the lam kieu tim id thay vi ten -> de tim nhung kho xoa)
// User se la curUser nma t lam tong quat
// Host se luon la thang o cuoi danh sach
int add_user_to_room(const char *room_name, const char * curUser) {
    pthread_mutex_lock(&room_mutex);
    User * user = find_user(curUser);

    for (int i = 0; i < room_count; ++i) {
        if (strcmp(rooms[i].name, room_name) == 0) {
            // Check if room is full

            if (countUserRoom(rooms[i].users) >= rooms[i].capacity) {
                pthread_mutex_unlock(&room_mutex);
                return 0; // Room is full
            }
            
            // Add user to the room (stack)
            UserNode *new_user_node = malloc(sizeof(UserNode));
            if (new_user_node == NULL) {
                pthread_mutex_unlock(&room_mutex);
                return 0; // Memory allocation failed
            }
            new_user_node->user = user;
            new_user_node->next = rooms[i].users;
            rooms[i].users = new_user_node;

            pthread_mutex_unlock(&room_mutex);
            return 1; // User added successfully
        }
    }
    pthread_mutex_unlock(&room_mutex);
    return 0; // Room not found
}

// Delete a user from a room
int delete_user_from_room(const char *room_name, const char * curUser) {
    pthread_mutex_lock(&room_mutex);
    User * user = find_user(curUser);

    for (int i = 0; i < room_count; ++i) {
        if (strcmp(rooms[i].name, room_name) == 0) {
            UserNode *current = rooms[i].users;
            UserNode *prev = NULL;
            while (current != NULL) {
                if (current->user == user) {
                    if (prev == NULL) {
                        rooms[i].users = current->next;
                    } else {
                        prev->next = current->next;
                    }
                    free(current);
                    pthread_mutex_unlock(&room_mutex);
                    return 1; // User deleted successfully
                }
                prev = current;
                current = current->next;
            }
            pthread_mutex_unlock(&room_mutex);
            return 0; // User not found in the room
        }
    }
    pthread_mutex_unlock(&room_mutex);
    return 0; // Room not found
}

int check_user_in_room(const char *room_name, const char * curUser) {
    pthread_mutex_lock(&room_mutex);
    User * user = find_user(curUser);

    for (int i = 0; i < room_count; ++i) {
        if (strcmp(rooms[i].name, room_name) == 0) {
            UserNode *current = rooms[i].users;
            while (current != NULL) {
                if (current->user == user) {
                    pthread_mutex_unlock(&room_mutex);
                    return 1; // User found in the room
                }
                current = current->next;
            }
            pthread_mutex_unlock(&room_mutex);
            return 0; // User not found in the room
        }
    }
    pthread_mutex_unlock(&room_mutex);
    return 0; // Room not found
}

Room* get_room_by_name(const char *name) {
    pthread_mutex_lock(&room_mutex);
    for (int i = 0; i < room_count; ++i) {
        if (strcmp(rooms[i].name, name) == 0) {
            pthread_mutex_unlock(&room_mutex);
            return &rooms[i];
        }
    }
    pthread_mutex_unlock(&room_mutex);
    return NULL; // Room not found
}

int get_all_rooms(Room all_rooms[]) {
    pthread_mutex_lock(&room_mutex);
    
    for (int i = 0; i < room_count; i++) {
        all_rooms[i] = rooms[i];
    }

    pthread_mutex_unlock(&room_mutex);
    return room_count;
}
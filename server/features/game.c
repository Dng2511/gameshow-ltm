#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <json-c/json.h>
#include "../core/sse.h"

#define TOPIC "database/country.txt"
#define NUM_QUESTIONS 5

GameRoom game_rooms[MAX_ROOMS];
int num_rooms = 0;

void load_data(const char *filename, char data[MAX_LINES][MAX_LINE_LENGTH]) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    int i = 0;
    while (fgets(data[i], MAX_LINE_LENGTH, file) && i < MAX_LINES) {
        data[i][strcspn(data[i], "\n")] = '\0'; // Remove newline character
        i++;
    }

    fclose(file);
}

// Đọc toàn bộ dòng từ file data vào mảng `data` (mỗi dòng 1 entry), xử lý EOF.

void shuffle(int *array, size_t n) {
    if (n > 1) {
        size_t i;
        for (i = 0; i < n - 1; i++) {
            size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
            int t = array[j];
            array[j] = array[i];
            array[i] = t;
        }
    }
}

// Hoán vị mảng các chỉ số (Fisher–Yates) để random hóa lựa chọn câu hỏi.

void create_questions(GameRoom *room, const char *topic) {
    char data[MAX_LINES][MAX_LINE_LENGTH];
    
    // Construct the file path
    char file_path[MAX_LINE_LENGTH];
    snprintf(file_path, sizeof(file_path), "database/%s.txt", topic);

    // Load data from the constructed file path
    load_data(file_path, data);

    srand(time(NULL));
    int indices[MAX_LINES];
    for (int i = 0; i < MAX_LINES; i++) {
        indices[i] = i;
    }

    shuffle(indices, MAX_LINES);

    // Pick the first NUM_QUESTIONS + 1 distinct entries
    int selected_indices[NUM_QUESTIONS + 1];
    for (int i = 0; i < NUM_QUESTIONS + 1; i++) {
        selected_indices[i] = indices[i];
    }

    // Create NUM_QUESTIONS questions from the selected entries
    for (int i = 0; i < NUM_QUESTIONS; i++) {
        int idx1 = selected_indices[i];
        int idx2 = selected_indices[(i + 1) % (NUM_QUESTIONS + 1)];

        if (i % 2 == 1) {
            // Swap the order for odd indices
            sscanf(data[idx2], "%d|%49[^|]|%lld|%49[^|]|%199[^\n]", 
                &room->questions[i].id, 
                room->questions[i].name1, 
                &room->questions[i].value1, 
                room->questions[i].unit, 
                room->questions[i].pic1);

            sscanf(data[idx1], "%d|%49[^|]|%lld|%49[^|]|%199[^\n]", 
                &room->questions[i].id, 
                room->questions[i].name2, 
                &room->questions[i].value2, 
                room->questions[i].unit, 
                room->questions[i].pic2);
        } else {
            // Default order for even indices
            sscanf(data[idx1], "%d|%49[^|]|%lld|%49[^|]|%199[^\n]", 
                &room->questions[i].id, 
                room->questions[i].name1, 
                &room->questions[i].value1, 
                room->questions[i].unit, 
                room->questions[i].pic1);

            sscanf(data[idx2], "%d|%49[^|]|%lld|%49[^|]|%199[^\n]", 
                &room->questions[i].id, 
                room->questions[i].name2, 
                &room->questions[i].value2, 
                room->questions[i].unit, 
                room->questions[i].pic2);
        }

        printf("Raw line 1: %s\n", data[idx1]);
        printf("Raw line 2: %s\n", data[idx2]);
        printf("Parsed 1: %d | %s | %lld | %s | %s\n", 
        room->questions[i].id, room->questions[i].name1, room->questions[i].value1, room->questions[i].unit, room->questions[i].pic1);
        printf("Parsed 2: %d | %s | %lld | %s | %s\n", 
        room->questions[i].id, room->questions[i].name2, room->questions[i].value2, room->questions[i].unit, room->questions[i].pic2);

        room->questions[i].answer = (room->questions[i].value1 >= room->questions[i].value2) ? 1 : 2;
        printf("%d\n", room->questions[i].answer);
    }

    // Initialize client progress
    for (int i = 0; i < MAX_PLAYERS; i++) {
        room->client_progress[i].username[0] = '\0'; // Empty string indicates no user
        room->client_progress[i].current_question = 0;
        room->client_progress[i].answered = 0;
        room->client_progress[i].score = 0;
        room->client_progress[i].streak = 0;
        for (int j = 1; j < MAX_POWERUPS; j++) {
            room->client_progress[i].used_powerup[j] = 0; // Initialize used_powerup array
        }
    }
    room->current_question_index = 0;
    room->all_answered = 0;
    room->all_answered_time = 0;
}

// Tạo danh sách câu hỏi cho phòng chơi `room` dựa trên topic:
// - load file topic, shuffle, chọn NUM_QUESTIONS, phân tích chuỗi vào cấu trúc question,
// - khởi tạo trạng thái tiến độ của client trong phòng.

GameRoom* create_game_room(const char *room_name, const char *topic) {
    if (num_rooms < MAX_ROOMS) {
        GameRoom *new_room = &game_rooms[num_rooms++];
        strncpy(new_room->room_name, room_name, sizeof(new_room->room_name));
        create_questions(new_room, topic);
        return new_room;
    }
    return NULL; // No available room slots
}

// Tạo phòng chơi mới nếu còn slot, gọi create_questions để chuẩn bị câu hỏi.

GameRoom* find_room(const char *room_name) {
    for (int i = 0; i < num_rooms; i++) {
        if (strcmp(game_rooms[i].room_name, room_name) == 0) {
            return &game_rooms[i];
        }
    }
    return NULL;
}

// Tìm phòng theo tên và trả con trỏ tới GameRoom nếu tồn tại.

void delete_game_room(const char *room_name) {
    for (int i = 0; i < num_rooms; i++) {
        if (strcmp(game_rooms[i].room_name, room_name) == 0) {
            // Shift remaining rooms
            for (int j = i; j < num_rooms - 1; j++) {
                game_rooms[j] = game_rooms[j + 1];
            }
            num_rooms--;
            printf("Game room %s deleted\n", room_name);
            return;
        }
    }
    printf("Game room %s not found\n", room_name);
}

// Xóa phòng chơi theo tên (ghi đè dịch chuyển các phần tử trong mảng và giảm num_rooms).

void check_timeout(GameRoom *room) { 
    time_t current_time = time(NULL); 
    int remain_time = 20 - (int)difftime(current_time, room->question_start_time);
    printf("Checking timeout for room: %s, remain time: %d\n", room->room_name, remain_time);

    // Handle delayed deletion of the game room
    if (room->all_answered == 2) {
        int delay_elapsed = (int)difftime(current_time, room->all_answered_time);
        if (delay_elapsed >= 2) {
            delete_game_room(room->room_name);
        }
    }
    // Handle delayed execution for all_answered
    if (room->all_answered) {
        int delay_elapsed = (int)difftime(current_time, room->all_answered_time);
        if (delay_elapsed >= 2) {
            room->all_answered = 0; // Reset the flag
            room->current_question_index++;
            printf("current index: %d\n",room->current_question_index);

            if (room->current_question_index >= NUM_QUESTIONS) {
                // Broadcast "Finish"
                struct json_object *broadcast_json = json_object_new_object();
                json_object_object_add(broadcast_json, "action", json_object_new_string("finish"));
                json_object_object_add(broadcast_json, "room_name", json_object_new_string(room->room_name));
                broadcast_json_object(broadcast_json, -1);
                json_object_put(broadcast_json);

                // Set the deletion time for the game room
                room->all_answered_time = current_time;
                room->all_answered = 2; // Set a new flag to indicate the room is ready for deletion
            } else {
                room->question_start_time = current_time; // Set the timestamp for the next question
                for (int i = 0; i < room->num_players; i++) {
                    room->client_progress[i].current_question = room->current_question_index;
                    room->client_progress[i].answered = 0;
                }
            }
        }
    } else if (remain_time < 0 && remain_time > -3) {
        printf("20 sec over\n");
        for (int i = 0; i < room->num_players; i++) {
            if (!room->client_progress[i].answered) {
                room->client_progress[i].score += 0; // Mark unanswered clients with score 0
                room->client_progress[i].answered = 1;
                room->client_progress[i].streak = 0;
            }
        }

        // Broadcast score, value1, value2, streak
        for (int i = 0; i < room->num_players; i++) {
            struct json_object *score_json = json_object_new_object();
            json_object_object_add(score_json, "action", json_object_new_string("score_update"));
            json_object_object_add(score_json, "room_name", json_object_new_string(room->room_name));
            json_object_object_add(score_json, "username", json_object_new_string(room->client_progress[i].username));
            json_object_object_add(score_json, "score", json_object_new_int(room->client_progress[i].score));
            json_object_object_add(score_json, "value1", json_object_new_int(room->questions[room->current_question_index].value1));
            json_object_object_add(score_json, "value2", json_object_new_int(room->questions[room->current_question_index].value2));
            json_object_object_add(score_json, "streak", json_object_new_int(room->client_progress[i].streak));
            broadcast_json_object(score_json, -1);
            json_object_put(score_json);
        }
    } else if (remain_time >= -1) {
        // Broadcast elapsed time and question index every second
        struct json_object *broadcast_json = json_object_new_object();
        json_object_object_add(broadcast_json, "action", json_object_new_string("update"));
        json_object_object_add(broadcast_json, "room_name", json_object_new_string(room->room_name));
        json_object_object_add(broadcast_json, "remain_time", json_object_new_int(remain_time));
        json_object_object_add(broadcast_json, "question_index", json_object_new_int(room->current_question_index));
        
        // Create a JSON array for client progress
        struct json_object *clients_array = json_object_new_array();
        for (int i = 0; i < room->num_players; i++) {
            struct json_object *client_json = json_object_new_object();
            json_object_object_add(client_json, "username", json_object_new_string(room->client_progress[i].username));
            json_object_object_add(client_json, "answered", json_object_new_int(room->client_progress[i].answered));
            json_object_array_add(clients_array, client_json);
        }
        json_object_object_add(broadcast_json, "clients", clients_array);
        
        broadcast_json_object(broadcast_json, -1);
        json_object_put(broadcast_json);
    } else if (remain_time <= -2) {
        printf("22 sec over\n");
        room->current_question_index++;
        printf("current index: %d\n",room->current_question_index);
        if (room->current_question_index >= NUM_QUESTIONS) {
            // Broadcast "Finish"
            struct json_object *broadcast_json = json_object_new_object();
            json_object_object_add(broadcast_json, "action", json_object_new_string("finish"));
            json_object_object_add(broadcast_json, "room_name", json_object_new_string(room->room_name));
            broadcast_json_object(broadcast_json, -1);
            json_object_put(broadcast_json);

            // Set the deletion time for the game room
            room->all_answered_time = current_time;
            room->all_answered = 2; // Set a new flag to indicate the room is ready for deletion
        } else {
            room->question_start_time = current_time; // Set the timestamp for the next question
            for (int i = 0; i < room->num_players; i++) {
                room->client_progress[i].current_question = room->current_question_index;
                room->client_progress[i].answered = 0;
            }
        }
    }
}

// Kiểm tra timeout cho phòng chơi: điều khiển thời gian trả lời, broadcast update, xử lý
// khi hết thời gian cho câu hỏi, chuyển câu hỏi tiếp theo hoặc kết thúc phòng.
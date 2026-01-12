#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <json-c/json.h>
#include "../core/sse.h"

#define TOPIC "database/country.txt"

GameRoom game_rooms[MAX_ROOMS];
int num_rooms = 0;

void load_data(const char *filename, char data[MAX_LINES][MAX_LINE_LENGTH], int *out_count) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        *out_count = 0;
        return;
    }

    int i = 0;
    while (i < MAX_LINES && fgets(data[i], MAX_LINE_LENGTH, file)) {
        data[i][strcspn(data[i], "\n")] = '\0'; // Remove newline character
        i++;
    }

    fclose(file);
    *out_count = i;
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
    int data_count = 0;
    load_data(file_path, data, &data_count);
    if (data_count == 0) {
        fprintf(stderr, "No data loaded from %s\n", file_path);
        return;
    }

    srand(time(NULL));
    int indices[data_count];
    for (int i = 0; i < data_count; i++) {
        indices[i] = i;
    }

    shuffle(indices, data_count);

    // Limit requested questions so we have enough data (4 options per question)
    int max_possible_questions = data_count / 4;
    if (room->num_questions > max_possible_questions) room->num_questions = max_possible_questions;
    if (room->num_questions <= 0) room->num_questions = 1;

    int needed = room->num_questions * 4;
    int selected_indices[needed];
    for (int i = 0; i < needed; i++) {
        selected_indices[i] = indices[i];
    }

    // Create room->num_questions questions from the selected entries (4 per question)
    for (int i = 0; i < room->num_questions; i++) {
        int base = i * 4;
        int idx1 = selected_indices[base + 0];
        int idx2 = selected_indices[base + 1];
        int idx3 = selected_indices[base + 2];
        int idx4 = selected_indices[base + 3];

        // Parse four lines into four options by splitting on '|'
        char *parts[5];
        char buf[MAX_LINE_LENGTH];
        // helper lambda-like block
        for (int opt = 0; opt < 4; opt++) {
            int idx = (opt == 0) ? idx1 : (opt == 1) ? idx2 : (opt == 2) ? idx3 : idx4;
            strncpy(buf, data[idx], sizeof(buf));
            buf[sizeof(buf)-1] = '\0';
            // Initialize parts
            for (int p = 0; p < 5; p++) parts[p] = NULL;
            // split
            char *tok = strtok(buf, "|");
            int p = 0;
            while (tok && p < 5) {
                parts[p++] = tok;
                tok = strtok(NULL, "|");
            }
            // fill fields safely
            int target = i; // to use inside switch
            if (parts[0]) {
                int parsed_id = atoi(parts[0]);
                if (opt == 0) room->questions[i].id = parsed_id;
                else room->questions[i].id = room->questions[i].id; // keep existing id
            }
            const char *name = parts[1] ? parts[1] : "";
            long long val = parts[2] ? atoll(parts[2]) : 0;
            const char *unit = parts[3] ? parts[3] : "";
            const char *pic = parts[4] ? parts[4] : "";
            if (opt == 0) {
                strncpy(room->questions[i].name1, name, sizeof(room->questions[i].name1));
                room->questions[i].value1 = val;
                strncpy(room->questions[i].unit, unit, sizeof(room->questions[i].unit));
                strncpy(room->questions[i].pic1, pic, sizeof(room->questions[i].pic1));
            } else if (opt == 1) {
                strncpy(room->questions[i].name2, name, sizeof(room->questions[i].name2));
                room->questions[i].value2 = val;
                strncpy(room->questions[i].pic2, pic, sizeof(room->questions[i].pic2));
            } else if (opt == 2) {
                strncpy(room->questions[i].name3, name, sizeof(room->questions[i].name3));
                room->questions[i].value3 = val;
                strncpy(room->questions[i].pic3, pic, sizeof(room->questions[i].pic3));
            } else if (opt == 3) {
                strncpy(room->questions[i].name4, name, sizeof(room->questions[i].name4));
                room->questions[i].value4 = val;
                strncpy(room->questions[i].pic4, pic, sizeof(room->questions[i].pic4));
            }
            // ensure null termination
            room->questions[i].name1[sizeof(room->questions[i].name1)-1] = '\0';
            room->questions[i].name2[sizeof(room->questions[i].name2)-1] = '\0';
            room->questions[i].name3[sizeof(room->questions[i].name3)-1] = '\0';
            room->questions[i].name4[sizeof(room->questions[i].name4)-1] = '\0';
            room->questions[i].unit[sizeof(room->questions[i].unit)-1] = '\0';
            room->questions[i].pic1[sizeof(room->questions[i].pic1)-1] = '\0';
            room->questions[i].pic2[sizeof(room->questions[i].pic2)-1] = '\0';
            room->questions[i].pic3[sizeof(room->questions[i].pic3)-1] = '\0';
            room->questions[i].pic4[sizeof(room->questions[i].pic4)-1] = '\0';
        }

        // Debug prints
        printf("Parsed options for question %d:\n", i);
        printf("  1: %d | %s | %lld | %s | %s\n",
            room->questions[i].id, room->questions[i].name1, room->questions[i].value1, room->questions[i].unit, room->questions[i].pic1);
        printf("  2: %d | %s | %lld | %s | %s\n",
            room->questions[i].id, room->questions[i].name2, room->questions[i].value2, room->questions[i].unit, room->questions[i].pic2);
        printf("  3: %d | %s | %lld | %s | %s\n",
            room->questions[i].id, room->questions[i].name3, room->questions[i].value3, room->questions[i].unit, room->questions[i].pic3);
        printf("  4: %d | %s | %lld | %s | %s\n",
            room->questions[i].id, room->questions[i].name4, room->questions[i].value4, room->questions[i].unit, room->questions[i].pic4);

        // Determine which option is the correct one (largest value)
        long long int maxv = room->questions[i].value1;
        int ans = 1;
        if (room->questions[i].value2 > maxv) { maxv = room->questions[i].value2; ans = 2; }
        if (room->questions[i].value3 > maxv) { maxv = room->questions[i].value3; ans = 3; }
        if (room->questions[i].value4 > maxv) { maxv = room->questions[i].value4; ans = 4; }
        room->questions[i].answer = ans;
        printf("answer: %d\n", room->questions[i].answer);
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

GameRoom* create_game_room(const char *room_name, const char *topic, int num_questions) {
    if (num_rooms < MAX_ROOMS) {
        GameRoom *new_room = &game_rooms[num_rooms++];
        strncpy(new_room->room_name, room_name, sizeof(new_room->room_name));
        // clamp num_questions
        if (num_questions <= 0) num_questions = NUM_QUESTIONS;
        if (num_questions > NUM_QUESTIONS) num_questions = NUM_QUESTIONS;
        new_room->num_questions = num_questions;
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

            if (room->current_question_index >= room->num_questions) {
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
            json_object_object_add(score_json, "value3", json_object_new_int(room->questions[room->current_question_index].value3));
            json_object_object_add(score_json, "value4", json_object_new_int(room->questions[room->current_question_index].value4));
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
    if (room->current_question_index >= room->num_questions) {
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
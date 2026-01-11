#ifndef GAME_H
#define GAME_H

#define MAX_LINES 50
#define MAX_LINE_LENGTH 1000
#define MAX_PLAYERS 3
#define MAX_ROOMS 10
#define MAX_POWERUPS 4
// Number of questions per game
#define NUM_QUESTIONS 20
#include <time.h>

typedef struct {
    int id;
    char name1[50];
    char name2[50];
    char name3[50];
    char name4[50];
    long long int value1;
    long long int value2;
    long long int value3;
    long long int value4;
    char unit[50];
    char pic1[200];
    char pic2[200];
    char pic3[200];
    char pic4[200];
    int answer;
} Question;

typedef struct {
    char username[50];
    int current_question;
    int answered;
    int score;
    int streak;
    int used_powerup[MAX_POWERUPS]; // Array to track used power-ups
} ClientProgress;

typedef struct {
    char room_name[50];
    Question questions[10];
    int current_question_index;
    ClientProgress client_progress[MAX_PLAYERS];
    int num_players;
    time_t question_start_time;
    int all_answered; // Flag to indicate if all players have answered
    time_t all_answered_time; // Timestamp when all players have answered
} GameRoom;

extern GameRoom game_rooms[MAX_ROOMS];
extern int num_rooms;

void create_questions(GameRoom *room, const char *topic);
GameRoom* create_game_room(const char *room_name, const char *topic);
GameRoom* find_room(const char *room_name);
void delete_game_room(const char *room_name);
void check_timeout(GameRoom *room);

#endif // GAME_H
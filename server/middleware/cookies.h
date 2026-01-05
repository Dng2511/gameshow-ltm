#include <stddef.h>
#ifndef COOKIE_H
#define COOKIE_H

#define COOKIE_LENGTH 32

typedef struct Session {
    char session_id[COOKIE_LENGTH];
    char username[64]; // Associated user data
    struct Session *next;
} Session;
// Common utility functions
int compare_string_sums(const char *str1, const char *str2);


void generate_session_id(char *buffer, size_t length);
void add_session(const char *username, char *session_id);
const char * validate_session(const char *session_id);
void delete_session(const char *session_id);

#endif // COOKIE_H
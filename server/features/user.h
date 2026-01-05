#ifndef USER_H
#define USER_H

typedef struct User {
    int id;
    char username[50];
    char password[50];
    int status;
    char homepage[50];
    struct User *next;
} User;

void _initUser();
void _cleanUser();


int authenticate_user(const char *username, const char *password);
int register_user(const char *username, const char *password);
int log_out();

void setCurUser(User *user);
User *find_user(const char *username);

#endif // USER_H
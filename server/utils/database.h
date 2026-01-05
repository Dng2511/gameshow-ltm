#ifndef DATABASE_H
#define DATABASE_H

#define USER_DATABASE "database/user.csv"

// For user database
#include "../features/user.h"
void loadUsers(User **head);
void addUser(User *user);
void saveUsers(User *head);

// For other database

#endif // DATABASE_H
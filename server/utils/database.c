#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "database.h"

void loadUsers(User **head) {
    const char *filename = USER_DATABASE;
    FILE *fileHandle = fopen(filename, "r");
    if (!fileHandle) {
        printf("Unable to open the file: %s\n", filename);
        return;
    }

    char tempLine[150];

    // Read and discard the header line
    if (fgets(tempLine, sizeof(tempLine), fileHandle) == NULL) {
        printf("Error reading the header line from the file: %s\n", filename);
        fclose(fileHandle);
        return;
    }

    while (fgets(tempLine, sizeof(tempLine), fileHandle)) {
        User *tempUser = malloc(sizeof(User));
        sscanf(tempLine, "%[^,],%s", tempUser->username, tempUser->password);
        tempUser->next = *head;
        *head = tempUser;
    }
    fclose(fileHandle);
}

void addUser(User *user) {
    const char *filename = USER_DATABASE;
    FILE *fileHandle = fopen(filename, "a");
    if (!fileHandle) {
        printf("Unable to open the file: %s\n", filename);
        return;
    }

    fprintf(fileHandle, "%s,%s\n", user->username, user->password);
    fclose(fileHandle);
}

void saveUsers(User *head) {
    const char *filename = USER_DATABASE;
    FILE *output = fopen(filename, "w");
    if (!output) {
        printf("Failed to open %s for writing.\n", filename);
        return;
    }

    fprintf(output, "username,password\n");
    for (User *currentUser = head; currentUser != NULL; currentUser = currentUser->next) {
        fprintf(output, "%s,%s\n", currentUser->username, currentUser->password);
    }
    fclose(output);
}
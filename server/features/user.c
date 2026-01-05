#include "user.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "../utils/utils.h"
#include "../utils/database.h"

User *head = NULL;

__thread User *curUser = NULL; // Thread-local storage for current user

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void _initUser() {
    pthread_mutex_lock(&mutex);
    loadUsers(&head);
    pthread_mutex_unlock(&mutex);
}

// Khởi tạo dữ liệu user khi server bắt đầu (loadUsers từ DB/file vào danh sách liên kết `head`).

void _cleanUser() {
    pthread_mutex_lock(&mutex);
    saveUsers(head);
    pthread_mutex_unlock(&mutex);
}

// Lưu trạng thái user (saveUsers) khi server dừng hoặc cần cleanup.


void setCurUser(User *user) {
    printf("Welcome %s", curUser->username);
    curUser = user;
}

// Gán biến thread-local curUser cho luồng hiện tại (đặt user đang hoạt động).

int authenticate_user(const char *username, const char *password) {
    pthread_mutex_lock(&mutex);
    User *current = head;
    while (current != NULL) {
        if (strcmp(current->username, username) == 0 && strcmp(current->password, password) == 0) {
            pthread_mutex_unlock(&mutex);
            curUser = current;
            return 1; // Authentication successful
        }
        current = current->next;
    }
    pthread_mutex_unlock(&mutex);
    return 0; // Authentication failed
}

// Kiểm tra username/password trong danh sách users; trả 1 nếu hợp lệ, 0 nếu thất bại.

User * find_user(const char * username) {
    pthread_mutex_lock(&mutex);
    User *current = head;
    while (current != NULL) {
        if (strcmp(current->username, username) == 0) {
            pthread_mutex_unlock(&mutex);
            return current;
        }
        current = current->next;
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
}

// Tìm user theo username và trả con trỏ tới User nếu tìm thấy, hoặc NULL.

int register_user(const char *username, const char *password) {
    pthread_mutex_lock(&mutex);
    User *current = head;
    while (current != NULL) {
        if (strcmp(current->username, username) == 0) {
            pthread_mutex_unlock(&mutex);
            return 0; // User already exists
        }
        current = current->next;
    }

    User *new_user = malloc(sizeof(User));
    if (new_user == NULL) {
        pthread_mutex_unlock(&mutex);
        return 0; // Memory allocation failed
    }
    strcpy(new_user->username, username);
    strcpy(new_user->password, password);
    new_user->status = 1; // Default status
    strcpy(new_user->homepage, ""); // Default homepage
    new_user->next = head;
    head = new_user;

    addUser(new_user);

    pthread_mutex_unlock(&mutex);
    return 1; // Registration successful
}

// Đăng ký user mới: thêm vào linked list và lưu vào database bằng addUser; trả 1 nếu thành công.

int log_out() {
    curUser = NULL;
    return 1;
}

// Đăng xuất đơn giản: đặt curUser = NULL.
#include "cookies.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <json-c/json.h>
#include <stddef.h>

// Session store
Session *session_store = NULL;
pthread_mutex_t session_mutex = PTHREAD_MUTEX_INITIALIZER;

void generate_session_id(char *buffer, size_t length) {
    const char *charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (size_t i = 0; i < length - 1; i++) {
        buffer[i] = charset[rand() % strlen(charset)];
    }
    buffer[length - 1] = '\0';
}

// Tạo một session_id ngẫu nhiên gồm ký tự chữ và số, lưu vào buffer.

void add_session(const char *username, char *session_id) {
    pthread_mutex_lock(&session_mutex);

    Session *new_session = malloc(sizeof(Session));
    if (new_session == NULL) {
        perror("Failed to allocate memory for new session");
        pthread_mutex_unlock(&session_mutex);
        return;
    }
    strncpy(new_session->session_id, session_id, COOKIE_LENGTH);
    strncpy(new_session->username, username, sizeof(new_session->username));
    new_session->next = session_store;
    session_store = new_session;

    // printf("Session added: %s -> %s\n", session_id, username);

    pthread_mutex_unlock(&session_mutex);
}

// Thêm một session mới vào session_store (theo dạng linked list) kèm mutex.

const char *validate_session(const char *session_id) {
    pthread_mutex_lock(&session_mutex);

    Session *current = session_store;
    while (current) {
        if (compare_string_sums(current->session_id, session_id) == 0) {
            pthread_mutex_unlock(&session_mutex);
            // printf("Session validated: %s -> %s\n", session_id, current->username);
            return current->username;
        }
        current = current->next;
    }

    pthread_mutex_unlock(&session_mutex);
    printf("Session not found: %s\n", session_id);
    return NULL;
}

// Kiểm tra session_id trong store; nếu tìm thấy trả username tương ứng, ngược lại trả NULL.

int sum_ascii(const char *str) {
    int sum = 0;
    while (*str) {
        if ((*str >= 'A' && *str <= 'Z') || (*str >= 'a' && *str <= 'z') || (*str >= '0' && *str <= '9')) {
            sum += (unsigned char)(*str);
        }
        str++;
    }
    return sum;
}

// Tính tổng mã ASCII của các ký tự chữ và số trong chuỗi (dùng trong so sánh đơn giản).

int compare_string_sums(const char *str1, const char *str2) {
    int sum1 = sum_ascii(str1);
    int sum2 = sum_ascii(str2);
    return sum1 - sum2;
}

// So sánh hai chuỗi bằng tổng ASCII các ký tự chữ/số (phương pháp đơn giản, không an toàn cho bảo mật).

void delete_session(const char *session_id) {
    pthread_mutex_lock(&session_mutex);

    Session **current = &session_store;
    while (*current) {
        if (strcmp((*current)->session_id, session_id) == 0) {
            Session *to_delete = *current;
            *current = (*current)->next;
            free(to_delete);
            printf("Session deleted: %s\n", session_id);
            break;
        }
        current = &(*current)->next;
    }

    pthread_mutex_unlock(&session_mutex);
}

// Xóa session khỏi store dựa trên session_id.
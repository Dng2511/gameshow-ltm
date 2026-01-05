#include "utils.h"
#include "../features/user.h"
#include "../features/room.h"
#include <json-c/json.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "../middleware/cookies.h"

#define BUFF_SIZE 4096

void sendResponse(int client_sock, const char *response) {
    char res[BUFF_SIZE];
    snprintf(res, sizeof(res),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Access-Control-Allow-Origin: http://localhost:5173\r\n"
        "Access-Control-Allow-Credentials: true\r\n"
        "Content-Length: %zu\r\n"
        "Connection: keep-alive\r\n\r\n%s",
        strlen(response), response);

    send(client_sock, res, strlen(res), 0);
}

// Gửi response HTTP 200 với body JSON (response là chuỗi JSON đã được tạo sẵn).

void sendError(int client_sock, const char *message, int error_code) {
    char response[BUFF_SIZE];
    snprintf(response, sizeof(response),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: text/plain\r\n"
        "Access-Control-Allow-Origin: http://localhost:5173\r\n"
        "Access-Control-Allow-Credentials: true\r\n"
        "Content-Length: %zu\r\n"
        "Connection: keep-alive\r\n\r\n%s", error_code, message,
        strlen(message), message);

    send(client_sock, response, strlen(response), 0);
}

// Gửi response HTTP lỗi với mã lỗi và message (plain text hoặc JSON string).

void send_cookie_response(int client_sock, const char *response, const char *username) {
    char session_id[COOKIE_LENGTH];
    generate_session_id(session_id, COOKIE_LENGTH);

    // Add the session to the store
    add_session(username, session_id);

    char res[BUFF_SIZE];
    snprintf(res, sizeof(res),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Access-Control-Allow-Origin: http://localhost:5173\r\n"
        "Access-Control-Allow-Credentials: true\r\n"
        "Set-Cookie: session_id=%s; HttpOnly; Path=/; SameSite=None; Secure\r\n"
        "Content-Length: %zu\r\n"
        "Connection: keep-alive\r\n\r\n%s", session_id,
        strlen(response), response);

    send(client_sock, res, strlen(res), 0);
}

// Tạo session mới (generate_session_id), lưu session vào store và trả response kèm Set-Cookie.

const char *extract_cookie(const char *request, const char *cookie_name) {
    // printf("%s\n", request);
    const char *cookie_header = strstr(request, "Cookie: ");
    // printf("Cookie header: %s\n", cookie_header);
    if (!cookie_header) return NULL;

    cookie_header += strlen("Cookie: ");
    const char *start = strstr(cookie_header, cookie_name);
    if (!start) return NULL;

    start += strlen(cookie_name) + 1; // Skip "cookie_name="
    const char *end = strchr(start, ';');
    size_t length = end ? (size_t)(end - start) : strlen(start);

    char *cookie_value = malloc(length + 1); // Allocate space for null terminator
    strncpy(cookie_value, start, length);
    cookie_value[length] = '\0'; // Null terminator
    // printf("Cookie value: %s\n", cookie_value);
    return cookie_value;
}

// Trích xuất giá trị cookie theo tên từ header request (trả một chuỗi malloc'd, cần free sau khi dùng).

int check_cookies(const char *request) {
    const char *session_id = extract_cookie(request, "session_id");
    // printf("Session: %s\n", session_id);
    
    if (!session_id) {
        return 0;
    }
    const char *username = validate_session(session_id);
    printf("User: %s\n\n", username);
    free((void *)session_id);

    if (username && find_user(username) != NULL) {
        return 1;
    }

    return 0;
}

// Kiểm tra cookie session trong request: trả 1 nếu session hợp lệ và user tồn tại.

int get_user_id_from_request(const char *request) {
    const char *header_start = strstr(request, "User-ID: ");
    if (header_start) {
        header_start += strlen("User-ID: ");
        const char *header_end = strstr(header_start, "\r\n");
        if (header_end) {
            int user_id_length = header_end - header_start;
            char user_id[user_id_length + 1];
            strncpy(user_id, header_start, user_id_length);
            user_id[user_id_length] = '\0';
            return atoi(user_id);
        }
    }
    return -1; // Return -1 if User-ID header is not found
}

// Lấy header User-ID từ request (nếu có) và trả giá trị số, hoặc -1 nếu không tìm thấy.
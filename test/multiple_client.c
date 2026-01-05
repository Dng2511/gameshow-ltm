#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <json-c/json.h>

#define SERVER_IP "127.0.0.1" // Change to your server's IP
#define SERVER_PORT 8080      // Change to your server's port
#define NUM_CLIENTS 4         // Number of clients to simulate
#define BUFF_SIZE 1024

// Function to simulate a client sending a request with headers and body
void *client_thread(void *arg) {
    int client_num = *((int *)arg);
    free(arg);

    // Create a socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        pthread_exit(NULL);
    }

    // Define the server address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid server address");
        close(sock);
        pthread_exit(NULL);
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        pthread_exit(NULL);
    }

    // Prepare HTTP request based on client number
    char request[BUFF_SIZE];
    if (client_num == 2) {
        char *post_body = "{\"username\":\"minh\",\"password\":\"123\"}";
        snprintf(request, sizeof(request),
                 "POST /auth/login HTTP/1.1\r\n"
                 "Host: %s:%d\r\n"
                 "Content-Type: application/json\r\n"
                 "Content-Length: %zu\r\n"
                 "Origin: http://localhost\r\n"
                 "Connection: keep-alive\r\n\r\n"
                 "%s",
                 SERVER_IP, SERVER_PORT, strlen(post_body), post_body);
    } else if (client_num == 1) {
        // GET request
        snprintf(request, sizeof(request),
                 "GET /auth/logout HTTP/1.1\r\n"
                 "Host: %s:%d\r\n"
                 "Accept: application/json\r\n"
                 "Origin: http://localhost\r\n"
                 "Connection: keep-alive\r\n\r\n",
                 SERVER_IP, SERVER_PORT);
    } else if (client_num == 3) {
        // POST request
        char *post_body = "{\"username\":\"hello\",\"password\":\"123\"}";
        snprintf(request, sizeof(request),
                 "POST /auth/login HTTP/1.1\r\n"
                 "Host: %s:%d\r\n"
                 "Content-Type: application/json\r\n"
                 "Content-Length: %zu\r\n"
                 "Origin: http://localhost\r\n"
                 "Connection: keep-alive\r\n\r\n"
                 "%s",
                 SERVER_IP, SERVER_PORT, strlen(post_body), post_body);
    } else {
        // Invalid request (to test error handling)
        snprintf(request, sizeof(request),
                 "GET /auth/logout HTTP/1.1\r\n"
                 "Host: %s:%d\r\n"
                 "Accept: application/json\r\n"
                 "Origin: http://localhost\r\n"
                 "Connection: keep-alive\r\n\r\n",
                 SERVER_IP, SERVER_PORT);
    }

    // Send the request
    if (send(sock, request, strlen(request), 0) < 0) {
        perror("Request send failed");
        close(sock);
        pthread_exit(NULL);
    }

    printf("Client %d: Sent request:\n%s\n", client_num, request);

    // Receive the response
    char buffer[BUFF_SIZE] = {0};
    ssize_t len = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (len > 0) {
        buffer[len] = '\0';
        printf("Client %d: Received response:\n%s\n", client_num, buffer);
    } else {
        perror("Response receive failed");
    }

    // Close the socket
    close(sock);
    pthread_exit(NULL);
}

int main() {
    pthread_t threads[NUM_CLIENTS];

    // Create multiple client threads
    for (int i = 0; i < NUM_CLIENTS; i++) {
        int *client_num = malloc(sizeof(int));
        *client_num = i + 1; // Assign client number
        if (pthread_create(&threads[i], NULL, client_thread, client_num) != 0) {
            perror("Thread creation failed");
            free(client_num);
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < NUM_CLIENTS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("All clients finished.\n");
    return 0;
}

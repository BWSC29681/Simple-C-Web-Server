#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <signal.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 1024
#define ROOT_FOLDER "www"

// HTTP Response Constants
const char* HTTP_200_OK = "HTTP/1.1 200 OK\r\n";
const char* HTTP_404_NOT_FOUND =
"HTTP/1.1 404 Not Found\r\n"
"Content-Type: text/html\r\n"
"Content-Length: 54\r\n"
"\r\n"
"<html><body><h1>404 Not Found</h1></body></html>";

SOCKET sockfd; // Global variable for the server socket

// Function to print error messages and exit
void error(const char* msg) {
    perror(msg);
    exit(1);
}

// Function to respond to HTTP requests
void respond(SOCKET newsockfd) {
    char buffer[BUFFER_SIZE];
    int n = recv(newsockfd, buffer, BUFFER_SIZE - 1, 0);
    if (n == SOCKET_ERROR) {
        error("ERROR reading from socket");
    }

    buffer[n] = '\0';
    printf("Here is the message: %s\n", buffer);

    // Extract the requested file path from the HTTP request
    char* next_token = NULL;
    char* file_path = strtok_s(buffer, " ", &next_token);
    file_path = strtok_s(NULL, " ", &next_token);

    if (file_path[0] == '/') {
        file_path++;
    }

    if (strlen(file_path) == 0) {
        file_path = "index.html";
    }

    char full_path[BUFFER_SIZE];
    snprintf(full_path, sizeof(full_path), "%s/%s", ROOT_FOLDER, file_path);
    int file_fd = _open(full_path, _O_RDONLY);

    if (file_fd == -1) {
        // If the file is not found, send a 404 response
        n = send(newsockfd, HTTP_404_NOT_FOUND, strlen(HTTP_404_NOT_FOUND), 0);
    }
    else {
        // If the file is found, send the file content
        struct _stat file_stat;
        _fstat(file_fd, &file_stat);

        char response_header[BUFFER_SIZE];
        snprintf(response_header, sizeof(response_header),
            "%s"
            "Content-Type: text/html\r\n"
            "Content-Length: %ld\r\n"
            "\r\n", HTTP_200_OK, file_stat.st_size);

        send(newsockfd, response_header, strlen(response_header), 0);

        while ((n = _read(file_fd, buffer, BUFFER_SIZE)) > 0) {
            send(newsockfd, buffer, n, 0);
        }
        _close(file_fd);
    }
}

// Signal handler to gracefully shut down the server
void signal_handler(int signum) {
    printf("Interrupt signal received. Shutting down...\n");
    closesocket(sockfd);
    WSACleanup();
    exit(signum);
}

int main() {
    WSADATA wsa;
    SOCKET newsockfd;
    struct sockaddr_in serv_addr, cli_addr;
    int clilen;

    // Initialize Winsock
    printf("Initialising Winsock...\n");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed. Error Code : %d", WSAGetLastError());
        return 1;
    }

    printf("Initialised.\n");

    // Create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == INVALID_SOCKET) {
        printf("Could not create socket : %d", WSAGetLastError());
        return 1;
    }

    printf("Socket created.\n");

    // Prepare the sockaddr_in structure
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    // Bind the socket
    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        printf("Bind failed with error code : %d", WSAGetLastError());
        return 1;
    }

    printf("Bind done.\n");

    // Listen for incoming connections
    listen(sockfd, 3);

    printf("Waiting for incoming connections...\n");

    // Set up signal handling for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    clilen = sizeof(struct sockaddr_in);

    // Accept and handle incoming connections
    while ((newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen)) != INVALID_SOCKET) {
        printf("Connection accepted.\n");

        // Respond to the client
        respond(newsockfd);
        closesocket(newsockfd);
    }

    if (newsockfd == INVALID_SOCKET) {
        printf("Accept failed with error code : %d", WSAGetLastError());
        return 1;
    }

    // Clean up (although not reached in normal operation, since the loop is infinite)
    closesocket(sockfd);
    WSACleanup();

    return 0;
}

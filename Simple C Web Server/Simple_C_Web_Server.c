#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <signal.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

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

SOCKET sockfd;

// Print console error messages and exit
void error(const char* msg) {
    perror(msg);
    exit(1);
}

// Respond to HTTP requests
void respond(SOCKET newsockfd) {
    char buffer[BUFFER_SIZE];
    int n = recv(newsockfd, buffer, BUFFER_SIZE - 1, 0);
    if (n == SOCKET_ERROR) {
        error("ERROR reading from socket");
    }

    buffer[n] = '\0';
    printf("----------------------------------\nIncoming request: %s\n", buffer);

    // Extract requested file path from the HTTP request
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
        // 404 Not Found
        n = send(newsockfd, HTTP_404_NOT_FOUND, strlen(HTTP_404_NOT_FOUND), 0);
    }
    else {
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

// Shut down server gracefully
void signal_handler(int signum) {
    printf("Shutting down...\n");
    closesocket(sockfd);
    WSACleanup();
    exit(signum);
}

void InitializeWinsock() {
    WSADATA wsa;

    printf("Initializing Winsock...");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed. Error Code : %d", WSAGetLastError());
        return 1;
    }

    printf("done.\n");
}

void CreateSocket() {
    printf("Creating Socket...");
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == INVALID_SOCKET) {
        printf("Could not create socket : %d", WSAGetLastError());
        return 1;
    }

    printf("done.\n");
}

void BindSocket(struct sockaddr_in serv_addr) {
    printf("Binding Socket...");
    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        printf("Bind failed with error code : %d", WSAGetLastError());
        return 1;
    }

    printf("done.\n");
}

int main() {    
    SOCKET newsockfd;
    struct sockaddr_in serv_addr, cli_addr;
    int clilen;

    InitializeWinsock();
    CreateSocket();    

    // Prepare the sockaddr_in structure
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    BindSocket(serv_addr);    

    // Listen for incoming connections
    listen(sockfd, 3);

    printf("Waiting for incoming connections...\n\n");

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

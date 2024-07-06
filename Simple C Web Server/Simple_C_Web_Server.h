#pragma once

// HTTP Response Constants

const char* HTTP_200_OK = "HTTP/1.1 200 OK\r\n";

const char* HTTP_404_NOT_FOUND =
"HTTP/1.1 404 Not Found\r\n"
"Content-Type: text/html\r\n"
"Content-Length: 54\r\n"
"\r\n"
"<html><body><h1>404 Not Found</h1></body></html>";
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <arpa/inet.h>  // For inet_ntoa()
#include "ThreadPool.h"

// Parse the HTTP request
void parseRequest(char* buffer) {
    char* method = strtok(buffer, " ");
    char* path = strtok(NULL, " ");
    char* version = strtok(NULL, "\r\n");

    if (!method || !path || !version) {
        std::cerr << "[DEBUG] Malformed request." << std::endl;
        return;
    }

    std::cout << "[DEBUG] Method: " << method << "\n Path: " << path << ",\n Version: " << version << std::endl;
}

int main() {
    int port = 8080;

    std::cout << "[DEBUG] Starting server on port " << port << "..." << std::endl;

    // Create a socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "[DEBUG] Error creating socket." << std::endl;
        return -1;
    }
    std::cout << "[DEBUG] Socket created successfully." << std::endl;

    // Bind the socket
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("[DEBUG] Bind Error");
        close(serverSocket);
        return -1;
    }
    std::cout << "[DEBUG] Socket bound to port " << port << "." << std::endl;

    // Start listening
    if (listen(serverSocket, 10) < 0) {
        perror("[DEBUG] Listen Failed");
        close(serverSocket);
        return -1;
    }
    std::cout << "[DEBUG] Server is listening for incoming connections..." << std::endl;

    // Initialize and start the thread pool
    ThreadPool pool(6);
    pool.start(6);  // Start the thread pool
    std::cout << "[DEBUG] Thread pool started with 4 threads." << std::endl;

    while (true) {
        struct sockaddr_in clientAddress;
        socklen_t clientAddressLen = sizeof(clientAddress);

        std::cout << "[DEBUG] Waiting for a client connection..." << std::endl;
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLen);
        if (clientSocket < 0) {
            perror("[DEBUG] Error in Accepting incoming request");
            continue;
        }

        std::cout << "[DEBUG] Accepted connection from " << inet_ntoa(clientAddress.sin_addr) << "." << std::endl;

        // Submit the task to the thread pool
        pool.addTask([clientSocket, clientAddress]() {
            std::cout << "[DEBUG] Thread started for client " << inet_ntoa(clientAddress.sin_addr) << "." << std::endl;

            char buffer[4096];
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (bytesReceived > 0) {
                buffer[bytesReceived] = '\0';
                std::cout << "[DEBUG] Bytes received: " << bytesReceived << std::endl;

                std::cout << "[DEBUG] Request received from " << inet_ntoa(clientAddress.sin_addr) << "." << std::endl;
                parseRequest(buffer);

                std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!";
                send(clientSocket, response.c_str(), response.size(), 0);
            } else if (bytesReceived == 0) {
                std::cout << "[DEBUG] Client disconnected." << std::endl;
            } else {
                std::cerr << "[DEBUG] Error receiving data." << std::endl;
            }

            close(clientSocket);
            std::cout << "[DEBUG] Connection closed for client " << inet_ntoa(clientAddress.sin_addr) << "." << std::endl;
        });
    }

    pool.shutdown();  // Gracefully shut down the thread pool
    close(serverSocket);
    std::cout << "[DEBUG] Server shut down." << std::endl;
    return 0;
}

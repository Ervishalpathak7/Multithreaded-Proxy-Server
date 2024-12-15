#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <netdb.h>  // For gethostbyname()
#include <arpa/inet.h>  // For inet_ntoa()
#include "ThreadPool.h"
#include "Server.h"
#include <exception>  // Added this header
#include <string>


// Function to get the number of CPU cores
int getNumberOfCores() {
    long cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (cores == -1) {
        std::cerr << "[DEBUG] Error determining number of cores. Defaulting to 1." << std::endl;
        return 1;
    }
    return static_cast<int>(cores);
}

// Function to create a socket
int createServerSocket(int port) {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "[DEBUG] Error creating socket." << std::endl;
        return -1;
    }

    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("[DEBUG] setsockopt failed");
        close(serverSocket);
        return -1;
    }

    std::cout << "[DEBUG] Socket created successfully." << std::endl;
    return serverSocket;
}

// Function to bind the server socket
bool bindSocket(int serverSocket, int port) {
    struct sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("[DEBUG] Bind Error");
        close(serverSocket);
        return false;
    }
    std::cout << "[DEBUG] Socket bound to port " << port << "." << std::endl;
    return true;
}

// Function to parse the HTTP request
RequestInfo parseRequest(char* buffer) {
    RequestInfo reqInfo;
    char* method = strtok(buffer, " ");
    char* path = strtok(NULL, " ");
    char* version = strtok(NULL, "\r\n");

    if (!method || !path || !version) {
        std::cerr << "[DEBUG] Malformed request." << std::endl;
        return reqInfo; // Return empty struct in case of failure
    }

    reqInfo.method = method;
    reqInfo.path = path;
    reqInfo.version = version;

    std::cout << "[DEBUG] Method: " << reqInfo.method << "\n Path: " << reqInfo.path << "\n Version: " << reqInfo.version << std::endl;
    return reqInfo;
}

// Function to request the route from the backend
std::string routeRequestToBackend(const std::string& method, const std::string& path) {
    const std::string backendHost = "jsonplaceholder.typicode.com";  // Backend domain
    const int backendPort = 80;  // HTTP port
    std::string backendResponse;

    // Resolve the backend host to an IP address
    struct hostent* backendHostEntry = gethostbyname(backendHost.c_str());
    if (!backendHostEntry) {
        perror("[DEBUG] DNS resolution failed");
        return "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 21\r\n\r\nBackend Resolution Failed";
    }

    // Create a socket for communication
    int backendSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (backendSocket < 0) {
        std::cerr << "[DEBUG] Failed to create socket for backend." << std::endl;
        return "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 21\r\n\r\nBackend Connection Failed";
    }

    // Set up the backend address struct
    struct sockaddr_in backendAddress{};
    backendAddress.sin_family = AF_INET;
    backendAddress.sin_port = htons(backendPort);
    std::memcpy(&backendAddress.sin_addr.s_addr, backendHostEntry->h_addr, backendHostEntry->h_length);

    // Connect to the backend
    if (connect(backendSocket, (struct sockaddr*)&backendAddress, sizeof(backendAddress)) < 0) {
        perror("[DEBUG] Backend connection failed");
        close(backendSocket);
        return "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 21\r\n\r\nBackend Connection Failed";
    }

    // Build the HTTP request for the backend
    std::string request = method + " " + path + " HTTP/1.1\r\n";
    request += "Host: " + backendHost + "\r\n";
    request += "Connection: close\r\n\r\n";
    if (send(backendSocket, request.c_str(), request.size(), 0) < 0) {
        perror("[DEBUG] Error sending request to backend");
        close(backendSocket);
        return "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 21\r\n\r\nSend Failed";
    }

    // Receive the response from the backend
    char buffer[4096] = {0};
    int bytesReceived;
    while ((bytesReceived = recv(backendSocket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytesReceived] = '\0';
        backendResponse += buffer;
    }

    if (bytesReceived < 0) {
        perror("[DEBUG] Error receiving backend response");
    }

    if (backendResponse.empty()) {
        backendResponse = "HTTP/1.1 502 Bad Gateway\r\nContent-Length: 15\r\n\r\nInvalid Response";
        std::cerr << "[DEBUG] No response from backend." << std::endl;
    }

    close(backendSocket);
    return backendResponse;
}

// Function to handle client requests
void handleClient(int clientSocket, struct sockaddr_in clientAddress) {
    char buffer[4096];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        std::cout << "[DEBUG] Request received from " << inet_ntoa(clientAddress.sin_addr) << "." << std::endl;

        // Parse the HTTP request
        RequestInfo reqInfo = parseRequest(buffer);

        // Route the request to the backend and get the response
        std::string backendResponse = routeRequestToBackend(reqInfo.method, reqInfo.path);

        //  std::string backendResponse = "HTTP/1.1 200 OK\r\nContent-Length: 11\r\n\r\nHello World";

        // Send the backend response back to the client
        if (send(clientSocket, backendResponse.c_str(), backendResponse.size(), 0) < 0) {
            perror("[DEBUG] Error sending response to client");
        }
    } else if (bytesReceived == 0) {
        std::cout << "[DEBUG] Client disconnected." << std::endl;
    } else {
        perror("[DEBUG] Error receiving data");
    }

    close(clientSocket);
    std::cout << "[DEBUG] Connection closed for client " << inet_ntoa(clientAddress.sin_addr) << "." << std::endl;
}

// Function to initialize the server
void startServer(int port) {
    int serverSocket = createServerSocket(port);
    if (serverSocket == -1) return;

    if (!bindSocket(serverSocket, port)) return;

    if (listen(serverSocket, 10) < 0) {
        perror("[DEBUG] Listen Failed");
        close(serverSocket);
        return;
    }
    std::cout << "[DEBUG] Server is listening for incoming connections..." << std::endl;

    // Initialize and start the thread pool
    int cores = getNumberOfCores();
    ThreadPool pool(cores);
    std::cout << "[DEBUG] Thread pool started with " << cores << " threads." << std::endl;

    while (true) {
        std::cout << "[DEBUG] Waiting for a client connection..." << std::endl;

        struct sockaddr_in clientAddress;
        socklen_t clientAddressLen = sizeof(clientAddress);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLen);
        if (clientSocket < 0) {
            perror("[DEBUG] Error in accepting incoming request");
            continue;
        }

        std::cout << "[DEBUG] Accepted connection from " << inet_ntoa(clientAddress.sin_addr) << "." << std::endl;

        // Submit the task to the thread pool
        pool.addTask([clientSocket, clientAddress]() {
            handleClient(clientSocket, clientAddress);
        });
    }

    pool.shutdown();  // Gracefully shut down the thread pool
    close(serverSocket);
    std::cout << "[DEBUG] Server shut down." << std::endl;
}

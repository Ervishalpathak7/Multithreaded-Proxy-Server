#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>
#include <string>


// structure of the request 
struct RequestInfo {
    std::string method;
    std::string path;
    std::string version;
};

// Function to get the number of CPU cores
int getNumberOfCores();

// Function to create a socket
int createServerSocket(int port);

// Function to bind the server socket
bool bindSocket(int serverSocket, int port);

// Function to parse HTTP request
RequestInfo parseRequest(char* buffer);

// Function to handle client requests
std::string routeRequestToBackend(const std::string& method, const std::string& path);


// function to handle client req
void handleClient(int clientSocket, struct sockaddr_in clientAddress , auto start );

// function to start the server
void startServer(int port);

// function to request the backend

#endif // SERVER_H

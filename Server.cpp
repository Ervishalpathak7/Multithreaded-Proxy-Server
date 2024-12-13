#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <arpa/inet.h>  // For inet_ntoa()



void parseRequest(char* buffer) {
    // Split the request line (first line of the HTTP request)
    char* method = strtok(buffer, " ");  // e.g., "GET"
    char* path = strtok(NULL, " ");      // e.g., "/"
    char* version = strtok(NULL, "\r\n");  // e.g., "HTTP/1.1"

    if (!method || !path || !version) {
    std::cerr << "Malformed request" << std::endl;
    return;
    }

    // Print the parsed values
    std::cout << "Method: " << method << std::endl;
    std::cout << "Path: " << path << std::endl;
    std::cout << "Version: " << version << std::endl;
}



int main() {

    // defining the port number
    int port = 8080;

    // Create a socket
    int serverSocket = socket(AF_INET, SOCK_STREAM , 0);
    if (serverSocket == -1) {
        std::cerr << "Error creating socket" << std::endl;
        return -1;
    }

    // Bind the socket to an IP address and port
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    
    int bindStatus = bind(serverSocket , (struct sockaddr*)&serverAddress , sizeof(serverAddress));
    if (bindStatus < 0) {
        perror("Bind Error");
        close(serverSocket);
        return -1;
    }

    // listen for incoming requests
    if (listen(serverSocket, 10) < 0) {
        perror("Listen Failed");
        close(serverSocket);
        return -1;
    }

    std::cout << "Server is running on port " << port << '\n';

    while (1) {
        struct sockaddr_in clientAddress;
        socklen_t clientAddressLen = sizeof(clientAddress);
        
        // Accepting the request
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLen);
        if (clientSocket < 0) {
            perror("Error in Accepting incoming request");
            continue;  // Continue accepting other connections even if one fails
        }

        // Receive the request
        char buffer[4096];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);  // Ensure space for null-terminator

        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';  // Null-terminate the received data

            // Print the client IP address
            std::cout << "Request received from " << inet_ntoa(clientAddress.sin_addr) << std::endl;

            // parse the request 
            parseRequest(buffer);

            std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!";
            send(clientSocket, response.c_str(), response.size(), 0);

        } else if (bytesReceived == 0) {
            std::cout << "Client disconnected" << std::endl;
        } else {
            std::cerr << "Error receiving data" << std::endl;
        }

        close(clientSocket); // Close client socket after response is sent
    }

    close(serverSocket); // Close the server socket after exiting the loop
    return 0;
}

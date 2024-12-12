#include <iostream>
#include <thread>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>


// Handle a client connection
void handleClient(int clientSocket) {

    // Receive the request
    char buffer[4096];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

    if (bytesReceived > 0) {
        std::cout << "Received request:\n" << std::string(buffer, bytesReceived) << std::endl;

         // Simulate a response
        std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!";

        send(clientSocket, response.c_str(), response.size(), 0);
    }

    close(clientSocket); // Close the client socket
}




int main() {

    // Create a socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Failed to create socket!" << std::endl;
        return -1;
    }


    // Bind the socket to an IP / port
    sockaddr_in serverAddress = {};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Failed to bind socket!" << std::endl;
        close(serverSocket);
        return -1;
    }

    // Tell the socket is for listening
    if (listen(serverSocket, SOMAXCONN) == -1) {
        std::cerr << "Failed to listen on socket!" << std::endl;
        close(serverSocket);
        return -1;
    }


    std::cout << "Server is running on port 54000..." << std::endl;

    // Wait for a connection
    while (true) {

        // Accept the client connection
        sockaddr_in clientAddress = {};
        socklen_t clientSize = sizeof(clientAddress);


        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddress, &clientSize);
        if (clientSocket == -1) {
            std::cerr << "Failed to accept client connection!" << std::endl;
            continue;
        }


        // Spawn a thread to handle the client
        std::thread clientThread(handleClient, clientSocket);
        clientThread.detach(); // Detach the thread to allow independent execution
    }

    close(serverSocket);
    return 0;
}

#include "Server.h"
#include<iostream>

int main() {
    int port = 8080;
    std::cout << "[DEBUG] Starting server on port " << port << "..." << std::endl;
    startServer(port);
    return 0;
}

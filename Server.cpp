#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <netdb.h>  // For gethostbyname()
#include <arpa/inet.h>  // For inet_ntoa()
#include "ThreadPool.h"
#include <exception>  // Added this header
#include <string>
#include <sstream>
#include<chrono>
#include "Server.h"
#include "Logger.h"
#include "TokenBucket.h"   // Add TokenBucket class
#include <unordered_map>
#include <mutex>
#include "RequestException.h"
#include "Lrucache.h"


// Rate limiter to prevent request flooding
AdvancedRateLimiter globalRateLimiter;


LRUCache<std::string, std::string> cache(100);


std::mutex rateLimiterMutex;     // Mutex for thread-safe rate limit checks


std::string generateErrorResponse(int statusCode, const std::string& message) {
    std::stringstream response;
    response << "HTTP/1.1 " << statusCode << " " 
             << (statusCode == 429 ? "Too Many Requests" : 
                 statusCode == 500 ? "Internal Server Error" : "Bad Request") 
             << "\r\n"
             << "Content-Type: text/plain\r\n"
             << "Content-Length: " << message.length() << "\r\n"
             << "\r\n"
             << message;
    return response.str();
}

// Improve IP conversion to handle potential conversion errors
std::string getClientIP(struct sockaddr_in clientAddress) {
    char ipStr[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &(clientAddress.sin_addr), ipStr, INET_ADDRSTRLEN) == nullptr) {
        logError("IP Conversion Failed", strerror(errno));
        return "unknown";
    }
    return std::string(ipStr);
}


// Function to get the number of CPU cores
int getNumberOfCores() {
    long cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (cores == -1) {
        logError("Error determining number of cores", "Defaulting to 4 cores");
        return 4;
    }
    return static_cast<int>(cores);
}


// Function to create a socket
int createServerSocket(int port) {
    // Create a TCP socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        logError("Socket creation failed", "Unable to create server socket");
        return -1;
    }

    // Enable socket reuse to prevent "Address already in use" errors
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        logError("Socket option setting failed", strerror(errno));
        close(serverSocket);
        return -1;
    }

    return serverSocket;
}


// Function to bind the server socket
bool bindSocket(int serverSocket, int port) {
    struct sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        logError("Socket binding failed", 
                 "Port: " + std::to_string(port) + 
                 " Error: " + strerror(errno));
        close(serverSocket);
        return false;
    }
    
    std::cout << "[INFO] Socket successfully bound to port " << port << std::endl;
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

    // std::cout << "[DEBUG] Method: " << reqInfo.method << "\n Path: " << reqInfo.path << "\n Version: " << reqInfo.version << std::endl;
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
void handleClient(int clientSocket, struct sockaddr_in clientAddress, auto waitingTimeStart) {

    // Buffer to store incoming request data
    char buffer[4096];

    // waiting time finished as the request is started processing
    auto waitingTimeFinished = std::chrono::high_resolution_clock::now();
    auto waitingTime = std::chrono::duration_cast<std::chrono::milliseconds>(waitingTimeFinished - waitingTimeStart ).count();

    // processing time start 
    auto processingTimeStart = std::chrono::high_resolution_clock::now();

    // get the client IP address
    std::string clientIP = getClientIP(clientAddress);

    try {
        // Rate Limiting: Prevent excessive requests from a single IP
        if (!globalRateLimiter.allowRequest(clientIP)) {
            // Send 429 Too Many Requests response
            std::string ratelimitResponse = generateErrorResponse(
                429, 
                "Too many requests. Please slow down and try again later."
            );
            
            // Attempt to send rate limit response, ignore send errors
            send(clientSocket, ratelimitResponse.c_str(), ratelimitResponse.length(), 0);
            
            // end processing time 
            auto processingTimeEnd = std::chrono::high_resolution_clock::now();
            long processingTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                processingTimeEnd - processingTimeStart
            ).count();

            // Log rate limit event with detailed information
            logRequest(
                clientIP, 
                "RATE_LIMITED", 
                "N/A", 
                429, 
                waitingTime, processingTime , processingTime + waitingTime, 
                "IP rate limit exceeded"
            );
            // Close socket and exit function
            close(clientSocket);
            return;
        }

        
        // Receive HTTP request with robust error handling
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        
        // Handle connection errors or client disconnection
        if (bytesReceived <= 0) {
            if (bytesReceived == 0) {
                auto processingTimeEnd = std::chrono::high_resolution_clock::now();
                long processingTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                processingTimeEnd - processingTimeStart
            ).count();
                logRequest(clientIP, "DISCONNECT", "N/A", 499, waitingTime, processingTime, waitingTime + processingTime , "Client Closed Connection");
            } else {
                auto processingTimeEnd = std::chrono::high_resolution_clock::now();
                long processingTime = std::chrono::duration_cast<std::chrono::milliseconds>(processingTimeEnd - processingTimeStart).count();
                logRequest(clientIP, "ERROR", "N/A", 500,waitingTime, processingTime, waitingTime + processingTime, "Socket Receive Error");
                perror("Error receiving client data");
            }
            close(clientSocket);
            return;
        }

        // Null-terminate received data for safe string processing
        buffer[bytesReceived] = '\0';

        // Parse the HTTP request with comprehensive validation
        RequestInfo reqInfo = parseRequest(buffer);
        
        // Validate parsed request
        if (reqInfo.method.empty() || reqInfo.path.empty() || reqInfo.version.empty()) {
            auto processingTimeEnd = std::chrono::high_resolution_clock::now();
            long processingTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                processingTimeEnd - processingTimeStart
            ).count();
            throw RequestException("Invalid Request Format", 400 , waitingTime , processingTime );
        }

        // Check if request is in cache to avoid unnecessary backend calls
        std::string cachedResponse;
        if (cache.get(reqInfo.path, cachedResponse)) {

            // Cache hit: Send cached response
            if (send(clientSocket, cachedResponse.c_str(), cachedResponse.size(), 0) < 0) {
                throw std::runtime_error("Failed to send cached response");
            }

            // Log cache hit with performance metrics
            auto processingTimeEnd = std::chrono::high_resolution_clock::now();
            long processingTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                processingTimeEnd - processingTimeStart
            ).count();
            
            logRequest(
                inet_ntoa(clientAddress.sin_addr), 
                reqInfo.method, 
                reqInfo.path, 
                200, 
                waitingTime, 
                processingTime, 
                processingTime + waitingTime, 
                "Served from Cache"
            );

            close(clientSocket);
            return;
        }

        // Route request to backend if not in cache
        std::string backendResponse = routeRequestToBackend(reqInfo.method, reqInfo.path);

        if (backendResponse.empty()) {
            // Backend returned empty response
            auto processingTimeEnd = std::chrono::high_resolution_clock::now();
            long processingTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                processingTimeEnd - processingTimeStart
            ).count();
            throw RequestException("Backend Error", 500 , waitingTime , processingTime);
        }

        // Cache the backend response for future requests
        cache.put(reqInfo.path, backendResponse);

        // Send backend response to client
        if (send(clientSocket, backendResponse.c_str(), backendResponse.size(), 0) < 0) {
            throw std::runtime_error("Failed to send backend response");
        }

        // Log successful backend request
        auto processingTimeEnd = std::chrono::high_resolution_clock::now();
        long processingTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            processingTimeEnd - processingTimeStart
        ).count();
        
        logRequest(
            inet_ntoa(clientAddress.sin_addr), 
            reqInfo.method, 
            reqInfo.path, 
            200, 
            waitingTime, 
            processingTime, 
            processingTime + waitingTime, 
            "Served from Backend"
        );
    }
   catch (const RequestException& e) {
    // Handle specific request-related exceptions
    std::string errorResponse = generateErrorResponse(e.getStatusCode(), e.what());
    send(clientSocket, errorResponse.c_str(), errorResponse.size(), 0);

    // Log the error with corrected function calls
    logRequest(
        inet_ntoa(clientAddress.sin_addr), 
        "CLIENT_ERROR",       // Use a more descriptive method name
        "N/A", 
        e.getStatusCode(), 
        e.getWaitingTime(),   
        e.getProcessingTime(),
        e.getTotalTime(),   
        e.what()
    );
}
    catch (const std::exception& e) {
        // Catch any unexpected exceptions
        std::string errorResponse = generateErrorResponse(500, "Internal Server Error");
        send(clientSocket, errorResponse.c_str(), errorResponse.size(), 0);
        
        // Log unexpected errors
        logRequest(
            inet_ntoa(clientAddress.sin_addr), 
            "FATAL", 
            "N/A", 
            500, 
            0,
            0,
            0,
            e.what()
        );
    }

    // Ensure socket is always closed, even if an exception occurs
    close(clientSocket);
}

// Function to initialize the server
void startServer(int port) {
    // Validate port range
    if (port < 1024 || port > 65535) {
        logError("Invalid port", "Port must be between 1024 and 65535");
        throw std::invalid_argument("Invalid port number");
    }

    // Create server socket
    int serverSocket = createServerSocket(port);
    if (serverSocket == -1) {
        logError("Server initialization failed", "Could not create socket");
        return;
    }

    // Bind socket
    if (!bindSocket(serverSocket, port)) {
        close(serverSocket);
        return;
    }

    // Start listening with improved backlog
    if (listen(serverSocket, SOMAXCONN) < 0) {
        logError("Listen failed", strerror(errno));
        close(serverSocket);
        return;
    }

    // Determine thread pool size based on available cores
    int cores = getNumberOfCores();
    ThreadPool pool(cores);
    setupLogger();

    std::cout << "[INFO] Server started successfully on port " << port 
              << " with " << cores << " worker threads" << std::endl;

    // Main server loop with signal handling considerations
    while (true) {
        struct sockaddr_in clientAddress;
        socklen_t clientAddressLen = sizeof(clientAddress);
        
        // Accept incoming connection with error handling
        int clientSocket = accept(serverSocket, 
                                  (struct sockaddr*)&clientAddress, 
                                  &clientAddressLen);
        
        auto waitingTimeStart = std::chrono::high_resolution_clock::now();
        
        if (clientSocket < 0) {
            // Log specific accept errors
            if (errno == EMFILE || errno == ENFILE) {
                logError("Accept failed", "Too many open file descriptors");
                // Implement some form of back-off or connection shedding
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            } else {
                logError("Accept failed", strerror(errno));
            }
            continue;
        }

        // Add client handling task to thread pool
        pool.addTask([clientSocket, clientAddress, waitingTimeStart]() {
            handleClient(clientSocket, clientAddress, waitingTimeStart);
        });
    }

    // Graceful shutdown (though this will rarely be reached in practice)
    pool.shutdown();
    close(serverSocket);
    std::cout << "[INFO] Server shutdown complete." << std::endl;
}
#ifndef REQUESTEXCEPTION_H
#define REQUESTEXCEPTION_H

#include <exception>
#include <string>

// Custom Exception Class for Request Errors
class RequestException : public std::exception {
private:
    std::string message;   // Human-readable error message
    int statusCode;        // HTTP status code representing the error (e.g., 400, 404, 500)
    long waitingTime;      // Time spent waiting before processing (in milliseconds)
    long processingTime;   // Time spent processing the request (in milliseconds)
    long totalTime;        // Total time = waitingTime + processingTime (in milliseconds)

public:
    // Constructor: Initialize all members, calculate total time
    RequestException(const std::string& msg, int code, long waitingTime, long processingTime) 
        : message(msg), 
          statusCode(code), 
          waitingTime(waitingTime), 
          processingTime(processingTime), 
          totalTime(processingTime + waitingTime) {}

    // Override the what() method to return the error message
    const char* what() const noexcept override {
        return message.c_str();
    }

    // Getter for the HTTP status code
    int getStatusCode() const noexcept {
        return statusCode;
    }

    // Getter for waiting time
    long getWaitingTime() const noexcept {
        return waitingTime;
    }

    // Getter for processing time
    long getProcessingTime() const noexcept {
        return processingTime;
    }

    // Getter for total time
    long getTotalTime() const noexcept {
        return totalTime;
    }
};

#endif // REQUESTEXCEPTION_H

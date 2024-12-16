#include "Logger.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_sinks.h"  // Include stdout sink for console logging
#include "spdlog/spdlog.h"
#include <iostream>
#include <filesystem>  // For checking if the directory exists
#include <memory>


void setupLogger() {
    try {

        std::cout << "Setting up logger..." << std::endl;

        auto console_sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();

        auto logger = std::make_shared<spdlog::logger>("",console_sink);

        spdlog::set_default_logger(logger);
    }
    catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Log initialization failed: " << ex.what() << '\n';
    }
}

// Log request details to the file (basic logging without color)
void logRequest(const std::string& clientIP, const std::string& method, 
                const std::string& path, int statusCode, 
                long waitingTime, long processingTime, long totalTime , std::string backendResponse) {
    spdlog::info("IP: {} | Method: {} | Path: {} | Status: {} | Waiting Time: {} ms | Processing Time: {} ms | Total Time: {} ms | Backend Response: {}", 
                 clientIP, method, path, statusCode, waitingTime, processingTime, totalTime, backendResponse);
}

// Log errors to the file (basic logging without color)
void logError(const std::string& errorMessage, const std::string& context) {
    spdlog::error("Context: {} | Error: {}", context, errorMessage);
}

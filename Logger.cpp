#include "Logger.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/spdlog.h"
#include <iostream>
#include <filesystem>
#include <memory>

void setupLogger() {
    try {
        // Create console sink for standard output logging
        auto console_sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();

        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            "logs/server.log", 1024 * 1024 * 5, 3);  // 5MB per file, 3 rotated files

        // Create a multi-sink logger
        std::vector<spdlog::sink_ptr> sinks {console_sink};
        sinks.push_back(file_sink);

        auto logger = std::make_shared<spdlog::logger>("", sinks.begin(), sinks.end());
        
        // Set logging level (optional)
        logger->set_level(spdlog::level::info);

        spdlog::set_default_logger(logger);
        
        // Enable automatic flushing after each log message
        spdlog::flush_every(std::chrono::seconds(3));
    }
    catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Log initialization failed: " << ex.what() << '\n';
    }
}

// Log detailed request information for tracking and debugging
void logRequest(const std::string& clientIP, const std::string& method, 
                const std::string& path, int statusCode, 
                long waitingTime, long processingTime, long totalTime, 
                std::string backendResponse) {
    // Structured logging with comprehensive request details
    spdlog::info(
        "Request Details: "
        "IP: {} | Method: {} | Path: {} | Status: {} | "
        "Waiting Time: {} ms | Processing Time: {} ms | Total Time: {} ms | "
        "Backend Response: {}", 
        clientIP, method, path, statusCode, 
        waitingTime, processingTime, totalTime, 
        backendResponse
    );
}

// Log errors with context for easier troubleshooting
void logError(const std::string& errorMessage, const std::string& context) {
    // Error logging with additional context
    spdlog::error("Error Context: {} | Detailed Error: {}", context, errorMessage);
}
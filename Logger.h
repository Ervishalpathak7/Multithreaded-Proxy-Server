// logger.h

#ifndef LOGGER_H
#define LOGGER_H

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/async.h>



void setupLogger();
void logRequest(const std::string& clientIP, const std::string& method, 
                const std::string& path, int statusCode, 
                long waitingTime, long processingTime, long totalTime , std::string backendResponse);
void logError(const std::string& errorMessage, const std::string& context);

#endif // LOGGER_H

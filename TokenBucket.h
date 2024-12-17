// TokenBucket.h
#ifndef TOKEN_BUCKET_H
#define TOKEN_BUCKET_H

#include <unordered_map>
#include <mutex>
#include <chrono>
#include <string>

class AdvancedRateLimiter {
public:
    // Constructor with configurable parameters
    AdvancedRateLimiter(
        int globalMaxTokens = 10000,     // Global max requests
        double globalRefillRate = 10.0,// Global token refill rate
        int perIPMaxTokens = 100,       // Max requests per IP
        double perIPRefillRate = 2.0,  // Token refill rate per IP
        std::chrono::seconds windowDuration = std::chrono::seconds(60) // Tracking window
    );

    // Check if a request from a specific IP is allowed
    bool allowRequest(const std::string& clientIP);

    // Clear old entries to prevent memory leaks
    void cleanupStaleEntries();

private:
    // Global rate limiting parameters
    int globalCapacity;
    double globalRefillRate;
    double globalTokens;
    std::chrono::steady_clock::time_point globalLastRefillTime;

    // Per-IP rate limiting structure
    struct IPBucket {
        double tokens;
        std::chrono::steady_clock::time_point lastRefillTime;
        int consecutiveBlocks;  // Track consecutive blocking to implement progressive slowdown
    };

    // Containers for tracking
    std::unordered_map<std::string, IPBucket> ipBuckets;
    std::mutex mtx;

    // Configuration parameters
    int perIPCapacity;
    double perIPTokenRefillRate;
    std::chrono::seconds trackingWindow;

    // Internal methods for token management
    void refillGlobalTokens();
    bool consumeGlobalTokens();
    void refillIPTokens(IPBucket& bucket);
};

#endif // TOKEN_BUCKET_H
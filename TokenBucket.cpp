#include "TokenBucket.h"
#include <algorithm>
#include <iostream>

AdvancedRateLimiter::AdvancedRateLimiter(
    int globalMaxTokens, 
    double globalRefillRate, 
    int perIPMaxTokens, 
    double perIPRefillRate,
    std::chrono::seconds windowDuration
) : 
    globalCapacity(globalMaxTokens),
    globalRefillRate(globalRefillRate),
    globalTokens(globalMaxTokens),
    perIPCapacity(perIPMaxTokens),
    perIPTokenRefillRate(perIPRefillRate),
    trackingWindow(windowDuration)
{
    globalLastRefillTime = std::chrono::steady_clock::now();
}

bool AdvancedRateLimiter::allowRequest(const std::string& clientIP) {
    std::lock_guard<std::mutex> lock(mtx);

    auto now = std::chrono::steady_clock::now();

    // Refill global tokens
    refillGlobalTokens();

    // Check global rate limit first
    if (!consumeGlobalTokens()) {
        return false;
    }

    // Find or create IP bucket
    auto& ipBucket = ipBuckets[clientIP];

    // Refill IP-specific tokens
    refillIPTokens(ipBucket);

    // Check if IP has tokens
    if (ipBucket.tokens >= 1.0) {
        ipBucket.tokens -= 1.0;
        ipBucket.consecutiveBlocks = 0;  // Reset block counter on successful request
        return true;
    }

    // Implement progressive slowdown
    ipBucket.consecutiveBlocks++;
    
    // Exponential backoff: more consecutive blocks = longer block time
    if (ipBucket.consecutiveBlocks > 3) {
        std::cerr << "IP " << clientIP << " is being rate limited aggressively." << std::endl;
        return false;
    }

    return false;
}

void AdvancedRateLimiter::refillGlobalTokens() {
    auto now = std::chrono::steady_clock::now();
    double timeElapsed = std::chrono::duration<double>(now - globalLastRefillTime).count();

    globalTokens += timeElapsed * globalRefillRate;
    if (globalTokens > globalCapacity) globalTokens = globalCapacity;

    globalLastRefillTime = now;
}

bool AdvancedRateLimiter::consumeGlobalTokens() {
    if (globalTokens >= 1.0) {
        globalTokens -= 1.0;
        return true;
    }
    return false;
}

void AdvancedRateLimiter::refillIPTokens(IPBucket& bucket) {
    auto now = std::chrono::steady_clock::now();
    double timeElapsed = std::chrono::duration<double>(now - bucket.lastRefillTime).count();

    bucket.tokens += timeElapsed * perIPTokenRefillRate;
    if (bucket.tokens > perIPCapacity) bucket.tokens = perIPCapacity;

    bucket.lastRefillTime = now;
}

void AdvancedRateLimiter::cleanupStaleEntries() {
    std::lock_guard<std::mutex> lock(mtx);
    auto now = std::chrono::steady_clock::now();

    // Remove entries older than tracking window
    for (auto it = ipBuckets.begin(); it != ipBuckets.end(); ) {
        if (now - it->second.lastRefillTime > trackingWindow) {
            it = ipBuckets.erase(it);
        } else {
            ++it;
        }
    }
}
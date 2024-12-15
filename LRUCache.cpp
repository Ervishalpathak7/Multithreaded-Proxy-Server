#include "LRUCache.h"

// Constructor to initialize the cache with a given capacity
template <typename KeyType, typename ValueType>
LRUCache<KeyType, ValueType>::LRUCache(size_t capacity) : capacity(capacity) {}

// Method to get the value associated with a key
template <typename KeyType, typename ValueType>
bool LRUCache<KeyType, ValueType>::get(const KeyType& key, ValueType& value) {
    std::lock_guard<std::mutex> lock(cacheMutex);  // Lock the cache for thread safety

    // Check if the key is in the cache
    if (cache.find(key) != cache.end()) {
        value = cache[key]->second;  // Retrieve the value from the cache
        // Move the key to the back of the list to mark it as most recently used
        usageOrder.splice(usageOrder.end(), usageOrder, cache[key]);  // Move to the end of the list
        return true;
    }
    return false;
}

// Method to add or update a key-value pair in the cache
template <typename KeyType, typename ValueType>
void LRUCache<KeyType, ValueType>::put(const KeyType& key, const ValueType& value) {
    std::lock_guard<std::mutex> lock(cacheMutex);  // Lock the cache for thread safety

    // Check if the key already exists in the cache
    if (cache.find(key) != cache.end()) {
        cache[key]->second = value;  // Update the value
        // Move the key to the back of the list
        usageOrder.splice(usageOrder.end(), usageOrder, cache[key]);  // Move to the end of the list
    } else {
        // If the cache is full, remove the least recently used (LRU) key
        if (cache.size() >= capacity) {
            KeyType lruKey = usageOrder.front().first;  // The front of the list is the least recently used
            usageOrder.pop_front();  // Remove the LRU key from the list
            cache.erase(lruKey);  // Remove the LRU key from the cache
        }
        // Add the new key-value pair to the cache
        usageOrder.push_back(std::make_pair(key, value));  // Add to the list
        cache[key] = --usageOrder.end();  // Store the iterator to the list node
    }
}

// Explicit template instantiation for commonly used types
template class LRUCache<std::string, std::string>;

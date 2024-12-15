#include "LRUCache.h"
#include <list>



// Constructor to initialize the cache with a given capacity
template <typename KeyType, typename ValueType>
LRUCache<KeyType, ValueType>::LRUCache(size_t capacity) : capacity(capacity) {}

// Method to get the value associated with a key
template <typename KeyType, typename ValueType>
bool LRUCache<KeyType, ValueType>::get(const KeyType& key, ValueType& value) {
    for (auto it = cache.begin(); it != cache.end(); ++it) {
        if (it->first == key) {
            // Key found; move it to the back of the list (most recently used)
            value = it->second;
            cache.erase(it);
            cache.push_back({key, value});
            return true;
        }
    }
    // Key not found
    return false;
}



// Method to add or update a key-value pair in the cache
template <typename KeyType, typename ValueType>
void LRUCache<KeyType, ValueType>::put(const KeyType& key, const ValueType& value) {
    for (auto it = cache.begin(); it != cache.end(); ++it) {
        if (it->first == key) {
            // Key already exists; update its value and move it to the back
            cache.erase(it);
            break;
        }
    }

    // Check if we need to evict an item
    if (cache.size() >= capacity) {
        // Evict the least recently used item (front of the list)
        cache.pop_front();
    }

    // Add the new key-value pair to the back
    cache.push_back({key, value});
}

#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <iostream>
#include <list>
#include <unordered_map>
#include <mutex>

template <typename KeyType, typename ValueType>
class LRUCache {
public:
    LRUCache(size_t capacity);
    bool get(const KeyType& key, ValueType& value);
    void put(const KeyType& key, const ValueType& value);

private:
    size_t capacity;
    std::list<std::pair<KeyType, ValueType>> usageOrder;  // List to keep track of access order (key, value)
    std::unordered_map<KeyType, typename std::list<std::pair<KeyType, ValueType>>::iterator> cache;  // Hash map for O(1) access to list node
    std::mutex cacheMutex;  // Mutex for thread safety
};

#endif

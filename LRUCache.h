#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <list>
#include <utility> // For std::pair

template <typename KeyType, typename ValueType>
class LRUCache {
public:
    // Constructor to initialize the cache with a given capacity
    explicit LRUCache(size_t capacity);

    // Method to get the value associated with a key
    bool get(const KeyType& key, ValueType& value);

    // Method to add or update a key-value pair in the cache
    void put(const KeyType& key, const ValueType& value);

private:
    size_t capacity; // Maximum capacity of the cache
    std::list<std::pair<KeyType, ValueType>> cache; // List to store key-value pairs
};

#endif // LRU_CACHE_H

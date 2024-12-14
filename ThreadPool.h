#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional> // For std::function

class ThreadPool {
public:
    // Constructor
    ThreadPool(int numThreads);

    // Destructor
    ~ThreadPool();

    // Method to add a task to the queue
    void addTask(std::function<void()> task);

    // Method to start the thread pool
    void start(int numThreads);

    // Method to shut down the pool gracefully
    void shutdown();

private:
    std::queue<std::function<void()>> taskQueue;  // Queue to hold tasks
    std::mutex queueMutex;                       // Mutex to protect task queue
    std::condition_variable taskAvailable;       // Condition variable for task notification
    std::vector<std::thread> threads;            // Vector of worker threads
    bool isShutdown;                             // Flag to indicate shutdown
    int numThreads;                              // Number of threads
};

#endif // THREADPOOL_H

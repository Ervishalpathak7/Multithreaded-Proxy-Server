#include "ThreadPool.h"
#include <iostream>

// Constructor to initialize the thread pool
ThreadPool::ThreadPool(int numThreads) 
    : isShutdown(false), numThreads(numThreads) {
    // Initialize other members if needed
}

// Destructor to shut down the pool gracefully
ThreadPool::~ThreadPool() {
    shutdown(); 
}

// Add a new task to the queue
void ThreadPool::addTask(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        taskQueue.push(task);  // Add the task to the queue
    }
    taskAvailable.notify_one();  // Notify one waiting thread
}

// Start the worker threads
void ThreadPool::start(int numThreads) {
    this->numThreads = numThreads; // Store the thread count
    auto worker = [this]() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                // Wait until tasks are available or shutdown is triggered
                taskAvailable.wait(lock, [this]() { 
                    return !taskQueue.empty() || isShutdown; 
                });

                // If shutting down and no tasks, exit the loop
                if (isShutdown && taskQueue.empty()) {
                    break;
                }

                // Get the task from the queue
                task = taskQueue.front();
                taskQueue.pop();
            }
            // Execute the task outside the lock to avoid blocking other threads
            task();
        }
    };

    // Create the specified number of threads
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(worker);  // Add a worker thread to the pool
    }
}

// Shutdown the thread pool
void ThreadPool::shutdown() {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        isShutdown = true;  // Set the shutdown flag
    }
    taskAvailable.notify_all();  // Wake up all threads to finish processing

    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();  // Wait for all threads to finish
        }
    }

    threads.clear(); // Clear the threads for reuse (if needed)
}

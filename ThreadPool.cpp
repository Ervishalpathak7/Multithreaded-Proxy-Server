#include "ThreadPool.h"
#include <iostream>
#include <stdexcept>
#include <exception>

// Constructor to initialize the thread pool
ThreadPool::ThreadPool(int numThreads) 
    : isShutdown(false), numThreads(numThreads) {
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([this]() {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queueMutex);
                    taskAvailable.wait(lock, [this]() { return isShutdown || !taskQueue.empty(); });
                    if (isShutdown && taskQueue.empty()) {
                        return; // Exit thread if shutdown and no tasks are available
                    }
                    task = std::move(taskQueue.front());
                    taskQueue.pop();
                }
                try {
                    task();
                } catch (const std::exception &e) {
                    std::cerr << "Task exception: " << e.what() << std::endl;
                } catch (...) {
                    std::cerr << "Task threw an unknown exception." << std::endl;
                }
            }
        });
    }
}

// Destructor to shut down the pool gracefully
ThreadPool::~ThreadPool() {
    shutdown();
}

// Add a new task to the queue
void ThreadPool::addTask(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (isShutdown) {
            throw std::runtime_error("Cannot add tasks to a shutting down ThreadPool.");
        }
        taskQueue.push(std::move(task));
    }
    taskAvailable.notify_one();
}

// Shutdown the thread pool
void ThreadPool::shutdown() {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (isShutdown) {
            std::cerr << "ThreadPool is already shutting down!" << std::endl;
            return;
        }
        isShutdown = true;
    }
    taskAvailable.notify_all(); // Wake up all threads to allow them to exit

    // Wait for all threads to finish execution
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join(); // Ensure all threads complete their work before shutting down
        }
    }
}

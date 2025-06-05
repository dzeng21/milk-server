#ifndef MILK_SERVER_H
#define MILK_SERVER_H

#include <mutex>
#include <thread>
#include <condition_variable>
#include <functional>
#include <sstream>
#include <iostream>
#include <queue>
#include <string>
#include <vector>

#pragma once

class ThreadPool {
public:
    ThreadPool() {}

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> queue_lk(queue_mutex_);
            stopped_ = true;
        }

        queue_cv_.notify_all();

        for (auto& thread : threads_) {
            thread.join();
        }
    }

    void init(size_t num_threads = std::thread::hardware_concurrency()) {
        std::stringstream s;
        for (size_t i = 0; i < num_threads; i++) {
            threads_.emplace_back([this]() {
                while (true) {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> queue_lk(queue_mutex_);
                        
                        queue_cv_.wait(queue_lk, [this]() {
                            return !tasks_.empty() || stopped_;
                        });
                        
                        if (stopped_ && tasks_.empty()) {
                            return;
                        }

                        task = move(tasks_.front());
                        tasks_.pop();
                    }

                    task();
                }
            });
        }
        s << "initialized thread pool with " << num_threads << " threads\n";
        std::cout << s.str();
    }

    void enqueue(std::function<void()> task) {
        {
            std::unique_lock<std::mutex> queue_lk(queue_mutex_);
            tasks_.emplace(task);
        }
        queue_cv_.notify_one();
    }

private:
    std::vector<std::thread> threads_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    bool stopped_ = false;
};

// Global variables
extern std::unordered_map<std::string, int> milk_storage;
extern ThreadPool pool;
extern std::atomic<bool> stop;

// Signal handler
void signal_handler(int sig);

// Milk storage
void init_milk_storage();
int read_milk_storage_unsafe();
int write_milk_storage_unsafe();

// User milk operations
bool user_exists(std::string& user);
int user_get_global_milk();
int user_register(std::string& user);
int user_add_milk(std::string& user, int milk);
int user_set_milk(std::string& user, int milk);
int user_get_milk(std::string& user);

// Parsing and processing
int tokenize(const char (&buffer)[], std::vector<std::string>& tokens);
int read_buffer(const char (&buffer)[], std::string& response);

// Server networking
void handle_accept_client(int serverSocket);
void handle_client(int clientSocket);

#endif // MILK_SERVER_H
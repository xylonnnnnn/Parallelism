#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <utility>

namespace lab {
template <class T>
class TaskServer {
public:
    using result_type = T;
    using task_type = std::function<T()>;

    TaskServer() = default;
    TaskServer(const TaskServer&) = delete;
    TaskServer& operator=(const TaskServer&) = delete;

    ~TaskServer() { stop(); }

    void start() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (running_) {
            return;
        }
        stop_requested_ = false;
        running_ = true;
        worker_ = std::thread(&TaskServer::worker_loop, this);
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!running_ && !worker_.joinable()) {
                return;
            }
            stop_requested_ = true;
        }
        queue_cv_.notify_all();
        result_cv_.notify_all();
        if (worker_.joinable()) {
            worker_.join();
        }
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;
    }

    size_t add_task(task_type task) {
        if (!task) {
            throw std::invalid_argument("empty task");
        }

        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_ || stop_requested_) {
            throw std::runtime_error("server is not running");
        }

        const size_t id = next_id_++;
        tasks_.push(queued_task{id, std::move(task)});
        queue_cv_.notify_one();
        return id;
    }

    T request_result(size_t id_res) {
        std::unique_lock<std::mutex> lock(mutex_);
        result_cv_.wait(lock, [this, id_res] {
            return results_.contains(id_res) || (stop_requested_ && tasks_.empty());
        });

        auto it = results_.find(id_res);
        if (it == results_.end()) {
            throw std::runtime_error("result is unavailable: server was stopped");
        }

        T value = std::move(it->second);
        results_.erase(it);
        return value;
    }

    std::optional<T> try_request_result(size_t id_res) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = results_.find(id_res);
        if (it == results_.end()) {
            return std::nullopt;
        }
        T value = std::move(it->second);
        results_.erase(it);
        return value;
    }

private:
    struct queued_task {
        size_t id;
        task_type task;
    };

    void worker_loop() {
        while (true) {
            queued_task current;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                queue_cv_.wait(lock, [this] {
                    return stop_requested_ || !tasks_.empty();
                });

                if (stop_requested_ && tasks_.empty()) {
                    break;
                }

                current = std::move(tasks_.front());
                tasks_.pop();
            }

            T value = current.task();

            {
                std::lock_guard<std::mutex> lock(mutex_);
                results_.emplace(current.id, std::move(value));
            }
            result_cv_.notify_all();
        }
    }

    std::mutex mutex_;
    std::condition_variable queue_cv_;
    std::condition_variable result_cv_;

    std::queue<queued_task> tasks_;
    std::unordered_map<size_t, T> results_;

    std::thread worker_;
    size_t next_id_ = 1;
    bool running_ = false;
    bool stop_requested_ = false;
};

} 

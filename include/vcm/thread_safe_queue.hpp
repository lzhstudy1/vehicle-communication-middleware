#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <chrono>

template <typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;

    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    void push(T value) {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            if (stopped_) return;
            queue_.push(std::move(value));
        }
        cv_.notify_one();
    }

    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (queue_.empty()) return false;
        value = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    bool wait_and_pop(T& value, int timeout_ms) {
        std::unique_lock<std::mutex> lock(mtx_);

        bool ok = cv_.wait_for(
            lock,
            std::chrono::milliseconds(timeout_ms),
            [this] { return !queue_.empty() || stopped_; });

        if (!ok) return false;
        if (queue_.empty()) return false;

        value = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            stopped_ = true;
        }
        cv_.notify_all();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.empty();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.size();
    }

private:
    mutable std::mutex mtx_;
    std::condition_variable cv_;
    std::queue<T> queue_;
    bool stopped_ = false;
};
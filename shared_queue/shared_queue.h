#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

namespace util {

template <typename T>
class SharedQueue {
public:
    SharedQueue() = default;
    ~SharedQueue() = default;

    // Add an element to the queue
    void Enqueue(const T& item) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(item);
        }
        cond_var_.notify_one();
    }

    // Get the front element from the queue, blocks if the queue is empty.
    std::optional<T> Dequeue() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_var_.wait(lock, [this] { return closed_ || !queue_.empty(); });
        if (closed_) {
            return std::nullopt;
        }
        T item = queue_.front();
        queue_.pop();
        return item;
    }

    // Check if the queue is empty
    bool Empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    // Get the size of the queue
    size_t Size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    void Close() {
        closed_ = true;
        cond_var_.notify_all();
    }

private:
    mutable std::mutex mutex_;
    std::queue<T> queue_;
    std::condition_variable cond_var_;
    bool closed_ = false;
};

}  // namespace util

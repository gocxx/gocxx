#pragma once
#include <mutex>
#include <condition_variable>
#include <stdexcept>

namespace gocxx::sync {

/**
 * @brief A synchronization primitive that waits for a group of tasks to complete.
 *
 * Inspired by Go's `sync.WaitGroup`. Use `Add()` to set the number of tasks, `Done()` when a task finishes,
 * and `Wait()` to block until all tasks are complete.
 */
class WaitGroup {
    std::mutex mtx_;
    std::condition_variable cv_;
    int count_ = 0;

public:
    /**
     * @brief Adds delta to the WaitGroup counter.
     *
     * @param delta The number of tasks to add. Can be negative (but should be used carefully).
     */
    void Add(int delta) {
        std::lock_guard<std::mutex> lock(mtx_);
        count_ += delta;
        if (count_ < 0) {
            throw std::runtime_error("WaitGroup counter went negative");
        }
        if (count_ == 0) {
            cv_.notify_all();
        }
    }

    /**
     * @brief Marks one task as done. Decrements the counter.
     */
    void Done() {
        Add(-1);
    }

    /**
     * @brief Blocks until the counter becomes zero.
     */
    void Wait() {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this] { return count_ == 0; });
    }
};

}  // namespace gocxx::sync

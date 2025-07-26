#pragma once
#include <mutex>
#include <condition_variable>
#include <functional>
#include "mutex.h"


namespace gocxx::sync {

/**
 * @brief A condition variable, similar to Go's sync.Cond.
 *
 * Used for signaling between threads that a condition or state has changed.
 * Internally wraps a `std::condition_variable`.
 */
class Cond {
    std::condition_variable cv_;

public:
    /**
     * @brief Blocks the calling thread until notified.
     *
     * The thread must hold a `std::unique_lock<std::mutex>` before calling this.
     * It will release the lock while waiting and re-acquire it upon wake-up.
     *
     * @param lock A unique lock that is held before calling wait.
     */
    void Wait(UniqueLock& mtx) {
        cv_.wait(mtx);
    }

    /**
     * @brief Wakes one thread that is waiting on this condition variable.
     */
    void NotifyOne() {
        cv_.notify_one();
    }

    /**
     * @brief Wakes all threads waiting on this condition variable.
     */
    void NotifyAll() {
        cv_.notify_all();
    }
};

}  // namespace gocxx::sync

#pragma once

#include <functional>
#include <stack>
#include <mutex>
#include <memory>

namespace gocxx::sync {

/**
 * @brief A thread-safe object pool, similar to Go's `sync.Pool`.
 *
 * Designed for efficient reuse of expensive-to-create objects.
 * It stores objects in a stack and reuses them across multiple threads.
 *
 * @tparam T The type of objects to store.
 */
template <typename T>
class Pool {
    std::stack<std::shared_ptr<T>> pool_;
    std::mutex mtx_;
    std::function<std::shared_ptr<T>()> newFunc_;

public:
    /**
     * @brief Constructs a Pool with a custom creation function.
     *
     * @param newFunc A function that creates a new object when the pool is empty.
     */
    explicit Pool(std::function<std::shared_ptr<T>()> newFunc)
        : newFunc_(std::move(newFunc)) {}

    /**
     * @brief Retrieves an object from the pool.
     *
     * If the pool is empty, a new object is created using `newFunc`.
     *
     * @return A shared pointer to the object.
     */
    std::shared_ptr<T> Get() {
        std::lock_guard<std::mutex> lock(mtx_);
        if (pool_.empty()) {
            return newFunc_();
        }
        auto obj = pool_.top();
        pool_.pop();
        return obj;
    }

    /**
     * @brief Returns an object back to the pool for future reuse.
     *
     * @param obj A shared pointer to the object to store.
     */
    void Put(std::shared_ptr<T> obj) {
        std::lock_guard<std::mutex> lock(mtx_);
        pool_.push(std::move(obj));
    }
};


}  // namespace gocxx::sync

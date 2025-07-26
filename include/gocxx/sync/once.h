#pragma once
#include <mutex>

namespace gocxx::sync {

/**
 * @brief A wrapper for one-time execution, similar to Go's sync.Once.
 *
 * Ensures that the given function is only executed once, even when called
 * concurrently from multiple threads.
 */
class Once {
    std::once_flag flag_;
public:
    /**
     * @brief Executes the provided callable only once.
     *
     * @tparam Callable A callable object like a lambda or function pointer.
     * @param fn The function to execute.
     */
    template <typename Callable>
    void Do(Callable&& fn) {
        std::call_once(flag_, std::forward<Callable>(fn));
    }
};

}  // namespace gocxx::sync

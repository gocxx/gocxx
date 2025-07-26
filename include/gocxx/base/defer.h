/**
 * @file defer.h
 * @brief RAII-based defer mechanism similar to Go's defer statement
 * 
 * Provides a defer macro and class that ensures cleanup code runs
 * when leaving a scope, similar to Go's defer statement.
 */

#pragma once
#include <utility>
#include <functional>

#define CONCAT2(x, y) x##y
#define CONCAT(x, y) CONCAT2(x, y)
/**
 * @def defer
 * @brief Macro for Go-like defer functionality
 * 
 * Executes the given lambda/function when leaving the current scope.
 * This provides similar functionality to Go's defer statement.
 * 
 * @par Example
 * @code
 * {
 *     auto* resource = acquire_resource();
 *     defer([resource]() {
 *         release_resource(resource);
 *     });
 *     
 *     // Use resource...
 *     // resource will be automatically released when leaving scope
 * }
 * @endcode
 */
#define defer auto CONCAT(_defer_, __COUNTER__) = gocxx::sync::Defer

/**
 * @namespace gocxx::sync
 * @brief Synchronization primitives and utilities
 * 
 * This namespace contains thread synchronization tools including
 * defer, mutexes, wait groups, and other concurrency utilities.
 */

namespace gocxx {
namespace sync {

/**
 * @class Defer
 * @brief RAII class for defer functionality
 * 
 * This class implements the defer mechanism by storing a function
 * and calling it in the destructor. Use the defer macro for convenience.
 * 
 * @par Thread Safety
 * Individual Defer objects are not thread-safe, but multiple Defer
 * objects can be used concurrently in different threads.
 * 
 * @par Example
 * @code
 * {
 *     gocxx::sync::Defer cleanup([]() {
 *         std::cout << "Cleanup executed\n";
 *     });
 *     
 *     // Or use the macro (recommended):
 *     defer([]() {
 *         std::cout << "This runs when leaving scope\n";
 *     });
 * }
 * @endcode
 */
class Defer {
    std::function<void()> fn_;
public:
    /**
     * @brief Construct a Defer object with a cleanup function
     * @param fn Function to call when the object is destroyed
     */
    explicit Defer(std::function<void()> fn) : fn_(std::move(fn)) {}
    
    /**
     * @brief Destructor that executes the deferred function
     */
    ~Defer() { fn_(); }

    /// @brief Defer objects cannot be copied
    Defer(const Defer&) = delete;
    /// @brief Defer objects cannot be assigned
    Defer& operator=(const Defer&) = delete;
};

} // namespace sync
} // namespace gocxx

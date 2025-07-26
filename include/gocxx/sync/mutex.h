#pragma once
#include <mutex>

namespace gocxx::sync {

	/**
	 * @brief Alias for `std::mutex` to match Go's `sync.Mutex`.
	 */
	using Mutex = std::mutex;

	/**
	 * @brief A simple scoped lock guard (RAII).
	 * Equivalent to Go's:
	 *     mu.Lock()
	 *     defer mu.Unlock()
	 */
	using Lock = std::lock_guard<Mutex>;

	/**
	 * @brief A flexible lock with manual control.
	 * Use this when working with condition variables or advanced patterns.
	 */
	using UniqueLock = std::unique_lock<Mutex>;

}  // namespace gocxx::sync

#pragma once
#include <shared_mutex>

namespace gocxx::sync {

/**
 * @brief Alias for a read-write mutex (similar to Go's `sync.RWMutex`).
 *
 * Allows multiple readers or a single writer.
 */
using RWMutex = std::shared_mutex;

/**
 * @brief Shared (read-only) lock for RWMutex.
 *
 * Equivalent to calling `RLock()` in Go. Multiple `ReadLock`s can coexist.
 */
using ReadLock = std::shared_lock<RWMutex>;

/**
 * @brief Exclusive (write) lock for RWMutex.
 *
 * Equivalent to `Lock()` in Go. Only one `WriteLock` can exist at a time.
 */
using WriteLock = std::unique_lock<RWMutex>;

}  // namespace gocxx::sync

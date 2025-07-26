/**
 * @file context.h
 * @brief Go-inspired context for cancellation, timeouts, and request-scoped values
 * 
 * This module provides Go-like context functionality that matches Go's context API exactly:
 * - Context.Deadline() (deadline time.Time, ok bool)
 * - Context.Done() <-chan struct{}
 * - Context.Err() error
 * - Context.Value(key interface{}) interface{}
 * 
 * Key features:
 * - Exact Go API mapping with gocxx Result types
 * - Cancellation propagation across call chains
 * - Timeout and deadline support
 * - Request-scoped value storage
 * - Thread-safe operations
 * - RAII-based resource management
 * 
 * @author gocxx
 * @date 2025
 */

#pragma once

#include <memory>
#include <string>
#include <chrono>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <any>
#include <vector>
#include <thread>
#include <atomic>

#include <gocxx/base/result.h>
#include <gocxx/base/chan.h>
#include <gocxx/time/time.h>
#include <condition_variable>
#include <atomic>
#include <any>
#include <typeindex>
#include <future>
#include <thread>
#include <gocxx/base/result.h>
#include <gocxx/base/chan.h>
#include <gocxx/time/time.h>

namespace gocxx {
namespace context {

// Forward declarations
class Context;
using ContextPtr = std::shared_ptr<Context>;

/**
 * @brief Context interface matching Go's context.Context exactly
 * 
 * Context carries deadlines, cancellation signals, and request-scoped values
 * across API boundaries and between goroutines.
 */
class Context {
public:
    virtual ~Context() = default;
    
    /**
     * @brief Returns the deadline for this context
     * Go equivalent: Deadline() (deadline time.Time, ok bool)
     * @return Result containing deadline if set, or error if no deadline
     */
    virtual gocxx::base::Result<gocxx::time::Time> Deadline() const = 0;
    
    /**
     * @brief Returns a channel that is closed when context is canceled
     * Go equivalent: Done() <-chan struct{}
     * @return Channel that receives cancellation signal
     */
    virtual gocxx::base::Chan<bool> Done() const = 0;
    
    /**
     * @brief Returns error explaining why context was canceled
     * Go equivalent: Err() error
     * @return Result with error if context is canceled, success if not canceled
     */
    virtual gocxx::base::Result<void> Err() const = 0;
    
    /**
     * @brief Returns value associated with key
     * Go equivalent: Value(key interface{}) interface{}
     * @param key The key to look up
     * @return Result containing value if found, error if not found
     */
    virtual gocxx::base::Result<std::any> Value(const std::any& key) const = 0;
};

/**
 * @brief Cancellation function type
 * Go equivalent: type CancelFunc func()
 */
using CancelFunc = std::function<void()>;

/**
 * @brief Context with cancellation capability
 */
class CancelContext : public Context {
private:
    ContextPtr parent_;
    mutable std::mutex mutex_;
    std::atomic<bool> canceled_;
    std::string err_;
    mutable gocxx::base::Chan<bool> done_chan_;
    std::vector<std::weak_ptr<CancelContext>> children_;
    
public:
    explicit CancelContext(ContextPtr parent = nullptr);
    
    gocxx::base::Result<gocxx::time::Time> Deadline() const override;
    gocxx::base::Chan<bool> Done() const override;
    gocxx::base::Result<void> Err() const override;
    gocxx::base::Result<std::any> Value(const std::any& key) const override;
    
    /**
     * @brief Cancel this context and all its children
     * @param reason Reason for cancellation
     */
    void Cancel(const std::string& reason = "context canceled");
    
    /**
     * @brief Add a child context
     * @param child Child context to add
     */
    void AddChild(std::shared_ptr<CancelContext> child);
    
    /**
     * @brief Check if context is canceled (convenience method, not in Go)
     * @return true if context is canceled
     */
    bool IsCanceled() const { return canceled_.load(); }
};

/**
 * @brief Context with timeout/deadline
 */
class TimerContext : public CancelContext {
private:
    gocxx::time::Time deadline_;
    std::unique_ptr<std::thread> timer_thread_;
    
public:
    explicit TimerContext(ContextPtr parent, gocxx::time::Time deadline);
    explicit TimerContext(ContextPtr parent, gocxx::time::Duration timeout);
    ~TimerContext();
    
    gocxx::base::Result<gocxx::time::Time> Deadline() const override;
    
private:
    void StartTimer();
};

/**
 * @brief Context with values
 */
class ValueContext : public Context {
private:
    ContextPtr parent_;
    std::any key_;
    std::any value_;
    
public:
    ValueContext(ContextPtr parent, const std::any& key, const std::any& value);
    
    gocxx::base::Result<gocxx::time::Time> Deadline() const override;
    gocxx::base::Chan<bool> Done() const override;
    gocxx::base::Result<void> Err() const override;
    gocxx::base::Result<std::any> Value(const std::any& key) const override;
};

/**
 * @brief Background context (never canceled, no deadline, no values)
 */
class BackgroundContext : public Context {
public:
    gocxx::base::Result<gocxx::time::Time> Deadline() const override;
    gocxx::base::Chan<bool> Done() const override;
    gocxx::base::Result<void> Err() const override;
    gocxx::base::Result<std::any> Value(const std::any& key) const override;
};

/**
 * @brief TODO context (like background but indicates missing context)
 */
class TODOContext : public BackgroundContext {
    // Same as BackgroundContext but semantically different
};

// Factory functions - exact Go API

/**
 * @brief Returns a non-nil, empty Context
 * Go equivalent: context.Background() Context
 * @return Background context that is never canceled
 */
ContextPtr Background();

/**
 * @brief Returns a non-nil, empty Context marked as TODO
 * Go equivalent: context.TODO() Context
 * @return TODO context for development placeholders
 */
ContextPtr TODO();

/**
 * @brief Returns a copy of parent with a cancel function
 * Go equivalent: context.WithCancel(parent Context) (Context, CancelFunc)
 * @param parent Parent context
 * @return Result containing pair of (context, cancel_function)
 */
gocxx::base::Result<std::pair<ContextPtr, CancelFunc>> WithCancel(ContextPtr parent);

/**
 * @brief Returns a copy of parent with a timeout
 * Go equivalent: context.WithTimeout(parent Context, timeout time.Duration) (Context, CancelFunc)
 * @param parent Parent context
 * @param timeout Timeout duration
 * @return Result containing pair of (context, cancel_function)
 */
gocxx::base::Result<std::pair<ContextPtr, CancelFunc>> WithTimeout(
    ContextPtr parent, 
    gocxx::time::Duration timeout
);

/**
 * @brief Returns a copy of parent with a deadline
 * Go equivalent: context.WithDeadline(parent Context, d time.Time) (Context, CancelFunc)
 * @param parent Parent context
 * @param deadline Absolute deadline
 * @return Result containing pair of (context, cancel_function)
 */
gocxx::base::Result<std::pair<ContextPtr, CancelFunc>> WithDeadline(
    ContextPtr parent, 
    gocxx::time::Time deadline
);

/**
 * @brief Returns a copy of parent with an associated key-value pair
 * Go equivalent: context.WithValue(parent Context, key, val interface{}) Context
 * @param parent Parent context
 * @param key Key for the value
 * @param value Value to associate
 * @return Result containing context with the value
 */
gocxx::base::Result<ContextPtr> WithValue(
    ContextPtr parent, 
    const std::any& key, 
    const std::any& value
);

// Error constants - exact Go equivalents
extern const std::string Canceled;        // "context canceled"
extern const std::string DeadlineExceeded; // "context deadline exceeded"

// Utility functions for interoperability

/**
 * @brief Sleep that respects context cancellation
 * @param ctx Context to check for cancellation
 * @param duration Duration to sleep
 * @return Result indicating success or cancellation
 */
gocxx::base::Result<void> SleepWithContext(ContextPtr ctx, gocxx::time::Duration duration);

/**
 * @brief Wait for context to be done or timeout
 * @param ctx Context to wait on
 * @param timeout Maximum time to wait
 * @return Result indicating whether context was canceled or timeout occurred
 */
gocxx::base::Result<bool> WaitForContext(ContextPtr ctx, gocxx::time::Duration timeout);

/**
 * @brief Check if context will be canceled soon (within duration)
 * @param ctx Context to check
 * @param within Duration to check within
 * @return Result indicating whether context will be canceled within duration
 */
gocxx::base::Result<bool> WillBeCanceledSoon(ContextPtr ctx, gocxx::time::Duration within);

} // namespace context
} // namespace gocxx

/**
 * @file context.cpp
 * @brief Implementation of Go-inspired context functionality with exact Go API mapping
 */

#include <gocxx/context/context.h>
#include <gocxx/base/result.h>
#include <gocxx/time/time.h>
#include <gocxx/errors/errors.h>
#include <thread>
#include <algorithm>

namespace gocxx {
namespace context {

// Error constants - exact Go equivalents
const std::string Canceled = "context canceled";
const std::string DeadlineExceeded = "context deadline exceeded";

// CancelContext implementation

CancelContext::CancelContext(ContextPtr parent)
    : parent_(parent), canceled_(false), done_chan_(1) {
    // Initialize done channel as unbuffered, will be closed on cancellation
}

gocxx::base::Result<gocxx::time::Time> CancelContext::Deadline() const {
    if (parent_) {
        return parent_->Deadline();
    }
    return gocxx::base::Result<gocxx::time::Time>(gocxx::errors::New("no deadline"));
}

gocxx::base::Chan<bool> CancelContext::Done() const {
    return done_chan_;
}

gocxx::base::Result<void> CancelContext::Err() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (canceled_) {
        return gocxx::base::Result<void>(gocxx::errors::New(err_));
    }
    return gocxx::base::Result<void>();
}

gocxx::base::Result<std::any> CancelContext::Value(const std::any& key) const {
    // Check parent
    if (parent_) {
        return parent_->Value(key);
    }
    
    return gocxx::base::Result<std::any>(gocxx::errors::New("key not found"));
}

void CancelContext::Cancel(const std::string& reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (canceled_) {
        return; // Already canceled
    }
    
    canceled_ = true;
    err_ = reason;
    
    // Close the done channel
    done_chan_.close();
    
    // Cancel all children
    for (auto it = children_.begin(); it != children_.end();) {
        if (auto child = it->lock()) {
            child->Cancel(reason);
            ++it;
        } else {
            // Remove expired weak_ptr
            it = children_.erase(it);
        }
    }
}

void CancelContext::AddChild(std::shared_ptr<CancelContext> child) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (canceled_) {
        // If already canceled, cancel the child immediately
        child->Cancel(err_);
        return;
    }
    
    children_.push_back(std::weak_ptr<CancelContext>(child));
}

// TimerContext implementation

TimerContext::TimerContext(ContextPtr parent, gocxx::time::Time deadline)
    : CancelContext(parent), deadline_(deadline) {
    StartTimer();
}

TimerContext::TimerContext(ContextPtr parent, gocxx::time::Duration timeout)
    : CancelContext(parent), deadline_(gocxx::time::Time::Now().Add(timeout)) {
    StartTimer();
}

TimerContext::~TimerContext() {
    if (timer_thread_ && timer_thread_->joinable()) {
        Cancel("destructor called");
        timer_thread_->join();
    }
}

gocxx::base::Result<gocxx::time::Time> TimerContext::Deadline() const {
    return gocxx::base::Result<gocxx::time::Time>(deadline_);
}

void TimerContext::StartTimer() {
    timer_thread_ = std::make_unique<std::thread>([this]() {
        auto now = gocxx::time::Time::Now();
        if (deadline_.After(now)) {
            auto duration = deadline_.Sub(now);
            std::this_thread::sleep_for(duration.ToStdDuration());
        }
        
        if (!IsCanceled()) {
            Cancel(DeadlineExceeded);
        }
    });
}

// ValueContext implementation

ValueContext::ValueContext(ContextPtr parent, const std::any& key, const std::any& value)
    : parent_(parent), key_(key), value_(value) {
}

gocxx::base::Result<gocxx::time::Time> ValueContext::Deadline() const {
    if (parent_) {
        return parent_->Deadline();
    }
    return gocxx::base::Result<gocxx::time::Time>(gocxx::errors::New("no deadline"));
}

gocxx::base::Chan<bool> ValueContext::Done() const {
    if (parent_) {
        return parent_->Done();
    }
    // Return a never-closing channel for value-only contexts
    static gocxx::base::Chan<bool> never_done(1);
    return never_done;
}

gocxx::base::Result<void> ValueContext::Err() const {
    if (parent_) {
        return parent_->Err();
    }
    return gocxx::base::Result<void>();
}

gocxx::base::Result<std::any> ValueContext::Value(const std::any& key) const {
    // Check if this is the key we're storing
    if (key_.type() == key.type()) {
        try {
            if (key_.type() == typeid(std::string)) {
                auto our_key = std::any_cast<std::string>(key_);
                auto search_key = std::any_cast<std::string>(key);
                if (our_key == search_key) {
                    return gocxx::base::Result<std::any>(value_);
                }
            } else if (key_.type() == typeid(int)) {
                auto our_key = std::any_cast<int>(key_);
                auto search_key = std::any_cast<int>(key);
                if (our_key == search_key) {
                    return gocxx::base::Result<std::any>(value_);
                }
            } else if (key_.type() == typeid(const char*)) {
                auto our_key = std::any_cast<const char*>(key_);
                auto search_key = std::any_cast<const char*>(key);
                if (std::string(our_key) == std::string(search_key)) {
                    return gocxx::base::Result<std::any>(value_);
                }
            }
        } catch (const std::bad_any_cast&) {
            // Keys don't match, continue to parent
        }
    }
    
    // Check parent
    if (parent_) {
        return parent_->Value(key);
    }
    
    return gocxx::base::Result<std::any>(gocxx::errors::New("key not found"));
}

// BackgroundContext implementation

gocxx::base::Result<gocxx::time::Time> BackgroundContext::Deadline() const {
    return gocxx::base::Result<gocxx::time::Time>(gocxx::errors::New("no deadline"));
}

gocxx::base::Chan<bool> BackgroundContext::Done() const {
    // Return a never-closing channel
    static gocxx::base::Chan<bool> never_done(1);
    return never_done;
}

gocxx::base::Result<void> BackgroundContext::Err() const {
    return gocxx::base::Result<void>();
}

gocxx::base::Result<std::any> BackgroundContext::Value(const std::any& key) const {
    return gocxx::base::Result<std::any>(gocxx::errors::New("key not found"));
}

// Factory functions - exact Go API

ContextPtr Background() {
    static auto bg_context = std::make_shared<BackgroundContext>();
    return bg_context;
}

ContextPtr TODO() {
    static auto todo_context = std::make_shared<TODOContext>();
    return todo_context;
}

gocxx::base::Result<std::pair<ContextPtr, CancelFunc>> WithCancel(ContextPtr parent) {
    if (!parent) {
        return gocxx::base::Result<std::pair<ContextPtr, CancelFunc>>(gocxx::errors::New("parent context is nil"));
    }
    
    auto cancel_ctx = std::make_shared<CancelContext>(parent);
    
    // If parent is also a CancelContext, add this as a child
    if (auto parent_cancel_ctx = std::dynamic_pointer_cast<CancelContext>(parent)) {
        parent_cancel_ctx->AddChild(cancel_ctx);
    }
    
    CancelFunc cancel_func = [cancel_ctx]() {
        cancel_ctx->Cancel();
    };
    
    return gocxx::base::Result<std::pair<ContextPtr, CancelFunc>>(
        std::make_pair(cancel_ctx, cancel_func)
    );
}

gocxx::base::Result<std::pair<ContextPtr, CancelFunc>> WithTimeout(
    ContextPtr parent, 
    gocxx::time::Duration timeout) {
    
    if (!parent) {
        return gocxx::base::Result<std::pair<ContextPtr, CancelFunc>>(gocxx::errors::New("parent context is nil"));
    }
    
    auto timer_ctx = std::make_shared<TimerContext>(parent, timeout);
    
    // If parent is a CancelContext, add this as a child
    if (auto parent_cancel_ctx = std::dynamic_pointer_cast<CancelContext>(parent)) {
        parent_cancel_ctx->AddChild(timer_ctx);
    }
    
    CancelFunc cancel_func = [timer_ctx]() {
        timer_ctx->Cancel();
    };
    
    return gocxx::base::Result<std::pair<ContextPtr, CancelFunc>>(
        std::make_pair(timer_ctx, cancel_func)
    );
}

gocxx::base::Result<std::pair<ContextPtr, CancelFunc>> WithDeadline(
    ContextPtr parent, 
    gocxx::time::Time deadline) {
    
    if (!parent) {
        return gocxx::base::Result<std::pair<ContextPtr, CancelFunc>>(gocxx::errors::New("parent context is nil"));
    }
    
    auto timer_ctx = std::make_shared<TimerContext>(parent, deadline);
    
    // If parent is a CancelContext, add this as a child
    if (auto parent_cancel_ctx = std::dynamic_pointer_cast<CancelContext>(parent)) {
        parent_cancel_ctx->AddChild(timer_ctx);
    }
    
    CancelFunc cancel_func = [timer_ctx]() {
        timer_ctx->Cancel();
    };
    
    return gocxx::base::Result<std::pair<ContextPtr, CancelFunc>>(
        std::make_pair(timer_ctx, cancel_func)
    );
}

gocxx::base::Result<ContextPtr> WithValue(
    ContextPtr parent, 
    const std::any& key, 
    const std::any& value) {
    
    if (!parent) {
        return gocxx::base::Result<ContextPtr>(gocxx::errors::New("parent context is nil"));
    }
    
    auto value_ctx = std::make_shared<ValueContext>(parent, key, value);
    return gocxx::base::Result<ContextPtr>(value_ctx);
}

// Utility functions

gocxx::base::Result<void> SleepWithContext(ContextPtr ctx, gocxx::time::Duration duration) {
    if (!ctx) {
        std::this_thread::sleep_for(duration.ToStdDuration());
        return gocxx::base::Result<void>();
    }
    
    auto start = gocxx::time::Time::Now();
    auto end = start.Add(duration);
    
    while (gocxx::time::Time::Now().Before(end)) {
        auto err_result = ctx->Err();
        if (!err_result.Ok()) {
            return gocxx::base::Result<void>(gocxx::errors::New("context canceled during sleep"));
        }
        
        // Sleep in small increments to check cancellation
        auto remaining = end.Sub(gocxx::time::Time::Now());
        auto sleep_duration = std::min(remaining, gocxx::time::Milliseconds(10));
        
        if (sleep_duration.Nanoseconds() > 0) {
            std::this_thread::sleep_for(sleep_duration.ToStdDuration());
        }
    }
    
    auto final_err = ctx->Err();
    if (!final_err.Ok()) {
        return gocxx::base::Result<void>(gocxx::errors::New("context canceled during sleep"));
    }
    
    return gocxx::base::Result<void>();
}

gocxx::base::Result<bool> WaitForContext(ContextPtr ctx, gocxx::time::Duration timeout) {
    if (!ctx) {
        return gocxx::base::Result<bool>(gocxx::errors::New("context is nil"));
    }
    
    auto start = gocxx::time::Time::Now();
    auto end = start.Add(timeout);
    
    while (gocxx::time::Time::Now().Before(end)) {
        auto err_result = ctx->Err();
        if (!err_result.Ok()) {
            return gocxx::base::Result<bool>(true); // Context was canceled
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    auto final_err = ctx->Err();
    return gocxx::base::Result<bool>(!final_err.Ok());
}

gocxx::base::Result<bool> WillBeCanceledSoon(ContextPtr ctx, gocxx::time::Duration within) {
    if (!ctx) {
        return gocxx::base::Result<bool>(gocxx::errors::New("context is nil"));
    }
    
    auto deadline_result = ctx->Deadline();
    if (!deadline_result.Ok()) {
        return gocxx::base::Result<bool>(false); // No deadline, won't be canceled soon
    }
    
    auto deadline = deadline_result.value;
    auto now = gocxx::time::Time::Now();
    auto remaining = deadline.Sub(now);
    
    return gocxx::base::Result<bool>(remaining.Nanoseconds() <= within.Nanoseconds());
}

} // namespace context
} // namespace gocxx

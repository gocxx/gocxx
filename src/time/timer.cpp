#include "gocxx/time/timer.h"
#include <chrono>
#include <thread>

namespace gocxx::time {

Timer::Timer(Duration d)
    : duration_(d), stopped_(false) {
    ch_ = gocxx::base::Chan<Time>::Make(1);
    start_ = std::chrono::steady_clock::now();
    thread_ = std::thread(&Timer::run, this);
}

Timer::~Timer() {
    Stop();
    if (thread_.joinable()) thread_.join();
}

bool Timer::Stop() {
    bool was_stopped = stopped_.exchange(true);
    if (!was_stopped) {
        cv_.notify_all(); // Wake up the waiting thread
    }
    return !was_stopped; // returns true if it was running
}

bool Timer::Reset(Duration d) {
    // Stop and wait for the old thread to finish
    Stop();
    if (thread_.joinable()) thread_.join();
    
    // Reset state for new timer
    stopped_ = false;
    duration_ = d;
    ch_ = gocxx::base::Chan<Time>::Make(1);
    start_ = std::chrono::steady_clock::now();
    
    // Start new thread
    thread_ = std::thread(&Timer::run, this);
    return true;
}

std::shared_ptr<gocxx::base::Chan<Time>> Timer::C() {
    return ch_;
}

void Timer::run() {
    std::unique_lock<std::mutex> lock(mutex_);
    auto deadline = start_ + std::chrono::nanoseconds(duration_.Nanoseconds());
    
    // Wait until the deadline or until stopped
    if (cv_.wait_until(lock, deadline, [this] { return stopped_.load(); })) {
        // We were stopped before the timer expired
        return;
    }

    // Timer expired normally, send the time
    if (!stopped_) {
        try {
            ch_->send(Time::Now()); // send current time
        } catch (const std::exception&) {
            // Channel might be closed, ignore
        }
    }
}

std::unique_ptr<Timer> NewTimer(Duration d) {
    return std::make_unique<Timer>(d);
}

} // namespace gocxx::time

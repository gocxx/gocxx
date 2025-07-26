#include "gocxx/time/ticker.h"
#include "gocxx/time/time.h"
#include <chrono>
#include <thread>

namespace gocxx::time {

Ticker::Ticker(Duration d)
    : duration_(d), stopped_(false)
{
    ch_ = gocxx::base::Chan<Time>::Make(0); // Unbuffered channel
    thread_ = std::thread(&Ticker::run, this);
}

Ticker::~Ticker() {
    Stop();
    if (thread_.joinable()) {
        thread_.join();
    }
}

void Ticker::Stop() {
    if (!stopped_.exchange(true)) {
        ch_->close();
        if (thread_.joinable()) {
            thread_.join();
        }
    }
}

std::shared_ptr<gocxx::base::Chan<Time>> Ticker::C() {
    return ch_;
}

void Ticker::run() {
    while (!stopped_) {
        Sleep(duration_);
        if (stopped_) break;
        
        try {
            auto t = Time::Now();
            ch_->send(std::move(t));
        } catch (const std::exception&) {
            // Channel might be closed, exit
            break;
        }
    }
}

std::unique_ptr<Ticker> NewTicker(Duration d) {
    return std::make_unique<Ticker>(d);
}

} // namespace gocxx::time

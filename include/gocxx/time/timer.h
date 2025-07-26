#pragma once
#include <gocxx/base/chan.h>
#include "time.h"
#include "duration.h"
#include <thread>
#include <atomic>
#include <memory>
#include <mutex>
#include <condition_variable>

namespace gocxx::time {

class Timer {
public:
    explicit Timer(Duration d);
    ~Timer();

    bool Stop();
    bool Reset(Duration d);
    std::shared_ptr<gocxx::base::Chan<Time>> C();

private:
    Duration duration_;
    std::chrono::steady_clock::time_point start_;
    std::thread thread_;
    std::atomic<bool> stopped_;
    std::shared_ptr<gocxx::base::Chan<Time>> ch_;
    std::mutex mutex_;
    std::condition_variable cv_;

    void run();
};

std::unique_ptr<Timer> NewTimer(Duration d);

} // namespace gocxx::time

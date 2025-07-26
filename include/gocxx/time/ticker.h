#pragma once
#include "duration.h"
#include "time.h"
#include <thread>
#include <atomic>
#include <memory>
#include <gocxx/base/chan.h>

namespace gocxx::time {

class Ticker {
public:
    explicit Ticker(Duration d);
    ~Ticker();

    void Stop();
    std::shared_ptr<gocxx::base::Chan<Time>> C();

private:
    void run();

    Duration duration_;
    std::thread thread_;
    std::atomic<bool> stopped_;
    std::shared_ptr<gocxx::base::Chan<Time>> ch_;
};

std::unique_ptr<Ticker> NewTicker(Duration d);

} // namespace gocxx::time

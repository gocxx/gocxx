#include "gocxx/time/duration.h"
#include <sstream>
#include <iomanip>
#include <cmath>

namespace gocxx::time {

    Duration::Duration() : ns_(0) {}

    Duration::Duration(int64_t ns) : ns_(ns) {}

    int64_t Duration::Nanoseconds() const {
        return ns_;
    }

    int64_t Duration::Microseconds() const {
        return ns_ / 1000;
    }

    int64_t Duration::Milliseconds() const {
        return ns_ / 1'000'000;
    }

    double Duration::Seconds() const {
        return static_cast<double>(ns_) / 1'000'000'000;
    }

    double Duration::Minutes() const {
        return Seconds() / 60.0;
    }

    double Duration::Hours() const {
        return Minutes() / 60.0;
    }

    std::chrono::nanoseconds Duration::ToStdDuration() const {
        return std::chrono::nanoseconds(ns_);
    }

    std::string DurationToString(Duration d) {
        auto total_ns = d.Nanoseconds();
        std::ostringstream oss;

        bool negative = total_ns < 0;
        if (negative) total_ns = -total_ns;

        auto hours = total_ns / 3'600'000'000'000;
        total_ns %= 3'600'000'000'000;
        auto minutes = total_ns / 60'000'000'000;
        total_ns %= 60'000'000'000;
        auto seconds = total_ns / 1'000'000'000;
        total_ns %= 1'000'000'000;
        auto milliseconds = total_ns / 1'000'000;
        total_ns %= 1'000'000;
        auto microseconds = total_ns / 1'000;
        auto nanoseconds = total_ns % 1'000;

        if (negative) oss << "-";
        if (hours) oss << hours << "h";
        if (minutes) oss << minutes << "m";
        if (seconds || (hours == 0 && minutes == 0))
            oss << seconds << "s";
        if (milliseconds) oss << milliseconds << "ms";
        if (microseconds) oss << microseconds << "us";
        if (nanoseconds) oss << nanoseconds << "ns";

        return oss.str();
    }

    std::string Duration::String() const {
        return DurationToString(*this);
    }

    // Arithmetic operators
    Duration Duration::operator+(const Duration& other) const {
        return Duration(ns_ + other.ns_);
    }

    Duration Duration::operator-(const Duration& other) const {
        return Duration(ns_ - other.ns_);
    }

    Duration Duration::operator*(int64_t n) const {
        return Duration(ns_ * n);
    }

    Duration Duration::operator/(int64_t n) const {
        return Duration(ns_ / n);
    }

    // Comparison operators
    bool Duration::operator<(const Duration& other) const {
        return ns_ < other.ns_;
    }

    bool Duration::operator<=(const Duration& other) const {
        return ns_ <= other.ns_;
    }

    bool Duration::operator>(const Duration& other) const {
        return ns_ > other.ns_;
    }

    bool Duration::operator>=(const Duration& other) const {
        return ns_ >= other.ns_;
    }

    bool Duration::operator==(const Duration& other) const {
        return ns_ == other.ns_;
    }

    bool Duration::operator!=(const Duration& other) const {
        return ns_ != other.ns_;
    }


   

} // namespace gocxx::time

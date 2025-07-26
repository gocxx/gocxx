// time.h
#pragma once
#include <cstdint>
#include <string>
#include <memory>
#include <chrono>
#include "duration.h"
#include <thread>

namespace gocxx::time {

// Represents a specific point in time
class Time {
public:
    Time();
    Time(int64_t sec, int32_t nsec);

    static Time Now();
    static Time Unix(int64_t sec, int64_t nsec);
    static Time Date(int year, int month, int day, int hour, int min, int sec, int nsec = 0);

    int64_t Unix() const;
    int64_t UnixNano() const;

    std::string String() const;
    std::string Format(const std::string& layout) const;


    Duration Sub(const Time& other) const;
    Time Add(Duration d) const;

    bool Before(const Time& other) const;
    bool After(const Time& other) const;
    bool Equal(const Time& other) const;

    Time Truncate(Duration d) const;
    Time Round(Duration d) const;

    int Year() const;
    int Month() const;
    int Day() const;
    int Hour() const;
    int Minute() const;
    int Second() const;
    int Nanosecond() const;
    int Weekday() const;
    int YearDay() const;

    bool IsZero() const;
    

private:
    int64_t sec_;
    int32_t nsec_;
};

inline void Sleep(Duration d) {
    std::this_thread::sleep_for(std::chrono::nanoseconds(d.Nanoseconds()));
}

} // namespace gocxx::time
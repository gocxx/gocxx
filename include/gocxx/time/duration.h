#pragma once
#include <cstdint>
#include <string>
#include <chrono>

namespace gocxx::time {

    class Duration {
    public:
        static const int64_t Nanosecond = 1;
        static const int64_t Microsecond = 1000 * Nanosecond;
        static const int64_t Millisecond = 1000 * Microsecond;
        static const int64_t Second = 1000 * Millisecond;
        static const int64_t Minute = 60 * Second;
        static const int64_t Hour = 60 * Minute;

        Duration();
        Duration(int64_t ns);

        int64_t Nanoseconds() const;
        int64_t Microseconds() const;
        int64_t Milliseconds() const;
        double Seconds() const;
        double Minutes() const;
        double Hours() const;

        std::string String() const;
        
        /// @brief Convert to std::chrono::nanoseconds for interop
        std::chrono::nanoseconds ToStdDuration() const;

        Duration operator+(const Duration& other) const;
        Duration operator-(const Duration& other) const;
        Duration operator*(int64_t n) const;
        Duration operator/(int64_t n) const;

        bool operator<(const Duration& other) const;
        bool operator<=(const Duration& other) const;
        bool operator>(const Duration& other) const;
        bool operator>=(const Duration& other) const;
        bool operator==(const Duration& other) const;
        bool operator!=(const Duration& other) const;

    private:
        int64_t ns_;
    };

    inline Duration Nanoseconds(int64_t ns) { return Duration(ns); }
    inline Duration Microseconds(int64_t us) { return Duration(us * Duration::Microsecond); }
    inline Duration Milliseconds(int64_t ms) { return Duration(ms * Duration::Millisecond); }
    inline Duration Seconds(int64_t s) { return Duration(s * Duration::Second); }
    inline Duration Minutes(int64_t m) { return Duration(m * Duration::Minute); }
    inline Duration Hours(int64_t h) { return Duration(h * Duration::Hour); }

    // Floating point duration constructors
    inline Duration Seconds(double s) { return Duration(static_cast<int64_t>(s * Duration::Second)); }
    inline Duration Minutes(double m) { return Duration(static_cast<int64_t>(m * Duration::Minute)); }
    inline Duration Hours(double h) { return Duration(static_cast<int64_t>(h * Duration::Hour)); }

} // namespace gocxx::time

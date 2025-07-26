#include "gocxx/time/time.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <thread>
#include <ctime>

namespace gocxx::time {

    using namespace std::chrono;

    Time::Time() : sec_(0), nsec_(0) {}

    Time::Time(int64_t sec, int32_t nsec) : sec_(sec), nsec_(nsec) {}

    Time Time::Now() {
        auto now = system_clock::now();
        auto since_epoch = now.time_since_epoch();
        int64_t sec = duration_cast<seconds>(since_epoch).count();
        int32_t nsec = static_cast<int32_t>(duration_cast<nanoseconds>(since_epoch).count() % 1'000'000'000);
        return Time(sec, nsec);
    }

    Time Time::Unix(int64_t sec, int64_t nsec) {
        return Time(sec, static_cast<int32_t>(nsec));
    }

    Time Time::Date(int year, int month, int day, int hour, int min, int sec, int nsec) {
        std::tm tm = {};
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        tm.tm_hour = hour;
        tm.tm_min = min;
        tm.tm_sec = sec;
        
        std::time_t t = std::mktime(&tm);
        int64_t seconds = static_cast<int64_t>(t);
        return Time(seconds, static_cast<int32_t>(nsec));
    }


    int64_t Time::Unix() const {
        return sec_;
    }

    int64_t Time::UnixNano() const {
        return sec_ * 1'000'000'000LL + nsec_;
    }

    std::string Time::String() const {
        std::time_t t = static_cast<std::time_t>(sec_);
        std::tm local_tm{};

#ifdef _WIN32
        localtime_s(&local_tm, &t);
#else
        localtime_r(&t, &local_tm);
#endif

        char buf[64];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &local_tm);
        return std::string(buf);
    }


    std::string Time::Format(const std::string& layout) const {
        std::time_t t = static_cast<std::time_t>(sec_);
        std::tm local_tm{};

#ifdef _WIN32
        localtime_s(&local_tm, &t);
#else
        localtime_r(&t, &local_tm);
#endif

        std::ostringstream oss;
        // Simple format support - can be extended later
        if (layout == "2006-01-02 15:04:05") {
            oss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
        } else if (layout == "2006-01-02") {
            oss << std::put_time(&local_tm, "%Y-%m-%d");
        } else if (layout == "15:04:05") {
            oss << std::put_time(&local_tm, "%H:%M:%S");
        } else {
            // Fallback to standard format
            oss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
        }
        return oss.str();
    }

    Duration Time::Sub(const Time& other) const {
        return Duration(UnixNano() - other.UnixNano());
    }

    Time Time::Add(Duration d) const {
        int64_t ns = UnixNano() + d.Nanoseconds();
        return Time(ns / 1'000'000'000, ns % 1'000'000'000);
    }

    bool Time::Before(const Time& other) const {
        return UnixNano() < other.UnixNano();
    }

    bool Time::After(const Time& other) const {
        return UnixNano() > other.UnixNano();
    }

    bool Time::Equal(const Time& other) const {
        return UnixNano() == other.UnixNano();
    }

    Time Time::Truncate(Duration d) const {
        int64_t ns = UnixNano();
        int64_t mod = ns % d.Nanoseconds();
        return Time::Unix(0, 0).Add(Duration(ns - mod));
    }

    Time Time::Round(Duration d) const {
        int64_t ns = UnixNano();
        int64_t round = (ns + d.Nanoseconds() / 2) / d.Nanoseconds() * d.Nanoseconds();
        return Time::Unix(0, 0).Add(Duration(round));
    }

    int Time::Year() const {
        std::time_t t = static_cast<std::time_t>(sec_);
        std::tm local_tm{};
#ifdef _WIN32
        localtime_s(&local_tm, &t);
#else
        localtime_r(&t, &local_tm);
#endif
        return local_tm.tm_year + 1900;
    }

    int Time::Month() const {
        std::time_t t = static_cast<std::time_t>(sec_);
        std::tm local_tm{};
#ifdef _WIN32
        localtime_s(&local_tm, &t);
#else
        localtime_r(&t, &local_tm);
#endif
        return local_tm.tm_mon + 1;
    }

    int Time::Day() const {
        std::time_t t = static_cast<std::time_t>(sec_);
        std::tm local_tm{};
#ifdef _WIN32
        localtime_s(&local_tm, &t);
#else
        localtime_r(&t, &local_tm);
#endif
        return local_tm.tm_mday;
    }

    int Time::Hour() const {
        std::time_t t = static_cast<std::time_t>(sec_);
        std::tm local_tm{};
#ifdef _WIN32
        localtime_s(&local_tm, &t);
#else
        localtime_r(&t, &local_tm);
#endif
        return local_tm.tm_hour;
    }

    int Time::Minute() const {
        std::time_t t = static_cast<std::time_t>(sec_);
        std::tm local_tm{};
#ifdef _WIN32
        localtime_s(&local_tm, &t);
#else
        localtime_r(&t, &local_tm);
#endif
        return local_tm.tm_min;
    }

    int Time::Second() const {
        std::time_t t = static_cast<std::time_t>(sec_);
        std::tm local_tm{};
#ifdef _WIN32
        localtime_s(&local_tm, &t);
#else
        localtime_r(&t, &local_tm);
#endif
        return local_tm.tm_sec;
    }

    int Time::Nanosecond() const {
        return nsec_;
    }

    int Time::Weekday() const {
        std::time_t t = static_cast<std::time_t>(sec_);
        std::tm local_tm{};
#ifdef _WIN32
        localtime_s(&local_tm, &t);
#else
        localtime_r(&t, &local_tm);
#endif
        return local_tm.tm_wday; // 0 = Sunday, 1 = Monday, etc.
    }

    int Time::YearDay() const {
        std::time_t t = static_cast<std::time_t>(sec_);
        std::tm local_tm{};
#ifdef _WIN32
        localtime_s(&local_tm, &t);
#else
        localtime_r(&t, &local_tm);
#endif
        return local_tm.tm_yday + 1; // tm_yday is 0-based, we want 1-based
    }

    bool Time::IsZero() const {
        return sec_ == 0 && nsec_ == 0;
    }

} // namespace gocxx::time

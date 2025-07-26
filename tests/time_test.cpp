#include <gtest/gtest.h>
#include <gocxx/time/time.h>
#include <gocxx/time/duration.h>

TEST(TimeTest, NowIsNotZero) {
    gocxx::time::Time now = gocxx::time::Time::Now();

	std::cout << "Current time: " << now.String() << std::endl;

    EXPECT_FALSE(now.IsZero());
}

TEST(SleepTest, SleepsApproximatelyCorrectDuration) {
    auto start = std::chrono::high_resolution_clock::now();
    gocxx::time::Sleep(gocxx::time::Duration(200 * gocxx::time::Duration::Millisecond));
    auto end = std::chrono::high_resolution_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    EXPECT_GE(elapsed, 180);
    EXPECT_LE(elapsed, 250);
}

TEST(TimeTest, NowIsCloseToSystemClock) {
    auto sys_now = std::chrono::system_clock::now().time_since_epoch();
    auto sys_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(sys_now).count();

    gocxx::time::Time t = gocxx::time::Time::Now();
    int64_t t_ns = t.UnixNano();

    int64_t diff = std::abs(t_ns - sys_ns);

    EXPECT_LT(diff, 5'000'000); // within 5ms
}

TEST(TimeTest, UnixConstructionAndAccessors) {
    gocxx::time::Time t = gocxx::time::Time::Unix(1620000000, 123456789);
    EXPECT_EQ(t.Unix(), 1620000000);
    EXPECT_EQ(t.UnixNano(), 1620000000'000'000'000 + 123456789);
}

TEST(TimeTest, DateConstruction) {
    gocxx::time::Time t = gocxx::time::Time::Date(2023, 5, 7, 12, 34, 56, 789);
    EXPECT_EQ(t.Year(), 2023);
    EXPECT_EQ(t.Month(), 5);
    EXPECT_EQ(t.Day(), 7);
    EXPECT_EQ(t.Hour(), 12);
    EXPECT_EQ(t.Minute(), 34);
    EXPECT_EQ(t.Second(), 56);
    EXPECT_EQ(t.Nanosecond(), 789);
}

TEST(TimeTest, ComparisonOperators) {
    gocxx::time::Time a = gocxx::time::Time::Unix(100, 0);
    gocxx::time::Time b = gocxx::time::Time::Unix(200, 0);

    EXPECT_TRUE(a.Before(b));
    EXPECT_TRUE(b.After(a));
    EXPECT_FALSE(a.Equal(b));
    EXPECT_TRUE(a.Equal(gocxx::time::Time::Unix(100, 0)));
}

TEST(TimeTest, AddAndSubDuration) {
    gocxx::time::Time a = gocxx::time::Time::Unix(1, 500000000); // 1.5s
    gocxx::time::Duration d = gocxx::time::Duration(1'500'000'000); // 1.5s

    gocxx::time::Time b = a.Add(d); // Should be 3.0s
    EXPECT_EQ(b.Unix(), 3);
    EXPECT_EQ(b.Nanosecond(), 0);

    gocxx::time::Duration delta = b.Sub(a);
    EXPECT_EQ(delta.Seconds(), 1.5);
}

TEST(TimeTest, IsZeroWorks) {
    gocxx::time::Time zero;
    EXPECT_TRUE(zero.IsZero());

    gocxx::time::Time t = gocxx::time::Time::Now();
    EXPECT_FALSE(t.IsZero());
}

TEST(TimeTest, StringFormatNotEmpty) {
    gocxx::time::Time t = gocxx::time::Time::Now();
    EXPECT_FALSE(t.String().empty());
    EXPECT_FALSE(t.Format("2006-01-02 15:04:05").empty());
}

TEST(TimeTest, TruncateRoundsDown) {
    gocxx::time::Time t = gocxx::time::Time::Unix(1234, 987654321);
    gocxx::time::Time truncated = t.Truncate(gocxx::time::Duration::Second);

    EXPECT_EQ(truncated.Unix(), 1234);
    EXPECT_EQ(truncated.Nanosecond(), 0);
}

TEST(TimeTest, RoundRoundsNearest) {
    gocxx::time::Time t = gocxx::time::Time::Unix(1234, 1'600'000'000); 
    gocxx::time::Time rounded = t.Round(gocxx::time::Duration::Second);

    EXPECT_EQ(rounded.Unix(), 1236);
    EXPECT_EQ(rounded.Nanosecond(), 0);
}
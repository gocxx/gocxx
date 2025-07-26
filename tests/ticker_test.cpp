#include <gtest/gtest.h>
#include "gocxx/time/ticker.h"
#include "gocxx/time/timer.h"
#include "gocxx/time/time.h"
#include <chrono>
#include <thread>

using namespace gocxx::time;
using namespace std::chrono;

class DurationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST(TickerTest, BasicTicking) {
    Duration interval(100 * Duration::Millisecond); // 100 milliseconds
    auto ticker = NewTicker(interval);

    auto ch = ticker->C();
    auto t1 = ch->recv();
    auto t2 = ch->recv();

    ticker->Stop();

    ASSERT_TRUE(t1.has_value());
    ASSERT_TRUE(t2.has_value());

    Duration elapsed = t2->Sub(*t1);
    EXPECT_GE(elapsed.Nanoseconds(), interval.Nanoseconds() - 50'000'000); // 50ms tolerance
}

TEST(TickerTest, TicksAtExpectedIntervals) {
    Duration tickDuration(100 * Duration::Millisecond); // 100ms
    auto ticker = NewTicker(tickDuration);

    auto ch = ticker->C();
    ASSERT_NE(ch, nullptr);

    auto t1 = ch->recv(); // block until first tick
    auto t2 = ch->recv(); // block until second tick

    ASSERT_TRUE(t1.has_value());
    ASSERT_TRUE(t2.has_value());

    auto elapsed = t2->Sub(*t1).Milliseconds();
    EXPECT_GE(elapsed, 80);  // Allow some system delay
    EXPECT_LE(elapsed, 200); // Allow some jitter

    ticker->Stop();
}

TEST(TickerTest, StopsCorrectly) {
    Duration tickDuration(50 * Duration::Millisecond); // 50ms
    auto ticker = NewTicker(tickDuration);

    auto ch = ticker->C();
    ASSERT_NE(ch, nullptr);

    auto firstTick = ch->recv();
    ASSERT_TRUE(firstTick.has_value());
    
    ticker->Stop();

    // After stopping, channel should be closed
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Channel should be closed after stopping
    EXPECT_TRUE(ch->isClosed());
}

TEST(TimerTest, BasicTimer) {
    Duration delay(100 * Duration::Millisecond);
    auto timer = NewTimer(delay);
    
    auto start = Time::Now();
    auto ch = timer->C();
    auto result = ch->recv();
    auto elapsed = Time::Now().Sub(start);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_GE(elapsed.Milliseconds(), 80);  // Allow some tolerance
    EXPECT_LE(elapsed.Milliseconds(), 200);
}

TEST(TimerTest, TimerStop) {
    Duration delay(200 * Duration::Millisecond);
    auto timer = NewTimer(delay);
    
    // Stop the timer immediately
    bool wasStopped = timer->Stop();
    EXPECT_TRUE(wasStopped);
    
    // Channel should not have any value after stopping
    // Since timer was stopped before delay, no time should be sent
}

TEST(TimerTest, TimerReset) {
    // Use longer durations to avoid timing sensitivity
    Duration delay1(300 * Duration::Millisecond);
    Duration delay2(100 * Duration::Millisecond);
    
    auto timer = NewTimer(delay1);
    
    // Reset to shorter duration
    auto start = Time::Now();
    timer->Reset(delay2);
    
    // Get the channel AFTER reset since Reset creates a new channel
    auto ch = timer->C();
    auto result = ch->recv();
    auto elapsed = Time::Now().Sub(start);
    
    ASSERT_TRUE(result.has_value());
    
    // The key test is that it's closer to delay2 (100ms) than delay1 (300ms)
    // Allow generous tolerance for system scheduling overhead
    EXPECT_LE(elapsed.Milliseconds(), 250); // Should be much less than 300ms
    EXPECT_GE(elapsed.Milliseconds(), 80);  // Should be at least close to 100ms
}

TEST_F(DurationTest, ArithmeticOperations) {
    Duration d1(Duration::Second);
    Duration d2(500 * Duration::Millisecond);
    
    auto sum = d1 + d2;
    EXPECT_EQ(sum.Milliseconds(), 1500);
    
    auto diff = d1 - d2;
    EXPECT_EQ(diff.Milliseconds(), 500);
    
    auto mult = d2 * 3;
    EXPECT_EQ(mult.Milliseconds(), 1500);
    
    auto div = d1 / 2;
    EXPECT_EQ(div.Milliseconds(), 500);
}

TEST_F(DurationTest, Comparisons) {
    Duration d1(Duration::Second);
    Duration d2(2 * Duration::Second);
    Duration d3(Duration::Second);
    
    EXPECT_TRUE(d1 < d2);
    EXPECT_TRUE(d1 <= d2);
    EXPECT_TRUE(d1 <= d3);
    EXPECT_TRUE(d2 > d1);
    EXPECT_TRUE(d2 >= d1);
    EXPECT_TRUE(d1 >= d3);
    EXPECT_TRUE(d1 == d3);
    EXPECT_TRUE(d1 != d2);
}

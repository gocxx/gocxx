#include <gtest/gtest.h>
#include <sstream>
#include <limits>
#include <chrono>
#include <gocxx/time/duration.h>

using namespace gocxx::time;

class DurationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common test setup if needed
    }
    
    void TearDown() override {
        // Common test cleanup if needed
    }
};

// Test Duration constants
TEST_F(DurationTest, Constants) {
    EXPECT_EQ(Duration::Nanosecond, 1);
    EXPECT_EQ(Duration::Microsecond, 1000);
    EXPECT_EQ(Duration::Millisecond, 1000000);
    EXPECT_EQ(Duration::Second, 1000000000);
    EXPECT_EQ(Duration::Minute, 60000000000);
    EXPECT_EQ(Duration::Hour, 3600000000000);
}

// Test constructors
TEST_F(DurationTest, DefaultConstructor) {
    Duration d;
    EXPECT_EQ(d.Nanoseconds(), 0);
}

TEST_F(DurationTest, ExplicitConstructor) {
    Duration d(1000);
    EXPECT_EQ(d.Nanoseconds(), 1000);
}

TEST_F(DurationTest, ConstructorWithNegativeValue) {
    Duration d(-500);
    EXPECT_EQ(d.Nanoseconds(), -500);
}

// Test time unit conversions
TEST_F(DurationTest, NanosecondsConversion) {
    Duration d(123456789);
    EXPECT_EQ(d.Nanoseconds(), 123456789);
}

TEST_F(DurationTest, MicrosecondsConversion) {
    Duration d(5000000); // 5 milliseconds
    EXPECT_EQ(d.Microseconds(), 5000);
}

TEST_F(DurationTest, MillisecondsConversion) {
    Duration d(5000000000); // 5 seconds
    EXPECT_EQ(d.Milliseconds(), 5000);
}

TEST_F(DurationTest, SecondsConversion) {
    Duration d(Duration::Second * 5);
    EXPECT_DOUBLE_EQ(d.Seconds(), 5.0);
    
    Duration d2(Duration::Second + Duration::Millisecond * 500);
    EXPECT_DOUBLE_EQ(d2.Seconds(), 1.5);
}

TEST_F(DurationTest, MinutesConversion) {
    Duration d(Duration::Minute * 3);
    EXPECT_DOUBLE_EQ(d.Minutes(), 3.0);
    
    Duration d2(Duration::Minute + Duration::Second * 30);
    EXPECT_DOUBLE_EQ(d2.Minutes(), 1.5);
}

TEST_F(DurationTest, HoursConversion) {
    Duration d(Duration::Hour * 2);
    EXPECT_DOUBLE_EQ(d.Hours(), 2.0);
    
    Duration d2(Duration::Hour + Duration::Minute * 30);
    EXPECT_DOUBLE_EQ(d2.Hours(), 1.5);
}

// Test fractional conversions
TEST_F(DurationTest, FractionalConversions) {
    Duration d(Duration::Microsecond * 1500); // 1.5 milliseconds
    EXPECT_DOUBLE_EQ(d.Seconds(), 0.0015);
    EXPECT_DOUBLE_EQ(d.Minutes(), 0.000025);
    EXPECT_NEAR(d.Hours(), 0.000000416666667, 1e-12); // approximately
}

// Test arithmetic operators
TEST_F(DurationTest, AdditionOperator) {
    Duration d1(Duration::Second);
    Duration d2(Duration::Millisecond * 500);
    Duration result = d1 + d2;
    
    EXPECT_EQ(result.Nanoseconds(), Duration::Second + Duration::Millisecond * 500);
    EXPECT_DOUBLE_EQ(result.Seconds(), 1.5);
}

TEST_F(DurationTest, SubtractionOperator) {
    Duration d1(Duration::Second * 2);
    Duration d2(Duration::Millisecond * 500);
    Duration result = d1 - d2;
    
    EXPECT_EQ(result.Nanoseconds(), Duration::Second * 2 - Duration::Millisecond * 500);
    EXPECT_DOUBLE_EQ(result.Seconds(), 1.5);
}

TEST_F(DurationTest, MultiplicationOperator) {
    Duration d(Duration::Second);
    Duration result = d * 3;
    
    EXPECT_EQ(result.Nanoseconds(), Duration::Second * 3);
    EXPECT_DOUBLE_EQ(result.Seconds(), 3.0);
}

TEST_F(DurationTest, DivisionOperator) {
    Duration d(Duration::Second * 6);
    Duration result = d / 3;
    
    EXPECT_EQ(result.Nanoseconds(), Duration::Second * 2);
    EXPECT_DOUBLE_EQ(result.Seconds(), 2.0);
}

TEST_F(DurationTest, ArithmeticWithNegativeValues) {
    Duration d1(Duration::Second);
    Duration d2(-Duration::Millisecond * 500);
    Duration result = d1 + d2;
    
    EXPECT_EQ(result.Nanoseconds(), Duration::Second - Duration::Millisecond * 500);
    EXPECT_DOUBLE_EQ(result.Seconds(), 0.5);
}

// Test comparison operators
TEST_F(DurationTest, EqualityOperators) {
    Duration d1(Duration::Second);
    Duration d2(Duration::Second);
    Duration d3(Duration::Millisecond * 500);
    
    EXPECT_TRUE(d1 == d2);
    EXPECT_FALSE(d1 == d3);
    EXPECT_FALSE(d1 != d2);
    EXPECT_TRUE(d1 != d3);
}

TEST_F(DurationTest, ComparisonOperators) {
    Duration small(Duration::Millisecond * 500);
    Duration large(Duration::Second);
    
    EXPECT_TRUE(small < large);
    EXPECT_TRUE(small <= large);
    EXPECT_FALSE(small > large);
    EXPECT_FALSE(small >= large);
    
    EXPECT_FALSE(large < small);
    EXPECT_FALSE(large <= small);
    EXPECT_TRUE(large > small);
    EXPECT_TRUE(large >= small);
}

TEST_F(DurationTest, ComparisonWithNegativeValues) {
    Duration negative(-Duration::Second);
    Duration positive(Duration::Second);
    Duration zero(0);
    
    EXPECT_TRUE(negative < zero);
    EXPECT_TRUE(negative < positive);
    EXPECT_TRUE(zero < positive);
    EXPECT_TRUE(negative <= zero);
    EXPECT_TRUE(negative <= positive);
    
    EXPECT_FALSE(positive < negative);
    EXPECT_FALSE(zero < negative);
    EXPECT_TRUE(positive > negative);
    EXPECT_TRUE(zero > negative);
}

TEST_F(DurationTest, ComparisonEdgeCases) {
    Duration d1(Duration::Second);
    Duration d2(Duration::Second);
    
    EXPECT_TRUE(d1 <= d2);
    EXPECT_TRUE(d1 >= d2);
    EXPECT_FALSE(d1 < d2);
    EXPECT_FALSE(d1 > d2);
}

// Test String representation
TEST_F(DurationTest, StringRepresentationZero) {
    Duration d(0);
    EXPECT_EQ(d.String(), "0s");
}

TEST_F(DurationTest, StringRepresentationPositive) {
    Duration d(Duration::Hour + Duration::Minute * 30 + Duration::Second * 45);
    std::string result = d.String();
    
    // Should contain hours, minutes, and seconds
    EXPECT_NE(result.find("1h"), std::string::npos);
    EXPECT_NE(result.find("30m"), std::string::npos);
    EXPECT_NE(result.find("45s"), std::string::npos);
}

TEST_F(DurationTest, StringRepresentationNegative) {
    Duration d(-Duration::Second);
    std::string result = d.String();
    
    EXPECT_EQ(result[0], '-');
    EXPECT_NE(result.find("1s"), std::string::npos);
}

TEST_F(DurationTest, StringRepresentationMilliseconds) {
    Duration d(Duration::Millisecond * 250);
    std::string result = d.String();
    
    EXPECT_NE(result.find("250ms"), std::string::npos);
}

TEST_F(DurationTest, StringRepresentationMicroseconds) {
    Duration d(Duration::Microsecond * 750);
    std::string result = d.String();
    
    EXPECT_NE(result.find("750us"), std::string::npos);
}

TEST_F(DurationTest, StringRepresentationNanoseconds) {
    Duration d(123);
    std::string result = d.String();
    
    EXPECT_NE(result.find("123ns"), std::string::npos);
}

TEST_F(DurationTest, StringRepresentationComplex) {
    Duration d(Duration::Hour * 2 + Duration::Minute * 15 + Duration::Second * 30 + 
              Duration::Millisecond * 123 + Duration::Microsecond * 456 + 789);
    std::string result = d.String();
    
    EXPECT_NE(result.find("2h"), std::string::npos);
    EXPECT_NE(result.find("15m"), std::string::npos);
    EXPECT_NE(result.find("30s"), std::string::npos);
    EXPECT_NE(result.find("123ms"), std::string::npos);
    EXPECT_NE(result.find("456us"), std::string::npos);
    EXPECT_NE(result.find("789ns"), std::string::npos);
}

// Test edge cases
TEST_F(DurationTest, MaxValue) {
    Duration d(std::numeric_limits<int64_t>::max());
    EXPECT_EQ(d.Nanoseconds(), std::numeric_limits<int64_t>::max());
}

TEST_F(DurationTest, MinValue) {
    Duration d(std::numeric_limits<int64_t>::min());
    EXPECT_EQ(d.Nanoseconds(), std::numeric_limits<int64_t>::min());
}

TEST_F(DurationTest, ZeroValue) {
    Duration d(0);
    EXPECT_EQ(d.Nanoseconds(), 0);
    EXPECT_EQ(d.Microseconds(), 0);
    EXPECT_EQ(d.Milliseconds(), 0);
    EXPECT_DOUBLE_EQ(d.Seconds(), 0.0);
    EXPECT_DOUBLE_EQ(d.Minutes(), 0.0);
    EXPECT_DOUBLE_EQ(d.Hours(), 0.0);
}

// Test precision
TEST_F(DurationTest, PrecisionTest) {
    Duration d(1); // 1 nanosecond
    EXPECT_EQ(d.Nanoseconds(), 1);
    EXPECT_EQ(d.Microseconds(), 0); // Should truncate
    EXPECT_EQ(d.Milliseconds(), 0); // Should truncate
    EXPECT_DOUBLE_EQ(d.Seconds(), 1e-9);
}

// Test large values
TEST_F(DurationTest, LargeValues) {
    Duration d(Duration::Hour * 24 * 365); // One year
    EXPECT_DOUBLE_EQ(d.Hours(), 24.0 * 365.0);
    EXPECT_DOUBLE_EQ(d.Minutes(), 24.0 * 365.0 * 60.0);
}

// Test chaining operations
TEST_F(DurationTest, ChainedOperations) {
    Duration d1(Duration::Second);
    Duration d2(Duration::Millisecond * 500);
    Duration d3(Duration::Microsecond * 250);
    
    Duration result = d1 + d2 - d3;
    int64_t expected = Duration::Second + Duration::Millisecond * 500 - Duration::Microsecond * 250;
    
    EXPECT_EQ(result.Nanoseconds(), expected);
}

// Test operator precedence
TEST_F(DurationTest, OperatorPrecedence) {
    Duration d(Duration::Second * 2);
    Duration result = d * 3 + Duration::Millisecond * 500;
    
    Duration expected = Duration(Duration::Second * 6 + Duration::Millisecond * 500);
    EXPECT_EQ(result.Nanoseconds(), expected.Nanoseconds());
}

// Performance test (optional)
TEST_F(DurationTest, PerformanceTest) {
    const int iterations = 1000000;
    Duration d1(Duration::Second);
    Duration d2(Duration::Millisecond * 500);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        Duration result = d1 + d2;
        (void)result; 
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Duration operations per second: " 
              << (iterations * 1000000.0) / duration.count() << std::endl;
}

// Test copy semantics
TEST_F(DurationTest, CopySemantics) {
    Duration original(Duration::Second * 5);
    Duration copy = original;
    
    EXPECT_EQ(original.Nanoseconds(), copy.Nanoseconds());

    copy = copy + Duration::Second;
    EXPECT_EQ(original.Nanoseconds(), Duration::Second * 5);
    EXPECT_EQ(copy.Nanoseconds(), Duration::Second * 6);
}

// Test assignment
TEST_F(DurationTest, Assignment) {
    Duration d1(Duration::Second);
    Duration d2(Duration::Millisecond * 500);
    
    d1 = d2;
    EXPECT_EQ(d1.Nanoseconds(), d2.Nanoseconds());
}

// Integration test
TEST_F(DurationTest, IntegrationTest) {
    Duration timeout(Duration::Second * 30);
    Duration elapsed(Duration::Millisecond * 15750); 
    Duration remaining = timeout - elapsed;
    
    EXPECT_DOUBLE_EQ(remaining.Seconds(), 14.25);
    EXPECT_TRUE(remaining > Duration(0));
    EXPECT_TRUE(elapsed < timeout);
}

// Test with various time units
TEST_F(DurationTest, TimeUnitConsistency) {
    Duration d(Duration::Hour + Duration::Minute * 30 + Duration::Second * 45 + 
              Duration::Millisecond * 123);
    
    int64_t total_ns = d.Nanoseconds();
    double total_seconds = d.Seconds();
    
    Duration from_ns(total_ns);
    Duration from_seconds(static_cast<int64_t>(total_seconds * Duration::Second));
    
    EXPECT_EQ(from_ns.Nanoseconds(), total_ns);
}

// Test boundary conditions
TEST_F(DurationTest, BoundaryConditions) {
    Duration d1(1);
    Duration d2(-1);
    
    EXPECT_TRUE(d1 > d2);
    EXPECT_TRUE(d2 < d1);
    EXPECT_FALSE(d1 == d2);
    
    Duration zero(0);
    EXPECT_TRUE(d1 > zero);
    EXPECT_TRUE(zero > d2);
}
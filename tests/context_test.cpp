/**
 * @file context_test_fixed.cpp
 * @brief Tests for Go-exact context functionality with correct Result API usage
 */

#include <gtest/gtest.h>
#include <gocxx/context/context.h>
#include <gocxx/time/time.h>
#include <thread>
#include <chrono>

using namespace gocxx::context;
using namespace gocxx::time;
using namespace gocxx::base;

class ContextTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test fixtures if needed
    }
    
    void TearDown() override {
        // Cleanup after tests
    }
};

// Test Background context - exact Go behavior
TEST_F(ContextTest, BackgroundContext) {
    auto ctx = Background();
    ASSERT_TRUE(ctx != nullptr);
    
    // Background should have no deadline
    auto deadline = ctx->Deadline();
    EXPECT_FALSE(deadline.Ok());
    EXPECT_EQ(deadline.err->error(), "no deadline");
    
    // Background should never return error
    auto err = ctx->Err();
    EXPECT_TRUE(err.Ok());
    
    // Background should not find any values
    auto value = ctx->Value(std::string("test"));
    EXPECT_FALSE(value.Ok());
    EXPECT_EQ(value.err->error(), "key not found");
}

// Test TODO context - exact Go behavior
TEST_F(ContextTest, TODOContext) {
    auto ctx = TODO();
    ASSERT_TRUE(ctx != nullptr);
    
    // TODO should behave like Background
    auto deadline = ctx->Deadline();
    EXPECT_FALSE(deadline.Ok());
    
    auto err = ctx->Err();
    EXPECT_TRUE(err.Ok());
    
    auto value = ctx->Value(std::string("test"));
    EXPECT_FALSE(value.Ok());
}

// Test WithCancel - exact Go behavior
TEST_F(ContextTest, WithCancel) {
    auto parent = Background();
    auto result = WithCancel(parent);
    
    ASSERT_TRUE(result.Ok());
    auto [ctx, cancel] = result.value;
    
    // Should not be canceled initially
    auto err = ctx->Err();
    EXPECT_TRUE(err.Ok());
    
    // Cancel the context
    cancel();
    
    // Should be canceled now
    err = ctx->Err();
    EXPECT_FALSE(err.Ok());
    EXPECT_EQ(err.err->error(), Canceled);
}

// Test WithTimeout - exact Go behavior
TEST_F(ContextTest, WithTimeout) {
    auto parent = Background();
    auto timeout = Milliseconds(100);
    auto result = WithTimeout(parent, timeout);
    
    ASSERT_TRUE(result.Ok());
    auto [ctx, cancel] = result.value;
    
    // Should have a deadline
    auto deadline = ctx->Deadline();
    EXPECT_TRUE(deadline.Ok());
    
    // Should not be canceled initially
    auto err = ctx->Err();
    EXPECT_TRUE(err.Ok());
    
    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    
    // Should be canceled due to timeout
    err = ctx->Err();
    EXPECT_FALSE(err.Ok());
    EXPECT_EQ(err.err->error(), DeadlineExceeded);
    
    cancel(); // Cleanup
}

// Test WithDeadline - exact Go behavior
TEST_F(ContextTest, WithDeadline) {
    auto parent = Background();
    auto deadline = Time::Now().Add(Milliseconds(100));
    auto result = WithDeadline(parent, deadline);
    
    ASSERT_TRUE(result.Ok());
    auto [ctx, cancel] = result.value;
    
    // Should have the correct deadline
    auto ctx_deadline = ctx->Deadline();
    EXPECT_TRUE(ctx_deadline.Ok());
    
    // Deadline should be approximately equal (within some tolerance)
    auto diff = ctx_deadline.value.Sub(deadline);
    EXPECT_LT(std::abs(diff.Nanoseconds()), 1000000); // Within 1ms
    
    cancel(); // Cleanup
}

// Test WithValue - exact Go behavior
TEST_F(ContextTest, WithValue) {
    auto parent = Background();
    std::string key = "user_id";
    std::string value = "12345";
    
    auto result = WithValue(parent, std::any(key), std::any(value));
    ASSERT_TRUE(result.Ok());
    auto ctx = result.value;
    
    // Should find the value with correct key
    auto found = ctx->Value(std::any(key));
    ASSERT_TRUE(found.Ok());
    
    auto found_value = std::any_cast<std::string>(found.value);
    EXPECT_EQ(found_value, value);
    
    // Should not find value with different key
    auto not_found = ctx->Value(std::any(std::string("other_key")));
    EXPECT_FALSE(not_found.Ok());
    EXPECT_EQ(not_found.err->error(), "key not found");
}

// Test context value propagation
TEST_F(ContextTest, ValuePropagation) {
    auto parent = Background();
    
    // Add value to parent
    auto result1 = WithValue(parent, std::any(std::string("key1")), std::any(std::string("value1")));
    ASSERT_TRUE(result1.Ok());
    auto ctx1 = result1.value;
    
    // Add another value to child
    auto result2 = WithValue(ctx1, std::any(std::string("key2")), std::any(std::string("value2")));
    ASSERT_TRUE(result2.Ok());
    auto ctx2 = result2.value;
    
    // Child should find both values
    auto value1 = ctx2->Value(std::any(std::string("key1")));
    EXPECT_TRUE(value1.Ok());
    EXPECT_EQ(std::any_cast<std::string>(value1.value), "value1");
    
    auto value2 = ctx2->Value(std::any(std::string("key2")));
    EXPECT_TRUE(value2.Ok());
    EXPECT_EQ(std::any_cast<std::string>(value2.value), "value2");
    
    // Parent should only find its value
    auto parent_value1 = ctx1->Value(std::any(std::string("key1")));
    EXPECT_TRUE(parent_value1.Ok());
    
    auto parent_value2 = ctx1->Value(std::any(std::string("key2")));
    EXPECT_FALSE(parent_value2.Ok());
}

// Test cancellation propagation
TEST_F(ContextTest, CancellationPropagation) {
    auto parent = Background();
    
    // Create parent cancelable context
    auto parent_result = WithCancel(parent);
    ASSERT_TRUE(parent_result.Ok());
    auto [parent_ctx, parent_cancel] = parent_result.value;
    
    // Create child context
    auto child_result = WithCancel(parent_ctx);
    ASSERT_TRUE(child_result.Ok());
    auto [child_ctx, child_cancel] = child_result.value;
    
    // Neither should be canceled initially
    EXPECT_TRUE(parent_ctx->Err().Ok());
    EXPECT_TRUE(child_ctx->Err().Ok());
    
    // Cancel parent
    parent_cancel();
    
    // Both should be canceled
    EXPECT_FALSE(parent_ctx->Err().Ok());
    EXPECT_FALSE(child_ctx->Err().Ok());
    
    child_cancel(); // Cleanup
}

// Test nil parent error handling
TEST_F(ContextTest, NilParentHandling) {
    // Test WithCancel with nil parent
    auto cancel_result = WithCancel(nullptr);
    EXPECT_FALSE(cancel_result.Ok());
    EXPECT_EQ(cancel_result.err->error(), "parent context is nil");
    
    // Test WithTimeout with nil parent  
    auto timeout_result = WithTimeout(nullptr, Seconds(int64_t(1)));
    EXPECT_FALSE(timeout_result.Ok());
    EXPECT_EQ(timeout_result.err->error(), "parent context is nil");
    
    // Test WithDeadline with nil parent
    auto deadline_result = WithDeadline(nullptr, Time::Now().Add(Seconds(int64_t(1))));
    EXPECT_FALSE(deadline_result.Ok());
    EXPECT_EQ(deadline_result.err->error(), "parent context is nil");
    
    // Test WithValue with nil parent
    auto value_result = WithValue(nullptr, std::any(std::string("key")), std::any(std::string("value")));
    EXPECT_FALSE(value_result.Ok());
    EXPECT_EQ(value_result.err->error(), "parent context is nil");
}

// Test utility functions
TEST_F(ContextTest, UtilityFunctions) {
    auto parent = Background();
    
    // Test SleepWithContext with no context
    auto sleep_result = SleepWithContext(nullptr, Milliseconds(10));
    EXPECT_TRUE(sleep_result.Ok());
    
    // Test SleepWithContext with canceled context
    auto cancel_result = WithCancel(parent);
    ASSERT_TRUE(cancel_result.Ok());
    auto [ctx, cancel] = cancel_result.value;
    
    cancel(); // Cancel immediately
    
    auto sleep_canceled = SleepWithContext(ctx, Milliseconds(100));
    EXPECT_FALSE(sleep_canceled.Ok());
    EXPECT_EQ(sleep_canceled.err->error(), "context canceled during sleep");
    
    // Test WaitForContext
    auto wait_result = WaitForContext(ctx, Milliseconds(10));
    EXPECT_TRUE(wait_result.Ok());
    EXPECT_TRUE(wait_result.value); // Should return true (context was canceled)
}

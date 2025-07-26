/**
 * @file context_example.cpp
 * @brief Example demonstrating gocxx context usage for cancellation and timeouts
 */

#include <gocxx/gocxx.h>
#include <iostream>
#include <thread>
#include <chrono>

using namespace gocxx::context;
using namespace std::chrono_literals;

// Simulate a long-running operation that respects context cancellation
bool longRunningOperation(std::shared_ptr<Context> ctx, const std::string& name) {
    std::cout << "[" << name << "] Starting long operation..." << std::endl;
    
    for (int i = 0; i < 10; ++i) {
        // Check for cancellation before each iteration
        if (ctx->IsCanceled()) {
            auto err = ctx->Err();
            std::cout << "[" << name << "] Operation canceled: " << err.value_or("unknown") << std::endl;
            return false;
        }
        
        std::cout << "[" << name << "] Working... step " << (i + 1) << "/10" << std::endl;
        
        // Simulate work with context-aware sleep
        if (!SleepWithContext(ctx, 500ms)) {
            std::cout << "[" << name << "] Sleep interrupted by cancellation" << std::endl;
            return false;
        }
    }
    
    std::cout << "[" << name << "] Operation completed successfully!" << std::endl;
    return true;
}

// Example of passing context with values
void processRequest(std::shared_ptr<Context> ctx) {
    // Extract request ID from context
    auto request_id = ctx->Value(std::string("request_id"));
    std::string id = request_id ? std::any_cast<std::string>(request_id.value()) : "unknown";
    
    std::cout << "Processing request: " << id << std::endl;
    
    // Check if context has a deadline
    auto deadline = ctx->Deadline();
    if (deadline) {
        auto now = std::chrono::steady_clock::now();
        auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline.value() - now);
        std::cout << "Request deadline in: " << remaining.count() << "ms" << std::endl;
    }
    
    // Simulate processing
    longRunningOperation(ctx, "Request-" + id);
}

int main() {
    std::cout << "=== gocxx Context Examples ===" << std::endl;
    
    // Example 1: Basic timeout context
    std::cout << "\n--- Example 1: Timeout Context ---" << std::endl;
    {
        auto [ctx, cancel] = WithTimeout(Background(), 2s);
        
        // This should be canceled due to timeout
        longRunningOperation(ctx, "TimeoutExample");
    }
    
    // Example 2: Manual cancellation
    std::cout << "\n--- Example 2: Manual Cancellation ---" << std::endl;
    {
        auto [ctx, cancel] = WithCancel(Background());
        
        // Start operation in a separate thread
        std::thread worker([ctx]() {
            longRunningOperation(ctx, "ManualExample");
        });
        
        // Cancel after 1.5 seconds
        std::this_thread::sleep_for(1500ms);
        std::cout << "Manually canceling operation..." << std::endl;
        cancel();
        
        worker.join();
    }
    
    // Example 3: Context with values
    std::cout << "\n--- Example 3: Context with Values ---" << std::endl;
    {
        // Create context with request metadata
        auto ctx = WithValue(Background(), std::string("request_id"), std::string("REQ-12345"));
        ctx = WithValue(ctx, std::string("user_id"), std::string("user789"));
        
        // Add timeout to the value context
        auto [timeout_ctx, cancel] = WithTimeout(ctx, 1s);
        
        processRequest(timeout_ctx);
    }
    
    // Example 4: Context hierarchy
    std::cout << "\n--- Example 4: Context Hierarchy ---" << std::endl;
    {
        // Create a parent context with timeout
        auto [parent_ctx, parent_cancel] = WithTimeout(Background(), 3s);
        
        // Create child contexts
        auto [child1_ctx, child1_cancel] = WithCancel(parent_ctx);
        auto [child2_ctx, child2_cancel] = WithCancel(parent_ctx);
        
        // Start operations in separate threads
        std::thread worker1([child1_ctx]() {
            longRunningOperation(child1_ctx, "Child1");
        });
        
        std::thread worker2([child2_ctx]() {
            longRunningOperation(child2_ctx, "Child2");
        });
        
        // Cancel parent after 1 second (should cancel both children)
        std::this_thread::sleep_for(1s);
        std::cout << "Canceling parent context..." << std::endl;
        parent_cancel();
        
        worker1.join();
        worker2.join();
    }
    
    // Example 5: Deadline context
    std::cout << "\n--- Example 5: Deadline Context ---" << std::endl;
    {
        auto deadline = std::chrono::steady_clock::now() + 1500ms;
        auto [ctx, cancel] = WithDeadline(Background(), deadline);
        
        std::cout << "Operation will be canceled at a specific deadline..." << std::endl;
        longRunningOperation(ctx, "DeadlineExample");
    }
    
    // Example 6: Context utilities
    std::cout << "\n--- Example 6: Context Utilities ---" << std::endl;
    {
        auto [ctx, cancel] = WithTimeout(Background(), 2s);
        
        std::cout << "Checking if context will be canceled soon..." << std::endl;
        
        if (WillBeCanceledSoon(ctx, 3s)) {
            std::cout << "Context will be canceled within 3 seconds" << std::endl;
        }
        
        if (WillBeCanceledSoon(ctx, 1s)) {
            std::cout << "Context will be canceled within 1 second" << std::endl;
        } else {
            std::cout << "Context will not be canceled within 1 second" << std::endl;
        }
        
        // Wait for context cancellation
        std::cout << "Waiting for context to be canceled..." << std::endl;
        if (WaitForContext(ctx, 5s)) {
            std::cout << "Context was canceled" << std::endl;
        } else {
            std::cout << "Timeout waiting for context cancellation" << std::endl;
        }
    }
    
    std::cout << "\n=== All examples completed ===" << std::endl;
    return 0;
}

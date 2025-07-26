
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "gocxx/sync/sync.h"

using namespace gocxx::sync;

TEST(CondTest, WaitAndNotifyOne) {
    Cond cond;
    Mutex mtx;
	UniqueLock lock(mtx);

    bool signaled = false;

    std::thread t([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        {
            Lock lg(mtx);
            signaled = true;
        }
        cond.NotifyOne();
    });

    cond.Wait(lock);
    EXPECT_TRUE(signaled);
    t.join();
}

TEST(CondTest, WaitAndNotifyAll) {
    Cond cond;
    Mutex mtx;
    std::atomic<int> ready{0};

    const int thread_count = 3;
    std::vector<std::thread> threads;

    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back([&]() {
            UniqueLock lock(mtx);
            cond.Wait(lock);
            ready++;
        });
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    cond.NotifyAll();

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(ready.load(), thread_count);
}

TEST(MutexTest, BasicLockUnlock) {
    Mutex mtx;
    int counter = 0;

    {
        Lock lock(mtx);
        counter++;
    }

    EXPECT_EQ(counter, 1);
}

TEST(MutexTest, MultiThreadedIncrement) {
    Mutex mtx;
    int counter = 0;
    const int num_threads = 10;
    const int num_iterations = 1000;

    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < num_iterations; ++j) {
                Lock lock(mtx);
                counter++;
            }
            });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(counter, num_threads * num_iterations);
}

TEST(SyncOnceTest, OnlyRunsOnce) {
    Once once;
    std::atomic<int> counter{0};

    auto fn = [&]() {
        counter++;
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&]() {
            once.Do(fn);
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(counter, 1);
}


struct Dummy {
    int value = 0;
};

TEST(PoolTest, BasicAllocationAndReuse) {
    std::atomic<int> created{0};

    Pool<Dummy> pool([&] {
        created++;
        return std::make_shared<Dummy>();
    });

    auto d1 = pool.Get();
    d1->value = 42;
    EXPECT_EQ(d1->value, 42);
    EXPECT_EQ(created, 1);

    pool.Put(d1);

    auto d2 = pool.Get();
    EXPECT_EQ(d2->value, 42);  
    EXPECT_EQ(created, 1);

    pool.Put(d2);
    auto d3 = pool.Get();
    EXPECT_EQ(created, 1);
    EXPECT_EQ(d3->value, 42);
}

TEST(PoolTest, MultipleAllocations) {
    std::atomic<int> created{0};
    Pool<Dummy> pool([&] {
        created++;
        return std::make_shared<Dummy>();
    });

    auto d1 = pool.Get();
    auto d2 = pool.Get();
    auto d3 = pool.Get();

    EXPECT_EQ(created, 3);

    pool.Put(d1);
    pool.Put(d2);
    pool.Put(d3);

    auto r1 = pool.Get();
    auto r2 = pool.Get();
    auto r3 = pool.Get();

    EXPECT_EQ(created, 3); 
}

TEST(PoolTest, ThreadSafety) {
    std::atomic<int> created{0};

    Pool<Dummy> pool([&] {
        created++;
        return std::make_shared<Dummy>();
    });

    constexpr int threadCount = 10;
    constexpr int perThreadOps = 100;

    std::vector<std::thread> threads;
    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back([&] {
            for (int j = 0; j < perThreadOps; ++j) {
                auto obj = pool.Get();
                obj->value = j;
                pool.Put(obj);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_GE(created, 1); 
    EXPECT_LE(created, threadCount * perThreadOps); 
}

TEST(RWMutexTest, ReadWriteAccess) {
    RWMutex mtx;
    int counter = 0;

    auto reader = [&]() {
        ReadLock lock(mtx);
        EXPECT_GE(counter, 0); 
    };

    auto writer = [&]() {
        WriteLock lock(mtx);
        ++counter;
    };

    std::vector<std::thread> threads;

    for (int i = 0; i < 5; ++i)
        threads.emplace_back(writer);

    for (int i = 0; i < 5; ++i)
        threads.emplace_back(reader);

    for (auto& t : threads)
        t.join();

    EXPECT_EQ(counter, 5);
}

TEST(WaitGroupTest, ParallelTasksComplete) {
    WaitGroup wg;
    std::atomic<int> shared{0};

    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        wg.Add(1);
        threads.emplace_back([&wg, &shared]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            shared++;
            wg.Done();
        });
    }

    wg.Wait();
    
    // Clean up threads
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(shared.load(), 5);
}
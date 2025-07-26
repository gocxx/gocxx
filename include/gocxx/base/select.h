// gocxx/base/select.h
#pragma once
#include <vector>
#include <memory>
#include <condition_variable>
#include <mutex>
#include <random>
#include <atomic>
#include <functional>
#include <optional>
#include <algorithm>
#include <gocxx/base/chan.h>
#include <gocxx/base/defer.h>

namespace gocxx {
namespace base {

    class Select;

    /**
     * Abstract base class for select cases.
     * Each case represents a channel operation that can be selected.
     */
    class SelectCase {
    public:
        SelectCase() : caseId_(nextCaseId_.fetch_add(1, std::memory_order_relaxed)) {}
        virtual ~SelectCase() = default;

        // Non-copyable, non-movable for safety
        SelectCase(const SelectCase&) = delete;
        SelectCase& operator=(const SelectCase&) = delete;
        SelectCase(SelectCase&&) = delete;
        SelectCase& operator=(SelectCase&&) = delete;

        /**
         * Check if this case is ready to proceed immediately.
         * @return true if the case can execute without blocking
         */
        virtual bool isReady() = 0;

        /**
         * Execute the case operation.
         * This is called when the case is selected.
         */
        virtual void execute() = 0;

        /**
         * Register this case with a Select instance.
         * @param sel The Select instance to register with
         */
        virtual void registerWith(Select* sel) = 0;

        /**
         * Unregister this case from its Select instance.
         */
        virtual void unregister() = 0;

        /**
         * Get a string representation of the case type.
         * @return String identifying the case type
         */
        virtual std::string getType() const = 0;

        /**
         * Get the unique case ID.
         * @return The unique identifier for this case
         */
        size_t getCaseId() const { return caseId_; }

    private:
        size_t caseId_;
        static std::atomic<size_t> nextCaseId_;
    };

    std::atomic<size_t> SelectCase::nextCaseId_{1};

    /**
     * Select implementation that mirrors Go's select statement.
     * Allows waiting on multiple channel operations simultaneously.
     */
    class Select {
    public:
        Select() : done_(false), ready_(false), selectId_(nextSelectId_.fetch_add(1, std::memory_order_relaxed)) {}

        ~Select() {
            // Ensure cleanup happens even if run() wasn't called
            cleanup();
        }

        // Non-copyable, non-movable
        Select(const Select&) = delete;
        Select& operator=(const Select&) = delete;
        Select(Select&&) = delete;
        Select& operator=(Select&&) = delete;

        /**
         * Add a case to this select statement.
         * @param sc Unique pointer to the case to add
         */
        void addCase(std::unique_ptr<SelectCase> sc) {
            if (sc) {
                cases_.push_back(std::move(sc));
            }
        }

        /**
         * Execute the select statement.
         * This will block until one of the cases can proceed.
         */
        void run() {
            std::unique_lock<std::mutex> lock(mutex_);

            // Register all cases with their respective channels
            for (auto& c : cases_) {
                c->registerWith(this);
            }

            // Use RAII to ensure cleanup
            auto cleanup_guard = [this]() { this->cleanup(); };
            defer(cleanup_guard);

            while (true) {
                // Check for immediately ready cases
                std::vector<size_t> readyIndices;
                bool hasDefaultCase = false;
                size_t defaultCaseIndex = 0;
                
                for (size_t i = 0; i < cases_.size(); ++i) {
                    if (cases_[i]->getType() == "DefaultCase") {
                        hasDefaultCase = true;
                        defaultCaseIndex = i;
                    } else if (cases_[i]->isReady()) {
                        readyIndices.push_back(i);
                    }
                }

                // If non-default cases are ready, execute one randomly and return
                if (!readyIndices.empty()) {
                    executeRandomCase(readyIndices);
                    return;
                }

                // If no cases are ready and we have a default case, execute it
                if (hasDefaultCase) {
                    cases_[defaultCaseIndex]->execute();
                    return;
                }

                // Reset flags before waiting
                ready_ = false;
                done_.store(false, std::memory_order_release);

                // Wait for notification with proper spurious wakeup protection
                cv_.wait(lock, [this] { 
                    return done_.load(std::memory_order_acquire) || ready_; 
                });

                // After waking up, check again for ready cases
                // The loop will continue and re-evaluate all cases
            }
        }

        /**
         * Notify the select that a case may be ready.
         * This is called by channels when they become available.
         */
        void notify() {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!done_.load(std::memory_order_relaxed)) {
                ready_ = true;
                done_.store(true, std::memory_order_release);
                cv_.notify_one();
            }
        }

        /**
         * Get the condition variable for channel registration.
         * @return Pointer to the internal condition variable
         */
        std::condition_variable* cv() { return &cv_; }

        /**
         * Get the ready flag for channel registration.
         * @return Pointer to the internal ready flag
         */
        bool* readyFlag() { return &ready_; }

        /**
         * Get the unique select ID.
         * @return The unique identifier for this select instance
         */
        size_t getSelectId() const { return selectId_; }

    private:
        /**
         * Clean up all cases by unregistering them.
         */
        void cleanup() {
            // Signal that select is done to prevent further notifications
            done_.store(true, std::memory_order_release);
            
            for (auto& c : cases_) {
                if (c) {
                    c->unregister();
                }
            }
        }

        /**
         * Execute a randomly selected case from the ready cases.
         * @param readyIndices Vector of indices of ready cases
         */
        void executeRandomCase(const std::vector<size_t>& readyIndices) {
            if (readyIndices.empty()) return;

            size_t selectedIndex;
            if (readyIndices.size() == 1) {
                selectedIndex = readyIndices[0];
            } else {
                // Use thread-local random number generator for performance
                static thread_local std::random_device rd;
                static thread_local std::mt19937 gen(rd());
                std::uniform_int_distribution<size_t> dist(0, readyIndices.size() - 1);
                size_t randomIdx = dist(gen);
                selectedIndex = readyIndices[randomIdx];
            }

            if (selectedIndex < cases_.size() && cases_[selectedIndex]) {
                cases_[selectedIndex]->execute();
            }
        }

        std::vector<std::unique_ptr<SelectCase>> cases_;
        std::atomic<bool> done_;
        std::mutex mutex_;
        std::condition_variable cv_;
        bool ready_;
        size_t selectId_;
        static std::atomic<size_t> nextSelectId_;
    };

    std::atomic<size_t> Select::nextSelectId_{1};

    /**
     * Case for receiving from a channel.
     */
    template<typename T>
    class RecvCase : public SelectCase {
    public:
        RecvCase(Chan<T>& ch, std::function<void(std::optional<T>)> fn) 
            : chan_(ch), fn_(std::move(fn)), sel_(nullptr) {}

        ~RecvCase() {
            unregister();
        }

        bool isReady() override {
            return chan_.canRecv();
        }

        void execute() override {
            // For unbuffered channels, we need to handle the case where 
            // the channel might be closed but has no data
            auto result = chan_.tryRecv();
            if (result.Ok()) {
                fn_(std::move(result.value));
            } else {
                // If tryRecv failed but channel is closed, send nullopt
                // If tryRecv failed and channel is not closed, this shouldn't happen
                // if isReady() returned true
                if (chan_.isClosed()) {
                    fn_(std::nullopt);
                } else {
                    // This is a race condition - channel was ready but now isn't
                    // Try regular recv which will block until something is available
                    auto val = chan_.recv();
                    fn_(std::move(val));
                }
            }
        }

        void registerWith(Select* sel) override {
            sel_ = sel;
            if (sel && sel->cv()) {
                chan_.impl()->registerRecvWaiter(sel->cv(), sel->readyFlag());
            }
        }

        void unregister() override {
            if (sel_ && sel_->cv()) {
                chan_.impl()->unregisterRecvWaiter(sel_->cv());
                sel_ = nullptr;
            }
        }

        std::string getType() const override {
            return "RecvCase";
        }

    private:
        Chan<T>& chan_;
        std::function<void(std::optional<T>)> fn_;
        Select* sel_;
    };

    /**
     * Case for sending to a channel.
     */
    template<typename T>
    class SendCase : public SelectCase {
    public:
        SendCase(Chan<T>& ch, T val, std::function<void(bool)> fn) 
            : chan_(ch), value_(std::move(val)), fn_(std::move(fn)), sel_(nullptr) {}

        ~SendCase() {
            unregister();
        }

        bool isReady() override {
            return chan_.canSend();
        }

        void execute() override {
            // Make a copy to preserve the original value for potential retry
            T valueCopy = value_;
            auto result = chan_.trySend(std::move(valueCopy));
            fn_(result.Ok());
        }

        void registerWith(Select* sel) override {
            sel_ = sel;
            if (sel && sel->cv()) {
                chan_.impl()->registerSendWaiter(sel->cv(), sel->readyFlag());
            }
        }

        void unregister() override {
            if (sel_ && sel_->cv()) {
                chan_.impl()->unregisterSendWaiter(sel_->cv());
                sel_ = nullptr;
            }
        }

        std::string getType() const override {
            return "SendCase";
        }

    private:
        Chan<T>& chan_;
        T value_;
        std::function<void(bool)> fn_;
        Select* sel_;
    };

    /**
     * Default case that always executes if no other cases are ready.
     */
    class DefaultCase : public SelectCase {
    public:
        explicit DefaultCase(std::function<void()> fn) : fn_(std::move(fn)) {}

        bool isReady() override { 
            return true;  // Default case is always ready
        }

        void execute() override { 
            if (fn_) fn_(); 
        }

        void registerWith(Select* /* sel */) override {
            // Default case doesn't need channel registration
        }

        void unregister() override {
            // Default case doesn't need unregistration
        }

        std::string getType() const override {
            return "DefaultCase";
        }

    private:
        std::function<void()> fn_;
    };

    // =================== HELPER FUNCTIONS ===================

    /**
     * Create a receive case for a select statement.
     * @param ch The channel to receive from
     * @param fn Function to call with the received value (or nullopt if closed)
     * @return Unique pointer to the created case
     */
    template<typename T>
    std::unique_ptr<RecvCase<T>> recv(Chan<T>& ch, std::function<void(std::optional<T>)> fn) {
        return std::make_unique<RecvCase<T>>(ch, std::move(fn));
    }

    /**
     * Create a send case for a select statement.
     * @param ch The channel to send to
     * @param val The value to send
     * @param fn Function to call with send result (true if successful)
     * @return Unique pointer to the created case
     */
    template<typename T>
    std::unique_ptr<SendCase<T>> send(Chan<T>& ch, T val, std::function<void(bool)> fn) {
        return std::make_unique<SendCase<T>>(ch, std::move(val), std::move(fn));
    }

    /**
     * Create a default case for a select statement.
     * @param fn Function to call if no other cases are ready
     * @return Unique pointer to the created case
     */
    inline std::unique_ptr<DefaultCase> defaultCase(std::function<void()> fn) {
        return std::make_unique<DefaultCase>(std::move(fn));
    }

    /**
     * Execute a select statement with the given cases.
     * This is the main entry point that mimics Go's select statement.
     * @param cs Variable number of case arguments
     */
    template<typename... Cases>
    void select(Cases&&... cs) {
        Select sel;
        (sel.addCase(std::forward<Cases>(cs)), ...);
        sel.run();
    }

} // namespace base
} // namespace gocxx
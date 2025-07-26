/**
 * @file chan.h
 * @brief Channel implementation for Go-like communication
 * 
 * This file provides the core channel implementation that enables
 * safe communication between threads, similar to Go channels.
 */

// gocxx/base/chan.h
#pragma once
#include <memory>
#include <optional>
#include <stdexcept>
#include <gocxx/sync/sync.h>
#include <queue>
#include <type_traits>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <algorithm>
#include <gocxx/errors/errors.h>
#include <gocxx/base/result.h>

/**
 * @namespace gocxx
 * @brief Root namespace for all gocxx library components
 */
namespace gocxx {

/**
 * @namespace gocxx::base
 * @brief Core primitives and fundamental types
 * 
 * This namespace contains the essential building blocks of the gocxx library,
 * including channels, defer, select operations, and result types.
 */
namespace base {

    /**
     * @interface IChan
     * @brief Interface for channel operations
     * @tparam T The type of data transmitted through the channel
     * 
     * This interface defines the contract for all channel implementations,
     * providing both blocking and non-blocking send/receive operations.
     */
    template<typename T>
    class IChan {
    public:
        /**
         * @brief Send a value to the channel (blocking)
         * @param value The value to send
         * @throws std::runtime_error if the channel is closed
         */
        virtual void send(T&& value) = 0;
        
        /**
         * @brief Receive a value from the channel (blocking)
         * @return Optional containing the received value, or nullopt if channel is closed
         */
        virtual std::optional<T> recv() = 0;
        
        /**
         * @brief Try to send a value without blocking
         * @param value The value to send
         * @return Result indicating success or failure reason
         */
        virtual Result<void> trySend(T&& value) = 0;
        
        /**
         * @brief Try to receive a value without blocking
         * @return Result containing the value or error
         */
        virtual Result<T> tryRecv() = 0;
        
        /**
         * @brief Close the channel
         * 
         * After closing, no more values can be sent, but remaining
         * buffered values can still be received.
         */
        virtual void close() = 0;
        
        /**
         * @brief Check if the channel is closed
         * @return true if closed, false otherwise
         */
        virtual bool isClosed() const = 0;
        
        /**
         * @brief Register a condition variable for receive waiting (internal use)
         */
        virtual void registerRecvWaiter(std::condition_variable* cv, bool* ready) = 0;
        
        /**
         * @brief Unregister a condition variable for receive waiting (internal use)
         */
        virtual void unregisterRecvWaiter(std::condition_variable* cv) = 0;
        
        /**
         * @brief Register a condition variable for send waiting (internal use)
         */
        virtual void registerSendWaiter(std::condition_variable* cv, bool* ready) = 0;
        
        /**
         * @brief Unregister a condition variable for send waiting (internal use)
         */
        virtual void unregisterSendWaiter(std::condition_variable* cv) = 0;
        
        /**
         * @brief Check if the channel can accept a send operation
         * @return true if send won't block, false otherwise
         */
        virtual bool canSend() const = 0;
        
        /**
         * @brief Check if the channel has data ready for receive
         * @return true if receive won't block, false otherwise
         */
        virtual bool canRecv() const = 0;
        
        virtual ~IChan() = default;
    };

    template<typename T>
    class ChanImpl : public IChan<T> {
    public:
        explicit ChanImpl(std::size_t bufferSize)
            : bufferSize_(bufferSize), closed_(false) {}

        void send(T&& value) override {
            gocxx::sync::UniqueLock lock(mutex_);
            if (closed_) {
                throw std::runtime_error("send on closed channel");
            }

            if (bufferSize_ == 0) {
                // Unbuffered channel - synchronous send/receive
                sendValue_ = std::move(value);
                hasSendValue_ = true;
                
                // Notify any waiting receivers
                cond_recv_.NotifyOne();
                notifySelectWaiters(recvWaiters_);
                
                // Wait for receiver to pick up the value
                while (!closed_ && hasSendValue_) {
                    cond_send_.Wait(lock);
                }
                if (closed_) {
                    throw std::runtime_error("send on closed channel");
                }
            } else {
                // Buffered channel
                while (!closed_ && queue_.size() >= bufferSize_) {
                    cond_send_.Wait(lock);
                }
                if (closed_) {
                    throw std::runtime_error("send on closed channel");
                }

                queue_.push(std::move(value));
                cond_recv_.NotifyOne();
                notifySelectWaiters(recvWaiters_);
            }
        }

        std::optional<T> recv() override {
            gocxx::sync::UniqueLock lock(mutex_);

            if (bufferSize_ == 0) {
                // Unbuffered channel - wait for sender
                while (!closed_ && !hasSendValue_) {
                    cond_recv_.Wait(lock);
                }

                if (!hasSendValue_ && closed_) {
                    return std::nullopt;
                }

                if (hasSendValue_) {
                    auto val = std::move(sendValue_.value());
                    sendValue_.reset();
                    hasSendValue_ = false;
                    cond_send_.NotifyOne(); // Wake up the sender
                    return val;
                }
                
                return std::nullopt;
            } else {
                // Buffered channel
                while (!closed_ && queue_.empty()) {
                    cond_recv_.Wait(lock);
                }

                if (queue_.empty() && closed_) {
                    return std::nullopt;
                }

                auto val = std::move(queue_.front());
                queue_.pop();
                cond_send_.NotifyOne(); // Wake up any waiting senders
                return val;
            }
        }

        Result<void> trySend(T&& value) override {
            gocxx::sync::UniqueLock lock(mutex_);
            if (closed_) {
                return Result<void>(gocxx::errors::New("trySend on closed channel"));
            }

            if (bufferSize_ == 0) {
                // Unbuffered channel - need immediate receiver ready
                // For non-blocking, we can't wait - just check if receiver is waiting
                if (hasSendValue_) {
                    return Result<void>(gocxx::errors::New("channel busy"));
                }

                sendValue_ = std::move(value);
                hasSendValue_ = true;
                cond_recv_.NotifyOne();
                notifySelectWaiters(recvWaiters_);
                return Result<void>();
            } else {
                // Buffered channel
                if (queue_.size() >= bufferSize_) {
                    return Result<void>(gocxx::errors::New("buffer full"));
                }

                queue_.push(std::move(value));
                cond_recv_.NotifyOne();
                notifySelectWaiters(recvWaiters_);
                return Result<void>();
            }
        }

        Result<T> tryRecv() override {
            gocxx::sync::UniqueLock lock(mutex_);

            if (bufferSize_ == 0) {
                // Unbuffered channel
                if (!hasSendValue_) {
                    if (closed_) {
                        return Result<T>(gocxx::errors::New("channel closed"));
                    }
                    return Result<T>(gocxx::errors::New("no data to receive"));
                }

                auto val = std::move(sendValue_.value());
                sendValue_.reset();
                hasSendValue_ = false;
                cond_send_.NotifyOne();
                return Result<T>(std::move(val));
            } else {
                // Buffered channel
                if (queue_.empty()) {
                    if (closed_) {
                        return Result<T>(gocxx::errors::New("channel closed"));
                    }
                    return Result<T>(gocxx::errors::New("buffer empty"));
                }

                auto val = std::move(queue_.front());
                queue_.pop();
                cond_send_.NotifyOne();
                return Result<T>(std::move(val));
            }
        }

        void close() override {
            gocxx::sync::Lock lock(mutex_);
            if (closed_) return;  // Already closed

            closed_ = true;
            cond_recv_.NotifyAll();
            cond_send_.NotifyAll();
            
            // Notify all select statements waiting on this channel
            notifySelectWaiters(recvWaiters_);
            notifySelectWaiters(sendWaiters_);
        }

        bool isClosed() const override {
            gocxx::sync::Lock lock(mutex_);
            return closed_;
        }

        void registerRecvWaiter(std::condition_variable* cv, bool* ready) override {
            if (!cv) return;
            gocxx::sync::Lock lock(mutex_);
            recvWaiters_.emplace_back(cv, ready);
        }

        void unregisterRecvWaiter(std::condition_variable* cv) override {
            if (!cv) return;
            gocxx::sync::Lock lock(mutex_);
            recvWaiters_.erase(
                std::remove_if(recvWaiters_.begin(), recvWaiters_.end(),
                    [cv](const auto& pair) { return pair.first == cv; }),
                recvWaiters_.end()
            );
        }

        void registerSendWaiter(std::condition_variable* cv, bool* ready) override {
            if (!cv) return;
            gocxx::sync::Lock lock(mutex_);
            sendWaiters_.emplace_back(cv, ready);
        }

        void unregisterSendWaiter(std::condition_variable* cv) override {
            if (!cv) return;
            gocxx::sync::Lock lock(mutex_);
            sendWaiters_.erase(
                std::remove_if(sendWaiters_.begin(), sendWaiters_.end(),
                    [cv](const auto& pair) { return pair.first == cv; }),
                sendWaiters_.end()
            );
        }

        bool canSend() const override {
            gocxx::sync::Lock lock(mutex_);
            if (closed_) return false;

            if (bufferSize_ == 0) {
                return !hasSendValue_; // Can send if no pending send
            } else {
                return queue_.size() < bufferSize_;
            }
        }

        bool canRecv() const override {
            gocxx::sync::Lock lock(mutex_);
            if (bufferSize_ == 0) {
                // For unbuffered channels: can receive if there's a send value waiting OR if closed
                // But only return true if there's actually something to receive or channel is closed
                return hasSendValue_ || closed_;
            }
            else {
                // For buffered channels: can receive if buffer has data OR if closed
                return !queue_.empty() || closed_;
            }
        }

    private:
        void notifySelectWaiters(const std::vector<std::pair<std::condition_variable*, bool*>>& waiters) {
            for (const auto& [cv, ready] : waiters) {
                if (ready) *ready = true;
                if (cv) cv->notify_one();
            }
        }

        std::size_t bufferSize_;
        std::atomic<bool> closed_;

        mutable gocxx::sync::Mutex mutex_;
        gocxx::sync::Cond cond_recv_, cond_send_;

        // Unbuffered channel state
        std::optional<T> sendValue_;
        bool hasSendValue_ = false;

        // Buffered channel state
        std::queue<T> queue_;

        // Select statement waiters
        std::vector<std::pair<std::condition_variable*, bool*>> recvWaiters_;
        std::vector<std::pair<std::condition_variable*, bool*>> sendWaiters_;
    };

    /**
     * @class Chan
     * @brief Thread-safe channel for communication between threads
     * @tparam T The type of data transmitted through the channel
     * 
     * Chan provides a Go-like channel implementation that enables safe
     * communication and synchronization between threads. Channels can be
     * buffered or unbuffered, and support both blocking and non-blocking operations.
     * 
     * @par Unbuffered Channels (bufferSize = 0)
     * Send operations block until a receiver is ready, providing synchronization.
     * 
     * @par Buffered Channels (bufferSize > 0)
     * Send operations only block when the buffer is full.
     * 
     * @par Example Usage
     * @code
     * // Create an unbuffered channel
     * Chan<int> ch;
     * 
     * // Send data (will block until receiver is ready)
     * ch << 42;
     * 
     * // Receive data
     * int value;
     * ch >> value;
     * 
     * // Create a buffered channel
     * Chan<std::string> buffered_ch(5);
     * @endcode
     * 
     * @par Thread Safety
     * All operations on Chan are thread-safe and can be called concurrently
     * from multiple threads.
     */
    template<typename T>
    class Chan {
    public:
        explicit Chan(std::size_t bufferSize = 0)
            : impl_(std::make_shared<ChanImpl<T>>(bufferSize)) {}

        // Static factory method for Go-like syntax
        static std::shared_ptr<Chan<T>> Make(std::size_t bufferSize = 0) {
            return std::make_shared<Chan<T>>(bufferSize);
        }

        void send(T&& value) { 
            impl_->send(std::move(value)); 
        }

        template<typename U = T>
        std::enable_if_t<std::is_copy_constructible_v<U>, void>
        send(const T& value) { 
            T tmp(value); 
            impl_->send(std::move(tmp)); 
        }

        std::optional<T> recv() { 
            return impl_->recv(); 
        }

        Result<void> trySend(T&& value) { 
            return impl_->trySend(std::move(value)); 
        }

        template<typename U = T>
        std::enable_if_t<std::is_copy_constructible_v<U>, Result<void>>
        trySend(const T& value) { 
            T tmp(value); 
            return impl_->trySend(std::move(tmp)); 
        }

        Result<T> tryRecv() { 
            return impl_->tryRecv(); 
        }

        void close() { 
            impl_->close(); 
        }

        bool isClosed() const { 
            return impl_->isClosed(); 
        }

        bool canSend() const { 
            return impl_->canSend(); 
        }

        bool canRecv() const { 
            return impl_->canRecv(); 
        }

        std::shared_ptr<IChan<T>> impl() const { 
            return impl_; 
        }

    private:
        std::shared_ptr<IChan<T>> impl_;
    };

    // Operator overloads for syntactic sugar

    template<typename T>
    Chan<T>& operator<<(Chan<T>& ch, const T& val) {
        T tmp(val);
        ch.send(std::move(tmp));
        return ch;
    }

    template<typename T, typename U>
    std::enable_if_t<std::is_constructible_v<T, U>, Chan<T>&> 
    operator<<(Chan<T>& ch, U&& value) {
        ch.send(T(std::forward<U>(value)));
        return ch;
    }

    template<typename T>
    Chan<T>& operator<<(Chan<T>& ch, T&& val) {
        ch.send(std::move(val));
        return ch;
    }

    template<typename T>
    Chan<T>& operator>>(Chan<T>& ch, T& out) {
        auto val = ch.recv();
        if (!val) throw std::runtime_error("recv on closed channel");
        out = std::move(*val);
        return ch;
    }

} // namespace base
} // namespace gocxx

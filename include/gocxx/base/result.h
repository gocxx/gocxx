#pragma once

#include <memory>
#include <utility>
#include <type_traits>
#include <gocxx/errors/errors.h>

// Fix for the errors project's undefined 'e' type
namespace gocxx::errors {
    using e = Error;
}

namespace gocxx::base {

/**
 * @brief A container that holds either a valid result or an error.
 *
 * Mimics Go-style `(T, error)` return values in C++.
 *
 * @tparam T Type of the successful value.
 */
template <typename T>
struct Result {
    T value{};  ///< The result value (may be default-initialized).
    std::shared_ptr<gocxx::errors::Error> err;  ///< Error, if any.

    /// @brief Constructs a Result with a value and no error.
    /// This is the success case, similar to Go's `return value, nil`.
    Result(T value) : value(std::move(value)), err(nullptr) {}

    /// @brief Constructs a Result with an error and no value.
    /// This is the error case, similar to Go's `return nil, err`.
    Result(std::shared_ptr<gocxx::errors::Error> error)
        : value(), err(std::move(error)) {
    }

    /// @brief Constructs a Result with both a value and an error.
    /// This is used when an operation returns a value along with an error.
    Result(T value, std::shared_ptr<gocxx::errors::Error> error)
        : value(std::move(value)), err(std::move(error)) {
    }

    Result() = default;  

    /// @brief Returns true if the operation was successful.
    bool Ok() const noexcept { return !err; }

    /// @brief Returns true if an error occurred.
    bool Failed() const noexcept { return static_cast<bool>(err); }

    /// @brief Returns the value if no error, otherwise returns the fallback.
    T UnwrapOr(T fallback) const {
        return err ? fallback : value;
    }

    /// @brief Moves out the value if no error, or returns the fallback.
    T UnwrapOrMove(T fallback) {
        return err ? std::move(fallback) : std::move(value);
    }

    /// @brief Conversion to bool: true if Ok().
    explicit operator bool() const noexcept { return Ok(); }
};


/// @brief Specialization for void results, used when no value is returned.
/// This is similar to Go's `error` type, which indicates success or failure without a value.
template <>
struct Result<void> {
    std::shared_ptr<gocxx::errors::Error> err;

    Result(std::shared_ptr<gocxx::errors::Error> error)
        : err(std::move(error)) {
    }

    Result() = default;

     /// @brief Returns true if the operation was successful.
    bool Ok() const noexcept { return !err; }

    /// @brief Returns true if an error occurred.
    bool Failed() const noexcept { return static_cast<bool>(err); }

    /// @brief Conversion to bool: true if Ok().
    explicit operator bool() const noexcept { return Ok(); }
};


}  // namespace gocxx::base

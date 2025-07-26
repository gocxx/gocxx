#pragma once

#include <memory>
#include <string>
#include <gocxx/errors/errors.h>

namespace gocxx::io {

    // ------------------ IO Errors (inline shared constants) ------------------

    inline const std::shared_ptr<errors::Error> ErrEOF =
        std::make_shared<errors::simpleError>("EOF");

    inline const std::shared_ptr<errors::Error> ErrUnexpectedEOF =
        std::make_shared<errors::simpleError>("unexpected EOF");

    inline const std::shared_ptr<errors::Error> ErrShortWrite =
        std::make_shared<errors::simpleError>("short write");

    inline const std::shared_ptr<errors::Error> ErrShortBuffer =
        std::make_shared<errors::simpleError>("short buffer");

    inline const std::shared_ptr<errors::Error> ErrNoProgress =
        std::make_shared<errors::simpleError>("multiple Read calls return no data");

    inline const std::shared_ptr<errors::Error> ErrTimeout =
        std::make_shared<errors::simpleError>("I/O timeout");

    inline const std::shared_ptr<errors::Error> ErrInterrupted =
        std::make_shared<errors::simpleError>("I/O interrupted");

    inline const std::shared_ptr<errors::Error> ErrBufferTooSmall =
        std::make_shared<errors::simpleError>("buffer too small");

    inline const std::shared_ptr<errors::Error> ErrUnknownIO =
        std::make_shared<errors::simpleError>("unknown I/O error");

    // ------------------ For dynamic/custom errors ------------------


    inline std::shared_ptr<errors::Error> NewTimeoutError(const std::string& msg = "") {
        return msg.empty() ? ErrTimeout : errors::Wrap(msg, ErrTimeout);
    }

    inline std::shared_ptr<errors::Error> NewInterruptedError(const std::string& msg = "") {
        return msg.empty() ? ErrInterrupted : errors::Wrap(msg, ErrInterrupted);
    }

    inline std::shared_ptr<errors::Error> NewUnexpectedEOFError(const std::string& msg = "") {
        return msg.empty() ? ErrUnexpectedEOF : errors::Wrap(msg, ErrUnexpectedEOF);
    }

    inline std::shared_ptr<errors::Error> NewEOFError(const std::string& msg = "") {
        return msg.empty() ? ErrEOF : errors::Wrap(msg, ErrEOF);
    }

    inline std::shared_ptr<errors::Error> NewBufferTooSmallError(const std::string& msg = "") {
        return msg.empty() ? ErrBufferTooSmall : errors::Wrap(msg, ErrBufferTooSmall);
    }

}  // namespace gocxx::io

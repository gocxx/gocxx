#pragma once

#include <cstdint>
#include <cstddef>
#include <optional>
#include <string>
#include <memory>
#include <cstdio>
#include <vector>
#include <istream>

#include <gocxx/errors/errors.h>
#include <gocxx/base/result.h>

namespace gocxx::io {

    class Reader
    {
    public:
        virtual ~Reader() = default;

        // Reads up to `size` bytes into `buffer`.
        // Returns the number of bytes Read (0 = EOF).
        virtual gocxx::base::Result<std::size_t> Read(uint8_t* buffer, std::size_t size) = 0;

        gocxx::base::Result<std::size_t> Read(std::vector<uint8_t>& buffer) {
            return Read(buffer.data(), buffer.size());
        }
    };

    class Writer
    {
    public:
        virtual ~Writer() = default;

        // Writes `size` bytes from `buffer`.
        // Returns the number of bytes written (0 = error).
        virtual gocxx::base::Result<std::size_t> Write(const uint8_t* buffer, std::size_t size) = 0;

        gocxx::base::Result<std::size_t> Write(const std::vector<uint8_t>& buffer) {
            return Write(buffer.data(), buffer.size());
        }
    };

    class Closer {
    public:
        virtual ~Closer() = default;
        virtual void close() = 0;
    };

    class ReadCloser : public Reader, public Closer {};
    class WriteCloser : public Writer, public Closer {};

    class ReaderAt {
    public:
        virtual ~ReaderAt() = default;
        virtual gocxx::base::Result<std::size_t> ReadAt(uint8_t* buffer, std::size_t size, std::size_t offset) = 0;
    };

    class WriterAt {
    public:
        virtual ~WriterAt() = default;
        virtual gocxx::base::Result<std::size_t> WriteAt(const uint8_t* buffer, std::size_t size, std::size_t offset) = 0;
    };

    enum whence {
        SeekStart = 0,   // io.SeekStart
        SeekCurrent = 1, // io.SeekCurrent
        SeekEnd = 2      // io.SeekEnd
    };

    class Seeker {
    public:
        virtual ~Seeker() = default;
        virtual gocxx::base::Result<std::size_t> Seek(std::size_t offset, whence whence) = 0;
    };

    class OffsetWriter : public Writer, public WriterAt, public Seeker {
    public:
        explicit OffsetWriter(std::shared_ptr<WriterAt> w, std::size_t offset);
        gocxx::base::Result<std::size_t> WriteAt(const uint8_t* buffer, std::size_t size, std::size_t offset) override;
        gocxx::base::Result<std::size_t> Write(const uint8_t* buffer, std::size_t size) override;
        gocxx::base::Result<std::size_t> Seek(std::size_t offset, whence whence) override;

    private:
        std::shared_ptr<WriterAt> w;
        std::size_t base;
        std::size_t currentOffset;
    };

    class ByteReader {
    public:
        virtual ~ByteReader() = default;
        virtual gocxx::base::Result<std::size_t> ReadByte(uint8_t& outByte) = 0;
    };

    class ByteWriter {
    public:
        virtual ~ByteWriter() = default;
        virtual gocxx::base::Result<std::size_t> WriteByte(uint8_t byte) = 0;
    };

    class PipeReader : public Reader {
    public:
        virtual gocxx::base::Result<std::size_t> Close() = 0;
        virtual gocxx::base::Result<std::size_t> CloseWithError(std::shared_ptr<gocxx::errors::Error> err) = 0;
    };

    class PipeWriter : public Writer {
    public:
        virtual gocxx::base::Result<std::size_t> Close() = 0;
        virtual gocxx::base::Result<std::size_t> CloseWithError(std::shared_ptr<gocxx::errors::Error> err) = 0;
    };

    // Function declarations
    gocxx::base::Result<std::size_t> Copy(std::shared_ptr<Writer> dst, std::shared_ptr<Reader> src);
    gocxx::base::Result<std::size_t> CopyBuffer(std::shared_ptr<Writer> dst, std::shared_ptr<Reader> src, uint8_t* buf, std::size_t size);
    gocxx::base::Result<std::size_t> CopyN(std::shared_ptr<Writer> dst, std::shared_ptr<Reader> src, std::size_t n);
    gocxx::base::Result<std::size_t> ReadAll(std::shared_ptr<Reader> r, std::vector<uint8_t>& out);
    gocxx::base::Result<std::size_t> ReadAtLeast(std::shared_ptr<Reader> r, std::vector<uint8_t>& buf, std::size_t min);
    gocxx::base::Result<std::size_t> ReadFull(std::shared_ptr<Reader> r, std::vector<uint8_t>& buf);
    gocxx::base::Result<std::size_t> WriteString(std::shared_ptr<Writer> w, const std::string& s);

    class LimitedReader : public Reader {
    public:
        LimitedReader(std::shared_ptr<Reader> base, std::size_t n);
        gocxx::base::Result<std::size_t> Read(uint8_t* buffer, std::size_t size) override;

    private:
        std::shared_ptr<Reader> r;
        std::size_t remaining;
        std::size_t totalRead = 0;
    };

    std::pair<std::shared_ptr<PipeReader>, std::shared_ptr<PipeWriter>> Pipe();

} // namespace gocxx::io
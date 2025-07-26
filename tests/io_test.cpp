#include <gtest/gtest.h>
#include <gocxx/io/io.h>
#include <gocxx/io/io_errors.h>
#include <gocxx/errors/errors.h>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <cstring>

using namespace gocxx::io;
using gocxx::base::Result;
using gocxx::errors::Is;
using gocxx::errors::Error;

class StringReader : public Reader {
public:
    explicit StringReader(const std::string& data) : data_(data), offset_(0) {}

    Result<std::size_t> Read(uint8_t* buffer, std::size_t size) override {
        if (offset_ >= data_.size()) {
            return { 0, ErrEOF };
        }
        std::size_t bytesToRead = std::min(size, data_.size() - offset_);
        std::memcpy(buffer, data_.data() + offset_, bytesToRead);
        offset_ += bytesToRead;
        return bytesToRead;
    }

private:
    std::string data_;
    std::size_t offset_;
};

class VectorWriter : public Writer {
public:
    Result<std::size_t> Write(const uint8_t* buffer, std::size_t size) override {
        out.insert(out.end(), buffer, buffer + size);
        return size;
    }

    std::string str() const {
        return std::string(out.begin(), out.end());
    }

    std::vector<uint8_t> out;
};

// ------------------- Tests -------------------

TEST(IOTest, CopyCopiesAllData) {
    auto reader = std::make_shared<StringReader>("Hello, gocxx IO!");
    auto writer = std::make_shared<VectorWriter>();

    auto res = Copy(writer, reader);
    EXPECT_TRUE(res.Ok());
    EXPECT_EQ(writer->str(), "Hello, gocxx IO!");
}

TEST(IOTest, CopyNStopsAfterNBytes) {
    auto reader = std::make_shared<StringReader>("abcdefg");
    auto writer = std::make_shared<VectorWriter>();

    auto res = CopyN(writer, reader, 4);
    EXPECT_TRUE(res.Ok());
    EXPECT_EQ(writer->str(), "abcd");
    EXPECT_EQ(res.value, 4);
}

TEST(IOTest, ReadFullReadsExactlySize) {
    auto reader = std::make_shared<StringReader>("12345678");
    std::vector<uint8_t> buf(5);

    auto res = ReadFull(reader, buf);
    EXPECT_TRUE(res.Ok());
    EXPECT_EQ(std::string(buf.begin(), buf.end()), "12345");
}

TEST(IOTest, ReadAllConcatenatesAllData) {
    auto reader = std::make_shared<StringReader>("abcde12345xyz");
    std::vector<uint8_t> buf;

    auto res = ReadAll(reader, buf);

    EXPECT_TRUE(res.Ok() || Is(res.err, ErrEOF));
    EXPECT_EQ(std::string(buf.begin(), buf.end()), "abcde12345xyz");
}

TEST(IOTest, PipeTransfersData) {
    auto [r, w] = Pipe();

    std::thread writerThread([w] {
        std::string msg = "pipe-data";
        auto res = w->Write(reinterpret_cast<const uint8_t*>(msg.data()), msg.size());
        EXPECT_TRUE(res.Ok());
        EXPECT_EQ(res.value, msg.size());
        w->Close();
        });

    std::vector<uint8_t> buf(128);
    auto res = r->Read(buf.data(), buf.size());
    EXPECT_TRUE(res.Ok());
    EXPECT_EQ(std::string(buf.begin(), buf.begin() + res.value), "pipe-data");

    writerThread.join();
}

TEST(IOTest, LimitedReaderStopsAtLimit) {
    auto baseReader = std::make_shared<StringReader>("HelloWorld");
    LimitedReader limited(baseReader, 5);

    std::vector<uint8_t> buf(10);
    auto res = limited.Read(buf.data(), buf.size());

    EXPECT_TRUE(res.Ok());
    EXPECT_EQ(res.value, 5);
    EXPECT_EQ(std::string(buf.begin(), buf.begin() + res.value), "Hello");

    auto eof = limited.Read(buf.data(), buf.size());
    EXPECT_FALSE(eof.Ok());
    EXPECT_TRUE(Is(eof.err, ErrEOF));
}

TEST(IOTest, OffsetWriterSeeksAndWrites) {
    class MemoryWriter : public WriterAt {
    public:
        std::vector<uint8_t> buffer = std::vector<uint8_t>(10, '.');

        Result<std::size_t> WriteAt(const uint8_t* data, std::size_t size, std::size_t offset) override {
            if (offset + size > buffer.size()) buffer.resize(offset + size, '.');
            std::copy(data, data + size, buffer.begin() + offset);
            return size;
        }
    };

    auto w = std::make_shared<MemoryWriter>();
    OffsetWriter offsetWriter(w, 2);
    std::string str = "abc";

    offsetWriter.Write(reinterpret_cast<const uint8_t*>(str.data()), str.size());

    EXPECT_EQ(std::string(w->buffer.begin(), w->buffer.end()), "..abc.....");

    auto seekRes = offsetWriter.Seek(2, SeekCurrent);
    EXPECT_TRUE(seekRes.Ok());

    offsetWriter.Write(reinterpret_cast<const uint8_t*>("X"), 1);
    EXPECT_EQ(w->buffer[7], 'X');
}

TEST(IOTest, CopyNFailsOnEOF) {
    auto reader = std::make_shared<StringReader>("abcd");
    auto writer = std::make_shared<VectorWriter>();

    auto res = CopyN(writer, reader, 10);  // Ask more than available

    EXPECT_FALSE(res.Ok());
    EXPECT_TRUE(Is(res.err, ErrUnexpectedEOF));
    EXPECT_EQ(writer->str(), "abcd");
    EXPECT_EQ(res.value, 4);
}

TEST(IOTest, ReadAtLeastFailsOnEOF) {
    auto reader = std::make_shared<StringReader>("abc");
    std::vector<uint8_t> buf(10); // Enough size, but input too small

    auto res = ReadAtLeast(reader, buf, 5);
    EXPECT_FALSE(res.Ok());
    EXPECT_TRUE(Is(res.err, ErrUnexpectedEOF));
    EXPECT_EQ(std::string(buf.begin(), buf.begin() + res.value), "abc");
}

TEST(IOTest, ReadAtLeastFailsOnSmallBuffer) {
    auto reader = std::make_shared<StringReader>("123456");
    std::vector<uint8_t> buf(3); // Too small for min = 5

    auto res = ReadAtLeast(reader, buf, 5);
    EXPECT_FALSE(res.Ok());
    EXPECT_TRUE(Is(res.err, ErrBufferTooSmall));
}

TEST(IOTest, ReadFullFailsOnEOF) {
    auto reader = std::make_shared<StringReader>("123");
    std::vector<uint8_t> buf(5);  // Buffer larger than available data

    auto res = ReadFull(reader, buf);

    EXPECT_FALSE(res.Ok());
    EXPECT_TRUE(Is(res.err, ErrUnexpectedEOF));
    EXPECT_LT(res.value, buf.size());
    EXPECT_EQ(std::string(buf.begin(), buf.begin() + res.value), "123");
}

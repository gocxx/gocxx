#include "gocxx/io/io.h"
#include "gocxx/io/io_errors.h"

#include <memory>
#include <mutex>
#include <deque>

namespace gocxx::io {


    using gocxx::errors::Error;
    using gocxx::base::Result;

    Result<std::size_t> Copy(std::shared_ptr<Writer> dest, std::shared_ptr<Reader> source) {
        constexpr std::size_t bufferSize = 8192;
        std::unique_ptr<uint8_t[]> buffer(new uint8_t[bufferSize]);
        std::size_t totalBytes = 0;

        while (true) {
            auto res = source->Read(buffer.get(), bufferSize);
            if (!res.Ok()) {
                if (errors::Is(res.err, ErrEOF)) break;
                return Result<std::size_t>{ totalBytes, res.err };
            }

            if (res.value > 0) {
                auto wres = dest->Write(buffer.get(), res.value);
                if (!wres.Ok()) return { totalBytes, wres.err };
                totalBytes += wres.value;
            }
        }

        return {totalBytes,nullptr};
    }

    Result<std::size_t> CopyBuffer(std::shared_ptr<Writer> dst, std::shared_ptr<Reader> src, uint8_t* buf, std::size_t size) {
        if (!buf || size == 0) {
            return { 0, ErrUnknownIO };
        }

        std::size_t total = 0;
        while (true) {
            auto rres = src->Read(buf, size);
            if (rres.value > 0) {
                auto wres = dst->Write(buf, rres.value);
                if (!wres.Ok()) return { total, wres.err };
                total += wres.value;
            }
            if (!rres.Ok()) return { total, rres.err };
        }
		return { total, nullptr };
    }

    Result<std::size_t> CopyN(std::shared_ptr<Writer> dst, std::shared_ptr<Reader> src, std::size_t n) {
        std::vector<uint8_t> buf(4096);
        std::size_t total = 0;

        while (total < n) {
            std::size_t toRead = std::min(buf.size(), n - total);
            auto rres = src->Read(buf.data(), toRead);

            if (rres.value > 0) {
                auto wres = dst->Write(buf.data(), rres.value);
                if (!wres.Ok()) return { total, wres.err };
                total += wres.value;
            }

            if (!rres.Ok()) {
                if (errors::Is(rres.err, ErrEOF)) {
                    return { total, errors::Cause(ErrUnexpectedEOF,rres.err)};
                }
                return { total, rres.err };
            }
        }

        return total;
    }

    Result<std::size_t> ReadAll(std::shared_ptr<Reader> r, std::vector<uint8_t>& out) {
        std::vector<uint8_t> buf(4096);
        std::size_t totalRead = 0;

        while (true) {
            auto res = r->Read(buf.data(), buf.size());
            if (res.value > 0) {
                out.insert(out.end(), buf.begin(), buf.begin() + res.value);
                totalRead += res.value;
            }
            if (!res.Ok()) {
                if (errors::Is(res.err, ErrEOF)) return totalRead;
                return { totalRead, res.err };
            }
        }
		return { totalRead, nullptr };
    }

    Result<std::size_t> ReadAtLeast(std::shared_ptr<Reader> r, std::vector<uint8_t>& buf, std::size_t min) {
        std::size_t total = 0;
        if (buf.size() < min) {
            return { 0, ErrBufferTooSmall };
        }

        while (total < min) {
            auto res = r->Read(buf.data() + total, buf.size() - total);
            if (res.value > 0) total += res.value;

            if (!res.Ok()) {
                if (errors::Is(res.err, ErrEOF)) {
                    return { total, errors::Cause(ErrUnexpectedEOF,res.err)};
                }
                return { total, res.err };
            }
        }

        return total;
    }

    Result<std::size_t> ReadFull(std::shared_ptr<Reader> r, std::vector<uint8_t>& buf) {
        std::size_t total = 0;
        std::size_t size = buf.size();

        if (size == 0) return { 0 };

        while (total < size) {
            auto res = r->Read(buf.data() + total, size - total);
            if (!res.Ok()) {
                return { total, errors::Cause(ErrUnexpectedEOF,res.err)};
            }

            total += res.value;
            if (res.value == 0) return { total, ErrUnexpectedEOF };
        }

        return total;
    }


    Result<std::size_t> WriteString(std::shared_ptr<Writer> w, const std::string& s) {
        if (s.empty()) return { 0 };
        return w->Write(reinterpret_cast<const uint8_t*>(s.data()), s.size());
    }

    LimitedReader::LimitedReader(std::shared_ptr<Reader> r, std::size_t n) : r(r), remaining(n) {}


    gocxx::base::Result<std::size_t> LimitedReader::Read(uint8_t* buffer, std::size_t size) {
            if (!buffer) {
                return { 0, errors::New("LimitedReader: null buffer") };
            }

            if (remaining == 0) {
                return { 0, ErrEOF };  // limit reached
            }

            std::size_t toRead = std::min(size, remaining);
            gocxx::base::Result<std::size_t> res = r->Read(buffer, toRead);

            if (!res.Ok()) {
                return res;
            }

            if (res.value > 0) {
                remaining -= res.value;
                totalRead += res.value;
            }

            return res;
        }

        // --- OffsetWriter ---

        OffsetWriter::OffsetWriter(std::shared_ptr<WriterAt> w, std::size_t offset)
            : w(std::move(w)), base(offset), currentOffset(offset) {
        }

        gocxx::base::Result<std::size_t> OffsetWriter::WriteAt(const uint8_t* buffer, std::size_t size, std::size_t offset) {
            if (!w) {
                return { 0, errors::New("OffsetWriter: null WriterAt") };
            }
            return w->WriteAt(buffer, size, offset);
        }

        gocxx::base::Result<std::size_t> OffsetWriter::Write(const uint8_t* buffer, std::size_t size) {
            if (!w) {
                return { 0, errors::New("OffsetWriter: null WriterAt") };
            }

            gocxx::base::Result<std::size_t> res = w->WriteAt(buffer, size, currentOffset);
            if (res.Ok() && res.value > 0) {
                currentOffset += res.value;
            }
            return res;
        }

        gocxx::base::Result<std::size_t> OffsetWriter::Seek(std::size_t offset, whence whence) {
            std::size_t newOffset = 0;

            switch (whence) {
            case whence::SeekStart:
                newOffset = base + offset;
                break;
            case whence::SeekCurrent:
                newOffset = currentOffset + offset;
                break;
            case whence::SeekEnd:
                return { 0, errors::New("OffsetWriter: SeekEnd not supported") };
            default:
                return { 0, errors::New("OffsetWriter: Invalid seek origin") };
            }

            if (newOffset < base) {
                return { 0, errors::New("OffsetWriter: Seek before base not allowed") };
            }

            currentOffset = newOffset;
            return { newOffset - base }; // relative offset
        }

        // --- SharedPipe ---

        class SharedPipe {
        public:
            std::mutex mtx;
            std::condition_variable cv;

            std::deque<uint8_t> buffer;
            bool closed = false;
            std::shared_ptr<errors::Error> closeError = nullptr;

            gocxx::base::Result<std::size_t> Write(const uint8_t* data, std::size_t size) {
                if (!data) return { 0, errors::New("Pipe Write: null buffer") };

                std::unique_lock lock(mtx);
                if (closed) {
                    return { 0, errors::New("Pipe Write: closed") };
                }

                buffer.insert(buffer.end(), data, data + size);
                cv.notify_all();
                return { size };
            }

            gocxx::base::Result<std::size_t> Read(uint8_t* out, std::size_t size) {
                if (!out) return { 0, errors::New("Pipe Read: null buffer") };

                std::unique_lock lock(mtx);
                while (buffer.empty() && !closed) {
                    cv.wait(lock);
                }

                std::size_t n = 0;
                while (!buffer.empty() && n < size) {
                    out[n++] = buffer.front();
                    buffer.pop_front();
                }

                if (n > 0) return { n };

                if (closed) {
                    return { 0, closeError ? closeError : ErrEOF };
                }

                return { 0, errors::New("Pipe Read: unknown error state") };
            }

            gocxx::base::Result<std::size_t> Close(const std::shared_ptr<errors::Error>& err) {
                std::unique_lock lock(mtx);
                if (!closed) {
                    closed = true;
                    closeError = err;
                    cv.notify_all();
                }
                return { 0 };
            }
        };

        // --- PipeReaderImpl ---

        class PipeReaderImpl : public PipeReader {
        public:
            explicit PipeReaderImpl(std::shared_ptr<SharedPipe> pipe) : pipe_(std::move(pipe)) {}

            gocxx::base::Result<std::size_t> Read(uint8_t* buffer, std::size_t size) override {
                return pipe_->Read(buffer, size);
            }

            gocxx::base::Result<std::size_t> Close() override {
                return pipe_->Close(nullptr);
            }

            gocxx::base::Result<std::size_t> CloseWithError(std::shared_ptr<errors::Error> err) override {
                return pipe_->Close(std::move(err));
            }

        private:
            std::shared_ptr<SharedPipe> pipe_;
        };

        // --- PipeWriterImpl ---

        class PipeWriterImpl : public PipeWriter {
        public:
            explicit PipeWriterImpl(std::shared_ptr<SharedPipe> pipe) : pipe_(std::move(pipe)) {}

            gocxx::base::Result<std::size_t> Write(const uint8_t* buffer, std::size_t size) override {
                return pipe_->Write(buffer, size);
            }

            gocxx::base::Result<std::size_t> Close() override {
                return pipe_->Close(nullptr);
            }

            gocxx::base::Result<std::size_t> CloseWithError(std::shared_ptr<errors::Error> err) override {
                return pipe_->Close(std::move(err));
            }

        private:
            std::shared_ptr<SharedPipe> pipe_;
        };

        // --- Pipe creation ---

        std::pair<std::shared_ptr<PipeReader>, std::shared_ptr<PipeWriter>> Pipe() {
            auto pipe = std::make_shared<SharedPipe>();
            return {
                std::make_shared<PipeReaderImpl>(pipe),
                std::make_shared<PipeWriterImpl>(pipe)
            };
        }

    } // namespace gocxx::io

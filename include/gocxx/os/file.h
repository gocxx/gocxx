#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <memory>
#include <vector>
#include <chrono>
#include <random>
#include <sys/stat.h>

#include <gocxx/errors/errors.h>
#include <gocxx/base/result.h>
#include <gocxx/io/io.h>

namespace gocxx::os {

    // File mode constants (similar to Go's os package)
    enum class OpenFlag : int {
        RDONLY = 0x0,
        WRONLY = 0x1,
        RDWR = 0x2,
        APPEND = 0x400,
        CREATE = 0x40,
        EXCL = 0x80,
        SYNC = 0x101000,
        TRUNC = 0x200,
    };

    inline OpenFlag operator|(OpenFlag a, OpenFlag b) {
        return static_cast<OpenFlag>(static_cast<int>(a) | static_cast<int>(b));
    }

    inline OpenFlag operator&(OpenFlag a, OpenFlag b) {
        return static_cast<OpenFlag>(static_cast<int>(a) & static_cast<int>(b));
    }

    inline OpenFlag& operator|=(OpenFlag& a, OpenFlag b) {
        a = a | b;
        return a;
    }

    inline bool HasFlag(OpenFlag flags, OpenFlag test) {
        return static_cast<int>(flags & test) != 0;
    }

    template<typename... Flags>
    int CombineFlags(Flags... flags) {
        return (0 | ... | static_cast<int>(flags));
    }

    // File permissions (similar to Go's os package)
    constexpr int ModeDir = 0x80000000;
    constexpr int ModeAppend = 0x40000000;
    constexpr int ModeExclusive = 0x20000000;
    constexpr int ModeTemporary = 0x10000000;
    constexpr int ModeSymlink = 0x08000000;
    constexpr int ModeDevice = 0x04000000;
    constexpr int ModeNamedPipe = 0x02000000;
    constexpr int ModeSocket = 0x01000000;
    constexpr int ModeSetuid = 0x00800000;
    constexpr int ModeSetgid = 0x00400000;
    constexpr int ModeCharDevice = 0x00200000;
    constexpr int ModeSticky = 0x00100000;
    constexpr int ModeIrregular = 0x00080000;

    constexpr int ModePermBits = 0777;

    // FileMode represents a file's mode and permission bits
    using FileMode = uint32_t;

    // FileInfo describes a file and is returned by Stat
    struct FileInfo {
        std::string name;          // base name of the file
        int64_t size;              // length in bytes for regular files
        FileMode mode;             // file mode bits
        std::chrono::system_clock::time_point modTime; // modification time
        bool isDir;                // abbreviation for Mode().IsDir()
        
        bool IsDir() const { return isDir; }
        bool IsRegular() const { return (mode & ModeDir) == 0; }
        FileMode Mode() const { return mode; }
        std::string Name() const { return name; }
        int64_t Size() const { return size; }
        std::chrono::system_clock::time_point ModTime() const { return modTime; }
    };

    // PathError records an error and the operation and file path that caused it
    class PathError : public gocxx::errors::Error {
        std::string op;
        std::string path;
        std::shared_ptr<gocxx::errors::Error> err;

    public:
        PathError(std::string operation, std::string filepath, std::shared_ptr<gocxx::errors::Error> error)
            : op(std::move(operation)), path(std::move(filepath)), err(std::move(error)) {}

        std::string error() const noexcept override {
            if (err) {
                return op + " " + path + ": " + err->error();
            }
            return op + " " + path;
        }

        std::shared_ptr<gocxx::errors::Error> Unwrap() const noexcept override {
            return err;
        }

        std::string Op() const { return op; }
        std::string Path() const { return path; }
        std::shared_ptr<gocxx::errors::Error> Err() const { return err; }
    };

    // SyscallError records an error from a specific system call
    class SyscallError : public gocxx::errors::Error {
        std::string syscall;
        std::shared_ptr<gocxx::errors::Error> err;

    public:
        SyscallError(std::string syscallName, std::shared_ptr<gocxx::errors::Error> error)
            : syscall(std::move(syscallName)), err(std::move(error)) {}

        std::string error() const noexcept override {
            if (err) {
                return syscall + ": " + err->error();
            }
            return syscall;
        }

        std::shared_ptr<gocxx::errors::Error> Unwrap() const noexcept override {
            return err;
        }

        std::string Syscall() const { return syscall; }
        std::shared_ptr<gocxx::errors::Error> Err() const { return err; }
    };

    // Common errors
    extern std::shared_ptr<gocxx::errors::Error> ErrInvalid;
    extern std::shared_ptr<gocxx::errors::Error> ErrPermission;
    extern std::shared_ptr<gocxx::errors::Error> ErrExist;
    extern std::shared_ptr<gocxx::errors::Error> ErrNotExist;
    extern std::shared_ptr<gocxx::errors::Error> ErrClosed;
    extern std::shared_ptr<gocxx::errors::Error> ErrNoDeadline;
    extern std::shared_ptr<gocxx::errors::Error> ErrDeadlineExceeded;

    // File represents an open file descriptor
    class File : public gocxx::io::Reader, 
                 public gocxx::io::Writer, 
                 public gocxx::io::Closer,
                 public gocxx::io::ReaderAt,
                 public gocxx::io::WriterAt,
                 public gocxx::io::Seeker {
    private:
        int fd;
        std::string name;
        bool closed;

    public:
        explicit File(int file_descriptor, std::string filename);
        ~File();

        // Reader interface
        gocxx::base::Result<std::size_t> Read(uint8_t* buffer, std::size_t size) override;

        // Writer interface
        gocxx::base::Result<std::size_t> Write(const uint8_t* buffer, std::size_t size) override;

        // Closer interface
        void close() override;

        // ReaderAt interface
        gocxx::base::Result<std::size_t> ReadAt(uint8_t* buffer, std::size_t size, std::size_t offset) override;

        // WriterAt interface
        gocxx::base::Result<std::size_t> WriteAt(const uint8_t* buffer, std::size_t size, std::size_t offset) override;

        // Seeker interface
        gocxx::base::Result<std::size_t> Seek(std::size_t offset, gocxx::io::whence whence) override;
        // File-specific methods
        gocxx::base::Result<void> Chdir();

        gocxx::base::Result<void> Chmod(FileMode mode);

        gocxx::base::Result<void> Chown(int uid, int gid);

        gocxx::base::Result<FileInfo> Stat();

        gocxx::base::Result<void> Sync();

        gocxx::base::Result<void> Truncate(int64_t size);

        gocxx::base::Result<std::string> ReadLink();

        gocxx::base::Result<std::vector<FileInfo>> ReadDir();

        // Getters
        std::string Name() const { return name; }
        int Fd() const { return fd; }
        bool IsClosed() const { return closed; }
    };

    // Standard file descriptors
    extern std::shared_ptr<File> Stdin;
    extern std::shared_ptr<File> Stdout;
    extern std::shared_ptr<File> Stderr;

    // Directory entry
    struct DirEntry {
        std::string name;
        bool isDir;
        FileMode mode;
        
        std::string Name() const { return name; }
        bool IsDir() const { return isDir; }
        FileMode Type() const { return mode; }
        gocxx::base::Result<FileInfo> Info() const;
    };

    // File operations (similar to Go's os package)
    
    // Create creates or truncates the named file
    gocxx::base::Result<std::shared_ptr<File>> Create(const std::string& name);
    
    // Open opens the named file for reading
    gocxx::base::Result<std::shared_ptr<File>> Open(const std::string& name);
    
    // OpenFile is the generalized open call
    gocxx::base::Result<std::shared_ptr<File>> OpenFile(const std::string& name, int flag, FileMode perm);

    // File information
    gocxx::base::Result<FileInfo> Stat(const std::string& name);
    gocxx::base::Result<FileInfo> Lstat(const std::string& name);

    // Directory operations
    gocxx::base::Result<void> Chdir(const std::string& dir);
    gocxx::base::Result<std::string> Getwd();
    gocxx::base::Result<void> Mkdir(const std::string& name, FileMode perm);
    gocxx::base::Result<void> MkdirAll(const std::string& path, FileMode perm);
    gocxx::base::Result<std::vector<DirEntry>> ReadDir(const std::string& name);
    gocxx::base::Result<void> Remove(const std::string& name);
    gocxx::base::Result<void> RemoveAll(const std::string& path);
    gocxx::base::Result<void> Rename(const std::string& oldpath, const std::string& newpath);

    // File operations
    //gocxx::base::Result<void> Chmod(const std::string& name, FileMode mode);
    //gocxx::base::Result<void> Chown(const std::string& name, int uid, int gid);
    //gocxx::base::Result<void> Lchown(const std::string& name, int uid, int gid);
    //gocxx::base::Result<void> Link(const std::string& oldname, const std::string& newname);
    //gocxx::base::Result<void> Symlink(const std::string& oldname, const std::string& newname);
    //gocxx::base::Result<std::string> Readlink(const std::string& name);

    // Temporary files
    //gocxx::base::Result<std::shared_ptr<File>> CreateTemp(const std::string& dir, const std::string& pattern);
    //gocxx::base::Result<std::string> MkdirTemp(const std::string& dir, const std::string& pattern);

    // Convenience functions
    gocxx::base::Result<std::vector<uint8_t>> ReadFile(const std::string& name);
    gocxx::base::Result<void> WriteFile(const std::string& name, const std::vector<uint8_t>& data, FileMode perm);
    gocxx::base::Result<void> WriteFile(const std::string& name, const std::string& data, FileMode perm);

    // Path utilities
    std::string TempDir();
    //std::string UserCacheDir();
    //std::string UserConfigDir();
    //std::string UserHomeDir();

    // File utility functions
    bool PathExists(const std::string& path);
    bool IsFile(const std::string& path);
    bool IsDirectory(const std::string& path);
    gocxx::base::Result<int64_t> FileSize(const std::string& path);

    // Process utilities
    //gocxx::base::Result<void> Exit(int code);
    //std::vector<std::string> Args();
    //std::string Getenv(const std::string& key);
    //gocxx::base::Result<std::string> LookupEnv(const std::string& key);
    //gocxx::base::Result<void> Setenv(const std::string& key, const std::string& value);
    //gocxx::base::Result<void> Unsetenv(const std::string& key);
    //std::vector<std::string> Environ();
    //gocxx::base::Result<void> Clearenv();

    // Helper functions for error handling
    bool IsExist(const std::shared_ptr<gocxx::errors::Error>& err);
    bool IsNotExist(const std::shared_ptr<gocxx::errors::Error>& err);
    bool IsPermission(const std::shared_ptr<gocxx::errors::Error>& err);
    bool IsTimeout(const std::shared_ptr<gocxx::errors::Error>& err);

    // File mode utilities
    bool IsDir(FileMode mode);
    bool IsRegular(FileMode mode);
    FileMode ModePerm(FileMode mode);

} // namespace gocxx::os
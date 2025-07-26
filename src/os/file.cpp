#include "gocxx/os/file.h"
#include <errno.h>
#include <cstring>
#include <random>

#ifdef _WIN32
#include <direct.h>     // _mkdir, _getcwd, _chdir
#include <io.h>         // _chmod, _unlink
#include <windows.h>    // Windows API for symlink detection
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#ifndef S_ISDIR
#define S_ISDIR(mode)  (((mode) & S_IFMT) == S_IFDIR)
#endif
#define stat _stat
#define lstat _stat     // Windows doesn't distinguish
#define mkdir(path, mode) _mkdir(path)
#define getcwd _getcwd
#define chdir _chdir
#define chmod _chmod
#define remove _unlink
#include <cstdlib>
#define setenv(k, v, o) _putenv_s(k, v)
#define unsetenv(k) _putenv_s(k, "")
#define PATH_SEPARATOR '\\'
#define O_RDONLY _O_RDONLY
#define O_WRONLY _O_WRONLY
#define O_RDWR _O_RDWR
#define O_CREAT _O_CREAT
#define O_TRUNC _O_TRUNC
#define O_EXCL _O_EXCL
#define O_APPEND _O_APPEND
#define O_CREATE _O_CREAT
#else
#include <unistd.h>     // POSIX
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <cstring>
extern char** environ;
#define PATH_SEPARATOR '/'
#define O_CREATE O_CREAT
#endif

namespace gocxx::os {

    // Common errors
    std::shared_ptr<gocxx::errors::Error> ErrInvalid = gocxx::errors::New("invalid argument");
    std::shared_ptr<gocxx::errors::Error> ErrPermission = gocxx::errors::New("permission denied");
    std::shared_ptr<gocxx::errors::Error> ErrExist = gocxx::errors::New("file already exists");
    std::shared_ptr<gocxx::errors::Error> ErrNotExist = gocxx::errors::New("file does not exist");
    std::shared_ptr<gocxx::errors::Error> ErrClosed = gocxx::errors::New("file already closed");
    std::shared_ptr<gocxx::errors::Error> ErrNoDeadline = gocxx::errors::New("file type does not support deadline");
    std::shared_ptr<gocxx::errors::Error> ErrDeadlineExceeded = gocxx::errors::New("deadline exceeded");

    // Standard file descriptors
#ifdef _WIN32
    std::shared_ptr<File> Stdin = std::make_shared<File>(0, "stdin");
    std::shared_ptr<File> Stdout = std::make_shared<File>(1, "stdout");
    std::shared_ptr<File> Stderr = std::make_shared<File>(2, "stderr");
#else
    std::shared_ptr<File> Stdin = std::make_shared<File>(0, "/dev/stdin");
    std::shared_ptr<File> Stdout = std::make_shared<File>(1, "/dev/stdout");
    std::shared_ptr<File> Stderr = std::make_shared<File>(2, "/dev/stderr");
#endif

    // Helper function to convert errno to error
    std::shared_ptr<gocxx::errors::Error> errnoToError(int errnum) {
        switch (errnum) {
            case ENOENT:
                return ErrNotExist;
            case EEXIST:
                return ErrExist;
            case EACCES:
#ifndef _WIN32
            case EPERM:
#endif
                return ErrPermission;
            case EINVAL:
                return ErrInvalid;
            default:
                return gocxx::errors::New(std::string("system error: ") + strerror(errnum));
        }
    }

    inline int ToNativeFlags(int openFlags) {
        int native = 0;
        
        switch (openFlags & 0x3) {
            case static_cast<int>(OpenFlag::RDONLY): native |= O_RDONLY; break;
            case static_cast<int>(OpenFlag::WRONLY): native |= O_WRONLY; break;
            case static_cast<int>(OpenFlag::RDWR):   native |= O_RDWR;   break;
            default:
                return O_RDONLY; // Default fallback
        }
        
        if (openFlags & static_cast<int>(OpenFlag::APPEND)) native |= O_APPEND;
        if (openFlags & static_cast<int>(OpenFlag::CREATE)) native |= O_CREAT;
        if (openFlags & static_cast<int>(OpenFlag::EXCL))   native |= O_EXCL;
        if (openFlags & static_cast<int>(OpenFlag::TRUNC))  native |= O_TRUNC;
        
        return native;
    }

    // ========== FILE CLASS IMPLEMENTATION ==========

    File::File(int file_descriptor, std::string filename) 
        : fd(file_descriptor), name(std::move(filename)), closed(false) {}

    File::~File() {
        if (!closed && fd >= 0) {
            close();
        }
    }

    gocxx::base::Result<std::size_t> File::Read(uint8_t* buffer, std::size_t size) {
        if (closed) {
            return {0, ErrClosed};
        }

#ifdef _WIN32
        int result = _read(fd, buffer, static_cast<unsigned int>(size));
#else
        ssize_t result = ::read(fd, buffer, size);
#endif

        if (result < 0) {
            auto err = std::make_shared<PathError>("read", name, errnoToError(errno));
            return {0, err};
        }

        return {static_cast<std::size_t>(result), nullptr};
    }

    gocxx::base::Result<std::size_t> File::Write(const uint8_t* buffer, std::size_t size) {
        if (closed) {
            return {0, ErrClosed};
        }

#ifdef _WIN32
        int result = _write(fd, buffer, static_cast<unsigned int>(size));
#else
        ssize_t result = ::write(fd, buffer, size);
#endif

        if (result < 0) {
            auto err = std::make_shared<PathError>("write", name, errnoToError(errno));
            return {0, err};
        }

        return {static_cast<std::size_t>(result), nullptr};
    }

    void File::close() {
        if (!closed && fd >= 0) {
#ifdef _WIN32
            _close(fd);
#else
            ::close(fd);
#endif
            closed = true;
        }
    }

    gocxx::base::Result<std::size_t> File::ReadAt(uint8_t* buffer, std::size_t size, std::size_t offset) {
        if (closed) {
            return {0, ErrClosed};
        }

#ifdef _WIN32
        auto seekRes = _lseek(fd, static_cast<long>(offset), SEEK_SET);
#else
        auto seekRes = lseek(fd, static_cast<off_t>(offset), SEEK_SET);
#endif
        if (seekRes < 0) {
            auto err = std::make_shared<PathError>("readAt-lseek", name, errnoToError(errno));
            return {0, err};
        }

        return Read(buffer, size);
    }

    gocxx::base::Result<std::size_t> File::WriteAt(const uint8_t* buffer, std::size_t size, std::size_t offset) {
        if (closed) {
            return {0, ErrClosed};
        }
        
#ifdef _WIN32
        auto seekRes = _lseek(fd, static_cast<long>(offset), SEEK_SET);
#else
        auto seekRes = lseek(fd, static_cast<off_t>(offset), SEEK_SET);
#endif
        if (seekRes < 0) {
            auto err = std::make_shared<PathError>("writeAt-lseek", name, errnoToError(errno));
            return {0, err};
        }
        
        return Write(buffer, size);
    }

    gocxx::base::Result<std::size_t> File::Seek(std::size_t offset, gocxx::io::whence whence) {
        if (closed) {
            return {0, ErrClosed};
        }

        int wh = SEEK_SET;
        switch (whence) {
        case gocxx::io::SeekStart:   wh = SEEK_SET; break;
        case gocxx::io::SeekCurrent: wh = SEEK_CUR; break;
        case gocxx::io::SeekEnd:     wh = SEEK_END; break;
        default: 
            return {0, ErrInvalid};
        }

#ifdef _WIN32
        auto result = _lseek(fd, static_cast<long>(offset), wh);
#else
        auto result = lseek(fd, static_cast<off_t>(offset), wh);
#endif

        if (result < 0) {
            auto err = std::make_shared<PathError>("seek", name, errnoToError(errno));
            return {0, err};
        }

        return {static_cast<std::size_t>(result), nullptr};
    }

    gocxx::base::Result<void> File::Chdir() {
        if (closed) {
            return {ErrClosed};
        }
#ifdef _WIN32
        return {gocxx::errors::New("fchdir not supported on Windows")};
#else
        if (::fchdir(fd) < 0) {
            auto err = std::make_shared<PathError>("chdir", name, errnoToError(errno));
            return {err};
        }
        return {};
#endif
    }

    gocxx::base::Result<void> File::Chmod(FileMode mode) {
        if (closed) {
            return {ErrClosed};
        }

#ifdef _WIN32
        return {gocxx::errors::New("chmod not supported on Windows")};
#else
        if (::fchmod(fd, mode) < 0) {
            auto err = std::make_shared<PathError>("chmod", name, errnoToError(errno));
            return {err};
        }
        return {};
#endif
    }

    gocxx::base::Result<void> File::Chown(int uid, int gid) {
        if (closed) {
            return {ErrClosed};
        }

#ifdef _WIN32
        return {gocxx::errors::New("chown not supported on Windows")};
#else
        if (::fchown(fd, uid, gid) < 0) {
            auto err = std::make_shared<PathError>("chown", name, errnoToError(errno));
            return {err};
        }
        return {};
#endif
    }

    gocxx::base::Result<FileInfo> File::Stat() {
        if (closed) {
            return {FileInfo{}, ErrClosed};
        }

        struct stat st;
#ifdef _WIN32
        if (_fstat(fd, &st) < 0) {
#else
        if (::fstat(fd, &st) < 0) {
#endif
            auto err = std::make_shared<PathError>("stat", name, errnoToError(errno));
            return {FileInfo{}, err};
        }

        FileInfo info;
        info.name = name.substr(name.find_last_of("/\\") + 1);
        info.size = st.st_size;
        info.mode = st.st_mode;
        info.modTime = std::chrono::system_clock::from_time_t(st.st_mtime);
        info.isDir = S_ISDIR(st.st_mode);
        
        return {info, nullptr};
    }

    gocxx::base::Result<void> File::Sync() {
        if (closed) {
            return {ErrClosed};
        }

#ifdef _WIN32
        if (_commit(fd) < 0) {
#else
        if (::fsync(fd) < 0) {
#endif
            auto err = std::make_shared<PathError>("sync", name, errnoToError(errno));
            return {err};
        }

        return {};
    }

    gocxx::base::Result<void> File::Truncate(int64_t size) {
        if (closed) {
            return {ErrClosed};
        }

#ifdef _WIN32
        if (_chsize_s(fd, size) != 0) {
#else
        if (::ftruncate(fd, size) < 0) {
#endif
            auto err = std::make_shared<PathError>("truncate", name, errnoToError(errno));
            return {err};
        }

        return {};
    }

    gocxx::base::Result<std::string> File::ReadLink() {
        return {std::string(), gocxx::errors::New("readlink not supported on file descriptor")};
    }

    gocxx::base::Result<std::vector<FileInfo>> File::ReadDir() {
        if (closed) {
            return {std::vector<FileInfo>(), ErrClosed};
        }
        return {std::vector<FileInfo>(), gocxx::errors::New("readdir not implemented")};
    }

    // ========== GLOBAL FILE OPERATIONS ==========

    gocxx::base::Result<std::shared_ptr<File>> Create(const std::string& name) {
        int flags = static_cast<int>(OpenFlag::RDWR) | static_cast<int>(OpenFlag::CREATE) | static_cast<int>(OpenFlag::TRUNC);
        return OpenFile(name, flags, 0666);
    }

    gocxx::base::Result<std::shared_ptr<File>> Open(const std::string& name) {
        return OpenFile(name, static_cast<int>(OpenFlag::RDONLY), 0);
    }

    gocxx::base::Result<std::shared_ptr<File>> OpenFile(const std::string& name, int flag, FileMode perm) {
        int nativeFlags = ToNativeFlags(flag);
        
#ifdef _WIN32
        int fd = _open(name.c_str(), nativeFlags, perm);
#else
        int fd = ::open(name.c_str(), nativeFlags, perm);
#endif

        if (fd < 0) {
            auto err = std::make_shared<PathError>("open", name, errnoToError(errno));
            return {nullptr, err};
        }

        return {std::make_shared<File>(fd, name), nullptr};
    }

    gocxx::base::Result<FileInfo> Stat(const std::string& name) {
        struct stat st;
        if (::stat(name.c_str(), &st) < 0) {
            auto err = std::make_shared<PathError>("stat", name, errnoToError(errno));
            return {FileInfo{}, err};
        }

        FileInfo info;
        info.name = name.substr(name.find_last_of("/\\") + 1);
        info.size = st.st_size;
        info.mode = st.st_mode;
        info.modTime = std::chrono::system_clock::from_time_t(st.st_mtime);
        info.isDir = S_ISDIR(st.st_mode);
        
        return {info, nullptr};
    }

    gocxx::base::Result<FileInfo> Lstat(const std::string& name) {
        struct stat st;
        if (::lstat(name.c_str(), &st) < 0) {
            auto err = std::make_shared<PathError>("lstat", name, errnoToError(errno));
            return {FileInfo{}, err};
        }

        FileInfo info;
        info.name = name.substr(name.find_last_of("/\\") + 1);
        info.size = st.st_size;
        info.mode = st.st_mode;
        info.modTime = std::chrono::system_clock::from_time_t(st.st_mtime);
        info.isDir = S_ISDIR(st.st_mode);
        
        return {info, nullptr};
    }

    gocxx::base::Result<void> Chdir(const std::string& dir) {
        if (::chdir(dir.c_str()) < 0) {
            auto err = std::make_shared<PathError>("chdir", dir, errnoToError(errno));
            return {err};
        }
        
        return {};
    }

    gocxx::base::Result<std::string> Getwd() {
#ifdef _WIN32
        char* cwd = _getcwd(nullptr, 0);
#else
        char* cwd = ::getcwd(nullptr, 0);
#endif
        if (cwd == nullptr) {
            auto err = std::make_shared<SyscallError>("getcwd", errnoToError(errno));
            return {std::string(), err};
        }
        
        std::string result(cwd);
        free(cwd);
        return {result, nullptr};
    }

    gocxx::base::Result<void> Mkdir(const std::string& name, FileMode perm) {
#ifdef _WIN32
        if (_mkdir(name.c_str()) != 0) {
#else
        if (::mkdir(name.c_str(), perm) != 0) {
#endif
            auto err = std::make_shared<PathError>("mkdir", name, errnoToError(errno));
            return {err};
        }
        return {};
    }

    gocxx::base::Result<void> MkdirAll(const std::string& path, FileMode perm) {
        if (path.empty()) {
            return {ErrInvalid};
        }

        struct stat st;
        if (::stat(path.c_str(), &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                return {};
            }
            return {ErrExist};
        }

        size_t pos = path.find_last_of("/\\");
        if (pos != std::string::npos) {
            std::string parent = path.substr(0, pos);
            if (!parent.empty()) {
                auto result = MkdirAll(parent, perm);
                if (result.Failed()) return result;
            }
        }

        return Mkdir(path, perm);
    }

    gocxx::base::Result<std::vector<DirEntry>> ReadDir(const std::string& name) {
        std::vector<DirEntry> entries;

#ifdef _WIN32
        std::string pattern = name + "\\*";
        WIN32_FIND_DATAA findData;
        HANDLE hFind = FindFirstFileA(pattern.c_str(), &findData);

        if (hFind == INVALID_HANDLE_VALUE) {
            auto err = std::make_shared<PathError>("FindFirstFile", name, errnoToError(errno));
            return {entries, err};
        }

        do {
            std::string fname = findData.cFileName;
            if (fname == "." || fname == "..") continue;

            DirEntry e;
            e.name = fname;
            e.isDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            e.mode = 0;
            entries.push_back(e);
        } while (FindNextFileA(hFind, &findData));

        FindClose(hFind);
#else
        DIR* dir = ::opendir(name.c_str());
        if (!dir) {
            auto err = std::make_shared<PathError>("opendir", name, errnoToError(errno));
            return {entries, err};
        }

        struct dirent* entry;
        while ((entry = ::readdir(dir)) != nullptr) {
            std::string fname = entry->d_name;
            if (fname == "." || fname == "..") continue;

            DirEntry e;
            e.name = fname;
            e.mode = 0;
            
#ifdef DT_DIR
            if (entry->d_type != DT_UNKNOWN) {
                e.isDir = (entry->d_type == DT_DIR);
            } else
#endif
            {
                std::string fullPath = name + "/" + fname;
                struct stat st;
                if (::stat(fullPath.c_str(), &st) == 0) {
                    e.isDir = S_ISDIR(st.st_mode);
                } else {
                    e.isDir = false;
                }
            }

            entries.push_back(e);
        }

        ::closedir(dir);
#endif

        return {entries, nullptr};
    }

    gocxx::base::Result<void> Remove(const std::string& name) {
        if (::remove(name.c_str()) != 0) {
            auto err = std::make_shared<PathError>("remove", name, errnoToError(errno));
            return {err};
        }
        return {};
    }

    gocxx::base::Result<void> RemoveAll(const std::string& path) {
        struct stat st;
        if (::lstat(path.c_str(), &st) != 0) {
            if (errno == ENOENT) return {};
            auto err = std::make_shared<PathError>("lstat", path, errnoToError(errno));
            return {err};
        }

        if (!S_ISDIR(st.st_mode)) {
            return Remove(path);
        }

        auto entriesResult = ReadDir(path);
        if (entriesResult.Failed()) {
            return {entriesResult.err};
        }

        for (const auto& entry : entriesResult.value) {
            std::string fullPath = path + "/" + entry.name;
            auto res = RemoveAll(fullPath);
            if (res.Failed()) return res;
        }

#ifdef _WIN32
        if (_rmdir(path.c_str()) != 0) {
#else
        if (::rmdir(path.c_str()) != 0) {
#endif
            auto err = std::make_shared<PathError>("rmdir", path, errnoToError(errno));
            return {err};
        }

        return {};
    }

    gocxx::base::Result<void> Rename(const std::string& oldpath, const std::string& newpath) {
        if (::rename(oldpath.c_str(), newpath.c_str()) != 0) {
            auto err = std::make_shared<PathError>("rename", oldpath, errnoToError(errno));
            return {err};
        }
        return {};
    }

    // ========== CONVENIENCE FUNCTIONS ==========

    gocxx::base::Result<std::vector<uint8_t>> ReadFile(const std::string& name) {
        auto fileResult = Open(name);
        if (fileResult.Failed()) {
            return {std::vector<uint8_t>(), fileResult.err};
        }
        
        auto file = fileResult.value;
        auto statResult = file->Stat();
        if (statResult.Failed()) {
            return {std::vector<uint8_t>(), statResult.err};
        }
        
        auto info = statResult.value;
        std::vector<uint8_t> data(info.size);
        
        auto readResult = file->Read(data.data(), data.size());
        if (readResult.Failed()) {
            return {std::vector<uint8_t>(), readResult.err};
        }
        
        data.resize(readResult.value); 
        return {data, nullptr};
    }

    gocxx::base::Result<void> WriteFile(const std::string& name, const std::vector<uint8_t>& data, FileMode perm) {
        int flags = static_cast<int>(OpenFlag::WRONLY) | static_cast<int>(OpenFlag::CREATE) | static_cast<int>(OpenFlag::TRUNC);
        auto fileResult = OpenFile(name, flags, perm);
        if (fileResult.Failed()) {
            return {fileResult.err};
        }
        
        auto file = fileResult.value;
        auto writeResult = file->Write(data.data(), data.size());
        if (writeResult.Failed()) {
            return {writeResult.err};
        }
        
        return {};
    }

    gocxx::base::Result<void> WriteFile(const std::string& name, const std::string& data, FileMode perm) {
        std::vector<uint8_t> bytes(data.begin(), data.end());
        return WriteFile(name, bytes, perm);
    }

    // ========== UTILITY FUNCTIONS ==========

    bool IsExist(const std::shared_ptr<gocxx::errors::Error>& err) {
        return gocxx::errors::Is(err, ErrExist);
    }

    bool IsNotExist(const std::shared_ptr<gocxx::errors::Error>& err) {
        return gocxx::errors::Is(err, ErrNotExist);
    }

    bool IsPermission(const std::shared_ptr<gocxx::errors::Error>& err) {
        return gocxx::errors::Is(err, ErrPermission);
    }

    bool IsTimeout(const std::shared_ptr<gocxx::errors::Error>& err) {
        return gocxx::errors::Is(err, ErrDeadlineExceeded);
    }

    bool IsDir(FileMode mode) {
        return (mode & ModeDir) != 0;
    }

    bool IsRegular(FileMode mode) {
        return (mode & ModeDir) == 0;
    }

    FileMode ModePerm(FileMode mode) {
        return mode & ModePermBits;
    }

    // ========== DIRENTRY IMPLEMENTATION ==========

    gocxx::base::Result<FileInfo> DirEntry::Info() const {
        FileInfo info;
        info.name = name;
        info.size = 0;
        info.mode = mode;
        info.modTime = std::chrono::system_clock::now();
        info.isDir = isDir;
        return {info, nullptr};
    }

    // ========== PATH UTILITIES ==========

    std::string TempDir() {
#ifdef _WIN32
        char* temp = std::getenv("TEMP");
        if (temp) return std::string(temp);
        temp = std::getenv("TMP");
        if (temp) return std::string(temp);
        return "C:\\temp";
#else
        char* temp = std::getenv("TMPDIR");
        if (temp) return std::string(temp);
        return "/tmp";
#endif
    }

    bool PathExists(const std::string& path) {
        struct stat st;
        return ::stat(path.c_str(), &st) == 0;
    }

    bool IsFile(const std::string& path) {
        struct stat st;
        if (::stat(path.c_str(), &st) != 0) return false;
        return !S_ISDIR(st.st_mode);
    }

    bool IsDirectory(const std::string& path) {
        struct stat st;
        if (::stat(path.c_str(), &st) != 0) return false;
        return S_ISDIR(st.st_mode);
    }

    gocxx::base::Result<int64_t> FileSize(const std::string& path) {
        struct stat st;
        if (::stat(path.c_str(), &st) != 0) {
            auto err = std::make_shared<PathError>("stat", path, errnoToError(errno));
            return {0, err};
        }
        return {st.st_size, nullptr};
    }

} // namespace gocxx::os

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <functional>
#include <gocxx/base/result.h>
#include <gocxx/errors/errors.h>
#include <gocxx/os/file.h>

namespace gocxx::os {

    // ========== ENVIRONMENT VARIABLES ==========

    /**
     * Get the value of an environment variable.
     * Returns empty string if the variable doesn't exist.
     */
    std::string Getenv(const std::string& key);

    /**
     * Look up an environment variable.
     * Returns {value, true} if found, {"", false} if not found.
     */
    std::pair<std::string, bool> LookupEnv(const std::string& key);

    /**
     * Set an environment variable.
     */
    gocxx::base::Result<void> Setenv(const std::string& key, const std::string& value);

    /**
     * Unset an environment variable.
     */
    gocxx::base::Result<void> Unsetenv(const std::string& key);

    /**
     * Clear all environment variables.
     */
    gocxx::base::Result<void> Clearenv();

    /**
     * Get all environment variables as a vector of "key=value" strings.
     */
    std::vector<std::string> Environ();

    /**
     * Expand environment variables in a string using ${VAR} or $VAR syntax.
     */
    std::string ExpandEnv(const std::string& s);

    // ========== PROCESS INFORMATION ==========

    /**
     * Get command line arguments.
     */
    std::vector<std::string> Args();

    /**
     * Get the process ID.
     */
    int Getpid();

    /**
     * Get the parent process ID.
     */
    int Getppid();

    /**
     * Get the process group ID.
     */
    int Getpgrp();

    /**
     * Get the user ID.
     */
    int Getuid();

    /**
     * Get the effective user ID.
     */
    int Geteuid();

    /**
     * Get the group ID.
     */
    int Getgid();

    /**
     * Get the effective group ID.
     */
    int Getegid();

    /**
     * Get all group IDs.
     */
    std::vector<int> Getgroups();

    // ========== HOSTNAME AND SYSTEM INFO ==========

    /**
     * Get the hostname.
     */
    gocxx::base::Result<std::string> Hostname();

    /**
     * Get page size.
     */
    int Getpagesize();

    // ========== PATHS AND DIRECTORIES ==========

    /**
     * Get the user's home directory.
     */
    gocxx::base::Result<std::string> UserHomeDir();

    /**
     * Get the user's cache directory.
     */
    gocxx::base::Result<std::string> UserCacheDir();

    /**
     * Get the user's config directory.
     */
    gocxx::base::Result<std::string> UserConfigDir();

    /**
     * Get the temporary directory.
     */
    std::string TempDir();

    /**
     * Get the executable path.
     */
    gocxx::base::Result<std::string> Executable();

    // ========== SIGNAL HANDLING ==========

    /**
     * Signal represents an OS signal.
     */
    class Signal {
    public:
        virtual ~Signal() = default;
        virtual std::string String() const = 0;
        virtual int Code() const = 0;
    };

    /**
     * Interrupt signal.
     */
    extern std::shared_ptr<Signal> Interrupt;

    /**
     * Kill signal.
     */
    extern std::shared_ptr<Signal> Kill;

    // ========== PROCESS CONTROL ==========

    /**
     * Exit the program with the given code.
     */
    [[noreturn]] void Exit(int code);

    /**
     * Process state information.
     */
    struct ProcessState {
        int pid;
        bool exited;
        int exitCode;
        std::chrono::system_clock::time_point userTime;
        std::chrono::system_clock::time_point systemTime;
    };

    /**
     * Process represents a running process.
     */
    class Process {
    private:
        int pid_;
        std::shared_ptr<ProcessState> state_;

    public:
        explicit Process(int pid) : pid_(pid), state_(nullptr) {}

        /**
         * Get the process ID.
         */
        int Pid() const { return pid_; }

        /**
         * Kill the process.
         */
        gocxx::base::Result<void> Kill();

        /**
         * Send a signal to the process.
         */
        gocxx::base::Result<void> Signal(std::shared_ptr<os::Signal> sig);

        /**
         * Wait for the process to exit.
         */
        gocxx::base::Result<std::shared_ptr<ProcessState>> Wait();

        /**
         * Release any resources associated with the process.
         */
        gocxx::base::Result<void> Release();
    };

    /**
     * Find the process with the given PID.
     */
    gocxx::base::Result<std::shared_ptr<Process>> FindProcess(int pid);

    /**
     * Start a new process.
     */
    gocxx::base::Result<std::shared_ptr<Process>> StartProcess(
        const std::string& name,
        const std::vector<std::string>& argv,
        const std::function<void()>& setupFunc = nullptr);

    // ========== FILE SYSTEM UTILITIES ==========

    /**
     * Check if a path exists.
     */
    bool PathExists(const std::string& path);

    /**
     * Check if a path is a directory.
     */
    bool IsDir(const std::string& path);

    /**
     * Check if a path is a regular file.
     */
    bool IsFile(const std::string& path);

    /**
     * Get file size.
     */
    gocxx::base::Result<int64_t> FileSize(const std::string& path);

    /**
     * Walk directory tree and call walkFn for each file/directory.
     */
    gocxx::base::Result<void> Walk(
        const std::string& root,
        std::function<gocxx::base::Result<void>(const std::string& path, const FileInfo& info)> walkFn);

    // ========== TEMPORARY FILES ==========

    /**
     * Create a temporary file.
     */
    gocxx::base::Result<std::shared_ptr<File>> CreateTemp(const std::string& dir, const std::string& pattern);

    /**
     * Create a temporary directory.
     */
    gocxx::base::Result<std::string> MkdirTemp(const std::string& dir, const std::string& pattern);

    // ========== ADVANCED FILE OPERATIONS ==========

    /**
     * Create a symbolic link.
     */
    gocxx::base::Result<void> Symlink(const std::string& oldname, const std::string& newname);

    /**
     * Create a hard link.
     */
    gocxx::base::Result<void> Link(const std::string& oldname, const std::string& newname);

    /**
     * Read a symbolic link.
     */
    gocxx::base::Result<std::string> Readlink(const std::string& name);

    /**
     * Change file permissions.
     */
    gocxx::base::Result<void> Chmod(const std::string& name, FileMode mode);

    /**
     * Change file owner.
     */
    gocxx::base::Result<void> Chown(const std::string& name, int uid, int gid);

    /**
     * Change file owner (don't follow symlinks).
     */
    gocxx::base::Result<void> Lchown(const std::string& name, int uid, int gid);

    /**
     * Change access and modification times.
     */
    gocxx::base::Result<void> Chtimes(
        const std::string& name,
        std::chrono::system_clock::time_point atime,
        std::chrono::system_clock::time_point mtime);

    // ========== PIPES ==========

    /**
     * Create a pipe.
     * Returns {read_file, write_file, error}
     */
    gocxx::base::Result<std::pair<std::shared_ptr<File>, std::shared_ptr<File>>> Pipe();

    // ========== UTILITY FUNCTIONS ==========

    /**
     * Check if an error indicates that a file/directory already exists.
     */
    bool IsExist(const std::shared_ptr<gocxx::errors::Error>& err);

    /**
     * Check if an error indicates that a file/directory doesn't exist.
     */
    bool IsNotExist(const std::shared_ptr<gocxx::errors::Error>& err);

    /**
     * Check if an error indicates a permission problem.
     */
    bool IsPermission(const std::shared_ptr<gocxx::errors::Error>& err);

    /**
     * Check if an error indicates a timeout.
     */
    bool IsTimeout(const std::shared_ptr<gocxx::errors::Error>& err);

    /**
     * Check if an error indicates path not found.
     */
    bool IsPathError(const std::shared_ptr<gocxx::errors::Error>& err);

} // namespace gocxx::os

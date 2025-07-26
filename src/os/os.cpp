#include "gocxx/os/os.h"
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <random>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <io.h>
#include <process.h>
#include <userenv.h>
#include <shlobj.h>
#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "shell32.lib")
#define getpid _getpid
#define PATH_MAX _MAX_PATH
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <signal.h>
#include <dirent.h>
#include <limits.h>
extern char** environ;
#endif

namespace gocxx::os {

    // ========== ENVIRONMENT VARIABLES ==========

    std::string Getenv(const std::string& key) {
        const char* value = std::getenv(key.c_str());
        return value ? std::string(value) : std::string();
    }

    std::pair<std::string, bool> LookupEnv(const std::string& key) {
        const char* value = std::getenv(key.c_str());
        if (value) {
            return {std::string(value), true};
        }
        return {"", false};
    }

    gocxx::base::Result<void> Setenv(const std::string& key, const std::string& value) {
#ifdef _WIN32
        if (_putenv_s(key.c_str(), value.c_str()) != 0) {
            return gocxx::base::Result<void>(gocxx::errors::New("failed to set environment variable"));
        }
#else
        if (setenv(key.c_str(), value.c_str(), 1) != 0) {
            return gocxx::base::Result<void>(gocxx::errors::New("failed to set environment variable"));
        }
#endif
        return gocxx::base::Result<void>();
    }

    gocxx::base::Result<void> Unsetenv(const std::string& key) {
#ifdef _WIN32
        if (_putenv_s(key.c_str(), "") != 0) {
            return gocxx::base::Result<void>(gocxx::errors::New("failed to unset environment variable"));
        }
#else
        if (unsetenv(key.c_str()) != 0) {
            return gocxx::base::Result<void>(gocxx::errors::New("failed to unset environment variable"));
        }
#endif
        return gocxx::base::Result<void>();
    }

    gocxx::base::Result<void> Clearenv() {
#ifdef _WIN32
        // Windows doesn't have a direct equivalent, so we manually clear known variables
        auto env = Environ();
        for (const auto& entry : env) {
            auto pos = entry.find('=');
            if (pos != std::string::npos) {
                auto key = entry.substr(0, pos);
                _putenv_s(key.c_str(), "");
            }
        }
#else
        if (clearenv() != 0) {
            return gocxx::base::Result<void>(gocxx::errors::New("failed to clear environment"));
        }
#endif
        return gocxx::base::Result<void>();
    }

    std::vector<std::string> Environ() {
        std::vector<std::string> result;
        
#ifdef _WIN32
        LPWCH env = GetEnvironmentStringsW();
        if (env) {
            LPWCH p = env;
            while (*p) {
                std::wstring wstr(p);
                // Convert to UTF-8
                int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
                std::string str(size - 1, 0);
                WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], size, nullptr, nullptr);
                result.push_back(str);
                p += wstr.length() + 1;
            }
            FreeEnvironmentStringsW(env);
        }
#else
        char** env = environ;
        while (*env) {
            result.emplace_back(*env);
            ++env;
        }
#endif
        return result;
    }

    std::string ExpandEnv(const std::string& s) {
        std::string result;
        result.reserve(s.length() * 2); // Pre-allocate some space
        
        for (size_t i = 0; i < s.length(); ++i) {
            if (s[i] == '$') {
                if (i + 1 < s.length()) {
                    if (s[i + 1] == '{') {
                        // ${VAR} syntax
                        size_t end = s.find('}', i + 2);
                        if (end != std::string::npos) {
                            std::string var = s.substr(i + 2, end - i - 2);
                            result += Getenv(var);
                            i = end;
                            continue;
                        }
                    } else {
                        // $VAR syntax
                        size_t start = i + 1;
                        size_t end = start;
                        while (end < s.length() && 
                               (std::isalnum(s[end]) || s[end] == '_')) {
                            ++end;
                        }
                        if (end > start) {
                            std::string var = s.substr(start, end - start);
                            result += Getenv(var);
                            i = end - 1;
                            continue;
                        }
                    }
                }
            }
            result += s[i];
        }
        return result;
    }

    // ========== PROCESS INFORMATION ==========

    std::vector<std::string> Args() {
        // This would need to be initialized at program start
        // For now, return empty vector
        return {};
    }

    int Getpid() {
        return ::getpid();
    }

    int Getppid() {
#ifdef _WIN32
        return 0; // Windows doesn't have a direct equivalent
#else
        return ::getppid();
#endif
    }

    int Getpgrp() {
#ifdef _WIN32
        return 0; // Windows doesn't have process groups
#else
        return ::getpgrp();
#endif
    }

    int Getuid() {
#ifdef _WIN32
        return 0; // Windows doesn't have UNIX-style UIDs
#else
        return ::getuid();
#endif
    }

    int Geteuid() {
#ifdef _WIN32
        return 0; // Windows doesn't have UNIX-style UIDs
#else
        return ::geteuid();
#endif
    }

    int Getgid() {
#ifdef _WIN32
        return 0; // Windows doesn't have UNIX-style GIDs
#else
        return ::getgid();
#endif
    }

    int Getegid() {
#ifdef _WIN32
        return 0; // Windows doesn't have UNIX-style GIDs
#else
        return ::getegid();
#endif
    }

    std::vector<int> Getgroups() {
#ifdef _WIN32
        return {}; // Windows doesn't have UNIX-style groups
#else
        int ngroups = getgroups(0, nullptr);
        if (ngroups <= 0) return {};
        
        std::vector<gid_t> groups(ngroups);
        if (getgroups(ngroups, groups.data()) == -1) {
            return {};
        }
        
        std::vector<int> result;
        result.reserve(ngroups);
        for (gid_t gid : groups) {
            result.push_back(static_cast<int>(gid));
        }
        return result;
#endif
    }

    // ========== HOSTNAME AND SYSTEM INFO ==========

    gocxx::base::Result<std::string> Hostname() {
#ifdef _WIN32
        DWORD size = 0;
        GetComputerNameExA(ComputerNameDnsHostname, nullptr, &size);
        std::string hostname(size - 1, 0);
        if (!GetComputerNameExA(ComputerNameDnsHostname, &hostname[0], &size)) {
            return {std::string(), gocxx::errors::New("failed to get hostname")};
        }
        return {hostname, nullptr};
#else
        char hostname[HOST_NAME_MAX + 1];
        if (gethostname(hostname, sizeof(hostname)) != 0) {
            return {std::string(), gocxx::errors::New("failed to get hostname")};
        }
        return {std::string(hostname), nullptr};
#endif
    }

    int Getpagesize() {
#ifdef _WIN32
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        return static_cast<int>(si.dwPageSize);
#else
        return ::getpagesize();
#endif
    }

    // ========== PATHS AND DIRECTORIES ==========

    gocxx::base::Result<std::string> UserHomeDir() {
#ifdef _WIN32
        HANDLE token = nullptr;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
            DWORD size = 0;
            GetUserProfileDirectoryA(token, nullptr, &size);
            std::string path(size - 1, 0);
            if (GetUserProfileDirectoryA(token, &path[0], &size)) {
                CloseHandle(token);
                return {path, nullptr};
            }
            CloseHandle(token);
        }
        
        // Fallback to environment variables
        auto home = Getenv("USERPROFILE");
        if (!home.empty()) {
            return {home, nullptr};
        }
        
        auto drive = Getenv("HOMEDRIVE");
        auto path = Getenv("HOMEPATH");
        if (!drive.empty() && !path.empty()) {
            return {drive + path, nullptr};
        }
        
        return {std::string(), gocxx::errors::New("unable to determine user home directory")};
#else
        auto home = Getenv("HOME");
        if (!home.empty()) {
            return {home, nullptr};
        }
        
        struct passwd* pw = getpwuid(getuid());
        if (pw && pw->pw_dir) {
            return {std::string(pw->pw_dir), nullptr};
        }
        
        return {std::string(), gocxx::errors::New("unable to determine user home directory")};
#endif
    }

    gocxx::base::Result<std::string> UserCacheDir() {
#ifdef _WIN32
        // Use APPDATA\Local for cache
        auto appdata = Getenv("LOCALAPPDATA");
        if (!appdata.empty()) {
            return {appdata, nullptr};
        }
        
        // Fallback to APPDATA
        appdata = Getenv("APPDATA");
        if (!appdata.empty()) {
            return {appdata, nullptr};
        }
        
        return {std::string(), gocxx::errors::New("unable to determine cache directory")};
#else
        // Use XDG_CACHE_HOME or fallback to ~/.cache
        auto cache = Getenv("XDG_CACHE_HOME");
        if (!cache.empty()) {
            return {cache, nullptr};
        }
        
        auto home_result = UserHomeDir();
        if (home_result.Failed()) {
            return {std::string(), home_result.err};
        }
        
        return {home_result.value + "/.cache", nullptr};
#endif
    }

    gocxx::base::Result<std::string> UserConfigDir() {
#ifdef _WIN32
        // Use APPDATA for config
        auto appdata = Getenv("APPDATA");
        if (!appdata.empty()) {
            return {appdata, nullptr};
        }
        
        return {std::string(), gocxx::errors::New("unable to determine config directory")};
#else
        // Use XDG_CONFIG_HOME or fallback to ~/.config
        auto config = Getenv("XDG_CONFIG_HOME");
        if (!config.empty()) {
            return {config, nullptr};
        }
        
        auto home_result = UserHomeDir();
        if (home_result.Failed()) {
            return {std::string(), home_result.err};
        }
        
        return {home_result.value + "/.config", nullptr};
#endif
    }

    gocxx::base::Result<std::string> Executable() {
#ifdef _WIN32
        char path[MAX_PATH];
        DWORD size = GetModuleFileNameA(nullptr, path, MAX_PATH);
        if (size == 0 || size == MAX_PATH) {
            return {std::string(), gocxx::errors::New("failed to get executable path")};
        }
        return {std::string(path), nullptr};
#else
        char path[PATH_MAX];
        ssize_t size = readlink("/proc/self/exe", path, sizeof(path) - 1);
        if (size == -1) {
            return {std::string(), gocxx::errors::New("failed to get executable path")};
        }
        path[size] = '\0';
        return {std::string(path), nullptr};
#endif
    }

    // ========== PROCESS CONTROL ==========

    void Exit(int code) {
        std::exit(code);
    }

    // Signal implementations
    class UnixSignal : public Signal {
    private:
        int sig_;
        std::string name_;
    
    public:
        UnixSignal(int sig, const std::string& name) : sig_(sig), name_(name) {}
        
        std::string String() const override { return name_; }
        int Code() const override { return sig_; }
    };

#ifdef _WIN32
    std::shared_ptr<Signal> Interrupt = std::make_shared<UnixSignal>(CTRL_C_EVENT, "interrupt");
    std::shared_ptr<Signal> Kill = std::make_shared<UnixSignal>(0, "kill"); // No direct equivalent
#else
    std::shared_ptr<Signal> Interrupt = std::make_shared<UnixSignal>(SIGINT, "interrupt");
    std::shared_ptr<Signal> Kill = std::make_shared<UnixSignal>(SIGKILL, "killed");
#endif

    // Process implementation methods
    gocxx::base::Result<void> Process::Kill() {
#ifdef _WIN32
        HANDLE handle = OpenProcess(PROCESS_TERMINATE, FALSE, pid_);
        if (handle == nullptr) {
            return gocxx::base::Result<void>(gocxx::errors::New("failed to open process"));
        }
        
        BOOL result = TerminateProcess(handle, 1);
        CloseHandle(handle);
        
        if (!result) {
            return gocxx::base::Result<void>(gocxx::errors::New("failed to kill process"));
        }
        return gocxx::base::Result<void>();
#else
        if (kill(pid_, SIGKILL) != 0) {
            return gocxx::base::Result<void>(gocxx::errors::New("failed to kill process"));
        }
        return gocxx::base::Result<void>::Ok();
#endif
    }

    gocxx::base::Result<void> Process::Signal(std::shared_ptr<os::Signal> sig) {
#ifdef _WIN32
        // Windows has limited signal support
        return gocxx::base::Result<void>(gocxx::errors::New("signals not supported on Windows"));
#else
        if (kill(pid_, sig->Code()) != 0) {
            return gocxx::base::Result<void>(gocxx::errors::New("failed to send signal"));
        }
        return gocxx::base::Result<void>::Ok();
#endif
    }

    gocxx::base::Result<std::shared_ptr<ProcessState>> Process::Wait() {
#ifdef _WIN32
        HANDLE handle = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION, FALSE, pid_);
        if (handle == nullptr) {
            return gocxx::base::Result<std::shared_ptr<ProcessState>>(gocxx::errors::New("failed to open process"));
        }
        
        WaitForSingleObject(handle, INFINITE);
        
        DWORD exitCode;
        GetExitCodeProcess(handle, &exitCode);
        CloseHandle(handle);
        
        auto state = std::make_shared<ProcessState>();
        state->pid = pid_;
        state->exited = true;
        state->exitCode = static_cast<int>(exitCode);
        state->userTime = std::chrono::system_clock::now();
        state->systemTime = std::chrono::system_clock::now();
        
        state_ = state;
        return gocxx::base::Result<std::shared_ptr<ProcessState>>(state);
#else
        int status;
        if (waitpid(pid_, &status, 0) == -1) {
            return gocxx::base::Result<std::shared_ptr<ProcessState>>(gocxx::errors::New("failed to wait for process"));
        }
        
        auto state = std::make_shared<ProcessState>();
        state->pid = pid_;
        state->exited = WIFEXITED(status);
        state->exitCode = WEXITSTATUS(status);
        state->userTime = std::chrono::system_clock::now();
        state->systemTime = std::chrono::system_clock::now();
        
        state_ = state;
        return gocxx::base::Result<std::shared_ptr<ProcessState>>::Ok(state);
#endif
    }

    gocxx::base::Result<void> Process::Release() {
        // In most cases, no explicit cleanup is needed
        return gocxx::base::Result<void>();
    }

    gocxx::base::Result<std::shared_ptr<Process>> FindProcess(int pid) {
#ifdef _WIN32
        HANDLE handle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
        if (handle == nullptr) {
            return gocxx::base::Result<std::shared_ptr<Process>>(gocxx::errors::New("process not found"));
        }
        CloseHandle(handle);
#else
        if (kill(pid, 0) != 0) {
            return gocxx::base::Result<std::shared_ptr<Process>>(gocxx::errors::New("process not found"));
        }
#endif
        return gocxx::base::Result<std::shared_ptr<Process>>(std::make_shared<Process>(pid));
    }

    gocxx::base::Result<std::shared_ptr<Process>> StartProcess(
        const std::string& name,
        const std::vector<std::string>& argv,
        const std::function<void()>& setupFunc) {
        
        // This is a simplified implementation
        // A full implementation would need to handle pipes, environment, etc.
        return gocxx::base::Result<std::shared_ptr<Process>>(gocxx::errors::New("StartProcess not fully implemented"));
    }

    // ========== UTILITY FUNCTIONS ==========

    bool IsDir(const std::string& path) {
        auto result = Stat(path);
        return result.Ok() && result.value.IsDir();
    }

    // Generate a random string for temporary files
    std::string generateRandomString(size_t length) {
        const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, chars.size() - 1);
        
        std::string result;
        result.reserve(length);
        for (size_t i = 0; i < length; ++i) {
            result += chars[dis(gen)];
        }
        return result;
    }

    gocxx::base::Result<std::shared_ptr<File>> CreateTemp(const std::string& dir, const std::string& pattern) {
        std::string tempDir = dir.empty() ? TempDir() : dir;
        
        // Simple pattern replacement: replace * with random string
        std::string filename = pattern;
        size_t pos = filename.find('*');
        if (pos != std::string::npos) {
            filename.replace(pos, 1, generateRandomString(8));
        } else {
            filename += generateRandomString(8);
        }
        
        std::string fullPath = tempDir + "/" + filename;
        
        auto result = OpenFile(fullPath, 
                              static_cast<int>(OpenFlag::RDWR | OpenFlag::CREATE | OpenFlag::EXCL), 
                              0600);
        return result;
    }

    gocxx::base::Result<std::string> MkdirTemp(const std::string& dir, const std::string& pattern) {
        std::string tempDir = dir.empty() ? TempDir() : dir;
        
        // Simple pattern replacement: replace * with random string
        std::string dirname = pattern;
        size_t pos = dirname.find('*');
        if (pos != std::string::npos) {
            dirname.replace(pos, 1, generateRandomString(8));
        } else {
            dirname += generateRandomString(8);
        }
        
        std::string fullPath = tempDir + "/" + dirname;
        
        auto result = Mkdir(fullPath, 0700);
        if (result.Failed()) {
            return {std::string(), result.err};
        }
        
        return {fullPath, nullptr};
    }

} // namespace gocxx::os

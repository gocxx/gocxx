#include <gtest/gtest.h>
#include <gocxx/os/os.h>
#include <gocxx/os/file.h>
#include <thread>
#include <chrono>

using namespace gocxx::os;
using namespace std::chrono_literals;

class OsTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Environment variable tests
TEST_F(OsTest, EnvironmentVariables) {
    // Test setting and getting environment variables
    auto result = Setenv("GOCXX_TEST_VAR", "test_value");
    EXPECT_TRUE(result.Ok());
    
    std::string value = Getenv("GOCXX_TEST_VAR");
    EXPECT_EQ(value, "test_value");
    
    auto [lookupValue, found] = LookupEnv("GOCXX_TEST_VAR");
    EXPECT_TRUE(found);
    EXPECT_EQ(lookupValue, "test_value");
    
    // Test unsetting
    result = Unsetenv("GOCXX_TEST_VAR");
    EXPECT_TRUE(result.Ok());
    
    value = Getenv("GOCXX_TEST_VAR");
    EXPECT_TRUE(value.empty());
}

TEST_F(OsTest, ExpandEnv) {
    Setenv("TEST_VAR", "hello");
    Setenv("TEST_VAR2", "world");
    
    std::string expanded = ExpandEnv("$TEST_VAR ${TEST_VAR2}!");
    EXPECT_EQ(expanded, "hello world!");
    
    expanded = ExpandEnv("No variables here");
    EXPECT_EQ(expanded, "No variables here");
    
    // Cleanup
    Unsetenv("TEST_VAR");
    Unsetenv("TEST_VAR2");
}

// Process information tests
TEST_F(OsTest, ProcessInfo) {
    int pid = Getpid();
    EXPECT_GT(pid, 0);
    
    int pagesize = Getpagesize();
    EXPECT_GT(pagesize, 0);
}

// Directory tests
TEST_F(OsTest, Directories) {
    auto home_result = UserHomeDir();
    if (home_result.Ok()) {
        EXPECT_FALSE(home_result.value.empty());
    }
    
    auto cache_result = UserCacheDir();
    if (cache_result.Ok()) {
        EXPECT_FALSE(cache_result.value.empty());
    }
    
    auto config_result = UserConfigDir();
    if (config_result.Ok()) {
        EXPECT_FALSE(config_result.value.empty());
    }
    
    std::string temp = TempDir();
    EXPECT_FALSE(temp.empty());
}

// File operations tests
TEST_F(OsTest, FileOperations) {
    // Test file creation and writing
    std::string testFile = TempDir() + "/gocxx_test_file.txt";
    std::string testContent = "Hello, World!";
    
    auto write_result = WriteFile(testFile, testContent, 0644);
    EXPECT_TRUE(write_result.Ok());
    
    // Test file reading
    auto read_result = ReadFile(testFile);
    EXPECT_TRUE(read_result.Ok());
    
    std::string content(read_result.value.begin(), read_result.value.end());
    EXPECT_EQ(content, testContent);
    
    // Test file info
    auto stat_result = Stat(testFile);
    EXPECT_TRUE(stat_result.Ok());
    EXPECT_EQ(stat_result.value.Size(), testContent.length());
    EXPECT_FALSE(stat_result.value.IsDir());
    
    // Test file utilities
    EXPECT_TRUE(PathExists(testFile));
    EXPECT_TRUE(IsFile(testFile));
    EXPECT_FALSE(IsDirectory(testFile));
    
    auto size_result = FileSize(testFile);
    EXPECT_TRUE(size_result.Ok());
    EXPECT_EQ(size_result.value, testContent.length());
    
    // Cleanup
    Remove(testFile);
}

TEST_F(OsTest, DirectoryOperations) {
    std::string testDir = TempDir() + "/gocxx_test_dir";
    
    // Clean up any existing directory first
    if (PathExists(testDir)) {
        RemoveAll(testDir);
    }
    
    // Create directory
    auto mkdir_result = Mkdir(testDir, 0755);
    EXPECT_TRUE(mkdir_result.Ok());
    
    // Check directory exists
    EXPECT_TRUE(PathExists(testDir));
    EXPECT_TRUE(IsDirectory(testDir));
    EXPECT_FALSE(IsFile(testDir));
    
    // Get directory info
    auto stat_result = Stat(testDir);
    EXPECT_TRUE(stat_result.Ok());
    EXPECT_TRUE(stat_result.value.IsDir());
    
    // Remove directory  
    auto remove_result = RemoveAll(testDir);
    if (!remove_result.Ok()) {
        std::cout << "RemoveAll failed: " << remove_result.err->error() << std::endl;
    }
    EXPECT_TRUE(remove_result.Ok());
    
    EXPECT_FALSE(PathExists(testDir));
}

TEST_F(OsTest, TemporaryFiles) {
    auto temp_file_result = CreateTemp("", "gocxx_test_*");
    if (temp_file_result.Ok()) {
        auto file = temp_file_result.value;
        EXPECT_FALSE(file->Name().empty());
        
        // Write something to it
        std::string test_data = "temporary file content";
        auto write_result = file->Write(
            reinterpret_cast<const uint8_t*>(test_data.c_str()), 
            test_data.length());
        EXPECT_TRUE(write_result.Ok());
        
        file->close();
        
        // Clean up
        Remove(file->Name());
    }
    
    auto temp_dir_result = MkdirTemp("", "gocxx_test_dir_*");
    if (temp_dir_result.Ok()) {
        std::string temp_dir = temp_dir_result.value;
        EXPECT_TRUE(PathExists(temp_dir));
        EXPECT_TRUE(IsDirectory(temp_dir));
        
        // Clean up
        Remove(temp_dir);
    }
}

// Test hostname
TEST_F(OsTest, Hostname) {
    auto hostname_result = Hostname();
    if (hostname_result.Ok()) {
        EXPECT_FALSE(hostname_result.value.empty());
    }
}

// Test executable path
TEST_F(OsTest, Executable) {
    auto exe_result = Executable();
    if (exe_result.Ok()) {
        EXPECT_FALSE(exe_result.value.empty());
        EXPECT_TRUE(PathExists(exe_result.value));
    }
}

// Error utility tests
TEST_F(OsTest, ErrorUtilities) {
    // Test with a non-existent file
    auto stat_result = Stat("/non/existent/file/path");
    EXPECT_TRUE(stat_result.Failed());
    
    EXPECT_TRUE(IsNotExist(stat_result.err));
    EXPECT_FALSE(IsExist(stat_result.err));
    EXPECT_FALSE(IsPermission(stat_result.err));
}

// Test process finding
TEST_F(OsTest, ProcessManagement) {
    int current_pid = Getpid();
    auto process_result = FindProcess(current_pid);
    EXPECT_TRUE(process_result.Ok());
    
    if (process_result.Ok()) {
        auto process = process_result.value;
        EXPECT_EQ(process->Pid(), current_pid);
    }
}

# gocxx
[![Open Source Helpers](https://www.codetriage.com/gocxx/gocxx/badges/users.svg)](https://www.codetriage.com/gocxx/gocxx)
> 🧩 Go-inspired concurrency primitives and utilities for modern C++

`gocxx` is a C++17 library that brings Go-like concurrency primitives and utilities to C++, including channels, select operations, defer, synchronization tools, JSON processing with nlohmann/json, and more — all built with modern C++ best practices.

---

## 🔍 Why gocxx?

Go's concurrency model and standard library are praised for their:
- Simplicity and elegance
- Powerful communication primitives (channels)
- Clear synchronization patterns
- Intuitive error handling
- Rich standard library with JSON, HTTP, and more

`gocxx` brings that same philosophy to C++, using:
- Clean namespaces (`gocxx::base`, `gocxx::sync`, `gocxx::time`, `gocxx::encoding`)
- Thread-safe communication channels
- Go-like defer mechanism for RAII
- Result types for error handling without exceptions
- Production-ready JSON processing with nlohmann/json backend
- Familiar naming for Go developers working in C++

---

## 🏗️ Getting Started

### Prerequisites

- C++17 compatible compiler (GCC 8+, Clang 7+, MSVC 2019+)
- CMake 3.15 or later

### Dependencies

`gocxx` automatically fetches and manages its dependencies:
- **nlohmann/json** (v3.11.3) - High-performance JSON library for C++
- **Google Test** (for testing, optional)

No manual dependency installation required - CMake handles everything!

### Building

```bash
# Clone the repository
git clone <repository-url>
cd gocxx

# Create build directory
mkdir build && cd build

# Configure (with tests and documentation)
cmake .. -DGOCXX_ENABLE_TESTS=ON -DGOCXX_ENABLE_DOCS=ON

# Build
cmake --build .

# Run tests
ctest

# Generate documentation (optional)
cmake --build . --target docs
```

### Using in Your Project

```cmake
# Add gocxx to your CMake project
add_subdirectory(gocxx)
target_link_libraries(your_target PRIVATE gocxx)
```

### Quick Example

```cpp
#include <gocxx/gocxx.h>
#include <iostream>
#include <thread>

using namespace gocxx::base;
using namespace gocxx::time;
using namespace gocxx::encoding::json;

int main() {
    // JSON Processing with nlohmann/json backend
    JsonValue person;
    person["name"] = "Alice";
    person["age"] = 30;
    person["skills"] = JsonValue::array({"C++", "Go", "Python"});
    
    auto json_result = MarshalString(person);
    if (json_result.Ok()) {
        std::cout << "JSON: " << json_result.value << std::endl;
    }
    
    // Create a buffered channel
    Chan<int> ch(5);
    
    // Producer thread
    std::thread producer([&ch]() {
        for (int i = 0; i < 10; ++i) {
            ch << i;  // Send to channel
        }
        ch.close();
    });
    
    // Consumer - receive from channel
    int value;
    while (ch >> value) {
        std::cout << "Received: " << value << std::endl;
    }
    
    producer.join();
    
    // Defer example - automatic cleanup
    {
        auto* resource = new int(42);
        defer([resource]() {
            delete resource;
            std::cout << "Resource cleaned up!" << std::endl;
        });
        
        // Use resource...
        // Automatic cleanup when leaving scope
    }
    
    return 0;
}
```

## 🚀 Key Features

### JSON Processing
Go-style JSON encoding/decoding with nlohmann/json backend:
```cpp
#include <gocxx/encoding/json.h>
using namespace gocxx::encoding::json;

// Marshal/Unmarshal like Go
JsonValue data;
data["user"]["name"] = "John";
data["user"]["active"] = true;

auto json_str = MarshalString(data);  // Go: json.Marshal()
if (json_str.Ok()) {
    std::cout << json_str.value << std::endl;
}

// Parse JSON
JsonValue parsed;
auto result = UnmarshalString(json_str.value, parsed);  // Go: json.Unmarshal()
if (result.Ok()) {
    std::cout << "Name: " << GetString(parsed["user"]["name"]) << std::endl;
}

// Streaming JSON
auto encoder = NewEncoder(writer);  // Go: json.NewEncoder()
encoder->Encode(data);

auto decoder = NewDecoder(reader);  // Go: json.NewDecoder()
decoder->Decode(parsed);
```

### Channels
Thread-safe communication channels inspired by Go:
```cpp
// Unbuffered channel (synchronous)
Chan<std::string> ch;

// Buffered channel (asynchronous up to buffer size)
Chan<int> buffered_ch(10);

// Select operations
select(
    recv_case(ch1, [](auto value) { /* handle ch1 */ }),
    recv_case(ch2, [](auto value) { /* handle ch2 */ }),
    default_case([]() { /* no channel ready */ })
);
```

### Defer
Automatic cleanup with Go-like defer:
```cpp
{
    auto file = std::fopen("data.txt", "r");
    defer([file]() { std::fclose(file); });
    
    // File automatically closed when leaving scope
}
```

### Synchronization
Go-like synchronization primitives:
```cpp
// WaitGroup for coordinating multiple operations
gocxx::sync::WaitGroup wg;
wg.Add(3);

for (int i = 0; i < 3; ++i) {
    std::thread([&wg, i]() {
        // Do work...
        wg.Done();
    }).detach();
}

wg.Wait(); // Wait for all operations to complete
```

### Time Utilities
Duration and time operations:
```cpp
using namespace gocxx::time;

auto d = Duration::FromSeconds(5);
auto timer = Timer::AfterDuration(d);
// Use timer for time-based operations
```

### Error Handling
Result types for error handling without exceptions:
```cpp
auto result = someOperation();
if (result.Ok()) {
    auto value = result.Unwrap();
    // Use value
} else {
    auto error = result.Error();
    // Handle error
}
```

### Context & Cancellation
Go-like context for cancellation and timeouts:
```cpp
using namespace gocxx::context;

// Create context with timeout
auto [ctx, cancel] = WithTimeout(Background(), std::chrono::seconds(5));

// Use context in operations
if (SleepWithContext(ctx, std::chrono::seconds(10))) {
    std::cout << "Sleep completed" << std::endl;
} else {
    std::cout << "Sleep was canceled" << std::endl;
}

// Manual cancellation
cancel();
```

---

## 📦 Modules

| Module        | Description                               | Status |
|---------------|-------------------------------------------|--------|
| **base**      | Channels, Select, Defer, Result types    | ✅ Implemented |
| **sync**      | Mutex, WaitGroup, Once, synchronization  | ✅ Implemented |
| **time**      | Duration, Timer, Ticker, time utilities  | ✅ Implemented |
| **io**        | Reader, Writer, Copy, I/O interfaces     | ✅ Implemented |
| **os**        | File operations, environment, process    | ✅ Implemented |
| **errors**    | Error creation, wrapping, contextual     | ✅ Implemented |
| **context**   | Cancellation, timeouts, request-scoped   | ✅ Implemented |
| **encoding/json** | JSON parsing, serialization with nlohmann/json | ✅ Implemented |
| **net**       | HTTP client/server, TCP/UDP networking   | 🔜 Planned |

> All modules are integrated in a single library for optimal performance and ease of use.

---

## 🔧 Goals

- ✔️ **Familiar**: Match Go-style naming and concurrency patterns
- ✔️ **Modern**: Use idiomatic C++17 features without overengineering
- ✔️ **Thread-Safe**: All components designed for concurrent use
- ✔️ **Production-Ready**: Built on proven libraries (nlohmann/json)
- ✔️ **Minimal**: Clean API with carefully chosen dependencies
- ✔️ **Cross-platform**: Windows, Linux, macOS support
- ✔️ **Tested**: Comprehensive test suite with 149+ tests

---

## 🏗️ Using gocxx

Each module is self-contained and can be added via CMake.
Check each module's documentation for usage and build instructions.

---

## 🤝 Contributing

We welcome contributions! Start with:

1. Pick an open module (like `net`)
2. Check open issues or propose a design
3. Submit a PR with tests and clean CMake

See [CONTRIBUTING.md](CONTRIBUTING.md) for more.

---

## 📄 License

[Apache License 2.0](LICENSE)

---

## 🧭 Roadmap

- [x] ✅ Core concurrency primitives (channels, select, defer)
- [x] ✅ Synchronization tools (WaitGroup, Mutex, Once)
- [x] ✅ Time utilities (Duration, Timer, Ticker)
- [x] ✅ I/O interfaces and operations
- [x] ✅ OS integration utilities
- [x] ✅ Error handling with Result types
- [x] ✅ Context for cancellation and request-scoped values
- [x] ✅ **JSON module** - Go-style JSON with nlohmann/json backend
- [x] ✅ Comprehensive test suite (149+ tests)
- [x] ✅ API documentation with Doxygen
- [ ] 🚧 Performance optimizations
- [ ] 🔜 **net** module - HTTP client/server, TCP/UDP networking
- [ ] 🔜 Additional examples and tutorials

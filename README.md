# gocxx
[![Open Source Helpers](https://www.codetriage.com/gocxx/gocxx/badges/users.svg)](https://www.codetriage.com/gocxx/gocxx)
> 🧩 Go-inspired modular libraries for modern C++

# gocxx

> ## 🏗️ Getting Started

### Prerequisites

- C++17 compatible compiler (GCC 8+, Clang 7+, MSVC 2019+)
- CMake 3.15 or later

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

int main() {
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
```ncurrency primitives and utilities for modern C++

`gocxx` is a C++17 library that brings Go-like concurrency primitives and utilities to C++, including channels, select operations, defer, synchronization tools, and more — all built with modern C++ best practices.

---

## 🔍 Why gocxx?

Go's concurrency model and standard library are praised for their:
- Simplicity and elegance
- Powerful communication primitives (channels)
- Clear synchronization patterns
- Intuitive error handling

`gocxx` brings that same philosophy to C++, using:
- Clean namespaces (`gocxx::base`, `gocxx::sync`, `gocxx::time`)
- Thread-safe communication channels
- Go-like defer mechanism for RAII
- Result types for error handling without exceptions
- Familiar naming for Go developers working in C++

---

## 🔍 Why gocxx?

Go’s standard library is praised for its:
- Simplicity
- Composability
- Clear interfaces
- Minimalism

`gocxx` brings that same philosophy to C++, using:
- Clean namespaces (`gocxx::io`, `gocxx::net`)
- Interface-based designs (`Reader`, `Writer`, etc.)
- Lightweight, dependency-free components
- Familiar naming for Go developers entering C++

---

## 📦 Modules

| Module      | Description                               | Status |
|-------------|-------------------------------------------|--------|
| **base**    | Channels, Select, Defer, Result types           | ✅ Implemented |
| **sync**    | Mutex, WaitGroup, Once, synchronization         | ✅ Implemented |
| **time**    | Duration, Timer, Ticker, time utilities         | ✅ Implemented |
| **io**      | Reader, Writer, Copy, I/O interfaces            | ✅ Implemented |
| **os**      | File operations, environment, process info      | ✅ Implemented |
| **errors**  | Error creation, wrapping, contextual errors     | ✅ Implemented |
| **net**     | HTTP client/server, TCP/UDP networking          | 🔜 Planned |
| **json**    | JSON parsing, serialization, validation         | 🔜 Planned |

> All modules are integrated in a single library for optimal performance and ease of use.

---

## 🔧 Goals

- ✔️ **Familiar**: Match Go-style naming and concurrency patterns
- ✔️ **Modern**: Use idiomatic C++17 features without overengineering
- ✔️ **Thread-Safe**: All components designed for concurrent use
- ✔️ **Minimal**: Clean API with minimal dependencies
- ✔️ **Cross-platform**: Windows, Linux, macOS support
- ✔️ **Tested**: Comprehensive test suite with 120+ tests

---

## 🏗️ Getting Started

Each module is self-contained and can be added via CMake
Check each module’s README for usage and build instructions.

---

## 🤝 Contributing

We welcome contributions! Start with:

1. Pick an open module (like `net`, `sync`)
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
- [x] ✅ Comprehensive test suite (120+ tests)
- [x] ✅ API documentation with Doxygen
- [ ] 🚧 Performance optimizations
- [ ] 🔜 **net** module - HTTP client/server, TCP/UDP networking
- [ ] 🔜 **json** module - JSON parsing, serialization, validation
- [ ] 🔜 Additional examples and tutorials
- [ ] 🔜 Context and cancellation (planned)

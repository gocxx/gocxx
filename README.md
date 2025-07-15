# gocxx
[![Open Source Helpers](https://www.codetriage.com/gocxx/gocxx/badges/users.svg)](https://www.codetriage.com/gocxx/gocxx)
> 🧩 Go-inspired modular libraries for modern C++

`gocxx` is a collection of small, composable, and idiomatic C++ libraries inspired by the Go standard library — packages like `io`, `net`, `sync`, `errors`, and more — built for modern C++ (C++17 and beyond).

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

| Package     | Description                               | Status |
|-------------|-------------------------------------------|--------|
| [io](https://github.com/gocxx/io)       | `Reader`, `Writer`, `Copy`, etc.                 | 🔜 Planned |
| [net](https://github.com/gocxx/net)     | TCP/UDP, `Dial`, `Listener`, etc.                | 🔜 Planned |
| [sync](https://github.com/gocxx/sync)   | `Mutex`, `Once`, `WaitGroup`, etc.               | 🔜 Planned |
| [errors](https://github.com/gocxx/errors) | Error wrapping, contextual errors                | 🔜 Planned |
| [context](https://github.com/gocxx/context) | Cancellable and timeout-aware operations      | 🔜 Planned |
| [fmt](https://github.com/gocxx/fmt)     | `Printf`-style formatting, stream formatting     | 🔜 Planned |

---

## 🔧 Goals

- ✔️ **Familiar**: Match Go-style naming and interfaces
- ✔️ **Modern**: Use idiomatic C++17+ without overengineering
- ✔️ **Composable**: Small building blocks, not monoliths
- ✔️ **Minimal**: Header-only or tiny binary footprint
- ✔️ **Cross-platform**: Windows, Linux, macOS

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

## 🧭 Roadmap (2025)

- [x] Launch `io`
- [ ] Launch `net`
- [ ] Design `sync` and `errors`
- [ ] Create example apps
- [ ] Publish documentation site (gocxx.dev)

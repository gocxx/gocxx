
/**
 * @file gocxx.h
 * @brief Main header file for the gocxx library
 * 
 * This header includes all the essential components of the gocxx library,
 * providing Go-like concurrency primitives and utilities for C++20.
 * 
 * @author gocxx contributors
 * @version 0.0.1
 * 
 * @example
 * ```cpp
 * #include <gocxx/gocxx.h>
 * 
 * using namespace gocxx::base;
 * 
 * int main() {
 *     Chan<int> ch;
 *     // Use channels, defer, select, etc.
 *     return 0;
 * }
 * ```
 */

#pragma once

// includes all headers for the gocxx library
// This is the main entry point for users of the library

// base
#include <gocxx/base/result.h>
#include <gocxx/base/defer.h>
#include <gocxx/base/chan.h>
#include <gocxx/base/select.h>


// sync
#include <gocxx/sync/sync.h>

// os
#include <gocxx/os/os.h>
#include <gocxx/os/file.h>

// io
#include <gocxx/io/io.h>
#include <gocxx/io/io_errors.h>

// errors
#include <gocxx/errors/errors.h>

// time
#include <gocxx/time/time.h>
#include <gocxx/time/duration.h>
#include <gocxx/time/timer.h>
#include <gocxx/time/ticker.h>

namespace gocxx {
    void anchor();  
}

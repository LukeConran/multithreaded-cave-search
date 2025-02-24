// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// Standard library headers (stable and widely used)
#include <cstdint>      // For uint64_t, uint32_t, etc.
#include <cstring>      // For memcpy, strcmp, etc.
#include <queue>        // For std::queue in Ubreadth
#include <stack>        // For std::stack in Udepth (if implemented)
#include <set>          // For std::set in Discovered
#include <vector>       // For std::vector in main.cpp
#include <thread>       // For std::thread
#include <mutex>        // For std::mutex
#include <atomic>       // For std::atomic in Stats
#include <chrono>       // For std::chrono timing
#include <memory>       // For std::unique_ptr
#include <functional>   // For std::function or comparators in priority_queue (if used)

// Windows-specific headers
#include <Windows.h>    // For HANDLE, DWORD, CreateProcess, etc.

// Project-specific common header
#include "Common.h"     // For CONNECT, DISCONNECT, etc. (assuming it’s stable)


#endif //PCH_H
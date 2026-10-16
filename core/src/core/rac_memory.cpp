#include <atomic>
#include <cstdlib>
#include <cstring>

#include "rac/core/rac_types.h"

namespace {
std::atomic<size_t> g_fail_alloc_count{0};

bool should_fail_alloc() {
    size_t count = g_fail_alloc_count.load(std::memory_order_relaxed);
    while (count > 0) {
        if (g_fail_alloc_count.compare_exchange_weak(count, count - 1, std::memory_order_relaxed)) {
            return true;
        }
    }
    return false;
}
}  // namespace

extern "C" {

/**
 * Configure forced allocation failures for testing.
 */
void rac_test_fail_next_alloc(size_t count) {
    g_fail_alloc_count.store(count, std::memory_order_relaxed);
}

/**
 * Allocate memory using the RAC allocator.
 */
void* rac_alloc(size_t size) {
    if (size == 0) {
        return nullptr;
    }
    if (should_fail_alloc()) {
        return nullptr;
    }
    return malloc(size);
}

/**
 * Free memory allocated by RAC functions.
 * Matches the pattern from Swift's ra_free_string usage.
 */
void rac_free(void* ptr) {
    if (ptr != nullptr) {
        free(ptr);
    }
}

/**
 * Duplicate a string (caller must free with rac_free).
 * Matches Swift interop patterns.
 */
char* rac_strdup(const char* str) {
    if (str == nullptr) {
        return nullptr;
    }
    if (should_fail_alloc()) {
        return nullptr;
    }

    size_t len = strlen(str) + 1;
    char* copy = static_cast<char*>(malloc(len));
    if (copy != nullptr) {
        memcpy(copy, str, len);
    }
    return copy;
}

}  // extern "C"

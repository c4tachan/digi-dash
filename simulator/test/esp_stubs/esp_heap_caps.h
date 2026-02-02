#pragma once
#include <cstdlib>

// Minimal heap_caps stub for host tests
#ifndef MALLOC_CAP_8BIT
#define MALLOC_CAP_8BIT 0
#endif
#ifndef MALLOC_CAP_INTERNAL
#define MALLOC_CAP_INTERNAL 0
#endif

inline void* heap_caps_malloc(size_t size, int /*caps*/) {
    return malloc(size);
}

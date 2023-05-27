#pragma once

#define LOG(x) std::cout << x << std::endl
#define LOG_AND_EXIT(x)          \
    std::cout << x << std::endl; \
    std::exit(0)

#include <cstdint>
#include <vector>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <iterator>

// rotate left
template <class T>
T __ROL__(T value, int count) {
    const uint32_t nbits = sizeof(T) * 8;

    if (count > 0) {
        count %= nbits;
        T high = value >> (nbits - count);
        if (T(-1) < 0) // signed value
            high &= ~((T(-1) << count));
        value <<= count;
        value |= high;
    }
    else {
        count = -count % nbits;
        T low = value << (nbits - count);
        value >>= count;
        value |= low;
    }
    return value;
}

inline uint8_t __ROL1__(uint8_t value, int count) { return __ROL__((uint8_t)value, count); }
inline uint8_t __ROR1__(uint8_t value, int count) { return __ROL__((uint8_t)value, -count); }
inline uint32_t __ROL4__(uint32_t value, int count) { return __ROL__((uint32_t)value, count); }
#pragma once

#include <iostream>
#include <cstdint>
#include <vector>

#define LOG(x) std::cout << x << std::endl
#define LOG_AND_EXIT(x)          \
    std::cout << x << std::endl; \
    std::exit(0)

#include "buffer.hpp"
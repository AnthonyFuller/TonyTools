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
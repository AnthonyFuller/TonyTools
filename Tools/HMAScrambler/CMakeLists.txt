cmake_minimum_required(VERSION 3.15.0)

file(GLOB HMAScrambler_src
    "src/*.cpp"
)

add_executable(HMAScrambler
    ${HMAScrambler_src}
)

target_include_directories(HMAScrambler PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
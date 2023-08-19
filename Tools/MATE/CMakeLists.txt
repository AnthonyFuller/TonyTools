cmake_minimum_required(VERSION 3.15.0)

set(MATE_src 
    "src/main.cpp"
    "src/glob.cpp"
)

add_executable(MATE
    ${MATE_src}
)

target_include_directories(MATE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
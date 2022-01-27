cmake_minimum_required(VERSION 3.15.0)

set(MJBATesting_src 
    "src/main.cpp"
)

add_executable(MJBATesting
    ${MJBATesting_src}
)

target_include_directories(MJBATesting PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
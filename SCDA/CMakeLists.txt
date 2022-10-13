cmake_minimum_required(VERSION 3.15.0)

set(SCDA_src 
    "src/main.cpp"
)

add_executable(SCDA
    ${SCDA_src}
)

target_include_directories(SCDA PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
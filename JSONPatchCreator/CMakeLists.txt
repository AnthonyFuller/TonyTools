cmake_minimum_required(VERSION 3.15.0)

set(JSONPatchCreator_src 
    "src/main.cpp"
    "src/json.hpp"
)

add_executable(JSONPatchCreator
    ${JSONPatchCreator_src}
)

target_include_directories(JSONPatchCreator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
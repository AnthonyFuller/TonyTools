cmake_minimum_required(VERSION 3.15.0)

set(VTXD_src 
    "src/main.cpp"
)

add_executable(VTXD
    ${VTXD_src}
)

target_include_directories(VTXD PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
cmake_minimum_required(VERSION 3.15.0)

# DirectXTex
add_subdirectory("DirectXTex")

# lz4
set(LZ4_HEADERS
    lz4/lz4.h
    lz4/lz4frame_static.h
    lz4/lz4frame.h
    lz4/lz4hc.h
    lz4/xxhash.h
)

set(LZ4_SOURCES
    lz4/lz4.c
    lz4/lz4frame.c
    lz4/lz4hc.c
    lz4/xxhash.c
)
add_library(lz4 STATIC
    ${LZ4_HEADERS}
    ${LZ4_SOURCES}
)
target_include_directories(lz4 PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/lz4"
)

# libmorton
add_library(libmorton INTERFACE)
target_include_directories(libmorton INTERFACE
	"${CMAKE_CURRENT_SOURCE_DIR}/libmorton"
)

# rapidjson
add_library(rapidjson INTERFACE)
target_include_directories(rapidjson INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/rapidjson/include"
)

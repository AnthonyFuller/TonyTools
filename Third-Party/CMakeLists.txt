cmake_minimum_required(VERSION 3.15.0)
include(FetchContent)

# zhmtools/resourcelib
set(BUILD_ZHMTOOLS OFF)
FetchContent_Declare(zhmtools
    GIT_REPOSITORY https://github.com/OrfeasZ/ZHMTools.git
    GIT_TAG v3.2.0
)
FetchContent_MakeAvailable(zhmtools)

# DirectXTex
set(BUILD_TOOLS OFF)
set(BUILD_SAMPLE OFF)
FetchContent_Declare(directxtex
    GIT_REPOSITORY https://github.com/microsoft/DirectXTex.git
    GIT_TAG jan2023
)
FetchContent_MakeAvailable(directxtex)

# libmorton
set(BUILD_TESTING OFF)
FetchContent_Declare(libmorton
    GIT_REPOSITORY https://github.com/Forceflow/libmorton.git
    GIT_TAG v0.2.11
)
FetchContent_MakeAvailable(libmorton)

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

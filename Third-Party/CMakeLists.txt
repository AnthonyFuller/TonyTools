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

# zhmtools/resourcelib
add_library(resourcelib_2016 SHARED IMPORTED)
set_target_properties(resourcelib_2016 PROPERTIES
	IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/zhmtools/ResourceLib_HM2016.lib"
)
add_library(resourcelib_2 SHARED IMPORTED)
set_target_properties(resourcelib_2 PROPERTIES
	IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/zhmtools/ResourceLib_HM2.lib"
)
add_library(resourcelib_3 SHARED IMPORTED)
set_target_properties(resourcelib_3 PROPERTIES
	IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/zhmtools/ResourceLib_HM3.lib"
)

add_library(zhmtools INTERFACE)
target_link_libraries(zhmtools INTERFACE resourcelib_2016 resourcelib_2 resourcelib_3)
target_include_directories(zhmtools INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/zhmtools/include")

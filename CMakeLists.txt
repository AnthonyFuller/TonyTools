cmake_minimum_required(VERSION 3.15.0)
project(TonyTools)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_PDB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}../artifacts)

set(CMAKE_CXX_STANDARD 20)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

# For libmorton to compile
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOMINMAX=1")
endif()

# Add third-party libraries
add_subdirectory("Third-Party")

# Add projects
add_subdirectory("HMTextureTools")
add_subdirectory("HMAScrambler")
add_subdirectory("MJBATesting")
add_subdirectory("BOXCExporter")
add_subdirectory("VTXD")
add_subdirectory("SCDA")
add_subdirectory("JSONPatchCreator")

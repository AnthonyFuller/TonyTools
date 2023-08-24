cmake_minimum_required(VERSION 3.25.0)
include(FetchContent)

project(TonyTools)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0135 NEW)
cmake_policy(SET CMP0077 NEW)
cmake_policy(SET CMP0135 NEW)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_PDB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(CMAKE_CXX_STANDARD 20)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

option(TONYTOOLS_BUILD_TOOLS "Whether or not tools should be built" ON)

if(TONYTOOLS_BUILD_TOOLS)
    # For libmorton to compile
    if(MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOMINMAX=1")
    endif()

    # DirectXTex
    set(BUILD_TOOLS OFF)
    set(BUILD_SAMPLE OFF)
    FetchContent_Declare(directxtex
        GIT_REPOSITORY  https://github.com/microsoft/DirectXTex.git
        GIT_TAG         jan2023
    )

    # libmorton
    set(BUILD_TESTING OFF)
    FetchContent_Declare(libmorton
        GIT_REPOSITORY  https://github.com/Forceflow/libmorton.git
        GIT_TAG         v0.2.11
    )

    # lz4
    set(LZ4_BUILD_CLI OFF)
    set(LZ4_BUILD_LEGACY_LZ4C OFF)
    set(LZ4_BUNDLED_MODE ON)
    FetchContent_Declare(lz4
        GIT_REPOSITORY  https://github.com/lz4/lz4.git
        GIT_TAG         v1.9.4
        SOURCE_SUBDIR   build/cmake
    )

    # argparse
    FetchContent_Declare(argparse
        GIT_REPOSITORY  https://github.com/p-ranav/argparse.git
        GIT_TAG         v2.9
    )

    # bit7z
    FetchContent_Declare(bit7z
        GIT_REPOSITORY  https://github.com/rikyoz/bit7z
        GIT_TAG         94c3c76aa562803ec6686514b410ddf2cd8e4067
    )
endif()

# Currently, only the following external libraries are required for libraries

# Add third-party libraries
# hash library
add_subdirectory("Third-Party")

# zhmtools/resourcelib
set(ZHM_BUILD_TOOLS OFF)
FetchContent_Declare(zhmtools
    GIT_REPOSITORY  https://github.com/OrfeasZ/ZHMTools.git
    GIT_TAG         v3.4.1
)

# nlohmann json
FetchContent_Declare(json
    URL         https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
    URL_HASH    SHA256=8c4b26bf4b422252e13f332bc5e388ec0ab5c3443d24399acb675e68278d341f
)

# Tessil ordered-map (we only use the set)
FetchContent_Declare(tessil
    GIT_REPOSITORY  https://github.com/Tessil/ordered-map.git
    GIT_TAG         v1.0.0
)

if(TONYTOOLS_BUILD_TOOLS)
    FetchContent_MakeAvailable(directxtex libmorton lz4 argparse zhmtools json tessil bit7z)
else()
    FetchContent_MakeAvailable(zhmtools json tessil)
endif()

# Add Libraries and Tools
if(TONYTOOLS_BUILD_TOOLS)
    add_subdirectory("Tools/HMTextureTools")
    add_subdirectory("Tools/HMLanguageTools")
    add_subdirectory("Tools/HMAScrambler")
    add_subdirectory("Tools/MJBATesting")
    add_subdirectory("Tools/BOXCExporter")
    add_subdirectory("Tools/VTXD")
    add_subdirectory("Tools/SCDA")
    add_subdirectory("Tools/JSONPatchCreator")
    add_subdirectory("Tools/GFXFzip")
    add_subdirectory("Tools/MATE")
endif()

add_subdirectory("Libraries/HMLanguages")


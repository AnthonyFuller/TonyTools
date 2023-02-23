cmake_minimum_required(VERSION 3.15.0)

set(HMTextureTools_src
    "src/EncodingDevice.cpp"
    "src/main.cpp"
    "src/Texture.cpp"
)

add_executable(HMTextureTools
    ${HMTextureTools_src}
)

target_include_directories(HMTextureTools PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_dependencies(HMTextureTools DirectXTex lz4_static libmorton argparse)

target_include_directories(HMTextureTools PRIVATE DirectXTex lz4_static)
target_link_libraries(HMTextureTools PRIVATE DirectXTex libmorton::libmorton lz4_static argparse)
cmake_minimum_required(VERSION 3.15.0)

set(GFXFzip_src 
    "src/main.cpp"
    "src/GFXFzip.cpp"
)

add_executable(GFXFzip
    ${GFXFzip_src}
)

target_include_directories(GFXFzip PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_dependencies(GFXFzip ResourceLib_HM2016 ResourceLib_HM2 ResourceLib_HM3 argparse bit7z64 nlohmann_json::nlohmann_json)

target_include_directories(GFXFzip PRIVATE ResourceLib_HM2016 ResourceLib_HM2 ResourceLib_HM3 bit7z64 nlohmann_json::nlohmann_json)
target_link_libraries(GFXFzip PRIVATE ResourceLib_HM2016 ResourceLib_HM2 ResourceLib_HM3 argparse bit7z64 nlohmann_json::nlohmann_json)

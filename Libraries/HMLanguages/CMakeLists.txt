cmake_minimum_required(VERSION 3.25.0)

set(HMLanguages_src
    "src/Languages.cpp"
    "src/bimap.hpp"
    "src/buffer.hpp"
    "src/zip.hpp"
)

set(HMLanguages_hdrs
    "include/tonytools/Languages.h"
)

add_library(HMLanguages STATIC
    ${HMLanguages_src}
    ${HMLanguages_hdrs}
)
add_library(TonyTools::HMLanguages ALIAS HMLanguages)

target_include_directories(HMLanguages
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_dependencies(HMLanguages ResourceLib_HM2016 ResourceLib_HM2 ResourceLib_HM3 nlohmann_json::nlohmann_json hash tsl::ordered_map)

target_link_libraries(HMLanguages PRIVATE ResourceLib_HM2016 ResourceLib_HM2 ResourceLib_HM3 nlohmann_json::nlohmann_json hash tsl::ordered_map)

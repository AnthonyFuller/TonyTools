cmake_minimum_required(VERSION 3.15.0)

set(HMLanguageTools_src
    "src/main.cpp"
    "src/Language.cpp"
)

add_executable(HMLanguageTools
    ${HMLanguageTools_src}
)

target_include_directories(HMLanguageTools PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_dependencies(HMLanguageTools argparse HMLanguages)

target_link_libraries(HMLanguageTools PRIVATE argparse HMLanguages)

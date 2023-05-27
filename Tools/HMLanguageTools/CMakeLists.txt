cmake_minimum_required(VERSION 3.15.0)

set(HMLanguageTools_src
    "src/main.cpp"
)

add_executable(HMLanguageTools
    ${HMLanguageTools_src}
)

add_dependencies(HMLanguageTools argparse HMLanguages)

target_link_libraries(HMLanguageTools PRIVATE argparse HMLanguages)

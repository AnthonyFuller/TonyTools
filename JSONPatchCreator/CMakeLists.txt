cmake_minimum_required(VERSION 3.15.0)

set(JSONPatchCreator_src 
    "src/main.cpp"
)

add_executable(JSONPatchCreator
    ${JSONPatchCreator_src}
)

add_dependencies(JSONPatchCreator nlohmann_json::nlohmann_json)

target_include_directories(JSONPatchCreator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(JSONPatchCreator PRIVATE nlohmann_json::nlohmann_json)

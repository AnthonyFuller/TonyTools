cmake_minimum_required(VERSION 3.15.0)

set(BOXCExporter_src 
    "src/main.cpp"
)

add_executable(BOXCExporter
    ${BOXCExporter_src}
)

target_include_directories(BOXCExporter PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_dependencies(BOXCExporter DirectXTex)

target_include_directories(BOXCExporter PRIVATE DirectXTex rapidjson)
target_link_libraries(BOXCExporter PRIVATE DirectXTex rapidjson)

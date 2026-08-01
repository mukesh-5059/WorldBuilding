cmake_minimum_required(VERSION 3.10.0)
project(WorldBuilder VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE src src/*.cpp)
file(GLOB imgui dependencies/lib/imgui/*.cpp)
file(GLOB rlimgui dependencies/lib/rlimgui/*.cpp)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE ${imgui} ${rlimgui} ${src})

target_include_directories(${PROJECT_NAME} PRIVATE dependencies/includes)
target_include_directories(${PROJECT_NAME} PRIVATE src/includes)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/lib/raylib/libraylib.a"
    GL
    m
    pthread
    dl
    rt
    X11
)
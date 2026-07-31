cmake_minimum_required(VERSION 3.10.0)
project(WorldBuilder VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB imgui dependencies/lib/imgui/*.cpp)
file(GLOB rlimgui dependencies/lib/rlimgui/*.cpp)

file(GLOB core_src src/core/*.cpp)
file(GLOB world_builder_src src/apps/WorldBuilder/*.cpp)
file(GLOB landmass_testing_src src/apps/LandmassTesting/*.cpp)

set(COMMON_INCLUDES
    dependencies/includes
    src
    src/includes
    src/core
    src/apps/WorldBuilder
    src/apps/LandmassTesting
)

set(COMMON_LIBS
    "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/lib/raylib/libraylib.a"
    GL
    m
    pthread
    dl
    rt
    X11
)

# Main WorldBuilder executable target
add_executable(WorldBuilder ${imgui} ${rlimgui} ${core_src} ${world_builder_src})
target_include_directories(WorldBuilder PRIVATE ${COMMON_INCLUDES})
target_link_libraries(WorldBuilder PRIVATE ${COMMON_LIBS})

# LandmassTesting mini app executable target
add_executable(LandmassTesting ${imgui} ${rlimgui} ${core_src} ${landmass_testing_src})
target_include_directories(LandmassTesting PRIVATE ${COMMON_INCLUDES})
target_link_libraries(LandmassTesting PRIVATE ${COMMON_LIBS})
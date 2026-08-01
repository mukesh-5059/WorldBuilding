cmake_minimum_required(VERSION 3.10.0)
project(WorldBuilder VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Common sources (Core UI, ImGui, rlImGui wrapper)
file(GLOB core_src src/core/*.cpp)
file(GLOB imgui dependencies/lib/imgui/*.cpp)
file(GLOB rlimgui dependencies/lib/rlimgui/*.cpp)

# Shared include directories
set(SHARED_INCLUDES
    dependencies/includes
    src/includes
)

# Shared libraries
set(SHARED_LIBS
    "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/lib/raylib/libraylib.a"
    GL
    m
    pthread
    dl
    rt
    X11
)

# --- 1. Target: WorldBuilder ---
file(GLOB world_builder_src src/apps/WorldBuilder/*.cpp)
add_executable(WorldBuilder ${imgui} ${rlimgui} ${core_src} ${world_builder_src})
target_include_directories(WorldBuilder PRIVATE ${SHARED_INCLUDES})
target_link_libraries(WorldBuilder PRIVATE ${SHARED_LIBS})

# --- 2. Target: VoronoiTesting ---
file(GLOB voronoi_testing_src src/apps/VoronoiTesting/*.cpp)
add_executable(VoronoiTesting ${imgui} ${rlimgui} ${core_src} ${voronoi_testing_src})
target_include_directories(VoronoiTesting PRIVATE ${SHARED_INCLUDES} src/apps/VoronoiTesting)
target_link_libraries(VoronoiTesting PRIVATE ${SHARED_LIBS})
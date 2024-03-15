cmake_minimum_required(VERSION 3.21)
project(PulseKey)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable link time optimization
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

# Stripping unused secctions and functions
add_compile_options(-ffunction-sections -fdata-sections)
add_link_options(-Wl,--gc-sections)

# Strip debug info
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

# Use static linking (embedding DLL files in binary)
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")

# Find packages
list(APPEND CMAKE_PREFIX_PATH "C:/vcpkg/installed/x64-mingw-static/share/sdl2")
list(APPEND CMAKE_PREFIX_PATH "C:/vcpkg/installed/x64-mingw-static/share/yaml-cpp")
find_package(SDL2 CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

# Add ImGui source files
set(IMGUI_SOURCES
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_opengl2.cpp
    imgui/backends/imgui_impl_sdl2.cpp
)

# Specify your source files
set(SOURCE_FILES
    src/main.cpp
    src/game_config.cpp
    ${IMGUI_SOURCES}
    resources/icon.rc
)

# Define the executable
add_executable(PulseKey WIN32 ${SOURCE_FILES})

# Include directories
target_include_directories(PulseKey PRIVATE ${SDL2_INCLUDE_DIRS} ${YAML_CPP_INCLUDE_DIR} include imgui imgui/backends)

# Link libraries
target_link_libraries(PulseKey PRIVATE SDL2::SDL2 OpenGL::GL yaml-cpp::yaml-cpp)
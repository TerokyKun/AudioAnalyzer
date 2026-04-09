cmake_minimum_required(VERSION 3.10)
project(AudioVisualizer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ===============================================================
# Include Directories
# ===============================================================
include_directories(
    ${CMAKE_SOURCE_DIR}/Dependencies/glm
    ${CMAKE_SOURCE_DIR}/Dependencies/glad/include
    ${CMAKE_SOURCE_DIR}/Dependencies/glfw/include
    ${CMAKE_SOURCE_DIR}/Dependencies/miniaudio
    ${CMAKE_SOURCE_DIR}/Audio
)

# ===============================================================
# GLFW
# ===============================================================
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/Dependencies/glfw)

# ===============================================================
# GLAD
# ===============================================================
add_library(glad STATIC ${CMAKE_SOURCE_DIR}/Dependencies/glad/src/glad.c)
target_include_directories(glad PUBLIC ${CMAKE_SOURCE_DIR}/Dependencies/glad/include)

# ===============================================================
# MINIAUDIO
# ===============================================================
add_library(miniaudio STATIC
    ${CMAKE_SOURCE_DIR}/Dependencies/miniaudio/miniaudio_impl.cpp
)
target_include_directories(miniaudio PUBLIC ${CMAKE_SOURCE_DIR}/Dependencies/miniaudio)

# ===============================================================
# AudioEngine / FFT / Analyzer / BeatDetector / Visualizer
# ===============================================================
add_library(AudioEngine
    ${CMAKE_SOURCE_DIR}/Audio/AudioEngine.cpp
    ${CMAKE_SOURCE_DIR}/Audio/FFT.cpp
    ${CMAKE_SOURCE_DIR}/Audio/Analyzer.cpp
    ${CMAKE_SOURCE_DIR}/Audio/BeatDetector.cpp
    ${CMAKE_SOURCE_DIR}/Audio/Visualizer.cpp
)

target_include_directories(AudioEngine PUBLIC
    ${CMAKE_SOURCE_DIR}/Audio
)

target_link_libraries(AudioEngine PUBLIC
    glad
    miniaudio
)

# ===============================================================
# Executable
# ===============================================================
add_executable(AudioVisualizer
    ${CMAKE_SOURCE_DIR}/main.cpp
)

target_link_libraries(AudioVisualizer PRIVATE
    AudioEngine
    glad
    glfw
)

# Windows OpenGL
if (WIN32)
    target_link_libraries(AudioVisualizer PRIVATE opengl32)
endif()

# ===============================================================
# Output directory
# ===============================================================
set_target_properties(AudioVisualizer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
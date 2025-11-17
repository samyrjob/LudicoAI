cmake_minimum_required(VERSION 3.15)
project(visualia C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# Platform detection
if(APPLE)
    set(PLATFORM_MACOS ON)
    add_definitions(-DPLATFORM_MACOS)
elseif(WIN32)
    set(PLATFORM_WINDOWS ON)
    add_definitions(-DPLATFORM_WINDOWS)
elseif(UNIX)
    set(PLATFORM_LINUX ON)
    add_definitions(-DPLATFORM_LINUX)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/backend/include)

# Whisper.cpp
set(WHISPER_BUILD_TESTS OFF CACHE BOOL "")
set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "")
add_subdirectory(backend/libs/whisper.cpp EXCLUDE_FROM_ALL)

# Llama.cpp (for future LLM integration)
set(LLAMA_BUILD_TESTS OFF CACHE BOOL "")
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "")
set(LLAMA_BUILD_SERVER OFF CACHE BOOL "")
add_subdirectory(backend/libs/llama.cpp EXCLUDE_FROM_ALL)

# Source files
set(SOURCES
    backend/src/main.c
    backend/src/audio.c
    backend/src/whisper_engine.c
    backend/src/ipc.c
)

# Main executable
add_executable(visualia ${SOURCES})

# Link libraries
target_link_libraries(visualia PRIVATE whisper llama)

# Platform-specific libraries
if(PLATFORM_MACOS)
    target_link_libraries(visualia PRIVATE "-framework CoreAudio" "-framework AudioToolbox" "-framework CoreFoundation")
elseif(PLATFORM_LINUX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PULSEAUDIO REQUIRED libpulse-simple)
    target_include_directories(visualia PRIVATE ${PULSEAUDIO_INCLUDE_DIRS})
    target_link_libraries(visualia PRIVATE ${PULSEAUDIO_LIBRARIES})
elseif(PLATFORM_WINDOWS)
    target_link_libraries(visualia PRIVATE winmm)
endif()

# Threading
find_package(Threads REQUIRED)
target_link_libraries(visualia PRIVATE Threads::Threads)

# Install
install(TARGETS visualia DESTINATION bin)

cmake_minimum_required(VERSION 3.20)
project(ByteShrink VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler optimization and warning flags
if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8 /O2)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O2)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 1. Core Huffman DSA & File Format Library
set(CORE_SOURCES
    src/HuffmanNode.cpp
    src/HuffmanTree.cpp
    src/Checksum.cpp
    src/BitWriter.cpp
    src/BitReader.cpp
    src/ArchiveFormat.cpp
    src/CompressionStats.cpp
    src/FileUtils.cpp
    src/HuffmanEncoder.cpp
    src/HuffmanDecoder.cpp
)

add_library(huffman_core STATIC ${CORE_SOURCES})

# 2. Main ByteShrink CLI Engine
add_executable(ByteShrink src/main.cpp)
target_link_libraries(ByteShrink PRIVATE huffman_core)

# Compatibility target
add_executable(HuffmanX src/main.cpp)
target_link_libraries(HuffmanX PRIVATE huffman_core)

# 3. Automated Verification Test Suite Executable
add_executable(run_tests tests/test_suite.cpp)
target_link_libraries(run_tests PRIVATE huffman_core)

enable_testing()
add_test(NAME ComprehensiveVerificationSuite COMMAND run_tests)

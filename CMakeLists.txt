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
    ${CMAKE_CURRENT_SOURCE_DIR}/gui
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backends
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

# 2. Dear ImGui UI Library
set(IMGUI_SOURCES
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_win32.cpp
    third_party/imgui/backends/imgui_impl_dx11.cpp
)

add_library(imgui_lib STATIC ${IMGUI_SOURCES})

# 3. Main Desktop GUI Application
set(APP_SOURCES
    src/main.cpp
    gui/AppWindow.cpp
    gui/Theme.cpp
    gui/TreeRenderer.cpp
    gui/NativeFileDialog.cpp
)

add_executable(ByteShrink ${APP_SOURCES})
target_link_libraries(ByteShrink PRIVATE huffman_core imgui_lib)

if(WIN32)
    target_link_libraries(ByteShrink PRIVATE
        d3d11
        dxgi
        d3dcompiler
        dwmapi
        comdlg32
        shell32
        ole32
    )
endif()

# Also build HuffmanX target for compatibility
add_executable(HuffmanX ${APP_SOURCES})
target_link_libraries(HuffmanX PRIVATE huffman_core imgui_lib)
if(WIN32)
    target_link_libraries(HuffmanX PRIVATE d3d11 dxgi d3dcompiler dwmapi comdlg32 shell32 ole32)
endif()

# 4. Automated Verification Test Suite Executable
add_executable(run_tests tests/test_suite.cpp)
target_link_libraries(run_tests PRIVATE huffman_core)

enable_testing()
add_test(NAME ComprehensiveVerificationSuite COMMAND run_tests)

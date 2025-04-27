cmake_minimum_required(VERSION 3.15)

# Project name and language
project(DX12VectorAdd LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set target architecture to x64
set(CMAKE_GENERATOR_PLATFORM x64)


# Add the source file
add_executable(DX12VectorAdd VectorMulAdd.cpp)
# add_executable(DX12VectorAdd main.cpp)

# Include DirectX 12 and Windows headers
find_path(DIRECTX_INCLUDE_DIR d3d12.h)
find_library(DIRECTX_LIB_D3D12 d3d12.lib  PATHS "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64" REQUIRED)
find_library(DIRECTX_LIB_DXGI dxgi.lib  PATHS "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64"  REQUIRED)
find_library(DIRECTX_LIB_D3DCOMPILER d3dcompiler.lib  PATHS "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64"  REQUIRED)

if (NOT DIRECTX_INCLUDE_DIR OR NOT DIRECTX_LIB_D3D12 OR NOT DIRECTX_LIB_DXGI OR NOT DIRECTX_LIB_D3DCOMPILER)
    message(FATAL_ERROR "DirectX 12 SDK not found. Please ensure the Windows SDK is installed.")
endif()

message(STATUS "DirectX Include Directory: ${DIRECTX_INCLUDE_DIR}")
message(STATUS "DirectX D3D12 Library: ${DIRECTX_LIB_D3D12}")
message(STATUS "DirectX DXGI Library: ${DIRECTX_LIB_DXGI}")
message(STATUS "DirectX D3DCompiler Library: ${DIRECTX_LIB_D3DCOMPILER}")

add_subdirectory(third_party/DirectX-Headers)
target_include_directories(DX12VectorAdd PRIVATE third_party/DirectX-Headers/include/directx)

# Include directories
target_include_directories(DX12VectorAdd PRIVATE ${DIRECTX_INCLUDE_DIR})

# Link libraries
target_link_libraries(DX12VectorAdd PRIVATE ${DIRECTX_LIB_D3D12} ${DIRECTX_LIB_DXGI} ${DIRECTX_LIB_D3DCOMPILER})

# Enable debug symbols for debug builds
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(DX12VectorAdd PRIVATE _DEBUG)
endif()
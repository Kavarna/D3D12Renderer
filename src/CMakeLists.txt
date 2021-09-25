cmake_minimum_required(VERSION 3.8)
project(D3D12Renderer)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_definitions(-DGLOG_NO_ABBREVIATED_SEVERITIES)

FILE(GLOB SOURCES "*.cpp" "*.h")
FILE(GLOB COMMON "common/*.cpp" "common/*.h")
FILE(GLOB_RECURSE CORE_SRC "Core/*.cpp" "Core/*.h")
FILE(GLOB_RECURSE GRAPHICS_SRC "Graphics/*.cpp" "Graphics/*.h")

include_directories("Graphics")
include_directories("Core")

add_executable(D3D12Renderer ${SOURCES} ${CORE_SRC} ${COMMON} ${GRAPHICS_SRC})
target_link_libraries(D3D12Renderer ${CONAN_LIBS} "d3d12.lib" "dxgi.lib" "d3dcompiler.lib" "dxguid.lib")

make_filters("${CORE_SRC}")
make_filters("${COMMON}")
make_filters("${SOURCES}")
make_filters("${GRAPHICS_SRC}")

set_property(TARGET D3D12Renderer PROPERTY CXX_STANDARD 17)

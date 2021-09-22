cmake_minimum_required(VERSION 3.8)
project(D3D12Renderer)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_definitions(-DGLOG_NO_ABBREVIATED_SEVERITIES)

include_directories("src/common")

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

macro(make_filters _source_list)
    foreach(_source IN ITEMS ${_source_list})
        get_filename_component(_source_path "${_source}" PATH)
        string(REPLACE "${CMAKE_SOURCE_DIR}" "" _group_path "${_source_path}")
        string(REPLACE "/" "\\" _group_path "${_group_path}")
        source_group("${_group_path}" FILES "${_source}")
    endforeach()
endmacro()

add_subdirectory("src")

make_filters("${CORE_SRC}")
make_filters("${COMMON}")
make_filters("${SOURCES}")

set_property(TARGET D3D12Renderer PROPERTY CXX_STANDARD 17)


set(CMAKE_INSTALL_PREFIX ../bin)

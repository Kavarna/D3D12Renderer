cmake_minimum_required(VERSION 3.8)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
project(D3D12Renderer)

include_directories("src/common")
include_directories("src/Graphics")
include_directories("src/Input")
include_directories("src/Core")

FILE(GLOB SOURCES "src/*.cpp" "src/*.h")
FILE(GLOB COMMON "src/common/*.cpp" "src/common/*.h")
FILE(GLOB_RECURSE CORE_SRC "src/Core/*.cpp" "src/Core/*.h")
FILE(GLOB_RECURSE GRAPHICS_SRC "src/Graphics/*.cpp" "src/Graphics/*.h")
FILE(GLOB_RECURSE INPUT_SRC "src/Input/*.cpp" "src/Input/*.h")
FILE(GLOB_RECURSE SHADERS_COMMON "src/Shaders/*.hlsli")
FILE(GLOB_RECURSE PIXEL_SHADERS "src/Shaders/Pixel*.hlsl")
FILE(GLOB_RECURSE VERTEX_SHADERS "src/Shaders/Vertex*.hlsl")
FILE(GLOB_RECURSE COMPUTE_SHADERS "src/Shaders/Compute*.hlsl")
FILE(GLOB_RECURSE GEOMETRY_SHADERS "src/Shaders/Geometry*.hlsl")
FILE(GLOB_RECURSE DOMAIN_SHADERS "src/Shaders/Domain*.hlsl")
FILE(GLOB_RECURSE HULL_SHADERS "src/Shaders/Hull*.hlsl")

add_library(D3D12Renderer
            ${SOURCES}
            ${CORE_SRC}
            ${COMMON}
            ${GRAPHICS_SRC}
            ${INPUT_SRC}
            ${PIXEL_SHADERS}
            ${VERTEX_SHADERS}
            ${COMPUTE_SHADERS}
            ${GEOMETRY_SHADERS}
            ${DOMAIN_SHADERS}
            ${HULL_SHADERS}
            ${SHADERS_COMMON})



make_filters("${VERTEX_SHADERS}")
make_filters("${PIXEL_SHADERS}")
make_filters("${COMPUTE_SHADERS}")
make_filters("${GEOMETRY_SHADERS}")
make_filters("${DOMAIN_SHADERS}")
make_filters("${HULL_SHADERS}")
make_filters("${SHADERS_COMMON}")
make_filters("${CORE_SRC}")
make_filters("${COMMON}")
make_filters("${SOURCES}")
make_filters("${GRAPHICS_SRC}")
make_filters("${INPUT_SRC}")

target_link_libraries(D3D12Renderer ${CONAN_LIBS} "d3d12.lib" "dxgi.lib" "d3dcompiler.lib" "dxguid.lib" "Common")

set_property(TARGET D3D12Renderer PROPERTY CXX_STANDARD 20)

macro(prepare_shaders _source_list _shader_type _shader_model)
    foreach(_source IN ITEMS ${_source_list})
        
        set_property(SOURCE ${_source} PROPERTY VS_SHADER_TYPE "${_shader_type}")
        set_property(SOURCE ${_source} PROPERTY VS_SHADER_MODEL "${_shader_model}")

        if (CMAKE_BUILD_TYPE MATCHES "debug")
            # Add debug info for shaders
            set_property(SOURCE ${_source} PROPERTY VS_SHADER_FLAGS "/Zi /Od")
        endif ()

        # Output filename
        get_filename_component(filename  "${_source}" NAME_WE)
        get_filename_component(abs_directory "${_source}" DIRECTORY)
        get_filename_component(directory "${abs_directory}" NAME)
        set_property(SOURCE ${_source} PROPERTY VS_SHADER_OBJECT_FILE_NAME
                     "${CURRENT_WORKING_DIRECTORY}\\Shaders\\${directory}_${filename}.cso")
    endforeach()
endmacro()

prepare_shaders("${PIXEL_SHADERS}" "Pixel" "6.0")
prepare_shaders("${VERTEX_SHADERS}" "Vertex" "6.0")
prepare_shaders("${COMPUTE_SHADERS}" "Compute" "6.0")
prepare_shaders("${GEOMETRY_SHADERS}" "Geometry" "6.0")
prepare_shaders("${DOMAIN_SHADERS}" "Domain" "6.0")
prepare_shaders("${HULL_SHADERS}" "Hull" "6.0")

set(CMAKE_INSTALL_PREFIX ../bin)

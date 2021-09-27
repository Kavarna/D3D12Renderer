cmake_minimum_required(VERSION 3.8)
project(D3D12Renderer)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_definitions(-DGLOG_NO_ABBREVIATED_SEVERITIES)

FILE(GLOB SOURCES "*.cpp" "*.h")
FILE(GLOB COMMON "common/*.cpp" "common/*.h")
FILE(GLOB_RECURSE CORE_SRC "Core/*.cpp" "Core/*.h")
FILE(GLOB_RECURSE GRAPHICS_SRC "Graphics/*.cpp" "Graphics/*.h")
FILE(GLOB_RECURSE SHADERS_COMMON "Shaders/*.hlsli")
FILE(GLOB_RECURSE PIXEL_SHADERS "Shaders/Pixel*.hlsl")
FILE(GLOB_RECURSE VERTEX_SHADERS "Shaders/Vertex*.hlsl")

include_directories("Graphics")
include_directories("Core")

add_executable(D3D12Renderer ${SOURCES} ${CORE_SRC} ${COMMON} ${GRAPHICS_SRC} ${PIXEL_SHADERS} ${VERTEX_SHADERS} ${SHADERS_COMMON})
target_link_libraries(D3D12Renderer ${CONAN_LIBS} "d3d12.lib" "dxgi.lib" "d3dcompiler.lib" "dxguid.lib")

make_filters("${PIXEL_SHADERS}")
make_filters("${VERTEX_SHADERS}")
make_filters("${SHADERS_COMMON}")
make_filters("${CORE_SRC}")
make_filters("${COMMON}")
make_filters("${SOURCES}")
make_filters("${GRAPHICS_SRC}")

set_property(TARGET D3D12Renderer PROPERTY CXX_STANDARD 17)

set_property(TARGET D3D12Renderer PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CURRENT_WORKING_DIRECTORY}")
message("${CURRENT_WORKING_DIRECTORY}")

macro(prepare_shaders _source_list _shader_type _shader_model)
    foreach(_source IN ITEMS ${_source_list})
        
        set_property(SOURCE ${_source} PROPERTY VS_SHADER_TYPE "${_shader_type}")
        # ps_6_0
        # set_property(SOURCE ${_source} PROPERTY VS_SHADER_PROFILE "${_shader_type}")
        set_property(SOURCE ${_source} PROPERTY VS_SHADER_MODEL "${_shader_model}")

        # Output filename
        get_filename_component(filename  "${_source}" NAME_WE)
        get_filename_component(abs_directory "${_source}" DIRECTORY)
        get_filename_component(directory "${abs_directory}" NAME)

        message("Final path = ${CURRENT_WORKING_DIRECTORY}\\Shaders\\${directory}_${filename}.cso")
        set_property(SOURCE ${_source} PROPERTY VS_SHADER_OBJECT_FILE_NAME
                     "${CURRENT_WORKING_DIRECTORY}\\Shaders\\${directory}_${filename}.cso")
    endforeach()
endmacro()

prepare_shaders("${PIXEL_SHADERS}" "Pixel" "6.0")
prepare_shaders("${VERTEX_SHADERS}" "Vertex" "6.0")

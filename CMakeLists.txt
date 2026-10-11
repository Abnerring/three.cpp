cmake_minimum_required(VERSION 3.20)
project(THREECPP VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
option(THREECPP_BUILD_TESTS "Build CPU tests and three.js differential checks" ON)
option(THREECPP_BUILD_VULKAN "Build the Vulkan renderer and rendering checks" ON)
set(THREEJS_SOURCE_DIR "" CACHE PATH "Pinned upstream three.js checkout for differential tests")

add_library(threecpp
    src/math/MathUtils.cpp src/math/Vector2.cpp src/math/Vector3.cpp src/math/Vector4.cpp
    src/math/Matrix3.cpp src/math/Matrix4.cpp src/math/Quaternion.cpp src/math/Euler.cpp
    src/math/Color.cpp src/math/Box3.cpp src/math/Sphere.cpp
    src/math/Box2.cpp src/math/Cylindrical.cpp src/math/Spherical.cpp src/math/Matrix2.cpp
    src/math/Plane.cpp src/math/Line3.cpp src/math/Triangle.cpp src/math/Ray.cpp src/math/Frustum.cpp
    src/core/EventDispatcher.cpp src/core/Object3D.cpp
    src/core/BufferAttribute.cpp src/core/BufferGeometry.cpp
    src/core/Raycaster.cpp
    src/cameras/Camera.cpp src/cameras/PerspectiveCamera.cpp src/cameras/OrthographicCamera.cpp
    src/scenes/Scene.cpp src/geometries/PlaneGeometry.cpp src/geometries/BoxGeometry.cpp
    src/materials/Material.cpp src/materials/MeshBasicMaterial.cpp src/objects/Mesh.cpp
    src/objects/MeshRaycast.cpp
)
add_library(threecpp::threecpp ALIAS threecpp)
target_include_directories(threecpp PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
target_compile_features(threecpp PUBLIC cxx_std_20)
if(MSVC)
    target_compile_options(threecpp PUBLIC /utf-8 /permissive-)
    target_compile_options(threecpp PRIVATE /W4 /fp:strict)
else()
    target_compile_options(threecpp PRIVATE -Wall -Wextra -Wpedantic -ffp-contract=off)
endif()
add_executable(threecpp_example src/main.cpp)
set_target_properties(threecpp_example PROPERTIES OUTPUT_NAME THREECPP)
target_link_libraries(threecpp_example PRIVATE threecpp::threecpp)

if(THREECPP_BUILD_VULKAN)
    find_path(THREECPP_VULKAN_INCLUDE_DIR vulkan/vulkan.h HINTS "$ENV{VULKAN_SDK}" PATH_SUFFIXES Include include)
    find_program(THREECPP_GLSLANG_VALIDATOR NAMES glslangValidator HINTS "$ENV{VULKAN_SDK}" PATH_SUFFIXES Bin bin)
    if(NOT THREECPP_VULKAN_INCLUDE_DIR OR NOT THREECPP_GLSLANG_VALIDATOR)
        message(FATAL_ERROR "Vulkan requires headers and glslangValidator. Set THREECPP_VULKAN_INCLUDE_DIR and THREECPP_GLSLANG_VALIDATOR, or install the Vulkan SDK.")
    endif()
    set(THREECPP_SHADER_OUTPUTS)
    foreach(shader basic.vert basic.frag)
        set(output "${CMAKE_CURRENT_BINARY_DIR}/shaders/${shader}.spv")
        add_custom_command(OUTPUT "${output}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
            COMMAND "${THREECPP_GLSLANG_VALIDATOR}" -V --target-env vulkan1.1 "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${shader}" -o "${output}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${shader}" VERBATIM)
        list(APPEND THREECPP_SHADER_OUTPUTS "${output}")
    endforeach()
    add_custom_target(threecpp_shaders DEPENDS ${THREECPP_SHADER_OUTPUTS})
    add_library(threecpp_vulkan src/renderers/VulkanRenderer.cpp)
    add_library(threecpp::vulkan ALIAS threecpp_vulkan)
    target_link_libraries(threecpp_vulkan PUBLIC threecpp::threecpp PRIVATE ${CMAKE_DL_LIBS})
    target_include_directories(threecpp_vulkan PRIVATE "${THREECPP_VULKAN_INCLUDE_DIR}")
    target_compile_definitions(threecpp_vulkan PRIVATE THREECPP_SHADER_DIRECTORY="${CMAKE_CURRENT_BINARY_DIR}/shaders")
    target_compile_definitions(threecpp_vulkan PUBLIC THREECPP_HAS_VULKAN=1)
    add_dependencies(threecpp_vulkan threecpp_shaders)
    add_executable(vulkan_basic examples/vulkan_basic.cpp)
    target_link_libraries(vulkan_basic PRIVATE threecpp::vulkan)
endif()
if(THREECPP_BUILD_TESTS)
    include(CTest)
    add_subdirectory(test)
endif()

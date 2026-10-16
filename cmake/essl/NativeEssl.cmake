# Fixed native frontend fork and independent validator. No imported host libraries.
if(CMAKE_VERSION VERSION_LESS 3.24)
    message(FATAL_ERROR "THREECPP_NATIVE_ESSL requires CMake 3.24 or newer")
endif()
include(FetchContent)
find_package(Python3 3.8 REQUIRED COMPONENTS Interpreter)
set(_threecpp_essl_dir "${CMAKE_CURRENT_LIST_DIR}")
set(_threecpp_essl_root "${CMAKE_CURRENT_LIST_DIR}/../..")
set(THREECPP_ESSL_ARCHIVE_DIR "" CACHE PATH "Optional directory containing exact hash-locked dependency ZIP archives")
function(threecpp_essl_archive name file url sha)
    string(TOUPPER "${name}" upper)
    if(DEFINED FETCHCONTENT_SOURCE_DIR_${upper} AND NOT "${FETCHCONTENT_SOURCE_DIR_${upper}}" STREQUAL "")
        message(FATAL_ERROR "Unverified source override is not allowed for ${name}; use THREECPP_ESSL_ARCHIVE_DIR")
    endif()
    if(THREECPP_ESSL_ARCHIVE_DIR)
        set(url "${THREECPP_ESSL_ARCHIVE_DIR}/${file}")
        if(NOT EXISTS "${url}")
            message(FATAL_ERROR "Missing pinned ESSL archive: ${url}")
        endif()
    endif()
    FetchContent_Declare(${name} URL "${url}" URL_HASH "SHA256=${sha}"
        DOWNLOAD_EXTRACT_TIMESTAMP FALSE SOURCE_SUBDIR _threecpp_no_auto_add)
endfunction()
threecpp_essl_archive(threecpp_glslang glslang-2fbed.zip
    https://codeload.github.com/KhronosGroup/glslang/zip/2fbed5e1fb43a1c480fe020e3bb686a31010989a
    6c241ca6d3b88ac602e572401f95de1c247110d900d1bbb3e302c4f3714515cc)
threecpp_essl_archive(threecpp_spirv_tools spirv-tools-ef96ed.zip
    https://codeload.github.com/KhronosGroup/SPIRV-Tools/zip/ef96ed763b43b59b33b31b362f09a02b729fa1c9
    2a7666cfaec2818516019fccb8101a6e57c589417bef83a3c61d19ed63de1c1f)
threecpp_essl_archive(threecpp_spirv_headers spirv-headers-496543.zip
    https://codeload.github.com/KhronosGroup/SPIRV-Headers/zip/496543121ce6419f23d6fa5d7194ba66c36212d2
    b33782a6ccd124e6e02634f6382456b0aba2242c6ec5b68d07be20ee77a5b6bc)
FetchContent_MakeAvailable(threecpp_glslang threecpp_spirv_tools threecpp_spirv_headers)
execute_process(COMMAND "${Python3_EXECUTABLE}" "${_threecpp_essl_dir}/verify-dependencies.py"
    "${_threecpp_essl_dir}/dependency-lock.json" "${_threecpp_essl_dir}/glslang-frontend.patch"
    "${threecpp_glslang_SOURCE_DIR}" "${threecpp_spirv_tools_SOURCE_DIR}" "${threecpp_spirv_headers_SOURCE_DIR}"
    RESULT_VARIABLE _threecpp_essl_verify)
if(NOT _threecpp_essl_verify EQUAL 0)
    message(FATAL_ERROR "Pinned ESSL dependency verification failed")
endif()
function(threecpp_add_essl)
    # Function scope keeps third-party options from changing enclosing targets.
    set(BUILD_SHARED_LIBS OFF)
    set(ENABLE_HLSL OFF)
    set(ENABLE_SPIRV ON)
    set(ENABLE_OPT OFF)
    set(GLSLANG_TESTS OFF)
    set(BUILD_EXTERNAL OFF)
    set(ENABLE_GLSLANG_BINARIES OFF)
    set(ENABLE_PCH OFF)
    set(GLSLANG_ENABLE_INSTALL OFF)
    set(ENABLE_RTTI ON)
    set(ENABLE_EXCEPTIONS ON)
    set(BUILD_WERROR OFF)
    add_subdirectory("${threecpp_glslang_SOURCE_DIR}" "${threecpp_glslang_BINARY_DIR}" EXCLUDE_FROM_ALL)
    set(SPIRV_SKIP_TESTS ON)
    set(SPIRV_SKIP_EXECUTABLES ON)
    set(SPIRV_TOOLS_BUILD_STATIC ON)
    set(SKIP_SPIRV_TOOLS_INSTALL ON)
    set(SPIRV-Headers_SOURCE_DIR "${threecpp_spirv_headers_SOURCE_DIR}")
    add_subdirectory("${threecpp_spirv_tools_SOURCE_DIR}" "${threecpp_spirv_tools_BINARY_DIR}" EXCLUDE_FROM_ALL)
    # Available to validator tests without exposing an imported host library.
    add_library(threecpp::spirv_validator ALIAS SPIRV-Tools-static)
    add_library(threecpp_essl_native STATIC
        "${_threecpp_essl_root}/src/renderers/vulkan/essl/CompilerCore.cpp"
        "${_threecpp_essl_root}/src/renderers/vulkan/essl/AstTransform.cpp"
        "${_threecpp_essl_root}/src/renderers/vulkan/essl/SpirvEmit.cpp")
    target_compile_features(threecpp_essl_native PUBLIC cxx_std_20)
    target_include_directories(threecpp_essl_native PUBLIC "${_threecpp_essl_root}/src")
    target_link_libraries(threecpp_essl_native PRIVATE glslang::glslang glslang::glslang-default-resource-limits SPIRV-Tools-static)
    set_target_properties(threecpp_essl_native PROPERTIES POSITION_INDEPENDENT_CODE ON)
    if(MSVC)
        target_compile_options(threecpp_essl_native PRIVATE /utf-8 /permissive- /bigobj /W4 /fp:strict)
    else()
        target_compile_options(threecpp_essl_native PRIVATE -Wall -Wextra -ffp-contract=off)
    endif()
endfunction()
threecpp_add_essl()

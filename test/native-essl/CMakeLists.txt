find_package(Threads REQUIRED)
add_executable(NativeEsslProgramCompilerTests NativeEsslProgramCompilerTests.cpp)
target_link_libraries(NativeEsslProgramCompilerTests PRIVATE threecpp::vulkan Threads::Threads)
add_test(NAME threecpp.native-essl-program-compiler COMMAND NativeEsslProgramCompilerTests)
set_tests_properties(threecpp.native-essl-program-compiler PROPERTIES TIMEOUT 120 LABELS "ESSL;CPU")
add_executable(NativeEsslCompilerTests NativeEsslCompilerTests.cpp)
target_link_libraries(NativeEsslCompilerTests PRIVATE threecpp::vulkan Threads::Threads)
add_test(NAME threecpp.native-essl-interface COMMAND NativeEsslCompilerTests
    "${CMAKE_CURRENT_SOURCE_DIR}/sources" "${CMAKE_CURRENT_BINARY_DIR}/interface-$<CONFIG>" "${THREECPP_GLSLANG_VALIDATOR}")
set_tests_properties(threecpp.native-essl-interface PROPERTIES TIMEOUT 120 LABELS "ESSL;CPU")
add_executable(NativeEsslReflectionProbe NativeEsslReflectionProbe.cpp)
target_link_libraries(NativeEsslReflectionProbe PRIVATE threecpp::vulkan)
add_executable(NativeEsslFrontendTests NativeEsslFrontendReview.cpp)
target_link_libraries(NativeEsslFrontendTests PRIVATE glslang::glslang glslang::glslang-default-resource-limits)
add_test(NAME threecpp.native-essl-frontend COMMAND NativeEsslFrontendTests)
set_tests_properties(threecpp.native-essl-frontend PROPERTIES TIMEOUT 120 LABELS "ESSL;CPU")
# Captures come from the pinned three.js Programs source oracle. Empty disables
# this optional material corpus; the builtin/interface and frontend tests remain.
set(THREECPP_ESSL_CAPTURE_DIRECTORY "" CACHE PATH "Optional pinned 26-pair three.js Programs ESSL capture directory")
if(THREECPP_ESSL_CAPTURE_DIRECTORY)
    add_test(NAME threecpp.native-essl-material-corpus
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/check-material-corpus.py"
            "$<TARGET_FILE:NativeEsslReflectionProbe>" "${THREECPP_ESSL_CAPTURE_DIRECTORY}"
            "${CMAKE_CURRENT_BINARY_DIR}/materials-$<CONFIG>")
    set_tests_properties(threecpp.native-essl-material-corpus PROPERTIES TIMEOUT 180 LABELS "ESSL;CPU")
endif()
# Native GL is only a Windows test oracle. No OpenGL dependency enters production.
if(WIN32)
    add_executable(NativeEsslVulkanBuiltinsGPU VulkanBuiltinsGPU.cpp)
    target_include_directories(NativeEsslVulkanBuiltinsGPU PRIVATE "${THREECPP_VULKAN_INCLUDE_DIR}")
    add_executable(NativeEsslOpenGLBuiltinsGPU OpenGLBuiltinsGPU.cpp)
    target_link_libraries(NativeEsslOpenGLBuiltinsGPU PRIVATE opengl32 user32 gdi32)
    add_test(NAME threecpp.native-essl-gpu-builtins
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/check-gpu.py"
            "$<TARGET_FILE:NativeEsslCompilerTests>" "$<TARGET_FILE:NativeEsslVulkanBuiltinsGPU>"
            "$<TARGET_FILE:NativeEsslOpenGLBuiltinsGPU>" "${CMAKE_CURRENT_SOURCE_DIR}/sources"
            "${CMAKE_CURRENT_BINARY_DIR}/gpu-$<CONFIG>" "${THREECPP_GLSLANG_VALIDATOR}")
    set_tests_properties(threecpp.native-essl-gpu-builtins PROPERTIES TIMEOUT 180 LABELS "ESSL;GPU;Vulkan;OpenGL")
endif()
foreach(target NativeEsslCompilerTests NativeEsslReflectionProbe NativeEsslFrontendTests NativeEsslProgramCompilerTests)
    if(MSVC)
        target_compile_options(${target} PRIVATE /utf-8 /permissive- /bigobj /W4 /fp:strict)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -ffp-contract=off)
    endif()
endforeach()

add_executable(VulkanProgramUniformsTests
    "${PROJECT_SOURCE_DIR}/test/unit/three.source.unit.cpp"
    "${PROJECT_SOURCE_DIR}/test/unit/src/renderers/vulkan/VulkanProgramUniforms.tests.cpp")
target_include_directories(VulkanProgramUniformsTests PRIVATE "${PROJECT_SOURCE_DIR}/test/unit/utils")
target_link_libraries(VulkanProgramUniformsTests PRIVATE threecpp::vulkan)
add_test(NAME threecpp.vulkan-program-uniforms COMMAND VulkanProgramUniformsTests)
set_tests_properties(threecpp.vulkan-program-uniforms PROPERTIES TIMEOUT 120 LABELS "ESSL;CPU")
if(WIN32)
    add_executable(VulkanProgramUniformsGPU VulkanProgramUniformsGPU.cpp)
    target_include_directories(VulkanProgramUniformsGPU PRIVATE "${THREECPP_VULKAN_INCLUDE_DIR}")
    target_link_libraries(VulkanProgramUniformsGPU PRIVATE threecpp::vulkan)
    add_test(NAME threecpp.vulkan-program-uniforms-gpu
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/check-program-uniforms-gpu.py"
            "$<TARGET_FILE:VulkanProgramUniformsGPU>" "$<TARGET_FILE:NativeEsslOpenGLBuiltinsGPU>"
            "${CMAKE_CURRENT_SOURCE_DIR}/sources" "${CMAKE_CURRENT_BINARY_DIR}/program-uniforms-gpu-$<CONFIG>")
    set_tests_properties(threecpp.vulkan-program-uniforms-gpu PROPERTIES TIMEOUT 180 LABELS "ESSL;GPU;Vulkan;OpenGL")
endif()
if(MSVC)
    target_compile_options(VulkanProgramUniformsTests PRIVATE /utf-8 /permissive- /bigobj /W4 /fp:strict)
    if(TARGET VulkanProgramUniformsGPU)
        target_compile_options(VulkanProgramUniformsGPU PRIVATE /utf-8 /permissive- /W4 /fp:strict)
    endif()
else()
    target_compile_options(VulkanProgramUniformsTests PRIVATE -Wall -Wextra -ffp-contract=off)
endif()

add_executable(THREECPPTests
    TestMain.cpp
    math/Vector2.tests.cpp
    math/Vector3.tests.cpp
    math/Vector4.tests.cpp
    math/MathUtils.tests.cpp
    math/Matrix3.tests.cpp
    math/Matrix4.tests.cpp
    math/Quaternion.tests.cpp
    math/Euler.tests.cpp
    math/Color.tests.cpp
    math/ThreeJsParity.tests.cpp
    core/EventDispatcher.tests.cpp core/Object3D.tests.cpp core/Resources.tests.cpp
    core/ResourceParity.tests.cpp
    cameras/Camera.tests.cpp scenes/Scene.tests.cpp
)

target_include_directories(THREECPPTests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(THREECPPTests PRIVATE threecpp::threecpp)
add_test(NAME threecpp.unit COMMAND THREECPPTests)
if(THREECPP_BUILD_VULKAN)
    add_executable(VulkanRendererTests renderers/VulkanRenderer.tests.cpp)
    target_link_libraries(VulkanRendererTests PRIVATE threecpp::vulkan)
    add_test(NAME threecpp.vulkan COMMAND VulkanRendererTests)
    set_tests_properties(threecpp.vulkan PROPERTIES LABELS "GPU;Vulkan")
endif()
add_executable(ThreeJsParityProbe parity/ParityProbe.cpp)
target_link_libraries(ThreeJsParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsCoreParityProbe parity/CoreParityProbe.cpp)
target_link_libraries(ThreeJsCoreParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsHelperParityProbe parity/HelperParityProbe.cpp)
target_link_libraries(ThreeJsHelperParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsRaycastParityProbe parity/RaycastParityProbe.cpp)
target_link_libraries(ThreeJsRaycastParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsQueryParityProbe parity/QueryParityProbe.cpp)
target_link_libraries(ThreeJsQueryParityProbe PRIVATE threecpp::threecpp)
find_program(THREECPP_NODE_EXECUTABLE NAMES node)
if(THREEJS_SOURCE_DIR AND THREECPP_NODE_EXECUTABLE)
    add_test(NAME threecpp.threejs-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/parity/compare.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsParityProbe>")
    add_test(NAME threecpp.threejs-core-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/parity/compare-core.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsCoreParityProbe>")
    add_test(NAME threecpp.threejs-helper-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/parity/compare-helper.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsHelperParityProbe>")
    add_test(NAME threecpp.threejs-raycast-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/parity/compare-raycast.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsRaycastParityProbe>")
    add_test(NAME threecpp.threejs-query-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/parity/compare-query.mjs"
        "$<TARGET_FILE:ThreeJsQueryParityProbe>" "${THREEJS_SOURCE_DIR}")
endif()

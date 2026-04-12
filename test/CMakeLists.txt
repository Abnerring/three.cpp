cmake_minimum_required(VERSION 3.10)

# Test executable
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
)

# Include directories
target_include_directories(THREECPPTests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/test
)

# C++ standard
target_compile_features(THREECPPTests PRIVATE cxx_std_20)

# Output directory
set_target_properties(THREECPPTests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

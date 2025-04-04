cmake_minimum_required(VERSION 3.10)

project(THREECPP)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(include)
add_executable(THREECPP 
    src/main.cpp
    src/math/Vector2.cpp
    src/math/MathUtils.cpp
    src/math/Matrix3.cpp
)

cmake_minimum_required(VERSION 3.20)
project(market CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG -Wall -Wextra -Wpedantic")

enable_testing()

add_subdirectory(tests)
add_subdirectory(bench)

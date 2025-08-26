cmake_minimum_required(VERSION 3.20)
project(market CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG -Wall -Wextra -Wpedantic")

enable_testing()

add_subdirectory(tests)
add_subdirectory(bench)

# Add generated sources to bench target if they exist
if(EXISTS "${CMAKE_SOURCE_DIR}/generated/cboe_boe_v3/encoder.cpp")
    target_sources(bench_encode_decode PRIVATE
        generated/cboe_boe_v3/encoder.cpp
        generated/cboe_boe_v3/decoder.cpp
    )
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/generated/nasdaq_itch_5/encoder.cpp")
    target_sources(bench_encode_decode PRIVATE
        generated/nasdaq_itch_5/encoder.cpp
        generated/nasdaq_itch_5/decoder.cpp
    )
endif()

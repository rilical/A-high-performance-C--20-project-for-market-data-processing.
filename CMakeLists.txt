cmake_minimum_required(VERSION 3.20)
project(market CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG -Wall -Wextra -Wpedantic")

# Configuration options
option(EXCHCG_NO_EXCEPTIONS "Disable C++ exceptions" OFF)
option(EXCHCG_HIDE_SYMBOLS "Hide symbols by default (use -fvisibility=hidden)" OFF)
option(EXCHCG_STRICT_WARNINGS "Treat warnings as errors" OFF)

# Apply configuration-specific compiler flags
if(EXCHCG_NO_EXCEPTIONS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHs-c-")
    endif()
    add_compile_definitions(EXCHCG_NO_EXCEPTIONS)
endif()

if(EXCHCG_HIDE_SYMBOLS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
    endif()
endif()

if(EXCHCG_STRICT_WARNINGS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")
    endif()
endif()

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

# Examples target (BOE)
if(EXISTS "${CMAKE_SOURCE_DIR}/generated/cboe_boe_v3/encoder.cpp")
    add_executable(encode_boe_login examples/encode_boe_login.cpp)
    target_include_directories(encode_boe_login PRIVATE .)
    target_sources(encode_boe_login PRIVATE
        generated/cboe_boe_v3/encoder.cpp
        generated/cboe_boe_v3/decoder.cpp
    )
    
    add_executable(decode_boe_login examples/decode_boe_login.cpp)
    target_include_directories(decode_boe_login PRIVATE .)
    target_sources(decode_boe_login PRIVATE
        generated/cboe_boe_v3/encoder.cpp
        generated/cboe_boe_v3/decoder.cpp
    )
    
    # Group examples under examples folder in IDEs
    set_target_properties(encode_boe_login decode_boe_login PROPERTIES
        FOLDER "examples"
    )
endif()

# Examples target (ITCH)
if(EXISTS "${CMAKE_SOURCE_DIR}/generated/nasdaq_itch_5/encoder.cpp")
    add_executable(encode_itch_add examples/encode_itch_add.cpp)
    target_include_directories(encode_itch_add PRIVATE .)
    target_sources(encode_itch_add PRIVATE
        generated/nasdaq_itch_5/encoder.cpp
        generated/nasdaq_itch_5/decoder.cpp
    )

    add_executable(decode_itch_add examples/decode_itch_add.cpp)
    target_include_directories(decode_itch_add PRIVATE .)
    target_sources(decode_itch_add PRIVATE
        generated/nasdaq_itch_5/encoder.cpp
        generated/nasdaq_itch_5/decoder.cpp
    )

    # Group examples under examples folder in IDEs
    set_target_properties(encode_itch_add decode_itch_add PROPERTIES
        FOLDER "examples"
    )
endif()

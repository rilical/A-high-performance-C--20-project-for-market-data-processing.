cmake_minimum_required(VERSION 3.20)
project(market CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG -Wall -Wextra -Wpedantic")

# Configuration options
option(MARKET_NO_EXCEPTIONS "Disable C++ exceptions" OFF)
option(MARKET_HIDE_SYMBOLS "Hide symbols by default (use -fvisibility=hidden)" OFF)
option(EXCHCG_NO_EXCEPTIONS "(compat) Disable C++ exceptions" OFF)
option(EXCHCG_HIDE_SYMBOLS "(compat) Hide symbols by default" OFF)
option(EXCHCG_STRICT_WARNINGS "Treat warnings as errors" OFF)

# Apply configuration-specific compiler flags
if(MARKET_NO_EXCEPTIONS OR EXCHCG_NO_EXCEPTIONS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHs-c-")
    endif()
    add_compile_definitions(MARKET_NO_EXCEPTIONS)
endif()

if(MARKET_HIDE_SYMBOLS OR EXCHCG_HIDE_SYMBOLS)
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

    add_executable(encode_itch_delete examples/encode_itch_delete.cpp)
    target_include_directories(encode_itch_delete PRIVATE .)
    target_sources(encode_itch_delete PRIVATE
        generated/nasdaq_itch_5/encoder.cpp
        generated/nasdaq_itch_5/decoder.cpp
    )

    add_executable(decode_itch_delete examples/decode_itch_delete.cpp)
    target_include_directories(decode_itch_delete PRIVATE .)
    target_sources(decode_itch_delete PRIVATE
        generated/nasdaq_itch_5/encoder.cpp
        generated/nasdaq_itch_5/decoder.cpp
    )

    # Group examples under examples folder in IDEs
    set_target_properties(encode_itch_add decode_itch_add encode_itch_delete decode_itch_delete PROPERTIES
        FOLDER "examples"
    )
endif()
# Tools
add_executable(mdp_dump tools/mdp_dump.cpp)
target_include_directories(mdp_dump PRIVATE ${CMAKE_SOURCE_DIR})
if(EXISTS "${CMAKE_SOURCE_DIR}/generated/cboe_boe_v3/encoder.cpp")
  target_sources(mdp_dump PRIVATE
    generated/cboe_boe_v3/encoder.cpp
    generated/cboe_boe_v3/decoder.cpp
    generated/cboe_boe_v3/json.cpp
  )
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/generated/nasdaq_itch_5/encoder.cpp")
  target_sources(mdp_dump PRIVATE
    generated/nasdaq_itch_5/encoder.cpp
    generated/nasdaq_itch_5/decoder.cpp
    generated/nasdaq_itch_5/json.cpp
  )
endif()

add_executable(pcap_decode tools/pcap_decode/pcap_decode.cpp)
target_include_directories(pcap_decode PRIVATE ${CMAKE_SOURCE_DIR})
if(EXISTS "${CMAKE_SOURCE_DIR}/generated/cboe_boe_v3/encoder.cpp")
  target_sources(pcap_decode PRIVATE
    generated/cboe_boe_v3/encoder.cpp
    generated/cboe_boe_v3/decoder.cpp
    generated/cboe_boe_v3/json.cpp
  )
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/generated/nasdaq_itch_5/encoder.cpp")
  target_sources(pcap_decode PRIVATE
    generated/nasdaq_itch_5/encoder.cpp
    generated/nasdaq_itch_5/decoder.cpp
    generated/nasdaq_itch_5/json.cpp
  )
endif()

# Install/export
include(GNUInstallDirs)
add_library(market_runtime INTERFACE)
add_library(market::runtime ALIAS market_runtime)
target_include_directories(market_runtime INTERFACE
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/market>
)

install(DIRECTORY runtime/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/market/runtime)
if(EXISTS "${CMAKE_SOURCE_DIR}/generated")
  install(DIRECTORY generated/
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/market/generated)
endif()

install(TARGETS market_runtime EXPORT market-targets)
install(EXPORT market-targets
        NAMESPACE market::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/market)

install(FILES market-config.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/market)

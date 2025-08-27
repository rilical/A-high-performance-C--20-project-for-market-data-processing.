find_package(Threads REQUIRED)

set(TEST_SOURCES test_roundtrip.cpp)

# Add generated sources if they exist
if(EXISTS "${CMAKE_SOURCE_DIR}/generated/cboe_boe_v3/encoder.cpp")
    list(APPEND TEST_SOURCES 
        "${CMAKE_SOURCE_DIR}/generated/cboe_boe_v3/encoder.cpp"
        "${CMAKE_SOURCE_DIR}/generated/cboe_boe_v3/decoder.cpp"
    )
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/generated/nasdaq_itch_5/encoder.cpp")
    list(APPEND TEST_SOURCES 
        "${CMAKE_SOURCE_DIR}/generated/nasdaq_itch_5/encoder.cpp"
        "${CMAKE_SOURCE_DIR}/generated/nasdaq_itch_5/decoder.cpp"
    )
endif()

add_executable(test_roundtrip ${TEST_SOURCES})
target_include_directories(test_roundtrip PRIVATE ${CMAKE_SOURCE_DIR})

# Multi-threaded stress test
set(MT_TEST_SOURCES test_mt_decode.cpp)

# Add generated sources if they exist
if(EXISTS "${CMAKE_SOURCE_DIR}/generated/cboe_boe_v3/encoder.cpp")
    list(APPEND MT_TEST_SOURCES 
        "${CMAKE_SOURCE_DIR}/generated/cboe_boe_v3/encoder.cpp"
        "${CMAKE_SOURCE_DIR}/generated/cboe_boe_v3/decoder.cpp"
    )
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/generated/nasdaq_itch_5/encoder.cpp")
    list(APPEND MT_TEST_SOURCES 
        "${CMAKE_SOURCE_DIR}/generated/nasdaq_itch_5/encoder.cpp"
        "${CMAKE_SOURCE_DIR}/generated/nasdaq_itch_5/decoder.cpp"
    )
endif()

add_executable(test_mt_decode ${MT_TEST_SOURCES})
target_include_directories(test_mt_decode PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(test_mt_decode PRIVATE Threads::Threads)

include(CTest)
add_test(NAME test_roundtrip COMMAND test_roundtrip)
add_test(NAME test_mt_decode COMMAND test_mt_decode)

set(TEST_SOURCES test_roundtrip.cpp)

# Add generated sources if they exist
if(EXISTS "${CMAKE_SOURCE_DIR}/generated/cboe_boe_v3/encoder.cpp")
    list(APPEND TEST_SOURCES 
        "${CMAKE_SOURCE_DIR}/generated/cboe_boe_v3/encoder.cpp"
        "${CMAKE_SOURCE_DIR}/generated/cboe_boe_v3/decoder.cpp"
    )
endif()

add_executable(test_roundtrip ${TEST_SOURCES})

include(CTest)
add_test(NAME test_roundtrip COMMAND test_roundtrip)

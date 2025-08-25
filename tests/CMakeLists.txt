add_executable(test_roundtrip test_roundtrip.cpp)

include(CTest)
add_test(NAME test_roundtrip COMMAND test_roundtrip)

add_executable(bench_encode_decode bench_encode_decode.cpp)
target_include_directories(bench_encode_decode PRIVATE ${CMAKE_SOURCE_DIR})

# Optional Google Benchmark target
find_package(benchmark QUIET)
if(benchmark_FOUND)
  add_executable(bench_gbench bench_gbench.cpp)
  target_include_directories(bench_gbench PRIVATE ${CMAKE_SOURCE_DIR})
  target_link_libraries(bench_gbench PRIVATE benchmark::benchmark)
endif()

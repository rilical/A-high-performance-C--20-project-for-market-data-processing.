# Production Hardening Guide

This document provides a comprehensive checklist for hardening the MARKET library for production deployment in ultra-low latency trading systems.

## 🔒 Security & ABI Stability

### Symbol Visibility Controls
```bash
# Add to CMakeLists.txt compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")

# Define export macro in runtime/export.hpp
#define MARKET_API __attribute__((visibility("default")))

# Apply to public API functions in generated code
MARKET_API market::runtime::status encode(/*...*/);
```

### ABI Stability
- [ ] **Freeze struct layouts**: Document that POD message structs are ABI-stable within major versions
- [ ] **Version namespacing**: Use `market::v1::`, `market::v2::` for breaking changes
- [ ] **Symbol mangling**: Ensure C++ name mangling is consistent across compiler versions

### Security Review
- [ ] **Input validation**: Audit all bounds checking in decoders for buffer overruns
- [ ] **Integer overflow**: Review arithmetic in presence map and group count handling
- [ ] **Memory safety**: Verify `std::span` usage prevents out-of-bounds access
- [ ] **Fuzzing coverage**: Achieve >95% line coverage with libFuzzer on all protocols

## 🧪 Testing & Validation

### Multi-Threading Stress Tests
```bash
# Add to tests/CMakeLists.txt
add_executable(test_multithread test_multithread.cpp)
target_link_libraries(test_multithread PRIVATE pthread)

# Test concurrent encode/decode from multiple threads
# Verify no data races with ThreadSanitizer
cmake -DCMAKE_CXX_FLAGS="-fsanitize=thread" -S . -B build-tsan
```

### Sanitizer CI Matrix
```yaml
# .github/workflows/sanitizers.yml
strategy:
  matrix:
    sanitizer: [address, undefined, thread, memory]
    compiler: [gcc-11, clang-14]
    
steps:
  - name: Build with ${{ matrix.sanitizer }}
    run: |
      cmake -DCMAKE_CXX_FLAGS="-fsanitize=${{ matrix.sanitizer }}" \
            -DCMAKE_CXX_COMPILER=${{ matrix.compiler }} \
            -S . -B build
      cmake --build build -j
      ctest --test-dir build --output-on-failure
```

### Coverage Targets
```bash
# Enable coverage in CMake
cmake -DCMAKE_CXX_FLAGS="--coverage" -S . -B build-coverage
cmake --build build-coverage -j
ctest --test-dir build-coverage

# Generate coverage report
gcov build-coverage/tests/CMakeFiles/test_roundtrip.dir/*.gcno
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage-html

# Target: >90% line coverage, >80% branch coverage
```

### Fuzz Testing Expansion
```bash
# Expand fuzzing beyond BOE to all protocols
clang++ -std=c++20 -fsanitize=fuzzer,address,undefined -O2 -I. \
  generated/nasdaq_itch_5/decoder.cpp tests/fuzz_decode_itch.cpp -o fuzz_itch

# Run extended fuzzing campaigns
./fuzz_boe -runs=10000000 -max_len=512
./fuzz_itch -runs=10000000 -max_len=512

# Integrate with OSS-Fuzz for continuous fuzzing
```

## 🏗️ Build System & Distribution

### CMake Install Rules
```cmake
# Add to CMakeLists.txt
include(GNUInstallDirs)

# Install headers
install(DIRECTORY runtime/ 
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/market/runtime)
install(DIRECTORY generated/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/market/generated)

# Install CMake config
install(FILES market-config.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/market)

# Export targets
install(EXPORT market-targets
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/market)
```

### Package Configuration
```cmake
# market-config.cmake
find_dependency(Threads REQUIRED)

include("${CMAKE_CURRENT_LIST_DIR}/market-targets.cmake")

# Usage: find_package(market REQUIRED)
# Links: target_link_libraries(my_app PRIVATE market::runtime)
```

### Semantic Versioning
- [ ] **Version scheme**: Adopt `MAJOR.MINOR.PATCH` with schema compatibility rules
- [ ] **CHANGELOG.md**: Document breaking changes, new protocols, performance improvements  
- [ ] **Git tags**: Tag releases with `v1.0.0` format for packaging systems
- [ ] **ABI policy**: MAJOR = ABI break, MINOR = new features, PATCH = bug fixes

## 🔍 Static Analysis & Code Quality

### Clang-Tidy Integration
```bash
# .clang-tidy configuration
Checks: |
  bugprone-*,
  cert-*,
  cppcoreguidelines-*,
  modernize-*,
  performance-*,
  readability-*,
  -modernize-use-trailing-return-type

# Run on generated code
clang-tidy generated/**/*.cpp generated/**/*.hpp -- -std=c++20 -I.

# Integrate with CI
cmake -DCMAKE_CXX_CLANG_TIDY="clang-tidy" -S . -B build-tidy
```

### Additional Static Analysis
```bash
# Cppcheck for additional bug detection
cppcheck --std=c++20 --enable=all --inconclusive \
         runtime/ generated/ tests/ bench/

# PVS-Studio for commercial-grade analysis
pvs-studio-analyzer trace -- cmake --build build
pvs-studio-analyzer analyze --output-file report.log
plog-converter -t html report.log -o report-html/
```

## 📊 Performance Validation

### Benchmarking Standards
- [ ] **Baseline measurements**: Establish performance baselines per protocol/message type
- [ ] **Regression detection**: Fail CI if performance degrades >10% from baseline
- [ ] **Multi-platform testing**: Validate performance on x86_64, ARM64, different microarchitectures
- [ ] **Compiler optimization verification**: Ensure hot paths are properly vectorized/inlined

### Memory Profiling
```bash
# Valgrind memory analysis
valgrind --tool=memcheck --leak-check=full \
         ./build/bench/bench_encode_decode

# Heap profiling with gperftools
LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libprofiler.so \
CPUPROFILE=bench.prof ./build/bench/bench_encode_decode
google-pprof --pdf ./build/bench/bench_encode_decode bench.prof > profile.pdf
```

## 🌐 Extended Protocol Support

### Schema Coverage Expansion
- [ ] **FIX protocols**: Implement FIX 4.4, FIX 5.0 schemas
- [ ] **Additional exchanges**: CME MDP 3.0, ICE Multicast, Eurex ETI
- [ ] **Binary options protocols**: OPRA, MIAX, PHLX
- [ ] **Custom protocol support**: Generic schema validation for proprietary protocols

### Backward Compatibility Testing
```bash
# Test schema evolution scenarios
python3 codegen/generate.py --schema schemas/cboe_boe_v3.yaml --out generated/boe_v3_0
python3 codegen/generate.py --schema schemas/cboe_boe_v3_1.yaml --out generated/boe_v3_1

# Verify v3.0 messages decode correctly with v3.1 decoder
./build/test_schema_compatibility boe_v3_0 boe_v3_1
```

## ✅ Production Deployment Checklist

### Pre-Deployment
- [ ] All sanitizer tests pass (ASAN/UBSAN/TSAN/MSAN)
- [ ] Static analysis shows zero critical/high severity issues
- [ ] Code coverage >90% line, >80% branch
- [ ] Fuzz testing runs clean for 24+ hours per protocol
- [ ] Performance benchmarks meet latency requirements
- [ ] Multi-threaded stress tests pass for 1+ hour
- [ ] Memory profiling shows zero leaks
- [ ] Symbol visibility correctly exports only public APIs

### Documentation
- [ ] API documentation generated and published
- [ ] Integration examples for each supported protocol
- [ ] Performance tuning guide for target hardware
- [ ] Troubleshooting guide for common deployment issues

### Monitoring & Observability
- [ ] **Metrics integration**: Support for Prometheus/StatsD metrics
- [ ] **Latency tracking**: Histogram exports for encode/decode times
- [ ] **Error tracking**: Structured logging for decode failures
- [ ] **Health checks**: API for validating encoder/decoder functionality

### Production Hardening Complete ✓
Once all items are addressed, the library is suitable for production deployment in latency-sensitive trading environments.

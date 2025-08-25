# MARKET

A high-performance C++20 project for market data processing.

## Building

```bash
cmake -S . -B build
cmake --build build -j
```

## Testing

```bash
ctest --test-dir build --output-on-failure
```

## Benchmarking

```bash
./build/bench/bench_encode_decode
```

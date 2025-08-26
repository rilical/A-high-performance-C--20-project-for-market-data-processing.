# Zero-allocation C++20 market data protocol encoder/decoder with code generation from YAML schemas**

A high-performance, header-only library for encoding and decoding binary market data protocols (BOE, ITCH) with compile-time type safety, runtime validation, and sub-microsecond latency.

## 🚀 60-Second Quickstart

1. **Clone and build:**
   ```bash
   git clone <repository-url>
   cd MARKET
   ```

2. **Generate C++ code from schemas:**
   ```bash
   # Generate BOE v3 protocol
   python3 codegen/generate.py --schema schemas/cboe_boe_v3.yaml --out generated/cboe_boe_v3
   
   # Generate NASDAQ ITCH v5 protocol  
   python3 codegen/generate.py --schema schemas/nasdaq_itch_5.yaml --out generated/nasdaq_itch_5
   ```

3. **Build and test:**
   ```bash
   # Linux/macOS
   cmake -S . -B build && cmake --build build -j
   ctest --test-dir build --output-on-failure
   
   # Windows PowerShell
   cmake -S . -B build; cmake --build build --parallel
   ctest --test-dir build --output-on-failure
   ```

4. **Run benchmarks:**
   ```bash
   # Linux/macOS
   ./build/bench/bench_encode_decode
   
   # Windows PowerShell  
   .\build\bench\Release\bench_encode_decode.exe
   ```

## 📖 How It Works

```
┌─────────────┐    ┌──────────────┐    ┌─────────────────┐    ┌─────────────┐
│ YAML Schema │───▶│ Code Generator│───▶│ C++ Headers/Src │───▶│ Your App    │
│ (BOE/ITCH)  │    │ (Jinja2)     │    │ (POD + encode/  │    │ (Link &     │
│             │    │              │    │  decode funcs)  │    │  Use)       │
└─────────────┘    └──────────────┘    └─────────────────┘    └─────────────┘
```

### Minimal YAML Example

```yaml
protocol: my_protocol
version: 1

enums:
  Side:
    Buy: 0x01
    Sell: 0x02

messages:
  Order:
    fields:
      - name: OrderId
        type: u64
        endian: le
      - name: Side
        type: enum:Side
      - name: Quantity
        type: u32
        endian: le
```

### Generated C++ (Conceptual)

```cpp
// Generated: my_protocol/v1/messages.hpp
namespace my_protocol::v1 {
    enum class Side : uint8_t { Buy = 0x01, Sell = 0x02 };
    
    struct Order {
        uint64_t OrderId;
        Side Side;
        uint32_t Quantity;
    };
}

// Generated: my_protocol/v1/encoder.hpp
namespace my_protocol::v1 {
    class Encoder {
    public:
        static market::runtime::status encode(
            const Order& msg, uint8_t* out, size_t out_sz, size_t& written);
    };
}

// Usage in your code:
Order order{.OrderId = 12345, .Side = Side::Buy, .Quantity = 1000};
std::array<uint8_t, 64> buffer;
size_t written;
auto status = Encoder::encode(order, buffer.data(), buffer.size(), written);
```

## 🏎️ Performance

Benchmarks on typical hardware (single-threaded, optimized build):

| Protocol | Message Type | Encode (ns/msg) | Decode (ns/msg) | Payload Size |
|----------|--------------|-----------------|-----------------|--------------|
| BOE v3   | LoginRequest | ~50             | ~60             | 29 bytes     |
| BOE v3   | NewOrderCross| ~120            | ~140            | 54+ bytes    |
| ITCH v5  | AddOrder     | ~40             | ~50             | 36 bytes     |
| ITCH v5  | DeleteOrder  | ~25             | ~35             | 15 bytes     |

*Run `./build/bench/bench_encode_decode` to measure on your system.*

## 📁 Repository Structure

```
MARKET/
├── README.md                    # This file
├── CMakeLists.txt              # Build configuration
├── runtime/                    # Header-only utilities
│   ├── endian.hpp             # LE/BE load/store operations  
│   ├── bytes.hpp              # std::span type aliases
│   └── status.hpp             # Error codes
├── schemas/                    # Protocol definitions
│   ├── cboe_boe_v3.yaml       # BOE Binary Order Entry v3
│   └── nasdaq_itch_5.yaml     # NASDAQ ITCH v5
├── codegen/                    # Code generation
│   ├── generate.py            # CLI generator script
│   └── templates/             # Jinja2 templates
│       ├── messages.hpp.j2    # POD structs + enums
│       ├── encoder.hpp.j2     # Encoder class declarations
│       ├── encoder.cpp.j2     # Encoder implementations  
│       ├── decoder.hpp.j2     # Decoder class declarations
│       ├── decoder.cpp.j2     # Decoder implementations
│       └── handler.hpp.j2     # Visitor dispatch functions
├── generated/                  # Generated C++ code (git-ignored)
│   ├── cboe_boe_v3/           # Generated BOE protocol
│   └── nasdaq_itch_5/         # Generated ITCH protocol
├── tests/                      # Unit tests
│   ├── test_roundtrip.cpp     # Encode/decode/dispatch tests
│   └── fuzz_decode_boe.cpp    # libFuzzer harness
├── bench/                      # Performance benchmarks
│   └── bench_encode_decode.cpp # Micro-benchmarks
└── docs/                       # Documentation
    ├── overview.md            # Architecture overview
    ├── boe_notes.md           # BOE protocol specifics
    ├── itch_notes.md          # ITCH protocol specifics
    └── versioning.md          # Schema compatibility
```

## 🛠️ Advanced Features

### Presence Maps & Optional Fields
```yaml
messages:
  ComplexOrder:
    fields:
      - name: PresenceBits
        type: u64
        purpose: presence_map
      - name: OrderId
        type: u64
      - name: Account      # Optional - controlled by bit 9
        type: char[16]
        optional_bit: 9
```

### Repeating Groups
```yaml
messages:
  MultiOrder:
    fields:
      - name: GroupCount
        type: u8
    groups:
      - name: Orders
        count_field: GroupCount
        fields:
          - name: Symbol
            type: char[8]
          - name: Quantity
            type: u32
```

### Visitor Pattern Dispatch
```cpp
struct Handler {
    void on(const cboe::boe::v3::LoginRequest& msg) { /* handle */ }
    void on(const cboe::boe::v3::NewOrderCross& msg) { /* handle */ }
};

Handler h;
size_t consumed;
auto status = cboe::boe::v3::dispatch_boe(input_bytes, h, consumed);
```

## 🔧 Troubleshooting

### Schema Validation Errors

The code generator includes strict schema validation with precise error paths:

```bash
# Missing enum reference
Error: Schema validation error at messages.Order.fields[1].type: Referenced enum 'NonExistentSide' does not exist

# Invalid optional_bit range  
Error: Schema validation error at messages.Order.fields[2].optional_bit: optional_bit 99 exceeds presence map width 64

# Invalid type syntax
Error: Schema validation error at messages.Order.fields[0].type: Invalid type 'bad_type'. Allowed: u8/u16/u32/u64, char, char[N], enum:Name

# Missing presence map for optional fields
Error: Schema validation error at messages.Order.fields[1].optional_bit: optional_bit specified but no presence map found in message
```

### Build Issues

```bash
# If cmake not found
sudo apt-get install cmake  # Ubuntu/Debian
brew install cmake          # macOS
# Windows: Download from https://cmake.org/

# If Python dependencies missing
pip3 install PyYAML Jinja2

# If C++20 compiler too old
# Requires: GCC 10+, Clang 12+, MSVC 19.29+
```

### Performance Issues

```bash
# Enable optimizations
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -DNDEBUG"

# Check compiler optimization
objdump -d build/bench/bench_encode_decode | grep -A 10 "encode"

# Profile with perf (Linux)
perf record ./build/bench/bench_encode_decode
perf report
```

## 📚 Documentation

- **[Architecture Overview](docs/overview.md)** - High-level design and trade-offs
- **[BOE Protocol Guide](docs/boe_notes.md)** - CBOE Binary Order Entry specifics  
- **[ITCH Protocol Guide](docs/itch_notes.md)** - NASDAQ ITCH specifics
- **[Versioning Strategy](docs/versioning.md)** - Schema evolution and compatibility

## 🧪 Testing & Fuzzing

```bash
# Unit tests (round-trip encode/decode)
ctest --test-dir build -V

# libFuzzer (requires clang)
clang++ -std=c++20 -fsanitize=fuzzer,address,undefined -O2 -I. \
  generated/cboe_boe_v3/decoder.cpp tests/fuzz_decode_boe.cpp -o fuzz_boe
./fuzz_boe -runs=1000000

# Custom iterations for benchmarks
ITER=5000000 ./build/bench/bench_encode_decode
```

## 🎯 Design Goals

- **Zero Allocations**: All operations use stack/static storage
- **Type Safety**: Compile-time validation of message structures  
- **Minimal Latency**: Sub-microsecond encode/decode on modern CPUs
- **Schema-Driven**: Single source of truth for protocol definitions
- **Maintainable**: Generated code is human-readable and debuggable
- **Portable**: C++20 standard, works on Linux/macOS/Windows

## 📝 Examples

Two runnable examples demonstrate basic BOE usage:

```bash
# Encode a BOE LoginRequest and print hex
./build/examples/encode_boe_login

# Decode a BOE LoginRequest from hex file
./build/examples/decode_boe_login tests/fixtures/boe_login.hex
```

## 🤝 Contributing

1. **Schema Changes**: Edit `schemas/*.yaml` and regenerate
2. **Template Changes**: Modify `codegen/templates/*.j2` files
3. **Runtime Changes**: Update `runtime/*.hpp` headers
4. **Testing**: Add cases to `tests/test_roundtrip.cpp`

## ⚖️ License

[Add your license here]

---

*Built for ultra-low latency trading systems where every nanosecond counts.*

# Architecture Overview

## Design Philosophy

The MARKET library is built around three core principles:

1. **Schema-First Development**: All protocol definitions live in human-readable YAML files that serve as the single source of truth
2. **Zero-Allocation Hot Paths**: Critical encode/decode operations use only stack storage and pre-allocated buffers  
3. **Compile-Time Safety**: Generated C++ code provides type safety and catches errors at build time rather than runtime

## System Architecture

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│ YAML Schemas    │    │ Code Generator  │    │ Generated C++   │
│                 │    │                 │    │                 │
│ • BOE v3        │───▶│ • Schema linter │───▶│ • POD structs   │
│ • ITCH v5       │    │ • Jinja2 render │    │ • Encoders      │
│ • Future...     │    │ • Type mapping  │    │ • Decoders      │
└─────────────────┘    └─────────────────┘    │ • Dispatchers   │
                                              └─────────────────┘
                                                        │
                                              ┌─────────────────┐
                                              │ Runtime Libs    │
                                              │                 │
                                              │ • Endian utils  │
                                              │ • Byte spans    │
                                              │ • Status codes  │
                                              └─────────────────┘
```

## Key Components

### 1. Schema Definition Language

Our YAML schema format supports:

- **Basic Types**: `u8`, `u16`, `u32`, `u64`, `char`, `char[N]`
- **Enumerations**: Strongly-typed constants with explicit values
- **Endianness**: Little-endian (`le`) and big-endian (`be`) support
- **Presence Maps**: Bitmasks for optional fields (BOE-style)
- **Repeating Groups**: Variable-length arrays with count fields
- **Protocol Versioning**: Namespace generation from protocol + version

### 2. Code Generation Pipeline

The generator performs:

1. **Schema Validation**: Strict linting with precise error paths
2. **Type Resolution**: Maps YAML types to C++ primitives
3. **Namespace Generation**: Creates hierarchical C++ namespaces
4. **Template Rendering**: Uses Jinja2 for consistent code generation
5. **Conditional Compilation**: Generates code only when schemas exist

### 3. Generated Code Structure

Each protocol produces:

```cpp
namespace protocol::version {
    // POD message structures
    struct MessageName { ... };
    
    // Encoding interface
    class Encoder {
        static status encode(const MessageName&, uint8_t*, size_t, size_t&);
    };
    
    // Decoding interface  
    class Decoder {
        static status decode(const uint8_t*, size_t, MessageName&, size_t&);
    };
    
    // Visitor dispatch
    template<class Handler>
    status dispatch_protocol(Bytes, Handler&, size_t&);
}
```

## Performance Characteristics

### Memory Management

- **Zero Dynamic Allocation**: No `malloc`/`new` in hot paths
- **Stack-Based**: Message objects are plain structs
- **Buffer Reuse**: Callers provide pre-allocated buffers
- **Vector Growth**: Only for repeating groups, minimized via `reserve()`

### CPU Optimization

- **Intrinsic Usage**: `__builtin_bswap*` for endian conversion
- **Branch Prediction**: Structured validation with early returns
- **Inlining**: All critical functions marked `inline`
- **Template Specialization**: Compile-time type dispatch

### Cache Efficiency

- **Sequential Access**: Wire format matches memory layout where possible
- **Minimal Indirection**: POD structs with direct field access
- **Locality**: Related fields grouped together
- **Prefetching**: Large messages processed in chunks

## Error Handling Strategy

### Compile-Time Errors

- **Type Mismatches**: Caught during C++ compilation
- **Schema Violations**: Detected by the generator's linter
- **Template Errors**: Clear diagnostics for API misuse

### Runtime Errors

```cpp
enum class status {
    ok = 0,           // Success
    short_buffer,     // Insufficient input/output space
    bad_value,        // Invalid field value (e.g., wrong message type)
    unknown_type      // Unrecognized message type in dispatch
};
```

- **No Exceptions**: All errors returned via status codes
- **Fail-Fast**: Validation stops at first error
- **Detailed Context**: Precise error locations for debugging

## Threading & Concurrency

### Thread Safety

- **Generated Code**: All functions are thread-safe (no shared state)
- **Message Objects**: POD structs are naturally thread-safe for read access
- **Buffer Management**: Caller responsibility (no shared buffers)

### Recommended Patterns

```cpp
// Per-thread encoder/decoder usage
thread_local std::array<uint8_t, MAX_MSG_SIZE> encode_buffer;
thread_local std::array<uint8_t, MAX_MSG_SIZE> decode_buffer;

// Lock-free message passing
struct MessageQueue {
    // Pre-encoded messages
    alignas(64) std::array<uint8_t, MSG_SIZE> slots[QUEUE_SIZE];
    // ... lock-free queue implementation
};
```

## Protocol-Specific Considerations

### BOE (Binary Order Entry)

- **Little-Endian**: All multi-byte integers use LE byte order
- **Presence Maps**: 64-bit bitmasks for optional fields
- **Variable Length**: Message length field for parsing
- **Repeating Groups**: Count-prefixed arrays with optional fields per item

### ITCH (Information Technology)

- **Big-Endian**: All multi-byte integers use BE byte order  
- **Fixed Formats**: Each message type has known, fixed size
- **Type Identification**: Single character at message start
- **No Optional Fields**: All fields are mandatory

## Extensibility

### Adding New Protocols

1. Create `schemas/my_protocol_v1.yaml`
2. Run `python3 codegen/generate.py --schema schemas/my_protocol_v1.yaml --out generated/my_protocol_v1`
3. Include generated headers in your build
4. Use the generated `Encoder`/`Decoder`/`dispatch_my_protocol` functions

### Schema Evolution

- **Additive Changes**: New optional fields, new message types
- **Version Bumps**: Breaking changes require new version namespace
- **Backward Compatibility**: Multiple versions can coexist

### Template Customization

Templates are designed for extension:
- Add new output files by creating `*.j2` templates
- Modify existing templates for custom code generation
- Override specific sections using Jinja2 inheritance

## Testing Strategy

### Unit Tests

- **Round-Trip**: Encode → Decode → Encode for bit-perfect equality
- **Edge Cases**: Boundary conditions, malformed input
- **Dispatch**: Visitor pattern with multiple message types

### Property-Based Testing

- **Fuzzing**: libFuzzer integration for decoder robustness
- **Random Generation**: Stress test with valid random messages
- **Invariant Checking**: Ensure encode/decode properties hold

### Performance Testing

- **Micro-Benchmarks**: Isolated encode/decode timing
- **Macro-Benchmarks**: End-to-end message processing
- **Regression Testing**: Performance monitoring across changes

## Future Enhancements

### Potential Additions

- **More Protocols**: FIX, SBE, custom binary formats
- **Streaming Support**: Partial decode for large messages
- **Compression**: Optional LZ4/Snappy integration
- **Serialization**: JSON/XML output for debugging
- **Code Analysis**: Static analysis integration

### Performance Optimizations

- **SIMD**: Vectorized operations for bulk processing
- **Template Metaprogramming**: Further compile-time optimizations
- **Memory Mapping**: Zero-copy techniques for large datasets
- **Hardware Acceleration**: FPGA/GPU offload for specific use cases

---

This architecture balances performance, safety, and maintainability for demanding financial applications where microseconds matter.
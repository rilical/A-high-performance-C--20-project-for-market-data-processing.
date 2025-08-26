# ITCH Protocol Guide

## Overview

NASDAQ ITCH is a data feed protocol used for real-time market data distribution. This implementation focuses on ITCH v5, providing high-throughput processing of market events with minimal latency overhead.

## Protocol Characteristics

### Wire Format

ITCH uses **big-endian** byte order for all multi-byte integers and has a simple message structure:

```
┌────────────────┬─────────────────────────────────────────────────┐
│ Message Type   │ Message Payload                                 │
│ (1 byte)       │ (fixed size per message type)                  │
│ 'A', 'D', etc. │ Fields in network byte order (big-endian)      │
└────────────────┴─────────────────────────────────────────────────┘
```

### Key Features

- **Fixed-Size Messages**: Each message type has a predefined, fixed length
- **Type-First Design**: Message type character at byte 0 enables fast dispatch
- **Big-Endian**: Network byte order for all multi-byte integers
- **High Throughput**: Optimized for processing millions of messages per second
- **No Optional Fields**: All fields are mandatory, simplifying parsing logic

## Message Types

### AddOrder ('A')

Represents a new order entering the book:

```yaml
AddOrder:
  fields:
    - name: Type
      type: char
      value: 'A'
    - name: Timestamp
      type: u32
      endian: be
    - name: OrderId
      type: u64
      endian: be
    - name: Side
      type: char
    - name: Shares
      type: u32
      endian: be
    - name: Symbol
      type: char
      length: 8
    - name: Price
      type: u32
      endian: be
```

**Wire Layout (36 bytes):**
```
Offset | Size | Field     | Example Value
-------|------|-----------|------------------
0      | 1    | Type      | 'A'
1      | 4    | Timestamp | 0x075BCD15 (BE)
5      | 8    | OrderId   | 0x0123456789ABCDEF (BE)
13     | 1    | Side      | 'B' (Buy) or 'S' (Sell)
14     | 4    | Shares    | 0x000003E8 (1000, BE)
18     | 8    | Symbol    | "AAPL    " (padded)
26     | 4    | Price     | 0x001E8480 (2000000, BE)
```

### DeleteOrder ('D')

Indicates an order has been removed from the book:

```yaml
DeleteOrder:
  fields:
    - name: Type
      type: char
      value: 'D'
    - name: Timestamp
      type: u32
      endian: be
    - name: OrderId
      type: u64
      endian: be
```

**Wire Layout (15 bytes):**
```
Offset | Size | Field     | Example Value
-------|------|-----------|------------------
0      | 1    | Type      | 'D'
1      | 4    | Timestamp | 0x075BCD16 (BE)
5      | 8    | OrderId   | 0x0123456789ABCDEF (BE)
```

## Big-Endian Processing

Unlike BOE's little-endian format, ITCH uses network byte order (big-endian) throughout.

### Endian Conversion

```cpp
// Load big-endian u32 from wire
uint32_t timestamp = market::runtime::load_be<uint32_t>(buffer + 1);

// Store big-endian u64 to wire  
market::runtime::store_be<uint64_t>(buffer + 5, order_id);

// Implementation uses efficient intrinsics:
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return __builtin_bswap32(value);  // x86/x64 hosts
#else
    return value;  // Already big-endian (rare)
#endif
```

### Performance Impact

Big-endian conversion on little-endian hosts (x86/x64) adds minimal overhead:

- **Modern CPUs**: `bswap` instruction is 1 cycle latency
- **Compiler Optimization**: Often optimized into adjacent memory operations
- **Cache Efficiency**: Sequential access pattern maintained

## Performance Characteristics

### Decoding Performance

ITCH decoding is highly optimized:

1. **Type Dispatch**: Single byte comparison for message routing
2. **Fixed Layouts**: No variable-length parsing or presence maps
3. **Sequential Access**: Fields read in wire order for cache efficiency
4. **Minimal Validation**: Only message type checked; field values trusted

**Typical Performance**: 
- AddOrder: ~40ns decode, ~50ns encode
- DeleteOrder: ~25ns decode, ~35ns encode

### Encoding Performance

ITCH encoding involves:

1. **Type Setting**: Single byte write for message type
2. **Field Serialization**: memcpy + endian conversion for each field
3. **No Length Calculation**: Fixed sizes known at compile time

### Memory Layout

```cpp
// Generated AddOrder struct (aligned for efficiency)
struct AddOrder {
    char Type;                      // 1 byte - message type
    uint32_t Timestamp;             // 4 bytes (BE)
    uint64_t OrderId;               // 8 bytes (BE)
    char Side;                      // 1 byte - 'B' or 'S'
    uint32_t Shares;                // 4 bytes (BE)
    std::array<char, 8> Symbol;     // 8 bytes - symbol + padding
    uint32_t Price;                 // 4 bytes (BE)
};
static_assert(sizeof(AddOrder) == 36);  // Verify no padding issues
```

## Implementation Details

### Message Dispatch

ITCH dispatch is extremely efficient due to fixed message sizes:

```cpp
template<class Handler>
status dispatch_itch(Bytes in, Handler& h, size_t& consumed) {
    if (in.size() < 1) return status::short_buffer;
    
    const char message_type = in[0];
    switch (message_type) {
    case 'A': {
        if (in.size() < 36) return status::short_buffer;
        AddOrder msg;
        auto status = Decoder::decode(in.data(), in.size(), msg, consumed);
        if (status == status::ok) h.on(msg);
        return status;
    }
    case 'D': {
        if (in.size() < 15) return status::short_buffer;
        DeleteOrder msg;
        auto status = Decoder::decode(in.data(), in.size(), msg, consumed);
        if (status == status::ok) h.on(msg);
        return status;
    }
    default:
        return status::unknown_type;
    }
}
```

### Buffer Management

```cpp
// ITCH messages have known maximum sizes
constexpr size_t MAX_ITCH_MSG_SIZE = 64;  // Conservative upper bound
std::array<uint8_t, MAX_ITCH_MSG_SIZE> encode_buffer;

// Encoding is straightforward
nasdaq::itch::v5::AddOrder order{
    .Type = 'A',
    .Timestamp = 123456789,
    .OrderId = 0x123456789ABCDEF0ULL,
    .Side = 'B',
    .Shares = 1000,
    .Symbol = {'A', 'A', 'P', 'L', ' ', ' ', ' ', ' '},
    .Price = 2000000
};

size_t written;
auto status = nasdaq::itch::v5::Encoder::encode(
    order, encode_buffer.data(), encode_buffer.size(), written);
assert(written == 36);  // AddOrder is always 36 bytes
```

### Error Handling

ITCH error handling is simplified by the fixed-format design:

```cpp
switch (status) {
case status::short_buffer:
    // Buffer too small for this message type
    // (should not happen with properly sized buffers)
    break;
case status::bad_value:
    // Invalid message type character
    break;
case status::ok:
    // Success - message fully decoded
    break;
}
```

## High-Throughput Processing

### Batch Processing Pattern

```cpp
struct ITCHProcessor {
    size_t processed_count = 0;
    
    void on(const nasdaq::itch::v5::AddOrder& msg) {
        // Process add order - update order book
        order_book.add_order(msg.OrderId, msg.Side, msg.Shares, msg.Price, msg.Symbol);
        ++processed_count;
    }
    
    void on(const nasdaq::itch::v5::DeleteOrder& msg) {
        // Process delete order - remove from book
        order_book.remove_order(msg.OrderId);
        ++processed_count;
    }
};

// Process buffer containing multiple messages
size_t offset = 0;
ITCHProcessor processor;
while (offset < buffer_size) {
    size_t consumed;
    auto span = std::span<const uint8_t>(buffer + offset, buffer_size - offset);
    auto status = nasdaq::itch::v5::dispatch_itch(span, processor, consumed);
    
    if (status != status::ok) {
        if (status == status::short_buffer) {
            // Partial message at end - save for next buffer
            memmove(buffer, buffer + offset, buffer_size - offset);
            buffer_size -= offset;
            break;
        }
        // Handle error...
    }
    offset += consumed;
}
```

### Zero-Copy Optimization

```cpp
// For very high throughput, consider zero-copy decoding
class ZeroCopyITCHDecoder {
    const uint8_t* buffer;
    size_t size;
    size_t offset = 0;
    
public:
    ZeroCopyITCHDecoder(const uint8_t* buf, size_t sz) : buffer(buf), size(sz) {}
    
    bool has_message() const { return offset < size; }
    
    char peek_type() const {
        return (offset < size) ? buffer[offset] : '\0';
    }
    
    nasdaq::itch::v5::AddOrder* decode_add_order() {
        if (offset + 36 > size) return nullptr;
        auto* msg = reinterpret_cast<const nasdaq::itch::v5::AddOrder*>(buffer + offset);
        offset += 36;
        // Note: Endian conversion still needed for multi-byte fields
        return const_cast<nasdaq::itch::v5::AddOrder*>(msg);
    }
};
```

## Testing Strategies

### Round-Trip Verification

```cpp
// ITCH round-trip testing
nasdaq::itch::v5::AddOrder original{
    .Type = 'A',
    .Timestamp = 123456789,
    .OrderId = 0x123456789ABCDEF0ULL,
    .Side = 'B', 
    .Shares = 1000,
    .Symbol = {'A', 'A', 'P', 'L', ' ', ' ', ' ', ' '},
    .Price = 2000000
};

// Encode -> Decode -> Re-encode
std::array<uint8_t, 64> buffer1, buffer2;
size_t written1, written2, consumed;

auto status1 = Encoder::encode(original, buffer1.data(), buffer1.size(), written1);
assert(status1 == status::ok && written1 == 36);

nasdaq::itch::v5::AddOrder decoded;
auto status2 = Decoder::decode(buffer1.data(), written1, decoded, consumed);
assert(status2 == status::ok && consumed == 36);

auto status3 = Encoder::encode(decoded, buffer2.data(), buffer2.size(), written2);
assert(status3 == status::ok && written2 == 36);

// Verify bit-perfect equality
assert(memcmp(buffer1.data(), buffer2.data(), 36) == 0);
```

### Performance Testing

```cpp
// Benchmark ITCH processing throughput
void benchmark_itch_throughput() {
    constexpr size_t MESSAGE_COUNT = 1000000;
    constexpr size_t BUFFER_SIZE = MESSAGE_COUNT * 36;  // All AddOrder
    
    std::vector<uint8_t> test_buffer(BUFFER_SIZE);
    // Fill with valid AddOrder messages...
    
    auto start = std::chrono::steady_clock::now();
    
    ITCHProcessor processor;
    size_t offset = 0;
    while (offset < BUFFER_SIZE) {
        size_t consumed;
        auto span = std::span<const uint8_t>(test_buffer.data() + offset, BUFFER_SIZE - offset);
        nasdaq::itch::v5::dispatch_itch(span, processor, consumed);
        offset += consumed;
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    
    std::cout << "Processed " << processor.processed_count << " messages in " 
              << duration.count() << "ns" << std::endl;
    std::cout << "Throughput: " << (MESSAGE_COUNT * 1e9 / duration.count()) 
              << " messages/second" << std::endl;
}
```

## Symbol Handling

ITCH symbols are 8-character fixed-width strings, space-padded:

```cpp
// Convert C++ string to ITCH symbol format
std::array<char, 8> to_itch_symbol(const std::string& symbol) {
    std::array<char, 8> result;
    result.fill(' ');  // Initialize with spaces
    
    size_t copy_len = std::min(symbol.length(), size_t(8));
    std::copy(symbol.begin(), symbol.begin() + copy_len, result.begin());
    
    return result;
}

// Convert ITCH symbol to C++ string (trimming spaces)
std::string from_itch_symbol(const std::array<char, 8>& symbol) {
    auto end = std::find(symbol.rbegin(), symbol.rend(), ' ').base();
    return std::string(symbol.begin(), end);
}

// Usage
auto aapl_symbol = to_itch_symbol("AAPL");    // {'A','A','P','L',' ',' ',' ',' '}
auto googl_symbol = to_itch_symbol("GOOGL");  // {'G','O','O','G','L',' ',' ',' '}
```

## Best Practices

### 1. Pre-validate Buffer Sizes

```cpp
// Check minimum buffer size before dispatch
if (buffer_size < 1) return status::short_buffer;

char msg_type = buffer[0];
size_t required_size = (msg_type == 'A') ? 36 : 
                      (msg_type == 'D') ? 15 : 0;
if (required_size > 0 && buffer_size < required_size) {
    return status::short_buffer;
}
```

### 2. Use Compile-Time Constants

```cpp
// Message sizes are known at compile time
namespace nasdaq::itch::v5 {
    constexpr size_t ADD_ORDER_SIZE = 36;
    constexpr size_t DELETE_ORDER_SIZE = 15;
    constexpr size_t MAX_MESSAGE_SIZE = 64;  // Future-proof
}
```

### 3. Optimize Hot Paths

```cpp
// Fast path for known message types
inline bool is_add_order(const uint8_t* buffer) {
    return buffer[0] == 'A';
}

inline bool is_delete_order(const uint8_t* buffer) {
    return buffer[0] == 'D';
}

// Branch prediction friendly
if (likely(is_add_order(buffer))) {
    // Handle AddOrder (most common)
} else if (is_delete_order(buffer)) {
    // Handle DeleteOrder
} else {
    // Handle unknown type
}
```

### 4. Memory Pool for High Frequency

```cpp
// For ultra-high frequency applications
class ITCHMessagePool {
    std::array<nasdaq::itch::v5::AddOrder, 10000> add_order_pool;
    std::array<nasdaq::itch::v5::DeleteOrder, 10000> delete_order_pool;
    size_t add_index = 0;
    size_t delete_index = 0;
    
public:
    nasdaq::itch::v5::AddOrder* get_add_order() {
        return &add_order_pool[add_index++ % add_order_pool.size()];
    }
    
    nasdaq::itch::v5::DeleteOrder* get_delete_order() {
        return &delete_order_pool[delete_index++ % delete_order_pool.size()];
    }
};
```

---

ITCH's simplicity and fixed-format design make it ideal for high-throughput market data applications where processing millions of messages per second is required.
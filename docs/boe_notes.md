# BOE Protocol Guide

## Overview

BOE (Binary Order Entry) is CBOE's high-performance binary protocol for order entry and management. This implementation focuses on BOE v3, providing ultra-low latency encoding/decoding for trading applications.

## Protocol Characteristics

### Wire Format

BOE uses a **little-endian** byte order for all multi-byte integers and includes a standard message envelope:

```
┌────────────────┬────────────────┬────────────────┬─────────────────┐
│ StartOfMessage │ MessageLength  │ MessageType    │ Message Payload │
│ (2 bytes, LE)  │ (2 bytes, LE)  │ (1 byte)       │ (variable)      │
│ 0xBABA         │ Total length   │ Type enum      │ Fields...       │
└────────────────┴────────────────┴────────────────┴─────────────────┘
```

### Key Features

- **Self-Describing**: Length field enables streaming protocols
- **Type Safety**: Message type byte for dispatch routing
- **Variable Length**: Supports optional fields and repeating groups
- **Presence Maps**: Efficient optional field encoding using bitmasks

## Message Types

### LoginRequest (0x01)

The simplest BOE message, used for session authentication:

```yaml
LoginRequest:
  fields:
    - name: StartOfMessage
      type: u16
      value: 0xBABA
      endian: le
    - name: MessageLength  
      type: u16
      endian: le
    - name: MessageType
      type: enum:MessageType
    - name: Username
      type: char[4]
    - name: Password  
      type: char[20]
```

**Wire Layout (29 bytes):**
```
Offset | Size | Field           | Value
-------|------|-----------------|------------------
0      | 2    | StartOfMessage  | 0xBABA (LE)
2      | 2    | MessageLength   | 0x001D (29, LE)  
4      | 1    | MessageType     | 0x01
5      | 4    | Username        | "USER" 
9      | 20   | Password        | "PASSWORD_12345678901"
```

### NewOrderCross (0x41)

A complex message demonstrating presence maps and repeating groups:

```yaml
NewOrderCross:
  fields:
    - name: PresenceBits
      type: u64
      endian: le
      purpose: presence_map
    - name: CrossId
      type: char[20]
    - name: GroupCount
      type: u8
  groups:
    - name: Groups
      count_field: GroupCount
      fields:
        - name: Side
          type: u8
        - name: AllocQty
          type: u32
          endian: le
        - name: ClOrdId
          type: char[20]
        - name: Account      # Optional - controlled by bit 9
          type: char[16]
          optional_bit: 9
```

## Presence Maps

BOE's presence map mechanism allows efficient encoding of optional fields using a bitmask.

### How It Works

1. **Presence Field**: A `u64` field marked with `purpose: presence_map`
2. **Bit Assignment**: Optional fields specify `optional_bit: N` (0-63)
3. **Encoding Logic**: If bit N is set, include the field; otherwise, skip it
4. **Decoding Logic**: Check bit N before attempting to read the field

### Example: Account Field (Bit 9)

```cpp
// Encoding with Account
message.PresenceBits |= (1ULL << 9);  // Set bit 9
// ... encode other fields ...
if (message.PresenceBits & (1ULL << 9)) {
    memcpy(out + offset, message.Account.data(), 16);
    offset += 16;
}

// Decoding with Account check
if (presence_bits & (1ULL << 9)) {
    memcpy(decoded.Account.data(), in + offset, 16);
    offset += 16;
}
```

### Presence Map Width Validation

The schema validator ensures optional bits don't exceed the presence map width:

```
u8  → 8 bits  (optional_bit: 0-7)
u16 → 16 bits (optional_bit: 0-15)  
u32 → 32 bits (optional_bit: 0-31)
u64 → 64 bits (optional_bit: 0-63)
```

## Repeating Groups

BOE supports variable-length arrays using a count field + group definition pattern.

### Structure

1. **Count Field**: Scalar integer specifying number of groups
2. **Group Definition**: List of fields that repeat `count` times
3. **Optional Group Fields**: Can use presence map from parent message

### Example: NewOrderCross Groups

```
Message Layout:
┌─────────────┬─────────┬────────────┬─────────────────────────────────┐
│ Header      │PresenceMap│ CrossId   │ GroupCount │ Group[0] │ Group[1] │ ... │
│ (5 bytes)   │ (8 bytes)  │ (20 bytes)│ (1 byte)   │          │          │     │
└─────────────┴─────────┴────────────┴────────────┴──────────┴──────────┴─────┘

Each Group:
┌──────┬───────────┬───────────┬─────────────────┐
│ Side │ AllocQty  │ ClOrdId   │ Account (opt)   │
│(1 byte)│(4 bytes LE)│(20 bytes) │ (16 bytes)      │
└──────┴───────────┴───────────┴─────────────────┘
```

## Performance Characteristics

### Encoding Performance

BOE encoding involves:

1. **Header Construction**: Fixed 5-byte preamble
2. **Length Calculation**: Sum of all field sizes (including optional)
3. **Sequential Writing**: memcpy for fixed fields, loops for groups
4. **Endian Conversion**: LE store operations for multi-byte integers

**Typical Performance (LoginRequest)**: ~50ns encode, ~60ns decode

### Decoding Performance

BOE decoding involves:

1. **Header Validation**: Check StartOfMessage (0xBABA) and MessageType
2. **Progressive Parsing**: Validate buffer size before each read
3. **Presence Map Evaluation**: Bitwise operations for optional fields
4. **Group Iteration**: Loop with bounds checking

### Memory Layout Optimization

```cpp
// Generated struct layout (aligned for cache efficiency)
struct NewOrderCross {
    uint64_t PresenceBits;          // 8 bytes - presence map
    std::array<char, 20> CrossId;   // 20 bytes - fixed string
    uint8_t GroupCount;             // 1 byte - group count
    std::vector<Groups> Groups;     // Variable - preallocated vector
};

// Group struct (POD for memcpy efficiency)
struct Groups {
    uint8_t Side;                   // 1 byte
    uint32_t AllocQty;              // 4 bytes (LE)
    std::array<char, 20> ClOrdId;   // 20 bytes
    std::array<char, 16> Account;   // 16 bytes (optional)
};
```

## Implementation Details

### Buffer Management

```cpp
// Recommended usage pattern
constexpr size_t MAX_BOE_MSG_SIZE = 1024;
std::array<uint8_t, MAX_BOE_MSG_SIZE> encode_buffer;
std::array<uint8_t, MAX_BOE_MSG_SIZE> decode_buffer;

// Encode
size_t written;
auto status = cboe::boe::v3::Encoder::encode(message, 
    encode_buffer.data(), encode_buffer.size(), written);

// Decode  
size_t consumed;
cboe::boe::v3::NewOrderCross decoded;
status = cboe::boe::v3::Decoder::decode(
    decode_buffer.data(), received_bytes, decoded, consumed);
```

### Error Handling

Common BOE decoding errors:

```cpp
switch (status) {
case status::short_buffer:
    // Need more bytes - partial message received
    break;
case status::bad_value:
    // Invalid StartOfMessage or MessageType 
    break;
case status::ok:
    // Success - message in 'decoded', bytes consumed in 'consumed'
    break;
}
```

### Visitor Pattern Usage

```cpp
struct BOEHandler {
    void on(const cboe::boe::v3::LoginRequest& msg) {
        // Handle login request
        std::cout << "Login from: " << std::string_view(msg.Username.data(), 4) << std::endl;
    }
    
    void on(const cboe::boe::v3::NewOrderCross& msg) {
        // Handle new order cross
        std::cout << "Cross with " << static_cast<int>(msg.GroupCount) << " groups" << std::endl;
        for (const auto& group : msg.Groups) {
            // Process each group...
        }
    }
};

BOEHandler handler;
size_t consumed;
auto status = cboe::boe::v3::dispatch_boe(incoming_bytes, handler, consumed);
```

## Schema Evolution

### Backward Compatibility

BOE's design enables several compatibility strategies:

1. **Optional Fields**: Add new optional fields without breaking existing decoders
2. **New Message Types**: Add new message types (decoders return `unknown_type`)
3. **Version Namespaces**: Breaking changes use new version (e.g., `cboe::boe::v4`)

### Migration Path

```cpp
// Support multiple versions simultaneously
#include "generated/cboe_boe_v3/decoder.hpp"
#include "generated/cboe_boe_v4/decoder.hpp"

// Try v4 first, fall back to v3
cboe::boe::v4::Message msg_v4;
size_t consumed;
auto status = cboe::boe::v4::Decoder::decode(buffer, size, msg_v4, consumed);
if (status == status::bad_value) {
    cboe::boe::v3::Message msg_v3;
    status = cboe::boe::v3::Decoder::decode(buffer, size, msg_v3, consumed);
}
```

## Testing & Validation

### Round-Trip Testing

```cpp
// Create original message
cboe::boe::v3::LoginRequest original;
original.Username = {'U', 'S', 'E', 'R'};
// ... set other fields ...

// Encode
std::array<uint8_t, 64> buffer1;
size_t written;
auto status = Encoder::encode(original, buffer1.data(), buffer1.size(), written);

// Decode
cboe::boe::v3::LoginRequest decoded;
size_t consumed;
status = Decoder::decode(buffer1.data(), written, decoded, consumed);

// Re-encode
std::array<uint8_t, 64> buffer2;
size_t written2;
status = Encoder::encode(decoded, buffer2.data(), buffer2.size(), written2);

// Verify bit-perfect equality
assert(written == written2);
assert(memcmp(buffer1.data(), buffer2.data(), written) == 0);
```

### Fuzzing Integration

```cpp
// libFuzzer harness for BOE decoders
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    cboe::boe::v3::LoginRequest msg;
    size_t consumed;
    
    // Try to decode - should never crash
    auto status = cboe::boe::v3::Decoder::decode(data, size, msg, consumed);
    
    // Status is ignored - we're testing for crashes/UB only
    return 0;
}
```

## Best Practices

### 1. Pre-allocate Group Vectors

```cpp
// Reserve space for expected group count
newOrderCross.Groups.reserve(expected_group_count);
```

### 2. Validate Message Length

```cpp
// Always check MessageLength against buffer size
if (message_length > available_bytes) {
    return status::short_buffer;
}
```

### 3. Use Presence Maps Efficiently

```cpp
// Cache presence bits for multiple checks
const uint64_t presence = message.PresenceBits;
if (presence & (1ULL << 9)) { /* Account present */ }
if (presence & (1ULL << 15)) { /* Other optional field */ }
```

### 4. Minimize Memory Allocations

```cpp
// Reuse message objects
static thread_local cboe::boe::v3::NewOrderCross reusable_message;
reusable_message.Groups.clear();  // Reset but keep allocated capacity
```

---

BOE's combination of performance, flexibility, and extensibility makes it ideal for high-frequency trading applications where every nanosecond counts.
# Schema Versioning & Compatibility

## Overview

The MARKET library supports multiple protocol versions simultaneously through namespace-based isolation. This document outlines strategies for schema evolution, backward compatibility, and migration planning.

## Versioning Strategy

### Namespace Isolation

Each protocol version generates code in its own C++ namespace:

```cpp
// BOE v3
namespace cboe::boe::v3 {
    struct LoginRequest { /* v3 fields */ };
    class Encoder { /* v3 logic */ };
    class Decoder { /* v3 logic */ };
}

// BOE v4 (future)
namespace cboe::boe::v4 {
    struct LoginRequest { /* v4 fields - potentially different */ };
    class Encoder { /* v4 logic */ };
    class Decoder { /* v4 logic */ };
}
```

### Version Identification

Versions are specified in YAML schemas and encoded in generated namespaces:

```yaml
# schemas/cboe_boe_v3.yaml
protocol: cboe_boe
version: 3

# schemas/cboe_boe_v4.yaml  
protocol: cboe_boe
version: 4
```

This generates:
- `cboe::boe::v3::*` for version 3
- `cboe::boe::v4::*` for version 4

## Compatibility Levels

### 1. Forward Compatibility

Newer decoders can read older message formats:

```cpp
// v4 decoder handles v3 messages
namespace cboe::boe::v4 {
    class Decoder {
        // Can decode both v3 and v4 wire formats
        static status decode_any_version(const uint8_t* data, size_t size, 
                                       Message& out, size_t& consumed);
    };
}
```

### 2. Backward Compatibility

Older decoders gracefully handle newer message formats:

```cpp
// v3 decoder with v4 messages
auto status = cboe::boe::v3::Decoder::decode(v4_message_data, size, msg, consumed);
if (status == status::unknown_type) {
    // Graceful degradation - skip unknown message
    consumed = skip_message_length(v4_message_data);
    return status::ok;
}
```

### 3. Side-by-Side Compatibility

Multiple versions coexist in the same application:

```cpp
#include "generated/cboe_boe_v3/all.hpp"
#include "generated/cboe_boe_v4/all.hpp"

class MultiVersionHandler {
    void handle_v3(const cboe::boe::v3::LoginRequest& msg) { /* v3 logic */ }
    void handle_v4(const cboe::boe::v4::LoginRequest& msg) { /* v4 logic */ }
};
```

## Schema Evolution Patterns

### 1. Additive Changes (Safe)

New optional fields can be added without breaking compatibility:

```yaml
# BOE v3
LoginRequest:
  fields:
    - name: Username
      type: char[4]
    - name: Password
      type: char[20]

# BOE v4 - Add optional field
LoginRequest:
  fields:
    - name: PresenceBits
      type: u64
      purpose: presence_map
    - name: Username
      type: char[4]
    - name: Password
      type: char[20]
    - name: SessionId      # New optional field
      type: char[16]
      optional_bit: 0
```

### 2. New Message Types (Safe)

Adding new message types doesn't affect existing decoders:

```yaml
# v3 has: LoginRequest, NewOrderCross
enums:
  MessageType:
    LoginRequest: 0x01
    NewOrderCross: 0x41

# v4 adds new message type
enums:
  MessageType:
    LoginRequest: 0x01
    NewOrderCross: 0x41
    OrderCancel: 0x42    # New in v4
```

### 3. Field Type Changes (Breaking)

Changing field types requires version bump:

```yaml
# v3 - Small order ID
OrderId:
  type: u32

# v4 - Larger order ID (breaking change)
OrderId:
  type: u64  # Requires new version namespace
```

### 4. Field Removal (Breaking)

Removing fields requires version bump:

```yaml
# v3
LoginRequest:
  fields:
    - name: Username
      type: char[4]
    - name: Password
      type: char[20]
    - name: ClientId
      type: u32

# v4 - Remove ClientId (breaking)
LoginRequest:
  fields:
    - name: Username
      type: char[4]
    - name: Password
      type: char[20]
    # ClientId removed - breaking change
```

## Migration Strategies

### 1. Phased Rollout

```cpp
class PhaseAwareBOEHandler {
    enum class Phase { V3_ONLY, MIXED, V4_ONLY };
    Phase current_phase = Phase::V3_ONLY;
    
public:
    void set_phase(Phase phase) { current_phase = phase; }
    
    bool try_decode_message(const uint8_t* data, size_t size) {
        switch (current_phase) {
        case Phase::V3_ONLY:
            return try_v3_only(data, size);
        case Phase::MIXED:
            return try_mixed_decode(data, size);
        case Phase::V4_ONLY:
            return try_v4_only(data, size);
        }
    }
    
private:
    bool try_mixed_decode(const uint8_t* data, size_t size) {
        // Try v4 first (preferred)
        cboe::boe::v4::Message msg_v4;
        size_t consumed;
        auto status = cboe::boe::v4::Decoder::decode(data, size, msg_v4, consumed);
        if (status == status::ok) {
            handle_v4_message(msg_v4);
            return true;
        }
        
        // Fall back to v3
        cboe::boe::v3::Message msg_v3;
        status = cboe::boe::v3::Decoder::decode(data, size, msg_v3, consumed);
        if (status == status::ok) {
            handle_v3_message(msg_v3);
            return true;
        }
        
        return false;  // Unable to decode
    }
};
```

### 2. Version Negotiation

```cpp
struct VersionNegotiation {
    uint8_t supported_versions[4] = {4, 3, 2, 1};  // Preferred order
    
    uint8_t negotiate_version(const uint8_t* peer_versions, size_t count) {
        for (auto my_version : supported_versions) {
            for (size_t i = 0; i < count; ++i) {
                if (peer_versions[i] == my_version) {
                    return my_version;  // Found compatible version
                }
            }
        }
        return 0;  // No compatible version
    }
};
```

### 3. Gradual Migration

```cpp
class GradualMigrator {
    std::atomic<double> v4_traffic_percentage{0.0};  // Start with 0% v4
    
public:
    void set_v4_percentage(double percentage) {
        v4_traffic_percentage.store(std::clamp(percentage, 0.0, 100.0));
    }
    
    bool should_use_v4() {
        static thread_local std::random_device rd;
        static thread_local std::mt19937 gen(rd());
        static thread_local std::uniform_real_distribution<> dis(0.0, 100.0);
        
        return dis(gen) < v4_traffic_percentage.load();
    }
    
    void encode_message(const Order& order, std::vector<uint8_t>& output) {
        if (should_use_v4()) {
            encode_as_v4(order, output);
        } else {
            encode_as_v3(order, output);
        }
    }
};
```

## Schema Validation Across Versions

### Cross-Version Validation

The schema linter can validate compatibility:

```python
# In generate.py - add compatibility checking
def validate_compatibility(old_schema, new_schema):
    """Check if new_schema is compatible with old_schema."""
    
    # Check for removed required fields
    for msg_name, msg_def in old_schema.get('messages', {}).items():
        if msg_name not in new_schema.get('messages', {}):
            continue  # Message removal is allowed
            
        old_fields = {f['name'] for f in msg_def.get('fields', []) 
                     if 'optional_bit' not in f}
        new_fields = {f['name'] for f in new_schema['messages'][msg_name].get('fields', [])
                     if 'optional_bit' not in f}
        
        removed_required = old_fields - new_fields
        if removed_required:
            raise ValueError(f"Removed required fields in {msg_name}: {removed_required}")
    
    # Check for changed field types
    for msg_name in old_schema.get('messages', {}):
        if msg_name not in new_schema.get('messages', {}):
            continue
            
        old_msg = old_schema['messages'][msg_name]
        new_msg = new_schema['messages'][msg_name]
        
        old_field_types = {f['name']: f['type'] for f in old_msg.get('fields', [])}
        new_field_types = {f['name']: f['type'] for f in new_msg.get('fields', [])}
        
        for field_name in old_field_types:
            if field_name in new_field_types:
                if old_field_types[field_name] != new_field_types[field_name]:
                    raise ValueError(f"Changed type of {msg_name}.{field_name}: "
                                   f"{old_field_types[field_name]} -> {new_field_types[field_name]}")
```

## Version Detection at Runtime

### Wire Format Detection

```cpp
class VersionDetector {
public:
    enum class Version { V3, V4, UNKNOWN };
    
    static Version detect_boe_version(const uint8_t* data, size_t size) {
        if (size < 5) return Version::UNKNOWN;
        
        // Check standard BOE preamble
        uint16_t start_of_message = market::runtime::load_le<uint16_t>(data);
        if (start_of_message != 0xBABA) return Version::UNKNOWN;
        
        uint16_t message_length = market::runtime::load_le<uint16_t>(data + 2);
        uint8_t message_type = data[4];
        
        // Use heuristics or version-specific message types
        if (message_type == 0x42) {  // OrderCancel only in v4
            return Version::V4;
        }
        
        // Check for v4-specific field patterns
        if (message_type == 0x01 && message_length > 29) {  // Extended LoginRequest
            return Version::V4;
        }
        
        return Version::V3;  // Default/legacy
    }
};
```

### Application-Level Version Management

```cpp
class VersionAwareApplication {
    std::map<Version, std::unique_ptr<MessageProcessor>> processors;
    
public:
    VersionAwareApplication() {
        processors[Version::V3] = std::make_unique<V3Processor>();
        processors[Version::V4] = std::make_unique<V4Processor>();
    }
    
    void process_message(const uint8_t* data, size_t size) {
        auto version = VersionDetector::detect_boe_version(data, size);
        
        auto it = processors.find(version);
        if (it != processors.end()) {
            it->second->process(data, size);
        } else {
            log_warning("Unknown protocol version detected");
        }
    }
};
```

## Best Practices

### 1. Semantic Versioning for Schemas

```
Major.Minor.Patch
3.1.0 - BOE v3, minor addition (new optional field)
3.1.1 - BOE v3.1, patch (bug fix in generated code)
4.0.0 - BOE v4, major (breaking changes)
```

### 2. Deprecation Warnings

```cpp
namespace cboe::boe::v3 {
    [[deprecated("Use cboe::boe::v4::LoginRequest instead")]]
    struct LoginRequest { /* v3 definition */ };
}
```

### 3. Version Documentation

```yaml
# schemas/cboe_boe_v4.yaml
protocol: cboe_boe
version: 4
changelog:
  - "Added OrderCancel message type (0x42)"
  - "Extended LoginRequest with optional SessionId field"
  - "Increased maximum group count from 255 to 65535"
breaking_changes:
  - "OrderId field changed from u32 to u64"
  - "Removed ClientId field from LoginRequest"
compatibility:
  forward: true   # v4 can read v3 messages
  backward: false # v3 cannot read all v4 messages
```

### 4. Automated Compatibility Testing

```cpp
// Test harness for cross-version compatibility
class CompatibilityTester {
public:
    void test_v3_to_v4_compatibility() {
        // Create v3 message
        cboe::boe::v3::LoginRequest v3_msg;
        v3_msg.Username = {'U', 'S', 'E', 'R'};
        // ... fill other fields
        
        // Encode with v3
        std::array<uint8_t, 64> buffer;
        size_t written;
        auto status = cboe::boe::v3::Encoder::encode(v3_msg, buffer.data(), buffer.size(), written);
        assert(status == status::ok);
        
        // Decode with v4 (forward compatibility)
        cboe::boe::v4::LoginRequest v4_msg;
        size_t consumed;
        status = cboe::boe::v4::Decoder::decode(buffer.data(), written, v4_msg, consumed);
        
        // Should succeed for compatible changes
        assert(status == status::ok || status == status::unknown_type);
    }
};
```

## Migration Checklist

### Pre-Migration

- [ ] Document all breaking changes
- [ ] Create compatibility test suite
- [ ] Plan rollback strategy
- [ ] Set up monitoring for version distribution

### During Migration

- [ ] Deploy new schema generators
- [ ] Update client applications gradually
- [ ] Monitor error rates and performance
- [ ] Validate cross-version communication

### Post-Migration

- [ ] Remove deprecated version support (after grace period)
- [ ] Update documentation
- [ ] Archive old schema versions
- [ ] Plan next version roadmap

---

Proper versioning strategy ensures smooth protocol evolution while maintaining system reliability and developer productivity.
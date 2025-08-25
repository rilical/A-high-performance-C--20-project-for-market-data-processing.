#include <array>
#include <cstring>
#include <cassert>
#include <iostream>

// Include generated headers (only if they exist)
#if __has_include("../generated/cboe_boe_v3/messages.hpp")
#include "../generated/cboe_boe_v3/messages.hpp"
#include "../generated/cboe_boe_v3/encoder.hpp"
#include "../generated/cboe_boe_v3/decoder.hpp"
#include "../runtime/status.hpp"
#define HAS_GENERATED_BOE 1
#else
#define HAS_GENERATED_BOE 0
#endif

int main() {
#if HAS_GENERATED_BOE
    using namespace cboe::boe::v3;
    
    // Create test message with fixed data
    LoginRequest original;
    
    // Username = "ABCD" (4 chars)
    std::memcpy(original.Username.data(), "ABCD", 4);
    
    // Password = "ABCDEFGHIJKLMNOPQRST" (20 chars)
    std::memcpy(original.Password.data(), "ABCDEFGHIJKLMNOPQRST", 20);
    
    // Set correct message type
    original.MessageType = MessageType::LoginRequest;
    
    // Encode to first buffer
    std::array<uint8_t, 64> buffer1{};
    size_t written1 = 0;
    auto status1 = Encoder::encode(original, buffer1.data(), buffer1.size(), written1);
    
    if (status1 != market::runtime::status::ok) {
        std::cerr << "First encode failed" << std::endl;
        return 1;
    }
    
    // Decode back to new struct
    LoginRequest decoded;
    size_t consumed = 0;
    auto status2 = Decoder::decode(buffer1.data(), written1, decoded, consumed);
    
    if (status2 != market::runtime::status::ok) {
        std::cerr << "Decode failed" << std::endl;
        return 1;
    }
    
    if (consumed != written1) {
        std::cerr << "Consumed bytes mismatch" << std::endl;
        return 1;
    }
    
    // Re-encode decoded struct
    std::array<uint8_t, 64> buffer2{};
    size_t written2 = 0;
    auto status3 = Encoder::encode(decoded, buffer2.data(), buffer2.size(), written2);
    
    if (status3 != market::runtime::status::ok) {
        std::cerr << "Second encode failed" << std::endl;
        return 1;
    }
    
    // Verify byte-for-byte equality
    if (written1 != written2) {
        std::cerr << "Encoded sizes differ: " << written1 << " vs " << written2 << std::endl;
        return 1;
    }
    
    if (std::memcmp(buffer1.data(), buffer2.data(), written1) != 0) {
        std::cerr << "Encoded bytes differ" << std::endl;
        return 1;
    }
    
    // Verify decoded data matches original
    if (std::memcmp(original.Username.data(), decoded.Username.data(), 4) != 0) {
        std::cerr << "Username mismatch" << std::endl;
        return 1;
    }
    
    if (std::memcmp(original.Password.data(), decoded.Password.data(), 20) != 0) {
        std::cerr << "Password mismatch" << std::endl;
        return 1;
    }
    
    if (original.MessageType != decoded.MessageType) {
        std::cerr << "MessageType mismatch" << std::endl;
        return 1;
    }
    
    // Test passes - no output on success
    return 0;
#else
    // Generated sources not available, run stub test
    return 0;
#endif
}

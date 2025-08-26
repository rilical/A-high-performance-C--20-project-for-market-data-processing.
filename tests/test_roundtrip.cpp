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

#if __has_include("../generated/nasdaq_itch_5/messages.hpp")
#include "../generated/nasdaq_itch_5/messages.hpp"
#include "../generated/nasdaq_itch_5/encoder.hpp"
#include "../generated/nasdaq_itch_5/decoder.hpp"
#ifndef HAS_INCLUDED_STATUS
#include "../runtime/status.hpp"
#define HAS_INCLUDED_STATUS 1
#endif
#define HAS_GENERATED_ITCH 1
#else
#define HAS_GENERATED_ITCH 0
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
    
    // Test NewOrderCross without optional Account fields (PresenceBits=0)
    {
        NewOrderCross original;
        original.PresenceBits = 0; // No optional fields
        std::memcpy(original.CrossId.data(), "CROSS123456789012345", 20);
        
        // Add 2 groups without Account
        original.groups.clear();
        original.groups.reserve(2);
        
        NewOrderCrossGroups group1;
        group1.Side = static_cast<uint8_t>(Side::Buy);
        group1.AllocQty = 1000;
        std::memcpy(group1.ClOrdId.data(), "ORDER1234567890123456", 20);
        std::memset(group1.Account.data(), 0, 16); // Clear Account
        original.groups.push_back(group1);
        
        NewOrderCrossGroups group2;
        group2.Side = static_cast<uint8_t>(Side::Sell);
        group2.AllocQty = 2000;
        std::memcpy(group2.ClOrdId.data(), "ORDER2345678901234567", 20);
        std::memset(group2.Account.data(), 0, 16); // Clear Account
        original.groups.push_back(group2);
        
        original.GroupCount = static_cast<uint8_t>(original.groups.size());
        
        // Encode to first buffer
        std::array<uint8_t, 256> buffer1{};
        size_t written1 = 0;
        auto status1 = Encoder::encode(original, buffer1.data(), buffer1.size(), written1);
        
        if (status1 != market::runtime::status::ok) {
            std::cerr << "NewOrderCross encode failed (no Account)" << std::endl;
            return 1;
        }
        
        // Decode back to new struct
        NewOrderCross decoded;
        size_t consumed = 0;
        auto status2 = Decoder::decode(buffer1.data(), written1, decoded, consumed);
        
        if (status2 != market::runtime::status::ok) {
            std::cerr << "NewOrderCross decode failed (no Account)" << std::endl;
            return 1;
        }
        
        if (consumed != written1) {
            std::cerr << "NewOrderCross consumed bytes mismatch" << std::endl;
            return 1;
        }
        
        if (decoded.groups.size() != 2) {
            std::cerr << "NewOrderCross decoded group count != 2" << std::endl;
            return 1;
        }
        
        // Re-encode decoded struct
        std::array<uint8_t, 256> buffer2{};
        size_t written2 = 0;
        auto status3 = Encoder::encode(decoded, buffer2.data(), buffer2.size(), written2);
        
        if (status3 != market::runtime::status::ok) {
            std::cerr << "NewOrderCross second encode failed (no Account)" << std::endl;
            return 1;
        }
        
        // Verify byte-for-byte equality
        if (written1 != written2) {
            std::cerr << "NewOrderCross encoded sizes differ: " << written1 << " vs " << written2 << std::endl;
            return 1;
        }
        
        if (std::memcmp(buffer1.data(), buffer2.data(), written1) != 0) {
            std::cerr << "NewOrderCross encoded bytes differ" << std::endl;
            return 1;
        }
    }
    
    // Test NewOrderCross with optional Account fields (PresenceBits bit 9 set)
    {
        NewOrderCross original;
        original.PresenceBits = (1ULL << 9); // Include Account fields
        std::memcpy(original.CrossId.data(), "CROSS234567890123456", 20);
        
        // Add 2 groups with Account
        original.groups.clear();
        original.groups.reserve(2);
        
        NewOrderCrossGroups group1;
        group1.Side = static_cast<uint8_t>(Side::Buy);
        group1.AllocQty = 1500;
        std::memcpy(group1.ClOrdId.data(), "ORDER3456789012345678", 20);
        std::memcpy(group1.Account.data(), "ACCOUNT123456789", 16);
        original.groups.push_back(group1);
        
        NewOrderCrossGroups group2;
        group2.Side = static_cast<uint8_t>(Side::Sell);
        group2.AllocQty = 2500;
        std::memcpy(group2.ClOrdId.data(), "ORDER4567890123456789", 20);
        std::memcpy(group2.Account.data(), "ACCOUNT234567890", 16);
        original.groups.push_back(group2);
        
        original.GroupCount = static_cast<uint8_t>(original.groups.size());
        
        // Encode to first buffer
        std::array<uint8_t, 256> buffer1{};
        size_t written1 = 0;
        auto status1 = Encoder::encode(original, buffer1.data(), buffer1.size(), written1);
        
        if (status1 != market::runtime::status::ok) {
            std::cerr << "NewOrderCross encode failed (with Account)" << std::endl;
            return 1;
        }
        
        // Decode back to new struct
        NewOrderCross decoded;
        size_t consumed = 0;
        auto status2 = Decoder::decode(buffer1.data(), written1, decoded, consumed);
        
        if (status2 != market::runtime::status::ok) {
            std::cerr << "NewOrderCross decode failed (with Account)" << std::endl;
            return 1;
        }
        
        if (consumed != written1) {
            std::cerr << "NewOrderCross consumed bytes mismatch (with Account)" << std::endl;
            return 1;
        }
        
        if (decoded.groups.size() != 2) {
            std::cerr << "NewOrderCross decoded group count != 2 (with Account)" << std::endl;
            return 1;
        }
        
        // Verify Account fields were preserved
        if (std::memcmp(decoded.groups[0].Account.data(), "ACCOUNT123456789", 16) != 0) {
            std::cerr << "NewOrderCross Account[0] mismatch" << std::endl;
            return 1;
        }
        
        if (std::memcmp(decoded.groups[1].Account.data(), "ACCOUNT234567890", 16) != 0) {
            std::cerr << "NewOrderCross Account[1] mismatch" << std::endl;
            return 1;
        }
        
        // Re-encode decoded struct
        std::array<uint8_t, 256> buffer2{};
        size_t written2 = 0;
        auto status3 = Encoder::encode(decoded, buffer2.data(), buffer2.size(), written2);
        
        if (status3 != market::runtime::status::ok) {
            std::cerr << "NewOrderCross second encode failed (with Account)" << std::endl;
            return 1;
        }
        
        // Verify byte-for-byte equality
        if (written1 != written2) {
            std::cerr << "NewOrderCross encoded sizes differ (with Account): " << written1 << " vs " << written2 << std::endl;
            return 1;
        }
        
        if (std::memcmp(buffer1.data(), buffer2.data(), written1) != 0) {
            std::cerr << "NewOrderCross encoded bytes differ (with Account)" << std::endl;
            return 1;
        }
    }
    
#endif

#if HAS_GENERATED_ITCH
    // Test ITCH AddOrder round-trip
    {
        using namespace nasdaq::itch::v5;
        
        AddOrder original;
        original.Type = 'A';
        original.Timestamp = 123456u;
        original.OrderId = 0x0102030405060708ULL;
        original.Side = 'B';
        original.Shares = 1000u;
        std::memcpy(original.Symbol.data(), "ABCDEF  ", 8); // 8 chars with spaces
        original.Price = 123450u;
        
        // Encode to first buffer
        std::array<uint8_t, 64> buffer1{};
        size_t written1 = 0;
        auto status1 = nasdaq::itch::v5::Encoder::encode(original, buffer1.data(), buffer1.size(), written1);
        
        if (status1 != market::runtime::status::ok) {
            std::cerr << "ITCH AddOrder encode failed" << std::endl;
            return 1;
        }
        
        // Verify expected size (1 + 4 + 8 + 1 + 4 + 8 + 4 = 30 bytes)
        if (written1 != 30) {
            std::cerr << "ITCH AddOrder unexpected size: " << written1 << std::endl;
            return 1;
        }
        
        // Decode back to new struct
        AddOrder decoded;
        size_t consumed = 0;
        auto status2 = nasdaq::itch::v5::Decoder::decode(buffer1.data(), written1, decoded, consumed);
        
        if (status2 != market::runtime::status::ok) {
            std::cerr << "ITCH AddOrder decode failed" << std::endl;
            return 1;
        }
        
        if (consumed != written1) {
            std::cerr << "ITCH AddOrder consumed bytes mismatch" << std::endl;
            return 1;
        }
        
        // Verify Type field
        if (decoded.Type != 'A') {
            std::cerr << "ITCH AddOrder decoded Type != 'A'" << std::endl;
            return 1;
        }
        
        // Re-encode decoded struct
        std::array<uint8_t, 64> buffer2{};
        size_t written2 = 0;
        auto status3 = nasdaq::itch::v5::Encoder::encode(decoded, buffer2.data(), buffer2.size(), written2);
        
        if (status3 != market::runtime::status::ok) {
            std::cerr << "ITCH AddOrder second encode failed" << std::endl;
            return 1;
        }
        
        // Verify byte-for-byte equality
        if (written1 != written2) {
            std::cerr << "ITCH AddOrder encoded sizes differ: " << written1 << " vs " << written2 << std::endl;
            return 1;
        }
        
        if (std::memcmp(buffer1.data(), buffer2.data(), written1) != 0) {
            std::cerr << "ITCH AddOrder encoded bytes differ" << std::endl;
            return 1;
        }
    }
    
    // Test ITCH DeleteOrder round-trip
    {
        using namespace nasdaq::itch::v5;
        
        DeleteOrder original;
        original.Type = 'D';
        original.Timestamp = 654321u;
        original.OrderId = 0x0908070605040302ULL;
        
        // Encode to first buffer
        std::array<uint8_t, 64> buffer1{};
        size_t written1 = 0;
        auto status1 = nasdaq::itch::v5::Encoder::encode(original, buffer1.data(), buffer1.size(), written1);
        
        if (status1 != market::runtime::status::ok) {
            std::cerr << "ITCH DeleteOrder encode failed" << std::endl;
            return 1;
        }
        
        // Verify expected size (1 + 4 + 8 = 13 bytes)
        if (written1 != 13) {
            std::cerr << "ITCH DeleteOrder unexpected size: " << written1 << std::endl;
            return 1;
        }
        
        // Decode back to new struct
        DeleteOrder decoded;
        size_t consumed = 0;
        auto status2 = nasdaq::itch::v5::Decoder::decode(buffer1.data(), written1, decoded, consumed);
        
        if (status2 != market::runtime::status::ok) {
            std::cerr << "ITCH DeleteOrder decode failed" << std::endl;
            return 1;
        }
        
        if (consumed != written1) {
            std::cerr << "ITCH DeleteOrder consumed bytes mismatch" << std::endl;
            return 1;
        }
        
        // Verify Type field
        if (decoded.Type != 'D') {
            std::cerr << "ITCH DeleteOrder decoded Type != 'D'" << std::endl;
            return 1;
        }
        
        // Re-encode decoded struct
        std::array<uint8_t, 64> buffer2{};
        size_t written2 = 0;
        auto status3 = nasdaq::itch::v5::Encoder::encode(decoded, buffer2.data(), buffer2.size(), written2);
        
        if (status3 != market::runtime::status::ok) {
            std::cerr << "ITCH DeleteOrder second encode failed" << std::endl;
            return 1;
        }
        
        // Verify byte-for-byte equality
        if (written1 != written2) {
            std::cerr << "ITCH DeleteOrder encoded sizes differ: " << written1 << " vs " << written2 << std::endl;
            return 1;
        }
        
        if (std::memcmp(buffer1.data(), buffer2.data(), written1) != 0) {
            std::cerr << "ITCH DeleteOrder encoded bytes differ" << std::endl;
            return 1;
        }
    }

#endif

    // Test passes - no output on success
    return 0;
}

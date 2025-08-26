#include <array>
#include <chrono>
#include <cstring>
#include <iostream>
#include <cstdlib>

// Include generated headers (only if they exist)
#if __has_include("../generated/cboe_boe_v3/messages.hpp")
#include "../generated/cboe_boe_v3/messages.hpp"
#include "../generated/cboe_boe_v3/encoder.hpp"
#include "../generated/cboe_boe_v3/decoder.hpp"
#define HAS_GENERATED_BOE 1
#else
#define HAS_GENERATED_BOE 0
#endif

#if __has_include("../generated/nasdaq_itch_5/messages.hpp")
#include "../generated/nasdaq_itch_5/messages.hpp"
#include "../generated/nasdaq_itch_5/encoder.hpp"
#include "../generated/nasdaq_itch_5/decoder.hpp"
#define HAS_GENERATED_ITCH 1
#else
#define HAS_GENERATED_ITCH 0
#endif

using namespace std::chrono;

template<typename Func>
double benchmark_ns_per_op(Func&& func, size_t iterations) {
    // Warmup (5% of iterations)
    size_t warmup = iterations / 20;
    for (size_t i = 0; i < warmup; ++i) {
        func();
    }
    
    // Timed run
    auto start = steady_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        func();
    }
    auto end = steady_clock::now();
    
    auto duration_ns = duration_cast<nanoseconds>(end - start).count();
    return static_cast<double>(duration_ns) / static_cast<double>(iterations);
}

int main() {
    // Get iteration count from environment variable, default to 1M
    const char* iter_env = std::getenv("ITER");
    size_t iterations = iter_env ? std::strtoul(iter_env, nullptr, 10) : 1'000'000;
    
    std::cout << "Running benchmarks with " << iterations << " iterations" << std::endl;
    std::cout << std::endl;

#if HAS_GENERATED_BOE
    using namespace cboe::boe::v3;
    
    // ===== BOE LoginRequest Benchmark =====
    {
        LoginRequest msg;
        std::memcpy(msg.Username.data(), "TEST", 4);
        std::memcpy(msg.Password.data(), "PASSWORD123456789012", 20);
        msg.MessageType = MessageType::LoginRequest;
        
        std::array<uint8_t, 64> buffer{};
        size_t written = 0, consumed = 0;
        
        // Encode benchmark
        auto encode_ns = benchmark_ns_per_op([&]() {
            auto status = Encoder::encode(msg, buffer.data(), buffer.size(), written);
            (void)status; // Suppress unused variable warning
        }, iterations);
        
        // Decode benchmark
        LoginRequest decoded_msg;
        auto decode_ns = benchmark_ns_per_op([&]() {
            auto status = Decoder::decode(buffer.data(), written, decoded_msg, consumed);
            (void)status; // Suppress unused variable warning
        }, iterations);
        
        std::cout << "BOE::LoginRequest decode: " << static_cast<int>(decode_ns) 
                  << " ns/msg (N=" << iterations << ", size=" << written << ")" << std::endl;
        std::cout << "BOE::LoginRequest encode: " << static_cast<int>(encode_ns) 
                  << " ns/msg (N=" << iterations << ", size=" << written << ")" << std::endl;
    }
    
    // ===== BOE NewOrderCross Benchmark (no optional) =====
    {
        NewOrderCross msg;
        msg.PresenceBits = 0; // No optional fields
        std::memcpy(msg.CrossId.data(), "CROSS123456789012345", 20);
        msg.GroupCount = 2;
        
        // Add 2 groups
        NewOrderCrossGroups group1;
        group1.Side = static_cast<uint8_t>(Side::Buy);
        group1.AllocQty = 1000;
        std::memcpy(group1.ClOrdId.data(), "ORDER12345678901234X", 20);
        std::memset(group1.Account.data(), 0, 16); // Clear Account
        msg.groups.push_back(group1);
        
        NewOrderCrossGroups group2;
        group2.Side = static_cast<uint8_t>(Side::Sell);
        group2.AllocQty = 2000;
        std::memcpy(group2.ClOrdId.data(), "ORDER12345678901234Y", 20);
        std::memset(group2.Account.data(), 0, 16); // Clear Account
        msg.groups.push_back(group2);
        
        std::array<uint8_t, 256> buffer{};
        size_t written = 0, consumed = 0;
        
        // Encode benchmark
        auto encode_ns = benchmark_ns_per_op([&]() {
            auto status = Encoder::encode(msg, buffer.data(), buffer.size(), written);
            (void)status; // Suppress unused variable warning
        }, iterations);
        
        // Decode benchmark
        NewOrderCross decoded_msg;
        auto decode_ns = benchmark_ns_per_op([&]() {
            auto status = Decoder::decode(buffer.data(), written, decoded_msg, consumed);
            (void)status; // Suppress unused variable warning
        }, iterations);
        
        std::cout << "BOE::NewOrderCross(no-opt) decode: " << static_cast<int>(decode_ns) 
                  << " ns/msg (N=" << iterations << ", size=" << written << ")" << std::endl;
        std::cout << "BOE::NewOrderCross(no-opt) encode: " << static_cast<int>(encode_ns) 
                  << " ns/msg (N=" << iterations << ", size=" << written << ")" << std::endl;
    }
    
    // ===== BOE NewOrderCross Benchmark (with Account) =====
    {
        NewOrderCross msg;
        msg.PresenceBits = (1ULL << 9); // Account present
        std::memcpy(msg.CrossId.data(), "CROSS123456789012345", 20);
        msg.GroupCount = 2;
        
        // Add 2 groups with Account
        NewOrderCrossGroups group1;
        group1.Side = static_cast<uint8_t>(Side::Buy);
        group1.AllocQty = 1000;
        std::memcpy(group1.ClOrdId.data(), "ORDER12345678901234X", 20);
        std::memcpy(group1.Account.data(), "ACCOUNT123456789", 16);
        msg.groups.push_back(group1);
        
        NewOrderCrossGroups group2;
        group2.Side = static_cast<uint8_t>(Side::Sell);
        group2.AllocQty = 2000;
        std::memcpy(group2.ClOrdId.data(), "ORDER12345678901234Y", 20);
        std::memcpy(group2.Account.data(), "ACCOUNT234567890", 16);
        msg.groups.push_back(group2);
        
        std::array<uint8_t, 256> buffer{};
        size_t written = 0, consumed = 0;
        
        // Encode benchmark
        auto encode_ns = benchmark_ns_per_op([&]() {
            auto status = Encoder::encode(msg, buffer.data(), buffer.size(), written);
            (void)status; // Suppress unused variable warning
        }, iterations);
        
        // Decode benchmark
        NewOrderCross decoded_msg;
        auto decode_ns = benchmark_ns_per_op([&]() {
            auto status = Decoder::decode(buffer.data(), written, decoded_msg, consumed);
            (void)status; // Suppress unused variable warning
        }, iterations);
        
        std::cout << "BOE::NewOrderCross(+Account) decode: " << static_cast<int>(decode_ns) 
                  << " ns/msg (N=" << iterations << ", size=" << written << ")" << std::endl;
        std::cout << "BOE::NewOrderCross(+Account) encode: " << static_cast<int>(encode_ns) 
                  << " ns/msg (N=" << iterations << ", size=" << written << ")" << std::endl;
    }
#endif

#if HAS_GENERATED_ITCH
    using namespace nasdaq::itch::v5;
    
    // ===== ITCH AddOrder Benchmark =====
    {
        AddOrder msg;
        msg.Type = 'A';
        msg.Timestamp = 123456u;
        msg.OrderId = 0x1234567890ABCDEFULL;
        msg.Side = 'B';
        msg.Shares = 1000u;
        std::memcpy(msg.Symbol.data(), "TESTSMBL", 8);
        msg.Price = 50000u;
        
        std::array<uint8_t, 64> buffer{};
        size_t written = 0, consumed = 0;
        
        // Encode benchmark
        auto encode_ns = benchmark_ns_per_op([&]() {
            auto status = nasdaq::itch::v5::Encoder::encode(msg, buffer.data(), buffer.size(), written);
            (void)status; // Suppress unused variable warning
        }, iterations);
        
        // Decode benchmark
        AddOrder decoded_msg;
        auto decode_ns = benchmark_ns_per_op([&]() {
            auto status = nasdaq::itch::v5::Decoder::decode(buffer.data(), written, decoded_msg, consumed);
            (void)status; // Suppress unused variable warning
        }, iterations);
        
        std::cout << "ITCH::AddOrder decode: " << static_cast<int>(decode_ns) 
                  << " ns/msg (N=" << iterations << ", size=" << written << ")" << std::endl;
        std::cout << "ITCH::AddOrder encode: " << static_cast<int>(encode_ns) 
                  << " ns/msg (N=" << iterations << ", size=" << written << ")" << std::endl;
    }
    
    // ===== ITCH DeleteOrder Benchmark =====
    {
        DeleteOrder msg;
        msg.Type = 'D';
        msg.Timestamp = 654321u;
        msg.OrderId = 0xFEDCBA0987654321ULL;
        
        std::array<uint8_t, 64> buffer{};
        size_t written = 0, consumed = 0;
        
        // Encode benchmark
        auto encode_ns = benchmark_ns_per_op([&]() {
            auto status = nasdaq::itch::v5::Encoder::encode(msg, buffer.data(), buffer.size(), written);
            (void)status; // Suppress unused variable warning
        }, iterations);
        
        // Decode benchmark
        DeleteOrder decoded_msg;
        auto decode_ns = benchmark_ns_per_op([&]() {
            auto status = nasdaq::itch::v5::Decoder::decode(buffer.data(), written, decoded_msg, consumed);
            (void)status; // Suppress unused variable warning
        }, iterations);
        
        std::cout << "ITCH::DeleteOrder decode: " << static_cast<int>(decode_ns) 
                  << " ns/msg (N=" << iterations << ", size=" << written << ")" << std::endl;
        std::cout << "ITCH::DeleteOrder encode: " << static_cast<int>(encode_ns) 
                  << " ns/msg (N=" << iterations << ", size=" << written << ")" << std::endl;
    }
#endif

    std::cout << std::endl;
    std::cout << "Benchmark completed successfully!" << std::endl;
    return 0;
}
#include <array>
#include <cstring>
#include <cassert>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include "runtime/status.hpp"
#include "runtime/bytes.hpp"

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

struct CanonicalBuffers {
    std::array<uint8_t, 256> boe_login_buffer{};
    size_t boe_login_size = 0;
    
    std::array<uint8_t, 256> boe_neworder_no_account_buffer{};
    size_t boe_neworder_no_account_size = 0;
    
    std::array<uint8_t, 256> boe_neworder_with_account_buffer{};
    size_t boe_neworder_with_account_size = 0;
    
    std::array<uint8_t, 256> itch_addorder_buffer{};
    size_t itch_addorder_size = 0;
};

// Create canonical buffers once
bool create_canonical_buffers(CanonicalBuffers& buffers) {
#if HAS_GENERATED_BOE
    // Create BOE LoginRequest
    {
        using namespace cboe::boe::v3;
        LoginRequest original;
        std::memcpy(original.Username.data(), "TEST", 4);
        std::memcpy(original.Password.data(), "PASSWORD123456789012", 20);
        original.MessageType = MessageType::LoginRequest;
        
        auto status = Encoder::encode(original, buffers.boe_login_buffer.data(), 
                                    buffers.boe_login_buffer.size(), buffers.boe_login_size);
        if (status != market::runtime::status::ok) {
            std::cerr << "Failed to create BOE LoginRequest canonical buffer" << std::endl;
            return false;
        }
    }
    
    // Create BOE NewOrderCross without Account
    {
        using namespace cboe::boe::v3;
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
        std::memset(group1.Account.data(), 0, 16);
        original.groups.push_back(group1);
        
        NewOrderCrossGroups group2;
        group2.Side = static_cast<uint8_t>(Side::Sell);
        group2.AllocQty = 2000;
        std::memcpy(group2.ClOrdId.data(), "ORDER2345678901234567", 20);
        std::memset(group2.Account.data(), 0, 16);
        original.groups.push_back(group2);
        
        original.GroupCount = static_cast<uint8_t>(original.groups.size());
        
        auto status = Encoder::encode(original, buffers.boe_neworder_no_account_buffer.data(),
                                    buffers.boe_neworder_no_account_buffer.size(), 
                                    buffers.boe_neworder_no_account_size);
        if (status != market::runtime::status::ok) {
            std::cerr << "Failed to create BOE NewOrderCross (no account) canonical buffer" << std::endl;
            return false;
        }
    }
    
    // Create BOE NewOrderCross with Account
    {
        using namespace cboe::boe::v3;
        NewOrderCross original;
        original.PresenceBits = (1ULL << 9); // Account is present (bit 9)
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
        
        auto status = Encoder::encode(original, buffers.boe_neworder_with_account_buffer.data(),
                                    buffers.boe_neworder_with_account_buffer.size(), 
                                    buffers.boe_neworder_with_account_size);
        if (status != market::runtime::status::ok) {
            std::cerr << "Failed to create BOE NewOrderCross (with account) canonical buffer" << std::endl;
            return false;
        }
    }
#endif

#if HAS_GENERATED_ITCH
    // Create ITCH AddOrder
    {
        using namespace nasdaq::itch::v5;
        AddOrder original;
        original.Type = 'A';
        original.Timestamp = 123456u;
        original.OrderId = 0x0102030405060708ULL;
        original.Side = 'B';
        original.Shares = 1000u;
        std::memcpy(original.Symbol.data(), "ABCDEF  ", 8);
        original.Price = 123450u;
        
        auto status = nasdaq::itch::v5::Encoder::encode(original, buffers.itch_addorder_buffer.data(),
                                                       buffers.itch_addorder_buffer.size(), 
                                                       buffers.itch_addorder_size);
        if (status != market::runtime::status::ok) {
            std::cerr << "Failed to create ITCH AddOrder canonical buffer" << std::endl;
            return false;
        }
    }
#endif

    return true;
}

// Thread worker function
void thread_worker(int thread_id, const CanonicalBuffers& canonical, 
                  int iterations, std::atomic<bool>& error_flag) {
    constexpr size_t BUFFER_SIZE = 256;
    
    for (int i = 0; i < iterations && !error_flag.load(); ++i) {
#if HAS_GENERATED_BOE
        // Test BOE LoginRequest
        {
            using namespace cboe::boe::v3;
            
            // Decode from canonical buffer
            LoginRequest decoded;
            size_t consumed = 0;
            auto decode_status = Decoder::decode(canonical.boe_login_buffer.data(), 
                                               canonical.boe_login_size, decoded, consumed);
            if (decode_status != market::runtime::status::ok || consumed != canonical.boe_login_size) {
                std::cerr << "Thread " << thread_id << " BOE LoginRequest decode failed at iteration " << i << std::endl;
                error_flag.store(true);
                return;
            }
            
            // Re-encode to thread-local buffer
            std::array<uint8_t, BUFFER_SIZE> encode_buffer{};
            size_t written = 0;
            auto encode_status = Encoder::encode(decoded, encode_buffer.data(), encode_buffer.size(), written);
            if (encode_status != market::runtime::status::ok || written != canonical.boe_login_size) {
                std::cerr << "Thread " << thread_id << " BOE LoginRequest encode failed at iteration " << i << std::endl;
                error_flag.store(true);
                return;
            }
            
            // Verify byte-for-byte equality
            if (std::memcmp(canonical.boe_login_buffer.data(), encode_buffer.data(), written) != 0) {
                std::cerr << "Thread " << thread_id << " BOE LoginRequest memcmp failed at iteration " << i << std::endl;
                error_flag.store(true);
                return;
            }
        }
        
        // Test BOE NewOrderCross (no account)
        {
            using namespace cboe::boe::v3;
            
            NewOrderCross decoded;
            size_t consumed = 0;
            auto decode_status = Decoder::decode(canonical.boe_neworder_no_account_buffer.data(), 
                                               canonical.boe_neworder_no_account_size, decoded, consumed);
            if (decode_status != market::runtime::status::ok || consumed != canonical.boe_neworder_no_account_size) {
                std::cerr << "Thread " << thread_id << " BOE NewOrderCross (no account) decode failed at iteration " << i << std::endl;
                error_flag.store(true);
                return;
            }
            
            std::array<uint8_t, BUFFER_SIZE> encode_buffer{};
            size_t written = 0;
            auto encode_status = Encoder::encode(decoded, encode_buffer.data(), encode_buffer.size(), written);
            if (encode_status != market::runtime::status::ok || written != canonical.boe_neworder_no_account_size) {
                std::cerr << "Thread " << thread_id << " BOE NewOrderCross (no account) encode failed at iteration " << i << std::endl;
                error_flag.store(true);
                return;
            }
            
            if (std::memcmp(canonical.boe_neworder_no_account_buffer.data(), encode_buffer.data(), written) != 0) {
                std::cerr << "Thread " << thread_id << " BOE NewOrderCross (no account) memcmp failed at iteration " << i << std::endl;
                error_flag.store(true);
                return;
            }
        }
        
        // Test BOE NewOrderCross (with account)
        {
            using namespace cboe::boe::v3;
            
            NewOrderCross decoded;
            size_t consumed = 0;
            auto decode_status = Decoder::decode(canonical.boe_neworder_with_account_buffer.data(), 
                                               canonical.boe_neworder_with_account_size, decoded, consumed);
            if (decode_status != market::runtime::status::ok || consumed != canonical.boe_neworder_with_account_size) {
                std::cerr << "Thread " << thread_id << " BOE NewOrderCross (with account) decode failed at iteration " << i << std::endl;
                error_flag.store(true);
                return;
            }
            
            std::array<uint8_t, BUFFER_SIZE> encode_buffer{};
            size_t written = 0;
            auto encode_status = Encoder::encode(decoded, encode_buffer.data(), encode_buffer.size(), written);
            if (encode_status != market::runtime::status::ok || written != canonical.boe_neworder_with_account_size) {
                std::cerr << "Thread " << thread_id << " BOE NewOrderCross (with account) encode failed at iteration " << i << std::endl;
                error_flag.store(true);
                return;
            }
            
            if (std::memcmp(canonical.boe_neworder_with_account_buffer.data(), encode_buffer.data(), written) != 0) {
                std::cerr << "Thread " << thread_id << " BOE NewOrderCross (with account) memcmp failed at iteration " << i << std::endl;
                error_flag.store(true);
                return;
            }
        }
#endif

#if HAS_GENERATED_ITCH
        // Test ITCH AddOrder
        {
            using namespace nasdaq::itch::v5;
            
            AddOrder decoded;
            size_t consumed = 0;
            auto decode_status = nasdaq::itch::v5::Decoder::decode(canonical.itch_addorder_buffer.data(), 
                                                                 canonical.itch_addorder_size, decoded, consumed);
            if (decode_status != market::runtime::status::ok || consumed != canonical.itch_addorder_size) {
                std::cerr << "Thread " << thread_id << " ITCH AddOrder decode failed at iteration " << i << std::endl;
                error_flag.store(true);
                return;
            }
            
            std::array<uint8_t, BUFFER_SIZE> encode_buffer{};
            size_t written = 0;
            auto encode_status = nasdaq::itch::v5::Encoder::encode(decoded, encode_buffer.data(), encode_buffer.size(), written);
            if (encode_status != market::runtime::status::ok || written != canonical.itch_addorder_size) {
                std::cerr << "Thread " << thread_id << " ITCH AddOrder encode failed at iteration " << i << std::endl;
                error_flag.store(true);
                return;
            }
            
            if (std::memcmp(canonical.itch_addorder_buffer.data(), encode_buffer.data(), written) != 0) {
                std::cerr << "Thread " << thread_id << " ITCH AddOrder memcmp failed at iteration " << i << std::endl;
                error_flag.store(true);
                return;
            }
        }
#endif
    }
}

int main() {
    std::cout << "Starting multi-threaded decode/encode stress test..." << std::endl;
    
    // Create canonical buffers
    CanonicalBuffers canonical;
    if (!create_canonical_buffers(canonical)) {
        std::cerr << "Failed to create canonical buffers" << std::endl;
        return 1;
    }
    
    std::cout << "Canonical buffers created:" << std::endl;
#if HAS_GENERATED_BOE
    std::cout << "  BOE LoginRequest: " << canonical.boe_login_size << " bytes" << std::endl;
    std::cout << "  BOE NewOrderCross (no account): " << canonical.boe_neworder_no_account_size << " bytes" << std::endl;
    std::cout << "  BOE NewOrderCross (with account): " << canonical.boe_neworder_with_account_size << " bytes" << std::endl;
#endif
#if HAS_GENERATED_ITCH
    std::cout << "  ITCH AddOrder: " << canonical.itch_addorder_size << " bytes" << std::endl;
#endif
    
    // Test configuration
    const int num_threads = std::min(static_cast<int>(std::thread::hardware_concurrency()), 8);
    const int iterations_per_thread = 100000;
    
    std::cout << "Launching " << num_threads << " threads, " << iterations_per_thread 
              << " iterations each (" << (num_threads * iterations_per_thread) << " total operations)" << std::endl;
    
    // Start timing
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Launch threads
    std::atomic<bool> error_flag{false};
    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(thread_worker, i, std::cref(canonical), iterations_per_thread, std::ref(error_flag));
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    
    if (error_flag.load()) {
        std::cerr << "Multi-threaded test FAILED with errors!" << std::endl;
        return 1;
    }
    
    std::cout << "Multi-threaded test PASSED!" << std::endl;
    std::cout << "Completed in " << duration_ms << " ms" << std::endl;
    
    const int total_operations = num_threads * iterations_per_thread * 4; // 4 message types per iteration
    const double ops_per_sec = (total_operations * 1000.0) / duration_ms;
    std::cout << "Performance: " << static_cast<int>(ops_per_sec) << " decode/encode operations per second" << std::endl;
    
    return 0;
}

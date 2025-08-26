#include <cstddef>
#include <cstdint>

// Include generated BOE headers
#include "../generated/cboe_boe_v3/messages.hpp"
#include "../generated/cboe_boe_v3/decoder.hpp"

// libFuzzer entry point
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // Early exit for empty or very small buffers
    if (size < 5) {
        return 0;  // BOE messages need at least 5 bytes for preamble + type
    }
    
    using namespace cboe::boe::v3;
    
    // Try to decode as LoginRequest
    {
        LoginRequest msg;
        size_t consumed = 0;
        
        // Safe decode attempt - decoder will handle size validation
        auto status = Decoder::decode(data, size, msg, consumed);
        
        // Ignore status - we're just testing for crashes/undefined behavior
        (void)status;
        (void)consumed;
    }
    
    // Try to decode as NewOrderCross
    {
        NewOrderCross msg;
        size_t consumed = 0;
        
        // Safe decode attempt - decoder will handle size validation
        auto status = Decoder::decode(data, size, msg, consumed);
        
        // Ignore status - we're just testing for crashes/undefined behavior
        (void)status;
        (void)consumed;
    }
    
    // Also try with a subspan if we have enough data
    if (size >= 10) {
        // Try LoginRequest from offset 1
        {
            LoginRequest msg;
            size_t consumed = 0;
            
            auto status = Decoder::decode(data + 1, size - 1, msg, consumed);
            (void)status;
            (void)consumed;
        }
        
        // Try NewOrderCross from offset 2
        if (size >= 12) {
            NewOrderCross msg;
            size_t consumed = 0;
            
            auto status = Decoder::decode(data + 2, size - 2, msg, consumed);
            (void)status;
            (void)consumed;
        }
    }
    
    // Try with very short buffers to test edge cases
    if (size >= 1) {
        LoginRequest msg;
        size_t consumed = 0;
        
        // Test with progressively smaller buffers
        for (size_t test_size = 1; test_size <= 4 && test_size <= size; ++test_size) {
            auto status = Decoder::decode(data, test_size, msg, consumed);
            (void)status;
            (void)consumed;
        }
    }
    
    return 0;  // Always return 0 (non-crash)
}

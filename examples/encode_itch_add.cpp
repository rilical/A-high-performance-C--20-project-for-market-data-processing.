// Example: Encode ITCH AddOrder message
#include <iostream>
#include <iomanip>
#include <array>
#include <cstring>

#include "generated/nasdaq_itch_5/messages.hpp"
#include "generated/nasdaq_itch_5/encoder.hpp"
#include "runtime/status.hpp"

int main() {
    using namespace nasdaq::itch::v5;

    // Create an ITCH AddOrder message
    AddOrder msg;
    msg.Type = 'A';
    msg.Timestamp = 123456u;
    msg.OrderId = 0x0102030405060708ULL;
    msg.Side = 'B';
    msg.Shares = 1000u;
    std::memcpy(msg.Symbol.data(), "ABCDEF  ", 8); // 8 chars (pad with spaces)
    msg.Price = 123450u;

    // Prepare encoding buffer
    std::array<uint8_t, 64> buffer{};
    size_t written = 0;

    // Encode the message
    auto status = Encoder::encode(msg, buffer.data(), buffer.size(), written);
    if (status != market::runtime::status::ok) {
        std::cerr << "Encoding failed: " << market::runtime::status_to_string(status) << std::endl;
        return 1;
    }

    // Print results
    std::cout << "size=" << written << "\n";

    // Print hex bytes
    std::cout << std::hex << std::setfill('0');
    for (size_t i = 0; i < written; ++i) {
        std::cout << std::setw(2) << static_cast<unsigned>(buffer[i]);
    }
    std::cout << std::endl;

    return 0;
}


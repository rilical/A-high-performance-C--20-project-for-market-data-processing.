// Example: Encode ITCH DeleteOrder message
#include <iostream>
#include <iomanip>
#include <array>

#include "generated/nasdaq_itch_5/messages.hpp"
#include "generated/nasdaq_itch_5/encoder.hpp"

int main() {
    using namespace nasdaq::itch::v5;
    DeleteOrder msg{}; msg.Type='D'; msg.Timestamp=654321; msg.OrderId=0x0102030405060708ULL;
    std::array<uint8_t, 64> buf{}; size_t written=0;
    auto st = Encoder::encode(msg, buf.data(), buf.size(), written);
    if (st != market::runtime::status::ok) { std::cerr << "encode failed" << std::endl; return 1; }
    std::cout << "size=" << written << "\n";
    std::cout << std::hex << std::setfill('0');
    for (size_t i=0;i<written;++i) std::cout << std::setw(2) << +buf[i];
    std::cout << std::endl;
    return 0;
}


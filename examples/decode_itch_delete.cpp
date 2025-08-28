// Example: Decode ITCH DeleteOrder message from hex
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "generated/nasdaq_itch_5/messages.hpp"
#include "generated/nasdaq_itch_5/decoder.hpp"

static std::vector<uint8_t> hex_to_bytes(const std::string& hex) {
    std::vector<uint8_t> bytes; bytes.reserve(hex.size()/2);
    for (size_t i=0;i+1<hex.size();i+=2) bytes.push_back(static_cast<uint8_t>(std::stoul(hex.substr(i,2), nullptr, 16)));
    return bytes;
}

int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Usage: " << argv[0] << " <hex_file>" << std::endl; return 1; }
    std::ifstream in(argv[1]); if (!in) { std::cerr << "open fail" << std::endl; return 1; }
    std::string hex, line; while (std::getline(in, line)) for (char c: line) if (std::isxdigit(static_cast<unsigned char>(c))) hex += c;
    auto bytes = hex_to_bytes(hex);
    nasdaq::itch::v5::DeleteOrder m{}; size_t consumed=0;
    auto st = nasdaq::itch::v5::Decoder::decode(bytes.data(), bytes.size(), m, consumed);
    if (st != market::runtime::status::ok) { std::cerr << "decode failed" << std::endl; return 1; }
    std::cout << "Type=" << m.Type << " Timestamp=" << m.Timestamp << " OrderId=" << m.OrderId << " consumed=" << consumed << std::endl;
    return 0;
}


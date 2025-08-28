// Example: Decode ITCH AddOrder message from hex file
#include <iostream>
#include <fstream>
#include <string>
#include <array>
#include <sstream>
#include <vector>
#include <cctype>

#include "generated/nasdaq_itch_5/messages.hpp"
#include "generated/nasdaq_itch_5/decoder.hpp"
#include "runtime/status.hpp"

static std::vector<uint8_t> hex_to_bytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    bytes.reserve(hex.size() / 2);
    for (size_t i = 0; i + 1 < hex.length(); i += 2) {
        std::string byte_str = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <hex_file>" << std::endl;
        return 1;
    }

    // Read hex file
    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Cannot open file: " << argv[1] << std::endl;
        return 1;
    }

    std::string hex_content;
    std::string line;
    while (std::getline(file, line)) {
        for (char c : line) {
            if (std::isxdigit(static_cast<unsigned char>(c))) {
                hex_content += c;
            }
        }
    }

    // Convert hex to bytes
    auto bytes = hex_to_bytes(hex_content);

    // Decode the message
    nasdaq::itch::v5::AddOrder out{};
    size_t consumed = 0;
    auto status = nasdaq::itch::v5::Decoder::decode(bytes.data(), bytes.size(), out, consumed);

    if (status != market::runtime::status::ok) {
        std::cerr << "Decoding failed: " << market::runtime::status_to_string(status) << std::endl;
        return 1;
    }

    // Print decoded fields
    std::cout << "Type=" << out.Type << "\n";
    std::cout << "Timestamp=" << out.Timestamp << "\n";
    std::cout << "OrderId=" << out.OrderId << "\n";
    std::cout << "Side=" << out.Side << "\n";
    std::cout << "Shares=" << out.Shares << "\n";
    std::cout << "Symbol=" << std::string(out.Symbol.data(), out.Symbol.size()) << "\n";
    std::cout << "Price=" << out.Price << "\n";
    std::cout << "consumed=" << consumed << std::endl;

    return 0;
}


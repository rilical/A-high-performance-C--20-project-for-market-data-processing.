// Example: Decode BOE LoginRequest message from hex file
#include <iostream>
#include <fstream>
#include <string>
#include <array>
#include <sstream>

#include "generated/cboe_boe_v3/messages.hpp"
#include "generated/cboe_boe_v3/decoder.hpp"

std::vector<uint8_t> hex_to_bytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
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
        // Remove whitespace and combine lines
        for (char c : line) {
            if (std::isxdigit(c)) {
                hex_content += c;
            }
        }
    }
    
    // Convert hex to bytes
    auto bytes = hex_to_bytes(hex_content);
    
    // Decode the message
    cboe::boe::v3::LoginRequest login;
    size_t consumed;
    auto status = cboe::boe::v3::Decoder::decode(bytes.data(), bytes.size(), login, consumed);
    
    if (status != market::runtime::status::ok) {
        std::cerr << "Decoding failed" << std::endl;
        return 1;
    }
    
    // Print decoded fields
    std::cout << "Username=" << std::string(login.Username.data(), 4) << std::endl;
    std::cout << "Password=" << std::string(login.Password.data(), 20) << std::endl;
    std::cout << "consumed=" << consumed << std::endl;
    
    return 0;
}

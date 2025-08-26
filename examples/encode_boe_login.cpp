// Example: Encode BOE LoginRequest message
#include <iostream>
#include <iomanip>
#include <array>
#include <cstring>

#include "generated/cboe_boe_v3/messages.hpp"
#include "generated/cboe_boe_v3/encoder.hpp"

int main() {
    // Create a BOE LoginRequest message
    cboe::boe::v3::LoginRequest login;
    
    // Set username (4 characters)
    login.Username[0] = 'A';
    login.Username[1] = 'B';
    login.Username[2] = 'C';
    login.Username[3] = 'D';
    
    // Set password (20 characters)
    const char* password = "ABCDEFGHIJKLMNOPQRST";
    std::memcpy(login.Password.data(), password, 20);
    
    // Prepare encoding buffer
    std::array<uint8_t, 64> buffer;
    size_t written;
    
    // Encode the message
    auto status = cboe::boe::v3::Encoder::encode(login, buffer.data(), buffer.size(), written);
    
    if (status != market::runtime::status::ok) {
        std::cerr << "Encoding failed" << std::endl;
        return 1;
    }
    
    // Print results
    std::cout << "size=" << written << std::endl;
    
    // Print hex bytes
    std::cout << std::hex << std::setfill('0');
    for (size_t i = 0; i < written; ++i) {
        std::cout << std::setw(2) << static_cast<unsigned>(buffer[i]);
    }
    std::cout << std::endl;
    
    return 0;
}

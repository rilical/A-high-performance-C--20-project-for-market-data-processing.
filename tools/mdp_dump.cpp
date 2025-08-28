// Unified CLI to decode BOE/ITCH messages and dump as JSON
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cctype>

#include "runtime/bytes.hpp"
#include "runtime/status.hpp"

#if __has_include("generated/cboe_boe_v3/handler.hpp")
#include "generated/cboe_boe_v3/handler.hpp"
#include "generated/cboe_boe_v3/json.hpp"
#endif
#if __has_include("generated/nasdaq_itch_5/handler.hpp")
#include "generated/nasdaq_itch_5/handler.hpp"
#include "generated/nasdaq_itch_5/json.hpp"
#endif

static std::vector<uint8_t> read_all_bytes(std::istream& in) {
    std::vector<uint8_t> data;
    std::string buf((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    data.assign(buf.begin(), buf.end());
    return data;
}

static std::vector<uint8_t> read_hex_stream(std::istream& in) {
    std::string hex;
    std::string line;
    while (std::getline(in, line)) {
        for (unsigned char c : line) {
            if (std::isxdigit(c)) hex.push_back(static_cast<char>(c));
        }
    }
    std::vector<uint8_t> out;
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        out.push_back(static_cast<uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16)));
    }
    return out;
}

int main(int argc, char** argv) {
    std::string protocol;
    bool is_hex = false;
    std::string file;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--protocol" && i + 1 < argc) {
            protocol = argv[++i];
        } else if (arg == "--hex") {
            is_hex = true;
        } else if (arg == "-f" && i + 1 < argc) {
            file = argv[++i];
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: mdp_dump --protocol boe|itch [--hex] [-f input]" << std::endl;
            return 0;
        }
    }

    if (protocol != "boe" && protocol != "itch") {
        std::cerr << "Missing or invalid --protocol (boe|itch)" << std::endl;
        return 1;
    }

    std::vector<uint8_t> bytes;
    if (file.empty()) {
        if (is_hex) bytes = read_hex_stream(std::cin);
        else bytes = read_all_bytes(std::cin);
    } else {
        std::ifstream in(file, std::ios::binary);
        if (!in) { std::cerr << "Cannot open: " << file << std::endl; return 1; }
        if (is_hex) bytes = read_hex_stream(in);
        else bytes = std::vector<uint8_t>((std::istreambuf_iterator<char>(in)), {});
    }

    size_t offset = 0;
    using market::runtime::Bytes;
    using market::runtime::status;

    if (protocol == "boe") {
#if __has_include("generated/cboe_boe_v3/handler.hpp")
        struct H {
            void on(const cboe::boe::v3::LoginRequest& m) {
                std::cout << cboe::boe::v3::to_json(m) << std::endl;
            }
            void on(const cboe::boe::v3::NewOrderCross& m) {
                std::cout << cboe::boe::v3::to_json(m) << std::endl;
            }
        } h;
        while (offset < bytes.size()) {
            size_t consumed = 0;
            auto st = cboe::boe::v3::dispatch_boe(Bytes{bytes.data() + offset, bytes.size() - offset}, h, consumed);
            if (st != status::ok || consumed == 0) break;
            offset += consumed;
        }
        return 0;
#else
        std::cerr << "BOE generated handlers not found. Generate code first." << std::endl;
        return 2;
#endif
    } else {
#if __has_include("generated/nasdaq_itch_5/handler.hpp")
        struct H {
            void on(const nasdaq::itch::v5::AddOrder& m) {
                std::cout << nasdaq::itch::v5::to_json(m) << std::endl;
            }
            void on(const nasdaq::itch::v5::DeleteOrder& m) {
                std::cout << nasdaq::itch::v5::to_json(m) << std::endl;
            }
        } h;
        while (offset < bytes.size()) {
            size_t consumed = 0;
            auto st = nasdaq::itch::v5::dispatch_itch(Bytes{bytes.data() + offset, bytes.size() - offset}, h, consumed);
            if (st != status::ok || consumed == 0) break;
            offset += consumed;
        }
        return 0;
#else
        std::cerr << "ITCH generated handlers not found. Generate code first." << std::endl;
        return 2;
#endif
    }
}


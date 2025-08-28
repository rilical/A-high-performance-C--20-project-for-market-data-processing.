// Minimal PCAP reader that decodes BOE/ITCH payloads and emits JSON per message
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>

#include "runtime/bytes.hpp"
#include "runtime/status.hpp"

#if __has_include("../../generated/cboe_boe_v3/handler.hpp")
#include "../../generated/cboe_boe_v3/handler.hpp"
#include "../../generated/cboe_boe_v3/json.hpp"
#endif
#if __has_include("../../generated/nasdaq_itch_5/handler.hpp")
#include "../../generated/nasdaq_itch_5/handler.hpp"
#include "../../generated/nasdaq_itch_5/json.hpp"
#endif

struct PcapGlobal {
    uint32_t magic;
    uint16_t vmajor;
    uint16_t vminor;
    int32_t thiszone;
    uint32_t sigfigs;
    uint32_t snaplen;
    uint32_t network;
};
struct PcapRecHdr {
    uint32_t ts_sec;
    uint32_t ts_usec;
    uint32_t incl_len;
    uint32_t orig_len;
};

static bool read_exact(std::ifstream& in, void* buf, size_t n) {
    in.read(reinterpret_cast<char*>(buf), n);
    return static_cast<size_t>(in.gcount()) == n;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: pcap_decode <boe|itch> <pcap_file>" << std::endl;
        return 1;
    }
    std::string protocol = argv[1];
    std::ifstream in(argv[2], std::ios::binary);
    if (!in) { std::cerr << "Cannot open: " << argv[2] << std::endl; return 1; }

    PcapGlobal gh{};
    if (!read_exact(in, &gh, sizeof(gh))) { std::cerr << "Bad pcap header" << std::endl; return 1; }
    const bool swap = (gh.magic == 0xd4c3b2a1); // little vs big endian magic
    (void)swap; // assume native order for minimal demo

    using market::runtime::Bytes;
    using market::runtime::status;

    if (protocol == "boe") {
#if __has_include("../../generated/cboe_boe_v3/handler.hpp")
        struct H {
            void on(const cboe::boe::v3::LoginRequest& m) { std::cout << cboe::boe::v3::to_json(m) << "\n"; }
            void on(const cboe::boe::v3::NewOrderCross& m) { std::cout << cboe::boe::v3::to_json(m) << "\n"; }
        } h;
        while (in) {
            PcapRecHdr rh{};
            if (!read_exact(in, &rh, sizeof(rh))) break;
            std::vector<uint8_t> pkt(rh.incl_len);
            if (!read_exact(in, pkt.data(), pkt.size())) break;
            size_t off = 0;
            while (off < pkt.size()) {
                size_t consumed = 0;
                auto st = cboe::boe::v3::dispatch_boe(Bytes{pkt.data() + off, pkt.size() - off}, h, consumed);
                if (st != status::ok || consumed == 0) break;
                off += consumed;
            }
        }
        return 0;
#else
        std::cerr << "BOE generated handlers not found. Generate code first." << std::endl; return 2;
#endif
    } else {
#if __has_include("../../generated/nasdaq_itch_5/handler.hpp")
        struct H {
            void on(const nasdaq::itch::v5::AddOrder& m) { std::cout << nasdaq::itch::v5::to_json(m) << "\n"; }
            void on(const nasdaq::itch::v5::DeleteOrder& m) { std::cout << nasdaq::itch::v5::to_json(m) << "\n"; }
        } h;
        while (in) {
            PcapRecHdr rh{};
            if (!read_exact(in, &rh, sizeof(rh))) break;
            std::vector<uint8_t> pkt(rh.incl_len);
            if (!read_exact(in, pkt.data(), pkt.size())) break;
            size_t off = 0;
            while (off < pkt.size()) {
                size_t consumed = 0;
                auto st = nasdaq::itch::v5::dispatch_itch(Bytes{pkt.data() + off, pkt.size() - off}, h, consumed);
                if (st != status::ok || consumed == 0) break;
                off += consumed;
            }
        }
        return 0;
#else
        std::cerr << "ITCH generated handlers not found. Generate code first." << std::endl; return 2;
#endif
    }
}


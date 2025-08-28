#include <cstddef>
#include <cstdint>

#include "../generated/nasdaq_itch_5/messages.hpp"
#include "../generated/nasdaq_itch_5/decoder.hpp"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size == 0) return 0;
    using namespace nasdaq::itch::v5;

    AddOrder add{}; size_t consumed = 0;
    (void)Decoder::decode(data, size, add, consumed);

    if (size > 1) {
        DeleteOrder del{}; consumed = 0;
        (void)Decoder::decode(data + 1, size - 1, del, consumed);
    }

    return 0;
}


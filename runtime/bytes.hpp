#pragma once

#include <cstdint>
#include <span>

namespace market::runtime {

using Bytes = std::span<const uint8_t>;
using MutBytes = std::span<uint8_t>;

}

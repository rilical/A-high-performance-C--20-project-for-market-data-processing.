#pragma once

namespace market::runtime {

enum class status {
    ok = 0,
    short_buffer,
    bad_value,
    unknown_type
};

}  // namespace market::runtime

#pragma once

namespace market::runtime {

enum class status {
    ok = 0,
    short_buffer,
    bad_value,
    unknown_type
};

// Convert status enum to a human-readable string (for logs/debugging)
inline const char* status_to_string(status s) noexcept {
    switch (s) {
        case status::ok:            return "ok";
        case status::short_buffer:  return "short_buffer";
        case status::bad_value:     return "bad_value";
        case status::unknown_type:  return "unknown_type";
        default:                    return "<unknown_status>";
    }
}

}

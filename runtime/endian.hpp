#pragma once

#include <bit>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace market::runtime {

namespace detail {
    template<typename T>
    constexpr bool is_supported_type_v = 
        std::is_same_v<T, uint16_t> || 
        std::is_same_v<T, uint32_t> || 
        std::is_same_v<T, uint64_t>;

    inline constexpr uint16_t byteswap(uint16_t value) noexcept {
        return __builtin_bswap16(value);
    }

    inline constexpr uint32_t byteswap(uint32_t value) noexcept {
        return __builtin_bswap32(value);
    }

    inline constexpr uint64_t byteswap(uint64_t value) noexcept {
        return __builtin_bswap64(value);
    }
}

template<typename T>
inline T load_le(const uint8_t* p) noexcept {
    static_assert(detail::is_supported_type_v<T>, "Unsupported type for endian operations");
    
    T value;
    std::memcpy(&value, p, sizeof(T));
    
    if constexpr (std::endian::native == std::endian::big) {
        return detail::byteswap(value);
    } else {
        return value;
    }
}

template<typename T>
inline void store_le(uint8_t* p, T v) noexcept {
    static_assert(detail::is_supported_type_v<T>, "Unsupported type for endian operations");
    
    if constexpr (std::endian::native == std::endian::big) {
        v = detail::byteswap(v);
    }
    
    std::memcpy(p, &v, sizeof(T));
}

template<typename T>
inline T load_be(const uint8_t* p) noexcept {
    static_assert(detail::is_supported_type_v<T>, "Unsupported type for endian operations");
    
    T value;
    std::memcpy(&value, p, sizeof(T));
    
    if constexpr (std::endian::native == std::endian::little) {
        return detail::byteswap(value);
    } else {
        return value;
    }
}

template<typename T>
inline void store_be(uint8_t* p, T v) noexcept {
    static_assert(detail::is_supported_type_v<T>, "Unsupported type for endian operations");
    
    if constexpr (std::endian::native == std::endian::little) {
        v = detail::byteswap(v);
    }
    
    std::memcpy(p, &v, sizeof(T));
}

}

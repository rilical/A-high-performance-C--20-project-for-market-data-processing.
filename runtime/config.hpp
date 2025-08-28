#pragma once

// *** Runtime Configuration Macros ***

// API export/import macros (defaults empty; future use for visibility/DLL export)
// Prefer MARKET_* macro family. Keep EXCHCG_* as backwards-compatible aliases.
#ifndef MARKET_API
  #if defined(_MSC_VER)
    #define MARKET_API __declspec(dllexport)
  #else
    #define MARKET_API __attribute__((visibility("default")))
  #endif
#endif

#ifndef EXCHCG_API
#define EXCHCG_API MARKET_API
#endif

// Compiler hints for branch prediction
#if defined(__GNUC__) || defined(__clang__)
    #define MARKET_LIKELY(x)   __builtin_expect(!!(x), 1)
    #define MARKET_UNLIKELY(x) __builtin_expect(!!(x), 0)
#elif defined(_MSC_VER)
    #define MARKET_LIKELY(x)   (x)
    #define MARKET_UNLIKELY(x) (x)
#else
    #define MARKET_LIKELY(x)   (x)
    #define MARKET_UNLIKELY(x) (x)
#endif

#ifndef EXCHCG_LIKELY
#define EXCHCG_LIKELY(x) MARKET_LIKELY(x)
#endif
#ifndef EXCHCG_UNLIKELY
#define EXCHCG_UNLIKELY(x) MARKET_UNLIKELY(x)
#endif

// Function inlining attributes (portable best-effort)
#if defined(__GNUC__) || defined(__clang__)
    #define MARKET_NOINLINE      __attribute__((noinline))
    #define MARKET_ALWAYS_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
    #define MARKET_NOINLINE      __declspec(noinline)
    #define MARKET_ALWAYS_INLINE __forceinline
#else
    #define MARKET_NOINLINE
    #define MARKET_ALWAYS_INLINE inline
#endif

#ifndef EXCHCG_NOINLINE
#define EXCHCG_NOINLINE MARKET_NOINLINE
#endif
#ifndef EXCHCG_ALWAYS_INLINE
#define EXCHCG_ALWAYS_INLINE MARKET_ALWAYS_INLINE
#endif

// Exception handling control (defined by CMake option)
#ifndef MARKET_NO_EXCEPTIONS
// MARKET_NO_EXCEPTIONS not defined - exceptions are enabled
#endif

#ifndef EXCHCG_NO_EXCEPTIONS
#define EXCHCG_NO_EXCEPTIONS MARKET_NO_EXCEPTIONS
#endif

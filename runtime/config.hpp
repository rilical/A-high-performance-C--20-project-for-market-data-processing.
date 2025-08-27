#pragma once

// *** Runtime Configuration Macros ***

// API export/import macros (defaults empty; future use for visibility/DLL export)
#ifndef EXCHCG_API
#define EXCHCG_API
#endif

// Compiler hints for branch prediction
#if defined(__GNUC__) || defined(__clang__)
    #define EXCHCG_LIKELY(x)   __builtin_expect(!!(x), 1)
    #define EXCHCG_UNLIKELY(x) __builtin_expect(!!(x), 0)
#elif defined(_MSC_VER)
    #define EXCHCG_LIKELY(x)   (x)
    #define EXCHCG_UNLIKELY(x) (x)
#else
    #define EXCHCG_LIKELY(x)   (x)
    #define EXCHCG_UNLIKELY(x) (x)
#endif

// Function inlining attributes (portable best-effort)
#if defined(__GNUC__) || defined(__clang__)
    #define EXCHCG_NOINLINE      __attribute__((noinline))
    #define EXCHCG_ALWAYS_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
    #define EXCHCG_NOINLINE      __declspec(noinline)
    #define EXCHCG_ALWAYS_INLINE __forceinline
#else
    #define EXCHCG_NOINLINE
    #define EXCHCG_ALWAYS_INLINE inline
#endif

// Exception handling control (defined by CMake option)
#ifndef EXCHCG_NO_EXCEPTIONS
// EXCHCG_NO_EXCEPTIONS not defined - exceptions are enabled
#endif

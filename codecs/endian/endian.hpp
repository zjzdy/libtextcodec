//
// Created by zjzdy on 2017/6/16.
//

#ifndef LIBZDA_ENDIAN_HPP
#define LIBZDA_ENDIAN_HPP

#include <cstdint>
#include <cstring>
#include <cassert>
#include <climits>
#include <algorithm>
#include "boost_endian_intrinsic.hpp"

#if CHAR_BIT != 8
#    error Platforms with CHAR_BIT != 8 are not supported
#endif

/* Machine byte-order, reuse preprocessor provided macros when available */
#if defined(__ORDER_BIG_ENDIAN__)
#  define Z_BIG_ENDIAN __ORDER_BIG_ENDIAN__
#else
#  define Z_BIG_ENDIAN 4321
#endif
#if defined(__ORDER_LITTLE_ENDIAN__)
#  define Z_LITTLE_ENDIAN __ORDER_LITTLE_ENDIAN__
#else
#  define Z_LITTLE_ENDIAN 1234
#endif

#if defined(__arm__) || defined(__TARGET_ARCH_ARM) || defined(_M_ARM) || defined(__aarch64__) || defined(__ARM64__)
#  if defined(__ARMEL__)
#    define Z_BYTE_ORDER Z_LITTLE_ENDIAN
#  elif defined(__ARMEB__)
#    define Z_BYTE_ORDER Z_BIG_ENDIAN
#  endif
//AVR32 is big-endian.
#elif defined(__avr32__)
#  define Z_BYTE_ORDER Z_BIG_ENDIAN
//Blackfin is little-endian.
#elif defined(__bfin__)
#  define Z_BYTE_ORDER Z_LITTLE_ENDIAN
//X86 is little-endian.
#elif defined(__i386) || defined(__i386__) || defined(_M_IX86)
#  define Z_BYTE_ORDER Z_LITTLE_ENDIAN
#elif defined(__x86_64) || defined(__x86_64__) || defined(__amd64) || defined(_M_X64)
#  define Z_BYTE_ORDER Z_LITTLE_ENDIAN
#elif defined(__mips) || defined(__mips__) || defined(_M_MRX000)
#  if defined(__MIPSEL__)
#    define Z_BYTE_ORDER Z_LITTLE_ENDIAN
#  elif defined(__MIPSEB__)
#    define Z_BYTE_ORDER Z_BIG_ENDIAN
#  endif
//S390 is big-endian.
#elif defined(__s390__)
#  define Z_BYTE_ORDER Z_BIG_ENDIAN
/*
    SPARC family, optional revision: V9
    SPARC is big-endian only prior to V9, while V9 is bi-endian with big-endian
    as the default byte order. Assume all SPARC systems are big-endian.
*/
#elif defined(__sparc__)
#  define Z_BYTE_ORDER Z_BIG_ENDIAN
#endif
/*
  NOTE:
  GCC 4.6 added __BYTE_ORDER__, __ORDER_BIG_ENDIAN__, __ORDER_LITTLE_ENDIAN__
  and __ORDER_PDP_ENDIAN__ in SVN r165881. If you are using GCC 4.6 or newer,
  this code will properly detect your target byte order; if you are not, and
  the __LITTLE_ENDIAN__ or __BIG_ENDIAN__ macros are not defined, then this
  code will fail to detect the target byte order.
*/
// Some processors support either endian format, try to detect which we are using.
#if !defined(Z_BYTE_ORDER)
#  if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == Z_BIG_ENDIAN || __BYTE_ORDER__ == Z_LITTLE_ENDIAN)
// Reuse __BYTE_ORDER__ as-is, since our Z_*_ENDIAN #defines match the preprocessor defaults
#    define Z_BYTE_ORDER __BYTE_ORDER__
#  elif defined(__BIG_ENDIAN__) || defined(_big_endian__) || defined(_BIG_ENDIAN)
#    define Z_BYTE_ORDER Z_BIG_ENDIAN
#  elif defined(__LITTLE_ENDIAN__) || defined(_little_endian__) || defined(_LITTLE_ENDIAN) \
        || defined(WINAPI_FAMILY) // WinRT is always little-endian according to MSDN.
#    define Z_BYTE_ORDER Z_LITTLE_ENDIAN
#  endif
#endif
namespace zdytool {
class endian {
public:
    static bool is_big_endian()
    {
        float x = 1.0f;
        unsigned char first_byte;
        memcpy(&first_byte, &x, 1);
        return first_byte != 0;
        //const int i = 1;
        //return *reinterpret_cast<const int8_t*>(&i) == 0;
    }

    static bool is_little_endian()
    {
        float x = 1.0f;
        unsigned char first_byte;
        memcpy(&first_byte, &x, 1);
        return first_byte == 0;
        //const int i = 1;
        //return *reinterpret_cast<const int8_t*>(&i) == 1;
    }

    // Used to implement a type-safe and alignment-safe copy operation
    // If you want to avoid the memcpy, you must write specializations for these functions
    template <typename T>
    static inline void toUnaligned(const T src, void *dest)
    {
        // Using sizeof(T) inside memcpy function produces internal compiler error with
        // MSVC2008/ARM in tst_endian -> use extra indirection to resolve size of T.
        const size_t size = sizeof(T);
        memcpy(dest, &src, size);
    }

    template <typename T>
    static inline T fromUnaligned(const void *src)
    {
        T dest;
        const size_t size = sizeof(T);
        memcpy(&dest, src, size);
        return dest;
    }

    template<class T>
    static inline T std_endian_reverse(T x) {
        T tmp(x);
        std::reverse(
                reinterpret_cast<unsigned char *>(&tmp),
                reinterpret_cast<unsigned char *>(&tmp) + sizeof(T));
        return tmp;
    }

    static inline int8_t endian_reverse(int8_t x) {
        return x;
    }

    static inline int16_t endian_reverse(int16_t x) {
    # ifdef BOOST_ENDIAN_NO_INTRINSICS
        return (static_cast<uint16_t>(x) << 8)
              | (static_cast<uint16_t>(x) >> 8);
    # else
        return BOOST_ENDIAN_INTRINSIC_BYTE_SWAP_2(static_cast<uint16_t>(x));
    # endif
    }

    static inline int32_t endian_reverse(int32_t x) {
    # ifdef BOOST_ENDIAN_NO_INTRINSICS
        uint32_t step16;
            step16 = static_cast<uint32_t>(x) << 16 | static_cast<uint32_t>(x) >> 16;
            return
                ((static_cast<uint32_t>(step16) << 8) & 0xff00ff00)
              | ((static_cast<uint32_t>(step16) >> 8) & 0x00ff00ff);
    # else
        return BOOST_ENDIAN_INTRINSIC_BYTE_SWAP_4(static_cast<uint32_t>(x));
    # endif
    }

    static inline int64_t endian_reverse(int64_t x) {
    # ifdef BOOST_ENDIAN_NO_INTRINSICS
        uint64_t step32, step16;
            step32 = static_cast<uint64_t>(x) << 32 | static_cast<uint64_t>(x) >> 32;
            step16 = (step32 & 0x0000FFFF0000FFFFULL) << 16
                   | (step32 & 0xFFFF0000FFFF0000ULL) >> 16;
            return static_cast<int64_t>((step16 & 0x00FF00FF00FF00FFULL) << 8
                   | (step16 & 0xFF00FF00FF00FF00ULL) >> 8);
    # else
        return BOOST_ENDIAN_INTRINSIC_BYTE_SWAP_8(static_cast<uint64_t>(x));
    # endif
    }

    static inline uint8_t endian_reverse(uint8_t x) {
        return x;
    }

    static inline uint16_t endian_reverse(uint16_t x) {
    # ifdef BOOST_ENDIAN_NO_INTRINSICS
        return (x << 8)
              | (x >> 8);
    # else
        return BOOST_ENDIAN_INTRINSIC_BYTE_SWAP_2(x);
    # endif
    }

    static inline uint32_t endian_reverse(uint32_t x) {
    # ifdef BOOST_ENDIAN_NO_INTRINSICS
        uint32_t step16;
            step16 = x << 16 | x >> 16;
            return
                ((step16 << 8) & 0xff00ff00)
              | ((step16 >> 8) & 0x00ff00ff);
    # else
        return BOOST_ENDIAN_INTRINSIC_BYTE_SWAP_4(x);
    # endif
    }

    static inline uint64_t endian_reverse(uint64_t x) {
    # ifdef BOOST_ENDIAN_NO_INTRINSICS
        uint64_t step32, step16;
            step32 = x << 32 | x >> 32;
            step16 = (step32 & 0x0000FFFF0000FFFFULL) << 16
                   | (step32 & 0xFFFF0000FFFF0000ULL) >> 16;
            return (step16 & 0x00FF00FF00FF00FFULL) << 8
                   | (step16 & 0xFF00FF00FF00FF00ULL) >> 8;
    # else
        return BOOST_ENDIAN_INTRINSIC_BYTE_SWAP_8(x);
    # endif
    }
};
}


#endif //LIBZDA_ENDIAN_HPP

/****************************************************************************
**
** Copyright zjzdy
**
** This source code is licensed under under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
****************************************************************************/

#ifndef TEXTCODE_P_H
#define TEXTCODE_P_H
#include <string>
#include <list>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <algorithm>
#include <mutex>
#include <cstring>
#include <sstream>
#include <iostream>
#include <vector>

#if defined(__APPLE__) || defined(__ANDROID__) || defined(ANDROID) || defined(__QNXNTO__)
#define Z_LOCALE_IS_UTF8
#endif
namespace zdytool {
    using std::list;
    using std::basic_string;
    typedef basic_string<uint16_t> u16string;
    typedef basic_string<char> string;
    typedef uint16_t ushort;
    typedef unsigned char uchar;
    typedef unsigned int uint;
    template<int>
    struct IntegerForSize;
    template<>
    struct IntegerForSize<1> {
        typedef uint8_t Unsigned;
        typedef int8_t Signed;
    };
    template<>
    struct IntegerForSize<2> {
        typedef uint16_t Unsigned;
        typedef int16_t Signed;
    };
    template<>
    struct IntegerForSize<4> {
        typedef uint32_t Unsigned;
        typedef int32_t Signed;
    };
    template<>
    struct IntegerForSize<8> {
        typedef uint64_t Unsigned;
        typedef int64_t Signed;
    };
#if defined(__GNUC__) && defined(__SIZEOF_INT128__)
    template <>    struct QIntegerForSize<16> { __extension__ typedef unsigned __int128 Unsigned; __extension__ typedef __int128 Signed; };
#endif
    template<class T>
    struct IntegerForSizeof : IntegerForSize<sizeof(T)> {
    };
    typedef IntegerForSizeof<void *>::Unsigned uintptr;
    typedef IntegerForSizeof<void *>::Signed ptrdiff;
    typedef ptrdiff intptr;
    enum SpecialCharacter {
        Null = 0x0000,
        Tabulation = 0x0009,
        LineFeed = 0x000a,
        CarriageReturn = 0x000d,
        Space = 0x0020,
        Nbsp = 0x00a0,
        SoftHyphen = 0x00ad,
        ReplacementCharacter = 0xfffd,
        ObjectReplacementCharacter = 0xfffc,
        ByteOrderMark = 0xfeff,
        ByteOrderSwapped = 0xfffe,
        ParagraphSeparator = 0x2029,
        LineSeparator = 0x2028,
        LastValidCodePoint = 0x10ffff
    };
    typedef std::map<string, TextCodec *> TextCodecCache;

    void from_latin1(ushort *dst, const char *str, size_t size);

    void to_latin1(uchar *dst, const ushort *src, int length);

    u16string u16string_fromLatin1(const char *str, int size);

    string u16string_toLatin1(const ushort *src, int length);

    extern list<TextCodec *> allCodecs;
    extern TextCodec *codecForLocale_m;
    extern TextCodecCache codecCache;

    class UCS2Tool {
    public:
        static inline uchar cell(uint16_t ucs) { return uchar(ucs & 0xff); }

        static inline uchar row(uint16_t ucs) { return uchar((ucs >> 8) & 0xff); }

        static inline void setCell(uint16_t &ucs, uchar acell) { ucs = uint16_t((ucs & 0xff00) + acell); }

        static inline void setRow(uint16_t &ucs, uchar arow) { ucs = uint16_t((uint16_t(arow) << 8) + (ucs & 0xff)); }

        static inline uint16_t toUshort(uchar c, uchar r) { return uint16_t((r << 8) | c); }

        static inline uint16_t toUshort(unsigned int rc) { return uint16_t(rc & 0xffff); }

        static inline uint16_t toUshort(int rc) { return uint16_t(rc & 0xffff); }
    };

    class UCS4Tool {
    public:
        static inline bool isNonCharacter(unsigned int ucs4) {
            return ucs4 >= 0xfdd0 && (ucs4 <= 0xfdef || (ucs4 & 0xfffe) == 0xfffe);
        }

        static inline bool isHighSurrogate(unsigned int ucs4) {
            return ((ucs4 & 0xfffffc00) == 0xd800);
        }

        static inline bool isLowSurrogate(unsigned int ucs4) {
            return ((ucs4 & 0xfffffc00) == 0xdc00);
        }

        static inline bool isSurrogate(unsigned int ucs4) {
            return (ucs4 - 0xd800u < 2048u);
        }

        static inline bool requiresSurrogates(unsigned int ucs4) {
            return (ucs4 >= 0x10000);
        }

        static inline unsigned int surrogateToUcs4(uint16_t high, uint16_t low) {
            return ((unsigned int) (high) << 10) + low - 0x35fdc00;
        }

        static inline uint16_t highSurrogate(unsigned int ucs4) {
            return uint16_t((ucs4 >> 10) + 0xd7c0);
        }

        static inline uint16_t lowSurrogate(unsigned int ucs4) {
            return uint16_t(ucs4 % 0x400 + 0xdc00);
        }
    };

    typedef void (*TextCodecStateFreeFunction)(TextCodec::ConverterState *);

    struct TextCodecUnalignedPointer {
        static inline TextCodecStateFreeFunction decode(const uint *src) {
            uintptr data;
            memcpy(&data, src, sizeof(data));
            return reinterpret_cast<TextCodecStateFreeFunction>(data);
        }

        static inline void encode(uint *dst, TextCodecStateFreeFunction fn) {
            uintptr data = reinterpret_cast<uintptr>(fn);
            memcpy(dst, &data, sizeof(data));
        }
    };
}
#endif // TEXTCODE_P_H

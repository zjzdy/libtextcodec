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

// Most of the code here was originally written by Qt term,
// and the grateful thanks of the Qt team.

#ifndef TEXTCODEC_H
#define TEXTCODEC_H

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
#if defined(__GNUC__)
#  define Z_LIKELY(expr)    __builtin_expect(!!(expr), true)
#  define Z_UNLIKELY(expr)  __builtin_expect(!!(expr), false)
#else
#  define Z_LIKELY(x) (x)
#  define Z_UNLIKELY(x) (x)
#endif
namespace zdytool {
    class TextDecoder;
    class TextEncoder;
    class TextCodec {
    public:
        static TextCodec *codecForName(const std::basic_string<char> &name);

        static TextCodec *codecForName(const char *name) { return codecForName(std::basic_string<char>(name)); }

        static TextCodec *codecForMib(int mib);

        static std::list<std::basic_string<char>> availableCodecs();

        static std::list<int> availableMibs();

        static TextCodec *codecForLocale();

        static void setCodecForLocale(TextCodec *c);

        static TextCodec *codecForHtml(const std::basic_string<char> &ba);

        static TextCodec *codecForHtml(const std::basic_string<char> &ba, TextCodec *defaultCodec);

        static TextCodec *codecForUtfText(const std::basic_string<char> &ba);

        static TextCodec *codecForUtfText(const std::basic_string<char> &ba, TextCodec *defaultCodec);

        bool canEncode(uint16_t) const;

        bool canEncode(const std::basic_string<uint16_t> &) const;

        std::basic_string<uint16_t> toUnicode(const std::basic_string<char> &) const;

        std::basic_string<uint16_t> toUnicode(const char *chars) const;

        std::basic_string<char> fromUnicode(const std::basic_string<uint16_t> &uc) const;

        enum ConversionFlags {
            DefaultConversion,
            ConvertInvalidToNull = 0x80000000,
            IgnoreHeader = 0x1,
            FreeFunction = 0x2
        };

        struct ConverterState {
            ConverterState(ConversionFlags f = DefaultConversion)
                    : flags(f), remainingChars(0), invalidChars(0),
                      d(nullptr) { state_data[0] = state_data[1] = state_data[2] = 0; }

            ~ConverterState();

            ConversionFlags flags;
            int remainingChars;
            int invalidChars;
            unsigned int state_data[3];
            void *d;
        private:
            ConverterState(const ConverterState &) = delete;

            ConverterState &operator=(const ConverterState &) = delete;
        };

        std::basic_string<uint16_t> toUnicode(const char *in, int length, ConverterState *state = nullptr) const {
            return convertToUnicode(in, length, state);
        }

        std::basic_string<char> fromUnicode(const uint16_t *in, int length, ConverterState *state = nullptr) const {
            return convertFromUnicode(in, length, state);
        }

        TextDecoder *makeDecoder(ConversionFlags flags = DefaultConversion) const;

        TextEncoder *makeEncoder(ConversionFlags flags = DefaultConversion) const;

        virtual std::basic_string<char> name() const = 0;

        virtual std::list<std::basic_string<char>> aliases() const;

        virtual int mibEnum() const = 0;

    protected:
        virtual std::basic_string<uint16_t>
        convertToUnicode(const char *in, int length, ConverterState *state) const = 0;

        virtual std::basic_string<char>
        convertFromUnicode(const uint16_t *in, int length, ConverterState *state) const = 0;

        static bool TextCodecNameMatch(const char *a, const char *b);

        TextCodec();

        virtual ~TextCodec();

    private:
        TextCodec(const TextCodec &) = delete;

        TextCodec &operator=(const TextCodec &) = delete;
    };

    class TextEncoder {
    public:
        explicit TextEncoder(const TextCodec *codec) : c(codec), state() {}

        TextEncoder(const TextCodec *codec, TextCodec::ConversionFlags flags);

        ~TextEncoder();

        std::basic_string<char> fromUnicode(const std::basic_string<uint16_t> &str);

        std::basic_string<char> fromUnicode(const uint16_t *uc, int len);

        bool hasFailure() const;

    private:
        const TextCodec *c;
        TextCodec::ConverterState state;

        TextEncoder(const TextEncoder &) = delete;

        TextEncoder &operator=(const TextEncoder &) = delete;
    };

    class TextDecoder {
    public:
        explicit TextDecoder(const TextCodec *codec) : c(codec), state() {}

        TextDecoder(const TextCodec *codec, TextCodec::ConversionFlags flags);

        ~TextDecoder();

        std::basic_string<uint16_t> toUnicode(const char *chars, int len);

        std::basic_string<uint16_t> toUnicode(const std::basic_string<char> &ba);

        void toUnicode(std::basic_string<uint16_t> *target, const char *chars, int len);

        bool hasFailure() const;

    private:
        const TextCodec *c;
        TextCodec::ConverterState state;

        TextDecoder(const TextDecoder &) = delete;

        TextDecoder &operator=(const TextDecoder &) = delete;
    };
}
#endif // TEXTCODEC_H

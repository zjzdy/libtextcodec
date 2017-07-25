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

// Most of the code here was originally written by Serika Kurusugawa
// a.k.a. Junji Takagi, and is included in Qt with the author's permission,
// and the grateful thanks of the Qt team.

#include "sjiscodec_p.h"

#ifndef Z_NO_BIG_TEXTCODECS
enum {
    Esc = 0x1b
};

#define        IsKana(c)        (((c) >= 0xa1) && ((c) <= 0xdf))
#define        IsSjisChar1(c)        ((((c) >= 0x81) && ((c) <= 0x9f)) ||        \
                         (((c) >= 0xe0) && ((c) <= 0xfc)))
#define        IsSjisChar2(c)        (((c) >= 0x40) && ((c) != 0x7f) && ((c) <= 0xfc))
#define        IsUserDefinedChar1(c)        (((c) >= 0xf0) && ((c) <= 0xfc))

#define        ZValidChar(u)        ((u) ? (ushort)(u) : (ushort)SpecialCharacter::ReplacementCharacter)

SjisCodec::SjisCodec() : conv(JpUnicodeConv::newConverter(JpUnicodeConv::Default))
{
}

SjisCodec::~SjisCodec()
{
    delete (const JpUnicodeConv*)conv;
    conv = 0;
}


string SjisCodec::convertFromUnicode(const ushort *uc, int len, ConverterState *state) const
{
    char replacement = '?';
    if (state) {
        if (state->flags & ConvertInvalidToNull)
            replacement = 0;
    }
    int invalid = 0;

    int rlen = 2*len + 1;
    std::vector<char> rstr(rlen+1);
    rstr[rlen] = '\0';
    uchar* cursor = (uchar*)rstr.data();
    for (int i = 0; i < len; i++) {
        ushort ch = uc[i];
        uint j;
        if (UCS2Tool::row(ch) == 0x00 && UCS2Tool::cell(ch) < 0x80) {
            // ASCII
            *cursor++ = UCS2Tool::cell(ch);
        } else if ((j = conv->unicodeToJisx0201(UCS2Tool::row(ch), UCS2Tool::cell(ch))) != 0) {
            // JIS X 0201 Latin or JIS X 0201 Kana
            *cursor++ = j;
        } else if ((j = conv->unicodeToSjis(UCS2Tool::row(ch), UCS2Tool::cell(ch))) != 0) {
            // JIS X 0208
            *cursor++ = (j >> 8);
            *cursor++ = (j & 0xff);
        } else if ((j = conv->unicodeToSjisibmvdc(UCS2Tool::row(ch), UCS2Tool::cell(ch))) != 0) {
            // JIS X 0208 IBM VDC
            *cursor++ = (j >> 8);
            *cursor++ = (j & 0xff);
        } else if ((j = conv->unicodeToCp932(UCS2Tool::row(ch), UCS2Tool::cell(ch))) != 0) {
            // CP932 (for lead bytes 87, ee & ed)
            *cursor++ = (j >> 8);
            *cursor++ = (j & 0xff);
        } else if ((j = conv->unicodeToJisx0212(UCS2Tool::row(ch), UCS2Tool::cell(ch))) != 0) {
            // JIS X 0212 (can't be encoded in ShiftJIS !)
            *cursor++ = 0x81;        // white square
            *cursor++ = 0xa0;        // white square
        } else {
            // Error
            *cursor++ = replacement;
            ++invalid;
        }
    }
    string rstr_str(rstr.data(),cursor - (const uchar*)rstr.data());

    if (state) {
        state->invalidChars += invalid;
    }
    return rstr_str;
}

u16string SjisCodec::convertToUnicode(const char* chars, int len, ConverterState *state) const
{
    uchar buf[1] = {0};
    int nbuf = 0;
    ushort replacement = SpecialCharacter::ReplacementCharacter;
    if (state) {
        if (state->flags & ConvertInvalidToNull)
            replacement = SpecialCharacter::Null;
        nbuf = state->remainingChars;
        buf[0] = state->state_data[0];
    }
    int invalid = 0;
    uint u= 0;
    u16string result;
    for (int i=0; i<len; i++) {
        uchar ch = chars[i];
        switch (nbuf) {
        case 0:
            if (ch < 0x80) {
                result += ZValidChar(ch);
            } else if (IsKana(ch)) {
                // JIS X 0201 Latin or JIS X 0201 Kana
                u = conv->jisx0201ToUnicode(ch);
                result += ZValidChar(u);
            } else if (IsSjisChar1(ch)) {
                // JIS X 0208
                buf[0] = ch;
                nbuf = 1;
            } else {
                // Invalid
                result += replacement;
                ++invalid;
            }
            break;
        case 1:
            // JIS X 0208
            if (IsSjisChar2(ch)) {
                if ((u = conv->sjisibmvdcToUnicode(buf[0], ch))) {
                    result += ZValidChar(u);
                } else if ((u = conv->cp932ToUnicode(buf[0], ch))) {
                    result += ZValidChar(u);
                }
                else if (IsUserDefinedChar1(buf[0])) {
                    result += SpecialCharacter::ReplacementCharacter;
                } else {
                    u = conv->sjisToUnicode(buf[0], ch);
                    result += ZValidChar(u);
                }
            } else {
                // Invalid
                result += replacement;
                ++invalid;
            }
            nbuf = 0;
            break;
        }
    }

    if (state) {
        state->remainingChars = nbuf;
        state->state_data[0] = buf[0];
        state->invalidChars += invalid;
    }
    return result;
}


int SjisCodec::_mibEnum()
{
    return 17;
}

string SjisCodec::_name()
{
    return "Shift_JIS";
}

/*!
    Returns the codec's mime name.
*/
list<string> SjisCodec::_aliases()
{
    list<string> list;
    list.push_back("SJIS");
    list.push_back("MS_Kanji");
    return list;
}
#endif // Z_NO_BIG_TEXTCODECS

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

/*
 * Copyright (C) 1999 Serika Kurusugawa, All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "eucjpcodec_p.h"

#ifndef Z_NO_BIG_TEXTCODECS

static const uchar Ss2 = 0x8e;        // Single Shift 2
static const uchar Ss3 = 0x8f;        // Single Shift 3

#define        IsKana(c)        (((c) >= 0xa1) && ((c) <= 0xdf))
#define        IsEucChar(c)        (((c) >= 0xa1) && ((c) <= 0xfe))

#define        ZValidChar(u)        ((u) ? (ushort)(u) : (ushort)SpecialCharacter::ReplacementCharacter)

EucJpCodec::EucJpCodec() : conv(JpUnicodeConv::newConverter(JpUnicodeConv::Default))
{
}

EucJpCodec::~EucJpCodec()
{
    delete (const JpUnicodeConv*)conv;
    conv = 0;
}

string EucJpCodec::convertFromUnicode(const ushort *uc, int len, ConverterState *state) const
{
    char replacement = '?';
    if (state) {
        if (state->flags & ConvertInvalidToNull)
            replacement = 0;
    }
    int invalid = 0;

    int rlen = 3*len + 1;
    std::vector<char> rstr(rlen+1);
    rstr[len] = '\0';
    uchar* cursor = (uchar*)rstr.data();
    for (int i = 0; i < len; i++) {
        ushort ch = uc[i];
        uint j;
        if (ch < 0x80) {
            // ASCII
            *cursor++ = UCS2Tool::cell(ch);
        } else if ((j = conv->unicodeToJisx0201(UCS2Tool::row(ch), UCS2Tool::cell(ch))) != 0) {
            if (j < 0x80) {
                // JIS X 0201 Latin ?
                *cursor++ = j;
            } else {
                // JIS X 0201 Kana
                *cursor++ = Ss2;
                *cursor++ = j;
            }
        } else if ((j = conv->unicodeToJisx0208(UCS2Tool::row(ch), UCS2Tool::cell(ch))) != 0) {
            // JIS X 0208
            *cursor++ = (j >> 8)   | 0x80;
            *cursor++ = (j & 0xff) | 0x80;
        } else if ((j = conv->unicodeToJisx0212(UCS2Tool::row(ch), UCS2Tool::cell(ch))) != 0) {
            // JIS X 0212
            *cursor++ = Ss3;
            *cursor++ = (j >> 8)   | 0x80;
            *cursor++ = (j & 0xff) | 0x80;
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


u16string EucJpCodec::convertToUnicode(const char* chars, int len, ConverterState *state) const
{
    uchar buf[2] = {0, 0};
    int nbuf = 0;
    ushort replacement = SpecialCharacter::ReplacementCharacter;
    if (state) {
        if (state->flags & ConvertInvalidToNull)
            replacement = SpecialCharacter::Null;
        nbuf = state->remainingChars;
        buf[0] = state->state_data[0];
        buf[1] = state->state_data[1];
    }
    int invalid = 0;

    u16string result;
    for (int i=0; i<len; i++) {
        uchar ch = chars[i];
        switch (nbuf) {
        case 0:
            if (ch < 0x80) {
                // ASCII
                result += char(ch);
            } else if (ch == Ss2 || ch == Ss3) {
                // JIS X 0201 Kana or JIS X 0212
                buf[0] = ch;
                nbuf = 1;
            } else if (IsEucChar(ch)) {
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
            if (buf[0] == Ss2) {
                // JIS X 0201 Kana
                if (IsKana(ch)) {
                    uint u = conv->jisx0201ToUnicode(ch);
                    result += ZValidChar(u);
                } else {
                    result += replacement;
                    ++invalid;
                }
                nbuf = 0;
            } else if (buf[0] == Ss3) {
                // JIS X 0212-1990
                if (IsEucChar(ch)) {
                    buf[1] = ch;
                    nbuf = 2;
                } else {
                    // Error
                    result += replacement;
                    ++invalid;
                    nbuf = 0;
                }
            } else {
                // JIS X 0208-1990
                if (IsEucChar(ch)) {
                    uint u = conv->jisx0208ToUnicode(buf[0] & 0x7f, ch & 0x7f);
                    result += ZValidChar(u);
                } else {
                    // Error
                    result += replacement;
                    ++invalid;
                }
                nbuf = 0;
            }
            break;
        case 2:
            // JIS X 0212
            if (IsEucChar(ch)) {
                uint u = conv->jisx0212ToUnicode(buf[1] & 0x7f, ch & 0x7f);
                result += ZValidChar(u);
            } else {
                result += replacement;
                ++invalid;
            }
            nbuf = 0;
        }
    }
    if (state) {
        state->remainingChars = nbuf;
        state->state_data[0] = buf[0];
        state->state_data[1] = buf[1];
        state->invalidChars += invalid;
    }
    return result;
}

int EucJpCodec::_mibEnum()
{
    return 18;
}

string EucJpCodec::_name()
{
    return "EUC-JP";
}
#endif // Z_NO_BIG_TEXTCODECS

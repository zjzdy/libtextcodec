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

// Contributed by James Su <suzhe@gnuchina.org>

#ifndef GB18030CODEC_P_H
#define GB18030CODEC_P_H

#include <string>
#include <list>
#include <cstdint>
#include "textcodec.h"
#include "textcodec_p.h"

#ifndef Z_NO_BIG_TEXTCODECS

class Gb18030Codec : public TextCodec {
public:
    Gb18030Codec();

    static string _name() { return "GB18030"; }
    static list<string> _aliases() { return list<string>(); }
    static int _mibEnum() { return 114; }

    string name() const { return _name(); }
    list<string> aliases() const { return _aliases(); }
    int mibEnum() const { return _mibEnum(); }

    u16string convertToUnicode(const char *, int, ConverterState *) const;
    string convertFromUnicode(const uint16_t *, int, ConverterState *) const;
};

class GbkCodec : public Gb18030Codec {
public:
    GbkCodec();

    static string _name();
    static list<string> _aliases();
    static int _mibEnum();

    string name() const { return _name(); }
    list<string> aliases() const { return _aliases(); }
    int mibEnum() const { return _mibEnum(); }

    u16string convertToUnicode(const char *, int, ConverterState *) const;
    string convertFromUnicode(const ushort *, int, ConverterState *) const;
};

class Gb2312Codec : public Gb18030Codec {
public:
    Gb2312Codec();

    static string _name();
    static list<string> _aliases() { return list<string>(); }
    static int _mibEnum();

    string name() const { return _name(); }
    int mibEnum() const { return _mibEnum(); }

    u16string convertToUnicode(const char *, int, ConverterState *) const;
    string convertFromUnicode(const ushort *, int, ConverterState *) const;
};

#endif // Z_NO_BIG_TEXTCODECS

#endif // GB18030CODEC_P_H

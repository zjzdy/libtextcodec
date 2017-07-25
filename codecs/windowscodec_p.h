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

#ifndef WINDOWSCODEC_P_H
#define WINDOWSCODEC_P_H

#include <string>
#include <list>
#include <cstdint>
#include "textcodec.h"
#include "textcodec_p.h"

class WindowsLocalCodec: public TextCodec
{
public:
    WindowsLocalCodec();
    ~WindowsLocalCodec();

    u16string convertToUnicode(const char *, int, ConverterState *) const;
    string convertFromUnicode(const ushort *, int, ConverterState *) const;
    u16string convertToUnicodeCharByChar(const char *chars, int length, ConverterState *state) const;

    string name() const;
    int mibEnum() const;

};

#endif

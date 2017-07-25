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

#ifndef SIMPLECODEC_P_H
#define SIMPLECODEC_P_H

#include <string>
#include <list>
#include <cstdint>
#include "textcodec.h"
#include "textcodec_p.h"

class SimpleTextCodec: public TextCodec
{
public:
    enum { numSimpleCodecs = 30 };
    explicit SimpleTextCodec(int);
    ~SimpleTextCodec();

    u16string convertToUnicode(const char *, int, ConverterState *) const override;
    string convertFromUnicode(const ushort *, int, ConverterState *) const override;

    string name() const override;
    list<string> aliases() const override;
    int mibEnum() const override;

private:
    int forwardIndex;
    mutable string *reverseMap;
};

#endif // SIMPLECODEC_P_H

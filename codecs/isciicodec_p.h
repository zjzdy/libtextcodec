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

#ifndef ISCIICODEC_P_H
#define ISCIICODEC_P_H

#include <string>
#include <list>
#include <cstdint>
#include "textcodec.h"
#include "textcodec_p.h"

class IsciiCodec : public TextCodec {
public:
    explicit IsciiCodec(int i) : idx(i) {}
    ~IsciiCodec();

    static TextCodec *create(const char *name);

    string name() const;
    int mibEnum() const;

    u16string convertToUnicode(const char *, int, ConverterState *) const;
    string convertFromUnicode(const ushort *, int, ConverterState *) const;

private:
    int idx;
};

#endif // ISCIICODEC_P_H

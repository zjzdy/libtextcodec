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

#ifndef LATINCODEC_P_H
#define LATINCODEC_P_H

#include <string>
#include <list>
#include <cstdint>
#include "textcodec.h"
#include "textcodec_p.h"
namespace zdytool {
	class Latin1Codec : public TextCodec
	{
	public:
		~Latin1Codec();

		u16string convertToUnicode(const char *, int, ConverterState *) const;
		string convertFromUnicode(const ushort *, int, ConverterState *) const;

		string name() const;
		list<string> aliases() const;
		int mibEnum() const;
	};



	class Latin15Codec: public TextCodec
	{
	public:
		~Latin15Codec();

		u16string convertToUnicode(const char *, int, ConverterState *) const;
		string convertFromUnicode(const ushort *, int, ConverterState *) const;

		string name() const;
		list<string> aliases() const;
		int mibEnum() const;
	};
}
#endif // LATINCODEC_P_H

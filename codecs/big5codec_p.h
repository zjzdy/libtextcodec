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

// Most of the code here was originally written by Ming-Che Chuang and
// is included in Qt with the author's permission, and the grateful
// thanks of the Qt team.

#ifndef BIG5CODEC_P_H
#define BIG5CODEC_P_H

#include <string>
#include <list>
#include <cstdint>
#include "textcodec.h"
#include "textcodec_p.h"
namespace zdytool {
#ifndef Z_NO_BIG_TEXTCODECS

	class Big5Codec : public TextCodec {
	public:
		static string _name();
		static list<string> _aliases();
		static int _mibEnum();

		string name() const { return _name(); }
		list<string> aliases() const { return _aliases(); }
		int mibEnum() const { return _mibEnum(); }

		u16string convertToUnicode(const char *, int, ConverterState *) const;
		string convertFromUnicode(const ushort *, int, ConverterState *) const;
	};

	class Big5hkscsCodec : public TextCodec {
	public:
		static string _name();
		static list<string> _aliases() { return list<string>(); }
		static int _mibEnum();

		string name() const { return _name(); }
		list<string> aliases() const { return _aliases(); }
		int mibEnum() const { return _mibEnum(); }

		u16string convertToUnicode(const char *, int, ConverterState *) const;
		string convertFromUnicode(const ushort *, int, ConverterState *) const;
	};

#endif // Z_NO_BIG_TEXTCODECS
}
#endif // BIG5CODEC_P_H

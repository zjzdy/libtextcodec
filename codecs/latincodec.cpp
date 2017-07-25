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

#include "latincodec_p.h"
namespace zdytool {
	Latin1Codec::~Latin1Codec()
	{
	}

	u16string Latin1Codec::convertToUnicode(const char *chars, int len, ConverterState *) const
	{
		if (chars == 0)
			return u16string();

		return u16string_fromLatin1(chars, len);
	}


	string Latin1Codec::convertFromUnicode(const ushort *ch, int len, ConverterState *state) const
	{
		const char replacement = (state && state->flags & ConvertInvalidToNull) ? 0 : '?';
		std::vector<char> r(len+1);
		r[len] ='\0';
		char *d = r.data();
		int invalid = 0;
		for (int i = 0; i < len; ++i) {
			if (ch[i] > 0xff) {
				d[i] = replacement;
				++invalid;
			} else {
				d[i] = (char)UCS2Tool::cell(ch[i]);
			}
		}
		if (state) {
			state->invalidChars += invalid;
		}
		string r_str(r.data(),len);
		return r_str;
	}

	string Latin1Codec::name() const
	{
		return "ISO-8859-1";
	}

	list<string> Latin1Codec::aliases() const
	{
		list<string> list;
		list.push_back("latin1");
		list.push_back("CP819");
		list.push_back("IBM819");
		list.push_back("iso-ir-100");
		list.push_back("csISOLatin1");
		/*
		list << "latin1"
			 << "CP819"
			 << "IBM819"
			 << "iso-ir-100"
			 << "csISOLatin1";
		*/
		return list;
	}


	int Latin1Codec::mibEnum() const
	{
		return 4;
	}


	Latin15Codec::~Latin15Codec()
	{
	}

	u16string Latin15Codec::convertToUnicode(const char* chars, int len, ConverterState *) const
	{
		if (chars == 0)
			return u16string();

		std::basic_string<uint16_t> str = u16string_fromLatin1(chars, len);
		std::vector<uint16_t> str_c(len+1);
		int str_len = len;
		memcpy(str_c.data(),str.data(),len*sizeof(uint16_t));
		str_c[len] = '\0';
		ushort *uc = str_c.data();
		while(len--) {
			switch(*uc) {
				case 0xa4:
					*uc = 0x20ac;
					break;
				case 0xa6:
					*uc = 0x0160;
					break;
				case 0xa8:
					*uc = 0x0161;
					break;
				case 0xb4:
					*uc = 0x017d;
					break;
				case 0xb8:
					*uc = 0x017e;
					break;
				case 0xbc:
					*uc = 0x0152;
					break;
				case 0xbd:
					*uc = 0x0153;
					break;
				case 0xbe:
					*uc = 0x0178;
					break;
				default:
					break;
			}
			uc++;
		}
		u16string str_str(str_c.data(),str_len);
		return str_str;
	}

	string Latin15Codec::convertFromUnicode(const ushort *in, int length, ConverterState *state) const
	{
		const char replacement = (state && state->flags & ConvertInvalidToNull) ? 0 : '?';
		std::vector<char> r(length+1);
		r[length] = '\0';
		char *d = r.data();
		int invalid = 0;
		for (int i = 0; i < length; ++i) {
			uchar c;
			ushort uc = in[i];
			if (uc < 0x0100) {
				if (uc > 0xa3) {
					switch(uc) {
					case 0xa4:
					case 0xa6:
					case 0xa8:
					case 0xb4:
					case 0xb8:
					case 0xbc:
					case 0xbd:
					case 0xbe:
						c = replacement;
						++invalid;
						break;
					default:
						c = (uchar) uc;
						break;
					}
				} else {
					c = (uchar) uc;
				}
			} else {
				if (uc == 0x20ac)
					c = 0xa4;
				else if ((uc & 0xff00) == 0x0100) {
					switch(uc) {
					case 0x0160:
						c = 0xa6;
						break;
					case 0x0161:
						c = 0xa8;
						break;
					case 0x017d:
						c = 0xb4;
						break;
					case 0x017e:
						c = 0xb8;
						break;
					case 0x0152:
						c = 0xbc;
						break;
					case 0x0153:
						c = 0xbd;
						break;
					case 0x0178:
						c = 0xbe;
						break;
					default:
						c = replacement;
						++invalid;
					}
				} else {
					c = replacement;
					++invalid;
				}
			}
			d[i] = (char)c;
		}
		if (state) {
			state->remainingChars = 0;
			state->invalidChars += invalid;
		}
		string r_str(r.data(),length);
		return r_str;
	}


	string Latin15Codec::name() const
	{
		return "ISO-8859-15";
	}

	list<string> Latin15Codec::aliases() const
	{
		list<string> list;
		list.push_back("latin9");
		return list;
	}

	int Latin15Codec::mibEnum() const
	{
		return 111;
	}
}
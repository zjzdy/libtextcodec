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

#include "utfcodec_p.h"
#include <string>
#include "endian/endian.hpp"
namespace zdytool {
	enum { Endian = 0, Data = 1 };
	static const uchar utf8bom[] = { 0xef, 0xbb, 0xbf };

	string Utf8::convertFromUnicode(const ushort *uc, int len)
	{
		std::vector<char> result(len * 3 + 1);
		result[len * 3] = '\0';
		uchar *dst = reinterpret_cast<uchar *>(result.data());
		const ushort *src = reinterpret_cast<const ushort *>(uc);
		const ushort *const end = src + len;

		while (src != end) {
			const ushort *nextAscii = end;

			do {
				ushort uc = *src++;
				int res = Utf8Functions::toUtf8<Utf8BaseTraits>(uc, dst, src, end);
				if (res < 0) {
					// encoding error - append '?'
					*dst++ = '?';
				}
			} while (src < nextAscii);
		}

		string result_str(result.data(),dst - reinterpret_cast<uchar *>(result.data()));
		return result_str;
	}

	string Utf8::convertFromUnicode(const ushort *uc, int len, TextCodec::ConverterState *state)
	{
		uchar replacement = '?';
		int rlen = 3*len;
		int surrogate_high = -1;
		if (state) {
			if (state->flags & TextCodec::ConvertInvalidToNull)
				replacement = 0;
			if (!(state->flags & TextCodec::IgnoreHeader))
				rlen += 3;
			if (state->remainingChars)
				surrogate_high = state->state_data[0];
		}


		std::vector<char> rstr(rlen+1);
		rstr[rlen] = '\0';
		uchar *cursor = reinterpret_cast<uchar *>(rstr.data());
		const ushort *src = reinterpret_cast<const ushort *>(uc);
		const ushort *const end = src + len;

		int invalid = 0;
		if (state && !(state->flags & TextCodec::IgnoreHeader)) {
			// append UTF-8 BOM
			*cursor++ = utf8bom[0];
			*cursor++ = utf8bom[1];
			*cursor++ = utf8bom[2];
		}

		while (src != end) {
			int res;
			ushort uc;
			if (surrogate_high != -1) {
				uc = surrogate_high;
				surrogate_high = -1;
				res = Utf8Functions::toUtf8<Utf8BaseTraits>(uc, cursor, src, end);
			} else {
				uc = *src++;
				res = Utf8Functions::toUtf8<Utf8BaseTraits>(uc, cursor, src, end);
			}
			if (Z_LIKELY(res >= 0))
				continue;

			if (res == Utf8BaseTraits::Error) {
				// encoding error
				++invalid;
				*cursor++ = replacement;
			} else if (res == Utf8BaseTraits::EndOfString) {
				surrogate_high = uc;
				break;
			}
		}

		string rstr_str(rstr.data(),cursor - (const uchar*)rstr.data());
		if (state) {
			state->invalidChars += invalid;
			state->flags = TextCodec::ConversionFlags(state->flags | TextCodec::IgnoreHeader);
			state->remainingChars = 0;
			if (surrogate_high >= 0) {
				state->remainingChars = 1;
				state->state_data[0] = surrogate_high;
			}
		}
		return rstr_str;
	}

	u16string Utf8::convertToUnicode(const char *chars, int len)
	{
		// UTF-8 to UTF-16 always needs the exact same number of words or less:
		//    UTF-8     UTF-16
		//   1 byte     1 word
		//   2 bytes    1 word
		//   3 bytes    1 word
		//   4 bytes    2 words (one surrogate pair)
		// That is, we'll use the full buffer if the input is US-ASCII (1-byte UTF-8),
		// half the buffer for U+0080-U+07FF text (e.g., Greek, Cyrillic, Arabic) or
		// non-BMP text, and one third of the buffer for U+0800-U+FFFF text (e.g, CJK).
		//
		// The table holds for invalid sequences too: we'll insert one replacement char
		// per invalid byte.
		std::vector<uint16_t> result(len+1);
		result[len] = '\0';
		ushort *data = const_cast<ushort*>(result.data()); // we know we're not shared
		const ushort *end = convertToUnicode(data, chars, len);
		u16string result_str(result.data(),end - data);
		return result_str;
	}

	/*!
		\overload

		Converts the UTF-8 sequence of \a len octets beginning at \a chars to
		a sequence of uint16_t starting at \a buffer. The buffer is expected to be
		large enough to hold the result. An upper bound for the size of the
		buffer is \a len (uint16_t)s.

		If, during decoding, an error occurs, a SpecialCharacter::ReplacementCharacter is
		written.

		Returns a pointer to one past the last uint16_t written.

		This function never throws.
	*/

	ushort *Utf8::convertToUnicode(ushort *buffer, const char *chars, int len)
	{
		ushort *dst = reinterpret_cast<ushort *>(buffer);
		const uchar *src = reinterpret_cast<const uchar *>(chars);
		const uchar *end = src + len;

		const uchar *nextAscii = end;
		// at least one non-ASCII entry
		// check if we failed to decode the UTF-8 BOM; if so, skip it
		if (Z_UNLIKELY(src == reinterpret_cast<const uchar *>(chars))
				&& end - src >= 3
				&& Z_UNLIKELY(src[0] == utf8bom[0] && src[1] == utf8bom[1] && src[2] == utf8bom[2])) {
			src += 3;
		}

		while (src < end) {
			nextAscii = end;
			do {
				uchar b = *src++;
				int res = Utf8Functions::fromUtf8<Utf8BaseTraits>(b, dst, src, end);
				if (res < 0) {
					// decoding error
					*dst++ = SpecialCharacter::ReplacementCharacter;
				}
			} while (src < nextAscii);
		}

		return reinterpret_cast<ushort *>(dst);
	}

	u16string Utf8::convertToUnicode(const char *chars, int len, TextCodec::ConverterState *state)
	{
		bool headerdone = false;
		ushort replacement = SpecialCharacter::ReplacementCharacter;
		int invalid = 0;
		int res;
		uchar ch = 0;

		// See above for buffer requirements for stateless decoding. However, that
		// fails if the state is not empty. The following situations can add to the
		// requirements:
		//  state contains      chars starts with           requirement
		//   1 of 2 bytes       valid continuation          0
		//   2 of 3 bytes       same                        0
		//   3 bytes of 4       same                        +1 (need to insert surrogate pair)
		//   1 of 2 bytes       invalid continuation        +1 (need to insert replacement and restart)
		//   2 of 3 bytes       same                        +1 (same)
		//   3 of 4 bytes       same                        +1 (same)
		std::vector<uint16_t> result(len+2);
		result[len+1] = '\0';

		ushort *dst = const_cast<ushort *>(result.data());
		const uchar *src = reinterpret_cast<const uchar *>(chars);
		const uchar *end = src + len;

		if (state) {
			if (state->flags & TextCodec::IgnoreHeader)
				headerdone = true;
			if (state->flags & TextCodec::ConvertInvalidToNull)
				replacement = SpecialCharacter::Null;
			if (state->remainingChars) {
				// handle incoming state first
				uchar remainingCharsData[4]; // longest UTF-8 sequence possible
				int remainingCharsCount = state->remainingChars;
				int newCharsToCopy = std::min<int>(sizeof(remainingCharsData) - remainingCharsCount, end - src);

				memset(remainingCharsData, 0, sizeof(remainingCharsData));
				memcpy(remainingCharsData, &state->state_data[0], remainingCharsCount);
				memcpy(remainingCharsData + remainingCharsCount, src, newCharsToCopy);

				const uchar *begin = &remainingCharsData[1];
				res = Utf8Functions::fromUtf8<Utf8BaseTraits>(remainingCharsData[0], dst, begin,
						static_cast<const uchar *>(remainingCharsData) + remainingCharsCount + newCharsToCopy);
				if (res == Utf8BaseTraits::Error || (res == Utf8BaseTraits::EndOfString && len == 0)) {
					// special case for len == 0:
					// if we were supplied an empty string, terminate the previous, unfinished sequence with error
					++invalid;
					*dst++ = replacement;
				} else if (res == Utf8BaseTraits::EndOfString) {
					// if we got EndOfString again, then there were too few bytes in src;
					// copy to our state and return
					state->remainingChars = remainingCharsCount + newCharsToCopy;
					memcpy(&state->state_data[0], remainingCharsData, state->remainingChars);
					return u16string();
				} else if (!headerdone && res >= 0) {
					// eat the UTF-8 BOM
					headerdone = true;
					if (dst[-1] == 0xfeff)
						--dst;
				}

				// adjust src now that we have maybe consumed a few chars
				if (res >= 0) {
					src += res - remainingCharsCount;
				}
			}
		}

		// main body, stateless decoding
		res = 0;
		const uchar *start = src;
		while (res >= 0 && src < end) {
			ch = *src++;
			res = Utf8Functions::fromUtf8<Utf8BaseTraits>(ch, dst, src, end);
			if (!headerdone && res >= 0) {
				headerdone = true;
				if (src == start + 3) { // 3 == sizeof(utf8-bom)
					// eat the UTF-8 BOM (it can only appear at the beginning of the string).
					if (dst[-1] == 0xfeff)
						--dst;
				}
			}
			if (res == Utf8BaseTraits::Error) {
				res = 0;
				++invalid;
				*dst++ = replacement;
			}
		}

		if (!state && res == Utf8BaseTraits::EndOfString) {
			// unterminated UTF sequence
			*dst++ = SpecialCharacter::ReplacementCharacter;
			while (src++ < end)
				*dst++ = SpecialCharacter::ReplacementCharacter;
		}

		u16string result_str(result.data(),dst - (const ushort *)result.data());
		if (state) {
			state->invalidChars += invalid;
			if (headerdone)
				state->flags = TextCodec::ConversionFlags(state->flags | TextCodec::IgnoreHeader);
			if (res == Utf8BaseTraits::EndOfString) {
				--src; // unread the byte in ch
				state->remainingChars = end - src;
				memcpy(&state->state_data[0], src, end - src);
			} else {
				state->remainingChars = 0;
			}
		}
		return result_str;
	}

	string Utf16::convertFromUnicode(const ushort *uc, int len, TextCodec::ConverterState *state, DataEndianness e)
	{
		DataEndianness endian = e;
		int length =  2*len;
		if (!state || (!(state->flags & TextCodec::IgnoreHeader))) {
			length += 2;
		}
		if (e == DetectEndianness) {
			endian = (Z_BYTE_ORDER == Z_BIG_ENDIAN) ? BigEndianness : LittleEndianness;
		}

		std::vector<char> d(length+1);
		d[length] = '\0';
		char *data = d.data();
		if (!state || !(state->flags & TextCodec::IgnoreHeader)) {
			ushort bom(SpecialCharacter::ByteOrderMark);
			if (endian == BigEndianness) {
				data[0] = UCS2Tool::row(bom);
				data[1] = UCS2Tool::cell(bom);
			} else {
				data[0] = UCS2Tool::cell(bom);
				data[1] = UCS2Tool::row(bom);
			}
			data += 2;
		}
		if (endian == BigEndianness) {
			for (int i = 0; i < len; ++i) {
				*(data++) = UCS2Tool::row(uc[i]);
				*(data++) = UCS2Tool::cell(uc[i]);
			}
		} else {
			for (int i = 0; i < len; ++i) {
				*(data++) = UCS2Tool::cell(uc[i]);
				*(data++) = UCS2Tool::row(uc[i]);
			}
		}

		if (state) {
			state->remainingChars = 0;
			state->flags = TextCodec::ConversionFlags(state->flags | TextCodec::IgnoreHeader);
		}
		string d_str(d.data(),length);
		return d_str;
	}

	u16string Utf16::convertToUnicode(const char *chars, int len, TextCodec::ConverterState *state, DataEndianness e)
	{
		DataEndianness endian = e;
		bool half = false;
		uchar buf = 0;
		bool headerdone = false;
		if (state) {
			headerdone = state->flags & TextCodec::IgnoreHeader;
			if (endian == DetectEndianness)
				endian = (DataEndianness)state->state_data[Endian];
			if (state->remainingChars) {
				half = true;
				buf = state->state_data[Data];
			}
		}
		if (headerdone && endian == DetectEndianness)
			endian = (Z_BYTE_ORDER == Z_BIG_ENDIAN) ? BigEndianness : LittleEndianness;

		std::vector<uint16_t> result(len+1);
		result[len] ='\0';
		ushort *zch = (ushort *)result.data();
		while (len--) {
			if (half) {
				ushort ch =0;
				if (endian == LittleEndianness) {
					UCS2Tool::setRow(ch,*chars++);
					UCS2Tool::setCell(ch,buf);
				} else {
					UCS2Tool::setRow(ch,buf);
					UCS2Tool::setCell(ch,*chars++);
				}
				if (!headerdone) {
					headerdone = true;
					if (endian == DetectEndianness) {
						if (ch == SpecialCharacter::ByteOrderSwapped) {
							endian = LittleEndianness;
						} else if (ch == SpecialCharacter::ByteOrderMark) {
							endian = BigEndianness;
						} else {
							if (Z_BYTE_ORDER == Z_BIG_ENDIAN) {
								endian = BigEndianness;
							} else {
								endian = LittleEndianness;
								ch = ushort((ch >> 8) | ((ch & 0xff) << 8));
							}
							*zch++ = ch;
						}
					} else if (ch != SpecialCharacter::ByteOrderMark) {
						*zch++ = ch;
					}
				} else {
					*zch++ = ch;
				}
				half = false;
			} else {
				buf = *chars++;
				half = true;
			}
		}
		u16string result_str(result.data(),zch - result.data());

		if (state) {
			if (headerdone)
				state->flags = TextCodec::ConversionFlags(state->flags | TextCodec::IgnoreHeader);
			state->state_data[Endian] = endian;
			if (half) {
				state->remainingChars = 1;
				state->state_data[Data] = buf;
			} else {
				state->remainingChars = 0;
				state->state_data[Data] = 0;
			}
		}
		return result_str;
	}

	string Utf32::convertFromUnicode(const ushort *uc, int len, TextCodec::ConverterState *state, DataEndianness e)
	{
		DataEndianness endian = e;
		int length =  4*len;
		if (!state || (!(state->flags & TextCodec::IgnoreHeader))) {
			length += 4;
		}
		if (e == DetectEndianness) {
			endian = (Z_BYTE_ORDER == Z_BIG_ENDIAN) ? BigEndianness : LittleEndianness;
		}

		std::vector<char> d(length+1);
		d[length] ='\0';
		char *data = d.data();
		if (!state || !(state->flags & TextCodec::IgnoreHeader)) {
			if (endian == BigEndianness) {
				data[0] = 0;
				data[1] = 0;
				data[2] = (char)0xfe;
				data[3] = (char)0xff;
			} else {
				data[0] = (char)0xff;
				data[1] = (char)0xfe;
				data[2] = 0;
				data[3] = 0;
			}
			data += 4;
		}

		int uc_pos = 0;
		if (endian == BigEndianness) {
			while (uc_pos < len) {
				uint cp = uc[uc_pos];
				uc_pos++;

				*(data++) = cp >> 24;
				*(data++) = (cp >> 16) & 0xff;
				*(data++) = (cp >> 8) & 0xff;
				*(data++) = cp & 0xff;
			}
		} else {
			while (uc_pos < len) {
				uint cp = uc[uc_pos];
				uc_pos++;

				*(data++) = cp & 0xff;
				*(data++) = (cp >> 8) & 0xff;
				*(data++) = (cp >> 16) & 0xff;
				*(data++) = cp >> 24;
			}
		}

		if (state) {
			state->remainingChars = 0;
			state->flags = TextCodec::ConversionFlags(state->flags | TextCodec::IgnoreHeader);
		}
		string d_str(d.data(),length);
		return d_str;
	}

	u16string Utf32::convertToUnicode(const char *chars, int len, TextCodec::ConverterState *state, DataEndianness e)
	{
		DataEndianness endian = e;
		uchar tuple[4];
		int num = 0;
		bool headerdone = false;
		if (state) {
			headerdone = state->flags & TextCodec::IgnoreHeader;
			if (endian == DetectEndianness) {
				endian = (DataEndianness)state->state_data[Endian];
			}
			num = state->remainingChars;
			memcpy(tuple, &state->state_data[Data], 4);
		}
		if (headerdone && endian == DetectEndianness)
			endian = (Z_BYTE_ORDER == Z_BIG_ENDIAN) ? BigEndianness : LittleEndianness;

		int result_size = (num + len) >> 2 << 1;
		std::vector<uint16_t> result(result_size+1);
		result[result_size] = '\0';
		ushort *zch = (ushort *)result.data();

		const char *end = chars + len;
		while (chars < end) {
			tuple[num++] = *chars++;
			if (num == 4) {
				if (!headerdone) {
					if (endian == DetectEndianness) {
						if (tuple[0] == 0xff && tuple[1] == 0xfe && tuple[2] == 0 && tuple[3] == 0 && endian != BigEndianness) {
							endian = LittleEndianness;
							num = 0;
							continue;
						} else if (tuple[0] == 0 && tuple[1] == 0 && tuple[2] == 0xfe && tuple[3] == 0xff && endian != LittleEndianness) {
							endian = BigEndianness;
							num = 0;
							continue;
						} else if (Z_BYTE_ORDER == Z_BIG_ENDIAN) {
							endian = BigEndianness;
						} else {
							endian = LittleEndianness;
						}
					} else if ((((endian == BigEndianness) == (Z_BYTE_ORDER == Z_BIG_ENDIAN)) ? zdytool::endian::fromUnaligned<uint32_t>(tuple) : zdytool::endian::endian_reverse(zdytool::endian::fromUnaligned<uint32_t>(tuple))) == SpecialCharacter::ByteOrderMark) {
						num = 0;
						continue;
					}
				}
				uint code = (((endian == BigEndianness) == (Z_BYTE_ORDER == Z_BIG_ENDIAN)) ? zdytool::endian::fromUnaligned<uint32_t>(tuple) : zdytool::endian::endian_reverse(zdytool::endian::fromUnaligned<uint32_t>(tuple)));
				if (UCS4Tool::requiresSurrogates(code)) {
					*zch++ = UCS4Tool::highSurrogate(code);
					*zch++ = UCS4Tool::lowSurrogate(code);
				} else {
					*zch++ = code;
				}
				num = 0;
			}
		}
		u16string result_str(result.data(),zch - result.data());

		if (state) {
			if (headerdone)
				state->flags = TextCodec::ConversionFlags(state->flags | TextCodec::IgnoreHeader);
			state->state_data[Endian] = endian;
			state->remainingChars = num;
			memcpy(&state->state_data[Data], tuple, 4);
		}
		return result_str;
	}

	Utf8Codec::~Utf8Codec()
	{
	}

	string Utf8Codec::convertFromUnicode(const ushort *uc, int len, ConverterState *state) const
	{
		return Utf8::convertFromUnicode(uc, len, state);
	}

	void Utf8Codec::convertToUnicode(u16string *target, const char *chars, int len, ConverterState *state) const
	{
		*target += Utf8::convertToUnicode(chars, len, state);
	}

	u16string Utf8Codec::convertToUnicode(const char *chars, int len, ConverterState *state) const
	{
		return Utf8::convertToUnicode(chars, len, state);
	}

	string Utf8Codec::name() const
	{
		return "UTF-8";
	}

	int Utf8Codec::mibEnum() const
	{
		return 106;
	}

	Utf16Codec::~Utf16Codec()
	{
	}

	string Utf16Codec::convertFromUnicode(const ushort *uc, int len, ConverterState *state) const
	{
		return Utf16::convertFromUnicode(uc, len, state, e);
	}

	u16string Utf16Codec::convertToUnicode(const char *chars, int len, ConverterState *state) const
	{
		return Utf16::convertToUnicode(chars, len, state, e);
	}

	int Utf16Codec::mibEnum() const
	{
		return 1015;
	}

	string Utf16Codec::name() const
	{
		return "UTF-16";
	}

	list<string> Utf16Codec::aliases() const
	{
		return list<string>();
	}

	int Utf16BECodec::mibEnum() const
	{
		return 1013;
	}

	string Utf16BECodec::name() const
	{
		return "UTF-16BE";
	}

	list<string> Utf16BECodec::aliases() const
	{
		list<string> list;
		return list;
	}

	int Utf16LECodec::mibEnum() const
	{
		return 1014;
	}

	string Utf16LECodec::name() const
	{
		return "UTF-16LE";
	}

	list<string> Utf16LECodec::aliases() const
	{
		list<string> list;
		return list;
	}

	Utf32Codec::~Utf32Codec()
	{
	}

	string Utf32Codec::convertFromUnicode(const ushort *uc, int len, ConverterState *state) const
	{
		return Utf32::convertFromUnicode(uc, len, state, e);
	}

	u16string Utf32Codec::convertToUnicode(const char *chars, int len, ConverterState *state) const
	{
		return Utf32::convertToUnicode(chars, len, state, e);
	}

	int Utf32Codec::mibEnum() const
	{
		return 1017;
	}

	string Utf32Codec::name() const
	{
		return "UTF-32";
	}

	list<string> Utf32Codec::aliases() const
	{
		list<string> list;
		return list;
	}

	int Utf32BECodec::mibEnum() const
	{
		return 1018;
	}

	string Utf32BECodec::name() const
	{
		return "UTF-32BE";
	}

	list<string> Utf32BECodec::aliases() const
	{
		list<string> list;
		return list;
	}

	int Utf32LECodec::mibEnum() const
	{
		return 1019;
	}

	string Utf32LECodec::name() const
	{
		return "UTF-32LE";
	}

	list<string> Utf32LECodec::aliases() const
	{
		list<string> list;
		return list;
	}
}
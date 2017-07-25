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

#include "windowscodec_p.h"
#include <windows.h>
namespace zdytool {
	WindowsLocalCodec::WindowsLocalCodec()
	{
	}

	WindowsLocalCodec::~WindowsLocalCodec()
	{
	}

	u16string WindowsLocalCodec::convertToUnicode(const char *chars, int length, ConverterState *state) const
	{
		const char *mb = chars;
		int mblen = length;

		if (!mb || !mblen)
			return u16string();

		wchar_t *wc = new wchar_t[4096+1]();
		size_t wc_length = 4096;
		int len;
		u16string sp;
		bool prepend = false;
		char state_data = 0;
		int remainingChars = 0;

		//save the current state information
		if (state) {
			state_data = (char)state->state_data[0];
			remainingChars = state->remainingChars;
		}

		//convert the pending charcter (if available)
		if (state && remainingChars) {
			char prev[3] = {0};
			prev[0] = state_data;
			prev[1] = mb[0];
			remainingChars = 0;
			len = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
										prev, 2, wc, 4096);
			if (len) {
				prepend = true;
				ushort a[2] = {wc[0],'\0'};
				sp.append(a);
				mb++;
				mblen--;
				wc[0] = 0;
			}
		}

		while (!(len=MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED|MB_ERR_INVALID_CHARS,
					mb, mblen, wc, wc_length))) {
			int r = GetLastError();
			if (r == ERROR_INSUFFICIENT_BUFFER) {
					const int wclen = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
										mb, mblen, 0, 0);
					delete[] wc;
					wc = new wchar_t[wclen+1]();
					wc[wclen] = '\0';
					wc_length = wclen;
					//wc.resize(wclen);
			} else if (r == ERROR_NO_UNICODE_TRANSLATION) {
				//find the last non NULL character
				while (mblen > 1  && !(mb[mblen-1]))
					mblen--;
				//check whether,  we hit an invalid character in the middle
				if ((mblen <= 1) || (remainingChars && state_data))
					return convertToUnicodeCharByChar(chars, length, state);
				//Remove the last character and try again...
				state_data = mb[mblen-1];
				remainingChars = 1;
				mblen--;
			} else {
				// Fail.
				break;
			}
		}

		if (len <= 0)
			return u16string();

		if (wc[len-1] == 0) // len - 1: we don't want terminator
			--len;

		//save the new state information
		if (state) {
			state->state_data[0] = (char)state_data;
			state->remainingChars = remainingChars;
		}
		u16string s((ushort*)wc, len);
		delete[] wc;
		if (prepend) {
			return sp+s;
		}
		return s;
	}

	u16string WindowsLocalCodec::convertToUnicodeCharByChar(const char *chars, int length, ConverterState *state) const
	{
		if (!chars || !length)
			return u16string();

		int copyLocation = 0;
		int extra = 2;
		if (state && state->remainingChars) {
			copyLocation = state->remainingChars;
			extra += copyLocation;
		}
		int newLength = length + extra;
		char *mbcs = new char[newLength];
		//ensure that we have a NULL terminated string
		mbcs[newLength-1] = 0;
		mbcs[newLength-2] = 0;
		memcpy(&(mbcs[copyLocation]), chars, length);
		if (copyLocation) {
			//copy the last character from the state
			mbcs[0] = (char)state->state_data[0];
			state->remainingChars = 0;
		}
		const char *mb = mbcs;
#if !defined(WINAPI_FAMILY_PHONE_APP) || WINAPI_FAMILY!=WINAPI_FAMILY_PHONE_APP
		const char *next = 0;
		u16string s;
		while ((next = CharNextExA(CP_ACP, mb, 0)) != mb) {
			wchar_t wc[2] ={0};
			int charlength = next - mb;
			int len = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED|MB_ERR_INVALID_CHARS, mb, charlength, wc, 2);
			if (len>0) {
				ushort a[2] = {wc[0],'\0'};
				s.append(a);
			} else {
				int r = GetLastError();
				//check if the character being dropped is the last character
				if (r == ERROR_NO_UNICODE_TRANSLATION && mb == (mbcs+newLength -3) && state) {
					state->remainingChars = 1;
					state->state_data[0] = (char)*mb;
				}
			}
			mb = next;
		}
#else
		u16string s;
		size_t size = mbstowcs(NULL, mb, length);
		if (size < 0) {
			return u16string();
		}
		wchar_t* ws = new wchar_t[size + 2];
		ws[size +1] = 0;
		ws[size] = 0;
		size = mbstowcs(ws, mb, length);
		s.append((ushort*)ws,size);
		for (size_t i = 0; i < size; i++)
		delete [] ws;
#endif
		delete [] mbcs;
		return s;
	}

	string WindowsLocalCodec::convertFromUnicode(const ushort *ch, int uclen, ConverterState *) const
	{
		if (!ch)
			return string();
		if (uclen == 0)
			return string("");
		BOOL used_def;
		char *mb = new char[4096+1]();
		size_t mb_size = 4096;
		int len;
		while (!(len=WideCharToMultiByte(CP_ACP, 0, (const wchar_t*)ch, uclen,
					mb, mb_size-1, 0, &used_def)))
		{
			int r = GetLastError();
			if (r == ERROR_INSUFFICIENT_BUFFER) {
				delete[] mb;
				mb = new char[1+WideCharToMultiByte(CP_ACP, 0,
													 (const wchar_t*)ch, uclen,
													 0, 0, 0, &used_def)]();
					// and try again...
			} else {
				// Fail.  Probably can't happen in fact (dwFlags is 0).
				fprintf(stderr,
						"WideCharToMultiByte: Cannot convert multibyte text (error %d): %ls\n",
						r, reinterpret_cast<const wchar_t*>(u16string(ch, uclen).data()));
				break;
			}
		}
		string mb_str(mb,len);
		delete[] mb;
		return mb_str;
	}


	string WindowsLocalCodec::name() const
	{
		return "System";
	}

	int WindowsLocalCodec::mibEnum() const
	{
		return 0;
	}
}
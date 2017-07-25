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

#include "textcodec.h"
#include "textcodec_p.h"

#include "utfcodec_p.h"
#include "latincodec_p.h"
#include "tsciicodec_p.h"
#include "isciicodec_p.h"

#ifdef WIN32
#  include "windowscodec_p.h"
#endif
#  include "simplecodec_p.h"
#if !defined(Z_NO_BIG_TEXTCODECS)
#  ifndef __INTEGRITY
#    include "gb18030codec_p.h"
#    include "eucjpcodec_p.h"
#    include "jiscodec_p.h"
#    include "sjiscodec_p.h"
#    include "euckrcodec_p.h"
#    include "big5codec_p.h"
#  endif // !__INTEGRITY
#endif // !Z_NO_BIG_TEXTCODECS

#include <stdlib.h>
#include <ctype.h>
#include <locale.h>
#if defined (_XOPEN_UNIX) && !defined(__QNXNTO__) && !defined(__osf__) && !(defined(__ANDROID__) || defined(ANDROID))
# include <langinfo.h>
#endif

typedef list<TextCodec*>::const_iterator TextCodecListConstIt;
list<TextCodec*> allCodecs;
TextCodec *codecForLocale_m;
TextCodecCache codecCache;

static std::recursive_mutex textCodecsMutex;

static char z_tolower(char c)
{ if (c >= 'A' && c <= 'Z') return c + 0x20; return c; }
static bool z_isalnum(char c)
{ return (c >= '0' && c <= '9') || ((c | 0x20) >= 'a' && (c | 0x20) <= 'z'); }

bool TextCodec::TextCodecNameMatch(const char *n, const char *h)
{
    if (stricmp(n, h) == 0)
        return true;

    // if the letters and numbers are the same, we have a match
    while (*n != '\0') {
        if (z_isalnum(*n)) {
            for (;;) {
                if (*h == '\0')
                    return false;
                if (z_isalnum(*h))
                    break;
                ++h;
            }
            if (z_tolower(*n) != z_tolower(*h))
                return false;
            ++h;
        }
        ++n;
    }
    while (*h && !z_isalnum(*h))
           ++h;
    return (*h == '\0');
}


#if !defined(WIN32) && !defined(Z_LOCALE_IS_UTF8)
static TextCodec *checkForCodec(const string &name) {
    TextCodec *c = TextCodec::codecForName(name);
    if (!c) {
        const int index = name.find('@');
        if (index != -1) {
            c = TextCodec::codecForName(name.substr(0,index));
        }
    }
    return c;
}
#endif

static void setup();

// \threadsafe
// this returns the codec the method sets up as locale codec to
// avoid a race condition in codecForLocale() when
// setCodecForLocale(0) is called at the same time.
static TextCodec *setupLocaleMapper()
{
    TextCodec *locale = 0;

    {
        std::lock_guard<std::recursive_mutex> locker(textCodecsMutex);
        if (allCodecs.empty())
            setup();
    }
#if defined(Z_LOCALE_IS_UTF8)
    locale = TextCodec::codecForName("UTF-8");
#elif defined(WIN32)
    locale = TextCodec::codecForName("System");
#else

    // First try getting the codecs name from nl_langinfo and see
    // if we have a builtin codec for it.

#if defined (_XOPEN_UNIX) && !defined(__osf__)
    char *charset = nl_langinfo(CODESET);
    if (charset)
        locale = TextCodec::codecForName(charset);
#endif

    if (!locale) {
        // Very poorly defined and followed standards causes lots of
        // code to try to get all the cases...

        // Try to determine locale codeset from locale name assigned to
        // LC_CTYPE category.

        // First part is getting that locale name.  First try setlocale() which
        // definitely knows it, but since we cannot fully trust it, get ready
        // to fall back to environment variables.
        const string ctype = setlocale(LC_CTYPE, 0);

        // Get the first nonempty value from $LC_ALL, $LC_CTYPE, and $LANG
        // environment variables.
        auto env = std::getenv("LC_ALL");
        string lang = env ? env : "";
        if (lang.empty() || lang == "C") {
            env = std::getenv("LC_CTYPE");
            lang = env ? env : "";
        }
        if (lang.empty() || lang == "C") {
            env = std::getenv("LANG");
            lang = env ? env : "";
        }

        // Now try these in order:
        // 1. CODESET from ctype if it contains a .CODESET part (e.g. en_US.ISO8859-15)
        // 2. CODESET from lang if it contains a .CODESET part
        // 3. ctype (maybe the locale is named "ISO-8859-1" or something)
        // 4. locale (ditto)
        // 5. check for "@euro"
        // 6. guess locale from ctype unless ctype is "C"
        // 7. guess locale from lang

        // 1. CODESET from ctype if it contains a .CODESET part (e.g. en_US.ISO8859-15)
        int indexOfDot = ctype.find('.');
        if (indexOfDot != -1)
            locale = checkForCodec( ctype.substr(indexOfDot + 1) );

        // 2. CODESET from lang if it contains a .CODESET part
        if (!locale) {
            indexOfDot = lang.find('.');
            if (indexOfDot != -1)
                locale = checkForCodec( lang.substr(indexOfDot + 1) );
        }

        // 3. ctype (maybe the locale is named "ISO-8859-1" or something)
        if (!locale && !ctype.empty() && ctype != "C")
            locale = checkForCodec(ctype);

        // 4. locale (ditto)
        if (!locale && !lang.empty())
            locale = checkForCodec(lang);

        // 5. "@euro"
        if ((!locale && (ctype.find("@euro") != string::npos)) || (lang.find("@euro") != string::npos))
            locale = checkForCodec("ISO 8859-15");
    }

#endif
    // If everything failed, we default to 8859-1
    if (!locale)
        locale = TextCodec::codecForName("ISO 8859-1");
    codecForLocale_m = locale;
    return locale;
}

static void setup()
{
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;

    (void)new TsciiCodec;
    for (int i = 0; i < 9; ++i)
        (void)new IsciiCodec(i);
    for (int i = 0; i < SimpleTextCodec::numSimpleCodecs; ++i)
        (void)new SimpleTextCodec(i);

#  if !defined(Z_NO_BIG_TEXTCODECS) && !defined(__INTEGRITY)
    (void)new Gb18030Codec;
    (void)new GbkCodec;
    (void)new Gb2312Codec;
    (void)new EucJpCodec;
    (void)new JisCodec;
    (void)new SjisCodec;
    (void)new EucKrCodec;
    (void)new CP949Codec;
    (void)new Big5Codec;
    (void)new Big5hkscsCodec;
#  endif // !Z_NO_BIG_TEXTCODECS && !__INTEGRITY

#if defined(WIN32)
    (void) new WindowsLocalCodec;
#endif // WIN32

    (void)new Utf16Codec;
    (void)new Utf16BECodec;
    (void)new Utf16LECodec;
    (void)new Utf32Codec;
    (void)new Utf32BECodec;
    (void)new Utf32LECodec;
    (void)new Latin15Codec;
    (void)new Latin1Codec;
    (void)new Utf8Codec;
}

/*!
    \enum TextCodec::ConversionFlag

    \value DefaultConversion  No flag is set.
    \value ConvertInvalidToNull  If this flag is set, each invalid input
                                 character is output as a null character.
    \value IgnoreHeader  Ignore any Unicode byte-order mark and don't generate any.

    \omitvalue FreeFunction
*/

/*!
    \fn TextCodec::ConverterState::ConverterState(ConversionFlags flags)

    Constructs a ConverterState object initialized with the given \a flags.
*/

/*!
    Destroys the ConverterState object.
*/
TextCodec::ConverterState::~ConverterState()
{
    if (flags & FreeFunction)
        (TextCodecUnalignedPointer::decode(state_data))(this);
    else if (d)
        free(d);
}

/*!
    \class TextCodec
    \brief The TextCodec class provides conversions between text encodings.
    \reentrant

    TextCodec classes to help with converting non-Unicode formats to and
    from Unicode. You can also create your own codec classes.

    The supported encodings are:

    \list
    \li \l{Big5 Text Codec}{Big5}
    \li \l{Big5-HKSCS Text Codec}{Big5-HKSCS}
    \li CP949
    \li \l{EUC-JP Text Codec}{EUC-JP}
    \li \l{EUC-KR Text Codec}{EUC-KR}
    \li \l{GBK Text Codec}{GB18030}
    \li HP-ROMAN8
    \li IBM 850
    \li IBM 866
    \li IBM 874
    \li \l{ISO 2022-JP (JIS) Text Codec}{ISO 2022-JP}
    \li ISO 8859-1 to 10
    \li ISO 8859-13 to 16
    \li Iscii-Bng, Dev, Gjr, Knd, Mlm, Ori, Pnj, Tlg, and Tml
    \li KOI8-R
    \li KOI8-U
    \li Macintosh
    \li \l{Shift-JIS Text Codec}{Shift-JIS}
    \li TIS-620
    \li \l{TSCII Text Codec}{TSCII}
    \li UTF-8
    \li UTF-16
    \li UTF-16BE
    \li UTF-16LE
    \li UTF-32
    \li UTF-32BE
    \li UTF-32LE
    \li Windows-1250 to 1258
    \endlist

    \l {TextCodec}s can be used as follows to convert some locally encoded
    string to Unicode. Suppose you have some string encoded in Russian
    KOI8-R encoding, and want to convert it to Unicode. The simple way
    to do it is like this:

    \code
    std::string encodedString = "...";
    TextCodec *codec = TextCodec::codecForName("KOI8-R");
    std::basic_string<uint16_t> u16string = codec->toUnicode(encodedString);
    \endcode

    After this, \c string holds the text converted to Unicode.
    Converting a string from Unicode to the local encoding is just as
    easy:

    \code
    std::basic_string<uint16_t> string = "...";
    TextCodec *codec = TextCodec::codecForName("KOI8-R");
    std::string encodedString = codec->fromUnicode(string);
    \endcode

    Some care must be taken when trying to convert the data in chunks,
    for example, when receiving it over a network. In such cases it is
    possible that a multi-byte character will be split over two
    chunks. At best this might result in the loss of a character and
    at worst cause the entire conversion to fail.

    The approach to use in these situations is to create a TextDecoder
    object for the codec and use this TextDecoder for the whole
    decoding process, as shown below:

    \code
    TextCodec *codec = TextCodec::codecForName("Shift-JIS");
    TextDecoder *decoder = codec->makeDecoder();

    std::basic_string<uint16_t> string;
    while (new_data_available()) {
        std::string chunk = get_new_data();
       string += decoder->toUnicode(chunk);
    }
    delete decoder;
    \endcode

    The TextDecoder object maintains state between chunks and therefore
    works correctly even if a multi-byte character is split between
    chunks.

    \section1 Creating Your Own Codec Class

    Support for new text encodings can be added to libtextcodec by creating
    TextCodec subclasses.

    The pure virtual functions describe the encoder to the system and the
    coder is used as required in the different text file formats supported
    by others, for the locale-specific character input and output.

    To add support for another encoding to libtextcodec, make a subclass of
    TextCodec and implement the functions listed in the table below.

    \table
    \header \li Function \li Description

    \row \li name()
         \li Returns the official name for the encoding. If the
            encoding is listed in the
            \l{IANA character-sets encoding file}, the name
            should be the preferred MIME name for the encoding.

    \row \li aliases()
         \li Returns a list of alternative names for the encoding.
            TextCodec provides a default implementation that returns
            an empty list. For example, "ISO-8859-1" has "latin1",
            "CP819", "IBM819", and "iso-ir-100" as aliases.

    \row \li \l{TextCodec::mibEnum()}{mibEnum()}
         \li Return the MIB enum for the encoding if it is listed in
            the \l{IANA character-sets encoding file}.

    \row \li convertToUnicode()
         \li Converts an 8-bit character string to Unicode.

    \row \li convertFromUnicode()
         \li Converts a Unicode string to an 8-bit character string.
    \endtable

    \sa TextDecoder, TextEncoder
*/

/*!
    Constructs a TextCodec, and gives it the highest precedence. The
    TextCodec should always be constructed on the heap (i.e. with \c
    new). Libtextcodec takes ownership and will delete it when the application
    terminates.
*/
TextCodec::TextCodec()
{
    std::lock_guard<std::recursive_mutex> locker(textCodecsMutex);

    if (allCodecs.empty())
        setup();

    allCodecs.push_front(this);
}


/*!
    \nonreentrant

    Destroys the TextCodec. Note that you should not delete codecs
    yourself: once created they become libtextcodec responsibility.
*/
TextCodec::~TextCodec()
{
}

/*!
    \fn TextCodec *TextCodec::codecForName(const char *name)

    Searches all installed TextCodec objects and returns the one
    which best matches \a name; the match is case-insensitive. Returns
    0 if no codec matching the name \a name could be found.
*/

/*!
    \threadsafe
    Searches all installed TextCodec objects and returns the one
    which best matches \a name; the match is case-insensitive. Returns
    0 if no codec matching the name \a name could be found.
*/
TextCodec *TextCodec::codecForName(const string &name)
{
    if (name.size() <= 0)
        return 0;

    std::lock_guard<std::recursive_mutex> locker(textCodecsMutex);

    setup();
    TextCodecCache *cache = &codecCache;
    TextCodec *codec;
    if (cache && cache->find(name) != cache->cend()) {
        codec = cache->at(name);
        if (codec)
            return codec;
    }

    for (TextCodecListConstIt it = allCodecs.cbegin(), cend = allCodecs.cend(); it != cend; ++it) {
        TextCodec *cursor = *it;
        if (TextCodecNameMatch(cursor->name().data(), name.data())) {
            if (cache)
                cache->insert(std::pair<string, TextCodec *>(name, cursor));
            return cursor;
        }
        list<string> aliases = cursor->aliases();
        for (list<string>::const_iterator ait = aliases.cbegin(), acend = aliases.cend(); ait != acend; ++ait) {
            if (TextCodecNameMatch((*ait).data(), name.data())) {
                if (cache)
                    cache->insert(std::pair<string, TextCodec *>(name, cursor));
                return cursor;
            }
        }
    }

    return 0;
}


/*!
    \threadsafe
    Returns the TextCodec which matches the
    \l{TextCodec::mibEnum()}{MIBenum} \a mib.
*/
TextCodec* TextCodec::codecForMib(int mib)
{
    std::lock_guard<std::recursive_mutex> locker(textCodecsMutex);

    if (allCodecs.empty())
        setup();

    //key = "MIB: " + string::number(mib);
    std::stringstream key_s;
    key_s << "MIB: " << mib;
    string key = key_s.str();

    TextCodecCache *cache = &codecCache;
    TextCodec *codec;
    if (cache && cache->find(key) != cache->cend()) {
        codec = cache->at(key);
        if (codec)
            return codec;
    }

    for (TextCodecListConstIt it = allCodecs.cbegin(), cend = allCodecs.cend(); it != cend; ++it) {
        TextCodec *cursor = *it;
        if (cursor->mibEnum() == mib) {
            if (cache)
                cache->insert(std::pair<string, TextCodec *>(key, cursor));
            return cursor;
        }
    }
    return 0;
}

/*!
    \threadsafe
    Returns the list of all available codecs, by name. Call
    TextCodec::codecForName() to obtain the TextCodec for the name.

    The list may contain many mentions of the same codec
    if the codec has aliases.

    \sa availableMibs(), name(), aliases()
*/
list<string> TextCodec::availableCodecs()
{
    std::lock_guard<std::recursive_mutex> locker(textCodecsMutex);

    if (allCodecs.empty())
        setup();

    list<string> codecs;

    for (TextCodecListConstIt it = allCodecs.cbegin(), cend = allCodecs.cend(); it != cend; ++it) {
        codecs.push_back((*it)->name());
        codecs.merge((*it)->aliases());
    }
    return codecs;
}

/*!
    \threadsafe
    Returns the list of MIBs for all available codecs. Call
    TextCodec::codecForMib() to obtain the TextCodec for the MIB.

    \sa availableCodecs(), mibEnum()
*/
list<int> TextCodec::availableMibs()
{
    std::lock_guard<std::recursive_mutex> locker(textCodecsMutex);

    if (allCodecs.empty())
        setup();

    list<int> codecs;

    for (TextCodecListConstIt it = allCodecs.cbegin(), cend = allCodecs.cend(); it != cend; ++it)
        codecs.push_back((*it)->mibEnum());

    return codecs;
}

/*!
    \nonreentrant

    Set the codec to \a c; this will be returned by
    codecForLocale(). If \a c is a null pointer, the codec is reset to
    the default.

    This might be needed for some applications that want to use their
    own mechanism for setting the locale.

    \sa codecForLocale()
*/
void TextCodec::setCodecForLocale(TextCodec *c)
{
    codecForLocale_m = c;
}

/*!
    \threadsafe
    Returns a pointer to the codec most suitable for this locale.

    On Windows, the codec will be based on a system locale.
    Note that in these cases the codec's name will be "System".
*/

TextCodec* TextCodec::codecForLocale()
{
    TextCodec *codec = codecForLocale_m;
    if (!codec) {
        // setupLocaleMapper locks as necessary
        codec = setupLocaleMapper();
    }

    return codec;
}


/*!
    \fn std::string TextCodec::name() const

    TextCodec subclasses must reimplement this function. It returns
    the name of the encoding supported by the subclass.

    If the codec is registered as a character set in the
    \l{IANA character-sets encoding file} this method should
    return the preferred mime name for the codec if defined,
    otherwise its name.
*/

/*!
    \fn int TextCodec::mibEnum() const

    Subclasses of TextCodec must reimplement this function. It
    returns the \l{TextCodec::mibEnum()}{MIBenum} (see \l{IANA character-sets encoding file}
    for more information). It is important that each TextCodec
    subclass returns the correct unique value for this function.
*/

/*!
  Subclasses can return a number of aliases for the codec in question.

  Standard aliases for codecs can be found in the
  \l{IANA character-sets encoding file}.
*/
list<string> TextCodec::aliases() const
{
    return list<string>();
}

/*!
    \fn std::basic_string<uint16_t> TextCodec::convertToUnicode(const char *chars, int len,
                                             ConverterState *state) const

    TextCodec subclasses must reimplement this function.

    Converts the first \a len characters of \a chars from the
    encoding of the subclass to Unicode, and returns the result in a
    std::basic_string<uint16_t>.

    \a state can be 0, in which case the conversion is stateless and
    default conversion rules should be used. If state is not 0, the
    codec should save the state after the conversion in \a state, and
    adjust the \c remainingChars and \c invalidChars members of the struct.
*/

/*!
    \fn std::string TextCodec::convertFromUnicode(const uint16_t *input, int number,
                                                  ConverterState *state) const

    TextCodec subclasses must reimplement this function.

    Converts the first \a number of characters from the \a input array
    from Unicode to the encoding of the subclass, and returns the result
    in a string.

    \a state can be 0 in which case the conversion is stateless and
    default conversion rules should be used. If state is not 0, the
    codec should save the state after the conversion in \a state, and
    adjust the \c remainingChars and \c invalidChars members of the struct.
*/

/*!
    Creates a TextDecoder with a specified \a flags to decode chunks
    of \c{char *} data to create chunks of Unicode data.

    The caller is responsible for deleting the returned object.
*/
TextDecoder* TextCodec::makeDecoder(TextCodec::ConversionFlags flags) const
{
    return new TextDecoder(this, flags);
}

/*!
    Creates a TextEncoder with a specified \a flags to encode chunks
    of Unicode data as \c{char *} data.

    The caller is responsible for deleting the returned object.
*/
TextEncoder* TextCodec::makeEncoder(TextCodec::ConversionFlags flags) const
{
    return new TextEncoder(this, flags);
}

/*!
    \fn std::string TextCodec::fromUnicode(const uint16_t *input, int number,
                                           ConverterState *state) const

    Converts the first \a number of characters from the \a input array
    from Unicode to the encoding of this codec, and returns the result
    in a string.

    The \a state of the convertor used is updated.
*/

/*!
    Converts \a str from Unicode to the encoding of this codec, and
    returns the result in a string.
*/
string TextCodec::fromUnicode(const u16string& str) const
{
    return convertFromUnicode(str.data(), str.length(), 0);
}

/*!
    \fn std::basic_string<uint16_t> TextCodec::toUnicode(const char *input, int size,
                                      ConverterState *state) const

    Converts the first \a size characters from the \a input from the
    encoding of this codec to Unicode, and returns the result in a
    std::basic_string<uint16_t>.

    The \a state of the convertor used is updated.
*/

/*!
    Converts \a a from the encoding of this codec to Unicode, and
    returns the result in a std::basic_string<uint16_t>.
*/
u16string TextCodec::toUnicode(const string& a) const
{
    return convertToUnicode(a.data(), a.length(), 0);
}

/*!
    Returns \c true if the Unicode character \a ch can be fully encoded
    with this codec; otherwise returns \c false.
*/
bool TextCodec::canEncode(ushort ch) const
{
    ConverterState state;
    state.flags = ConvertInvalidToNull;
    convertFromUnicode(&ch, 1, &state);
    return (state.invalidChars == 0);
}

/*!
    \overload

    \a s contains the string being tested for encode-ability.
*/
bool TextCodec::canEncode(const u16string& s) const
{
    ConverterState state;
    state.flags = ConvertInvalidToNull;
    convertFromUnicode(s.data(), s.length(), &state);
    return (state.invalidChars == 0);
}

/*!
    \overload

    \a chars contains the source characters.
*/
u16string TextCodec::toUnicode(const char *chars) const
{
    int len = strlen(chars);
    return convertToUnicode(chars, len, 0);
}


/*!
    \class TextEncoder
    \brief The TextEncoder class provides a state-based encoder.
    \reentrant

    A text encoder converts text from Unicode into an encoded text format
    using a specific codec.

    The encoder converts Unicode into another format, remembering any
    state that is required between calls.

    \sa TextCodec::makeEncoder(), TextDecoder
*/

/*!
    \fn TextEncoder::TextEncoder(const TextCodec *codec)

    Constructs a text encoder for the given \a codec.
*/

/*!
    Constructs a text encoder for the given \a codec and conversion \a flags.
*/
TextEncoder::TextEncoder(const TextCodec *codec, TextCodec::ConversionFlags flags)
    : c(codec), state()
{
    state.flags = flags;
}

/*!
    Destroys the encoder.
*/
TextEncoder::~TextEncoder()
{
}

/*!
    \internal
    Determines whether the eecoder encountered a failure while decoding the input. If
    an error was encountered, the produced result is undefined, and gets converted as according
    to the conversion flags.
 */
bool TextEncoder::hasFailure() const
{
    return state.invalidChars != 0;
}

/*!
    Converts the uint16 type string \a str into an encoded string.
*/
string TextEncoder::fromUnicode(const u16string& str)
{
    string result = c->fromUnicode(str.data(), str.length(), &state);
    return result;
}

/*!
    \overload

    Converts \a len characters (not bytes) from \a uc, and returns the
    result in a string.
*/
string TextEncoder::fromUnicode(const ushort *uc, int len)
{
    string result = c->fromUnicode(uc, len, &state);
    return result;
}

/*!
    \class TextDecoder
    \brief The TextDecoder class provides a state-based decoder.
    \reentrant

    A text decoder converts text from an encoded text format into Unicode
    using a specific codec.

    The decoder converts text in this format into Unicode, remembering any
    state that is required between calls.

    \sa TextCodec::makeDecoder(), TextEncoder
*/

/*!
    \fn TextDecoder::TextDecoder(const TextCodec *codec)

    Constructs a text decoder for the given \a codec.
*/

/*!
    Constructs a text decoder for the given \a codec and conversion \a flags.
*/

TextDecoder::TextDecoder(const TextCodec *codec, TextCodec::ConversionFlags flags)
    : c(codec), state()
{
    state.flags = flags;
}

/*!
    Destroys the decoder.
*/
TextDecoder::~TextDecoder()
{
}

/*!
    \fn std::basic_string<uint16_t> TextDecoder::toUnicode(const char *chars, int len)

    Converts the first \a len bytes in \a chars to Unicode, returning
    the result.

    If not all characters are used (e.g. if only part of a multi-byte
    encoding is at the end of the characters), the decoder remembers
    enough state to continue with the next call to this function.
*/
u16string TextDecoder::toUnicode(const char *chars, int len)
{
    return c->toUnicode(chars, len, &state);
}

void from_latin1(ushort *dst, const char *str, size_t size)
{
    while (size--)
        *dst++ = (uchar)*str++;
}
void to_latin1(uchar *dst, const ushort *src, int length)
{
    while (length--) {
        *dst++ = (*src>0xff) ? '?' : (uchar) *src;
        ++src;
    }
}
u16string u16string_fromLatin1(const char *str, int size)
{
    std::vector<uint16_t> d(size+1);
    d[size] = '\0';
    from_latin1(d.data(), str, uint(size));
    std::basic_string<uint16_t> s(&d[0],size);
    return s;
}
string u16string_toLatin1(const ushort *src, int length)
{
    std::vector<char> d(length+1);
    d[length] = '\0';
    to_latin1((uchar *)d.data(), src, length);
    return string(d.data(),length);
}


/*! \overload

    The converted string is returned in \a target.
 */
void TextDecoder::toUnicode(u16string *target, const char *chars, int len)
{
    if(!target)
        return;
    switch (c->mibEnum()) {
    case 106: // utf8
        static_cast<const Utf8Codec*>(c)->convertToUnicode(target, chars, len, &state);
        break;
    case 4: // latin1
        target->resize(len);
        from_latin1((ushort*)target->data(), chars, len);
        break;
    default:
        *target = c->toUnicode(chars, len, &state);
    }
}


/*!
    \overload

    Converts the bytes in the byte array specified by \a ba to Unicode
    and returns the result.
*/
u16string TextDecoder::toUnicode(const string &ba)
{
    return c->toUnicode(ba.data(), ba.length(), &state);
}

/*!
    Tries to detect the encoding of the provided snippet of HTML in
    the given byte array, \a ba, by checking the BOM (Byte Order Mark)
    and the content-type meta header and returns a TextCodec instance
    that is capable of decoding the html to unicode.  If the codec
    cannot be detected from the content provided, \a defaultCodec is
    returned.

    \sa codecForUtfText()
*/
TextCodec *TextCodec::codecForHtml(const string &ba, TextCodec *defaultCodec)
{
    // determine charset
    TextCodec *c = TextCodec::codecForUtfText(ba, 0);
    if (!c) {
        string header = ba.substr(0,1024);
        std::transform(header.begin(),header.end(),header.begin(),z_tolower);
        int pos = header.find("meta ");
        if (pos != -1) {
            pos = header.find("charset=", pos);
            if (pos != -1) {
                pos += strlen("charset=");

                int pos2 = pos;
                // The attribute can be closed with either """, "'", ">" or "/",
                // none of which are valid charset characters.
                while ((size_t)++pos2 < header.size()) {
                    char ch = header.at(pos2);
                    if (ch == '\"' || ch == '\'' || ch == '>') {
                        string name = header.substr(pos, pos2 - pos);
                        if (name == "unicode") // ICU will return UTF-16.
                            name = "UTF-8";
                        c = TextCodec::codecForName(name);
                        return c ? c : defaultCodec;
                    }
                }
            }
        }
    }
    if (!c)
        c = defaultCodec;

    return c;
}

/*!
    \overload

    Tries to detect the encoding of the provided snippet of HTML in
    the given byte array, \a ba, by checking the BOM (Byte Order Mark)
    and the content-type meta header and returns a TextCodec instance
    that is capable of decoding the html to unicode. If the codec cannot
    be detected, this overload returns a Latin-1 TextCodec.
*/
TextCodec *TextCodec::codecForHtml(const string &ba)
{
    return codecForHtml(ba, TextCodec::codecForName("ISO-8859-1"));
}

/*!
    Tries to detect the encoding of the provided snippet \a ba by
    using the BOM (Byte Order Mark) and returns a TextCodec instance
    that is capable of decoding the text to unicode. If the codec
    cannot be detected from the content provided, \a defaultCodec is
    returned.

    \sa codecForHtml()
*/
TextCodec *TextCodec::codecForUtfText(const string &ba, TextCodec *defaultCodec)
{
    const int arraySize = ba.size();

    if (arraySize > 3) {
        if ((uchar)ba[0] == 0x00
            && (uchar)ba[1] == 0x00
            && (uchar)ba[2] == 0xFE
            && (uchar)ba[3] == 0xFF)
            return TextCodec::codecForMib(1018); // utf-32 be
        else if ((uchar)ba[0] == 0xFF
                 && (uchar)ba[1] == 0xFE
                 && (uchar)ba[2] == 0x00
                 && (uchar)ba[3] == 0x00)
            return TextCodec::codecForMib(1019); // utf-32 le
    }

    if (arraySize < 2)
        return defaultCodec;
    if ((uchar)ba[0] == 0xfe && (uchar)ba[1] == 0xff)
        return TextCodec::codecForMib(1013); // utf16 be
    else if ((uchar)ba[0] == 0xff && (uchar)ba[1] == 0xfe)
        return TextCodec::codecForMib(1014); // utf16 le

    if (arraySize < 3)
        return defaultCodec;
    if ((uchar)ba[0] == 0xef
        && (uchar)ba[1] == 0xbb
        && (uchar)ba[2] == 0xbf)
        return TextCodec::codecForMib(106); // utf-8

    return defaultCodec;
}

/*!
    \overload

    Tries to detect the encoding of the provided snippet \a ba by
    using the BOM (Byte Order Mark) and returns a TextCodec instance
    that is capable of decoding the text to unicode. If the codec
    cannot be detected, this overload returns a Latin-1 TextCodec.

    \sa codecForHtml()
*/
TextCodec *TextCodec::codecForUtfText(const string &ba)
{
    return codecForUtfText(ba, TextCodec::codecForMib(/*Latin 1*/ 4));
}
/*!
    \internal
    Determines whether the decoder encountered a failure while decoding the
    input. If an error was encountered, the produced result is undefined, and
    gets converted as according to the conversion flags.
 */
bool TextDecoder::hasFailure() const
{
    return state.invalidChars != 0;
}

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

#include "gb18030bitmap.h"
#include <bitset>

#define InRange(c, lower, upper)  (((c) >= (lower)) && ((c) <= (upper)))
#define IsLatin(c)        ((c) <= 0x7F)
#define IsByteInGb2312(c)        (InRange((c), 0xA1, 0xFE))
#define Is1stByte(c)        (InRange((c), 0x81, 0xFE))
#define Is2ndByteIn2Bytes(c)        (InRange((c), 0x40, 0xFE) && (c) != 0x7F)
#define Is2ndByteIn4Bytes(c)        (InRange((c), 0x30, 0x39))
#define Is2ndByte(c)        (Is2ndByteIn2Bytes(c) || Is2ndByteIn4Bytes(c))
#define Is3rdByte(c)        (InRange((c), 0x81, 0xFE))
#define Is4thByte(c)        (InRange((c), 0x30, 0x39))

/*!
    \fn Gb18030Bitmap::Block Gb18030Bitmap::findFirstBlock(const unsigned char *chars, int len)

    Find the \a chars 's first word's block type, returning the result.
    The \a len is the \a chars max length.
*/
Gb18030Bitmap::Block Gb18030Bitmap::findFirstBlock(const unsigned char *chars, int len) {
    if (len < 1)
        return ERROR;
    if (IsLatin(*chars))
        return ASCII;
    if (len < 2 || !Is1stByte(*chars) || !Is2ndByte(*(chars + 1)))
        return ERROR;
    if (Is2ndByteIn2Bytes(*(chars + 1))) {
        if (IsByteInGb2312(*(chars + 1))) {
            if (InRange(*chars, 0xA1, 0xA9))
                return GB2312_1;
            if (InRange(*chars, 0xAA, 0xAF))
                return GB2312_USER_1;
            if (InRange(*chars, 0xB0, 0xF7))
                return GB2312_2;
            if (InRange(*chars, 0xF8, 0xFE))
                return GB2312_USER_2;
        }
        if (InRange(*chars, 0x81, 0xA0) && InRange(*(chars + 1), 0x40, 0xFE))
            return GBK_3;
        if (InRange(*(chars + 1), 0x40, 0xA0)) {
            if (InRange(*chars, 0xA1, 0xA7))
                return GBK_USER_3;
            if (InRange(*chars, 0xA8, 0xA9))
                return GBK_5;
            if (InRange(*chars, 0xAA, 0xFE))
                return GBK_4;
        }
        return ERROR;
    }
    if (len < 4 || !Is2ndByteIn4Bytes(*(chars + 1)) || !Is3rdByte(*(chars + 2)) || !Is4thByte(*(chars + 3)))
        return ERROR;
    if (InRange(*chars, 0x81, 0x84))
        return GB18030_2000;
    if (*chars == 0x85)
        return GB18030_EXT_1;
    if (InRange(*chars, 0x86, 0x8F))
        return GB18030_EXT_2;
    if (InRange(*chars, 0x90, 0xE3))
        return GB18030_2005;
    if (InRange(*chars, 0xE4, 0xFC))
        return GB18030_EXT_3;
    if (InRange(*chars, 0xFD, 0xFE))
        return GB18030_USER;
    return ERROR;
}

/*!
    \fn int Gb18030Bitmap::getLengthForBlock(Gb18030Bitmap::Block block)

    Return this \a block 's word occupancy bytes length.
*/
int Gb18030Bitmap::getLengthForBlock(Gb18030Bitmap::Block block) {
    if (block < 0)
        return 0;
    if (block <= int(ASCII)) //include ERROR
        return 1;
    if (block <= int(GBK_4))
        return 2;
    if (block <= int(GB18030_USER))
        return 4;
    return 0;
}

/*!
    \fn unsigned long int Gb18030Bitmap::calcOffsetForBitmap(const unsigned char *word, Gb18030Bitmap::Block wordBlock,  Gb18030Bitmap::Block startBlock, Gb18030Bitmap::CalcMode mode)

    Calc the \a word offset in the bitmap, returning the result.
    The \a wordBlock come from call findFirstBlock()'s result, and \a startBlock is the bitmap data start from what
    block.Calc offset will use \a mode to set up calc way. Same as call calcOffsetForBitmapFile() by width and height
    is one.
*/
unsigned long
Gb18030Bitmap::calcOffsetForBitmap(const unsigned char *word, Gb18030Bitmap::Block wordBlock,  Gb18030Bitmap::Block startBlock,
                                   Gb18030Bitmap::CalcMode mode) {
    if (word == nullptr || wordBlock == ERROR || startBlock == ERROR || wordBlock < startBlock)
        return 0xFFFFFFFF;
    unsigned long offset = 0;
    if (mode == Full) {
        int len = getLengthForBlock(wordBlock);
        for (int i = 0; i < len; i++) {
            offset *= 0x100;
            offset += *(word + i);
        }
        return offset;
    }
    static unsigned long fourBytesSize = (0x39 - 0x30 + 1) * (0xFE - 0x81 + 1) * (0x39 - 0x30 + 1);
    switch (wordBlock) {
        case GB18030_USER:
            if (wordBlock == GB18030_USER) {
                offset += (*word - 0xFD) * (0x39 - 0x30 + 1) * (0xFE - 0x81 + 1) * (0x39 - 0x30 + 1);
                offset += (*(word + 1) - 0x30) * (0xFE - 0x81 + 1) * (0x39 - 0x30 + 1);
                offset += (*(word + 2) - 0x81) * (0x39 - 0x30 + 1);
                offset += (*(word + 3) - 0x30);
            }
            if (startBlock == GB18030_USER)
                break;
            if (mode != OfficialDefineKnown)
                offset += (0xFC - 0xE4 + 1) * fourBytesSize;
        case GB18030_EXT_3:
            if (wordBlock == GB18030_EXT_3) {
                offset += (*word - 0xE4) * (0x39 - 0x30 + 1) * (0xFE - 0x81 + 1) * (0x39 - 0x30 + 1);
                offset += (*(word + 1) - 0x30) * (0xFE - 0x81 + 1) * (0x39 - 0x30 + 1);
                offset += (*(word + 2) - 0x81) * (0x39 - 0x30 + 1);
                offset += (*(word + 3) - 0x30);
            }
            if (startBlock == GB18030_EXT_3)
                break;
            offset += (0xE3 - 0x90 + 1) * fourBytesSize;
        case GB18030_2005:
            if (wordBlock == GB18030_2005) {
                offset += (*word - 0xE3) * (0x39 - 0x30 + 1) * (0xFE - 0x81 + 1) * (0x39 - 0x30 + 1);
                offset += (*(word + 1) - 0x30) * (0xFE - 0x81 + 1) * (0x39 - 0x30 + 1);
                offset += (*(word + 2) - 0x81) * (0x39 - 0x30 + 1);
                offset += (*(word + 3) - 0x30);
            }
            if (startBlock == GB18030_2005)
                break;
            if (mode != OfficialDefineKnown)
                offset += (0x8F - 0x86 + 1) * fourBytesSize;
        case GB18030_EXT_2:
            if (wordBlock == GB18030_EXT_2) {
                offset += (*word - 0x86) * (0x39 - 0x30 + 1) * (0xFE - 0x81 + 1) * (0x39 - 0x30 + 1);
                offset += (*(word + 1) - 0x30) * (0xFE - 0x81 + 1) * (0x39 - 0x30 + 1);
                offset += (*(word + 2) - 0x81) * (0x39 - 0x30 + 1);
                offset += (*(word + 3) - 0x30);
            }
            if (startBlock == GB18030_EXT_2)
                break;
            if (mode != OfficialDefineKnown)
                offset += fourBytesSize;
        case GB18030_EXT_1:
            if (wordBlock == GB18030_EXT_1) {
                offset += (*(word + 1) - 0x30) * (0xFE - 0x81 + 1) * (0x39 - 0x30 + 1);
                offset += (*(word + 2) - 0x81) * (0x39 - 0x30 + 1);
                offset += (*(word + 3) - 0x30);
            }
            if (startBlock == GB18030_EXT_1)
                break;
            offset += (0x84 - 0x81 + 1) * fourBytesSize;
        case GB18030_2000:
            if (wordBlock == GB18030_2000) {
                offset += (*word - 0x81) * (0x39 - 0x30 + 1) * (0xFE - 0x81 + 1) * (0x39 - 0x30 + 1);
                offset += (*(word + 1) - 0x30) * (0xFE - 0x81 + 1) * (0x39 - 0x30 + 1);
                offset += (*(word + 2) - 0x81) * (0x39 - 0x30 + 1);
                offset += (*(word + 3) - 0x30);
            }
            if (startBlock == GB18030_2000)
                break;
            offset += (0xFE - 0xAA + 1) * (0xA0 - 0x40);//not 0x7F
        case GBK_4:
            if (wordBlock == GBK_4) {
                offset += (*word - 0xAA) * (0xA0 - 0x40);
                offset += (*(word + 1) - 0x40);
                if (*(word + 1) > 0x7F) offset--;
            }
            if (startBlock == GBK_4)
                break;
            offset += (0xA9 - 0xA8 + 1) * (0xA0 - 0x40);//not 0x7F
        case GBK_5:
            if (wordBlock == GBK_5) {
                offset += (*word - 0xA8) * (0xA0 - 0x40);
                offset += (*(word + 1) - 0x40);
                if (*(word + 1) > 0x7F) offset--;
            }
            if (startBlock == GBK_5)
                break;
            if (mode != OfficialDefineKnown && mode != OfficialDefine)
                offset += (0xA7 - 0xA1 + 1) * (0xA0 - 0x40);//not 0x7F
        case GBK_USER_3:
            if (wordBlock == GBK_USER_3) {
                offset += (*word - 0xA1) * (0xA0 - 0x40);
                offset += (*(word + 1) - 0x40);
                if (*(word + 1) > 0x7F) offset--;
            }
            if (startBlock == GBK_USER_3)
                break;
            offset += (0xA0 - 0x81 + 1) * (0xFE - 0x40);//not 0x7F
        case GBK_3:
            if (wordBlock == GBK_3) {
                offset += (*word - 0x81) * (0xA0 - 0x40);
                offset += (*(word + 1) - 0x40);
                if (*(word + 1) > 0x7F) offset--;
            }
            if (startBlock == GBK_3)
                break;
            if (mode != OfficialDefineKnown && mode != OfficialDefine)
                offset += (0xFE - 0xF8 + 1) * (0xFE - 0xA1 + 1);
        case GB2312_USER_2:
            if (wordBlock == GB2312_USER_2) {
                offset += (*word - 0xF8) * (0xFE - 0xA1 + 1);
                offset += (*(word + 1) - 0xA1);
            }
            if (startBlock == GB2312_USER_2)
                break;
            offset += (0xF7 - 0xB0 + 1) * (0xFE - 0xA1 + 1);
        case GB2312_2:
            if (wordBlock == GB2312_2) {
                offset += (*word - 0xB0) * (0xFE - 0xA1 + 1);
                offset += (*(word + 1) - 0xA1);
            }
            if (startBlock == GB2312_2)
                break;
            if (mode != OfficialDefineKnown && mode != OfficialDefine)
                offset += (0xAF - 0xAA + 1) * (0xFE - 0xA1 + 1);
        case GB2312_USER_1:
            if (wordBlock == GB2312_USER_1) {
                offset += (*word - 0xAA) * (0xFE - 0xA1 + 1);
                offset += (*(word + 1) - 0xA1);
            }
            if (startBlock == GB2312_USER_1)
                break;
            offset += (0xA9 - 0xA1 + 1) * (0xFE - 0xA1 + 1);
        case GB2312_1:
            if (wordBlock == GB2312_1) {
                offset += (*word - 0xA1) * (0xFE - 0xA1 + 1);
                offset += (*(word + 1) - 0xA1);
            }
            if (startBlock == GB2312_1)
                break;
            offset += 0x7E + 1;
        case ASCII:
            if (wordBlock == ASCII)
                offset = *word;
    }
    return offset;
}

/*!
    \fn unsigned long long int Gb18030Bitmap::calcOffsetForBitmapFile(const unsigned char *word, Gb18030Bitmap::Block wordBlock, Gb18030Bitmap::Block startBlock, Gb18030Bitmap::CalcMode mode, unsigned int width, unsigned int height)

    Calc the \a word offset in the bitmap file, returning the result.
    The \a wordBlock come from call findFirstBlock()'s result, and \a startBlock is the bitmap file content start
    from what block.Calc offset will use \a mode to set up calc way. The \a width and \a height will tell this function
    the size about per word's bitmap
*/
unsigned long long Gb18030Bitmap::calcOffsetForBitmapFile(const unsigned char *word, Gb18030Bitmap::Block wordBlock,
                                                          Gb18030Bitmap::Block startBlock, Gb18030Bitmap::CalcMode mode,
                                                          unsigned int width, unsigned int height) {
    unsigned long offset = calcOffsetForBitmap(word, wordBlock, startBlock, mode);
    unsigned int size = (width + 7) / 8 * height;
    if (offset == 0xFFFF)
        return 0xFFFFFFFF;
    return offset * size;
}

/*!
    \fn std::string Gb18030Bitmap::parseBitmap(const char *bitmapData, unsigned int width, unsigned int height, char bg, char fg)

    Parse the \a bitmapData by \a width and \a height to a string, returning the result.

    The result string is use \a bg to be background and use \a fg to be foreground, split by '\n'.
    If \a ignoreEndEmpty is true, function will ignore the empty line(include full \a bg line) of result's end.
*/
std::string
Gb18030Bitmap::parseBitmap(const char *bitmapData, unsigned int width, unsigned int height, char bg, char fg,
                           bool ignoreEndEmpty) {
    std::string str;
    std::string emptyCache;
    for (int i = 0; i < height; i++) {
        std::string cache;
        for (int j = 0; j < width; j += 8) {
            std::bitset<8> b(*(bitmapData + (width + 7) / 8 * i + (j + 7) / 8));
            cache += b.to_string(bg, fg).substr(0, (j + 8 > width ? 8 : width - j));
        }
        if (ignoreEndEmpty && cache.find(fg) == std::string::npos)
            emptyCache += cache + "\n";
        else {
            str += emptyCache + cache + "\n";
            emptyCache = "";
        }
    }
    return str;
}

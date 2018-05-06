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

#ifndef LIBTEXTCODEC_GB18030BITMAP_H
#define LIBTEXTCODEC_GB18030BITMAP_H

#include <string>

/*
 * ASCII                    00-7F
 * GBK-1/GB2312-1           A1-A9   A1-FE
 * GBK-2/GB2312-2&3         B0-F7   A1-FE
 * GBK-3                    81-A0   40-FE (7F除外)
 * GBK-4                    AA-FE   40-A0 (7F除外)
 * GBK-5                    A8-A9   40-A0 (7F除外)
 * GBK-USER-1/GB2312-USER-1 AA-AF   A1-FE
 * GBK-USER-2/GB2312-USER-2 F8-FE   A1-FE
 * GBK-USER-3               A1-A7   40-A0 (7F除外)
 * GB18030-2000-1           81-84   30-39   81-FE   30-39
 * GB18030-2005-1           90-E3   30-39   81-FE   30-39
 * GB18030-EXT-1            85      30-39   81-FE   30-39
 * GB18030-EXT-2            86-8F   30-39   81-FE   30-39
 * GB18030-EXT-3            E4-FC   30-39   81-FE   30-39
 * GB18030-USER             FD-FE   30-39   81-FE   30-39
 */

class Gb18030Bitmap {
public:
    enum Block {
        ASCII = 1, //1 Byte
        GB2312_1 = 2,
        GB2312_USER_1 = 3,
        GB2312_2 = 4,
        GB2312_USER_2 = 6, //2 Bytes
        GBK_3 = 10,
        GBK_USER_3 = 11,
        GBK_5 = 12,
        GBK_4 = 13, //2 Bytes
        GB18030_2000 = 20,
        GB18030_EXT_1 = 21,
        GB18030_EXT_2 = 22,
        GB18030_2005 = 23,
        GB18030_EXT_3 = 24,
        GB18030_USER = 25, //4 Bytes
        ERROR = 0
    };
    enum CalcMode {
        Valid,
        Full,
        OfficialDefine, //No calc USER Block
        OfficialDefineKnown //No calc EXT and USER Block
    };

    static Block findFirstBlock(const unsigned char *chars, int len);
    static int getLengthForBlock(Block block);
    static unsigned long calcOffsetForBitmap(const unsigned char *word, Block wordBlock, Block startBlock, CalcMode mode = Valid);
    static unsigned long long calcOffsetForBitmapFile(const unsigned char *word, Block wordBlock, Block startBlock, CalcMode mode, unsigned int width, unsigned int height);
    static std::string parseBitmap(const char *bitmapData, unsigned int width, unsigned int height, char bg, char fg, bool ignoreEndEmpty = false);
};

#endif //LIBTEXTCODEC_GB18030BITMAP_H

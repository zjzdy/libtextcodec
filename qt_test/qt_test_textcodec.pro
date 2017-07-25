QT       = core testlib

TARGET = tst_qtextcodec
TEMPLATE = app
CONFIG += testcase
CONFIG += c++11
TESTDATA += *.txt

SOURCES += tst_qtextcodec.cpp

HEADERS += \
    ../codecs/gb18030codec_p.h \
    ../codecs/eucjpcodec_p.h \
    ../codecs/jiscodec_p.h \
    ../codecs/sjiscodec_p.h \
    ../codecs/euckrcodec_p.h \
    ../codecs/big5codec_p.h \
    ../codecs/isciicodec_p.h \
    ../codecs/latincodec_p.h \
    ../codecs/simplecodec_p.h \
    ../codecs/textcodec.h \
    ../codecs/tsciicodec_p.h \
    ../codecs/utfcodec_p.h \
    ../codecs/textcodec_p.h \
    ../codecs/jpunicode_p.h

SOURCES += \
    ../codecs/gb18030codec.cpp \
    ../codecs/eucjpcodec.cpp \
    ../codecs/jiscodec.cpp \
    ../codecs/sjiscodec.cpp \
    ../codecs/euckrcodec.cpp \
    ../codecs/big5codec.cpp \
    ../codecs/isciicodec.cpp \
    ../codecs/latincodec.cpp \
    ../codecs/simplecodec.cpp \
    ../codecs/textcodec.cpp \
    ../codecs/tsciicodec.cpp \
    ../codecs/utfcodec.cpp \
    ../codecs/jpunicode.cpp
	
	
win32{
SOURCES += ../codecs/windowscodec.cpp
HEADERS += ../codecs/windowscodec_p.h
LIBS += -lUser32
}
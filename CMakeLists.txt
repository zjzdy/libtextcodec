cmake_minimum_required(VERSION 3.8)
project(libtextcodec)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        codecs/big5codec.cpp
        codecs/big5codec_p.h
        codecs/cp949codetbl_p.h
        codecs/eucjpcodec.cpp
        codecs/eucjpcodec_p.h
        codecs/euckrcodec.cpp
        codecs/euckrcodec_p.h
        codecs/gb18030codec.cpp
        codecs/gb18030codec_p.h
        codecs/isciicodec.cpp
        codecs/isciicodec_p.h
        codecs/jiscodec.cpp
        codecs/jiscodec_p.h
        codecs/jpunicode.cpp
        codecs/jpunicode_p.h
        codecs/latincodec.cpp
        codecs/latincodec_p.h
        codecs/simplecodec.cpp
        codecs/simplecodec_p.h
        codecs/sjiscodec.cpp
        codecs/sjiscodec_p.h
        codecs/textcodec.cpp
        codecs/textcodec.h
        codecs/textcodec_p.h
        codecs/tsciicodec.cpp
        codecs/tsciicodec_p.h
        codecs/utfcodec.cpp
        codecs/utfcodec_p.h)
set(SOURCE_WINAPI_FILES
        codecs/windowscodec.cpp
        codecs/windowscodec_p.h)
set(SOURCE_ENDIAN_FILES
        codecs/endian/boost_endian_intrinsic.hpp
        codecs/endian/endian.hpp)
set(SOURCE_FILES  ${SOURCE_FILES} ${SOURCE_ENDIAN_FILES})
if(WIN32 OR CYGWIN)
    set(SOURCE_FILES  ${SOURCE_FILES} ${SOURCE_WINAPI_FILES})
    link_libraries(User32)
endif(WIN32 OR CYGWIN)
add_library(libtextcodec SHARED ${SOURCE_FILES})
add_library (libtextcodec_static STATIC ${SOURCE_FILES})
set_target_properties(libtextcodec_static PROPERTIES OUTPUT_NAME "libtextcodec")
#add_executable(libtextcodec ${SOURCE_FILES})
/*
  Smartversion
  Copyright (c) Gilles Vollant, 2002-2022

  https://github.com/gvollant/smartversion
  https://www.smartversion.com/
  https://www.winimage.com/

 This source code is licensed under MIT licence.


  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/


#ifndef ABSTRACT_COMPRESS_DECOMPRESS_COMMON_H
#define ABSTRACT_COMPRESS_DECOMPRESS_COMMON_H 1


#define ABSTRACT_CPR_PREFIX_STORE 0
#define ABSTRACT_CPR_PREFIX_DEFLATE 1
//#define ABSTRACT_CPR_PREFIX_DEFLATE64 2
#define ABSTRACT_CPR_PREFIX_ZSTD 5
#define ABSTRACT_CPR_PREFIX_LZHAM_DICT_15 9
#define ABSTRACT_CPR_PREFIX_LZHAM_DICT_16 10
#define ABSTRACT_CPR_PREFIX_LZHAM_DICT_17 11
#define ABSTRACT_CPR_PREFIX_LZHAM_DICT_18 12
#define ABSTRACT_CPR_PREFIX_LZHAM_DICT_19 13
#define ABSTRACT_CPR_PREFIX_LZHAM_DICT_20 14
#define ABSTRACT_CPR_PREFIX_LZHAM_DICT_21 15
#define ABSTRACT_CPR_PREFIX_LZHAM_DICT_22 16
#define ABSTRACT_CPR_PREFIX_LZHAM_DICT_23 17
#define ABSTRACT_CPR_PREFIX_LZHAM_DICT_24 18
#define ABSTRACT_CPR_PREFIX_LZHAM_DICT_25 19
#define ABSTRACT_CPR_PREFIX_LZHAM_DICT_26 20
#define ABSTRACT_CPR_PREFIX_LZHAM_DICT_27 21
#define ABSTRACT_CPR_PREFIX_LZHAM_DICT_28 22
#define ABSTRACT_CPR_PREFIX_LZHAM_DICT_29 23

#define ABSTRACT_CPR_PREFIX_FASTZL_LOW_BLOCKSIZE        0xe1
#define ABSTRACT_CPR_PREFIX_FASTZL_DEFAULT_BLOCKSIZE    0xe2
#define ABSTRACT_CPR_PREFIX_FASTZL_HIGH_BLOCKSIZE       0xe3

#define ABSTRACT_CPR_PREFIX_LZFSE_LOW_BLOCKSIZE         0xe5
#define ABSTRACT_CPR_PREFIX_LZFSE_DEFAULT_BLOCKSIZE     0xe6
#define ABSTRACT_CPR_PREFIX_LZFSE_HIGH_BLOCKSIZE        0xe7

#define ABSTRACT_CPR_PREFIX_LZ4_LOW_BLOCKSIZE           0xf1
#define ABSTRACT_CPR_PREFIX_LZ4_DEFAULT_BLOCKSIZE       0xf2
#define ABSTRACT_CPR_PREFIX_LZ4_HIGH_BLOCKSIZE          0xf3


#define ABSTRACT_CPR_PREFIX_GZIP 0x1f

#define FASTZLIB_LOW_BLOCKSIZE 65536
#define FASTZLIB_DEFAULT_BLOCKSIZE 262144
#define FASTZLIB_HIGH_BLOCKSIZE (262144*4)

// note if (prefix & 0xf) == 8 reserved for zlib

#define ABSTRACT_CPR_XZUTILS  0x80
#define ABSTRACT_CPR_XZUTILS_XZ_EMBEDDED  0xc0

#define ABSTRACT_CPR_PREFIX_STANDARD_XZ 0xfd
#define ABSTRACT_CPR_PREFIX_STANDARD_BZIP2 0x42
#define ABSTRACT_CPR_PREFIX_STANDARD_FASTLZLIB 0x46
#define ABSTRACT_CPR_PREFIX_STANDARD_ZSTD 0x28
#define ABSTRACT_CPR_PREFIX_STANDARD_LZ4 0x04
#define ABSTRACT_CPR_PREFIX_ZSTANDARD 0x05

// mask

#ifdef HAVE_BZIP2
#define COMPRESS_BZIP2 1
#define UNCOMPRESS_BZIP2 1
#endif



#if defined(HAVE_ZSTD) && (!defined(COMPRESS_ZSTD))
#define COMPRESS_ZSTD
#endif


#if defined(HAVE_ZSTD) && (!defined(UNCOMPRESS_ZSTD))
#define UNCOMPRESS_ZSTD
#endif


#ifdef HAVE_FAST_LZLIB
#include "fastlzlib.h"
#ifndef HAVE_LZ4
#define HAVE_LZ4
#endif
#endif

#if defined(HAVE_LZ4) && (!defined(COMPRESS_LZ4))
#define COMPRESS_LZ4
#endif

#if defined(HAVE_LZ4) && (!defined(UNCOMPRESS_LZ4))
#define UNCOMPRESS_LZ4
#endif


#if defined(HAVE_ZSTD) || defined(COMPRESS_ZSTD) || defined(UNCOMPRESS_ZSTD)
#include "zstd.h"
#endif


#if defined(HAVE_LZ4) || defined(COMPRESS_LZ4) || defined(UNCOMPRESS_LZ4)
#include "lz4frame.h"
#include "lz4.h"
#endif

#if defined(HAVE_LZHAM) && (!defined(COMPRESS_LZHAM))
#define COMPRESS_LZHAM
#endif


#if defined(HAVE_LZHAM) && (!defined(UNCOMPRESS_LZHAM))
#define UNCOMPRESS_LZHAM
#endif


#if defined(HAVE_LZHAM) || defined(COMPRESS_LZHAM) || defined(UNCOMPRESS_LZHAM)
#include "lzham.h"
#endif

#if (!defined(PREVENT_COMPILE_LZMASDK)) && defined(HAVE_LZMASDK_DECOMPRESS) && (!defined(UNCOMPRESS_LZMASDK))
#define UNCOMPRESS_LZMASDK 1
#endif

#ifdef UNCOMPRESS_LZMASDK
#include "lzma_sdk/C/Xz.h"
#include "../lzma_sdk_crc_and_crc64_init.h"
#endif

#if defined(COMPRESS_BZIP2) || defined(UNCOMPRESS_BZIP2)
#include "bzlib.h"
#endif


#if (!defined(PREVENT_COMPILE_XZEMBEDDED)) && (!defined(HAVE_XZEMBEDDED)) && (!defined(UNCOMPRESS_LZMASDK))
#define HAVE_XZEMBEDDED 1
#endif

#ifdef HAVE_XZEMBEDDED
#define UNCOMPRESS_XZEMBEDDED 1
#endif

#ifdef UNCOMPRESS_XZEMBEDDED
#include "xz.h"
#endif

#if defined(WINAPI_FAMILY) && defined(WINAPI_FAMILY_APP)
#if WINAPI_FAMILY==WINAPI_FAMILY_APP
#ifndef PREVENT_DIFSTREAM_USING_METRO_INCOMPATIBLE_FUNCTION
#define PREVENT_DIFSTREAM_USING_METRO_INCOMPATIBLE_FUNCTION 1
#endif
#endif
#endif

#if  (!(defined(SVF_EXTRACT_ONLY) || defined(_M_AMD64) || defined(_M_X64) || defined(PREVENT_COMPILE_XZUTIL) || defined(PREVENT_DIFSTREAM_USING_METRO_INCOMPATIBLE_FUNCTION)))
#ifndef HAVE_XZUTILS
#define HAVE_XZUTILS 1
#endif
#endif

#ifdef HAVE_XZUTILS
#define COMPRESS_XZUTILS 1
#define UNCOMPRESS_XZUTILS 1
#endif

// for msvc
#define LZMA_API_STATIC 1

#if defined(COMPRESS_XZUTILS) || defined(UNCOMPRESS_XZUTILS)
#ifdef LZMA_INCLUDE_COMMON
#include "common.h"
#endif
#include "lzma.h"
#endif

#endif

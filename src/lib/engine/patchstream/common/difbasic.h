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

/* difbasic.h */
/* Define basic type and structure for SmartVersion engine */
/* stream for applying */


#ifndef DIFBASIC_INCLUDED
#define DIFBASIC_INCLUDED 1


#if defined(WIN32) || defined(_WIN32)
#define SMARTVERSION_USE_WIN32
#else
#define SMARTVERSION_USE_POSIX
#endif

/****************************************************************************/

#define DSERR_END       (1)
#define DSERR_OK        (0)
#define DSERR_INTERNAL  (-101)

/****************************************************************************/
#ifdef _MSC_VER
#define SVFAPI __stdcall
#else
#define SVFAPI
#endif

#if (defined(MSDOS) || defined(_WINDOWS) || defined(WIN32))  && !defined(STDC)
#  define STDC
#endif
#if defined(__STDC__) || defined(__cplusplus) || defined(__OS2__)
#  ifndef STDC
#    define STDC
#  endif
#endif

#ifndef OF                      /* function prototypes */
#  ifdef STDC
#    define OF(args)  args
#  else
#    define OF(args)  ()
#  endif
#endif


#if (!defined(BOOL)) && (!defined(OBJC_BOOL_DEFINED))
typedef int BOOL;
#endif

#ifndef FALSE
#define FALSE               0
#endif

#ifndef TRUE
#define TRUE                1
#endif

#define DECLARE_DFHANDLE(name)    struct name##__ { int _name##_unused; }; \
                                  typedef const struct name##__ * name

#ifndef SMARTVERSION_DISCARD_UNUSED
#define SMARTVERSION_DISCARD_UNUSED(v) ((void)(v));
#endif

#ifndef BASIC_INT_TYPE_DEFINED
#define BASIC_INT_TYPE_DEFINED

typedef unsigned int dfuInt;

#ifdef HAS_STDINT_H
#include "stdint.h"
typedef uint16_t dfuShort;
typedef uint16_t dfuInt16;

typedef int32_t Int32;
typedef uint32_t UInt32;

typedef uint32_t dfuLong32;
typedef int32_t dfsLong32;


typedef uint64_t dfuLong64;
typedef int64_t dfsLong64;

typedef uintptr_t dfuIntPtr;

#else
typedef unsigned short dfuShort;

typedef unsigned short dfuInt16;
typedef signed short dfsInt16;

#ifdef _SMV_LONG_IS_NOT_32
typedef int Int32;
typedef unsigned int UInt32;
#else
typedef unsigned int dfuLong32;
typedef signed int dfsLong32;
#endif



#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef unsigned __int64 dfuLong64;
typedef signed __int64 dfsLong64;
#else
typedef signed long long int dfsLong64;
typedef unsigned long long int dfuLong64;
#endif


//typedef DWORD_PTR dfuIntPtr;
#if defined(_WIN64) || defined(WIN64)
    typedef unsigned __int64 dfuIntPtr;
#else
  #if (_MSC_VER >= 1300)
    typedef __w64 unsigned long dfuIntPtr;
  #else
    typedef unsigned long dfuIntPtr;
  #endif
#endif

#endif


typedef dfuLong64 dfuIntFileSize;
typedef dfsLong64 dfsIntFileSize;


typedef void *dfvoidp;
typedef const void *dfvoidpc;
typedef unsigned char dfbyte;
typedef dfbyte *dfbytep;
typedef const dfbyte *dfbytepc;
#endif

#ifdef _MSC_VER
#define DFSCALLBACK __stdcall
#else
#define DFSCALLBACK
#endif
/* #define DFSAPI __stdcall*/

#ifdef wchar_t
typedef wchar_t dfwchar;
#else
typedef unsigned short dfwchar;
#endif
typedef dfwchar *dfwcharp;
typedef const dfwchar *dfwcharpc;

typedef struct
{
    dfuLong32 dfSize;
    dfuLong32 dfInfoTag;
    dfbytep pInfo;
} ERROR_INFO_ITEM;
DECLARE_DFHANDLE(H_ERROR_INFO);

/*
typedef dfvoidp(*dfGivePtr) OF((dfuLong32 ulPos, dfuLong32 ulSize, dfvoidp data));
     typedef void (*dfFreePtr)
  OF((dfuLong32 pos, dfuLong32 size, dfvoidp ptr, dfvoidp data));
*/

#define COMPRESSIONPARAM_AUTOVALUE ((dfuLong32)(0xffffffffL))


// historyc : 16 K then 128 Ko
#define DEFAULT_SIZE_BUF_STREAM_KB (16)
#define DEFAULT_SIZE_BUF_STREAM (1024*DEFAULT_SIZE_BUF_STREAM_KB)



typedef struct
{
    dfuLong32 dfStructSize;
    dfuLong32 uZlibCompressRatio;
    dfuLong32 dfBlockCalcSizeSearch;

    dfuLong32 dfGeneralCompressionRatio;
    dfuLong32 dfBlockSizeInFatLikeTable;
    dfuLong32 dfBlockSizeInReduceTable;
    dfuLong32 dfAlignPredict;
    dfuLong32 dfPhysicalMemoryKB;

    dfuLong32 dfSizeFactorAlignAcceptance ;
    dfuLong32 dfMaxAlignedSearchNumber ;

    dfuLong32 dfNbHashBit ;

    dfuLong32 dfMinimalSearchAlignement;
    dfuLong32 dfSizeButStreamKB;
    dfuLong32 dfReservedA;
    dfuLong32 dfReservedB;
    dfuLong32 dfReservedC;
} COMPRESSIONPARAM;

#define COMPRESSIONPARAM_DEFINED

typedef struct
{
  dfvoidp next_in;              /* next input byte */
  dfuLong32 avail_in;             /* number of bytes available at next_in */
  dfuLong64 total_in;             /* total nb of input bytes read so far */
  dfuLong32 done_latest_in;
} IN_DATA_STREAM;

typedef struct
{
  dfvoidp next_out;             /* next output byte should be put there */
  dfuLong32 avail_out;            /* remaining free space at next_out */
  dfuLong64 total_out;            /* total nb of bytes output so far */
  dfuLong32 done_latest_out;
} OUT_DATA_STREAM;


typedef struct
{
    dfuLong32 df_msec;              /* milisecond [0..999] 10 bits */
    dfuLong32 df_sec;               /* seconds after the minute - [0..59]  6 bits */
    dfuLong32 df_min;               /* minutes after the hour - [0..59] 6 bits */
    dfuLong32 df_hour;              /* hours since midnight - [0..23]  5 bits */
    dfuLong32 df_mday;              /* day of the month - [1..31] 5 bits */
    dfuLong32 df_mon;               /* months since January - [1..12] 4 bits */
    dfuLong32 df_year;              /* years - [0..4095] 12 bits */
    dfuLong32 df_timezone_bias;     /* bias [0..255] 8 bits. this is ((bias in minute/15)+128) */
} DFSTM;


#define DFS_STORAGESTATUS_REFERENCE (0x00000001)
#define DFS_STORAGESTATUS_IDENTICAL (0x00000002)
#define DFS_STORAGESTATUS_MODIFIED  (0x00000003)
#define DFS_STORAGESTATUS_NEW       (0x00000004)
#define DFS_STORAGESTATUS_NEWSTORED (0x00000005)

#endif

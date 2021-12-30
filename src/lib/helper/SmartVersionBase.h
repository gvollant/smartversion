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

//

#ifndef _SMART_VERSION_BASE_H
#define _SMART_VERSION_BASE_H  1

#include <stdlib.h>
//
#ifdef HAS_STDINT_H
#include <stdint.h>
typedef  uint64_t smv_uint_64;
typedef  int64_t smv_int_64;
typedef  uint32_t smv_uint_32;
typedef  int32_t smv_int_32;
#else
#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef unsigned __int64 smv_uint_64;
typedef signed __int64 smv_sint_64;
typedef unsigned int smv_uint_32;
typedef signed int smv_int_32;
#else
typedef signed long long int smv_uint_64;
typedef unsigned long long int smv_sint_64;

typedef unsigned int smv_uint_32;
typedef signed int smv_int_32;
#endif
#endif

#define DECLARE_INTERNALHANDLE(name)    struct name_##__ { int _name_##_private; }; \
                                  typedef const struct name_##__ * name;



typedef struct
{
	smv_uint_32 df_msec;              /* milisecond [0..999] 10 bits */
	smv_uint_32 df_sec;               /* seconds after the minute - [0..59]  6 bits */
	smv_uint_32 df_min;               /* minutes after the hour - [0..59] 6 bits */
	smv_uint_32 df_hour;              /* hours since midnight - [0..23]  5 bits */
	smv_uint_32 df_mday;              /* day of the month - [1..31] 5 bits */
	smv_uint_32 df_mon;               /* months since January - [1..12] 4 bits */
	smv_uint_32 df_year;              /* years - [0..4095] 12 bits */
	smv_uint_32 df_timezone_bias;     /* bias [0..255] 8 bits. this is ((bias in minute/15)+128) */
} smv_file_date;

#define SMV_MINIMAL_FILE_SIZE_BUFFER (0x200)
typedef struct
{
	smv_uint_32 struct_size;

	smv_uint_32 crc32;
	//dfwcharpc FileName;
	smv_uint_64 file_size;
	smv_uint_64 file_size_encoded;

	smv_uint_64 size_in_place;
	smv_uint_64 size_inserted;

	smv_uint_32 flags; // have crc, md5, sha1, date filled, is patch/ref/compress
	smv_uint_32 flags2;
	smv_file_date file_date;
	unsigned char md5[16];
	unsigned char sha1[20];
	smv_uint_32 filename_size;
	unsigned char sha256[32];
	char filename[SMV_MINIMAL_FILE_SIZE_BUFFER];
} smv_file_info;

DECLARE_INTERNALHANDLE(smv_file)


#endif

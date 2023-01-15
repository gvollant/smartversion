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

/* */
#ifndef ABSTRACT_COMPRESS_H
#define ABSTRACT_COMPRESS_H 1

#include "zlib.h"

#define INTERNALDFSCALLBACK

#ifdef __cplusplus
extern "C" {
#endif



#define ABSTRACT_CPR_PARAM_UNPREFIXED_GZIP_0 10
#define ABSTRACT_CPR_PARAM_UNPREFIXED_GZIP_1 11
#define ABSTRACT_CPR_PARAM_UNPREFIXED_GZIP_9 19
#define ABSTRACT_CPR_PARAM_UNPREFIXED_XZ_1 131
#define ABSTRACT_CPR_PARAM_UNPREFIXED_XZ_9 139
#define ABSTRACT_CPR_PARAM_UNPREFIXED_XZ_EXTREME_1 141
#define ABSTRACT_CPR_PARAM_UNPREFIXED_XZ_EXTREME_9 149
#define ABSTRACT_CPR_PARAM_UNPREFIXED_BZIP2_1 151
#define ABSTRACT_CPR_PARAM_UNPREFIXED_BZIP2_9 159

#define ABSTRACT_CPR_PARAM_UNPREFIXED_XZ_MT_0 270
#define ABSTRACT_CPR_PARAM_UNPREFIXED_XZ_MT_1 271
#define ABSTRACT_CPR_PARAM_UNPREFIXED_XZ_MT_9 279
#define ABSTRACT_CPR_PARAM_UNPREFIXED_XZ_EXTREME_MT_0 280
#define ABSTRACT_CPR_PARAM_UNPREFIXED_XZ_EXTREME_MT_1 281
#define ABSTRACT_CPR_PARAM_UNPREFIXED_XZ_EXTREME_MT_9 289

#define ABSTRACT_CPR_PARAM_ZLIB_STORE 160

#define ABSTRACT_CPR_PARAM_FASTZLIB_LZFSE_LOW_BLOCKSIZE 161
#define ABSTRACT_CPR_PARAM_FASTZLIB_LZFSE_DEFAULT_BLOCKSIZE 162
#define ABSTRACT_CPR_PARAM_FASTZLIB_LZFSE_HIGH_BLOCKSIZE 163

#define ABSTRACT_CPR_PARAM_ZSTANDARD_PREFIXED_1 171
#define ABSTRACT_CPR_PARAM_ZSTANDARD_PREFIXED_9 179
#define ABSTRACT_CPR_PARAM_ZSTANDARD_PREFIXED_22 192


#define ABSTRACT_CPR_PARAM_ZSTANDARD_1 201
#define ABSTRACT_CPR_PARAM_ZSTANDARD_9 209
#define ABSTRACT_CPR_PARAM_ZSTANDARD_22 222

#define ABSTRACT_CPR_PARAM_LZHAM_22_1 231
#define ABSTRACT_CPR_PARAM_LZHAM_22_9 239

#define ABSTRACT_CPR_PARAM_LZHAM_23_1 241
#define ABSTRACT_CPR_PARAM_LZHAM_23_9 249

#define ABSTRACT_CPR_PARAM_LZHAM_24_1 251
#define ABSTRACT_CPR_PARAM_LZHAM_24_9 259

#define ABSTRACT_CPR_PARAM_LZHAM_26_1 261
#define ABSTRACT_CPR_PARAM_LZHAM_26_9 269


#define ABSTRACT_CPR_PARAM_LZ4_16_MIN12   308
#define ABSTRACT_CPR_PARAM_LZ4_16_12     332

#define ABSTRACT_CPR_PARAM_LZ4_20_MIN12 338
#define ABSTRACT_CPR_PARAM_LZ4_20_12  362

#define ABSTRACT_CPR_PARAM_LZ4_22_MIN12 368
#define ABSTRACT_CPR_PARAM_LZ4_22_12  392

#define ABSTRACT_CPR_PARAM_LZ4FDEC_16_MIN12   408
#define ABSTRACT_CPR_PARAM_LZ4FDEC_16_12     432

#define ABSTRACT_CPR_PARAM_LZ4FDEC_20_MIN12 438
#define ABSTRACT_CPR_PARAM_LZ4FDEC_20_12  462

#define ABSTRACT_CPR_PARAM_LZ4FDEC_22_MIN12 468
#define ABSTRACT_CPR_PARAM_LZ4FDEC_22_12  492

/* new 3.8.2 */
/* _VERYLONG_ need smartversion 3.8.2 to decompress */
#define ABSTRACT_CPR_PARAM_ZSTANDARD_MT_1 671
#define ABSTRACT_CPR_PARAM_ZSTANDARD_MT_9 679
#define ABSTRACT_CPR_PARAM_ZSTANDARD_MT_22 692


#define ABSTRACT_CPR_PARAM_ZSTANDARD_LONG_MT_1 631
#define ABSTRACT_CPR_PARAM_ZSTANDARD_LONG_MT_9 639
#define ABSTRACT_CPR_PARAM_ZSTANDARD_LONG_MT_22 652

#define ABSTRACT_CPR_PARAM_ZSTANDARD_VERYLONG_MT_1 601
#define ABSTRACT_CPR_PARAM_ZSTANDARD_VERYLONG_MT_9 609
#define ABSTRACT_CPR_PARAM_ZSTANDARD_VERYLONG_MT_22 622

#define ABSTRACT_CPR_PARAM_ZSTANDARD_VERYLONG_1 501
#define ABSTRACT_CPR_PARAM_ZSTANDARD_VERYLONG_9 509
#define ABSTRACT_CPR_PARAM_ZSTANDARD_VERYLONG_22 522

#define ABSTRACT_CPR_PARAM_ZSTANDARD_LONG_1 531
#define ABSTRACT_CPR_PARAM_ZSTANDARD_LONG_9 539
#define ABSTRACT_CPR_PARAM_ZSTANDARD_LONG_22 552

#define ABSTRACT_CPR_PARAM_ZSTANDARD_BIS_1 571
#define ABSTRACT_CPR_PARAM_ZSTANDARD_BIS_9 579
#define ABSTRACT_CPR_PARAM_ZSTANDARD_BIS_22 592


	struct abstract_compress_stream_s;

	typedef int(INTERNALDFSCALLBACK * t_abstract_compress) (struct abstract_compress_stream_s* strm, int flush);
	typedef int(INTERNALDFSCALLBACK * t_abstract_compress_end) (struct abstract_compress_stream_s* strm);

	typedef struct abstract_compress_stream_s {
		Bytef    *next_in;  /* next input byte */
		uInt     avail_in;  /* number of bytes available at next_in */
		dfuLong64 total_in;  /* total nb of input bytes read so far */

		Bytef    *next_out; /* next output byte should be put there */
		uInt     avail_out; /* remaining free space at next_out */
		dfuLong64 total_out; /* total nb of bytes output so far */

		t_abstract_compress fnc_compress;
		t_abstract_compress_end fnc_compress_end;

		void* p_native_stream;

		int contain_prefix;
		int is_prefix_write;
		uInt size_prefix;
		unsigned char prefix_value;
	} abstract_compress_stream;



#define ABSTR_COMPRESS_Z_OK            0
#define ABSTR_COMPRESS_Z_STREAM_END    1
#define ABSTR_COMPRESS_Z_ERRNO        (-1)
#define ABSTR_COMPRESS_Z_MISSINGLIB   (-2)


#define ABSTR_COMPRESS_Z_NO_FLUSH      0
#define ABSTR_COMPRESS_Z_SYNC_FLUSH    2
#define ABSTR_COMPRESS_Z_FINISH        4

#define ABSTR_COMPRESS_Z_DEFAULT_COMPRESSION (-1)


#define ABSTR_CPR_POS_FORMAT_NAME        0
#define ABSTR_CPR_POS_FORMAT_EXTENSION   1
#define ABSTR_CPR_POS_FORMAT_MIME        2

	void clear_abstract_compress_stream(
		abstract_compress_stream* strm);

	int abstract_compress_end(abstract_compress_stream* strm);
	int abstract_compress(abstract_compress_stream* strm, int flush);


	/*
	#define abstract_compress(p_strm,flush) \
	(((p_strm)->fnc_compress)((p_strm),(flush)))

	#define abstract_compress_end(p_strm) \
	(((p_strm)->fnc_compress_end)((p_strm)))
	*/
	/*
	int abstract_init_prefix(abstract_decompress_stream* strm);
	// inflateInit2(&pReadMultiBuffer->ReadStream[stream_number].zstream, -MAX_WBITS);
	*/

	/* deflateInit2(&stream,level,Z_DEFLATED, -MAX_WBITS, DEF_MEM_LEVEL, 0);*/
	int abstract_init_compress_raw_store(abstract_compress_stream* strm);

	int abstract_init_compress_inflate_withNegMaxWBits(abstract_compress_stream* strm, int level);

	/* deflateInit(&zstream,level);*/
	int abstract_init_compress_inflate_withoutNegMaxWBits(abstract_compress_stream* strm, int level);
	int abstract_init_compress_inflate_withNegMaxWBits_prefix(abstract_compress_stream* strm, int level);
#ifdef COMPRESS_BZIP2
	int abstract_init_compress_bzip2(abstract_compress_stream* strm, int level);
	int abstract_init_compress_bzip2_with_prefix(abstract_compress_stream* strm, int level, int prefix);
#endif

#ifdef COMPRESS_XZUTILS
	int abstract_init_compress_xzUtils(abstract_compress_stream* strm, int level, int extreme);
	int abstract_init_compress_xzUtils_with_prefix(abstract_compress_stream* strm, int level, int extreme, int prefix, int mt);
#endif

	int XflateStore_compress(abstract_compress_stream* strm, int flush);

	int abstract_init_compress_autoselect(abstract_compress_stream* strm, unsigned int ratio, int isStandardzLibWithNegMaxWBits);

	const char* abstract_init_compress_autoselect_info(unsigned int ratio,unsigned pos);

	/* to prevent abstractCompress from write prefix, call abstract_compress_consume_prefix after init */
	unsigned char abstract_compress_consume_prefix(abstract_compress_stream* p_strm);

#ifdef __cplusplus
}
#endif

#endif

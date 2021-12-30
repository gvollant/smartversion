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

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "zlib.h"
#include "compress_store.h"

#include "difbasic.h"


#include "abstractDecompress.h"
#include "abstractCompressDecompressCommon.h"

#if defined(UNCOMPRESS_LZHAM) && defined(USE_LZHAM_MINI_DECOMP_LIB)
#include "lzham_decomplib.h"

#define lzham_z_inflateInit2 lzham_decomp_z_inflateInit2
#define lzham_z_inflate lzham_decomp_z_inflate
#define lzham_z_inflateEnd lzham_decomp_z_inflateEnd
#endif

#ifdef UNCOMPRESS_XZEMBEDDED
typedef struct {
	struct xz_buf buf;
	struct xz_dec * dec;
} xz_embedded_stream_t;
#endif

typedef union
{
    z_stream zlib_stream;
#ifdef UNCOMPRESS_BZIP2
    bz_stream bzip2_stream;
#endif

#if defined(UNCOMPRESS_LZHAM) || defined(HAVE_LZHAM)
    lzham_z_stream lzham_z_stream;
#endif
#ifdef HAVE_FAST_LZLIB
	zfast_stream fastzlib_stream;
#endif

#ifdef UNCOMPRESS_XZUTILS
    lzma_stream xzutils_stream;
#endif
#ifdef UNCOMPRESS_LZMASDK
    CXzUnpacker lzma_sdk_xz_stream;
#endif
#ifdef UNCOMPRESS_XZEMBEDDED
    xz_embedded_stream_t xz_embedded_stream;
#endif
} union_native_stream;


static void ClearZLibDecompressStream(
     z_streamp strm)
{
  strm->next_in = NULL;
  strm->avail_in = 0;
  strm->total_in = 0;

  strm->next_out = NULL;
  strm->avail_out = 0;
  strm->total_out = 0;

  strm->zalloc = (alloc_func) 0;
  strm->zfree = (free_func) 0;
  strm->opaque = (voidpf) 0;
}

int abstract_init_prefix(abstract_decompress_stream* strm);
// inflateInit2(&pReadMultiBuffer->ReadStream[stream_number].zstream, -MAX_WBITS);
int abstract_init_inflate_withNegMaxWBits(abstract_decompress_stream* strm);

// inflateInit(&pReadMultiBuffer->ReadStream[stream_number].zstream);
int abstract_init_inflate_withoutNegMaxWBits(abstract_decompress_stream* strm);


int zlib_abstract_decompress(struct abs_stream_s* strm,int flush)
{
	int retValue;
    union_native_stream* native_stream = (union_native_stream*)strm->p_native_stream;
	native_stream->zlib_stream.avail_in = strm->avail_in;
	native_stream->zlib_stream.total_in = (uLong)(strm->total_in - strm->size_prefix);
	native_stream->zlib_stream.next_in = strm->next_in;

	native_stream->zlib_stream.avail_out = strm->avail_out;
	native_stream->zlib_stream.total_out = (uLong)(strm->total_out);
	native_stream->zlib_stream.next_out = strm->next_out;

	retValue = inflate(&native_stream->zlib_stream, flush);

	strm->avail_in = native_stream->zlib_stream.avail_in ;
	strm->total_in = native_stream->zlib_stream.total_in + strm->size_prefix;
	strm->next_in = native_stream->zlib_stream.next_in ;

	strm->avail_out = native_stream->zlib_stream.avail_out ;
	strm->total_out = native_stream->zlib_stream.total_out ;
	strm->next_out = native_stream->zlib_stream.next_out ;

	return retValue;
}


int zlib_abstract_decompress_end(struct abs_stream_s* strm)
{
	int retValue;

    union_native_stream* native_stream = (union_native_stream*)strm->p_native_stream;
	native_stream->zlib_stream.avail_in = strm->avail_in;
	native_stream->zlib_stream.total_in = (uLong)(strm->total_in - strm->size_prefix);
	native_stream->zlib_stream.next_in = strm->next_in;

	native_stream->zlib_stream.avail_out = strm->avail_out;
	native_stream->zlib_stream.total_out = (uLong)strm->total_out;
	native_stream->zlib_stream.next_out = strm->next_out;

	retValue = inflateEnd(&native_stream->zlib_stream);

	strm->avail_in =native_stream->zlib_stream.avail_in ;
	strm->total_in = native_stream->zlib_stream.total_in + strm->size_prefix;
	strm->next_in = native_stream->zlib_stream.next_in ;

	strm->avail_out = native_stream->zlib_stream.avail_out ;
	strm->total_out = native_stream->zlib_stream.total_out ;
	strm->next_out = native_stream->zlib_stream.next_out ;

	return retValue;
}


int zlib_store_abstract_decompress(struct abs_stream_s* strm,int flush)
{
	int retValue;

    union_native_stream* native_stream = (union_native_stream*)strm->p_native_stream;

	native_stream->zlib_stream.avail_in = strm->avail_in;
	native_stream->zlib_stream.total_in = (uLong)(strm->total_in - strm->size_prefix);
	native_stream->zlib_stream.next_in = strm->next_in;

	native_stream->zlib_stream.avail_out = strm->avail_out;
	native_stream->zlib_stream.total_out = (uLong)strm->total_out;
	native_stream->zlib_stream.next_out = strm->next_out;

	retValue = XflateStore(&native_stream->zlib_stream, flush);

	strm->avail_in = native_stream->zlib_stream.avail_in ;
	strm->total_in = native_stream->zlib_stream.total_in + strm->size_prefix;;
	strm->next_in = native_stream->zlib_stream.next_in ;

	strm->avail_out = native_stream->zlib_stream.avail_out ;
	strm->total_out = native_stream->zlib_stream.total_out ;
	strm->next_out = native_stream->zlib_stream.next_out ;

	return retValue;
}


int zlib_store_abstract_decompress_end(struct abs_stream_s* strm)
{
    //union_native_stream* native_stream = (union_native_stream*)strm->p_native_stream;
	return ABSTR_DECOMPRESS_Z_OK;
}


int error_abstract_decompress(struct abs_stream_s* strm,int flush)
{
	return ABSTR_DECOMPRESS_Z_ERRNO;
}


int error_abstract_decompress_end(struct abs_stream_s* strm)
{
	return ABSTR_DECOMPRESS_Z_ERRNO;
}


#ifdef UNCOMPRESS_LZ4


#define LZ4_LOG_WRAPPER(...) { }
//#include <stdio.h>
//#define LZ4_LOG_WRAPPER(...)  { printf(__VA_ARGS__); }

void ClearLz4DecompressStream(z_stream* strm)
{
	memset(strm, 0, sizeof(z_stream));
}


static int lz4_zlib_mini_wrap_inflate(z_streamp strm, int flush)
{
	LZ4F_decompressionContext_t dCtx = (LZ4F_decompressionContext_t)strm->state;
	LZ4F_errorCode_t errorCode;

	void* dstBuffer=strm->next_out;
	size_t dstSizePtr=(size_t)strm->avail_out;
	const void* srcBuffer=strm->next_in;
	size_t srcSizePtr=(size_t)strm->avail_in;
	size_t number_bytes_read, number_bytes_write;

	errorCode = LZ4F_decompress(dCtx, dstBuffer, &dstSizePtr, srcBuffer, &srcSizePtr, NULL);
	if (LZ4F_isError(errorCode)) {
		LZ4_LOG_WRAPPER("ERROR: lz4_zlib_mini_wrap_inflate %s\n", LZ4F_getErrorName(errorCode));
		return ABSTR_DECOMPRESS_Z_ERRNO;
	}
	number_bytes_read = srcSizePtr;
	number_bytes_write = dstSizePtr;

	strm->next_in += number_bytes_read;
	strm->total_in += (uLong)number_bytes_read;
	strm->avail_in -= (uLong)number_bytes_read;

	strm->next_out += number_bytes_write;
	strm->total_out += (uLong)number_bytes_write;
	strm->avail_out -= (uLong)number_bytes_write;

	return (errorCode == 0) ? Z_STREAM_END : Z_OK;
}


static int lz4_zlib_mini_wrap_inflateEnd (z_streamp strm)
{
	int ret = Z_OK;
	LZ4F_decompressionContext_t dCtx = (LZ4F_decompressionContext_t)strm->state;
	LZ4F_freeDecompressionContext(dCtx);

    strm->state = NULL;
    return ret;
}

int lz4_abstract_decompress(struct abs_stream_s* strm, int flush)
{
  int retValue;
  union_native_stream* native_stream = (union_native_stream*)strm->p_native_stream;
  native_stream->zlib_stream.avail_in = strm->avail_in;
  native_stream->zlib_stream.total_in = (uLong)(strm->total_in - strm->size_prefix);
  native_stream->zlib_stream.next_in = strm->next_in;

  native_stream->zlib_stream.avail_out = strm->avail_out;
  native_stream->zlib_stream.total_out = (uLong)(strm->total_out);
  native_stream->zlib_stream.next_out = strm->next_out;

  retValue = lz4_zlib_mini_wrap_inflate(&native_stream->zlib_stream, flush);

  strm->avail_in = native_stream->zlib_stream.avail_in;
  strm->total_in = native_stream->zlib_stream.total_in + strm->size_prefix;
  strm->next_in = native_stream->zlib_stream.next_in;

  strm->avail_out = native_stream->zlib_stream.avail_out;
  strm->total_out = native_stream->zlib_stream.total_out;
  strm->next_out = native_stream->zlib_stream.next_out;

  return retValue ;
}


int lz4_abstract_decompress_end(struct abs_stream_s* strm)
{
  int retValue;

  union_native_stream* native_stream = (union_native_stream*)strm->p_native_stream;
  native_stream->zlib_stream.avail_in = strm->avail_in;
  native_stream->zlib_stream.total_in = (uLong)(strm->total_in - strm->size_prefix);
  native_stream->zlib_stream.next_in = strm->next_in;

  native_stream->zlib_stream.avail_out = strm->avail_out;
  native_stream->zlib_stream.total_out = (uLong)strm->total_out;
  native_stream->zlib_stream.next_out = strm->next_out;

  retValue = lz4_zlib_mini_wrap_inflateEnd(&native_stream->zlib_stream);

  strm->avail_in = native_stream->zlib_stream.avail_in;
  strm->total_in = native_stream->zlib_stream.total_in + strm->size_prefix;
  strm->next_in = native_stream->zlib_stream.next_in;

  strm->avail_out = native_stream->zlib_stream.avail_out;
  strm->total_out = native_stream->zlib_stream.total_out;
  strm->next_out = native_stream->zlib_stream.next_out;

  return retValue;
}


int lz4DecompressInit (z_streamp strm)
{
  LZ4F_decompressionContext_t dCtx;
  LZ4F_errorCode_t const errorCode = LZ4F_createDecompressionContext(&dCtx, LZ4F_VERSION);


  if (LZ4F_isError(errorCode)) {
	  LZ4_LOG_WRAPPER("ERROR: LZ4F_createDecompressionContext %s\n", LZ4F_getErrorName(errorCode));
	  return ABSTR_DECOMPRESS_Z_ERRNO;
  }

  strm->state = (struct internal_state*) dCtx; /* use state which in not used by user */
  strm->total_in = 0;
  strm->total_out = 0;
  strm->reserved = 1; /* mark as unknown steam */

  return Z_OK;
}


#endif

#ifdef UNCOMPRESS_ZSTD


void ClearZstdDecompressStream(z_stream* strm)
{
  memset(strm, 0, sizeof(z_stream));
}


#define ZSTD_LOG_WRAPPER(...) { }
//#include <stdio.h>
//#define ZSTD_LOG_WRAPPER(...)  { printf(__VA_ARGS__); }

static int zstd_zlib_mini_wrap_inflate (z_streamp strm, int flush)
{
  ZSTD_DStream* zwd = (ZSTD_DStream*)strm->state;
  ZSTD_inBuffer in;
  ZSTD_outBuffer out;
  size_t errorCode;

  in.pos = 0;
  in.src = strm->next_in;
  in.size = strm->avail_in;

  out.pos = 0;
  out.dst = strm->next_out;
  out.size = strm->avail_out;

  errorCode = ZSTD_decompressStream(zwd, &out, &in);

  strm->next_in += in.pos;
  strm->total_in += (uLong)in.pos;
  strm->avail_in -= (uLong)in.pos;

  strm->next_out += out.pos;
  strm->total_out += (uLong)out.pos;
  strm->avail_out -= (uLong)out.pos;

  if (ZSTD_isError(errorCode)) {
	  ZSTD_LOG_WRAPPER("ERROR: zstd_zlib_mini_wrap_inflate %s\n", ZSTD_getErrorName(errorCode));
    return ABSTR_DECOMPRESS_Z_ERRNO;
  }

  //return (errorCode == 0) ? Z_STREAM_END : Z_OK;
  //return ((flush == Z_FINISH) && (strm->avail_in == 0) && (errorCode == 0)) ? Z_STREAM_END : Z_OK;
  return ((errorCode == 0) && (strm->avail_in == 0)) ? Z_STREAM_END : Z_OK;
}


static int zstd_zlib_mini_wrap_inflateEnd (z_streamp strm)
{
  int ret = Z_OK;

  //ZSTD_LOG_WRAPPER("- inflateEnd total_in=%d total_out=%d\n", (int)(strm->total_in), (int)(strm->total_out));
  {   ZSTD_DStream* zwd = (ZSTD_DStream*)strm->state;
  size_t const errorCode = ZSTD_freeDStream(zwd);
  strm->state = NULL;
  if (ZSTD_isError(errorCode)) return Z_MEM_ERROR;
  }
  return ret;
}

int zstd_abstract_decompress(struct abs_stream_s* strm, int flush)
{
  int retValue;
  union_native_stream* native_stream = (union_native_stream*)strm->p_native_stream;
  native_stream->zlib_stream.avail_in = strm->avail_in;
  native_stream->zlib_stream.total_in = (uLong)(strm->total_in - strm->size_prefix);
  native_stream->zlib_stream.next_in = strm->next_in;

  native_stream->zlib_stream.avail_out = strm->avail_out;
  native_stream->zlib_stream.total_out = (uLong)(strm->total_out);
  native_stream->zlib_stream.next_out = strm->next_out;

  retValue = zstd_zlib_mini_wrap_inflate(&native_stream->zlib_stream, flush);

  strm->avail_in = native_stream->zlib_stream.avail_in;
  strm->total_in = native_stream->zlib_stream.total_in + strm->size_prefix;
  strm->next_in = native_stream->zlib_stream.next_in;

  strm->avail_out = native_stream->zlib_stream.avail_out;
  strm->total_out = native_stream->zlib_stream.total_out;
  strm->next_out = native_stream->zlib_stream.next_out;

  return retValue ;
}


int zstd_abstract_decompress_end(struct abs_stream_s* strm)
{
  int retValue;

  union_native_stream* native_stream = (union_native_stream*)strm->p_native_stream;
  native_stream->zlib_stream.avail_in = strm->avail_in;
  native_stream->zlib_stream.total_in = (uLong)(strm->total_in - strm->size_prefix);
  native_stream->zlib_stream.next_in = strm->next_in;

  native_stream->zlib_stream.avail_out = strm->avail_out;
  native_stream->zlib_stream.total_out = (uLong)strm->total_out;
  native_stream->zlib_stream.next_out = strm->next_out;

  retValue = zstd_zlib_mini_wrap_inflateEnd(&native_stream->zlib_stream);

  strm->avail_in = native_stream->zlib_stream.avail_in;
  strm->total_in = native_stream->zlib_stream.total_in + strm->size_prefix;
  strm->next_in = native_stream->zlib_stream.next_in;

  strm->avail_out = native_stream->zlib_stream.avail_out;
  strm->total_out = native_stream->zlib_stream.total_out;
  strm->next_out = native_stream->zlib_stream.next_out;

  return retValue;
}


int zstdDecompressInit (z_streamp strm)
{
  size_t errorCode;
  ZSTD_DStream* zwd = ZSTD_createDStream();
  ZSTD_LOG_WRAPPER("- inflateInit\n");
  if (zwd == NULL) { strm->state = NULL; return Z_MEM_ERROR; }

  ZSTD_DCtx_setMaxWindowSize(zwd, ((size_t)1) << ZSTD_WINDOWLOG_MAX);

  errorCode=ZSTD_initDStream(zwd);
  if (ZSTD_isError(errorCode))
  {
    ZSTD_freeDStream(zwd);
    return ABSTR_DECOMPRESS_Z_ERRNO;
  }

  strm->state = (struct internal_state*) zwd; /* use state which in not used by user */
  strm->total_in = 0;
  strm->total_out = 0;
  strm->reserved = 1; /* mark as unknown steam */

  return Z_OK;
}
#endif


#ifdef UNCOMPRESS_BZIP2

void ClearBZip2DecompressStream(bz_stream* strm)
{
	memset(strm, 0, sizeof(bz_stream));
}

int bzip2_abstract_decompress(struct abs_stream_s* strm, int flush)
{
	int retValue;

	union_native_stream* native_stream = (union_native_stream*)strm->p_native_stream;
	native_stream->bzip2_stream.avail_in = strm->avail_in;
	native_stream->bzip2_stream.total_in_lo32 = (unsigned int)strm->total_in;
	native_stream->bzip2_stream.total_in_hi32 = (unsigned int)(strm->total_in >> 32);
	native_stream->bzip2_stream.next_in = (char*)strm->next_in;

	native_stream->bzip2_stream.avail_out = strm->avail_out;

	native_stream->bzip2_stream.total_out_lo32 = (unsigned int)strm->total_out;
	native_stream->bzip2_stream.total_out_hi32 = (unsigned int)(strm->total_out >> 32);
	native_stream->bzip2_stream.next_out = (char*)strm->next_out;

	retValue = BZ2_bzDecompress(&native_stream->bzip2_stream/*, flush*/);

	strm->avail_in = native_stream->bzip2_stream.avail_in;
	strm->total_in = native_stream->bzip2_stream.total_in_lo32 |
		(((dfuLong64)native_stream->bzip2_stream.total_in_hi32) << 32);
	strm->next_in = (Bytef*)native_stream->bzip2_stream.next_in;

	strm->avail_out = native_stream->bzip2_stream.avail_out;
	strm->total_out = native_stream->bzip2_stream.total_out_lo32 |
		(((dfuLong64)native_stream->bzip2_stream.total_out_hi32) << 32);
	strm->next_out = (Bytef*)native_stream->bzip2_stream.next_out;

	if (retValue == BZ_STREAM_END)
		retValue = ABSTR_DECOMPRESS_Z_STREAM_END;
	return retValue;
}


int bzip2_abstract_decompress_end(struct abs_stream_s* strm)
{
	int retValue;

	union_native_stream* native_stream = (union_native_stream*)strm->p_native_stream;

	native_stream->bzip2_stream.avail_in = strm->avail_in;
	native_stream->bzip2_stream.total_in_lo32 = (unsigned int)strm->total_in;
	native_stream->bzip2_stream.total_in_hi32 = (unsigned int)(strm->total_in >> 32);
	native_stream->bzip2_stream.next_in = (char*)strm->next_in;

	native_stream->bzip2_stream.avail_out = strm->avail_out;

	native_stream->bzip2_stream.total_out_lo32 = (unsigned int)strm->total_out;
	native_stream->bzip2_stream.total_out_hi32 = (unsigned int)(strm->total_out >> 32);
	native_stream->bzip2_stream.next_out = (char*)strm->next_out;

	retValue = BZ2_bzDecompressEnd(&native_stream->bzip2_stream);

	strm->avail_in = native_stream->bzip2_stream.avail_in;
	strm->total_in = native_stream->bzip2_stream.total_in_lo32 |
		(((dfuLong64)native_stream->bzip2_stream.total_in_hi32) << 32);
	strm->next_in = (Bytef*)native_stream->bzip2_stream.next_in;

	strm->avail_out = native_stream->bzip2_stream.avail_out;
	strm->total_out = native_stream->bzip2_stream.total_out_lo32 |
		(((dfuLong64)native_stream->bzip2_stream.total_out_hi32) << 32);
	strm->next_out = (Bytef*)native_stream->bzip2_stream.next_out;

	return retValue;
}
#endif


#ifdef HAVE_FAST_LZLIB

void ClearFastZlibDecompressStream(zfast_stream* strm)
{
	memset(strm, 0, sizeof(zfast_stream));
}

int fast_zlib_abstract_decompress(struct abs_stream_s* strm, int flush)
{
	int retValue;

	union_native_stream* native_stream = (union_native_stream*)strm->p_native_stream;
	for (;;)
	{
		native_stream->fastzlib_stream.avail_in = strm->avail_in;
		native_stream->fastzlib_stream.total_in = (uLong)(strm->total_in - strm->size_prefix);
		native_stream->fastzlib_stream.next_in = strm->next_in;

		native_stream->fastzlib_stream.avail_out = strm->avail_out;
		native_stream->fastzlib_stream.total_out = (uLong)(strm->total_out);
		native_stream->fastzlib_stream.next_out = strm->next_out;

		retValue = fastlzlibDecompress(&native_stream->fastzlib_stream);
		//param_fastlzlib_flush = (flush == Z_FINISH) ? Z_FINISH : Z_PARTIAL_FLUSH;
		//retValue = fastlzlibDecompress2(&native_stream->fastzlib_stream, param_fastlzlib_flush, 1);
		//if (retValue == Z_NEED_DICT) retValue = Z_OK;

		strm->avail_in = native_stream->fastzlib_stream.avail_in;
		strm->total_in = ((dfuLong64)native_stream->fastzlib_stream.total_in) + strm->size_prefix;
		strm->next_in = native_stream->fastzlib_stream.next_in;

		strm->avail_out = native_stream->fastzlib_stream.avail_out;
		strm->total_out = native_stream->fastzlib_stream.total_out;
		strm->next_out = native_stream->fastzlib_stream.next_out;

		if ((retValue!=Z_OK) || (strm->avail_in==0) || (strm->avail_out==0)) break;
	}
	return retValue;
}


int fast_zlib_abstract_decompress_end(struct abs_stream_s* strm)
{
	int retValue;

	union_native_stream* native_stream = (union_native_stream*)strm->p_native_stream;
	native_stream->fastzlib_stream.avail_in = strm->avail_in;
	native_stream->fastzlib_stream.total_in = (uLong)(strm->total_in - strm->size_prefix);
	native_stream->fastzlib_stream.next_in = strm->next_in;

	native_stream->fastzlib_stream.avail_out = strm->avail_out;
	native_stream->fastzlib_stream.total_out = (uLong)strm->total_out;
	native_stream->fastzlib_stream.next_out = strm->next_out;

	retValue = fastlzlibDecompressEnd(&native_stream->fastzlib_stream);

	strm->avail_in = native_stream->fastzlib_stream.avail_in;
	strm->total_in = native_stream->fastzlib_stream.total_in + strm->size_prefix;
	strm->next_in = native_stream->fastzlib_stream.next_in;

	strm->avail_out = native_stream->fastzlib_stream.avail_out;
	strm->total_out = native_stream->fastzlib_stream.total_out;
	strm->next_out = native_stream->fastzlib_stream.next_out;

	return retValue;
}

#endif

#ifdef UNCOMPRESS_LZMASDK

#include "lzma_sdk/C/Alloc.h"

#include "lzma_sdk/C/7zCrc.h"
#include "lzma_sdk/C/XzCrc64.h"
#include "lzma_sdk/C/7zVersion.h"


#if defined(MY_VER_MAJOR)
#if MY_VER_MAJOR>=18
#define LZMA_SDK_HAS_ISZALLOCPTR
#if (MY_VER_MAJOR>=19) || (MY_VER_MINOR>=05)
#define LZMASDK_XZUNPACKER_HAS_SRCFINISHED
#endif
#endif
#endif

#if defined(LZMA_SDK_HAS_ISZALLOCPTR)
static void *SzAlloc(ISzAllocPtr ptr, size_t size) { return MyAlloc(size); }
static void SzFree(ISzAllocPtr ptr, void *address) { MyFree(address); }
static ISzAlloc g_Alloc_ = { SzAlloc, SzFree };
#else
static void *SzAlloc(void *ptr, size_t size) { return MyAlloc(size); }
static void SzFree(void *ptr, void *address) { MyFree(address); }
static ISzAlloc g_Alloc_ = { SzAlloc, SzFree };
#endif

int abstract_decompress_buildInitCrc()
{
	CrcGenerateTable();
	Crc64GenerateTable();
	return 1;
}



static void ClearLzmaSdkUncompressStream(CXzUnpacker* strm)
{
    check_lzma_sdk_crc_init();
	memset(strm, 0, sizeof(CXzUnpacker));
	//if (abstract_decompress_buildInitCrc() != 1)
    //		return;
	// XzUnpacker_Create replaced by
#ifndef OLD_LZMA_SDK
	/*res =*/ XzUnpacker_Construct(strm, &g_Alloc_);
#else
	XzUnpacker_Create(strm, &g_Alloc_);
#endif
}

int lzmasdk_abstract_decompress(struct abs_stream_s* strm, int flush)
{

    union_native_stream* native_stream = (union_native_stream*)strm->p_native_stream;

	SizeT inLen = (SizeT)strm->avail_in;
	SizeT outLen = (SizeT)strm->avail_out;
	ECoderStatus status;
	SRes res;
	int retSuccess = ABSTR_DECOMPRESS_Z_OK;
	if ((strm->end_reached != 0) && (strm->avail_in == 0))
	{
		retSuccess = ABSTR_DECOMPRESS_Z_STREAM_END;
	}

#if defined(LZMASDK_XZUNPACKER_HAS_SRCFINISHED)
  int srcFinished = (retSuccess == ABSTR_DECOMPRESS_Z_STREAM_END) ? 1 : 0;
#endif

	res = XzUnpacker_Code(&native_stream->lzma_sdk_xz_stream,
		strm->next_out, &outLen,
		strm->next_in, &inLen,
#if defined(LZMASDK_XZUNPACKER_HAS_SRCFINISHED)
    srcFinished,
#endif
		CODER_FINISH_ANY,
		&status);

	if (XzUnpacker_IsStreamWasFinished(&native_stream->lzma_sdk_xz_stream))
	{
		strm->end_reached = 1;
		retSuccess = ABSTR_DECOMPRESS_Z_STREAM_END;
	}

	if ((status != CODER_STATUS_FINISHED_WITH_MARK) &&
		(status != CODER_STATUS_NOT_FINISHED) &&
		(status != CODER_STATUS_NEEDS_MORE_INPUT))
	{
		return ABSTR_DECOMPRESS_Z_ERRNO;
	}

	strm->avail_in -= (uInt)inLen;
	strm->total_in += (dfuLong64)inLen;
	strm->next_in += inLen;

	strm->avail_out -= (uInt)outLen;
	strm->total_out += (dfuLong64)outLen;
	strm->next_out += outLen;

	if ((status == CODER_STATUS_FINISHED_WITH_MARK) && (inLen>0))
	{
		return ABSTR_DECOMPRESS_Z_STREAM_END;
	}

	if ((status == CODER_STATUS_FINISHED_WITH_MARK) && (inLen==0))
	{
		strm->end_reached = 1;
		return ABSTR_DECOMPRESS_Z_STREAM_END;
	}

	return retSuccess;
}

int lzmasdk_abstract_decompress_end(struct abs_stream_s* strm)
{

    union_native_stream* native_stream = (union_native_stream*)strm->p_native_stream;

	XzUnpacker_Free(&native_stream->lzma_sdk_xz_stream);
	return ABSTR_DECOMPRESS_Z_OK;
}
#endif

#if defined(UNCOMPRESS_XZUTILS) || defined(ABSTRACT_CPR_XZUTILS_DYNAMIC_AND_UNCOMPRESS_XZEMBEDDED)


static void ClearXzUtilsUncompressStream(lzma_stream* strm)
{
	memset(strm,0,sizeof(lzma_stream));
}

int xzutils_abstract_decompress(struct abs_stream_s* strm,int flush)
{

    union_native_stream* native_stream = (union_native_stream*)strm->p_native_stream;

	int retValue,retValueXZ;
	lzma_action action ;


	native_stream->xzutils_stream.avail_in = strm->avail_in;
	native_stream->xzutils_stream.total_in = (strm->total_in - strm->size_prefix);
	native_stream->xzutils_stream.next_in = strm->next_in;

	native_stream->xzutils_stream.avail_out = strm->avail_out;
	native_stream->xzutils_stream.total_out = strm->total_out;
	native_stream->xzutils_stream.next_out = strm->next_out;

	action = LZMA_RUN;

#ifdef ABSTRACT_CPR_XZUTILS_DYNAMIC_AND_UNCOMPRESS_XZEMBEDDED
	retValueXZ = dynamic_lzma_code(&native_stream->xzutils_stream, action);
#else
	retValueXZ = lzma_code(&native_stream->xzutils_stream, action);
#endif

	strm->avail_in = (uInt)native_stream->xzutils_stream.avail_in ;
	strm->total_in = native_stream->xzutils_stream.total_in + strm->size_prefix;
	strm->next_in = (Bytef*)native_stream->xzutils_stream.next_in ;

	strm->avail_out = (uInt)native_stream->xzutils_stream.avail_out ;
	strm->total_out = native_stream->xzutils_stream.total_out ;
	strm->next_out = native_stream->xzutils_stream.next_out ;

	if (retValueXZ == LZMA_OK)
		retValue = ABSTR_DECOMPRESS_Z_OK;
	else
	if (retValueXZ == LZMA_STREAM_END)
		retValue = ABSTR_DECOMPRESS_Z_STREAM_END;
	else
		retValue = ABSTR_DECOMPRESS_Z_ERRNO;
//printf("ret xzutils_abstract_decompress %d-> %d (lz:%d->%d)\n",flush,retValue,(int)action,(int)retValueXZ);
	return retValue;
}


int xzutils_abstract_decompress_end(struct abs_stream_s* strm)
{

    union_native_stream* native_stream = (union_native_stream*)strm->p_native_stream;

	native_stream->xzutils_stream.avail_in = strm->avail_in;
	native_stream->xzutils_stream.total_in = (strm->total_in - strm->size_prefix);
	native_stream->xzutils_stream.next_in = strm->next_in;

	native_stream->xzutils_stream.avail_out = strm->avail_out;
	native_stream->xzutils_stream.total_out = strm->total_out;
	native_stream->xzutils_stream.next_out = strm->next_out;


#ifdef ABSTRACT_CPR_XZUTILS_DYNAMIC_AND_UNCOMPRESS_XZEMBEDDED
	dynamic_lzma_end(&native_stream->xzutils_stream);
#else
	lzma_end(&native_stream->xzutils_stream);
#endif

	strm->avail_in = (uInt)native_stream->xzutils_stream.avail_in ;
	strm->total_in = native_stream->xzutils_stream.total_in + strm->size_prefix ;
	strm->next_in = (Bytef*)native_stream->xzutils_stream.next_in ;

	strm->avail_out = (uInt)native_stream->xzutils_stream.avail_out ;
	strm->total_out = native_stream->xzutils_stream.total_out ;
	strm->next_out = native_stream->xzutils_stream.next_out ;

	return ABSTR_DECOMPRESS_Z_OK;
}
#endif

#ifdef UNCOMPRESS_XZEMBEDDED

void ClearXZEmbeddedDecompressStream(xz_embedded_stream_t* strm)
{
	memset(strm,0,sizeof(xz_embedded_stream_t));
}


int xz_embedded_abstract_decompress(struct abs_stream_s* strm,int flush)
{
	enum xz_ret xzret;
	dfuLong64 previousTotalIn,previousTotalOut;
    union_native_stream* native_stream = (union_native_stream*)strm->p_native_stream;

	previousTotalIn = (strm->total_in - strm->size_prefix);
	native_stream->xz_embedded_stream.buf.in = (const unsigned char*)strm->next_in;
	native_stream->xz_embedded_stream.buf.in_pos = 0;
	native_stream->xz_embedded_stream.buf.in_size = strm->avail_in;

	previousTotalOut = (size_t)strm->total_out;
	native_stream->xz_embedded_stream.buf.out = strm->next_out;
	native_stream->xz_embedded_stream.buf.out_pos = 0;
	native_stream->xz_embedded_stream.buf.out_size = strm->avail_out;

	xzret = xz_dec_run(native_stream->xz_embedded_stream.dec,&native_stream->xz_embedded_stream.buf);

	strm->total_in += native_stream->xz_embedded_stream.buf.in_pos;
	strm->next_in += native_stream->xz_embedded_stream.buf.in_pos;
	strm->avail_in -= (uInt)native_stream->xz_embedded_stream.buf.in_pos;

	strm->total_out += native_stream->xz_embedded_stream.buf.out_pos;
	strm->next_out += native_stream->xz_embedded_stream.buf.out_pos;
	strm->avail_out -= (uInt)native_stream->xz_embedded_stream.buf.out_pos;

	if (xzret == XZ_OK)
		return ABSTR_DECOMPRESS_Z_OK;
	if (xzret == XZ_STREAM_END)
		return ABSTR_DECOMPRESS_Z_STREAM_END;
	return ABSTR_DECOMPRESS_Z_ERRNO;
}


int xz_embedded_abstract_decompress_end(struct abs_stream_s* strm)
{

    union_native_stream* native_stream = (union_native_stream*)strm->p_native_stream;

	xz_dec_end(native_stream->xz_embedded_stream.dec);
	return ABSTR_DECOMPRESS_Z_OK;
}
#endif


#if defined(UNCOMPRESS_LZHAM) || defined(HAVE_LZHAM)

void ClearLzHamDecompressStream(lzham_z_stream* strm)
{
  memset(strm, 0, sizeof(lzham_z_stream));
}

int lzham_abstract_decompress(struct abs_stream_s* strm, int flush)
{
  int retValue;
  //int param_fastlzlib_flush;

  union_native_stream* native_stream = (union_native_stream*)strm->p_native_stream;
  native_stream->lzham_z_stream.avail_in = strm->avail_in;
  native_stream->lzham_z_stream.total_in = (uLong)(strm->total_in - strm->size_prefix);
  native_stream->lzham_z_stream.next_in = strm->next_in;

  native_stream->lzham_z_stream.avail_out = strm->avail_out;
  native_stream->lzham_z_stream.total_out = (uLong)(strm->total_out);
  native_stream->lzham_z_stream.next_out = strm->next_out;

  retValue = lzham_z_inflate(&native_stream->lzham_z_stream,flush);

  strm->avail_in = native_stream->lzham_z_stream.avail_in;
  strm->total_in = native_stream->lzham_z_stream.total_in + strm->size_prefix;
  strm->next_in = (Bytef*)native_stream->lzham_z_stream.next_in;

  strm->avail_out = native_stream->lzham_z_stream.avail_out;
  strm->total_out = native_stream->lzham_z_stream.total_out;
  strm->next_out = native_stream->lzham_z_stream.next_out;

  return retValue;
}


int lzham_abstract_decompress_end(struct abs_stream_s* strm)
{
  int retValue;

  union_native_stream* native_stream = (union_native_stream*)strm->p_native_stream;
  native_stream->lzham_z_stream.avail_in = strm->avail_in;
  native_stream->lzham_z_stream.total_in = (uLong)(strm->total_in - strm->size_prefix);
  native_stream->lzham_z_stream.next_in = strm->next_in;

  native_stream->lzham_z_stream.avail_out = strm->avail_out;
  native_stream->lzham_z_stream.total_out = (uLong)strm->total_out;
  native_stream->lzham_z_stream.next_out = strm->next_out;

  retValue = lzham_z_inflateEnd(&native_stream->lzham_z_stream);

  strm->avail_in = native_stream->lzham_z_stream.avail_in;
  strm->total_in = native_stream->lzham_z_stream.total_in + strm->size_prefix;
  strm->next_in = (Bytef*)native_stream->lzham_z_stream.next_in;

  strm->avail_out = native_stream->lzham_z_stream.avail_out;
  strm->total_out = native_stream->lzham_z_stream.total_out;
  strm->next_out = native_stream->lzham_z_stream.next_out;

  return retValue;
}

#endif

void clear_abstract_decompress_stream(
     abstract_decompress_stream* strm)
{

	strm->avail_in = 0 ;
	strm->total_in = 0 ;
	strm->next_in = NULL ;

	strm->avail_out = 0 ;
	strm->total_out = 0 ;
	strm->next_out = NULL ;

	strm->fnc_decompress = NULL;
	strm->fnc_decompress_end = NULL;
}

int abstract_init_store(abstract_decompress_stream* strm)
{
	int retValue;

	union_native_stream* native_stream = (union_native_stream*)malloc(sizeof(union_native_stream));
	if (native_stream == NULL)
		return ABSTR_DECOMPRESS_Z_MEM_ERROR;
	memset(native_stream, 0, sizeof(union_native_stream));
	strm->p_native_stream = (void*)native_stream;

	ClearZLibDecompressStream(&(native_stream->zlib_stream));

	strm->contain_prefix = 0;
	strm->is_prefix_read = 0;
	strm->size_prefix = 0;

	strm->fnc_decompress = zlib_store_abstract_decompress;
	strm->fnc_decompress_end = zlib_store_abstract_decompress_end;

	retValue = ABSTR_DECOMPRESS_Z_OK;
	return retValue;
}

int abstract_init_inflate_withoutNegMaxWBits(abstract_decompress_stream* strm)
{
	int retValue;

    union_native_stream* native_stream = (union_native_stream*)malloc(sizeof(union_native_stream));
    if (native_stream == NULL)
        return ABSTR_DECOMPRESS_Z_MEM_ERROR;
    memset(native_stream,0,sizeof(union_native_stream));
    strm->p_native_stream = (void*)native_stream;

	ClearZLibDecompressStream(&native_stream->zlib_stream);

	strm->contain_prefix=0;
	strm->is_prefix_read=0;
	strm->size_prefix=0;

	strm->fnc_decompress = zlib_abstract_decompress;
	strm->fnc_decompress_end = zlib_abstract_decompress_end;

	retValue = inflateInit(&native_stream->zlib_stream);
	return retValue;
}

int abstract_init_inflate_withNegMaxWBits(abstract_decompress_stream* strm)
{
	int retValue;

    union_native_stream* native_stream = (union_native_stream*)malloc(sizeof(union_native_stream));
    if (native_stream == NULL)
        return ABSTR_DECOMPRESS_Z_MEM_ERROR;
    memset(native_stream,0,sizeof(union_native_stream));
    strm->p_native_stream = (void*)native_stream;

	ClearZLibDecompressStream(&native_stream->zlib_stream);

	strm->contain_prefix=0;
	strm->is_prefix_read=0;
	strm->size_prefix=0;

	strm->fnc_decompress = zlib_abstract_decompress;
	strm->fnc_decompress_end = zlib_abstract_decompress_end;

	retValue = inflateInit2(&native_stream->zlib_stream, -MAX_WBITS);
	return retValue;
}

/*
int abstract_init_raw_store(abstract_decompress_stream* strm)
{
	int retValue;
	ClearZLibDecompressStream(&native_stream->zlib_stream);


	strm->fnc_decompress = zlib_store_abstract_decompress;
	strm->fnc_decompress_end = zlib_store_abstract_decompress_end;

	retValue = ABSTR_DECOMPRESS_Z_OK;
	return retValue;
}*/

int abstract_init_prefix(abstract_decompress_stream* strm)
{
	int retValue;

    union_native_stream* native_stream = (union_native_stream*)malloc(sizeof(union_native_stream));
    if (native_stream == NULL)
        return ABSTR_DECOMPRESS_Z_MEM_ERROR;
    memset(native_stream,0,sizeof(union_native_stream));
    strm->p_native_stream = (void*)native_stream;

	ClearZLibDecompressStream(&native_stream->zlib_stream);

	strm->contain_prefix=1;
	strm->size_prefix=1; // filled after
	strm->is_prefix_read=0;

	strm->fnc_decompress = NULL;
	strm->fnc_decompress_end = NULL;

	retValue = ABSTR_DECOMPRESS_Z_OK;


	return retValue;
}


#ifdef HAVE_FAST_LZLIB
static int configureDecompressFastZlib(abstract_decompress_stream* p_strm, zfast_stream_compressor compressor, int block_size)
{
	union_native_stream* native_stream = (union_native_stream*)(p_strm->p_native_stream);
	int retValue;

		p_strm->fnc_decompress = fast_zlib_abstract_decompress;
		p_strm->fnc_decompress_end = fast_zlib_abstract_decompress_end;

		ClearFastZlibDecompressStream(&(native_stream->zlib_stream));
		retValue = fastlzlibDecompressInit2(&(native_stream->zlib_stream), block_size);
		if (retValue != Z_OK)
			return ABSTR_DECOMPRESS_Z_MEM_ERROR;

		if (fastlzlibSetCompressor(&native_stream->fastzlib_stream, compressor) != Z_OK)
			retValue = ABSTR_DECOMPRESS_Z_MEM_ERROR;

		return ABSTR_DECOMPRESS_Z_OK;
}
#endif


int abstract_decompress (abstract_decompress_stream* p_strm, int flush)
{
	unsigned char prefix;
	int retValue = ABSTR_DECOMPRESS_Z_OK;

	union_native_stream* native_stream = (union_native_stream*)(p_strm->p_native_stream);

	if ((p_strm->contain_prefix == 0) || (p_strm->is_prefix_read != 0))
		return (((p_strm)->fnc_decompress)((p_strm),(flush)));
	if (p_strm->avail_in == 0)
		return ABSTR_DECOMPRESS_Z_OK;

	p_strm->is_prefix_read = 1;
	prefix = *(p_strm->next_in);

  /*
  // see https://www.ietf.org/rfc/rfc1950.txt section 2.2 - Data format
  // 0x78 mean zlib with 32k window size
  // 0x68 mean zlib with 16k window size
  // 0x58 mean zlib with 8k window size
  // 0x48 mean zlib with 4k window size
  // 0x38 mean zlib with 2k window size
  // 0x28 mean zlib with 1k window size in zlib but not accepted by Abstract decompress, used as zstd header
  //  (and zlib stream with less than 2k window size don't exist in real life)
  */
#ifdef UNCOMPRESS_ZSTD
  if (((prefix & 0x0f) == 8) && (prefix != ABSTRACT_CPR_PREFIX_STANDARD_ZSTD))
#else
  if ((prefix & 0x0f) == 8)
#endif
	{
		p_strm->fnc_decompress = zlib_abstract_decompress;
		p_strm->fnc_decompress_end = zlib_abstract_decompress_end;

		ClearZLibDecompressStream(&(native_stream->zlib_stream));
		retValue = inflateInit(&(native_stream->zlib_stream));
		if (retValue != Z_OK)
			return ABSTR_DECOMPRESS_Z_MEM_ERROR;
		return (((p_strm)->fnc_decompress)((p_strm),(flush)));
	}

	if (prefix == ABSTRACT_CPR_PREFIX_GZIP)
	{
		p_strm->fnc_decompress = zlib_abstract_decompress;
		p_strm->fnc_decompress_end = zlib_abstract_decompress_end;

		ClearZLibDecompressStream(&(native_stream->zlib_stream));
		retValue = inflateInit2(&(native_stream->zlib_stream), 47);
		if (retValue != Z_OK)
			return ABSTR_DECOMPRESS_Z_MEM_ERROR;
		return (((p_strm)->fnc_decompress)((p_strm),(flush)));
	}

	/* ABSTRACT_CPR_PREFIX_XZ is the normal, standard XZ prefix. Do not consume it! */
	/* ABSTRACT_CPR_PREFIX_BZIP2 is the normal, standard BZIP2 prefix. Do not consume it! */
	if ((prefix != ABSTRACT_CPR_PREFIX_STANDARD_XZ) && (prefix != ABSTRACT_CPR_PREFIX_STANDARD_BZIP2) &&
		(prefix != ABSTRACT_CPR_PREFIX_STANDARD_LZ4) && (prefix != ABSTRACT_CPR_PREFIX_STANDARD_ZSTD))
	{
		p_strm->size_prefix = 1;

		p_strm->next_in++;
		p_strm->total_in++;
		p_strm->avail_in--;
	}

	switch (prefix)
	{
	case ABSTRACT_CPR_PREFIX_STORE:
		p_strm->fnc_decompress = zlib_store_abstract_decompress;
		p_strm->fnc_decompress_end = zlib_store_abstract_decompress_end;
		ClearZLibDecompressStream(&(native_stream->zlib_stream));
		break;

	case ABSTRACT_CPR_PREFIX_DEFLATE:
		p_strm->fnc_decompress = zlib_abstract_decompress;
		p_strm->fnc_decompress_end = zlib_abstract_decompress_end;

		ClearZLibDecompressStream(&(native_stream->zlib_stream));
		retValue = inflateInit2(&(native_stream->zlib_stream), -MAX_WBITS);
		if (retValue != Z_OK)
			return ABSTR_DECOMPRESS_Z_MEM_ERROR;
		break;

#ifdef HAVE_FAST_LZLIB
	case ABSTRACT_CPR_PREFIX_FASTZL_LOW_BLOCKSIZE:
		if (configureDecompressFastZlib(p_strm, COMPRESSOR_FASTLZ, FASTZLIB_LOW_BLOCKSIZE) != ABSTR_DECOMPRESS_Z_OK)
			return ABSTR_DECOMPRESS_Z_MEM_ERROR;
		break;

	case ABSTRACT_CPR_PREFIX_FASTZL_DEFAULT_BLOCKSIZE:
		if (configureDecompressFastZlib(p_strm, COMPRESSOR_FASTLZ, FASTZLIB_DEFAULT_BLOCKSIZE) != ABSTR_DECOMPRESS_Z_OK)
			return ABSTR_DECOMPRESS_Z_MEM_ERROR;
		break;

	case ABSTRACT_CPR_PREFIX_FASTZL_HIGH_BLOCKSIZE:
		if (configureDecompressFastZlib(p_strm, COMPRESSOR_FASTLZ, FASTZLIB_HIGH_BLOCKSIZE) != ABSTR_DECOMPRESS_Z_OK)
			return ABSTR_DECOMPRESS_Z_MEM_ERROR;
		break;


	case ABSTRACT_CPR_PREFIX_LZ4_LOW_BLOCKSIZE:
		if (configureDecompressFastZlib(p_strm, COMPRESSOR_LZ4, FASTZLIB_LOW_BLOCKSIZE) != ABSTR_DECOMPRESS_Z_OK)
			return ABSTR_DECOMPRESS_Z_MEM_ERROR;
		break;

	case ABSTRACT_CPR_PREFIX_LZ4_DEFAULT_BLOCKSIZE:
		if (configureDecompressFastZlib(p_strm, COMPRESSOR_LZ4, FASTZLIB_DEFAULT_BLOCKSIZE) != ABSTR_DECOMPRESS_Z_OK)
			return ABSTR_DECOMPRESS_Z_MEM_ERROR;
		break;

	case ABSTRACT_CPR_PREFIX_LZ4_HIGH_BLOCKSIZE:
		if (configureDecompressFastZlib(p_strm, COMPRESSOR_LZ4, FASTZLIB_HIGH_BLOCKSIZE) != ABSTR_DECOMPRESS_Z_OK)
			return ABSTR_DECOMPRESS_Z_MEM_ERROR;
		break;

#if defined(COMPRESSOR_LZFSE) || defined(ZFAST_USE_LZFSE)
  case ABSTRACT_CPR_PREFIX_LZFSE_LOW_BLOCKSIZE:
    if (configureDecompressFastZlib(p_strm, COMPRESSOR_LZFSE, FASTZLIB_LOW_BLOCKSIZE) != ABSTR_DECOMPRESS_Z_OK)
      return ABSTR_DECOMPRESS_Z_MEM_ERROR;
    break;

  case ABSTRACT_CPR_PREFIX_LZFSE_DEFAULT_BLOCKSIZE:
    if (configureDecompressFastZlib(p_strm, COMPRESSOR_LZFSE, FASTZLIB_DEFAULT_BLOCKSIZE) != ABSTR_DECOMPRESS_Z_OK)
      return ABSTR_DECOMPRESS_Z_MEM_ERROR;
    break;

  case ABSTRACT_CPR_PREFIX_LZFSE_HIGH_BLOCKSIZE:
    if (configureDecompressFastZlib(p_strm, COMPRESSOR_LZFSE, FASTZLIB_HIGH_BLOCKSIZE) != ABSTR_DECOMPRESS_Z_OK)
      return ABSTR_DECOMPRESS_Z_MEM_ERROR;
    break;
#endif
#endif

#ifdef UNCOMPRESS_BZIP2
	case ABSTRACT_CPR_PREFIX_STANDARD_BZIP2:
		p_strm->fnc_decompress = bzip2_abstract_decompress;
		p_strm->fnc_decompress_end = bzip2_abstract_decompress_end;

		ClearBZip2DecompressStream(&(native_stream->bzip2_stream));
		retValue = BZ2_bzDecompressInit(&(native_stream->bzip2_stream), 0, 0);
		if (retValue != Z_OK)
			return ABSTR_DECOMPRESS_Z_MEM_ERROR;
		break;
#endif


#if defined(UNCOMPRESS_LZHAM) || defined(HAVE_LZHAM)
  //case ABSTRACT_CPR_PREFIX_STANDARD_LZHAM:


  case ABSTRACT_CPR_PREFIX_LZHAM_DICT_15:
  case ABSTRACT_CPR_PREFIX_LZHAM_DICT_16:
  case ABSTRACT_CPR_PREFIX_LZHAM_DICT_17:
  case ABSTRACT_CPR_PREFIX_LZHAM_DICT_18:
  case ABSTRACT_CPR_PREFIX_LZHAM_DICT_19:
  case ABSTRACT_CPR_PREFIX_LZHAM_DICT_20:
  case ABSTRACT_CPR_PREFIX_LZHAM_DICT_21:
  case ABSTRACT_CPR_PREFIX_LZHAM_DICT_22:
  case ABSTRACT_CPR_PREFIX_LZHAM_DICT_23:
  case ABSTRACT_CPR_PREFIX_LZHAM_DICT_24:
  case ABSTRACT_CPR_PREFIX_LZHAM_DICT_25:
  case ABSTRACT_CPR_PREFIX_LZHAM_DICT_26:
  case ABSTRACT_CPR_PREFIX_LZHAM_DICT_27:
  case ABSTRACT_CPR_PREFIX_LZHAM_DICT_28:
  case ABSTRACT_CPR_PREFIX_LZHAM_DICT_29:

    p_strm->fnc_decompress = lzham_abstract_decompress;
    p_strm->fnc_decompress_end = lzham_abstract_decompress_end;

    ClearLzHamDecompressStream(&(native_stream->lzham_z_stream));
    //retValue = lzham_z_inflateInit(&(native_stream->lzham_z_stream));
    retValue = lzham_z_inflateInit2(&(native_stream->lzham_z_stream),(prefix + (int)15) - ABSTRACT_CPR_PREFIX_LZHAM_DICT_15);
    if (retValue != Z_OK)
      return ABSTR_DECOMPRESS_Z_MEM_ERROR;
    break;
#endif

#ifdef UNCOMPRESS_LZ4
  case ABSTRACT_CPR_PREFIX_STANDARD_LZ4:
  //case ABSTRACT_CPR_PREFIX_LZ4:
    p_strm->fnc_decompress = lz4_abstract_decompress;
    p_strm->fnc_decompress_end = lz4_abstract_decompress_end;

    ClearLz4DecompressStream(&(native_stream->zlib_stream));
    retValue = lz4DecompressInit(&(native_stream->zlib_stream));
    if (retValue != Z_OK)
      return ABSTR_DECOMPRESS_Z_MEM_ERROR;
    break;
#endif

#ifdef UNCOMPRESS_ZSTD
  case ABSTRACT_CPR_PREFIX_STANDARD_ZSTD:
  case ABSTRACT_CPR_PREFIX_ZSTD:
    p_strm->fnc_decompress = zstd_abstract_decompress;
    p_strm->fnc_decompress_end = zstd_abstract_decompress_end;

    ClearZstdDecompressStream(&(native_stream->zlib_stream));
    retValue = zstdDecompressInit(&(native_stream->zlib_stream));
    if (retValue != Z_OK)
      return ABSTR_DECOMPRESS_Z_MEM_ERROR;
    break;
#endif

#ifdef UNCOMPRESS_LZMASDK
	case ABSTRACT_CPR_XZUTILS:
	case ABSTRACT_CPR_PREFIX_STANDARD_XZ:
	case ABSTRACT_CPR_XZUTILS_XZ_EMBEDDED:

		ClearLzmaSdkUncompressStream(&(native_stream->lzma_sdk_xz_stream));
		p_strm->end_reached = 0;

		p_strm->fnc_decompress = lzmasdk_abstract_decompress;
		p_strm->fnc_decompress_end = lzmasdk_abstract_decompress_end;

		break;

#else

#ifdef ABSTRACT_CPR_XZUTILS_DYNAMIC_AND_UNCOMPRESS_XZEMBEDDED

	case ABSTRACT_CPR_XZUTILS:
	case ABSTRACT_CPR_PREFIX_STANDARD_XZ:
	case ABSTRACT_CPR_XZUTILS_XZ_EMBEDDED:
		if (dynamic_lzma_lib_load() != 0)
		{
			ClearXzUtilsUncompressStream(&(native_stream->xzutils_stream));
			p_strm->fnc_decompress = xzutils_abstract_decompress;
			p_strm->fnc_decompress_end = xzutils_abstract_decompress_end;

			if (dynamic_lzma_stream_decoder(&(native_stream->xzutils_stream),UINT64_MAX,LZMA_TELL_NO_CHECK) != LZMA_OK)
				return ABSTR_DECOMPRESS_Z_MEM_ERROR;
		}
		else
		{
			ClearXZEmbeddedDecompressStream(&(native_stream->xz_embedded_stream));
			native_stream->xz_embedded_stream.dec = xz_dec_init(XZ_DYNALLOC, 1 << 26);
			if (native_stream->xz_embedded_stream.dec == NULL)
				return ABSTR_DECOMPRESS_Z_MEM_ERROR;

			p_strm->fnc_decompress = xz_embedded_abstract_decompress;
			p_strm->fnc_decompress_end = xz_embedded_abstract_decompress_end;
		}
		break;
#else
#if defined(UNCOMPRESS_XZUTILS) && (!defined(UNCOMPRESS_LZMASDK))

	case ABSTRACT_CPR_XZUTILS:
	case ABSTRACT_CPR_PREFIX_STANDARD_XZ:
	case ABSTRACT_CPR_XZUTILS_XZ_EMBEDDED:
		ClearXzUtilsUncompressStream(&(native_stream->xzutils_stream));
		p_strm->fnc_decompress = xzutils_abstract_decompress;
		p_strm->fnc_decompress_end = xzutils_abstract_decompress_end;

		if (lzma_stream_decoder(&(native_stream->xzutils_stream), UINT64_MAX, LZMA_TELL_NO_CHECK) != LZMA_OK)
			return ABSTR_DECOMPRESS_Z_MEM_ERROR;
		break;
#endif


#if defined(UNCOMPRESS_XZEMBEDDED) && (!defined(UNCOMPRESS_XZUTILS))
//#ifdef Z_OK
	case ABSTRACT_CPR_PREFIX_STANDARD_XZ:
	case ABSTRACT_CPR_XZUTILS_XZ_EMBEDDED:
		ClearXZEmbeddedDecompressStream(&(native_stream->xz_embedded_stream));
		native_stream->xz_embedded_stream.dec = xz_dec_init(XZ_DYNALLOC, 1 << 26);
		if (native_stream->xz_embedded_stream.dec == NULL)
			return ABSTR_DECOMPRESS_Z_MEM_ERROR;

		p_strm->fnc_decompress = xz_embedded_abstract_decompress;
		p_strm->fnc_decompress_end = xz_embedded_abstract_decompress_end;
		break;

#endif
#endif
#endif


	default:
		p_strm->fnc_decompress = error_abstract_decompress;
		p_strm->fnc_decompress_end = error_abstract_decompress_end;
		return ABSTR_DECOMPRESS_Z_ERRNO ;
	}

	return (((p_strm)->fnc_decompress)((p_strm),(flush)));
}

int abstract_decompress_end (abstract_decompress_stream* p_strm)
{
	int ret;
	union_native_stream* native_stream = (union_native_stream*)p_strm->p_native_stream;

	if ((p_strm->contain_prefix == 0) || (p_strm->is_prefix_read != 0))
	{
		if (((p_strm)->fnc_decompress_end) != NULL)
			ret = (((p_strm)->fnc_decompress_end)((p_strm)));
		else
			ret = ABSTR_DECOMPRESS_Z_ERRNO;
	}
	else
		ret = ABSTR_DECOMPRESS_Z_ERRNO;

	if (native_stream != NULL)
		free(native_stream);
	return ret;
}




int XflateStore_decompress(abstract_decompress_stream* strm, int flush)
{
      int retErr = Z_OK;
      uLong dwDoThis;
      if ((strm->avail_in) < (strm->avail_out))
          dwDoThis = strm->avail_in;
      else
          dwDoThis = strm->avail_out;

      //memcpy(strm->next_out,strm->next_in,dwDoThis);
      {
          const unsigned char* src;
          unsigned char* dst;
          uLong dwToCopy = dwDoThis;
          dst = strm->next_out;
          src = strm->next_in;
          /*
          while (dwToCopy>0)
          {
              *dst = *src;
              dst++;
              src++;
              dwToCopy--;
          }*/
          memcpy(dst,src,dwToCopy);
      }
      strm->avail_in  -= dwDoThis;
      strm->avail_out -= dwDoThis;
      strm->total_in  += dwDoThis;
      strm->total_out += dwDoThis;

      strm->next_out += dwDoThis;
      strm->next_in += dwDoThis;
      if ((flush==Z_FINISH) && (strm->avail_in==0))
          retErr = Z_STREAM_END;
      return retErr;
}

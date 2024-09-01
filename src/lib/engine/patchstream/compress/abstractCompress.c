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
#include <stdio.h>
#include "zlib.h"
#include "../common/compress_store.h"


#include "../common/difbasic.h"
#include "abstractCompress.h"
#include "../common/abstractCompressDecompressCommon.h"

#ifndef DEF_MEM_LEVEL
#if MAX_MEM_LEVEL >= 8
#  define DEF_MEM_LEVEL 8
#else
#  define DEF_MEM_LEVEL  MAX_MEM_LEVEL
#endif
#endif

#ifdef COMPRESS_LZ4
typedef struct
{
	LZ4F_compressionContext_t ctx;
	unsigned char* flush_buf;
	unsigned char* read_buf;
	size_t pos_read_buf;
	size_t max_input_size;
	size_t max_flush_size_data;
	size_t max_flush_size_end_or_flush;
	size_t current_flush_size;
	size_t pos_in_flush_size;
	/* event_after_flush:
	  0 : for header or normal compression
	  ABSTR_COMPRESS_Z_STREAM_END : end of flush requested by user
	  ABSTR_COMPRESS_Z_STREAM_END : end of file */
	int event_after_flush;
} lz4_compress_strm;
#endif

typedef union
{
	z_stream zlib_stream;
#ifdef COMPRESS_BZIP2
	bz_stream bzip2_stream;
#endif

#ifdef COMPRESS_LZ4
	lz4_compress_strm lz4_z_stream;
#endif

#ifdef COMPRESS_LZHAM
	lzham_z_stream lzham_z_stream;
#endif

#ifdef HAVE_FAST_LZLIB
	zfast_stream fastzlib_stream;
#endif

#ifdef COMPRESS_XZUTILS
	struct {
		lzma_stream xzutils_stream;
		int mt;
	} xzut;
#endif
} union_native_compress_stream;

static void ClearZLibCompressStream(
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




void clear_abstract_compress_stream(
     abstract_compress_stream* strm)
{
	strm->avail_in = 0 ;
	strm->total_in = 0 ;
	strm->next_in = NULL ;

	strm->avail_out = 0 ;
	strm->total_out = 0 ;
	strm->next_out = NULL ;

	strm->fnc_compress = NULL;
	strm->fnc_compress_end = NULL;
}

int zlib_abstract_compress(struct abstract_compress_stream_s* strm,int flush)
{
	int retValue;
	union_native_compress_stream* native_stream = (union_native_compress_stream*)strm->p_native_stream;
	native_stream->zlib_stream.avail_in = strm->avail_in;
	native_stream->zlib_stream.total_in = (uLong)strm->total_in;
	native_stream->zlib_stream.next_in = strm->next_in;

	native_stream->zlib_stream.avail_out = strm->avail_out;
	native_stream->zlib_stream.total_out = (uLong)(strm->total_out - strm->size_prefix);
	native_stream->zlib_stream.next_out = strm->next_out;

	retValue = deflate(&native_stream->zlib_stream, flush);

	strm->avail_in =native_stream->zlib_stream.avail_in ;
	strm->total_in = native_stream->zlib_stream.total_in;
	strm->next_in = native_stream->zlib_stream.next_in ;

	strm->avail_out = native_stream->zlib_stream.avail_out ;
	strm->total_out = native_stream->zlib_stream.total_out + strm->size_prefix;
	strm->next_out = native_stream->zlib_stream.next_out ;

	return retValue;
}


static int zlib_abstract_compress_end(struct abstract_compress_stream_s* strm)
{
	int retValue;
	union_native_compress_stream* native_stream = (union_native_compress_stream*)strm->p_native_stream;
	native_stream->zlib_stream.avail_in = strm->avail_in;
	native_stream->zlib_stream.total_in = (uLong)strm->total_in;
	native_stream->zlib_stream.next_in = strm->next_in;

	native_stream->zlib_stream.avail_out = strm->avail_out;
	native_stream->zlib_stream.total_out = (uLong)(strm->total_out - strm->size_prefix);
	native_stream->zlib_stream.next_out = strm->next_out;

	retValue = deflateEnd(&native_stream->zlib_stream);

	strm->avail_in =native_stream->zlib_stream.avail_in ;
	strm->total_in = native_stream->zlib_stream.total_in ;
	strm->next_in = native_stream->zlib_stream.next_in ;

	strm->avail_out = native_stream->zlib_stream.avail_out ;
	strm->total_out = native_stream->zlib_stream.total_out + strm->size_prefix ;
	strm->next_out = native_stream->zlib_stream.next_out ;

	return retValue;
}




#ifdef COMPRESS_LZHAM

static void ClearLzHamCompressStream(lzham_z_stream* strm)
{
  memset(strm, 0, sizeof(lzham_z_stream));
}

static int LzHam_abstract_compress(struct abstract_compress_stream_s* strm, int flush)
{
  int retValue;
  union_native_compress_stream* native_stream = (union_native_compress_stream*)strm->p_native_stream;
  /*
  #define ABSTR_COMPRESS_Z_NO_FLUSH      0
  #define ABSTR_COMPRESS_Z_SYNC_FLUSH    2
  #define ABSTR_COMPRESS_Z_FINISH        4
  */
  int flushbz = LZHAM_Z_NO_FLUSH;
  if (flush == ABSTR_COMPRESS_Z_SYNC_FLUSH)
    flushbz = LZHAM_Z_SYNC_FLUSH;
  if (flush == ABSTR_COMPRESS_Z_FINISH)
    flushbz = LZHAM_Z_FINISH;

  native_stream->lzham_z_stream.avail_in = strm->avail_in;
  native_stream->lzham_z_stream.total_in = (uLong)strm->total_in;
  native_stream->lzham_z_stream.next_in = strm->next_in;

  native_stream->lzham_z_stream.avail_out = strm->avail_out;
  native_stream->lzham_z_stream.total_out = (uLong)(strm->total_out - strm->size_prefix);
  native_stream->lzham_z_stream.next_out = strm->next_out;

  retValue = lzham_z_deflate(&native_stream->lzham_z_stream, flushbz);

  strm->avail_in = native_stream->lzham_z_stream.avail_in;
  strm->total_in = native_stream->lzham_z_stream.total_in;
  strm->next_in = (Bytef*)native_stream->lzham_z_stream.next_in;

  strm->avail_out = native_stream->lzham_z_stream.avail_out;
  strm->total_out = native_stream->lzham_z_stream.total_out + strm->size_prefix;
  strm->next_out = native_stream->lzham_z_stream.next_out;

  return retValue;
}


static int LzHam_abstract_compress_end(struct abstract_compress_stream_s* strm)
{
  int retValue;

  union_native_compress_stream* native_stream = (union_native_compress_stream*)strm->p_native_stream;
  native_stream->lzham_z_stream.avail_in = strm->avail_in;
  native_stream->lzham_z_stream.total_in = (uLong)strm->total_in;
  native_stream->lzham_z_stream.next_in = strm->next_in;

  native_stream->lzham_z_stream.avail_out = strm->avail_out;
  native_stream->lzham_z_stream.total_out = (uLong)(strm->total_out - strm->size_prefix);
  native_stream->lzham_z_stream.next_out = strm->next_out;

  retValue = lzham_z_deflateEnd(&native_stream->lzham_z_stream);

  strm->avail_in = native_stream->lzham_z_stream.avail_in;
  strm->total_in = native_stream->lzham_z_stream.total_in;
  strm->next_in = (Bytef*)native_stream->lzham_z_stream.next_in;

  strm->avail_out = native_stream->lzham_z_stream.avail_out;
  strm->total_out = native_stream->lzham_z_stream.total_out + strm->size_prefix;
  strm->next_out = native_stream->lzham_z_stream.next_out;

  return retValue;
}
#endif

#ifdef COMPRESS_LZ4




/*


static int zstd_abstract_compress(struct abstract_compress_stream_s* strm, int flush)
{
  int retValue = Z_OK;
  union_native_compress_stream* native_stream = (union_native_compress_stream*)strm->p_native_stream;

  native_stream->zlib_stream.avail_in = strm->avail_in;
  native_stream->zlib_stream.total_in = (uLong)strm->total_in;
  native_stream->zlib_stream.next_in = strm->next_in;

  native_stream->zlib_stream.avail_out = strm->avail_out;
  native_stream->zlib_stream.total_out = (uLong)(strm->total_out - strm->size_prefix);
  native_stream->zlib_stream.next_out = strm->next_out;

  retValue = zstd_zlib_mini_wrap_clone_deflate(&native_stream->zlib_stream, flush);

  strm->avail_in = native_stream->zlib_stream.avail_in;
  strm->total_in = native_stream->zlib_stream.total_in;
  strm->next_in = native_stream->zlib_stream.next_in;

  strm->avail_out = native_stream->zlib_stream.avail_out;
  strm->total_out = native_stream->zlib_stream.total_out + strm->size_prefix;
  strm->next_out = native_stream->zlib_stream.next_out;

  return retValue;
}*/

/* flush the buffer in strm out. return 1 if we have flushed data AND terminated the flush stream */
static int lz4_flushbuf(struct abstract_compress_stream_s* strm, lz4_compress_strm* lz4_z_stream)
{
	if ((strm->avail_out>0) && (lz4_z_stream->pos_in_flush_size<lz4_z_stream->current_flush_size)) {
		size_t size_out_this = lz4_z_stream->current_flush_size - lz4_z_stream->pos_in_flush_size;
		if (size_out_this > strm->avail_out)
			size_out_this = strm->avail_out;
		memcpy(strm->next_out, lz4_z_stream->flush_buf + lz4_z_stream->pos_in_flush_size, size_out_this);
		strm->avail_out -= (uInt)size_out_this;
		strm->next_out += size_out_this;
		strm->total_out += (dfuLong64)size_out_this;
		lz4_z_stream->pos_in_flush_size += size_out_this;

		if (lz4_z_stream->pos_in_flush_size == lz4_z_stream->current_flush_size) {
			lz4_z_stream->pos_in_flush_size = lz4_z_stream->current_flush_size = 0;
			return lz4_z_stream->event_after_flush;
		}
    }
	return 0;
}

static int lz4_abstract_compress(struct abstract_compress_stream_s* strm, int flush)
{
	union_native_compress_stream* native_stream = (union_native_compress_stream*)strm->p_native_stream;
	LZ4F_compressOptions_t compressOptions;
	lz4_compress_strm* lz4_z_stream=&native_stream->lz4_z_stream;
	int flush_result=lz4_flushbuf(strm, lz4_z_stream);
	if (flush_result) {
		return flush_result;
	}

	/* if there is data to flush and no more space in out, we need more output space and return */
	if ((strm->avail_out==0) && (lz4_z_stream->pos_in_flush_size<lz4_z_stream->current_flush_size)) {
		return ABSTR_COMPRESS_Z_OK;
	}

	/* now we known flush buffer is empty */
	lz4_z_stream->event_after_flush=0;

	memset(&compressOptions, 0, sizeof(LZ4F_compressOptions_t));
	while ((strm->avail_out>0) && ((strm->avail_in>0) ||
		                           ((flush != ABSTR_COMPRESS_Z_NO_FLUSH) && (lz4_z_stream->pos_read_buf > 0))))
	{
		int write_directly_out=(strm->avail_out>=lz4_z_stream->max_flush_size_data);
		size_t size_in_this;
		void* dst_buffer= write_directly_out ? strm->next_out : lz4_z_stream->flush_buf;
		size_t dst_buffer_size = write_directly_out ? strm->avail_out : lz4_z_stream->max_flush_size_data;
		const void* src_buffer;

		int read_directly_in=((lz4_z_stream->pos_read_buf==0) &&
			                  ((strm->avail_in >= lz4_z_stream->max_input_size) || (flush != ABSTR_COMPRESS_Z_NO_FLUSH)));

		if (!read_directly_in) {
			size_t size_copy_read_to_buf=(lz4_z_stream->max_input_size-lz4_z_stream->pos_read_buf);
			if (size_copy_read_to_buf>strm->avail_in) size_copy_read_to_buf=strm->avail_in;
			memcpy(lz4_z_stream->read_buf+lz4_z_stream->pos_read_buf,strm->next_in,size_copy_read_to_buf);
			lz4_z_stream->pos_read_buf+=size_copy_read_to_buf;

			strm->avail_in -= (uInt)size_copy_read_to_buf;
			strm->next_in += size_copy_read_to_buf;
			strm->total_in += size_copy_read_to_buf;

			src_buffer=lz4_z_stream->read_buf;
			size_in_this=lz4_z_stream->pos_read_buf;
		} else {
			src_buffer = strm->next_in;
			size_in_this = (strm->avail_in < lz4_z_stream->max_input_size) ? strm->avail_in : lz4_z_stream->max_input_size;
		}

		if ((flush == ABSTR_COMPRESS_Z_NO_FLUSH) && (size_in_this < lz4_z_stream->max_input_size))
			return ABSTR_COMPRESS_Z_OK;

		size_t outSize = (size_in_this==0) ? 0 :
			LZ4F_compressUpdate(lz4_z_stream->ctx,
			                    dst_buffer, dst_buffer_size,
			                    src_buffer, size_in_this, &compressOptions);

		lz4_z_stream->pos_read_buf=0;
		if (read_directly_in) {
			strm->avail_in-=(uInt)size_in_this;
			strm->next_in+=size_in_this;
			strm->total_in+=size_in_this;
		}

		if (write_directly_out) {
			strm->avail_out-=(uInt)outSize;
			strm->next_out+=outSize;
			strm->total_out+=outSize;
		} else {
			lz4_z_stream->current_flush_size+=outSize;
			lz4_flushbuf(strm, lz4_z_stream);
		}
	}

	if ((flush==ABSTR_COMPRESS_Z_NO_FLUSH) || (strm->avail_out==0))
		return ABSTR_COMPRESS_Z_OK;

	if (strm->avail_in>0)
		return ABSTR_COMPRESS_Z_OK;

	/* now we want flush, there is space in avail_out and flush_buf is empty */
	{
		int write_directly_out = (strm->avail_out >= lz4_z_stream->max_flush_size_end_or_flush);
		void* dst_buffer = write_directly_out ? strm->next_out : lz4_z_stream->flush_buf;
		size_t dst_buffer_size = write_directly_out ? strm->avail_out : lz4_z_stream->max_flush_size_end_or_flush;
		size_t outSize= (flush == ABSTR_COMPRESS_Z_SYNC_FLUSH) ?
			LZ4F_flush(lz4_z_stream->ctx, dst_buffer, dst_buffer_size,&compressOptions) :
			LZ4F_compressEnd(lz4_z_stream->ctx, dst_buffer, dst_buffer_size, &compressOptions);

		if (write_directly_out) {
			strm->avail_out -= (uInt)outSize;
			strm->next_out += outSize;
			strm->total_out += outSize;
			return ABSTR_COMPRESS_Z_STREAM_END;
		}
		else {
			lz4_z_stream->event_after_flush=ABSTR_COMPRESS_Z_STREAM_END;
			lz4_z_stream->current_flush_size+=outSize;
			return lz4_flushbuf(strm, lz4_z_stream);
		}
	}
}


static int lz4_abstract_compress_end(struct abstract_compress_stream_s* strm)
{
	union_native_compress_stream* native_stream = (union_native_compress_stream*)strm->p_native_stream;
	lz4_compress_strm* lz4_z_stream = &native_stream->lz4_z_stream;

	LZ4F_freeCompressionContext(lz4_z_stream->ctx);
	free(lz4_z_stream->read_buf);

	return ABSTR_COMPRESS_Z_OK;
}

/* FIO_LZ4_GetBlockSize_FromBlockId from zstd programs/fileio.c */
static int FIO_LZ4_GetBlockSize_FromBlockId(int id) { return (1 << (8 + (2 * id))); }

static int abstract_init_compress_lz4_complevel(abstract_compress_stream* strm, int level,int nbbits,unsigned favorDecSpeed)
{
	lz4_compress_strm* lz4_z_stream;
	LZ4F_preferences_t prefs;
	union_native_compress_stream* native_stream = (union_native_compress_stream*)malloc(sizeof(union_native_compress_stream));
	if (native_stream == NULL)
		return ABSTR_COMPRESS_Z_ERRNO;
	memset(native_stream, 0, sizeof(union_native_compress_stream));
	lz4_z_stream=&native_stream->lz4_z_stream;

	memset(&prefs, 0, sizeof(LZ4F_preferences_t));
	prefs.frameInfo.blockSizeID = LZ4F_max64KB;
	if (nbbits>=18) prefs.frameInfo.blockSizeID = LZ4F_max256KB;
	if (nbbits>=20) prefs.frameInfo.blockSizeID = LZ4F_max1MB;
	if (nbbits>=22) prefs.frameInfo.blockSizeID = LZ4F_max4MB;
	prefs.favorDecSpeed=favorDecSpeed;

	prefs.autoFlush = 1;
	prefs.compressionLevel = level;
	prefs.frameInfo.blockMode = LZ4F_blockLinked;
	prefs.frameInfo.contentChecksumFlag = 0;
#if LZ4_VERSION_NUMBER >= 10600
	prefs.frameInfo.contentSize = 0;
#endif
	lz4_z_stream->max_input_size=FIO_LZ4_GetBlockSize_FromBlockId(prefs.frameInfo.blockSizeID);
	lz4_z_stream->max_flush_size_data=LZ4F_compressBound(lz4_z_stream->max_input_size, &prefs);
	lz4_z_stream->max_flush_size_end_or_flush=LZ4F_compressBound(0, &prefs);
	if (lz4_z_stream->max_flush_size_data<lz4_z_stream->max_flush_size_end_or_flush) lz4_z_stream->max_flush_size_data=lz4_z_stream->max_flush_size_end_or_flush;
	if (lz4_z_stream->max_flush_size_data<LZ4F_HEADER_SIZE_MAX) lz4_z_stream->max_flush_size_data=LZ4F_HEADER_SIZE_MAX;
	lz4_z_stream->read_buf=(unsigned char*)malloc(lz4_z_stream->max_input_size+lz4_z_stream->max_flush_size_data);
	if (lz4_z_stream->read_buf==NULL) {
		free(native_stream);
		return ABSTR_COMPRESS_Z_ERRNO;
	}
	if (LZ4F_isError(LZ4F_createCompressionContext(&lz4_z_stream->ctx, LZ4F_VERSION))) {
		free(lz4_z_stream->read_buf);
		free(native_stream);
		return ABSTR_COMPRESS_Z_ERRNO;
	}
	lz4_z_stream->flush_buf=lz4_z_stream->read_buf+lz4_z_stream->max_input_size;
	lz4_z_stream->pos_read_buf=0;
	strm->contain_prefix = 0;
	strm->is_prefix_write = 1;
	strm->size_prefix = 0;
	strm->prefix_value = 0;

	strm->p_native_stream = (void*)native_stream;
	lz4_z_stream->pos_in_flush_size=0;
	lz4_z_stream->event_after_flush=0;
	lz4_z_stream->current_flush_size=LZ4F_compressBegin(lz4_z_stream->ctx, lz4_z_stream->flush_buf, lz4_z_stream->max_flush_size_data, &prefs);

	strm->fnc_compress = lz4_abstract_compress;
	strm->fnc_compress_end = lz4_abstract_compress_end;

	return ABSTR_COMPRESS_Z_OK;
}
#endif

#ifdef COMPRESS_ZSTD
#define COMPRESS_ZSTD_MT
#endif


#ifdef COMPRESS_ZSTD_MT

static void ClearZstdMtZlibWrapCompressStream(z_stream* strm)
{
  memset(strm, 0, sizeof(z_stream));
}


static int zstd_mt_zlib_mini_wrap_clone_deflate (z_streamp strm, int flush)
{
  ZSTD_CCtx* zwc;
  ZSTD_inBuffer in;
  ZSTD_outBuffer out;
  size_t return_code;

  zwc = (ZSTD_CCtx*)strm->state;

  if ((flush == ABSTR_COMPRESS_Z_NO_FLUSH) || (strm->avail_in > 0))
  {
    in.pos = 0;
    in.src = strm->next_in;
    in.size = strm->avail_in;

    out.pos = 0;
    out.dst = strm->next_out;
    out.size = strm->avail_out;

    return_code = ZSTD_compressStream2(zwc, &out, &in, ZSTD_e_continue);

    strm->next_in += in.pos;
    strm->total_in += (uLong)in.pos;
    strm->avail_in -= (uLong)in.pos;

    strm->next_out += out.pos;
    strm->total_out += (uLong)out.pos;
    strm->avail_out -= (uLong)out.pos;

    if (ZSTD_isError(return_code))
      return ABSTR_COMPRESS_Z_ERRNO;
  }

  /* we can flush only if in buffer is empty */
  if ((flush == ABSTR_COMPRESS_Z_NO_FLUSH) || (strm->avail_in > 0))
    return ABSTR_COMPRESS_Z_OK;

  out.pos = 0;
  out.dst = strm->next_out;
  out.size = strm->avail_out;

  in.pos = 0;
  in.size = 0;

  return_code = ZSTD_compressStream2(zwc, &out, &in, (flush != ABSTR_COMPRESS_Z_FINISH) ? ZSTD_e_flush : ZSTD_e_end);

  strm->next_out += out.pos;
  strm->total_out += (uLong)out.pos;
  strm->avail_out -= (uLong)out.pos;

  if (ZSTD_isError(return_code)) {
	/*
	const char* errstr = ZSTD_getErrorName(return_code);
	fprintf(stderr,"%s\n",errstr);
	*/
    return ABSTR_COMPRESS_Z_ERRNO;
  }

  return ((flush == ABSTR_COMPRESS_Z_FINISH) && (return_code == 0)) ? ABSTR_COMPRESS_Z_STREAM_END : ABSTR_COMPRESS_Z_OK;
}


static int zstd_mt_zlib_mini_wrap_clone_deflateEnd (z_streamp strm)
{
  ZSTD_CCtx* zwc = (ZSTD_CCtx*)strm->state;
  size_t errorCode = ZSTD_freeCCtx(zwc);

  if (ZSTD_isError(errorCode))
    return Z_ERRNO;
  return Z_OK;
}


static int zstd_mt_zlib_mini_wrap_clone_deflateInit (z_streamp strm, int level, int longwindow, int mt)
{
  SMARTVERSION_DISCARD_UNUSED(mt);
  ZSTD_CCtx* zwc=ZSTD_createCCtx();
  /* ZSTD_CStreamInSize(); ZSTD_CStreamOutSize(); */
  /*

        DISPLAYLEVEL(5,"set nb workers = %u \n", prefs->nbWorkers);
        CHECK( ZSTD_CCtx_setParameter(ress.cctx, ZSTD_c_nbWorkers, prefs->nbWorkers) );
        CHECK( ZSTD_CCtx_setParameter(ress.cctx, ZSTD_c_jobSize, prefs->blockSize) );
		*/
  if (zwc == NULL)
    return Z_MEM_ERROR;

  ZSTD_CCtx_setParameter(zwc, ZSTD_c_compressionLevel, level);
  if (longwindow==2)
	ZSTD_CCtx_setParameter(zwc, ZSTD_c_windowLog, (int)ZSTD_WINDOWLOG_MAX_32);
  else if (longwindow!=0)
	ZSTD_CCtx_setParameter(zwc, ZSTD_c_windowLog, (int)ZSTD_WINDOWLOG_LIMIT_DEFAULT);

#ifdef COMPRESS_XZUTILS
  if (mt!=0)
    ZSTD_CCtx_setParameter(zwc, ZSTD_c_nbWorkers, lzma_cputhreads());
  /*ZSTD_CCtx_setParameter(zwc, ZSTD_c_jobSize, 65536);*/
#endif
  strm->state = (struct internal_state*) zwc; /* use state which in not used by user */
  strm->total_in = 0;
  strm->total_out = 0;
  return Z_OK;
}



static int zstd_mt_abstract_compress(struct abstract_compress_stream_s* strm, int flush)
{
  int retValue = Z_OK;
  union_native_compress_stream* native_stream = (union_native_compress_stream*)strm->p_native_stream;

  native_stream->zlib_stream.avail_in = strm->avail_in;
  native_stream->zlib_stream.total_in = (uLong)strm->total_in;
  native_stream->zlib_stream.next_in = strm->next_in;

  native_stream->zlib_stream.avail_out = strm->avail_out;
  native_stream->zlib_stream.total_out = (uLong)(strm->total_out - strm->size_prefix);
  native_stream->zlib_stream.next_out = strm->next_out;

  retValue = zstd_mt_zlib_mini_wrap_clone_deflate(&native_stream->zlib_stream, flush);

  strm->avail_in = native_stream->zlib_stream.avail_in;
  strm->total_in = native_stream->zlib_stream.total_in;
  strm->next_in = native_stream->zlib_stream.next_in;

  strm->avail_out = native_stream->zlib_stream.avail_out;
  strm->total_out = native_stream->zlib_stream.total_out + strm->size_prefix;
  strm->next_out = native_stream->zlib_stream.next_out;

  return retValue;
}


static int zstd_mt_abstract_compress_end(struct abstract_compress_stream_s* strm)
{
  int retValue;
  union_native_compress_stream* native_stream = (union_native_compress_stream*)strm->p_native_stream;
  native_stream->zlib_stream.avail_in = strm->avail_in;
  native_stream->zlib_stream.total_in = (uLong)strm->total_in;
  native_stream->zlib_stream.next_in = strm->next_in;

  native_stream->zlib_stream.avail_out = strm->avail_out;
  native_stream->zlib_stream.total_out = (uLong)(strm->total_out - strm->size_prefix);
  native_stream->zlib_stream.next_out = strm->next_out;

  retValue = zstd_mt_zlib_mini_wrap_clone_deflateEnd(&native_stream->zlib_stream);

  strm->avail_in = native_stream->zlib_stream.avail_in;
  strm->total_in = native_stream->zlib_stream.total_in;
  strm->next_in = native_stream->zlib_stream.next_in;

  strm->avail_out = native_stream->zlib_stream.avail_out;
  strm->total_out = native_stream->zlib_stream.total_out + strm->size_prefix;
  strm->next_out = native_stream->zlib_stream.next_out;

  return retValue;
}

#endif


#ifdef COMPRESS_ZSTD

static void ClearZstdZlibWrapCompressStream(z_stream* strm)
{
  memset(strm, 0, sizeof(z_stream));
}


static int zstd_zlib_mini_wrap_clone_deflate (z_streamp strm, int flush)
{
  ZSTD_CStream* zwc;
  ZSTD_inBuffer in;
  ZSTD_outBuffer out;
  size_t return_code;

  zwc = (ZSTD_CStream*)strm->state;

  if ((flush == ABSTR_COMPRESS_Z_NO_FLUSH) || (strm->avail_in > 0))
  {
    in.pos = 0;
    in.src = strm->next_in;
    in.size = strm->avail_in;

    out.pos = 0;
    out.dst = strm->next_out;
    out.size = strm->avail_out;

    return_code = ZSTD_compressStream(zwc, &out, &in);

    strm->next_in += in.pos;
    strm->total_in += (uLong)in.pos;
    strm->avail_in -= (uLong)in.pos;

    strm->next_out += out.pos;
    strm->total_out += (uLong)out.pos;
    strm->avail_out -= (uLong)out.pos;

    if (ZSTD_isError(return_code))
      return ABSTR_COMPRESS_Z_ERRNO;
  }

  /* we can flush only if in buffer is empty */
  if ((flush == ABSTR_COMPRESS_Z_NO_FLUSH) || (strm->avail_in > 0))
    return ABSTR_COMPRESS_Z_OK;

  out.pos = 0;
  out.dst = strm->next_out;
  out.size = strm->avail_out;

  return_code = (flush != ABSTR_COMPRESS_Z_FINISH) ? ZSTD_flushStream(zwc, &out) : ZSTD_endStream(zwc, &out);



  strm->next_out += out.pos;
  strm->total_out += (uLong)out.pos;
  strm->avail_out -= (uLong)out.pos;

  if (ZSTD_isError(return_code))
    return ABSTR_COMPRESS_Z_ERRNO;

  return ((flush == ABSTR_COMPRESS_Z_FINISH) && (return_code == 0)) ? ABSTR_COMPRESS_Z_STREAM_END : ABSTR_COMPRESS_Z_OK;
}


static int zstd_zlib_mini_wrap_clone_deflateEnd (z_streamp strm)
{
  ZSTD_CStream* zwc = (ZSTD_CStream*)strm->state;
  size_t errorCode = ZSTD_freeCStream(zwc);
  if (ZSTD_isError(errorCode)) return Z_MEM_ERROR;

  return Z_OK;
}


static int zstd_zlib_mini_wrap_clone_deflateInit (z_streamp strm, int level)
{
  ZSTD_CStream* zwc= ZSTD_createCStream();
  size_t errorCode;
  if (zwc == NULL)
    return Z_MEM_ERROR;

  errorCode=ZSTD_initCStream(zwc, level);
  if (ZSTD_isError(errorCode))
  {
    ZSTD_freeCStream(zwc);
    return Z_MEM_ERROR;
  }

  strm->state = (struct internal_state*) zwc; /* use state which in not used by user */
  strm->total_in = 0;
  strm->total_out = 0;
  return Z_OK;
}



static int zstd_abstract_compress(struct abstract_compress_stream_s* strm, int flush)
{
  int retValue = Z_OK;
  union_native_compress_stream* native_stream = (union_native_compress_stream*)strm->p_native_stream;

  native_stream->zlib_stream.avail_in = strm->avail_in;
  native_stream->zlib_stream.total_in = (uLong)strm->total_in;
  native_stream->zlib_stream.next_in = strm->next_in;

  native_stream->zlib_stream.avail_out = strm->avail_out;
  native_stream->zlib_stream.total_out = (uLong)(strm->total_out - strm->size_prefix);
  native_stream->zlib_stream.next_out = strm->next_out;

  retValue = zstd_zlib_mini_wrap_clone_deflate(&native_stream->zlib_stream, flush);

  strm->avail_in = native_stream->zlib_stream.avail_in;
  strm->total_in = native_stream->zlib_stream.total_in;
  strm->next_in = native_stream->zlib_stream.next_in;

  strm->avail_out = native_stream->zlib_stream.avail_out;
  strm->total_out = native_stream->zlib_stream.total_out + strm->size_prefix;
  strm->next_out = native_stream->zlib_stream.next_out;

  return retValue;
}


static int zstd_abstract_compress_end(struct abstract_compress_stream_s* strm)
{
  int retValue;
  union_native_compress_stream* native_stream = (union_native_compress_stream*)strm->p_native_stream;
  native_stream->zlib_stream.avail_in = strm->avail_in;
  native_stream->zlib_stream.total_in = (uLong)strm->total_in;
  native_stream->zlib_stream.next_in = strm->next_in;

  native_stream->zlib_stream.avail_out = strm->avail_out;
  native_stream->zlib_stream.total_out = (uLong)(strm->total_out - strm->size_prefix);
  native_stream->zlib_stream.next_out = strm->next_out;

  retValue = zstd_zlib_mini_wrap_clone_deflateEnd(&native_stream->zlib_stream);

  strm->avail_in = native_stream->zlib_stream.avail_in;
  strm->total_in = native_stream->zlib_stream.total_in;
  strm->next_in = native_stream->zlib_stream.next_in;

  strm->avail_out = native_stream->zlib_stream.avail_out;
  strm->total_out = native_stream->zlib_stream.total_out + strm->size_prefix;
  strm->next_out = native_stream->zlib_stream.next_out;

  return retValue;
}

#endif



#ifdef COMPRESS_BZIP2

static void ClearBZip2CompressStream(bz_stream* strm)
{
	memset(strm,0,sizeof(bz_stream));
}

static int bzip2_abstract_compress(struct abstract_compress_stream_s* strm,int flush)
{
	int retValue,retValueBZ;
	union_native_compress_stream* native_stream = (union_native_compress_stream*)strm->p_native_stream;
	/*
#define ABSTR_COMPRESS_Z_NO_FLUSH      0
#define ABSTR_COMPRESS_Z_SYNC_FLUSH    2
#define ABSTR_COMPRESS_Z_FINISH        4
	*/
	int flushbz = BZ_RUN;
	if (flush == ABSTR_COMPRESS_Z_SYNC_FLUSH)
		flushbz = BZ_FLUSH;
	if (flush == ABSTR_COMPRESS_Z_FINISH)
		flushbz = BZ_FINISH;

	native_stream->bzip2_stream.avail_in = strm->avail_in;
	native_stream->bzip2_stream.total_in_lo32 = (unsigned int)strm->total_in;
	native_stream->bzip2_stream.total_in_hi32 = (unsigned int)(strm->total_in >> 32);
	native_stream->bzip2_stream.next_in = (char*)strm->next_in;

	native_stream->bzip2_stream.avail_out = strm->avail_out;

	strm->total_out -= strm->size_prefix;
	native_stream->bzip2_stream.total_out_lo32 = (unsigned int)strm->total_out;
	native_stream->bzip2_stream.total_out_hi32 = (unsigned int)(strm->total_out >> 32);
	native_stream->bzip2_stream.next_out = (char*)strm->next_out;

	retValueBZ = BZ2_bzCompress(&native_stream->bzip2_stream, flushbz);

	strm->avail_in =native_stream->bzip2_stream.avail_in ;
	strm->total_in = native_stream->bzip2_stream.total_in_lo32 |
		(((dfuLong64)native_stream->bzip2_stream.total_in_hi32) << 32);
	strm->next_in = (Bytef*)native_stream->bzip2_stream.next_in ;

	strm->avail_out = native_stream->bzip2_stream.avail_out ;
	strm->total_out = native_stream->bzip2_stream.total_out_lo32 |
		(((dfuLong64)native_stream->bzip2_stream.total_out_hi32) << 32);
	strm->total_out += strm->size_prefix;
	strm->next_out = (Bytef*)native_stream->bzip2_stream.next_out ;


	if ((retValueBZ == BZ_FINISH_OK) || (retValueBZ == BZ_RUN_OK))
		retValue = ABSTR_COMPRESS_Z_OK;
	else
	if (retValueBZ == BZ_STREAM_END)
		retValue = ABSTR_COMPRESS_Z_STREAM_END;
	else
		retValue = ABSTR_COMPRESS_Z_ERRNO;

	return retValue;
}


static int bzip2_abstract_compress_end(struct abstract_compress_stream_s* strm)
{
	int retValue;
	union_native_compress_stream* native_stream = (union_native_compress_stream*)strm->p_native_stream;
	native_stream->bzip2_stream.avail_in = strm->avail_in;
	strm->total_out -= strm->size_prefix;
	native_stream->bzip2_stream.total_in_lo32 = (unsigned int)strm->total_in;
	native_stream->bzip2_stream.total_in_hi32 = (unsigned int)(strm->total_in >> 32);
	native_stream->bzip2_stream.next_in = (char*)strm->next_in;

	native_stream->bzip2_stream.avail_out = strm->avail_out;

	native_stream->bzip2_stream.total_out_lo32 = (unsigned int)strm->total_out;
	native_stream->bzip2_stream.total_out_hi32 = (unsigned int)(strm->total_out >> 32);
	native_stream->bzip2_stream.next_out = (char*)strm->next_out;

	retValue = BZ2_bzCompressEnd(&native_stream->bzip2_stream);

	strm->avail_in =native_stream->bzip2_stream.avail_in ;
	strm->total_in = native_stream->bzip2_stream.total_in_lo32 |
		(((dfuLong64)native_stream->bzip2_stream.total_in_hi32) << 32);
	strm->next_in = (Bytef*)native_stream->bzip2_stream.next_in ;

	strm->avail_out = native_stream->bzip2_stream.avail_out ;
	strm->total_out = native_stream->bzip2_stream.total_out_lo32 |
		(((dfuLong64)native_stream->bzip2_stream.total_out_hi32) << 32);
	strm->total_out += strm->size_prefix;
	strm->next_out = (Bytef*)native_stream->bzip2_stream.next_out ;

	return retValue;
}
#endif


#if defined(COMPRESS_XZUTILS) || defined(ABSTRACT_CPR_XZUTILS_DYNAMIC_AND_UNCOMPRESS_XZEMBEDDED)


static void ClearXzUtilsCompressStream(lzma_stream* strm)
{
	memset(strm,0,sizeof(lzma_stream));
}

static int xzutils_abstract_compress(struct abstract_compress_stream_s* strm,int flush)
{
	int retValue,retValueXZ;
	lzma_action action ;
	union_native_compress_stream* native_stream = (union_native_compress_stream*)strm->p_native_stream;
	native_stream->xzut.xzutils_stream.avail_in = strm->avail_in;
	native_stream->xzut.xzutils_stream.total_in = strm->total_in;
	native_stream->xzut.xzutils_stream.next_in = strm->next_in;

	native_stream->xzut.xzutils_stream.avail_out = strm->avail_out;
	native_stream->xzut.xzutils_stream.total_out = (strm->total_out - strm->size_prefix);
	native_stream->xzut.xzutils_stream.next_out = strm->next_out;

	action = LZMA_RUN;

	if (flush == ABSTR_COMPRESS_Z_SYNC_FLUSH)
		action = native_stream->xzut.mt ? LZMA_FULL_FLUSH : LZMA_SYNC_FLUSH; // LZMA_RUN; ?
	if (flush == ABSTR_COMPRESS_Z_FINISH)
		action = LZMA_FINISH;

#ifdef ABSTRACT_CPR_XZUTILS_DYNAMIC_AND_UNCOMPRESS_XZEMBEDDED
	retValueXZ = dynamic_lzma_code(&native_stream->xzutils_stream, action);
#else
	retValueXZ = lzma_code(&native_stream->xzut.xzutils_stream, action);
#endif

	strm->avail_in = (uInt)native_stream->xzut.xzutils_stream.avail_in ;
	strm->total_in = native_stream->xzut.xzutils_stream.total_in;
	strm->next_in = (Bytef*)native_stream->xzut.xzutils_stream.next_in ;

	strm->avail_out = (uInt)native_stream->xzut.xzutils_stream.avail_out ;
	strm->total_out = native_stream->xzut.xzutils_stream.total_out + strm->size_prefix;
	strm->next_out = native_stream->xzut.xzutils_stream.next_out ;

	if (retValueXZ == LZMA_OK)
		retValue = ABSTR_COMPRESS_Z_OK;
	else
	if ((retValueXZ == LZMA_STREAM_END) && (flush == ABSTR_COMPRESS_Z_SYNC_FLUSH))
		retValue = ABSTR_COMPRESS_Z_OK;
	else
	if (retValueXZ == LZMA_STREAM_END)
		retValue = ABSTR_COMPRESS_Z_STREAM_END;
	else
		retValue = ABSTR_COMPRESS_Z_ERRNO;
//printf("ret xzutils_abstract_compress %d-> %d (lz:%d->%d)\n",flush,retValue,(int)action,(int)retValueXZ);
	return retValue;
}


static int xzutils_abstract_compress_end(struct abstract_compress_stream_s* strm)
{
	union_native_compress_stream* native_stream = (union_native_compress_stream*)strm->p_native_stream;
	native_stream->xzut.xzutils_stream.avail_in = strm->avail_in;
	native_stream->xzut.xzutils_stream.total_in = strm->total_in;
	native_stream->xzut.xzutils_stream.next_in = strm->next_in;

	native_stream->xzut.xzutils_stream.avail_out = strm->avail_out;
	native_stream->xzut.xzutils_stream.total_out = strm->total_out - strm->size_prefix;
	native_stream->xzut.xzutils_stream.next_out = strm->next_out;

#ifdef ABSTRACT_CPR_XZUTILS_DYNAMIC_AND_UNCOMPRESS_XZEMBEDDED
	dynamic_lzma_end(&native_stream->xzutils_stream);
#else
	lzma_end(&native_stream->xzut.xzutils_stream);
#endif

	strm->avail_in = (uInt)native_stream->xzut.xzutils_stream.avail_in ;
	strm->total_in = native_stream->xzut.xzutils_stream.total_in ;
	strm->next_in = (Bytef*)native_stream->xzut.xzutils_stream.next_in ;

	strm->avail_out = (uInt)native_stream->xzut.xzutils_stream.avail_out ;
	strm->total_out = native_stream->xzut.xzutils_stream.total_out + strm->size_prefix ;
	strm->next_out = native_stream->xzut.xzutils_stream.next_out ;

	return ABSTR_COMPRESS_Z_OK;
}
#endif

static int zlib_store_abstract_compress(struct abstract_compress_stream_s* strm,int flush)
{
	int retValue;
	union_native_compress_stream* native_stream = (union_native_compress_stream*)strm->p_native_stream;
	native_stream->zlib_stream.avail_in = strm->avail_in;
	native_stream->zlib_stream.total_in = (uLong)strm->total_in;
	native_stream->zlib_stream.next_in = strm->next_in;

	native_stream->zlib_stream.avail_out = strm->avail_out;
	native_stream->zlib_stream.total_out = (uLong)(strm->total_out - strm->size_prefix);
	native_stream->zlib_stream.next_out = strm->next_out;

	retValue = XflateStore(&native_stream->zlib_stream, flush);

	strm->avail_in =native_stream->zlib_stream.avail_in ;
	strm->total_in = native_stream->zlib_stream.total_in ;
	strm->next_in = native_stream->zlib_stream.next_in ;

	strm->avail_out = native_stream->zlib_stream.avail_out ;
	strm->total_out = native_stream->zlib_stream.total_out + strm->size_prefix;
	strm->next_out = native_stream->zlib_stream.next_out ;

	return retValue;
}


static int zlib_store_abstract_compress_end(struct abstract_compress_stream_s* strm)
{
	SMARTVERSION_DISCARD_UNUSED(strm);
	return ABSTR_COMPRESS_Z_OK;
}




int abstract_init_compress_inflate_withNegMaxWBits(abstract_compress_stream* strm, int level)
{
	int retValue;
	union_native_compress_stream* native_stream = (union_native_compress_stream*)malloc(sizeof(union_native_compress_stream));
	if (native_stream == NULL)
		return ABSTR_COMPRESS_Z_ERRNO;
	memset(native_stream, 0, sizeof(union_native_compress_stream));
	strm->p_native_stream = (void*)native_stream;
	ClearZLibCompressStream(&native_stream->zlib_stream);

	strm->contain_prefix = 0;
	strm->is_prefix_write = 1;
	strm->size_prefix = 0;

	strm->fnc_compress = zlib_abstract_compress;
	strm->fnc_compress_end = zlib_abstract_compress_end;

	retValue = deflateInit2(&native_stream->zlib_stream,level,Z_DEFLATED, -MAX_WBITS, DEF_MEM_LEVEL, 0);
	return retValue;
}

int abstract_init_compress_inflate_withoutNegMaxWBits(abstract_compress_stream* strm, int level)
{
	int retValue;
	union_native_compress_stream* native_stream = (union_native_compress_stream*)malloc(sizeof(union_native_compress_stream));
	if (native_stream == NULL)
		return ABSTR_COMPRESS_Z_ERRNO;
	memset(native_stream, 0, sizeof(union_native_compress_stream));
	strm->p_native_stream = (void*)native_stream;
	ClearZLibCompressStream(&native_stream->zlib_stream);

	strm->contain_prefix = 0;
	strm->is_prefix_write = 1;
	strm->size_prefix = 0;
	strm->prefix_value = 0;

	strm->fnc_compress = zlib_abstract_compress;
	strm->fnc_compress_end = zlib_abstract_compress_end;

	retValue = deflateInit(&native_stream->zlib_stream, level);
	return retValue;
}


int abstract_init_compress_inflate_with_gzip_header(abstract_compress_stream* strm, int level)
{
	int retValue;
	union_native_compress_stream* native_stream = (union_native_compress_stream*)malloc(sizeof(union_native_compress_stream));
	if (native_stream == NULL)
		return ABSTR_COMPRESS_Z_ERRNO;
	memset(native_stream, 0, sizeof(union_native_compress_stream));
	strm->p_native_stream = (void*)native_stream;
	ClearZLibCompressStream(&native_stream->zlib_stream);

	strm->contain_prefix = 0;
	strm->is_prefix_write = 1;
	strm->size_prefix = 0;
	strm->prefix_value = 0;

	strm->fnc_compress = zlib_abstract_compress;
	strm->fnc_compress_end = zlib_abstract_compress_end;

	retValue = deflateInit2(&native_stream->zlib_stream, level, Z_DEFLATED, MAX_WBITS+16, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);
	return retValue;
}

int abstract_init_compress_inflate_withNegMaxWBits_prefix(abstract_compress_stream* strm, int level)
{
	int retValue;
	union_native_compress_stream* native_stream = (union_native_compress_stream*)malloc(sizeof(union_native_compress_stream));
	if (native_stream == NULL)
		return ABSTR_COMPRESS_Z_ERRNO;
	memset(native_stream, 0, sizeof(union_native_compress_stream));
	strm->p_native_stream = (void*)native_stream;
	ClearZLibCompressStream(&native_stream->zlib_stream);

	strm->contain_prefix = 1;
	strm->is_prefix_write = 0;
	strm->size_prefix = 1;
	strm->prefix_value = ABSTRACT_CPR_PREFIX_DEFLATE;

	strm->fnc_compress = zlib_abstract_compress;
	strm->fnc_compress_end = zlib_abstract_compress_end;

	retValue = deflateInit2(&native_stream->zlib_stream,level,Z_DEFLATED, -MAX_WBITS, DEF_MEM_LEVEL, 0);
	return retValue;
}

int abstract_init_compress_raw_store(abstract_compress_stream* strm)
{
	int retValue;
	union_native_compress_stream* native_stream = (union_native_compress_stream*)malloc(sizeof(union_native_compress_stream));
	if (native_stream == NULL)
		return ABSTR_COMPRESS_Z_ERRNO;
	memset(native_stream, 0, sizeof(union_native_compress_stream));
	strm->p_native_stream = (void*)native_stream;
	ClearZLibCompressStream(&native_stream->zlib_stream);

	strm->contain_prefix = 1;
	strm->is_prefix_write = 0;
	strm->size_prefix = 1;
	strm->prefix_value = ABSTRACT_CPR_PREFIX_STORE;

	strm->fnc_compress = zlib_store_abstract_compress;
	strm->fnc_compress_end = zlib_store_abstract_compress_end;

	retValue = ABSTR_COMPRESS_Z_OK;
	return retValue;
}


#ifdef COMPRESS_ZSTD
#ifdef COMPRESS_ZSTD_MT
static int abstract_init_compress_zstd_mt_with_prefix(abstract_compress_stream* strm, int level, int longwindow, int mt, int prefix)
{
  int retValue = ABSTR_COMPRESS_Z_OK;
  int ret_bzCompressInit;
  union_native_compress_stream* native_stream = (union_native_compress_stream*)malloc(sizeof(union_native_compress_stream));
  if (native_stream == NULL)
    return ABSTR_COMPRESS_Z_ERRNO;
  memset(native_stream, 0, sizeof(union_native_compress_stream));
  strm->p_native_stream = (void*)native_stream;
  ClearZstdMtZlibWrapCompressStream(&native_stream->zlib_stream);

  if (prefix)
  {
    strm->contain_prefix = 1;
    strm->is_prefix_write = 0;
    strm->size_prefix = 1;
  }
  else
  {
    strm->contain_prefix = 0;
    strm->is_prefix_write = 1;
    strm->size_prefix = 0;
  }
  strm->prefix_value = ABSTRACT_CPR_PREFIX_ZSTD;

  strm->fnc_compress = zstd_mt_abstract_compress;
  strm->fnc_compress_end = zstd_mt_abstract_compress_end;

  ret_bzCompressInit = zstd_mt_zlib_mini_wrap_clone_deflateInit(&native_stream->zlib_stream, level, longwindow, mt);
  if (ret_bzCompressInit != Z_OK)
    retValue = ABSTR_COMPRESS_Z_ERRNO;
  return retValue;
}
#endif

static int abstract_init_compress_zstd_with_prefix(abstract_compress_stream* strm, int level, int prefix)
{
  int retValue = ABSTR_COMPRESS_Z_OK;
  int ret_bzCompressInit;
  union_native_compress_stream* native_stream = (union_native_compress_stream*)malloc(sizeof(union_native_compress_stream));
  if (native_stream == NULL)
    return ABSTR_COMPRESS_Z_ERRNO;
  memset(native_stream, 0, sizeof(union_native_compress_stream));
  strm->p_native_stream = (void*)native_stream;
  ClearZstdZlibWrapCompressStream(&native_stream->zlib_stream);

  if (prefix)
  {
    strm->contain_prefix = 1;
    strm->is_prefix_write = 0;
    strm->size_prefix = 1;
  }
  else
  {
    strm->contain_prefix = 0;
    strm->is_prefix_write = 1;
    strm->size_prefix = 0;
  }
  strm->prefix_value = ABSTRACT_CPR_PREFIX_ZSTD;

  strm->fnc_compress = zstd_abstract_compress;
  strm->fnc_compress_end = zstd_abstract_compress_end;

  ret_bzCompressInit = zstd_zlib_mini_wrap_clone_deflateInit(&native_stream->zlib_stream,level);
  if (ret_bzCompressInit != Z_OK)
    retValue = ABSTR_COMPRESS_Z_ERRNO;
  return retValue;
}

#endif

#ifdef COMPRESS_BZIP2

static int abstract_init_compress_bzip2_with_prefix(abstract_compress_stream* strm, int level, int prefix)
{
	int retValue = ABSTR_COMPRESS_Z_OK;
	int ret_bzCompressInit;
	union_native_compress_stream* native_stream = (union_native_compress_stream*)malloc(sizeof(union_native_compress_stream));
	if (native_stream == NULL)
		return ABSTR_COMPRESS_Z_ERRNO;
	memset(native_stream, 0, sizeof(union_native_compress_stream));
	strm->p_native_stream = (void*)native_stream;
	ClearBZip2CompressStream(&native_stream->bzip2_stream);



	strm->contain_prefix = 0;
	strm->is_prefix_write = 1;
	strm->size_prefix = 0;
	strm->prefix_value = 0;

	strm->fnc_compress = bzip2_abstract_compress;
	strm->fnc_compress_end = bzip2_abstract_compress_end;

	ret_bzCompressInit = BZ2_bzCompressInit(&native_stream->bzip2_stream, (level == -1) ? (6+2+1) : level, 0,30);
	if (ret_bzCompressInit != BZ_OK)
		retValue = ABSTR_COMPRESS_Z_ERRNO;
	return retValue;
}

static int abstract_init_compress_bzip2(abstract_compress_stream* strm, int level)
{
	return abstract_init_compress_bzip2_with_prefix(strm, level, 0);
}
#endif

#ifdef HAVE_FAST_LZLIB


static void ClearFastZlibCompressStream(zfast_stream* strm)
{
	memset(strm, 0, sizeof(zfast_stream));
}


static int fastzlib_abstract_compress(struct abstract_compress_stream_s* strm, int flush)
{
	int retValue;
	union_native_compress_stream* native_stream = (union_native_compress_stream*)strm->p_native_stream;
	for (;;) {
		native_stream->fastzlib_stream.avail_in = strm->avail_in;
		native_stream->fastzlib_stream.total_in = (uLong)strm->total_in;
		native_stream->fastzlib_stream.next_in = strm->next_in;

		native_stream->fastzlib_stream.avail_out = strm->avail_out;
		native_stream->fastzlib_stream.total_out = (uLong)(strm->total_out - strm->size_prefix);
		native_stream->fastzlib_stream.next_out = strm->next_out;

		retValue = fastlzlibCompress(&native_stream->fastzlib_stream, flush);

		strm->avail_in = native_stream->fastzlib_stream.avail_in;
		strm->total_in = native_stream->fastzlib_stream.total_in;
		strm->next_in = native_stream->fastzlib_stream.next_in;

		strm->avail_out = native_stream->fastzlib_stream.avail_out;
		strm->total_out = (dfuLong64)(native_stream->fastzlib_stream.total_out) + strm->size_prefix;
		strm->next_out = native_stream->fastzlib_stream.next_out;

		if ((retValue!=Z_OK) || (strm->avail_in==0) || (strm->avail_out==0)) break;
	}
	return retValue;
}


/*
int fastzlib_abstract_compress(struct abstract_compress_stream_s* strm, int flush)
{
	int retValue;
	uInt process_this_in, process_this_out;
	union_native_compress_stream* native_stream = (union_native_compress_stream*)strm->p_native_stream;
	native_stream->fastzlib_stream.avail_in = strm->avail_in;
	native_stream->fastzlib_stream.total_in = (uLong)strm->total_in;
	native_stream->fastzlib_stream.next_in = strm->next_in;

	native_stream->fastzlib_stream.avail_out = strm->avail_out;
	native_stream->fastzlib_stream.total_out = (uLong)(strm->total_out - strm->size_prefix);
	native_stream->fastzlib_stream.next_out = strm->next_out;

	do
	{
		process_this_in = native_stream->fastzlib_stream.avail_in;
		process_this_out = native_stream->fastzlib_stream.avail_out;
		retValue = fastlzlibCompress(&native_stream->fastzlib_stream, flush);
		process_this_in = process_this_in - native_stream->fastzlib_stream.avail_in;
		process_this_out = process_this_out - native_stream->fastzlib_stream.avail_out;
	} while ((retValue == Z_OK) && (strm->avail_in > 0) && (strm->avail_out > 0) && ((process_this_in+ process_this_out)>0));

	strm->avail_in = native_stream->fastzlib_stream.avail_in;
	strm->total_in = native_stream->fastzlib_stream.total_in;
	strm->next_in = native_stream->fastzlib_stream.next_in;

	strm->avail_out = native_stream->fastzlib_stream.avail_out;
	strm->total_out = native_stream->fastzlib_stream.total_out + strm->size_prefix;
	strm->next_out = native_stream->fastzlib_stream.next_out;

	return retValue;
}
*/


static int fastzlib_abstract_compress_end(struct abstract_compress_stream_s* strm)
{
	int retValue;
	union_native_compress_stream* native_stream = (union_native_compress_stream*)strm->p_native_stream;
	native_stream->fastzlib_stream.avail_in = strm->avail_in;
	native_stream->fastzlib_stream.total_in = (uLong)strm->total_in;
	native_stream->fastzlib_stream.next_in = strm->next_in;

	native_stream->fastzlib_stream.avail_out = strm->avail_out;
	native_stream->fastzlib_stream.total_out = (uLong)(strm->total_out - strm->size_prefix);
	native_stream->fastzlib_stream.next_out = strm->next_out;

	retValue = fastlzlibCompressEnd(&native_stream->fastzlib_stream);

	strm->avail_in = native_stream->fastzlib_stream.avail_in;
	strm->total_in = native_stream->fastzlib_stream.total_in;
	strm->next_in = native_stream->fastzlib_stream.next_in;

	strm->avail_out = native_stream->fastzlib_stream.avail_out;
	strm->total_out = native_stream->fastzlib_stream.total_out + strm->size_prefix;
	strm->next_out = native_stream->fastzlib_stream.next_out;

	return retValue;
}


static int abstract_init_compress_fast_zlib(abstract_compress_stream* strm, int level, unsigned char prefix_value)
{
	int retValue = ABSTR_COMPRESS_Z_OK;
	int ret_CompressInit;
	int block_size;
	zfast_stream_compressor type;
	union_native_compress_stream* native_stream;
	//zfast_stream_compressor type = (prefix_value == ABSTRACT_CPR_PREFIX_FASTZL) ? COMPRESSOR_FASTLZ : COMPRESSOR_LZ4;
	switch (prefix_value)
	{
		case ABSTRACT_CPR_PREFIX_FASTZL_LOW_BLOCKSIZE : type = COMPRESSOR_FASTLZ; block_size = FASTZLIB_LOW_BLOCKSIZE; break;
		case ABSTRACT_CPR_PREFIX_FASTZL_DEFAULT_BLOCKSIZE: type = COMPRESSOR_FASTLZ; block_size = FASTZLIB_DEFAULT_BLOCKSIZE; break;
		case ABSTRACT_CPR_PREFIX_FASTZL_HIGH_BLOCKSIZE: type = COMPRESSOR_FASTLZ; block_size = FASTZLIB_HIGH_BLOCKSIZE; break;

		case ABSTRACT_CPR_PREFIX_LZ4_LOW_BLOCKSIZE: type = COMPRESSOR_LZ4; block_size = FASTZLIB_LOW_BLOCKSIZE; break;
		case ABSTRACT_CPR_PREFIX_LZ4_DEFAULT_BLOCKSIZE: type = COMPRESSOR_LZ4; block_size = FASTZLIB_DEFAULT_BLOCKSIZE; break;
		case ABSTRACT_CPR_PREFIX_LZ4_HIGH_BLOCKSIZE: type = COMPRESSOR_LZ4; block_size = FASTZLIB_HIGH_BLOCKSIZE; break;

#if defined(COMPRESSOR_LZFSE) || defined(ZFAST_USE_LZFSE)
		case ABSTRACT_CPR_PREFIX_LZFSE_LOW_BLOCKSIZE: type = COMPRESSOR_LZFSE; block_size = FASTZLIB_LOW_BLOCKSIZE; break;
		case ABSTRACT_CPR_PREFIX_LZFSE_DEFAULT_BLOCKSIZE: type = COMPRESSOR_LZFSE; block_size = FASTZLIB_DEFAULT_BLOCKSIZE; break;
		case ABSTRACT_CPR_PREFIX_LZFSE_HIGH_BLOCKSIZE: type = COMPRESSOR_LZFSE; block_size = FASTZLIB_HIGH_BLOCKSIZE; break;
#endif

		default:
			return ABSTR_COMPRESS_Z_ERRNO;
	}

	native_stream = (union_native_compress_stream*)malloc(sizeof(union_native_compress_stream));
	if (native_stream == NULL)
		return ABSTR_COMPRESS_Z_ERRNO;
	memset(native_stream, 0, sizeof(union_native_compress_stream));
	strm->p_native_stream = (void*)native_stream;
	ClearFastZlibCompressStream(&native_stream->fastzlib_stream);

	strm->contain_prefix = 1;
	strm->is_prefix_write = 0;
	strm->size_prefix = 1;
	strm->prefix_value = prefix_value;

	strm->fnc_compress = fastzlib_abstract_compress;
	strm->fnc_compress_end = fastzlib_abstract_compress_end;

	ret_CompressInit = fastlzlibCompressInit2(&native_stream->fastzlib_stream, (level == -1) ? (6 + 2 + 1) : level, block_size);
	if (ret_CompressInit != Z_OK)
		retValue = ABSTR_COMPRESS_Z_ERRNO;

	if (fastlzlibSetCompressor(&native_stream->fastzlib_stream, type) != Z_OK)
		retValue = ABSTR_COMPRESS_Z_ERRNO;

	if (retValue != ABSTR_COMPRESS_Z_OK)
	{
		retValue = ABSTR_COMPRESS_Z_ERRNO;
		free((void*)native_stream);
		strm->p_native_stream = NULL;
	}
	return retValue;
}
#endif


#ifdef COMPRESS_XZUTILS

static lzma_ret init_lzma_stream_mt_for_ratio(lzma_stream* strm, int level, int extreme)
{
	lzma_mt mt;
	memset(&mt,0,sizeof(lzma_mt));

	mt.flags = 0;
	mt.block_size = 0;

	mt.timeout = 0;


	mt.preset = LZMA_PRESET_DEFAULT;
	if ((level>=1) && (level<=9)) {
		mt.preset = ((uint32_t)level) | ((extreme!=0) ? LZMA_PRESET_EXTREME : 0);
	}
	mt.filters = NULL;

	mt.check = LZMA_CHECK_CRC32;
	mt.threads = lzma_cputhreads();
	if (mt.threads == 0)
		mt.threads = 1;

	return lzma_stream_encoder_mt(strm, &mt);
}

int abstract_init_compress_xzUtils_with_prefix(abstract_compress_stream* strm, int level, int extreme, int prefix, int mt)
{
	int retValue = ABSTR_COMPRESS_Z_OK;
	lzma_ret lzma_ret_value;
	uint32_t preset = (uint32_t)level;
	union_native_compress_stream* native_stream = (union_native_compress_stream*)malloc(sizeof(union_native_compress_stream));
	if (native_stream == NULL)
		return ABSTR_COMPRESS_Z_ERRNO;
	memset(native_stream, 0, sizeof(union_native_compress_stream));
	strm->p_native_stream = (void*)native_stream;

#ifdef ABSTRACT_CPR_XZUTILS_DYNAMIC_AND_UNCOMPRESS_XZEMBEDDED
	if (dynamic_lzma_lib_load() == 0)
		return ABSTR_COMPRESS_Z_MISSINGLIB;
#endif

	ClearXzUtilsCompressStream(&native_stream->xzut.xzutils_stream);
	strm->prefix_value = ABSTRACT_CPR_XZUTILS_XZ_EMBEDDED;

	if (prefix)
	{
		strm->contain_prefix = 1;
		strm->is_prefix_write = 0;
		strm->size_prefix = 1;
	}
	else
	{
		strm->contain_prefix = 0;
		strm->is_prefix_write = 1;
		strm->size_prefix = 0;
	}

	strm->fnc_compress = xzutils_abstract_compress;
	strm->fnc_compress_end = xzutils_abstract_compress_end;

	if (preset == -1)
		preset = 9;
	if (extreme != 0)
		preset |= LZMA_PRESET_EXTREME;
	if (mt!=0)
		lzma_ret_value=init_lzma_stream_mt_for_ratio(&native_stream->xzut.xzutils_stream, level, extreme);
	else
#ifdef ABSTRACT_CPR_XZUTILS_DYNAMIC_AND_UNCOMPRESS_XZEMBEDDED
		lzma_ret_value = dynamic_lzma_easy_encoder(&native_stream->xzutils_stream, (preset == -1) ? 9 : preset, LZMA_CHECK_CRC32);
#else
		lzma_ret_value = lzma_easy_encoder(&native_stream->xzut.xzutils_stream, (preset == -1) ? 9 : preset, LZMA_CHECK_CRC32);
#endif
	native_stream->xzut.mt = mt;

	if (lzma_ret_value != LZMA_OK)
	{
		free(strm->p_native_stream);
		strm->p_native_stream = NULL;
		retValue = ABSTR_COMPRESS_Z_ERRNO;
	}
	return retValue;
}

int abstract_init_compress_xzUtils(abstract_compress_stream* strm, int level, int extreme)
{
	return abstract_init_compress_xzUtils_with_prefix(strm, level, extreme, 1, 0);
}
#endif


#ifdef COMPRESS_LZHAM

static int abstract_init_compress_LzHam_with_prefix_value(abstract_compress_stream* strm, int level, int prefix_value)
{
  int retValue = ABSTR_COMPRESS_Z_OK;
  int ret_lzHamCompressInit;
  union_native_compress_stream* native_stream = (union_native_compress_stream*)malloc(sizeof(union_native_compress_stream));
  if (native_stream == NULL)
    return ABSTR_COMPRESS_Z_ERRNO;
  memset(native_stream, 0, sizeof(union_native_compress_stream));
  strm->p_native_stream = (void*)native_stream;
  ClearLzHamCompressStream(&native_stream->lzham_z_stream);

  //if (prefix)
  //{
    strm->contain_prefix = 1;
    strm->is_prefix_write = 0;
    strm->size_prefix = 1;
  //}
  //else
  //{
  //  strm->contain_prefix = 0;
  //  strm->is_prefix_write = 1;
  //  strm->size_prefix = 0;
  //}
  strm->prefix_value = (unsigned char)prefix_value;

  strm->fnc_compress = LzHam_abstract_compress;
  strm->fnc_compress_end = LzHam_abstract_compress_end;

  //ret_bzCompressInit = lzham_z_deflateInit(&native_stream->lzham_z_stream, (level == -1) ? (6 + 2 + 1) : level);
  ret_lzHamCompressInit = lzham_z_deflateInit2(&native_stream->lzham_z_stream, (level == -1) ? (6 + 2 + 1) : level, LZHAM_Z_LZHAM,
    (prefix_value + (int)15) - ABSTRACT_CPR_PREFIX_LZHAM_DICT_15, 9, LZHAM_Z_DEFAULT_STRATEGY);
  if (ret_lzHamCompressInit != BZ_OK)
    retValue = ABSTR_COMPRESS_Z_ERRNO;
  return retValue;
}


#endif

unsigned char abstract_compress_consume_prefix(abstract_compress_stream* p_strm)
{
	unsigned char prefix = 0;
	if ((p_strm->contain_prefix == 1) && (p_strm->is_prefix_write == 0))
	{
		p_strm->is_prefix_write = 1;
		prefix = p_strm->prefix_value;
	}

	return prefix;
}


int abstract_compress (abstract_compress_stream* p_strm, int flush)
{
	int retValue;
	if ((p_strm->contain_prefix == 0) || (p_strm->is_prefix_write == 1))
		return (((p_strm)->fnc_compress)((p_strm),(flush)));

	if (p_strm->avail_out == 0)
		return ABSTR_COMPRESS_Z_OK;

	*((unsigned char*)(p_strm->next_out)) = p_strm->prefix_value;
	p_strm->next_out ++;
	p_strm->total_out ++; // size_prefix
	p_strm->avail_out --;
	p_strm->is_prefix_write = 1;


	retValue = (((p_strm)->fnc_compress)((p_strm),(flush)));
	/*
	if ((retValue != ABSTR_COMPRESS_Z_OK) && (retValue != ABSTR_COMPRESS_Z_STREAM_END))
		printf("abstract_compress error %d\n",retValue);
		*/
	return retValue;
}


int abstract_compress_end (abstract_compress_stream* p_strm)
{
	int retValue = (((p_strm)->fnc_compress_end)((p_strm)));
	/*
	if ((retValue != ABSTR_COMPRESS_Z_OK) && (retValue != ABSTR_COMPRESS_Z_STREAM_END))
		printf("abstract_compress_end error\n");
		*/
	union_native_compress_stream* native_stream = (union_native_compress_stream*)p_strm->p_native_stream;
	if (native_stream != NULL)
		free(native_stream);
	return retValue;
}


int XflateStore_compress(abstract_compress_stream* strm, int flush)
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

int abstract_init_compress_autoselect(abstract_compress_stream* strm, unsigned int ratio, int isStandardzLibWithNegMaxWBits)
{

#ifdef COMPRESS_BZIP2
		if ((ratio>=21) && (ratio<=29))
			return abstract_init_compress_bzip2(strm,(int)ratio-20);
		if ((ratio>=ABSTRACT_CPR_PARAM_UNPREFIXED_BZIP2_1) && (ratio<=ABSTRACT_CPR_PARAM_UNPREFIXED_BZIP2_9))
			return abstract_init_compress_bzip2_with_prefix(strm,(int)(ratio-ABSTRACT_CPR_PARAM_UNPREFIXED_BZIP2_1)+1, 0);
#endif

#ifdef COMPRESS_XZUTILS
		if ((ratio>=31) && (ratio<=39))
			return abstract_init_compress_xzUtils(strm,(int)ratio-30,0);

		if ((ratio>=41) && (ratio<=49))
			return abstract_init_compress_xzUtils(strm,(int)ratio-40,0);

		if ((ratio>=51) && (ratio<=59))
			return abstract_init_compress_xzUtils(strm,(int)ratio-50,0);

		if ((ratio>=61) && (ratio<=69))
			return abstract_init_compress_xzUtils(strm,(int)ratio-60,1);

		if ((ratio >= ABSTRACT_CPR_PARAM_UNPREFIXED_XZ_1) && (ratio <= ABSTRACT_CPR_PARAM_UNPREFIXED_XZ_9))
			return abstract_init_compress_xzUtils_with_prefix(strm, (int)(ratio - ABSTRACT_CPR_PARAM_UNPREFIXED_XZ_1) + 1, 0, 0, 0);

		if ((ratio >= ABSTRACT_CPR_PARAM_UNPREFIXED_XZ_EXTREME_1) && (ratio <= ABSTRACT_CPR_PARAM_UNPREFIXED_XZ_EXTREME_9))
			return abstract_init_compress_xzUtils_with_prefix(strm, (int)(ratio - ABSTRACT_CPR_PARAM_UNPREFIXED_XZ_EXTREME_1) + 1, 1, 0, 0);

		if ((ratio >= ABSTRACT_CPR_PARAM_UNPREFIXED_XZ_MT_0) && (ratio <= ABSTRACT_CPR_PARAM_UNPREFIXED_XZ_MT_9))
			return abstract_init_compress_xzUtils_with_prefix(strm, (int)(ratio - ABSTRACT_CPR_PARAM_UNPREFIXED_XZ_MT_1)+1, 0, 0, 1);

		if ((ratio >= ABSTRACT_CPR_PARAM_UNPREFIXED_XZ_EXTREME_MT_0) && (ratio <= ABSTRACT_CPR_PARAM_UNPREFIXED_XZ_EXTREME_MT_9))
			return abstract_init_compress_xzUtils_with_prefix(strm, (int)(ratio - ABSTRACT_CPR_PARAM_UNPREFIXED_XZ_EXTREME_MT_1)+1, 1, 0, 1);
#endif

#ifdef HAVE_FAST_LZLIB
		if ((ratio >= 71) && (ratio <= 79))
			return abstract_init_compress_fast_zlib(strm, (int)ratio - 70, ABSTRACT_CPR_PREFIX_FASTZL_LOW_BLOCKSIZE);

		if ((ratio >= 81) && (ratio <= 89))
			return abstract_init_compress_fast_zlib(strm, (int)ratio - 80, ABSTRACT_CPR_PREFIX_FASTZL_DEFAULT_BLOCKSIZE);

		if ((ratio >= 91) && (ratio <= 99))
			return abstract_init_compress_fast_zlib(strm, (int)ratio - 90, ABSTRACT_CPR_PREFIX_FASTZL_HIGH_BLOCKSIZE);

		if ((ratio >= 101) && (ratio <= 109))
			return abstract_init_compress_fast_zlib(strm, (int)ratio - 100, ABSTRACT_CPR_PREFIX_LZ4_LOW_BLOCKSIZE);

		if ((ratio >= 111) && (ratio <= 119))
			return abstract_init_compress_fast_zlib(strm, (int)ratio - 110, ABSTRACT_CPR_PREFIX_LZ4_DEFAULT_BLOCKSIZE);

		if ((ratio >= 121) && (ratio <= 129))
			return abstract_init_compress_fast_zlib(strm, (int)ratio - 120, ABSTRACT_CPR_PREFIX_LZ4_HIGH_BLOCKSIZE);

#if defined(COMPRESSOR_LZFSE) || defined(ZFAST_USE_LZFSE)
		if (ratio == ABSTRACT_CPR_PARAM_FASTZLIB_LZFSE_LOW_BLOCKSIZE)
			return abstract_init_compress_fast_zlib(strm, 10, ABSTRACT_CPR_PREFIX_LZFSE_LOW_BLOCKSIZE);

		if (ratio == ABSTRACT_CPR_PARAM_FASTZLIB_LZFSE_DEFAULT_BLOCKSIZE)
			return abstract_init_compress_fast_zlib(strm, 10, ABSTRACT_CPR_PREFIX_LZFSE_DEFAULT_BLOCKSIZE);

		if (ratio == ABSTRACT_CPR_PARAM_FASTZLIB_LZFSE_HIGH_BLOCKSIZE)
			return abstract_init_compress_fast_zlib(strm, 10, ABSTRACT_CPR_PREFIX_LZFSE_HIGH_BLOCKSIZE);
#endif
#endif


#ifdef COMPRESS_LZHAM
    if ((ratio >= ABSTRACT_CPR_PARAM_LZHAM_22_1) && (ratio <= ABSTRACT_CPR_PARAM_LZHAM_22_9))
      return abstract_init_compress_LzHam_with_prefix_value(strm, (int)(ratio - ABSTRACT_CPR_PARAM_LZHAM_22_1) + 1, ABSTRACT_CPR_PREFIX_LZHAM_DICT_22);

    if ((ratio >= ABSTRACT_CPR_PARAM_LZHAM_23_1) && (ratio <= ABSTRACT_CPR_PARAM_LZHAM_23_9))
      return abstract_init_compress_LzHam_with_prefix_value(strm, (int)(ratio - ABSTRACT_CPR_PARAM_LZHAM_23_1) + 1, ABSTRACT_CPR_PREFIX_LZHAM_DICT_23);

    if ((ratio >= ABSTRACT_CPR_PARAM_LZHAM_24_1) && (ratio <= ABSTRACT_CPR_PARAM_LZHAM_24_9))
      return abstract_init_compress_LzHam_with_prefix_value(strm, (int)(ratio - ABSTRACT_CPR_PARAM_LZHAM_24_1) + 1, ABSTRACT_CPR_PREFIX_LZHAM_DICT_24);

    if ((ratio >= ABSTRACT_CPR_PARAM_LZHAM_26_1) && (ratio <= ABSTRACT_CPR_PARAM_LZHAM_26_9))
      return abstract_init_compress_LzHam_with_prefix_value(strm, (int)(ratio - ABSTRACT_CPR_PARAM_LZHAM_26_1) + 1, ABSTRACT_CPR_PREFIX_LZHAM_DICT_26);
#endif

#ifdef COMPRESS_ZSTD
		if ((ratio >= ABSTRACT_CPR_PARAM_ZSTANDARD_1) && (ratio <= ABSTRACT_CPR_PARAM_ZSTANDARD_22))
			return abstract_init_compress_zstd_with_prefix(strm, (int)(ratio - ABSTRACT_CPR_PARAM_ZSTANDARD_1) + 1, 0);

		if ((ratio >= ABSTRACT_CPR_PARAM_ZSTANDARD_PREFIXED_1) && (ratio <= ABSTRACT_CPR_PARAM_ZSTANDARD_PREFIXED_22))
			return abstract_init_compress_zstd_with_prefix(strm, (int)(ratio - ABSTRACT_CPR_PARAM_ZSTANDARD_PREFIXED_1) + 1, 1);
#ifdef COMPRESS_ZSTD_MT
		if ((ratio >= ABSTRACT_CPR_PARAM_ZSTANDARD_MT_1) && (ratio <= ABSTRACT_CPR_PARAM_ZSTANDARD_MT_22))
			return abstract_init_compress_zstd_mt_with_prefix(strm, (int)(ratio - ABSTRACT_CPR_PARAM_ZSTANDARD_MT_1) + 1, 0, 1, 0);

		if ((ratio >= ABSTRACT_CPR_PARAM_ZSTANDARD_VERYLONG_MT_1) && (ratio <= ABSTRACT_CPR_PARAM_ZSTANDARD_VERYLONG_MT_22))
			return abstract_init_compress_zstd_mt_with_prefix(strm, (int)(ratio - ABSTRACT_CPR_PARAM_ZSTANDARD_VERYLONG_MT_1) + 1, 2, 1, 0);

		if ((ratio >= ABSTRACT_CPR_PARAM_ZSTANDARD_VERYLONG_1) && (ratio <= ABSTRACT_CPR_PARAM_ZSTANDARD_VERYLONG_22))
			return abstract_init_compress_zstd_mt_with_prefix(strm, (int)(ratio - ABSTRACT_CPR_PARAM_ZSTANDARD_VERYLONG_1) + 1, 2, 0, 0);

		if ((ratio >= ABSTRACT_CPR_PARAM_ZSTANDARD_BIS_1) && (ratio <= ABSTRACT_CPR_PARAM_ZSTANDARD_BIS_22))
			return abstract_init_compress_zstd_mt_with_prefix(strm, (int)(ratio - ABSTRACT_CPR_PARAM_ZSTANDARD_BIS_1) + 1, 0, 0, 0);

		if ((ratio >= ABSTRACT_CPR_PARAM_ZSTANDARD_LONG_1) && (ratio <= ABSTRACT_CPR_PARAM_ZSTANDARD_LONG_22))
			return abstract_init_compress_zstd_mt_with_prefix(strm, (int)(ratio - ABSTRACT_CPR_PARAM_ZSTANDARD_LONG_1) + 1, 1, 0, 0);

		if ((ratio >= ABSTRACT_CPR_PARAM_ZSTANDARD_LONG_MT_1) && (ratio <= ABSTRACT_CPR_PARAM_ZSTANDARD_LONG_MT_22))
			return abstract_init_compress_zstd_mt_with_prefix(strm, (int)(ratio - ABSTRACT_CPR_PARAM_ZSTANDARD_LONG_MT_1) + 1, 1, 1, 0);
#endif
#endif

#ifdef COMPRESS_LZ4
		if ((ratio >= ABSTRACT_CPR_PARAM_LZ4_16_MIN12) && (ratio <= ABSTRACT_CPR_PARAM_LZ4_16_12))
			return abstract_init_compress_lz4_complevel(strm, (((int)(ratio))-ABSTRACT_CPR_PARAM_LZ4_16_MIN12) + 1,16,0);

		if ((ratio >= ABSTRACT_CPR_PARAM_LZ4_20_MIN12) && (ratio <= ABSTRACT_CPR_PARAM_LZ4_20_12))
			return abstract_init_compress_lz4_complevel(strm, (((int)(ratio))-ABSTRACT_CPR_PARAM_LZ4_20_MIN12) + 1,20,0);

		if ((ratio >= ABSTRACT_CPR_PARAM_LZ4_22_MIN12) && (ratio <= ABSTRACT_CPR_PARAM_LZ4_22_12))
			return abstract_init_compress_lz4_complevel(strm, (((int)(ratio))-ABSTRACT_CPR_PARAM_LZ4_22_MIN12) + 1,22,0);

		if ((ratio >= ABSTRACT_CPR_PARAM_LZ4FDEC_16_MIN12) && (ratio <= ABSTRACT_CPR_PARAM_LZ4FDEC_16_12))
			return abstract_init_compress_lz4_complevel(strm, (((int)(ratio))-ABSTRACT_CPR_PARAM_LZ4FDEC_16_MIN12) + 1,16,1);

		if ((ratio >= ABSTRACT_CPR_PARAM_LZ4FDEC_20_MIN12) && (ratio <= ABSTRACT_CPR_PARAM_LZ4FDEC_20_12))
			return abstract_init_compress_lz4_complevel(strm, (((int)(ratio))-ABSTRACT_CPR_PARAM_LZ4FDEC_20_MIN12) + 1,20,1);

		if ((ratio >= ABSTRACT_CPR_PARAM_LZ4FDEC_22_MIN12) && (ratio <= ABSTRACT_CPR_PARAM_LZ4FDEC_22_12))
			return abstract_init_compress_lz4_complevel(strm, (((int)(ratio))-ABSTRACT_CPR_PARAM_LZ4FDEC_22_MIN12) + 1,22,1);
#endif

		if ((ratio>=ABSTRACT_CPR_PARAM_UNPREFIXED_GZIP_0) && (ratio<=ABSTRACT_CPR_PARAM_UNPREFIXED_GZIP_9))
			return abstract_init_compress_inflate_with_gzip_header(strm,(int)ratio - ABSTRACT_CPR_PARAM_UNPREFIXED_GZIP_0);

		if (ratio == ABSTRACT_CPR_PARAM_ZLIB_STORE)
			return abstract_init_compress_inflate_withNegMaxWBits_prefix(strm, 0);

		if (isStandardzLibWithNegMaxWBits)
			return abstract_init_compress_inflate_withNegMaxWBits_prefix(strm,ratio);
		else
			return abstract_init_compress_inflate_withoutNegMaxWBits(strm,ratio);
}

const char* abstract_init_compress_autoselect_info(unsigned int ratio,unsigned pos)
{
#ifdef COMPRESS_BZIP2
		if ((ratio>=21) && (ratio<=29))
		{
			switch (pos) {
			case ABSTR_CPR_POS_FORMAT_NAME: return "bzip2";
			case ABSTR_CPR_POS_FORMAT_EXTENSION: return "bz2";
			case ABSTR_CPR_POS_FORMAT_MIME: return "application/x-bzip2";
			default: return "";
			}
		}
#endif

#ifdef COMPRESS_ZSTD
		if (((ratio >= ABSTRACT_CPR_PARAM_ZSTANDARD_1) && (ratio <= ABSTRACT_CPR_PARAM_ZSTANDARD_22)) ||
		    ((ratio >= ABSTRACT_CPR_PARAM_ZSTANDARD_PREFIXED_1) && (ratio <= ABSTRACT_CPR_PARAM_ZSTANDARD_PREFIXED_22))  ||
		    ((ratio >= ABSTRACT_CPR_PARAM_ZSTANDARD_MT_1) && (ratio <= ABSTRACT_CPR_PARAM_ZSTANDARD_MT_22)) ||
		    ((ratio >= ABSTRACT_CPR_PARAM_ZSTANDARD_VERYLONG_MT_1) && (ratio <= ABSTRACT_CPR_PARAM_ZSTANDARD_VERYLONG_MT_22)) ||
		    ((ratio >= ABSTRACT_CPR_PARAM_ZSTANDARD_VERYLONG_1) && (ratio <= ABSTRACT_CPR_PARAM_ZSTANDARD_VERYLONG_22)) ||
		    ((ratio >= ABSTRACT_CPR_PARAM_ZSTANDARD_BIS_1) && (ratio <= ABSTRACT_CPR_PARAM_ZSTANDARD_BIS_22)) ||
		    ((ratio >= ABSTRACT_CPR_PARAM_ZSTANDARD_LONG_1) && (ratio <= ABSTRACT_CPR_PARAM_ZSTANDARD_LONG_22)) ||
		    ((ratio >= ABSTRACT_CPR_PARAM_ZSTANDARD_LONG_MT_1) && (ratio <= ABSTRACT_CPR_PARAM_ZSTANDARD_LONG_MT_22)))
		{
			switch (pos) {
			case ABSTR_CPR_POS_FORMAT_NAME: return "zstd";
			case ABSTR_CPR_POS_FORMAT_EXTENSION: return "zst";
			case ABSTR_CPR_POS_FORMAT_MIME: return "application/zstd";
			default: return "";
			}
		}
#endif
#ifdef COMPRESS_XZUTILS
		if (((ratio>=31) && (ratio<=39)) ||
            ((ratio>=41) && (ratio<=49)) ||
            ((ratio>=51) && (ratio<=59)) ||
            ((ratio>=61) && (ratio<=69)) ||
            ((ratio >= ABSTRACT_CPR_PARAM_UNPREFIXED_XZ_1) && (ratio <= ABSTRACT_CPR_PARAM_UNPREFIXED_XZ_9)) ||
		    ((ratio >= ABSTRACT_CPR_PARAM_UNPREFIXED_XZ_EXTREME_1) && (ratio <= ABSTRACT_CPR_PARAM_UNPREFIXED_XZ_EXTREME_9)) ||
		    ((ratio >= ABSTRACT_CPR_PARAM_UNPREFIXED_XZ_MT_0) && (ratio <= ABSTRACT_CPR_PARAM_UNPREFIXED_XZ_MT_9)) ||
		    ((ratio >= ABSTRACT_CPR_PARAM_UNPREFIXED_XZ_EXTREME_MT_0) && (ratio <= ABSTRACT_CPR_PARAM_UNPREFIXED_XZ_EXTREME_MT_9)))
		{
			switch (pos) {
			case ABSTR_CPR_POS_FORMAT_NAME: return "xz";
			case ABSTR_CPR_POS_FORMAT_EXTENSION: return "xz";
			case ABSTR_CPR_POS_FORMAT_MIME: return "application/x-xz";
			default: return "";
			}
		}
#endif



#ifdef COMPRESS_LZ4
		if (((ratio >= ABSTRACT_CPR_PARAM_LZ4_16_MIN12) && (ratio <= ABSTRACT_CPR_PARAM_LZ4_16_12)) ||
		    ((ratio >= ABSTRACT_CPR_PARAM_LZ4_20_MIN12) && (ratio <= ABSTRACT_CPR_PARAM_LZ4_20_12)) ||
		    ((ratio >= ABSTRACT_CPR_PARAM_LZ4_22_MIN12) && (ratio <= ABSTRACT_CPR_PARAM_LZ4_22_12)) ||
		    ((ratio >= ABSTRACT_CPR_PARAM_LZ4FDEC_16_MIN12) && (ratio <= ABSTRACT_CPR_PARAM_LZ4FDEC_16_12)) ||
		    ((ratio >= ABSTRACT_CPR_PARAM_LZ4FDEC_20_MIN12) && (ratio <= ABSTRACT_CPR_PARAM_LZ4FDEC_20_12)) ||
		    ((ratio >= ABSTRACT_CPR_PARAM_LZ4FDEC_22_MIN12) && (ratio <= ABSTRACT_CPR_PARAM_LZ4FDEC_22_12)))
		{
			switch (pos) {
			case ABSTR_CPR_POS_FORMAT_NAME: return "lz4";
			case ABSTR_CPR_POS_FORMAT_EXTENSION: return "lz4";
			case ABSTR_CPR_POS_FORMAT_MIME: return "application/octet-stream";
			default: return "";
			}
		}
#endif

		if ((ratio>=ABSTRACT_CPR_PARAM_UNPREFIXED_GZIP_0) && (ratio<=ABSTRACT_CPR_PARAM_UNPREFIXED_GZIP_9))
		{
			switch (pos) {
			case ABSTR_CPR_POS_FORMAT_NAME: return "gzip";
			case ABSTR_CPR_POS_FORMAT_EXTENSION: return "gz";
			case ABSTR_CPR_POS_FORMAT_MIME: return "application/gzip";
			default: return "";
			}
		}

		return NULL;
}

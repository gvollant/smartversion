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
#include <stdio.h>

#include <memory.h>



#include "../common/difbasic.h"
#include "abstractCompress.h"

#include "../common/difstrm.h"
#include "../common/difstrmi.h"
#include "../common/DfsTlTyp.h"
#include "../common/difstool.h"
#include "MkDifStm.h"


#ifndef DEF_MEM_LEVEL
#if MAX_MEM_LEVEL >= 8
#  define DEF_MEM_LEVEL 8
#else
#  define DEF_MEM_LEVEL  MAX_MEM_LEVEL
#endif
#endif

int InitOutPatchStream(OUT_MULTI_BUFFER *pOutPatchStream,dfuLong32 CompressRatio, dfuLong32 dfDefaultSizeBufStreamKB)
{
  dfuLong32 i;
  int err;
  if (dfDefaultSizeBufStreamKB==0) dfDefaultSizeBufStreamKB=1;
  pOutPatchStream->sizeBufStream = dfDefaultSizeBufStreamKB * 1024;

  pOutPatchStream->outBufferPos = 0;
  pOutPatchStream->outBufferSize = pOutPatchStream->sizeBufStream * 2;
  pOutPatchStream->outBuffer =
    (dfbytep) DfsMalloc((size_t) (pOutPatchStream->outBufferSize));
  pOutPatchStream->in_flush = 0;
  pOutPatchStream->fLongHeaderStreamFormat = FALSE;
  pOutPatchStream->fCompressBzip2 = FALSE;
  pOutPatchStream->fCompressPrefix = FALSE;
  pOutPatchStream->fCompressXZUtils = FALSE;
  pOutPatchStream->total_out_internal = 0;
  pOutPatchStream->uncompressed_stream_pos = 0;
  pOutPatchStream->max_uncompressed_all_stream_to_be_buffered = 0;

  pOutPatchStream->max_uncompressed_all_stream_to_be_buffered =
                    pOutPatchStream->sizeBufStream * 16/4;//+++---


  if (CompressRatio>=21)
	  pOutPatchStream->fCompressPrefix=TRUE;

  if (pOutPatchStream->outBuffer == NULL)
  {
    //DfsFree(internal);
    return DSERR_INTERNAL;
  }

  pOutPatchStream->nextStreamToFlush = FIRST_STREAM;

  for (i = FIRST_STREAM; i <= LAST_STREAM; i++)
  {
    clear_abstract_compress_stream(&pOutPatchStream->WriteStream[i].zstream);

    pOutPatchStream->WriteStream[i].posBufStream = 0;
    pOutPatchStream->WriteStream[i].totalDataWritten = 0;
    pOutPatchStream->WriteStream[i].total_compressed = 0;
    pOutPatchStream->WriteStream[i].total_uncompressed = 0;
    pOutPatchStream->WriteStream[i].uncompressed_all_stream_pos_last_flush = 0;
    pOutPatchStream->WriteStream[i].streamBuf =
      (dfbytep) DfsMalloc((size_t) (pOutPatchStream->sizeBufStream));
    /* ##$$++**-- */


    if (pOutPatchStream->fLongHeaderStreamFormat)
    {
        if (pOutPatchStream->WriteStream[i].streamBuf != NULL)
          err = abstract_init_compress_inflate_withoutNegMaxWBits(&pOutPatchStream->WriteStream[i].zstream,
                            (int) (CompressRatio >=
                                   6) ? 9 : ABSTR_COMPRESS_Z_DEFAULT_COMPRESSION);
    }
	else if (!pOutPatchStream->fCompressPrefix)
    {
        if (pOutPatchStream->WriteStream[i].streamBuf != NULL)
          err = abstract_init_compress_inflate_withNegMaxWBits(&pOutPatchStream->WriteStream[i].zstream,
                             (int) (CompressRatio >= 6) ? 9 : ABSTR_COMPRESS_Z_DEFAULT_COMPRESSION);
    }
	else
	{
		int CompressRatioThis = (int)CompressRatio;
		if ((CompressRatioThis>=51) && (CompressRatioThis<=59) && (i == SEQINFO_STREAM))
			CompressRatioThis=9;
    if ((CompressRatioThis & 0x40000000) != 0)
    {
      if (i == SEQINFO_STREAM)
        CompressRatioThis = CompressRatioThis & 0xffff;
      else
        CompressRatioThis = (CompressRatioThis & 0x3fff0000) / 0x10000;
    }
        if (pOutPatchStream->WriteStream[i].streamBuf != NULL)
          err = abstract_init_compress_autoselect(&pOutPatchStream->WriteStream[i].zstream,
                             CompressRatioThis,1);
	}

    if ((err != ABSTR_COMPRESS_Z_OK) || (pOutPatchStream->WriteStream[i].streamBuf == NULL))
    {
      dfuLong32 j;

      if (pOutPatchStream->WriteStream[i].streamBuf != NULL)
        DfsFree(pOutPatchStream->WriteStream[i].streamBuf);
      for (j = FIRST_STREAM; j < i; j++)
      {
        abstract_compress_end(&pOutPatchStream->WriteStream[j].zstream);
        DfsFree(pOutPatchStream->WriteStream[j].streamBuf);
      }
      DfsFree(pOutPatchStream->outBuffer);
      /* DfsFree(internal); */
      return DSERR_INTERNAL;
    }
  }

#if (defined (SHOWSTREAM) || defined (SHOWSTREAMEND)) && defined(NOQUIET)
  printf("MEM: out pOutPatchStream->outBufferSize = %u\n",pOutPatchStream->outBufferSize );
  printf("          param pOutPatchStream->max_uncompressed_all_stream_to_be_buffered=%u\n\n",pOutPatchStream->max_uncompressed_all_stream_to_be_buffered);
#endif

  pOutPatchStream->latest_written_seq_is_ins = 0;
  pOutPatchStream->latest_written_seq_size = 0;
  pOutPatchStream->latest_written_seq_pos = 0;
  pOutPatchStream->latest_size_of_written_seq = 0;
  pOutPatchStream->latest_written_seq_posCurOrg = 0;


  pOutPatchStream->latest_write_out_pos = 0;
  pOutPatchStream->write_out_insert_size = 0;
  pOutPatchStream->write_out_size_depl_in_place = 0;
  pOutPatchStream->write_out_size_depl_out_place = 0;

  return DSERR_OK;
}


void ReleaseOutPatchStream(OUT_MULTI_BUFFER *pOutPatchStream)
{
  int i;
  for (i = FIRST_STREAM; i <= LAST_STREAM; i++)
  {
#if (defined (SHOWSTREAM) || defined (SHOWSTREAMEND)) && defined(NOQUIET)
    printf("stream %d :\tuncompr= %u\tcompr= %u\n", i,
           (unsigned int)pOutPatchStream->WriteStream[i].zstream.total_in,
           (unsigned int)pOutPatchStream->WriteStream[i].zstream.total_out);
#endif
    abstract_compress_end(&pOutPatchStream->WriteStream[i].zstream);
    DfsFree(pOutPatchStream->WriteStream[i].streamBuf);
  }
	  DfsFree(pOutPatchStream->outBuffer);
}


void DoFlushWork(OUT_MULTI_BUFFER *pOutPatchStream,
                 OUT_DATA_STREAM* pOutDataStream,
                 dfuLong32 dfuThis)
{
  dfuLong32 i;
  {
    dfbytep dfSource = (dfbytep) pOutPatchStream->outBuffer;
    dfbytep dfDest = (dfbytep) pOutDataStream->next_out;
    for (i = 0; i < dfuThis; i++)
      *(dfDest++) = *(dfSource++);

    pOutDataStream->next_out = ((dfbytep) pOutDataStream->next_out) + dfuThis;
    pOutDataStream->avail_out -= dfuThis;
    pOutDataStream->total_out += dfuThis;
    pOutDataStream->done_latest_out += dfuThis;
    pOutPatchStream->total_out_internal += dfuThis;
  }

  for (i = dfuThis; i < pOutPatchStream->outBufferPos; i++)
    *((pOutPatchStream->outBuffer) + (i - dfuThis)) = *((pOutPatchStream->outBuffer) + i);

  DFASSERT(pOutPatchStream->outBufferPos <= pOutPatchStream->outBufferSize);
  pOutPatchStream->outBufferPos -= dfuThis;
  DFASSERT(pOutPatchStream->outBufferPos <= pOutPatchStream->outBufferSize);
}


void FlushOutInStream(OUT_MULTI_BUFFER *pOutPatchStream,OUT_DATA_STREAM* pOutDataStream)
{
  dfuLong32 dfuThis;

  if (pOutPatchStream->outBufferPos == 0)
      return;

  if (pOutDataStream->avail_out < pOutPatchStream->outBufferPos)
    dfuThis = pOutDataStream->avail_out;
  else
    dfuThis = pOutPatchStream->outBufferPos;

  if (dfuThis != 0)
      DoFlushWork(pOutPatchStream,pOutDataStream,dfuThis);
}


int NeedMoreOutSpaceForFlushing(OUT_MULTI_BUFFER *pOutPatchStream,OUT_DATA_STREAM* pOutDataStream)
{
    if (pOutDataStream->avail_out == 0)
        if (pOutPatchStream->outBufferPos > 0)
            return 1;
    return 0;
}

/****++*/
int EnlargeOutBuffer(OUT_MULTI_BUFFER *pOutPatchStream)
{
  dfbytep new_out;
  int err;

  new_out = (dfbytep) DfsRealloc(pOutPatchStream->outBuffer,
                                 (size_t) (pOutPatchStream->outBufferSize +
                                           pOutPatchStream->sizeBufStream));

  if (new_out != NULL)
  {
    pOutPatchStream->outBuffer = new_out;
    pOutPatchStream->outBufferSize += pOutPatchStream->sizeBufStream;
    err = DSERR_OK;
  }
  else
    err = DSERR_INTERNAL;

  return err;
}

int WriteOutBuffer(OUT_MULTI_BUFFER *pOutPatchStream, dfvoidp data, dfuLong32 size)
{
  int err = DSERR_OK;

  while ((size + pOutPatchStream->outBufferPos > pOutPatchStream->outBufferSize) &&
         (err == DSERR_OK))
    err = EnlargeOutBuffer(pOutPatchStream);

  if (err == DSERR_OK)
  {
    dfuLong32 i;
    dfbytep dfSource = (dfbytep) data;
    dfbytep dfDest = pOutPatchStream->outBuffer + pOutPatchStream->outBufferPos;
    for (i = 0; i < size; i++)
      *(dfDest++) = *(dfSource++);

    DFASSERT(pOutPatchStream->outBufferPos <= pOutPatchStream->outBufferSize);
    pOutPatchStream->outBufferPos += size;
    DFASSERT(pOutPatchStream->outBufferPos <= pOutPatchStream->outBufferSize);
  }

  return err;
}

void GetBufferSituation(OUT_MULTI_BUFFER *pOutPatchStream, int nbStream,
                        dfuLong32 *pRemovableInBufferSize,
                        dfuLong32 *pWrittableInBufferWithoutFlushSize)
{
  WR_UNIT_STREAM *wtr;
  wtr = &(pOutPatchStream->WriteStream[nbStream]);
  if (pRemovableInBufferSize != NULL)
      *pRemovableInBufferSize = wtr->posBufStream;

  if (pWrittableInBufferWithoutFlushSize != NULL)
      *pWrittableInBufferWithoutFlushSize =
               (pOutPatchStream->sizeBufStream) - (wtr->posBufStream);
}

void RemoveTopDataInStream(OUT_MULTI_BUFFER *pOutPatchStream, int nbStream,
                        dfuLong32 dfNbData)
{
  WR_UNIT_STREAM *wtr;
  wtr = &(pOutPatchStream->WriteStream[nbStream]);
  wtr->posBufStream -= dfNbData;
  wtr->totalDataWritten -= dfNbData;
}

/* FlushWriteStreamData : flush a stream
*/
int FlushWriteStreamData(OUT_MULTI_BUFFER *pOutPatchStream, OUT_DATA_STREAM* pOutDataStream, int nbStream, int flush)
{
  int err = DSERR_OK;
  WR_UNIT_STREAM *wtr;
  dfbyte Head[8];
  int iPosInHead;
  dfuLong32 flushing;
  dfuLong32 outBufferPosBeforeFlush;
  int deflate_workdone;
  int headerSize,headerPacketSizeSize ;

  FlushOutInStream(pOutPatchStream,pOutDataStream);


  wtr = &(pOutPatchStream->WriteStream[nbStream]);

  wtr->zstream.next_in = wtr->streamBuf;
  wtr->zstream.avail_in = wtr->posBufStream;

  outBufferPosBeforeFlush = pOutPatchStream->outBufferPos;


  /* if we flush with no data, and no data where written before, we just write a
         SMALLHEADER_SIZE_VALUE0 chunk*/
  if ((!pOutPatchStream->fLongHeaderStreamFormat) &&
      (wtr->zstream.avail_in == 0) &&
      (wtr->total_compressed == 0) &&
      (wtr->total_uncompressed == 0) &&
      (flush == ABSTR_COMPRESS_Z_FINISH))
  {
      difstr_putValue_inmemory(&(Head[0]), SMALLHEADER_SIZE_VALUE0 |
                                           SMALLHEADER_SIGN_VALUE |
                                           SMALLHEADER_ENDSTREAM_MASK | nbStream, 1);
      err = WriteOutBuffer(pOutPatchStream, Head, 1);

      return err;
  }

  if (pOutPatchStream->fLongHeaderStreamFormat)
  {
      flushing = (flush == ABSTR_COMPRESS_Z_FINISH) ? ENDOFSTREAM_MASK : 0;
      iPosInHead = 4;
      headerPacketSizeSize=4;
      difstr_putValue_inmemory(&(Head[0]), nbStream | flushing, 4);
  }
  else
  {
	  dfuLong32 abstract_decompressor = (pOutPatchStream->fCompressPrefix) ? SMALLHEADER_ASTRACT_DECOMPRESSOR : 0;
      iPosInHead = 1;
      flushing = (flush == ABSTR_COMPRESS_Z_FINISH) ? SMALLHEADER_ENDSTREAM_MASK : 0;

/*
      if (wtr->total_uncompressed < 0xffffffff)
          if (wtr->total_uncompressed != wtr->zstream.total_in)
              printf("---\n+++\n");
*/

      if ((wtr->total_uncompressed + wtr->zstream.avail_in) < 0x80)
      {
        headerPacketSizeSize=1;
        difstr_putValue_inmemory(&(Head[0]), SMALLHEADER_SIZE_VALUE1 |
                                             SMALLHEADER_SIGN_VALUE |
                                             flushing | nbStream | abstract_decompressor, 1);
      }
      else
      {
        headerPacketSizeSize=4;
        difstr_putValue_inmemory(&(Head[0]), SMALLHEADER_SIZE_VALUE4 |
                                             SMALLHEADER_SIGN_VALUE | flushing | nbStream | abstract_decompressor, 1);
      }
  }
  /* difstr_putValue_inmemory(&(Head[iPosInHead]), 0x11223345, headerPacketSizeSize); */
  headerSize = headerPacketSizeSize + iPosInHead;

  err = WriteOutBuffer(pOutPatchStream, Head, headerSize);

  do
  {
    deflate_workdone = 0;
    if ((pOutPatchStream->outBufferSize == pOutPatchStream->outBufferPos) &&
        (err == DSERR_OK))
      err = EnlargeOutBuffer(pOutPatchStream);

    if (err == DSERR_OK)
    {
      int errDeflate;

      wtr->zstream.next_out = pOutPatchStream->outBuffer + pOutPatchStream->outBufferPos;
      wtr->zstream.avail_out = pOutPatchStream->outBufferSize -
        pOutPatchStream->outBufferPos;

      DFASSERT(pOutPatchStream->outBufferPos <= pOutPatchStream->outBufferSize);
      #if defined(SHOWSTREAM) && defined(NOQUIET)
      printf("deflate stream%2u ,avail bef=%6d",
             nbStream, wtr->zstream.avail_out);
      #endif

      {
          dfuLong32 avail_in_before = wtr->zstream.avail_in;
          dfuLong32 avail_out_before = wtr->zstream.avail_out;
          errDeflate = abstract_compress(&wtr->zstream, flush);
          wtr->total_compressed += (avail_out_before - wtr->zstream.avail_out);
          wtr->total_uncompressed += (avail_in_before - wtr->zstream.avail_in);
          pOutPatchStream->uncompressed_stream_pos += (avail_in_before - wtr->zstream.avail_in);
          wtr->uncompressed_all_stream_pos_last_flush = pOutPatchStream->uncompressed_stream_pos;
      }
      if ((errDeflate != ABSTR_COMPRESS_Z_STREAM_END) && (errDeflate != ABSTR_COMPRESS_Z_OK))
          err = DSERR_INTERNAL;
      #if defined(SHOWSTREAM) && defined(NOQUIET)
      printf(" err=%4d ,avail after=%6d", errDeflate, wtr->zstream.avail_out);
      #endif

      pOutPatchStream->outBufferPos = pOutPatchStream->outBufferSize -
        wtr->zstream.avail_out;
      DFASSERT(pOutPatchStream->outBufferPos <= pOutPatchStream->outBufferSize);

      if (flush != ABSTR_COMPRESS_Z_FINISH)
        deflate_workdone = wtr->zstream.avail_in == 0;
      else
      {
        deflate_workdone = (errDeflate == ABSTR_COMPRESS_Z_STREAM_END);
        if (errDeflate == ABSTR_COMPRESS_Z_STREAM_END)
          err = DSERR_OK;
      }
    }
  }
  while ((err == DSERR_OK) && (!deflate_workdone));

  //if (flush==ABSTR_COMPRESS_Z_FINISH) printf("\nstream %u: cpr=%08x%08x uncpr=%08x%08x",nbStream,((dfuLong32)((wtr->total_compressed)>>32)),((dfuLong32)wtr->total_compressed),((dfuLong32)((wtr->total_uncompressed)>>32)),((dfuLong32)wtr->total_uncompressed));//++--@@@

  /* if no data flushed, remove header */
  if (pOutPatchStream->outBufferPos == outBufferPosBeforeFlush + headerSize)
    pOutPatchStream->outBufferPos = outBufferPosBeforeFlush;

  if (err == DSERR_OK)
  {
    if (pOutPatchStream->outBufferPos != outBufferPosBeforeFlush)
    {
      difstr_putValue_inmemory(pOutPatchStream->outBuffer +
                                 outBufferPosBeforeFlush + iPosInHead,
                               pOutPatchStream->outBufferPos -
                                 (outBufferPosBeforeFlush + headerSize),
                               headerPacketSizeSize);
#if defined(SHOWSTREAM) && defined(NOQUIET)
      printf(" Strm:%d,size=%d\n", nbStream, pOutPatchStream->outBufferPos -
             (outBufferPosBeforeFlush + headerSize));
#endif
    }
    else
    {
#if defined(SHOWSTREAM) && defined(NOQUIET)
      printf("\n");
#endif
    }
    FlushOutInStream(pOutPatchStream,pOutDataStream);
    wtr->posBufStream = 0;
  }

  return err;
}

int FlushWriteStreamDataWithCheckOtherStream(OUT_MULTI_BUFFER *pOutPatchStream, OUT_DATA_STREAM* pOutDataStream, int nbStream, int flush)
{
    int err = DSERR_OK;
    dfuLong32 i;
    if ((pOutPatchStream->max_uncompressed_all_stream_to_be_buffered != 0) &&
        (pOutPatchStream->uncompressed_stream_pos >= pOutPatchStream->max_uncompressed_all_stream_to_be_buffered))
    {
        dfuLong64 pos_need_flush;
        pos_need_flush = pOutPatchStream->uncompressed_stream_pos - pOutPatchStream->max_uncompressed_all_stream_to_be_buffered;
        for (i = FIRST_STREAM; i <= LAST_STREAM; i++)
            if ((i!=nbStream) &&
                (pOutPatchStream->WriteStream[i].uncompressed_all_stream_pos_last_flush
                                 <= pos_need_flush))
            {
                err = FlushWriteStreamData(pOutPatchStream, pOutDataStream, i, flush);
                if (err != DSERR_OK)
                    break;
            }
    }

    if (err == DSERR_OK)
        err = FlushWriteStreamData(pOutPatchStream, pOutDataStream, nbStream, flush);
    return err;
}

int WriteStreamData(OUT_MULTI_BUFFER *pOutPatchStream, OUT_DATA_STREAM* pOutDataStream, int nbStream,
                dfvoidp data, dfuLong64 size)
{
  WR_UNIT_STREAM *wtr;
  dfuLong32 dfuThis, dfuFree, i;
  dfbytep dfSource, dfDest;
  int err = DSERR_OK;

  if (IsFinalFlush(pOutPatchStream)==1)
      return DSERR_INTERNAL;

  wtr = &(pOutPatchStream->WriteStream[nbStream]);
  wtr->totalDataWritten += size;

  DFASSERT(wtr->posBufStream <= pOutPatchStream->sizeBufStream);

  while ((size > 0) && (err == DSERR_OK))
  {
    dfuFree = (pOutPatchStream->sizeBufStream) - (wtr->posBufStream);
    if (size > dfuFree)
      dfuThis = dfuFree;
    else
      dfuThis = (dfuLong32)size;

    dfSource = (dfbytep) data;
    dfDest = wtr->streamBuf + wtr->posBufStream;
    for (i = 0; i < dfuThis; i++)
      *(dfDest++) = *(dfSource++);


    DFASSERT(wtr->posBufStream <= pOutPatchStream->sizeBufStream);
    DFASSERT((wtr->posBufStream + dfuThis) <= pOutPatchStream->sizeBufStream);

    wtr->posBufStream += dfuThis;


    size -= dfuThis;
    data = (dfvoidp) (((dfbytep) data) + dfuThis);

    if (wtr->posBufStream == pOutPatchStream->sizeBufStream)
      err = FlushWriteStreamDataWithCheckOtherStream(pOutPatchStream, pOutDataStream, nbStream,
                                 ABSTR_COMPRESS_Z_SYNC_FLUSH /*Z_NO_FLUSH */ );
  }

  DFASSERT(wtr->posBufStream <= pOutPatchStream->sizeBufStream);

  return err;
}



int FinalFlushOutData(OUT_MULTI_BUFFER *pOutPatchStream, OUT_DATA_STREAM* pOutDataStream)
{
  int err=DSERR_OK;
  FlushOutInStream(pOutPatchStream,pOutDataStream);
  while ((pOutPatchStream->nextStreamToFlush <= LAST_STREAM) &&
         (pOutDataStream->avail_out > 0) &&
         (err == DSERR_OK))
  {
//printf("\ndo flush str %u\n",pOutPatchStream->nextStreamToFlush); // @@@

    err = FlushWriteStreamData(pOutPatchStream, pOutDataStream, pOutPatchStream->nextStreamToFlush, ABSTR_COMPRESS_Z_FINISH);

    pOutPatchStream->nextStreamToFlush++;
    FlushOutInStream(pOutPatchStream,pOutDataStream);
  }

  if ((pOutPatchStream->nextStreamToFlush == (LAST_STREAM+1)) && (err == DSERR_OK) &&
      (pOutPatchStream->outBufferPos == 0))
    err = DSERR_END;
  return err;
}

/***************************************************************************/
/***************************************************************************/

void SetInFinalFlush(OUT_MULTI_BUFFER *pOutPatchStream)
{
    pOutPatchStream->in_flush = 1;
}

dfuLong32 IsFinalFlush(OUT_MULTI_BUFFER *pOutPatchStream)
{
    return pOutPatchStream->in_flush;
}

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/


int GetNbByteNumber(dfuLong32 size)
{
  return (size < 0x100) ? 1 : (size < 0x10000) ? 2 :
    (size < 0x1000000) ? 3 : 4;
}

int GetNbByteNumber64(dfuLong64 value64)
{
    dfuLong32 valueLow = (dfuLong32)value64;
    dfuLong32 valueHigh = (dfuLong32)(value64>>32);
    if (valueHigh != 0)
        return GetNbByteNumber(valueHigh)+4;
    else
        return GetNbByteNumber(valueLow);
}

int GetNbBitNumber(dfuLong32 num)
{
  int nbbits = 0;
  dfuLong32 val = 1;
  while (num >= val)
  {
    nbbits++;
    val *= 2;
  }
  return nbbits;
}


int
WriteHeadSeq(OUT_MULTI_BUFFER *pOutPatchStream, OUT_DATA_STREAM* pOutDataStream, dfuLong32 highBit, dfuLong32 size,
             dfuLong32 *psize_of_written_seq)
{
  int err;
  dfbyte c;
  dfbyte bHighByte = (highBit != 0) ? ((dfbyte) 0x80) : 0;

  if ((size < 0x38) || ((size < 0x77) && (highBit != 0)))
  {
    c = ((dfbyte) size) | bHighByte;
    err = WriteStreamData(pOutPatchStream, pOutDataStream, SEQINFO_STREAM, &c, 1);
    *psize_of_written_seq = 1;
  }
  else
  {
    dfuLong32Intel sizeIntel;
    dfbyte bNbByteNb;
    bNbByteNb = GetNbByteNumber(size);

    c = ((dfbyte) (bNbByteNb + 0x7b)) | bHighByte;
    err = WriteStreamData(pOutPatchStream, pOutDataStream, SEQINFO_STREAM, &c, 1);
    sizeIntel = ConvertuLongToLongIntel(size);
    if (err == DSERR_OK)
    {
      err = WriteStreamData(pOutPatchStream, pOutDataStream, SEQINFO_STREAM,
                            &sizeIntel, bNbByteNb);
      *psize_of_written_seq = bNbByteNb+1;
    }
  }
#ifdef DO_STATIS
  {
    INTERNAL_WRITESTATE *internal;
    internal = (INTERNAL_WRITESTATE *) (write_stream->state);
    if (internal->statis[3] != NULL)
      AddValue(internal->statis[3], c);
  }
#endif
  return err;
}


int WriteHeadSeq64(OUT_MULTI_BUFFER *pOutPatchStream, OUT_DATA_STREAM* pOutDataStream,
                   dfuLong32 highBit, dfuLong64 size, dfuLong64 pos,
                   dfuLong32 *size_of_written_seq)
{
    dfbyte dfBuf[0x20];
    dfbyte dfNbBytesSize,dfNbBytesPos;
    dfuLong64Intel dfSizeIntel64 = ConvertuLongToLongIntel64(size) ;
    dfuLong64Intel dfPosIntel64 = ConvertuLongToLongIntel64(pos) ;
    int err;

    pOutPatchStream->latest_written_seq_size=0;

    dfBuf[0] = 0x7b;
    dfNbBytesSize=GetNbByteNumber64(size);
    dfNbBytesPos=GetNbByteNumber64(pos);
    if (dfNbBytesSize==0)
        dfNbBytesSize++;
    dfBuf[1] = ((highBit!=0) ? 0x80 : 00) | (((dfNbBytesSize-1) & 0x07) << 4) | (dfNbBytesPos & 0x0f);
    DfsMemcpy(&dfBuf[2],&dfSizeIntel64,dfNbBytesSize);
    DfsMemcpy(&dfBuf[2+dfNbBytesSize],&dfPosIntel64,dfNbBytesPos);
    /*
    printf("mk: size,pos=%x,%x, is_insert=%u - size,pos : %u-%u bytes\n",(dfuLong32)size ,(dfuLong32)pos,!highBit,dfNbBytesSize,dfNbBytesPos);
{ int i; for (i=0;i<2 + dfNbBytesSize + dfNbBytesPos;i++) printf("%02x ",dfBuf[i]); printf("\n"); }
*/
//printf("writeheadseq64 highbit=%u size=%08x%08x - pos=%08x%08x, seq sz=%u\n",highBit,((dfuLong32)(size>>32)),((dfuLong32)size),((dfuLong32)(pos>>32)),((dfuLong32)pos),2 + dfNbBytesSize + dfNbBytesPos); //++--@@@

    err = WriteStreamData(pOutPatchStream, pOutDataStream, SEQINFO_STREAM,
                            dfBuf, 2 + dfNbBytesSize + dfNbBytesPos);
    if (err == DSERR_OK)
        *size_of_written_seq = 2 + dfNbBytesSize + dfNbBytesPos;
    /* maximum size is 2 + 8 + 8 */
    return err;
}

int GetSteamAnalysis(const OUT_MULTI_BUFFER *pOutPatchStream, DFSPATCHANALYSEINFO_MEMORY* pDfsPatchAnalyseInfo)
{
    if ((pOutPatchStream == NULL) || (pDfsPatchAnalyseInfo == NULL))
        return DSERR_INTERNAL;
    pDfsPatchAnalyseInfo->total_size_depl_in_place = pOutPatchStream->write_out_size_depl_in_place;
    pDfsPatchAnalyseInfo->total_size_depl_out_place = pOutPatchStream->write_out_size_depl_out_place;
    pDfsPatchAnalyseInfo->total_size_insert = pOutPatchStream->write_out_insert_size;

    return DSERR_OK;
}

int WriteInsertBytesInStream(OUT_MULTI_BUFFER *pOutPatchStream, OUT_DATA_STREAM* pOutDataStream, dfvoidp data, dfuLong64 size)
{
  int err = DSERR_OK;
  dfuLong32 size_of_written_seq=0;
  dfuLong64 size_on_seq=size;


  pOutPatchStream->latest_write_out_pos += size;
  pOutPatchStream->write_out_insert_size += size;



  if (size > 0)
  {
#ifdef SHOWSEQDEBUG
    printf("insert %u bytes \n", size);
#endif

#ifdef DO_STATIS
    {
      INTERNAL_WRITESTATE *internal;
      internal = (INTERNAL_WRITESTATE *) (write_stream->state);
      if (internal->statis[0] != NULL)
        AddValue(internal->statis[0], size);

      write_byte(internal->tsthufout[0], 0);
      IncBits(internal->tsthufout[1], 1);

      if (size < 0xff)
      {
        write_byte(internal->tsthufout[0], (int) size);
        write_byte(internal->tsthufout[2], (int) size);
      }
      else
      {
        write_byte(internal->tsthufout[0], 0);
        write_byte(internal->tsthufout[2], 0);
        write_byte(internal->tsthufout[0], GetNbByteNumber(size));
        write_byte(internal->tsthufout[3], GetNbByteNumber(size));
        IncBits(internal->tsthufout[0], GetNbByteNumber(size) * 8);
        IncBits(internal->tsthufout[4], GetNbByteNumber(size) * 8);
      }
    }
#endif

    if (pOutPatchStream->latest_written_seq_is_ins==1)
    {
        dfuLong32 dfRemovableInBufferSize,dfWrittableInBufferWithoutFlushSize;
        GetBufferSituation(pOutPatchStream, SEQINFO_STREAM,
                            &dfRemovableInBufferSize,
                            &dfWrittableInBufferWithoutFlushSize);
        if ((dfRemovableInBufferSize>=pOutPatchStream->latest_size_of_written_seq) &&
            (dfWrittableInBufferWithoutFlushSize >= 2 + 8 + 1))
        {
            RemoveTopDataInStream(pOutPatchStream, SEQINFO_STREAM,
                                  pOutPatchStream->latest_size_of_written_seq);
            size_on_seq += pOutPatchStream->latest_written_seq_size;
        }
    }


    if ((size_on_seq>>32) == 0)
      err = WriteHeadSeq(pOutPatchStream, pOutDataStream, 0, (dfuLong32)size_on_seq, &size_of_written_seq);
    else
      err = WriteHeadSeq64(pOutPatchStream, pOutDataStream, 0, size_on_seq, 0, &size_of_written_seq);

    if (err == DSERR_OK)
      err = WriteStreamData(pOutPatchStream, pOutDataStream, INSBYTE_STREAM, data, size);

    /*
    {
      //INTERNAL_WRITESTATE *internal;
      //internal = (INTERNAL_WRITESTATE *) (write_stream->state);
      internal ->fFileIdentical = FALSE;
      internal->curPosIns += size;
      internal->curPos_newwritten += size;
    }
    */


      if (err == DSERR_OK)
      {
        pOutPatchStream->latest_written_seq_is_ins=1;
        pOutPatchStream->latest_written_seq_size=size_on_seq;
        pOutPatchStream->latest_written_seq_pos=0;
        pOutPatchStream->latest_size_of_written_seq = size_of_written_seq;

        FlushOutInStream(pOutPatchStream,pOutDataStream);
      }
  }

  return err;
}



int WriteInsertDeplInStream(OUT_MULTI_BUFFER *pOutPatchStream, OUT_DATA_STREAM* pOutDataStream,
                            dfuLong64 posCurOrg,
                            dfuLong64 posIntPtr, dfuLong64 size)
{
  int err = DSERR_OK;
  //INTERNAL_WRITESTATE *internal;
  dfuLong64 pos= (dfuLong64)posIntPtr; //6432 // 3264
  dfuLong32 size_of_written_seq=0;

  if (size == 0)
    return DSERR_OK;


  if (posIntPtr == pOutPatchStream->latest_write_out_pos)
     pOutPatchStream->write_out_size_depl_in_place += size;
  else
     pOutPatchStream->write_out_size_depl_out_place += size;
  pOutPatchStream->latest_write_out_pos += size;


    if (pOutPatchStream->latest_written_seq_is_ins==0)
        if (((pOutPatchStream->latest_written_seq_pos + pOutPatchStream->latest_written_seq_size)==pos)
          && (posCurOrg >= pOutPatchStream->latest_written_seq_size))
    {
        dfuLong32 dfRemovableInBufferSize,dfWrittableInBufferWithoutFlushSize;
        GetBufferSituation(pOutPatchStream, SEQINFO_STREAM,
                            &dfRemovableInBufferSize,
                            &dfWrittableInBufferWithoutFlushSize);
        if ((dfRemovableInBufferSize>=pOutPatchStream->latest_size_of_written_seq) &&
            (dfWrittableInBufferWithoutFlushSize >= 2 + 8 + 1))
        {
            RemoveTopDataInStream(pOutPatchStream, SEQINFO_STREAM,
                                  pOutPatchStream->latest_size_of_written_seq);
            pos -= pOutPatchStream->latest_written_seq_size;
            size += pOutPatchStream->latest_written_seq_size;
            posCurOrg = pOutPatchStream->latest_written_seq_posCurOrg;
        }
    }

  pOutPatchStream->latest_written_seq_size=0;

#ifdef SHOWSEQDEBUG
  printf("insert %u bytes from orig pos %u\n", size, pos);
#endif

  //internal = (INTERNAL_WRITESTATE *) (write_stream->state);

  /*
  posCurOrg = internal->curPosIns;
  if (pos != posCurOrg)
      internal->fFileIdentical = FALSE;
*/

#ifdef DO_STATIS
  {
    if (internal->statis[1] != NULL)
      AddValue(internal->statis[1], size);
    if (internal->statis[2] != NULL)
      AddValue(internal->statis[2],
               (pos + (GetMaxValue(internal->statis[2]) / 2)) - posCurOrg);



    write_byte(internal->tsthufout[0], 1);
    IncBits(internal->tsthufout[1], 1);

    if (size < 0xff)
    {
      write_byte(internal->tsthufout[0], (int) size);
      write_byte(internal->tsthufout[5], (int) size);
    }
    else
    {
      write_byte(internal->tsthufout[0], 0);
      write_byte(internal->tsthufout[5], 0);
      write_byte(internal->tsthufout[0], GetNbBitNumber(size));
      write_byte(internal->tsthufout[6], GetNbBitNumber(size));
      IncBits(internal->tsthufout[0], GetNbBitNumber(size) * 1);
      IncBits(internal->tsthufout[7], GetNbBitNumber(size) * 1);
    }

    if ((posCurOrg + 0x80 > pos) && (pos + 0x80 > posCurOrg))
    {
      int v;
      if (posCurOrg >= pos)
        v = (int) (((posCurOrg - pos) * 2) + 1);
      else
        v = (int) ((pos - posCurOrg) * 2);
      write_byte(internal->tsthufout[0], v);
      write_byte(internal->tsthufout[8], v);
    }
    else
    {
      dfuLong32 v;
      if (posCurOrg >= pos)
        v = (((posCurOrg - pos) * 2) + 1);
      else
        v = ((pos - posCurOrg) * 2);
      write_byte(internal->tsthufout[0], 0);
      write_byte(internal->tsthufout[8], 0);
      write_byte(internal->tsthufout[0], GetNbBitNumber(v));
      write_byte(internal->tsthufout[9], GetNbBitNumber(v));
      IncBits(internal->tsthufout[0], GetNbBitNumber(v) * 1);
      IncBits(internal->tsthufout[10], GetNbBitNumber(v) * 1);
    }

  }
#endif


  if ((posCurOrg == pos) && (size < 0x1b))
                                        /**++**/
  {
    dfbyte ct[1];
    ct[0] = 0x40 + 0x20 + (dfbyte) (size - 1);
#ifdef DO_STATIS
    if (internal->statis[3] != NULL)
      AddValue(internal->statis[3], ct[0]);
#endif
    err = WriteStreamData(pOutPatchStream, pOutDataStream, SEQINFO_STREAM, &(ct[0]), 1);
    size_of_written_seq = 1;
  }
  else if ((posCurOrg <= pos) && (pos - posCurOrg < 0x20) && (size <= 0x100))
  {
    dfbyte ct[2];
    ct[0] = 0x40 + (dfbyte) (pos - posCurOrg);
    ct[1] = (dfbyte) size - 1;
#ifdef DO_STATIS
    if (internal->statis[3] != NULL)
      AddValue(internal->statis[3], ct[0]);
#endif
    err = WriteStreamData(pOutPatchStream, pOutDataStream, SEQINFO_STREAM, &(ct[0]), 2);
    size_of_written_seq = 2;
  }
  else
    if ((posCurOrg + 0x80 > pos) && (pos + 0x80 > posCurOrg)
        && (size <= 0x100))
  {
    dfbyte ct[3];
    ct[0] = 0xfa;
    if (posCurOrg > pos)
      ct[1] = (dfbyte) (((posCurOrg - pos) * 2) + 1);
    else
      ct[1] = (dfbyte) ((pos - posCurOrg) * 2);

    ct[2] = (dfbyte) size - 1;
#ifdef DO_STATIS
    if (internal->statis[3] != NULL)
      AddValue(internal->statis[3], ct[0]);
#endif
    err = WriteStreamData(pOutPatchStream, pOutDataStream, SEQINFO_STREAM, &(ct[0]), 3);
    size_of_written_seq = 3;
  }
  else                          /*
                                   if ((posCurOrg<=pos) && (pos-posCurOrg<0x120) && (pos-posCurOrg>=0x20) &&
                                   (size<=0x100))
                                   {
                                   dfbyte ct[3];
                                   ct[0]=0xf8;
                                   ct[1]=(dfbyte)((pos-posCurOrg)-0x20);
                                   ct[2]=(dfbyte)size-1;
                                   #ifdef DO_STATIS
                                   if (internal->statis[3]!=NULL)
                                   AddValue(internal->statis[3],ct[0]);
                                   #endif
                                   err = WriteStreamData(pOutPatchStream, pOutDataStream,SEQINFO_STREAM,&(ct[0]),3);
                                   size_of_written_seq = 3;
                                   }
                                   else
                                   if ((pos<posCurOrg) && (pos+0x100>posCurOrg) && (size<=0x100))
                                   {
                                   dfbyte ct[3];
                                   ct[0]=0xf9;
                                   ct[1]=(dfbyte)((pos+0x100)-posCurOrg);
                                   ct[2]=(dfbyte)size-1;
                                   #ifdef DO_STATIS
                                   if (internal->statis[3]!=NULL)
                                   AddValue(internal->statis[3],ct[0]);
                                   #endif
                                   err = WriteStreamData(pOutPatchStream, pOutDataStream,SEQINFO_STREAM,&(ct[0]),3);
                                   size_of_written_seq = 3;
                                   }
                                   else- */
  {
//printf("\nwriteheadseq need highbit=%u size=%08x%08x - pos=%08x%08x\n",1,((dfuLong32)(size>>32)),((dfuLong32)size),((dfuLong32)(pos>>32)),((dfuLong32)pos));//++--@@@
    if (((pos>>32) == 0) && ((size>>32) == 0))
    {
        dfuLong32Intel posIntel = ConvertuLongToLongIntel((dfuLong32)pos);       /*+++ */
        err = WriteHeadSeq(pOutPatchStream, pOutDataStream, 1, (dfuLong32)size, &size_of_written_seq);
        if (err == DSERR_OK)
        {
          err = WriteStreamData(pOutPatchStream, pOutDataStream, SEQINFO_STREAM, &posIntel, 4);
          size_of_written_seq += 4;
        }

    }
    else
    {
        err = WriteHeadSeq64(pOutPatchStream, pOutDataStream, 1, size, pos, &size_of_written_seq);
    }
  }
    /*
  internal->curPosIns = pos + size;
  internal->curPos_newwritten += size;
   */
  if (err == DSERR_OK)
  {
    pOutPatchStream->latest_written_seq_is_ins = 0;
    pOutPatchStream->latest_written_seq_size = size;
    pOutPatchStream->latest_written_seq_pos = pos;
    pOutPatchStream->latest_size_of_written_seq = size_of_written_seq;
    pOutPatchStream->latest_written_seq_posCurOrg = posCurOrg ;

    FlushOutInStream(pOutPatchStream,pOutDataStream);
  }

  return err;
}

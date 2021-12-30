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
#include <string.h>

#include "../common/difbasic.h"

//#include "zlib.h"
#include "../common/abstractDecompress.h"
#include "../common/difstrm.h"
#include "../common/difstrmi.h"
#include "../common/DfsTlTyp.h"
#include "../common/difstool.h"

#include "ApDifStm.h"




int InitReadMultiBuffer(READ_MULTI_BUFFER* pReadMultiBuffer)
{
  int errRet=DSERR_OK;
  int err = ABSTR_DECOMPRESS_Z_OK;
  int i;

  pReadMultiBuffer->bufferSizeUnit = DEFAULT_SIZE_BUF_STREAM;
  pReadMultiBuffer->sizeBufStream = pReadMultiBuffer->bufferSizeUnit;


  //pReadMultiBuffer->nbStreamInReading = -1;
  //pReadMultiBuffer->CurrentStreamBytesToRead = 0;

  pReadMultiBuffer->inBufferPos = 0;
  pReadMultiBuffer->inBufferSize = pReadMultiBuffer->sizeBufStream * 3;
  pReadMultiBuffer->nullDummy=0;

  pReadMultiBuffer->nextStreamToFlush = 0;


  for (i = FIRST_STREAM; i <= LAST_STREAM; i++)
  {
      clear_abstract_decompress_stream(&pReadMultiBuffer->ReadStream[i].zstream);
      pReadMultiBuffer->ReadStream[i].inflateInitDone = FALSE;
      pReadMultiBuffer->ReadStream[i].inflateInitNegMaxWBits = FALSE;
	  pReadMultiBuffer->ReadStream[i].abstractDecompressor = FALSE;
  }

  pReadMultiBuffer->inBuffer = (dfbytep) DfsMalloc((size_t) (pReadMultiBuffer->inBufferSize));
  if (pReadMultiBuffer->inBuffer == NULL)
      return DSERR_INTERNAL;

  for (i = FIRST_STREAM; i <= LAST_STREAM; i++)
  {
    pReadMultiBuffer->ReadStream[i].posWriteBuf = 0;
    pReadMultiBuffer->ReadStream[i].posReadBuf = 0;
    pReadMultiBuffer->ReadStream[i].end_reached = 0;
    pReadMultiBuffer->ReadStream[i].sizeBuf = pReadMultiBuffer->sizeBufStream;
    pReadMultiBuffer->ReadStream[i].streamBuf =
      (dfbytep) DfsMalloc((size_t) (pReadMultiBuffer->ReadStream[i].sizeBuf));
    /* ##$$++**-- */
/*
    if (pReadMultiBuffer->ReadStream[i].inflateInitNegMaxWBits)
    {
        if (pReadMultiBuffer->ReadStream[i].streamBuf != NULL)
            err = inflateInit2(&pReadMultiBuffer->ReadStream[i].zstream, -MAX_WBITS);
    }
    else
    {
        if (pReadMultiBuffer->ReadStream[i].streamBuf != NULL)
          err = inflateInit(&pReadMultiBuffer->ReadStream[i].zstream);
    }*/

    if ((err != ABSTR_DECOMPRESS_Z_OK) || (pReadMultiBuffer->ReadStream[i].streamBuf == NULL))
    {
      int j;

      if (pReadMultiBuffer->ReadStream[i].streamBuf != NULL)
        DfsFree(pReadMultiBuffer->ReadStream[i].streamBuf);
      for (j = FIRST_STREAM; j < i; j++)
      {/*
        if (pReadMultiBuffer->ReadStream[j].inflateInitDone)
          inflateEnd(&pReadMultiBuffer->ReadStream[j].zstream);
        pReadMultiBuffer->ReadStream[j].inflateInitDone=FALSE;*/
        DfsFree(pReadMultiBuffer->ReadStream[j].streamBuf);
      }
      DfsFree(pReadMultiBuffer->inBuffer);

      return DSERR_INTERNAL;
    }
    //else pReadMultiBuffer->ReadStream[i].inflateInitDone = TRUE;
  }
  return errRet;
}




void RealignStream(RD_UNIT_STREAM *rstr)
{
  if (rstr->posReadBuf != 0)
  {
    dfbytep dfDest = rstr->streamBuf;
    dfbytep dfSource = rstr->streamBuf + rstr->posReadBuf;
	  /*
    dfuLong32 i;
    for (i = 0; i < rstr->posWriteBuf - rstr->posReadBuf; i++)
      *(dfDest++) = *(dfSource++);

	   */
    DfsMemmove(dfDest,dfSource,rstr->posWriteBuf - rstr->posReadBuf);

    rstr->posWriteBuf -= rstr->posReadBuf;
    rstr->posReadBuf = 0;
  }
}

int TryReadInBuffer(READ_MULTI_BUFFER* pReadMultiBuffer,IN_DATA_STREAM * pIn_data_stream,
     dfuLong32 sizeNeeded)
{
  int err = DSERR_OK;
  dfuLong32 dfDoCopy;

  dfuLong32 size;

/*
  if (pReadMultiBuffer->inBufferPos >= sizeNeeded)
      return err;
  size = sizeNeeded-pReadMultiBuffer->inBufferPos;
*/
  size = sizeNeeded;

  if (pReadMultiBuffer->inBufferPos + size > pReadMultiBuffer->inBufferSize)    /* we must enlarge the buffer */
  {
    dfbytep new_in;
    dfuLong32 new_sizein = pReadMultiBuffer->bufferSizeUnit + pReadMultiBuffer->inBufferSize;
    /* yes, we can divide, we'll do it later */
    while (pReadMultiBuffer->inBufferPos + size > new_sizein)
      new_sizein += pReadMultiBuffer->bufferSizeUnit;

    new_in = (dfbytep) DfsRealloc(pReadMultiBuffer->inBuffer, (size_t) new_sizein);

    if (new_in != NULL)
    {
      pReadMultiBuffer->inBuffer = new_in;
      pReadMultiBuffer->inBufferSize = new_sizein;
    }
    else
      err = DSERR_INTERNAL;
  }

  dfDoCopy = 0;
  if (err != DSERR_INTERNAL)
    if (size > pReadMultiBuffer->inBufferPos)
    {
      dfuLong32 size_ask_add = size - pReadMultiBuffer->inBufferPos;
      if (pIn_data_stream->avail_in < size_ask_add)
      {
        dfDoCopy = pIn_data_stream->avail_in;
        err = DSERR_NEED_READMORE;
      }
      else
        dfDoCopy = size_ask_add;
    }

  if (dfDoCopy > 0)
  {
    dfbytep dfDest = pReadMultiBuffer->inBuffer + pReadMultiBuffer->inBufferPos;
    dfbytep dfSource = (dfbytep)pIn_data_stream->next_in;
	  /*
    dfuLong32 i;
    for (i = 0; i < dfDoCopy; i++)
      *(dfDest++) = *(dfSource++);
*/

    DfsMemcpy(dfDest,dfSource,dfDoCopy);

    pReadMultiBuffer->inBufferPos += dfDoCopy;
    pIn_data_stream->next_in = ((dfbytep) pIn_data_stream->next_in) + dfDoCopy;
    pIn_data_stream->total_in += dfDoCopy;
    pIn_data_stream->avail_in -= dfDoCopy;
  }
  return err;
}



int
ReadUntilDataFilledFromStream(READ_MULTI_BUFFER* pReadMultiBuffer, IN_DATA_STREAM * pIn_data_stream,
                              int nbStream,dfuLong32 size)
{
  int err = DSERR_OK;
  RD_UNIT_STREAM *rstr;
  rstr = &pReadMultiBuffer->ReadStream[nbStream];

  while ((err == DSERR_OK) &&
         ((rstr->posWriteBuf - rstr->posReadBuf) < size) &&
         (rstr->end_reached == 0))
  {
    dfuLong32 size_current_packet = 0;
    dfuLong32 stream_number = 0;
    int flush = ABSTR_DECOMPRESS_Z_SYNC_FLUSH;
    int inflate_workdone = 0;
    RD_UNIT_STREAM *rstrCurrent = NULL;
    int headerSize=0;
    int headerPacketSizeSize = 0;
    BOOL fEndOfStream = FALSE;

    err = TryReadInBuffer(pReadMultiBuffer, pIn_data_stream,1);
    if (err == DSERR_OK)
    {
      unsigned char headerFirstByte = *((unsigned char*)pReadMultiBuffer->inBuffer);
      headerPacketSizeSize = 4;
      headerSize = 8;

      if ((headerFirstByte & SMALLHEADER_SIGN_MASK) == SMALLHEADER_SIGN_VALUE)
      {
          unsigned char headerSmallSize = headerFirstByte & SMALLHEADER_SIZE_MASK;
          if (headerSmallSize == SMALLHEADER_SIZE_VALUE4)
              headerPacketSizeSize = 4;
          if (headerSmallSize == SMALLHEADER_SIZE_VALUE2)
              headerPacketSizeSize = 2;
          if (headerSmallSize == SMALLHEADER_SIZE_VALUE1)
              headerPacketSizeSize = 1;
          if (headerSmallSize == SMALLHEADER_SIZE_VALUE0)
              headerPacketSizeSize = 0;
          headerSize = headerPacketSizeSize + 1;
          fEndOfStream = (headerFirstByte & SMALLHEADER_ENDSTREAM_MASK) != 0;
          stream_number = (headerFirstByte & SMALLHEADER_NUMBERSTREAM_MASK);
          pReadMultiBuffer->ReadStream[stream_number].inflateInitNegMaxWBits = TRUE;
		  pReadMultiBuffer->ReadStream[stream_number].abstractDecompressor =
			  ((headerFirstByte & SMALLHEADER_ASTRACT_DECOMPRESSOR_MASK) != 0) ? TRUE : FALSE;
      }
    }

    /* We try fill the end of current paquet in inBuffer */
    if (err == DSERR_OK)
      err = TryReadInBuffer(pReadMultiBuffer,pIn_data_stream, headerSize);
    if (err == DSERR_OK)
    {
      if (headerSize == headerPacketSizeSize + 4)
      {
        stream_number = difstr_getValue_inmemory(pReadMultiBuffer->inBuffer, 4);
        fEndOfStream = (stream_number & ENDOFSTREAM_MASK) != 0;
        stream_number = stream_number & NUMBERSTREAM_MASK;
      }
      size_current_packet =
        difstr_getValue_inmemory(pReadMultiBuffer->inBuffer + headerSize - headerPacketSizeSize,
                                 headerPacketSizeSize);

      err = TryReadInBuffer(pReadMultiBuffer, pIn_data_stream,headerSize + size_current_packet);
    }
    if ((err==DSERR_OK) && (stream_number>LAST_STREAM))
        err = DSERR_INTERNAL;

    if ((err == DSERR_OK) && (!pReadMultiBuffer->ReadStream[stream_number].inflateInitDone) && (size_current_packet>0))
    {
        int errInit = ABSTR_DECOMPRESS_Z_ERRNO;

        if (pReadMultiBuffer->ReadStream[stream_number].abstractDecompressor)
        {
            if (pReadMultiBuffer->ReadStream[stream_number].streamBuf != NULL)
                errInit = abstract_init_prefix(&pReadMultiBuffer->ReadStream[stream_number].zstream);
		}
		else
        if (pReadMultiBuffer->ReadStream[stream_number].inflateInitNegMaxWBits)
        {
            if (pReadMultiBuffer->ReadStream[stream_number].streamBuf != NULL)
                errInit = abstract_init_inflate_withNegMaxWBits(&pReadMultiBuffer->ReadStream[stream_number].zstream);
        }
        else
        {
            if (pReadMultiBuffer->ReadStream[stream_number].streamBuf != NULL)
              errInit = abstract_init_inflate_withoutNegMaxWBits(&pReadMultiBuffer->ReadStream[stream_number].zstream);
        }
        if (errInit == ABSTR_DECOMPRESS_Z_OK)
            pReadMultiBuffer->ReadStream[stream_number].inflateInitDone = TRUE;
        else
            err = DSERR_INTERNAL;
    }

    if (err == DSERR_OK)
    {
      rstrCurrent = &pReadMultiBuffer->ReadStream[stream_number];
      RealignStream(rstrCurrent);

      rstrCurrent->zstream.next_in = pReadMultiBuffer->inBuffer + headerSize;
      rstrCurrent->zstream.avail_in = size_current_packet;
      flush =
        (!fEndOfStream) ? ABSTR_DECOMPRESS_Z_SYNC_FLUSH : ABSTR_DECOMPRESS_Z_FINISH;
      //flush = Z_SYNC_FLUSH;
    }

    if ((err == DSERR_OK) && (!pReadMultiBuffer->ReadStream[stream_number].inflateInitDone) && (fEndOfStream))
        rstrCurrent->end_reached = 1;

    if ((err == DSERR_OK) && (pReadMultiBuffer->ReadStream[stream_number].inflateInitDone))
    {
      do
      {
        if (rstrCurrent->sizeBuf == rstrCurrent->posWriteBuf)
        {
          dfbytep new_in;

          new_in = (dfbytep) DfsRealloc(rstrCurrent->streamBuf,
                                        (size_t) (pReadMultiBuffer->bufferSizeUnit +
                                                  rstrCurrent->sizeBuf));

          if (new_in != NULL)
          {
            rstrCurrent->streamBuf = new_in;
            rstrCurrent->sizeBuf += pReadMultiBuffer->bufferSizeUnit;
          }
          else
            err = DSERR_INTERNAL;
        }

        if (err != DSERR_INTERNAL)
        {
          int errinflate;
          rstrCurrent->zstream.next_out = rstrCurrent->streamBuf +
            rstrCurrent->posWriteBuf;
          rstrCurrent->zstream.avail_out = rstrCurrent->sizeBuf -
            rstrCurrent->posWriteBuf;

#ifdef SHOWSTREAM
          printf(" inflate strm:%d,%d inbef:%d, outbef:%d, flsh:%d",
                 nbStream, stream_number,
                 rstrCurrent->zstream.avail_in,
                 rstrCurrent->zstream.avail_out, flush);
#endif
#ifdef SHOWSTREAM
          if (flush == Z_FINISH)
            printf("Finish\n");
#endif

          //if (rstrCurrent->zstream.avail_in == 0)
          //  errinflate = Z_OK;
          //else
          {
            int flushParam=ABSTR_DECOMPRESS_Z_SYNC_FLUSH;
            /*
            if ((flush==Z_FINISH) && (rstrCurrent->zstream.avail_in==0))
             flushParam = Z_FINISH;
             */

            /* If windowBits is passed < 0 to tell that there is no zlib header.
             * Note that in this case inflate *requires* an extra "dummy" byte
             * after the compressed stream in order to complete decompression and
             * return Z_STREAM_END.
             */


            if ((flush==ABSTR_DECOMPRESS_Z_FINISH) && (rstrCurrent->zstream.avail_in==0))
            {
                rstrCurrent->zstream.avail_in=sizeof(pReadMultiBuffer->nullDummy);
                rstrCurrent->zstream.next_in=(unsigned char*)&pReadMultiBuffer->nullDummy;
            }


            errinflate = abstract_decompress(&rstrCurrent->zstream, flushParam);
          }

#ifdef SHOWSTREAM
          printf(" inaf : %d outaf : %d err=%d\n",
                 rstrCurrent->zstream.avail_in,
                 rstrCurrent->zstream.avail_out, errinflate);
#endif

          if ((errinflate != ABSTR_DECOMPRESS_Z_OK) && (errinflate != ABSTR_DECOMPRESS_Z_STREAM_END))
            err = DSERR_INTERNAL;
          /* error here */
          rstrCurrent->posWriteBuf = rstrCurrent->sizeBuf -
            rstrCurrent->zstream.avail_out;

/***+++---/ bugfix */
/*
          inflate_workdone = (rstrCurrent->zstream.avail_in == 0) &&
            ((flush!= Z_FINISH) || (rstrCurrent->zstream.avail_out > 0));

          if (((flush == Z_FINISH)) && (inflate_workdone != 0))
            if (errinflate == Z_STREAM_END)
              inflate_workdone = 1;
              */
          if (flush == ABSTR_DECOMPRESS_Z_FINISH)
              inflate_workdone = (errinflate == ABSTR_DECOMPRESS_Z_STREAM_END);
          else
              inflate_workdone = (rstrCurrent->zstream.avail_in == 0);
          if (errinflate == ABSTR_DECOMPRESS_Z_STREAM_END)
            rstrCurrent->end_reached = 1;
        }
      }
      while ((inflate_workdone == 0) && (err != DSERR_INTERNAL));
    }

    if (err == DSERR_OK)
        pReadMultiBuffer->inBufferPos = 0;        /* forgot the current paquet */
  }

  if (err == DSERR_OK)
  {
    dfuLong32 i;
    err = DSERR_END;
    for (i = FIRST_STREAM; i <= LAST_STREAM; i++)
    {
      RD_UNIT_STREAM *rstrCurrenti = &pReadMultiBuffer->ReadStream[i];

      if ((rstrCurrenti->end_reached != 1) ||
          (((rstrCurrenti->posWriteBuf != rstrCurrenti->posReadBuf))))
      {
        err = DSERR_OK;
        break;
      }
    }
  }

  return err;
}

/* possible return value :
  DSERR_OK
  DSERR_NEED_READMORE
  DSERR_END
  DSERR_INTERNAL
*/

int
GetDataFromStream(READ_MULTI_BUFFER* pReadMultiBuffer, IN_DATA_STREAM * pIn_data_stream,
                  int nbStream, dfvoidp buf,
                  dfuLong32 size, dfuLong32 * written_size)
{
  int err = DSERR_OK;


  err = ReadUntilDataFilledFromStream(pReadMultiBuffer, pIn_data_stream, nbStream, size);
  if ((err == DSERR_OK) || (err == DSERR_END))
  {
    dfuLong32 dfCopy;
    RD_UNIT_STREAM *rstr;
    rstr = &pReadMultiBuffer->ReadStream[nbStream];
    if (size < rstr->posWriteBuf - rstr->posReadBuf)
      dfCopy = size;
    else
      dfCopy = rstr->posWriteBuf - rstr->posReadBuf;


    if (dfCopy > 0)
    {
      dfbytep dfDest = (dfbytep)buf;
      dfbytep dfSource = rstr->streamBuf + rstr->posReadBuf;
		/*
      dfuLong32 i;
      for (i = 0; i < dfCopy; i++)
        *(dfDest++) = *(dfSource++);
*/

      DfsMemcpy(dfDest,dfSource,dfCopy);

      rstr->posReadBuf += dfCopy;
    }
    if (written_size != NULL)
      *written_size = dfCopy;
  }

  return err;
}


int VerifyEachStreamEnded(READ_MULTI_BUFFER* pReadMultiBuffer)
{
    dfuLong32 i;
    int err = DSERR_END;
    for (i = FIRST_STREAM; i <= LAST_STREAM; i++)
    {
      RD_UNIT_STREAM *rstrCurrenti = &pReadMultiBuffer->ReadStream[i];

      if ((rstrCurrenti->end_reached != 1) ||
          (((rstrCurrenti->posWriteBuf != rstrCurrenti->posReadBuf))))
      {
        err = DSERR_OK;
        break;
      }
    }

  return err;
}

void CloseReadMultiBuffer(READ_MULTI_BUFFER* pReadMultiBuffer)
{
  dfuLong32 i;

#if (defined (SHOWSTREAM) || defined (SHOWSTREAMEND)) && defined(NOQUIET)
  printf("MEMREAD : pReadMultiBuffer->inBufferSize = %u\n",pReadMultiBuffer->inBufferSize);
  for (i = FIRST_STREAM; i <= LAST_STREAM; i++)
      printf("    pReadMultiBuffer->ReadStream[%u].sizeBuf = %u (%u KB)\n",i,
          pReadMultiBuffer->ReadStream[i].sizeBuf,pReadMultiBuffer->ReadStream[i].sizeBuf/1024);
#endif

  for (i = FIRST_STREAM; i <= LAST_STREAM; i++)
  {
    if (pReadMultiBuffer->ReadStream[i].inflateInitDone)
      abstract_decompress_end(&pReadMultiBuffer->ReadStream[i].zstream);
    pReadMultiBuffer->ReadStream[i].inflateInitDone=FALSE;
    DfsFree(pReadMultiBuffer->ReadStream[i].streamBuf);
  }
  DfsFree(pReadMultiBuffer->inBuffer);
}

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/


dfuLong32 DecodeSeqInfo(dfuLong64* pCurPos_depl, dfbytep ptr,
                        dfuLong32 size, dfuLong32 * is_insert_bytes,
                        dfuLong64 * size_block, dfuLong64 * pos_seq)
{
  dfbyte c;
  dfuLong32 byteNeeded = 0;

  if (size == 0)
    return 1;
  c = *ptr;

  if ((c & 0x80)==0)
  {
      if (c < 0x38)
      {
        byteNeeded = 1;
        *is_insert_bytes = 1;
        *size_block = c;
      }
      else
      if ((c >= 0x40) && (c < (0x60)))
      {
        byteNeeded = 1 + 1;
        *is_insert_bytes = 0;
        if (size >= byteNeeded)
        {
          *size_block = (*(ptr + 1)) + 1;
          *pos_seq = ((*pCurPos_depl + c) - 0x40) - 0;
        }
      }
      else
      if ((c >= 0x60) && (c < (0x7b)))
                                    /**++**/
      {
        byteNeeded = 1;
        *is_insert_bytes = 0;
        if (size >= byteNeeded)
        {
          *size_block = (c - 0x60) + 1;
          *pos_seq = (*pCurPos_depl);
        }
      }
      else
      // note 0x7b used for 64 (7c-7f real range)
      if ((c > 0x7b) && (c <= 0x7f))
      {
        dfuLong32 i;
        *is_insert_bytes = 1;
        byteNeeded = 1 + (c - 0x7b);
        if (size >= byteNeeded)
        {
          dfuLong32Intel sizeIntel = ConvertuLongToLongIntel(0);
          for (i = 1; i < byteNeeded; i++)
            *(((dfbytep) (&sizeIntel)) + (i - 1)) = *(ptr + i);
          *size_block = ConvertuLongIntelToLong(sizeIntel);
        }
      }
      else
      if (c == 0x7b)
      {
          if (size < 2)
            byteNeeded = 3;
          else
          {
              dfbyte c2 = *(ptr+1);
              dfbyte dfNbBytesPos=(c2 & 0x0f);
              dfbyte dfNbBytesSize=((c2 >> 4) & 7)+1;
              byteNeeded = 2 + dfNbBytesPos + dfNbBytesSize;
              if (size >= byteNeeded)
              {
                  dfuLong64Intel posIntel = ConvertuLongToLongIntel64(0);
                  dfuLong64Intel sizeIntel = ConvertuLongToLongIntel64(0);
                  *is_insert_bytes = ((c2 & 0x80) == 0) ? 1 : 0 ;
                  DfsMemcpy(&sizeIntel,ptr+2,dfNbBytesSize);
                  DfsMemcpy(&posIntel,ptr+2+dfNbBytesSize,dfNbBytesPos);

                  *size_block = ConvertuLongIntelToLong64(sizeIntel);
                  *pos_seq = ConvertuLongIntelToLong64(posIntel);
                  /*
                  printf("ap:size,pos=%x,%x, is_insert=%u, size,pos size=%u-%u\n",*size_block ,*pos_seq ,*is_insert_bytes,dfNbBytesSize,dfNbBytesPos);
                  { int i; for (i=0;i<byteNeeded;i++) printf("%02x ",*(ptr+i)); printf("\n"); }*/
              }
          }
      }
  }
  else
  {
      if ((c >= 0x80) && (c <= 0xf8))
      {
        byteNeeded = 1 + 4;
        *is_insert_bytes = 0;
        *size_block = c & 0x7f;
        if (size >= byteNeeded)
        {
          dfuLong32 i;
          dfuLong32Intel sizeIntel = ConvertuLongToLongIntel(0);
          for (i = 1; i < byteNeeded; i++)
            *(((dfbytep) (&sizeIntel)) + (i - 1)) = *(ptr + i);
          *pos_seq = ConvertuLongIntelToLong(sizeIntel);
        }
      }
      else
      if (c == 0xf8)
      {
        *is_insert_bytes = 0;
        byteNeeded = 1 + 1 + 1;
        if (size >= 3)
        {
          *size_block = (*(ptr + 2)) + 1;
          *pos_seq = *pCurPos_depl + (*(ptr + 1)) + 0x20;
        }
      }
      else
      if (c == 0xf9)
      {
        *is_insert_bytes = 0;
        byteNeeded = 1 + 1 + 1;
        if (size >= 3)
        {
          *size_block = (*(ptr + 2)) + 1;
          *pos_seq = (*pCurPos_depl + (*(ptr + 1)) + 0x00) - 0x100;
        }
      }
      else
      if (c == 0xfa)
      {
        *is_insert_bytes = 0;
        byteNeeded = 1 + 1 + 1;
        if (size >= 3)
        {
          dfbyte c2 = *(ptr + 1);
          *size_block = (*(ptr + 2)) + 1;
          if ((c2 & 1) == 1)
            *pos_seq = *pCurPos_depl - (c2 / 2);
          else
            *pos_seq = *pCurPos_depl + (c2 / 2);

          //*pos_seq = (*pCurPos_depl  + (*(ptr+1)) + 0x00) - 0x80;
        }
      }
      else

    // note 0xfb probably never user (fc-ff real range)
    //if ((c >= 0xfb) && (c <= 0xff))

      if ((c >= 0xfb))

      {
        *is_insert_bytes = 0;
        byteNeeded = 1 + (c - 0xfb) + 4;
        if (size >= byteNeeded)
        {
          dfuLong32 i;
          dfuLong32Intel sizeIntel = ConvertuLongToLongIntel(0);
          for (i = 1; i <= (dfuLong32) (c - 0xfb); i++)
            *(((dfbytep) (&sizeIntel)) + i - 1) = *(ptr + i);
          *size_block = ConvertuLongIntelToLong(sizeIntel);

          sizeIntel = ConvertuLongToLongIntel(0);
          for (i = 1; i <= 4; i++)
            *(((dfbytep) (&sizeIntel)) + i - 1) = *(ptr + i + (c - 0xfb));
          *pos_seq = ((ConvertuLongIntelToLong(sizeIntel))) ;
        }
      }

  }




  if ((size >= byteNeeded) && ((*is_insert_bytes) == 0))
    *pCurPos_depl = (*pos_seq) + (*size_block);

  if ((size >= byteNeeded) && ((*is_insert_bytes) == 1))        /*+++* */
    *pCurPos_depl += (*size_block);

  if (byteNeeded == 0)
    printf("bad code\n");

#ifdef DBGWIN
  if (size >= byteNeeded)
  {
        //dfuLong32 * is_insert_bytes,
        //                 dfuLong64 * size_block, dfuLong64 * pos_seq)
      char sz[100];
      wsprintf(sz,"\nsizeblock = %I64x, pos_seq= %I64x, is_insert_byte=%u, bytes seq=%u\n",* size_block,*pos_seq,*is_insert_bytes,byteNeeded);
      printf(sz); OutputDebugString(sz);
  }
#endif

  if (size >= byteNeeded)
    return 0;
  else
    return (byteNeeded - size);
}

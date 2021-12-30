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
#include "zlib.h"
#include "../common/difbasic.h"
#include "../common/difstrm.h"
#include "../common/DfsOrigMemoryMap.h"
#include "apldifst.h"
#include "../common/difstrmi.h"
#include "../common/DfsTlTyp.h"
#include "../common/difstool.h"

#include "ApDifStm.h"

#ifndef local
#define local static
#endif

#define DBGWIN_
#ifdef DBGWIN
//#include <windows. h>
#endif
//#define SHOWSTREAM

/*
TODO :
- real Multi OrigPtr support
- test streaming
*/



#define SIZE_SEQ_INFO_PREREAD (0x20)

typedef struct
{
  READ_MULTI_BUFFER readMultiBuffer;


  dfuLong32 uLastAdler32;

  dfuLong64 curPos_depl;

  dfbyte seq_info_inread_tab[SIZE_SEQ_INFO_PREREAD];
  dfbytep seq_info_inread_ptr;
  dfuLong32 seq_info_inread_size;
  dfuLong32 seq_info_inread_pos;

  dfuLong64 bytes_in_depl_seq;
  dfuLong64 pos_in_depl_seq;
  dfuLong64 bytes_in_ins_seq;
}
INTERNAL_READSTATE;

/*

int ApplyDifInit OF((APPLYDIF_STREAM *read_stream));
int DoApplyDifWork OF((APPLYDIF_STREAM *read_stream));
int CloseApplyDif OF((APPLYDIF_STREAM *read_stream, dfuLong32* uChecksum));

*/

int SVFAPI ApplyDifInit OF((APPLYDIF_STREAM * read_stream));

int SVFAPI ApplyDifInit(
     APPLYDIF_STREAM *read_stream)
{
    return ApplyDifInitEx(read_stream, FALSE);
}

int SVFAPI ApplyDifInitEx(
     APPLYDIF_STREAM *read_stream, BOOL fComputeChecksumSet)
{
  //int err = Z_OK;
  dfbytep ptrAlloc;

  INTERNAL_READSTATE *internal;
  read_stream->uDoChecksum = fComputeChecksumSet ? 1 : 0;

  read_stream->in_data_stream.next_in = NULL;
  read_stream->in_data_stream.avail_in = 0;
  read_stream->in_data_stream.total_in = 0;
  read_stream->in_data_stream.done_latest_in = 0;

  read_stream->out_data_stream.next_out = NULL;
  read_stream->out_data_stream.avail_out = 0;
  read_stream->out_data_stream.total_out = 0;
  read_stream->out_data_stream.done_latest_out = 0;

  ptrAlloc = (dfbytep) DfsMalloc(sizeof(INTERNAL_READSTATE));
  internal = (INTERNAL_READSTATE *) ptrAlloc;
  read_stream->state = (HAPPLYDIF_INTERNALSTATE) internal;
  if (internal == NULL)
    return DSERR_INTERNAL;

  if (InitReadMultiBuffer(&internal->readMultiBuffer) != DSERR_OK)
  {
    DfsFree(internal);
    return DSERR_INTERNAL;
  }

  internal->uLastAdler32 = (dfuLong32)adler32(0L, 0, 0);

  internal->seq_info_inread_ptr = internal->seq_info_inread_tab;
  internal->seq_info_inread_size = SIZE_SEQ_INFO_PREREAD;
  internal->seq_info_inread_pos = 0;

  internal->bytes_in_depl_seq = 0;
  internal->bytes_in_ins_seq = 0;
  internal->pos_in_depl_seq = 0;

  internal->curPos_depl = 0;

  return DSERR_OK;
}

/****************************************************************************/


#include <stdio.h>
#include <string.h>

int SVFAPI DoApplyDifWork(APPLYDIF_STREAM * read_stream)
{
  INTERNAL_READSTATE *internal;
  int err = DSERR_OK;
  dfuLong32 is_insert_bytes;
  dfuLong64 size_block, pos_seq;

  internal = (INTERNAL_READSTATE *) read_stream->state;

  while ((err == DSERR_OK) && (read_stream->out_data_stream.avail_out > 0))
  {
    if ((internal->bytes_in_depl_seq == 0)
        && (internal->bytes_in_ins_seq == 0))
    {
        dfuLong32 byte_needed = DecodeSeqInfo(&internal->curPos_depl,
                                    internal->seq_info_inread_ptr,
                                    internal->seq_info_inread_pos,
                                    &is_insert_bytes,
                                    &size_block,
                                    &pos_seq);

      if (byte_needed > 0)
      {
        dfuLong32 size_read_in_seq_stream = 0;
        err = GetDataFromStream(&internal->readMultiBuffer,& read_stream->in_data_stream,
                                SEQINFO_STREAM,
                                internal->seq_info_inread_ptr +
                                internal->seq_info_inread_pos,
                                byte_needed, &size_read_in_seq_stream);

        if (err == DSERR_NEED_READMORE)
        {
          return DSERR_OK;
        }

        if ((err == DSERR_OK) && (size_read_in_seq_stream == 0) && (read_stream->in_data_stream.avail_in == 0))        // we need more data to read
          return DSERR_OK;

        if ((err == DSERR_OK) && (size_read_in_seq_stream == 0))        // we need more data to read
        {
            dfuLong32 i;
            for (i = FIRST_STREAM; i <= LAST_STREAM; i++)
                ReadUntilDataFilledFromStream(&internal->readMultiBuffer,& read_stream->in_data_stream,
                                              i,read_stream->in_data_stream.avail_in);
            if (read_stream->in_data_stream.avail_in != 0)
                return DSERR_INTERNAL;
        }

        if ((err == DSERR_END) && (size_read_in_seq_stream == byte_needed))     // to do better
          err = DSERR_OK;

        internal->seq_info_inread_pos += size_read_in_seq_stream;
      }
      else
      {
        // we have fully read the seq!
        internal->seq_info_inread_pos = 0;

        if ((is_insert_bytes) == 0)
        {
          ///printf("block depl, size=%u, pos=%u\n",(size_block),pos_seq);
          internal->bytes_in_depl_seq = size_block;
          internal->pos_in_depl_seq = pos_seq;
        }
        else
        {
          ///printf("block ins, size=%u\n",(size_block));
          internal->bytes_in_ins_seq = size_block;
        }
      }
    }

    if ((internal->bytes_in_depl_seq > 0) && (err == DSERR_OK))
    {
      dfuLong32 doThis ;


#ifdef _DEBUG
      if (internal->bytes_in_depl_seq + internal->pos_in_depl_seq>read_stream->OrigDataPtr->size)
          printf("overflow seq\n");
#endif

      if (internal->bytes_in_depl_seq < read_stream->out_data_stream.avail_out)
          doThis = (dfuLong32)internal->bytes_in_depl_seq;
      else
          doThis = read_stream->out_data_stream.avail_out;

      /* here we check if ramdif */
      {
          dfuLong32 doThisMemCopy;
          dfuLong32 doThisMemCopyPos = 0;
          doThisMemCopy = doThis;
          while (doThisMemCopy>0)
          {
            dfuLong32 doThisMemCopyStep = (dfuLong32)(dfmin(doThisMemCopy,
                                                        GetMaxOrigDataExigibleSizeView(read_stream->OrigDataPtr)));
            DfsMemcpy(((dfbytep)(read_stream->out_data_stream.next_out)) + doThisMemCopyPos,
                        ((dfbytep) (GetOrigDataPtrpDataBySizeView(read_stream->OrigDataPtr,
                          internal->pos_in_depl_seq + doThisMemCopyPos, doThisMemCopyStep))) +
                        internal->pos_in_depl_seq + doThisMemCopyPos, doThisMemCopyStep);
            doThisMemCopy -= doThisMemCopyStep;
            doThisMemCopyPos += doThisMemCopyStep;
          }
      }

      internal->pos_in_depl_seq += doThis;

      /*if (inadler) */
      if (read_stream->uDoChecksum != 0)
        internal->uLastAdler32 = (dfuLong32)adler32(internal->uLastAdler32,
                                       (dfbytep)read_stream->out_data_stream.next_out, doThis);

      read_stream->out_data_stream.next_out = ((dfbytep) read_stream->out_data_stream.next_out) + doThis;
      read_stream->out_data_stream.total_out += doThis;
      read_stream->out_data_stream.avail_out -= doThis;

      internal->bytes_in_depl_seq -= doThis;
    }

    if ((internal->bytes_in_ins_seq > 0) && (err == DSERR_OK))
    {
      dfuLong32 doThis ;

      if (internal->bytes_in_ins_seq < read_stream->out_data_stream.avail_out)
          doThis = (dfuLong32)(internal->bytes_in_ins_seq);
      else
          doThis = read_stream->out_data_stream.avail_out;

      err = GetDataFromStream(&internal->readMultiBuffer,& read_stream->in_data_stream,
                              INSBYTE_STREAM,
                              read_stream->out_data_stream.next_out, doThis, &doThis);

      if (err == DSERR_NEED_READMORE)
      {
        return DSERR_OK;
      }

      /*if (inadler) */
      if (read_stream->uDoChecksum != 0)
        internal->uLastAdler32 = (dfuLong32)adler32(internal->uLastAdler32,
                                       (dfbytep)read_stream->out_data_stream.next_out, doThis);

      read_stream->out_data_stream.next_out = ((dfbytep) read_stream->out_data_stream.next_out) + doThis;
      read_stream->out_data_stream.total_out += doThis;
      read_stream->out_data_stream.avail_out -= doThis;

      internal->bytes_in_ins_seq -= doThis;
    }
  }

  if (err == DSERR_NEED_READMORE)
    err = DSERR_OK;

  if ((err == DSERR_OK) && (internal->seq_info_inread_pos==0) && (internal->bytes_in_depl_seq==0) && (internal->bytes_in_ins_seq==0))
  {
      err = VerifyEachStreamEnded(&internal->readMultiBuffer);
  }
  return err;
}


int SVFAPI CloseApplyDif(APPLYDIF_STREAM * read_stream, dfuLong32 * uChecksum)
{
  INTERNAL_READSTATE *internal;
  int err = DSERR_OK;
  internal = (INTERNAL_READSTATE *) read_stream->state;

  if (uChecksum != NULL)
  {
      if (read_stream->uDoChecksum != 0)
          *uChecksum = internal->uLastAdler32;
      if ((read_stream->uDoChecksum == 0) && (err==DSERR_OK))
          err = DSERR_INTERNAL;
  }

  CloseReadMultiBuffer(&internal->readMultiBuffer);
  DfsFree(internal);

  return err;
}

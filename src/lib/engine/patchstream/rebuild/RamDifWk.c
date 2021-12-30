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

#include "zlib.h"
#include "../common/difbasic.h"
#include "../common/difstrm.h"
#include "../common/difstrmi.h"
#include "../common/DfsTlTyp.h"
#include "../common/difstool.h"

#include "../decompress/ApDifStm.h"

#include "../common/DfsOrigMemoryMap.h"

#include "RamDifWk.h"
#include "RamDifTl.h"

/*
 tools :
  1) StreamDif -> RamDif (DONE)
  1a) RamDif + StreamDif -> RamDif (DONE)

  RamDif + StreamDif -> StreamDif

  Apply Ramdif on file to apply patch (DONE)
  (RamDif -> StreamDif, not useful)
*/

// .H


// .c


#define SIZE_SEQ_INFO_PRE_READ (0x20)
typedef struct
{
    HRAMDIF hRamDifBase;
    CLASS_OUT_PATCH Class_Out_Patch;
    dfuLong64 dfExpectedSizeOfPatchResult;
    READ_MULTI_BUFFER readMultiBuffer;

    dfuLong32 dfMaxSizeWriteInsByteArray;
    dfuLong32 dfSizeInsByteArray ;
    dfbytep pInsByteArray;

  dfbyte seq_info_inread_tab[SIZE_SEQ_INFO_PRE_READ];
  dfbytep seq_info_inread_ptr;

  dfuLong64 curPos_depl;
  dfuLong32 seq_info_inread_size;
  dfuLong32 seq_info_inread_pos;

  dfuLong64 bytes_in_depl_seq;
  dfuLong64 pos_in_depl_seq;
  dfuLong64 bytes_in_ins_seq;

  dfuLong64 data_pos;

  dfuLong64 total_size_insert;
  dfuLong64 total_size_depl_in_place;
  dfuLong64 total_size_depl_out_place;

  ORIGDATAPTR OrigDataPtrForInsertNoInPlace;

} RAMDIFBLD_FROM_DATA_STREAM_INTERNAL;

int SVFAPI PatchBldFromDataStreamAndRamDifInit (RAMDIFBLD_FROM_DATA_STREAM * pDifbld,
                                                const CLASS_OUT_PATCH* pClass_Out_PatchSet,
                                                 dfuLong64 dfExpectedSizeOfPatchResult,
                                                 ORIGDATAPTR OrigDataPtrForInsertNoInPlace,
                                                 HRAMDIF hRamDifBase)
{
    RAMDIFBLD_FROM_DATA_STREAM_INTERNAL *pRamDifBld_From_Data_Stream_Internal;

    pDifbld->state = NULL;

    pRamDifBld_From_Data_Stream_Internal = (RAMDIFBLD_FROM_DATA_STREAM_INTERNAL *)
                DfsMalloc(sizeof(RAMDIFBLD_FROM_DATA_STREAM_INTERNAL));

    if (pRamDifBld_From_Data_Stream_Internal == NULL)
        return DSERR_INTERNAL;

    pRamDifBld_From_Data_Stream_Internal->hRamDifBase = hRamDifBase;
    pRamDifBld_From_Data_Stream_Internal->Class_Out_Patch = *pClass_Out_PatchSet;
    pRamDifBld_From_Data_Stream_Internal->dfExpectedSizeOfPatchResult = dfExpectedSizeOfPatchResult;

    pRamDifBld_From_Data_Stream_Internal->dfMaxSizeWriteInsByteArray = pClass_Out_PatchSet->dfMaxSizeWriteInsByteArray;
    pRamDifBld_From_Data_Stream_Internal->dfSizeInsByteArray = dfmin(pRamDifBld_From_Data_Stream_Internal->dfMaxSizeWriteInsByteArray,0x8000);
    pRamDifBld_From_Data_Stream_Internal->pInsByteArray = (dfbytep)DfsMalloc(pRamDifBld_From_Data_Stream_Internal->dfSizeInsByteArray);



    pRamDifBld_From_Data_Stream_Internal->total_size_insert = 0;
    pRamDifBld_From_Data_Stream_Internal->total_size_depl_in_place = 0;
    pRamDifBld_From_Data_Stream_Internal->total_size_depl_out_place = 0;

    pRamDifBld_From_Data_Stream_Internal->OrigDataPtrForInsertNoInPlace = OrigDataPtrForInsertNoInPlace;

    pRamDifBld_From_Data_Stream_Internal->data_pos = 0;

    if (pRamDifBld_From_Data_Stream_Internal->pInsByteArray == NULL)
    {
        DfsFree(pRamDifBld_From_Data_Stream_Internal);
        return DSERR_INTERNAL;
    }

    if (InitReadMultiBuffer(&pRamDifBld_From_Data_Stream_Internal->readMultiBuffer) != DSERR_OK)
    {
        DfsFree(pRamDifBld_From_Data_Stream_Internal->pInsByteArray);
        DfsFree(pRamDifBld_From_Data_Stream_Internal);
        return DSERR_INTERNAL;
    }

    pDifbld->state = (HRAMDIFBLD_FROM_DATA_STREAM)pRamDifBld_From_Data_Stream_Internal;


    pRamDifBld_From_Data_Stream_Internal->seq_info_inread_ptr = pRamDifBld_From_Data_Stream_Internal->seq_info_inread_tab;
    pRamDifBld_From_Data_Stream_Internal->seq_info_inread_size = SIZE_SEQ_INFO_PRE_READ;
    pRamDifBld_From_Data_Stream_Internal->seq_info_inread_pos = 0;

    pRamDifBld_From_Data_Stream_Internal->bytes_in_depl_seq = 0;
    pRamDifBld_From_Data_Stream_Internal->bytes_in_ins_seq = 0;
    pRamDifBld_From_Data_Stream_Internal->pos_in_depl_seq = 0;

    pRamDifBld_From_Data_Stream_Internal->curPos_depl = 0;
    return DSERR_OK;
}




int SVFAPI ClosePatchBldFromDataStream (RAMDIFBLD_FROM_DATA_STREAM * pDifbld,DFSPATCHANALYSEINFO_MEMORY *p_dfsPatchAnalyseInfoToFill)
{
    RAMDIFBLD_FROM_DATA_STREAM_INTERNAL *pRamDifBld_From_Data_Stream_Internal;
    //dfuLong64 dfSizeOfPatchResultRecorded=0;
    int err = DSERR_OK;

    pRamDifBld_From_Data_Stream_Internal = (RAMDIFBLD_FROM_DATA_STREAM_INTERNAL *)(pDifbld->state);

    CloseReadMultiBuffer(&pRamDifBld_From_Data_Stream_Internal->readMultiBuffer);
    DfsFree(pRamDifBld_From_Data_Stream_Internal->pInsByteArray);

    if (p_dfsPatchAnalyseInfoToFill != NULL)
    {
        p_dfsPatchAnalyseInfoToFill->total_size_depl_in_place = pRamDifBld_From_Data_Stream_Internal->total_size_depl_in_place;
        p_dfsPatchAnalyseInfoToFill->total_size_depl_out_place = pRamDifBld_From_Data_Stream_Internal->total_size_depl_out_place;
        p_dfsPatchAnalyseInfoToFill->total_size_insert = pRamDifBld_From_Data_Stream_Internal->total_size_insert;
    }

    DfsFree(pRamDifBld_From_Data_Stream_Internal);

    return err;
}


int DoWriteInsertBytesOutBldPatch(RAMDIFBLD_FROM_DATA_STREAM_INTERNAL *pRamDifBld_From_Data_Stream_Internal,
                          dfvoidp data, dfuLong64 size)
{
    /*return WriteInsertBytesInRamDif(pRamDifBld_From_Data_Stream_Internal->hRamDifBld,
                                    data, size);*/
    pRamDifBld_From_Data_Stream_Internal->data_pos += size;
    pRamDifBld_From_Data_Stream_Internal->total_size_insert += size;
    return (*(pRamDifBld_From_Data_Stream_Internal->Class_Out_Patch.fncWriteInsertBytesFnc))
        (pRamDifBld_From_Data_Stream_Internal->Class_Out_Patch.pUserPtr,
          data,size);
}


int DoWriteInsertDeplOutBldPatch(RAMDIFBLD_FROM_DATA_STREAM_INTERNAL *pRamDifBld_From_Data_Stream_Internal,
                            dfuLong64 posIntPtr, dfuLong64 size)
{
    /*return WriteInsertDeplInRamDif(pRamDifBld_From_Data_Stream_Internal->hRamDifBld,
                                     posIntPtr, size);*/
    if (pRamDifBld_From_Data_Stream_Internal->data_pos == posIntPtr)
    {
        pRamDifBld_From_Data_Stream_Internal->total_size_depl_in_place += size;
    }
    else
    {
        if (pRamDifBld_From_Data_Stream_Internal->OrigDataPtrForInsertNoInPlace==NULL)
            pRamDifBld_From_Data_Stream_Internal->total_size_depl_out_place += size;
        else
        {
            int ret = 0;
            dfuLong64 sizeToDo = size;
            dfuLong64 posIntPtrBrowse = posIntPtr;
            ORIGDATAPTR OrigDataPtr = pRamDifBld_From_Data_Stream_Internal->OrigDataPtrForInsertNoInPlace;
            dfuIntPtr thisInsertSize = GetMaxOrigDataExigibleSizeView(OrigDataPtr);
            while (sizeToDo>0)
            {
                if (thisInsertSize > sizeToDo)
                    thisInsertSize = (dfuLong32)sizeToDo;
                ret =
                    DoWriteInsertBytesOutBldPatch(pRamDifBld_From_Data_Stream_Internal,
                        (((dfbytep) (GetOrigDataPtrpDataBySizeView(OrigDataPtr,posIntPtrBrowse,thisInsertSize)))+posIntPtrBrowse),thisInsertSize);
                sizeToDo -= thisInsertSize;
                posIntPtrBrowse += thisInsertSize;
            }
            return ret;
        }
    }
    pRamDifBld_From_Data_Stream_Internal->data_pos += size;

    return (*(pRamDifBld_From_Data_Stream_Internal->Class_Out_Patch.fncWriteInsertDeplFnc))
        (pRamDifBld_From_Data_Stream_Internal->Class_Out_Patch.pUserPtr,
          posIntPtr,size);
}

int DoFlushOutBldPatch(RAMDIFBLD_FROM_DATA_STREAM_INTERNAL *pRamDifBld_From_Data_Stream_Internal,int final_flush)
{
    return (*(pRamDifBld_From_Data_Stream_Internal->Class_Out_Patch.fncDoFlushOut))
             (pRamDifBld_From_Data_Stream_Internal->Class_Out_Patch.pUserPtr, final_flush);
}

int SVFAPI DoPatchBldFromDataStream (RAMDIFBLD_FROM_DATA_STREAM * pDifbld)
{
  int err = DSERR_OK;
  dfuLong32 is_insert_bytes;
  dfuLong64 size_block, pos_seq;

  RAMDIFBLD_FROM_DATA_STREAM_INTERNAL *pRamDifBld_From_Data_Stream_Internal;
  RAMDIFBLD_FROM_DATA_STREAM_INTERNAL *internal;
  pRamDifBld_From_Data_Stream_Internal = (RAMDIFBLD_FROM_DATA_STREAM_INTERNAL *)(pDifbld->state);

  internal=pRamDifBld_From_Data_Stream_Internal;



  while (err == DSERR_OK)
  {
    /* if we need return to get more space */
    if (DoFlushOutBldPatch(pRamDifBld_From_Data_Stream_Internal,0)==1)
        return DSERR_OK;

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
        err = GetDataFromStream(&internal->readMultiBuffer,& pDifbld->in_data_stream,
                                SEQINFO_STREAM,
                                internal->seq_info_inread_ptr +
                                internal->seq_info_inread_pos,
                                byte_needed, &size_read_in_seq_stream);

        if (err == DSERR_NEED_READMORE)
        {
          return DSERR_OK;
        }

        if ((err == DSERR_OK) && (size_read_in_seq_stream == 0) && (pDifbld->in_data_stream.avail_in == 0))        // we need more data to read
          return DSERR_OK;

        if ((err == DSERR_OK) && (size_read_in_seq_stream == 0))        // we need more data to read
        {
            dfuLong32 i;
            for (i = FIRST_STREAM; i <= LAST_STREAM; i++)
                ReadUntilDataFilledFromStream(&internal->readMultiBuffer,& pDifbld->in_data_stream,
                                              i,pDifbld->in_data_stream.avail_in);
            if (pDifbld->in_data_stream.avail_in != 0)
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
      dfuLong64 doThis = internal->bytes_in_depl_seq;

      if (internal->hRamDifBase == NULL)
      {
          err = DoWriteInsertDeplOutBldPatch(pRamDifBld_From_Data_Stream_Internal,
                                internal->pos_in_depl_seq,
                                doThis);
          internal->pos_in_depl_seq += doThis;
          internal->bytes_in_depl_seq -= doThis;
      }
      else
      {
          dfuLong64 pos_in_dif_base = internal->pos_in_depl_seq;
          dfuLong64 size_in_dif_base = doThis;
          while ((size_in_dif_base>0) && (err==DSERR_OK))
          {
              dfuLong64 doThis_base=0;
              dfuLong32 is_insert_byte_base;
              dfuLong64 posInOrigForDeplSeq_base;
              dfvoidp pDataForInsSeq_base;
              int res_flush;

              err = GetRamDifSeq(internal->hRamDifBase, pos_in_dif_base,
                  size_in_dif_base,
                  dfmin(size_in_dif_base,
                        pRamDifBld_From_Data_Stream_Internal->dfMaxSizeWriteInsByteArray),
                  &is_insert_byte_base,&doThis_base,
                  &posInOrigForDeplSeq_base,
                  &pDataForInsSeq_base);

              if (err == DSERR_END)
                  err = DSERR_OK;

              if (err!=DSERR_OK)
                  break;

              if (is_insert_byte_base != 0)
              {
                  err = DoWriteInsertBytesOutBldPatch(pRamDifBld_From_Data_Stream_Internal,pDataForInsSeq_base,doThis_base);
              }
              else
              {
                  err = DoWriteInsertDeplOutBldPatch(pRamDifBld_From_Data_Stream_Internal,posInOrigForDeplSeq_base,doThis_base);
              }

              pos_in_dif_base += doThis_base;
              size_in_dif_base -= doThis_base;

              internal->pos_in_depl_seq += doThis_base;
              internal->bytes_in_depl_seq -= doThis_base;
              res_flush = DoFlushOutBldPatch(pRamDifBld_From_Data_Stream_Internal,0);
              if (res_flush == 1)
                  break;
          }
      }
    }

    if ((internal->bytes_in_ins_seq > 0) && (err == DSERR_OK))
    {
      dfuLong32 doThis ;

      doThis = (dfuLong32)dfmin(internal->bytes_in_ins_seq,pRamDifBld_From_Data_Stream_Internal->dfSizeInsByteArray);

      err = GetDataFromStream(&internal->readMultiBuffer,& pDifbld->in_data_stream,
                              INSBYTE_STREAM,
                              pRamDifBld_From_Data_Stream_Internal->pInsByteArray,
                              doThis, &doThis);

      if (err == DSERR_NEED_READMORE)
      {
        return DSERR_OK;
      }

      /*if (inadler) */
/*
      read_stream->out_data_stream.next_out = ((dfbytep) read_stream->out_data_stream.next_out) + doThis;
      read_stream->out_data_stream.total_out += doThis;
      read_stream->out_data_stream.avail_out -= doThis;
*/
      err = DoWriteInsertBytesOutBldPatch(pRamDifBld_From_Data_Stream_Internal,
                                     pRamDifBld_From_Data_Stream_Internal->pInsByteArray, doThis);
      internal->bytes_in_ins_seq -= doThis;
    }
  }


  if (err == DSERR_NEED_READMORE)
    err = DSERR_OK;
  if (err != DSERR_END)
      return err;

  //if ((err == DSERR_OK)/* || (err==DSERR_END)*/)
  {
      int flush=DoFlushOutBldPatch(pRamDifBld_From_Data_Stream_Internal,(err==DSERR_END) ? 1:0);
      // 1 = need more space
      if (flush==1)
          return DSERR_OK;
  }

  if (err == DSERR_NEED_READMORE)
    err = DSERR_OK;

  if ((err == DSERR_OK) && (internal->seq_info_inread_pos==0) && (internal->bytes_in_depl_seq==0) && (internal->bytes_in_ins_seq==0))
  {
      err = VerifyEachStreamEnded(&internal->readMultiBuffer);
  }
  return err;
}

/***************************************************************************/
/***************************************************************************/

typedef struct
{
    HRAMDIF hRamDifBld;
} PATCH_OUT_RAMDIF_INTERNAL;



int DFSCALLBACK RamDifWriteInsertBytesFnc(dfvoidp pUserPtr,
                                               dfvoidp data, dfuLong64 size)
{
    PATCH_OUT_RAMDIF_INTERNAL* pPatch_Out_RamDif_Internal;
    pPatch_Out_RamDif_Internal = (PATCH_OUT_RAMDIF_INTERNAL*)pUserPtr;
    return WriteInsertBytesInRamDif(pPatch_Out_RamDif_Internal->hRamDifBld,
                                    data, size);

}

int DFSCALLBACK RamDifWriteInsertDeplFnc(dfvoidp pUserPtr,
                                            dfuLong64 posIntPtr, dfuLong64 size)
{
    PATCH_OUT_RAMDIF_INTERNAL* pPatch_Out_RamDif_Internal;
    pPatch_Out_RamDif_Internal = (PATCH_OUT_RAMDIF_INTERNAL*)pUserPtr;
    return WriteInsertDeplInRamDif(pPatch_Out_RamDif_Internal->hRamDifBld,
                                     posIntPtr, size);
}

int DFSCALLBACK RamDifDoFlushOut(dfvoidp pUserPtr,int final_flush)
{
    return final_flush;
}

int SVFAPI RamDifBldFromDataStreamAndRamDifInit (RAMDIFBLD_FROM_DATA_STREAM * pDifbld,
                                                 dfuLong64 dfExpectedSizeOfPatchResult,
                                                 ORIGDATAPTR OrigDataPtrForInsertNoInPlace,
                                                 HRAMDIF hRamDifBase)
{
    PATCH_OUT_RAMDIF_INTERNAL* pPatch_Out_RamDif_Internal;
    CLASS_OUT_PATCH Class_Out_Patch_Internal;
    int err;
    pPatch_Out_RamDif_Internal = (PATCH_OUT_RAMDIF_INTERNAL*)DfsMalloc(sizeof(PATCH_OUT_RAMDIF_INTERNAL));
    if (pPatch_Out_RamDif_Internal == NULL)
        return DSERR_INTERNAL;

    pPatch_Out_RamDif_Internal->hRamDifBld = CreateRamDif(dfExpectedSizeOfPatchResult);
    if (pPatch_Out_RamDif_Internal->hRamDifBld == NULL)
    {
        DfsFree(pPatch_Out_RamDif_Internal);
        return DSERR_INTERNAL;
    }

    Class_Out_Patch_Internal.fncWriteInsertBytesFnc = RamDifWriteInsertBytesFnc;
    Class_Out_Patch_Internal.fncWriteInsertDeplFnc = RamDifWriteInsertDeplFnc;
    Class_Out_Patch_Internal.fncDoFlushOut = RamDifDoFlushOut;
    Class_Out_Patch_Internal.pUserPtr = (dfvoidp)pPatch_Out_RamDif_Internal;
    Class_Out_Patch_Internal.dfMaxSizeWriteInsByteArray = 0x4000;

    err = PatchBldFromDataStreamAndRamDifInit(pDifbld,
                                            &Class_Out_Patch_Internal,
                                            dfExpectedSizeOfPatchResult,OrigDataPtrForInsertNoInPlace,hRamDifBase);
    if (err != DSERR_OK)
    {
        DeleteRamDif(pPatch_Out_RamDif_Internal->hRamDifBld);
        DfsFree(pPatch_Out_RamDif_Internal);
    }
    return err;
}

int SVFAPI RamDifBldFromDataStreamInit (RAMDIFBLD_FROM_DATA_STREAM * pDifbld, dfuLong64 dfExpectedSizeOfPatchResult)
{
    return RamDifBldFromDataStreamAndRamDifInit(pDifbld, dfExpectedSizeOfPatchResult, NULL, NULL);
}

int SVFAPI DoRamDifBldFromDataStream (RAMDIFBLD_FROM_DATA_STREAM * pDifbld)
{
    return DoPatchBldFromDataStream(pDifbld);
}

int SVFAPI CloseRamDifBldFromDataStream (RAMDIFBLD_FROM_DATA_STREAM * pDifbld,HRAMDIF *phRamDifBld,DFSPATCHANALYSEINFO_MEMORY *p_dfsPatchAnalyseInfoToFill)
{
    int err,err2;
    RAMDIFBLD_FROM_DATA_STREAM_INTERNAL *pRamDifBld_From_Data_Stream_Internal;
    PATCH_OUT_RAMDIF_INTERNAL* pPatch_Out_RamDif_Internal;
    dfuLong64 dfSizeOfPatchResultRecorded=0;

    pRamDifBld_From_Data_Stream_Internal = (RAMDIFBLD_FROM_DATA_STREAM_INTERNAL *)(pDifbld->state);
    pPatch_Out_RamDif_Internal = (PATCH_OUT_RAMDIF_INTERNAL*)pRamDifBld_From_Data_Stream_Internal->Class_Out_Patch.pUserPtr;
////DFSPATCHANALYSEINFO_MEMORY *p_dfsPatchAnalyseInfoToFill = NULL;

 //pPatch_Out_RamDif_Internal->Class_Out_Patch_Internal.pUserPtr = (dfvoidp)pPatch_Out_RamDif_Internal;

    err = ClosePatchBldFromDataStream (pDifbld,p_dfsPatchAnalyseInfoToFill);
    err2 = CloseWritingInRamDif(pPatch_Out_RamDif_Internal->hRamDifBld,&dfSizeOfPatchResultRecorded);

    if ((err == DSERR_OK) && (err2 == DSERR_OK))
    {
        *phRamDifBld = pPatch_Out_RamDif_Internal->hRamDifBld;
    }
    else
    {
        if (err == DSERR_OK)
            err = err2;
        *phRamDifBld=NULL;
        DeleteRamDif(pPatch_Out_RamDif_Internal->hRamDifBld);
    }
    DfsFree(pPatch_Out_RamDif_Internal);
    return err;
}


/***************************************************************************/
/***************************************************************************/
/***************************************************************************/

typedef struct
{
    HRAMDIF hRamDif;
    //dfuLong64 dfExpectedSizeOfPatchResult;
    dfuLong64 dfWritePosition;
    dfuLong64 bytes_in_depl_seq;
    dfuLong64 pos_in_depl_seq;
    dfuLong64 bytes_in_ins_seq;
    dfbytep ptr_ins_seq;

    dfuLong32 uLastAdler32 ;
    dfuLong32 is_end_reached;

} APPLYRAMDIF_INTERNAL;

int SVFAPI ApplyRamDifInit (HRAMDIF hRamDif, APPLYRAMDIF_STREAM * apply_ramdif_stream, BOOL fComputeChecksumSet)
{
    APPLYRAMDIF_INTERNAL* pApplyRamDifInternal;

    pApplyRamDifInternal = (APPLYRAMDIF_INTERNAL*)DfsMalloc(sizeof(APPLYRAMDIF_INTERNAL));
    if (pApplyRamDifInternal == NULL)
        return DSERR_INTERNAL;

    pApplyRamDifInternal->hRamDif = hRamDif;

    apply_ramdif_stream->uDoChecksum = fComputeChecksumSet ? 1 : 0;
    pApplyRamDifInternal->uLastAdler32 = (dfuLong32)adler32(0L, 0, 0);

    pApplyRamDifInternal->bytes_in_depl_seq = 0;
    pApplyRamDifInternal->bytes_in_ins_seq = 0;
    pApplyRamDifInternal->pos_in_depl_seq = 0;

    pApplyRamDifInternal->dfWritePosition = 0;
    pApplyRamDifInternal->is_end_reached = 0;
    apply_ramdif_stream->state = (HAPPLYRAMDIF_INTERNALSTATE)pApplyRamDifInternal;
    return DSERR_OK;
}


int SVFAPI CloseRamApplyDif (APPLYRAMDIF_STREAM * apply_ramdif_stream, dfuLong32 * uChecksum)
{
    APPLYRAMDIF_INTERNAL* pApplyRamDifInternal;
    int err = DSERR_OK;

    pApplyRamDifInternal = (APPLYRAMDIF_INTERNAL*)apply_ramdif_stream->state;
    if (uChecksum != NULL)
    {
        *uChecksum = pApplyRamDifInternal->uLastAdler32;
        if (apply_ramdif_stream->uDoChecksum == 0)
            err = DSERR_INTERNAL;
    }
    DfsFree(pApplyRamDifInternal);
    return err;
}

int SVFAPI DoApplyRamDifWork (APPLYRAMDIF_STREAM * apply_ramdif_stream)
{
  APPLYRAMDIF_INTERNAL* pApplyRamDifInternal;
  int err = DSERR_OK;
  int is_end_reached_and_flush=0;

  pApplyRamDifInternal = (APPLYRAMDIF_INTERNAL*)apply_ramdif_stream->state;

  while ((err == DSERR_OK) && (apply_ramdif_stream->out_data_stream.avail_out > 0))
  {
    if ((pApplyRamDifInternal->bytes_in_depl_seq == 0)
        && (pApplyRamDifInternal->bytes_in_ins_seq == 0))
    {
        dfuLong32 is_insert_byte;
        dfuLong64 dfSize;
        dfuLong64 posInOrigForDeplSeq;
        dfvoidp pDataForInsSeq;

        if (pApplyRamDifInternal->is_end_reached != 0)
        {
            is_end_reached_and_flush = 1;
            break;
        }

        err = GetRamDifSeq(pApplyRamDifInternal->hRamDif,
                            pApplyRamDifInternal->dfWritePosition,
                            apply_ramdif_stream->out_data_stream.avail_out,
                            apply_ramdif_stream->out_data_stream.avail_out,
                            &is_insert_byte,
                            &dfSize,
                            &posInOrigForDeplSeq,
                            &pDataForInsSeq);

        if (err == DSERR_END)
        {
            err = DSERR_OK;
            pApplyRamDifInternal->is_end_reached = 1;
        }

        if (err != DSERR_OK)
            break;

        if (is_insert_byte != 0)
        {
            pApplyRamDifInternal->bytes_in_ins_seq = dfSize;
            pApplyRamDifInternal->ptr_ins_seq = pDataForInsSeq;
        }
        else
        {
            pApplyRamDifInternal->bytes_in_depl_seq = dfSize;
            pApplyRamDifInternal->pos_in_depl_seq = posInOrigForDeplSeq;
        }
    }
/*********************/


    if ((pApplyRamDifInternal->bytes_in_depl_seq > 0) && (err == DSERR_OK))
    {
      dfuLong32 doThis ;

      if (pApplyRamDifInternal->bytes_in_depl_seq < apply_ramdif_stream->out_data_stream.avail_out)
          doThis = (dfuLong32)pApplyRamDifInternal->bytes_in_depl_seq;
      else
          doThis = apply_ramdif_stream->out_data_stream.avail_out;

      {
          dfuLong32 doThisMemCopy;
          dfuLong32 doThisMemCopyPos = 0;
          doThisMemCopy = doThis;
          while (doThisMemCopy>0)
          {
            dfuLong32 doThisMemCopyStep = (dfuLong32)(dfmin(doThisMemCopy,
                                                        GetMaxOrigDataExigibleSizeView(apply_ramdif_stream->OrigDataPtr)));
/*
//++ CMP
            if (memcmp(((dfbytep)(apply_ramdif_stream->out_data_stream.next_out)) + doThisMemCopyPos,
                        ((dfbytep) (GetOrigDataPtrpDataBySizeView(apply_ramdif_stream->OrigDataPtr,
                          pApplyRamDifInternal->pos_in_depl_seq + doThisMemCopyPos, doThisMemCopyStep))) +
                        pApplyRamDifInternal->pos_in_depl_seq + doThisMemCopyPos, doThisMemCopyStep) !=0)
                        printf(".");
//++ CMP
*/
            DfsMemcpy(((dfbytep)(apply_ramdif_stream->out_data_stream.next_out)) + doThisMemCopyPos,
                        ((dfbytep) (GetOrigDataPtrpDataBySizeView(apply_ramdif_stream->OrigDataPtr,
                          pApplyRamDifInternal->pos_in_depl_seq + doThisMemCopyPos, doThisMemCopyStep))) +
                        pApplyRamDifInternal->pos_in_depl_seq + doThisMemCopyPos, doThisMemCopyStep);
            doThisMemCopy -= doThisMemCopyStep;
            doThisMemCopyPos += doThisMemCopyStep;
          }
      }


      if (apply_ramdif_stream->uDoChecksum != 0)
          pApplyRamDifInternal->uLastAdler32 = (dfuLong32)adler32(pApplyRamDifInternal->uLastAdler32,
                                       (dfbytep)apply_ramdif_stream->out_data_stream.next_out, doThis);

      apply_ramdif_stream->out_data_stream.next_out = ((dfbytep) apply_ramdif_stream->out_data_stream.next_out) + doThis;
      apply_ramdif_stream->out_data_stream.total_out += doThis;
      apply_ramdif_stream->out_data_stream.avail_out -= doThis;

      pApplyRamDifInternal->dfWritePosition += doThis;

      pApplyRamDifInternal->pos_in_depl_seq += doThis;
      pApplyRamDifInternal->bytes_in_depl_seq -= doThis;
    }

    if ((pApplyRamDifInternal->bytes_in_ins_seq > 0) && (err == DSERR_OK))
    {
      dfuLong32 doThis ;

      if (pApplyRamDifInternal->bytes_in_ins_seq < apply_ramdif_stream->out_data_stream.avail_out)
          doThis = (dfuLong32)(pApplyRamDifInternal->bytes_in_ins_seq);
      else
          doThis = apply_ramdif_stream->out_data_stream.avail_out;


//++ CMP
      /*
            if (memcmp((dfbytep)apply_ramdif_stream->out_data_stream.next_out,pApplyRamDifInternal->ptr_ins_seq,doThis) !=0)
                        printf(".");
                        */
//++ CMP

      DfsMemcpy((dfbytep)apply_ramdif_stream->out_data_stream.next_out,pApplyRamDifInternal->ptr_ins_seq,doThis);

      if (apply_ramdif_stream->uDoChecksum != 0)
          pApplyRamDifInternal->uLastAdler32 = (dfuLong32)adler32(pApplyRamDifInternal->uLastAdler32,
                                       (dfbytep)apply_ramdif_stream->out_data_stream.next_out, doThis);

      apply_ramdif_stream->out_data_stream.next_out = ((dfbytep) apply_ramdif_stream->out_data_stream.next_out) + doThis;
      apply_ramdif_stream->out_data_stream.total_out += doThis;
      apply_ramdif_stream->out_data_stream.avail_out -= doThis;

      pApplyRamDifInternal->dfWritePosition += doThis;

      pApplyRamDifInternal->bytes_in_ins_seq -= doThis;
      pApplyRamDifInternal->ptr_ins_seq += doThis;
    }
  }

  if (is_end_reached_and_flush == 1)
      if (err == DSERR_OK)
          err = DSERR_END;

  return err;
}



/***************************************************************************/
/***************************************************************************/
/***************************************************************************/

typedef struct
{
    HRAMDIF hRamDif;
    //dfuLong64 dfExpectedSizeOfPatchResult;
    dfuLong64 dfWritePosition;
    dfuLong64 bytes_in_depl_seq;
    dfuLong64 pos_in_depl_seq;
    dfuLong64 bytes_in_ins_seq;
    dfbytep ptr_ins_seq;

    dfuLong32 uLastAdler32 ;
    dfuLong32 is_end_reached;

} APPLYRAMDIFINPLACE_INTERNAL;

int SVFAPI ApplyRamDifInPlaceInit (HRAMDIF hRamDif, APPLYRAMDIFINPLACE_STREAM * apply_ramdif_inplace_stream, BOOL fComputeChecksumSet)
{
    APPLYRAMDIFINPLACE_INTERNAL* pApplyRamDifInPlaceInternal;

    pApplyRamDifInPlaceInternal = (APPLYRAMDIFINPLACE_INTERNAL*)DfsMalloc(sizeof(APPLYRAMDIFINPLACE_INTERNAL));
    if (pApplyRamDifInPlaceInternal == NULL)
        return DSERR_INTERNAL;

    pApplyRamDifInPlaceInternal->hRamDif = hRamDif;

    apply_ramdif_inplace_stream->uDoChecksum = fComputeChecksumSet ? 1 : 0;
    pApplyRamDifInPlaceInternal->uLastAdler32 = (dfuLong32)adler32(0L, 0, 0);

    pApplyRamDifInPlaceInternal->bytes_in_depl_seq = 0;
    pApplyRamDifInPlaceInternal->bytes_in_ins_seq = 0;
    pApplyRamDifInPlaceInternal->pos_in_depl_seq = 0;

    pApplyRamDifInPlaceInternal->dfWritePosition = 0;
    pApplyRamDifInPlaceInternal->is_end_reached = 0;
    apply_ramdif_inplace_stream->state = (HAPPLYRAMDIFINPLACE_INTERNALSTATE)pApplyRamDifInPlaceInternal;
    return DSERR_OK;
}


int SVFAPI CloseRamApplyInPlaceDif (APPLYRAMDIFINPLACE_STREAM * apply_ramdif_inplace_stream, dfuLong32 * uChecksum)
{
    APPLYRAMDIFINPLACE_INTERNAL* pApplyRamDifInPlaceInternal;
    int err = DSERR_OK;

    pApplyRamDifInPlaceInternal = (APPLYRAMDIFINPLACE_INTERNAL*)apply_ramdif_inplace_stream->state;
    if (uChecksum != NULL)
    {
        *uChecksum = pApplyRamDifInPlaceInternal->uLastAdler32;
        if (apply_ramdif_inplace_stream->uDoChecksum == 0)
            err = DSERR_INTERNAL;
    }
    DfsFree(pApplyRamDifInPlaceInternal);
    return err;
}

int SVFAPI DoApplyRamDifInPlaceWork(APPLYRAMDIFINPLACE_STREAM * apply_ramdif_inplace_stream)
{
    BOOL fDummyDirty = FALSE;
    return DoApplyRamDifInPlaceWorkWithDirtyFlag(apply_ramdif_inplace_stream, &fDummyDirty);
}

int SVFAPI DoApplyRamDifInPlaceWorkWithDirtyFlag(APPLYRAMDIFINPLACE_STREAM * apply_ramdif_inplace_stream, BOOL *pfDirty)
{
  APPLYRAMDIFINPLACE_INTERNAL* pApplyRamDifInPlaceInternal;
  int err = DSERR_OK;
  int is_end_reached_and_flush=0;
  *pfDirty = FALSE;

  pApplyRamDifInPlaceInternal = (APPLYRAMDIFINPLACE_INTERNAL*)apply_ramdif_inplace_stream->state;

  while ((err == DSERR_OK) && (apply_ramdif_inplace_stream->out_data_stream.avail_out > 0))
  {
    if ((pApplyRamDifInPlaceInternal->bytes_in_depl_seq == 0)
        && (pApplyRamDifInPlaceInternal->bytes_in_ins_seq == 0))
    {
        dfuLong32 is_insert_byte;
        dfuLong64 dfSize;
        dfuLong64 posInOrigForDeplSeq;
        dfvoidp pDataForInsSeq;

        if (pApplyRamDifInPlaceInternal->is_end_reached != 0)
        {
            is_end_reached_and_flush = 1;
            break;
        }


        err = GetRamDifSeq(pApplyRamDifInPlaceInternal->hRamDif,
                            pApplyRamDifInPlaceInternal->dfWritePosition,
                            apply_ramdif_inplace_stream->out_data_stream.avail_out,
                            apply_ramdif_inplace_stream->out_data_stream.avail_out,
                            &is_insert_byte,
                            &dfSize,
                            &posInOrigForDeplSeq,
                            &pDataForInsSeq);

        if (err == DSERR_END)
        {
            err = DSERR_OK;
            pApplyRamDifInPlaceInternal->is_end_reached = 1;
        }

        if (err != DSERR_OK)
            break;

        if (is_insert_byte != 0)
        {
            pApplyRamDifInPlaceInternal->bytes_in_ins_seq = dfSize;
            pApplyRamDifInPlaceInternal->ptr_ins_seq = pDataForInsSeq;
        }
        else
        {
            pApplyRamDifInPlaceInternal->bytes_in_depl_seq = dfSize;
            pApplyRamDifInPlaceInternal->pos_in_depl_seq = posInOrigForDeplSeq;

            if (pApplyRamDifInPlaceInternal->pos_in_depl_seq != pApplyRamDifInPlaceInternal->dfWritePosition)
            {
                // here we are not inplace!
                return DSERR_INTERNAL;
            }

        }
    }
/*********************/


    if ((pApplyRamDifInPlaceInternal->bytes_in_depl_seq > 0) && (err == DSERR_OK))
    {
      dfuLong32 doThis ;

      if (pApplyRamDifInPlaceInternal->bytes_in_depl_seq < apply_ramdif_inplace_stream->out_data_stream.avail_out)
          doThis = (dfuLong32)pApplyRamDifInPlaceInternal->bytes_in_depl_seq;
      else
          doThis = apply_ramdif_inplace_stream->out_data_stream.avail_out;

      {
          dfuLong32 doThisMemCopy;
          dfuLong32 doThisMemCopyPos = 0;
          doThisMemCopy = doThis;
          while (doThisMemCopy>0)
          {
            dfuLong32 doThisMemCopyStep = doThisMemCopy;
              /*
            dfuLong32 doThisMemCopyStep = (dfuLong32)(dfmin(doThisMemCopy,
                                                        GetMaxOrigDataExigibleSizeView(apply_ramdif_stream->OrigDataPtr)));


            DfsMemcpy(((dfbytep)(apply_ramdif_stream->out_data_stream.next_out)) + doThisMemCopyPos,
                        ((dfbytep) (GetOrigDataPtrpDataBySizeView(apply_ramdif_stream->OrigDataPtr,
                          pApplyRamDifInternal->pos_in_depl_seq + doThisMemCopyPos, doThisMemCopyStep))) +
                        pApplyRamDifInternal->pos_in_depl_seq + doThisMemCopyPos, doThisMemCopyStep);
                        */
            doThisMemCopy -= doThisMemCopyStep;
            doThisMemCopyPos += doThisMemCopyStep;
          }
      }


      /*if (inadler) */
      if (apply_ramdif_inplace_stream->uDoChecksum != 0)
          pApplyRamDifInPlaceInternal->uLastAdler32 = (dfuLong32)adler32(pApplyRamDifInPlaceInternal->uLastAdler32,
                                       (dfbytep)apply_ramdif_inplace_stream->out_data_stream.next_out, doThis);

      apply_ramdif_inplace_stream->out_data_stream.next_out = ((dfbytep) apply_ramdif_inplace_stream->out_data_stream.next_out) + doThis;
      apply_ramdif_inplace_stream->out_data_stream.total_out += doThis;
      apply_ramdif_inplace_stream->out_data_stream.avail_out -= doThis;

      pApplyRamDifInPlaceInternal->dfWritePosition += doThis;

      pApplyRamDifInPlaceInternal->pos_in_depl_seq += doThis;
      pApplyRamDifInPlaceInternal->bytes_in_depl_seq -= doThis;
    }

    if ((pApplyRamDifInPlaceInternal->bytes_in_ins_seq > 0) && (err == DSERR_OK))
    {
      dfuLong32 doThis ;

      if (pApplyRamDifInPlaceInternal->bytes_in_ins_seq < apply_ramdif_inplace_stream->out_data_stream.avail_out)
          doThis = (dfuLong32)(pApplyRamDifInPlaceInternal->bytes_in_ins_seq);
      else
          doThis = apply_ramdif_inplace_stream->out_data_stream.avail_out;


//++ CMP
      /*
            if (memcmp((dfbytep)apply_ramdif_stream->out_data_stream.next_out,pApplyRamDifInternal->ptr_ins_seq,doThis) !=0)
                        printf(".");
                        */
//++ CMP

      DfsMemcpy((dfbytep)apply_ramdif_inplace_stream->out_data_stream.next_out,pApplyRamDifInPlaceInternal->ptr_ins_seq,doThis);

      if (apply_ramdif_inplace_stream->uDoChecksum != 0)
          pApplyRamDifInPlaceInternal->uLastAdler32 = (dfuLong32)adler32(pApplyRamDifInPlaceInternal->uLastAdler32,
                                       (dfbytep)apply_ramdif_inplace_stream->out_data_stream.next_out, doThis);

      apply_ramdif_inplace_stream->out_data_stream.next_out = ((dfbytep) apply_ramdif_inplace_stream->out_data_stream.next_out) + doThis;
      apply_ramdif_inplace_stream->out_data_stream.total_out += doThis;
      apply_ramdif_inplace_stream->out_data_stream.avail_out -= doThis;

      pApplyRamDifInPlaceInternal->dfWritePosition += doThis;

      pApplyRamDifInPlaceInternal->bytes_in_ins_seq -= doThis;
      pApplyRamDifInPlaceInternal->ptr_ins_seq += doThis;
      *pfDirty = TRUE;
    }
  }

  if (is_end_reached_and_flush == 1)
      if (err == DSERR_OK)
          err = DSERR_END;

  return err;
}


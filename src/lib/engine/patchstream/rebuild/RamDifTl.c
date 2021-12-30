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

#include "../common/DfsOrigMemoryMap.h"

#include "RamDifWk.h"
#include "RamDifTl.h"




typedef struct
{
    dfvoidp pData;
    dfuLong32 dfDifEncodedDataUsed;
    dfuLong32 dfDifEncodedDataAllocated;
    dfuLong32 dfDifResultSize;
    dfuLong32 dfNbSequence;

    dfuLong32 dfNbQuickJumpEntryWritten;
    dfuLong32 dfQuickJumpFirstLoopStep;
    dfuLong32 *pQuickJumpResultPosArray;
    dfuLong32 *pQuickJumpEncodedPosArray;
} RAMDIFARRAY;


typedef struct
{
    dfuLong64 dfCurrentResultPositionAdded;

    dfuLong32 dfCurrentDifArray;
    dfuLong32 dfCurrentDifArrayPositionEncoded;
    dfuLong64 dfCurrentResultPositionOfDifArray;
    dfuLong32 dfCurrentResultPositionInDifArray;

    dfuLong32 dfSize_in_depl;
    dfuLong64 dfPosition_in_depl;
    dfuLong32 dfSize_in_ins;
} CURRENT_READ_PTR_RAMDIF;

typedef struct
{
    dfuLong64 dfExpectedSizeOfPatchResult;
    dfuLong64 dfSizePatchResult;
    dfuLong32 dfNbBytePatchResultByDiffArray;

    dfuLong32 dfNbRamDifArray;
    dfuLong32 dfCurRamDifArrayWriting;

    dfuLong32 dfNbQuickJumpArrayByDifArray;
    dfuLong32 dfQuickJumpArrayInitialGranularity;

    dfuLong32 dfStepAlloc;
    dfuLong32 dfWritePossible;
	dfuLong32 countUsage;

    CURRENT_READ_PTR_RAMDIF current_read_ptr_ramdif;
    DFSPATCHANALYSEINFO_MEMORY dfsPatchAnalyseInfo;

    /* dfQuickJumpArrayGranularity * dfNbQuickJumpArrayByDifArray = dfNbBytePatchResultByDiffArray */
    RAMDIFARRAY* pRamDifArray;
} RAMDIF_INTERNAL;

int IndexOnQuickJumpCurrentEntry(RAMDIF_INTERNAL *pRamDifInternal,dfuLong32 dfSelectRamDif);

int BuildNextRamDifArray(RAMDIF_INTERNAL *pRamDifInternal)
{
    dfuLong32 dfNbRamDifArrayNeeded = pRamDifInternal->dfNbRamDifArray +1;
    dfuLong32 dfuSizeRamDifArrayNeeded = sizeof(RAMDIFARRAY)*(dfNbRamDifArrayNeeded);
    RAMDIFARRAY* pNewRamDifArray;
    int err=DSERR_OK;

    if (pRamDifInternal->pRamDifArray == NULL)
        pNewRamDifArray = (RAMDIFARRAY*)DfsMalloc(dfuSizeRamDifArrayNeeded);
    else
        pNewRamDifArray = (RAMDIFARRAY*)DfsRealloc(pRamDifInternal->pRamDifArray,dfuSizeRamDifArrayNeeded);
    if (pNewRamDifArray == NULL)
    {
        return DSERR_INTERNAL;
    }

    pRamDifInternal->pRamDifArray = pNewRamDifArray;

    ((pRamDifInternal->pRamDifArray)+dfNbRamDifArrayNeeded-1)->dfDifEncodedDataAllocated = 0;
    ((pRamDifInternal->pRamDifArray)+dfNbRamDifArrayNeeded-1)->dfDifEncodedDataUsed = 0;
    ((pRamDifInternal->pRamDifArray)+dfNbRamDifArrayNeeded-1)->dfDifResultSize = 0;
    ((pRamDifInternal->pRamDifArray)+dfNbRamDifArrayNeeded-1)->dfNbQuickJumpEntryWritten = 0;
    ((pRamDifInternal->pRamDifArray)+dfNbRamDifArrayNeeded-1)->pData = NULL;
    ((pRamDifInternal->pRamDifArray)+dfNbRamDifArrayNeeded-1)->dfNbSequence = 0;


    ((pRamDifInternal->pRamDifArray)+dfNbRamDifArrayNeeded-1)->dfNbQuickJumpEntryWritten = 0;
    ((pRamDifInternal->pRamDifArray)+dfNbRamDifArrayNeeded-1)->dfQuickJumpFirstLoopStep = 0;
    ((pRamDifInternal->pRamDifArray)+dfNbRamDifArrayNeeded-1)->pQuickJumpResultPosArray = NULL;
    ((pRamDifInternal->pRamDifArray)+dfNbRamDifArrayNeeded-1)->pQuickJumpEncodedPosArray = NULL;

    pRamDifInternal->dfNbRamDifArray = dfNbRamDifArrayNeeded;
    if ((err==DSERR_OK) && (pRamDifInternal->dfNbRamDifArray >1))
      err = IndexOnQuickJumpCurrentEntry(pRamDifInternal,pRamDifInternal->dfNbRamDifArray-2) ;

    return err;
}

int DecodeHeadPosition(RAMDIF_INTERNAL *pRamDifInternal,CURRENT_READ_PTR_RAMDIF* pcurrent_read_ptr_ramdif);
int GotoPositionSelectBeginRamDif(RAMDIF_INTERNAL *pRamDifInternal,dfuLong32 dfSelectRamDif,
                                  CURRENT_READ_PTR_RAMDIF* pcurrent_read_ptr_ramdif);

#define NB_SEQ_BY_CELL (4)
int IndexOnQuickJumpCurrentEntry(RAMDIF_INTERNAL *pRamDifInternal,dfuLong32 dfSelectRamDif)
{
    dfuLong32 dfNbQuickJumpEntry = 1;
    int err;
    dfuLong32 dfCurrentSeqNum=0;
    dfuLong32 dfCurFilling=0;
    CURRENT_READ_PTR_RAMDIF current_read_ptr_index;
    RAMDIFARRAY* pCurRamDifArray ;
    dfuLong32 dfCurrentPosWritten = 0xffffffff*0;

    pCurRamDifArray = ((pRamDifInternal->pRamDifArray) + dfSelectRamDif);
/*                                       (pRamDifInternal->current_read_ptr_ramdif.dfCurrentDifArray);*/

    while ((dfNbQuickJumpEntry*NB_SEQ_BY_CELL) <= (pCurRamDifArray->dfNbSequence))
    {
        dfNbQuickJumpEntry*=2;
    }
    if (dfNbQuickJumpEntry <= 2)
        return DSERR_OK;

    err = GotoPositionSelectBeginRamDif(pRamDifInternal,dfSelectRamDif,&current_read_ptr_index);
    if (err != DSERR_OK)
        return err;


    // we known (dfNbQuickJumpEntry*NB_SEQ_BY_CELL) > (pRamDifArray->dfNbSequence)

    pCurRamDifArray -> pQuickJumpResultPosArray = (dfuLong32*)DfsMalloc(dfNbQuickJumpEntry * sizeof(dfuLong32) * 2);
    pCurRamDifArray -> pQuickJumpEncodedPosArray = pCurRamDifArray -> pQuickJumpResultPosArray + dfNbQuickJumpEntry;

    while (current_read_ptr_index.dfCurrentResultPositionInDifArray < pCurRamDifArray->dfDifResultSize)
    {
        dfuLong32 dfCurPosition;
        dfCurPosition = (dfuLong32)((dfCurrentSeqNum*(dfuLong64)dfNbQuickJumpEntry)/pCurRamDifArray->dfNbSequence);
        if (dfCurPosition != dfCurrentPosWritten)
        {
            *((pCurRamDifArray -> pQuickJumpResultPosArray)  + (dfCurFilling)) = current_read_ptr_index.dfCurrentResultPositionInDifArray;
            *((pCurRamDifArray -> pQuickJumpEncodedPosArray) + (dfCurFilling)) = current_read_ptr_index.dfCurrentDifArrayPositionEncoded;
            dfCurFilling++;
            dfCurrentPosWritten=dfCurPosition;
        }

        DecodeHeadPosition(pRamDifInternal,&current_read_ptr_index);

        if (current_read_ptr_index.dfSize_in_ins>0)
        {
            current_read_ptr_index.dfCurrentDifArrayPositionEncoded += current_read_ptr_index.dfSize_in_ins;

            current_read_ptr_index.dfCurrentResultPositionInDifArray += current_read_ptr_index.dfSize_in_ins;
            current_read_ptr_index.dfCurrentResultPositionAdded += current_read_ptr_index.dfSize_in_ins;
            current_read_ptr_index.dfSize_in_ins = 0;
        }

        if (current_read_ptr_index.dfSize_in_depl>0)
        {
            current_read_ptr_index.dfPosition_in_depl += current_read_ptr_index.dfSize_in_depl;

            current_read_ptr_index.dfCurrentResultPositionInDifArray += current_read_ptr_index.dfSize_in_depl;
            current_read_ptr_index.dfCurrentResultPositionAdded += current_read_ptr_index.dfSize_in_depl;
            current_read_ptr_index.dfSize_in_depl = 0;
        }
        dfCurrentSeqNum++;
    }
    pCurRamDifArray -> dfNbQuickJumpEntryWritten = dfCurFilling;
    pCurRamDifArray -> dfQuickJumpFirstLoopStep = dfNbQuickJumpEntry/2;

    return DSERR_OK;
}

void IncrementUsageRamDif(HRAMDIF hRamDif)
{
    RAMDIF_INTERNAL *pRamDifInternal = (RAMDIF_INTERNAL *)hRamDif;
    if (pRamDifInternal != NULL)
    {
        (pRamDifInternal->countUsage)++;
    }
}

void DeleteRamDif(HRAMDIF hRamDif)
{
    RAMDIF_INTERNAL *pRamDifInternal = (RAMDIF_INTERNAL *)hRamDif;
    if (pRamDifInternal!=NULL)
    {
        if (pRamDifInternal->countUsage > 1)
        {
           (pRamDifInternal->countUsage) -- ;
            return;
        }

        dfuLong32 i;
        for (i=0;i<pRamDifInternal->dfNbRamDifArray;i++)
        {
            if (((pRamDifInternal->pRamDifArray)+i)-> pQuickJumpResultPosArray != NULL)
                DfsFree(((pRamDifInternal->pRamDifArray)+i)->pQuickJumpResultPosArray);
            if (((pRamDifInternal->pRamDifArray)+i)->pData != NULL)
                DfsFree(((pRamDifInternal->pRamDifArray)+i)->pData);
        }
        if (pRamDifInternal->pRamDifArray != NULL)
            DfsFree(pRamDifInternal->pRamDifArray);
        DfsFree(pRamDifInternal);
    }
}


HRAMDIF CreateRamDif(dfuLong64 dfExpectedSizeOfPatchResult)
{
    RAMDIF_INTERNAL *pRamDifInternal;
    dfuLong32 dfNbRamDifArrayNeeded;

    pRamDifInternal = (RAMDIF_INTERNAL*)DfsMalloc(sizeof(RAMDIF_INTERNAL));
    pRamDifInternal->countUsage = 1;
    pRamDifInternal->dfExpectedSizeOfPatchResult = dfExpectedSizeOfPatchResult;
    pRamDifInternal->dfNbBytePatchResultByDiffArray = 256 * 256 * 16;
    pRamDifInternal->dfNbRamDifArray = 0;
    pRamDifInternal->pRamDifArray = NULL;
    pRamDifInternal->dfSizePatchResult = 0;
    pRamDifInternal->dfCurRamDifArrayWriting = 0;
    pRamDifInternal->dfWritePossible = 1;

    pRamDifInternal->dfStepAlloc = 0x1000;
    pRamDifInternal->dfQuickJumpArrayInitialGranularity = 0x400;
    pRamDifInternal->dfNbQuickJumpArrayByDifArray = 0x40;
    pRamDifInternal->dfQuickJumpArrayInitialGranularity =
         pRamDifInternal->dfNbBytePatchResultByDiffArray /
           pRamDifInternal->dfNbQuickJumpArrayByDifArray;
    /* dfQuickJumpArrayGranularity * dfNbQuickJumpArrayByDifArray = dfNbBytePatchResultByDiffArray */

    pRamDifInternal->dfsPatchAnalyseInfo.total_size_depl_in_place = 0;
    pRamDifInternal->dfsPatchAnalyseInfo.total_size_depl_out_place = 0;
    pRamDifInternal->dfsPatchAnalyseInfo.total_size_insert = 0;

    dfNbRamDifArrayNeeded = (dfuLong32)((dfExpectedSizeOfPatchResult + pRamDifInternal->dfNbBytePatchResultByDiffArray - 1) /
                                  pRamDifInternal->dfNbBytePatchResultByDiffArray);

    //if (AddRamDifArray(pRamDifInternal,dfNbRamDifArrayNeeded) != DSERR_OK)
    if (BuildNextRamDifArray(pRamDifInternal) != DSERR_OK)
    {
        DeleteRamDif((HRAMDIF)pRamDifInternal);
        return NULL;
    }

    return (HRAMDIF)pRamDifInternal;
}



int PrepareRamDifDataBuffer(RAMDIF_INTERNAL *pRamDifInternal,
                            dfuLong32 dfNbByteToWrite)
{
    RAMDIFARRAY* pRamDifArray;
    dfuLong32 dfDataAllocNeeded;
    /*
    *pNumRamDifArray =(dfuLong32)(pRamDifInternal->dfSizePatchResult /
                        pRamDifInternal->dfNbBytePatchResultByDiffArray);
*/
    pRamDifArray = ((pRamDifInternal->pRamDifArray) +
                       (pRamDifInternal->dfCurRamDifArrayWriting));
    dfDataAllocNeeded = pRamDifArray->dfDifEncodedDataUsed + dfNbByteToWrite;
    if (dfDataAllocNeeded > pRamDifArray->dfDifEncodedDataAllocated)
    {
        dfvoidp pNewData;
        dfuLong32 dfDifEncodedDataAllocatedTry =
             ((dfDataAllocNeeded / pRamDifInternal->dfStepAlloc) + 1) *
                      pRamDifInternal->dfStepAlloc;

        pNewData = DfsRealloc(pRamDifArray->pData,dfDifEncodedDataAllocatedTry+0x10);
        if (pNewData == NULL)
            return DSERR_INTERNAL;
        pRamDifArray->pData = pNewData;
        pRamDifArray->dfDifEncodedDataAllocated = dfDifEncodedDataAllocatedTry;
    }

    return DSERR_OK;
}


/*****/

void FlushNumber(dfbytep pBin,dfuLong64 value64,dfbyte dfNbByte)
{
    dfbyte i;
    for (i=0;i<dfNbByte;i++)
    {
        *(pBin+i) = (dfbyte)(value64 & 0xff);
        value64 = value64 >> 8;
    }
}

/*
dfbyte GetNbByteForNumber32(dfuLong32 size)
{
  return (size < 0x100) ? 1 : (size < 0x10000) ? 2 :
    (size < 0x1000000) ? 3 : 4;
}

dfbyte GetNbByteForNumber64(dfuLong64 value64)
{
    dfuLong32 valueLow = (dfuLong32)value64;
    dfuLong32 valueHigh = (dfuLong32)(value64>>32);
    if (valueHigh != 0)
        return GetNbByteForNumber32(valueHigh)+4;
    else
        return GetNbByteForNumber32(valueLow);
}

dfuLong32 ReadNumber32FromByteArray(dfbytepc pNum,dfuLong32 dfNbByte)
{
    dfuLong32 dfRet=0;
    while (dfNbByte>0)
    {
        dfNbByte--;
        dfRet = (*(pNum+dfNbByte)) | (dfRet << 8);
    }
    return dfRet;
}

dfuLong64 ReadNumber64FromByteArray(dfbytepc pNum,dfuLong32 dfNbByte)
{
    dfuLong64 dfRet=0;
    while (dfNbByte>0)
    {
        dfNbByte--;
        dfRet = (*(pNum+dfNbByte)) | (dfRet << 8);
    }
    return dfRet;
}
*/


dfbyte GetNbByteForNumber32(dfuLong32 value)
{
    if ((value & 0xffff0000) == 0)
    {
        return ((value & 0xff00) == 0) ? 1 : 2;
    }
    else
    {
        return ((value & 0xff000000) == 0) ? 3 : 4;
    }
}


dfbyte GetNbByteForNumber64(dfuLong64 value64)
{
    dfuLong32 valueLow = (dfuLong32)value64;
    dfuLong32 valueHigh = (dfuLong32)(value64>>32);
    if (valueHigh != 0)
    {
        if ((valueHigh & 0xffff0000) == 0)
        {
            return ((valueHigh & 0xff00) == 0) ? 5 : 6;
        }
        else
        {
            return ((valueHigh & 0xff000000) == 0) ? 7 : 8;
        }
    }
    else
    {
        if ((valueLow & 0xffff0000) == 0)
        {
            return ((valueLow & 0xff00) == 0) ? 1 : 2;
        }
        else
        {
            return ((valueLow & 0xff000000) == 0) ? 3 : 4;
        }
    }
}


dfuLong32 ReadNumber32FromByteArray(dfbytepc pNum,dfuLong32 dfNbByte)
{
    dfuLong32 dfRet;

    switch (dfNbByte)
    {
    case 0 :
        return 0;
    case 1 :
        return  (((dfuLong32)(*(pNum)+0)) << 0);
    case 2 :
        return  (((dfuLong32)(*(pNum)+0)) << 0) |
                (((dfuLong32)(*(pNum+1))) << 8);
    case 3 :
        return  (((dfuLong32)(*(pNum+0))) << 0) |
                (((dfuLong32)(*(pNum+1))) << 8) |
                (((dfuLong32)(*(pNum+2))) << 16);
    case 4 :
        return  (((dfuLong32)(*(pNum+0))) << 0) |
                (((dfuLong32)(*(pNum+1))) << 8) |
                (((dfuLong32)(*(pNum+2))) << 16)|
                (((dfuLong32)(*(pNum+3))) << 24);
    }

    dfRet = 0;
    while (dfNbByte>0)
    {
        dfNbByte--;
        dfRet = (*(pNum+dfNbByte)) | (dfRet << 8);
    }

    return dfRet;
}


dfuLong64 ReadNumber64FromByteArray(dfbytepc pNum,dfuLong32 dfNbByte)
{
    dfuLong64 dfRet=0;

    switch (dfNbByte)
    {
    case 0 :
        return 0;
    case 1 :
        return  (((dfuLong32)(*(pNum)+0)) << 0);
    case 2 :
        return  (((dfuLong32)(*(pNum)+0)) << 0) |
                (((dfuLong32)(*(pNum+1))) << 8);
    case 3 :
        return  (((dfuLong32)(*(pNum+0))) << 0) |
                (((dfuLong32)(*(pNum+1))) << 8) |
                (((dfuLong32)(*(pNum+2))) << 16);
    case 4 :
        return  (((dfuLong32)(*(pNum+0))) << 0) |
                (((dfuLong32)(*(pNum+1))) << 8) |
                (((dfuLong32)(*(pNum+2))) << 16)|
                (((dfuLong32)(*(pNum+3))) << 24);
    case 5 :
        return  (((dfuLong32)(*(pNum+0))) << 0) |
                (((dfuLong32)(*(pNum+1))) << 8) |
                (((dfuLong32)(*(pNum+2))) << 16)|
                (((dfuLong32)(*(pNum+3))) << 24)|
                (((dfuLong64)(*(pNum+4))) << 32);
    case 6 :
        return  (((dfuLong32)(*(pNum+0))) << 0) |
                (((dfuLong32)(*(pNum+1))) << 8) |
                (((dfuLong32)(*(pNum+2))) << 16)|
                (((dfuLong32)(*(pNum+3))) << 24)|
                (((dfuLong64)(*(pNum+4))) << 32)|
                (((dfuLong64)(*(pNum+5))) << 40);
    case 7 :
        return  (((dfuLong32)(*(pNum+0))) << 0) |
                (((dfuLong32)(*(pNum+1))) << 8) |
                (((dfuLong32)(*(pNum+2))) << 16)|
                (((dfuLong32)(*(pNum+3))) << 24)|
                (((dfuLong64)(*(pNum+4))) << 32)|
                (((dfuLong64)(*(pNum+5))) << 40)|
                (((dfuLong64)(*(pNum+6))) << 48);
    case 8 :
        return  (((dfuLong32)(*(pNum+0))) << 0) |
                (((dfuLong32)(*(pNum+1))) << 8) |
                (((dfuLong32)(*(pNum+2))) << 16)|
                (((dfuLong32)(*(pNum+3))) << 24)|
                (((dfuLong64)(*(pNum+4))) << 32)|
                (((dfuLong64)(*(pNum+5))) << 40)|
                (((dfuLong64)(*(pNum+6))) << 48)|
                (((dfuLong64)(*(pNum+7))) << 56);
    }

    dfRet = 0;
    while (dfNbByte>0)
    {
        dfNbByte--;
        dfRet = (*(pNum+dfNbByte)) | (dfRet << 8);
    }
    return dfRet;
}

/*
sequence in RamDif :
header byte
bit 0 :
  0 : insert bytes patch
  1 : depl patch (recopy in original file)

 for instert byte,
  bit 1 = 0: bit 2-7 give number size
  bit 1 = 1: bit 2-7 give number of byte needed to size
   bytes after gives size
   then after data to be inserted

 for depl patch
  bit 1-3 : nb byte for size of depl
  bit 4-7 : nb bute for pos
    then bytes for size
    then byte for seq
*/

dfuLong32 BuidInsertByteHead(dfbytep pBin,dfuLong32 sizeThis)
{
    if (sizeThis <= 0x3f)
    {
        *pBin = (dfbyte)(((dfbyte)(sizeThis-1)) << 2);
        return 1;
    }
    else
    {
        dfbyte dfNbByteSize = GetNbByteForNumber32(sizeThis);
        dfuLong32 dfRet = 1 + dfNbByteSize;
        *pBin = (dfbyte)(0x2 | ((dfNbByteSize-1) << 2));
        FlushNumber(pBin+1,sizeThis,dfNbByteSize);
        return dfRet;
    }
}

dfuLong32 BuidDeplByteHead(dfbytep pBin,dfuLong32 sizeThis,dfuLong64 posIntPtr)
{
    dfbyte dfNbByteSize = GetNbByteForNumber32(sizeThis);
    dfbyte dfNbBytePos = GetNbByteForNumber64(posIntPtr);
    *pBin = (dfbyte)(1 | ((dfNbByteSize-1) << 1) | ((dfNbBytePos-1) << 4));
    FlushNumber(pBin+1,sizeThis,dfNbByteSize);
    FlushNumber(pBin+1+dfNbByteSize,posIntPtr,dfNbBytePos);

    return 1 + dfNbByteSize + dfNbBytePos;
}

int WriteInsertBytesInRamDif(HRAMDIF hRamDif, dfvoidp data, dfuLong64 size)
{
    RAMDIF_INTERNAL *pRamDifInternal;
    int err=DSERR_OK;
    RAMDIFARRAY* pRamDifArray;

    pRamDifInternal = (RAMDIF_INTERNAL *)hRamDif;

    if (pRamDifInternal->dfWritePossible == 0)
        return DSERR_INTERNAL;

    while (size>0)
    {
        dfuLong32 sizePossible,sizeThis;
        pRamDifArray = ((pRamDifInternal->pRamDifArray) +
                           (pRamDifInternal->dfCurRamDifArrayWriting));

        sizePossible = pRamDifInternal->dfNbBytePatchResultByDiffArray -
                          pRamDifArray->dfDifResultSize;

        if (sizePossible > size)
            sizeThis = (dfuLong32)size;
        else
            sizeThis = sizePossible;

        /* do the work */
        if (sizeThis>0)
        {
            dfbyte TabHeadInsertBytesInRamDif[0x20];
            dfuLong32 dfSizeHead = BuidInsertByteHead(TabHeadInsertBytesInRamDif,sizeThis);

            err=PrepareRamDifDataBuffer(pRamDifInternal,dfSizeHead+sizeThis);
            if (err != DSERR_OK)
                break;

            DfsMemcpy( ((dfbytep)pRamDifArray->pData) + pRamDifArray->dfDifEncodedDataUsed,
                        TabHeadInsertBytesInRamDif, dfSizeHead);
            pRamDifArray->dfDifEncodedDataUsed += dfSizeHead;

            DfsMemcpy( ((dfbytep)pRamDifArray->pData) + pRamDifArray->dfDifEncodedDataUsed,
                        data, sizeThis);
            pRamDifArray->dfDifEncodedDataUsed += sizeThis;

            pRamDifArray->dfDifResultSize += sizeThis;
            pRamDifInternal->dfSizePatchResult += sizeThis;

            pRamDifInternal->dfsPatchAnalyseInfo.total_size_insert += sizeThis;

            pRamDifArray->dfNbSequence ++;
        }

        size -= sizeThis;
        data = (dfvoidp)(((dfbytep)data)+sizeThis);


        if ((pRamDifInternal->dfNbBytePatchResultByDiffArray == pRamDifArray->dfDifResultSize) && (size>0))
        {
            err = BuildNextRamDifArray(pRamDifInternal);
            if (err != DSERR_OK)
                break;
            pRamDifInternal->dfCurRamDifArrayWriting++;
        }
    }
    return err;
}

int WriteInsertDeplInRamDif(HRAMDIF hRamDif,
                            dfuLong64 posIntPtr, dfuLong64 size)
{
    RAMDIF_INTERNAL *pRamDifInternal;
    int err=DSERR_OK;
    RAMDIFARRAY* pRamDifArray;

    pRamDifInternal = (RAMDIF_INTERNAL *)hRamDif;

    if (pRamDifInternal->dfWritePossible == 0)
        return DSERR_INTERNAL;

    while (size>0)
    {
        dfuLong32 sizePossible,sizeThis;
        pRamDifArray = ((pRamDifInternal->pRamDifArray) +
                           (pRamDifInternal->dfCurRamDifArrayWriting));

        sizePossible = pRamDifInternal->dfNbBytePatchResultByDiffArray -
                          pRamDifArray->dfDifResultSize;

        if (sizePossible > size)
            sizeThis = (dfuLong32)size;
        else
            sizeThis = sizePossible;

        /* do the work */
        if (sizeThis>0)
        {
            dfbyte TabHeadInsertBytesInRamDif[0x20];
            dfuLong32 dfSizeHead ;

            dfSizeHead = BuidDeplByteHead(TabHeadInsertBytesInRamDif,sizeThis,posIntPtr);
            err=PrepareRamDifDataBuffer(pRamDifInternal,dfSizeHead);
            if (err != DSERR_OK)
                break;

            DfsMemcpy( ((dfbytep)pRamDifArray->pData) + pRamDifArray->dfDifEncodedDataUsed,
                      TabHeadInsertBytesInRamDif, dfSizeHead);
            pRamDifArray->dfDifEncodedDataUsed += dfSizeHead;

            pRamDifArray->dfDifResultSize += sizeThis;

            if (pRamDifInternal->dfSizePatchResult == posIntPtr)
            {
                pRamDifInternal->dfsPatchAnalyseInfo.total_size_depl_in_place += sizeThis;
            }
            else
            {
                pRamDifInternal->dfsPatchAnalyseInfo.total_size_depl_out_place += sizeThis;
            }

            pRamDifInternal->dfSizePatchResult += sizeThis;

            pRamDifArray->dfNbSequence ++;
        }

        size -= sizeThis;
        posIntPtr += sizeThis;


        if ((pRamDifInternal->dfNbBytePatchResultByDiffArray == pRamDifArray->dfDifResultSize) && (size>0))
        {
            err = BuildNextRamDifArray(pRamDifInternal);
            if (err != DSERR_OK)
                break;
            pRamDifInternal->dfCurRamDifArrayWriting++;
        }
    }
    return err;
}

static int GotoCurrentPositionSelectBeginRamDif(RAMDIF_INTERNAL *pRamDifInternal,dfuLong32 dfSelectRamDif);
static int GotoPosition(RAMDIF_INTERNAL *pRamDifInternal,dfuLong64 dfPositionToRead);

int CloseWritingInRamDif(HRAMDIF hRamDif,dfuLong64* pdfSizeOfPatchResultRecorded)
{
    int err = DSERR_OK;
    RAMDIF_INTERNAL *pRamDifInternal = (RAMDIF_INTERNAL *)hRamDif;
    int resultPosition;

    if ((pRamDifInternal->dfSizePatchResult!=0) && (pRamDifInternal->dfNbRamDifArray>0))
      err = IndexOnQuickJumpCurrentEntry(pRamDifInternal,pRamDifInternal->dfNbRamDifArray-1) ;

    if (err != DSERR_OK)
        return err;

    pRamDifInternal->dfWritePossible = 0;


    /* internal pointer */

    if (pdfSizeOfPatchResultRecorded != NULL)
        *pdfSizeOfPatchResultRecorded = pRamDifInternal->dfSizePatchResult;

    resultPosition = GotoPosition(pRamDifInternal, 0);
    return (resultPosition == DSERR_END) ? DSERR_OK : resultPosition;
}

/*********************************************************************/

int GotoPositionSelectBeginRamDif(RAMDIF_INTERNAL *pRamDifInternal,dfuLong32 dfSelectRamDif,
                                  CURRENT_READ_PTR_RAMDIF* pcurrent_read_ptr_ramdif)
{
    if (dfSelectRamDif >= pRamDifInternal->dfNbRamDifArray)
        return DSERR_INTERNAL;

    pcurrent_read_ptr_ramdif->dfCurrentDifArray = dfSelectRamDif;
    pcurrent_read_ptr_ramdif->dfCurrentResultPositionOfDifArray =
        (pRamDifInternal->dfNbBytePatchResultByDiffArray) * ((dfuLong64)dfSelectRamDif);
    pcurrent_read_ptr_ramdif->dfCurrentResultPositionInDifArray = 0;
    pcurrent_read_ptr_ramdif->dfCurrentDifArrayPositionEncoded = 0;

    pcurrent_read_ptr_ramdif->dfCurrentResultPositionAdded =
        pcurrent_read_ptr_ramdif->dfCurrentResultPositionOfDifArray;

    pcurrent_read_ptr_ramdif->dfSize_in_depl = 0;
    pcurrent_read_ptr_ramdif->dfPosition_in_depl = 0;
    pcurrent_read_ptr_ramdif->dfSize_in_ins = 0;
    return DSERR_OK;
}

static int GotoCurrentPositionSelectBeginRamDif(RAMDIF_INTERNAL *pRamDifInternal,dfuLong32 dfSelectRamDif)
{
    return GotoPositionSelectBeginRamDif(pRamDifInternal,dfSelectRamDif,&pRamDifInternal->current_read_ptr_ramdif);
}

int DecodeHeadPosition(RAMDIF_INTERNAL *pRamDifInternal,CURRENT_READ_PTR_RAMDIF* pcurrent_read_ptr_ramdif)
{
    if ((pcurrent_read_ptr_ramdif->dfSize_in_depl !=0) ||
        (pcurrent_read_ptr_ramdif->dfSize_in_ins != 0))
        return DSERR_OK;

    if (pcurrent_read_ptr_ramdif->dfCurrentResultPositionInDifArray ==
           (pRamDifInternal->dfNbBytePatchResultByDiffArray))
    {
        int err ;
        if (pcurrent_read_ptr_ramdif->dfCurrentDifArray + 1 == pRamDifInternal->dfNbRamDifArray)
            err = DSERR_END;
        else
            err = GotoPositionSelectBeginRamDif(pRamDifInternal,
                pcurrent_read_ptr_ramdif->dfCurrentDifArray+1,pcurrent_read_ptr_ramdif);

        if (err != DSERR_OK)
            return err;
    }

    {
        const RAMDIFARRAY* pRamDifArray = ((pRamDifInternal->pRamDifArray) +
                                 (pcurrent_read_ptr_ramdif->dfCurrentDifArray));
        dfbytepc pCurEncodedData ;
        dfbyte c;
        dfuLong32 dfSizeOfHead;

#ifdef _DEBUG
        if (pcurrent_read_ptr_ramdif->dfCurrentDifArrayPositionEncoded >= pRamDifArray->dfDifEncodedDataUsed)
        {
            return DSERR_INTERNAL;
        }
#endif

        pCurEncodedData = ((dfbytepc)pRamDifArray->pData) +
                      pcurrent_read_ptr_ramdif->dfCurrentDifArrayPositionEncoded;
        c=*pCurEncodedData;


#ifdef _DEBUG
        if (c==0x0d)
            c +=0;
#endif

        if ((c & 1) != 0)
        {
            dfbyte dfNbByteSize = ((c >> 1) & 7) + 1;
            dfbyte dfNbBytePos = ((c >> 4) & 7) + 1;
            pcurrent_read_ptr_ramdif->dfSize_in_depl = ReadNumber32FromByteArray(pCurEncodedData+1,dfNbByteSize);
            pcurrent_read_ptr_ramdif->dfPosition_in_depl = ReadNumber64FromByteArray(pCurEncodedData+1+dfNbByteSize,dfNbBytePos);
            dfSizeOfHead = 1 + dfNbByteSize + dfNbBytePos;
        }
        else
        {
            if ((c & 2) != 0)
            {
                dfbyte dfNbByteSize = ((c >> 2) & 7) + 1;
                pcurrent_read_ptr_ramdif->dfSize_in_ins = ReadNumber32FromByteArray(pCurEncodedData+1,dfNbByteSize);
                dfSizeOfHead = 1 + dfNbByteSize ;
            }
            else
            {
                pcurrent_read_ptr_ramdif->dfSize_in_ins = (c >> 2)+1;
                dfSizeOfHead = 1;
            }
        }
        pcurrent_read_ptr_ramdif->dfCurrentDifArrayPositionEncoded += dfSizeOfHead;
        return DSERR_OK;
    }
}

int DecodeHeadCurrentPosition(RAMDIF_INTERNAL *pRamDifInternal)
{
    return DecodeHeadPosition(pRamDifInternal,&pRamDifInternal->current_read_ptr_ramdif);
}

static int GotoPosition(RAMDIF_INTERNAL *pRamDifInternal,dfuLong64 dfPositionToRead)
{
    int err = DSERR_OK;
    dfuLong32 dfExpectedResultPositionInDifArray;
    if (dfPositionToRead > pRamDifInternal->dfSizePatchResult)
        return DSERR_INTERNAL;
    else
    if (dfPositionToRead == pRamDifInternal->dfSizePatchResult)
        return DSERR_END;

    err = GotoCurrentPositionSelectBeginRamDif(pRamDifInternal,(dfuLong32)
                 (dfPositionToRead / (pRamDifInternal->dfNbBytePatchResultByDiffArray)));
    if (err != DSERR_OK)
        return err;

    dfExpectedResultPositionInDifArray = (dfuLong32)
        (dfPositionToRead -
             pRamDifInternal->current_read_ptr_ramdif.dfCurrentResultPositionOfDifArray);

    // todo : quickjump to modify pRamDifInternal->current_read_ptr_ramdif.dfCurrentResultPositionInDifArray
    //                            pRamDifInternal->current_read_ptr_ramdif.dfCurrentResultPositionAdded
    //                            pRamDifInternal->current_read_ptr_ramdif.dfCurrentDifArrayPositionEncoded

    {
        const RAMDIFARRAY* pCurRamDifArray =
                               ((pRamDifInternal->pRamDifArray) +
                                       (pRamDifInternal->current_read_ptr_ramdif.dfCurrentDifArray));

        dfuLong32 dfLoopStep = pCurRamDifArray -> dfQuickJumpFirstLoopStep;
        dfuLong32 dfQuickJumpPos = 0;
        while (dfLoopStep > 0)
        {
            if ((*((pCurRamDifArray -> pQuickJumpResultPosArray)  + (dfLoopStep + dfQuickJumpPos - 1))) <= dfExpectedResultPositionInDifArray)
                dfQuickJumpPos += dfLoopStep;
            dfLoopStep = dfLoopStep >> 1;
        }

        if (dfQuickJumpPos>0)
        {
            pRamDifInternal->current_read_ptr_ramdif.dfCurrentDifArrayPositionEncoded =
                (*((pCurRamDifArray -> pQuickJumpEncodedPosArray)  + (dfQuickJumpPos - 1)));
            pRamDifInternal->current_read_ptr_ramdif.dfCurrentResultPositionInDifArray =
                (*((pCurRamDifArray -> pQuickJumpResultPosArray)  + (dfQuickJumpPos - 1)));
            pRamDifInternal->current_read_ptr_ramdif.dfCurrentResultPositionAdded +=
                (*((pCurRamDifArray -> pQuickJumpResultPosArray)  + (dfQuickJumpPos - 1)));
        }
    }

    while (pRamDifInternal->current_read_ptr_ramdif.dfCurrentResultPositionInDifArray <
                             dfExpectedResultPositionInDifArray)
    {
        dfuLong32 dfResultSizeThis;
        err = DecodeHeadCurrentPosition(pRamDifInternal);
        if (err != DSERR_OK)
            break;
        dfResultSizeThis = dfmin( dfExpectedResultPositionInDifArray -
                                       pRamDifInternal->current_read_ptr_ramdif.dfCurrentResultPositionInDifArray,
                                  dfmax(pRamDifInternal->current_read_ptr_ramdif.dfSize_in_depl,
                                        pRamDifInternal->current_read_ptr_ramdif.dfSize_in_ins));

        if (pRamDifInternal->current_read_ptr_ramdif.dfSize_in_ins > 0)
        {
            pRamDifInternal->current_read_ptr_ramdif.dfSize_in_ins -= dfResultSizeThis;
            pRamDifInternal->current_read_ptr_ramdif.dfCurrentDifArrayPositionEncoded += dfResultSizeThis;
        }
        else
        {
            pRamDifInternal->current_read_ptr_ramdif.dfSize_in_depl -= dfResultSizeThis;
            pRamDifInternal->current_read_ptr_ramdif.dfPosition_in_depl += dfResultSizeThis;
        }
        pRamDifInternal->current_read_ptr_ramdif.dfCurrentResultPositionInDifArray += dfResultSizeThis;
        pRamDifInternal->current_read_ptr_ramdif.dfCurrentResultPositionAdded += dfResultSizeThis;
    }

    return err;
}


int GetRamDifSeq(HRAMDIF hRamDif, dfuLong64 dfPositionToRead,
                 dfuLong64 dfSizeToGetDepl, dfuLong64 dfSizeToGetIns,
                 dfuLong32* is_insert_byte, dfuLong64* pSize,
                 dfuLong64* posInOrigForDeplSeq,
                 dfvoidp* pDataForInsSeq)
{
    RAMDIF_INTERNAL *pRamDifInternal;
    int err=DSERR_OK;
    //RAMDIFARRAY* pRamDifArray;

    pRamDifInternal = (RAMDIF_INTERNAL *)hRamDif;

    /*
    BAD COMPARE BUG BUG BUG
    dfCurrentResultPositionAdded is not the value
    */
    //if (pRamDifInternal->current_read_ptr_ramdif.dfCurrentResultPositionInDifArray == pRamDifInternal->dfSizePatchResult)
    //    return DSERR_END;


        /*
    if (pRamDifInternal->current_read_ptr_ramdif.dfCurrentResultPositionAdded == pRamDifInternal->dfSizePatchResult)
        return DSERR_END;
*/
    if (pRamDifInternal->dfWritePossible != 0)
        {

            *is_insert_byte = 0;
            *pDataForInsSeq = NULL;
            *posInOrigForDeplSeq = pRamDifInternal->current_read_ptr_ramdif.dfPosition_in_depl;
            *pSize = 0;
            return DSERR_INTERNAL;
        }


    if (dfPositionToRead != pRamDifInternal->current_read_ptr_ramdif.dfCurrentResultPositionAdded)
    {
        // do the jump

        err = GotoPosition(pRamDifInternal,dfPositionToRead);
        if (err != DSERR_OK)
        {

            *is_insert_byte = 0;
            *pDataForInsSeq = NULL;
            *posInOrigForDeplSeq = pRamDifInternal->current_read_ptr_ramdif.dfPosition_in_depl;
            *pSize = 0;
            return err;
        }
    }

    if ((pRamDifInternal->current_read_ptr_ramdif.dfSize_in_depl == 0) &&
           (pRamDifInternal->current_read_ptr_ramdif.dfSize_in_ins == 0) &&
           (err == DSERR_OK))
    {
        err = DecodeHeadCurrentPosition(pRamDifInternal);
        if ((pRamDifInternal->current_read_ptr_ramdif.dfSize_in_depl == 0) &&
                   (pRamDifInternal->current_read_ptr_ramdif.dfSize_in_ins == 0) &&
                   (err == DSERR_OK))
                     err=DSERR_INTERNAL;
    }

    if ((pRamDifInternal->current_read_ptr_ramdif.dfSize_in_depl != 0) && (err == DSERR_OK))
    {
        dfuLong32 dfSizeThis ;
        dfuLong64 dfToDoInResult;
        dfuLong64 dfSizeToGetLeft = dfSizeToGetDepl;
        *is_insert_byte = 0;
        *pDataForInsSeq = NULL;


        /* now we test if we cannot risk to reach end of the stream */
        dfToDoInResult = (pRamDifInternal->dfSizePatchResult - pRamDifInternal->current_read_ptr_ramdif.dfCurrentResultPositionAdded);
        if (dfToDoInResult < dfSizeToGetLeft)
            dfSizeToGetLeft = dfToDoInResult;

        dfSizeThis = (dfuLong32)dfmin(dfSizeToGetLeft,pRamDifInternal->current_read_ptr_ramdif.dfSize_in_depl);

        *posInOrigForDeplSeq = pRamDifInternal->current_read_ptr_ramdif.dfPosition_in_depl;
        *pSize = dfSizeThis;
        dfSizeToGetLeft -= dfSizeThis;

        pRamDifInternal->current_read_ptr_ramdif.dfSize_in_depl -= dfSizeThis;
        pRamDifInternal->current_read_ptr_ramdif.dfPosition_in_depl += dfSizeThis;

        pRamDifInternal->current_read_ptr_ramdif.dfCurrentResultPositionInDifArray += dfSizeThis;
        pRamDifInternal->current_read_ptr_ramdif.dfCurrentResultPositionAdded += dfSizeThis;




        while (dfSizeToGetLeft>0)
        {
            err = DecodeHeadCurrentPosition(pRamDifInternal);
            if (err != DSERR_OK)
                break;
            // try decode, and if position is contiguous, add
            if ((pRamDifInternal->current_read_ptr_ramdif.dfPosition_in_depl !=
                                         ((*posInOrigForDeplSeq)+(*pSize))) ||
                (pRamDifInternal->current_read_ptr_ramdif.dfSize_in_depl == 0))
                   break;

            dfSizeThis = (dfuLong32)dfmin(dfSizeToGetLeft,pRamDifInternal->current_read_ptr_ramdif.dfSize_in_depl);

            *pSize += dfSizeThis;
            dfSizeToGetLeft -= dfSizeThis;

            pRamDifInternal->current_read_ptr_ramdif.dfSize_in_depl -= dfSizeThis;
            pRamDifInternal->current_read_ptr_ramdif.dfPosition_in_depl += dfSizeThis;

            pRamDifInternal->current_read_ptr_ramdif.dfCurrentResultPositionInDifArray += dfSizeThis;
            pRamDifInternal->current_read_ptr_ramdif.dfCurrentResultPositionAdded += dfSizeThis;
        }
    }
    else
    if ((pRamDifInternal->current_read_ptr_ramdif.dfSize_in_ins != 0) && (err == DSERR_OK))
    {
        dfuLong32 dfSizeThis ;
        dfuLong64 dfToDoInResult ;
        const RAMDIFARRAY* pCurRamDifArray =
                               ((pRamDifInternal->pRamDifArray) +
                                       (pRamDifInternal->current_read_ptr_ramdif.dfCurrentDifArray));
        *is_insert_byte = 1;
        *posInOrigForDeplSeq = 0;


        /* now we test if we cannot risk to reach end of the stream */
        dfToDoInResult = (pRamDifInternal->dfSizePatchResult - pRamDifInternal->current_read_ptr_ramdif.dfCurrentResultPositionAdded);
        if (dfToDoInResult < dfSizeToGetIns)
            dfSizeToGetIns = dfToDoInResult;

        dfSizeThis = (dfuLong32)dfmin(dfSizeToGetIns,pRamDifInternal->current_read_ptr_ramdif.dfSize_in_ins);
        *pSize = dfSizeThis;
        *pDataForInsSeq = (dfvoidp)(((dfbytep)pCurRamDifArray->pData) +
             pRamDifInternal->current_read_ptr_ramdif.dfCurrentDifArrayPositionEncoded);

        pRamDifInternal->current_read_ptr_ramdif.dfSize_in_ins -= dfSizeThis;
        pRamDifInternal->current_read_ptr_ramdif.dfCurrentDifArrayPositionEncoded += dfSizeThis;

        pRamDifInternal->current_read_ptr_ramdif.dfCurrentResultPositionInDifArray += dfSizeThis;
        pRamDifInternal->current_read_ptr_ramdif.dfCurrentResultPositionAdded += dfSizeThis;
    }
    else
        if (err == DSERR_OK)
            err = DSERR_INTERNAL;

    if (err == DSERR_OK)
        if (pRamDifInternal->current_read_ptr_ramdif.dfCurrentResultPositionAdded == pRamDifInternal->dfSizePatchResult)
            err = DSERR_END;

    return err;
}

int CheckPositionEnd(HRAMDIF hRamDif, dfuLong64 dfPos)
{
    RAMDIF_INTERNAL *pRamDifInternal;

    pRamDifInternal = (RAMDIF_INTERNAL *)hRamDif;

    if (pRamDifInternal->dfSizePatchResult >= dfPos)
        return DSERR_END;
    return DSERR_OK;
}

dfuLong64 GetLatestPositionWritten(HRAMDIF hRamDif)
{
    RAMDIF_INTERNAL *pRamDifInternal;
    pRamDifInternal = (RAMDIF_INTERNAL *)hRamDif;
    return pRamDifInternal->dfSizePatchResult;
}

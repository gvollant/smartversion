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


//#include <windows. h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zlib.h"
#include "../../patchstream/common/difbasic.h"
#include "../../patchstream/common/DfsTlTyp.h"
#include "../../patchstream/common/difstool.h"
#include "DfsTagDf.h"
#include "DfsTagMg.h"
#include "DfsTagBlockFloatEnd.h"
#include "DfsTagBlockFloatEndStruct.h"

#ifndef DEF_MEM_LEVEL
#define DEF_MEM_LEVEL (8)
#endif

/*

DFTAGBLOCKFLOAT ReadDfTagBlockFloat(dfvoidp ptr);

DFTAGBLOCKFLOAT ReadDfTagBlockFloatSizeLimited(dfvoidp ptr, dfuLong32 dfLimit);

BOOL FlushTagBlockFloat(DFTAGBLOCKFLOAT TagBlockFloat, dfvoidp * ptr, dfuLong32 * size, BOOL compressed);

BOOL FreeFlushTagBlockFloatPtr(dfvoidp ptr);

*/




DFTAGBLOCKFLOAT SVFAPI BuildEmptyBlockFloat()
{
    DFTAGBLOCKFLOATINTERNAL * pdfTagBlockFloatInternal = (DFTAGBLOCKFLOATINTERNAL *)DfsMalloc(sizeof(DFTAGBLOCKFLOATINTERNAL));
    if (pdfTagBlockFloatInternal!=NULL)
    {
        pdfTagBlockFloatInternal -> dfNbTagBlockUsed = 0;
        pdfTagBlockFloatInternal -> dfNbTagBlockAllocated = 0;
        pdfTagBlockFloatInternal -> pdfTabBlock = NULL;
        pdfTagBlockFloatInternal -> dfNbTagAllocStep = 0x10;
    }
    return (DFTAGBLOCKFLOAT)pdfTagBlockFloatInternal;
}



DFTAGBLOCK* GetTagBlock(DFTAGBLOCKFLOAT TagBlockFloat, dfuLong32 dfDirNum, dfuLong32 dfFileNum,BOOL fCreateIfNeeded)
{
    dfuLong32 i;
    DFTAGBLOCKFLOATINTERNAL * pdfTagBlockFloatInternal = (DFTAGBLOCKFLOATINTERNAL *)TagBlockFloat;
    for (i=0;i<pdfTagBlockFloatInternal->dfNbTagBlockUsed;i++)
    {
        if (((((pdfTagBlockFloatInternal->pdfTabBlock)+i)->dfDirNum) == dfDirNum) &&
            ((((pdfTagBlockFloatInternal->pdfTabBlock)+i)->dfFileNum) == dfFileNum))
            return ((pdfTagBlockFloatInternal->pdfTabBlock)+i);
    }

    if (fCreateIfNeeded)
    {
        if (pdfTagBlockFloatInternal->dfNbTagBlockAllocated < (pdfTagBlockFloatInternal->dfNbTagBlockUsed+1))
        {
            DFTAGBLOCK* bufNew;
            dfuLong32 dfNbTagAllocStep = pdfTagBlockFloatInternal->dfNbTagAllocStep;
            dfuLong32 dfNeedAllocItem;
            if (dfNbTagAllocStep == 0)
                dfNbTagAllocStep = 1;
            dfNeedAllocItem = pdfTagBlockFloatInternal->dfNbTagBlockAllocated+dfNbTagAllocStep;
            bufNew = (DFTAGBLOCK*)DfsRealloc(pdfTagBlockFloatInternal->pdfTabBlock, (dfNeedAllocItem+2)*sizeof(DFTAGBLOCK));
            if (bufNew == NULL)
                return NULL;
            pdfTagBlockFloatInternal->dfNbTagBlockAllocated = dfNeedAllocItem;
            pdfTagBlockFloatInternal->pdfTabBlock = bufNew;
        }

        for (i=0;i<pdfTagBlockFloatInternal->dfNbTagBlockUsed;i++)
        {
            if (((((pdfTagBlockFloatInternal->pdfTabBlock)+i)->dfDirNum) == dfDirNum) &&
                ((((pdfTagBlockFloatInternal->pdfTabBlock)+i)->dfFileNum) > dfFileNum))
                break;

            if (((((pdfTagBlockFloatInternal->pdfTabBlock)+i)->dfDirNum) > dfDirNum))
                break;
        }
        /*
        for (j=pdfTagBlockFloatInternal->dfNbTagBlockUsed;j>=i;j--)
            *((pdfTagBlockFloatInternal->pdfTabBlock)+j+1) = *((pdfTagBlockFloatInternal->pdfTabBlock)+j);
            */
        {
            dfuLong32 j;
            for (j=pdfTagBlockFloatInternal->dfNbTagBlockUsed+(1*0);(j)>i;j--) // NEW DEBUG CODE COMPUWARE
                *((pdfTagBlockFloatInternal->pdfTabBlock)+j) = *((pdfTagBlockFloatInternal->pdfTabBlock)+j-1);
        }
        ((pdfTagBlockFloatInternal->pdfTabBlock)+i)->dfDirNum = dfDirNum;
        ((pdfTagBlockFloatInternal->pdfTabBlock)+i)->dfFileNum = dfFileNum;
        ((pdfTagBlockFloatInternal->pdfTabBlock)+i)->dfTagList = NULL;
        pdfTagBlockFloatInternal->dfNbTagBlockUsed++;
        return ((pdfTagBlockFloatInternal->pdfTabBlock)+i);
    }
    return NULL;
}


#define READBLOCKTAGSIZE_NOSIZELIMIT (0xffffffff)

DFTAGBLOCKFLOAT SVFAPI ReadDfTagBlockFloatSizeLimited(dfvoidp ptr, dfuLong32 * pTagListPackedSize, dfuLong32 dfLimit)
{
    DFTAGBLOCKSTOREDGENERALHEADER* pdfTagBlockStoreGeneralHeader;
    dfuLong32 dfSizeFloatStreamCompressed;
    dfuLong32 dfStoreMethod;
    dfvoidp ptrBrowseTagBlock=NULL;
    dfvoidp ptrBrowseTagBlockAllocated=NULL;
    dfuLong32 dfBrowseTagBlockToDo;
    DFTAGBLOCKFLOAT TagBlockFloat=NULL;

    if (pTagListPackedSize != NULL)
        *pTagListPackedSize=0;

    pdfTagBlockStoreGeneralHeader = (DFTAGBLOCKSTOREDGENERALHEADER*)ptr;
    if ((dfLimit != READBLOCKTAGSIZE_NOSIZELIMIT) &&
        (dfLimit < sizeof(pdfTagBlockStoreGeneralHeader)))
        return NULL;

    dfSizeFloatStreamCompressed = ConvertuLongIntelToLong(pdfTagBlockStoreGeneralHeader->dfSizeFloatStreamCompressed);
    if ((dfLimit != READBLOCKTAGSIZE_NOSIZELIMIT) &&
        ((dfLimit+dfSizeFloatStreamCompressed) < sizeof(pdfTagBlockStoreGeneralHeader)))
        return NULL;

    dfStoreMethod = ConvertuLongIntelToLong(pdfTagBlockStoreGeneralHeader->dfStoreMethod);
    dfBrowseTagBlockToDo = ConvertuLongIntelToLong(pdfTagBlockStoreGeneralHeader->dfSizeFloatStreamUncompressed);


    if (dfStoreMethod==TAGBLOCKFLOATSTOREMETHOD_NONE)
    {
        if (dfSizeFloatStreamCompressed > dfBrowseTagBlockToDo)
            return NULL;
        ptrBrowseTagBlock = (pdfTagBlockStoreGeneralHeader+1);
    }
    else if (dfStoreMethod==TAGBLOCKFLOATSTOREMETHOD_DEFLATE)
    {
          z_stream zstr;
          BOOL fDone = TRUE;
          DfsClearStruct(&zstr, 0, sizeof(z_stream));

          ptrBrowseTagBlockAllocated = DfsMalloc(dfBrowseTagBlockToDo + 0x10);
          ptrBrowseTagBlock = ptrBrowseTagBlockAllocated;

          if (inflateInit2(&zstr, -MAX_WBITS) != Z_OK)
              fDone = FALSE;
          else
          {
              int err;
              zstr.next_out=(dfbytep)ptrBrowseTagBlock;
              zstr.avail_out=dfBrowseTagBlockToDo+0x08;
              zstr.next_in=(dfbytep)(pdfTagBlockStoreGeneralHeader+1);
              zstr.avail_in=dfSizeFloatStreamCompressed;
              err = inflate(&zstr,Z_SYNC_FLUSH);
              fDone = ((err==Z_STREAM_END) || (err==Z_OK)) && (zstr.total_out == dfBrowseTagBlockToDo);
              inflateEnd(&zstr);
          }


          if (!fDone)
          {
              DfsFree(ptrBrowseTagBlockAllocated);
              return NULL;
          }
    }

    TagBlockFloat=BuildEmptyBlockFloat();

    while (dfBrowseTagBlockToDo > sizeof(DFTAGBLOCKSTOREDLOCALHEADER))
    {
        DFTAGBLOCKSTOREDLOCALHEADER DfTagBlockStoredLocalHeader;
        DFTAGBLOCK* pTagBlock;
        dfuLong32 dfTagListPackedSize=0;
        DfsMemcpy(&DfTagBlockStoredLocalHeader,ptrBrowseTagBlock,sizeof(DFTAGBLOCKSTOREDLOCALHEADER));
        pTagBlock = GetTagBlock(TagBlockFloat,
                                ConvertuLongIntelToLong(DfTagBlockStoredLocalHeader.dfDirNum),
                                ConvertuLongIntelToLong(DfTagBlockStoredLocalHeader.dfFileNum),
                                TRUE);
        if (pTagBlock == NULL)
            return FALSE;
        if (pTagBlock->dfTagList == NULL)
        {
            pTagBlock->dfTagList = ReadTagListSizeLimited(
                      (((char*)ptrBrowseTagBlock)+sizeof(DFTAGBLOCKSTOREDLOCALHEADER)),
                      &dfTagListPackedSize,
                      dfBrowseTagBlockToDo - sizeof(DFTAGBLOCKSTOREDLOCALHEADER));
        }

        ptrBrowseTagBlock = ((char*)ptrBrowseTagBlock)+sizeof(DFTAGBLOCKSTOREDLOCALHEADER)+dfTagListPackedSize;
        if (dfTagListPackedSize + sizeof(DFTAGBLOCKSTOREDLOCALHEADER) <= dfBrowseTagBlockToDo)
        {
            dfBrowseTagBlockToDo -= dfTagListPackedSize + sizeof(DFTAGBLOCKSTOREDLOCALHEADER);
        }
        else
            dfBrowseTagBlockToDo = 0;

    }

    if (dfStoreMethod==TAGBLOCKFLOATSTOREMETHOD_DEFLATE)
    {
        DfsFree(ptrBrowseTagBlockAllocated);
    }

    if (pTagListPackedSize != NULL)
        *pTagListPackedSize=dfSizeFloatStreamCompressed+sizeof(DFTAGBLOCKSTOREDGENERALHEADER);

    return TagBlockFloat;
}

DFTAGBLOCKFLOAT SVFAPI ReadDfTagBlockFloat(dfvoidp ptr,dfuLong32 * pTagListPackedSize)
{
    return ReadDfTagBlockFloatSizeLimited(ptr, pTagListPackedSize, READBLOCKTAGSIZE_NOSIZELIMIT);
}

BOOL SVFAPI CheckPtrBufSize(dfvoidp* ptr,dfuLong32* pdfSizeAllocated, dfuLong32 dfSizeNeeded, dfuLong32 dfStepAlloc)
{
    dfuLong32 dfNewAlloc;
    dfvoidp newptr;
    if (dfSizeNeeded <= (*pdfSizeAllocated))
        return TRUE;
    dfNewAlloc = ((dfSizeNeeded / dfStepAlloc) + 1 ) * dfStepAlloc;
    newptr = DfsRealloc(*ptr,dfNewAlloc);
    if (newptr == NULL)
        return FALSE;
    *pdfSizeAllocated = dfNewAlloc;
    *ptr = newptr;
    return TRUE;
}



BOOL SVFAPI CloseDfTagBlockFloat(DFTAGBLOCKFLOAT TagBlockFloat)
{
    DFTAGBLOCKFLOATINTERNAL * pdfTagBlockFloatInternal = (DFTAGBLOCKFLOATINTERNAL *)TagBlockFloat;
    BOOL fRet = TRUE;
    dfuLong32 i;
    for (i=0;i<pdfTagBlockFloatInternal->dfNbTagBlockUsed;i++)
        if (!CloseTagList(((pdfTagBlockFloatInternal->pdfTabBlock)+i)->dfTagList))
            fRet = FALSE;
    if (pdfTagBlockFloatInternal->pdfTabBlock != NULL)
      DfsFree(pdfTagBlockFloatInternal->pdfTabBlock);
    DfsFree(pdfTagBlockFloatInternal);
    return fRet;
}



BOOL SVFAPI AddTagBlockFloat(DFTAGBLOCKFLOAT TagBlockFloat, dfuLong32 dfDirNum, dfuLong32 dfFileNum, dfuLong32 TagNumber, dfvoidpc TagBuf,
            dfuLong32 TagSize)
{
    DFTAGBLOCK* pTagBlock = GetTagBlock(TagBlockFloat, dfDirNum, dfFileNum, TRUE);
    if (pTagBlock == NULL)
        return FALSE;
    if (pTagBlock->dfTagList == NULL)
        pTagBlock->dfTagList = AllocNewTagList();
    if (pTagBlock->dfTagList == NULL)
        return FALSE;
    return AddTag(pTagBlock->dfTagList, TagNumber, TagBuf, TagSize);
}

BOOL SVFAPI RemoveTagBlockFloat(DFTAGBLOCKFLOAT TagBlockFloat, dfuLong32 dfDirNum, dfuLong32 dfFileNum, dfuLong32 TagNumber)
{
    DFTAGBLOCK* pTagBlock = GetTagBlock(TagBlockFloat, dfDirNum, dfFileNum, FALSE);
    if (pTagBlock == NULL)
        return FALSE;
    return RemoveTag(pTagBlock->dfTagList, TagNumber);
}

BOOL SVFAPI AddTaguLongBlockFloat(DFTAGBLOCKFLOAT TagBlockFloat, dfuLong32 dfDirNum, dfuLong32 dfFileNum, dfuLong32 TagNumber, dfuLong32 value)
{
    DFTAGBLOCK* pTagBlock = GetTagBlock(TagBlockFloat, dfDirNum, dfFileNum, TRUE);
    if (pTagBlock == NULL)
        return FALSE;
    if (pTagBlock->dfTagList == NULL)
        pTagBlock->dfTagList = AllocNewTagList();
    if (pTagBlock->dfTagList == NULL)
        return FALSE;
    return AddTaguLong(pTagBlock->dfTagList, TagNumber, value);
}

BOOL SVFAPI GetTagBlockFloat(DFTAGBLOCKFLOAT TagBlockFloat, dfuLong32 dfDirNum, dfuLong32 dfFileNum, dfuLong32 TagNumber, dfvoidp * pTagBuf,
            dfuLong32 * pTagSize)
{
    DFTAGBLOCK* pTagBlock = GetTagBlock(TagBlockFloat, dfDirNum, dfFileNum, FALSE);
    if (pTagBlock == NULL)
        return FALSE;
    return GetTag(pTagBlock->dfTagList, TagNumber, pTagBuf, pTagSize);
}

BOOL SVFAPI GetTaguLongBlockFloat(DFTAGBLOCKFLOAT TagBlockFloat, dfuLong32 dfDirNum, dfuLong32 dfFileNum, dfuLong32 TagNumber, dfuLong32 * value)
{
    DFTAGBLOCK* pTagBlock = GetTagBlock(TagBlockFloat, dfDirNum, dfFileNum, FALSE);
    if (pTagBlock == NULL)
        return FALSE;
    return GetTaguLong(pTagBlock->dfTagList, TagNumber, value);
}

BOOL SVFAPI DuplicateTagBlockFloatRange(DFTAGBLOCKFLOAT TagBlockFloatSrc, dfuLong32 dfDirNumSrc, dfuLong32 dfFileNumSrc,
                                 DFTAGBLOCKFLOAT TagBlockFloatDst, dfuLong32 dfDirNumDst, dfuLong32 dfFileNumDst,
                                 dfuLong32 TagNumberFirst,
                                 dfuLong32 TagNumberLast)
{
    DFTAGBLOCK* pTagBlockSrc ;
    DFTAGBLOCK* pTagBlockDst ;

    pTagBlockSrc = GetTagBlock(TagBlockFloatSrc, dfDirNumSrc, dfFileNumSrc, FALSE);
    if (pTagBlockSrc == NULL)
        return TRUE;

    pTagBlockDst = GetTagBlock(TagBlockFloatDst, dfDirNumDst, dfFileNumDst, TRUE);

    if (pTagBlockSrc == NULL)
        return FALSE;

    if (pTagBlockDst == NULL)
        return FALSE;

    if (pTagBlockDst->dfTagList == NULL)
        pTagBlockDst->dfTagList = AllocNewTagList();

    return DuplicateTagRange(pTagBlockSrc->dfTagList,pTagBlockDst->dfTagList,TagNumberFirst,TagNumberLast);
}

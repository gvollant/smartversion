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
#include "../common/DfsTagDf.h"
#include "../common/DfsTagMg.h"
#include "../common/DfsTagBlockFloatEnd.h"
#include "../common/DfsTagBlockFloatEndStruct.h"
#include "DfsTagBlockFloatEndWr.h"

#ifndef DEF_MEM_LEVEL
#define DEF_MEM_LEVEL (8)
#endif

/*

DFTAGBLOCKFLOAT ReadDfTagBlockFloat(dfvoidp ptr);

DFTAGBLOCKFLOAT ReadDfTagBlockFloatSizeLimited(dfvoidp ptr, dfuLong32 dfLimit);

BOOL FlushTagBlockFloat(DFTAGBLOCKFLOAT TagBlockFloat, dfvoidp * ptr, dfuLong32 * size, BOOL compressed);

BOOL FreeFlushTagBlockFloatPtr(dfvoidp ptr);

*/


BOOL SVFAPI FlushTagBlockFloat(DFTAGBLOCKFLOAT TagBlockFloat, dfvoidp * ptr, dfuLong32 * size, BOOL compressed)
{
    DFTAGBLOCKFLOATINTERNAL * pdfTagBlockFloatInternal = (DFTAGBLOCKFLOATINTERNAL *)TagBlockFloat;
    dfuLong32 dfSizeAllocated=0;
    dfuLong32 dfStepAlloc=0x200;
    dfuLong32 dfSizeUsed=0;
    dfvoidp ptrUncompressed=NULL;
    dfuLong32 i;
    dfuLong32 dfSizeFloatStreamUncompressed;
    dfuLong32  dfPosBeginFloatLocalStream;
    DFTAGBLOCKSTOREDGENERALHEADER dfTagBlockStoreGeneralHeader;
    dfvoidp ptrFloatStreamUncompressed;
    BOOL fSuccess=TRUE;
    dfuLong32 dfDummy;
    if (size == NULL)
        size = &dfDummy;
    *size = 0;


    if (!CheckPtrBufSize(&ptrUncompressed,&dfSizeAllocated,sizeof(DFTAGBLOCKSTOREDGENERALHEADER),dfStepAlloc))
        return FALSE;
    dfSizeUsed = sizeof(DFTAGBLOCKSTOREDGENERALHEADER);
    dfPosBeginFloatLocalStream = dfSizeUsed;


    for (i=0;i<pdfTagBlockFloatInternal->dfNbTagBlockUsed;i++)
        if (!IsTagListEmpty(((pdfTagBlockFloatInternal->pdfTabBlock)+i)->dfTagList))
    {
        DFTAGBLOCKSTOREDLOCALHEADER dfTagBlockStoredLocalHeader;
        dfvoidp ptrTagList = NULL;
        dfuLong32 sizeTagList = 0;
        BOOL fBuildTagList;
        BOOL fAllocDone;

        dfTagBlockStoredLocalHeader.dfDirNum = ConvertuLongToLongIntel(((pdfTagBlockFloatInternal->pdfTabBlock)+i)->dfDirNum);
        dfTagBlockStoredLocalHeader.dfFileNum = ConvertuLongToLongIntel(((pdfTagBlockFloatInternal->pdfTabBlock)+i)->dfFileNum);

        fBuildTagList = BuildTagListPacking(((pdfTagBlockFloatInternal->pdfTabBlock)+i)->dfTagList,
                                            &ptrTagList,&sizeTagList,FALSE);

        fAllocDone = CheckPtrBufSize(&ptrUncompressed,&dfSizeAllocated,dfSizeUsed+sizeof(DFTAGBLOCKSTOREDLOCALHEADER)+sizeTagList,dfStepAlloc);
        if ((!fAllocDone) || (!fBuildTagList))
        {
            if (fBuildTagList)
                FreeTagPtr(ptrTagList);

            fSuccess = FALSE;
            break;
        }
        DfsMemcpy(((char*)ptrUncompressed)+dfSizeUsed,&dfTagBlockStoredLocalHeader,sizeof(DFTAGBLOCKSTOREDLOCALHEADER));
        dfSizeUsed += sizeof(DFTAGBLOCKSTOREDLOCALHEADER);
        DfsMemcpy(((char*)ptrUncompressed)+dfSizeUsed,ptrTagList,sizeTagList);
        dfSizeUsed += sizeTagList;
        FreeTagPtr(ptrTagList);
    }

    if (fSuccess)
    {

        dfSizeFloatStreamUncompressed = dfSizeUsed-sizeof(DFTAGBLOCKSTOREDGENERALHEADER);
        ptrFloatStreamUncompressed = ((char*)ptrUncompressed)+sizeof(DFTAGBLOCKSTOREDGENERALHEADER);
        dfTagBlockStoreGeneralHeader.dfSizeFloatStreamUncompressed = ConvertuLongToLongIntel(dfSizeFloatStreamUncompressed);
        dfTagBlockStoreGeneralHeader.dfCrc32Tags = ConvertuLongToLongIntel(
                                crc32(0, (dfbytep)ptrFloatStreamUncompressed,
                                    dfSizeFloatStreamUncompressed));


        if (compressed)
        {
            z_stream zstr;
            dfuLong32 dfCompressBufferSize ;
            dfuLong32 dfSizeFloatStreamCompressed;
            dfvoidp ptrCompressed;

            dfCompressBufferSize = dfSizeFloatStreamUncompressed + (dfSizeFloatStreamUncompressed / 0x10) +
                                     sizeof(DFTAGBLOCKSTOREDGENERALHEADER) + 0x100;
            ptrCompressed = DfsMalloc(dfCompressBufferSize);


            DfsClearStruct(&zstr, 0, sizeof(z_stream));
            //deflateInit(&zstr,Z_DEFAULT_COMPRESSION);
            deflateInit2(&zstr, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, DEF_MEM_LEVEL, 0);
            zstr.avail_out = dfCompressBufferSize-sizeof(DFTAGBLOCKSTOREDGENERALHEADER);
            zstr.next_out = ((dfbytep) ptrCompressed) + sizeof(DFTAGBLOCKSTOREDGENERALHEADER);
            zstr.avail_in = dfSizeFloatStreamUncompressed;
            zstr.next_in = (dfbytep)ptrFloatStreamUncompressed;
            if (deflate(&zstr, Z_FINISH) != Z_STREAM_END)
            {
                fSuccess=FALSE;
            }
            dfSizeFloatStreamCompressed = zstr.total_out;
            deflateEnd(&zstr);


            if (fSuccess)
            {
                dfTagBlockStoreGeneralHeader.dfStoreMethod = ConvertuLongToLongIntel(TAGBLOCKFLOATSTOREMETHOD_DEFLATE);
                dfTagBlockStoreGeneralHeader.dfSizeFloatStreamCompressed = ConvertuLongToLongIntel(dfSizeFloatStreamCompressed);
                dfSizeFloatStreamCompressed = zstr.total_out;
                *size = dfSizeFloatStreamCompressed + sizeof(DFTAGBLOCKSTOREDGENERALHEADER);
                *ptr = ptrCompressed;
                ptrCompressed = DfsRealloc(ptrCompressed,*size);
                if (ptrCompressed != NULL)
                {
                  *ptr = ptrCompressed;
                }
            }
            else
            {
                *ptr = NULL;
                *size = 0;
                DfsFree(ptrCompressed);
            }
            DfsFree(ptrUncompressed);
        }
        else
        {
            dfTagBlockStoreGeneralHeader.dfSizeFloatStreamCompressed = ConvertuLongToLongIntel(dfSizeFloatStreamUncompressed);
            dfTagBlockStoreGeneralHeader.dfStoreMethod = ConvertuLongToLongIntel(TAGBLOCKFLOATSTOREMETHOD_NONE);
            *ptr = ptrUncompressed;
            *size = dfSizeUsed;
        }
        if ((fSuccess) && ((*ptr) != NULL))
        {
            DfsMemcpy(*ptr,&dfTagBlockStoreGeneralHeader,sizeof(DFTAGBLOCKSTOREDGENERALHEADER));
        }
    }
    else
    {
        if (ptrUncompressed != NULL)
            DfsFree(ptrUncompressed);
        *ptr = NULL;
        *size = 0;
    }



    return fSuccess;
}

BOOL SVFAPI FreeFlushTagBlockFloatPtr(dfvoidp ptr)
{
  BOOL fSuccess;

  fSuccess = (ptr != NULL);
  DfsFree(ptr);
  return fSuccess;
}

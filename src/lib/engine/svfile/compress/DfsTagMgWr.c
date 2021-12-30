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
#include "../common/DfsTagStruct.h"
#include "DfsTagMgWr.h"
#include "../../patchstream/common/ltoolsc.h"

#define CHECKSTATIC_ARRAY


/*
DFTAGLIST AllocNewTagList();
BOOL AddTag(dfuLong32 TagNumber, dfvoidp TagBuf, dfuLong32 TagSize);
BOOL EndBuildTagList(dfvoip *ptr, dfuLong32 *size, BOOL compressed);
*/

/* internal structure, can change */

typedef struct
{
  dfvoidp bufTag;
  STATIC_ARRAY_C sa;
  dfuLong32 TagAllocated;
  dfuLong32 TagBufSize;
  dfuLong32 TagStep;
}
DFSTAGLISTINTERNAL;


#ifndef DEF_MEM_LEVEL
#define DEF_MEM_LEVEL (8)
#endif

BOOL
SVFAPI BuildTagListPacking(DFTAGLIST TagList, dfvoidp * ptr, dfuLong32 * size,
                    BOOL compressed)
{
  BOOL fSuccess = TRUE;
  dfvoidp dfTagListFormatted;
  dfuLong32 dfSizeMaxTagList;
  DFSTAGLISTINTERNAL *DfsTagListInternal;

  DfsTagListInternal = (DFSTAGLISTINTERNAL *) TagList;

  if (DfsTagListInternal->TagBufSize<8)
      compressed=FALSE;

  if (size != NULL)
    *size = 0;

  if (ptr != NULL)
  {
    *ptr = NULL;

    dfSizeMaxTagList =
      sizeof(DFSTAGLISTHEADER) + DfsTagListInternal->TagBufSize;
    if (compressed)
        dfSizeMaxTagList += (0x100) + (DfsTagListInternal->TagBufSize/0x10);


    dfTagListFormatted = DfsMalloc(dfSizeMaxTagList);
    fSuccess = dfTagListFormatted != NULL;
    if (fSuccess)
    {
      DFSTAGLISTHEADER DfsTagListHeader;
      dfuLong32 dfSizeCompressed=0;
      *ptr = dfTagListFormatted;

      DfsTagListHeader.dfCrc32Tags =
        ConvertuLongToLongIntel(crc32
                                (0, (dfbytep)DfsTagListInternal->bufTag,
                                 DfsTagListInternal->TagBufSize));
      DfsTagListHeader.dfSizeUncompressed =
        ConvertuLongToLongIntel(DfsTagListInternal->TagBufSize);

      if (compressed)
      {
          z_stream zstr;
          DfsClearStruct(&zstr, 0, sizeof(z_stream));
          //deflateInit(&zstr,Z_DEFAULT_COMPRESSION);
          deflateInit2(&zstr, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, DEF_MEM_LEVEL, 0);
          zstr.avail_out = dfSizeMaxTagList-sizeof(DFSTAGLISTHEADER);
          zstr.next_out = ((dfbytep) dfTagListFormatted) + sizeof(DFSTAGLISTHEADER);
          zstr.avail_in =  DfsTagListInternal->TagBufSize;
          zstr.next_in = (dfbytep)DfsTagListInternal->bufTag;
          if (deflate(&zstr, Z_FINISH) != Z_STREAM_END)
          {
              fSuccess=FALSE;
          }
          dfSizeCompressed = zstr.total_out;
          deflateEnd(&zstr);
          DfsTagListHeader.dfStoreMethod = ConvertuLongToLongIntel(TAGSTOREMETHOD_DEFLATE);
      }

      if ((!compressed) || (dfSizeCompressed >= DfsTagListInternal->TagBufSize))
      {
          DfsTagListHeader.dfStoreMethod = ConvertuLongToLongIntel(TAGSTOREMETHOD_NONE);
          dfSizeCompressed = DfsTagListInternal->TagBufSize;

          DfsMemcpy(((char *) dfTagListFormatted) +
                    sizeof(DFSTAGLISTHEADER), DfsTagListInternal->bufTag,
                    DfsTagListInternal->TagBufSize);
      }
      DfsTagListHeader.dfSizeCompressed =
        ConvertuLongToLongIntel(dfSizeCompressed);
      if (size != NULL)
        *size = sizeof(DFSTAGLISTHEADER) + dfSizeCompressed;
      DfsMemcpy(dfTagListFormatted, &DfsTagListHeader,
                sizeof(DFSTAGLISTHEADER));
    }
  }

  return fSuccess;
}


BOOL SVFAPI FreeTagPtr(dfvoidp ptr)
{
  BOOL fSuccess;

  fSuccess = (ptr != NULL);
  DfsFree(ptr);
  return fSuccess;
}


BOOL
SVFAPI EndBuildTagList(DFTAGLIST TagList, dfvoidp * ptr, dfuLong32 * size,
                BOOL compressed)
{
    BOOL fSuccess;
    fSuccess = BuildTagListPacking(TagList,ptr,size,compressed);
    CloseTagList(TagList);
    return fSuccess;
}

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
#include "../../patchstream/common/ltoolsc.h"

#define CHECKSTATIC_ARRAY
/* external structure */
#include "DfsTagStruct.h"

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

typedef struct
{
    dfuLong32 dfTagNumber;
    dfuLong32 dfTagPos;
} DFSTAG_ITEM_SORT_INDEX;

long DFSCALLBACK fncCompareItemItemSortTagIndex(const void *lpElem1, const void *lpElem2)
{
  const DFSTAG_ITEM_SORT_INDEX *pfi1 = (const DFSTAG_ITEM_SORT_INDEX *) lpElem1;
  const DFSTAG_ITEM_SORT_INDEX *pfi2 = (const DFSTAG_ITEM_SORT_INDEX *) lpElem2;

  if (pfi1->dfTagNumber < pfi2->dfTagNumber)
      return -1;
  if (pfi1->dfTagNumber > pfi2->dfTagNumber)
      return 1;

  return 0;
}

BOOL DFSCALLBACK fncDestructorItemSortTagIndex(const void *lpElem)
{
  return TRUE;
}

BOOL DoIncrementAfterPos(STATIC_ARRAY_C sa,dfuLong32 dfFirstPosMoved,dfsLong32 dfIncrement)
{
    dfuLong32 i;
    dfuLong32 dfNbItem = GetNbElemSA(sa);
    for (i=0;i<dfNbItem;i++)
    {
        DFSTAG_ITEM_SORT_INDEX* pf=(DFSTAG_ITEM_SORT_INDEX*)GetElemPtrSA(sa, i) ;
        if (pf -> dfTagPos >= dfFirstPosMoved)
            pf -> dfTagPos += dfIncrement;
    }
    return TRUE;
}

BOOL SearchPosOfTag(STATIC_ARRAY_C sa,dfuLong32 dfTagNumber,dfuLong32 *pdfPosFound, dfuLong32* pdfPosInStaticArray)
{
    DFSTAG_ITEM_SORT_INDEX disiSearch;
    const DFSTAG_ITEM_SORT_INDEX * pFound;
    dfuLong32 dfPosFound;
    disiSearch.dfTagNumber = dfTagNumber;

    if (!FindSameElemPosSA(sa, &disiSearch, &dfPosFound))
        return FALSE;
    pFound = (const DFSTAG_ITEM_SORT_INDEX *)GetElemPtrSA(sa,dfPosFound);
    if (pdfPosFound != NULL)
        *pdfPosFound = pFound -> dfTagPos ;
    if (pdfPosInStaticArray != NULL)
        *pdfPosInStaticArray = dfPosFound;
    return TRUE;
}


DFTAGLIST SVFAPI AllocNewTagList()
{
  DFSTAGLISTINTERNAL *DfsTagListInternal;
  DfsTagListInternal =
    (DFSTAGLISTINTERNAL *) DfsMalloc(sizeof(DFSTAGLISTINTERNAL));
  if (DfsTagListInternal != NULL)
  {
    DfsTagListInternal->bufTag = NULL;
    DfsTagListInternal->TagAllocated = 0;
    DfsTagListInternal->TagBufSize = 0;
    DfsTagListInternal->TagStep = 0x100;
    DfsTagListInternal->sa = InitStaticArray_C(sizeof(DFSTAG_ITEM_SORT_INDEX),0x10);;
    if (DfsTagListInternal->sa == NULL)
    {
        DfsFree(DfsTagListInternal);
        DfsTagListInternal = NULL;
    }
    else
    {
        SetFuncCompareDataSA(DfsTagListInternal->sa, fncCompareItemItemSortTagIndex);
        SetFuncDestructorSA(DfsTagListInternal->sa, fncDestructorItemSortTagIndex);
    }
  }
  return (DFTAGLIST) DfsTagListInternal;
}

BOOL SVFAPI IsTagListEmpty(DFTAGLIST TagList)
{
  DFSTAGLISTINTERNAL *DfsTagListInternal = (DFSTAGLISTINTERNAL *) TagList;
  return (DfsTagListInternal -> TagBufSize == 0);
}

BOOL SVFAPI RemoveTag(DFTAGLIST TagList, dfuLong32 TagNumber)
{
  DFSTAGLISTINTERNAL *DfsTagListInternal = (DFSTAGLISTINTERNAL *) TagList;
  dfuLong32 dfPosSa;
  dfuLong32 dfPosInSa;
  BOOL fFoundSA = SearchPosOfTag(DfsTagListInternal->sa,TagNumber,&dfPosSa,&dfPosInSa);

  if (fFoundSA)
  {
    DFSTAGDATAHEADER DfsTagDataHeader;
    dfuLong32 TagSize;
    dfbytep pRmvTag;
    DfsMemcpy(&DfsTagDataHeader,
              ((char *) DfsTagListInternal->bufTag) + dfPosSa,
              sizeof(DFSTAGDATAHEADER));

    TagSize = ConvertuLongIntelToLong(DfsTagDataHeader.TagSize);
    pRmvTag = ((dfbytep) DfsTagListInternal->bufTag) + dfPosSa;

    DfsTagListInternal->TagBufSize -= (TagSize + sizeof(DFSTAGDATAHEADER));
    DfsMemmove(pRmvTag, pRmvTag + TagSize + sizeof(DFSTAGDATAHEADER),
            DfsTagListInternal->TagBufSize - dfPosSa);
    DeleteElemSA(DfsTagListInternal->sa,dfPosInSa,1);

    DoIncrementAfterPos(DfsTagListInternal->sa,
                    dfPosSa,(dfsLong32)(((dfsLong32)-1)*(TagSize + sizeof(DFSTAGDATAHEADER))));
  }

  return fFoundSA;
}


dfuLong32 SVFAPI GetCountOfTags(DFTAGLIST TagList)
{
	DFSTAGLISTINTERNAL *DfsTagListInternal = (DFSTAGLISTINTERNAL *)TagList;
	if (TagList == NULL)
		return 0;
	return GetNbElemSA(DfsTagListInternal->sa);
}

dfuLong32 SVFAPI GetTagNumberAtPos(DFTAGLIST TagList, dfuLong32 pos)
{
	DFSTAGLISTINTERNAL *DfsTagListInternal = (DFSTAGLISTINTERNAL *)TagList;
	const DFSTAG_ITEM_SORT_INDEX * pFound;
	if (TagList == NULL)
		return 0;
	if (pos >= GetNbElemSA(DfsTagListInternal->sa))
		return 0;
	pFound = (const DFSTAG_ITEM_SORT_INDEX *)GetElemPtrSA(DfsTagListInternal->sa, pos);

	return (pFound != NULL) ? pFound->dfTagNumber : 0;
}

BOOL
SVFAPI AddTag(DFTAGLIST TagList, dfuLong32 TagNumber, dfvoidpc TagBuf, dfuLong32 TagSize)
{
  BOOL fSuccess = TRUE;
  DFSTAGLISTINTERNAL *DfsTagListInternal;
  dfuLong32 dfSizeBufNeeded;

  RemoveTag(TagList, TagNumber);

  DfsTagListInternal = (DFSTAGLISTINTERNAL *) TagList;
  dfSizeBufNeeded =
    TagSize + DfsTagListInternal->TagBufSize + sizeof(DFSTAGDATAHEADER);

  if (dfSizeBufNeeded > DfsTagListInternal->TagAllocated)
  {
    dfvoidp bufNew;
    dfuLong32 dfNeedAlloc =
      (((dfSizeBufNeeded + DfsTagListInternal->TagStep -
         1) / DfsTagListInternal->TagStep) * DfsTagListInternal->TagStep);
    bufNew = DfsRealloc(DfsTagListInternal->bufTag, dfNeedAlloc);
    if (bufNew == NULL)
      fSuccess = FALSE;
    else
    {
      DfsTagListInternal->bufTag = bufNew;
      DfsTagListInternal->TagAllocated = dfNeedAlloc;
    }
  }

  if (fSuccess)
  {
    DFSTAGDATAHEADER DfsTagDataHeader;
    DFSTAG_ITEM_SORT_INDEX disi;

    disi.dfTagNumber = TagNumber;
    disi.dfTagPos = DfsTagListInternal->TagBufSize ;
    if (!InsertSortedSA(DfsTagListInternal->sa, &disi))
        fSuccess=FALSE;

    DfsTagDataHeader.TagNumber = ConvertuLongToLongIntel(TagNumber);
    DfsTagDataHeader.TagSize = ConvertuLongToLongIntel(TagSize);
    DfsMemcpy(((char *) DfsTagListInternal->bufTag) +
              DfsTagListInternal->TagBufSize, &DfsTagDataHeader,
              sizeof(DFSTAGDATAHEADER));
    DfsTagListInternal->TagBufSize += sizeof(DFSTAGDATAHEADER);
    DfsMemcpy(((char *) DfsTagListInternal->bufTag) +
              DfsTagListInternal->TagBufSize, TagBuf, TagSize);
    DfsTagListInternal->TagBufSize += TagSize;
  }

  return fSuccess;
}


BOOL SVFAPI CloseTagList(DFTAGLIST TagList)
{
  DFSTAGLISTINTERNAL *DfsTagListInternal = (DFSTAGLISTINTERNAL *) TagList;

  if (DfsTagListInternal->bufTag != NULL)
    DfsFree(DfsTagListInternal->bufTag);
  DeleteStaticArray_C(DfsTagListInternal->sa);
  DfsFree(DfsTagListInternal);
  return TRUE;
}
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/


#define READTAGSIZE_NOSIZELIMIT (0xffffffff)
DFTAGLIST SVFAPI ReadTagListSizeLimited(dfvoidp ptr, dfuLong32 * pTagListPackedSize,
                                 dfuLong32 dfLimit)
{
  DFSTAGLISTHEADER *pDfsTagListHeader = (DFSTAGLISTHEADER *) ptr;
  DFSTAGLISTINTERNAL *DfsTagListInternal;
  dfuLong32 dfuTagListPackedSize = 0;
  BOOL fDone=FALSE;

  if (dfLimit != READTAGSIZE_NOSIZELIMIT)
  {
    if (dfLimit < sizeof(DFSTAGLISTHEADER))
      return NULL;

    if (dfLimit <
        (sizeof(DFSTAGLISTHEADER) +
         ConvertuLongIntelToLong(pDfsTagListHeader->dfSizeCompressed)))
      return NULL;
  }

  DfsTagListInternal =
    (DFSTAGLISTINTERNAL *) DfsMalloc(sizeof(DFSTAGLISTINTERNAL));

  DfsTagListInternal->sa = InitStaticArray_C(sizeof(DFSTAG_ITEM_SORT_INDEX),0x10);;
  if (DfsTagListInternal->sa == NULL)
    {
        DfsFree(DfsTagListInternal);
        DfsTagListInternal = NULL;
    }
  else
    {
        SetFuncCompareDataSA(DfsTagListInternal->sa, fncCompareItemItemSortTagIndex);
        SetFuncDestructorSA(DfsTagListInternal->sa, fncDestructorItemSortTagIndex);
    }

  if (DfsTagListInternal != NULL)
  {
    DfsTagListInternal->bufTag = ((char *) ptr) + sizeof(DFSTAGLISTHEADER);
    DfsTagListInternal->TagStep = 0x100;
    DfsTagListInternal->TagBufSize =
      ConvertuLongIntelToLong(pDfsTagListHeader->dfSizeUncompressed);
    DfsTagListInternal->TagAllocated = DfsTagListInternal->TagBufSize + 0x10;

    DfsTagListInternal->bufTag =
      DfsMalloc(DfsTagListInternal->TagAllocated + 0x10);
    if (DfsTagListInternal->bufTag == NULL)
    {
        DfsFree(DfsTagListInternal);
        DfsTagListInternal=NULL;
    }
  }

  if (DfsTagListInternal != NULL)
  {

    dfuTagListPackedSize =
      sizeof(DFSTAGLISTHEADER) + ConvertuLongIntelToLong(pDfsTagListHeader->dfSizeCompressed);

    if (DfsTagListInternal->bufTag != NULL)
    {
      dfuLong32 dfStoreMethod=ConvertuLongIntelToLong(pDfsTagListHeader->dfStoreMethod);
      if (dfStoreMethod==TAGSTOREMETHOD_NONE)
      {
          DfsMemcpy(DfsTagListInternal->bufTag,
                    ((char *) ptr) + sizeof(DFSTAGLISTHEADER),
                    DfsTagListInternal->TagBufSize);
          fDone=TRUE;
      }
      else if (dfStoreMethod==TAGSTOREMETHOD_DEFLATE)
      {
          z_stream zstr;
          DfsClearStruct(&zstr, 0, sizeof(z_stream));
          //if (inflateInit(&zstr) != Z_OK)
          if (inflateInit2(&zstr, -MAX_WBITS) != Z_OK)
              fDone=FALSE;
          else
          {
              int err;
              zstr.next_out=(dfbytep)DfsTagListInternal->bufTag;
              zstr.avail_out=DfsTagListInternal->TagAllocated;
              zstr.next_in=((dfbytep) ptr) + sizeof(DFSTAGLISTHEADER);
              zstr.avail_in=ConvertuLongIntelToLong(pDfsTagListHeader->dfSizeCompressed);
              err = inflate(&zstr,Z_SYNC_FLUSH);
              fDone = ((err==Z_STREAM_END) || (err==Z_OK)) && (zstr.total_out == DfsTagListInternal->TagBufSize);
              inflateEnd(&zstr);
          }
      }
    }
  }
  if (pTagListPackedSize != NULL)
    *pTagListPackedSize = dfuTagListPackedSize;

  if (fDone)
  {
      dfuLong32 dfCrc32;
      dfCrc32 = (dfuLong32)crc32(0, (dfbytep)DfsTagListInternal->bufTag,
                                 DfsTagListInternal->TagBufSize);

      if (dfCrc32 != ConvertuLongIntelToLong(pDfsTagListHeader->dfCrc32Tags))
      {
          fDone=FALSE;
      }
  }

  if (fDone)
  {
      dfuLong32 dfBrowseTagBuf=0;
      while (dfBrowseTagBuf < DfsTagListInternal->TagBufSize)
      {
        DFSTAGDATAHEADER DfsTagDataHeader;
        DFSTAG_ITEM_SORT_INDEX disi;
        DfsMemcpy(&DfsTagDataHeader,
                ((char *) DfsTagListInternal->bufTag) + dfBrowseTagBuf,
                sizeof(DFSTAGDATAHEADER));

        disi.dfTagNumber = ConvertuLongIntelToLong(DfsTagDataHeader.TagNumber);
        disi.dfTagPos = dfBrowseTagBuf ;


        if (!InsertSortedSA(DfsTagListInternal->sa, &disi))
        {
            fDone=FALSE;
            break;
        }

        dfBrowseTagBuf +=
          ConvertuLongIntelToLong(DfsTagDataHeader.TagSize) +
          sizeof(DFSTAGDATAHEADER);
      }
  }

  if ((!fDone) && (DfsTagListInternal!=NULL))
  {
      DfsFree(DfsTagListInternal);
      DfsTagListInternal=NULL;
  }
  return (DFTAGLIST) DfsTagListInternal;
}

DFTAGLIST SVFAPI ReadTagList(dfvoidp ptr, dfuLong32 * pTagListPackedSize)
{
  return ReadTagListSizeLimited(ptr, pTagListPackedSize,
                                READTAGSIZE_NOSIZELIMIT);
}



BOOL
SVFAPI GetTag(DFTAGLIST TagList, dfuLong32 TagNumber, dfvoidp * pTagBuf,
       dfuLong32 * pTagSize)
{
  DFSTAGLISTINTERNAL *DfsTagListInternal = (DFSTAGLISTINTERNAL *) TagList;
  dfuLong32 dfPosSa;
  BOOL fFoundSA ;
  fFoundSA = SearchPosOfTag(DfsTagListInternal->sa,TagNumber,&dfPosSa,NULL);


  if (fFoundSA)
  {
    DFSTAGDATAHEADER DfsTagDataHeader;
    DfsMemcpy(&DfsTagDataHeader,
              ((char *) DfsTagListInternal->bufTag) + dfPosSa,
              sizeof(DFSTAGDATAHEADER));

    *pTagBuf =
        ((char *) DfsTagListInternal->bufTag) + dfPosSa +
        sizeof(DFSTAGDATAHEADER);
    *pTagSize = ConvertuLongIntelToLong(DfsTagDataHeader.TagSize);
  }
  return fFoundSA;
}

BOOL SVFAPI AddTaguLong(DFTAGLIST TagList, dfuLong32 TagNumber, dfuLong32 value)
{
  BOOL fResult;
  dfuLong32Intel valueIntel;
  valueIntel = ConvertuLongToLongIntel(value);
  fResult = AddTag(TagList, TagNumber, &valueIntel, sizeof(dfuLong32Intel));

  return fResult;
}

BOOL SVFAPI GetTaguLong(DFTAGLIST TagList, dfuLong32 TagNumber, dfuLong32 * value)
{
  BOOL fResult;
  dfvoidp ptr;
  dfuLong32 TagSize;

  fResult = GetTag(TagList, TagNumber, &ptr, &TagSize);
  if (fResult && (TagSize == sizeof(dfuLong32)))
    *value = ConvertuLongIntelToLong(*((dfuLong32Intel *) ptr));
  else
    *value = 0;

  return fResult;
}


BOOL SVFAPI DuplicateTagRange(DFTAGLIST TagListSrc,
                       DFTAGLIST TagListDst,
                       dfuLong32 TagNumberFirst,
                       dfuLong32 TagNumberLast)
{
  DFSTAGLISTINTERNAL *DfsTagListSrcInternal = (DFSTAGLISTINTERNAL *) TagListSrc;
  //DFSTAGLISTINTERNAL *DfsTagListDstInternal = (DFSTAGLISTINTERNAL *) TagListDst;
  dfuLong32 dfNbTagSrc = GetNbElemSA(DfsTagListSrcInternal->sa);
  dfuLong32 i;
  BOOL fRet = TRUE;
  dfuLong32 dfTagWrittenCount = 0;

  for (i=0;i<dfNbTagSrc;i++)
  {
      const DFSTAG_ITEM_SORT_INDEX* pItem ;

      pItem = (const DFSTAG_ITEM_SORT_INDEX*) GetElemPtrSA(DfsTagListSrcInternal->sa, i);
      if (pItem->dfTagNumber > TagNumberLast)
          return fRet;
      if (pItem->dfTagNumber >= TagNumberFirst)
      {
          dfvoidp TagBuf;
          dfuLong32 TagSize;

          DFSTAGDATAHEADER DfsTagDataHeader;
          DfsMemcpy(&DfsTagDataHeader,
                ((char *) DfsTagListSrcInternal->bufTag) + pItem->dfTagPos,
                sizeof(DFSTAGDATAHEADER));


          TagBuf =
              ((char *) DfsTagListSrcInternal->bufTag) +
                 pItem->dfTagPos + sizeof(DFSTAGDATAHEADER);
          TagSize = ConvertuLongIntelToLong(DfsTagDataHeader.TagSize);
          if (!AddTag(TagListDst,pItem->dfTagNumber,TagBuf,TagSize))
              fRet=FALSE;
          else
              dfTagWrittenCount++;
      }
  }

  return fRet && (dfTagWrittenCount>0);
}


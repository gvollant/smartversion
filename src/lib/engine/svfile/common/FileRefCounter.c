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
#include <stdio.h>
/*
#include "difbasic.h"
#include "DfsTlTyp.h"
#include "difstool.h"
#include "DfsStruc.h"
*/
//#include <windows. h>




#include "../../patchstream/common/difbasic.h"
#include "../../patchstream/common/DfsTlTyp.h"
#include "../../patchstream/common/difstool.h"

#include "../../patchstream/common/ltoolsc.h"

#include "FileRefCounter.h"

#include "../../patchstream/common/DfsOrigMemoryMap.h"
#include "../../patchstream/common/DfsIoHlp.h"

#ifdef _DEBUG
//#define MSGOUT
#endif

/*
FILEREFCOUNTER InitFileRefCounter();
void DeleteFileRefCounter();
dfuLong32 GetFileRefCounter(dfwcharpc dfFileName);
dfuLong32 AddFileRefCounter(dfwcharpc dfFileName);
dfuLong32 SubFileRefCounter(dfwcharpc dfFileName);
*/

typedef struct
{
    dfwcharpc dfFileName;
    dfuLong32 dfCount;
} ITEM_FILE_COUNTER;



long DFSCALLBACK fncCompareFileCounter(const void *lpElem1, const void *lpElem2)
{
  const ITEM_FILE_COUNTER *pfi1 = (const ITEM_FILE_COUNTER *) lpElem1;
  const ITEM_FILE_COUNTER *pfi2 = (const ITEM_FILE_COUNTER *) lpElem2;
  return dfUnicodeStrcmp(pfi1->dfFileName, pfi2->dfFileName);
}

BOOL DFSCALLBACK fncDestructorFileCounter(const void *lpElem)
{
  ITEM_FILE_COUNTER *pfi = (ITEM_FILE_COUNTER *) lpElem;
  if (pfi->dfFileName != NULL)
    DfsFree((dfwcharp) pfi->dfFileName);

  return TRUE;
}
FILEREFCOUNTER InitFileRefCounter()
{
    FILEREFCOUNTER frc = NULL;
    STATIC_ARRAY_C pSA;
    pSA = InitStaticArray_C(sizeof(ITEM_FILE_COUNTER),256);
    SetFuncCompareDataSA(pSA,fncCompareFileCounter);
    SetFuncDestructorSA(pSA,fncDestructorFileCounter);
    frc=(FILEREFCOUNTER)pSA;
    return frc;
}

void DeleteFileRefCounter(FILEREFCOUNTER frc,BOOL fDeleteReferedFile)
{
    STATIC_ARRAY_C pSA = (STATIC_ARRAY_C)frc;
    if (fDeleteReferedFile)
    {
        dfuLong32 i;
        for (i=0;i<GetNbElemSA(pSA);i++)
        {
            ITEM_FILE_COUNTER * pItem;
            pItem = (ITEM_FILE_COUNTER *)GetElemPtrSA(pSA,i);

            DeleteTempFile(pItem->dfFileName,NULL);
        }
    }

    DeleteStaticArray_C(pSA);
}

//BOOL FindSameElemPosSA(LPCVOID lpDataElem, LPDWORD lpdwPos) const;
ITEM_FILE_COUNTER * GetExistingElem(FILEREFCOUNTER frc,dfwcharpc dfFileNameSearch,dfuLong32* pPos)
{
    STATIC_ARRAY_C pSA = (STATIC_ARRAY_C)frc;
    //ITEM_FILE_COUNTER * pItemFound = NULL;
    ITEM_FILE_COUNTER itemSearch;
    dfuLong32 dfPos = 0;
    itemSearch.dfFileName = dfFileNameSearch;
    itemSearch.dfCount = 0;

    if (FindSameElemPosSA(pSA,&itemSearch,&dfPos))
    {
        if (pPos != NULL)
            *pPos=dfPos;
        return (ITEM_FILE_COUNTER*)GetElemPtrSA(pSA,dfPos);
    }
    return NULL;
}


BOOL GetFileRefCounter(FILEREFCOUNTER frc,dfwcharpc dfFileName,dfuLong32 * pRefCount)
{
    dfuLong32 dfPos;
    ITEM_FILE_COUNTER * pItem;

    if (pRefCount !=NULL)
        *pRefCount = 0;
    pItem = GetExistingElem(frc,dfFileName,&dfPos);
    if (pItem != NULL)
    {
        if (pRefCount !=NULL)
            *pRefCount = pItem->dfCount;
    }

    return (pItem != NULL);
}

BOOL AddFileRefCounter(FILEREFCOUNTER frc,dfwcharpc dfFileName,dfuLong32 *pdfNewCount)
{
    BOOL fRet;
    dfuLong32 dfPos;
    ITEM_FILE_COUNTER * pItemSearch;
    dfuLong32 dfNewCount;
    STATIC_ARRAY_C pSA = (STATIC_ARRAY_C)frc;

    #if defined(_DEBUG) && defined (MSGOUT)
    printf("ADDref %ws\n",dfFileName);
    #endif


    pItemSearch = GetExistingElem(frc,dfFileName,&dfPos);
    if (pItemSearch != NULL)
    {
        (pItemSearch->dfCount)++;
        dfNewCount = (pItemSearch->dfCount);
        fRet=TRUE;
    }
    else
    {
        ITEM_FILE_COUNTER ItemInsert;
        dfNewCount = ItemInsert.dfCount = 1;
        ItemInsert.dfFileName = dfUnicodeCopyAlloc(dfFileName);
        InsertSortedSA(pSA,&ItemInsert);
    }

    if ((pdfNewCount!=NULL) && (fRet))
        *pdfNewCount = dfNewCount;
    return fRet;
}

BOOL SubFileRefCounter(FILEREFCOUNTER frc,dfwcharpc dfFileName,dfuLong32 *pdfNewCount, BOOL fDeleteNullFile)
{
    ITEM_FILE_COUNTER * pItemSearch;
    dfuLong32 dfNewCount = 0;
    STATIC_ARRAY_C pSA = (STATIC_ARRAY_C)frc;
    dfuLong32 dfPos;
    BOOL fRet=TRUE;

    #if defined(_DEBUG) && defined (MSGOUT)
    printf("Subref %ws\n",dfFileName);
    #endif

    pItemSearch = GetExistingElem(frc,dfFileName,&dfPos);
    if (pItemSearch==NULL)
        fRet = FALSE;
    else
    {
        (pItemSearch->dfCount)--;
        dfNewCount=pItemSearch->dfCount;
        if (dfNewCount==0)
        {
            fRet = DeleteElemSA(pSA,dfPos,1);
            if (fRet && fDeleteNullFile)
                DeleteTempFile(dfFileName,NULL);
        }
    }
    if ((pdfNewCount!=NULL) && (fRet))
        *pdfNewCount = dfNewCount;
    return fRet;
}


dfuLong32 GetNbFileInFileRefCounter(FILEREFCOUNTER frc)
{
  STATIC_ARRAY_C pSA = (STATIC_ARRAY_C)frc;
    return GetNbElemSA(pSA);
}

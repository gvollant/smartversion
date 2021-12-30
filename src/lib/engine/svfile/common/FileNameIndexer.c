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

/* FileNameIndexer.cpp */
#include <stddef.h>
#include "../../patchstream/common/difbasic.h"
#include "../../patchstream/common/DfsTlTyp.h"
#include "../../patchstream/common/difstool.h"
#include "DfsStruc.h"

//#include <windows. h>
#include "../../patchstream/common/ltoolsc.h"
#include "FileRefCounter.h"

#include "../../patchstream/common/DfsOrigMemoryMap.h"
#include "../../patchstream/common/DfsIoHlp.h"


#include "FileNameIndexer.h"

typedef struct
{
    dfwcharpc dfFileName;
    dfuLong32 dfItem;
} ITEM_FILE_INDEXER;


typedef struct
{
    STATIC_ARRAY_C sac;
    BOOL fCopyString;
} FILE_INDEXER_INTERNAL;

long DFSCALLBACK fncCompareFileIndexer(const void *lpElem1, const void *lpElem2)
{
  const ITEM_FILE_INDEXER *pfi1 = (const ITEM_FILE_INDEXER *) lpElem1;
  const ITEM_FILE_INDEXER *pfi2 = (const ITEM_FILE_INDEXER *) lpElem2;
  return dfUnicodeStrcmpi(pfi1->dfFileName, pfi2->dfFileName);
}


BOOL DFSCALLBACK fncDestructorFileIndexerCopyString(const void *lpElem)
{
  ITEM_FILE_INDEXER *pfi = (ITEM_FILE_INDEXER *) lpElem;
  if (pfi->dfFileName != NULL)
    DfsFree((dfwcharp) pfi->dfFileName);

  return TRUE;
}


BOOL DFSCALLBACK fncDestructorFileIndexerNoCopyString(const void *lpElem)
{
  return TRUE;
}

FILEINDEXER InitFileRefIndexer(BOOL fCopyString,dfuLong32 dfPreReservMemory)
{
    FILE_INDEXER_INTERNAL* fii;
    fii = (FILE_INDEXER_INTERNAL*)DfsMalloc(sizeof(FILE_INDEXER_INTERNAL));
    if (fii == NULL)
        return NULL;

    fii->sac = InitStaticArray_C(sizeof(ITEM_FILE_INDEXER),0x100);
    if (fii->sac == NULL)
    {
        DfsFree(fii);
        return NULL;
    }
    SetFuncCompareDataSA(fii->sac,fncCompareFileIndexer);
    SetFuncDestructorSA(fii->sac,fCopyString ? fncDestructorFileIndexerCopyString : fncDestructorFileIndexerNoCopyString);
    ReservAllocationSA(fii->sac,dfPreReservMemory);
    fii->fCopyString = fCopyString;

    return (FILEINDEXER)fii;
}

void DeleteFileRefIndexer(FILEINDEXER fic)
{
    FILE_INDEXER_INTERNAL* fii = (FILE_INDEXER_INTERNAL*)fic;
    DeleteStaticArray_C(fii->sac);
    DfsFree(fii);
}

BOOL GetFileRefIndexer(FILEINDEXER fic,dfwcharpc dfFileName,dfuLong32 *pIndexValue)
{
    FILE_INDEXER_INTERNAL* fii = (FILE_INDEXER_INTERNAL*)fic;
    BOOL fFound;
    dfuLong32 dfPos;
    ITEM_FILE_INDEXER ifiSearch;

    ifiSearch.dfFileName = dfFileName;
    fFound = FindSameElemPosSA(fii->sac,&ifiSearch,&dfPos);
    if (pIndexValue!=NULL)
    {
        if (fFound)
        {
            ITEM_FILE_INDEXER *pfi = (ITEM_FILE_INDEXER *) GetElemPtrSA(fii->sac,dfPos);
            *pIndexValue = pfi->dfItem;
        }
        else
            *pIndexValue = FILE_INDEXER_NOT_FOUND ;
    }
    return fFound;
}

BOOL AddFileRefIndexer(FILEINDEXER fic,dfwcharpc dfFileName,dfuLong32 dfIndexValue,BOOL* pfAlreadyExist)
{
    FILE_INDEXER_INTERNAL* fii = (FILE_INDEXER_INTERNAL*)fic;
    BOOL fFound;
    dfuLong32 dfPos;
    ITEM_FILE_INDEXER ifiSearch;
    BOOL fRet=FALSE;

    ifiSearch.dfFileName = dfFileName;
    fFound = FindSameElemPosSA(fii->sac,&ifiSearch,&dfPos);
    if (pfAlreadyExist != NULL)
        *pfAlreadyExist = fFound;

    if (!fFound)
    {
        ITEM_FILE_INDEXER ifiInsert;

        if (fii->fCopyString)
            ifiInsert.dfFileName = dfUnicodeCopyAlloc(dfFileName);
        else
            ifiInsert.dfFileName = dfFileName;

        ifiInsert.dfItem = dfIndexValue;
        fRet = InsertSortedSA(fii->sac,&ifiInsert);
    }
    return fFound;
}

dfuLong32 GetNbFileInFileRefIndexer(FILEINDEXER fic)
{
    FILE_INDEXER_INTERNAL* fii = (FILE_INDEXER_INTERNAL*)fic;
    return GetNbElemSA(fii->sac);
}

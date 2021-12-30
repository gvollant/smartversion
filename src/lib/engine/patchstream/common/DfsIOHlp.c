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


/* DfsIOhlp.c */
#include <stdlib.h>
#include <stdio.h>
//#include <io.h>
#include <fcntl.h>

#include "difbasic.h"
#include "DfsTlTyp.h"
#include "difstool.h"
#include "DfsOrigMemoryMap.h"
#include "DfsIoHlp.h"
#include "ltoolsc.h"

#include "DfsIOHlpInternal.h"


 dfwchar szTempDirectoryPreset[MAX_PATH_LENGTH+1];
 BOOL  fTempDirectoryPreset=FALSE;

/*
LOWLEVELFILE OpenLowLevel(dfwcharp FileName, TYPEOPEN TypeOpen)
{
  LOWLEVELFILE llfRet = NULL;
  int iOpen;
  int oflag = 0;
  char szAnsiFileName[512];

  switch (TypeOpen)
  {
  case OPEN_READ:
    oflag = _O_RDONLY | _O_BINARY;
    break;
  case OPEN_READWRITE:
    oflag = _O_RDWR | _O_BINARY;
    break;
  case OPEN_CREATE:
    oflag = _O_RDWR | _O_CREAT | _O_TRUNC | _O_BINARY;
    break;
  }

  ConvertUnicodeToAnsi(FileName, szAnsiFileName, sizeof(szAnsiFileName) - 1);
  if (TypeOpen == OPEN_CREATE)
  {
    if (_wchmod(FileName, _S_IWRITE) != 0)
      _chmod(szAnsiFileName, _S_IWRITE);
  }

  if (oflag != 0)
    iOpen = _wopen(FileName, oflag);
  if (((iOpen == -1) || (iOpen == 0)) && (oflag != 0))
    iOpen = _open(szAnsiFileName, oflag);
  if ((iOpen != -1) && (iOpen != 0))
    llfRet = (LOWLEVELFILE) iOpen;
  return llfRet;
}

dfuLong32 LowLevelWrite(LOWLEVELFILE llfFile, void const *Buf, dfuLong32 size)
{
  dfuLong32 dfRet;
  dfRet = _write((int) llfFile, Buf, size);
  return dfRet;
}

dfuLong32 LowLevelRead(LOWLEVELFILE llfFile, void *Buf, dfuLong32 size)
{
  dfuLong32 dfRet;
  dfRet = _read((int) llfFile, Buf, size);
  return dfRet;
}


dfuLong32 LowLevelSeek(LOWLEVELFILE llfFile, dfuLong32 Pos, TYPESEEK TypeSeek)
{
  dfuLong32 dfRet;
  dfRet =
    _lseek((int) llfFile, Pos,
           (TypeSeek == TYPESEEK_END) ? SEEK_END : SEEK_SET);
  return dfRet;
}

dfuLong32 LowLevelTell(LOWLEVELFILE llfFile)
{
  dfuLong32 dfRet;
  dfRet = _tell((int) llfFile);
  return dfRet;
}

BOOL LowLevelClose(LOWLEVELFILE llfFile)
{
  BOOL fRet = _close((int) llfFile) == 0;
  return fRet;
}
*/

 BOOL SVFAPI SetTempDirectory(dfwcharpc lpDirectory)
{
    if (lpDirectory==NULL)
    {
        fTempDirectoryPreset=FALSE;
        szTempDirectoryPreset[0]=0;
        return TRUE;
    }
    else
    {
        if (dfUnicodeStrlen(lpDirectory)>=MAX_PATH_LENGTH)
            return FALSE;
        dfUnicodeCopyConcat(szTempDirectoryPreset,lpDirectory,NULL);
        fTempDirectoryPreset=TRUE;
        return TRUE;
    }
}



BOOL GetTempDirectory(dfwcharp lpDirectory,dfuLong32 dfSizeBufferinwChar,BOOL fGiveDefaultIfNoPreset)
{
    dfwcharpc szTempDirectory = szTempDirectoryPreset;
    dfwchar   szTempDirectoryWindows[MAX_PATH_LENGTH+0x20];

    if ((!fTempDirectoryPreset) && (!fGiveDefaultIfNoPreset))
    {
        *lpDirectory=0;
        return FALSE;
    }

    if (fTempDirectoryPreset)
    {
        GetTempDirectorySystem(szTempDirectoryWindows,MAX_PATH_LENGTH+0x10);
        szTempDirectory = szTempDirectoryWindows;
    }


    if (dfUnicodeStrlen(szTempDirectory)>=dfSizeBufferinwChar)
        return FALSE;

    dfUnicodeCopyConcat(lpDirectory,szTempDirectory,NULL);
    return TRUE;
}


BOOL IsFileNameForMemorySpace(const dfwchar* FileName);



BOOL SVFAPI CopyFileCutSize(dfwcharpc lpExistingFileName,dfwcharpc lpNewFileName,BOOL bFailIfExists, H_ERROR_INFO * pei,
                     BOOL fTempWrite,BOOL fCutSize, BOOL fFlushWrite, BOOL* lpfErrorOnOpeningDest, dfuLong64 dfMaxSize)
{
    return CopyFileCutSizeEx(lpExistingFileName,lpNewFileName,bFailIfExists, pei,
                     fTempWrite,fCutSize, fFlushWrite, lpfErrorOnOpeningDest, dfMaxSize,
                     NULL,NULL);
}

BOOL SVFAPI CopyFileCutSizeEx(dfwcharpc lpExistingFileName,dfwcharpc lpNewFileName,BOOL bFailIfExists, H_ERROR_INFO * pei,
                     BOOL fTempWrite,BOOL fCutSize, BOOL fFlushWrite, BOOL* lpfErrorOnOpeningDest, dfuLong64 dfMaxSize,
                     tProgressCopyFileCallBack pProgressCopyFileCallBack, dfvoidp dfUserPtr)
{
    LOWLEVELFILE llr,llw;
    dfuLong64 size,totalSize,pos;
    dfuLong32 bufsize = 0x8000*1*4;
    dfbytep buf;
    BOOL fRet;
    BOOL realCut = FALSE;
    DFSTM dfsTm;
    DFSTM* pDfsTmChangeDate=NULL;
    BOOL dateTimeChanged=FALSE;

    if (lpfErrorOnOpeningDest!=NULL)
        *lpfErrorOnOpeningDest=FALSE;

    if (bFailIfExists)
    {
        // not implemented
    }
    llr = OpenLowLevel(lpExistingFileName,OPEN_READ,FALSE,FALSE,0,pei);

    if (llr == NULL)
        return FALSE;

    LowLevelGetSize64(llr,&size);
    if (fCutSize)
        if (size>dfMaxSize)
        {
            size=dfMaxSize;
            realCut = TRUE;
        }
    if (size<bufsize)
        bufsize=((dfuLong32)size)+1;

    buf = (dfbytep)DfsMalloc(bufsize);
    if (buf==NULL)
    {
        LowLevelClose(llr,pei);
        return FALSE;
    }
    llw = OpenLowLevel(lpNewFileName,OPEN_CREATE,fTempWrite,TRUE,size,pei);
    if (llw==NULL)
    {
        DfsFree(buf);
        LowLevelClose(llr,pei);
        if (lpfErrorOnOpeningDest!=NULL)
            *lpfErrorOnOpeningDest=TRUE;
        return FALSE;
    }

    fRet = TRUE;
    pos = 0;
    totalSize = size;
    while ((size>0) && (fRet))
    {
        if (size<bufsize)
            bufsize=((dfuLong32)size);
        if (LowLevelRead(llr,buf,bufsize,pei) != bufsize)
            fRet = FALSE;
        if (fRet)
            if (LowLevelWrite(llw,buf,bufsize,pei) != bufsize)
                fRet=FALSE;
        size -= bufsize;
        pos += bufsize;
        if (pProgressCopyFileCallBack!=NULL)
            (*pProgressCopyFileCallBack)(pos,totalSize,dfUserPtr);
    }

    DfsFree(buf);
    LowLevelClose(llr,pei);

    if (fRet && (!realCut))
    {
        if (GetFileSizeByName(lpExistingFileName, &dfsTm, NULL) == totalSize)
            pDfsTmChangeDate=&dfsTm;
    }
    LowLevelCloseChangeDateTime(llw, fFlushWrite, &dfsTm,&dateTimeChanged, pei);

    if ((pDfsTmChangeDate!=NULL) && (pDfsTmChangeDate==FALSE))
        ChangeDateTimeFile(lpNewFileName, &dfsTm);
    return fRet;
}

BOOL ManualCopyFile(dfwcharpc lpExistingFileName,dfwcharpc lpNewFileName,BOOL bFailIfExists, BOOL fFlushWrite, H_ERROR_INFO * pei)
{
    return CopyFileCutSize(lpExistingFileName,lpNewFileName,bFailIfExists, pei,
                                IsFileNameForMemorySpace(lpNewFileName),
                                FALSE,fFlushWrite,NULL, 0);
}






/***************************************************************************/

typedef struct
{
    STATIC_ARRAY_C sacVirtualFileNameSpace;
    dfuLong64 dfTotalSizeVirtualSpace;
    BOOL      fLimitSizeTotalVirtualSpace;
    dfuLong64 dfMaxSizeTotalVirtualSpace;
    SYNC_DIF_MUTEX_OBJECT SyncDifMutex;
} VIRTUALFILESPACE ;

static VIRTUALFILESPACE* pVirtualFileSpace=NULL;

/***************************************************************************/

#define GET_MUTEX \
   { \
        if (pVirtualFileSpace!=NULL) \
            if (pVirtualFileSpace->SyncDifMutex!=NULL) \
                SyncDifGetMutex(pVirtualFileSpace->SyncDifMutex); \
   } ;

#define RELEASE_MUTEX \
   { \
        if (pVirtualFileSpace!=NULL) \
            if (pVirtualFileSpace->SyncDifMutex!=NULL) \
                SyncDifReleaseMutex(pVirtualFileSpace->SyncDifMutex); \
   } ;

BOOL GetTempMemoryTmpFileName(dfwcharp dfFileName,dfuLong32 dfSizeFileName,dfuLong64 dfFileSize)
{
    static dfuLong32 dfLow=0;
    static dfuLong32 dfHigh=0;
    char szStr[0x80];

    dfuLong32 dfCurLow,dfCurHigh;

    GET_MUTEX
    dfLow ++;
    if (dfLow==0)
        dfHigh++;
    dfCurLow=dfLow;
    dfCurHigh=dfHigh;
    RELEASE_MUTEX

    sprintf(szStr,"*Tmp%08x%08x_%08x%08x",dfCurHigh,dfCurLow,((dfuLong32)(dfFileSize>>32)),(dfuLong32)dfFileSize);

    pVirtualFileSpace->dfTotalSizeVirtualSpace += dfFileSize;
    return ConvertAnsiToUnicode(szStr,dfFileName,dfSizeFileName);
}



typedef struct
{
    VIRTUALFILEITEMINFO* pvfii;
} VIRTUALFILEITEM;

BOOL DFSCALLBACK fncDestructorVirtualItem(const void* lpElem)
{
    VIRTUALFILEITEM* pvfi=(VIRTUALFILEITEM*)lpElem;
    if (pvfi->pvfii!=NULL)
    {
        if ((pvfi->pvfii->Buf!=NULL) && (!pvfi->pvfii->fIsPermanentReadOnlyBuffer))
        {
#if defined(_DEBUG) && defined(WINAPI)
            if ((pvfi->pvfii->posLastWrite != 0) && (pvfi->pvfii->posLastWrite != pvfi->pvfii->dfFileSize))
            {
                OutputDebugString("incomplete\n");
            }
#endif
            DfsFree(pvfi->pvfii->Buf);
            pVirtualFileSpace->dfTotalSizeVirtualSpace -= pvfi->pvfii->dfFileSize;
        }
        pvfi->pvfii->Buf=NULL;
        pvfi->pvfii->posLastWrite = 0;

        if (pvfi->pvfii->dfVirtualFileName != NULL)
            DfsFree((dfvoidp)(pvfi->pvfii->dfVirtualFileName));

        if (pvfi->pvfii->pVirtualFileDate != NULL)
            DfsFree((dfvoidp)(pvfi->pvfii->pVirtualFileDate));
        pvfi->pvfii->dfVirtualFileName=NULL;

        DfsFree(pvfi->pvfii);
        pvfi->pvfii=NULL;
    }
    return TRUE;
}

long DFSCALLBACK fncCmpVirtualItem(const void* lpElem1, const void* lpElem2)
{
    VIRTUALFILEITEM* pvfi1=(VIRTUALFILEITEM*)lpElem1;
    VIRTUALFILEITEM* pvfi2=(VIRTUALFILEITEM*)lpElem2;

    return dfUnicodeStrcmp(pvfi1->pvfii->dfVirtualFileName,pvfi2->pvfii->dfVirtualFileName);
}


BOOL InitStaticArrayVirtualFileNameSpace()
{
  if (pVirtualFileSpace->sacVirtualFileNameSpace!=NULL)
      return TRUE;

    pVirtualFileSpace->sacVirtualFileNameSpace = InitStaticArray_C(sizeof(VIRTUALFILEITEM),0x100);
    if (pVirtualFileSpace->sacVirtualFileNameSpace ==NULL)
        return FALSE;

    SetFuncDestructorSA(pVirtualFileSpace->sacVirtualFileNameSpace,&fncDestructorVirtualItem);
    SetFuncCompareDataSA(pVirtualFileSpace->sacVirtualFileNameSpace,&fncCmpVirtualItem);
    return TRUE;
}

BOOL SVFAPI dfsInitVirtualFileNameSpace()
{
    if (pVirtualFileSpace!=NULL)
    {
        return InitStaticArrayVirtualFileNameSpace();
    }

    pVirtualFileSpace = (VIRTUALFILESPACE*)DfsMalloc(sizeof(VIRTUALFILESPACE));
    if (pVirtualFileSpace==NULL)
    {
        return FALSE;
    }
    pVirtualFileSpace->sacVirtualFileNameSpace = NULL;
    pVirtualFileSpace->dfTotalSizeVirtualSpace = 0;
    pVirtualFileSpace->dfMaxSizeTotalVirtualSpace = 0;
    pVirtualFileSpace->fLimitSizeTotalVirtualSpace = FALSE;
    pVirtualFileSpace->SyncDifMutex = NULL;
    return InitStaticArrayVirtualFileNameSpace();
}

BOOL SVFAPI dfsSetVirtualFileNameMaximumMemory(BOOL fLimitSize, dfuLong64 dfMaxSize)
{
    if (!(dfsInitVirtualFileNameSpace()))
        return FALSE;
    pVirtualFileSpace->fLimitSizeTotalVirtualSpace = fLimitSize;
    //pVirtualFileSpace->dfMaxSizeTotalVirtualSpace = dfMaxSize*1024;
    pVirtualFileSpace->dfMaxSizeTotalVirtualSpace = dfMaxSize<<10;
    return TRUE;
}

STATIC_ARRAY_C GetSacVirtualFileNameSpace()
{
    if (!(dfsInitVirtualFileNameSpace()))
        return FALSE;
    if (pVirtualFileSpace==NULL)
        return NULL;
    return pVirtualFileSpace->sacVirtualFileNameSpace;
}

BOOL SVFAPI dfsUnInitVirtualFileNameSpace(BOOL fClearMaximumMemoryValue)
{
    if (pVirtualFileSpace!=NULL)
    {

        if (pVirtualFileSpace->sacVirtualFileNameSpace != NULL)
        {
#if defined(_DEBUG) && defined(WINAPI)
            if (GetNbElemSA(pVirtualFileSpace->sacVirtualFileNameSpace)>0)
                MessageBox(NULL,"Uncleared Virtual temp file",NULL,MB_OK|MB_ICONERROR);
#endif
            DeleteStaticArray_C(pVirtualFileSpace->sacVirtualFileNameSpace);
        }
        pVirtualFileSpace->sacVirtualFileNameSpace = NULL;

        pVirtualFileSpace->dfTotalSizeVirtualSpace = 0;

        if (((fClearMaximumMemoryValue) || (!pVirtualFileSpace->fLimitSizeTotalVirtualSpace)) && (pVirtualFileSpace->SyncDifMutex==NULL))
        {
            DfsFree(pVirtualFileSpace);
            pVirtualFileSpace = NULL;
        }
    }

    return TRUE;
}

SYNC_DIF_MUTEX_OBJECT SVFAPI GetVirtualFileNameSpaceMutex()
{
    if (!dfsInitVirtualFileNameSpace())
        return NULL;
    return pVirtualFileSpace->SyncDifMutex;
}

BOOL SVFAPI SetVirtualFileNameSpaceMutex(SYNC_DIF_MUTEX_OBJECT SyncDifMutexSet,SYNC_DIF_MUTEX_OBJECT* pSyncDifMutexSetPrev,BOOL fTryClear,BOOL fClearMaximumMemoryValue)
{
    if (!dfsInitVirtualFileNameSpace())
        return FALSE;
    if (pSyncDifMutexSetPrev!=NULL)
        *pSyncDifMutexSetPrev=pVirtualFileSpace->SyncDifMutex;
    pVirtualFileSpace->SyncDifMutex=SyncDifMutexSet;
    if ((SyncDifMutexSet==NULL) && fTryClear)
    {
        STATIC_ARRAY_C sacVirtualFileNameSpace=GetSacVirtualFileNameSpace();
        if (GetNbElemSA(sacVirtualFileNameSpace)==0)
        {
            dfsUnInitVirtualFileNameSpace(fClearMaximumMemoryValue);
        }
    }
    return TRUE;
}



dfuLong64 GetMaxSizeTotalVirtualSpace()
{
  dfuLong64 dfRet = ((((dfuLong64)GetPhysicalMemoryKb()) << 10)/5)*2;
  if ((sizeof(void*))<4)
      dfRet = dfmin(dfRet,1024*1024*1024);
  return dfRet;
}

dfuLong64 GetSuggestedSizeTotalVirtualSpaceMax()
{
  dfuLong64 dfRet = ((((dfuLong64)GetPhysicalMemoryKb()) << 10)/4)*1;
  if ((sizeof(void*))<=4)
      dfRet = dfmin(dfRet,1024*1024*1024);
  return dfRet;
}

BOOL CheckIfVirtualFilePossible(dfuLong64 dfFileSize)
{
    BOOL fRet;
    dfuLong64 dfMaxSizeTotalVirtualSpaceUse;
    if ((dfFileSize >> 32) != 0)
        return FALSE;

    if (!(dfsInitVirtualFileNameSpace()))
        return FALSE;
    if (pVirtualFileSpace->fLimitSizeTotalVirtualSpace)
        dfMaxSizeTotalVirtualSpaceUse = pVirtualFileSpace->dfMaxSizeTotalVirtualSpace;
    else
        dfMaxSizeTotalVirtualSpaceUse = GetSuggestedSizeTotalVirtualSpaceMax();
    dfMaxSizeTotalVirtualSpaceUse = dfmin(dfMaxSizeTotalVirtualSpaceUse,GetMaxSizeTotalVirtualSpace());

    fRet = (dfMaxSizeTotalVirtualSpaceUse >=
                  (pVirtualFileSpace->dfTotalSizeVirtualSpace + dfFileSize));

    return fRet;
}


VIRTUALFILEITEM* GetVirtualFileItemForVirtualFileName(dfwcharpc FileName)
{
    VIRTUALFILEITEM vfiSearch;
    VIRTUALFILEITEMINFO vfiSearchInfo;
    dfuLong32 dwPosItem;
    VIRTUALFILEITEM* pVfiFile;
    STATIC_ARRAY_C sacVirtualFileNameSpace=GetSacVirtualFileNameSpace();
    BOOL fFound;

    vfiSearchInfo.dfVirtualFileName = FileName;
    vfiSearch.pvfii = &vfiSearchInfo;
    GET_MUTEX;
    fFound = FindSameElemPosSA(sacVirtualFileNameSpace, &vfiSearch, &dwPosItem);
    RELEASE_MUTEX;
    if (!fFound)
        return NULL;
    pVfiFile=(VIRTUALFILEITEM*)GetElemPtrSA(sacVirtualFileNameSpace,dwPosItem);
    return pVfiFile;
}
/***************************************************************************/



LOWLEVELFILE memOpenOrBuildPermanentLowLevel(dfwcharpc FileName, TYPEOPEN TypeOpen,
                          BOOL fTemporaryFile, BOOL fProjectedSize, dfuLong64 dfProjectedSize,const void*bufPermanent,
                          H_ERROR_INFO * pei)
{
    // check if exist
    VIRTUALFILEITEM vfiSearch;
    VIRTUALFILEITEMINFO vfiSearchInfo;
    VIRTUALFILEITEM* pVfiFile;
    dfuLong32 dwPosItem=0;
    LOWLEVELINTERNAL* pLowLevelIntern;
    BOOL fCreatingFile = FALSE;
    STATIC_ARRAY_C sacVirtualFileNameSpace;
    BOOL fItemFound;
    if (!dfsInitVirtualFileNameSpace())
        return NULL;

    sacVirtualFileNameSpace=GetSacVirtualFileNameSpace();

    GET_MUTEX;

#if defined(_DEBUG) && defined(WINAPI)
    if (GetNbElemSA(sacVirtualFileNameSpace)==0)
    {
        OutputDebugString("\n**+ Start virtual\n");
    }
    {
        char sz[0x200];
        wsprintf(sz,"open virt file %ws, size=%08x %s\n",FileName,((dfuLong32)dfProjectedSize),(TypeOpen==OPEN_CREATE) ? " Create":"(nocreate)");
        OutputDebugString(sz);
    }
#endif
    vfiSearchInfo.dfVirtualFileName = FileName;
    vfiSearch.pvfii = &vfiSearchInfo;

    fItemFound = FindSameElemPosSA(sacVirtualFileNameSpace, &vfiSearch, &dwPosItem);

    if ((!fItemFound) && (TypeOpen!=OPEN_CREATE))
    {
#if defined(_DEBUG) && defined(WINAPI)
        OutputDebugString("internal error\n");
#endif
        RELEASE_MUTEX;
        return NULL;
    }

    if ((!fItemFound) && (TypeOpen==OPEN_CREATE))
    {
        VIRTUALFILEITEM vfiAdd;

#if defined(_DEBUG) && defined(WINAPI)
        if (!fTemporaryFile)
        {
            MessageBox(NULL,"bad usage : no temp in memDiskOpen create",NULL,MB_OK);
        }
#endif
        vfiAdd.pvfii=(VIRTUALFILEITEMINFO*)DfsMalloc(sizeof(VIRTUALFILEITEMINFO));
        vfiAdd.pvfii->dfVirtualFileName = dfUnicodeCopyAlloc(FileName);
        vfiAdd.pvfii->dfFileSize = dfProjectedSize;
        if (bufPermanent != NULL)
        {
            vfiAdd.pvfii->Buf=(dfbytep)bufPermanent;
            vfiAdd.pvfii->fIsPermanentReadOnlyBuffer=TRUE;
            vfiAdd.pvfii->posLastWrite = dfProjectedSize;
			vfiAdd.pvfii->allocatedBufSize = 0;
        }
        else
        {
            vfiAdd.pvfii->Buf=(dfbytep)DfsMalloc((size_t)(dfProjectedSize+1));
            vfiAdd.pvfii->fIsPermanentReadOnlyBuffer=FALSE;
            vfiAdd.pvfii->posLastWrite = 0;
			vfiAdd.pvfii->allocatedBufSize = (size_t)(dfProjectedSize + 1);
        }

        vfiAdd.pvfii->pVirtualFileDate = NULL;

        //pVirtualFileSpace->dfTotalSize += dfProjectedSize;

        if (vfiAdd.pvfii->Buf==NULL)
        {  // mem error
            RELEASE_MUTEX;
            return NULL;
        }

        fCreatingFile = InsertSortedSA(sacVirtualFileNameSpace,&vfiAdd);
        if (!fCreatingFile )
        {
            RELEASE_MUTEX;
            return NULL;
        }
        if (!FindSameElemPosSA(sacVirtualFileNameSpace, &vfiSearch, &dwPosItem))
        {
            RELEASE_MUTEX;
            return NULL;
        }
    }

#if defined(_DEBUG) && defined(WINAPI)
    if (fCreatingFile)
    {
        dfuLong32 i=0;
        while ((*(FileName+i)!= 0) && (*(FileName+i)!= '_'))
            i++;
        if (*(FileName+i)=='_')
        {
            char szStr[0x40];
            dfwchar dfStr[0x40*2];
            wsprintf(szStr,"_%08x%08x",((dfuLong32)(dfProjectedSize>>32)),(dfuLong32)dfProjectedSize);
            ConvertAnsiToUnicode(szStr,dfStr,0x40);
            if (DfsMemcmp(dfStr,FileName+i,(8+8+1)*sizeof(dfwchar))!=0)
            {
                OutputDebugString("\n\n**ERROR in size\n");
            }
        }
    }
#endif

    pVfiFile=(VIRTUALFILEITEM*)GetElemPtrSA(sacVirtualFileNameSpace,dwPosItem);
    pLowLevelIntern=(LOWLEVELINTERNAL*)DfsMalloc(sizeof(LOWLEVELINTERNAL));
    if (pLowLevelIntern==NULL)
    {
        RELEASE_MUTEX;
        return NULL;
    }
    pLowLevelIntern->u.hlFile = NULL;
    pLowLevelIntern->dfFileName = dfUnicodeCopyAlloc(FileName);
    pLowLevelIntern->pvfio = (VIRTUALFILEITEMFILEOPEN*)DfsMalloc(sizeof(VIRTUALFILEITEMFILEOPEN));
    pLowLevelIntern->pvfio->pvfi =pVfiFile->pvfii;
    pLowLevelIntern->pvfio->pos = 0;
    RELEASE_MUTEX;
    return (LOWLEVELFILE) pLowLevelIntern;
}

LOWLEVELFILE memOpenLowLevel(dfwcharpc FileName, TYPEOPEN TypeOpen,
                          BOOL fTemporaryFile, BOOL fProjectedSize, dfuLong64 dfProjectedSize,
                          H_ERROR_INFO * pei)
{
    return memOpenOrBuildPermanentLowLevel(FileName,TypeOpen,fTemporaryFile,fProjectedSize,dfProjectedSize,NULL,pei);
}

BOOL memDeleteFile(dfwcharpc lpFileName, H_ERROR_INFO* pei)
{
    VIRTUALFILEITEM vfiSearch;
    VIRTUALFILEITEMINFO vfiSearchInfo;
    dfuLong32 dwPosItem;
    BOOL fRet;
    STATIC_ARRAY_C sacVirtualFileNameSpace=GetSacVirtualFileNameSpace();

    vfiSearchInfo.dfVirtualFileName = lpFileName;
    vfiSearch.pvfii = &vfiSearchInfo;

    GET_MUTEX;

    if (!FindSameElemPosSA(sacVirtualFileNameSpace, &vfiSearch, &dwPosItem))
    {
        RELEASE_MUTEX;
        return FALSE;
    }
    fRet = DeleteElemSA(sacVirtualFileNameSpace,dwPosItem,1);


    {
        dfuLong32 dwNbElem=GetNbElemSA(sacVirtualFileNameSpace);
        if ((dwNbElem==0) && (pVirtualFileSpace->SyncDifMutex==NULL))
        {
            dfsUnInitVirtualFileNameSpace(FALSE);
        }
#if defined(_DEBUG) && defined(WINAPI)
        {
            char szOut[0x200];
            wsprintf(szOut,"nb item=%u after close %ws\n",dwNbElem,lpFileName);
            OutputDebugString(szOut);
        }
#endif

#if defined(_DEBUG) && defined(WINAPI)
        if (dwNbElem==0)
            OutputDebugString("\n**+ Clear virtual (nb item=0)\n");
#endif
    }
    RELEASE_MUTEX;
    return fRet;
}

BOOL myRenameFile(dfwcharpc lpFileName, dfwcharpc lpszCopyFileName, H_ERROR_INFO* pei)
{
    if (!MyCopyFile(lpFileName, lpszCopyFileName, FALSE,FALSE, pei))
        return FALSE;
    return MyDeleteFile(lpFileName, pei);
}

/*
BOOL memRenameFile(dfwcharpc lpFileName, dfwcharpc lpszCopyFileName, H_ERROR_INFO* pei)
{
    VIRTUALFILEITEM vfiSearch;
    VIRTUALFILEITEMINFO vfiSearchInfo;
    dfuLong32 dwPosItem;
    BOOL fRet;
    STATIC_ARRAY_C sacVirtualFileNameSpace=GetSacVirtualFileNameSpace();

	VIRTUALFILEITEM* pVfiFile;
    vfiSearchInfo.dfVirtualFileName = lpFileName;
    vfiSearch.pvfii = &vfiSearchInfo;

    GET_MUTEX;

    if (!FindSameElemPosSA(sacVirtualFileNameSpace, &vfiSearch, &dwPosItem))
    {
        RELEASE_MUTEX;
        return FALSE;
    }
	pVfiFile = (VIRTUALFILEITEM*)GetElemPtrSA(sacVirtualFileNameSpace, dwPosItem);
	// todo : recycle all data possible
    fRet = DeleteElemSA(sacVirtualFileNameSpace,dwPosItem,1);


    {
        dfuLong32 dwNbElem=GetNbElemSA(sacVirtualFileNameSpace);




    }
    RELEASE_MUTEX;
    return fRet;
}
*/

//LOWLEVELFILE memOpenLowLevel(dfwcharpc FileName, TYPEOPEN TypeOpen,

static dfuIntPtr memCheckSizeRead(LOWLEVELINTERNAL* pLowLevelIntern, dfuIntPtr size)
{
    if ((pLowLevelIntern->pvfio->pos+size)>pLowLevelIntern->pvfio->pvfi->dfFileSize)
    {
        size = (dfuIntPtr)(pLowLevelIntern->pvfio->pvfi->dfFileSize - pLowLevelIntern->pvfio->pos);
    }
    return size;
}

size_t idealBufSizeFor(size_t needed)
{
	size_t step = 0x200;
	while (step <= needed)
		step *= 2;
	return step;
}

static dfuIntPtr memCheckSizeWrite(LOWLEVELINTERNAL* pLowLevelIntern, dfuIntPtr size)
{
	if ((pLowLevelIntern->pvfio->pos + size)>pLowLevelIntern->pvfio->pvfi->dfFileSize)
	{
		if (pLowLevelIntern->pvfio->pvfi->fIsPermanentReadOnlyBuffer)
			size = (dfuIntPtr)(pLowLevelIntern->pvfio->pvfi->dfFileSize - pLowLevelIntern->pvfio->pos);
		else if (pLowLevelIntern->pvfio->pvfi->allocatedBufSize > (pLowLevelIntern->pvfio->pos + size))
		{
			pLowLevelIntern->pvfio->pvfi->dfFileSize = pLowLevelIntern->pvfio->pos + size;
		}
		else
		{
			size_t ideal_size = (size_t)idealBufSizeFor((size_t)(pLowLevelIntern->pvfio->pos + size));
			void * newBuf = DfsRealloc(pLowLevelIntern->pvfio->pvfi->Buf, ideal_size);
			if (newBuf == NULL)
			{
				size = (dfuIntPtr)(pLowLevelIntern->pvfio->pvfi->dfFileSize - pLowLevelIntern->pvfio->pos);
			}
			else
			{
				pLowLevelIntern->pvfio->pvfi->Buf = newBuf;
				pLowLevelIntern->pvfio->pvfi->dfFileSize = pLowLevelIntern->pvfio->pos + size;
				pLowLevelIntern->pvfio->pvfi->allocatedBufSize = ideal_size;
			}
		}
#if defined(_DEBUG) && defined(WINAPI)
		else
			MessageBox(0, "dep", "dep", 0);
#endif
	}
	return size;
}

static dfuLong32 memLowLevelWrite(LOWLEVELFILE llFile, void const *Buf, dfuLong32 sizeParam, H_ERROR_INFO * pei)
{
	dfuIntPtr size = sizeParam;
    LOWLEVELINTERNAL* pLowLevelIntern = (LOWLEVELINTERNAL*)llFile;
    if (pLowLevelIntern->pvfio->pvfi->fIsPermanentReadOnlyBuffer) {
        return 0;
    }
	size = memCheckSizeWrite(pLowLevelIntern, (dfuIntPtr)size);

    DfsMemcpy(pLowLevelIntern->pvfio->pvfi->Buf + pLowLevelIntern->pvfio->pos,Buf,size);
    pLowLevelIntern->pvfio->pos+=size;
    if (pLowLevelIntern->pvfio->pvfi->posLastWrite < pLowLevelIntern->pvfio->pos) {
        pLowLevelIntern->pvfio->pvfi->posLastWrite = pLowLevelIntern->pvfio->pos;
    }
	return (dfuLong32)size;
}

static BOOL memLowLevelSetFileSize(LOWLEVELFILE llFile, dfuLong32 PosLow, dfuLong32 PosHigh, H_ERROR_INFO * pei)
{
    LOWLEVELINTERNAL* pLowLevelIntern = (LOWLEVELINTERNAL*)llFile;
    /* to be checked */
    return pLowLevelIntern->pvfio->pvfi->dfFileSize == MakeuLong64(PosLow,PosHigh);
}

static dfuLong32 memLowLevelRead(LOWLEVELFILE llFile, void *Buf, dfuLong32 size, H_ERROR_INFO* pei)
{
    LOWLEVELINTERNAL* pLowLevelIntern = (LOWLEVELINTERNAL*)llFile;
	size = (dfuLong32)memCheckSizeRead(pLowLevelIntern, size);
    DfsMemcpy(Buf,pLowLevelIntern->pvfio->pvfi->Buf + pLowLevelIntern->pvfio->pos,size);
    pLowLevelIntern->pvfio->pos+=size;
    return size;
}

static BOOL memLowLevelSeek(LOWLEVELFILE llFile, dfuLong32 PosLow, dfuLong32 PosHigh, TYPESEEK TypeSeek, H_ERROR_INFO* pei)
{
    BOOL fRet=TRUE;
    LOWLEVELINTERNAL* pLowLevelIntern = (LOWLEVELINTERNAL*)llFile;
    dfuLong64 newpos = MakeuLong64(PosLow,PosHigh);


    if (newpos > pLowLevelIntern->pvfio->pvfi->dfFileSize)
    {
        newpos = pLowLevelIntern->pvfio->pvfi->dfFileSize;
        fRet=FALSE;
    }

    if (TypeSeek==TYPESEEK_END)
    {
        newpos = pLowLevelIntern->pvfio->pvfi->dfFileSize - newpos;
    }

    pLowLevelIntern->pvfio->pos=newpos;
    return fRet;
}

static void memLowLevelGetSize(LOWLEVELFILE llFile, dfuLong32 *PosLow, dfuLong32 *PosHigh)
{
    LOWLEVELINTERNAL* pLowLevelIntern = (LOWLEVELINTERNAL*)llFile;
    if (PosLow!=NULL)
        *PosLow = GetuLong64Low32(pLowLevelIntern->pvfio->pvfi->dfFileSize);
    if (PosHigh!=NULL)
        *PosHigh = GetuLong64High32(pLowLevelIntern->pvfio->pvfi->dfFileSize);
}


static void memLowLevelTell(LOWLEVELFILE llFile, dfuLong32 *PosLow, dfuLong32 *PosHigh)
{
    LOWLEVELINTERNAL* pLowLevelIntern = (LOWLEVELINTERNAL*)llFile;
    if (PosLow!=NULL)
        *PosLow = GetuLong64Low32(pLowLevelIntern->pvfio->pos);
    if (PosHigh!=NULL)
        *PosHigh = GetuLong64High32(pLowLevelIntern->pvfio->pos);
}

static BOOL memLowLevelClose(LOWLEVELFILE llFile, H_ERROR_INFO* pei)
{
  BOOL fRet=TRUE;
  LOWLEVELINTERNAL* pLowLevelIntern = (LOWLEVELINTERNAL*)llFile;

#if defined(_DEBUG) && defined(WINAPI)
  {
    if ((pLowLevelIntern->pvfio->pos)!=pLowLevelIntern->pvfio->pvfi->dfFileSize)
    {
        MessageBox(0,"depnot ful","dep not ful",0);
    }
  }
#endif
//  DfsFree(pLowLevelIntern->pvfio->pvfi);

  if (pLowLevelIntern->dfFileName!=NULL)
    DfsFree(pLowLevelIntern->dfFileName);
  if (pLowLevelIntern->pvfio!=NULL)
      DfsFree(pLowLevelIntern->pvfio);
  DfsFree(pLowLevelIntern);
  return fRet;
}


dfuLong32 SVFAPI LowLevelWrite(LOWLEVELFILE llFile, void const *Buf, dfuLong32 size, H_ERROR_INFO * pei)
{
  LOWLEVELINTERNAL* pLowLevelIntern = (LOWLEVELINTERNAL*)llFile;

  if (pLowLevelIntern == NULL)
      return 0;
  if (pLowLevelIntern->u.hlFile!=NULL)
      return diskLowLevelWrite(llFile, Buf, size, pei);
  if (pLowLevelIntern->pvfio!=NULL)
      return memLowLevelWrite(llFile, Buf, size, pei);
  return 0;
}

BOOL SVFAPI LowLevelSetFileSize(LOWLEVELFILE llFile, dfuLong32 PosLow, dfuLong32 PosHigh, H_ERROR_INFO * pei)
{
  LOWLEVELINTERNAL* pLowLevelIntern = (LOWLEVELINTERNAL*)llFile;

  if (pLowLevelIntern == NULL)
      return FALSE;
  if (pLowLevelIntern->u.hlFile!=NULL)
      return diskLowLevelSetFileSize(llFile, PosLow, PosHigh, pei);
  if (pLowLevelIntern->pvfio!=NULL)
      return memLowLevelSetFileSize(llFile, PosLow, PosHigh, pei);
  return FALSE;
}

dfuLong32 SVFAPI LowLevelRead(LOWLEVELFILE llFile, void *Buf, dfuLong32 size, H_ERROR_INFO* pei)
{
  LOWLEVELINTERNAL* pLowLevelIntern = (LOWLEVELINTERNAL*)llFile;

  if (pLowLevelIntern == NULL)
      return FALSE;
  if (pLowLevelIntern->u.hlFile!=NULL)
      return diskLowLevelRead(llFile, Buf, size, pei);
  if (pLowLevelIntern->pvfio!=NULL)
      return memLowLevelRead(llFile, Buf, size, pei);
  return FALSE;
}

BOOL SVFAPI LowLevelSeek(LOWLEVELFILE llFile, dfuLong32 PosLow, dfuLong32 PosHigh, TYPESEEK TypeSeek, H_ERROR_INFO* pei)
{
  LOWLEVELINTERNAL* pLowLevelIntern = (LOWLEVELINTERNAL*)llFile;

  if (pLowLevelIntern == NULL)
      return FALSE;
  if (pLowLevelIntern->u.hlFile!=NULL)
      return diskLowLevelSeek(llFile, PosLow, PosHigh, TypeSeek, pei);
  if (pLowLevelIntern->pvfio!=NULL)
      return memLowLevelSeek(llFile, PosLow, PosHigh, TypeSeek, pei);
  return FALSE;
}

void SVFAPI LowLevelTell(LOWLEVELFILE llFile, dfuLong32 *PosLow, dfuLong32 *PosHigh)
{
  LOWLEVELINTERNAL* pLowLevelIntern = (LOWLEVELINTERNAL*)llFile;

  if (pLowLevelIntern == NULL)
      return ;

  if (pLowLevelIntern->u.hlFile!=NULL)
      diskLowLevelTell(llFile, PosLow, PosHigh);
  else
  if (pLowLevelIntern->pvfio!=NULL)
      memLowLevelTell(llFile, PosLow, PosHigh);
}


void SVFAPI LowLevelGetSize(LOWLEVELFILE llFile, dfuLong32 *PosLow, dfuLong32 *PosHigh)
{
  LOWLEVELINTERNAL* pLowLevelIntern = (LOWLEVELINTERNAL*)llFile;

  if (pLowLevelIntern == NULL)
      return ;
  if (pLowLevelIntern->u.hlFile!=NULL)
      diskLowLevelGetSize(llFile, PosLow, PosHigh);
  else if (pLowLevelIntern->pvfio!=NULL)
      memLowLevelGetSize(llFile, PosLow, PosHigh);
}



BOOL SVFAPI LowLevelSeek64(LOWLEVELFILE llfFile, dfuLong64 Pos, TYPESEEK TypeSeek, H_ERROR_INFO* pei)
{
    BOOL ret;
    dfuLong32 dfPosLow, dfPosHigh;
    dfPosLow = GetuLong64Low32(Pos);
    dfPosHigh = GetuLong64High32(Pos);
    ret = LowLevelSeek(llfFile, dfPosLow, dfPosHigh, TypeSeek, pei);
    return ret;
}

void SVFAPI LowLevelTell64(LOWLEVELFILE llfFile, dfuLong64 *Pos)
{
    dfuLong32 dfPosLow=0;
    dfuLong32 dfPosHigh=0;
    LowLevelTell(llfFile, &dfPosLow, &dfPosHigh);
    *Pos = MakeuLong64(dfPosLow, dfPosHigh);
}


void SVFAPI LowLevelGetSize64(LOWLEVELFILE llfFile, dfuLong64 *Pos)
{
    dfuLong32 dfPosLow=0;
    dfuLong32 dfPosHigh=0;
    LowLevelGetSize(llfFile, &dfPosLow, &dfPosHigh);
    *Pos = MakeuLong64(dfPosLow, dfPosHigh);
}


BOOL SVFAPI LowLevelSetFileSize64(LOWLEVELFILE llfFile, dfuLong64 Pos, H_ERROR_INFO* pei)
{
    BOOL ret;
    dfuLong32 dfPosLow, dfPosHigh;
    dfPosLow = GetuLong64Low32(Pos);
    dfPosHigh = GetuLong64High32(Pos);
    ret = LowLevelSetFileSize(llfFile, dfPosLow, dfPosHigh,pei);
    return ret;
}


BOOL SVFAPI LowLevelClose(LOWLEVELFILE llFile, H_ERROR_INFO* pei)
{
  LOWLEVELINTERNAL* pLowLevelIntern = (LOWLEVELINTERNAL*)llFile;

  if (pLowLevelIntern == NULL)
      return FALSE;
  if (pLowLevelIntern->u.hlFile!=NULL)
      return diskLowLevelClose(llFile, FALSE, pei);
  if (pLowLevelIntern->pvfio!=NULL)
      return memLowLevelClose(llFile, pei);
  DfsFree(pLowLevelIntern);
  return FALSE;
}

BOOL SVFAPI LowLevelCloseEx(LOWLEVELFILE llFile, BOOL fFlushWrite, H_ERROR_INFO* pei)
{
  LOWLEVELINTERNAL* pLowLevelIntern = (LOWLEVELINTERNAL*)llFile;

  if (pLowLevelIntern == NULL)
      return FALSE;
  if (pLowLevelIntern->u.hlFile!=NULL)
      return diskLowLevelClose(llFile, fFlushWrite, pei);
  if (pLowLevelIntern->pvfio!=NULL)
      return memLowLevelClose(llFile, pei);
  DfsFree(pLowLevelIntern);
  return FALSE;
}


BOOL SVFAPI LowLevelCloseChangeDateTime(LOWLEVELFILE llFile, BOOL fFlushWrite, const DFSTM* pDfsTm, BOOL* pDateTimeChanged, H_ERROR_INFO* pei)
{
    LOWLEVELINTERNAL* pLowLevelIntern = (LOWLEVELINTERNAL*)llFile;

    if (pDateTimeChanged!=NULL)
        *pDateTimeChanged=FALSE;
    if (pLowLevelIntern == NULL)
        return FALSE;
    if (pLowLevelIntern->u.hlFile != NULL)
        return diskLowLevelCloseChangeDateTime(llFile, fFlushWrite, pDfsTm, pDateTimeChanged, pei);
    if (pLowLevelIntern->pvfio != NULL)
        return memLowLevelClose(llFile, pei);
    DfsFree(pLowLevelIntern);
    return FALSE;
}

BOOL IsFileNameForMemorySpace(const dfwchar* FileName)
{
    if (FileName!=NULL)
        if ((*FileName)=='*')
            return TRUE;
    return FALSE;
}

BOOL SVFAPI DeleteTempFile(dfwcharpc lpFileName, H_ERROR_INFO* pei)
{
    if (!IsFileNameForMemorySpace(lpFileName))
        return MyDeleteFile(lpFileName,pei);
    return memDeleteFile(lpFileName,pei);
}


/***************************************************************************/

/***************************************************************************/
/***************************************************************************/



LOWLEVELFILE SVFAPI OpenLowLevel(dfwcharpc FileName, TYPEOPEN TypeOpen,
                          BOOL fTemporaryFile, BOOL fProjectedSize, dfuLong64 dfProjectedSize,
                          H_ERROR_INFO * pei)
{
    if (IsFileNameForMemorySpace(FileName))
        return memOpenLowLevel(FileName, TypeOpen,
                          fTemporaryFile, fProjectedSize, dfProjectedSize, pei);
    return diskOpenLowLevel(FileName, TypeOpen,
                        fTemporaryFile, fProjectedSize, dfProjectedSize, pei);
}
/***************************************************************************/






BOOL SVFAPI SplitFileNameAndPath(dfwcharpc dfFullPathName,
                          dfwcharp dfOnlyPath,dfuLong32 dfPathSize,
                          dfwcharp dfOnlyFileName,dfuLong32 dfFileNameSize,
                          BOOL fOperatingSystemSeparatorInPath)
{
    int i;
    dfuLong32 iln = dfUnicodeStrlen(dfFullPathName);

    if ((dfPathSize>0) && (dfOnlyPath!=NULL))
        dfOnlyPath[0]=0;
    if ((dfFileNameSize>0) && (dfOnlyFileName!=NULL))
        dfOnlyFileName[0]=0;
    for (i=iln;i>=0;i--)
    {
        if (CompareUnicodeWithSimpleChar(dfFullPathName[i],'/'))
            break;
    }

    if (i==-1)
    {
        if (dfFileNameSize>iln)
        {
            dfUnicodeCopyConcat(dfOnlyFileName,dfFullPathName,NULL);
            return TRUE;
        }
        else
            return FALSE;
    }
    else
    {
        BOOL fRet;
        int j;
        fRet = (i<(signed)dfPathSize) && ((iln-i)+1<(signed)dfFileNameSize);
        if (fRet)
        {
          DfsMemcpy(dfOnlyFileName,((dfwcharpc)dfFullPathName)+i+1,2*((iln-i)-0));
          DfsMemcpy(dfOnlyPath,dfFullPathName,2*i);
          dfOnlyPath[i] = 0;

          if (fOperatingSystemSeparatorInPath)
              for (j=0;j<i;j++)
              {/*
                  if (dfOnlyPath[j] == '/')
                      (dfOnlyPath[j] = '\\');
                      */
                  if (CompareUnicodeWithSimpleChar(dfOnlyPath[j],'/') || (CompareUnicodeWithSimpleChar(dfOnlyPath[j],'\\')))
                      dfOnlyPath[j] = *GetUnicodeStringDirectorySeparator();
              }
        }
        return fRet;
    }
}

BOOL SVFAPI ConvertFileNameAndPath(dfwcharpc dfFullPathName,
                          dfwcharp dfFullPathNameAdapted,dfuLong32 dfPathSize,
                          BOOL fConvertToOperatingSystem)
{
    dfuLong32 i=0;
    dfwchar wc;
    dfwcharp dfFullPathWrite;
    if (dfFullPathNameAdapted == NULL)
        dfFullPathWrite = (dfwcharp)dfFullPathName;
    else
    {
        dfuLong32 iln = dfUnicodeStrlen(dfFullPathName);
        if (dfPathSize < iln + 1)
            return FALSE;
        dfFullPathWrite = dfFullPathNameAdapted;
    }

    do
    {
        wc = dfFullPathName[i];
        if (fConvertToOperatingSystem && (
              (CompareUnicodeWithSimpleChar(wc,'/') || CompareUnicodeWithSimpleChar(wc,'\\'))))
            wc = *GetUnicodeStringDirectorySeparator();//GetUnicodeStringAntiSlashSeparator();
        if ((!fConvertToOperatingSystem) && (CompareUnicodeWithSimpleChar(wc,'\\')))
            wc = *GetUnicodeStringSlashSeparator();

        dfFullPathWrite[i] = wc;
        i++;
    }
    while (wc != 0) ;
    return TRUE;
}



dfuLong64 memGetFileSizeByName(dfwcharpc FileName, DFSTM * pDfsTm, H_ERROR_INFO* pei)
{
    VIRTUALFILEITEM* pVfiFile;
    pVfiFile=GetVirtualFileItemForVirtualFileName(FileName);
    if (pVfiFile==NULL)
        return 0;

    if ((pVfiFile->pvfii->pVirtualFileDate != NULL) && (pDfsTm != NULL))
        *pDfsTm = *(pVfiFile->pvfii->pVirtualFileDate);
    return (dfuLong64)pVfiFile->pvfii->dfFileSize;
}

dfuLong64 SVFAPI GetFileSizeByName(dfwcharpc FileName, DFSTM * pDfsTm, H_ERROR_INFO* pei)
{
    if (IsFileNameForMemorySpace(FileName))
        return memGetFileSizeByName(FileName, pDfsTm, pei);
    return diskGetFileSizeByName(FileName, pDfsTm, pei);

}
//DECLARE_DFHANDLE(HFILECONTENTREADBUF);



BOOL memChangeDateTimeFile(dfwcharpc FileName, const DFSTM * pDfsTm)
{
    VIRTUALFILEITEM* pVfiFile;
    pVfiFile = GetVirtualFileItemForVirtualFileName(FileName);
    if ((pVfiFile == NULL) || (pDfsTm == NULL))
        return 0;

    if (pVfiFile->pvfii->pVirtualFileDate == NULL)
    {
        pVfiFile->pvfii->pVirtualFileDate = (DFSTM*)DfsMalloc(sizeof(DFSTM));
        if (pVfiFile->pvfii->pVirtualFileDate == NULL)
            return FALSE;
    }

    *(pVfiFile->pvfii->pVirtualFileDate) = *pDfsTm;

    return TRUE;
}

BOOL SVFAPI ChangeDateTimeFile(dfwcharpc FileName, const DFSTM * pDfsTm)
{
    if (IsFileNameForMemorySpace(FileName))
        return memChangeDateTimeFile(FileName, pDfsTm);
    return diskChangeDateTimeFile(FileName, pDfsTm);
}



BOOL memGetFileFullContentBuffer(dfwcharpc FileName,dfuLong32 dfAccessFlag,dfuLong64 dfSizeRequested,//BOOL fReadOnly,
                                  HFILECONTENTREADBUF* phFileContentReadBuf,
                                  ORIGDATA* pOrg,
                                  H_ERROR_INFO* pei)
{
    VIRTUALFILEITEM* pVfiFile;
    FILECONTENTREADBUFINTERNAL *pFileContentReadBufInternal;

    if (pOrg == NULL)
        return FALSE;

    FillOrigDataForFullMemoryOrg(pOrg,NULL,FILE_SIZE_NOT_EXIST);

    if (phFileContentReadBuf==NULL)
        return FALSE;

    pVfiFile=GetVirtualFileItemForVirtualFileName(FileName);
    if (pVfiFile==NULL)
        return FALSE;

    if (((dfAccessFlag & (FILEFULLCONTENTBUFFER_WRITE | FILEFULLCONTENTBUFFER_RESIZE)) != 0) && (pVfiFile->pvfii->fIsPermanentReadOnlyBuffer))
        return FALSE;
    // TODO : resize
    if ((dfAccessFlag & FILEFULLCONTENTBUFFER_RESIZE) != 0)
    {
        dfbytep NewBuf = (dfbytep)DfsRealloc(pVfiFile->pvfii->Buf,(size_t)(dfSizeRequested+1));
        if (NewBuf == NULL)
            return FALSE;
        pVfiFile->pvfii->Buf = NewBuf;
        pVfiFile->pvfii->dfFileSize = dfSizeRequested;
        pVfiFile->pvfii->posLastWrite = dfSizeRequested;
    }
    pFileContentReadBufInternal=(FILECONTENTREADBUFINTERNAL*)DfsMalloc(sizeof(FILECONTENTREADBUFINTERNAL));
    if (phFileContentReadBuf==NULL) {
        return FALSE;
    }
	memset(pFileContentReadBufInternal, 0, sizeof(FILECONTENTREADBUFINTERNAL));
    *phFileContentReadBuf=(HFILECONTENTREADBUF)pFileContentReadBufInternal;

    pFileContentReadBufInternal->fPartialMapping = FALSE;
    pFileContentReadBufInternal->dfBeginMapPos = 0;
    pFileContentReadBufInternal->dfMapSize = 0;
    pFileContentReadBufInternal->dfGranularityMapping = 0;

    pFileContentReadBufInternal->fWritePossible=FALSE;

    pFileContentReadBufInternal->dwSize = pVfiFile->pvfii->dfFileSize;
    pFileContentReadBufInternal->lpBuf = pVfiFile->pvfii->Buf;

    FillOrigDataForFullMemoryOrg(pOrg,pFileContentReadBufInternal->lpBuf,pFileContentReadBufInternal->dwSize);

    pFileContentReadBufInternal->fVirtualMemFile = TRUE;
    pFileContentReadBufInternal->u.wh.hFile = NULL;
    pFileContentReadBufInternal->u.wh.hFileMap = NULL;

    return TRUE;
}

BOOL SVFAPI GetFileFullContentBuffer(dfwcharpc FileName,dfuLong32 dfAccessFlag,dfuLong64 dfSizeRequested,
                                  HFILECONTENTREADBUF* phFileContentReadBuf,
                                  ORIGDATA* pOrg,
                                  H_ERROR_INFO* pei)
{
    if (!IsFileNameForMemorySpace(FileName))
        return diskGetFileFullContentBuffer(FileName, dfAccessFlag, dfSizeRequested,phFileContentReadBuf,
                                  pOrg,pei);

    return memGetFileFullContentBuffer(FileName, dfAccessFlag, dfSizeRequested,phFileContentReadBuf,
                                  pOrg,pei);
}


BOOL memCloseFileFullContentBuffer(FILECONTENTREADBUFINTERNAL *pFileContentReadBufInternal)
{
  if (pFileContentReadBufInternal==NULL)
      return FALSE;

  DfsFree(pFileContentReadBufInternal);
  return TRUE;
}

BOOL SVFAPI CloseFileFullContentBuffer(HFILECONTENTREADBUF hFileContentReadBuf)
{
  FILECONTENTREADBUFINTERNAL *pFileContentReadBufInternal;

  if (hFileContentReadBuf == NULL)
    return FALSE;

  pFileContentReadBufInternal =
    (FILECONTENTREADBUFINTERNAL *) hFileContentReadBuf;

  if (!pFileContentReadBufInternal->fVirtualMemFile)
  {
      return diskCloseFileFullContentBuffer(pFileContentReadBufInternal, FALSE,NULL);
  }
  else
  {
      return memCloseFileFullContentBuffer(pFileContentReadBufInternal);
  }
}

BOOL SVFAPI CloseFileFullContentBufferEx(HFILECONTENTREADBUF hFileContentReadBuf,BOOL fFlushWrite,H_ERROR_INFO* pei)
{
  FILECONTENTREADBUFINTERNAL *pFileContentReadBufInternal;

  if (hFileContentReadBuf == NULL)
    return FALSE;

  pFileContentReadBufInternal =
    (FILECONTENTREADBUFINTERNAL *) hFileContentReadBuf;

  if (!pFileContentReadBufInternal->fVirtualMemFile)
  {
      return diskCloseFileFullContentBuffer(pFileContentReadBufInternal,fFlushWrite,pei);
  }
  else
  {
      return memCloseFileFullContentBuffer(pFileContentReadBufInternal);
  }
}


BOOL SVFAPI PerformInitAndMutexForMemFS()
{
    BOOL ret = TRUE;

    dfsInitVirtualFileNameSpace();

    if (GetVirtualFileNameSpaceMutex() == NULL)
    {
        SYNC_DIF_MUTEX_OBJECT SyncDifMutex = SyncDifBuildMutex();
        if (SyncDifMutex == NULL)
            ret = FALSE;
        else
            if (!SetVirtualFileNameSpaceMutex(SyncDifMutex, NULL, FALSE, FALSE))
            {
                SyncDifDeleteMutex(SyncDifMutex);
                ret = FALSE;
            }
    }

    GET_MUTEX
    RELEASE_MUTEX

    return ret;
}

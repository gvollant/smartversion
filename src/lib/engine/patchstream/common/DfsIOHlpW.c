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

/* DfsIOhlpW.c */


#include "difbasic.h"

#if defined(SMARTVERSION_USE_WIN32)

#include <stdlib.h>
#include <stdio.h>
//#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <windows.h>
#include "DfsTlTyp.h"
#include "difstool.h"
#include "DfsOrigMemoryMap.h"
#include "DfsIoHlp.h"
#include "ltoolsc.h"

#include "DfsIOHlpInternal.h"



// for test remove FMIO set #define WINRT_PHONE_WITHOUT_MAP
//#define WINRT_PHONE_WITHOUT_MAP

#if (defined(WINRT_PHONE_WITHOUT_MAP)) && (!(defined(PREVENT_USING_WINDOWS_FILEMAP)))  && (!(defined(FORCE_USING_WINDOWS_FILEMAP)))
#define PREVENT_USING_WINDOWS_FILEMAP
#endif

#if defined(WINAPI_FAMILY) && (!(defined(DIFSTRM_USING_WINRT_API))) && (!(defined(PREVENT_DIFSTRM_USING_WINRT_API)))
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#else
#define DIFSTRM_USING_WINRT_API 1
#endif
#endif

#ifdef DIFSTRM_USING_WINRT_API
#include "dfsToolWinRT.h"
#endif

#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER (0xffffffff)
#endif

extern dfwchar szTempDirectoryPreset[MAX_PATH_LENGTH+1];
extern BOOL  fTempDirectoryPreset;

#define MyAroundLower(dwValue,dwModulo) ((((dwValue)) / ((dwModulo))) * (dwModulo))
#define MyAroundUpper(dwValue,dwModulo) (((((dwValue)) + ((dwModulo)) -1) / ((dwModulo))) * (dwModulo))

#define DEFAULT_GRANULARITY_MAPPING 32768
#define DEFAULT_DIRTY_UNIT_PER_GRANUL (DEFAULT_GRANULARITY_MAPPING/4096)
#define MAX_MAP_VIEW_SIZE (1024*1024*16)

BOOL SVFAPI GetTempDirectorySystem(dfwcharp lpDirectory,dfuLong32 dfSizeBufferinwChar)
{
    dfwcharp szDirectoryUse;
    dfwchar   szTempDirectoryWindows[MAX_PATH_LENGTH+1];
    if (!fTempDirectoryPreset)
    {
#ifdef DIFSTRM_USING_WINRT_API
        GetTempPathWinRT(MAX_PATH,(LPWSTR)szTempDirectoryWindows);
#else
        if (IsUnicodeApiSupported())
        {
            GetTempPathW(MAX_PATH,szTempDirectoryWindows);
        }
        else
        {
            char szDirectoryAnsi[MAX_PATH_LENGTH+1];
            GetTempPathA(MAX_PATH,szDirectoryAnsi);
            ConvertAnsiToUnicode(szDirectoryAnsi,szTempDirectoryWindows,MAX_PATH_LENGTH);
        }
#endif
        szDirectoryUse = szTempDirectoryWindows;
    }
    else
        szDirectoryUse = szTempDirectoryPreset;

    if (dfUnicodeStrlen(szDirectoryUse)>=dfSizeBufferinwChar)
        return FALSE;

    dfUnicodeCopyConcat(lpDirectory,szDirectoryUse,NULL);
    return TRUE;
}


BOOL SVFAPI MyCopyFile(dfwcharpc lpExistingFileName,dfwcharpc lpNewFileName,BOOL bFailIfExists, BOOL fFlushWrite, H_ERROR_INFO * pei)
{
  BOOL fRet;


#ifdef DIFSTRM_USING_WINRT_API
  COPYFILE2_EXTENDED_PARAMETERS cep;
#endif

  if (IsFileNameForMemorySpace(lpExistingFileName) || IsFileNameForMemorySpace(lpNewFileName) || fFlushWrite)
  {
#if defined(_DEBUG) && defined(TCHAR)
      MessageBoxA(0,"bug MyCopyFile : using on temp","bug MyCopyFileW",0);
#endif
      return ManualCopyFile(lpExistingFileName, lpNewFileName, bFailIfExists, fFlushWrite, pei);
  }

#ifdef DIFSTRM_USING_WINRT_API
  memset(&cep,0,sizeof(cep));
  cep.dwSize = sizeof(cep);
  cep.dwCopyFlags = COPY_FILE_ALLOW_DECRYPTED_DESTINATION | (bFailIfExists ? COPY_FILE_FAIL_IF_EXISTS : 0);
  fRet = CopyFile2((PCWSTR)lpExistingFileName,(PCWSTR)lpNewFileName,&cep);
#else
  if (IsUnicodeApiSupported())
      fRet = CopyFileW(lpExistingFileName,lpNewFileName,bFailIfExists);
  else
  {
    char szAnsiExistingFileName[512+MAX_PATH];
    char szAnsiNewFileName[512+MAX_PATH];

    ConvertUnicodeToAnsi(lpExistingFileName, szAnsiExistingFileName,
                         sizeof(szAnsiExistingFileName) - 1);
    ConvertUnicodeToAnsi(lpNewFileName, szAnsiNewFileName,
                         sizeof(szAnsiNewFileName) - 1);
    fRet=CopyFileA(szAnsiExistingFileName,szAnsiNewFileName,bFailIfExists);
  }
#endif
  return fRet;
}

BOOL SVFAPI MyDeleteFile(dfwcharpc lpFileName, H_ERROR_INFO * pei)
{
  BOOL fRet;

  if (IsFileNameForMemorySpace(lpFileName))
      return DeleteTempFile(lpFileName, pei);

  if (IsUnicodeApiSupported())
      fRet=DeleteFileW(lpFileName);
  else
  {

    char szAnsiFileName[512+MAX_PATH];

    ConvertUnicodeToAnsi(lpFileName, szAnsiFileName,
                         sizeof(szAnsiFileName) - 1);
    fRet=DeleteFileA(szAnsiFileName);
  }

  return fRet;
}

HANDLE SVFAPI MyCreateFileW(dfwcharpc FileName,        // file name
                     DWORD dwDesiredAccess,     // access mode
                     DWORD dwShareMode, // share mode
                     LPSECURITY_ATTRIBUTES lpSecurityAttributes,        // SD
                     DWORD dwCreationDisposition,       // how to create
                     DWORD dwFlagsAndAttributes,        // file attributes
                     HANDLE hTemplateFile       // handle to template file
  )
{
  HANDLE hRet;
  dfuLong32 dfSize;
  dfwcharp pFileNameAdapted;


#ifdef DIFSTRM_USING_WINRT_API
  CREATEFILE2_EXTENDED_PARAMETERS cep;
#endif

  dfSize = (dfUnicodeStrlen(FileName) *2) + 0x10;
  pFileNameAdapted = (dfwcharp)DfsMalloc(dfSize * 2);

  ConvertFileNameAndPath(FileName,pFileNameAdapted,dfSize,TRUE);

#ifdef DIFSTRM_USING_WINRT_API
  memset(&cep,0,sizeof(cep));
  cep.dwSize = sizeof(cep);
  cep.hTemplateFile = hTemplateFile;
  cep.lpSecurityAttributes = lpSecurityAttributes;
  cep.dwFileFlags = dwFlagsAndAttributes;

  hRet =
      CreateFile2((LPCWSTR)pFileNameAdapted, dwDesiredAccess, dwShareMode,
                  dwCreationDisposition,&cep);
#else
  if (IsUnicodeApiSupported())
  {
    hRet =
      CreateFileW(pFileNameAdapted, dwDesiredAccess, dwShareMode,
                  lpSecurityAttributes, dwCreationDisposition,
                  dwFlagsAndAttributes, hTemplateFile);
    #ifdef _DEBUG
    if (hRet == INVALID_HANDLE_VALUE)
    {
        DWORD dwErr = GetLastError();
        WCHAR sz[128+MAX_PATH];
        wsprintfW(sz,L"error opening %ws = %u\n",pFileNameAdapted,dwErr);
        OutputDebugStringW(sz);
    }
    #endif
  }
  else
  {
    char szAnsiFileName[512+MAX_PATH];

    ConvertUnicodeToAnsi(pFileNameAdapted, szAnsiFileName,
                         sizeof(szAnsiFileName) - 1);
    hRet =
      CreateFileA(szAnsiFileName, dwDesiredAccess, dwShareMode,
                  lpSecurityAttributes, dwCreationDisposition,
                  dwFlagsAndAttributes, hTemplateFile);
  }
#endif
  DfsFree(pFileNameAdapted);
  return hRet;
}



BOOL BuildErrorInfoWinError(dfuLong32 dwErr,ERROR_INFO_ITEM* peii)
{
    dfwcharp lpBuffer=NULL;

    if (dwErr != 0)
    {
        //dfwchar szErrTxt[(MAX_PATH*2)+1];
        dfuLong32 dwNbChar;

#ifdef DIFSTRM_USING_WINRT_API
        DWORD dwAllocBuf = 32767;
        LPWSTR buf = (LPWSTR)DfsMalloc((dwAllocBuf + 1) * sizeof(WCHAR));
        *buf=0;
        dwNbChar = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,NULL,dwErr,0,buf,dwAllocBuf,NULL);
        lpBuffer = (dfwcharp)DfsMalloc((dfUnicodeStrlen((dfwcharp)buf)+1)*2);
        DfsMemcpy(lpBuffer,buf,(dfUnicodeStrlen((dfwcharp)buf)+1)*2);
        DfsFree(buf);
#else
        if (IsUnicodeApiSupported())
        {
            LPWSTR buf = NULL;
            dwNbChar=FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_ALLOCATE_BUFFER,NULL,dwErr,0,
                (LPWSTR)(&buf),MAX_PATH,NULL) ;
            lpBuffer = (dfwcharp)DfsMalloc((dfUnicodeStrlen((dfwcharp)buf)+1)*2);
            DfsMemcpy(lpBuffer,buf,(dfUnicodeStrlen((dfwcharp)buf)+1)*2);
            LocalFree(buf);
        }
        else
        {
            LPSTR lpBufferAnsi=NULL;

            dwNbChar=FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_ALLOCATE_BUFFER,NULL,dwErr,0,
                (LPSTR)(&lpBufferAnsi),MAX_PATH,NULL) ;
            if ((dwNbChar>0) && (lpBufferAnsi!=NULL))
            {
            lpBuffer=(LPWSTR)LocalAlloc(LMEM_ZEROINIT,(lstrlenA((LPSTR)lpBufferAnsi)*4)+0x10);
            ConvertAnsiToUnicode((LPCSTR)(lpBufferAnsi),(LPWSTR)(lpBuffer),(dwNbChar+1)*2);
            LocalFree(lpBufferAnsi);
            }
        }
#endif

        if (lpBuffer!=NULL)
        {
            peii->dfInfoTag = DFS_ERRORTAG_ERRORMSG;
            peii->pInfo = (dfbytep)(lpBuffer);
            peii->dfSize = (dfUnicodeStrlen((dfwcharp)lpBuffer)+1)*2;
            return TRUE;
        }
    }
    return FALSE;
}


BOOL SVFAPI FillWinErrorFileName(dfwcharpc dfFileName,dfuLong32 dwErr,H_ERROR_INFO * pei)
{
    ERROR_INFO_ITEM eii[3];
    dfuLong32 dfNbErrorItem=0;
    BOOL fFirstWinError = FALSE;
    if (pei == NULL)
        return FALSE;
    if ((*pei) != NULL)
        return FALSE;
    // now create error

    if (dwErr != NO_ERROR)
    {
        fFirstWinError = BuildErrorInfoWinError(dwErr,&eii[dfNbErrorItem]);
        if (fFirstWinError)
                dfNbErrorItem++;
    }

    if (dfFileName!=NULL)
    {
        {
            eii[dfNbErrorItem].dfInfoTag = DFS_ERRORTAG_FILENAME;
            eii[dfNbErrorItem].pInfo = (dfbytep)dfFileName;
            eii[dfNbErrorItem].dfSize = (dfUnicodeStrlen(dfFileName)+1)*2;
            dfNbErrorItem++;
        }
    }
    {
        eii[dfNbErrorItem].dfInfoTag = DFS_ERRORTAG_WIN32NUMBER;
        eii[dfNbErrorItem].pInfo = (dfbytep)&dwErr;
        eii[dfNbErrorItem].dfSize = sizeof(DWORD);
        dfNbErrorItem++;
    }


    *pei = CreateErrorInfoBlock(DFS_ERROR_ERRORIO,dwErr,dfNbErrorItem,eii);
    if (fFirstWinError)
        if (eii[0].pInfo != NULL)
            DfsFree(eii[0].pInfo);
    return ((*pei)!=NULL);
}

/***************************************************************************/




BOOL FillWinError(LOWLEVELFILE llf,DWORD dwErr,H_ERROR_INFO * pei)
{
    dfwcharp dfFileName=NULL;
    if (pei == NULL)
        return FALSE;
    if ((*pei) != NULL)
        return FALSE;
    // now create error


    if (llf!=NULL)
    {
        LOWLEVELINTERNAL* pLowLevelIntern = (LOWLEVELINTERNAL*)llf;
        dfFileName = pLowLevelIntern->dfFileName;
    }
    return FillWinErrorFileName(dfFileName,dwErr,pei);
}

static BOOL MySetFilePointerEx(HANDLE hFile, LARGE_INTEGER pos,PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod)
{
#ifdef DIFSTRM_USING_WINRT_API
    return SetFilePointerEx(hFile, pos, lpNewFilePointer, dwMoveMethod);
#else
    DWORD PosLow, PosHigh;
    DWORD dwRet;
    BOOL fRet = TRUE;
    PosLow = (DWORD)pos.LowPart;
    PosHigh = (DWORD)pos.HighPart;
    dwRet = SetFilePointer(hFile, PosLow, (PLONG)&PosHigh, dwMoveMethod);
    if ((dwRet == INVALID_SET_FILE_POINTER) && (GetLastError() != NO_ERROR))
        fRet = FALSE;
    if (fRet && (lpNewFilePointer != NULL))
    {
        lpNewFilePointer->LowPart = dwRet;
        lpNewFilePointer->HighPart = PosHigh;
    }
    return fRet;
#endif
}

#ifdef DIFSTRM_USING_WINRT_API
#else
BOOL SVFAPI GetCanonicalFileName(dfwcharpc FileName,
                          dfwcharp szBuffer, dfuLong32 dfBufSizeInWChar,
                          dfuLong32* pdfBufSizeNeededInWChar)
{
    if (IsUnicodeApiSupported())
    {
        dfuLong32 dfRet;
        dfwcharp lpFilePart=NULL;
        dfRet =  GetFullPathNameW(FileName,  dfBufSizeInWChar, szBuffer, &lpFilePart);
        if (pdfBufSizeNeededInWChar!=NULL)
            *pdfBufSizeNeededInWChar = dfRet;
        return (dfRet < dfBufSizeInWChar);
    }
    else
    {
        char* pBufAlloc;
        char* pBuf1;
        char* pBuf2;
        dfuLong32 dfLenOrg ;
        dfuLong32 dfPosBuf2;
        dfuLong32 dfRet;
        char* lpFilePart=NULL;
        BOOL fRet=FALSE;

        dfLenOrg = dfUnicodeStrlen(FileName);
        dfPosBuf2 = ((dfLenOrg + 0x20)/2)*4;

        pBufAlloc = DfsMalloc(dfPosBuf2 + (dfBufSizeInWChar*2) + 0x10);
        if (pBufAlloc == NULL)
        {
            if (pdfBufSizeNeededInWChar!=NULL)
                *pdfBufSizeNeededInWChar = 0;
            return FALSE;
        }
        pBuf1 = pBufAlloc;
        pBuf2 = pBufAlloc + dfPosBuf2;

        ConvertUnicodeToAnsi(FileName, pBuf1, dfPosBuf2 - 1);
        dfRet =  GetFullPathNameA(pBuf1,  dfBufSizeInWChar, pBuf2, &lpFilePart);

        if (pdfBufSizeNeededInWChar!=NULL)
            *pdfBufSizeNeededInWChar = dfRet;

        if (dfRet < dfBufSizeInWChar)
        {
            fRet = ConvertAnsiToUnicode(pBuf2,szBuffer,dfBufSizeInWChar);
        }

        DfsFree(pBufAlloc);
        return fRet;
    }
}
#endif

/*
DECLARE_DFHANDLE(HFILESEARCHING);

typedef struct
{
    dfwcharpc FileName;
    dfuLong64 FileSize;
} FILESEARCHITEMFOUND;*/

typedef struct
{
    HANDLE hFind;
    FILESEARCHITEMFOUND FileSearchCurrentItem;
    WIN32_FIND_DATAW FileData;
    BOOL fFirstReturned;
} FILESEARCHINGINTERNAL;

HFILESEARCHING SVFAPI InitFileSearching(dfwcharpc szWildcardSearch)
{
    FILESEARCHINGINTERNAL* pfsi;
    pfsi = (FILESEARCHINGINTERNAL*)DfsMalloc(sizeof(FILESEARCHINGINTERNAL));
    if (pfsi == NULL)
        return NULL;

#ifdef DIFSTRM_USING_WINRT_API
    pfsi->hFind = FindFirstFileExW((LPCWSTR)szWildcardSearch, FindExInfoBasic, &(pfsi->FileData),FindExSearchNameMatch,NULL,0);
#else
    pfsi->hFind = FindFirstFileW((LPCWSTR)szWildcardSearch, &(pfsi->FileData));
#endif
    if ((pfsi->hFind == NULL) || (pfsi->hFind == INVALID_HANDLE_VALUE))
    {
        DfsFree(pfsi);
        return NULL;
    }
    pfsi->fFirstReturned=FALSE;
    return (HFILESEARCHING)pfsi;
}

FILESEARCHITEMFOUND* SVFAPI GetNextItemContent(HFILESEARCHING hfsi)
{
    FILESEARCHINGINTERNAL* pfsi = (FILESEARCHINGINTERNAL*)hfsi;
    BOOL fFound = TRUE;
    if (pfsi->fFirstReturned != FALSE)
    {
        fFound = FindNextFileW(pfsi->hFind, &pfsi->FileData);
    }
    else
        pfsi->fFirstReturned = TRUE;



    if (fFound)
    {
        pfsi->FileSearchCurrentItem.fIsDirectory = ((pfsi->FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
        pfsi->FileSearchCurrentItem.FileName = (dfwcharpc)pfsi->FileData.cFileName;
        pfsi->FileSearchCurrentItem.FileSize = pfsi->FileData.nFileSizeLow | (((dfuLong64)pfsi->FileData.nFileSizeHigh) << 32);
        return &(pfsi->FileSearchCurrentItem);
    }
    else
        return NULL;

}

BOOL SVFAPI CloseFileSearching(HFILESEARCHING hfsi)
{
    FILESEARCHINGINTERNAL* pfsi = (FILESEARCHINGINTERNAL*)hfsi;
    if (pfsi == NULL)
        return FALSE;
    FindClose(pfsi->hFind);
    DfsFree(pfsi);
    return TRUE;
}


#ifdef DIFSTRM_USING_WINRT_API
#else
BOOL SVFAPI SvfGetFullPathName(dfwcharpc lpFileName,dfuLong32 nBufferLength,dfwcharp lpBuffer,dfwcharp *lpFilePart)
{
    return GetFullPathNameW(lpFileName,nBufferLength,lpBuffer,lpFilePart);
}

BOOL SVFAPI SvfGetTempFileName(dfwcharpc lpPathName,dfwcharpc lpPrefixString,dfuLong32 uUnique,dfwcharp lpTempFileName)
{
    return GetTempFileNameW(lpPathName,lpPrefixString,uUnique,lpTempFileName);
}
#endif

/*
BOOL SVFAPI CreateTemporaryFileNameInSameDirectoryCdLine(LPCTSTR lpszOriginalFile,LPTSTR lpszTempFileName,UINT uiSizeBuffer)
{
    TCHAR szDirectory[MAX_PATH+8];
    TCHAR szTempFileName[MAX_PATH+8];
    LPTSTR lpszFilePart=NULL;
    GetFullPathName(lpszOriginalFile,MAX_PATH,szDirectory,&lpszFilePart);
    *lpszFilePart=0;
    GetTempFileName(szDirectory,"SVF",0,szTempFileName);
    if (((unsigned)lstrlen(szTempFileName))>=uiSizeBuffer)
        return FALSE;
    lstrcpy(lpszTempFileName,szTempFileName);
    return TRUE;
}
*/

BOOL SVFAPI SvfMoveFile(dfwcharpc szTempFileName,dfwcharpc lpszCopyFileName)
{
    BOOL fRenamed;
    DWORD dwErr = 0;


    if (IsFileNameForMemorySpace(szTempFileName) || (IsFileNameForMemorySpace(lpszCopyFileName)))
    {
        return myRenameFile(szTempFileName, lpszCopyFileName,NULL);
    }

#ifdef DIFSTRM_USING_WINRT_API
    fRenamed = MoveFileExW(szTempFileName, lpszCopyFileName,
        MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING);
#else
    if (IsUnicodeApiSupported())
    {
        fRenamed = MoveFileExW(szTempFileName, lpszCopyFileName,
            MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING);
    }
    else
    {
        char szAnsiFileNameOld[512 + MAX_PATH];
        char szAnsiFileNameNew[512 + MAX_PATH];

        ConvertUnicodeToAnsi(szTempFileName, szAnsiFileNameOld,
            sizeof(szAnsiFileNameOld) - 1);

        ConvertUnicodeToAnsi(lpszCopyFileName, szAnsiFileNameNew,
            sizeof(szAnsiFileNameNew) - 1);

        fRenamed = MoveFileExA(szAnsiFileNameOld, szAnsiFileNameNew,
            MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING);
    }
#endif
    if (!fRenamed)
        dwErr=GetLastError();
    /*
    if (dwErr == ERROR_CALL_NOT_IMPLEMENTED)
    {
        dfwchar szTempFileNameBis[MAX_MATH+0x200+8];
        CreateTemporaryFileNameInSameDirectory(lpszCopyFileName,szTempFileNameBis,MAX_MATH+0x200);
        DeleteFileW(szTempFileName);
        if (MoveFileW(lpszCopyFileName,szTempFileNameBis))
            if (MoveFileW(szTempFileName,lpszCopyFileName))
            {
                fRenamed=TRUE;
                DeleteFileW(szTempFileNameBis);
            }
    }*/
    return fRenamed;
}

LOWLEVELFILE diskOpenLowLevel(dfwcharpc FileName, TYPEOPEN TypeOpen,
                          BOOL fTemporaryFile, BOOL fProjectedSize, dfuLong64 dfProjectedSize,
                          H_ERROR_INFO * pei)
{
  LOWLEVELFILE llfRet = NULL;
  HANDLE hFile;

  DWORD dwDesiredAccess = 0;    // access mode
  DWORD dwShareMode = 0;
  DWORD dwCreationDisposition = 0;      // how to create
  DWORD dwFlagsAndAttributes = 0;

  switch (TypeOpen)
  {
  case OPEN_READ:
    dwDesiredAccess = GENERIC_READ;
    dwCreationDisposition = OPEN_EXISTING;
    dwShareMode = FILE_SHARE_READ;
    break;
  case OPEN_READWRITE:
    dwDesiredAccess = GENERIC_WRITE | GENERIC_READ;
    dwCreationDisposition = OPEN_EXISTING;
    break;
  case OPEN_CREATE:
    dwDesiredAccess = GENERIC_WRITE | GENERIC_READ;
    dwCreationDisposition = CREATE_ALWAYS;
    break;
  }

  if (fTemporaryFile)
      dwFlagsAndAttributes |= FILE_ATTRIBUTE_TEMPORARY;
  hFile = MyCreateFileW(FileName, dwDesiredAccess, dwShareMode, NULL,
                        dwCreationDisposition, dwFlagsAndAttributes, NULL);

  if (hFile == INVALID_HANDLE_VALUE)
    hFile = NULL;

  if (hFile != NULL)
  {
      LOWLEVELINTERNAL* pLowLevelIntern=(LOWLEVELINTERNAL*)DfsMalloc(sizeof(LOWLEVELINTERNAL));
      if (pLowLevelIntern == NULL)
      {
          CloseHandle(hFile);
          hFile=NULL;
      }
      else
      {
          pLowLevelIntern->u.hlFile = hFile;
          pLowLevelIntern->dfFileName = dfUnicodeCopyAlloc(FileName);

          pLowLevelIntern->pvfio=NULL;

          llfRet = (LOWLEVELFILE) pLowLevelIntern;
      }
  }
  else
  {
      DWORD dwErr = GetLastError();
      BOOL fFirstWinError=FALSE;


      if (pei != NULL)
          if ((*pei)==NULL)
              FillWinErrorFileName(FileName,dwErr,pei);

  }

  return llfRet;
}

dfuLong32 diskLowLevelWrite(LOWLEVELFILE llFile, void const *Buf, dfuLong32 size, H_ERROR_INFO * pei)
{
  dfuLong32 dfRet = 0;
  LOWLEVELINTERNAL* pLowLevelIntern = (LOWLEVELINTERNAL*)llFile;
  HANDLE hFile ;
  BOOL fRet;

  if (pLowLevelIntern == NULL)
      return 0;
  hFile = (HANDLE)pLowLevelIntern->u.hlFile;
  if (hFile == NULL)
    return 0;

  fRet = WriteFile(hFile, Buf, size, &dfRet, NULL);
  if ((!fRet) || (dfRet != size))
  {
      DWORD dwErr = GetLastError();
      FillWinError(llFile,dwErr,pei);
  }
  return dfRet;
}

BOOL diskLowLevelSetFileSize(LOWLEVELFILE llFile, dfuLong32 PosLow, dfuLong32 PosHigh, H_ERROR_INFO * pei)
{
  LOWLEVELINTERNAL* pLowLevelIntern = (LOWLEVELINTERNAL*)llFile;
  HANDLE hFile ;
  LARGE_INTEGER pos;

  if (pLowLevelIntern == NULL)
      return FALSE;
  hFile = (HANDLE)pLowLevelIntern->u.hlFile;



  pos.LowPart = PosLow;
  pos.HighPart = PosHigh;
  if (!MySetFilePointerEx(hFile, pos, NULL, FILE_BEGIN))
  {
      DWORD dwErr = GetLastError();
      if (dwErr != NO_ERROR)
      {
          FillWinError(llFile,dwErr,pei);
          return FALSE;
      }
  }

  return SetEndOfFile(hFile);
}

dfuLong32 diskLowLevelRead(LOWLEVELFILE llFile, void *Buf, dfuLong32 size, H_ERROR_INFO* pei)
{
  dfuLong32 dfRet = 0;
  LOWLEVELINTERNAL* pLowLevelIntern = (LOWLEVELINTERNAL*)llFile;
  HANDLE hFile ;
  BOOL fRet;
  if (pLowLevelIntern == NULL)
      return 0;
  hFile = (HANDLE)pLowLevelIntern->u.hlFile;
  if (hFile == NULL)
    return 0;

  fRet=ReadFile(hFile, Buf, size, &dfRet, NULL);
  if ((!fRet) || (dfRet != size))
  {
      DWORD dwErr = GetLastError();
      if ((!fRet) || (dwErr != NO_ERROR))
        FillWinError(llFile,dwErr,pei);
  }

  return dfRet;
}


BOOL diskLowLevelSeek(LOWLEVELFILE llFile, dfuLong32 PosLow, dfuLong32 PosHigh, TYPESEEK TypeSeek, H_ERROR_INFO* pei)
{
  dfuLong32 dfRet;
  LOWLEVELINTERNAL* pLowLevelIntern = (LOWLEVELINTERNAL*)llFile;
  HANDLE hFile ;
  LARGE_INTEGER pos;


  if (pLowLevelIntern == NULL)
      return FALSE;
  hFile = (HANDLE)pLowLevelIntern->u.hlFile;
  if (hFile == NULL)
    return 0;

  pos.LowPart = PosLow;
  pos.HighPart = PosHigh;
  dfRet =
      MySetFilePointerEx(hFile, pos, NULL,
                   (TypeSeek == TYPESEEK_END) ? FILE_END : FILE_BEGIN) ? 0 : INVALID_SET_FILE_POINTER;

  if (dfRet == INVALID_SET_FILE_POINTER)
  {
      DWORD dwErr = GetLastError();
      if (dwErr != NO_ERROR)
      {
          FillWinError(llFile,dwErr,pei);
          return FALSE;
      }
  }
  return TRUE;
}


void diskLowLevelTell(LOWLEVELFILE llFile, dfuLong32 *PosLow, dfuLong32 *PosHigh)
{
  LOWLEVELINTERNAL* pLowLevelIntern = (LOWLEVELINTERNAL*)llFile;
  HANDLE hFile ;


  LARGE_INTEGER pos;

  *PosHigh = 0;
  *PosLow = 0;
  pos.QuadPart = 0;

  if (pLowLevelIntern == NULL)
      return ;
  hFile = (HANDLE)pLowLevelIntern->u.hlFile;

  if (hFile == NULL)
    return ;

  MySetFilePointerEx(hFile, pos, &pos, FILE_CURRENT);
  *PosLow = pos.LowPart;
  *PosHigh = pos.HighPart;
}


static BOOL diskChangeDateTimeFileHandle(HANDLE hFile, const DFSTM* pDfsTm)
{
    FILETIME ft, ftCreate, ftLastAcc, ftLastWrite;
    SYSTEMTIME SystemTime;
#ifdef DIFSTRM_USING_WINRT_API
    SYSTEMTIME SystemTimeLocal;
    FILE_BASIC_INFO fbi;
#else
    FILETIME ftLocal;
#endif
    BOOL res;

#ifdef DIFSTRM_USING_WINRT_API
  if (GetFileInformationByHandleEx(hFile,FileBasicInfo,&fbi,sizeof(fbi)))
  {
      ftCreate.dwHighDateTime = fbi.CreationTime.HighPart;
      ftCreate.dwLowDateTime = fbi.CreationTime.LowPart;

      ftLastAcc.dwHighDateTime = fbi.LastAccessTime.HighPart;
      ftLastAcc.dwLowDateTime = fbi.LastAccessTime.LowPart;

      ftLastWrite.dwHighDateTime = fbi.LastWriteTime.HighPart;
      ftLastWrite.dwLowDateTime = fbi.LastWriteTime.LowPart;
  }
#else
  res=GetFileTime(hFile, &ftCreate, &ftLastAcc, &ftLastWrite);
  if (!res) return FALSE;
#endif

  SystemTime.wMilliseconds = (WORD) pDfsTm->df_msec;    /* milisecond [0..999] 10 bits */
  SystemTime.wSecond = (WORD) pDfsTm->df_sec;   /* seconds after the minute - [0..59]  6 bits */
  SystemTime.wMinute = (WORD) pDfsTm->df_min;   /* minutes after the hour - [0..59] 6 bits */
  SystemTime.wHour = (WORD) pDfsTm->df_hour;    /* hours since midnight - [0..23]  5 bits */
  SystemTime.wDayOfWeek = (WORD) 0;
  SystemTime.wDay = (WORD) pDfsTm->df_mday;     /* day of the month - [1..31] 5 bits */
  SystemTime.wMonth = (WORD) pDfsTm->df_mon;    /* months since January - [1..12] 4 bits */
  SystemTime.wYear = (WORD) pDfsTm->df_year;    /* years - [0..4095] 12 bits */


#ifdef DIFSTRM_USING_WINRT_API
  SystemTimeToTzSpecificLocalTime(NULL,&SystemTime,&SystemTimeLocal);
  SystemTimeToFileTime(&SystemTimeLocal, &ft);
  fbi.CreationTime.HighPart = fbi.LastWriteTime.HighPart = ft.dwHighDateTime;
  fbi.CreationTime.LowPart = fbi.LastWriteTime.LowPart = ft.dwLowDateTime;
  GetFileInformationByHandleEx(hFile,FileBasicInfo,&fbi,sizeof(fbi));
#else
  SystemTimeToFileTime(&SystemTime, &ftLocal);
  LocalFileTimeToFileTime(&ftLocal, &ft);
  res=SetFileTime(hFile, &ft, &ftLastAcc, &ft);
#endif
  return res;
}

BOOL diskLowLevelCloseChangeDateTime(LOWLEVELFILE llFile, BOOL fFlushWrite, const DFSTM* pDfsTm, BOOL* pDateTimeChanged, H_ERROR_INFO* pei)
{
  BOOL fRet;
  LOWLEVELINTERNAL* pLowLevelIntern = (LOWLEVELINTERNAL*)llFile;
  HANDLE hFile ;

  if (pDateTimeChanged != NULL)
      *pDateTimeChanged = FALSE;

  if (pLowLevelIntern == NULL)
      return FALSE;
  hFile = (HANDLE)pLowLevelIntern->u.hlFile;
  if (hFile == NULL)
    return FALSE;

  if (fFlushWrite)
      FlushFileBuffers (hFile);

  if (pDfsTm != NULL)
  {
      BOOL fresDate = diskChangeDateTimeFileHandle(hFile,pDfsTm);
      if (pDateTimeChanged != NULL)
          *pDateTimeChanged = fresDate;
  }

  fRet = CloseHandle(hFile);

  if (!fRet)
  {
      DWORD dwErr = GetLastError();
      if (dwErr != NO_ERROR)
        FillWinError(llFile,dwErr,pei);
  }

  if (pLowLevelIntern->dfFileName!=NULL)
    DfsFree(pLowLevelIntern->dfFileName);
  DfsFree(pLowLevelIntern);
  return fRet;
}

BOOL diskLowLevelClose(LOWLEVELFILE llFile, BOOL fFlushWrite, H_ERROR_INFO* pei)
{
    return diskLowLevelCloseChangeDateTime(llFile, fFlushWrite, NULL, NULL, pei);
}

void diskLowLevelGetSize(LOWLEVELFILE llFile, dfuLong32 *PosLow, dfuLong32 *PosHigh)
{
  LOWLEVELINTERNAL* pLowLevelIntern = (LOWLEVELINTERNAL*)llFile;

  HANDLE hFile ;

#ifdef DIFSTRM_USING_WINRT_API
    FILE_STANDARD_INFO fsi;
#endif

  if (pLowLevelIntern == NULL)
      return ;
  hFile = (HANDLE)pLowLevelIntern->u.hlFile;

  *PosLow = *PosHigh = 0;

#ifdef DIFSTRM_USING_WINRT_API
    if (GetFileInformationByHandleEx(hFile,FileStandardInfo,&fsi,sizeof(fsi)))
    {
        *PosLow = fsi.EndOfFile.LowPart;
        *PosHigh = fsi.EndOfFile.HighPart;
    }
#else
  if (hFile != NULL)
      *PosLow =  GetFileSize(hFile,PosHigh);
#endif
}



BOOL SVFAPI GetTemporaryFilename(dfwcharpc lpTreeLetter,dfwcharp lpBuffer, dfuLong32 dfSizeBufferinwChar,BOOL fTempMemPossible,dfuLong64 dfFileSizeProjected)
{
    BOOL fRet=TRUE;

    if (fTempMemPossible)
      if (CheckIfVirtualFilePossible(dfFileSizeProjected))
        return GetTempMemoryTmpFileName(lpBuffer, dfSizeBufferinwChar,dfFileSizeProjected);


#ifdef DIFSTRM_USING_WINRT_API
    {
        return GetTempFileNameWinRT(dfSizeBufferinwChar,(LPWSTR)lpBuffer);
    }
        //GetTempPathWinRT(MAX_PATH,(LPWSTR)szTempDirectoryWindows);
#else
    if (IsUnicodeApiSupported())
    {
        WCHAR szDirectory[MAX_PATH+1];
        //GetTempPathW(MAX_PATH,szDirectory);
        GetTempDirectorySystem(szDirectory,MAX_PATH);
        GetTempFileNameW(szDirectory, lpTreeLetter, 0, lpBuffer);
    }
    else
    {
        char szAnsiTempFN[MAX_PATH];
        char szDirectoryA[MAX_PATH+1];
        WCHAR szDirectoryW[MAX_PATH+1];
        char szAnsiTreeLetter[MAX_PATH+1];
        //GetTempPathA(MAX_PATH,szDirectory);
        GetTempDirectorySystem(szDirectoryW,MAX_PATH);
        ConvertUnicodeToAnsi(szDirectoryW,szDirectoryA,MAX_PATH);

        ConvertUnicodeToAnsi(lpTreeLetter,szAnsiTreeLetter,MAX_PATH);

        GetTempFileNameA(szDirectoryA, szAnsiTreeLetter, 0, szAnsiTempFN);
        ConvertAnsiToUnicode(szAnsiTempFN, lpBuffer,
                            dfSizeBufferinwChar);
    }
#endif
    return fRet;
}


// was BOOL SVFAPI ChangeDateTimeFile(dfwcharpc FileName, const DFSTM * pDfsTm)
BOOL diskChangeDateTimeFile(dfwcharpc FileName, const DFSTM * pDfsTm)
{
  HANDLE hFile;
  BOOL res;


  if (IsFileNameForMemorySpace(FileName))
      return TRUE;

  hFile = MyCreateFileW(FileName, GENERIC_READ | GENERIC_WRITE,
                        0, NULL, OPEN_EXISTING, 0, NULL);
  if ((hFile == INVALID_HANDLE_VALUE) || (hFile == NULL))
    return FALSE;
  // Win32: API SetDateAndTimeFile
  res=diskChangeDateTimeFileHandle(hFile, pDfsTm);

  CloseHandle(hFile);
  return res;
}


dfuLong64 diskGetFileSizeByName(dfwcharpc FileName, DFSTM * pDfsTm, H_ERROR_INFO* pei)
{
  //HANDLE hFind;
  dfuLong64 dfRet = FILE_SIZE_NOT_EXIST;
  FILETIME ft;
  BOOL fFoundTime = FALSE;
  HANDLE hf;

  hf = MyCreateFileW(FileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                  OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);

  if ((hf!=NULL) && (hf!=INVALID_HANDLE_VALUE))
  {
      DWORD dwError = 0;
#ifdef DIFSTRM_USING_WINRT_API
      DWORD dwHigh = 0;
      DWORD dwLow = 0;
      FILE_STANDARD_INFO fsi;
      FILE_BASIC_INFO fbi;
      if (GetFileInformationByHandleEx(hf,FileStandardInfo,&fsi,sizeof(fsi)))
      {
          dfRet = fsi.EndOfFile.QuadPart;
      }

      if (pDfsTm != NULL)
        {
            if (GetFileInformationByHandleEx(hf,FileBasicInfo,&fbi,sizeof(fbi)))
            {
                LARGE_INTEGER tm = fbi.LastWriteTime;
                ft.dwLowDateTime = tm.LowPart;
                ft.dwHighDateTime = tm.HighPart;
                fFoundTime = TRUE;
            }
      }
#else
      DWORD dwHigh=0;
      DWORD dwLow = GetFileSize(hf,&dwHigh);
      if (dwLow == INVALID_FILE_SIZE)
          dwError = GetLastError();

      if (dwError == NO_ERROR)
      {
        dfRet = dwLow | (((dfuLong64)dwHigh)<<32);

        if (pDfsTm != NULL)
        {
            BY_HANDLE_FILE_INFORMATION bhfi;
            if (GetFileInformationByHandle(hf,&bhfi))
            {
                ft = bhfi.ftLastWriteTime;
                fFoundTime = TRUE;
            }
        }
      }
#endif
      CloseHandle(hf);
  }
  else
  {
      DWORD dwErr=GetLastError();
      #if defined(_DEBUG) && defined(TCHAR)
      WCHAR szErrorMsg[MAX_PATH+1024];
      wsprintfW(szErrorMsg,L"error %u in opening %ws\n",dwErr,FileName);
      OutputDebugStringW(szErrorMsg);
      #endif
      if (dwErr != NO_ERROR)
            FillWinErrorFileName(FileName,dwErr,pei);
  }


  if (fFoundTime)
  {
    SYSTEMTIME SystemTime;
    LONG bias;
    DWORD retval;
    TIME_ZONE_INFORMATION tzinfo;

#ifdef DIFSTRM_USING_WINRT_API
    SYSTEMTIME SystemTime1;
    FileTimeToSystemTime(&ft, &SystemTime1);
    SystemTimeToTzSpecificLocalTime(NULL,&SystemTime1,&SystemTime);
#else
    FILETIME ftLocal;
    FileTimeToLocalFileTime(&ft, &ftLocal);
    FileTimeToSystemTime(&ftLocal, &SystemTime);
#endif
    pDfsTm->df_msec = SystemTime.wMilliseconds; /* milisecond [0..999] 10 bits */
    pDfsTm->df_sec = SystemTime.wSecond;        /* seconds after the minute - [0..59]  6 bits */
    pDfsTm->df_min = SystemTime.wMinute;        /* minutes after the hour - [0..59] 6 bits */
    pDfsTm->df_hour = SystemTime.wHour; /* hours since midnight - [0..23]  5 bits */
    pDfsTm->df_mday = SystemTime.wDay;  /* day of the month - [1..31] 5 bits */
    pDfsTm->df_mon = SystemTime.wMonth; /* months since January - [1..12] 4 bits */
    pDfsTm->df_year = SystemTime.wYear; /* years - [0..4095] 12 bits */

    retval = GetTimeZoneInformation( &tzinfo );
    bias = tzinfo.Bias ;

    if ( retval == TIME_ZONE_ID_STANDARD )
        bias += tzinfo.StandardBias;

    if ( retval == TIME_ZONE_ID_DAYLIGHT )
        bias += tzinfo.DaylightBias;
    pDfsTm->df_timezone_bias = ((bias / 15) + 128); /* bias [0..255] 7 bits */

    /*      SystemTime.wDayOfWeek; unused */
  }

  return dfRet;
}


////////////////////////////////////////////////////////++++++++//////////////
////////////////////////////////////////////////////////++++++++//////////////
////////////////////////////////////////////////////////++++++++//////////////

// Windows Phone 8.1 DON'T HAVE File map!!!
#ifdef PREVENT_USING_WINDOWS_FILEMAP


static BOOL AdaptBufferForManualMapSize(FILECONTENTREADBUFINTERNAL* pFileContentReadBufInternal, dfuIntPtr bufSize)
{
    if (pFileContentReadBufInternal->dfGranularityMapping == 0)
        pFileContentReadBufInternal->dfGranularityMapping = DEFAULT_GRANULARITY_MAPPING;
    if (pFileContentReadBufInternal->no_map.nbDirtyUnitPerGranul == 0)
        pFileContentReadBufInternal->no_map.nbDirtyUnitPerGranul = DEFAULT_DIRTY_UNIT_PER_GRANUL;
    //dfuIntPtr nbGranularityDirty = (bufSize / pFileContentReadBufInternal->dfGranularityMapping) * (pFileContentReadBufInternal->nbDirtyUnitPerGranul);

    dfuIntPtr dfDirtyUnitSize = pFileContentReadBufInternal->dfGranularityMapping / pFileContentReadBufInternal->no_map.nbDirtyUnitPerGranul;
    dfuIntPtr nbDirtyMapItem = (dfuIntPtr)MyAroundUpper(bufSize, dfDirtyUnitSize) / dfDirtyUnitSize;

    if ((pFileContentReadBufInternal->no_map.allocatedInternalBufferSize >= bufSize) && (pFileContentReadBufInternal->no_map.allocatedInternalBufferDirtySize >= nbDirtyMapItem))
        return TRUE;

    if (pFileContentReadBufInternal->no_map.allocatedInternalBufferSize < bufSize)
    {
        void*new_buffer;
        dfuIntPtr dfAllocNeededSize = pFileContentReadBufInternal->no_map.allocatedInternalBufferSize;
        if (dfAllocNeededSize < DEFAULT_GRANULARITY_MAPPING)
            dfAllocNeededSize = DEFAULT_GRANULARITY_MAPPING;
        while (dfAllocNeededSize < bufSize)
            dfAllocNeededSize *= 2;
        new_buffer = DfsRealloc(pFileContentReadBufInternal->no_map.allocatedInternalBufferData, dfAllocNeededSize);
        if (new_buffer == NULL)
            return FALSE;
        pFileContentReadBufInternal->no_map.allocatedInternalBufferData = new_buffer;
        pFileContentReadBufInternal->no_map.allocatedInternalBufferSize = dfAllocNeededSize;
    }

    if (pFileContentReadBufInternal->no_map.allocatedInternalBufferDirtySize < nbDirtyMapItem)
    {
        void*new_buffer;
        dfuIntPtr dfAllocNeededSize = pFileContentReadBufInternal->no_map.allocatedInternalBufferDirtySize;
        if (dfAllocNeededSize < 0x10)
            dfAllocNeededSize = 0x10;
        while (dfAllocNeededSize < nbDirtyMapItem)
            dfAllocNeededSize *= 2;
        new_buffer = DfsRealloc(pFileContentReadBufInternal->no_map.allocatedInternalBufferDirty, dfAllocNeededSize);
        if (new_buffer == NULL)
            return FALSE;
        pFileContentReadBufInternal->no_map.allocatedInternalBufferDirty = new_buffer;
        memset(((unsigned char*)new_buffer) + pFileContentReadBufInternal->no_map.allocatedInternalBufferDirtySize, 0,
            dfAllocNeededSize - pFileContentReadBufInternal->no_map.allocatedInternalBufferDirtySize);
        pFileContentReadBufInternal->no_map.allocatedInternalBufferDirtySize = dfAllocNeededSize;
    }

    return TRUE;
}


typedef struct {
    const unsigned char*buf;
    dfuLong64 pos;
    dfuLong32 size;
} WRITE_DIRTY_OPERATION;


static BOOL PerformDirtyWriteOperation(HANDLE hFile, const WRITE_DIRTY_OPERATION *oper)
{
    BOOL fSetPosDone;
    BOOL fWriteDone = FALSE;
    LARGE_INTEGER pos;

    if (oper->size == 0)
        return TRUE;

    pos.QuadPart = oper->pos;
    fSetPosDone = MySetFilePointerEx(hFile, pos, NULL, FILE_BEGIN);
    if (fSetPosDone)
    {
        DWORD dfRet = 0;
        DWORD size = (DWORD)oper->size;


        fWriteDone = WriteFile(hFile, oper->buf, size, &dfRet, NULL);

        if (dfRet != size)
            fWriteDone = FALSE;
    }

    return fWriteDone;
}


static BOOL WriteDirtyData(FILECONTENTREADBUFINTERNAL* pFileContentReadBufInternal)
{
    BOOL fSuccess = TRUE;
    dfuIntPtr dfDirtyUnitSize = pFileContentReadBufInternal->dfGranularityMapping / pFileContentReadBufInternal->no_map.nbDirtyUnitPerGranul;
    dfuIntPtr nbMapItem = (dfuIntPtr)MyAroundUpper(pFileContentReadBufInternal->dfMapSize, dfDirtyUnitSize) / dfDirtyUnitSize;

    const unsigned char* walkBuf = (const unsigned char*)pFileContentReadBufInternal->no_map.allocatedInternalBufferData;
    dfuLong64 posEndMap = pFileContentReadBufInternal->dfBeginMapPos + pFileContentReadBufInternal->dfMapSize;
    WRITE_DIRTY_OPERATION oper;
    dfuLong64 pos_write = pFileContentReadBufInternal->dfBeginMapPos;
    oper.size = 0;
    for (dfuIntPtr i = 0; i < nbMapItem; i++)
    {
        char flag = (*(((char*)(pFileContentReadBufInternal->no_map.allocatedInternalBufferDirty)) + i));

        if (*(((char*)(pFileContentReadBufInternal->no_map.allocatedInternalBufferDirty)) + i))
        {
            BOOL fWriteDone = FALSE;
            DWORD size = (DWORD)dfDirtyUnitSize;
            if (((dfuLong64)(pos_write + size)) > posEndMap)
                size = (DWORD)(posEndMap - pos_write);

            if ((oper.size != 0) && ((oper.buf + oper.size) == walkBuf) && ((oper.pos + oper.size) == pos_write))
            {
                oper.size += size;
            }
            else
            {
                if (!PerformDirtyWriteOperation(pFileContentReadBufInternal->u.wh.hFile,&oper))
                    fSuccess = FALSE;
                oper.size = size;
                oper.buf = walkBuf;
                oper.pos = pos_write;
            }

            *(((char*)(pFileContentReadBufInternal->no_map.allocatedInternalBufferDirty)) + i) = 0;
        }
        pos_write += dfDirtyUnitSize;
        walkBuf += dfDirtyUnitSize;
    }

    if (!PerformDirtyWriteOperation(pFileContentReadBufInternal->u.wh.hFile, &oper))
        fSuccess = FALSE;

    return fSuccess;
}


static void fncSetDirtyMapNoMap(void* pOrgData_, dfuLong64 dfCurrentViewBegin, dfuLong64 size)
{
    ORIGDATA* pOrgData = (ORIGDATA*)pOrgData_;
    FILECONTENTREADBUFINTERNAL* pFileContentReadBufInternal = (FILECONTENTREADBUFINTERNAL*)pOrgData->pInternalfncAdaptData;
    dfuLong32 dfGranularity = pFileContentReadBufInternal->dfGranularityMapping;

    dfuLong64 dfBeginAround = MyAroundLower(dfCurrentViewBegin, dfGranularity);
    dfuLong64 dfEndAround = MyAroundUpper(dfCurrentViewBegin + size, dfGranularity);

    dfuIntPtr dfDirtyUnitSize = pFileContentReadBufInternal->dfGranularityMapping / pFileContentReadBufInternal->no_map.nbDirtyUnitPerGranul;
    dfuIntPtr nbMapItem = (dfuIntPtr)MyAroundUpper(pFileContentReadBufInternal->dfMapSize, dfDirtyUnitSize) / dfDirtyUnitSize;

    const unsigned char* walkBuf = (const unsigned char*)pFileContentReadBufInternal->lpBuf;
    dfuLong64 posEndMap = pFileContentReadBufInternal->dfBeginMapPos + pFileContentReadBufInternal->dfMapSize;
    dfuLong64 pos_begin_Current_block = pFileContentReadBufInternal->dfBeginMapPos;

    for (dfuIntPtr i = 0; i < nbMapItem; i++)
    {
        dfuLong64 pos_end_Current_block = pos_begin_Current_block + dfDirtyUnitSize;
        unsigned char must_set_dirty = 0;

        if ((dfBeginAround < pos_end_Current_block) && (dfEndAround > pos_begin_Current_block))
            must_set_dirty = 1;

        *(((char*)(pFileContentReadBufInternal->no_map.allocatedInternalBufferDirty)) + i) |= must_set_dirty;
        pos_begin_Current_block += dfDirtyUnitSize;
    }
}


static BOOL FreeManualMapBuffer(FILECONTENTREADBUFINTERNAL* pFileContentReadBufInternal)
{
    if (!WriteDirtyData(pFileContentReadBufInternal))
        return FALSE;

    if (pFileContentReadBufInternal->no_map.allocatedInternalBufferData != NULL)
        DfsFree(pFileContentReadBufInternal->no_map.allocatedInternalBufferData);
    pFileContentReadBufInternal->no_map.allocatedInternalBufferSize = 0;

    if (pFileContentReadBufInternal->no_map.allocatedInternalBufferDirty != NULL)
        DfsFree(pFileContentReadBufInternal->no_map.allocatedInternalBufferDirty);
    pFileContentReadBufInternal->no_map.allocatedInternalBufferDirtySize = 0;

    return TRUE;
}


dfvoidp fncDiskAdaptDataMapViewNoMap(void* pOrgData_, dfuLong64 dfCurrentViewBegin, dfuLong64 dfCurrentViewEndOrSize, BOOL fBySize, BOOL *pfDoneOk)
{
    ORIGDATA* pOrgData = (ORIGDATA*)pOrgData_;
    FILECONTENTREADBUFINTERNAL* pFileContentReadBufInternal = (FILECONTENTREADBUFINTERNAL*)pOrgData->pInternalfncAdaptData;

    dfuLong64 dfCurrentViewEnd;
    dfuLong64 dfCurrentViewBeginAround;
    dfuLong64 dfCurrentViewEndAround;
    dfuLong64 dfCurrentViewEndFixed;
    dfuLong32 dfGranularity = pFileContentReadBufInternal->dfGranularityMapping;


    LARGE_INTEGER pos;
    BOOL fSetPosDone;
    BOOL fReadDone=FALSE;

    if (fBySize)
        dfCurrentViewEnd = dfCurrentViewBegin + dfCurrentViewEndOrSize;
    else
        dfCurrentViewEnd = dfCurrentViewEndOrSize;

    dfCurrentViewBeginAround = MyAroundLower(dfCurrentViewBegin,dfGranularity) ;
    dfCurrentViewEndAround = MyAroundUpper(dfCurrentViewEnd , dfGranularity);
    dfCurrentViewEndFixed = (pOrgData->size < dfCurrentViewEndAround) ? pOrgData->size : dfCurrentViewEndAround;

    if ((dfCurrentViewEnd < dfCurrentViewBegin) ||
        (!WriteDirtyData(pFileContentReadBufInternal)) ||
        (!AdaptBufferForManualMapSize(pFileContentReadBufInternal, (dfuIntPtr)(dfCurrentViewEndAround - dfCurrentViewBeginAround))))
    {
        if (pfDoneOk != NULL)
            *pfDoneOk = FALSE;
        return NULL;
    }

    pos.QuadPart = dfCurrentViewBeginAround;

    fSetPosDone = MySetFilePointerEx(pFileContentReadBufInternal->u.wh.hFile, pos, NULL, FILE_BEGIN);

    if (fSetPosDone)
    {
        DWORD dfRet = 0;
        DWORD size = (DWORD)(dfCurrentViewEndFixed - dfCurrentViewBeginAround);

        fReadDone = ReadFile(pFileContentReadBufInternal->u.wh.hFile, pFileContentReadBufInternal->no_map.allocatedInternalBufferData, size, &dfRet, NULL);
        if ((dfRet != size) && (size!=0))
            fReadDone = FALSE;
    }

    if (fReadDone)
    {
        pOrgData->dfCurrentViewBegin = dfCurrentViewBeginAround;
        pOrgData->dfCurrentViewLimitEnd = dfCurrentViewEndFixed;
        pFileContentReadBufInternal->dfMapSize = (dfuIntPtr)(dfCurrentViewEndFixed - dfCurrentViewBeginAround);;
        pFileContentReadBufInternal->dfBeginMapPos = dfCurrentViewBeginAround;
    }

    if (pfDoneOk != NULL)
        *pfDoneOk = fReadDone;
    pOrgData->pCurrentView = ((dfbytep)(pFileContentReadBufInternal->no_map.allocatedInternalBufferData)) - dfCurrentViewBeginAround;


    return fReadDone ? (pOrgData->pCurrentView) : NULL;
}


dfuIntPtr CalculateMaxOrigDataExigibleSizeViewMask(dfuIntPtr dfMaxOrigDataExigibleSizeView);



BOOL diskGetFileFullContentBuffer(dfwcharpc FileName, dfuLong32 dfAccessFlag, dfuLong64 dfSizeRequested,//BOOL fReadOnly,
    HFILECONTENTREADBUF* phFileContentReadBuf,
    ORIGDATA* pOrg,
    H_ERROR_INFO* pei)
{
    FILECONTENTREADBUFINTERNAL FileContentReadBufInternal;
    DWORD dwErr = 0;
    dfuLong64 memAllocSize;
    DWORD dwReadSizeDone = 0;
    FILECONTENTREADBUFINTERNAL *pFileContentReadBufInternal = NULL;
    BOOL fReadOnly = (dfAccessFlag & FILEFULLCONTENTBUFFER_ACCESSMASK) == FILEFULLCONTENTBUFFER_READ;
    BOOL fWritePossible = (dfAccessFlag & FILEFULLCONTENTBUFFER_WRITE) != 0;
    if (phFileContentReadBuf == NULL)
        return FALSE;
    *phFileContentReadBuf = NULL;


    if (pOrg == NULL)
        return FALSE;

    FillOrigDataForFullMemoryOrg(pOrg, NULL, FILE_SIZE_NOT_EXIST);

    if (FileName == NULL)
        return FALSE;
    memset(&FileContentReadBufInternal, 0, sizeof(FILECONTENTREADBUFINTERNAL));
    FileContentReadBufInternal.fPartialMapping = FALSE;
    FileContentReadBufInternal.dfBeginMapPos = 0;
    FileContentReadBufInternal.dfMapSize = 0;
    FileContentReadBufInternal.dfGranularityMapping = 0;
    FileContentReadBufInternal.fWritePossible = fWritePossible;
    FileContentReadBufInternal.fProgressiveFlush = (dfAccessFlag & FILEFULLCONTENTBUFFER_PROGRESSIVE_FLUSH) != 0;

    FileContentReadBufInternal.dwSize = GetFileSizeByName(FileName, NULL, pei);
    if (FileContentReadBufInternal.dwSize == FILE_SIZE_NOT_EXIST)
        return FALSE;

    if (FileContentReadBufInternal.dwSize == 0)
    {
        (pOrg->size) = 0;
        return TRUE;
    }

    FileContentReadBufInternal.fVirtualMemFile = FALSE;
    FileContentReadBufInternal.u.wh.hFile =
        MyCreateFileW(FileName, GENERIC_READ | (fWritePossible ? GENERIC_WRITE : 0), fWritePossible ? 0 : FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);

    if (FileContentReadBufInternal.u.wh.hFile == NULL)
    {
        FillWinErrorFileName(FileName, dwErr, pei);

        return FALSE;
    }

    memAllocSize = FileContentReadBufInternal.dwSize;

    if ((dfAccessFlag & FILEFULLCONTENTBUFFER_RESIZE) != 0)
        memAllocSize = dfSizeRequested;
    if (memAllocSize == 0)
        memAllocSize = 1;

    if ((FileContentReadBufInternal.u.wh.hFile != NULL) && ((dfAccessFlag & FILEFULLCONTENTBUFFER_RESIZE) != 0))
    {
        BOOL fSetPositionOk;

        LARGE_INTEGER pos;
        pos.QuadPart = dfSizeRequested;
        fSetPositionOk = MySetFilePointerEx(FileContentReadBufInternal.u.wh.hFile, pos, NULL, FILE_BEGIN);

        if (!fSetPositionOk)
        {
            DWORD dwErr = GetLastError();
            if (dwErr != NO_ERROR)
            {
                FillWinErrorFileName(FileName, dwErr, pei);
                CloseHandle(FileContentReadBufInternal.u.wh.hFile);
                FileContentReadBufInternal.u.wh.hFile = NULL;
            }
        }
        if (SetEndOfFile(FileContentReadBufInternal.u.wh.hFile))
        {
            FileContentReadBufInternal.dwSize = dfSizeRequested;
        }
        else
        {
            FillWinErrorFileName(FileName, GetLastError(), pei);
            CloseHandle(FileContentReadBufInternal.u.wh.hFile);
            FileContentReadBufInternal.u.wh.hFile = NULL;
        }
    }

    pOrg->dfCurrentViewLimitEnd = 0;
    pOrg->dfCurrentViewBegin = 0;
    pOrg->fncAdaptDataMapView = fncDiskAdaptDataMapViewNoMap;
    pOrg->fncSetDirtyMapView = fncSetDirtyMapNoMap;
    pOrg->dfMaxOrigDataExigibleSizeView = (dfuIntPtr)(memAllocSize < MAX_MAP_VIEW_SIZE ? memAllocSize : MAX_MAP_VIEW_SIZE);
    pOrg->dfMaxOrigDataExigibleSizeViewMask = CalculateMaxOrigDataExigibleSizeViewMask(pOrg->dfMaxOrigDataExigibleSizeView);
    pOrg->size = FileContentReadBufInternal.dwSize;

    FileContentReadBufInternal.dfGranularityMapping = DEFAULT_GRANULARITY_MAPPING;
    FileContentReadBufInternal.no_map.nbDirtyUnitPerGranul = DEFAULT_DIRTY_UNIT_PER_GRANUL;

    pFileContentReadBufInternal =
        (FILECONTENTREADBUFINTERNAL *)
        DfsMalloc(sizeof(FILECONTENTREADBUFINTERNAL));
    if (pFileContentReadBufInternal == NULL)
    {
        FillWinErrorFileName(FileName, dwErr, pei);
        CloseHandle(pFileContentReadBufInternal->u.wh.hFile);
        DfsFree(FileContentReadBufInternal.lpBuf);
        return FALSE;
    }
    pOrg->pInternalfncAdaptData = (dfvoidp)pFileContentReadBufInternal;

    *pFileContentReadBufInternal = FileContentReadBufInternal;
    *phFileContentReadBuf = (HFILECONTENTREADBUF)pFileContentReadBufInternal;
    return TRUE;
}


BOOL diskCloseFileFullContentBuffer(FILECONTENTREADBUFINTERNAL *pFileContentReadBufInternal, BOOL fFlushWrite, H_ERROR_INFO* pei)
{
    BOOL fRet = TRUE;
    if (pFileContentReadBufInternal == NULL)
        return FALSE;

    fRet = WriteDirtyData(pFileContentReadBufInternal);
    if (!FreeManualMapBuffer(pFileContentReadBufInternal))
        fRet = FALSE;

    CloseHandle(pFileContentReadBufInternal->u.wh.hFile);
    DfsFree(pFileContentReadBufInternal->lpBuf);
    DfsFree(pFileContentReadBufInternal);
    return fRet;
}


////////////////////////////////////////////////////////++++++++//////////////
////////////////////////////////////////////////////////++++++++//////////////
#else
////////////////////////////////////////////////////////++++++++//////////////
////////////////////////////////////////////////////////++++++++//////////////

dfvoidp fncDiskAdaptDataMapView(void* pOrgData_,dfuLong64 dfCurrentViewBegin,dfuLong64 dfCurrentViewEndOrSize,BOOL fBySize,BOOL *pfDoneOk);
dfuIntPtr CalculateMaxOrigDataExigibleSizeViewMask(dfuIntPtr dfMaxOrigDataExigibleSizeView);


BOOL diskGetFileFullContentBuffer(dfwcharpc FileName,dfuLong32 dfAccessFlag,dfuLong64 dfSizeRequested,//BOOL fReadOnly,
                                  HFILECONTENTREADBUF* phFileContentReadBuf,
                                  ORIGDATA* pOrg,
                                  H_ERROR_INFO* pei)
{
  FILECONTENTREADBUFINTERNAL FileContentReadBufInternal;
  DWORD dwErr = 0;
  FILECONTENTREADBUFINTERNAL *pFileContentReadBufInternal = NULL;
  BOOL fReadOnly = (dfAccessFlag & FILEFULLCONTENTBUFFER_ACCESSMASK)==FILEFULLCONTENTBUFFER_READ;
  BOOL fWritePossible = (dfAccessFlag & FILEFULLCONTENTBUFFER_WRITE) != 0;
  if (phFileContentReadBuf==NULL)
      return FALSE;
  *phFileContentReadBuf = NULL;
  memset(&FileContentReadBufInternal, 0, sizeof(FILECONTENTREADBUFINTERNAL));

  if (pOrg == NULL)
      return FALSE;

  FillOrigDataForFullMemoryOrg(pOrg,NULL,FILE_SIZE_NOT_EXIST);

  if (FileName == NULL)
    return FALSE;

  FileContentReadBufInternal.fPartialMapping = FALSE;
  FileContentReadBufInternal.dfBeginMapPos = 0;
  FileContentReadBufInternal.dfMapSize = 0;
  FileContentReadBufInternal.dfGranularityMapping = 0;
  FileContentReadBufInternal.fWritePossible = fWritePossible;
  FileContentReadBufInternal.fProgressiveFlush = (dfAccessFlag & FILEFULLCONTENTBUFFER_PROGRESSIVE_FLUSH) != 0;

  FileContentReadBufInternal.dwSize = GetFileSizeByName(FileName, NULL, pei);
  if (FileContentReadBufInternal.dwSize == FILE_SIZE_NOT_EXIST)
    return FALSE;

  if (FileContentReadBufInternal.dwSize == 0)
  {
      (pOrg->size) = 0;
      return TRUE;
  }

  FileContentReadBufInternal.fVirtualMemFile=FALSE;
  FileContentReadBufInternal.u.wh.hFile =
    MyCreateFileW(FileName, GENERIC_READ | (fWritePossible ? GENERIC_WRITE:0), fWritePossible ? 0 : FILE_SHARE_READ, NULL,
                  OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);

  if ((FileContentReadBufInternal.u.wh.hFile != NULL) && ((dfAccessFlag & FILEFULLCONTENTBUFFER_RESIZE) != 0))
  {
      BOOL fSetPositionOk;

      LARGE_INTEGER pos;
      pos.QuadPart = dfSizeRequested;
      fSetPositionOk = MySetFilePointerEx(FileContentReadBufInternal.u.wh.hFile, pos, NULL, FILE_BEGIN);



      if (!fSetPositionOk)
      {
          DWORD dwErr = GetLastError();
          if (dwErr != NO_ERROR)
          {
              //FillWinError(FileContentReadBufInternal.u.wh.hFile,dwErr,pei);
              FillWinErrorFileName(FileName,dwErr,pei);
              CloseHandle(FileContentReadBufInternal.u.wh.hFile);
              FileContentReadBufInternal.u.wh.hFile = NULL;
          }
      }
      if (SetEndOfFile(FileContentReadBufInternal.u.wh.hFile))
      {
          FileContentReadBufInternal.dwSize = dfSizeRequested ;
      }
      else
      {
          FillWinErrorFileName(FileName,GetLastError(),pei);
          CloseHandle(FileContentReadBufInternal.u.wh.hFile);
          FileContentReadBufInternal.u.wh.hFile = NULL;
      }
  }

  if (FileContentReadBufInternal.u.wh.hFile != NULL)
  {
#ifdef DIFSTRM_USING_WINRT_API
    FileContentReadBufInternal.u.wh.hFileMap =
      CreateFileMappingFromApp(FileContentReadBufInternal.u.wh.hFile, NULL,
                        fWritePossible ? PAGE_READWRITE:PAGE_READONLY, 0, NULL);
#else
    FileContentReadBufInternal.u.wh.hFileMap =
      CreateFileMapping(FileContentReadBufInternal.u.wh.hFile, NULL,
                        fWritePossible ? PAGE_READWRITE:PAGE_READONLY, 0, 0, NULL);
#endif
  }

  if (FileContentReadBufInternal.u.wh.hFileMap == NULL)
  {
    if (FileContentReadBufInternal.u.wh.hFile != NULL)
      CloseHandle(FileContentReadBufInternal.u.wh.hFile);
    FileContentReadBufInternal.u.wh.hFile = NULL;
  }

#define ABSOLUTE_TRY_MAPPING
#if defined(ABSOLUTE_TRY_MAPPING) && (!defined(MEMORY_MAP_FULLFILE))
  FileContentReadBufInternal.lpBuf = NULL;
  dwErr=ERROR_NOT_ENOUGH_MEMORY;
#else
  FileContentReadBufInternal.lpBuf =
    MapViewOfFile(FileContentReadBufInternal.u.wh.hFileMap, fWritePossible ? FILE_MAP_WRITE : FILE_MAP_READ, 0, 0, 0);

  if (FileContentReadBufInternal.lpBuf == NULL)
      dwErr = GetLastError();
#endif


  if ((FileContentReadBufInternal.u.wh.hFileMap != NULL) &&
      ((FileContentReadBufInternal.lpBuf != NULL) || (dwErr == ERROR_NOT_ENOUGH_MEMORY)))
    pFileContentReadBufInternal =
      (FILECONTENTREADBUFINTERNAL *)
      DfsMalloc(sizeof(FILECONTENTREADBUFINTERNAL));

  if (pFileContentReadBufInternal == NULL)
  {
    DWORD dwErr = GetLastError();
    if (dwErr != NO_ERROR)
    {
        FillWinErrorFileName(FileName,dwErr,pei);
        return FALSE;
    }

    if (FileContentReadBufInternal.lpBuf != NULL)
      UnmapViewOfFile(FileContentReadBufInternal.lpBuf);
    CloseHandle(FileContentReadBufInternal.u.wh.hFileMap);
    CloseHandle(FileContentReadBufInternal.u.wh.hFile);
  }
  else
  {
    FillOrigDataForFullMemoryOrg(pOrg,FileContentReadBufInternal.lpBuf,FileContentReadBufInternal.dwSize);

#ifndef MEMORY_MAP_FULLFILE

    if (FileContentReadBufInternal.lpBuf == NULL)
    {
        SYSTEM_INFO si;

#ifdef DIFSTRM_USING_WINRT_API
        GetNativeSystemInfo(&si);
#else
        GetSystemInfo(&si);
#endif
        FileContentReadBufInternal.fPartialMapping = TRUE;
        FileContentReadBufInternal.dfBeginMapPos = 0;

        FileContentReadBufInternal.dfGranularityMapping = si.dwAllocationGranularity;

        FileContentReadBufInternal.dfMapSize = 1024*1024*64;//(si.dwAllocationGranularity*2);//

#ifdef DIFSTRM_USING_WINRT_API
        pOrg->pCurrentView =
        FileContentReadBufInternal.lpBuf = MapViewOfFileFromApp(FileContentReadBufInternal.u.wh.hFileMap,
           fWritePossible ? FILE_MAP_WRITE : FILE_MAP_READ, 0,
           (pOrg->size < FileContentReadBufInternal.dfMapSize) ? 0 : FileContentReadBufInternal.dfMapSize);
#else
        pOrg->pCurrentView =
        FileContentReadBufInternal.lpBuf = MapViewOfFile(FileContentReadBufInternal.u.wh.hFileMap,
           fWritePossible ? FILE_MAP_WRITE : FILE_MAP_READ, 0, 0,
           (pOrg->size < FileContentReadBufInternal.dfMapSize) ? 0 : FileContentReadBufInternal.dfMapSize);
#endif
/*
    if (((pOrg->pCurrentView) == NULL) && (pOrg->dfCurrentViewBegin ==0) && (pOrg->size!=0))
        printf("+");
  */
        pOrg->dfCurrentViewBegin = FileContentReadBufInternal.dfBeginMapPos;
        pOrg->dfCurrentViewLimitEnd = FileContentReadBufInternal.dfBeginMapPos+FileContentReadBufInternal.dfMapSize;
        pOrg->pInternalfncAdaptData = (dfvoidp)pFileContentReadBufInternal;
        pOrg->fncAdaptDataMapView = fncDiskAdaptDataMapView;
        pOrg->fncSetDirtyMapView = NULL;
        pOrg->dfMaxOrigDataExigibleSizeView = (FileContentReadBufInternal.dfMapSize) -
                                           (1*(FileContentReadBufInternal.dfGranularityMapping));
        pOrg->dfMaxOrigDataExigibleSizeViewMask = CalculateMaxOrigDataExigibleSizeViewMask(pOrg->dfMaxOrigDataExigibleSizeView);
    }

    if (((pOrg->pCurrentView) == NULL) && (pOrg->dfCurrentViewBegin ==0) && (pOrg->size!=0))
        printf("*");
#endif
    *pFileContentReadBufInternal = FileContentReadBufInternal;

  }
  *phFileContentReadBuf = (HFILECONTENTREADBUF) pFileContentReadBufInternal;
  return TRUE;
}



BOOL diskCloseFileFullContentBuffer(FILECONTENTREADBUFINTERNAL *pFileContentReadBufInternal,BOOL fFlushWrite, H_ERROR_INFO* pei)
{
  BOOL fRet=TRUE;
  if (pFileContentReadBufInternal==NULL)
      return FALSE;
/*
  FILECONTENTREADBUFINTERNAL *pFileContentReadBufInternal;

  if (hFileContentReadBuf == NULL)
    return FALSE;

  pFileContentReadBufInternal =
    (FILECONTENTREADBUFINTERNAL *) hFileContentReadBuf;
*/
  if (!pFileContentReadBufInternal->fVirtualMemFile)
  {
    if (pFileContentReadBufInternal->fProgressiveFlush)
       FlushViewOfFile(pFileContentReadBufInternal->lpBuf, 0);
    UnmapViewOfFile(pFileContentReadBufInternal->lpBuf);
    CloseHandle(pFileContentReadBufInternal->u.wh.hFileMap);

    if (fFlushWrite || pFileContentReadBufInternal->fProgressiveFlush)
        FlushFileBuffers (pFileContentReadBufInternal->u.wh.hFile);
    fRet = CloseHandle(pFileContentReadBufInternal->u.wh.hFile);

    if (!fRet)
    {
        DWORD dwErr = GetLastError();
        if (dwErr != NO_ERROR)
          FillWinError(NULL,dwErr,pei);
    }

  }
  DfsFree(pFileContentReadBufInternal);
  return fRet;
}
#endif



////////////////////////////////////////////////////////++++++++//////////////
////////////////////////////////////////////////////////++++++++//////////////
////////////////////////////////////////////////////////++++++++//////////////




#ifdef DIFSTRM_USING_WINRT_API
#else
BOOL MyMkdirTstr(LPSTR dirname)
{
//int iret;

  //iret = _mkdir(dirname);
  if (CreateDirectoryA(dirname,NULL))
    return TRUE;

//  if (errno==EACCES)
//    return FALSE;

  if ((*dirname)!='\0')
  {
      LPSTR lpParc = CharNextA(dirname);
      for (;;)
      {
          LPSTR lpNext;
          if ( ((*(lpParc))=='\\') && /*((*(dirname+i-1))!='\\') &&*/ ((*(lpParc+1))!='\\') )
              if ((*CharPrevA(dirname,lpParc))!='\\')
              {
                  *(lpParc) = '\0';
                  //mkdir(dirname);
                  CreateDirectoryA(dirname,NULL);
                  *(lpParc) = '\\';
              }

          lpNext = CharNextA(lpParc);
          if (lpParc==lpNext)
              break;
          lpParc=lpNext;
      }
  }

  return CreateDirectoryA(dirname,NULL);
}
#endif

#ifdef DIFSTRM_USING_WINRT_API
static LPWSTR MyCharPrevW(LPWSTR begin,LPWSTR str) {
    if (str == begin)
        return begin;
    else
        return str-1;
}

LPWSTR MyCharNextW(LPWSTR str)
{
    return str+1;
}
#else
#define MyCharPrevW(begin,str) (CharPrevW((begin),(str)))
#define MyCharNextW(str) (CharNextW((str)))
#endif

BOOL MyMkdirW_WinNT(LPWSTR dirname)
{
//int iret;

  //iret = _mkdir(dirname);
  if (CreateDirectoryW(dirname,NULL))
    return TRUE;

//  if (errno==EACCES)
//    return FALSE;

  if ((*dirname)!='\0')
  {
      LPWSTR lpParc = MyCharNextW(dirname);
      for (;;)
      {
          LPWSTR lpNext;
          if ( ((*(lpParc))=='\\') && /*((*(dirname+i-1))!='\\') &&*/ ((*(lpParc+1))!='\\') )
              if ((*MyCharPrevW(dirname,lpParc))!='\\')
              {
                  *(lpParc) = '\0';
                  //mkdir(dirname);
                  CreateDirectoryW(dirname,NULL);
                  *(lpParc) = '\\';
              }

          lpNext = MyCharNextW(lpParc);
          if (lpParc==lpNext)
              break;
          lpParc=lpNext;
      }
  }

  return CreateDirectoryW(dirname,NULL);
}


BOOL SVFAPI MyMkdir(dfwcharpc DirectoryName, H_ERROR_INFO * pei)
{
  BOOL fRet;
  dfuLong32 dfSize;
  dfwcharp pDirectoryNameAdapted;

  dfSize = (dfUnicodeStrlen(DirectoryName) *2) + 0x10;
  pDirectoryNameAdapted = (dfwcharp)DfsMalloc(dfSize * 2);

  ConvertFileNameAndPath(DirectoryName,pDirectoryNameAdapted,dfSize,TRUE);
#ifdef DIFSTRM_USING_WINRT_API
  fRet =
      MyMkdirW_WinNT((LPWSTR)pDirectoryNameAdapted);
#else
  if (IsUnicodeApiSupported())
    fRet =
      MyMkdirW_WinNT((dfwcharp)pDirectoryNameAdapted);

  else
  {
    char szAnsiDirName[512+MAX_PATH];

    ConvertUnicodeToAnsi(pDirectoryNameAdapted, szAnsiDirName,
                         sizeof(szAnsiDirName) - 1);
    fRet =
      MyMkdirTstr(szAnsiDirName);
  }
#endif
  DfsFree(pDirectoryNameAdapted);
  return fRet;
}


#ifndef MEMORY_MAP_FULLFILE

dfuIntPtr CalculateMaxOrigDataExigibleSizeViewMask(dfuIntPtr dfMaxOrigDataExigibleSizeView)
{
    dfuIntPtr dfRes = 1;
    while ((dfRes << 1)<=dfMaxOrigDataExigibleSizeView)
        dfRes = dfRes << 1;
    return dfRes - 1;
}

#ifndef PREVENT_USING_WINDOWS_FILEMAP

dfvoidp fncDiskAdaptDataMapView(void* pOrgData_,dfuLong64 dfCurrentViewBegin,dfuLong64 dfCurrentViewEndOrSize,BOOL fBySize,BOOL *pfDoneOk)
{
    BOOL fDoneOk=TRUE;
    dfuLong64 dfCurrentViewEnd;
    dfuLong64 dfCurrentViewBeginAround;
    dfuLong64 dfCurrentViewEndAround;
    ORIGDATA* pOrgData = (ORIGDATA*)pOrgData_;
    FILECONTENTREADBUFINTERNAL* pFileContentReadBufInternal = (FILECONTENTREADBUFINTERNAL*)pOrgData->pInternalfncAdaptData;
    dfuLong32 dfGranularity = pFileContentReadBufInternal->dfGranularityMapping;

    if (fBySize)
        dfCurrentViewEnd = dfCurrentViewBegin + dfCurrentViewEndOrSize ;
    else
        dfCurrentViewEnd = dfCurrentViewEndOrSize ;

    if (dfCurrentViewEnd<dfCurrentViewBegin)
        fDoneOk = FALSE;
    else
    {
        dfCurrentViewBeginAround = (dfCurrentViewBegin / dfGranularity) * dfGranularity;
        dfCurrentViewEndAround = ((dfCurrentViewEnd + dfGranularity - 1) / dfGranularity) * dfGranularity;

        if ((dfCurrentViewEndAround - dfCurrentViewBeginAround) < pFileContentReadBufInternal->dfMapSize)
        {
            dfuLong32 dfNbMaxAddGranUnit = (dfuLong32)((pFileContentReadBufInternal->dfMapSize -
                                                (dfCurrentViewEndAround - dfCurrentViewBeginAround)) /
                                                dfGranularity);
            dfuLong32 dfNbMaxAddGranUnitBefore = (dfuLong32)((dfCurrentViewBeginAround) / dfGranularity);
            dfuLong32 dfNbMaxAddGranUnitAfter = (dfuLong32)((pFileContentReadBufInternal->dwSize +
                                                (dfGranularity - 1) - dfCurrentViewEndAround) / dfGranularity);
            dfuLong32 dfNbAddGranBefore,dfNbAddGranAfter;
            dfNbAddGranBefore = min((dfNbMaxAddGranUnit)/2,dfNbMaxAddGranUnitBefore);
            dfNbAddGranAfter = min(dfNbMaxAddGranUnit - dfNbAddGranBefore,dfNbMaxAddGranUnitAfter);
            dfCurrentViewBeginAround -= dfNbAddGranBefore * dfGranularity;
            dfCurrentViewEndAround += dfNbAddGranAfter * dfGranularity;
        }
    }

    if (fDoneOk)
    {
        if ((dfCurrentViewEndAround - dfCurrentViewBeginAround) > (pFileContentReadBufInternal->dfMapSize))
            printf("\n+++---overmap %u %u\n",(unsigned int)((dfuIntPtr)dfCurrentViewEndAround - dfCurrentViewBeginAround),(unsigned int)(pFileContentReadBufInternal->dfMapSize));
    }
    if (fDoneOk)
    {
        if (pFileContentReadBufInternal->lpBuf)
        {
            if (pFileContentReadBufInternal->fProgressiveFlush)
                FlushViewOfFile(pFileContentReadBufInternal->lpBuf, 0);
            UnmapViewOfFile(pFileContentReadBufInternal->lpBuf);
        }
#ifdef DIFSTRM_USING_WINRT_API
        pFileContentReadBufInternal->lpBuf = MapViewOfFileFromApp(pFileContentReadBufInternal->u.wh.hFileMap,
                     pFileContentReadBufInternal->fWritePossible ? FILE_MAP_WRITE : FILE_MAP_READ,
                     dfCurrentViewBeginAround,
                     (dfCurrentViewEndAround >= pOrgData->size) ? 0 :
                       (dfuIntPtr)(dfCurrentViewEndAround - dfCurrentViewBeginAround));
#else
        pFileContentReadBufInternal->lpBuf = MapViewOfFile(pFileContentReadBufInternal->u.wh.hFileMap,
                     pFileContentReadBufInternal->fWritePossible ? FILE_MAP_WRITE : FILE_MAP_READ,
                     (DWORD)(dfCurrentViewBeginAround>>32),
                     (DWORD)(dfCurrentViewBeginAround),
                     (dfCurrentViewEndAround >= pOrgData->size) ? 0 :
                       (dfuIntPtr)(dfCurrentViewEndAround - dfCurrentViewBeginAround));
#endif

        if (pFileContentReadBufInternal->lpBuf == NULL)
            fDoneOk = FALSE;
        else
        {
          pOrgData->pCurrentView = ((dfbytep)(pFileContentReadBufInternal->lpBuf)) - dfCurrentViewBeginAround;
          pOrgData->dfCurrentViewBegin = dfCurrentViewBeginAround;
          pOrgData->dfCurrentViewLimitEnd = dfCurrentViewEndAround;
        }
    }

    if (((pOrgData->pCurrentView) == NULL) && (pOrgData->dfCurrentViewBegin ==0) && (pOrgData->size!=0))
        printf("-");

    if (pfDoneOk!=NULL)
        *pfDoneOk = fDoneOk;

    if (!fDoneOk)
      return NULL;
    else
      return pOrgData->pCurrentView;
}
#endif
#endif



/*******************************************************************************/
#ifdef DIFSTRM_USING_WINRT_API
#else
BOOL SVFAPI CheckIfFileSameA(const char* lpFile1,const char* lpFile2)
{
        BOOL fRet;
        char *lpFile1Full,*lpFile1Short,*lpFile1Cmp;
        char *lpFile2Full,*lpFile2Short,*lpFile2Cmp;
        char *lpFilePos;
        dfuLong32 dfBufItemSize1;
        dfuLong32 dfBufItemSize2;

        dfBufItemSize1=GetFullPathNameA(lpFile1,0,NULL,&lpFilePos);
        lpFile1Full = (char*)DfsMalloc(sizeof(char)*((dfBufItemSize1+0x20)*2));
        lpFile1Short = lpFile1Full + dfBufItemSize1 + 0x10;
        dfBufItemSize1=GetFullPathNameA(lpFile1,dfBufItemSize1+0x10,lpFile1Full,&lpFilePos);
        if (GetShortPathNameA(lpFile1Full,lpFile1Short,dfBufItemSize1+0x10)>0)
            lpFile1Cmp = lpFile1Short;
        else
            lpFile1Cmp = lpFile1Full;

        dfBufItemSize2=GetFullPathNameA(lpFile2,0,NULL,&lpFilePos);
        lpFile2Full = (char*)DfsMalloc(sizeof(char)*((dfBufItemSize2+0x20)*2));
        lpFile2Short = lpFile2Full + dfBufItemSize2 + 0x10;
        dfBufItemSize2=GetFullPathNameA(lpFile2,dfBufItemSize2+0x10,lpFile2Full,&lpFilePos);
        if (GetShortPathNameA(lpFile2Full,lpFile2Short,dfBufItemSize2+0x10)>0)
            lpFile2Cmp = lpFile2Short;
        else
            lpFile2Cmp = lpFile2Full;

        fRet = lstrcmpiA(lpFile1Cmp,lpFile2Cmp) == 0;
        DfsFree(lpFile1Full);
        DfsFree(lpFile2Full);

        return fRet;
}

BOOL SVFAPI CheckIfFileSame(dfwcharpc lpFile1,dfwcharpc lpFile2)
{
    if ((lpFile1 == NULL) || (lpFile2 == NULL))
        return (lpFile1 == lpFile2);
    if ((*lpFile1)=='*')
        return dfUnicodeStrcmpi(lpFile1,lpFile2)==0;

#ifdef DIFSTRM_USING_WINRT_API
#else
    if (!(IsUnicodeApiSupported()))
    {
        char szFile1[MAX_PATH+0x100];
        char szFile2[MAX_PATH+0x100];

        ConvertUnicodeToAnsi(lpFile1, szFile1, sizeof(szFile1) - 1);
        ConvertUnicodeToAnsi(lpFile2, szFile2, sizeof(szFile2) - 1);

        return CheckIfFileSameA(szFile1,szFile2);
    }
    else
#endif
    {
        BOOL fRet;
        dfwcharp lpFile1Full,lpFile1Short,lpFile1Cmp;
        dfwcharp lpFile2Full,lpFile2Short,lpFile2Cmp;
        dfwcharp lpFilePos;
        dfuLong32 dfBufItemSize1;
        dfuLong32 dfBufItemSize2;

        dfBufItemSize1=GetFullPathNameW(lpFile1,0,NULL,&lpFilePos);
        lpFile1Full = (dfwcharp)DfsMalloc(sizeof(WCHAR)*((dfBufItemSize1+0x20)*2));
        lpFile1Short = lpFile1Full + dfBufItemSize1 + 0x10;
        dfBufItemSize1=GetFullPathNameW(lpFile1,dfBufItemSize1+0x10,lpFile1Full,&lpFilePos);
        if (GetShortPathNameW(lpFile1Full,lpFile1Short,dfBufItemSize1+0x10)>0)
            lpFile1Cmp = lpFile1Short;
        else
            lpFile1Cmp = lpFile1Full;

        dfBufItemSize2=GetFullPathNameW(lpFile2,0,NULL,&lpFilePos);
        lpFile2Full = (dfwcharp)DfsMalloc(sizeof(WCHAR)*((dfBufItemSize2+0x20)*2));
        lpFile2Short = lpFile2Full + dfBufItemSize2 + 0x10;
        dfBufItemSize2=GetFullPathNameW(lpFile2,dfBufItemSize2+0x10,lpFile2Full,&lpFilePos);
        if (GetShortPathNameW(lpFile2Full,lpFile2Short,dfBufItemSize2+0x10)>0)
            lpFile2Cmp = lpFile2Short;
        else
            lpFile2Cmp = lpFile2Full;

        fRet = dfUnicodeStrcmpi(lpFile1Cmp,lpFile2Cmp) == 0;
        DfsFree(lpFile1Full);
        DfsFree(lpFile2Full);

        return fRet;
    }
}
#endif

#include <conio.h>

#ifdef DIFSTRM_USING_WINRT_API
/*
dfwchar SVFAPI My_getch_console()
{
    return _getch();
}

char SVFAPI My_getch_console_char()
{
    return _getch();
}
*/
#else
dfwchar SVFAPI My_getch_console()
{
    return _getch();
}

char SVFAPI My_getch_console_char()
{
    return _getch();
}
#endif
#endif

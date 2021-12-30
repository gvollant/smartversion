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


#include "difbasic.h"

#if defined(SMARTVERSION_USE_WIN32)

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "DfsTlTyp.h"
#include "difstool.h"


#if defined(WINAPI_FAMILY) && (!(defined(DIFSTRM_USING_WINRT_API))) && (!(defined(PREVENT_DIFSTRM_USING_WINRT_API)))
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#else
#define DIFSTRM_USING_WINRT_API 1
#endif
#endif

/*

hDifTimeElasped SVFAPI SyncDifBuidTimeMarkerObject();
unsigned int SVFAPI SyncDifGetMSecElapsed(hDifTimeElasped ptr);
unsigned int SVFAPI SyncDifGetMSecElapsedNotDestructive(hDifTimeElasped ptr, int destructObject);

typedef void* SYNC_DIF_MUTEX_OBJECT;

SYNC_DIF_MUTEX_OBJECT SVFAPI SyncDifBuildMutex();
void SVFAPI SyncDifGetMutex(SYNC_DIF_MUTEX_OBJECT pMut);
void SVFAPI SyncDifReleaseMutex(SYNC_DIF_MUTEX_OBJECT pMut);
void SVFAPI SyncDifDeleteMutex(SYNC_DIF_MUTEX_OBJECT pMut);
*/
typedef struct
{
    DWORD Tick;
} TIMEBEGIN;

hDifTimeElasped SVFAPI SyncDifBuidTimeMarkerObject()
{
    TIMEBEGIN* pBegin = (TIMEBEGIN*)malloc(sizeof(TIMEBEGIN));
#ifdef DIFSTRM_USING_WINRT_API
    pBegin->Tick = (DWORD)GetTickCount64();
#else
    pBegin->Tick = GetTickCount();
#endif
    return pBegin;
}


unsigned int SVFAPI SyncDifGetMSecElapsedNotDestructive(hDifTimeElasped ptr, int destructObject)
{
    TIMEBEGIN tBegin;
    TIMEBEGIN tEnd;
    TIMEBEGIN* pBegin = (TIMEBEGIN*)ptr;
    unsigned int iRet;
    tBegin = *pBegin;
#ifdef DIFSTRM_USING_WINRT_API
    tEnd.Tick = (DWORD)GetTickCount64();
#else
    tEnd.Tick = GetTickCount();
#endif
    if (destructObject != 0)
      free(pBegin);


    iRet = (unsigned int)(tEnd.Tick - tBegin.Tick) ;
    return iRet;
}

unsigned int SVFAPI SyncDifGetMSecElapsed(hDifTimeElasped ptr)
{
    return SyncDifGetMSecElapsedNotDestructive(ptr,1);
}



typedef struct
{
    CRITICAL_SECTION cs;
} SYNC_MUTEX_OBJECT_INTERNAL;

SYNC_DIF_MUTEX_OBJECT SVFAPI SyncDifBuildMutex()
{
    SYNC_MUTEX_OBJECT_INTERNAL* pMoi = (SYNC_MUTEX_OBJECT_INTERNAL*)malloc(sizeof(SYNC_MUTEX_OBJECT_INTERNAL));
    if (pMoi == NULL)
        return NULL;
#ifdef DIFSTRM_USING_WINRT_API
    InitializeCriticalSectionEx(&(pMoi->cs),0,0);
#else
    InitializeCriticalSection(&(pMoi->cs));
#endif

    return (SYNC_DIF_MUTEX_OBJECT)pMoi;
}

void SVFAPI SyncDifGetMutex(SYNC_DIF_MUTEX_OBJECT pMut)
{
    SYNC_MUTEX_OBJECT_INTERNAL* pMoi = (SYNC_MUTEX_OBJECT_INTERNAL*)pMut;
    if (pMoi != NULL)
        EnterCriticalSection(&pMoi->cs);
}

void SVFAPI SyncDifReleaseMutex(SYNC_DIF_MUTEX_OBJECT pMut)
{
    SYNC_MUTEX_OBJECT_INTERNAL* pMoi = (SYNC_MUTEX_OBJECT_INTERNAL*)pMut;
    if (pMut != NULL)
        LeaveCriticalSection(&pMoi->cs);
}

void SVFAPI SyncDifDeleteMutex(SYNC_DIF_MUTEX_OBJECT pMut)
{
    SYNC_MUTEX_OBJECT_INTERNAL* pMoi = (SYNC_MUTEX_OBJECT_INTERNAL*)pMut;
    if (pMoi != NULL)
    {
        DeleteCriticalSection(&pMoi->cs);
        free(pMoi);
    }
}

#ifdef DIFSTRM_USING_WINRT_API
int SVFAPI dfUnicodeStrcmpi(dfwcharpc str1,dfwcharpc str2)
{
	return _wcsicmp((const wchar_t *)str1,(const wchar_t *)str2);
}

int SVFAPI dfUnicodeStrcmp(dfwcharpc str1,dfwcharpc str2)
{
	return wcscmp((const wchar_t *)str1,(const wchar_t *)str2);
}


#else

int SVFAPI dfUnicodeStrcmpi(dfwcharpc str1, dfwcharpc str2)
{
  int fRet;
  if (str1==str2)
      return 0;

  if (IsUnicodeApiSupported())
    fRet = lstrcmpiW(str1, str2);
  else
  {
    LPSTR lpszAnsiStr1;
    LPSTR lpszAnsiStr2;
    dfuLong32 dwAlloc1 = (dfUnicodeStrlen(str1) * 4) + 0x10;
    dfuLong32 dwAlloc2 = (dfUnicodeStrlen(str2) * 4) + 0x10;
    lpszAnsiStr1 = (LPSTR)DfsMalloc(dwAlloc1 + 0x10);
    lpszAnsiStr2 = (LPSTR)DfsMalloc(dwAlloc2 + 0x10);

    ConvertUnicodeToAnsi(str1, lpszAnsiStr1, dwAlloc1);
    ConvertUnicodeToAnsi(str2, lpszAnsiStr2, dwAlloc2);
    fRet = lstrcmpiA(lpszAnsiStr1, lpszAnsiStr2);
    DfsFree(lpszAnsiStr1);
    DfsFree(lpszAnsiStr2);
  }

  return fRet;
}

int SVFAPI dfUnicodeStrcmp(dfwcharpc str1, dfwcharpc str2)
{
  int fRet;
  if (str1==str2)
      return 0;

  if (IsUnicodeApiSupported())
    fRet = lstrcmpW(str1, str2);
  else
  {
    LPSTR lpszAnsiStr1;
    LPSTR lpszAnsiStr2;
    dfuLong32 dwAlloc1 = (dfUnicodeStrlen(str1) * 4) + 0x10;
    dfuLong32 dwAlloc2 = (dfUnicodeStrlen(str2) * 4) + 0x10;
    lpszAnsiStr1 = (LPSTR)DfsMalloc(dwAlloc1 + 0x10);
    lpszAnsiStr2 = (LPSTR)DfsMalloc(dwAlloc2 + 0x10);

    ConvertUnicodeToAnsi(str1, lpszAnsiStr1, dwAlloc1);
    ConvertUnicodeToAnsi(str2, lpszAnsiStr2, dwAlloc2);
    fRet = lstrcmpA(lpszAnsiStr1, lpszAnsiStr2);
    DfsFree(lpszAnsiStr1);
    DfsFree(lpszAnsiStr2);
  }

  return fRet;
}
#endif

int SVFAPI CompareUnicodeAndAnsiString(dfwcharpc str1,const char* lpszAnsi,BOOL fCaseSensitive)
{
    int iRet;
    //if (IsUnicodeApiSupported())
    {
        dfwcharp pszUnic;
        dfuLong32 dfLenAnsi = (dfuLong32)strlen(lpszAnsi);

        pszUnic = (dfwcharp)DfsMalloc((sizeof(dfwchar)*dfLenAnsi*4)+0x110);


        ConvertAnsiToUnicode(lpszAnsi, pszUnic, (dfLenAnsi*4)+0x100);


        iRet = fCaseSensitive ? dfUnicodeStrcmp(str1,pszUnic) : dfUnicodeStrcmpi(str1,pszUnic);

        DfsFree(pszUnic);
    }

  return iRet;
}

typedef enum
{
    KIND_WINNT, KIND_WIN95ORGREATHER, KIND_WIN32S
}
WIN32KIND;

WIN32KIND GetWin32VerKind()
{
//BOOL IsWin395OrHigher(void);


#ifdef DIFSTRM_USING_WINRT_API
	return KIND_WINNT;
#else
  WORD wVer;
  if ((GetVersion() & 0x80000000) == 0)
      return KIND_WINNT;
  wVer = LOWORD(GetVersion());
  wVer = (((WORD) LOBYTE(wVer)) << 8) | (WORD) HIBYTE(wVer);

  if (wVer >= 0x035F)
      return KIND_WIN95ORGREATHER;
  else
      return KIND_WIN32S;
#endif
}

BOOL IsUnicodeApiSupported()
{
  BOOL fUnicodeSupported = (GetWin32VerKind() == KIND_WINNT);
  // return yes for WinNT and Win2000
  return fUnicodeSupported ;
}


#ifdef DIFSTRM_USING_WINRT_API

	unsigned long dfPhysicalMemoryKBvalue=0;


dfuLong32 SVFAPI GetPhysicalMemoryKb()
{
	if (dfPhysicalMemoryKBvalue!=0)
		return (dfuLong32)dfPhysicalMemoryKBvalue;
	else
		return 256*1024;
}


#else
typedef struct {
    DWORD dwLength;
    DWORD dwMemoryLoad;
    unsigned __int64 ullTotalPhys;
    unsigned __int64 ullAvailPhys;
    unsigned __int64 ullTotalPageFile;
    unsigned __int64 ullAvailPageFile;
    unsigned __int64 ullTotalVirtual;
    unsigned __int64 ullAvailVirtual;
    unsigned __int64 ullAvailExtendedVirtual;
} myMEMORYSTATUSEX;

typedef BOOL (WINAPI* tGlobalMemoryStatusExEx)(myMEMORYSTATUSEX*);
static BOOL MyGlobalMemoryStatusEx(myMEMORYSTATUSEX* lpMS)
{
    {
        tGlobalMemoryStatusExEx fncGlobalMemoryStatusExEx=NULL;
        HMODULE hModule = GetModuleHandleA("KERNEL32.DLL");
        if (hModule!=NULL)
            fncGlobalMemoryStatusExEx = (tGlobalMemoryStatusExEx)
                        GetProcAddress(hModule,"GlobalMemoryStatusExEx");
        if (fncGlobalMemoryStatusExEx)
            return (*fncGlobalMemoryStatusExEx)(lpMS);
    }
    return FALSE;
}

dfuLong32 SVFAPI GetPhysicalMemoryKb()
{
    MEMORYSTATUS ms;
    myMEMORYSTATUSEX msx;
    LARGE_INTEGER li;
    if (MyGlobalMemoryStatusEx(&msx))
    {
        LARGE_INTEGER li;
        li.QuadPart = msx.ullTotalPhys;
    }
    else
    {
        GlobalMemoryStatus(&ms);
        li.QuadPart = ms.dwTotalPhys;
    }
    return (li.LowPart >> 10) | (li.HighPart << 22);
}
#endif

#ifdef DIFSTRM_USING_WINRT_API
#include <stdio.h>
#endif
BOOL ConvertNum64ToUnformattedStrAnsi(dfuLong64 dfNum,char* szAnsi, dfuLong32 dfBufSize,dfuLong32 dfMinimalTextSize)
{
    char szNum[32];
    dfuLong32 dfLenRaw ;
    dfuLong32 i=0;

#ifdef DIFSTRM_USING_WINRT_API
	if ((dfNum>>32) == 0)
      sprintf(szNum,"%lu",(DWORD)dfNum);
    else
	{
		sprintf(szNum,"%I64u",(unsigned __int64)dfNum);
	}
	dfLenRaw = (dfuLong32)strlen(szNum);
#else
    if ((dfNum>>32) == 0)
      wsprintfA(szNum,"%lu",(DWORD)dfNum);
    else
#if defined(WIN64) || defined(_WIN64)
      wsprintfA(szNum,"%I64u",dfNum);
#else
      wsprintfA(szNum,"%I64u",(DWORD)dfNum,(DWORD)(dfNum>>32));
#endif


    dfLenRaw = lstrlenA(szNum);
#endif
    if (dfLenRaw+1>dfBufSize)
        return FALSE;

    if (dfBufSize<=dfMinimalTextSize)
        return FALSE;

    if (dfLenRaw<dfMinimalTextSize)
      for (i=0;i<dfMinimalTextSize-dfLenRaw;i++)
          *(szAnsi+i)=' ';
    strcpy(szAnsi+i,szNum);
    return TRUE;
}

BOOL ConvertNum64ToUnformattedStrUnicode(dfuLong64 dfNum,dfwchar* szUnicode, dfuLong32 dfBufSize,dfuLong32 dfMinimalTextSize)
{
    char szAnsi[32];
    if (!ConvertNum64ToUnformattedStrAnsi(dfNum,szAnsi,min(32,dfBufSize),dfMinimalTextSize))
        return FALSE;

    ConvertAnsiToUnicode(szAnsi,szUnicode,dfBufSize);
    return TRUE;
}

void DispMessageForDebugger(dfwcharpc lpszMessage)
{
    OutputDebugStringW(lpszMessage);
}

void DispMessageForUser(dfwcharpc lpszMessageCaption,dfwcharpc lpszMessagePlain)
{
#ifdef DIFSTRM_USING_WINRT_API
#else
    MessageBoxW(NULL,lpszMessagePlain,lpszMessageCaption,MB_OK);
#endif
}


BOOL ConvertAnsiToUnicodeEx(const char *lpszAnsi, dfwchar * lpszUnicode,
                          dfuLong32 dfUnicodeSize,BOOL fTransformSlashToOperatingSystemCompatibleSlash)
{
  return (MultiByteToWideChar(CP_ACP, 0 /*WC_DEFAULTCHAR */ ,
                              (LPCSTR) lpszAnsi, -1,
                              (LPWSTR) lpszUnicode, dfUnicodeSize / 1) > 0);
  return TRUE;
}

BOOL ConvertUnicodeToAnsiEx(const dfwchar * lpszUnicode, char *lpszAnsi,
                          dfuLong32 dfAnsiSize,BOOL fTransformSlashToOperatingSystemCompatibleSlash)
{
  int iRet;

  iRet = WideCharToMultiByte(CP_ACP, 0 /*WC_DEFAULTCHAR */ , lpszUnicode, -1,
                             lpszAnsi, dfAnsiSize, NULL, NULL);
  return iRet > 0;
}

BOOL ConvertOemToUnicode(const char *lpszAnsi, dfwchar * lpszUnicode,
                          dfuLong32 dfUnicodeSize)
{
  return (MultiByteToWideChar(CP_OEMCP, 0 /*WC_DEFAULTCHAR */ ,
                              (LPCSTR) lpszAnsi, -1,
                              (LPWSTR) lpszUnicode, dfUnicodeSize / 1) > 0);
  return TRUE;
}

BOOL ConvertUnicodeToOem(const dfwchar * lpszUnicode, char *lpszAnsi,
                          dfuLong32 dfAnsiSize)
{
  int iRet;

  iRet = WideCharToMultiByte(CP_OEMCP, 0 /*WC_DEFAULTCHAR */ , lpszUnicode, -1,
                             lpszAnsi, dfAnsiSize, NULL, NULL);
  return iRet > 0;
}


BOOL ConvertAnsiToUnicode(const char *lpszAnsi, dfwcharp lpszUnicode,
                          dfuLong32 dfUnicodeSize)
{
    return ConvertAnsiToUnicodeEx(lpszAnsi, lpszUnicode,
                          dfUnicodeSize,
                          FALSE);
}

BOOL ConvertUnicodeToAnsi(dfwcharpc lpszUnicode, char *lpszAnsi,
                          dfuLong32 dfAnsiSize)
{
    return ConvertUnicodeToAnsiEx(lpszUnicode,lpszAnsi,dfAnsiSize,FALSE);
}

dfwcharpc SVFAPI GetUnicodeStringDirectorySeparator()
{
    return GetUnicodeStringAntiSlashSeparator();
}

#include <stdio.h>
void SVFAPI DispOutUnicodeString(dfwcharpc str)
{
    if (str!=NULL)
      printf("%ws",str);
}
#endif

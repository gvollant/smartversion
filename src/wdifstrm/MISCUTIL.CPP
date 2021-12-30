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

#include <windows.h>
#include <windowsx.h>
#ifndef WIN32
#include "winx31ad.h"
#endif
#include <commdlg.h>
#include <shellapi.h>
#include <dos.h>
#include <stdlib.h>
#include <stdio.h>
#include <direct.h>
#include <string.h>
#include <tchar.h>

#include "miscutil.h"
#include "zlib.h"
#include <dos.h>
#include <direct.h>
#include <errno.h>

#ifdef UNICODE
#define mymkdirmacro _wmkdir
#else
#define mymkdirmacro _mkdir
#endif

#ifdef __BORLANDC__
#include <dir.h>
#define _mkdir mkdir

#define _chdir chdir
#define _getcwd getcwd
#define _strupr strupr
#ifdef WIN32
#define hmemcpy memcpy
#endif
#endif


#ifndef WIN32
#define API WINAPI
#include "winnet16.h" // this is the WINNET.H giving for Windows/workgroup
#endif



BOOL TryFreePtr(LPVOID lp)
{
  if (lp == NULL) return FALSE;
  GlobalFreePtr(lp);
  //lp = NULL;
  return TRUE;
}

LPVOID ReallocOrAlloc(LPVOID lpOldPtr,DWORD dwNewSize)
{
  if (lpOldPtr == NULL) return GlobalAllocPtr(GMEM_MOVEABLE,dwNewSize);
          else return GlobalReAllocPtr(lpOldPtr,dwNewSize,GMEM_MOVEABLE);
}

/**********************/

// --------------------------------------------------------------------------
//  module wide definitions
// --------------------------------------------------------------------------
/*
#define CRC_POLYNOMIAL  0xEDB88320L     // standard polynomial constant

static DWORD dwCRCTable[256];                  // CRC table (1 KB)
static BOOL fInitialised=FALSE;
// --------------------------------------------------------------------------
//  FUNCTION    InitializeCRCTable
//  PURPOSE     initializes CRC table with proper values
// --------------------------------------------------------------------------
//  INPUT       void
//
//  OUTPUT      void
// --------------------------------------------------------------------------
//  COMMENTS
// --------------------------------------------------------------------------

void InitializeCRCTable(void)
{
  int   i, j;
  DWORD dwCRC;

  for(i = 0; i < 256; i++)
  {
    dwCRC = i;
    for(j = 8; j > 0; j--)
    {
      if(dwCRC & 1)
        dwCRC = (dwCRC >> 1) ^ CRC_POLYNOMIAL;
      else
        dwCRC >>= 1;
    }
    dwCRCTable[i] = dwCRC;
  }
  fInitialised=TRUE;
}
*/
// --------------------------------------------------------------------------
//  FUNCTION    ComputeBufferCRC
//  PURPOSE     compute buffer CRC
// --------------------------------------------------------------------------
//  INPUT       dwCRC           CRC initial value
//              pvBuffer        pointer to buffer to compute CRC for
//              cbBuffer        size of buffer in bytes
//
//  OUTPUT      DWORD           CRC value
// --------------------------------------------------------------------------
//  COMMENTS    this function computes the CRC for a block of data.
//              it accepts a CRC initial value and returns the updated value.
//              uses dwCRCTable array.
// --------------------------------------------------------------------------
/*
DWORD ComputeBufferCRC(DWORD dwCRC, LPVOID pvBuffer, DWORD cbBuffer)
{
  LPBYTE pb;
  DWORD dwTmp1, dwTmp2;
  UINT  i;

  if (!fInitialised)
      InitializeCRCTable();

  pb = (LPBYTE)pvBuffer;

  for(i = 0; i < cbBuffer; i++)
  {
    dwTmp1 = (dwCRC >> 8) & 0x00FFFFFFL;
    dwTmp2 = dwCRCTable[((int)dwCRC ^ *pb++) & 0xFF];
    dwCRC  = dwTmp1 ^ dwTmp2;
  }
  return dwCRC;
}
*/


DWORD ComputeBufferCRC(DWORD dwCRC, LPCVOID pvBuffer, DWORD cbBuffer)
{
    return crc32(dwCRC ^(0xffffffff),(Bytef*)pvBuffer,cbBuffer)^(0xffffffff);
}

/*
DWORD ComputeBufferCRC(DWORD dwCRC, LPVOID pvBuffer, DWORD cbBuffer)
{
  LPBYTE pb;
  DWORD dwTmp1, dwTmp2;
  UINT  i;

  if (!fInitialised)
      InitializeCRCTable();

  pb = (LPBYTE)pvBuffer;

  {
  DWORD dwFour,dwRest;
  dwFour = cbBuffer/8;
  dwRest = cbBuffer - (dwFour*8);

    for(i = 0; i < dwFour ; i++)
    {

          dwCRC  = (dwCRC >> 8) ^ (dwCRCTable[(BYTE)(((BYTE)dwCRC ^ *pb))]);
          dwCRC  = (dwCRC >> 8) ^ (dwCRCTable[(BYTE)(((BYTE)dwCRC ^ *(pb+1)))]);
          dwCRC  = (dwCRC >> 8) ^ (dwCRCTable[(BYTE)(((BYTE)dwCRC ^ *(pb+2)))]);
          dwCRC  = (dwCRC >> 8) ^ (dwCRCTable[(BYTE)(((BYTE)dwCRC ^ *(pb+3)))]);

          dwCRC  = (dwCRC >> 8) ^ (dwCRCTable[(BYTE)(((BYTE)dwCRC ^ *(pb+4)))]);
          dwCRC  = (dwCRC >> 8) ^ (dwCRCTable[(BYTE)(((BYTE)dwCRC ^ *(pb+5)))]);
          dwCRC  = (dwCRC >> 8) ^ (dwCRCTable[(BYTE)(((BYTE)dwCRC ^ *(pb+6)))]);
          dwCRC  = (dwCRC >> 8) ^ (dwCRCTable[(BYTE)(((BYTE)dwCRC ^ *(pb+7)))]);

          pb+=8;

    }

    for(i = 0; i < dwRest; i++)
    {
      dwTmp1 = (dwCRC >> 8) & 0x00FFFFFFL;
      dwTmp2 = dwCRCTable[((int)dwCRC ^ *pb++) & 0xFF];
      dwCRC  = dwTmp1 ^ dwTmp2;
    }
  }
  return dwCRC;
}

*/
#ifdef WIN32


DWORD GivePctDivide(DWORD dwDone, DWORD dwAll)
{
    DWORD dwPct;
    if ((dwDone<=((0x0fffffff)/100)) || (dwAll<100))
    {
        if (dwAll!=0)
            dwPct=(dwDone*100)/dwAll;
        else
            dwPct=1;
    }
    else
        dwPct=dwDone/(dwAll/100);
    return dwPct;
}

#if (defined(WIN64) || defined(_WIN64)) && (!defined(USEARITH64))
#define USEARITH64
#endif

#if (!defined(NO_USEARITH64)) &&  (!defined(USEARITH64))
#define USEARITH64
#endif

#ifdef USEARITH64

BOOL AddValueTo64(DWORD &dwLow,DWORD &dwHigh,DWORD dwAddLow,DWORD dwAddHigh)
{
    unsigned __int64 a,b,c;
    a = dwLow | (((unsigned __int64)dwHigh)<<32);
    b = dwAddLow | (((unsigned __int64)dwAddHigh)<<32);
    c=a+b;
    dwLow = (DWORD)(c & 0xffffffffUL);
    dwHigh = (DWORD)(c>>32);
    return TRUE;
}

BOOL SubValueFrom64(DWORD &dwLow,DWORD &dwHigh,DWORD dwSubLow,DWORD dwSubHigh)
{
    unsigned __int64 a,b,c;
    a = dwLow | (((unsigned __int64)dwHigh)<<32);
    b = dwSubLow | (((unsigned __int64)dwSubHigh)<<32);
    c=a-b;
    dwLow = (DWORD)(c & 0xffffffffUL);
    dwHigh = (DWORD)(c>>32);
    return TRUE;
}
#else
BOOL AddValueTo64(DWORD &dwLow,DWORD &dwHigh,DWORD dwAddLow,DWORD dwAddHigh)
{
    if ((dwLow+dwAddLow<dwAddLow) || (dwLow+dwAddLow<dwLow))
        dwHigh++;
    dwLow+=dwAddLow;
    dwHigh+=dwAddHigh;
    return TRUE;
}

BOOL SubValueFrom64(DWORD &dwLow,DWORD &dwHigh,DWORD dwSubLow,DWORD dwSubHigh)
{
    dwHigh -= dwSubHigh;
    if (dwLow>=dwSubLow)
    {
        dwLow -= dwSubLow;
    }
    else
    {
        dwHigh --;
        dwLow -= dwSubLow;
    }
    return TRUE;
}
#endif

DWORD GetKBSizeFrom64Size(DWORD dwLow,DWORD dwHigh)
{
   return(dwLow >> 10) | (dwHigh << 22);
}

int  CompareValue64(DWORD dwLow1,DWORD dwHigh1,DWORD dwLow2,DWORD dwHigh2)
{
    if (dwHigh1>dwHigh2)
        return 1;
    else
    if (dwHigh1<dwHigh2)
        return -1;
    else
    if (dwLow1>dwLow2)
        return 1;
    else
    if (dwLow1<dwLow2)
        return -1;
    else
    return 0;
}

#if (defined(WIN64) || defined(_WIN64) || defined(WIN32)) && (!defined(USEARITH64CPLX))
#define USEARITH64CPLX
#endif

#ifdef USEARITH64CPLX
BOOL CalcMulTo64(DWORD dwA,DWORD dwB,DWORD &dwLow,DWORD &dwHigh)
{
    unsigned __int64 a,b,c;
    a = dwA;
    b = dwB;
    c=a*b;
    dwLow = (DWORD)(c & 0xffffffffUL);
    dwHigh = (DWORD)(c>>32);
    return TRUE;
}

BOOL Multiply64(DWORD &dwLow,DWORD &dwHigh,DWORD dwLow2,DWORD dwHigh2)
{
    unsigned __int64 a,b,c;
    a = dwLow | (((unsigned __int64)dwHigh)<<32);
    b = dwLow2 | (((unsigned __int64)dwHigh2)<<32);
    c=a*b;
    dwLow = (DWORD)(c & 0xffffffffUL);
    dwHigh = (DWORD)(c>>32);
    return TRUE;
}
#else

BOOL CalcMulTo64(DWORD dwA,DWORD dwB,DWORD &dwLow,DWORD &dwHigh)
{/*
    if (((dwB/0x100)*0x100) == dwB)
    {
        dwB/=0x100;
        dwA*=dwB;
        dwLow = dwA*0x100;
        dwHigh=dwA/0x1000000;
    }
    else
    if (((dwA/0x100)*0x100) == dwA)
    {
        dwA/=0x100;
        dwB*=dwA;
        dwLow = dwB*0x100;
        dwHigh=dwB/0x1000000;
    }
    else*/
    {
        dwLow=dwA;
        dwHigh=0;
        Multiply64(dwLow,dwHigh,dwB,0);
    }
    return TRUE;
}

#define Mul64_PR_BIT(n)       ((DWORD)1 << (n))
#define Mul64_PR_BITMASK(n)   (Mul64_PR_BIT(n) - 1)

#define Mul64__lo16(a)        ((a) & Mul64_PR_BITMASK(16))
#define Mul64__hi16(a)        ((a) >> 16)




BOOL Multiply64(DWORD &dwLow,DWORD &dwHigh,DWORD dwLow2,DWORD dwHigh2)
{
    DWORD dwRLow,dwRHigh;

    DWORD _a1, _a0, _b1, _b0, _y0, _y1, _y2, _y3;


     _a1 = Mul64__hi16(dwLow);
     _a0 = Mul64__lo16(dwLow);
     _b1 = Mul64__hi16(dwLow2);
     _b0 = Mul64__lo16(dwLow2);
     _y0 = _a0 * _b0;
     _y1 = _a0 * _b1;
     _y2 = _a1 * _b0;
     _y3 = _a1 * _b1;
     _y1 += Mul64__hi16(_y0);                         /* can't carry */
     _y1 += _y2;                                /* might carry */
     if (_y1 < _y2)
        _y3 += (DWORD)(Mul64_PR_BIT(16));  /* propagate */
     dwRLow = (Mul64__lo16(_y1) << 16) + Mul64__lo16(_y0);
     dwRHigh = _y3 + Mul64__hi16(_y1);

    dwRHigh += dwHigh * dwLow2 + dwLow * dwHigh2;

    dwLow = dwRLow;
    dwHigh = dwRHigh;
    return TRUE;
}
#endif

BOOL Multiply64LH(DWORD &dwLow,LONG &lHigh,DWORD dwLow2,LONG dwHigh2)
{
    DWORD dwHighdw=lHigh;
    BOOL fRet=Multiply64(dwLow,dwHighdw,dwLow2,dwHigh2);
    lHigh=dwHighdw;
    return fRet;
}

DWORD CalculateRatio32(DWORD dfValue, DWORD dfTotal, DWORD dfWidth)
{
  if (dfValue==0)
      return 0;
  return MulDiv(dfValue , dfWidth , dfTotal);
/*
  if ((dfTotal==0))
      return 0;

  if ((((dfValue * dfWidth) / dfValue) != dfWidth) &&
      (dfTotal > dfWidth))
  {
    return ((dfValue) / (dfTotal / dfWidth));
  }
  else
  {
    return ((dfValue * dfWidth) / dfTotal);
  }*/
}




#ifndef ULONGLONG
typedef unsigned __int64 ULONGLONG;
#endif
void MyDoAdd64(LARGE_INTEGER &R,LARGE_INTEGER A,LARGE_INTEGER B)
{

    R.LowPart = A.LowPart + B.LowPart;
    R.HighPart = A.HighPart + B.HighPart;
    if ((R.LowPart<A.LowPart) || (R.LowPart<B.LowPart))
        R.HighPart++;
}

void MyDoMul64(LARGE_INTEGER &R,LARGE_INTEGER A,LARGE_INTEGER B)
{
    DWORD dw;
    ULONGLONG ull;
    dw= (A.LowPart*B.HighPart) + (B.LowPart*A.HighPart);
    ull = UInt32x32To64(A.LowPart,B.LowPart) ;
    R = *((LARGE_INTEGER*)&ull);
    R.HighPart += dw;
}

void MyDoMinus64(LARGE_INTEGER &R,LARGE_INTEGER A,LARGE_INTEGER B)
{
    R.HighPart = A.HighPart - B.HighPart;
    if (A.LowPart >= B.LowPart)
        R.LowPart = A.LowPart - B.LowPart;
    else
    {
        R.LowPart = A.LowPart - B.LowPart;
        R.HighPart --;
    }
}

#ifdef USEARITH64CPLX
DWORD GivePctDivide64(DWORD dwDoneLow_,DWORD dwDoneHigh_,DWORD dwAllLow_,DWORD dwAllHigh_)
{
    unsigned __int64 done,all;
    done = dwDoneLow_ | (((unsigned __int64)dwDoneHigh_)<<32);
    all = dwAllLow_ | (((unsigned __int64)dwAllHigh_)<<32);

    DWORD dwPct;

    if ((done<=((0x0ffffffffffUL)/100)) || (all<100))
    {
        if (all!=0)
            dwPct=(DWORD)((done*100)/all);
        else
            dwPct=1;
    }
    else
        dwPct=(DWORD)(done/(all/100));
    return dwPct;
}

#else
DWORD GivePctDivide64(DWORD dwDoneLow_,DWORD dwDoneHigh_,DWORD dwAllLow_,DWORD dwAllHigh_)
{
    DWORD dwNewPct=0;
            if ((dwAllHigh_==0) && (dwAllLow_ < (0xf000000 / 100)))
            {
                if ((dwAllLow_)>0)
                {
                    dwNewPct=MulDiv(dwDoneLow_,100,dwAllLow_);
                }
            }
            else
            {
                DWORD dwTotalMid = (dwDoneHigh_ << 16) | (dwDoneLow_ >> 16);
                DWORD dwSizeTotalMid = (dwAllHigh_ << 16) | (dwAllLow_ >> 16);
                if (dwSizeTotalMid < (0xf000000 / 100))
                {
                    if ((dwSizeTotalMid)>0)
                    {
                        dwNewPct=(dwTotalMid*100)/(dwSizeTotalMid);
                    }
                }
                else
                {
                    if (dwAllHigh_>0)
                      dwNewPct=(dwDoneHigh_ * 100)/dwAllHigh_;
                }
            }
    return dwNewPct;
}
#endif

void GetCurDateTime(WORD &wDosDate,WORD &wDosTime)
{
#ifdef WIN32
SYSTEMTIME st;
FILETIME ft,ftLocal;
   GetSystemTime(&st);
   SystemTimeToFileTime(&st,&ft);
   FileTimeToLocalFileTime(&ft,&ftLocal);
   FileTimeToDosDateTime(&ftLocal,&wDosDate,&wDosTime);
#else
#ifndef _dosdate_t
#define _dosdate_t dosdate_t
#define _dostime_t dostime_t
#endif
struct _dosdate_t dd;
struct _dostime_t dt;
  _dos_getdate(&dd);
  _dos_gettime(&dt);
  wDosDate = dd.day + (32 * dd.month) + (512 * (dd.year-1980));
  wDosTime = (dt.second/2) + (32* dt.minute) + (2048 * dt.hour);
#endif
}

BOOL TimeBetweenFileTime(FILETIME ft1,FILETIME ft2,LPDWORD lpdwDay,LPDWORD lpdwSec)
{
    FILETIME ftDif;
    DWORD dwNbSec;
    DWORD dwLow,dwHigh;
    DWORD dwDay,dwSec;
    BOOL fRet;
    // The FILETIME structure is a 64-bit value representing the number
    //    of 100-nanosecond intervals since January 1, 1601 (UTC).


    if (CompareValue64(ft1.dwLowDateTime,ft1.dwHighDateTime,ft2.dwLowDateTime,ft2.dwHighDateTime)==1)
    {
        dwDay=dwSec=0;
        fRet = FALSE;
    }
    else
    {
        ftDif=ft2;

        SubValueFrom64(ftDif.dwLowDateTime,ftDif.dwHighDateTime,ft1.dwLowDateTime,ft1.dwHighDateTime);

        dwLow=(ftDif.dwLowDateTime >> 7) | (ftDif.dwHighDateTime << (32-7));
        dwHigh=ftDif.dwHighDateTime>>(7);

        dwNbSec=0;

        while (dwHigh>0x80)
        {
            DWORD dwMulLow,dwMulHigh;
            dwMulLow = 78125;
            dwMulHigh=0;
            Multiply64(dwMulLow,dwMulHigh,0x800000UL,0);
            SubValueFrom64(dwLow,dwHigh,dwMulLow,dwMulHigh);
            dwNbSec+=0x800000UL;
        }

        while (dwHigh>0)
        {
            SubValueFrom64(dwLow,dwHigh,(78125UL*0x8000UL),0);
            dwNbSec+=0x8000;
        }
        dwNbSec+=dwLow/ 78125;


        dwDay = dwNbSec/(3600*24);
        dwSec= dwNbSec - (dwDay * 3600*24);
        fRet=TRUE;
    }

    if (lpdwDay!=NULL)
        *lpdwDay=dwDay;
    if (lpdwSec!=NULL)
        *lpdwSec=dwSec;
    return fRet;
}

LPTSTR Date_DOS2Text(WORD wDosDate,LPTSTR lpBuf,BOOL fLongYear)
{
SYSTEMTIME st;

  if (wDosDate==0)
  {
      *lpBuf=0;
  }
  else
  {
      memset(&st,0,sizeof(st));
      st.wDay = wDosDate & 31 ;
      st.wMonth= (wDosDate & 0x1E0)/0x20 ;
      st.wYear  = ((wDosDate & 0x0FE00)/0x0200)+80 ;
      st.wYear  += 1900;

      GetDateFormat(LOCALE_USER_DEFAULT,DATE_SHORTDATE,&st,NULL,lpBuf,255);
  }
  return lpBuf;
}

LPTSTR Time_DOS2Text(WORD wDosTime,LPTSTR lpBuf)
{
SYSTEMTIME st;
 if (wDosTime==0)
  {
      *lpBuf=0;
  }
  else
  {
  memset(&st,0,sizeof(st));
  st.wHour = (wDosTime & 0xF800)/0x800 ;
  st.wMinute=(wDosTime & 0x7E0)/0x20 ;
  st.wSecond=2* (wDosTime & 31);
  GetTimeFormat(LOCALE_USER_DEFAULT,0,&st,NULL,lpBuf,255);
  }

  return lpBuf;
}


BOOL GetWin32ErrorMessage(DWORD dwErr, LPTSTR lpBuffer, DWORD nSize,BOOL fAppend)
{
    DWORD dwRetFmt;
    if (fAppend)
    {
        DWORD dwLen = lstrlen(lpBuffer);
        if (dwLen>=nSize)
            return FALSE;
        nSize -= dwLen;
        lpBuffer += dwLen;
    }
    dwRetFmt= (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM ,NULL,dwErr,0,
                      lpBuffer,nSize,
                      NULL) );
    return (dwRetFmt!=0);
}

void BeginCountPerfCounter(LARGE_INTEGER * pbeginTime64)
{
    if (!QueryPerformanceCounter(pbeginTime64))
    {
        pbeginTime64->LowPart = GetTickCount();
        pbeginTime64->HighPart = 0;
    }
}

DWORD GetMsecSincePerfCounter(LARGE_INTEGER beginTime64)
{
    LARGE_INTEGER endTime64,ticksPerSecond,ticks;
    DWORDLONG ticksShifted,tickSecShifted;
    DWORD dwLog=16+0;
    DWORD dwRet;
    if (!QueryPerformanceCounter(&endTime64))
        dwRet = GetTickCount() - beginTime64.LowPart;
    else
    {
        MyDoMinus64(ticks,endTime64,beginTime64);
        QueryPerformanceFrequency(&ticksPerSecond);


        {
            ticksShifted = Int64ShrlMod32(*(DWORDLONG*)&ticks,dwLog);
            tickSecShifted = Int64ShrlMod32(*(DWORDLONG*)&ticksPerSecond,dwLog);

        }

        dwRet = (DWORD)((((DWORD)ticksShifted)*1000)/(DWORD)(tickSecShifted));
    }
    return dwRet;
}
#endif
/**********************/
LPCSTR CopyStrWord(LPCSTR lpSrc,LPSTR lpDest,DWORD dwMaxSize)
{
BOOL fInQuote=FALSE;
DWORD dwDestPos = 0;

  *lpDest = '\0';

  if (lpSrc==NULL)
      return NULL;

  while ((*lpSrc) == ' ')
      lpSrc++;

  while (((*lpSrc) != '\0') && ((dwMaxSize==0) || (dwDestPos<dwMaxSize)))
    {
      while ((*lpSrc) == '"')
        {
          fInQuote = !fInQuote;
          lpSrc++;
        }

      if (((*lpSrc) == ' ') && (!fInQuote))
      {
          while ((*lpSrc) == ' ')
              lpSrc++;
          return lpSrc;
      }


      *lpDest = *lpSrc;
      lpDest++;
      dwDestPos++;
      *lpDest = '\0';
      lpSrc++;
    }
  return lpSrc;
}

/*
BOOL MyGetDiskFreeSpaceEx(
  LPCTSTR lpDirectoryName,  // pointer to directory name on disk of
                            // interest
  PULARGE_INTEGER lpFreeBytesAvailableToCaller,
                            // pointer to variable to receive free
                            // bytes on disk available to the caller
  PULARGE_INTEGER lpTotalNumberOfBytes,
                            // pointer to variable to receive number
                            // of bytes on disk
  PULARGE_INTEGER lpTotalNumberOfFreeBytes)
                            // pointer to variable to receive free
                            // bytes on disk)
{
    typedef BOOL (WINAPI* tGetDiskFreeSpaceEx)(
  LPCTSTR lpDirectoryName,  // pointer to directory name on disk of
                            // interest
  PULARGE_INTEGER lpFreeBytesAvailableToCaller,
                            // pointer to variable to receive free
                            // bytes on disk available to the caller
  PULARGE_INTEGER lpTotalNumberOfBytes,
                            // pointer to variable to receive number
                            // of bytes on disk
  PULARGE_INTEGER lpTotalNumberOfFreeBytes);
                            // pointer to variable to receive free
                            // bytes on disk
    static tGetDiskFreeSpaceEx fncGetDiskFreeSpaceEx= NULL;
    static BOOL fTried=FALSE;
    BOOL fRet=FALSE;

    if (fTried==FALSE)
    {
        HMODULE hModule = GetModuleHandle("KERNEL32.DLL");
        if (hModule!=NULL)
            fncGetDiskFreeSpaceEx = (tGetDiskFreeSpaceEx)
                        GetProcAddress(hModule,"GetDiskFreeSpaceEx");
        fTried=TRUE;
    }

    if (fncGetDiskFreeSpaceEx != NULL)
        fRet = (*fncGetDiskFreeSpaceEx)(lpDirectoryName,lpFreeBytesAvailableToCaller,lpTotalNumberOfBytes,lpTotalNumberOfFreeBytes);

    if (!fRet) // we try the old GetDiskFreeSpace
    {
        DWORD dwSectorPerCluster=0;
        DWORD dwBytesPerSector=0;
        DWORD dwFreeCluster=0;
        DWORD dwCluster=0;
        fRet = GetDiskFreeSpace(lpDirectoryName,&dwSectorPerCluster,
                      &dwBytesPerSector,&dwFreeCluster,&dwCluster);
        if (fRet)
        {
            lpTotalNumberOfBytes->QuadPart = dwCluster*((LONGLONG)dwBytesPerSector*dwSectorPerCluster);
            lpFreeBytesAvailableToCaller->QuadPart = lpTotalNumberOfFreeBytes->QuadPart =
                dwFreeCluster*((LONGLONG)dwBytesPerSector*dwSectorPerCluster);
        }

    }
    return fRet ;
}
*/



BOOL MyGetDiskFreeSpaceEx(
  LPCTSTR lpDirectoryName,  // pointer to directory name on disk of
                            // interest
  PULARGE_INTEGER lpFreeBytesAvailableToCaller,
                            // pointer to variable to receive free
                            // bytes on disk available to the caller
  PULARGE_INTEGER lpTotalNumberOfBytes,
                            // pointer to variable to receive number
                            // of bytes on disk
  PULARGE_INTEGER lpTotalNumberOfFreeBytes)
                            // pointer to variable to receive free
                            // bytes on disk)
{
    typedef BOOL (WINAPI* tGetDiskFreeSpaceEx)(
  LPCTSTR lpDirectoryName,  // pointer to directory name on disk of
                            // interest
  PULARGE_INTEGER lpFreeBytesAvailableToCaller,
                            // pointer to variable to receive free
                            // bytes on disk available to the caller
  PULARGE_INTEGER lpTotalNumberOfBytes,
                            // pointer to variable to receive number
                            // of bytes on disk
  PULARGE_INTEGER lpTotalNumberOfFreeBytes);
                            // pointer to variable to receive free
                            // bytes on disk
    static tGetDiskFreeSpaceEx fncGetDiskFreeSpaceEx= NULL;
    static BOOL fTried=FALSE;
    BOOL fRet=FALSE;

    if (fTried==FALSE)
    {
        HMODULE hModule = GetModuleHandle(  _T("KERNEL32.DLL"));

        #ifdef UNICODE
        if (hModule!=NULL)
            fncGetDiskFreeSpaceEx = (tGetDiskFreeSpaceEx)
                        GetProcAddress(hModule,"GetDiskFreeSpaceExW");
        #else
        if (hModule!=NULL)
            fncGetDiskFreeSpaceEx = (tGetDiskFreeSpaceEx)
                        GetProcAddress(hModule,"GetDiskFreeSpaceExA");
        #endif
        fTried=TRUE;
    }

    if (fncGetDiskFreeSpaceEx != NULL)
        fRet = (*fncGetDiskFreeSpaceEx)(lpDirectoryName,lpFreeBytesAvailableToCaller,lpTotalNumberOfBytes,lpTotalNumberOfFreeBytes);

    if (!fRet) // we try the old GetDiskFreeSpace
    {
        DWORD dwSectorPerCluster=0;
        DWORD dwBytesPerSector=0;
        DWORD dwFreeCluster=0;
        DWORD dwCluster=0;
        fRet = GetDiskFreeSpace(lpDirectoryName,&dwSectorPerCluster,
                      &dwBytesPerSector,&dwFreeCluster,&dwCluster);
        if (fRet)
        {
            /*
            lpTotalNumberOfBytes->QuadPart = dwCluster*((LONGLONG)dwBytesPerSector*dwSectorPerCluster);
            lpFreeBytesAvailableToCaller->QuadPart = lpTotalNumberOfFreeBytes->QuadPart =
                dwFreeCluster*((LONGLONG)dwBytesPerSector*dwSectorPerCluster);
                */
            lpTotalNumberOfBytes->LowPart = dwCluster;
            lpTotalNumberOfBytes->HighPart = 0;
            Multiply64(lpTotalNumberOfBytes->LowPart,lpTotalNumberOfBytes->HighPart,dwBytesPerSector,0);
            Multiply64(lpTotalNumberOfBytes->LowPart,lpTotalNumberOfBytes->HighPart,dwSectorPerCluster,0);

            lpTotalNumberOfFreeBytes->LowPart = dwFreeCluster;
            lpTotalNumberOfFreeBytes->HighPart = 0;
            Multiply64(lpTotalNumberOfFreeBytes->LowPart,lpTotalNumberOfFreeBytes->HighPart,dwBytesPerSector,0);
            Multiply64(lpTotalNumberOfFreeBytes->LowPart,lpTotalNumberOfFreeBytes->HighPart,dwSectorPerCluster,0);
            lpFreeBytesAvailableToCaller->QuadPart = lpTotalNumberOfFreeBytes->QuadPart ;
        }

    }
    return fRet ;
}



BOOL CheckIfEmpty(BYTE bFloppy)
{
  TCHAR szLect[50];
  DWORD dwSectorPetCluster,dwBytesPerStector,dwFreeCluster,dwCluster;
  dwSectorPetCluster=dwBytesPerStector=dwFreeCluster=dwCluster=0;

  if ((bFloppy>='a') && (bFloppy<='z'))
      bFloppy-='a';
  if ((bFloppy>='A') && (bFloppy<='Z'))
      bFloppy-='A';

  wsprintf(szLect,_T("%c:"),'A'+bFloppy);
  // uiOldError =  //SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOALIGNMENTFAULTEXCEPT);

  UINT uiOldError = SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOALIGNMENTFAULTEXCEPT);

  BOOL fRet = GetDiskFreeSpace(szLect,&dwSectorPetCluster,
                          &dwBytesPerStector,&dwFreeCluster,&dwCluster);
  SetErrorMode(uiOldError);


  BOOL fEmpty = FALSE;
  if (fRet)
      fEmpty= !(dwCluster>dwFreeCluster);
  return fEmpty;
}

BOOL GetFloppyFromLine(LPSTR lpLine,BYTE &bNewFl)
{
    bNewFl=0;
    if (((*(lpLine+1)) == ':') && ((*(lpLine+2)) == '\0'))
      {
      //BOOL fRes;
      char c=*lpLine;
        if (c >= 'a')
          bNewFl = (BYTE)(c - 'a');
        else
          bNewFl = (BYTE)(c - 'A');

        if (bNewFl > 25)
          return FALSE;
        /*
        if (GetFloppyInfo(bNewFl)==0)
          return FALSE;

        bCurrentFloppy = (BYTE)bNewFl;
        CheckMiscMenuItem(hMenu);*/
        return TRUE;
      }
    return FALSE;
}


void GetDirectoryOfFileName(LPTSTR lpDest,LPTSTR lpFn)
{
UINT uiLen = (UINT)(GetNameWithoutPath(lpFn) - lpFn);
  hmemcpy(lpDest,lpFn,uiLen);
  if (uiLen > 3)
  {
      LPTSTR lpEnd = GetLatestChar(lpDest);
      if (lpEnd!=NULL)
          if ((*lpEnd)=='\\')
              uiLen--;
  }
  *(lpDest+uiLen)='\0';
}

LPTSTR AddNameInPathN(LPTSTR lpDest,DWORD dwLenDest,LPTSTR lpName,LPCTSTR lpOrgRep)
{
    DWORD iln;
    if (dwLenDest==0)
        return lpDest;

    if (lpOrgRep != NULL)
    {
        DWORD ilnOrg = lstrlen(lpOrgRep);
        DWORD iDoCopy = min(ilnOrg,dwLenDest-1);
        hmemcpy(lpDest,lpOrgRep,sizeof(TCHAR)*iDoCopy);
        *(lpDest+iDoCopy)=0;
        //lstrcpy(lpDest,lpOrgRep);
    }
    iln = lstrlen(lpDest);
    if ((iln != 0) && (iln<(dwLenDest-1)))
    {
        LPTSTR lpEnd = GetLatestChar(lpDest);
        if (lpEnd!=NULL)
        {
            TCHAR c = *(lpEnd);
            if ((c != ':') && (c != '\\'))
            {
                *(lpDest+iln) = '\\';
                iln++;
            }
        }
    }

    if ((dwLenDest - iln)>1)
    {
        LPTSTR lpDestNow = lpDest+iln;
        DWORD dwLenDestNow = dwLenDest - iln;

        DWORD ilnName = lstrlen(lpName);
        DWORD iDoCopy = min(ilnName,dwLenDestNow-1);
        hmemcpy(lpDestNow,lpName,sizeof(TCHAR)*iDoCopy);
        *(lpDestNow+iDoCopy)=0;
        //lstrcpy(lpDest+iln,lpName);
    }

    return lpDest;
}

BOOL IsFileNameHasExtension(LPCTSTR lpFn)
{
LPCTSTR lpNameWithoutPath;
int i,iln;

  lpNameWithoutPath = GetNameWithoutPath(lpFn);
  iln = lstrlen(lpNameWithoutPath);

  for (i=0;i<iln;i++)
    if ((*(lpNameWithoutPath + i)) == '.') return TRUE;
  return FALSE;
}

/**************************************************************************/

LPTSTR GetLatestChar(LPTSTR lpText)
{
    LPTSTR lpNext,lpNextNext,lpRet;
    lpRet = NULL;
    lpNext = lpText;

    if (lpText==NULL)
        return NULL;

    for (;;)
    {
        lpNextNext = CharNext(lpNext);
        if (lpNextNext == lpNext)
            return lpRet;
        else
        {
            lpRet = lpNext;
            lpNext = lpNextNext;
        }
    }
}

BOOL CreatePathIfNeeded(LPCTSTR lpPath)
{
TCHAR szPath[MAX_PATH+128];
int iln;
LPTSTR lpEndPath;
  if ((*lpPath) == '\0') return FALSE;
  iln = lstrlen(lpPath);
  if (iln == 2)
    if ((*(lpPath+1)) == ':') return FALSE;

  lstrcpy(szPath,lpPath);

  lpEndPath=GetLatestChar(szPath);
  if (lpEndPath!=NULL)
      if ((*lpEndPath)=='\\')
    {
      iln--;
      szPath[iln] = '\0';
    }

  return (MyMkdir(szPath));
}


BOOL CheckExistFile(LPCTSTR lpFn)
{
HANDLE hFile;
  hFile = CreateFile(lpFn,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,NULL);
  if ((hFile==NULL) || (hFile==INVALID_HANDLE_VALUE))
      return FALSE;
  CloseHandle(hFile);
  return TRUE;
}

static BOOL IsNumber(TCHAR c)
{
  return ((c>='0') && (c<='9'));
}


BOOL IncNumberInName(LPTSTR lpFn,BOOL fCheckExist,BOOL fNeedAbsolute,DWORD dwStepIncr)
{
int i,iBegName,iPoint,iEndName,iln,iBegChif,iEndChif;
BOOL fFoundChif=FALSE;
BOOL fForce=FALSE;
BOOL fInExt;
BOOL fExpand;
    iBegName = 0;
    iEndChif = 0;
    fInExt = FALSE;

    lpFn = (LPTSTR)GetNameWithoutPath(lpFn);
    iEndName = iln = lstrlen(lpFn);

    if (iln == 0) return FALSE;
    for (i=iln-1;i>=0;i--)
      {
      if ((*(lpFn+i)) == '.') iEndName = i;
      }
    iPoint = iEndName;

    //if ((!fFoundChif) && ((*(lpFn+iPoint)) == '.'))
    if ((*(lpFn+iPoint)) == '.') // search number in ext if there is ext
      {
      iBegName = iPoint + 1;
      iEndName = iln;
      for (i=iEndName-1;i>=iBegName;i--)
        if (IsNumber(*(lpFn+i)))
        {
          for (iBegChif=iEndChif=i;iBegChif>0;iBegChif--)
            if (!IsNumber(*(lpFn+iBegChif-1))) break;
          break;
        }
      fInExt = fFoundChif = (i>=iBegName) ;
      }

    if (!fFoundChif)
      {
      iBegName = 0 ;
      iEndName = iPoint;
      for (i=iEndName-1;i>=iBegName;i--)
        if (IsNumber(*(lpFn+i)))
          {
            for (iBegChif=iEndChif=i;iBegChif>0;iBegChif--)
              if (!IsNumber(*(lpFn+iBegChif-1))) break;
            break;
          }
      fFoundChif = (i>=iBegName) ;
      }

    if ((!fFoundChif) && fNeedAbsolute)
      {
      fForce = TRUE;
      if (iPoint > 7)
        {
          iBegChif = iEndChif = 7 ;
          fExpand = FALSE;
        }
       else
        {
          iBegChif = iPoint;
          iEndChif = iBegChif -1;
          fExpand = TRUE;
        }
      }

    if (fFoundChif || fForce)
      {
      DWORD dwNb;
      TCHAR szBuf[32];
      TCHAR szFmt[16];
      int iLenChif;

      int j;
    if (!fForce)
      {
      fExpand = TRUE;
      for (j=iBegChif;j<=iEndChif;j++)
        if (*(lpFn+j) != '9') fExpand = FALSE;
      if ((iEndName-iBegName > (fInExt ? 2 : 7)) && (fExpand))
        {
        fExpand = FALSE;
        if (((iBegChif > 0) && (!fInExt)) || ((iBegChif > (iPoint+2)) && (fInExt)))
          {
        iBegChif --;
        *(lpFn+iBegChif) = '0';
          }
        }
      }
    if (fExpand)
      {
        for (j = iln;j>iEndChif;j--)
          * (lpFn+j+1) = * (lpFn+j);
        iEndChif++;
      }

    iLenChif=iEndChif+1-iBegChif;
    hmemcpy(szBuf,lpFn+iBegChif,iLenChif);
    szBuf[iLenChif] = '\0';
    if (fForce) dwNb = 1 ;
    else
#ifdef UNICODE
        dwNb = _wtol(szBuf) ;
#else
        dwNb = atol(szBuf) ;
#endif
    dwNb+=dwStepIncr;
    wsprintf(szFmt,_T("%%0%ulu"),iLenChif);
    wsprintf(szBuf,szFmt,dwNb);
    hmemcpy(((LPSTR)lpFn)+iBegChif,szBuf,iLenChif);
    if (!fCheckExist) return TRUE;
             else return CheckExistFile(lpFn);
      }
  return FALSE;
}


BOOL PrepareForInc(LPTSTR lpFn)
{
    return TRUE;
}

BOOL ResizeStaticCtlSize(HWND hCtl,BOOL fAlignRight,BOOL fAlignBottom,SIZE sz)
{
    LONG lX,lY,lCx,lCy;
    GetWindowPos(hCtl,&lX,&lY,&lCx,&lCy);
    if ((!fAlignRight) && (!fAlignBottom))
        SetWindowPos(hCtl,NULL,0,0,sz.cx,sz.cy,SWP_NOMOVE | SWP_NOZORDER);
    else
        SetWindowPos(hCtl,NULL,
              fAlignRight ? (lX+lCx-sz.cx):lX,
              fAlignBottom ? (lY+lCy-sz.cy):lY,
              sz.cx,sz.cy,SWP_NOZORDER);
    return TRUE;
}

BOOL GetSizeForTextInCtl(HWND hCtl,LPCTSTR lpszTxt,LPSIZE lpsz)
{
    HFONT hOldFont;
    HDC hDC=GetDC(hCtl);
    TCHAR szTxt[MAX_PATH+0x200];
    if (lpszTxt==NULL)
    {
      GetWindowText(hCtl,szTxt,sizeof(szTxt)-1);
      lpszTxt=szTxt;
    }

    hOldFont=SelectFont(hDC,GetWindowFont(hCtl));

    GetTextExtentPoint32(hDC,lpszTxt,lstrlen(lpszTxt),lpsz);
    ReleaseDC(hCtl,hDC);
    return TRUE;
}

BOOL GetSizeForUiString(HWND hCtl,UINT uiRes,LPSIZE lpsz)
{
    TCHAR szTxt[MAX_PATH+0x200];
    LPCTSTR lpszTxt=NULL;
    szTxt[0]=0;
    if (uiRes!=0)
    {
        LoadInternatString(uiRes,szTxt,sizeof(szTxt)-1);
        lpszTxt=szTxt;
    }
    return GetSizeForTextInCtl(hCtl,lpszTxt,lpsz);
}


BOOL ResizeStaticCtl(HWND hCtl,BOOL fAlignRight,BOOL fAlignBottom)
{
    SIZE sz;

    GetSizeForTextInCtl(hCtl,NULL,&sz);
    return ResizeStaticCtlSize(hCtl,fAlignRight,fAlignBottom,sz);
}

BOOL ResizeStaticCtlForMaxUiString(HWND hCtl,LPUINT lpuiString,BOOL fAlignRight,BOOL fAlignBottom)
{
    SIZE maxsz;
    maxsz.cx=maxsz.cy=0;
    while ((*lpuiString)!=0)
    {
        SIZE sz;
        GetSizeForUiString(hCtl,*lpuiString,&sz);
        maxsz.cx=max(maxsz.cx,sz.cx);
        maxsz.cy=max(maxsz.cy,sz.cy);
        lpuiString++;
    }
    return ResizeStaticCtlSize(hCtl,fAlignRight,fAlignBottom,maxsz);
}

BOOL GetWindowPos(HWND hWnd,long* lpdwX,long* lpdwY,long* lpdwCx,long* lpdwCy)
{
    RECT rect;
    POINT pt;
    long dwX,dwY,dwCx,dwCy;
    GetWindowRect(hWnd,&rect);
    pt.x = 0;
    pt.y = 0;
    ClientToScreen(GetParent(hWnd),&pt);
    dwX = rect.left-pt.x;
    dwY = rect.top-pt.y;
    dwCx = rect.right-rect.left;
    dwCy = rect.bottom-rect.top;

    if (lpdwX!=NULL)
        *lpdwX=dwX;

    if (lpdwY!=NULL)
        *lpdwY=dwY;

    if (lpdwCx!=NULL)
        *lpdwCx=dwCx;

    if (lpdwCy!=NULL)
        *lpdwCy=dwCy;

    return TRUE;
}
/**************************************************************************/

// --------------------------------------------------------------------------
//  FUNCTION    CenterWindow
//  PURPOSE     center window relative to its parent (or to screen)
// --------------------------------------------------------------------------
//  INPUT       hwnd            window handle
//
//  OUTPUT      void            ...
// --------------------------------------------------------------------------
//  COMMENTS    designed to be called during window WM_INITDIALOG or
//              WM_CREATE handling. modified April 3rd 1993
// --------------------------------------------------------------------------

POINT GetPtCenterWin(HWND hwnd)
{
  HWND  hwndParent;
  RECT  r, rParent;
  POINT pt;
  int   nSMCX, nSMCY, nwndcx, nwndcy;

  GetWindowRect(hwnd, &r);
  nSMCX  = GetSystemMetrics(SM_CXSCREEN);
  nSMCY  = GetSystemMetrics(SM_CYSCREEN);
  nwndcx = r.right - r.left;
  nwndcy = r.bottom - r.top;

  if ((hwndParent = GetParent(hwnd))!=NULL)
  {
    GetWindowRect(hwndParent, &rParent);
    pt.x = rParent.left + (rParent.right - rParent.left - nwndcx) / 2;
    pt.y = rParent.top  + (rParent.bottom - rParent.top - nwndcy) / 2;

    // make sure window is within screen bounds in any case
    if(pt.x < 0)                pt.x = 0;
    if((pt.x + nwndcx) > nSMCX) pt.x = nSMCX - nwndcx;
    if(pt.y < 0)                pt.y = 0;
    if((pt.y + nwndcy) > nSMCY) pt.y = nSMCY - nwndcy;
  }
  else  // no parent so center window relative to screen
  {
    pt.x = (GetSystemMetrics(SM_CXSCREEN) - (r.right - r.left)) / 2;
    pt.y = (GetSystemMetrics(SM_CYSCREEN) - (r.bottom - r.top)) / 2;
  }
  return pt;
}

void CenterWindow(HWND hwnd)
{
POINT pt;
  pt = GetPtCenterWin(hwnd);
  SetWindowPos(hwnd, NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOREDRAW);
}


void CenterChild(HWND hWnd)
{
  CenterWindow(hWnd);
}


BOOL IsParentOrGrandParent(HWND hWndWin,HWND hWndCanPar)
{
    HWND hWndParc=hWndWin;
    while (hWndParc!=NULL)
    {
        if (hWndParc==hWndCanPar)
            return TRUE;
        hWndParc=GetParent(hWndParc);
    }
    return FALSE;
}


BOOL HideDialogInInit(HWND hDlg)
{
    ShowWindow(hDlg,SW_HIDE);
    SetWindowLongPtr(hDlg,DWLP_USER,0);
    SetWindowPos(hDlg, NULL, GetSystemMetrics(SM_CXSCREEN)+1,
                    GetSystemMetrics(SM_CYSCREEN)+1,
                    0, 0, // to be really invisible!
                    SWP_NOSIZE | SWP_NOZORDER | SWP_NOREDRAW);
    return TRUE;
}

void CnvMin(LPTSTR lpTxt)
{
  if (lpTxt == NULL) return ;
  while ((*lpTxt) != '\0')
    {
       if (((*lpTxt)>= 'A') && ((*lpTxt)<= 'Z')) (*lpTxt) += 0x20;
       lpTxt++;
    }
}

void CnvMaj(LPTSTR lpTxt)
{
  if (lpTxt == NULL) return ;
  while ((*lpTxt) != '\0')
    {
       if (((*lpTxt)>= 'a') && ((*lpTxt)<= 'z')) (*lpTxt) -= 0x20;
       lpTxt++;
    }
}

void RemoveSpace(LPTSTR lpPtr)
{
UINT i = lstrlen(lpPtr);
  while (i>0)
    if (*(lpPtr+(--i)) == ' ') *(lpPtr+i)='\0';
     else return;
}

LPCTSTR GetNameWithoutPath(LPCTSTR lpFn)
{
LPCTSTR lpRet = lpFn ;
LPCTSTR lpNext ;
  while ((*lpFn) != '\0')
    {
      if (((*lpFn) == ':') || ((*lpFn) == '\\')) lpRet = lpFn + 1;
      lpNext=CharNext(lpFn);
      lpFn = lpNext;
    }
  return lpRet;
}


BOOL ChangeFileNameExtension(LPTSTR lpFn,LPCTSTR lpExt)
{
    LPTSTR lpLastDot=NULL;
    while ((*lpFn) != '\0')
    {
        if (((*lpFn) == ':') || ((*lpFn) == '\\'))
            lpLastDot=NULL;
        if (((*lpFn) == '.'))
            lpLastDot=lpFn;
        lpFn=CharNext(lpFn);
    }
    if (lpLastDot==NULL)
        lstrcat(lpFn,lpExt);
    else
        lstrcpy(lpLastDot,lpExt);
    return TRUE;
}

HANDLE MyCreateFileWithModifiedExtension(LPCTSTR lpFn,LPCTSTR lpExt,
                                         DWORD dwDesiredAccess,
                                         DWORD dwShareMode,
                                         LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                         DWORD dwCreationDisposition,
                                         DWORD dwFlagsAndAttributes,
                                         HANDLE hTemplateFile)
{
    DWORD dwSizeAllocString = ((lstrlen(lpFn) + lstrlen(lpExt) + 0x10)) * sizeof(TCHAR);
    LPTSTR lpszModifedName = (LPTSTR)GlobalAllocPtr(GMEM_MOVEABLE,dwSizeAllocString);
    HANDLE hRet;
    if (lpszModifedName == NULL)
        return INVALID_HANDLE_VALUE;
    lstrcpy(lpszModifedName,lpFn);
    if (lpExt!=NULL)
        ChangeFileNameExtension(lpszModifedName,lpExt);
    hRet = CreateFile(lpszModifedName,
                         dwDesiredAccess,
                         dwShareMode,
                         lpSecurityAttributes,
                         dwCreationDisposition,
                         dwFlagsAndAttributes,
                         hTemplateFile);
    GlobalFreePtr(lpszModifedName);
    return hRet;
}

#ifdef WIN32
DWORD MyGetTempPath(DWORD nBufferLength,
                    LPTSTR lpBuffer)
{
    DWORD dwR ;
    //dwR= GetTempPath(nBufferLength,lpBuffer);
    dwR= GetWindowsDirectory(lpBuffer,nBufferLength);
    if ((dwR<nBufferLength) && (dwR>0))
    {
        LPTSTR lpEndBuf = GetLatestChar(lpBuffer);
        if (lpEndBuf!=NULL)
        {
            TCHAR c=*lpEndBuf;
            if ((c!=':') && (c!='\\'))
            {
                lstrcat(lpBuffer,_T("\\"));
                dwR++;
            }
        }
    }
    return dwR;
}
#endif


BOOL CheckExtension(LPCTSTR lpFn,LPSTR lpExt)
{
int iln=lstrlen(lpFn);
  if (iln<4)
      return FALSE;
  return ((lpFn[iln-4]=='.') &&
      ((lpFn[iln-3]==lpExt[0]) || (lpFn[iln-3]==(lpExt[0]+0x20))) &&
      ((lpFn[iln-2]==lpExt[1]) || (lpFn[iln-2]==(lpExt[1]+0x20))) &&
      ((lpFn[iln-1]==lpExt[2]) || (lpFn[iln-1]==(lpExt[2]+0x20))));
}

BOOL OlderTime(DWORD & dwTime)
{
DWORD dwNow = GetTickCount();
  if ((dwNow - dwTime) < 200) return FALSE;
  dwTime = dwNow;
  return TRUE;
}


void TrackFileName(LPCTSTR lpOrigFn,LPTSTR lpDestFn,int iMaxSize)
{
int iLn = lstrlen(lpOrigFn);
  if (iLn <= iMaxSize)
    {
      lstrcpy(lpDestFn,lpOrigFn);
      return;
    }

  *lpDestFn = *lpOrigFn;
  lstrcpy(lpDestFn+1,_T(":\\..."));
  lstrcpy(lpDestFn+6,lpOrigFn+iLn+6-iMaxSize);
}

BOOL FillAlignFile(HANDLE hFile,DWORD dwAlign,DWORD dwPos,LPDWORD lpdwWritten)
{
    DWORD dwNbWrite;
    DWORD dwNbWriteDone=0;
    LPCSTR lpBuf;
    BOOL fRet;
    if (lpdwWritten!=NULL)
        *lpdwWritten=0;

    if (dwPos==FILE_POS_UNKNOWN)
    {
        dwPos = SetFilePointer(
            hFile, // must have GENERIC_READ and/or GENERIC_WRITE
            0,     // do not move pointer
            NULL,  // hFile is not large enough to need this pointer
            FILE_CURRENT);
    }
    dwNbWrite = AroundUpper(dwPos,dwAlign)-dwPos;
    if (dwNbWrite==0)
        return TRUE;

    lpBuf = (LPCSTR)GlobalAllocPtr(GHND,dwNbWrite);
    if (lpBuf == NULL)
        return FALSE;

    fRet=WriteFile(hFile,lpBuf,dwNbWrite,&dwNbWriteDone,NULL);
    GlobalFreePtr(lpBuf);
    if ((fRet) && (dwNbWriteDone!=0))
    {
        fRet = dwNbWriteDone == dwNbWrite;
        if (lpdwWritten!=NULL)
            *lpdwWritten=dwNbWriteDone;
    }
    return fRet;
}

/*--------------------------------------------------------*/
// usualy, set hInstLangRes to the instance of resource
//  and wIndexStringRes to 0
static HINSTANCE hInstLangResMU;
static WORD wIndexStringResMU;
static UINT uiCodePage;
HINSTANCE GetMULingResHinst() { return (hInstLangResMU) ;}


void SetMUResourceBase(HINSTANCE hInstLangResNew,WORD wIndexStringResNew)
{
    hInstLangResMU = hInstLangResNew;
    wIndexStringResMU = wIndexStringResNew;
    {
      TCHAR szCodePage[256];
      uiCodePage=CP_ACP;
        #ifndef IDS_CODEPAGE
        #define IDS_CODEPAGE 1000
        #endif

      if (LoadInternatString(IDS_CODEPAGE,szCodePage,sizeof(szCodePage)-1))
#ifdef UNICODE
          uiCodePage = (UINT)_wtoi(szCodePage);
#else
          uiCodePage = (UINT)atoi(szCodePage);
#endif
    }
}


static TCHAR szPrev[16]=_T("");
void SetMUInternTemplate(LPTSTR sz)
{
    lstrcpy(szPrev,sz);
}

LPTSTR GetMUInternTemplate(LPTSTR lpTemp)
{
static TCHAR szTemp[40];
LPTSTR lpReturn;

  lpReturn = szTemp;
  lstrcpy(lpReturn,szPrev);
#ifdef UNICODE
  lstrcat(lpReturn,lpTemp);
#else
  _fstrcat(lpReturn,lpTemp);
#endif
  return lpReturn; // for use once only in call of DialogBox...
}


/*********/

int LoadInternatString(UINT uId,LPTSTR lpBuf,int cch)
{
  if (uId==0)
    {
      *lpBuf='\0';
      return 0;
    }
  else
  {
#ifdef UNICODE
      return LoadString(GetMULingResHinst(),uId+wIndexStringResMU,lpBuf,cch);
#else
      if (!IsUnicodeSupported())
      {
          return LoadString(GetMULingResHinst(),uId+wIndexStringResMU,lpBuf,cch);
      }
      else
      {
          LPWSTR lpwStr;

          lpwStr=(LPWSTR)GlobalAllocPtr(GHND,(cch*2)+0x10);
          LoadStringW(GetMULingResHinst(),uId+wIndexStringResMU,lpwStr,cch);
          WideCharToMultiByte(uiCodePage,0,lpwStr,-1,lpBuf,cch,NULL,NULL);
          GlobalFreePtr(lpwStr);

          return lstrlen(lpBuf);
      }
#endif
  }
}



static LPBEFOREMSG lpBefore=NULL;
static LPAFTERMSG lpAfter=NULL;

void SetBeforeAfterMsgFunc(LPBEFOREMSG lpBeforeSet,
                           LPAFTERMSG lpAfterSet)
{
    lpBefore=lpBeforeSet;
    lpAfter=lpAfterSet;
}

INT_PTR DialogBoxParWithABFunc(HINSTANCE hInstance,LPCTSTR lpTemplateName,
                          HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam)
{
    INT_PTR iRet;
    DWORD dwPar;

    if (lpBefore!=NULL)
        dwPar=(*lpBefore)();

    iRet = MyDialogBoxParam(hInstance,lpTemplateName,
                                    hWndParent,lpDialogFunc,dwInitParam);
    if (lpAfter!=NULL)
        (*lpAfter)(dwPar);
    return (INT_PTR)iRet;
}



/**********************************************************************************/

int MyGetMenuString(HMENU hMenu,UINT uIDItem,LPTSTR lpString,int nMaxCount,UINT uFlag)
{
#ifdef UNICODE
    return GetMenuString(hMenu,uIDItem,lpString,nMaxCount,uFlag);
#else
    if (!IsUnicodeSupported())
    {
        return GetMenuString(hMenu,uIDItem,lpString,nMaxCount,uFlag);
    }
    else
    {
        int iRet;
        WCHAR  wstrMenuString[255];

        iRet = GetMenuStringW(hMenu,uIDItem,wstrMenuString,(sizeof(wstrMenuString)/2)-1,uFlag);
        if (iRet>0)
        {
            WideCharToMultiByte(uiCodePage,0/*WC_DEFAULTCHAR*/,
                        wstrMenuString, -1,
                        lpString,nMaxCount,NULL,NULL);
            iRet=lstrlen(lpString);
        }
                        /*

        WCHAR  wstrTemplateName[255];
        LPWSTR lpwstrTemplateName;
        if (HIWORD((DWORD)(lpTemplateName))==0)
            lpwstrTemplateName=(LPWSTR)lpTemplateName;
        else
        {
            lpwstrTemplateName=(LPWSTR)wstrTemplateName;
            MyMultiByteToWideChar((LPCSTR)lpTemplateName,
                                  (LPWSTR)wstrTemplateName,
                                  sizeof(wstrTemplateName)/2);
        }
        return CreateDialogParamW(hInstance,lpwstrTemplateName,hWndParent,lpDialogFunc,dwInitParam);
        */
        return iRet;
    }
#endif
}

BOOL MyInsertMenu(HMENU hMenu,UINT uPosition,UINT uFlags,UINT_PTR uIDNewItem,LPCTSTR lpNewItem)
{
    if (!IsUnicodeSupported())
    {
        return InsertMenu(hMenu,uPosition,uFlags,uIDNewItem,lpNewItem);
    }
    else
    {
        WCHAR  wstrTemplateName[255];
        LPWSTR lpwstrTemplateName = NULL;
        if (lpNewItem!=NULL)
        {
            lpwstrTemplateName=(LPWSTR)lpNewItem;
            MultiByteToWideChar(uiCodePage,0/*WC_DEFAULTCHAR*/,
                        (LPCSTR)lpNewItem, -1,
                        (LPWSTR)wstrTemplateName,sizeof(wstrTemplateName)/2);
        }
        return InsertMenuW(hMenu,uPosition,uFlags,uIDNewItem,wstrTemplateName);
    }
}

HWND CreateDialogParWithABFunc(HINSTANCE hInstance,LPCTSTR lpTemplateName,
                          HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam)
{
    HWND hWndRet;
    DWORD dwPar;
    if (lpBefore!=NULL)
        dwPar=(*lpBefore)();
    hWndRet = MyCreateDialogParam(hInstance,lpTemplateName,
                                    hWndParent,lpDialogFunc,dwInitParam);
    if (lpAfter!=NULL)
        (*lpAfter)(dwPar);
    return hWndRet;
}

int MessageBoxWithABFuncWithSourceLine( HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType,LPCSTR lpszSourceFileName,DWORD dwLine )
{
    int iRet;
    DWORD dwPar;
    if (lpBefore!=NULL)
        dwPar=(*lpBefore)();
    iRet=MessageBoxWithSourceLine(hWnd,lpText,lpCaption,uType,lpszSourceFileName,dwLine);
    if (lpAfter!=NULL)
        (*lpAfter)(dwPar);
    return iRet;
}

int MessageLoadStringWithSourceLine(HWND hWnd,UINT uiRes,UINT uiTitle,UINT uiSty,LPCSTR lpszSourceFileName,DWORD dwLine)
{
TCHAR szTitle[MAX_PATH];
TCHAR szMsg[1024+MAX_PATH];
LPTSTR lpTitle = szTitle;

  LoadInternatString(uiRes,szMsg,sizeof(szMsg)-1);
  if (uiTitle) LoadInternatString(uiTitle,lpTitle,sizeof(szTitle)-1);
    else lpTitle = NULL;
  return MessageBoxWithABFuncWithSourceLine(hWnd,szMsg,lpTitle,uiSty,lpszSourceFileName,dwLine);
}

int MessageLoadStringTxtParamWithSourceLine(HWND hWnd,UINT uiRes,UINT uiTitle,UINT uiSty,LPCTSTR lpParam,LPCSTR lpszSourceFileName,DWORD dwLine)
{
TCHAR szFmt[MAX_PATH];
TCHAR szMsg[MAX_PATH*4];
TCHAR szTitle[MAX_PATH];
LPTSTR lpTitle = szTitle;

  LoadInternatString(uiRes,szFmt,sizeof(szFmt)-1);
  wsprintf(szMsg,szFmt,lpParam);
  if (uiTitle == NULL)
    return MessageBoxWithSourceLine(hWnd,szMsg,NULL,uiSty,lpszSourceFileName,dwLine);
  if (uiTitle) LoadInternatString(uiTitle,lpTitle,sizeof(szTitle)-1);
    else lpTitle = NULL;
  return MessageBoxWithABFuncWithSourceLine(hWnd,szMsg,lpTitle,uiSty,lpszSourceFileName,dwLine);
}

int MessageLoadStringWin32ErrorParamWithSourceLine(HWND hWnd,UINT uiRes,UINT uiTitle,UINT uiSty,DWORD dwErr,LPCSTR lpszSourceFileName,DWORD dwLine)
{
    TCHAR szMessage[MAX_PATH+1024]=_T("");
    if (!GetWin32ErrorMessage(dwErr,szMessage,sizeof(szMessage)-1,FALSE))
        szMessage[0]='\0';
    return MessageLoadStringTxtParamWithSourceLine(hWnd,uiRes,uiTitle,uiSty,szMessage,lpszSourceFileName,dwLine);
}

int MessageLoadStringWin32ErrorAtNextLineWithSourceLine(HWND hWnd,UINT uiRes,UINT uiTitle,UINT uiSty,DWORD dwErr,LPCSTR lpszSourceFileName,DWORD dwLine)
{
    TCHAR szMessage[MAX_PATH+1024]=_T("");
    TCHAR szFmt[MAX_PATH+0x20];
    TCHAR szMsg[(MAX_PATH*4)+0x20];
    TCHAR szTitle[MAX_PATH];
    LPTSTR lpTitle = szTitle;

    if (!GetWin32ErrorMessage(dwErr,szMessage,sizeof(szMessage)-1,FALSE))
        szMessage[0]='\0';

    LoadInternatString(uiRes,szFmt,sizeof(szFmt)-1);
    if (dwErr != 0)
      lstrcat(szFmt,_T("\n%s"));
    wsprintf(szMsg,szFmt,szMessage,_T(""));
    if (uiTitle == NULL)
        return MessageBoxWithSourceLine(hWnd,szMsg,NULL,uiSty,lpszSourceFileName,dwLine);
    if (uiTitle) LoadInternatString(uiTitle,lpTitle,sizeof(szTitle)-1);
        else lpTitle = NULL;
    return MessageBoxWithABFuncWithSourceLine(hWnd,szMsg,lpTitle,uiSty,lpszSourceFileName,dwLine);
}


int MessageBoxWin32ErrorWithSourceLine(HWND hWnd,LPCTSTR lpszCaption,DWORD dwErr,UINT dwFlag,LPCTSTR lpszFmt,LPCSTR lpszSourceFileName,DWORD dwLine)
{
    TCHAR szMsg[MAX_PATH+1+0x80];
    TCHAR szBufferErrorMsg[MAX_PATH+1];
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM ,NULL,dwErr,0,
       szBufferErrorMsg,MAX_PATH,
       NULL) ;
    wsprintf(szMsg,
             (lpszFmt == NULL) ? __T("Error Win32 %d : %s") : lpszFmt,
             dwErr,szBufferErrorMsg);
    return MessageBoxWithSourceLine(hWnd,szMsg,lpszCaption,dwFlag,lpszSourceFileName,dwLine);
}


LONG_PTR MyGetWindowLongPtr(HWND hWnd,int nIndex)
{
    return GetWindowLongPtr(hWnd,nIndex);
}


LONG_PTR MySetWindowLongPtr(HWND hWnd,int nIndex, LONG_PTR dwNewLong)
{
    #if defined(WIN64) || defined(_WIN64)
    return ::SetWindowLongPtr(hWnd, nIndex, dwNewLong) ;
    #else
    return ::SetWindowLongPtr(hWnd, nIndex, (LONG)((LONG_PTR)dwNewLong)) ;
    #endif
}

/**************************************/


BOOL TextOutLoadString(HDC hdc,int nXStart, int nYStart, UINT uiMsg)
{
TCHAR szTxt[256];
  LoadInternatString(uiMsg,szTxt,sizeof(szTxt)-1);
  return TextOut(hdc, nXStart, nYStart,szTxt,lstrlen(szTxt));
}

int DrawTextLoadString(HDC hdc,UINT uiMsg,LPRECT lprc,UINT wFormat)
{
TCHAR szTxt[256];
  LoadInternatString(uiMsg,szTxt,sizeof(szTxt)-1);
  return DrawText(hdc,szTxt,-1,lprc,wFormat);
}

void SetWindowLoadString(HWND hWnd,UINT uiRes)
{
TCHAR szMsg[256];
  LoadInternatString(uiRes,szMsg,sizeof(szMsg)-1);
  SetWindowText(hWnd,szMsg);
}

void SetDlgItemTextLoadString(HWND hWnd,int idctl,UINT uiRes)
{
TCHAR szMsg[256];
  LoadInternatString(uiRes,szMsg,sizeof(szMsg)-1);
  SetDlgItemText(hWnd,idctl,szMsg);
}





/*******************************************************/

void ConvertFilterAnsiToSepNull(LPSTR pFilter)
{
   char c;
   while ((c=(*pFilter)) != '\0')
     {
       LPSTR pNextFilter = CharNextA(pFilter);
       if (c == '|') *pFilter = '\0';
       pFilter = pNextFilter ;
     }
   *(pFilter) = '\0';
   *(pFilter+1) = '\0';
}


void ConvertFilterAnsiToSepNoNull(LPSTR pFilter)
{
   char c;
   while (((c=(*pFilter)) != '\0') || (((*(pFilter+1)) != '\0')))
     {
       LPSTR pNextFilter = CharNextA(pFilter);
       if (c == '\0') *pFilter = '|';
       pFilter = pNextFilter ;
     }
}


void ConvertFilterUnicodeToSepNull(LPWSTR pFilter)
{
   WCHAR c;
   while ((c=(*pFilter)) != '\0')
     {
       LPWSTR pNextFilter = (pFilter+1);
       if (c == L'|') *pFilter = L'\0';
       pFilter = pNextFilter ;
     }
   *(pFilter) = L'\0';
   *(pFilter+1) = L'\0';
}


void ConvertFilterUnicodeToSepNoNull(LPWSTR pFilter)
{
   WCHAR c;
   while (((c=(*pFilter)) != L'\0') || (((*(pFilter+1)) != L'\0')))
     {
       LPWSTR pNextFilter = pFilter+1;
       if (c == L'\0') *pFilter = L'|';
       pFilter = pNextFilter ;
     }
}


static void LoadFilter(WORD wId,LPTSTR pFilter,int sz)
{
   LoadInternatString(wId,pFilter,sz-4);
   lstrcat(pFilter,_T("||"));
#ifdef UNICODE
   ConvertFilterUnicodeToSepNull(pFilter);
#else
   ConvertFilterAnsiToSepNull(pFilter);
#endif
}

MYOPENFILENAME* AllocMyOpenFileName()
{
  MYOPENFILENAME* ret;
  ret = (MYOPENFILENAME*)GlobalAllocPtr(GHND,sizeof(MYOPENFILENAME));
  return ret;
}

void FreeMyOpenFileName(MYOPENFILENAME* ptr)
{
    GlobalFreePtr(ptr);
}


typedef struct
{
    LPCSTR lpszInAnsiString;
    LPWSTR lpszUnicodeConverted;
    DWORD  dwMinAlloc;
} STRCONVERTANSITOUNICODE;

#define STRCONVERTANSITOUNICODE_NUMBERUNKNOWN (0xffffffff)
BOOL CreateUnicodeConverted(STRCONVERTANSITOUNICODE* pStrConvertAnsiToUnicode,
                            DWORD dwNb=STRCONVERTANSITOUNICODE_NUMBERUNKNOWN)
{
    DWORD i=0;
    while ((i<dwNb) || (dwNb==STRCONVERTANSITOUNICODE_NUMBERUNKNOWN))
    {
        if (dwNb==STRCONVERTANSITOUNICODE_NUMBERUNKNOWN)
            if ((pStrConvertAnsiToUnicode->lpszInAnsiString)==NULL)
                break;

        LPCSTR lpszCnv = pStrConvertAnsiToUnicode->lpszInAnsiString;
        if ((lpszCnv==NULL) && (pStrConvertAnsiToUnicode->dwMinAlloc==0))
            pStrConvertAnsiToUnicode->lpszUnicodeConverted = NULL;
        else
        {
            DWORD dwSizeAnsi = (lpszCnv!=NULL) ? lstrlenA(lpszCnv) : 0;
            if (pStrConvertAnsiToUnicode->dwMinAlloc>dwSizeAnsi)
                dwSizeAnsi=pStrConvertAnsiToUnicode->dwMinAlloc;
            pStrConvertAnsiToUnicode->lpszUnicodeConverted =
                   (LPWSTR)GlobalAllocPtr(GHND,(dwSizeAnsi*2)+0x20);
            MyMultiByteToWideChar(lpszCnv,
                                  pStrConvertAnsiToUnicode->lpszUnicodeConverted,
                                  (dwSizeAnsi*1)+0x08);
        }
        i++;
        pStrConvertAnsiToUnicode++;
    }
    return TRUE;
}

BOOL FreeUnicodeConverted(STRCONVERTANSITOUNICODE* pStrConvertAnsiToUnicode,
                            DWORD dwNb=STRCONVERTANSITOUNICODE_NUMBERUNKNOWN)
{
    DWORD i=0;
    while ((i<dwNb) || (dwNb==STRCONVERTANSITOUNICODE_NUMBERUNKNOWN))
    {
        if (dwNb==STRCONVERTANSITOUNICODE_NUMBERUNKNOWN)
            if ((pStrConvertAnsiToUnicode->lpszInAnsiString)==NULL)
                break;
        if (pStrConvertAnsiToUnicode->lpszUnicodeConverted!=NULL)
           GlobalFreePtr(pStrConvertAnsiToUnicode->lpszUnicodeConverted);
        i++;
        pStrConvertAnsiToUnicode++;
    }
    return TRUE;
}

BOOL GetMyOpenSaveFileName(MYOPENFILENAME* pofn,BOOL fSave)
{
    BOOL fRet;
    BOOL fIsUnicodeSupported = IsUnicodeSupported();
    fIsUnicodeSupported = FALSE;
    if (!fIsUnicodeSupported)
    {
        if (!fSave)
            fRet = GetOpenFileName((OPENFILENAME*)pofn);
        else
            fRet = GetSaveFileName((OPENFILENAME*)pofn);
    }/*
    else
    {
        OPENFILENAMEW* pofnw;
        STRCONVERTANSITOUNICODE StrConvertAnsiToUnicode[10];
        DWORD i;
        BOOL fTemplateText = (HIWORD((DWORD)pofn->lpTemplateName)!=0);
        pofnw = (OPENFILENAMEW*)GlobalAllocPtr(GHND,max(sizeof(OPENFILENAMEW),pofn->lStructSize));
        memcpy(pofnw,pofn,pofn->lStructSize);
        pofnw -> nMaxCustFilter = pofnw->nMaxFileTitle = 0;
        pofnw -> lpstrCustomFilter = pofnw->lpstrFileTitle = 0;

        ConvertFilterAnsiToSepNoNull((LPSTR)pofn->lpstrFilter);

        StrConvertAnsiToUnicode[0].lpszInAnsiString = pofn->lpstrFilter;
        StrConvertAnsiToUnicode[1].lpszInAnsiString = pofn->lpstrInitialDir;
        StrConvertAnsiToUnicode[2].lpszInAnsiString = pofn->lpstrTitle;
        StrConvertAnsiToUnicode[3].lpszInAnsiString = pofn->lpstrDefExt;
        if (fTemplateText)
          StrConvertAnsiToUnicode[4].lpszInAnsiString = pofn->lpTemplateName;
        else
          StrConvertAnsiToUnicode[4].lpszInAnsiString = "";
        StrConvertAnsiToUnicode[5].lpszInAnsiString = pofn->lpstrFile;
        for (i=0;i<6;i++)
            StrConvertAnsiToUnicode[i].dwMinAlloc=0;

        StrConvertAnsiToUnicode[5].dwMinAlloc=pofn->nMaxFile*2;

        CreateUnicodeConverted(StrConvertAnsiToUnicode,6);
        pofnw->lpstrFilter = StrConvertAnsiToUnicode[0].lpszUnicodeConverted,
        pofnw->lpstrInitialDir = StrConvertAnsiToUnicode[1].lpszUnicodeConverted;
        pofnw->lpstrTitle = StrConvertAnsiToUnicode[2].lpszUnicodeConverted;
        pofnw->lpstrDefExt = StrConvertAnsiToUnicode[3].lpszUnicodeConverted;
        if (fTemplateText)
          pofnw->lpTemplateName = StrConvertAnsiToUnicode[4].lpszUnicodeConverted;
        pofnw->lpstrFile = StrConvertAnsiToUnicode[5].lpszUnicodeConverted;

        ConvertFilterAnsiToSepNull((LPSTR)pofn->lpstrFilter);
        ConvertFilterUnicodeToSepNull((LPWSTR)pofnw->lpstrFilter);

        if (!fSave)
            fRet = GetOpenFileNameW(pofnw);
        else
            fRet = GetSaveFileNameW(pofnw);
        if ((pofnw->Flags & OFN_ALLOWMULTISELECT)!=0)
            ConvertFilterUnicodeToSepNoNull((LPWSTR)pofnw->lpstrFile);
        MyWideCharToMultiByte(pofnw->lpstrFile,pofn->lpstrFile,pofn->nMaxFile);
        if ((pofnw->Flags & OFN_ALLOWMULTISELECT)!=0)
            ConvertFilterAnsiToSepNull((LPSTR)pofn->lpstrFile);
        FreeUnicodeConverted(StrConvertAnsiToUnicode,6);
        pofn->nFilterIndex = pofnw->nFilterIndex;

        GlobalFreePtr(pofnw);
    }*/
    return fRet;
}


BOOL GetMyOpenFileName(MYOPENFILENAME* pofn)
{
    pofn->Flags|= OFN_ENABLESIZING;
    //return GetOpenFileName((OPENFILENAME*)pofn);
    return GetMyOpenSaveFileName(pofn,FALSE);
}

BOOL GetMySaveFileName(MYOPENFILENAME* pofn)
{
    pofn->Flags|= OFN_ENABLESIZING;
    return GetSaveFileName((OPENFILENAME*)pofn);
    return GetMyOpenSaveFileName(pofn,TRUE);
}

void InitOpenFileName(MYOPENFILENAME * pofn,HWND hWnd,
                    WORD wIdFilter,LPTSTR pFilter,int szFilter,
                    LPTSTR lpstrFile,DWORD dwSizeFile,
                    LPTSTR lpCapt,WORD wIdCapt,int szCapt)
{

WORD wVer;
BOOL fSupportNewOpenFileName;
   wVer = LOWORD(GetVersion());
   wVer = (((WORD)LOBYTE(wVer)) << 8) | (WORD)HIBYTE(wVer);

   fSupportNewOpenFileName= (wVer>=0x45a);
   //fSupportNewOpenFileName=FALSE; // only for test for old Win9x

   /*
#ifdef _DEBUG
   { char sz[256]; wsprintf(sz,"wVer=%lx, GetVersion=%lx",(DWORD)wVer,GetVersion()); MessageBoxFullInfo(hWnd,sz,sz,0);}
#endif
    */

   /*
    ? LoByte(LoWord(GetVersion)), HiByte(LoWord(GetVersion))

    Win95:    4,0
    Win98:    4,10
    WinME:    4,90
    WinNT:    4,0
    Win2K:    5,0
    */

   //fSupportNewOpenFileName=FALSE;//!!!
   _fmemset(pofn, 0, sizeof(MYOPENFILENAME));

   LoadFilter(wIdFilter,pFilter,szFilter);

   if (fSupportNewOpenFileName)
       pofn->lStructSize = sizeof(MYOPENFILENAME);
   else
   #ifdef OPENFILENAME_SIZE_VERSION_400
       pofn->lStructSize = OPENFILENAME_SIZE_VERSION_400;
   #else
       pofn->lStructSize = sizeof(OPENFILENAME);
   #endif

   pofn->hwndOwner = hWnd;
   pofn->lpstrFilter = pFilter ;
   pofn->nFilterIndex = 1;
   pofn->lpstrFile = lpstrFile;
   pofn->nMaxFile = dwSizeFile;

   if (wIdCapt != 0)
     {
       LoadInternatString(wIdCapt,lpCapt,szCapt);
       pofn->lpstrTitle = lpCapt;
     }

   pofn->Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
}




#ifdef WIN32
//*************************************************************
//
//  GetWin32Kind()
//
//  Purpose:
//              Get the Win32 platform
//
//  Parameters:
//
//  Return: (WIN32KIND)
//      WINNT -           Run under Windows NT
//      WIN32S -          Run under Windows 3.1x + Win32s
//      WIN95ORGREATHER - Run under Windows 95
//
//
//  Comments:
//      Win32 function designed for Windows NT and Windows 95
//      See RegOpenKeyEx API for more info on lpszKey
//
//  History:    Date       Author       Comment
//              09/24/94   G. Vollant   Created
//
//*************************************************************

WIN32KIND GetWin32Kind()
{
//BOOL IsWin395OrHigher(void);

  WORD wVer;

  if ((GetVersion() & 0x80000000) == 0)
    return WINNT;
  wVer = LOWORD(GetVersion());
  wVer = (((WORD)LOBYTE(wVer)) << 8) | (WORD)HIBYTE(wVer);

  if (wVer >= 0x035F)
    return WIN95ORGREATHER;
  else
    return WIN32S;
}

BOOL IsNT4ForConnectFAT32()
{
    if (GetWin32Kind() == WINNT)
      return (LOBYTE(LOWORD(GetVersion())) <= 4+(666*0));
    return FALSE;
}
#endif

WORD GetVersionReordered()
{
  WORD wVer = LOWORD(GetVersion());
  return (((WORD)LOBYTE(wVer)) << 8) | (WORD)HIBYTE(wVer);
}

BOOL IsWin95OrGreatherLook()
{
  //return FALSE; // only for tired old CommDlg
  return (GetVersionReordered() >= 0x035F);
}

BOOL IsWin98orW2KOrGreatherLook()
{
  return (GetVersionReordered() >= 0x040a);
}

BOOL IsWhistler()
{
  return (GetVersionReordered() >= 0x0501);
}

BOOL IsVistaOrGreather()
{
  return (GetVersionReordered() >= 0x0600);
}


#ifndef STAP_ALLOW_NONCLIENT
#define STAP_ALLOW_NONCLIENT    (1 << 0)
#define STAP_ALLOW_CONTROLS     (1 << 1)
#define STAP_ALLOW_WEBCONTENT   (1 << 2)
#endif

typedef void (WINAPI* SETTHEMEAPPPROPERTIESPROC)(DWORD);

BOOL SetWhistlerThemeOk()
{
    HMODULE hLibUXT;
    SETTHEMEAPPPROPERTIESPROC lpSetThemeAppProperties;
    if (!IsWhistler())
        return FALSE;
    hLibUXT = LoadLibrary(_T("UXTHEME.DLL"));
    if (hLibUXT ==NULL)
        return FALSE;
    lpSetThemeAppProperties = (SETTHEMEAPPPROPERTIESPROC)GetProcAddress(hLibUXT,"SetThemeAppProperties");
    if (lpSetThemeAppProperties ==NULL)
        return FALSE;
    (*lpSetThemeAppProperties)(STAP_ALLOW_NONCLIENT|STAP_ALLOW_CONTROLS|STAP_ALLOW_WEBCONTENT);
    return TRUE;
}

typedef struct _myDLLVERSIONINFO
{
    DWORD cbSize;
    DWORD dwMajorVersion;                   // Major version
    DWORD dwMinorVersion;                   // Minor version
    DWORD dwBuildNumber;                    // Build number
    DWORD dwPlatformID;                     // DLLVER_PLATFORM_*
} myDLLVERSIONINFO;


typedef HRESULT (CALLBACK* myDLLGETVERSIONPROC)(myDLLVERSIONINFO *);

DWORD GetDllVersion(LPCTSTR lpszDllName)
{

    HINSTANCE hinstDll;
    DWORD dwVersion = 0;

    hinstDll = LoadLibrary(lpszDllName);

    if(hinstDll)
    {
        myDLLGETVERSIONPROC pDllGetVersion;

        pDllGetVersion = (myDLLGETVERSIONPROC) GetProcAddress(hinstDll, "DllGetVersion");

/*Because some DLLs might not implement this function, you
  must test for it explicitly. Depending on the particular
  DLL, the lack of a DllGetVersion function can be a useful
  indicator of the version.
*/
        if(pDllGetVersion)
        {
            myDLLVERSIONINFO dvi;
            HRESULT hr;

            ZeroMemory(&dvi, sizeof(dvi));
            dvi.cbSize = sizeof(dvi);

            hr = (*pDllGetVersion)(&dvi);

            if(SUCCEEDED(hr))
            {
                dwVersion = PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
            }
        }

        FreeLibrary(hinstDll);
    }
    return dwVersion;
}




typedef enum
{
  myTokenElevationTypeDefault = 1,
  myTokenElevationTypeFull,
  myTokenElevationTypeLimited
} myTOKEN_ELEVATION_TYPE;

#define myTOKEN_QUERY (8)


typedef enum _myTOKEN_INFORMATION_CLASS {
    myTokenUser = 1,
    myTokenGroups,
    myTokenPrivileges,
    myTokenOwner,
    myTokenPrimaryGroup,
    myTokenDefaultDacl,
    myTokenSource,
    myTokenType,
    myTokenImpersonationLevel,
    myTokenStatistics,
    myTokenRestrictedSids,
    myTokenSessionId,
    myTokenGroupsAndPrivileges,
    myTokenSessionReference,
    myTokenSandBoxInert,
    myTokenAuditPolicy,
    myTokenOrigin,
    myTokenElevationType,
    myTokenLinkedToken,
    myTokenElevation,
    myTokenHasRestrictions,
    myTokenAccessInformation,
    myTokenVirtualizationAllowed,
    myTokenVirtualizationEnabled,
    myTokenIntegrityLevel,
    myTokenUIAccess,
    myTokenMandatoryPolicy,
    myTokenLogonSid,
    myMaxTokenInfoClass  // MaxTokenInfoClass should always be the last enum
} myTOKEN_INFORMATION_CLASS, *PmyTOKEN_INFORMATION_CLASS;


UAC_SITUATION GetUacSituation()
{
  HANDLE hToken;
  myTOKEN_ELEVATION_TYPE elevationType;
  DWORD dwSize;
  UAC_SITUATION uacRet = NO_UAC_TOKEN;

  if ((GetVersion() & 0x80000000) == 0)
      if (OpenProcessToken(GetCurrentProcess(), myTOKEN_QUERY, &hToken))
      {
          if (GetTokenInformation(hToken,
                (TOKEN_INFORMATION_CLASS)myTokenElevationType,
                &elevationType, sizeof(elevationType), &dwSize))
          {

              switch (elevationType) {
                case myTokenElevationTypeDefault:
                  uacRet = UAC_TOKEN_NOADMINUSER;
                  break;

                case myTokenElevationTypeFull:
                  uacRet = UAC_TOKEN_ADMINELEVATED;
                  break;

                case myTokenElevationTypeLimited:
                  uacRet = UAC_TOKEN_ADMINNOTELEVATED;
                  break;
              }
          }

          if (hToken)
            CloseHandle(hToken);
      }

  return uacRet;
}



/**************************************************************/

BOOL IsUnicodeSupported()
{
    // return yes for WinNT and Win2000
    return GetWin32Kind()==WINNT;
}

BOOL MyMultiByteToWideChar(LPCSTR lpMultiByteStr, LPWSTR lpWideCharStr, int cchWideChar)
{
    int iRet;
    if (lpMultiByteStr==NULL)
        lpMultiByteStr="";
    iRet = MultiByteToWideChar(CP_ACP,0/*WC_DEFAULTCHAR*/,
                        lpMultiByteStr, -1,
                        lpWideCharStr, cchWideChar);
    return (iRet != 0);
}

BOOL MyWideCharToMultiByte(LPCWSTR lpwStr, LPSTR lpBuf, int cch)
{
    int iRet;
    iRet = WideCharToMultiByte(uiCodePage,0,lpwStr,-1,lpBuf,cch,NULL,NULL);
    return (iRet != 0);
}

HWND MyCreateDialog(HINSTANCE hInstance, LPCTSTR lpTemplateName,
                    HWND hWndParent, DLGPROC lpDialogFunc)
{
    return MyCreateDialogParam(hInstance,lpTemplateName,hWndParent,lpDialogFunc,0L);
}

HWND MyCreateDialogParam(HINSTANCE hInstance, LPCTSTR lpTemplateName,
                         HWND hWndParent, DLGPROC lpDialogFunc,
                         LPARAM dwInitParam)
{
    if (!IsUnicodeSupported())
    {
        return CreateDialogParam(hInstance,lpTemplateName,hWndParent,lpDialogFunc,dwInitParam);
    }
    else
    {
        WCHAR  wstrTemplateName[255];
        LPWSTR lpwstrTemplateName;
        DWORD_PTR dwCreateDialogParam = (DWORD_PTR)CreateDialogParam;
        DWORD_PTR dwCreateDialogParamLow = LOWORD(dwCreateDialogParam) & 0xffffUL;

        //if (HIWORD((DWORD)(lpTemplateName))==0)
        if (dwCreateDialogParam == dwCreateDialogParamLow)
            lpwstrTemplateName=(LPWSTR)lpTemplateName;
        else
        {
            lpwstrTemplateName=(LPWSTR)wstrTemplateName;
            MyMultiByteToWideChar((LPCSTR)lpTemplateName,
                                  (LPWSTR)wstrTemplateName,
                                  sizeof(wstrTemplateName)/2);
        }
        return CreateDialogParamW(hInstance,lpwstrTemplateName,hWndParent,lpDialogFunc,dwInitParam);
    }
}

INT_PTR MyDialogBox(HINSTANCE hInstance, LPCTSTR lpTemplateName,
                 HWND hWndParent, DLGPROC lpDialogFunc)
{
    return MyDialogBoxParam(hInstance,lpTemplateName,hWndParent,lpDialogFunc,0L);
}

INT_PTR MyDialogBoxParam(HINSTANCE hInstance, LPCTSTR lpTemplateName,
                      HWND hWndParent, DLGPROC lpDialogFunc,
                      LPARAM dwInitParam)
{
#ifdef UNICODE
    return DialogBoxParam(hInstance,lpTemplateName,hWndParent,lpDialogFunc,dwInitParam);
#else
    if (!IsUnicodeSupported())
    {
        return DialogBoxParam(hInstance,lpTemplateName,hWndParent,lpDialogFunc,dwInitParam);
    }
    else
    {
        WCHAR  wstrTemplateName[255];
        LPWSTR lpwstrTemplateName;
        DWORD_PTR dwTemplateName = (DWORD_PTR)lpTemplateName;
        DWORD_PTR dwTemplateNameLow = LOWORD(dwTemplateName) & 0xffffUL;


        //if (HIWORD((DWORD)(lpTemplateName))==0)
        if (dwTemplateNameLow == dwTemplateName)
            lpwstrTemplateName=(LPWSTR)lpTemplateName;
        else
        {
            lpwstrTemplateName=(LPWSTR)wstrTemplateName;
            MyMultiByteToWideChar((LPCSTR)lpTemplateName,
                                  (LPWSTR)wstrTemplateName,
                                  sizeof(wstrTemplateName)/2);
        }
        return DialogBoxParamW(hInstance,lpwstrTemplateName,hWndParent,lpDialogFunc,dwInitParam);
    }
#endif
}

/*****************************************************************/

BOOL MyMkdir(LPCTSTR dirname)
{
int iret;

  iret = mymkdirmacro(dirname);
  if (iret==0)
    return TRUE;

//  if (errno==EACCES)
//    return FALSE;

  if ((*dirname)!='\0')
  {
      LPTSTR lpParc = CharNext(dirname);
      for (;;)
      {
          LPTSTR lpNext;
          if ( ((*(lpParc))=='\\') && /*((*(dirname+i-1))!='\\') &&*/ ((*(lpParc+1))!='\\') )
              if ((*CharPrev(dirname,lpParc))!='\\')
              {
                  *(lpParc) = '\0';
                  mymkdirmacro(dirname);
                  *(lpParc) = '\\';
              }

          lpNext = CharNext(lpParc);
          if (lpParc==lpNext)
              break;
          lpParc=lpNext;
      }
  }

  return mymkdirmacro(dirname)==0;
}

/**************************************************************/



BOOL UncompressGZMem(const BYTE* lpRes,DWORD dwSizeRes,LPSTR & lpBuf,DWORD & dwSizeExe)
{
    BOOL fRet=FALSE;
    lpBuf = NULL;
    dwSizeExe=0;

    DWORD dwCrcRes = ComputeBufferCRC(0xFFFFFFFFL,(void*)lpRes,dwSizeRes ) ^ 0xFFFFFFFFL;


    if (!(((*lpRes)=='\x1f') && (*(lpRes+1)==((BYTE)'\x8b')) && (*(lpRes+2)=='\x08')))
    { // No GZ header
      lpBuf = (LPSTR)GlobalAllocPtr(GHND,dwSizeRes+0x10);
      fRet = (lpBuf != NULL);
      if (fRet)
      {
        dwSizeExe = dwSizeRes;
        memcpy(lpBuf,lpRes,dwSizeExe);
      }
    }

#ifndef ZLIB_NO_PRESENT
	else
     { // here GZ file
         BYTE flag = *(lpRes+3);
         const BYTE* lpBegin = lpRes + 0x0a;
         DWORD dwCrcComputed;
         DWORD dwUncompressedSize= ((*(lpRes+dwSizeRes-4))) |
                                   ((*(lpRes+dwSizeRes-3)) * 0x100) |
                                   ((*(lpRes+dwSizeRes-2)) * 0x10000) |
                                   ((*(lpRes+dwSizeRes-1)) * 0x1000000);

         DWORD dwCRC32=            ((*(lpRes+dwSizeRes-8))) |
                                   ((*(lpRes+dwSizeRes-7)) * 0x100) |
                                   ((*(lpRes+dwSizeRes-6)) * 0x10000) |
                                   ((*(lpRes+dwSizeRes-5)) * 0x1000000);

         if ((flag & 0x04) != 0) // FEXTRA
         {
             WORD wxLen = (*lpBegin) + (0x100 * ((*(lpBegin+1)))) ;
             lpBegin += wxLen +2;
         }
         if ((flag & 0x8) != 0) // ORIG NAME
         {
             while ((*lpBegin) != '\0')
                 lpBegin++;
             lpBegin++;
         }
         if ((flag & 0x10) != 0) // ORIG COMMENT
         {
             while ((*lpBegin) != '\0')
                 lpBegin++;
             lpBegin++;
         }
         if ((flag & 0x2) != 0) // ORIG COMMENT
         {
             lpBegin+=2;
         }

         lpBuf = (LPSTR)GlobalAllocPtr(GHND,dwUncompressedSize+0x20);
         dwSizeExe = dwUncompressedSize ;


          {
             /* We have the GZip file in resource, we uncompress it*/
              z_stream c_stream; /* compression stream */
              int err;
              DWORD dwRestIn = (uInt)((lpRes+dwSizeRes)-lpBegin)-8 ;
                _fmemset(&c_stream,0,sizeof(c_stream));
                c_stream.zalloc = (alloc_func)0;
                c_stream.zfree = (free_func)0;
                c_stream.opaque = (voidpf)0;
                err=inflateInit2(&c_stream,-MAX_WBITS);

                c_stream.next_in = (LPBYTE)lpBegin;
                c_stream.avail_in = (uInt)dwRestIn + 4;


                c_stream.next_out = (LPBYTE)lpBuf;
                c_stream.avail_out = (uInt)dwUncompressedSize + 0x10;

                err=inflate(&c_stream, Z_FINISH);

                err=inflateEnd(&c_stream);
          }

          dwCrcComputed = ComputeBufferCRC(0xFFFFFFFFL,lpBuf,dwUncompressedSize) ^ 0xFFFFFFFFL;

          fRet = dwCrcComputed == dwCRC32;
     }
#endif

    return fRet;
}

// --------------------------------------------------------------------------

BOOL UncompressGZRes(HMODULE hModule, LPCTSTR lpName, LPCTSTR lpType, LPSTR & lpBuf,DWORD & dwSizeExe)
{
    HRSRC hrSrc=NULL;
    HGLOBAL hGlob=NULL;
    BOOL fResUncompr = FALSE;
    LPSTR lpRes = NULL;

    hrSrc=FindResource(hModule, lpName, lpType);
    if (hrSrc!=NULL)
        hGlob=LoadResource(NULL,hrSrc);
    lpRes=(LPSTR)LockResource(hGlob);

    if (lpRes != NULL)
        fResUncompr=UncompressGZMem((LPBYTE)lpRes,SizeofResource(hModule,hrSrc),lpBuf,dwSizeExe);

    UnlockResource(hGlob);
    FreeResource(hGlob);
    return fResUncompr;
}

// --------------------------------------------------------------------------

BOOL FreeGzDecompressedMem(LPSTR lpBuf)
{
    GlobalFreePtr(lpBuf);
    return TRUE;
}

/*
 * TransparentBlt
 *
 * Purpose:
 *  Given a DC, a bitmap, and a color to assume as transparent in that
 *  bitmap, BitBlts the bitmap to the DC letting the existing background
 *  show in place of the transparent color.
 *
 * Parameters:
 *  hDC             HDC on which to draw.
 *  x, y            UINT location at which to draw the bitmap
 *  hBmp            HBITMIP to draw
 *  cr              COLORREF to consider as transparent.
 *
 * Return Value:
 *  None
 */

//Special ROP code for TransparentBlt.
#define ROP_DSPDxax  0x00E20746

void TransparentBlt(HDC hDC, UINT x, UINT y, HBITMAP hBmp, COLORREF cr,UINT xs,UINT ys,UINT cx,UINT cy,BOOL fDoBlack)
    {
    HDC         hDCSrc, hDCMid, hMemDC;
    HBITMAP     hBmpMono, hBmpT;
    HBRUSH      hBr, hBrT;
    COLORREF    crBack, crText;
    BITMAP      bm;

    if (NULL==hBmp)
    return;

    GetObject(hBmp, sizeof(bm), &bm);

    //Get three intermediate DC's

    hDCSrc=CreateCompatibleDC(hDC);
    if (hDCSrc==NULL)
        hDCSrc=CreateCompatibleDC(NULL);

    hDCMid=CreateCompatibleDC(hDC);
    if (hDCMid==NULL)
        hDCMid=CreateCompatibleDC(NULL);

    hMemDC=CreateCompatibleDC(hDC);
    if (hMemDC==NULL)
        hMemDC=CreateCompatibleDC(NULL);

    #ifdef _DEBUG
    if (hDCSrc==NULL)
    {
        TCHAR sz[128];
        wsprintf(sz,_T("hDC=%lx,hDCSrc=%lx\n"),hDC,hDCSrc);
        OutputDebugString(sz);
    }
    #endif
/*
    hDCSrc=CreateCompatibleDC(NULL);
    hDCMid=CreateCompatibleDC(NULL);
    hMemDC=CreateCompatibleDC(NULL);
*/
    SelectObject(hDCSrc, hBmp);
    if (cx==0)
        cx=bm.bmWidth;

    if (cy==0)
        cy=bm.bmHeight;

    //Create a monochrome bitmap for masking
    hBmpMono=CreateCompatibleBitmap(hDCMid, cx, cy);
    SelectObject(hDCMid, hBmpMono);

    //Create a middle bitmap
    hBmpT=CreateCompatibleBitmap(hDC, cx, cy);
    SelectObject(hMemDC, hBmpT);


    //Create a monochrome mask where we have 0's in the image, 1's elsewhere.
    crBack=SetBkColor(hDCSrc, cr);
    BitBlt(hDCMid, 0, 0, cx, cy, hDCSrc, xs, ys, SRCCOPY);
    SetBkColor(hDCSrc, crBack);

    //Put the unmodified image in the temporary bitmap
    BitBlt(hMemDC, 0, 0, cx, cy, hDCSrc, xs ,ys, SRCCOPY);

    //Create an select a brush of the background color
    hBr=CreateSolidBrush(GetBkColor(hDC));
    hBrT=SelectBrush(hMemDC, hBr);

    //Force conversion of the monochrome to stay black and white.
    crText=SetTextColor(hMemDC, 0L);
    crBack=SetBkColor(hMemDC, RGB(255*1, 255, 255*1));



    /*
     * Where the monochrome mask is 1, Blt the brush; where the mono mask
     * is 0, leave the destination untouches.  This results in painting
     * around the image with the background brush.  We do this first
     * in the temporary bitmap, then put the whole thing to the screen.
     */

    if (!fDoBlack)
    {
        BitBlt(hMemDC, 0, 0, /*bm.bmWidth*/cx, cy, hDCMid, 0, 0, ROP_DSPDxax);
        BitBlt(hDC,    x, y, /*bm.bmWidth*/cx, cy, hMemDC, 0, 0, SRCCOPY);
    }
    else
        BitBlt(hDC,    x, y, /*bm.bmWidth*/cx, bm.bmHeight, hDCMid, 0, 0, SRCCOPY);

    SetTextColor(hMemDC, crText);
    SetBkColor(hMemDC, crBack);

    SelectObject(hMemDC, hBrT);

    DeleteDC(hMemDC);
    DeleteDC(hDCSrc);
    DeleteDC(hDCMid);

    DeleteObject(hBr);
    DeleteObject(hBmpT);
    DeleteObject(hBmpMono);

    return;
    }

BOOL PaintBitmap(HDC hdc,int xd,int yd,int cx,int cy,HBITMAP hbm,int xs,int ys)
{
  HDC hdcMem;
  HBITMAP hBmpOld;
  BOOL fRet;

  if ((cx==0) || (cy==0))
  {
      BITMAP bm;
      GetObject(hbm, sizeof(bm), &bm);
      if (cx==0)
          cx = bm.bmWidth;
      if (cy==0)
          cy = bm.bmHeight;
  }

  hdcMem = CreateCompatibleDC(hdc);
  hBmpOld=SelectBitmap(hdcMem, hbm);
  fRet = BitBlt(hdc,xd,yd,cx,cy,hdcMem,xs,ys,SRCCOPY);
  SelectObject(hdcMem,hBmpOld);
  DeleteDC(hdcMem);
  return fRet;
}


BOOL GetPremultipliedBitmap(HBITMAP hbmp,COLORREF clrBkgnd)
{
  DIBSECTION dibsection;

  ::GetObject(hbmp, sizeof(DIBSECTION), &dibsection);
  if (dibsection.dsBmih.biBitCount!=32)
  {
      return FALSE;
  }

  RGBQUAD* pQuad = (RGBQUAD *)dibsection.dsBm.bmBits;

  for(int i = dibsection.dsBmih.biWidth * dibsection.dsBmih.biHeight; i--; pQuad++)
  {
      DWORD dwAlpha = (DWORD)pQuad->rgbReserved;
      pQuad->rgbRed   = (BYTE)(((pQuad->rgbRed*dwAlpha) / 255) +
                               ((GetRValue(clrBkgnd)*(255-dwAlpha))/255));
      pQuad->rgbGreen = (BYTE)(((pQuad->rgbGreen*dwAlpha) / 255) +
                               ((GetGValue(clrBkgnd)*(255-dwAlpha))/255));
      pQuad->rgbBlue  = (BYTE)(((pQuad->rgbBlue*dwAlpha) / 255) +
                               ((GetBValue(clrBkgnd)*(255-dwAlpha))/255));
  }
  return TRUE;
}


HBITMAP LoadBitmapRes(HINSTANCE hInstance,LPCTSTR lpBitmapName,LPCTSTR lpType,
                      HDC hDc,
                      ALPHA_STRATEGY AlphaStrategy,
                      DWORD dwNbClrBkGnd,const COLORREF* lpClrBkGnd,
                      BOOL fMagicColorToReplace,COLORREF clrToReplace)
{
    TRANSPARENCY_PAINT TransPaint;
    HBITMAP hBmp;
    TransPaint.AlphaStrategy = AlphaStrategy;
    TransPaint.dwNbClrBkGnd = dwNbClrBkGnd;
    TransPaint.lpClrBkGnd = lpClrBkGnd;
    hBmp = LoadBitmapResMutltiTrans(hInstance,lpBitmapName,lpType,
                                 hDc,
                                 1,&TransPaint,
                                 fMagicColorToReplace,clrToReplace);
    return hBmp;
}

/*
HBITMAP LoadBitmapResO(HINSTANCE hInstance,LPCSTR lpBitmapName,LPCTSTR lpType,
                      HDC hDc,
                      ALPHA_STRATEGY AlphaStrategy,
                      DWORD dwNbClrBkGnd,const COLORREF* lpClrBkGnd,
                      BOOL fMagicColorToReplace,COLORREF clrToReplace)
{
    LPSTR lpBuf=NULL;
    DWORD dwSize=0;
    HBITMAP hBmp=NULL;
    BOOL fNeedCreateDelHdc;
    BITMAPINFO * lpBmi ;
    LPBYTE lpOffBits;

    if (!UncompressGZRes(hInstance,lpBitmapName,lpType,lpBuf,dwSize))
        return NULL;

    BITMAPFILEHEADER *lpbmfh=(BITMAPFILEHEADER *)lpBuf;
    if (lpbmfh->bfType==0x4d42)
    {
        lpBmi =(BITMAPINFO *)(lpBuf+sizeof(BITMAPFILEHEADER));
        lpOffBits= (LPBYTE)lpBuf + (lpbmfh->bfOffBits);
    }
    else
    {
        lpBmi=(BITMAPINFO *)lpBuf;
        lpOffBits = (LPBYTE)lpBuf + lpBmi->bmiHeader.biSize + (lpBmi->bmiHeader.biClrUsed * sizeof(RGBQUAD));
    }

    fNeedCreateDelHdc = hDc==NULL;
    if (fNeedCreateDelHdc)
        hDc=GetDC(NULL);

    if (lpBmi->bmiHeader.biBitCount==32)
    {
        LONG i,j;
        lpBmi->bmiHeader.biBitCount=24;
        lpBmi->bmiHeader.biSizeImage=0;
        BOOL fDoAlphaBkgnd = (AlphaStrategy == ALPHA_MONOCOLOR) && (dwNbClrBkGnd==1);
        COLORREF clrBkGnd;

        if ((AlphaStrategy == ALPHA_MONOCOLOR) && (dwNbClrBkGnd==1))
        clrBkGnd = *lpClrBkGnd;

        for (i=0;i<lpBmi->bmiHeader.biHeight;i++)
        {
            LPBYTE lpOldPosBmp = lpOffBits + (lpBmi->bmiHeader.biWidth*4)*i;
            LPBYTE lpOldNewBmp = lpOffBits + (AroundUpper((lpBmi->bmiHeader.biWidth*3),4)*i);

            if ((AlphaStrategy == ALPHA_VERTICAL) && (dwNbClrBkGnd==lpBmi->bmiHeader.biHeight))
            {
                fDoAlphaBkgnd=TRUE;
                clrBkGnd = *(lpClrBkGnd+i);
            }

            for (j=0;j<lpBmi->bmiHeader.biWidth;j++)
            {
                RGBQUAD rgbQuadOld = *(RGBQUAD*)lpOldPosBmp;
                RGBTRIPLE* prgbTriple=(RGBTRIPLE*)lpOldNewBmp;

                if ((AlphaStrategy == ALPHA_HORIZONTAL) && (dwNbClrBkGnd==lpBmi->bmiHeader.biWidth))
                {
                    fDoAlphaBkgnd=TRUE;
                    clrBkGnd = *(lpClrBkGnd+j);
                }

                if (fDoAlphaBkgnd)
                {
                    DWORD dwAlpha = (DWORD)rgbQuadOld.rgbReserved;

                    prgbTriple->rgbtBlue =  (BYTE)(((rgbQuadOld.rgbBlue*dwAlpha) / 255) +
                                                   ((GetBValue(clrBkGnd)*(255-dwAlpha))/255));
                    prgbTriple->rgbtGreen = (BYTE)(((rgbQuadOld.rgbGreen*dwAlpha) / 255) +
                                                   ((GetGValue(clrBkGnd)*(255-dwAlpha))/255));
                    prgbTriple->rgbtRed =  (BYTE)(((rgbQuadOld.rgbRed*dwAlpha) / 255) +
                                                   ((GetRValue(clrBkGnd)*(255-dwAlpha))/255));
                }
                else
                {
                    prgbTriple->rgbtBlue = rgbQuadOld.rgbBlue;
                    prgbTriple->rgbtGreen = rgbQuadOld.rgbGreen;
                    prgbTriple->rgbtRed = rgbQuadOld.rgbRed;
                }
                lpOldPosBmp+=4;
                lpOldNewBmp+=3;
            }
        }
    }

    if ((lpBmi->bmiHeader.biBitCount==24) && fMagicColorToReplace)
    {
        LONG i,j;

        BOOL fDoAlphaBkgnd = (AlphaStrategy == ALPHA_MONOCOLOR) && (dwNbClrBkGnd==1);
        COLORREF clrBkGnd;

        if ((AlphaStrategy == ALPHA_MONOCOLOR) && (dwNbClrBkGnd==1))
        clrBkGnd = *lpClrBkGnd;

        for (i=0;i<lpBmi->bmiHeader.biHeight;i++)
        {
            LPBYTE lpPosBmp = lpOffBits + (AroundUpper((lpBmi->bmiHeader.biWidth*3),4)*i);

            if ((AlphaStrategy == ALPHA_VERTICAL) && (dwNbClrBkGnd==lpBmi->bmiHeader.biHeight))
            {
                fDoAlphaBkgnd=TRUE;
                clrBkGnd = *(lpClrBkGnd+i);
            }

            for (j=0;j<lpBmi->bmiHeader.biWidth;j++)
            {
                RGBTRIPLE rgbTripeOld = *(RGBTRIPLE*)lpPosBmp;
                RGBTRIPLE* prgbTriple=(RGBTRIPLE*)lpPosBmp;

                if ((AlphaStrategy == ALPHA_HORIZONTAL) && (dwNbClrBkGnd==lpBmi->bmiHeader.biWidth))
                {
                    fDoAlphaBkgnd=TRUE;
                    clrBkGnd = *(lpClrBkGnd+j);
                }

                if (fDoAlphaBkgnd)
                {
                    if ((rgbTripeOld.rgbtBlue == GetBValue(clrToReplace)) &&
                        (rgbTripeOld.rgbtGreen== GetGValue(clrToReplace)) &&
                        (rgbTripeOld.rgbtRed  == GetRValue(clrToReplace)))
                    {
                        prgbTriple->rgbtBlue =  (BYTE)GetBValue(clrBkGnd);
                        prgbTriple->rgbtGreen = (BYTE)GetGValue(clrBkGnd);
                        prgbTriple->rgbtRed =   (BYTE)GetRValue(clrBkGnd);
                    }
                }

                lpPosBmp+=3;
            }
        }
    }

    hBmp=CreateDIBitmap(hDc,&lpBmi->bmiHeader,CBM_INIT,lpOffBits,lpBmi,DIB_RGB_COLORS);

    FreeGzDecompressedMem(lpBuf);
    if ((fNeedCreateDelHdc)&&(hDc!=NULL))
        ReleaseDC(NULL,hDc);

    return hBmp;
}
*/

HBITMAP LoadBitmapResAuto(HINSTANCE hInstance,LPCTSTR lpBitmapName,
                          HDC hDc,
                          ALPHA_STRATEGY AlphaStrategy,
                          DWORD dwNbClrBkGnd,const COLORREF* lpClrBkGnd,
                          BOOL fMagicColorToReplace,COLORREF clrToReplace)
{
    HBITMAP hBmpRes=NULL;
    hBmpRes = LoadBitmapRes(hInstance,lpBitmapName,_T("BMPGZDATA"),
                            hDc,AlphaStrategy,dwNbClrBkGnd,lpClrBkGnd,
                            fMagicColorToReplace,clrToReplace);
    if (hBmpRes == NULL)
        hBmpRes = LoadBitmapRes(hInstance,lpBitmapName,RT_BITMAP,
                                hDc,AlphaStrategy,dwNbClrBkGnd,lpClrBkGnd,
                                fMagicColorToReplace,clrToReplace);
    return hBmpRes;
}




HBITMAP LoadBitmapResMutltiTrans(IN HINSTANCE hInstance,IN LPCTSTR lpBitmapName,LPCTSTR lpType,
                                 HDC hDc,
                                 DWORD dwNbTransparencyPaint,const TRANSPARENCY_PAINT* pTransPaint,
                                 BOOL fMagicColorToReplace,COLORREF clrToReplace)
{

    LPSTR lpBuf=NULL;
    LPSTR lpBufNew;
    DWORD dwSize=0;
    HBITMAP hBmp=NULL;
    BITMAPINFO * lpBmiRead ;
    BITMAPINFO * lpBmiWrite ;
    LPBYTE lpOffBitsRead;
    LPBYTE lpOffBitsWrite;
    DWORD dwOffBmi,dwOffBits;
    DWORD dwSizeBitmapHeader;
    TRANSPARENCY_PAINT TransPaintOne;
    DWORD i,j,k;

    if (!UncompressGZRes(hInstance,lpBitmapName,lpType,lpBuf,dwSize))
        return NULL;

    BITMAPFILEHEADER *lpbmfhRead=(BITMAPFILEHEADER *)lpBuf;
    if (lpbmfhRead->bfType==0x4d42)
    {
        dwOffBmi = sizeof(BITMAPFILEHEADER);
        dwOffBits= lpbmfhRead->bfOffBits;
        dwSizeBitmapHeader = dwOffBits - dwOffBmi ;
    }
    else
    {
        dwOffBmi = 0;
        lpBmiRead=(BITMAPINFO *)lpBuf;
        dwOffBits = lpBmiRead->bmiHeader.biSize + (lpBmiRead->bmiHeader.biClrUsed * sizeof(RGBQUAD));
        dwSizeBitmapHeader = dwOffBits ;
    }

    if ((dwNbTransparencyPaint==0) || (pTransPaint==NULL))
    {
        dwNbTransparencyPaint=1;
        TransPaintOne.AlphaStrategy = ALPHA_NOALPHAPAINT;
        pTransPaint= &TransPaintOne;
    }

    lpBmiRead =(BITMAPINFO *)(lpBuf+dwOffBmi);
    lpOffBitsRead= (LPBYTE)lpBuf + dwOffBits;
	BOOL fHeightInverted = lpBmiRead->bmiHeader.biHeight < 0;
	LONG absoluteHeight = (!fHeightInverted) ? lpBmiRead->bmiHeader.biHeight : -lpBmiRead->bmiHeader.biHeight;
    lpBufNew = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE,dwOffBits + ((dwNbTransparencyPaint)*(lpBmiRead->bmiHeader.biWidth*absoluteHeight*4)));

#ifndef ZLIB_NO_PRESENT
    if (lpBufNew == NULL)
    {
        FreeGzDecompressedMem(lpBuf);
        return FALSE;
    }
#endif

    memcpy(lpBufNew,lpBuf,dwOffBits);
    lpBmiWrite=(BITMAPINFO *)(lpBufNew+dwOffBmi);
    lpOffBitsWrite= (LPBYTE)lpBufNew + dwOffBits;


    lpBmiWrite->bmiHeader.biHeight = absoluteHeight*dwNbTransparencyPaint;


    if (lpBmiRead->bmiHeader.biBitCount==32)
    {
        lpBmiWrite->bmiHeader.biBitCount=24;
        //lpBmiWrite->bmiHeader.biSizeImage=0;
    }


    if ((lpBmiRead->bmiHeader.biBitCount==32) || (lpBmiRead->bmiHeader.biBitCount==24))
    {
        DWORD dwSizePixelRead = (lpBmiRead->bmiHeader.biBitCount==32)?4:3;
        lpBmiWrite->bmiHeader.biSizeImage=0;

        for (k=0;k<dwNbTransparencyPaint;k++)
        {
            TRANSPARENCY_PAINT CurTransPaint=*(pTransPaint+k);
            BOOL fDoAlphaBkgnd = (CurTransPaint.AlphaStrategy == ALPHA_MONOCOLOR) && (CurTransPaint.dwNbClrBkGnd==1);
            COLORREF clrBkGnd=RGB(255,255,255);

            if ((CurTransPaint.AlphaStrategy == ALPHA_MONOCOLOR) && (CurTransPaint.dwNbClrBkGnd==1))
            {
                if (CurTransPaint.lpClrBkGnd==NULL)
                    clrBkGnd = CurTransPaint.clrMonoColor;
                else
                    clrBkGnd = *(CurTransPaint.lpClrBkGnd);
            }

            for (i=0;i<(DWORD)absoluteHeight;i++)
            {
                DWORD dwPosLine = i + (absoluteHeight*((dwNbTransparencyPaint-k)-1));
                LPBYTE lpPosBmpWrite = lpOffBitsWrite + (AroundUpper((lpBmiRead->bmiHeader.biWidth*3),4)*dwPosLine);
				LPBYTE lpPosBmpRead = lpOffBitsRead +
					((!fHeightInverted) ? (AroundUpper((lpBmiRead->bmiHeader.biWidth*dwSizePixelRead), 4)*i) :
						(AroundUpper((lpBmiRead->bmiHeader.biWidth*dwSizePixelRead), 4)*(absoluteHeight - i)));

                if ((CurTransPaint.AlphaStrategy == ALPHA_VERTICAL) && (CurTransPaint.dwNbClrBkGnd==((DWORD)absoluteHeight)))
                {
                    fDoAlphaBkgnd=TRUE;
                    clrBkGnd = *(CurTransPaint.lpClrBkGnd+i);
                }

                for (j=0;j<(DWORD)lpBmiRead->bmiHeader.biWidth;j++)
                {
                    //RGBTRIPLE rgbTripeOld ;//= *(RGBTRIPLE*)lpPosBmp;
                    RGBTRIPLE* prgbTriple=(RGBTRIPLE*)lpPosBmpWrite;
                    RGBQUAD rgbQuadOld;// = *(RGBQUAD*)lpOldPosBmp;
					if ((lpBmiRead->bmiHeader.biCompression==BI_BITFIELDS) && (dwSizePixelRead==4))
					{
						DWORD sz3=sizeof(BITMAPINFOHEADER);
						DWORD sz4=sizeof(BITMAPV4HEADER);
						DWORD dwValue=*(DWORD*)lpPosBmpRead;

						DWORD dwBlueMask=((BITMAPV4HEADER*)(&(lpBmiRead->bmiHeader)))->bV4BlueMask;
						DWORD dwRedMask=((BITMAPV4HEADER*)(&(lpBmiRead->bmiHeader)))->bV4RedMask;
						DWORD dwGreenMask=((BITMAPV4HEADER*)(&(lpBmiRead->bmiHeader)))->bV4GreenMask;
						DWORD dwAlphaMask=((BITMAPV4HEADER*)(&(lpBmiRead->bmiHeader)))->bV4AlphaMask;

			
                        rgbQuadOld.rgbBlue = 0;
                        rgbQuadOld.rgbGreen = 0;
                        rgbQuadOld.rgbRed = 0;
                        rgbQuadOld.rgbReserved = (BYTE)(0x0);
						//dwValue=0x12345678;
						if (dwBlueMask>=0xff)
							rgbQuadOld.rgbBlue = (BYTE) ((dwValue & dwBlueMask) / (dwBlueMask/0xff));
						if (dwRedMask>=0xff)
							rgbQuadOld.rgbRed = (BYTE) ((dwValue & dwRedMask) / (dwRedMask/0xff));
						if (dwGreenMask>=0xff)
							rgbQuadOld.rgbGreen = (BYTE) ((dwValue & dwGreenMask) / (dwGreenMask/0xff));
						if (dwAlphaMask>=0xff)
							rgbQuadOld.rgbReserved = (BYTE) ((dwValue & dwAlphaMask) / (dwAlphaMask/0xff));

						//rgbQuadOld.rgbRed=250;
						//rgbQuadOld.rgbBlue=20;
						//rgbQuadOld.rgbGreen=20;
						/*
						if (j>40)
						  rgbQuadOld.rgbReserved=(BYTE)(0x100-(int)rgbQuadOld.rgbReserved);

						if (rgbQuadOld.rgbReserved!=0)
							rgbQuadOld.rgbReserved+=0;
						rgbQuadOld.rgbReserved=240;
			
						*/
						//rgbQuadOld = *(RGBQUAD*)lpPosBmpRead;
			
						if ((rgbQuadOld.rgbReserved!=0) &&  (rgbQuadOld.rgbReserved!=0xff))
							rgbQuadOld.rgbReserved+=0;
						rgbQuadOld.rgbReserved=0xff;
					}
					else
                    if (dwSizePixelRead==4)
					{
                        rgbQuadOld = *(RGBQUAD*)lpPosBmpRead;
			
						if ((rgbQuadOld.rgbReserved!=0) &&  (rgbQuadOld.rgbReserved!=0xff))
							rgbQuadOld.rgbReserved+=0;
					}
                    else
                    {
                        rgbQuadOld.rgbBlue = ((RGBTRIPLE*)lpPosBmpRead)->rgbtBlue;
                        rgbQuadOld.rgbGreen = ((RGBTRIPLE*)lpPosBmpRead)->rgbtGreen;
                        rgbQuadOld.rgbRed = ((RGBTRIPLE*)lpPosBmpRead)->rgbtRed;
                        rgbQuadOld.rgbReserved = (BYTE)(0xffU);
                    }

                    if ((CurTransPaint.AlphaStrategy == ALPHA_HORIZONTAL) && (CurTransPaint.dwNbClrBkGnd==((DWORD)lpBmiRead->bmiHeader.biWidth)))
                    {
                        fDoAlphaBkgnd=TRUE;
                        clrBkGnd = *(CurTransPaint.lpClrBkGnd+j);
                    }

                    if (fMagicColorToReplace && fDoAlphaBkgnd)
                        if ((rgbQuadOld.rgbBlue == GetBValue(clrToReplace)) &&
                            (rgbQuadOld.rgbGreen== GetGValue(clrToReplace)) &&
                            (rgbQuadOld.rgbRed  == GetRValue(clrToReplace)))
                                rgbQuadOld.rgbReserved = (BYTE)(0x00U);

                    DWORD dwAlpha = (DWORD)rgbQuadOld.rgbReserved;
                    if (fDoAlphaBkgnd || (dwAlpha==((BYTE)(0xffU))))
                    {
                        prgbTriple->rgbtBlue =  (BYTE)(((rgbQuadOld.rgbBlue*dwAlpha) / 255) +
                                                    ((GetBValue(clrBkGnd)*(255-dwAlpha))/255));
                        prgbTriple->rgbtGreen = (BYTE)(((rgbQuadOld.rgbGreen*dwAlpha) / 255) +
                                                    ((GetGValue(clrBkGnd)*(255-dwAlpha))/255));
                        prgbTriple->rgbtRed =  (BYTE)(((rgbQuadOld.rgbRed*dwAlpha) / 255) +
                                                    ((GetRValue(clrBkGnd)*(255-dwAlpha))/255));
                    }
                    else
                    {
                        prgbTriple->rgbtBlue = rgbQuadOld.rgbBlue;
                        prgbTriple->rgbtGreen = rgbQuadOld.rgbGreen;
                        prgbTriple->rgbtRed = rgbQuadOld.rgbRed;
                    }

                    lpPosBmpWrite+=3;
                    lpPosBmpRead+=dwSizePixelRead;
                }
            }
        }
    }
    else
        memcpy(lpOffBitsWrite,lpOffBitsRead,dwSize-dwOffBits);

    BOOL fNeedCreateDelHdc = hDc==NULL;
    if (fNeedCreateDelHdc)
        hDc=GetDC(NULL);



/*
    if (lpBmiWrite->bmiHeader.biBitCount==24)
    {
      hBmp = CreateBitmap(lpBmiWrite->bmiHeader.biWidth,lpBmiWrite->bmiHeader.biHeight,  1, 24, lpOffBitsWrite);
      SetBitmapBits(hBmp,AroundUpper(lpBmiWrite->bmiHeader.biWidth*3,2)*lpBmiWrite->bmiHeader.biHeight,lpOffBitsWrite);
    }
    else*/

    LPBITMAPINFO lpBmiAligned = (LPBITMAPINFO)GlobalAllocPtr(GHND,dwSizeBitmapHeader);
    memcpy(lpBmiAligned,lpBmiWrite,dwSizeBitmapHeader);
    //hBmp=CreateDIBitmap(hDc,&lpBmiWrite->bmiHeader,CBM_INIT,lpOffBitsWrite,lpBmiWrite,DIB_RGB_COLORS);
	if (lpBmiAligned->bmiHeader.biCompression==BI_BITFIELDS)
	{
		BITMAPV4HEADER * tst=(BITMAPV4HEADER *)lpBmiAligned;
		DWORD sz=sizeof(BITMAPV4HEADER);
		lpBmiAligned->bmiHeader.biCompression=BI_RGB;
	}
    hBmp=CreateDIBitmap(hDc,&lpBmiAligned->bmiHeader,CBM_INIT,lpOffBitsWrite,lpBmiAligned,DIB_RGB_COLORS);
    GlobalFreePtr(lpBmiAligned);


    FreeGzDecompressedMem(lpBuf);

    GlobalFreePtr(lpBufNew);
    if ((fNeedCreateDelHdc)&&(hDc!=NULL))
        ReleaseDC(NULL,hDc);

    return hBmp;
}

/*
HBITMAP LoadBitmapRes(HINSTANCE hInstance,LPCSTR lpBitmapName,LPCTSTR lpType,
                      HDC hDc,
                      ALPHA_STRATEGY AlphaStrategy,
                      DWORD dwNbClrBkGnd,const COLORREF* lpClrBkGnd,
                      BOOL fMagicColorToReplace,COLORREF clrToReplace)
{
    TRANSPARENCY_PAINT TransPaint;
    TransPaint.AlphaStrategy = AlphaStrategy;
    TransPaint.dwNbClrBkGnd = dwNbClrBkGnd;
    TransPaint.lpClrBkGnd = lpClrBkGnd;
    return LoadBitmapResMutltiTrans(hInstance,lpBitmapName,lpType,
                                 hDc,
                                 1,&TransPaint,
                                 fMagicColorToReplace,clrToReplace);
}
*/

HBITMAP LoadBitmapResMutltiTransAuto(IN HINSTANCE hInstance,IN LPCTSTR lpBitmapName,
                                     HDC hDc,
                                     DWORD dwNbTransparencyPaint,const TRANSPARENCY_PAINT* pTransPaint,
                                     BOOL fMagicColorToReplace,COLORREF clrToReplace)
{

    HBITMAP hBmpRes=NULL;
#ifndef ZLIB_NO_PRESENT
    hBmpRes = LoadBitmapResMutltiTrans(hInstance,lpBitmapName,_T("BMPGZDATA"),
                                        hDc,dwNbTransparencyPaint,pTransPaint,
                                        fMagicColorToReplace,clrToReplace);
#endif
    if (hBmpRes == NULL)
        hBmpRes =  LoadBitmapResMutltiTrans(hInstance,lpBitmapName,RT_BITMAP,
                                        hDc,dwNbTransparencyPaint,pTransPaint,
                                        fMagicColorToReplace,clrToReplace);
    return hBmpRes ;
}
//#endif

BOOL FindLine(LPCTSTR lpBuf,LPCTSTR lpSearch,DWORD dwSizeBuf,DWORD &dwPosFind)
{
TCHAR szTxt[256];
TCHAR szSearch[256];
int ilnsrc=lstrlen(lpSearch);
DWORD i;
  szTxt[ilnsrc]='\0';

  lstrcpy(szSearch,lpSearch);
  CnvMaj(szSearch);
  for (i=0;i+ilnsrc-1<dwSizeBuf;i++)
  {
  TCHAR c;

      c= ((*(lpBuf+i)));
      if ((c!=szSearch[0]) && (c!=(szSearch[0]+0x20))) continue;


                   hmemcpy(szTxt,lpBuf+i,ilnsrc);
                   CnvMaj(szTxt);
#ifdef UNICODE
                   if (wcsncmp(szTxt,szSearch,ilnsrc) == 0)
#else
                   if (strncmp(szTxt,szSearch,ilnsrc) == 0)
#endif
                   {
                   if (i>0)
                       if ((lpBuf[i-1]!='\xd') && (lpBuf[i-1]!='\xa'))
                         break;
                   dwPosFind=i;
                   return TRUE;
                   }
  }
  return FALSE;
}


#ifdef WIN32
#ifndef WIMA_SFX
// -----------------------------------------------------------------------
//  FUNCTION    RegDeleteKeyNT
//  PURPOSE     delete registry key and its subkeys
// -----------------------------------------------------------------------
//  INPUT       hStartKey       start registry key
//              pKeyName        name of key to delete
//
//  OUTPUT      DWORD           status
// -----------------------------------------------------------------------
//  COMMENTS    required for NT as it doesn't provide subkey deletion
// -----------------------------------------------------------------------

#ifndef MAX_KEY_LENGTH
#define MAX_KEY_LENGTH 256
#endif
DWORD RegDeleteKeyNT(HKEY hStartKey , LPCTSTR pKeyName)
{
  DWORD   dwReturn, dwSubKeyLength;
  LPTSTR  pSubKey = NULL;
  TCHAR   szSubKey[MAX_KEY_LENGTH]; // (256) this should be dynamic.
  HKEY    hKey;

  // do not allow NULL or empty key name
  if(pKeyName &&  lstrlen(pKeyName))
  {
    dwReturn = RegOpenKeyEx(hStartKey, pKeyName, 0, KEY_ALL_ACCESS, &hKey);

    while(ERROR_SUCCESS == dwReturn)
    {
      dwSubKeyLength = MAX_KEY_LENGTH;
      dwReturn = RegEnumKeyEx(hKey, 0, szSubKey, &dwSubKeyLength, NULL, NULL, NULL, NULL);

      if(ERROR_NO_MORE_ITEMS == dwReturn)
      {
        dwReturn = RegDeleteKey(hStartKey, pKeyName);
        break;
      }
      else if(ERROR_SUCCESS == dwReturn)
        dwReturn = RegDeleteKeyNT(hKey, szSubKey);
    }
    RegCloseKey(hKey);
  }
  else
    dwReturn = ERROR_BADKEY;

  return dwReturn;
}


// -----------------------------------------------------------------------
//  FUNCTION    DeleteExecutableBF
//  PURPOSE     delete running executable using a batch file
// -----------------------------------------------------------------------
//  INPUT       void
//
//  OUTPUT      void
// -----------------------------------------------------------------------
//  COMMENTS    code stolen/addapted from Jeffrey Richter's column in
//              Microsoft System Journal - January 1996
// -----------------------------------------------------------------------


#ifndef _T
#define _T(a) __TEXT(a)
#endif
#define DEL_UNINSTALL_BAT       _T("\\KillUnin.bat")

void DeleteExecutableBF(void)
{
  HANDLE hfile;
  STARTUPINFO si;
  PROCESS_INFORMATION pi;

   // create a batch file that continuously attempts to delete our executable file
   // when the executable no longer exists, remove its containing subdirectory,
   // and then delete the batch file too.
   hfile = CreateFile(DEL_UNINSTALL_BAT, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
   if(INVALID_HANDLE_VALUE != hfile)
   {
      TCHAR szBatFile[1000];
      TCHAR szUnsetupPathname[MAX_PATH];
      TCHAR szUnsetupPath[MAX_PATH];
      DWORD dwNumberOfBytesWritten;
      LPTSTR lpTrail=NULL;

      // get the full pathname of our executable file.
      GetModuleFileName(NULL, szUnsetupPathname, MAX_PATH);

      // get the path of the executable file (without the filename)


      GetFullPathName(szUnsetupPathname,MAX_PATH,szUnsetupPath,&lpTrail);
      *lpTrail='\0';
      if (lpTrail>szUnsetupPath)
          if (*(lpTrail-1)=='\\')
              *(lpTrail-1)='\0';


      // construct the lines for the batch file
      wsprintf(szBatFile,
         _T(":REPEAT\r\n")
         _T("DEL \"%s\"\r\n")
         _T("IF EXIST \"%s\" GOTO REPEAT\r\n")
         _T("RD \"%s\"\r\n")
         _T("DEL \"%s\"\r\n"),
         szUnsetupPathname, szUnsetupPathname, szUnsetupPath, DEL_UNINSTALL_BAT);

      // write the batch file and close it
      WriteFile(hfile, szBatFile, lstrlen(szBatFile) * sizeof(TCHAR), &dwNumberOfBytesWritten, NULL);
      CloseHandle(hfile);

      // get ready to spawn the batch file we just created.
      ZeroMemory(&si, sizeof(si));
      si.cb = sizeof(si);

      // we want its console window to be invisible to the user.
      si.dwFlags = STARTF_USESHOWWINDOW;
      si.wShowWindow = SW_HIDE;

      // spawn the batch file with low-priority and suspended.
      if(CreateProcess(NULL, DEL_UNINSTALL_BAT, NULL, NULL, FALSE, CREATE_SUSPENDED | IDLE_PRIORITY_CLASS, NULL, _T("\\"), &si, &pi))
      {
        // lower the batch file's priority even more.
        SetThreadPriority(pi.hThread, THREAD_PRIORITY_IDLE);

        // raise our priority so that we terminate as quickly as possible.
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
        SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

        // allow the batch file to run and clean-up our handles.
        CloseHandle(pi.hProcess);
        ResumeThread(pi.hThread);

        // we want to terminate right away now so that we can be deleted
        CloseHandle(pi.hThread);
      }
   }
}

#endif
#endif



int MessageBoxWithSourceLine(HWND hWnd,LPCTSTR lpText,LPCTSTR lpCaption,UINT uType,LPCSTR lpszSourceFileName,DWORD dwLine)
{
#ifdef ADD_SOURCE_INFO_BOX
    LPTSTR lpszTextDisp;
    if (lpText==NULL)
    {
        lpszTextDisp=(LPTSTR)malloc((0x100)*sizeof(TCHAR));
        *lpszTextDisp=0;
    }
    else
    {
        lpszTextDisp=(LPTSTR)malloc((lstrlen(lpText)+0x100)*sizeof(TCHAR));
        lstrcpy(lpszTextDisp,lpText);
    }
    if (lpszSourceFileName!=NULL)
      wsprintf(lpszTextDisp+lstrlen(lpszTextDisp),"\n\nSource file = %s line %d",lpszSourceFileName,dwLine);
    int iret=MessageBox(hWnd,lpszTextDisp,lpCaption,uType);
    free((void*)lpszTextDisp);
    return iret;
#else
    return MessageBox(hWnd,lpText,lpCaption,uType);
#endif
}

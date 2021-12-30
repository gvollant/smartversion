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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stddef.h>
#include <stdio.h>

#include "../lib/engine/patchstream/common/difbasic.h"
#include "../lib/engine/patchstream/common/DfsTlTyp.h"
#include "../lib/engine/patchstream/common/difstool.h"
#include "../lib/engine/patchstream/common/DfsType.h"
#include "../lib/engine/svfile/common/DfsTagDf.h"
#include "../lib/engine/svfile/common/DfsTagMg.h"
#include "../lib/engine/svfile/common/DfsTagBlockFloatEnd.h"


#include "../lib/engine/patchstream/common/DfsOrigMemoryMap.h"
#include "../lib/engine/patchstream/common/DfsIoHlp.h"
#include "../lib/engine/svfile/common/DfsIntf.h"

#include "../lib/engine/patchstream/common/difstrm.h"
#include "../lib/engine/patchstream/decompress/ApDifStm.h"
#include "../lib/engine/patchstream/decompress/apldifst.h"
#include "../lib/engine/patchstream/compress/makdifst.h"
#include "../lib/engine/patchstream/rebuild/RamDifWk.h"
#include "../lib/engine/patchstream/rebuild/RamDifTl.h"
#include "../lib/engine/patchstream/rebuild/RamDifWS.h"
#include "DfsCdLin.h"
#include "RawCompress.h"


#if ((!defined(RAWCOMPRESSDIRECT)) && (!defined(SVF_EXTRACT_ONLY)))
#define RAWCOMPRESSDIRECT 1
#endif

#if (!defined(RAWUNCOMPRESSDIRECT))
#define RAWUNCOMPRESSDIRECT 1
#endif

#ifdef RAWCOMPRESSDIRECT
#include "../lib/engine/patchstream/compress/abstractCompress.h"
#endif

#ifdef RAWUNCOMPRESSDIRECT
#include "../lib/engine/patchstream/common/abstractDecompress.h"
#endif

#include <windows.h>
//#include "mdump.h"
//#include "zlib.h"

#include "../lib/helper/compress/BuildHelper.h"
#include "../lib/helper/decompress/ExtractHelper.h"

#define COMPRESS_RATIO (2)

BOOL MyGetFileSize(LPCSTR lpFn, DWORD * dwSize,DWORD * lpdwSizeHigh)
{
  WIN32_FIND_DATA ffblk;
  HANDLE hFind;
  char szName[MAX_PATH + 1];


  lstrcpy(szName, lpFn);
  *dwSize = 0;

  if ((hFind = FindFirstFile(szName, &ffblk)) == INVALID_HANDLE_VALUE)
    return FALSE;
  else
  {
    *dwSize = ffblk.nFileSizeLow;
    if (lpdwSizeHigh!=NULL)
        *lpdwSizeHigh = ffblk.nFileSizeHigh;
  }
  FindClose(hFind);

  return TRUE;
}



BOOL TestPatchFileFalse(LPCSTR lpFileOld, LPCSTR lpFileNew,
                        dfuLong32 CompressRatio);
BOOL TestPatchFile(LPCSTR lpFileOld, LPCSTR lpFileNew, dfuLong32 CompressRatio);
BOOL TestPatch(dfvoidp pold, dfuLong32 size_old, dfvoidp pnew,
               dfuLong32 size_new, const char *szFnFormat,
               const char *szTitleFormat, dfuLong32 CompressRatio);


BOOL
TestPatchFileFalse(LPCSTR lpFileOld, LPCSTR lpFileNew, dfuLong32 CompressRatio)
{
  DWORD dwSizeFileOld, dwSizeFileNew;
  dfbytep pOld, pNew;
  HFILE hf;
  BOOL fRet;
  OFSTRUCT of;

  //fRet = MyGetFileSize(lpFileOld,&dwSizeFileOld) &&
  //       MyGetFileSize(lpFileNew,&dwSizeFileNew);
  dwSizeFileNew = 83728;
  dwSizeFileOld = 85776;
  fRet = TRUE;
  pOld = malloc(dwSizeFileOld + 1);
  pNew = malloc(dwSizeFileNew + 1);

  hf = OpenFile(lpFileOld, &of, OF_READ);
  if (hf != HFILE_ERROR)
  {
    if (dwSizeFileOld != _lread(hf, pOld, dwSizeFileOld))
      fRet = FALSE;
    _lclose(hf);
  }
  else
    fRet = FALSE;

  hf = OpenFile(lpFileNew, &of, OF_READ);
  if (hf != HFILE_ERROR)
  {
#ifdef BOUND_CHECKER
    memset(pNew, 1, dwSizeFileNew);
    memset(pNew, 2, dwSizeFileNew);
#endif

    if (dwSizeFileNew != _lread(hf, pNew, dwSizeFileNew))
      fRet = FALSE;

#ifdef BOUND_CHECKER
    if ((fRet) && (dwSizeFileNew > 4745))
    {
      dfbyte v;
      v = *(pNew + 1 + 4740);
    }
#endif
    _lclose(hf);
  }
  else
    fRet = FALSE;
  return fRet;
}


BOOL TestPatchFileNoMap(LPCSTR lpFileOld, LPCSTR lpFileNew, dfuLong32 CompressRatio)
{
  DWORD dwSizeFileOld, dwSizeFileNew;
  DWORD dwSizeFileOldHigh, dwSizeFileNewHigh;
  dfbytep pOld, pNew;
  HANDLE hf;
  BOOL fRet;

  fRet = MyGetFileSize(lpFileOld, &dwSizeFileOld,&dwSizeFileOldHigh) &&
    MyGetFileSize(lpFileNew, &dwSizeFileNew,&dwSizeFileNewHigh);

  pOld = malloc(dwSizeFileOld + 1);
  pNew = malloc(dwSizeFileNew + 1);

  hf =
    CreateFile(lpFileOld, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
               FILE_FLAG_SEQUENTIAL_SCAN, NULL);
  if (hf != NULL)
  {
    DWORD dwNbRead = 0;
    if (!ReadFile(hf, pOld, dwSizeFileOld, &dwNbRead, NULL))
      fRet = FALSE;
    if (dwNbRead != dwSizeFileOld)
      fRet = FALSE;
    CloseHandle(hf);
  }
  else
    fRet = FALSE;

  hf =
    CreateFile(lpFileNew, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
               FILE_FLAG_SEQUENTIAL_SCAN, NULL);
  if (hf != NULL)
  {
    DWORD dwNbRead = 0;

    if (!ReadFile(hf, pNew, dwSizeFileNew, &dwNbRead, NULL))
      fRet = FALSE;
    if (dwNbRead != dwSizeFileNew)
      fRet = FALSE;

#ifdef BOUND_CHECKER
    if ((fRet) && (dwSizeFileNew > 4745))
    {
      dfbyte v;
      v = *(pNew + 1 + 4740);
    }
#endif

    CloseHandle(hf);
  }
  else
    fRet = FALSE;

  if (fRet)
  {
    char szFnFormat[512];
    char szTitleFormat[512];

    sprintf(szFnFormat, "%s_%%d.txt", lpFileNew);
    sprintf(szTitleFormat, "Stream %%d of patch %s to %s", lpFileOld,
            lpFileNew);



    fRet =
      TestPatch(pOld, dwSizeFileOld, pNew, dwSizeFileNew, szFnFormat,
                szTitleFormat, CompressRatio);
  }

  free(pOld);
  free(pNew);

  return fRet;
}

BOOL TestPatchFile(LPCSTR lpFileOld, LPCSTR lpFileNew, dfuLong32 CompressRatio)
{
  DWORD dwSizeFileOld, dwSizeFileNew;
  DWORD dwSizeFileOldHigh, dwSizeFileNewHigh;
  dfbytep pOld, pNew;
  //HANDLE hf;
  BOOL fRet;
  HFILECONTENTREADBUF hfcrOld,hfcrNew;
  ORIGDATA OrigOld,OrigNew;
  dfwchar szwFileOld[MAX_PATH+0x10];
  dfwchar szwFileNew[MAX_PATH+0x10];

  hfcrOld=hfcrNew=NULL;

  fRet = MyGetFileSize(lpFileOld, &dwSizeFileOld,&dwSizeFileOldHigh) &&
    MyGetFileSize(lpFileNew, &dwSizeFileNew,&dwSizeFileNewHigh);

  ConvertAnsiToUnicode(lpFileOld,szwFileOld,MAX_PATH);
  ConvertAnsiToUnicode(lpFileNew,szwFileNew,MAX_PATH);
  fRet = GetFileFullContentBuffer(szwFileOld,FILEFULLCONTENTBUFFER_READ,0,&hfcrOld,&OrigOld,NULL) &&
         GetFileFullContentBuffer(szwFileNew,FILEFULLCONTENTBUFFER_READ,0,&hfcrNew,&OrigNew,NULL) ;
  pOld = GetOrigDataPtrpData(&OrigOld);
  pNew = GetOrigDataPtrpData(&OrigNew);

  if (fRet)
  {
    char szFnFormat[512];
    char szTitleFormat[512];

    sprintf(szFnFormat, "%s_%%d.txt", lpFileNew);
    sprintf(szTitleFormat, "Stream %%d of patch %s to %s", lpFileOld,
            lpFileNew);



    fRet =
      TestPatch(pOld, dwSizeFileOld, pNew, dwSizeFileNew, szFnFormat,
                szTitleFormat, CompressRatio);
  }

  CloseFileFullContentBuffer(hfcrOld);
  CloseFileFullContentBuffer(hfcrNew);

  return fRet;
}



BOOL
TestPatch(dfvoidp pold, dfuLong32 size_old, dfvoidp pnew, dfuLong32 size_new,
          const char *szFnFormat, const char *szTitleFormat,
          dfuLong32 CompressRatio)
{
  WRITEDIF_STREAM wstr;
  WRITEDIF_STREAM *write_stream = &wstr;
  ORIGDATA org;
  ORIGDATAPTR porg = &org;
  BOOL fRet = FALSE;
  DWORD dwBegin;
  BOOL fFileIdentical=FALSE;

  unsigned __int64 beginTime64, endTime64, ticksPerSecond, ticks;

  dfuLong32 ul = 0;
  dfbytep pszBuf;
  dfbytep data_in;
  dfuLong32 size_new_buffer;
  dfuLong32 size_cpr_buffer;
  int err = DSERR_OK;
  memset(&wstr, 0, sizeof(WRITEDIF_STREAM));
  printf("size org=%u,new=%u, compres param=%d\n",size_old,size_new,CompressRatio);

  QueryPerformanceCounter((LARGE_INTEGER *) & beginTime64);

  dwBegin = GetTickCount();

  size_cpr_buffer = size_new + 0x100;
  //pszBuf = (dfbytep) malloc(size_cpr_buffer + 1);
  pszBuf = (dfbytep)VirtualAlloc(NULL,size_cpr_buffer + 1,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
  memset(pszBuf, 0, size_cpr_buffer);

  FillOrigDataForFullMemoryOrg(porg,pold,size_old);
//  porg->givePtr = NULL;
//  porg->freePtr = NULL;

  wstr.nbOrig = 1;
  wstr.OrigDataPtr = porg;

  {
      COMPRESSIONPARAM cpr;
      InitDefaultCompressionParam(&cpr);
      cpr.uZlibCompressRatio = 9;
      cpr.dfBlockCalcSizeSearch = CompressRatio;
      cpr.dfPhysicalMemoryKB = GetPhysicalMemoryKb()/4;
      /*
      if (size_old > 1024*1024*16)
          cpr.dfBlockCalcSizeSearch = 0x80;
*/
      err = MakeDifInit(&wstr, &cpr);      // 5 = compress ratio
  }
  if (err != DSERR_OK)
    printf("1:err = %ld\n", err);

#ifdef DO_STATIS
  EnableStatis(&wstr, 512);
#endif

  wstr.nbOrig = 1;
  wstr.OrigDataPtr = porg;
  wstr.pPerformOrigDataAnalysisCB = NULL;
  wstr.dfUserPtr = NULL;

  wstr.OrigDataPtr = porg;

  data_in = (dfbytep) pnew;
  wstr.in_data_stream.next_in = data_in;
  wstr.in_data_stream.avail_in = size_new;


  wstr.out_data_stream.next_out = (dfvoidp *) pszBuf;
  wstr.out_data_stream.avail_out = size_cpr_buffer;

  size_new_buffer = size_new + 0x100;


  {
#define STEP (0x8000*4)
    dfbyte szBuf[STEP];
    dfuLong32 already_done = 0;
    dfuLong32 dwTotal = size_new;
    dfuLong32 dfLatestShow = 0;
    dfuLong32 dfNow;
    dfuLong32 dfLatestPercent=0xffffffff;

    printf("\n00 %%");
    wstr.in_data_stream.avail_in=0;

    while ((dwTotal > 0) || (wstr.in_data_stream.avail_in>0))
    {
      dfuLong32 i;

      if ((dwTotal > 0) && (wstr.in_data_stream.avail_in==0))
      {
          wstr.in_data_stream.next_in = szBuf;
          wstr.in_data_stream.avail_in = STEP;
          if (dwTotal < STEP)
            wstr.in_data_stream.avail_in = dwTotal;

          #ifdef BOUND_CHECKER
          {
            dfbyte v;
            v = *(data_in + 1 + 4740);
            for (i = 0; i < wstr.in_data_stream.avail_in; i++)
              szBuf[i] = 1 + v;
            for (i = 0; i < wstr.in_data_stream.avail_in; i++)
              szBuf[i] = 2 + v;
          }

          #endif

          for (i = 0; i < wstr.in_data_stream.avail_in; i++)
            szBuf[i] = *(data_in + i + already_done);
          already_done += wstr.in_data_stream.avail_in;
          dwTotal -= wstr.in_data_stream.avail_in;
      }

      wstr.out_data_stream.avail_out = 0x8000;
      if (wstr.out_data_stream.total_out + wstr.out_data_stream.avail_out > size_cpr_buffer)
         wstr.out_data_stream.avail_out = size_cpr_buffer - ((dfuLong32)wstr.out_data_stream.total_out);

      memset(wstr.out_data_stream.next_out,0,wstr.out_data_stream.avail_out);
      err = DoMakeDifWork(&wstr);
      memset(wstr.out_data_stream.next_out,0,wstr.out_data_stream.avail_out);
/*
      #ifdef _DEBUG
      {
          dfuLong32 crcin =  crc32(0,((dfbytep)wstr.in_data_stream.next_in)-wstr.done_latest_in,wstr.done_latest_in);
          dfuLong32 crcout = crc32(0,((dfbytep)wstr.out_data_stream.next_out)-wstr.done_latest_out,wstr.done_latest_out);
          printf("crc in,out=%lx,%lx for length %u,%u\n",crcin,crcout,wstr.done_latest_in,wstr.done_latest_out);
      }
      #endif
*/
      dfNow = GetTickCount();
      if (dfLatestShow + 300 < dfNow)
      {
        dfuLong32 dfPercent;
        if (size_new > 0x100000)
            dfPercent = already_done / (size_new /100);
        else
            dfPercent = (already_done * 100) / size_new;
        if (dfLatestPercent != dfPercent)
            printf("\x0d%02u %%", dfPercent);
        dfLatestPercent = dfPercent;
        dfLatestShow = dfNow;
      }
      if (err != DSERR_OK)
        printf("2:err = %ld\n", err);
    }
    printf("\x0d    \x0d");

  }

  /*
     err = DoMakeDifWork(&wstr);
     if (err != DSERR_OK)
     printf("2:err = %ld\n",err);
   */
  err = DSERR_OK;
  while (err == DSERR_OK)
  {
      wstr.out_data_stream.avail_out = 0x8000;
      if (wstr.out_data_stream.total_out + wstr.out_data_stream.avail_out > size_cpr_buffer)
         wstr.out_data_stream.avail_out = size_cpr_buffer - (dfuLong32)wstr.out_data_stream.total_out;

    err = FlushMakeDif(&wstr);
/*
      #ifdef _DEBUG
      {
          dfuLong32 crcin =  crc32(0,((dfbytep)wstr.in_data_stream.next_in)-wstr.done_latest_in,wstr.done_latest_in);
          dfuLong32 crcout = crc32(0,((dfbytep)wstr.out_data_stream.next_out)-wstr.done_latest_out,wstr.done_latest_out);
          printf("Flush:crc in,out=%lx,%lx for length %u,%u\n",crcin,crcout,wstr.done_latest_in,wstr.done_latest_out);
      }
      #endif
*/
  }
  if (err != DSERR_END)
    printf("3:err = %ld\n", err);
/***************************************************************/
#ifdef DO_STATIS
  {
    char szFn[512];
    char szTitle[1024];
    dfuLong32 i;
    for (i = 0;; i++)
    {
      sprintf(szFn, (szFnFormat == NULL) ? "stat_str%d.txt" : szFnFormat, i);
      sprintf(szTitle,
              (szTitleFormat == NULL) ? "Statis stream %d" : szTitleFormat,
              i);
      if (FlushStatis(&wstr, i, szFn, szTitle) == 0)
        break;
    }
    FlushHuff(&wstr);
  }
#endif

  err = CloseMakeDif(&wstr, &ul,&fFileIdentical);
  if (err != DSERR_OK)
    printf("4:err = %ld\n", err);
  printf("identical = %u\n",fFileIdentical);


  QueryPerformanceCounter((LARGE_INTEGER *) & endTime64);
  ticks = (endTime64 - beginTime64);
  QueryPerformanceFrequency((LARGE_INTEGER *) & ticksPerSecond);

/*
  printf("difstrm test adler=%lx, size=%lu, time = %lu sec (%f msec)\n", ul,
         wstr.out_data_stream.total_out, (GetTickCount() - dwBegin) / 1000,
         ((long double) ((DWORD) (ticks / 1024))) /
         (long double) (DWORD) (ticksPerSecond / 1024));
*/

  {
  DWORD dwNbTick = GetTickCount() - dwBegin;

  printf("difstrm test adler=%x, size=%u, time = %u sec (%u msec)\n",
         (DWORD)(ul), (DWORD)wstr.out_data_stream.total_out,
         (DWORD)(dwNbTick /1000),(DWORD)dwNbTick/*, dwBegin, GetTickCount() */);
  /*
  printf("time = %u sec (%u msec) (%u,%u)\n",
         (DWORD)(dwNbTick /1000),(DWORD)dwNbTick, dwBegin, GetTickCount() );*/
//printf("\ndwBegin6: %u - %u - %u - %u\n",dwBegin,dwNbTick,dwBegin,GetTickCount());
  }

    /************************************************/
  printf("\n\n");

  {
    APPLYDIF_STREAM rstr;
    dfuLong32 ulcr = 0;
    dfuLong32 dfRestOut;
    dfbytep pszBufOut = (dfbytep) malloc(size_new_buffer + 1);

    memset(&rstr, 0, sizeof(APPLYDIF_STREAM));
    err = ApplyDifInit(&rstr);
    if (err != DSERR_OK)
      printf("11:err = %ld\n", err);


    rstr.out_data_stream.next_out = pszBufOut;
    rstr.out_data_stream.avail_out = size_new_buffer;
    rstr.OrigDataPtr = porg;

    rstr.in_data_stream.next_in = (dfvoidp *) pszBuf;
    dfRestOut = (dfuLong32)wstr.out_data_stream.total_out;
    while (dfRestOut > 0)
    {
      dfuLong32 dfThis = min(dfRestOut, (0x20000));
      rstr.in_data_stream.avail_in = dfThis;

      err = DoApplyDifWork(&rstr);
      if ((err != DSERR_END) && (err != DSERR_OK))
        printf("12:err = %ld\n", err);
      dfRestOut -= dfThis;
    }

    if ((err != DSERR_END))
      printf("12b:err = %ld\n", err);

    err = CloseApplyDif(&rstr, &ulcr);
    if (err != DSERR_OK)
      printf("13:err = %ld\n", err);

    if (wstr.out_data_stream.total_out != rstr.in_data_stream.total_in)
      printf("\nERROR NUMBER READ IN APPLY  out=%lu, in=%lu\n\n",
             (unsigned long)wstr.out_data_stream.total_out, (unsigned long)rstr.in_data_stream.total_in);

    {
      int icmp = memcmp(pszBufOut, data_in, (dfuIntPtr)rstr.out_data_stream.total_out);
      printf("size in,out = %lu,%lu, size cpr,old = %lu,%lu\n",
             (unsigned long)wstr.in_data_stream.total_in, (unsigned long)rstr.out_data_stream.total_out, (unsigned long)wstr.out_data_stream.total_out, (unsigned long)size_old);
      printf("res cmp=%ld, adler=%lx\n", (signed long)icmp, (unsigned long)ulcr);
      fRet = ((icmp == 0) && (wstr.in_data_stream.total_in == rstr.out_data_stream.total_out));
      if (fRet)
        printf("ok, patch is GOOD, the patch size is %lu (%lu KB)\n",
                       (unsigned long)(dfuLong32)wstr.out_data_stream.total_out,
                       (unsigned long)(dfuLong32)(wstr.out_data_stream.total_out/1024));
      else
        printf("none, patch is BAD, icmp=%ld, total_in=%lu, total_out=%lu\n",
               (signed long)icmp, (unsigned long)wstr.in_data_stream.total_in, (unsigned long)rstr.out_data_stream.total_out);
    }

    free(pszBufOut);
  }


  //free(pszBuf);
  VirtualFree(pszBuf,0,MEM_RELEASE | MEM_DECOMMIT);
  return fRet;
}

#define TPAroundUpper(dwValue,dwModulo) (((((dwValue)) + ((dwModulo)) -1) / ((dwModulo))) * (dwModulo))
TestPatchMapOldOnly(ORIGDATA* pOrg, dfuLong64 size_old, HANDLE hNew, dfuLong64 size_new,
          const char *szFnFormat, const char *szTitleFormat,
          dfuLong32 CompressRatio, BOOL fNoBuffering, BOOL fVirtualAlloc)
{
  WRITEDIF_STREAM wstr;
  WRITEDIF_STREAM *write_stream = &wstr;
//  ORIGDATA org;
  //ORIGDATAPTR porg = &org;
  BOOL fRet = FALSE;
  DWORD dwBegin;
  BOOL fFileIdentical=FALSE;
  unsigned __int64 beginTime64, endTime64, ticksPerSecond, ticks;
  dfuLong32 ul = 0;
  dfbytep pszBufPatch=NULL;
  dfuIntPtr size_new_buffer;
  dfuIntPtr size_cpr_buffer;
  int err = DSERR_OK;
  dfbytep pWorkBuf ;
  dfuLong32 dfStepEnlargeOutPatchBuffer = 0x8000*1;
  TCHAR sz[160];

  if (!fVirtualAlloc)
      dfStepEnlargeOutPatchBuffer*=4;

#define SIZE_BUFFER_DTPFNMN (1024*256)
#define STEP_NOMAP SIZE_BUFFER_DTPFNMN

  pWorkBuf = malloc(SIZE_BUFFER_DTPFNMN*2);
  if (pWorkBuf == NULL)
      return FALSE;

  memset(&wstr, 0, sizeof(WRITEDIF_STREAM));
  wsprintf(sz,"size org=%I64u, new=%I64u, compres param=%d\n",size_old,size_new,CompressRatio);
  printf("%s",sz);

  QueryPerformanceCounter((LARGE_INTEGER *) & beginTime64);

  dwBegin = GetTickCount();

  size_cpr_buffer = (dfuIntPtr)((size_new + 0x100) + (size_new/0x100) + dfStepEnlargeOutPatchBuffer);
  if (size_cpr_buffer < size_new)
      size_cpr_buffer = 1024*1024*1024; // 1 GB for 32 bit
  //pszBuf = (dfbytep) malloc(size_cpr_buffer + 1);
  if (fVirtualAlloc)
  {
      for (;;)
      {
         pszBufPatch = (dfbytep)VirtualAlloc(NULL,size_cpr_buffer + 1,MEM_RESERVE,PAGE_READWRITE);
         if (pszBufPatch != NULL)
             break;
         else size_cpr_buffer = size_cpr_buffer >> 1;
      }
  }
  //memset(pszBuf, 0, size_cpr_buffer);

//  FillOrigDataForFullMemoryOrg(porg,pold,size_old);
//  porg->givePtr = NULL;
//  porg->freePtr = NULL;

  wstr.nbOrig = 1;
  wstr.OrigDataPtr = pOrg;

  {
      COMPRESSIONPARAM cpr;

      InitDefaultCompressionParam(&cpr);
      cpr.uZlibCompressRatio = 9;
      cpr.dfBlockCalcSizeSearch = CompressRatio;
      cpr.dfPhysicalMemoryKB = GetPhysicalMemoryKb()/4;
      /*
      if (size_old > 1024*1024*16)
          cpr.dfBlockCalcSizeSearch = 0x80;
*/
      err = MakeDifInit(&wstr, &cpr);      // 5 = compress ratio
  }
  if (err != DSERR_OK)
    printf("1:err = %ld\n", err);

#ifdef DO_STATIS
  EnableStatis(&wstr, 512);
#endif

  wstr.nbOrig = 1;
  wstr.OrigDataPtr = pOrg;
  wstr.pPerformOrigDataAnalysisCB = NULL;
  wstr.dfUserPtr = NULL;

  wstr.OrigDataPtr = pOrg;




  //wstr.out_data_stream.next_out = (dfvoidp *) pszBufPatch;
  wstr.out_data_stream.avail_out = (dfuLong32)dfmin(size_cpr_buffer,0xffffffffUL);

  size_new_buffer = (dfuIntPtr)(size_new + 0x100);
  if (size_new_buffer < size_new)
      size_new_buffer = 1024*1024*1024; // 1 GB for 32 bit


  {
    dfbytep   szBuf = pWorkBuf;
    dfuLong64 already_done = 0;
    dfuLong64 dwTotal = size_new;
    dfuLong32 dfLatestShow = 0;
    dfuLong32 dfNow;
    dfuLong32 dfLatestPercent=0xffffffff;
    dfuIntPtr dfLatestCommitPosition=0;

    printf("\n00 %%");
    wstr.in_data_stream.avail_in=0;

    while ((dwTotal > 0) || (wstr.in_data_stream.avail_in>0))
    {
      if ((dwTotal > 0) && (wstr.in_data_stream.avail_in==0))
      {
          wstr.in_data_stream.next_in = szBuf;
          wstr.in_data_stream.avail_in = STEP_NOMAP;
          if (dwTotal < STEP_NOMAP)
            wstr.in_data_stream.avail_in = (dfuLong32)dwTotal;

          #ifdef BOUND_CHECKER
          {
            dfbyte v=0;
            dfuLong32 i;

            for (i = 0; i < wstr.in_data_stream.avail_in; i++)
              *(szBuf+i) = 1 + v;
            for (i = 0; i < wstr.in_data_stream.avail_in; i++)
              *(szBuf+i) = 2 + v;
          }

          #endif
/*
          for (i = 0; i < wstr.in_data_stream.avail_in; i++)
            *(szBuf+i) = *(data_in + i + already_done);
            */
          ReadFile(hNew,szBuf,fNoBuffering ?
                            TPAroundUpper(wstr.in_data_stream.avail_in,0x200) :
                            wstr.in_data_stream.avail_in,&wstr.in_data_stream.avail_in,NULL);
          already_done += wstr.in_data_stream.avail_in;
          dwTotal -= wstr.in_data_stream.avail_in;
      }
// we must commit wstr.out_data_stream.avail_out + wstr.out_data_stream.total_out
      wstr.out_data_stream.avail_out = dfStepEnlargeOutPatchBuffer;
      while (dfLatestCommitPosition < (wstr.out_data_stream.avail_out + wstr.out_data_stream.total_out))
      {
          if (fVirtualAlloc)
          {
            LPVOID lpRet=VirtualAlloc(pszBufPatch + dfLatestCommitPosition,dfStepEnlargeOutPatchBuffer,MEM_COMMIT,PAGE_READWRITE);
            memset(pszBufPatch + dfLatestCommitPosition,0xf6,dfStepEnlargeOutPatchBuffer);
            dfLatestCommitPosition += dfStepEnlargeOutPatchBuffer;
          }
          else
          {
              if (pszBufPatch == NULL)
                  pszBufPatch = malloc(dfLatestCommitPosition + dfStepEnlargeOutPatchBuffer);
              else
                  pszBufPatch = realloc(pszBufPatch,dfLatestCommitPosition + dfStepEnlargeOutPatchBuffer);
              dfLatestCommitPosition += dfStepEnlargeOutPatchBuffer;
          }
      }

      if (wstr.out_data_stream.total_out + wstr.out_data_stream.avail_out > size_cpr_buffer)
      {
         wstr.out_data_stream.avail_out = (dfuLong32)(size_cpr_buffer - ((dfuLong32)wstr.out_data_stream.total_out));
         if (wstr.out_data_stream.avail_out < (size_cpr_buffer - ((dfuLong32)wstr.out_data_stream.total_out)))
             wstr.out_data_stream.avail_out = 0xffffffff;
      }


      wstr.out_data_stream.next_out = (dfvoidp *)(((dfbytep)pszBufPatch)+wstr.out_data_stream.total_out);
      //memset(wstr.out_data_stream.next_out,0,wstr.out_data_stream.avail_out);

      err = DoMakeDifWork(&wstr);
      //memset(wstr.out_data_stream.next_out,0,wstr.out_data_stream.avail_out);
/*
      #ifdef _DEBUG
      {
          dfuLong32 crcin =  crc32(0,((dfbytep)wstr.in_data_stream.next_in)-wstr.done_latest_in,wstr.done_latest_in);
          dfuLong32 crcout = crc32(0,((dfbytep)wstr.out_data_stream.next_out)-wstr.done_latest_out,wstr.done_latest_out);
          printf("crc in,out=%lx,%lx for length %u,%u\n",crcin,crcout,wstr.done_latest_in,wstr.done_latest_out);
      }
      #endif
*/
      dfNow = GetTickCount();
      if (dfLatestShow + 300 < dfNow)
      {
        dfuLong32 dfPercent;
        if (size_new > 0x100000)
            dfPercent = (dfuLong32)(already_done / (size_new /100));
        else
            dfPercent = (dfuLong32)((already_done * 100) / size_new);
        if (dfLatestPercent != dfPercent)
            printf("\x0d%02u %%", dfPercent);
        dfLatestPercent = dfPercent;
        dfLatestShow = dfNow;
      }
      if (err != DSERR_OK)
        printf("2:err = %ld\n", err);
    }
    printf("\x0d    \x0d");
  }

  /*
     err = DoMakeDifWork(&wstr);
     if (err != DSERR_OK)
     printf("2:err = %ld\n",err);
   */
  err = DSERR_OK;
  while (err == DSERR_OK)
  {
      wstr.out_data_stream.avail_out = 0x8000;
      if (wstr.out_data_stream.total_out + wstr.out_data_stream.avail_out > size_cpr_buffer)
      {
         wstr.out_data_stream.avail_out = (dfuLong32)(size_cpr_buffer - wstr.out_data_stream.total_out);
         if (wstr.out_data_stream.avail_out < size_cpr_buffer - wstr.out_data_stream.total_out)
             wstr.out_data_stream.avail_out = 0xffffffff;
      }

    err = FlushMakeDif(&wstr);
/*
      #ifdef _DEBUG
      {
          dfuLong32 crcin =  crc32(0,((dfbytep)wstr.in_data_stream.next_in)-wstr.done_latest_in,wstr.done_latest_in);
          dfuLong32 crcout = crc32(0,((dfbytep)wstr.out_data_stream.next_out)-wstr.done_latest_out,wstr.done_latest_out);
          printf("Flush:crc in,out=%lx,%lx for length %u,%u\n",crcin,crcout,wstr.done_latest_in,wstr.done_latest_out);
      }
      #endif
*/
  }
  if (err != DSERR_END)
    printf("3:err = %ld\n", err);
  wsprintf(sz,"-> Size of patch : %I64u\n",wstr.out_data_stream.total_out);
  printf("%s",sz);
/***************************************************************/
#ifdef DO_STATIS
  {
    char szFn[512];
    char szTitle[1024];
    dfuLong32 i;
    for (i = 0;; i++)
    {
      sprintf(szFn, (szFnFormat == NULL) ? "stat_str%d.txt" : szFnFormat, i);
      sprintf(szTitle,
              (szTitleFormat == NULL) ? "Statis stream %d" : szTitleFormat,
              i);
      if (FlushStatis(&wstr, i, szFn, szTitle) == 0)
        break;
    }
    FlushHuff(&wstr);
  }
#endif

  err = CloseMakeDif(&wstr, &ul,&fFileIdentical);
  if (err != DSERR_OK)
    printf("4:err = %ld\n", err);


  QueryPerformanceCounter((LARGE_INTEGER *) & endTime64);
  ticks = (endTime64 - beginTime64);
  QueryPerformanceFrequency((LARGE_INTEGER *) & ticksPerSecond);

/*
  printf("difstrm test adler=%lx, size=%lu, time = %lu sec (%f msec)\n", ul,
         wstr.out_data_stream.total_out, (GetTickCount() - dwBegin) / 1000,
         ((long double) ((DWORD) (ticks / 1024))) /
         (long double) (DWORD) (ticksPerSecond / 1024));
*/

  {
  DWORD dwNbTick = GetTickCount() - dwBegin;

  printf("difstrm test adler=%x, size=%u, time = %u sec (%u msec)\n",
         (DWORD)(ul), (DWORD)wstr.out_data_stream.total_out,
         (DWORD)(dwNbTick /1000),(DWORD)dwNbTick/*, dwBegin, GetTickCount() */);
  /*
  printf("time = %u sec (%u msec) (%u,%u)\n",
         (DWORD)(dwNbTick /1000),(DWORD)dwNbTick, dwBegin, GetTickCount() );*/
//printf("\ndwBegin6: %u - %u - %u - %u\n",dwBegin,dwNbTick,dwBegin,GetTickCount());
  }

    /************************************************/
  printf("\n\n");

  {
    APPLYDIF_STREAM rstr;
    dfuLong32 ulcr = 0;
    dfuLong64 dfRestPatchData;
    dfuLong32 dfLatestPercent=0xffffffff;

   // dfbytep pszBufOut = (dfbytep) malloc(size_new_buffer + 1);
    dfuIntPtr size_total_reread=0;
    int icmp=0;

printf("Adler of PATCH =%x\n",DfsAdler32(DfsAdler32(0L, NULL, 0),pszBufPatch,(dfuLong32)wstr.out_data_stream.total_out));
    SetFilePointer(hNew,0,NULL,FILE_BEGIN);
    memset(&rstr, 0, sizeof(APPLYDIF_STREAM));
    err = ApplyDifInit(&rstr);
    if (err != DSERR_OK)
      printf("11:err = %ld\n", err);


    //rstr.next_out = pszBufOut;
    rstr.out_data_stream.avail_out = (dfuLong32)size_new_buffer;
    if (rstr.out_data_stream.avail_out < size_new_buffer)
        rstr.out_data_stream.avail_out = 0xffffffff;
    rstr.OrigDataPtr = pOrg;

    rstr.in_data_stream.next_in = (dfvoidp *) pszBufPatch;
    dfRestPatchData = wstr.out_data_stream.total_out;


    while ((dfRestPatchData > 0) || (size_total_reread<wstr.in_data_stream.total_in))
    {
      dfbytep dfNewRead;
      dfbytep dfNewRebuild;
      //dfuLong32 dfThisPatchData ;

      dfuLong32 avail_out_before_operation ;
      dfuLong32 size_this,size_operation;
      DWORD sizecmp=0;

      dfNewRead = pWorkBuf;
      dfNewRebuild = pWorkBuf+SIZE_BUFFER_DTPFNMN;


      rstr.out_data_stream.next_out = dfNewRebuild;
      avail_out_before_operation = STEP_NOMAP-0;
      rstr.out_data_stream.avail_out = avail_out_before_operation;

      do
      {
        dfuLong32 dfPatchDataProcessed ;
        dfuLong32 dfThisPatchData = (dfuLong32)(min(dfRestPatchData, (0x20000)));
        dfuLong32 dfPercent;
        rstr.in_data_stream.avail_in = dfThisPatchData;

        err = DoApplyDifWork(&rstr);
        dfPercent = 0;

        if (size_new > 0x100000)
            dfPercent = (dfuLong32)(rstr.out_data_stream.total_out / (size_new /100));
        else
            dfPercent = (dfuLong32)((rstr.out_data_stream.total_out * 100) / size_new);
        if (dfLatestPercent != dfPercent)
            printf("\x0d%02u %%", dfPercent);
        dfLatestPercent = dfPercent;



        dfPatchDataProcessed = (dfThisPatchData - rstr.in_data_stream.avail_in);
        dfRestPatchData -= dfPatchDataProcessed;
      } while ((rstr.out_data_stream.avail_out > 0) && (err==0));

      if ((err != DSERR_END) && (err != DSERR_OK))
        printf("12:err = %ld\n", err);
      //
      size_this = avail_out_before_operation-rstr.out_data_stream.avail_out;
      size_operation = fNoBuffering ? TPAroundUpper(size_this,0x200) : size_this;
      if (size_operation != size_this)
          printf("dif : %u,%u, avail=%u\n",size_operation,size_this,(DWORD)rstr.out_data_stream.avail_out);

      if (!ReadFile(hNew,dfNewRead,size_operation,&sizecmp,NULL))
          printf("error read, code=%u %u-%u\n",GetLastError(),size_operation,sizecmp);
      if ((size_operation) != sizecmp)
          printf("error read; code=%u %u,%u  -- %u\n",GetLastError(),size_operation,sizecmp,size_this);
      if (icmp==0)
          icmp = memcmp(dfNewRead,dfNewRebuild,sizecmp);
      size_total_reread += sizecmp;
    }


    if ((err != DSERR_END))
      printf("12b:err = %ld\n", err);

    err = CloseApplyDif(&rstr, &ulcr);
    if (err != DSERR_OK)
      printf("13:err = %ld\n", err);

    if (wstr.out_data_stream.total_out != rstr.in_data_stream.total_in)
      printf("\nERROR NUMBER READ IN APPLY  out=%lu, in=%lu\n\n",
             (unsigned long)wstr.out_data_stream.total_out, (unsigned long)rstr.in_data_stream.total_in);

    if (size_total_reread != rstr.out_data_stream.total_out)
      printf("\nERROR NUMBER READ IN APPLY (bis)  out=%lu, in=%lu\n\n",
             (unsigned long)wstr.out_data_stream.total_out, (unsigned long)rstr.in_data_stream.total_in);

    {
      //int icmp = memcmp(pszBufOut, data_in, (dfuIntPtr)rstr.total_out);

      wsprintf(sz,"size in,out = %I64u,%I64u, size cpr,old = %I64u,%I64u\n",
             wstr.in_data_stream.total_in, rstr.out_data_stream.total_out, wstr.out_data_stream.total_out, size_old);
      printf("%s",sz);
      wsprintf(sz,"res cmp=%ld, adler=%lx\n", icmp, ulcr);
      printf("%s",sz);
      fRet = ((icmp == 0) && (wstr.in_data_stream.total_in == rstr.out_data_stream.total_out));
      if (fRet)
        wsprintf(sz,"ok, patch is GOOD, the patch size is %I64u (%I64u KB)\n", wstr.out_data_stream.total_out, (wstr.out_data_stream.total_out/1024));
      else
        wsprintf(sz,"\n--> BAD <-- : none, patch is BAD, icmp=%u, total_in=%I64u, total_out=%I64u\n",
               icmp, wstr.in_data_stream.total_in, rstr.out_data_stream.total_out);
      printf("%s",sz);
    }

    //free(pszBufOut);
  }

  //free(pszBuf);
  if (fVirtualAlloc)
    VirtualFree(pszBufPatch,0,MEM_RELEASE | MEM_DECOMMIT);
  else
      if (pszBufPatch != NULL)
          free(pszBufPatch);
  free(pWorkBuf);
  return fRet;
}


BOOL TestPatchFileNoMapNew(LPCSTR lpFileOld, LPCSTR lpFileNew, dfuLong32 CompressRatio,BOOL fNoBuffering,BOOL fVirtualAlloc)
{
  dfuLong64 dwSizeFileOld, dwSizeFileNew;
  DWORD dwSizeFileOldLow, dwSizeFileNewLow;
  DWORD dwSizeFileOldHigh, dwSizeFileNewHigh;
  //dfbytep pOld ;
  //HANDLE hf;
  BOOL fRet;
  HFILECONTENTREADBUF hfcrOld,hfcrNew;
  dfwchar szwFileOld[MAX_PATH+0x10];
  dfwchar szwFileNew[MAX_PATH+0x10];
  HANDLE hNew;
  ORIGDATA Orig;

  hfcrOld=hfcrNew=NULL;

  fRet = MyGetFileSize(lpFileOld, &dwSizeFileOldLow,&dwSizeFileOldHigh) &&
    MyGetFileSize(lpFileNew, &dwSizeFileNewLow,&dwSizeFileNewHigh);
  dwSizeFileOld = dwSizeFileOldLow | (((dfuLong64)dwSizeFileOldHigh) << 32);
  dwSizeFileNew = dwSizeFileNewLow | (((dfuLong64)dwSizeFileNewHigh) << 32);

  ConvertAnsiToUnicode(lpFileOld,szwFileOld,MAX_PATH);
  ConvertAnsiToUnicode(lpFileNew,szwFileNew,MAX_PATH);

  hNew = CreateFile(lpFileNew,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,
                    FILE_FLAG_SEQUENTIAL_SCAN | (fNoBuffering ? FILE_FLAG_NO_BUFFERING:0),
                    NULL);
  if (hNew == INVALID_HANDLE_VALUE)
    hNew = NULL;

  fRet = GetFileFullContentBuffer(szwFileOld,FILEFULLCONTENTBUFFER_READ,0,&hfcrOld,&Orig,NULL) && (hNew!=NULL);

  if ((!fRet) && (hNew!=NULL))
      printf("unable to map %s in memory (error %u)\n",lpFileOld,GetLastError());

  if (fRet)
  {
    char szFnFormat[512];
    char szTitleFormat[512];

    //pOld = (GetOrigDataPtrpData(&Orig));

    sprintf(szFnFormat, "%s_%%d.txt", lpFileNew);
    sprintf(szTitleFormat, "Stream %%d of patch %s to %s", lpFileOld,
            lpFileNew);

    fRet =
      TestPatchMapOldOnly(&Orig, dwSizeFileOld, hNew, dwSizeFileNew, szFnFormat,
                szTitleFormat, CompressRatio, fNoBuffering,fVirtualAlloc);
  }

  CloseFileFullContentBuffer(hfcrOld);
  CloseHandle(hNew);

  return fRet;
}

/*****************************************************************************/



BOOL
TestPatchRamDif(dfvoidp pold, dfuLong32 size_old, dfvoidp pnew, dfuLong32 size_new,
          const char *szFnFormat, const char *szTitleFormat,
          dfuLong32 CompressRatio)
{
  WRITEDIF_STREAM wstr;
  WRITEDIF_STREAM *write_stream = &wstr;
  ORIGDATA org;
  ORIGDATAPTR porg = &org;
  BOOL fRet = FALSE;
  DWORD dwBegin;
  BOOL fFileIdentical=FALSE;
  HRAMDIF hRamDif;

  unsigned __int64 beginTime64, endTime64, ticksPerSecond, ticks;

  dfuLong32 ul = 0;
  dfbytep pszBuf;
  dfbytep data_in;
  dfuLong32 size_new_buffer;
  dfuLong32 size_cpr_buffer;
  int err = DSERR_OK;
  memset(&wstr, 0, sizeof(WRITEDIF_STREAM));
  printf("TestPatchRamDif: size org=%u,new=%u, compres param=%d\n",size_old,size_new,CompressRatio);

  QueryPerformanceCounter((LARGE_INTEGER *) & beginTime64);

  dwBegin = GetTickCount();

  size_cpr_buffer = size_new + 0x100;
  //pszBuf = (dfbytep) malloc(size_cpr_buffer + 1);
  pszBuf = (dfbytep)VirtualAlloc(NULL,size_cpr_buffer + 1,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
  memset(pszBuf, 0, size_cpr_buffer);

  FillOrigDataForFullMemoryOrg(porg,pold,size_old);
//  porg->givePtr = NULL;
//  porg->freePtr = NULL;

  wstr.nbOrig = 1;
  wstr.OrigDataPtr = porg;

  {
      COMPRESSIONPARAM cpr;
      InitDefaultCompressionParam(&cpr);
      cpr.uZlibCompressRatio = 9;
      cpr.dfBlockCalcSizeSearch = CompressRatio;
      cpr.dfPhysicalMemoryKB = GetPhysicalMemoryKb()/4;
      /*
      if (size_old > 1024*1024*16)
          cpr.dfBlockCalcSizeSearch = 0x80;
*/
      err = MakeDifInit(&wstr, &cpr);      // 5 = compress ratio
  }
  if (err != DSERR_OK)
    printf("1:err = %ld\n", err);

#ifdef DO_STATIS
  EnableStatis(&wstr, 512);
#endif

  wstr.nbOrig = 1;
  wstr.OrigDataPtr = porg;
  wstr.pPerformOrigDataAnalysisCB = NULL;
  wstr.dfUserPtr = NULL;

  wstr.OrigDataPtr = porg;

  data_in = (dfbytep) pnew;
  wstr.in_data_stream.next_in = data_in;
  wstr.in_data_stream.avail_in = size_new;


  wstr.out_data_stream.next_out = (dfvoidp *) pszBuf;
  wstr.out_data_stream.avail_out = size_cpr_buffer;

  size_new_buffer = size_new + 0x100;


  {
#define STEP (0x8000*4)
    dfbyte szBuf[STEP];
    dfuLong32 already_done = 0;
    dfuLong32 dwTotal = size_new;
    dfuLong32 dfLatestShow = 0;
    dfuLong32 dfNow;
    dfuLong32 dfLatestPercent=0xffffffff;

    printf("\n00 %%");
    wstr.in_data_stream.avail_in=0;

    while ((dwTotal > 0) || (wstr.in_data_stream.avail_in>0))
    {
      dfuLong32 i;

      if ((dwTotal > 0) && (wstr.in_data_stream.avail_in==0))
      {
          wstr.in_data_stream.next_in = szBuf;
          wstr.in_data_stream.avail_in = STEP;
          if (dwTotal < STEP)
            wstr.in_data_stream.avail_in = dwTotal;

          #ifdef BOUND_CHECKER
          {
            dfbyte v;
            v = *(data_in + 1 + 4740);
            for (i = 0; i < wstr.in_data_stream.avail_in; i++)
              szBuf[i] = 1 + v;
            for (i = 0; i < wstr.in_data_stream.avail_in; i++)
              szBuf[i] = 2 + v;
          }

          #endif

          for (i = 0; i < wstr.in_data_stream.avail_in; i++)
            szBuf[i] = *(data_in + i + already_done);
          already_done += wstr.in_data_stream.avail_in;
          dwTotal -= wstr.in_data_stream.avail_in;
      }

      wstr.out_data_stream.avail_out = 0x8000;
      if (wstr.out_data_stream.total_out + wstr.out_data_stream.avail_out > size_cpr_buffer)
         wstr.out_data_stream.avail_out = size_cpr_buffer - ((dfuLong32)wstr.out_data_stream.total_out);

      memset(wstr.out_data_stream.next_out,0,wstr.out_data_stream.avail_out);
      err = DoMakeDifWork(&wstr);
      memset(wstr.out_data_stream.next_out,0,wstr.out_data_stream.avail_out);
/*
      #ifdef _DEBUG
      {
          dfuLong32 crcin =  crc32(0,((dfbytep)wstr.in_data_stream.next_in)-wstr.done_latest_in,wstr.done_latest_in);
          dfuLong32 crcout = crc32(0,((dfbytep)wstr.out_data_stream.next_out)-wstr.done_latest_out,wstr.done_latest_out);
          printf("crc in,out=%lx,%lx for length %u,%u\n",crcin,crcout,wstr.done_latest_in,wstr.done_latest_out);
      }
      #endif
*/
      dfNow = GetTickCount();
      if (dfLatestShow + 300 < dfNow)
      {
        dfuLong32 dfPercent;
        if (size_new > 0x100000)
            dfPercent = already_done / (size_new /100);
        else
            dfPercent = (already_done * 100) / size_new;
        if (dfLatestPercent != dfPercent)
            printf("\x0d%02u %%", dfPercent);
        dfLatestPercent = dfPercent;
        dfLatestShow = dfNow;
      }
      if (err != DSERR_OK)
        printf("2:err = %ld\n", err);
    }
    printf("\x0d    \x0d");

  }

  /*
     err = DoMakeDifWork(&wstr);
     if (err != DSERR_OK)
     printf("2:err = %ld\n",err);
   */
  err = DSERR_OK;
  while (err == DSERR_OK)
  {
      wstr.out_data_stream.avail_out = 0x8000;
      if (wstr.out_data_stream.total_out + wstr.out_data_stream.avail_out > size_cpr_buffer)
         wstr.out_data_stream.avail_out = size_cpr_buffer - (dfuLong32)wstr.out_data_stream.total_out;

    err = FlushMakeDif(&wstr);
/*
      #ifdef _DEBUG
      {
          dfuLong32 crcin =  crc32(0,((dfbytep)wstr.in_data_stream.next_in)-wstr.done_latest_in,wstr.done_latest_in);
          dfuLong32 crcout = crc32(0,((dfbytep)wstr.out_data_stream.next_out)-wstr.done_latest_out,wstr.done_latest_out);
          printf("Flush:crc in,out=%lx,%lx for length %u,%u\n",crcin,crcout,wstr.done_latest_in,wstr.done_latest_out);
      }
      #endif
*/
  }
  if (err != DSERR_END)
    printf("3:err = %ld\n", err);
/***************************************************************/
#ifdef DO_STATIS
  {
    char szFn[512];
    char szTitle[1024];
    dfuLong32 i;
    for (i = 0;; i++)
    {
      sprintf(szFn, (szFnFormat == NULL) ? "stat_str%d.txt" : szFnFormat, i);
      sprintf(szTitle,
              (szTitleFormat == NULL) ? "Statis stream %d" : szTitleFormat,
              i);
      if (FlushStatis(&wstr, i, szFn, szTitle) == 0)
        break;
    }
    FlushHuff(&wstr);
  }
#endif

  err = CloseMakeDif(&wstr, &ul,&fFileIdentical);
  if (err != DSERR_OK)
    printf("4:err = %ld\n", err);
  printf("identical = %u\n",fFileIdentical);


  QueryPerformanceCounter((LARGE_INTEGER *) & endTime64);
  ticks = (endTime64 - beginTime64);
  QueryPerformanceFrequency((LARGE_INTEGER *) & ticksPerSecond);

/*
  printf("difstrm test adler=%lx, size=%lu, time = %lu sec (%f msec)\n", ul,
         wstr.out_data_stream.total_out, (GetTickCount() - dwBegin) / 1000,
         ((long double) ((DWORD) (ticks / 1024))) /
         (long double) (DWORD) (ticksPerSecond / 1024));
*/
  {
      dfuLong32 dfRestOut;
      RAMDIFBLD_FROM_DATA_STREAM RamDifBldFromDataStream;

      printf("convert to RamDif\n");

      printf("\n00 %%");

      RamDifBldFromDataStream.in_data_stream.next_in = (dfvoidp *) pszBuf;
      RamDifBldFromDataStream.in_data_stream.total_in=0;
      if (RamDifBldFromDataStreamInit(&RamDifBldFromDataStream,wstr.out_data_stream.total_out)==DSERR_OK)
      {
          int err=DSERR_OK;
          dfuLong32 dfLatestPercent = 0xffffffff;
          dfuLong32 already_done=0;
          dfuLong32 size_to_do = (dfuLong32)wstr.out_data_stream.total_out;
          dfRestOut = (dfuLong32)wstr.out_data_stream.total_out;

          while ((dfRestOut > 0) && (err==DSERR_OK))
          {
              dfuLong32 dfThis = min(dfRestOut, (0x80));
              RamDifBldFromDataStream.in_data_stream.avail_in = dfThis;

              err = DoPatchBldFromDataStream(&RamDifBldFromDataStream);
              if ((err != DSERR_END) && (err != DSERR_OK))
                printf("12:err = %ld\n", err);
              dfRestOut -= dfThis;
              already_done += dfThis;


              {
                dfuLong32 dfPercent;
                if (size_to_do > 0x100000)
                    dfPercent = already_done / (size_to_do /100);
                else if (size_to_do>0)
                    dfPercent = (already_done * 100) / size_to_do;
                if (dfLatestPercent != dfPercent)
                    printf("\x0d%02u %%", dfPercent);
              }
          }
          printf("\x0d    \x0d");
          err = CloseRamDifBldFromDataStream (&RamDifBldFromDataStream,&hRamDif,NULL);
          if ((err != DSERR_END) && (err != DSERR_OK))
                printf("12:err = %ld\n", err);
      }
      if (wstr.out_data_stream.total_out != RamDifBldFromDataStream.in_data_stream.total_in)
          printf("size error\n");
  }

  {
  DWORD dwNbTick = GetTickCount() - dwBegin;

  printf("difstrm test adler=%x, size=%u, time = %u sec (%u msec)\n",
         (DWORD)(ul), (DWORD)wstr.out_data_stream.total_out,
         (DWORD)(dwNbTick /1000),(DWORD)dwNbTick/*, dwBegin, GetTickCount() */);
  /*
  printf("time = %u sec (%u msec) (%u,%u)\n",
         (DWORD)(dwNbTick /1000),(DWORD)dwNbTick, dwBegin, GetTickCount() );*/
//printf("\ndwBegin6: %u - %u - %u - %u\n",dwBegin,dwNbTick,dwBegin,GetTickCount());
  }

    /************************************************/
  printf("\n\n");


  if (hRamDif!=NULL)
  {
    APPLYRAMDIF_STREAM rstr;
    dfuLong32 ulcr = 0;
    dfuLong32 dfRestOut;
    dfuLong32 dfLatestPercent=0xffffffff;
    dfbytep pszBufOut = (dfbytep) malloc(size_new_buffer + 1);

    memset(&rstr, 0, sizeof(APPLYRAMDIF_STREAM));
    err = ApplyRamDifInit(hRamDif,&rstr,TRUE);
    if (err != DSERR_OK)
      printf("11:err = %ld\n", err);


    rstr.out_data_stream.total_out = 0;
    rstr.out_data_stream.next_out = pszBufOut;
    //rstr.out_data_stream.avail_out = size_new_buffer;
    rstr.OrigDataPtr = porg;

    printf("\n00 %%");
    dfRestOut = (dfuLong32)wstr.in_data_stream.total_in;
    while (dfRestOut > 0)
    {
      dfuLong32 dfThis = min(dfRestOut, (0x20000));
      rstr.out_data_stream.avail_out = dfThis;
      rstr.out_data_stream.next_out = pszBufOut + rstr.out_data_stream.total_out;
      err = DoApplyRamDifWork(&rstr);
      if ((err != DSERR_END) && (err != DSERR_OK))
        printf("12:err = %ld\n", err);
      dfRestOut -= dfThis;

             {
                dfuLong32 dfPercent;
                if (wstr.in_data_stream.total_in > 0x100000)
                    dfPercent = (dfuLong32)(rstr.out_data_stream.total_out / (wstr.in_data_stream.total_in /100));
                else if (wstr.in_data_stream.total_in>0)
                    dfPercent = (dfuLong32)((rstr.out_data_stream.total_out * 100) / wstr.in_data_stream.total_in);
                if (dfLatestPercent != dfPercent)
                    printf("\x0d%02u %%", dfPercent);
              }
    }
    printf("\x0d    \x0d");

    if ((err != DSERR_END))
      printf("12b:err = %ld\n", err);

    err = CloseRamApplyDif(&rstr, &ulcr);
    if (err != DSERR_OK)
      printf("13:err = %ld\n", err);
/*
    if (wstr.out_data_stream.total_out != rstr.in_data_stream.total_in)
      printf("\nERROR NUMBER READ IN APPLY  out=%u, in=%u\n\n",
             wstr.out_data_stream.total_out, rstr.in_data_stream.total_in);
*/
    {
      int icmp = memcmp(pszBufOut, data_in, (dfuIntPtr)wstr.in_data_stream.total_in);
      printf("size in,out = %lu,%lu, size cpr,old = %lu,%lu\n",
             (unsigned long)wstr.in_data_stream.total_in, (unsigned long)rstr.out_data_stream.total_out, (unsigned long)wstr.out_data_stream.total_out, (unsigned long)size_old);
      printf("res cmp=%ld, adler=%lx\n", icmp, ulcr);
      fRet = ((icmp == 0) && (wstr.in_data_stream.total_in == rstr.out_data_stream.total_out));
      if (fRet)
        printf("ok, patch is GOOD, the patch size is %u (%u KB)\n",
                       (dfuLong32)wstr.out_data_stream.total_out,
                       (dfuLong32)(wstr.out_data_stream.total_out/1024));
      else
        printf("none, patch is BAD, icmp=%ld, total_in=%lu, total_out=%lu\n",
               icmp, (signed long)wstr.in_data_stream.total_in, (unsigned long)rstr.out_data_stream.total_out);
    }

    free(pszBufOut);
  }

  //free(pszBuf);
  VirtualFree(pszBuf,0,MEM_RELEASE | MEM_DECOMMIT);
  return fRet;
}

BOOL TestPatchFileRamDif(LPCSTR lpFileOld, LPCSTR lpFileNew, dfuLong32 CompressRatio)
{
  DWORD dwSizeFileOld, dwSizeFileNew;
  DWORD dwSizeFileOldHigh, dwSizeFileNewHigh;
  dfbytep pOld, pNew;
  //HANDLE hf;
  BOOL fRet;
  HFILECONTENTREADBUF hfcrOld,hfcrNew;
  ORIGDATA OrigOld,OrigNew;
  dfwchar szwFileOld[MAX_PATH+0x10];
  dfwchar szwFileNew[MAX_PATH+0x10];

  hfcrOld=hfcrNew=NULL;

  fRet = MyGetFileSize(lpFileOld, &dwSizeFileOld,&dwSizeFileOldHigh) &&
    MyGetFileSize(lpFileNew, &dwSizeFileNew,&dwSizeFileNewHigh);

  ConvertAnsiToUnicode(lpFileOld,szwFileOld,MAX_PATH);
  ConvertAnsiToUnicode(lpFileNew,szwFileNew,MAX_PATH);
  fRet = GetFileFullContentBuffer(szwFileOld,FILEFULLCONTENTBUFFER_READ,0,&hfcrOld,&OrigOld,NULL) &&
         GetFileFullContentBuffer(szwFileNew,FILEFULLCONTENTBUFFER_READ,0,&hfcrNew,&OrigNew,NULL) ;
  pOld = GetOrigDataPtrpData(&OrigOld);
  pNew = GetOrigDataPtrpData(&OrigNew);

  if (fRet)
  {
    char szFnFormat[512];
    char szTitleFormat[512];

    sprintf(szFnFormat, "%s_%%d.txt", lpFileNew);
    sprintf(szTitleFormat, "Stream %%d of patch %s to %s", lpFileOld,
            lpFileNew);



    fRet =
      TestPatchRamDif(pOld, dwSizeFileOld, pNew, dwSizeFileNew, szFnFormat,
                szTitleFormat, CompressRatio);
  }

  CloseFileFullContentBuffer(hfcrOld);
  CloseFileFullContentBuffer(hfcrNew);

  return fRet;
}



BOOL
TestPatchRamDif3 (dfvoidp pfile1, dfuLong32 size1, dfvoidp pfile2,
                  dfuLong32 size2, dfvoidp pfile3, dfuLong32 size3,
                  const char *szFnFormat, const char *szTitleFormat,
                  dfuLong32 CompressRatio)
{
  BOOL fRet = FALSE;
  DWORD dwBegin;
  HRAMDIF hRamDif12 = NULL;
  HRAMDIF hRamDif123 = NULL;
  int err;

  unsigned __int64 beginTime64, endTime64, ticksPerSecond, ticks;

  dfuLong32 ul = 0;
  dfbytep data_in;

  ORIGDATA org1;
  ORIGDATAPTR porg1 = &org1;
  WRITEDIF_STREAM wstr12;
  WRITEDIF_STREAM *write_stream12 = &wstr12;
  dfbytep pszBuf12;
  dfuLong32 size_new_buffer12;
  dfuLong32 size_cpr_buffer12;
  BOOL fFileIdentical12 = FALSE;

  ORIGDATA org2;
  ORIGDATAPTR porg2 = &org2;
  WRITEDIF_STREAM wstr23;
  WRITEDIF_STREAM *write_stream23 = &wstr23;
  dfbytep pszBuf23;
  dfuLong32 size_new_buffer23;
  dfuLong32 size_cpr_buffer23;

  dfbytep pszBuf13;
  dfuLong32 size_cpr_buffer13;
  BOOL fFileIdentical23 = FALSE;
  dfuLong32 size_new_buffer13;
  dfuLong32 size_stream_dif_13;

  /*******************************/
  err = DSERR_OK;

  memset (&wstr12, 0, sizeof (WRITEDIF_STREAM));
  printf ("TestPatchRamDif: size org=%u,new=%u, compres param=%d\n", size1,
          size2, CompressRatio);

  QueryPerformanceCounter ((LARGE_INTEGER *) & beginTime64);

  dwBegin = GetTickCount ();

  size_cpr_buffer12 = size2 + 0x1000;

  pszBuf12 =
    (dfbytep) VirtualAlloc (NULL, size_cpr_buffer12 + 1,
                            MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  memset (pszBuf12, 0, size_cpr_buffer12);

  FillOrigDataForFullMemoryOrg (porg1, pfile1, size1);

  wstr12.nbOrig = 1;
  wstr12.OrigDataPtr = porg1;

  {
    COMPRESSIONPARAM cpr;
    InitDefaultCompressionParam (&cpr);
    cpr.uZlibCompressRatio = 9;
    cpr.dfBlockCalcSizeSearch = CompressRatio;
    cpr.dfPhysicalMemoryKB = GetPhysicalMemoryKb () / 4;

    err = MakeDifInit (&wstr12, &cpr);  // 5 = compress ratio
  }
  if (err != DSERR_OK)
    printf ("1:err = %ld\n", err);

  wstr12.nbOrig = 1;
  wstr12.OrigDataPtr = porg1;
  wstr12.pPerformOrigDataAnalysisCB = NULL;
  wstr12.dfUserPtr = NULL;

  wstr12.OrigDataPtr = porg1;

  data_in = (dfbytep) pfile2;
  wstr12.in_data_stream.next_in = data_in;
  wstr12.in_data_stream.avail_in = size2;


  wstr12.out_data_stream.next_out = (dfvoidp *) pszBuf12;
  wstr12.out_data_stream.avail_out = size_cpr_buffer12;

  size_new_buffer12 = size2 + 0x100;


  {
#define STEP (0x8000*4)
    dfbyte szBuf[STEP];
    dfuLong32 already_done = 0;
    dfuLong32 dwTotal = size2;
    dfuLong32 dfLatestShow = 0;
    dfuLong32 dfNow;
    dfuLong32 dfLatestPercent = 0xffffffff;

    printf ("\n00 %%");
    wstr12.in_data_stream.avail_in = 0;

    while ((dwTotal > 0) || (wstr12.in_data_stream.avail_in > 0))
      {
        dfuLong32 i;

        if ((dwTotal > 0) && (wstr12.in_data_stream.avail_in == 0))
          {
            wstr12.in_data_stream.next_in = szBuf;
            wstr12.in_data_stream.avail_in = STEP;
            if (dwTotal < STEP)
              wstr12.in_data_stream.avail_in = dwTotal;

            for (i = 0; i < wstr12.in_data_stream.avail_in; i++)
              szBuf[i] = *(data_in + i + already_done);

            already_done += wstr12.in_data_stream.avail_in;
            dwTotal -= wstr12.in_data_stream.avail_in;
          }

        wstr12.out_data_stream.avail_out = 0x8000;
        if (wstr12.out_data_stream.total_out +
            wstr12.out_data_stream.avail_out > size_cpr_buffer12)
          wstr12.out_data_stream.avail_out =
            size_cpr_buffer12 -
            ((dfuLong32) wstr12.out_data_stream.total_out);

        memset (wstr12.out_data_stream.next_out, 0,
                wstr12.out_data_stream.avail_out);
        err = DoMakeDifWork (&wstr12);
        memset (wstr12.out_data_stream.next_out, 0,
                wstr12.out_data_stream.avail_out);

        dfNow = GetTickCount ();
        if (dfLatestShow + 300 < dfNow)
          {
            dfuLong32 dfPercent;
            if (size2 > 0x100000)
              dfPercent = already_done / (size2 / 100);
            else
              dfPercent = (already_done * 100) / size2;
            if (dfLatestPercent != dfPercent)
              printf ("\x0d%02u %%", dfPercent);
            dfLatestPercent = dfPercent;
            dfLatestShow = dfNow;
          }
        if (err != DSERR_OK)
          printf ("2:err = %ld\n", err);
      }
    printf ("\x0d    \x0d");

  }

  err = DSERR_OK;
  while (err == DSERR_OK)
    {
      wstr12.out_data_stream.avail_out = 0x8000;
      if (wstr12.out_data_stream.total_out +
          wstr12.out_data_stream.avail_out > size_cpr_buffer12)
        wstr12.out_data_stream.avail_out =
          size_cpr_buffer12 - (dfuLong32) wstr12.out_data_stream.total_out;

      err = FlushMakeDif (&wstr12);
    }
  if (err != DSERR_END)
    printf ("3:err = %ld\n", err);
/***************************************************************/
  err = CloseMakeDif (&wstr12, &ul, &fFileIdentical12);
  if (err != DSERR_OK)
    printf ("4:err = %ld\n", err);
  printf ("Patch12 : identical = %ld, size of patch=%u\n", (signed long)fFileIdentical12,
          (unsigned long)wstr12.out_data_stream.total_out);

/****************************************************************/
/****************************************************************/
  printf ("time for MakeDisk12 : %u msec (%f sec)\n",
          GetTickCount () - dwBegin, (GetTickCount () - dwBegin) / 1000.0);

  memset (&wstr23, 0, sizeof (WRITEDIF_STREAM));
  printf ("TestPatchRamDif: size org=%u,new=%u, compres param=%d\n\n", size2,
          size3, CompressRatio);

  QueryPerformanceCounter ((LARGE_INTEGER *) & beginTime64);

  dwBegin = GetTickCount ();

  size_cpr_buffer23 = size3 + 0x1000;

  pszBuf23 =
    (dfbytep) VirtualAlloc (NULL, size_cpr_buffer23 + 1,
                            MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  memset (pszBuf23, 0, size_cpr_buffer23);

  FillOrigDataForFullMemoryOrg (porg2, pfile2, size2);

  wstr23.nbOrig = 1;
  wstr23.OrigDataPtr = porg2;

  {
    COMPRESSIONPARAM cpr;
    InitDefaultCompressionParam (&cpr);
    cpr.uZlibCompressRatio = 9;
    cpr.dfBlockCalcSizeSearch = CompressRatio;
    cpr.dfPhysicalMemoryKB = GetPhysicalMemoryKb () / 4;

    err = MakeDifInit (&wstr23, &cpr);  // 5 = compress ratio
  }
  if (err != DSERR_OK)
    printf ("1:err = %ld\n", err);

  wstr23.nbOrig = 1;
  wstr23.OrigDataPtr = porg2;
  wstr23.pPerformOrigDataAnalysisCB = NULL;
  wstr23.dfUserPtr = NULL;

  wstr23.OrigDataPtr = porg2;

  data_in = (dfbytep) pfile3;
  wstr23.in_data_stream.next_in = data_in;
  wstr23.in_data_stream.avail_in = size3;


  wstr23.out_data_stream.next_out = (dfvoidp *) pszBuf23;
  wstr23.out_data_stream.avail_out = size_cpr_buffer23;

  size_new_buffer23 = size3 + 0x100;


  {
#define STEP (0x8000*4)
    dfbyte szBuf[STEP];
    dfuLong32 already_done = 0;
    dfuLong32 dwTotal = size3;
    dfuLong32 dfLatestShow = 0;
    dfuLong32 dfNow;
    dfuLong32 dfLatestPercent = 0xffffffff;

    printf ("\n00 %%");
    wstr23.in_data_stream.avail_in = 0;

    while ((dwTotal > 0) || (wstr23.in_data_stream.avail_in > 0))
      {
        dfuLong32 i;

        if ((dwTotal > 0) && (wstr23.in_data_stream.avail_in == 0))
          {
            wstr23.in_data_stream.next_in = szBuf;
            wstr23.in_data_stream.avail_in = STEP;
            if (dwTotal < STEP)
              wstr23.in_data_stream.avail_in = dwTotal;

            for (i = 0; i < wstr23.in_data_stream.avail_in; i++)
              szBuf[i] = *(data_in + i + already_done);
            already_done += wstr23.in_data_stream.avail_in;
            dwTotal -= wstr23.in_data_stream.avail_in;
          }

        wstr23.out_data_stream.avail_out = 0x8000;
        if (wstr23.out_data_stream.total_out +
            wstr23.out_data_stream.avail_out > size_cpr_buffer23)
          wstr23.out_data_stream.avail_out =
            size_cpr_buffer23 -
            ((dfuLong32) wstr23.out_data_stream.total_out);

        memset (wstr23.out_data_stream.next_out, 0,
                wstr23.out_data_stream.avail_out);
        err = DoMakeDifWork (&wstr23);
        memset (wstr23.out_data_stream.next_out, 0,
                wstr23.out_data_stream.avail_out);

        dfNow = GetTickCount ();
        if (dfLatestShow + 300 < dfNow)
          {
            dfuLong32 dfPercent;
            if (size3 > 0x100000)
              dfPercent = already_done / (size3 / 100);
            else
              dfPercent = (already_done * 100) / size3;
            if (dfLatestPercent != dfPercent)
              printf ("\x0d%02u %%", dfPercent);
            dfLatestPercent = dfPercent;
            dfLatestShow = dfNow;
          }
        if (err != DSERR_OK)
          printf ("2:err = %ld\n", err);
      }
    printf ("\x0d    \x0d");

  }

  err = DSERR_OK;
  while (err == DSERR_OK)
    {
      wstr23.out_data_stream.avail_out = 0x8000;
      if (wstr23.out_data_stream.total_out +
          wstr23.out_data_stream.avail_out > size_cpr_buffer23)
        wstr23.out_data_stream.avail_out =
          size_cpr_buffer23 - (dfuLong32) wstr23.out_data_stream.total_out;

      err = FlushMakeDif (&wstr23);
    }
  if (err != DSERR_END)
    printf ("3:err = %ld\n", err);
/***************************************************************/
  err = CloseMakeDif (&wstr23, &ul, &fFileIdentical23);
  if (err != DSERR_OK)
    printf ("4:err = %ld\n", err);

  printf ("Patch23 : identical = %ld,size of patch=%lu\n", (signed long)fFileIdentical23,
          (unsigned long)wstr23.out_data_stream.total_out);
  printf ("time for MakeDisk23 : %u msec (%f sec)\n",
          GetTickCount () - dwBegin, (GetTickCount () - dwBegin) / 1000.0);


/****************************************************************/
/****************************************************************/

  QueryPerformanceCounter ((LARGE_INTEGER *) & endTime64);
  ticks = (endTime64 - beginTime64);
  QueryPerformanceFrequency ((LARGE_INTEGER *) & ticksPerSecond);
  dwBegin = GetTickCount ();

  {
    dfuLong32 dfRestOut;
    RAMDIFBLD_FROM_DATA_STREAM RamDifBldFromDataStream;

    printf ("convert to RamDif 12\n");

    printf ("\n00 %%");

    RamDifBldFromDataStream.in_data_stream.next_in = (dfvoidp *) pszBuf12;
    RamDifBldFromDataStream.in_data_stream.total_in = 0;
    if (RamDifBldFromDataStreamInit
        (&RamDifBldFromDataStream,
         wstr12.out_data_stream.total_out) == DSERR_OK)
      {
        int err = DSERR_OK;
        dfuLong32 dfLatestPercent = 0xffffffff;
        dfuLong32 already_done = 0;
        dfuLong32 size_to_do = (dfuLong32) wstr12.out_data_stream.total_out;
        dfRestOut = (dfuLong32) wstr12.out_data_stream.total_out;

        while ((dfRestOut > 0) && (err == DSERR_OK))
          {
            dfuLong32 dfThis = min (dfRestOut, (0x80));
            RamDifBldFromDataStream.in_data_stream.avail_in = dfThis;

            err = DoPatchBldFromDataStream (&RamDifBldFromDataStream);
            if ((err != DSERR_END) && (err != DSERR_OK))
              printf ("12:err = %ld\n", err);
            dfRestOut -= dfThis;
            already_done += dfThis;


            {
              dfuLong32 dfPercent;
              if (size_to_do > 0x100000)
                dfPercent = already_done / (size_to_do / 100);
              else if (size_to_do > 0)
                dfPercent = (already_done * 100) / size_to_do;
              if (dfLatestPercent != dfPercent)
                printf ("\x0d%02u %%", dfPercent);
              dfLatestPercent = dfPercent;
            }
          }
        printf ("\x0d    \x0d");
        err =
          CloseRamDifBldFromDataStream (&RamDifBldFromDataStream, &hRamDif12,NULL);
        if ((err != DSERR_END) && (err != DSERR_OK))
          printf ("12:err = %ld\n", err);
      }
    if (wstr12.out_data_stream.total_out !=
        RamDifBldFromDataStream.in_data_stream.total_in)
      printf ("size error\n");
  }
  printf ("time for Convert12 : %u msec (%f sec)\n",
          GetTickCount () - dwBegin, (GetTickCount () - dwBegin) / 1000.0);
  VirtualFree (pszBuf12, 0, MEM_RELEASE | MEM_DECOMMIT);


  dwBegin = GetTickCount ();
  {
    dfuLong32 dfRestOut;
    RAMDIFBLD_FROM_DATA_STREAM RamDifBldFromDataStream;

    printf ("convert 23 to RamDif 123\n");

    printf ("\n00 %%");

    RamDifBldFromDataStream.in_data_stream.next_in = (dfvoidp *) pszBuf23;
    RamDifBldFromDataStream.in_data_stream.total_in = 0;
    if (RamDifBldFromDataStreamAndRamDifInit
        (&RamDifBldFromDataStream, wstr23.out_data_stream.total_out,NULL,
         hRamDif12) == DSERR_OK)
      {
        int err = DSERR_OK;
        dfuLong32 dfLatestPercent = 0xffffffff;
        dfuLong32 already_done = 0;
        dfuLong32 size_to_do = (dfuLong32) wstr23.out_data_stream.total_out;
        dfRestOut = (dfuLong32) wstr23.out_data_stream.total_out;

        while ((dfRestOut > 0) && (err == DSERR_OK))
          {
            dfuLong32 dfThis = min (dfRestOut, (0x80));
            RamDifBldFromDataStream.in_data_stream.avail_in = dfThis;

            err = DoPatchBldFromDataStream (&RamDifBldFromDataStream);
            if ((err != DSERR_END) && (err != DSERR_OK))
              printf ("12:err = %ld\n", err);
            dfRestOut -= dfThis;
            already_done += dfThis;


            {
              dfuLong32 dfPercent;
              if (size_to_do > 0x100000)
                dfPercent = already_done / (size_to_do / 100);
              else if (size_to_do > 0)
                dfPercent = (already_done * 100) / size_to_do;
              if (dfLatestPercent != dfPercent)
                printf ("\x0d%02u %%", dfPercent);
              dfLatestPercent = dfPercent;
            }
          }
        printf ("\x0d    \x0d");
        err =
          CloseRamDifBldFromDataStream (&RamDifBldFromDataStream,
                                        &hRamDif123,NULL);
        if ((err != DSERR_END) && (err != DSERR_OK))
          printf ("12:err = %ld\n", err);
      }
    if (wstr23.out_data_stream.total_out !=
        RamDifBldFromDataStream.in_data_stream.total_in)
      printf ("size error\n");
  }

  printf ("time for Convert23 : %u msec (%f sec)\n",
          GetTickCount () - dwBegin, (GetTickCount () - dwBegin) / 1000.0);

    /************************************************/
  printf ("\n\n");

  dwBegin = GetTickCount ();
///// need add ramdif12 + patch23 -> ramdif 23
  if (hRamDif123 != NULL)
    {
      APPLYRAMDIF_STREAM rstr;
      dfuLong32 ulcr = 0;
      dfuLong32 dfRestOut;
      dfuLong32 dfLatestPercent = 0xffffffff;
      dfbytep pszBufOut = (dfbytep) malloc (size3 + 1);

      memset (&rstr, 0, sizeof (APPLYRAMDIF_STREAM));
      err = ApplyRamDifInit (hRamDif123, &rstr,TRUE);
      if (err != DSERR_OK)
        printf ("11:err = %ld\n", err);


      rstr.out_data_stream.total_out = 0;
      rstr.out_data_stream.next_out = pszBufOut;
      rstr.OrigDataPtr = porg1;

      printf ("\n00 %%");
      dfRestOut = (dfuLong32) size3;
      while (dfRestOut > 0)
        {
          dfuLong32 dfThis = min (dfRestOut, (0x20000));
          rstr.out_data_stream.avail_out = dfThis;
          rstr.out_data_stream.next_out =
            pszBufOut + rstr.out_data_stream.total_out;
          err = DoApplyRamDifWork (&rstr);
          if ((err != DSERR_END) && (err != DSERR_OK))
            printf ("12:err = %ld\n", err);
          dfRestOut -= dfThis;

          {
            dfuLong32 dfPercent;
            if (wstr12.in_data_stream.total_in > 0x100000)
              dfPercent =
                (dfuLong32) (rstr.out_data_stream.total_out /
                             (wstr12.in_data_stream.total_in / 100));
            else if (wstr12.in_data_stream.total_in > 0)
              dfPercent =
                (dfuLong32) ((rstr.out_data_stream.total_out * 100) /
                             wstr12.in_data_stream.total_in);
            if (dfLatestPercent != dfPercent)
              printf ("\x0d%02u %%", dfPercent);
          }
        }
      printf ("\x0d    \x0d");

      if ((err != DSERR_END))
        printf ("12b:err = %ld\n", err);

      err = CloseRamApplyDif (&rstr, &ulcr);
      if (err != DSERR_OK)
        printf ("13:err = %ld\n", err);
      {
        int icmp = memcmp (pszBufOut, pfile3,
                           (dfuIntPtr) rstr.out_data_stream.total_out);
        if (icmp != 0)
          {
            dfuIntPtr ii;
            dfuLong32 dfCountErr = 0;
            printf ("\n\n->Dif pos  list\n");
            for (ii = 0; ii < rstr.out_data_stream.total_out; ii++)
              if ((*(pszBufOut + ii)) != (*(((dfbytep) pfile3) + ii)))
                {
                  printf ("%07lu ", (unsigned long)ii);
                  if ((dfCountErr++) > 100)
                    break;
                }
          }
        printf ("size out=%lu\n", (unsigned long)rstr.out_data_stream.total_out);
        printf ("res cmp=%ld, adler=%lx\n", (signed long)icmp, (unsigned long)ulcr);
        fRet = ((icmp == 0) && (size3 == rstr.out_data_stream.total_out));
        if (fRet)
          printf ("ok, patch is GOOD\n");
        else
          printf ("none, patch is BAD, icmp=%ld, filesize3=%lu, total_out=%lu\n",
                  (signed long)icmp, (unsigned long)size3, (unsigned long)rstr.out_data_stream.total_out);
      }

      free (pszBufOut);
    }

  DeleteRamDif (hRamDif123);

  printf ("time for Apply RamDif 123: %u msec (%f sec)\n",
          GetTickCount () - dwBegin, (GetTickCount () - dwBegin) / 1000.0);





  /****************************************************************/

  {

#define STEPPSB (0x8000*1)
    PATCHSTREAMBLD_FROM_DATA_STREAM psbfds;
    dfbyte szBuf[STEPPSB];
    dfuLong32 already_done = 0;
    dfuLong32 dwTotal;
    dfuLong32 dfLatestShow = 0;
    dfuLong32 dfNow;
    dfuLong32 dfLatestPercent = 0xffffffff;



    err = DSERR_OK;
    dwBegin = GetTickCount ();

    memset (&psbfds, 0, sizeof (PATCHSTREAMBLD_FROM_DATA_STREAM));
    printf ("BuildPatch13: compres param=%d\n", CompressRatio);

    QueryPerformanceCounter ((LARGE_INTEGER *) & beginTime64);



    size_cpr_buffer13 = size3 + 0x1000;

    pszBuf13 =
      (dfbytep) VirtualAlloc (NULL, size_cpr_buffer13 + 1,
                              MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    //memset(pszBuf13, 0, size_cpr_buffer13);
    dwBegin = GetTickCount ();


    err =
      PatchOutStreamBldFromDataStreamAndRamDifInit (&psbfds, size3, hRamDif12,
                                                    CompressRatio, DEFAULT_SIZE_BUF_STREAM_KB);

    if (err != DSERR_OK)
      printf ("1:err = %ld\n", err);


    data_in = (dfbytep) pszBuf23;
    psbfds.in_data_stream.next_in = data_in;
    psbfds.in_data_stream.avail_in =
      (dfuLong32) wstr23.out_data_stream.total_out;
    dwTotal = (dfuLong32) wstr23.out_data_stream.total_out;


    psbfds.out_data_stream.next_out = (dfvoidp *) pszBuf13;
    psbfds.out_data_stream.avail_out = size_cpr_buffer13;

    size_new_buffer13 = size3 + 0x1000;




    printf ("\n00 %%");
    psbfds.in_data_stream.avail_in = 0;

    while ((dwTotal > 0) || (psbfds.in_data_stream.avail_in > 0))
      {
        dfuLong32 i;

        if ((dwTotal > 0) && (psbfds.in_data_stream.avail_in == 0))
          {
            psbfds.in_data_stream.next_in = szBuf;
            psbfds.in_data_stream.avail_in = STEPPSB;
            if (dwTotal < STEPPSB)
              psbfds.in_data_stream.avail_in = dwTotal;

            for (i = 0; i < psbfds.in_data_stream.avail_in; i++)
              szBuf[i] = *(data_in + i + already_done);

            already_done += psbfds.in_data_stream.avail_in;
            dwTotal -= psbfds.in_data_stream.avail_in;
          }

        psbfds.out_data_stream.avail_out = 0x8000;
        if (psbfds.out_data_stream.total_out +
            psbfds.out_data_stream.avail_out > size_cpr_buffer13)
          psbfds.out_data_stream.avail_out =
            size_cpr_buffer13 -
            ((dfuLong32) psbfds.out_data_stream.total_out);

        memset (psbfds.out_data_stream.next_out, 0,
                psbfds.out_data_stream.avail_out);
        err = DoPatchOutStreamBldFromDataStream (&psbfds);
        memset (psbfds.out_data_stream.next_out, 0,
                psbfds.out_data_stream.avail_out);

        dfNow = GetTickCount ();
        if (dfLatestShow + 300 < dfNow)
          {
            dfuLong32 dfPercent;
            if (wstr23.out_data_stream.total_out > 0x100000)
              dfPercent =
                (dfuLong32) (already_done /
                             (wstr23.out_data_stream.total_out / 100));
            else
              dfPercent =
                (dfuLong32) ((already_done * 100) /
                             wstr23.out_data_stream.total_out);
            if (dfLatestPercent != dfPercent)
              printf ("\x0d%02u %%", dfPercent);
            dfLatestPercent = dfPercent;
            dfLatestShow = dfNow;
          }
        if (err != DSERR_OK)
          if (!((err == DSERR_END) && (dwTotal == 0)))
            printf ("2:err = %ld\n", err);
      }
    printf ("\x0d    \x0d");



    err = DSERR_OK;
    while (err == DSERR_OK)
      {
        psbfds.out_data_stream.avail_out = 0x8000;
        if (psbfds.out_data_stream.total_out +
            psbfds.out_data_stream.avail_out > size_cpr_buffer13)
          psbfds.out_data_stream.avail_out =
            size_cpr_buffer13 - (dfuLong32) psbfds.out_data_stream.total_out;

        err = FlushPatchOutStreamBldFromDataStreamAndRamDif (&psbfds);
      }

    if (err != DSERR_END)
      printf ("3:err = %ld\n", err);


    err = ClosePatchOutStreamBldFromDataStreamAndRamDif (&psbfds,NULL);
    if (err != DSERR_OK)
      printf ("4:err = %ld\n", err);
    printf ("size of stream patch 13 = %lu\n",
            (unsigned long)psbfds.out_data_stream.total_out);
    size_stream_dif_13 = (dfuLong32) psbfds.out_data_stream.total_out;
  }
  printf ("time build stream dif 13 : %lu msec (%f sec)\n",
          (unsigned long)(GetTickCount () - dwBegin), (GetTickCount () - dwBegin) / 1000.0);
/************************************************************/
  DeleteRamDif (hRamDif12);
  VirtualFree (pszBuf23, 0, MEM_RELEASE | MEM_DECOMMIT);





    /************************************************/
  printf ("\n\n");

  printf ("try apply stream dif 13\n");
  {
    APPLYDIF_STREAM rstr;
    dfuLong32 ulcr = 0;
    dfuLong32 dfRestOut;
    dfbytep pszBufOut = (dfbytep) malloc (size_new_buffer13 + 1);

    memset (&rstr, 0, sizeof (APPLYDIF_STREAM));
    err = ApplyDifInit (&rstr);
    if (err != DSERR_OK)
      printf ("11:err = %ld\n", err);


    rstr.out_data_stream.next_out = pszBufOut;
    rstr.out_data_stream.avail_out = size_new_buffer13;
    rstr.OrigDataPtr = porg1;

    rstr.in_data_stream.next_in = (dfvoidp *) pszBuf13;
    dfRestOut = size_stream_dif_13;
    while (dfRestOut > 0)
      {
        dfuLong32 dfThis = min (dfRestOut, (0x20000));
        rstr.in_data_stream.avail_in = dfThis;

        err = DoApplyDifWork (&rstr);
        if ((err != DSERR_END) && (err != DSERR_OK))
          printf ("12:err = %ld\n", err);
        dfRestOut -= dfThis;
      }

    if ((err != DSERR_END))
      printf ("12b:err = %ld\n", err);

    err = CloseApplyDif (&rstr, &ulcr);
    if (err != DSERR_OK)
      printf ("13:err = %ld\n", err);

    if (size_stream_dif_13 != rstr.in_data_stream.total_in)
      printf ("\nERROR NUMBER READ IN APPLY  out=%lu, in=%lu\n\n",
              (unsigned long)size_stream_dif_13, (unsigned long)rstr.in_data_stream.total_in);

    {
      int icmp = memcmp (pszBufOut, pfile3, (dfuIntPtr) size3);
      printf ("size in,out = %lu,%lu, size cpr,old = %lu,%lu\n",
              (unsigned long)size_stream_dif_13, (unsigned long)rstr.out_data_stream.total_out,
              (unsigned long)size_stream_dif_13, (unsigned long)size1);
      printf ("res cmp=%ld, adler=%lx\n", icmp, ulcr);
      fRet = ((icmp == 0) && (size3 == rstr.out_data_stream.total_out));

      if (fRet)
        printf ("ok, patch is GOOD, the patch size is %u (%u KB)\n",
                (dfuLong32) size_stream_dif_13,
                (dfuLong32) (size_stream_dif_13 / 1024));
      else
        printf ("none, patch is BAD, icmp=%lu, total_in=%lu, total_out=%lu\n",
                (unsigned long)icmp, (unsigned long)size3, (unsigned long)rstr.out_data_stream.total_out);
    }

    free (pszBufOut);
  }


  VirtualFree (pszBuf13, 0, MEM_RELEASE | MEM_DECOMMIT);
  return fRet;
}


BOOL TestPatchFileRamDif3(LPCSTR lpFile1, LPCSTR lpFile2,LPCSTR lpFile3, dfuLong32 CompressRatio)
{
  DWORD dwSizeFile1, dwSizeFile2, dwSizeFile3;
  DWORD dwSizeFile1High, dwSizeFile2High, dwSizeFile3High;
  dfbytep p1,p2,p3;
  //HANDLE hf;
  BOOL fRet;
  HFILECONTENTREADBUF hfcr1,hfcr2,hfcr3;
  ORIGDATA Orig1,Orig2,Orig3;
  dfwchar szwFile1[MAX_PATH+0x10];
  dfwchar szwFile2[MAX_PATH+0x10];
  dfwchar szwFile3[MAX_PATH+0x10];

  hfcr1=hfcr2=hfcr3=NULL;

  fRet = MyGetFileSize(lpFile1, &dwSizeFile1,&dwSizeFile1High) &&
         MyGetFileSize(lpFile2, &dwSizeFile2,&dwSizeFile2High) &&
         MyGetFileSize(lpFile3, &dwSizeFile3,&dwSizeFile3High);

  ConvertAnsiToUnicode(lpFile1,szwFile1,MAX_PATH);
  ConvertAnsiToUnicode(lpFile2,szwFile2,MAX_PATH);
  ConvertAnsiToUnicode(lpFile3,szwFile3,MAX_PATH);

  fRet = GetFileFullContentBuffer(szwFile1,FILEFULLCONTENTBUFFER_READ,0,&hfcr1,&Orig1,NULL) &&
         GetFileFullContentBuffer(szwFile2,FILEFULLCONTENTBUFFER_READ,0,&hfcr2,&Orig2,NULL) &&
         GetFileFullContentBuffer(szwFile3,FILEFULLCONTENTBUFFER_READ,0,&hfcr3,&Orig3,NULL) ;

  p1 = GetOrigDataPtrpData(&Orig1);
  p2 = GetOrigDataPtrpData(&Orig2);
  p3 = GetOrigDataPtrpData(&Orig3);

  if (fRet)
  {
    char szFnFormat[512];
    char szTitleFormat[512];

    sprintf(szFnFormat, "%s_%%d.txt", lpFile1);
    sprintf(szTitleFormat, "Stream %%d of patch %s to %s to %s", lpFile1,
            lpFile2,lpFile3);



    fRet =
      TestPatchRamDif3(p1,dwSizeFile1,p2,dwSizeFile2,p3,dwSizeFile3, szFnFormat,
                szTitleFormat, CompressRatio);
  }

  CloseFileFullContentBuffer(hfcr1);
  CloseFileFullContentBuffer(hfcr2);
  CloseFileFullContentBuffer(hfcr3);

  return fRet;
}






#include "time.h"
void dummy()
{
#ifdef _DEBUG
  time_t long_time;
  localtime(&long_time);
#endif
}

//#include "zlib.h"
#include "../lib/engine/svfile/common/DfsMFile.h"

void testdateapi()
{
  DFSINFODATE dfsInfoDate;
  DFSTM dfsTm, dfsTm2;
  int icmp;

  dfsTm.df_hour = 21;
  dfsTm.df_mday = 23;
  dfsTm.df_min = 53;
  dfsTm.df_mon = 11;
  dfsTm.df_msec = 681;
  dfsTm.df_sec = 50;
  dfsTm.df_year = 2350;
  dfsTm.df_timezone_bias = 0;
  ConvertDfsTmToDfsInfoDate(&dfsTm, &dfsInfoDate);
  ConvertDfsInfoDateToDfsTm(&dfsInfoDate, &dfsTm2);
  icmp = memcmp(&dfsTm, &dfsTm2, sizeof(dfsTm));
  printf("res=%u\n", icmp);
}

dfuLong64 myGetRand(dfuLong64 limitHigh)
{
	dfuLong64 val;
	double rand1 = (rand()*(double)1.0) / RAND_MAX;
	double rand2 = (rand()*(double)1.0) / (RAND_MAX*100.);

	val=(dfuLong64)(limitHigh * (rand1 + rand2));
	while (val >= limitHigh)
		val -= limitHigh;
	return val;
}

void TestFMIO(dfwcharpc FileName)
{
	ORIGDATA org;
	H_ERROR_INFO* pei = NULL;

	HFILECONTENTREADBUF hFileContentReadBuf = NULL;
	dfuLong32 adler32_ = 0;

	dfuLong64 size = GetFileSizeByName(FileName,NULL, pei);
	if (GetFileFullContentBuffer(FileName, FILEFULLCONTENTBUFFER_READ, 0,
		&hFileContentReadBuf,
		&org, pei))
	{
		dfuLong64 pos=0;
		dfuLong32 sizeRead = 1;
		for (int iLoop = 0; iLoop < 1000; iLoop++)
		{
			if ((myGetRand(10)>4) && ((pos + sizeRead < size)))
			{
				pos += sizeRead;
			}
			else
			{
				pos = myGetRand(size - 1);
			}

			sizeRead = 1 + (dfuLong32)myGetRand(16384 * 8);
			if ((pos + sizeRead) > size)
				sizeRead = (dfuLong32)(size - pos);

			const unsigned char* ptr = ((const unsigned char*)GetOrigDataPtrpDataBySizeView(&org, pos, sizeRead));
			const unsigned char* ptr2 = ((const unsigned char*)GetOrigDataPtrpDataBySizeView(&org, pos, sizeRead));
			if (ptr != ptr2)
			{
				printf("**** adl stupid\n ");
			}
			ptr += pos;
			adler32_ = adler32(adler32_, ptr, sizeRead);
		}
		CloseFileFullContentBuffer(hFileContentReadBuf);
	}
	printf("File %S size %I64u adler32_work=%08x\n", FileName, size, adler32_);
	return;
}

void TestFMIOWrite(dfwcharpc FileName, dfwcharpc FileNameCop)
{
	ORIGDATA org;
	H_ERROR_INFO* pei = NULL;

	HFILECONTENTREADBUF hFileContentReadBuf = NULL;
	dfuLong32 adler32_ = 0;


	MyCopyFile(FileName, FileNameCop, FALSE, TRUE, NULL);
	dfuLong64 sizeCop = GetFileSizeByName(FileNameCop, NULL, pei);
	dfuLong64 size = sizeCop - (sizeCop / 10);
	if (GetFileFullContentBuffer(FileNameCop, FILEFULLCONTENTBUFFER_READWRITE | FILEFULLCONTENTBUFFER_RESIZE, size,
		&hFileContentReadBuf,
		&org, pei))
	{
		dfuLong64 pos = 0;
		dfuLong32 sizeRead = 1;
		for (int iLoop = 0; iLoop < 1000; iLoop++)
		{
			if ((myGetRand(10)>4) && ((pos + sizeRead < size)))
			{
				pos += sizeRead;
			}
			else
			{
				pos = myGetRand(size - 1);
			}

			sizeRead = 1 + (dfuLong32)myGetRand(16384 * 8);
			if ((pos + sizeRead) > size)
				sizeRead = (dfuLong32)(size - pos);

			unsigned char* ptr = ((unsigned char*)GetOrigDataPtrpDataBySizeView(&org, pos, sizeRead));
			ptr += pos;
			adler32_ = adler32(adler32_, ptr, sizeRead);

			if ((iLoop % 3)==0)
			{
				dfuLong32 addBefore = (dfuLong32)myGetRand(sizeRead);
				dfuLong32 sizeWrite = (dfuLong32)myGetRand(sizeRead - addBefore);
				dfuLong32 i;
				for (i = addBefore; i < sizeWrite; i++)
				{
					*(ptr + i)  ^= 0xf;
				}
				SetDataPtrDirtyByPosSize(&org, pos + addBefore, sizeWrite);
			}
		}

		CloseFileFullContentBuffer(hFileContentReadBuf);
	}
	printf("Write: File %S size %I64u=%I64u, adler32_work=%08x\n", FileName, sizeCop, size, adler32_);
	return;
}

void doTestFMIO()
{
	//TestFMIO(L"y:\\avir\\AN_AMERICAN_TAIL.ISO");
	//TestFMIOWrite(L"y:\\avir\\.\\Cassys_csc_rerun.ulp", L"y:\\avir\\garbage.bin");
}

void main210()
{
  dfuLong32 CompressRatio = COMPRESS_RATIO;
  testdateapi();
  dummy();
  /*
     ORIGDATA org;
     ORIGDATAPTR porg=&org;
     dfbytep data_in;
     porg->data="123456789*ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
     porg->size=strlen((const char*)porg->data);

     data_in="ABCDEFGHIJKLMabcdefgh1234567zz_____________________________________BCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
   */
  /*
     printf("\n\nwimr same Same\n");
     TestPatchFile("X:\\tstpat\\wimarold.mdb","X:\\tstpat\\wimarold.mdb");
   */
/*
    printf("\n--minitst1\n");
    TestPatch(porg->data,porg->size,data_in,strlen((const char*)data_in),"internal1_%d.txt","stream %d of internal patch 1");
    printf("\n--minitst2\n");
    TestPatch(data_in,strlen((const char*)data_in),porg->data,porg->size,"internal2_%d.txt","stream %d of internal patch 2");
*/

  printf("\nunimdm\n");
  TestPatchFile("m:\\difstrm\\unimdm.fr", "m:\\difstrm\\unimdm.us",
                CompressRatio);


  if (1 * 01)
  {
    printf("\nusrf\n");
    TestPatchFile("X:\\tstpat\\usrfold.cpp", "X:\\tstpat\\usrfnew.cpp",
                  CompressRatio);


    /*
       printf("\n\nC2 Same\n");
       TestPatchFile("X:\\tstpat\\c2.old","X:\\tstpat\\c2.old");
     */

    /*
       {
       char*szOld="123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmopqrs";
       char*szNew="123456789ABCvwFGHIJKyzNOPQRSTUVWXYZabcdefghijklmopqrs";
       //szNew=szOld;
       printf("\n--minitst3\n");
       TestPatch(szOld,strlen(szOld),szNew,strlen(szNew),"internal3_%d.txt","stream %d of internal patch 3");
       }

       printf("\n\nWIMA_VP4.exe Wima_vp4.reb\n");
       TestPatchFile("X:\\tstpat\\WIMA_VP4.exe","X:\\tstpat\\Wima_vp4.reb");

       printf("\n\nWIMA_VP4.old Wima_vp4.exe\n");
       TestPatchFile("X:\\tstpat\\WIMA_VP4.old","X:\\tstpat\\Wima_vp4.exe");
     */

    printf("\n\nWimar recent\n");
    TestPatchFile("c:\\wimareg.mdb", "D:\\winimg\\wimareg.mdb",
                  CompressRatio);

    printf("\n\nC2 Dif\n");
    TestPatchFile("X:\\tstpat\\c2.old", "X:\\tstpat\\c2.new", CompressRatio);

    {
      int i;
      for (i = 0; i <= 9; i++)
      {
        printf("\n\nwimrNEW access ratio %d\n", i);
        TestPatchFile("s:\\avr\\wimarsav.mdb", "s:\\avr\\wimareg.mdb", i);
      }
    }

    {
      int i;
      for (i = 0; i <= 9; i++)
      {
        printf("\n\nwimr access ratio %d\n", i);
        TestPatchFile("X:\\tstpat\\wimarold.mdb",
                      "X:\\tstpat\\wimareg.mdb", i);
      }
    }

    printf("\n\nwimr access ratio 1\n"/*, CompressRatio*/);
    TestPatchFile("X:\\tstpat\\wimarold.mdb", "X:\\tstpat\\wimareg.mdb", 1);

    printf("\n\nwimr access ratio 6\n"/*, CompressRatio*/);
    TestPatchFile("X:\\tstpat\\wimarold.mdb", "X:\\tstpat\\wimareg.mdb", 6);

  }
}

void main660()
{
  dfuLong32 CompressRatio = COMPRESS_RATIO;
  /*
     ORIGDATA org;
     ORIGDATAPTR porg=&org;
     dfbytep data_in;
     porg->data="123456789*ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
     porg->size=strlen((const char*)porg->data);

     data_in="ABCDEFGHIJKLMabcdefgh1234567zz_____________________________________BCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
   */
  /*
     printf("\n\nwimr same Same\n");
     TestPatchFile("X:\\tstpat\\wimarold.mdb","X:\\tstpat\\wimarold.mdb");
   */
/*
    printf("\n--minitst1\n");
    TestPatch(porg->data,porg->size,data_in,strlen((const char*)data_in),"internal1_%d.txt","stream %d of internal patch 1");
    printf("\n--minitst2\n");
    TestPatch(data_in,strlen((const char*)data_in),porg->data,porg->size,"internal2_%d.txt","stream %d of internal patch 2");
*/

  printf("\nunimdm\n");
  TestPatchFile("d:\\gil\\unimdm.fr", "d:\\gil\\unimdm.us", CompressRatio);

}


void tst210sml()
{
    /*
  dfuLong32 CompressRatio = COMPRESS_RATIO;
  printf("\nunimdm %u\n", CompressRatio);
  TestPatchFile("c:\\gil\\unimdm.fra", "c:\\gil\\unimdm.usa", CompressRatio);
*/
  printf("\nunimdm 1\n");
  TestPatchFile("c:\\gil\\unimdm.fra", "c:\\gil\\unimdm.usa", 1);
/*
  printf("\nunimdm 4\n");
  TestPatchFile("c:\\gil\\unimdm.fra", "c:\\gil\\unimdm.usa", 4);
  */
}

void tst210mdb()
{
  dfuLong32 CompressRatio = 1;
  printf("\nwimxp %u\n", CompressRatio);
  TestPatchFile("d:\\winimg\\wimrxp.old", "d:\\winimg\\wimrxp.mdb", CompressRatio);
}

/*************************************************************************/

void tst210wima()
{
  dfuLong32 CompressRatio = 1;
  /*
  printf("\nwimahlp %u\n", CompressRatio);

  TestPatchFile("l:\\difstrm\\tst\\v2\\WINIMAUS.HLP", "l:\\difstrm\\tst\\v3\\WINIMAUS.HLP", CompressRatio);
  */
  /*
  printf("\nwimahlp %u\n", CompressRatio);
  TestPatchFile("l:\\difstrm\\tst\\v3\\WINIMAUS.HLP", "l:\\difstrm\\tst\\v4\\WINIMAUS.HLP", CompressRatio);
  */
  /*
  printf("\nwimahlp %u\n", CompressRatio);
  TestPatchFile("l:\\difstrm\\tst\\v4\\WINIMAUS.HLP", "l:\\difstrm\\tst\\v5\\WINIMAUS.HLP", CompressRatio);
  printf("\nwimahlp %u\n", CompressRatio);
  TestPatchFile("l:\\difstrm\\tst\\v5\\WINIMAUS.HLP", "l:\\difstrm\\tst\\v6\\WINIMAUS.HLP", CompressRatio);
  */

  printf("\nwimahlp helpus begin %u\n", CompressRatio);
  TestPatchFile("c:\\winimage\\50\\WINIMAUS.HLP", "L:\\IMAGE\\HELPUS\\WINIMAUS.HLP", CompressRatio);
  printf("\nwimahlp helpus end %u\n", CompressRatio);

}
//DfsOpenFile

//#include "DfsMFile.h"

void TestPatchMain();
void TestBuildDfs();
BOOL TestRemix(dfuLong32 dfParam,dfwcharpc dffnOrig,dfwcharpc dffnDest,dfuLong32 dfnbVer,const dfuLong32 * pdfVerTab);

void Tstmain()
{
  DFSFILEINFOPARAMINTERNAL DfsFileParam;
  DFSFILEWRAP DfsFileWrap = NULL;
  dfuLong32 error = 0;

  DfsFileParam.sizeStruct = sizeof(DfsFileParam);
/*
  printf("\nunimdm 1\n");
  TestPatchFile("c:\\gil\\unimdm.fra", "c:\\gil\\unimdm.usa", 1);
*/


  TestBuildDfs();

/*
  DfsFileParam.sizeStruct = sizeof(DfsFileParam);
  DfsFileParam.dfStatus = DFS_NEWFILE | DFS_WRITABLE;
  DfsFileParam.filename = L"c:\\aavirer.bin";

  DfsFileWrap = DfsOpenFile(DfsFileParam);
  DfsWrite(DfsFileWrap, "buf", 3, &error);
  DfsCloseFile(DfsFileWrap);

  //////////////////////////////////////////////
  {
    DFSFILEINFOPARAM DfsFileParam;
    DFSFILE DfsFile;

    DfsFileParam.sizeStruct = sizeof(DFSFILEINFOPARAM);
    DfsFileParam.dfStatus = DFS_NEWFILE | DFS_WRITABLE;
    DfsFileParam.filename = L"c:\\aavirer2.bin";

    DfsFileOpen(&DfsFileParam, &DfsFile);
    DfsWCreateNewDir(DfsFile, 0);
    DfsWCloseFlushCurrentDir(DfsFile);
    DfsClose(DfsFile);
  }*/
  //TestPatchMain();


  //tst210sml();


  printf("end\n");
}

/*
#include "zlib.h"
#include "difstool.h"
#include "difstrmi.h"
#ifndef DEF_MEM_LEVEL
#define DEF_MEM_LEVEL (8)
#endif
int GetNbByteNumber(dfuLong32 size);

#define INTERNALRECOPYBUF (0x40)
int
WriteRecopyAllFile(dfvoidp ptr,dfuLong32 sizeBuffer, dfuLong32 size)
{
  int err;
  z_stream zstr;

  dfbyte tabStream0uncompressed[INTERNALRECOPYBUF];
  dfuLong32 sizeStream0uncompressed;
  dfbyte tabStream0compressed[INTERNALRECOPYBUF];
  dfuLong32 sizeStream0compressed;
  dfbyte tabResult[INTERNALRECOPYBUF];
  dfuLong32 dfResultSize;
  if ((size ==0))
  {
      sizeStream0uncompressed = 0;
  }
  else
  if ((size < 0x1b))
  {
    tabStream0uncompressed[0] = 0x40 + 0x20 + (dfbyte) (size - 1) ;
    sizeStream0uncompressed = 1;
  }
  else
  if ((size < 0x100))
  {
    tabStream0uncompressed[0] = 0x40  ;
    tabStream0uncompressed[1] = (dfbyte)(size-1) ;
    sizeStream0uncompressed = 2;
  }
  else
  {
    dfuLong32Intel sizeIntel;
    dfuLong32Intel posIntel = ConvertuLongToLongIntel(0);
    dfbyte bNbByteNb;
    bNbByteNb = GetNbByteNumber(size);

    tabStream0uncompressed[0] = ((dfbyte) (bNbByteNb + 0x7b) ) |((dfbyte) 0x80);

    sizeIntel = ConvertuLongToLongIntel(size);
    sizeStream0uncompressed = bNbByteNb + 1+4;
    DfsMemcpy(&tabStream0uncompressed[1],&sizeIntel,bNbByteNb);
    DfsMemcpy(&tabStream0uncompressed[1+bNbByteNb],&posIntel,4);

  }

  ClearZStream(&zstr);

    err = deflateInit2(&zstr,
                    (int) Z_DEFAULT_COMPRESSION,
                    Z_DEFLATED, -MAX_WBITS, DEF_MEM_LEVEL, 0);
    zstr.avail_in=sizeStream0uncompressed;
    zstr.next_in=tabStream0uncompressed;


    zstr.avail_out=INTERNALRECOPYBUF;
    zstr.next_out=tabStream0compressed;
    deflate(&zstr,Z_FINISH);
    sizeStream0compressed=zstr.total_out;
    deflateEnd(&zstr);


    tabResult[0]=SMALLHEADER_SIZE_VALUE1|SMALLHEADER_SIGN_VALUE|SMALLHEADER_ENDSTREAM_MASK;
    tabResult[1]=(dfbyte)sizeStream0compressed;
    DfsMemcpy(&tabResult[2],&tabStream0compressed[0],sizeStream0compressed);
    tabResult[sizeStream0compressed+2]=SMALLHEADER_SIZE_VALUE0|SMALLHEADER_SIGN_VALUE|SMALLHEADER_ENDSTREAM_MASK|1;
    dfResultSize=sizeStream0compressed+3;


    if (size==0)
    {
        tabResult[0]=SMALLHEADER_SIZE_VALUE0|SMALLHEADER_SIGN_VALUE|SMALLHEADER_ENDSTREAM_MASK;
        tabResult[1]=SMALLHEADER_SIZE_VALUE0|SMALLHEADER_SIGN_VALUE|SMALLHEADER_ENDSTREAM_MASK|1;
        dfResultSize=2;
    }

    {
        dfuLong32 i;
        printf("size = %6u : ",size);
        for (i=0;i<dfResultSize;i++)
            printf("%02x  ",tabResult[i]);
        printf("\n");
    }

    return 1;
}

int main()
{
    char sz[80];
    WriteRecopyAllFile(sz,32,50);
    WriteRecopyAllFile(sz,32,503808);
    WriteRecopyAllFile(sz,32,75581);
    WriteRecopyAllFile(sz,32,1191);
}*/

static BOOL IsUnicodeSupported()
{
    return  ((GetVersion() & 0x80000000) == 0); // WINNT
}

int PerformTestIdentical(int nbarg,char* argvid[])
{
    int i;
    for (i=0;i<nbarg;i++)
    {
        unsigned long size = atol(argvid[i]);
        unsigned char buf[0x100];
        dfuLong32 dfSizeCode,j;
        if (*argvid[i]=='*')
        {
            size = atol((argvid[i])+1);
            for (j=0;j<10000;j++)
              dfSizeCode=MakeDifStFileIdentical((dfvoidp)buf, sizeof(buf), size);
        }

        printf("identical size %8lu: ",size);
        dfSizeCode=MakeDifStFileIdentical((dfvoidp)buf, sizeof(buf), size);
        printf("%3u bytes : ",dfSizeCode);
        for (j=0;j<dfSizeCode;j++)
            printf("%02x ",buf[j]);
        printf("\n");
    }
    return 0;
}



BOOL DoTestUncompressDfsFile(const unsigned char* fileBuff, size_t fileSize)
{
    int ret = ApplyMonofilePatchMemory("", 1,
        fileBuff,  fileSize,
        "*dest/",
        NULL, NULL, NULL, 0);
        //ERROR_MOMENT *perr_moment,
        //int* perrinfo, char* errBufTxt, int errBufSize);
    return (ret == 1);
}


#define BLOCK_SIZE_RAWUNCOMPRESSDIRECT 65536*4*32

BOOL DoTestUncompressRawFile(const unsigned char* fileBuff, size_t fileSize)
{
    //fseek(fin, 0, SEEK_SET);
    int res;
    BOOL success = TRUE;
    abstract_decompress_stream strm;
    size_t pos_read = 0;

    if (fileSize > 4)
        if (memcmp(fileBuff, "DFS", 3) == 0)
            return DoTestUncompressDfsFile(fileBuff, fileSize);


    unsigned char* buf = (unsigned char*)DfsMalloc(2 * (BLOCK_SIZE_RAWUNCOMPRESSDIRECT));
    unsigned char* buf_in = buf;
    unsigned char* buf_out = buf + (BLOCK_SIZE_RAWUNCOMPRESSDIRECT);

    DfsClearStruct(&strm, 0, sizeof(abstract_decompress_stream));
    res = abstract_init_prefix(&strm);
    if (res != ABSTR_COMPRESS_Z_OK)
    {
        printf("cannot init compress\n");

        DfsFree(buf);
        return FALSE;
    }
    int err = ABSTR_DECOMPRESS_Z_OK;
    while (success)
    {
        if (pos_read >= fileSize)
            break;
        size_t nb_read = (BLOCK_SIZE_RAWUNCOMPRESSDIRECT < (fileSize - pos_read)) ?
        BLOCK_SIZE_RAWUNCOMPRESSDIRECT : (fileSize - pos_read);
        memcpy(buf_in, fileBuff + pos_read, nb_read);
        pos_read += nb_read;

        strm.avail_in = (uInt)nb_read;
        strm.next_in = buf_in;

        while (success)
        {
            strm.avail_out = (uInt)(BLOCK_SIZE_RAWUNCOMPRESSDIRECT);
            strm.next_out = buf_out;
            err = abstract_decompress(&strm, ABSTR_DECOMPRESS_Z_SYNC_FLUSH);

            if ((err != ABSTR_DECOMPRESS_Z_OK) && (err != ABSTR_DECOMPRESS_Z_STREAM_END))
            {
                printf("ERROR\n");
                break;
            }

            uInt doneOut = (uInt)((BLOCK_SIZE_RAWUNCOMPRESSDIRECT)-strm.avail_out);
            if (doneOut > 0)
            {


            }
            if (doneOut == 0)
                break;
            if (strm.avail_in == 0)
                break;
        }
        if (err == ABSTR_DECOMPRESS_Z_STREAM_END)
            break;
    }

    while ((success) && (err != ABSTR_DECOMPRESS_Z_STREAM_END))
    {
        strm.avail_out = (uInt)(BLOCK_SIZE_RAWUNCOMPRESSDIRECT);
        strm.next_out = buf_out;
        err = abstract_decompress(&strm, ABSTR_DECOMPRESS_Z_FINISH);
        uInt done = (uInt)((BLOCK_SIZE_RAWUNCOMPRESSDIRECT)-strm.avail_out);
        if (done > 0)
        {


        }
        if (done == 0)
            break;
    }
    if (err != ABSTR_DECOMPRESS_Z_STREAM_END)
        printf("Error:\n");
    abstract_decompress_end(&strm);
    DfsFree(buf);
    return success;
}


#define TstCprAroundUpper(dwValue,dwModulo) (((((dwValue)) + ((dwModulo)) -1) / ((dwModulo))) * (dwModulo))

BOOL DoTestsCompressThenUncompressMemory(const char* filename, int nbratios, int* ratiosarray)
{
    BOOL success = TRUE;
    BOOL success2 = TRUE;
    //int res;
    FILE* fin;

    fin = fopen(filename, "rb");
    if (fin == NULL)
    {
        printf("cannot open %s\n", filename);
        return FALSE;
    }

    fseek(fin, 0, SEEK_END);


    size_t fileSize = (size_t)ftell(fin);
    size_t fileSizeMarginAround = TstCprAroundUpper((fileSize + (fileSize / 8) + 4096), 0x1000);
    void* buf = (void*)DfsMalloc(fileSizeMarginAround);


    fseek(fin, 0, SEEK_SET);
    if (fread(buf, 1, (size_t)fileSize, fin) != (size_t)fileSize)
    {

        printf("cannot read file %s\n", filename);
        DfsFree(buf);
        fclose(fin);
        return FALSE;
    }
    fclose(fin);

    for (int loop = 0; loop < nbratios; loop++) {
        int compressratio=*(ratiosarray+loop);
        if (loop > 0) printf("\n");
        success = DoTestCompressThenUncompressBufMemory(buf, fileSize, compressratio);
        success2 = DoTestCompressThenUncompressBufMemoryByBlock(buf, fileSize, compressratio);
        if ((!success) || (!success2)) {
            fprintf(stderr, "Error compress ratio %d !!\n", compressratio);
            DfsFree(buf);
            return FALSE;
        }
    }
    DfsFree(buf);
    return TRUE;
}

BOOL DoTestCompressThenUncompressMemory(const char* filename, int compressratio)
{
    return DoTestsCompressThenUncompressMemory(filename, 1, &compressratio);
}


BOOL DoRawUncompressMemTest(const char*file_in, int nbLoop)
{
    BOOL success = TRUE;
    //int res;
    FILE* fin;


    unsigned char* buf = (unsigned char*)DfsMalloc(2 * (BLOCK_SIZE_RAWUNCOMPRESSDIRECT));
    unsigned char* buf_in = buf;
    unsigned char* buf_out = buf + (BLOCK_SIZE_RAWUNCOMPRESSDIRECT);
    if (buf == NULL)
        return FALSE;
    fin = fopen(file_in, "rb");
    if (fin == NULL)
    {
        printf("cannot open %s\n", file_in);
        DfsFree(buf);
        return FALSE;
    }

    fseek(fin, 0, SEEK_END);
    size_t fileSize = (size_t)ftell(fin);

    unsigned char* fileBuff = DfsMalloc((size_t)fileSize + 1);

    fseek(fin, 0, SEEK_SET);
    if (fread(fileBuff, 1, (size_t)fileSize, fin) != (size_t)fileSize)
    {

        printf("cannot read file %s\n", file_in);
        DfsFree(buf);
        DfsFree(fileBuff);
        fclose(fin);
        return FALSE;
    }

    for (int loop = 0; loop < nbLoop; loop++)
    {
        success = DoTestUncompressRawFile(fileBuff, fileSize);
        if (!success)
            break;

        /*
        //fseek(fin, 0, SEEK_SET);
        abstract_decompress_stream strm;
        size_t pos_read = 0;
        DfsClearStruct(&strm, 0, sizeof(abstract_decompress_stream));
        res = abstract_init_prefix(&strm);
        if (res != ABSTR_COMPRESS_Z_OK)
        {
            printf("cannot init compress\n");
            fclose(fin);
            DfsFree(buf);
            DfsFree(fileBuff);
            return FALSE;
        }
        int err = ABSTR_DECOMPRESS_Z_OK;
        while (success)
        {
            if (pos_read >= fileSize)
                break;
            size_t nb_read = (BLOCK_SIZE_RAWUNCOMPRESSDIRECT < (fileSize - pos_read)) ?
            BLOCK_SIZE_RAWUNCOMPRESSDIRECT : (fileSize - pos_read);
            memcpy(buf_in, fileBuff + pos_read, nb_read);
            pos_read += nb_read;

            strm.avail_in = (uInt)nb_read;
            strm.next_in = buf_in;

            while (success)
            {
                strm.avail_out = (uInt)(BLOCK_SIZE_RAWUNCOMPRESSDIRECT);
                strm.next_out = buf_out;
                err = abstract_decompress(&strm, ABSTR_DECOMPRESS_Z_SYNC_FLUSH);

                if ((err != ABSTR_DECOMPRESS_Z_OK) && (err != ABSTR_DECOMPRESS_Z_STREAM_END))
                {
                    printf("ERROR\n");
                    break;
                }

                uInt doneOut = (uInt)((BLOCK_SIZE_RAWUNCOMPRESSDIRECT)-strm.avail_out);
                if (doneOut > 0)
                {


                }
                if (doneOut == 0)
                    break;
                if (strm.avail_in == 0)
                    break;
            }
            if (err == ABSTR_DECOMPRESS_Z_STREAM_END)
                break;
        }

        while (success)
        {
            strm.avail_out = (uInt)(BLOCK_SIZE_RAWUNCOMPRESSDIRECT);
            strm.next_out = buf_out;
            err = abstract_decompress(&strm, ABSTR_DECOMPRESS_Z_FINISH);
            uInt done = (uInt)((BLOCK_SIZE_RAWUNCOMPRESSDIRECT)-strm.avail_out);
            if (done > 0)
            {


            }
            if (done == 0)
                break;
        }
        if (err != ABSTR_DECOMPRESS_Z_STREAM_END)
            printf("Error:\n");
        abstract_decompress_end(&strm);*/
    }
    fclose(fin);
    DfsFree(fileBuff);
    DfsFree(buf);
    return success;
}


#include <windows.h>
int TestFMIO210mo()
{
    HFILECONTENTREADBUF hFileContentReadBuf;
    ORIGDATA Org;
    char sz[150];
    H_ERROR_INFO hei=NULL;
    dfwcharpc dfFN = L"k:\\avirbig.svf";
    //dfwcharpc dfFN = L"y:\\IsoImg\\lhbeta1\\en_longhorn_beta1_dvd.iso";

    if (!GetFileFullContentBuffer(dfFN,FILEFULLCONTENTBUFFER_READ,0,&hFileContentReadBuf,
                               &Org, &hei))
    {
        printf("bad\n");
        return 1;
    }

    wsprintf(sz,"file size=%I64u -  %I64x '%ws', size of ptr,org str=(%u,%u)\n",Org.size,Org.size,dfFN,sizeof(void*),sizeof(ORIGDATA));
    printf("%s",sz);

    {
        dfbytep ptr;
        int i;

        ptr = (dfbytep)GetOrigDataPtrpDataByEndView(&Org,0x8F0ECA76,0x8F0ECA76+0x278);
        for (i=0;i<0x10;i++)
            printf("%02x ",*(ptr+i+0x8F0ECA76));
        printf("\n");

        ptr = (dfbytep)GetOrigDataPtrpDataBySizeView(&Org,0x2247b4e40,0x250);
        for (i=0;i<0x10;i++)
            printf("%02x ",*(ptr+i+0x2247b4e40));
        printf("\n");

        ptr = (dfbytep)GetOrigDataPtrpDataBySizeView(&Org,Org.size-0x11,0x11);
        for (i=0;i<0x11;i++)
            printf("%02x ",*(ptr+i+Org.size-0x11));
        printf("\n");


        ptr = (dfbytep)GetOrigDataPtrpDataBySizeView(&Org,0x23654782,0x11);
        for (i=0;i<0x11;i++)
            printf("%02x ",*(ptr+0x23654782+i));
        printf("\n");

        ptr = (dfbytep)GetOrigDataPtrpDataBySizeView(&Org,2,0x11);
        for (i=0;i<0x11;i++)
            printf("%02x ",*(ptr+i+2));
        printf("\n");
    }
    CloseFileFullContentBuffer(hFileContentReadBuf);
    return 0;
}

#include "../src/lib/helper/decompress/ExtractHelper.h"
#include "../src/lib/engine/patchstream/common/DfsIoHlp.h"
#include "../src/lib/engine/patchstream/common/DfsIoHlpInternal.h"

int testapplymem(const char* pat, const char* file, const char* expectedResult)
{
	char *prefix = "*y:\\avir\\testpat";

	char dstOrg[0x200];
	dfwchar dfwSrcOrg[MAX_PATH_LENGTH];
	dfwchar dfwDstOrg[MAX_PATH_LENGTH];
	strcpy(dstOrg, prefix);
	strcat(dstOrg, "\\");
	strcat(dstOrg, file);


	ConvertAnsiToUnicode(file, dfwSrcOrg, MAX_PATH_LENGTH);
	ConvertAnsiToUnicode(dstOrg, dfwDstOrg, MAX_PATH_LENGTH);
	//ManualCopyFile()
	BOOL res = ManualCopyFile(dfwSrcOrg, dfwDstOrg, FALSE, FALSE, NULL);
	ERROR_MOMENT err_moment = 0;
	int errinfo = 0;
	dfuLong32 dfExtractingMethod = 2;
	char szMsg[0x200] = "";
	int res2 = ApplyMonofilePatchEx2(prefix, 0, pat, prefix,
		&err_moment, &errinfo, szMsg, sizeof(szMsg) - 1,
		dfExtractingMethod, NULL, NULL, 0, 100);
	printf("res apply : %d - %s - %s\n", res, GetErrorMomentText(err_moment), szMsg);
	if (res2 == 1)
	{

		char dstRes[0x200];
		dfwchar dfwRes[MAX_PATH_LENGTH];
		dfwchar dfwResCopy[MAX_PATH_LENGTH];
		strcpy(dstRes, prefix);
		strcat(dstRes, "\\");
		strcat(dstRes, expectedResult);

		ConvertAnsiToUnicode(dstRes, dfwRes, MAX_PATH_LENGTH);
		ConvertAnsiToUnicode("theresult.bin", dfwResCopy, MAX_PATH_LENGTH);
		MyDeleteFile(dfwResCopy, NULL);
		if (ManualCopyFile(dfwRes, dfwResCopy, FALSE, FALSE, NULL) == 0)
			printf("cannot copy dest %s\n",dstRes);
		else
			printf("done copy result\n");
	}
	return res2;
}


void demoavir()
{

#define NB_BITS_PACK_PENDING 32
#define offset_pending(a) ((a) / NB_BITS_PACK_PENDING)
#define mask_pending(a) (1 << ((a) - (((a) / NB_BITS_PACK_PENDING) * NB_BITS_PACK_PENDING)) )

	for (int i = 0; i < 80; i++)
	{
		//printf("%03d = %03x  offset = %d mask = %lx\n", i, i, offset_pending(i), (unsigned long)mask_pending(i));
	}
	//printf("\n");
}

#if defined(SMARTVERSION_MAIN)

int main(int argc, char *argv[])
{
int iRet=0;
demoavir();
dfwcharp pCommandLine;

BOOL mustContinue;
int retTryCustomMain;


    SetErrorMode(SEM_NOALIGNMENTFAULTEXCEPT);   // FOR RISC and ia64

    DfsCrc32(0,(BYTE*)" ",1); // init CRC32

	doTestFMIO();



	retTryCustomMain = TryCustomMain(argc, argv, &mustContinue);
	if (mustContinue == FALSE)
		return retTryCustomMain;

#if defined(DUMPER_HANDER) && !defined(NO_DUMPER_HANDLER)
    if (argc>1)
        if (lstrcmpi(argv[1],"/dumpcrash")==0)
        {
            int i;
            InstallDumperHandler("SmartVersionConsole");
            for (i=2;i<argc;i++)
            {
                argv[i-1] = argv[i];
            }
        }
#endif

    if (argc>1)
        if (lstrcmpi(argv[1],"/crashtest")==0)
        {
            char* ptr=(char*)((DWORD_PTR)0xb1256622);
            printf("crash here");
            *ptr=4;
        }

    if (argc>3)
        if (lstrcmpi(argv[1],"/testapplymem")==0)
        {
		return testapplymem(argv[2], argv[3],argv[4]);

        }
#ifdef RAWCOMPRESSDIRECT
    if (argc>3)
        if (lstrcmpi(argv[1],"/compressraw")==0)
        {
            BOOL success;
            const char* srcFile = argv[2];
            const char* dstFile = argv[3];
            unsigned int ratio = 0;
            int strip_prefix = 0;
            int isStandardzLibWithNegMaxWBits = 0;
            if (argc > 4)
                ratio = (unsigned int)atol(argv[4]);
            if (argc > 5)
                isStandardzLibWithNegMaxWBits = (int)atoi(argv[5]);
            if (argc > 6)
                if ((lstrcmpi(argv[6], "stripprefix") == 0) || (lstrcmpi(argv[6], "strip") == 0))
                    strip_prefix = 1;
            success = DoRawCompress(srcFile, dstFile, ratio, isStandardzLibWithNegMaxWBits, strip_prefix);
            if (success)
                printf("compress success\n");
            else
                printf("compress fail\n");
            return success ? 0 : 1;
        }
#endif

#ifdef RAWUNCOMPRESSDIRECT
    if (argc>3)
        if (lstrcmpi(argv[1],"/uncompressraw")==0)
        {
            BOOL success;
            const char* srcFile = argv[2];
            const char* dstFile = argv[3];
            success = DoRawUncompress(srcFile, dstFile);
            if (success)
                printf("uncompress success\n");
            else
                printf("uncompress fail\n");
            return success ? 0 : 1;
        }

    if (argc>2)
        if (lstrcmpi(argv[1], "/uncompressrawtest") == 0)
        {
            BOOL success;
            const char* srcFile = argv[2];
            int nbLoop = 1;
            if (argc>3)
                nbLoop = atoi(argv[3]);
            success = DoRawUncompressMemTest(srcFile, nbLoop);
            if (success)
                printf("uncompress test success\n");
            else
                printf("uncompress test fail\n");
            return success ? 0 : 1;
        }
#endif
  //  tst210wima();
  //  tst210sml();
  //TestBuildDfs();
  //  tst210mdb();

//TestPatchMain();

  //return TestFMIO210mo();
    if (argc>1)
    {
        if (lstrcmpi(argv[1],"DoTestIdentical")==0)
        {
            return PerformTestIdentical(argc-2,&argv[2]);
        }
        else
        if (((lstrcmpi(argv[1],"DisplaySvfInfo")==0)) && (argc>2))
        {
            KINDDFSFILE kdfsf;
            char BaseFileName[0x200];
            dfuLong64 SizeBase;
            dfuLong32 crc32Base;
            BOOL fCrc32PresentBase,fMd5PresentBase,fSha1PresentBase, fSha256PresentBase;
            unsigned char bMd5Base[16];
            unsigned char bSha1Base[20];
			unsigned char bSha256Base[32];
            char TargetFileName[0x200];
            dfuLong64 SizeTarget;
            dfuLong32 crc32Target;
            BOOL fCrc32PresentTarget,fMd5PresentTarget,fSha1PresentTarget, fSha256PresentTarget;
            unsigned char bMd5Target[16];
            unsigned char bSha1Target[20];
			unsigned char bSha256Target[32];
            dfuLong64 dfDeplInPlaceSize,dfDeplOutPlaceSize;
            BOOL fInsertSizeFound;

            ERROR_MOMENT err_moment=ERROR_MOMENT_NO_ERROR;
            int errinfo=0;
            char errBuf[0x100]="";
			fSha256PresentBase = fSha256PresentTarget = FALSE;
            int ret = GetSvfMonoPatchInfoEx2(argv[2],
                        &kdfsf,
                        BaseFileName,0x200,&SizeBase,&crc32Base,&fCrc32PresentBase,
                        bMd5Base,&fMd5PresentBase,
                        bSha1Base,&fSha1PresentBase,
                        bSha256Base,&fSha256PresentBase,


                        TargetFileName,0x200,&SizeTarget,&crc32Target,&fCrc32PresentTarget,
                        bMd5Target,&fMd5PresentTarget,
                        bSha1Target,&fSha1PresentTarget,
                        bSha256Target,&fSha256PresentTarget,

                        &dfDeplInPlaceSize,&dfDeplOutPlaceSize,&fInsertSizeFound,
                        &err_moment,&errinfo,errBuf,sizeof(errBuf)-1,
                        NULL,NULL,NULL,0);


            printf("ret = %d , errinfo=%d, err_moment=%x errmsg='%s'\n",ret,errinfo,(int)err_moment,errBuf);
            if (BaseFileName[0]!='\0')
            {
                char szNumSize[40];
                ConvertNum64ToUnformattedStrAnsi(SizeBase,szNumSize,32,13);
                printf("Base : '%s' size=%s",BaseFileName,szNumSize);
                if (fCrc32PresentBase) printf(" crc32=0x%08x",crc32Base);

                printf("\n"/*,BaseFileName*/);
            }
            if (TargetFileName[0]!='\0')
            {
                char szNumSize[40];
                ConvertNum64ToUnformattedStrAnsi(SizeTarget,szNumSize,32,13);
                printf("Target : '%s' size=%s",TargetFileName,szNumSize);
                if (fCrc32PresentTarget) printf(" crc32=0x%08x",crc32Target);

                printf("\n"/*,TargetFileName*/);
            }
            if (fInsertSizeFound)
            {
                char szNumInPlace[40];
                char szNumOutPlace[40];
                char szNumInsertPlace[40];
                char szNumTotal[40];
                ConvertNum64ToUnformattedStrAnsi(dfDeplInPlaceSize,szNumInPlace,32,13);
                ConvertNum64ToUnformattedStrAnsi(dfDeplOutPlaceSize,szNumOutPlace,32,13);
                ConvertNum64ToUnformattedStrAnsi(SizeTarget-(dfDeplInPlaceSize+dfDeplOutPlaceSize),szNumInsertPlace,32,13);
                ConvertNum64ToUnformattedStrAnsi(SizeTarget,szNumTotal,32,13);
                printf("DeplInPlace=%s, DeplOutPlace=%s, Insert=%s for total %s\n",szNumInPlace,szNumOutPlace,szNumInsertPlace,szNumTotal);
            }
            return ret;
        }
        else
        if (((lstrcmpi(argv[1],"ApplyMonofilePatch")==0) || (lstrcmpi(argv[1],"ApplyMonofilePatchInPlace")==0) || (lstrcmpi(argv[1],"ApplyMonofilePatchInPlaceNoChecksum")==0)) && (argc>4))
        {
            ERROR_MOMENT err_moment=ERROR_MOMENT_NO_ERROR;
            int errinfo=0;
            char errBuf[0x100]="";

            int ret= ApplyMonofilePatchEx(argv[2],1*0,argv[3],argv[4],&err_moment,&errinfo,errBuf,sizeof(errBuf)-1,
                (lstrcmpi(argv[1],"ApplyMonofilePatchInPlace")==0) ? EXTRACTING_METHOD_INPLACE :
                (lstrcmpi(argv[1],"ApplyMonofilePatchInPlaceNoChecksum")==0) ? EXTRACTING_METHOD_INPLACE_NOCHECKSUM : EXTRACTING_METHOD_CLASSIC);
            printf("ret = %d , errinfo=%d, err_moment=%x errmsg='%s'\n",ret,errinfo,(int)err_moment,errBuf);
            return ret;
        }
        else
        if (((lstrcmpi(argv[1],"DoTestBuildHelper")==0) || (lstrcmpi(argv[1],"BuildSvfPatch")==0)) && (argc>4))
        {
            int retValue;
            signed long CompressRatio = -1;
            char errText[0x400] = "";
            if (argc>5)
                CompressRatio = atol(argv[5]);

            retValue = BuildPatchFromTwoFileEx3(argv[2], argv[3] , argv[4], errText, sizeof(errText), CompressRatio, -1, -1, 0);


            if (retValue != 0)
            {
                printf("Error %d : %s\n", retValue, errText);
            }

            return retValue;

        }
        if (lstrcmpi(argv[1],"DoTestRemix")==0)
        {
            dfuLong32 dfParam=0;
            dfwcharp dffnOrig=NULL;
            dfwcharp dffnDest=NULL;

            dfwchar szFnOrig[MAX_PATH+2];
            dfwchar szFnDest[MAX_PATH+2];
            dfuLong32 dfnbVer=0;
            dfuLong32 dfVerTab[200];

            if (argc>2)
              dfParam=(dfuLong32)atol(argv[2]);

            if (argc>3)
            {
                dffnOrig=szFnOrig;
                wsprintfW(dffnOrig,L"%S",argv[3]);
            }

            if (argc>4)
              wsprintfW(dffnDest=szFnDest,L"%S",argv[4]);

            if (argc>5)
            {
                dfuLong32 i;
                dfnbVer = argc-5;
                for (i=0;i<dfnbVer;i++)
                    dfVerTab[i]=(dfuLong32)(atol(argv[i+5]));
            }

            return (TestRemix(dfParam,dffnOrig,dffnDest,dfnbVer,dfVerTab) ? 0 : 1);
        }
    }

    if (argc>3)
    {
        if ((lstrcmpi(argv[1],"DoRecompressPatch")==0) || (lstrcmpi(argv[1],"DRP")==0))
        {
            dfuLong32 CompressRatio = COMPRESSIONPARAM_AUTOVALUE;
            char errText[0x400] = "";
            int retValue = 0;
            if (argc>4)
                CompressRatio=atol(argv[4]);
            retValue = RecompressPatch(argv[2], argv[3], errText, sizeof(errText), CompressRatio, -1, -1, 0) ;

            //printf("retValue main : %d\n", retValue);
            if (retValue != 0)
            {
                printf("Error %d : %s\n", retValue, errText);
            }

            if (retValue == 0)
            {
                {
                    dfuLong64 deplInPlaceSize = 0;
                    dfuLong64 deplOutPlaceSize = 0;
                    BOOL insertSizeFoud = FALSE;
                    int fromold = GetSvfMonoPatchInfo(argv[2],
                        NULL,
                        NULL, 0,
                        NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                        &deplInPlaceSize, &deplOutPlaceSize, &insertSizeFoud, NULL, NULL, NULL, 0);
                    if (fromold && insertSizeFoud)
                        printf("old : insert in place : %d, outplace : %d (total %d)\n", (int)deplInPlaceSize, (int)deplOutPlaceSize, (int)deplInPlaceSize + (int)deplOutPlaceSize);
                }
                {
                    dfuLong64 deplInPlaceSize = 0;
                    dfuLong64 deplOutPlaceSize = 0;
                    BOOL insertSizeFoud = FALSE;
                    int fromnew = GetSvfMonoPatchInfo(argv[3],
                        NULL,
                        NULL, 0,
                        NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                        &deplInPlaceSize, &deplOutPlaceSize, &insertSizeFoud, NULL, NULL, NULL, 0);
                    if (fromnew && insertSizeFoud)
                        printf("new : insert in place : %d, outplace : %d (total %d)\n", (int)deplInPlaceSize, (int)deplOutPlaceSize, (int)deplInPlaceSize + (int)deplOutPlaceSize);
                }
            }


            return retValue ? 0 : 1;
        }
        else
        if ((lstrcmpi(argv[1], "DoTestPatchFileNoMapNewBufOld") == 0) || (lstrcmpi(argv[1], "DTPFNMNBO") == 0))
        {
            dfuLong32 CompressRatio = COMPRESSIONPARAM_AUTOVALUE;
            if (argc>4)
                CompressRatio = atol(argv[4]);
            return (TestPatchFileNoMapNew(argv[2], argv[3], CompressRatio, FALSE, TRUE) ? 0 : 1);
        }
        else
        if ((lstrcmpi(argv[1],"DoTestCompressThenUncompressMemory")==0) && (argc>2))
        {
            dfuLong32 CompressRatio = COMPRESSIONPARAM_AUTOVALUE;
            if (argc>3)
                CompressRatio=atol(argv[3]);
            return (DoTestCompressThenUncompressMemory(argv[2],CompressRatio) ? 0 : 1);
        }
        else
        if ((lstrcmpi(argv[1],"DoTestsCompressThenUncompressMemory")==0) && (argc>2))
        {
            int nbratios=argc-3;
            int* ratiosarray=(int*)DfsMalloc(sizeof(int)*(nbratios+1));
            int loop;
            BOOL res;
            for (loop=0;loop<nbratios;loop++)
                *(ratiosarray+loop)=atol(argv[loop+3]);
            if (nbratios==0) {
                nbratios=1;
                *ratiosarray=COMPRESSIONPARAM_AUTOVALUE;
            }
            res=DoTestsCompressThenUncompressMemory(argv[2],nbratios,ratiosarray);
            DfsFree(ratiosarray);
            return (res ? 0 : 1);
        }
        if ((lstrcmpi(argv[1],"DoTestPatchFileNoMapNew")==0) || (lstrcmpi(argv[1],"DTPFNMN")==0))
        {
            dfuLong32 CompressRatio = COMPRESSIONPARAM_AUTOVALUE;
            if (argc>4)
                CompressRatio=atol(argv[4]);
            return (TestPatchFileNoMapNew(argv[2],argv[3],CompressRatio,TRUE,TRUE) ? 0 : 1);
        }
        else
        if ((lstrcmpi(argv[1],"DoTestPatchFileNoMapNewBufOldNoVirtualAlloc")==0) || (lstrcmpi(argv[1],"DTPFNMNBONVA")==0))
        {
            dfuLong32 CompressRatio = COMPRESSIONPARAM_AUTOVALUE;
            if (argc>4)
                CompressRatio=atol(argv[4]);
            return (TestPatchFileNoMapNew(argv[2],argv[3],CompressRatio,FALSE,FALSE) ? 0 : 1);
        }
        else
        if ((lstrcmpi(argv[1],"DoTestPatchFileNoMapNewNoVirtualAlloc")==0) || (lstrcmpi(argv[1],"DTPFNMNNVA")==0))
        {
            dfuLong32 CompressRatio = COMPRESSIONPARAM_AUTOVALUE;
            if (argc>4)
                CompressRatio=atol(argv[4]);
            return (TestPatchFileNoMapNew(argv[2],argv[3],CompressRatio,TRUE,FALSE) ? 0 : 1);
        }
        else
        if (lstrcmpi(argv[1],"DoTestPatchFile")==0)
        {
            dfuLong32 CompressRatio = COMPRESSIONPARAM_AUTOVALUE;
            if (argc>4)
                CompressRatio=atol(argv[4]);
            return (TestPatchFile(argv[2],argv[3],CompressRatio) ? 0 : 1);
        }
        else
        if (lstrcmpi(argv[1],"DoTestPatchFileRamDif")==0)
        {
            dfuLong32 CompressRatio = COMPRESSIONPARAM_AUTOVALUE;
            if (argc>4)
                CompressRatio=atol(argv[4]);
            return (TestPatchFileRamDif(argv[2],argv[3],CompressRatio) ? 0 : 1);
        }
        else
        if (lstrcmpi(argv[1],"DoTestPatchFileRamDif3")==0)
        {
            dfuLong32 CompressRatio = COMPRESSIONPARAM_AUTOVALUE;
            if (argc>4)
            {
                if (argc>5)
                CompressRatio=atol(argv[5]);
              return (TestPatchFileRamDif3(argv[2],argv[3],argv[4],CompressRatio) ? 0 : 1);
            }
        }
    }

    if (IsUnicodeSupported())
    {
        pCommandLine = GetCommandLineW();
        iRet=PerformCommandLine(pCommandLine);
    }
    else
    {
        LPCSTR lpCommandLineAnsi = GetCommandLineA();
        DWORD dwSize = (lstrlenA(lpCommandLineAnsi)*4) + 0x10;
        pCommandLine = DfsMalloc(dwSize+0x10);
        ConvertAnsiToUnicode(lpCommandLineAnsi,pCommandLine,dwSize/sizeof(dfwchar));
        iRet=PerformCommandLine(pCommandLine);
        DfsFree(pCommandLine);
    }

    return iRet;
}
#endif

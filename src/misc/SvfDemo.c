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
#include <conio.h>
#include <windows.h>

#include "svfdll.h"

#if defined(SVF_DEMO)

/*#include "mdump.h"*/




#define tryCall(fnc) \
                { dfuLong32 dfErrorTC=fnc; \
                  if (dfErrorTC!=DFS_SUCCESS) printf("TC:**error %u %s\n",dfErrorTC,#fnc); else printf("TC:success %s\n",#fnc); }


#define tryCallcp(fnc,vcp) \
                { dfuLong32 dfErrorTC=fnc; \
                  if (dfErrorTC!=DFS_SUCCESS) printf("TC:**error %u %s\n",dfErrorTC,#fnc); else printf("TC:success %s\n",#fnc); \
                  *(vcp)=dfErrorTC;}






/****************************************************************************/
/****************************************************************************/
/****************************************************************************/




void FillOneVersionFullCopy(VERSIONTOADD_REMIX * pVersionRemix,DIRINFO* pDirInfo)
{
    FILETOCOPYINFO_REMIX* pFileCopyInfo;
    dfuLong32 i;
    pVersionRemix->dfNbFileToAdd=0;
    pVersionRemix->pfta=NULL;
    pVersionRemix->dfNbPreviousFileInMask = pDirInfo->dfNbFile;

    pFileCopyInfo = (FILETOCOPYINFO_REMIX*)DfsMalloc(sizeof(FILETOCOPYINFO_REMIX)*(pVersionRemix->dfNbPreviousFileInMask+1));
    pVersionRemix->pFileCopyInfo=pFileCopyInfo;
    for (i=0;i<pVersionRemix->dfNbPreviousFileInMask;i++)
    {
        (pFileCopyInfo+i)->dfReferenceItem=FTCI_REFERENCE_UNMODIFIED;
        (pFileCopyInfo+i)->fIsReferenceInAddedFile=FALSE;
    }
}

void FreeOneVersionFullCopy(VERSIONTOADD_REMIX * pVersionRemix)
{
    DfsFree(pVersionRemix->pFileCopyInfo);
}

#if defined(_DEBUG) && (!defined (USE_SVF_DLL))
extern dfuLong32 dfTotalAlloc ;
extern dfuLong32 dfTotalNbAlloc ;
#else
static dfuLong32 dfTotalAlloc =0;
static dfuLong32 dfTotalNbAlloc =0;
#endif

BOOL TestRemix(dfuLong32 dfParam,dfwcharpc dffnOrig,dfwcharpc dffnDest,dfuLong32 dfnbVer,const dfuLong32 * pdfVerTab)
{
  DFSFILE DfsFileIn = NULL;
  DFSFILEINFOPARAM DfsFileParamIn;

  //DFSFILEINFOPARAM DfsFileParamOut;
  WCHAR swzoutput[MAX_PATH+1];
  dfwcharpc pswzoutput;

  PDIRINFO* pDirInfo=NULL;//(PDIRINFO*)DfsMalloc(sizeof(PDIRINFO)*(DfsFileAndInfo.dfNbDir+1));
  dfuLong32 dfNbDir;
  dfuLong32 dfNumDir;
  wsprintfW(swzoutput,L"Y:\\avir\\winimage_history_out_%u.svf",dfParam);
  pswzoutput = (dffnDest!=NULL) ?  dffnDest : swzoutput;

  DfsFileParamIn.sizeStruct = sizeof(DfsFileParamIn);
  DfsFileParamIn.dfStatus = DFS_READABLE;
  DfsFileParamIn.filename = (dffnOrig != NULL) ? dffnOrig : (L"Y:\\avir\\winimage_history_work.svf");
  tryCall(DfsOpen(DfsFileParamIn, &DfsFileIn, NULL));

  printf("remix start param %u %ws to %ws\n",dfParam,DfsFileParamIn.filename,pswzoutput);

  if (dfnbVer>0)
  {
      dfuLong32 i;
      for (i=0;i<dfnbVer;i++)
        printf("    integrate version %u\n",*(pdfVerTab+i));
  }


  DfsGetNbDir(DfsFileIn, &dfNbDir, NULL);
  pDirInfo = (PDIRINFO*)DfsMalloc(sizeof(PDIRINFO)*(dfNbDir+1));

  for (dfNumDir = 0; dfNumDir < dfNbDir; dfNumDir++)
  {
      dfuLong32 dfError = DFS_SUCCESS;
      dfError = ReadDirectoryInfo(DfsFileIn, dfNumDir, (pDirInfo)+dfNumDir, NULL, NULL, NULL);
      if (dfNumDir > 0)
      {
          PDIRINFO pDirInfoPrev=*((pDirInfo)+dfNumDir-1);
          PDIRINFO pDirInfoCur=*((pDirInfo)+dfNumDir);

          FixIndenticalDifferentSizeInReadDirectoryInfo(pDirInfoPrev,pDirInfoCur);
      }

      if (dfError != DFS_SUCCESS)
      {
          //fRet = FALSE;
          break;
      }
  }



    {
    dfuLong32 dfError = DFS_SUCCESS;
    dfuLong32 dfBlockType = 0;
    dfuLong32 dfNbDir = 0;
    dfuLong32 dfNumDir;

    ConvertOldDirectoryCommentStorage(DfsFileIn,NULL);
    DfsGetNbDir(DfsFileIn, &dfNbDir, NULL);


    for (dfNumDir = 0; dfNumDir < dfNbDir; dfNumDir++)
        {
        dfuLong32 i = 0;
        PDIRINFO pDirInfo = NULL;
        dfuLong32 dfGetNbFile = 0;

        dfError = ReadDirectoryInfo(DfsFileIn, dfNumDir, &pDirInfo, NULL, NULL, NULL);
        if (pDirInfo != NULL)
            {
            dfGetNbFile = pDirInfo->dfNbFile;
            printf("\nDirectory Number %u, type %u\n", dfNumDir + 0,
                    (pDirInfo->dfTypeDir));

            printf
                ("Name                                Date   Time       Size    Packed  CRC-32\n"
                "--------------------------------- -------- -----  --------  -------- --------\n");

            while ((i < dfGetNbFile) && (dfError == DFS_SUCCESS))
                {
                dfvoidp TagBuf;
                dfuLong32 TagSize;
                {
                    printf("%-32ws  ", (pDirInfo->pFileInDirInfo + i)->FileName);
                }
                if (GetTag
                    (*(pDirInfo->TagFile + i), DFSTAG_DATE, &TagBuf, &TagSize))
                    {
                    DFSTM dfsTm;
                    ConvertDfsInfoDateToDfsTm((DFSINFODATE *) TagBuf, &dfsTm);
                    printf("%02u/%02u/%02u %02u:%02u ", dfsTm.df_mday,
                            dfsTm.df_mon, dfsTm.df_year % 100, dfsTm.df_hour,
                            dfsTm.df_min, dfsTm.df_sec);
                    }
                printf("%9u %9u ", (pDirInfo->pFileInDirInfo + i)->dfSize,
                        (pDirInfo->pFileInDirInfo + i)->dfFileEncodedSize);

                if ((pDirInfo->pFileInDirInfo + i)->fCrc32Filled)
                    printf("%08lx", (pDirInfo->pFileInDirInfo + i)->dfCrc32);
                printf("\n");

                i++;
                }


            }
            FreeDirectoryInfo(&pDirInfo,NULL);
        }

    }

  {
      dfuLong32 dfNbVersionRemix=0;
      VERSIONTOADD_REMIX * pVersionRemix=NULL;
      COMPRESSIONPARAM cprParam;
      dfuLong32 dfTotalAllocSav,dfTotalNbAllocSav;
      DFSFILE DfsFileOut = NULL;
      dfuLong32 dfErr;
      const FILESET* pFileSetBase = NULL;

      InitDefaultCompressionParam(&cprParam);
      cprParam.uZlibCompressRatio=1;
      cprParam.dfBlockCalcSizeSearch=28;

      if (dfnbVer>0)
      {
          dfuLong32 i;
          dfNbVersionRemix = dfnbVer;

          pVersionRemix = (VERSIONTOADD_REMIX*)DfsMalloc(sizeof(VERSIONTOADD_REMIX)*(dfNbVersionRemix+1));

          for (i=0;i<dfnbVer;i++)
          {
              (pVersionRemix+i)->dfNumVersionPreviousSvf=*(pdfVerTab+i);
              FillOneVersionFullCopy((pVersionRemix+i),*(pDirInfo+(pVersionRemix+i)->dfNumVersionPreviousSvf));
          }

          if (dfParam==5)
          {
              (((pVersionRemix+1)->pFileCopyInfo)+5)->dfReferenceItem = FTCI_REFERENCE_DELETE;
              (((pVersionRemix+2)->pFileCopyInfo)+7)->dfReferenceItem = FTCI_REFERENCE_DELETE;
          }
      }
      else
      if (dfParam!=0)
      {
        dfNbVersionRemix = 2;

        pVersionRemix = (VERSIONTOADD_REMIX*)DfsMalloc(sizeof(VERSIONTOADD_REMIX)*(dfNbVersionRemix+1));
        (pVersionRemix+0)->dfNumVersionPreviousSvf=0;
        (pVersionRemix+1)->dfNumVersionPreviousSvf=dfParam;
        FillOneVersionFullCopy((pVersionRemix+0),*(pDirInfo+(pVersionRemix+0)->dfNumVersionPreviousSvf));
        FillOneVersionFullCopy((pVersionRemix+1),*(pDirInfo+(pVersionRemix+1)->dfNumVersionPreviousSvf));
      }
      else
      {
        dfNbVersionRemix = 3;

        pVersionRemix = (VERSIONTOADD_REMIX*)DfsMalloc(sizeof(VERSIONTOADD_REMIX)*(dfNbVersionRemix+1));
        (pVersionRemix+0)->dfNumVersionPreviousSvf=0;
        (pVersionRemix+1)->dfNumVersionPreviousSvf=1;
        (pVersionRemix+2)->dfNumVersionPreviousSvf=4;

        FillOneVersionFullCopy((pVersionRemix+0),*(pDirInfo+(pVersionRemix+0)->dfNumVersionPreviousSvf));
        FillOneVersionFullCopy((pVersionRemix+1),*(pDirInfo+(pVersionRemix+1)->dfNumVersionPreviousSvf));
        FillOneVersionFullCopy((pVersionRemix+2),*(pDirInfo+(pVersionRemix+2)->dfNumVersionPreviousSvf));
      }

      dfTotalAllocSav=dfTotalAlloc;
      dfTotalNbAllocSav=dfTotalNbAlloc;

      dfErr=
        DoReMixDfs(DfsFileIn, dfNbDir, pDirInfo,
                    &DfsFileOut,pswzoutput,      // BOOL fZipFile,
                    FALSE,
                    FALSE,// fBaseDirectorySelected
                    0, //dfBaseDirNum
                    NULL, //wchBaseDirectory
                    pFileSetBase,
                    dfNbVersionRemix,
                    pVersionRemix,
                    FALSE, // fFirstVersionAsReference
                    TRUE, //BOOL fReuseOldPatch,  // future
                    &cprParam, //   const COMPRESSIONPARAM * pCprParam
                    NULL,//   tSetExtractPosCallBack pSetExtractPosCallBack
                    NULL,   //dfvoidp dfUserPtr
                    0,0, //dfuLong32 dwMinProgress,dfuLong32 dwMaxProgress
                    NULL);

      tryCall(DfsClose(DfsFileOut,NULL));
      printf("dfTotalAllocSav=%u,dfTotalNbAllocSav=%u \n",dfTotalAllocSav,dfTotalNbAllocSav); ;

      printf("dfTotalAlloc=%u,dfTotalNbAlloc=%u \n",dfTotalAlloc,dfTotalNbAlloc); ;

      {
          dfuLong32 i;
          for (i=0;i<dfNbVersionRemix;i++)
              FreeOneVersionFullCopy((pVersionRemix+i));
      }
      DfsFree(pVersionRemix);
  }

  for (dfNumDir = 0; dfNumDir < dfNbDir; dfNumDir++)
  {
      FreeDirectoryInfo(pDirInfo+dfNumDir,NULL);
  }
  DfsFree(pDirInfo);


  tryCall(DfsClose(DfsFileIn,NULL));

  printf("final: dfTotalAlloc=%u,dfTotalNbAlloc=%u \n",dfTotalAlloc,dfTotalNbAlloc);
  return TRUE;
}


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
  fRet = GetFileFullContentReadBuffer(szwFileOld,&hfcrOld,&OrigOld,NULL) &&
         GetFileFullContentReadBuffer(szwFileNew,&hfcrNew,&OrigNew,NULL) ;
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

  CloseFileFullContentReadBuffer(hfcrOld);
  CloseFileFullContentReadBuffer(hfcrNew);

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
         (DWORD)(dwNbTick /1000),(DWORD)dwNbTick, dwBegin, GetTickCount() );
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
      printf("\nERROR NUMBER READ IN APPLY  out=%u, in=%u\n\n",
             wstr.out_data_stream.total_out, rstr.in_data_stream.total_in);

    {
      int icmp = memcmp(pszBufOut, data_in, (dfuIntPtr)rstr.out_data_stream.total_out);
      printf("size in,out = %lu,%lu, size cpr,old = %lu,%lu\n",
             wstr.in_data_stream.total_in, rstr.out_data_stream.total_out, wstr.out_data_stream.total_out, size_old);
      printf("res cmp=%ld, adler=%lx\n", icmp, ulcr);
      fRet = ((icmp == 0) && (wstr.in_data_stream.total_in == rstr.out_data_stream.total_out));
      if (fRet)
        printf("ok, patch is GOOD, the patch size is %u (%u KB)\n", (dfuLong32)wstr.out_data_stream.total_out, (dfuLong32)(wstr.out_data_stream.total_out/1024));
      else
        printf("none, patch is BAD, icmp=%u, total_in=%u, total_out=%u\n",
               icmp, wstr.in_data_stream.total_in, rstr.out_data_stream.total_out);
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
          ReadFile(hNew,szBuf,fNoBuffering ? TPAroundUpper(wstr.in_data_stream.avail_in,0x200) : wstr.in_data_stream.avail_in,&wstr.in_data_stream.avail_in,NULL);
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
         (DWORD)(dwNbTick /1000),(DWORD)dwNbTick, dwBegin, GetTickCount() );
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


    //rstr.out_data_stream.next_out = pszBufOut;
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
      printf("\nERROR NUMBER READ IN APPLY  out=%u, in=%u\n\n",
             wstr.out_data_stream.total_out, rstr.in_data_stream.total_in);

    if (size_total_reread != rstr.out_data_stream.total_out)
      printf("\nERROR NUMBER READ IN APPLY (bis)  out=%u, in=%u\n\n",
             wstr.out_data_stream.total_out, rstr.in_data_stream.total_in);

    {
      //int icmp = memcmp(pszBufOut, data_in, (dfuIntPtr)rstr.out_data_stream.total_out);

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

  fRet = GetFileFullContentReadBuffer(szwFileOld,&hfcrOld,&Orig,NULL) && (hNew!=NULL);

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

  CloseFileFullContentReadBuffer(hfcrOld);
  CloseHandle(hNew);

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
#include "../src/lib/engine/svfile/common/DfsMFile.h"

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


/*************************************************************************/





void TestPatchMain();
void TestBuildDfs();
BOOL TestRemix(dfuLong32 dfParam,dfwcharpc dffnOrig,dfwcharpc dffnDest,dfuLong32 dfnbVer,const dfuLong32 * pdfVerTab);

void Tstmain()
{
  DFSFILEINFOPARAMINTERNAL DfsFileParam;
  DFSFILEWRAP DfsFileWrap = NULL;
  dfuLong32 error = 0;

  DfsFileParam.sizeStruct = sizeof(DfsFileParam);


  printf("end\n");
}


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

        printf("identical size %8u: ",size);
        dfSizeCode=MakeDifStFileIdentical((dfvoidp)buf, sizeof(buf), size);
        printf("%3u bytes : ",dfSizeCode);
        for (j=0;j<dfSizeCode;j++)
            printf("%02x ",buf[j]);
        printf("\n");
    }
    return 0;
}

#include <windows.h>



int main(int argc, char *argv[])
{
int iRet=0;
//dfwcharp pCommandLine;
    SetErrorMode(SEM_NOALIGNMENTFAULTEXCEPT);   // FOR RISC and ia64
    DfsCrc32(0,(BYTE*)" ",1); // init CRC32

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

    if (argc>1)
        if (lstrcmpi(argv[1],"/crashtest")==0)
        {
            char* ptr=(char*)((DWORD_PTR)0xb1256622);
            printf("crash here");
            *ptr=4;
        }


    if (argc>1)
    {
        if (lstrcmpi(argv[1],"DoTestIdentical")==0)
        {
            return PerformTestIdentical(argc-2,&argv[2]);
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
        if ((lstrcmpi(argv[1],"DoTestPatchFileNoMapNewBufOld")==0) || (lstrcmpi(argv[1],"DTPFNMNBO")==0))
        {
            dfuLong32 CompressRatio = COMPRESSIONPARAM_AUTOVALUE;
            if (argc>4)
                CompressRatio=atol(argv[4]);
            return (TestPatchFileNoMapNew(argv[2],argv[3],CompressRatio,FALSE,TRUE) ? 0 : 1);
        }

        if ((lstrcmpi(argv[1],"DoTestPatchFileNoMapNew")==0) || (lstrcmpi(argv[1],"DTPFNMN")==0))
        {
            dfuLong32 CompressRatio = COMPRESSIONPARAM_AUTOVALUE;
            if (argc>4)
                CompressRatio=atol(argv[4]);
            return (TestPatchFileNoMapNew(argv[2],argv[3],CompressRatio,TRUE,TRUE) ? 0 : 1);
        }

        if ((lstrcmpi(argv[1],"DoTestPatchFileNoMapNewBufOldNoVirtualAlloc")==0) || (lstrcmpi(argv[1],"DTPFNMNBONVA")==0))
        {
            dfuLong32 CompressRatio = COMPRESSIONPARAM_AUTOVALUE;
            if (argc>4)
                CompressRatio=atol(argv[4]);
            return (TestPatchFileNoMapNew(argv[2],argv[3],CompressRatio,FALSE,FALSE) ? 0 : 1);
        }

        if ((lstrcmpi(argv[1],"DoTestPatchFileNoMapNewNoVirtualAlloc")==0) || (lstrcmpi(argv[1],"DTPFNMNNVA")==0))
        {
            dfuLong32 CompressRatio = COMPRESSIONPARAM_AUTOVALUE;
            if (argc>4)
                CompressRatio=atol(argv[4]);
            return (TestPatchFileNoMapNew(argv[2],argv[3],CompressRatio,TRUE,FALSE) ? 0 : 1);
        }

        if (lstrcmpi(argv[1],"DoTestPatchFile")==0)
        {
            dfuLong32 CompressRatio = COMPRESSIONPARAM_AUTOVALUE;
            if (argc>4)
                CompressRatio=atol(argv[4]);
            return (TestPatchFile(argv[2],argv[3],CompressRatio) ? 0 : 1);
        }
    }


    return iRet;
}
#endif

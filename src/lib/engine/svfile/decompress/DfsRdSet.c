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

/* DfsWrSet.c */
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "../../patchstream/common/difbasic.h"
#include "../common/DfsMFile.h"
#include "../../patchstream/common/DfsTlTyp.h"
#include "../../patchstream/common/difstool.h"
#include "../common/DfsStruc.h"
#include "../common/DfsTagDf.h"
#include "../common/DfsTagMg.h"
#include "../common/DfsTagBlockFloatEnd.h"

#include "../common/DfsIntf.h"
#include "../common/DfsIntfL.h"

#include "../../patchstream/common/DfsOrigMemoryMap.h"
#include "../../patchstream/rebuild/RamDifWk.h"
#include "../../patchstream/rebuild/RamDifTl.h"
#include "../../patchstream/rebuild/RamDifWS.h"

#include "../../patchstream/common/DfsIoHlp.h"
#include "../common/DfsSet.h"
#include "../common/DfsSetTl.h"
#include "DfsRdSet.h"


#include "../../patchstream/common/difstrm.h"
#include "../../patchstream/decompress/apldifst.h"


#include "zlib.h"
#include "../../patchstream/common/compress_store.h"
#include "../../../hash/svf_md5.h"
#include "../../../hash/svf_sha.h"
#include "../../../hash/svf_sha256.h"

#include "../../patchstream/common/abstractDecompress.h"

#ifdef _DEBUG
//#include <windows. h>
//#define OUTTEXT
#endif
/* to to : add a callback for progress */

#define SIZE_BUFFER (0x8000*1*4)


dfuLong32 GetDirectoryInfo(DFSFILE DfsFile, dfuLong32 dfNumDir,
                         dfuLong32 * pdfTypedir,
                         DFTAGLIST * pTagListDirCopy, dfuLong32 * pdfGetNbFile,
                         DFTAGLIST ** ppsTagListFileInDirCopy, H_ERROR_INFO * pei)
{
  dfuLong32 dfError = DFS_SUCCESS;
  if (dfNumDir != DF_NUMDIR_NOGOTO)
    dfError = DfsGotoDir(DfsFile, dfNumDir,pei);
  if (dfError == DFS_SUCCESS)
    dfError =
      DfsRGetNextDir(DfsFile, pdfTypedir, pTagListDirCopy, pdfGetNbFile,
                     ppsTagListFileInDirCopy,pei);
  return dfError;
}

#define BUFFER_READ_SIZE (0x8000*1*2)
#define OPTIMAL_BLOCK_SIZE_FOR_MANUAL_DIRTY (0x4000)

dfuLong32 CompareFileWithSizeAndCrcInfo(dfwcharpc filename,
                                      const DFSCRCINFOPARAM *
                                      pDfsCrcInfoParam, dfuLong32 dfNbCrcParam,
                                      BOOL * pdfIsGoodCrc, dfuLong64 dfFileSize,
                                      tProgressCallBack pProgressCallBack,
                                      PROGRESSCALLBACKINFO *
                                      pProgressCallBackInfo,
                                      dfvoidp dfDataEventPtr,
                                      H_ERROR_INFO* pei)
{
  dfuLong32 dfError = DFS_SUCCESS;
  LOWLEVELFILE llfr;
  dfuLong32 dfBufSize;
  dfuLong32 dfSizeRead = 0;
  dfvoidp BufPtr;
  dfuLong64 dfAllRead = 0;
  dfuLong64 dfSizeReadingFile = 0;
  dfuLong32 dfCrc32 = 0;
  BOOL dfIsGoodCrc = TRUE;
  BOOL fComputeMD5 = TRUE;
  BOOL fComputeSHA1 = TRUE;
  BOOL fComputeSHA256 = TRUE;
  SVF_MD5_INTERNAL md5_ctx;
  SVF_SHA1_INTERNAL sha1_ctx;
  SVF_SHA256_INTERNAL sha256_ctx;
  dfbyte bMd5[16];
  dfbyte bSha1[20];
  dfbyte bSha256[32];


  dfBufSize = BUFFER_READ_SIZE;
  BufPtr = DfsMalloc(dfBufSize);
  if (BufPtr == NULL)
    return DFS_ERROR_MEMORY_ERROR;

  llfr = OpenLowLevel(filename, OPEN_READ, FALSE,FALSE,0, pei);
  if (llfr == NULL)
  {
    DfsFree(BufPtr);
    return DFS_ERROR_ERRORIO;
  }

  LowLevelGetSize64(llfr,&dfSizeReadingFile);

  if (dfSizeReadingFile!=dfFileSize)
  {
      if (pdfIsGoodCrc != NULL)
          *pdfIsGoodCrc = 0;
      DfsFree(BufPtr);
      LowLevelClose(llfr, pei);

      pProgressCallBackInfo->dfFileOrigDone += dfFileSize;
      pProgressCallBackInfo->dfDirOrigDone += dfFileSize;
      return dfError;
  }

  if (pDfsCrcInfoParam!=NULL)
  {
    if (!pDfsCrcInfoParam->fMd5)
        fComputeMD5=FALSE;
    if (!pDfsCrcInfoParam->fSha1)
        fComputeSHA1=FALSE;
    if (!pDfsCrcInfoParam->fSha256)
        fComputeSHA256 = FALSE;
  }

  if (fComputeMD5)
    svf_md5_init(&md5_ctx);
  if (fComputeSHA1)
    svf_sha1_init(&sha1_ctx);
  if (fComputeSHA256)
      svf_sha256_init(&sha256_ctx);

/* todo : do real CRC */
  do
  {
    dfuLong32 dfSizeToRead;
    if (!CallCallBack
        (pProgressCallBack, pProgressCallBackInfo,
         DFCBM_PROGRESSWORKINGFILE, NULL))
      dfError = DFS_STOP_REQUESTED;

    dfSizeToRead = dfBufSize;
    if (dfAllRead+dfSizeToRead>dfFileSize)
        dfSizeToRead = (dfuLong32)(dfFileSize-dfAllRead);

    if (dfSizeToRead>0)
    {
#ifdef _DEBUG
      dfuLong32 dfLow,dfHigh;
      LowLevelTell(llfr,&dfLow,&dfHigh);
      if (dfLow!=dfAllRead)
          printf("error\n");
#endif
      dfSizeRead = LowLevelRead(llfr, BufPtr, dfSizeToRead, pei);
    }
    else
      dfSizeRead = 0;

    dfCrc32 = (dfuLong32)crc32(dfCrc32, (((dfbytep)BufPtr)), dfSizeRead);
    if (fComputeMD5)
      svf_md5_append(&md5_ctx, (((dfbytep)BufPtr)), dfSizeRead);
    if (fComputeSHA1)
      svf_sha1_append(&sha1_ctx, (((dfbytep)BufPtr)), dfSizeRead);
    if (fComputeSHA256)
        svf_sha256_append(&sha256_ctx, (((dfbytep)BufPtr)), dfSizeRead);
    dfAllRead += dfSizeRead;

    pProgressCallBackInfo->dfFileOrigDone += dfSizeRead;
    pProgressCallBackInfo->dfDirOrigDone += dfSizeRead;
  }
  while ((dfSizeRead > 0) && (dfError == DFS_SUCCESS));

  LowLevelClose(llfr, pei);

  DfsFree(BufPtr);
  if (fComputeMD5)
      svf_md5_finish(&md5_ctx,bMd5);
  if (fComputeSHA1)
      svf_sha1_finish(&sha1_ctx,bSha1);
  if (fComputeSHA256)
      svf_sha256_finish(&sha256_ctx, bSha256);

  if ((pDfsCrcInfoParam->dfBeginPos == 0) &&
      (pDfsCrcInfoParam->dfEndPos == dfFileSize))
  {
    dfIsGoodCrc = (pDfsCrcInfoParam->dfCrc32Value == dfCrc32);
#ifdef _DEBUG
    if (!dfIsGoodCrc)
        printf("crc compute = %08x, crc mem=%08x\n",dfCrc32,pDfsCrcInfoParam->dfCrc32Value);
#endif

    if (dfIsGoodCrc && fComputeMD5 && pDfsCrcInfoParam->fMd5)
        dfIsGoodCrc = DfsMemcmp(pDfsCrcInfoParam->bMd5,bMd5,16)==0;
    if (dfIsGoodCrc && fComputeSHA1 && pDfsCrcInfoParam->fSha1)
        dfIsGoodCrc = DfsMemcmp(pDfsCrcInfoParam->bSha1,bSha1,20)==0;
    if (dfIsGoodCrc && fComputeSHA256 && pDfsCrcInfoParam->fSha256)
        dfIsGoodCrc = DfsMemcmp(pDfsCrcInfoParam->bSha1, bSha256, 32) == 0;
  }

  // need compare MD5 MD5++
  if ((dfError == DFS_SUCCESS) && (pdfIsGoodCrc != NULL))
    *pdfIsGoodCrc = dfIsGoodCrc;

  /*
     if ((dfCrc32 != pDfsCrcInfoParam->dfCrcValue) ||
     (
   */
#ifdef _DEBUG
  if ((dfIsGoodCrc) && (dfSizeReadingFile != dfFileSize))
      printf("pathological match\n");
#endif
  return dfError;
}

dfuLong32 SVFAPI ReadDirectoryInfo(DFSFILE DfsFile, dfuLong32 dfNumDir,
                            PDIRINFO* ppDirInfo,
                            tProgressCallBack pProgressCallBack,
                            dfvoidp dfUserPtr,
                            H_ERROR_INFO * pei)
{
  dfuLong32 dfError = DFS_SUCCESS;
  dfuLong32 dfTypeDir = 0;
  dfuLong32 dfNbFile = 0;
  dfuLong32 dfBlockType = 0;
  DFTAGLIST TagListDirCopy = NULL;
  DFTAGLIST *psTagListFileInDirCopy = NULL;
  dfuLong32 i = 0;
  PROGRESSCALLBACKINFO ProgressCallBackInfo;
  PDIRINFO pDirInfo = NULL;

  *ppDirInfo=NULL;

  InitProgressCallBackInfo(&ProgressCallBackInfo, DfsFile, dfUserPtr);

  ProgressCallBackInfo.dfDirType = 0;
  ProgressCallBackInfo.dfDirOrigDone = 0;
  ProgressCallBackInfo.dfDirEncodedDone = 0;
  ProgressCallBackInfo.dfDirOrigSize = 0;
  ProgressCallBackInfo.dfDirEncodedSize = 0;

  if (dfError == DFS_SUCCESS)
    if (!CallCallBack
        (pProgressCallBack, &ProgressCallBackInfo, DFCBM_BEFOREOPENDIRECTORY,
         NULL))
      dfError = DFS_STOP_REQUESTED;


  if ((dfNumDir != DF_NUMDIR_NOGOTO) && (dfError == DFS_SUCCESS))
    dfError = DfsGotoDir(DfsFile, dfNumDir, pei);

  if (dfError == DFS_SUCCESS)
    dfError = DfsRGetNextBlockType(DfsFile, &dfBlockType, pei);

  if ((dfError == DFS_SUCCESS) && (dfBlockType != BLOCKTYPE_DIR))
    dfError = DFS_ERROR_BAD_PARAMETER;

  if (dfError == DFS_SUCCESS)
    dfError =
      DfsRGetNextDir(DfsFile, &dfTypeDir, &TagListDirCopy, &dfNbFile,
                     &psTagListFileInDirCopy, pei);

  ProgressCallBackInfo.dfDirType = dfTypeDir;

  if (dfError == DFS_SUCCESS)
    if (!CallCallBack
        (pProgressCallBack, &ProgressCallBackInfo, DFCBM_AFTEROPENDIRECTORY,
         NULL))
      dfError = DFS_STOP_REQUESTED;

  if (dfError == DFS_SUCCESS)
  {
      pDirInfo = (PDIRINFO)DfsMalloc(sizeof(DIRINFO));
      pDirInfo->dfNbFile = dfNbFile;
      pDirInfo->dfNumDir=dfNumDir;
      pDirInfo->pFileInDirInfo = (FILEINDIRINFO*)DfsMalloc(sizeof(FILEINDIRINFO)*(dfNbFile+1));
      pDirInfo->TagDir = TagListDirCopy;
      pDirInfo->TagFile = psTagListFileInDirCopy;
      pDirInfo->dfTypeDir = dfTypeDir;
      *ppDirInfo=pDirInfo;
  }

  while ((i < dfNbFile) && (dfError == DFS_SUCCESS))
  {
    DFSCRCINFOPARAM *pDfsCrcInfoParam = NULL;
    dfvoidp TagBufProperties, TagBufCrc, TagBufFileName,TagBufDate;
    dfuLong32 TagSizeProperties, TagSizeCrc,TagSizeFileName,TagSizeDate;
    DFTAGLIST TagListCurrentFileInDir = *(psTagListFileInDirCopy + i);
    FILEINDIRINFO* pFileInfo = pDirInfo->pFileInDirInfo +i;

    pFileInfo->FileName = NULL;
    pFileInfo->dfSize = VALUE_UNKNOWN;
    pFileInfo->dfCrc32 = VALUE_UNKNOWN;
    pFileInfo->fMd5Filled = FALSE;
    pFileInfo->fSha1Filled = FALSE;
    pFileInfo->fSha256Filled = FALSE;
    pFileInfo->fCrc32Filled = FALSE;
    pFileInfo->fDateFilled = FALSE;
    pFileInfo->dfFileEncodedSize = VALUE_UNKNOWN;
    if ((GetTag
         (TagListCurrentFileInDir, DFSTAG_FILENAME, &TagBufFileName, &TagSizeFileName)))
    {
        dfwcharp FileName = (dfwcharp)DfsMalloc(TagSizeFileName+sizeof(dfwchar));
        DfsMemcpy(FileName,TagBufFileName,TagSizeFileName);

        *(FileName + ((TagSizeFileName+1)/sizeof(dfwchar)))=0;
        pFileInfo->FileName = FileName ;
    }

    if ((GetTag
         (TagListCurrentFileInDir, DFSTAG_DATE, &TagBufDate, &TagSizeDate)))
         if (TagSizeDate>=sizeof(DFSINFODATE))
    {
        ConvertDfsInfoDateToDfsTm((DFSINFODATE *) TagBufDate, &pFileInfo->dfsTm);
        pFileInfo->fDateFilled = TRUE ;
    }

    if ((GetTag
         (TagListCurrentFileInDir, DFSTAG_CRCINFO, &TagBufCrc, &TagSizeCrc))
        &&
        (GetTag
         (TagListCurrentFileInDir, DFSTAG_FILEPOSPROPERTIES,
          &TagBufProperties, &TagSizeProperties)) &&
        (!ProgressCallBackInfo.fWillIgnoreFile))
      /* #define DFSTAG_FILEPOSPROPERTIES    (0x00000005) */

    {
      const DFSCRCINFO_FULLSIZESTRUCTURE *pDfsCrcInfo = (DFSCRCINFO_FULLSIZESTRUCTURE *) TagBufCrc;
      dfuLong32 dfNbCrcInfo = GetNbCrcInfo(pDfsCrcInfo,TagSizeCrc);
      DFSFILEPOSPROPERTIESINFO DfsFilePropertiesInfo;
      /*
      const DFSFILEPOSPROPERTIES *pDfsFileProp =
        (DFSFILEPOSPROPERTIES *) TagBufProperties;*/
      //dfuLong64 dfFileSizeUncompressed;
      ConvertDfsFileProperties(&DfsFilePropertiesInfo,TagBufProperties,TagSizeProperties);

      pFileInfo->dfSize = DfsFilePropertiesInfo.dfFileSizeContentUncompressed;

      pFileInfo->dfFileEncodedSize = DfsFilePropertiesInfo.dfFileEncodedSizeWithoutPreAndPostHeader;

      pDfsCrcInfoParam =
        (DFSCRCINFOPARAM *) DfsMalloc(sizeof(DFSCRCINFOPARAM) *
                                      (dfNbCrcInfo + 1));

      if (pDfsCrcInfoParam != NULL)
      {
        dfuLong32 j;
        dfuLong32 dfPosInCrc=0;
        //BOOL fIsGoodCrc = FALSE;
        for (j = 0; j < dfNbCrcInfo; j++)
        {
          DFSCRCINFO_FULLSIZESTRUCTURE* pDfsCrcInfoWithMd5cur;
          pDfsCrcInfoWithMd5cur = (DFSCRCINFO_FULLSIZESTRUCTURE*)(((dfbytep)(pDfsCrcInfo))+dfPosInCrc);
          ConvertDfsCrcInfoToDfsCrcInfoParam(pDfsCrcInfoWithMd5cur,
                                             pDfsCrcInfoParam + j);

          if (((pDfsCrcInfoParam+j)->dfBeginPos == 0) &&
              ((pDfsCrcInfoParam+j)->dfEndPos == pFileInfo->dfSize))
          {
            pFileInfo->dfCrc32 = (pDfsCrcInfoParam+j)->dfCrc32Value ;
            pFileInfo->fCrc32Filled=TRUE;

            pFileInfo->fSha1Filled = (pDfsCrcInfoParam+j)->fSha1;
            if (pFileInfo->fSha1Filled)
                DfsMemcpy(pFileInfo->bSha1,(pDfsCrcInfoParam+j)->bSha1,20);

            pFileInfo->fSha256Filled = (pDfsCrcInfoParam + j)->fSha256;
            if (pFileInfo->fSha256Filled)
                DfsMemcpy(pFileInfo->bSha256, (pDfsCrcInfoParam + j)->bSha256, 32);

            pFileInfo->fMd5Filled = (pDfsCrcInfoParam+j)->fMd5;
            if (pFileInfo->fMd5Filled)
                DfsMemcpy(pFileInfo->bMd5,(pDfsCrcInfoParam+j)->bMd5,16);
          }
          dfPosInCrc += GetCrcInfoSize(pDfsCrcInfoWithMd5cur);
        }
        DfsFree(pDfsCrcInfoParam);
      }
    }
    i++;
  }


//  DfsRFreeAllDirTag(TagListDirCopy, psTagListFileInDirCopy);

  return dfError;
}


void SVFAPI FixIndenticalDifferentSizeInReadDirectoryInfo(const DIRINFO* pDirInfoPrev,PDIRINFO pDirInfoCur)
{
    dfuLong32 i;
    dfvoidp TagBuf;
    dfuLong32 TagSize;
//return;
    for (i=0;i<pDirInfoCur->dfNbFile;i++)
    {
        FILEINDIRINFO* pFileInDirInfoCur = (pDirInfoCur->pFileInDirInfo+i);


        if (GetTag(*(pDirInfoCur->TagFile + i), DFSTAG_STORAGESTATUS, &TagBuf, &TagSize))
        {
            if (TagSize==sizeof(dfuLong32Intel))
            {
                dfuLong32 dfFileIdentical = ConvertuLongIntelToLong(*(dfuLong32Intel*)TagBuf);

#ifdef _DEBUG
                if ((pFileInDirInfoCur->dfFileEncodedSize==0) && (dfFileIdentical!=DFS_STORAGESTATUS_IDENTICAL))
                {
                    DispMessageForDebugger( L"true patch no present");
                }
#endif

                if (dfFileIdentical==DFS_STORAGESTATUS_IDENTICAL)
                    if (GetTag
                        (*(pDirInfoCur->TagFile + i), DFSTAG_PREVIOUSVERSIONINFO, &TagBuf,
                        &TagSize))
                            if (TagSize >= sizeof(DFSPREVIOUSVERSIONINFO))
                {
                    dfuLong32 dfPreviousVersionFileNumber, dfPreviousVersionFilePosition;
                    const DFSPREVIOUSVERSIONINFO *pDfsPreviousVersionInfo =
                    (const DFSPREVIOUSVERSIONINFO *) TagBuf;
                    dfPreviousVersionFileNumber =
                        ConvertuLongIntelToLong(pDfsPreviousVersionInfo->
                                                dfPreviousVersionFileNumber);
                    dfPreviousVersionFilePosition =
                        ConvertuLongIntelToLong(pDfsPreviousVersionInfo->
                                                dfPreviousVersionFilePosition);
                    if (dfPreviousVersionFileNumber == 1)
                    {
                        FILEINDIRINFO* pFileInDirInfoPrev = (pDirInfoPrev->pFileInDirInfo+dfPreviousVersionFilePosition);
#ifdef _DEBUG
                        if ((pFileInDirInfoCur->dfSize != pFileInDirInfoPrev->dfSize))
                        {
                            DispMessageForDebugger(L"pb");
                        }
#endif

                        if ((pFileInDirInfoCur->dfSize != pFileInDirInfoPrev->dfSize)/* && (pFileInDirInfoCur->dfFileEncodedSize>0)*/)
                        //if ((pFileInDirInfoCur->dfSize != pFileInDirInfoPrev->dfSize))
                        {
                            //OutputDebugString("mensonge\n");
                            AddTaguLong(*(pDirInfoCur->TagFile + i), DFSTAG_STORAGESTATUS, DFS_STORAGESTATUS_MODIFIED);
#ifdef _DEBUG
                            {
                                if (GetTag
                                    (*(pDirInfoCur->TagFile + i), DFSTAG_STORAGESTATUS, &TagBuf,
                                    &TagSize))
                                {
                                    DispMessageForDebugger(L"fix");
                                }
                            }
#endif
                        } // now fix MD5
                        else
                        {
#ifdef _DEBUG
                            if (pFileInDirInfoCur->dfCrc32 != pFileInDirInfoPrev->dfCrc32)
                                DispMessageForUser(L"error",L"Scandal");
#endif
                            if ((pFileInDirInfoCur->dfCrc32 == pFileInDirInfoPrev->dfCrc32) &&
                                (pFileInDirInfoPrev->fMd5Filled) && (!pFileInDirInfoCur->fMd5Filled))
                            {
//#ifndef _DEBUG
                                pFileInDirInfoCur->fMd5Filled = pFileInDirInfoPrev->fMd5Filled;
                                DfsMemcpy(pFileInDirInfoCur->bMd5,pFileInDirInfoPrev->bMd5,16);
//#endif
                            }

                            if ((pFileInDirInfoCur->dfCrc32 == pFileInDirInfoPrev->dfCrc32) &&
                                (pFileInDirInfoPrev->fSha1Filled) && (!pFileInDirInfoCur->fSha1Filled))
                            {
                                //#ifndef _DEBUG
                                pFileInDirInfoCur->fSha1Filled = pFileInDirInfoPrev->fSha1Filled;
                                DfsMemcpy(pFileInDirInfoCur->bSha1, pFileInDirInfoPrev->bSha1, 20);
                                //#endif
                            }

                            if ((pFileInDirInfoCur->dfCrc32 == pFileInDirInfoPrev->dfCrc32) &&
                                (pFileInDirInfoPrev->fSha256Filled) && (!pFileInDirInfoCur->fSha256Filled))
                            {
//#ifndef _DEBUG
                                pFileInDirInfoCur->fSha256Filled = pFileInDirInfoPrev->fSha256Filled;
                                DfsMemcpy(pFileInDirInfoCur->bSha256,pFileInDirInfoPrev->bSha256,32);
//#endif
                            }
                            /*
                            if ((GetTag
                                 (*(pDirInfoCur->TagFile + i), DFSTAG_CRCINFO, &TagBufCrcNew,&TagSizeCrcNew)) &&
                                (GetTag
                                 (*(pDirInfoPrev->TagFile + dfPreviousVersionFilePosition), DFSTAG_CRCINFO, &TagBufCrcOld,&TagSizeCrcOld)))
                                   if ((TagSizeCrcOld>DFSCRCINFO_SIZE_MINIMAL) && (TagSizeCrcNew==DFSCRCINFO_SIZE_MINIMAL))
                                   {
                                       AddTag(*(pDirInfoCur->TagFile + i), DFSTAG_CRCINFO, TagBufCrcOld, TagSizeCrcOld);
                                   }
                                   */
                        }
                    }
                }
            }
        }
    }
}


PDIRINFO* SVFAPI ReadAllDirInfo(DFSFILE DfsFile,dfuLong32 *pdfNbDir,dfuLong32 dfLimitReadDir,dfuLong32 *pdfError)
{
    PDIRINFO *pDirInfo;
    dfuLong32 dfNbDir;
    dfuLong32 dfError = DFS_SUCCESS;

    if (DfsFile==NULL)
    {
        if (pdfError != NULL)
            *pdfError = DFS_ERROR_BAD_PARAMETER;

        if (pdfNbDir != NULL)
            *pdfNbDir = 0;
        return NULL;
    }

    DfsGetNbDir(DfsFile, &dfNbDir, NULL);
    if (dfLimitReadDir!=READ_ALL_DIR)
        if (dfLimitReadDir<dfNbDir)
            dfNbDir = dfLimitReadDir;
    pDirInfo = (PDIRINFO *) DfsMalloc(sizeof(PDIRINFO) * (dfNbDir + 1));
    if (pDirInfo != NULL)
    {
        dfuLong32 dfNumDir ;
        BOOL fSuccess = TRUE;
        for (dfNumDir = 0; dfNumDir < dfNbDir; dfNumDir++)
            *((pDirInfo) + dfNumDir)=NULL;
        for (dfNumDir = 0; dfNumDir < dfNbDir; dfNumDir++)
        {
            dfError = ReadDirectoryInfo(DfsFile, dfNumDir, (pDirInfo) + dfNumDir, NULL, NULL, NULL);

            if (dfNumDir > 0)
            {
                PDIRINFO pDirInfoPrev=*((pDirInfo)+dfNumDir-1);
                PDIRINFO pDirInfoCur=*((pDirInfo)+dfNumDir);

                FixIndenticalDifferentSizeInReadDirectoryInfo(pDirInfoPrev,pDirInfoCur);
            }

            if (dfError != DFS_SUCCESS)
            {
                fSuccess = FALSE;
                break;
            }
        }

        if (!fSuccess)
        {
            for (dfNumDir = 0; dfNumDir < dfNbDir; dfNumDir++)
                FreeDirectoryInfo((pDirInfo) + dfNumDir, NULL);

            DfsFree(pDirInfo);
            pDirInfo = NULL;
        }
    }

    if (pdfError != NULL)
        *pdfError = dfError;

    if (pdfNbDir != NULL)
        *pdfNbDir = dfNbDir;

    return pDirInfo;
}

void SVFAPI FreeAllDirInfo(PDIRINFO* pDirInfo,dfuLong32 dfNbDir)
{
    dfuLong32 i;
    if (pDirInfo==NULL)
        return;
    for (i=0;i<dfNbDir;i++)
    {
        FreeDirectoryInfo(pDirInfo+i,NULL);
    }
    DfsFree(pDirInfo);
}

void SVFAPI AnalyseDfsFeature(const PCDIRINFO* pDirInfo,dfuLong32 dfNbDir,ANALYSE_DFS_FEATURE_USED* pAnalyseDfsFeature)
{
    dfuLong32 i,j;
    if (pAnalyseDfsFeature==NULL)
        return;
    if (pDirInfo==NULL)
        return ;
    pAnalyseDfsFeature->fMd5NoPresentFound = pAnalyseDfsFeature->fMd5PresentFound = FALSE;
    pAnalyseDfsFeature->fSha1NoPresentFound = pAnalyseDfsFeature->fSha1PresentFound = FALSE;
    pAnalyseDfsFeature->fSha256NoPresentFound = pAnalyseDfsFeature->fSha256PresentFound = FALSE;
    pAnalyseDfsFeature->fNoStripIdenticalFound = pAnalyseDfsFeature->fStripIdenticalFound = FALSE;

    for (i=0;i<dfNbDir;i++)
    {
        const PCDIRINFO pCurDirInfo=*(pDirInfo+i);
        for (j=0;j<pCurDirInfo->dfNbFile;j++)
        {
            const FILEINDIRINFO* pFileInDirInfo = (pCurDirInfo->pFileInDirInfo+j);
            BOOL fIdentical = FALSE;
            dfvoidp TagBuf;
            dfuLong32 TagSize;

            if (GetTag(*(pCurDirInfo->TagFile + j), DFSTAG_STORAGESTATUS, &TagBuf, &TagSize))
                if (TagSize==sizeof(dfuLong32Intel))
                {
                    dfuLong32 dfFileIdentical = ConvertuLongIntelToLong(*(dfuLong32Intel*)TagBuf);

                    if (dfFileIdentical==DFS_STORAGESTATUS_IDENTICAL)
                        fIdentical=TRUE;
                }

            if (fIdentical)
            {
                if (i>0)
                {
                    if ((pFileInDirInfo->dfFileEncodedSize) == 0)
                        pAnalyseDfsFeature->fStripIdenticalFound = TRUE;
                    else
                        pAnalyseDfsFeature->fNoStripIdenticalFound = TRUE;
                }
            }
            else
            {
                    if (pFileInDirInfo->fMd5Filled)
                        pAnalyseDfsFeature->fMd5PresentFound = TRUE;
                    else
                        pAnalyseDfsFeature->fMd5NoPresentFound = TRUE;

                    if (pFileInDirInfo->fSha1Filled)
                        pAnalyseDfsFeature->fSha1PresentFound = TRUE;
                    else
                        pAnalyseDfsFeature->fSha1NoPresentFound = TRUE;

                    if (pFileInDirInfo->fSha256Filled)
                        pAnalyseDfsFeature->fSha256PresentFound = TRUE;
                    else
                        pAnalyseDfsFeature->fSha256NoPresentFound = TRUE;
            }
        }
    }
}

BOOL SVFAPI AdaptDfsFeature(const PCDIRINFO* pDirInfo,dfuLong32 dfNbDir,DFSFEATUREPARAM* pDfsFeatureParam)
{
    ANALYSE_DFS_FEATURE_USED AnalyseDfsFeatureUsed;
    DFSFEATUREPARAM DfsFeatureParamOld;

    if (pDfsFeatureParam==NULL)
        return FALSE;
    if (pDirInfo==NULL)
        return FALSE;

    DfsFeatureParamOld=*pDfsFeatureParam;
    AnalyseDfsFeature(pDirInfo,dfNbDir,&AnalyseDfsFeatureUsed);
    if (AnalyseDfsFeatureUsed.fStripIdenticalFound)
        pDfsFeatureParam->fStripIdenticalBody = TRUE;

    if (AnalyseDfsFeatureUsed.fMd5NoPresentFound != AnalyseDfsFeatureUsed.fMd5PresentFound)
        pDfsFeatureParam->fComputeMd5 = AnalyseDfsFeatureUsed.fMd5PresentFound;

    if (AnalyseDfsFeatureUsed.fSha1NoPresentFound != AnalyseDfsFeatureUsed.fSha1PresentFound)
        pDfsFeatureParam->fComputeSha1 = AnalyseDfsFeatureUsed.fSha1PresentFound;

    if (AnalyseDfsFeatureUsed.fSha256NoPresentFound != AnalyseDfsFeatureUsed.fSha256PresentFound)
        pDfsFeatureParam->fComputeSha256 = AnalyseDfsFeatureUsed.fSha256PresentFound;

    return ((pDfsFeatureParam->fStripIdenticalBody != DfsFeatureParamOld.fStripIdenticalBody) ||
            (pDfsFeatureParam->fComputeMd5 != DfsFeatureParamOld.fComputeMd5) ||
            (pDfsFeatureParam->fComputeSha256 != DfsFeatureParamOld.fComputeSha256) ||
            (pDfsFeatureParam->fComputeSha1 != DfsFeatureParamOld.fComputeSha1));
}

BOOL SVFAPI AdaptDfsFileFeature(DFSFILE DfsFile,const PCDIRINFO* pDirInfo,dfuLong32 dfNbDir)
{
    DFSFEATUREPARAM DfsFeatureParam;
    if (pDirInfo==NULL)
        return FALSE;
    GetDfsFeatureParam(DfsFile, &DfsFeatureParam);
    if (!AdaptDfsFeature(pDirInfo,dfNbDir,&DfsFeatureParam))
        return FALSE;
    SetDfsFeatureParam(DfsFile, &DfsFeatureParam);
    return TRUE;
}

BOOL SVFAPI ConvertOldDirectoryCommentStorage(DFSFILE DfsFile, H_ERROR_INFO * pei)
{
  DFTAGBLOCKFLOAT TagBlockFloat;
  dfuLong32 dfValue=0;
  dfuLong32 dfError=DFS_SUCCESS;
  BOOL fNeedParseDirectoryForOldComment=FALSE; /* MUST CHECK REALLY */

  TagBlockFloat = GetDfsTagBlockFloat(DfsFile, pei);
  if (TagBlockFloat != NULL)
      if (GetTaguLongBlockFloat(TagBlockFloat,FLOATNUM_NOSPECIFIC,FLOATNUM_NOSPECIFIC,DFSTAG_NOFLOATFILLED,&dfValue))
          if (dfValue == 1)
              fNeedParseDirectoryForOldComment=TRUE;


  if ((fNeedParseDirectoryForOldComment) && (dfError == DFS_SUCCESS))
  {
      dfuLong32 dfNumDir;
      dfuLong32 dfNbDir = 0;

      RemoveTagBlockFloat(TagBlockFloat,FLOATNUM_NOSPECIFIC,FLOATNUM_NOSPECIFIC,DFSTAG_NOFLOATFILLED);
      DfsGetNbDir(DfsFile, &dfNbDir, pei);


      for (dfNumDir = 0; dfNumDir < dfNbDir; dfNumDir++)
      {
        PDIRINFO pDirInfo = NULL;

        dfError = ReadDirectoryInfo(DfsFile, dfNumDir, &pDirInfo, NULL, NULL, pei);
        if (pDirInfo != NULL)
        {
          dfvoidp TagBuf;
          dfuLong32 TagSize;
          if (GetTag(pDirInfo->TagDir, DFSTAG_DIR_NAME, &TagBuf, &TagSize))
          {
              dfwcharp pDirName;
              pDirName = (dfwcharp)TagBuf;
              {
                  #if ((defined(_DEBUG)) && (defined(OUTTEXT)))
                  char sz[2048];
                  wsprintf(sz,"dir %u comment '%ws'\n",dfNumDir,pDirName);
                  OutputDebugString(sz);
                  #endif
                  AddTagBlockFloat(TagBlockFloat,dfNumDir,FLOATNUM_NOSPECIFIC,DFSTAG_DIR_NAME_FLOAT,TagBuf, TagSize);
              }

          }
          FreeDirectoryInfo(&pDirInfo,NULL);
        }
      }
  }
  return fNeedParseDirectoryForOldComment;

}


void SVFAPI FreeDirectoryInfo(PDIRINFO* ppDirInfo, H_ERROR_INFO * pei)
{
    PDIRINFO pDirInfo;
    if (ppDirInfo==NULL)
        return;
    pDirInfo = *ppDirInfo;
    if (pDirInfo != NULL)
    {
        dfuLong32 i;
        DfsRFreeAllDirTag(pDirInfo->TagDir,pDirInfo->TagFile, pei);
        for (i=0;i<pDirInfo->dfNbFile;i++)
        {
            DfsFree((dfwcharp)(pDirInfo->pFileInDirInfo+i)->FileName);
        }
        DfsFree(pDirInfo->pFileInDirInfo);
        DfsFree(*ppDirInfo);
    }

    *ppDirInfo=NULL;
}

dfuLong32 SVFAPI CheckDirectoryCrcWithRealFileSet(DFSFILE DfsFile, dfuLong32 dfNumDir,
                                         dfuLong32 dfNbFile,
                                         FILETOCHECK * pFileToCheck,
                                         tProgressCallBack pProgressCallBack,
                                         dfvoidp dfUserPtr,H_ERROR_INFO* pei)
{
  dfuLong32 dfError = DFS_SUCCESS;
  dfuLong32 dfTypeDir = 0;
  dfuLong32 dfGetNbFile = 0;
  dfuLong32 dfBlockType = 0;
  DFTAGLIST TagListDirCopy = NULL;
  DFTAGLIST *psTagListFileInDirCopy = NULL;
  dfuLong32 i;
  PROGRESSCALLBACKINFO ProgressCallBackInfo;

  InitProgressCallBackInfo(&ProgressCallBackInfo, DfsFile, dfUserPtr);

  ProgressCallBackInfo.dfDirType = 0;
  ProgressCallBackInfo.dfDirOrigDone = 0;
  ProgressCallBackInfo.dfDirEncodedDone = 0;
  ProgressCallBackInfo.dfDirOrigSize = 0;
  ProgressCallBackInfo.dfDirEncodedSize = 0;

  if (dfError == DFS_SUCCESS)
    if (!CallCallBack
        (pProgressCallBack, &ProgressCallBackInfo, DFCBM_BEFOREOPENDIRECTORY,
         NULL))
      dfError = DFS_STOP_REQUESTED;


  if ((dfNumDir != DF_NUMDIR_NOGOTO) && (dfError == DFS_SUCCESS))
    dfError = DfsGotoDir(DfsFile, dfNumDir, pei);

  if (dfError == DFS_SUCCESS)
    dfError = DfsRGetNextBlockType(DfsFile, &dfBlockType, pei);

  if ((dfError == DFS_SUCCESS) && (dfBlockType != BLOCKTYPE_DIR))
    dfError = DFS_ERROR_BAD_PARAMETER;

  if (dfError == DFS_SUCCESS)
    dfError =
      DfsRGetNextDir(DfsFile, &dfTypeDir, &TagListDirCopy, &dfGetNbFile,
                     &psTagListFileInDirCopy, pei);

  if (dfGetNbFile != dfNbFile)
    dfError = DFS_ERROR_BAD_PARAMETER;

  ProgressCallBackInfo.dfDirType = dfTypeDir;

  if (dfError == DFS_SUCCESS)
    if (!CallCallBack
        (pProgressCallBack, &ProgressCallBackInfo, DFCBM_AFTEROPENDIRECTORY,
         NULL))
      dfError = DFS_STOP_REQUESTED;

  i = 0 ;
  while ((i < dfNbFile) && (dfError == DFS_SUCCESS))
  {
      dfvoidp TagBufProperties;
      dfuLong32 TagSizeProperties;
      DFTAGLIST TagListCurrentFileInDir = *(psTagListFileInDirCopy + i);

      if ((GetTag
         (TagListCurrentFileInDir, DFSTAG_FILEPOSPROPERTIES,
          &TagBufProperties, &TagSizeProperties)) &&
        (!ProgressCallBackInfo.fWillIgnoreFile))
      {
          DFSFILEPOSPROPERTIESINFO DfsFilePropertiesInfo;
          /*
            const DFSFILEPOSPROPERTIES *pDfsFileProp =
                (DFSFILEPOSPROPERTIES *) TagBufProperties;*/

          ConvertDfsFileProperties(&DfsFilePropertiesInfo,TagBufProperties,TagSizeProperties);


          ProgressCallBackInfo.dfDirOrigSize += DfsFilePropertiesInfo.dfFileSizeContentUncompressed;;
          ProgressCallBackInfo.dfDirEncodedSize += DfsFilePropertiesInfo.dfFileEncodedSizeWithoutPreAndPostHeader;
      }
      i++;
  }

  i = 0 ;
  while ((i < dfNbFile) && (dfError == DFS_SUCCESS))
  {
    DFSCRCINFOPARAM *pDfsCrcInfoParam = NULL;
    dfvoidp TagBufProperties, TagBufCrc;
    dfuLong32 TagSizeProperties, TagSizeCrc;
    DFTAGLIST TagListCurrentFileInDir = *(psTagListFileInDirCopy + i);

    ProgressCallBackInfo.filename_onwork =
      (pFileToCheck + i)->filename_ondisk_tocompare;
    ProgressCallBackInfo.filename_stored =
      (pFileToCheck + i)->filename_archive;

    ProgressCallBackInfo.fTemporaryFilename = FALSE;

    ProgressCallBackInfo.fWillIgnoreFile = (pFileToCheck + i)->fIgnore;
    ProgressCallBackInfo.dfFileOrigDone = 0;
    ProgressCallBackInfo.dfFileEncodedDone = 0;
    ProgressCallBackInfo.dfFileOrigSize = 0;
    ProgressCallBackInfo.dfFileEncodedSize = 0;
    ProgressCallBackInfo.dfFileNumber = i;

    if (!CallCallBack
        (pProgressCallBack, &ProgressCallBackInfo,
         DFCBM_BEFOREOPENWORKINGFILE, NULL))
      dfError = DFS_STOP_REQUESTED;

    if ((GetTag
         (TagListCurrentFileInDir, DFSTAG_CRCINFO, &TagBufCrc, &TagSizeCrc))
        &&
        (GetTag
         (TagListCurrentFileInDir, DFSTAG_FILEPOSPROPERTIES,
          &TagBufProperties, &TagSizeProperties)) &&
        (!ProgressCallBackInfo.fWillIgnoreFile))
      /* #define DFSTAG_FILEPOSPROPERTIES    (0x00000005) */

    {
      const DFSCRCINFO_FULLSIZESTRUCTURE *pDfsCrcInfo = (DFSCRCINFO_FULLSIZESTRUCTURE *) TagBufCrc;
      dfuLong64 dfFileSizeUncompressed;
      dfuLong32 dfNbCrcInfo = GetNbCrcInfo(pDfsCrcInfo,TagSizeCrc);

      DFSFILEPOSPROPERTIESINFO DfsFilePropertiesInfo;
      /*
        const DFSFILEPOSPROPERTIES *pDfsFileProp =
            (DFSFILEPOSPROPERTIES *) TagBufProperties;*/

      ConvertDfsFileProperties(&DfsFilePropertiesInfo,TagBufProperties,TagSizeProperties);



      dfFileSizeUncompressed = DfsFilePropertiesInfo.dfFileSizeContentUncompressed;

      ProgressCallBackInfo.dfFileOrigSize = dfFileSizeUncompressed ;

      ProgressCallBackInfo.dfFileEncodedSize = DfsFilePropertiesInfo.dfFileEncodedSizeWithoutPreAndPostHeader;

      pDfsCrcInfoParam =
        (DFSCRCINFOPARAM *) DfsMalloc(sizeof(DFSCRCINFOPARAM) *
                                      (dfNbCrcInfo + 1));

      if (pDfsCrcInfoParam != NULL)
      {
        dfuLong32 j;
        BOOL fIsGoodCrc = FALSE;
        for (j = 0; j < dfNbCrcInfo; j++)
          ConvertDfsCrcInfoToDfsCrcInfoParam(pDfsCrcInfo + j,
                                             pDfsCrcInfoParam + j);

#ifdef _DEBUG                   /*
                                   {
                                   dfvoidp TagBuf;
                                   dfuLong32 TagSize;
                                   if (GetTag
                                   (TagListCurrentFileInDir, DFSTAG_FILENAME, &TagBuf, &TagSize))
                                   printf("%u:compare %ws and %ws\n",i,ProgressCallBackInfo.filename_onwork,TagBuf);
                                   } */
#endif

        dfError =
          CompareFileWithSizeAndCrcInfo(ProgressCallBackInfo.filename_onwork,
                                        pDfsCrcInfoParam,
                                        dfNbCrcInfo,
                                        &fIsGoodCrc, dfFileSizeUncompressed,
                                        pProgressCallBack,
                                        &ProgressCallBackInfo, NULL,
                                        pei);

        if (dfError == DFS_SUCCESS)
          (pFileToCheck + i)->fIsIdentical = fIsGoodCrc;

        if (dfError == DFS_SUCCESS)
          if (!CallCallBack
              (pProgressCallBack, &ProgressCallBackInfo,
               DFCBM_AFTERCLOSINGWORKINGFILE, NULL))
            dfError = DFS_STOP_REQUESTED;

        DfsFree(pDfsCrcInfoParam);
      }
    }
    i++;
  }

  if (dfError == DFS_SUCCESS)
    if (!CallCallBack
        (pProgressCallBack, &ProgressCallBackInfo,
         DFCBM_AFTERCLOSINGDIRECTORY, NULL))
      dfError = DFS_STOP_REQUESTED;
  DfsRFreeAllDirTag(TagListDirCopy, psTagListFileInDirCopy, pei);

  return dfError;
}


void TryCreateDirForFileName(dfwcharpc filename_onwork_to_createdir,H_ERROR_INFO* pei)
{
    /* Try MkDir */
    dfuLong32 dfSizefilename_onwork;
    dfwcharp pdfwFilePathOnly,pdfwFileNameOnly;

    dfSizefilename_onwork = (dfUnicodeStrlen(filename_onwork_to_createdir)+8)*2;
    pdfwFileNameOnly = (dfwcharp)DfsMalloc(dfSizefilename_onwork * 2 *sizeof(dfwchar));
    pdfwFilePathOnly = pdfwFileNameOnly + dfSizefilename_onwork;
    if (SplitFileNameAndPath(filename_onwork_to_createdir,
                            pdfwFilePathOnly,dfSizefilename_onwork,
                            pdfwFileNameOnly,dfSizefilename_onwork,TRUE))
    {
        MyMkdir(pdfwFilePathOnly, pei);
    }
    DfsFree(pdfwFileNameOnly);
}

dfuLong32 SVFAPI ExtractDirectory(DFSFILE DfsFile, dfuLong32 dfNumDir,
                           PCDIRINFO pDirInfo,
                           dfuLong32 dfNbFile,
                           FILETOEXTRACT * pFileToExtract,
                           tProgressCallBack pProgressCallBack,
                           dfvoidp dfUserPtr,
                           tExtractingFileWorkingEvent pExtractingFileWorkingEvent,
                           dfvoidp dfUserPtrWorkingEvent,
                           BOOL fFlushWrite,
                           H_ERROR_INFO* pei){
  dfuLong32 dfError = DFS_SUCCESS;
  dfuLong32 dfTypeDir = 0;
  dfuLong32 dfGetNbFile = 0;
  //dfuLong32 dfBlockType = 0;
  DFTAGLIST TagListDirCopy = NULL;
  DFTAGLIST *psTagListFileInDirCopy = NULL;
  dfuLong32 i ;
  FILETOEXTRACT *pCurFileToExtract;
  dfuLong32 dfBufReadSize, dfBufWriteSize;
  dfbytep BufAlloc;
  dfbytep BufRead, BufWrite;

  PROGRESSCALLBACKINFO ProgressCallBackInfo;
  EXTR_WORK_EVENT_INFO ewei;


  InitProgressCallBackInfo(&ProgressCallBackInfo, DfsFile, dfUserPtr);




  ewei.dfNumVersion = dfNumDir;
  ewei.dfNumFile = 0;
  ewei.dfFileNameOnDisk = NULL;
  ewei.dfSuccess = 0;
  ewei.fTempFile = FALSE;


  dfBufWriteSize = SIZE_BUFFER;
  dfBufReadSize = SIZE_BUFFER;
  BufAlloc = (dfbytep) DfsMalloc(dfBufReadSize + dfBufWriteSize);
  if (BufAlloc == NULL)
    return DFS_ERROR_MEMORY_ERROR;
  BufRead = BufAlloc;
  BufWrite = BufAlloc + dfBufReadSize;


  ProgressCallBackInfo.dfDirType = 0;
  ProgressCallBackInfo.dfDirOrigDone = 0;
  ProgressCallBackInfo.dfDirEncodedDone = 0;
  ProgressCallBackInfo.dfDirOrigSize = 0;
  ProgressCallBackInfo.dfDirEncodedSize = 0;

  if (dfError == DFS_SUCCESS)
    if (!CallCallBack
        (pProgressCallBack, &ProgressCallBackInfo, DFCBM_BEFOREOPENDIRECTORY,
         NULL))
      dfError = DFS_STOP_REQUESTED;
/*
  if ((dfNumDir != DF_NUMDIR_NOGOTO) && (dfError == DFS_SUCCESS))
    dfError = DfsGotoDir(DfsFile, dfNumDir, pei);

  if (dfError == DFS_SUCCESS)
    dfError = DfsRGetNextBlockType(DfsFile, &dfBlockType, pei);

  if ((dfError == DFS_SUCCESS) && (dfBlockType != BLOCKTYPE_DIR))
    dfError = DFS_ERROR_BAD_PARAMETER;

  if (dfError == DFS_SUCCESS)
    dfError =
      DfsRGetNextDir(DfsFile, &dfTypeDir, &TagListDirCopy, &dfGetNbFile,
                     &psTagListFileInDirCopy, pei);
*/
  dfTypeDir = pDirInfo->dfTypeDir;
  TagListDirCopy = pDirInfo->TagDir;
  psTagListFileInDirCopy = pDirInfo->TagFile;
  dfGetNbFile = pDirInfo->dfNbFile;

  if (dfGetNbFile != dfNbFile)
    dfError = DFS_ERROR_BAD_PARAMETER;

  ProgressCallBackInfo.dfDirType = dfTypeDir;

  if (dfError == DFS_SUCCESS)
    if (!CallCallBack
        (pProgressCallBack, &ProgressCallBackInfo, DFCBM_AFTEROPENDIRECTORY,
         NULL))
      dfError = DFS_STOP_REQUESTED;



  i = 0 ;
  pCurFileToExtract = pFileToExtract;
  while ((i < dfNbFile) && (dfError == DFS_SUCCESS))
  {
      dfvoidp TagBufProperties;
      dfuLong32 TagSizeProperties;
      DFTAGLIST TagListCurrentFileInDir = *(psTagListFileInDirCopy + i);



      if ((GetTag
         (TagListCurrentFileInDir, DFSTAG_FILEPOSPROPERTIES,
          &TagBufProperties, &TagSizeProperties)) &&
        (!ProgressCallBackInfo.fWillIgnoreFile))
      {
          DFSFILEPOSPROPERTIESINFO DfsFilePropertiesInfo;
          /*
              const DFSFILEPOSPROPERTIES *pDfsFileProp =
                  (DFSFILEPOSPROPERTIES *) TagBufProperties;*/

          ConvertDfsFileProperties(&DfsFilePropertiesInfo,TagBufProperties,TagSizeProperties);


          if (!(pCurFileToExtract->fIgnore))
              if ((pCurFileToExtract->filename_ondisk_to_write==NULL) && (pCurFileToExtract->fTemporaryFile))
              {
                  dfwchar szTempFN[1024];
                  BOOL fBuildPatchfileTemp = TRUE;

                  dfuLong64 dfFileSizeOutProjected = 0;
                  /*
                      (pCurFileToExtract->fRawExtracting) ?
                             DfsFilePropertiesInfo.dfFileEncodedSizeWithoutPreAndPostHeader :
                             DfsFilePropertiesInfo.dfFileSizeContentUncompressed;
*/
                  if (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_EXTRACTRAWSTREAMPATCHFILE)
                  {
                      dfFileSizeOutProjected = DfsFilePropertiesInfo.dfFileEncodedSizeWithoutPreAndPostHeader;
                  }
                  else
                  if (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_APPLYSTREAMPATCH)
                  {
                      dfFileSizeOutProjected = DfsFilePropertiesInfo.dfFileSizeContentUncompressed;
                  }
                  else
                  if (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_COMBINERAMDIF_TORAMDIF)
                  {
                      dfFileSizeOutProjected = 0;
                      fBuildPatchfileTemp = FALSE;
                  }
                  else
                  if ((pCurFileToExtract->KindExtracting == KIND_EXTRACTING_COMBINERAMDIF_APPLY) ||
                      (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_COMBINERAMDIF_APPLY_INPLACE) ||
                      (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_COMBINERAMDIF_APPLY_INPLACE_NOCHECKSUM))
                  {
                      dfFileSizeOutProjected = DfsFilePropertiesInfo.dfFileSizeContentUncompressed;
                  }
#ifndef SVF_EXTRACT_ONLY
                  else
                  if (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_COMBINERAMDIF_BUILDSTREAMPATCHFILE)
                  {
                      dfFileSizeOutProjected = 0; /* unknown */
                  }
#endif

#if ((defined(_DEBUG)) && (defined(OUTTEXT)))
                  if (pCurFileToExtract->fRawExtracting)
                  {
                      OutputDebugString("\n*+\n");
                  }
#endif
                  if (fBuildPatchfileTemp)
                  {
                      GetTemporaryFilename(GetUnicodeSVFPrefix(),szTempFN,(sizeof(szTempFN) / sizeof(dfwchar))-1,TRUE,dfFileSizeOutProjected);

#if ((defined(_DEBUG)) && (defined(OUTTEXT)))
                      lstrcatW(szTempFN,(pCurFileToExtract->fRawExtracting) ? L"_Raw" : L"_noraw");
                      if (!(pCurFileToExtract)->fRawExtracting)
                      {
                          lstrcatW(szTempFN, L"_!!");
                      }
#endif
                      pCurFileToExtract->filename_ondisk_to_write = dfUnicodeCopyAlloc(szTempFN);
                  }
              }

          ProgressCallBackInfo.dfDirOrigSize += DfsFilePropertiesInfo.dfFileSizeContentUncompressed;
          ProgressCallBackInfo.dfDirEncodedSize += DfsFilePropertiesInfo.dfFileEncodedSizeWithoutPreAndPostHeader;
      }
      i++;
      pCurFileToExtract ++;
  }

  i = 0;
  pCurFileToExtract = pFileToExtract;
  while ((i < dfNbFile) && (dfError == DFS_SUCCESS))
  {
    DFSCRCINFOPARAM *pDfsCrcInfoParam = NULL;
    dfvoidp TagBufProperties, TagBufCrc;
    dfuLong32 TagSizeProperties, TagSizeCrc;
    DFTAGLIST TagListCurrentFileInDir = *(psTagListFileInDirCopy + i);
    BOOL fWillIgnoreFilePrevious;


    ProgressCallBackInfo.filename_onwork =
      pCurFileToExtract->filename_ondisk_to_write;

    ProgressCallBackInfo.filename_stored = NULL;
    ProgressCallBackInfo.fTemporaryFilename = pCurFileToExtract->fTemporaryFile;

    ProgressCallBackInfo.fWillIgnoreFile = pCurFileToExtract->fIgnore;
    ProgressCallBackInfo.dfFileOrigDone = 0;
    ProgressCallBackInfo.dfFileEncodedDone = 0;
    ProgressCallBackInfo.dfFileOrigSize = 0;
    ProgressCallBackInfo.dfFileEncodedSize = 0;
    ProgressCallBackInfo.dfFileNumber = i;


    ewei.dfNumFile = i;
    ewei.dfFileNameOnDisk = pCurFileToExtract->filename_ondisk_to_write;
    ewei.dfSuccess = 0;
    ewei.fTempFile = pCurFileToExtract->fTemporaryFile;

    fWillIgnoreFilePrevious = ProgressCallBackInfo.fWillIgnoreFile;
    if (!CallCallBack
        (pProgressCallBack, &ProgressCallBackInfo,
         DFCBM_BEFOREOPENWORKINGFILE, NULL))
      dfError = DFS_STOP_REQUESTED;

    if ((ProgressCallBackInfo.fWillIgnoreFile) && (!fWillIgnoreFilePrevious))
        pCurFileToExtract->fSkipUserRequested = TRUE;


    if (dfError == DFS_SUCCESS)
      if ((GetTag
           (TagListCurrentFileInDir, DFSTAG_CRCINFO, &TagBufCrc, &TagSizeCrc))
          &&
          (GetTag
           (TagListCurrentFileInDir, DFSTAG_FILEPOSPROPERTIES,
            &TagBufProperties, &TagSizeProperties)) &&
          (!ProgressCallBackInfo.fWillIgnoreFile))
        /* #define DFSTAG_FILEPOSPROPERTIES    (0x00000005) */

      {
        const DFSCRCINFO_FULLSIZESTRUCTURE *pDfsCrcInfo = (DFSCRCINFO_FULLSIZESTRUCTURE *) TagBufCrc;
        dfuLong32 dfNbCrcInfo = GetNbCrcInfo(pDfsCrcInfo,TagSizeCrc);


        DFSFILEPOSPROPERTIESINFO DfsFilePropertiesInfo;
        /*
            const DFSFILEPOSPROPERTIES *pDfsFileProp =
                (DFSFILEPOSPROPERTIES *) TagBufProperties;*/

        dfuLong64 dfFileSizeUncompressed=0; /* for progress info */
        dfuLong32 dfFilePosLow,dfFilePosHigh;
        dfuLong32 dfFileContentStoreMethod;
        dfuLong64 dfFileEncodedSizeWithoutPreAndPostHeader;
        dfuLong64 dfFileEncodedSizeWithPreAndPostHeader;
        dfuLong64 dfAllWrite = 0;
        dfuLong32 dfCrc32 = 0;
        dfbyte    bMd5[16];
        dfbyte    bSha1[20];
        dfbyte    bSha256[32];
        dfuLong64 dfUncompressedSize = 0;
        //BOOL dfIsGoodCrc = TRUE;
        BOOL fComputeCrc32,fComputeMD5,fComputeSHA1,fComputeSHA256;
        BOOL fIdenticalPatchInBuffer = FALSE;
        char szBufferIdentical[MAX_IDENTICAL_SIZE];
        dfuLong32 dfSizeBufferIdentical = 0;
        SVF_MD5_INTERNAL md5_ctx;
        SVF_SHA1_INTERNAL sha1_ctx;
        SVF_SHA256_INTERNAL sha256_ctx;
        /* fOutputSizeKnown and dfOutputSizeExpected for the file (temp?) creation */
        BOOL fOutputSizeKnown = FALSE;
        dfuLong64 dfOutputSizeExpected = 0;
        BOOL fSkipProgressiveWork = FALSE; /* not sure useful */
        BOOL fOutputFileToBeCreated = TRUE;
        BOOL fNoWriteSizeControl = FALSE;

        fComputeMD5 = fComputeSHA1 = fComputeSHA256 = fComputeCrc32 = (pCurFileToExtract->KindExtracting != KIND_EXTRACTING_COMBINERAMDIF_APPLY_INPLACE_NOCHECKSUM);


        ConvertDfsFileProperties(&DfsFilePropertiesInfo,TagBufProperties,TagSizeProperties);

        if (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_EXTRACTRAWSTREAMPATCHFILE)
        {
            fOutputSizeKnown = TRUE;
            dfOutputSizeExpected = DfsFilePropertiesInfo.dfFileEncodedSizeWithoutPreAndPostHeader;
            dfFileSizeUncompressed = DfsFilePropertiesInfo.dfFileEncodedSizeWithoutPreAndPostHeader;
        }
        else
        if (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_APPLYSTREAMPATCH)
        {
            /* there was a bug : fOutputSizeKnown = FALSE; on october 07*/
            fOutputSizeKnown = TRUE;
            dfOutputSizeExpected = DfsFilePropertiesInfo.dfFileSizeContentUncompressed;
            dfFileSizeUncompressed = DfsFilePropertiesInfo.dfFileSizeContentUncompressed;
        }
        else
        if (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_COMBINERAMDIF_TORAMDIF)
        {
            fOutputFileToBeCreated = FALSE;
            fOutputSizeKnown = FALSE; /* pehaps true */
            dfOutputSizeExpected = 0;
            dfFileSizeUncompressed = 0;
        }
        else
        if (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_COMBINERAMDIF_APPLY)
        {
            fOutputSizeKnown = TRUE;
            dfOutputSizeExpected = DfsFilePropertiesInfo.dfFileSizeContentUncompressed;
            dfFileSizeUncompressed = DfsFilePropertiesInfo.dfFileEncodedSizeWithoutPreAndPostHeader;
        }
        else
        if ((pCurFileToExtract->KindExtracting == KIND_EXTRACTING_COMBINERAMDIF_APPLY_INPLACE) ||
            (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_COMBINERAMDIF_APPLY_INPLACE_NOCHECKSUM))
        {
            fOutputFileToBeCreated = FALSE;
            fOutputSizeKnown = TRUE;
            dfOutputSizeExpected = DfsFilePropertiesInfo.dfFileSizeContentUncompressed;
            dfFileSizeUncompressed = DfsFilePropertiesInfo.dfFileEncodedSizeWithoutPreAndPostHeader;
        }

#ifndef SVF_EXTRACT_ONLY
        else
        if (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_COMBINERAMDIF_BUILDSTREAMPATCHFILE)
        {
            fOutputSizeKnown = FALSE;
            dfOutputSizeExpected = 0;
            dfFileSizeUncompressed = 0;
        }
#endif

        ProgressCallBackInfo.dfFileOrigSize = dfFileSizeUncompressed ;

        dfFilePosLow = (dfuLong32)DfsFilePropertiesInfo.dfFilePos;
        dfFilePosHigh = (dfuLong32)(DfsFilePropertiesInfo.dfFilePos>>32);

        dfFileContentStoreMethod = DfsFilePropertiesInfo.dfContentStoreMethod;


        dfFileEncodedSizeWithoutPreAndPostHeader =
             DfsFilePropertiesInfo.dfFileEncodedSizeWithoutPreAndPostHeader;
        ProgressCallBackInfo.dfFileEncodedSize = dfFileEncodedSizeWithoutPreAndPostHeader;

        dfFileEncodedSizeWithPreAndPostHeader =
                                  DfsFilePropertiesInfo.dfFileEncodedSizeWithPreAndPostHeader;


        ewei.dfSuccess = 0;
        if (pExtractingFileWorkingEvent!=NULL)
            (*pExtractingFileWorkingEvent)(EXTR_WORK_EVENT_BEFORE_EXTRACTING_FILE,&ewei,dfUserPtrWorkingEvent);

        pDfsCrcInfoParam =
          (DFSCRCINFOPARAM *) DfsMalloc(sizeof(DFSCRCINFOPARAM) *
                                        (dfNbCrcInfo + 1));

        if (pDfsCrcInfoParam != NULL)
        {
          dfuLong32 j;
          //BOOL fIsGoodCrc = FALSE;
          for (j = 0; j < dfNbCrcInfo; j++)
            ConvertDfsCrcInfoToDfsCrcInfoParam(pDfsCrcInfo + j,
                                               pDfsCrcInfoParam + j);

          #ifdef _DEBUG
               /*
               {
                   dfvoidp TagBuf;
                   dfuLong32 TagSize;
                   if (GetTag
                   (TagListCurrentFileInDir, DFSTAG_FILENAME, &TagBuf, &TagSize))
                   printf("%u:compare %ws and %ws\n",i,(pFileToCheck +i)->filename_ondisk_tocompare,TagBuf);
               } */
          #endif
        }

        /* if there is no patch stream (-> identical file) */

        if ((dfFilePosLow==0) && (dfFilePosHigh==0) &&
              ((pCurFileToExtract->KindExtracting == KIND_EXTRACTING_COMBINERAMDIF_APPLY) ||
               (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_COMBINERAMDIF_APPLY_INPLACE) ||
               (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_COMBINERAMDIF_APPLY_INPLACE_NOCHECKSUM) ||
               (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_COMBINERAMDIF_BUILDSTREAMPATCHFILE)))
                       // add KIND_EXTRACTING_EXTRACTRAWSTREAMPATCHFILE ?
        {
            fIdenticalPatchInBuffer = TRUE;
            dfSizeBufferIdentical =
                MakeDifStFileIdentical(szBufferIdentical,sizeof(szBufferIdentical),DfsFilePropertiesInfo.dfFileSizeContentUncompressed);
        }


        if ((dfFilePosLow==0) && (dfFilePosHigh==0) && (dfSizeBufferIdentical==0))
        {
            DFSINFODATE dfsInfoDate;
            BOOL fDateRead=FALSE;
            dfuLong32 dfErrorFile = DFS_SUCCESS;


            ProgressCallBackInfo.filename_previousversion =
                pCurFileToExtract->filename_ondisk_previous_to_read;

            if (dfError == DFS_SUCCESS)
                if (!CallCallBack
                    (pProgressCallBack, &ProgressCallBackInfo,
                    DFCBM_BEFOREOPENPREVIOUSFILE, NULL))
                    dfError = DFS_STOP_REQUESTED;

            //if (pCurFileToExtract->fRawExtracting)
            if (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_EXTRACTRAWSTREAMPATCHFILE)
            {
#ifndef SMARTVERSION_ONLYPUREEXTRACTINGSTUFF
                char szBufferIdentical[MAX_IDENTICAL_SIZE];
                LOWLEVELFILE llw;
                dfuLong32 dfSizeEncoded ;

                fSkipProgressiveWork = TRUE;

                dfSizeEncoded =
                  MakeDifStFileIdentical(szBufferIdentical,sizeof(szBufferIdentical),DfsFilePropertiesInfo.dfFileSizeContentUncompressed);
                llw = OpenLowLevel(ProgressCallBackInfo.filename_onwork,OPEN_CREATE,pCurFileToExtract->fTemporaryFile,
                                    fOutputSizeKnown,dfOutputSizeExpected,pei);
                if (llw == NULL)
                    dfErrorFile = dfError = DFS_ERROR_ERRORIO;
                else
                {
                    if (LowLevelWrite(llw,szBufferIdentical,dfSizeEncoded,pei) != dfSizeEncoded)
                        dfErrorFile = dfError = DFS_ERROR_ERRORIO;
                    LowLevelCloseEx(llw,fFlushWrite,pei);
                }
#endif
            }
            else
            if (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_APPLYSTREAMPATCH)
            //if (!MyCopyFile(pCurFileToExtract->filename_ondisk_previous_to_read,ProgressCallBackInfo.filename_onwork,FALSE, pei))
            {
                BOOL fCopyDone,fErrorOnOpeningDest;
                H_ERROR_INFO heiBefore = NULL;
                dfuLong64 dfFileSizeContentUncompressed = DfsFilePropertiesInfo.dfFileSizeContentUncompressed;
                fSkipProgressiveWork = TRUE;
                if (pei != NULL)
                  heiBefore = *pei;
                fCopyDone = CopyFileCutSize(pCurFileToExtract->filename_ondisk_previous_to_read,
                                 ProgressCallBackInfo.filename_onwork,FALSE, pei,
                                 pCurFileToExtract->fTemporaryFile,
                                 TRUE,fFlushWrite,&fErrorOnOpeningDest,dfFileSizeContentUncompressed);

                if (!fCopyDone)
                {
                    if (!fErrorOnOpeningDest)
                        dfErrorFile = dfError = DFS_ERROR_ERRORIO;
                    else
                    {
                      if (pei != NULL)
                          if ((heiBefore == NULL) && ((*pei)!=NULL))
                            {
                                FreeErrorInfoBlock(*pei);
                                *pei = NULL;
                            }
                      TryCreateDirForFileName(ProgressCallBackInfo.filename_onwork, pei);
                      if (!CopyFileCutSize(pCurFileToExtract->filename_ondisk_previous_to_read,
                            ProgressCallBackInfo.filename_onwork,FALSE, pei,
                            pCurFileToExtract->fTemporaryFile,
                            TRUE,fFlushWrite,&fErrorOnOpeningDest,dfFileSizeContentUncompressed))
                        dfErrorFile = dfError = DFS_ERROR_ERRORIO;
                    }
                }
            }
            else
            // todo : work with other KIND_EXTRACTING_* when we have other situation
            if (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_COMBINERAMDIF_TORAMDIF)
            {
                fSkipProgressiveWork = TRUE;
                ///pCurFileToExtract->hRamDifWork = pCurFileToExtract->hRamDifBaseCombine;
                //pCurFileToExtract->hRamDifBaseCombine = NULL;
            }
            else
            if (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_COMBINERAMDIF_APPLY)
            {
                // do loop only on output or use MakeDifStFileIdentical
            }
            else
            if ((pCurFileToExtract->KindExtracting == KIND_EXTRACTING_COMBINERAMDIF_APPLY_INPLACE) ||
                (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_COMBINERAMDIF_APPLY_INPLACE_NOCHECKSUM))
            {
                // do loop only on output or use MakeDifStFileIdentical
            }
#ifndef SVF_EXTRACT_ONLY
            else
            if (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_COMBINERAMDIF_BUILDSTREAMPATCHFILE)
            {
                // do loop only on output or use MakeDifStFileIdentical
            }
#endif

            if (dfErrorFile == DFS_SUCCESS)
            {
                dfvoidp pTagBuf;
                dfuLong32 TagSize;

                pCurFileToExtract->fCorrectlyDone = TRUE;

                if (GetTag(TagListCurrentFileInDir, DFSTAG_DATE, &pTagBuf, &TagSize))
                {
                    fDateRead = (TagSize>=sizeof(DFSINFODATE));
                }


                if (pCurFileToExtract->fSetNewDate)
                {
                    DFSTM dfsTm;
                    ConvertDfsInfoDateToDfsTm(&pCurFileToExtract->dfsInfoDate,&dfsTm);
                    ChangeDateTimeFile(ProgressCallBackInfo.filename_onwork,&dfsTm);
                }
                else
                if (fDateRead)
                {
                    DFSTM dfsTm;
                    dfsInfoDate = *((DFSINFODATE*)pTagBuf);

                    ConvertDfsInfoDateToDfsTm(&dfsInfoDate,&dfsTm);
                    #if ((!defined(_WINDOWS)) && ((defined(_DEBUG)) && defined(OUTTEXT)))
                    printf(" %02u/%02u/%04u %02u:%02u:%02u ",dfsTm.df_mday,dfsTm.df_mon,dfsTm.df_year,
                            dfsTm.df_hour,dfsTm.df_min,dfsTm.df_sec);
                    #endif

                    ChangeDateTimeFile(ProgressCallBackInfo.filename_onwork,&dfsTm);
                }


                ProgressCallBackInfo.dfDirOrigDone += dfFileSizeUncompressed;
                ProgressCallBackInfo.dfFileOrigDone += dfFileSizeUncompressed;
                if (dfError == DFS_SUCCESS)
                    if (!CallCallBack
                        (pProgressCallBack, &ProgressCallBackInfo,
                        DFCBM_AFTERCLOSINGWORKINGFILE, NULL))
                    dfError = DFS_STOP_REQUESTED;

                if (dfError == DFS_SUCCESS)
                    if (!CallCallBack
                        (pProgressCallBack, &ProgressCallBackInfo,
                        DFCBM_AFTERCLOSINGPREVIOUSFILE, NULL))
                        dfError = DFS_STOP_REQUESTED;
            }
        }
        else /* there is a real patch file to read or in buffer */
        {
            if (!fIdenticalPatchInBuffer)
              dfError = DfsGotoFilePointer(DfsFile, dfFilePosLow, dfFilePosHigh, pei);

            if ((dfError == DFS_SUCCESS)
                && (dfFileContentStoreMethod != TYPEDIR_FILECRCONLY))
            {
            DFSTM dfsTm;
            BOOL fMustRenameAfterClose = FALSE;
            ORIGDATA orgData;
            ORIGDATA* pOrgDataFilled=NULL;
            HFILECONTENTREADBUF hFileContentReadBuf = NULL;
            LOWLEVELFILE llfw=NULL;

            BOOL fRstrInitialized, fZstrInitialized,fZstrInitializedErzatzRaw;
            APPLYDIF_STREAM rstr;
            abstract_decompress_stream zstr;
            int zstr_stream_end_reached = 0;

            BOOL fApplyRamDifInitialised,fBuildRamDifFromStreamInitialised,fPatchBldFromRamDifAndStream;
            APPLYRAMDIF_STREAM ApplyRamDifStr;
            APPLYRAMDIFINPLACE_STREAM ApplyRamDifInPlaceStr;
            RAMDIFBLD_FROM_DATA_STREAM RamDifBldStr; /* KIND_EXTRACTING_COMBINERAMDIF_TORAMDIF */
            PATCHSTREAMBLD_FROM_DATA_STREAM PatchSteamBldStr; /* KIND_EXTRACTING_COMBINERAMDIF_BUILDSTREAMPATCHFILE */

            /* orig data for Apply in place prepare */


            dfuLong32 dfBufWriteAvailOut;
            DFSINFODATE dfsInfoDate;
            BOOL fDateRead=FALSE;

            fRstrInitialized = fZstrInitialized = fZstrInitializedErzatzRaw = FALSE;
            fApplyRamDifInitialised = fBuildRamDifFromStreamInitialised = fPatchBldFromRamDifAndStream = FALSE;

            if ((dfFileContentStoreMethod == TYPEDIR_PATCHFROMPREVIOUS) &&
                (pCurFileToExtract->filename_ondisk_previous_to_read == NULL) &&
                ((pCurFileToExtract->KindExtracting == KIND_EXTRACTING_APPLYSTREAMPATCH)))
                dfFileContentStoreMethod = TYPEDIR_FILEINSERTING_DEFLATE;


            if (((dfFileContentStoreMethod == TYPEDIR_PATCHFROMPREVIOUS) &&
                 (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_APPLYSTREAMPATCH)) ||
                (((dfFileContentStoreMethod == TYPEDIR_PATCHFROMPREVIOUS) &&
                 ((pCurFileToExtract->KindExtracting == KIND_EXTRACTING_COMBINERAMDIF_APPLY_INPLACE) ||
                  (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_COMBINERAMDIF_APPLY_INPLACE_NOCHECKSUM))) &&
                 (!fIdenticalPatchInBuffer)))
            {

                // if we need previouis
                ProgressCallBackInfo.filename_previousversion =
                pCurFileToExtract->filename_ondisk_previous_to_read;
                if (dfError == DFS_SUCCESS)
                if (!CallCallBack
                    (pProgressCallBack, &ProgressCallBackInfo,
                    DFCBM_BEFOREOPENPREVIOUSFILE, NULL))
                    dfError = DFS_STOP_REQUESTED;

                // create FMIO
                if (dfError == DFS_SUCCESS)
                {
                  if (GetFileFullContentBuffer(ProgressCallBackInfo.
                                                filename_previousversion,FILEFULLCONTENTBUFFER_READ,0,
                                                &hFileContentReadBuf,
                                                &orgData,pei))
                      pOrgDataFilled = &orgData;
                  else
                      dfError = DFS_ERROR_ERRORIO;
                }
            }

            if ((dfFileContentStoreMethod == TYPEDIR_PATCHFROMPREVIOUS) &&
                (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_APPLYSTREAMPATCH))
            {
                int err;

                DfsClearStruct(&rstr, 0, sizeof(APPLYDIF_STREAM));
                err = ApplyDifInitEx(&rstr,FALSE);

                rstr.nbOrig = 1;
                rstr.OrigDataPtr = pOrgDataFilled;
                rstr.out_data_stream.next_out = (dfvoidp) BufWrite;
                rstr.out_data_stream.avail_out = dfBufWriteAvailOut = dfBufWriteSize;

                if (err != DSERR_OK)
                    dfError = DFS_ERROR_MEMORY_ERROR;
                else
                    fRstrInitialized = TRUE;

            }

            if ((dfFileContentStoreMethod == TYPEDIR_FILEINSERTING_STORE) ||
                (dfFileContentStoreMethod == TYPEDIR_FILEINSERTING_DEFLATE) ||
                (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_EXTRACTRAWSTREAMPATCHFILE))
            {
                DfsClearStruct(&zstr, 0, sizeof(abstract_decompress_stream));
                zstr.next_out = BufWrite;
                zstr.avail_out = dfBufWriteAvailOut = dfBufWriteSize;
                if (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_EXTRACTRAWSTREAMPATCHFILE)
                {
                    if (abstract_init_store(&zstr) != ABSTR_DECOMPRESS_Z_OK)
                        dfError = DFS_ERROR_MEMORY_ERROR;
                    else
                        fZstrInitialized = fZstrInitializedErzatzRaw = TRUE;
                }
                else
                {
                    //if (abstract_init_inflate_withoutNegMaxWBits(&zstr) != ABSTR_DECOMPRESS_Z_OK)
                    if (abstract_init_prefix(&zstr) != ABSTR_DECOMPRESS_Z_OK)
                      dfError = DFS_ERROR_MEMORY_ERROR;
                    else
                      fZstrInitialized = TRUE;
                }
            }

            // we must initialise HRAMMDIF work with other KIND_EXTRACTING_*


            if ((dfFileContentStoreMethod == TYPEDIR_PATCHFROMPREVIOUS) &&
                ((pCurFileToExtract->KindExtracting == KIND_EXTRACTING_COMBINERAMDIF_TORAMDIF) ||
                 (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_COMBINERAMDIF_APPLY) ||
                 (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_COMBINERAMDIF_APPLY_INPLACE) ||
                 (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_COMBINERAMDIF_APPLY_INPLACE_NOCHECKSUM)))
                 if (!fIdenticalPatchInBuffer)
            {
                int err;

                DfsClearStruct(&RamDifBldStr, 0, sizeof(RAMDIFBLD_FROM_DATA_STREAM));
                err = RamDifBldFromDataStreamAndRamDifInit(&RamDifBldStr,DfsFilePropertiesInfo.dfFileSizeContentUncompressed,
                    pOrgDataFilled,
                    pCurFileToExtract->hRamDifWork);

                if (err != DSERR_OK)
                    dfError = DFS_ERROR_MEMORY_ERROR;
                else
                    fBuildRamDifFromStreamInitialised = TRUE;
            }


#ifndef SVF_EXTRACT_ONLY
            if ((dfFileContentStoreMethod == TYPEDIR_PATCHFROMPREVIOUS) &&
                (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_COMBINERAMDIF_BUILDSTREAMPATCHFILE))
            {
                int err;
                DfsClearStruct(&PatchSteamBldStr, 0, sizeof(PATCHSTREAMBLD_FROM_DATA_STREAM));


                ApplyRamDifStr.out_data_stream.next_out = BufWrite;
                ApplyRamDifStr.out_data_stream.avail_out = dfBufWriteAvailOut = dfBufWriteSize;

                /* to be checked */
                ApplyRamDifInPlaceStr.out_data_stream.next_out = BufWrite;
                ApplyRamDifInPlaceStr.out_data_stream.avail_out = dfBufWriteAvailOut ;


                err = PatchOutStreamBldFromDataStreamAndRamDifInit(&PatchSteamBldStr,
                                DfsFilePropertiesInfo.dfFileSizeContentUncompressed,
                                pCurFileToExtract->hRamDifWork,9, DEFAULT_SIZE_BUF_STREAM_KB);

                if (err != DSERR_OK)
                    dfError = DFS_ERROR_MEMORY_ERROR;
                else
                    fPatchBldFromRamDifAndStream = TRUE;
            }
#endif

            if (pDfsCrcInfoParam!=NULL)
              if (!pDfsCrcInfoParam->fMd5)
                 fComputeMD5=FALSE;

            if (pDfsCrcInfoParam!=NULL)
              if (!pDfsCrcInfoParam->fSha1)
                 fComputeSHA1=FALSE;

            if (pDfsCrcInfoParam != NULL)
                if (!pDfsCrcInfoParam->fSha256)
                    fComputeSHA256 = FALSE;

            if (fComputeMD5)
                svf_md5_init(&md5_ctx);
            if (fComputeSHA1)
                svf_sha1_init(&sha1_ctx);
            if (fComputeSHA256)
                svf_sha256_init(&sha256_ctx);

            if ((dfError == DFS_SUCCESS) && fOutputFileToBeCreated)
            {
                // To optimize temp WRITE dfOutputSizeExpected
                H_ERROR_INFO heiBefore = NULL;
                if (pei != NULL)
                  heiBefore = *pei;

                llfw = OpenLowLevel(ProgressCallBackInfo.filename_onwork, OPEN_CREATE,
                           pCurFileToExtract->fTemporaryFile,fOutputSizeKnown,dfOutputSizeExpected, pei);

                if (llfw == NULL)
                {
                    if (pei != NULL)
                        if ((heiBefore == NULL) && ((*pei)!=NULL))
                        {
                            FreeErrorInfoBlock(*pei);
                            *pei = NULL;
                        }

                    /* Try MkDir */
                    TryCreateDirForFileName(ProgressCallBackInfo.filename_onwork, pei);
                    // To optimize temp WRITE dfOutputSizeExpected
                    llfw = OpenLowLevel(ProgressCallBackInfo.filename_onwork, OPEN_CREATE,
                               pCurFileToExtract->fTemporaryFile,fOutputSizeKnown,dfOutputSizeExpected, pei);
                }
                if (llfw == NULL)
                    dfError = DFS_ERROR_ERRORIO;
            }

            if ((dfError == DFS_SUCCESS) && (fIdenticalPatchInBuffer))
            {
             //   dfSizeEncoded = dfSizeBufferIdentical;
            }

            if ((dfError == DFS_SUCCESS) && (!fIdenticalPatchInBuffer))
            {
                dfuLong64 dfSizeEncoded = 0;
                dfuLong32 dfBlockType = 0;
                dfuLong64 dfToDoEncoded;
                dfuLong32 dfBufWritePos = 0;

                dfvoidp pTagBuf;
                dfuLong32 TagSize;
                DFTAGLIST DfTagList = NULL;
                BOOL fContinuePrimaryLoop = TRUE;

                //dfvoidp bufread = NULL;
                dfError = DfsRGetNextBlockType(DfsFile, &dfBlockType, pei);
                if (dfError == DFS_SUCCESS)
                dfError = DfsROpenNextFileAndTagBeforeFile
                    (DfsFile, NULL, &dfSizeEncoded, NULL, &DfTagList, pei);
                dfToDoEncoded = dfSizeEncoded;

                #if (!defined(_WINDOWS) && (defined(_DEBUG)) && (defined(OUTTEXT)))
                printf("         size encoded=%u", dfSizeEncoded);
                #endif

                if (GetTag(DfTagList, DFSTAG_FILENAME, &pTagBuf, &TagSize))
                {
                #if (!defined(_WINDOWS) && (defined(_DEBUG)) && (defined(OUTTEXT)))
                printf(" filename = '%ws' ", pTagBuf);
                #endif
                }

                if (GetTag(DfTagList, DFSTAG_DATE, &pTagBuf, &TagSize))
                {
                    fDateRead = (TagSize>=sizeof(DFSINFODATE));
                }

                if (!fDateRead)
                    if (GetTag(TagListCurrentFileInDir, DFSTAG_DATE, &pTagBuf, &TagSize))
                    {
                        fDateRead = (TagSize>=sizeof(DFSINFODATE));
                    }

                if (fDateRead)
                {
                    dfsInfoDate = *((DFSINFODATE*)pTagBuf);

                    {
                    DFSTM dfsTm;

                    ConvertDfsInfoDateToDfsTm(&dfsInfoDate,&dfsTm);
                    #if ((!defined(_WINDOWS)) && ((defined(_DEBUG)) && defined(OUTTEXT)))
                    printf(" %02u/%02u/%04u %02u:%02u:%02u ",dfsTm.df_mday,dfsTm.df_mon,dfsTm.df_year,
                            dfsTm.df_hour,dfsTm.df_min,dfsTm.df_sec);
                    #endif
                    }
                }

                #if ((!defined(_WINDOWS)) && ((defined(_DEBUG)) && defined(OUTTEXT)))
                printf("\n");
                #endif

                CloseTagList(DfTagList);


                //bufread = DfsMalloc(dfSizeEncoded + 1);


                //while ((dfToDoEncoded > 0) && (dfError == DFS_SUCCESS))
                while ((fContinuePrimaryLoop) && (dfError == DFS_SUCCESS))
                {
                dfuLong32 dfReadThis;
                BOOL fAllEncodedDataInReadThis;

                if (dfToDoEncoded > dfBufReadSize)
                    dfReadThis = dfBufReadSize;
                else
                    dfReadThis = (dfuLong32)dfToDoEncoded;

                fAllEncodedDataInReadThis = (dfReadThis == dfToDoEncoded);

                if (fIdenticalPatchInBuffer)
                {
                    DfsMemcpy(BufRead, ((dfbytep)szBufferIdentical) + ProgressCallBackInfo.dfFileEncodedDone, dfReadThis);
                }
                else
                {
                    dfError =
                        DfsRReadFileEncoded(DfsFile, BufRead, dfReadThis, NULL, pei);
                }

                ProgressCallBackInfo.dfDirEncodedDone += dfReadThis;
                ProgressCallBackInfo.dfFileEncodedDone += dfReadThis;

                if (((fZstrInitialized) || (fRstrInitialized) || (fBuildRamDifFromStreamInitialised))
                    && (dfError == DFS_SUCCESS))
                {
                    dfuLong32 dfWriteThis = 0;
                    dfuLong32 dfAvailIn = 0;
                    BOOL fNoError = FALSE;
                    BOOL fEnd = FALSE;
                    BOOL fContinueSecondaryLoop = FALSE;
                    IN_DATA_STREAM* p_in_data_stream = NULL;

                    if (fZstrInitialized)
                    {
                        zstr.avail_in = dfReadThis;
                        zstr.next_in = BufRead;
                    }

                    if (fRstrInitialized)
                        p_in_data_stream = &rstr.in_data_stream;

                    if (fBuildRamDifFromStreamInitialised)
                        p_in_data_stream = &RamDifBldStr.in_data_stream;

                    if (fPatchBldFromRamDifAndStream)
                        p_in_data_stream = &PatchSteamBldStr.in_data_stream;
                    //
                    if (fApplyRamDifInitialised)
                    {
                        // todo
                    }


                    if (p_in_data_stream != NULL)
                    {
                        p_in_data_stream->avail_in = dfReadThis;
                        p_in_data_stream->next_in = BufRead;
                    }

                    if (fBuildRamDifFromStreamInitialised)
                    {
                        int err = DoRamDifBldFromDataStream(&RamDifBldStr);
                        if (err == DSERR_END)
                            fEnd=TRUE;
                        fNoError = fEnd || (err == DSERR_OK);
                    }

                    do
                    {
                        if (dfError == DFS_SUCCESS)
                            if (!CallCallBack
                                (pProgressCallBack, &ProgressCallBackInfo,
                                DFCBM_PROGRESSWORKINGFILE, NULL))
                            {
                            dfError = DFS_STOP_REQUESTED;
                            break;
                            }

                        if (fZstrInitialized)
                        {
                            int err;
                            zstr.avail_out = dfBufWriteSize - dfBufWritePos;
                            zstr.next_out = BufWrite + dfBufWritePos;


                            if (fZstrInitializedErzatzRaw)
                            {
                                //err=XflateStore_decompress(&zstr,
                                err = abstract_decompress(&zstr,
                                    /*(dfReadThis == 0) ? Z_FINISH : */ABSTR_DECOMPRESS_Z_SYNC_FLUSH);
                                if (fAllEncodedDataInReadThis && (err==ABSTR_DECOMPRESS_Z_OK) && (zstr.avail_in==0))
                                    err = ABSTR_DECOMPRESS_Z_STREAM_END;
                            }
                            else
                            {
                                if ((zstr_stream_end_reached != 0) && (zstr.avail_in == 0))
                                {
                                    err = ABSTR_DECOMPRESS_Z_STREAM_END;
                                }
                                else
                                {
                                    err = abstract_decompress(&zstr,
                                        /*(dfReadThis == 0) ? ABSTR_DECOMPRESS_Z_FINISH : */
                                        /*(dfSizeEncoded==(zstr.total_in+zstr.avail_in)) ? ABSTR_DECOMPRESS_Z_FINISH :*/ ABSTR_DECOMPRESS_Z_SYNC_FLUSH);
                                }
                            }
                            dfWriteThis = dfBufWriteSize - zstr.avail_out;
                            dfAvailIn = zstr.avail_in;

                            fEnd = (err == ABSTR_DECOMPRESS_Z_STREAM_END);
                            if (fEnd)
                                zstr_stream_end_reached = 1;
                            fNoError = fEnd || (err == ABSTR_DECOMPRESS_Z_OK);
                            if (dfReadThis == dfToDoEncoded)
                                fContinueSecondaryLoop = !fEnd;
                            else
                                fContinueSecondaryLoop = zstr.avail_in > 0;
                        }

                        if (fRstrInitialized)
                        {
                            int err;
                            rstr.out_data_stream.avail_out = dfBufWriteSize - dfBufWritePos;
                            rstr.out_data_stream.next_out = BufWrite + dfBufWritePos;
                            err = DoApplyDifWork(&rstr);
                            dfWriteThis = dfBufWriteSize - ((dfuLong32)rstr.out_data_stream.avail_out);
                            dfAvailIn = (dfuLong32)rstr.in_data_stream.avail_in;
                            fEnd = (err == DSERR_END);
                            fNoError = fEnd || (err == DSERR_OK);

                            if (dfReadThis == dfToDoEncoded)
                            fContinueSecondaryLoop = !fEnd;
                            else
                            fContinueSecondaryLoop = rstr.in_data_stream.avail_in > 0;
                        }

                        if ((fNoError) && (dfWriteThis > 0) && fOutputFileToBeCreated)
                        {
                            if ((fEnd) || (dfWriteThis == dfBufWriteSize) || (dfError != DFS_SUCCESS) || (dfAvailIn > 0))
                            {
                                dfuLong32 dfSizeWrite;
                                dfSizeWrite = LowLevelWrite(llfw, BufWrite, dfWriteThis, pei);
                                if (dfSizeWrite != dfWriteThis)
                                dfError = DFS_ERROR_ERRORIO;
                                if (fComputeCrc32)
                                    dfCrc32 = (dfuLong32)crc32(dfCrc32, BufWrite, dfWriteThis);
                                if (fComputeMD5)
                                    svf_md5_append(&md5_ctx, BufWrite, dfWriteThis);
                                if (fComputeSHA1)
                                    svf_sha1_append(&sha1_ctx, BufWrite, dfWriteThis);
                                if (fComputeSHA256)
                                    svf_sha256_append(&sha256_ctx, BufWrite, dfWriteThis);

                                dfAllWrite += dfSizeWrite;
                                dfBufWritePos = 0;

                                ProgressCallBackInfo.dfDirOrigDone += dfSizeWrite;
                                ProgressCallBackInfo.dfFileOrigDone += dfSizeWrite;

                                if (dfError == DFS_SUCCESS)
                                   if (!CallCallBack
                                        (pProgressCallBack, &ProgressCallBackInfo,
                                         DFCBM_PROGRESSWORKINGFILE, NULL))
                                   {
                                        dfError = DFS_STOP_REQUESTED;
                                        break;
                                   }
                            }
                            else
                            {
                                dfBufWritePos = dfWriteThis;
                            }
                        }

                        if ((!fNoError) && (dfError == DFS_SUCCESS))
                            dfError = DFS_ERROR_BAD_PARAMETER; /* --++** */

                    }
                    while ((dfError == DFS_SUCCESS) && (!fEnd)
                        && fContinueSecondaryLoop);
                }

                dfToDoEncoded -= dfReadThis;
                if (dfReadThis == 0)
                    fContinuePrimaryLoop = FALSE;
                }


                //// must do the loop*******



                //DfsFree(bufread);

                if ((dfError == DFS_SUCCESS) && (!fIdenticalPatchInBuffer))
                dfError =
                    DfsRReadGetPostFile(DfsFile, &dfUncompressedSize, NULL, NULL, pei);
            }



            if ((dfError == DFS_SUCCESS) && (fIdenticalPatchInBuffer))
                dfUncompressedSize = DfsFilePropertiesInfo.dfFileSizeContentUncompressed;

            if (fRstrInitialized)
                CloseApplyDif(&rstr, NULL);
            if (fZstrInitialized)
                abstract_decompress_end(&zstr);
            if (fBuildRamDifFromStreamInitialised)
                {
                    HRAMDIF hRamDifResult = NULL;
                    DFSPATCHANALYSEINFO_MEMORY dfsPatchAnalyseInfoToFill;
                    int errClose = CloseRamDifBldFromDataStream(&RamDifBldStr,&hRamDifResult,&dfsPatchAnalyseInfoToFill);
                    if (pCurFileToExtract->hRamDifWork != NULL)
                        DeleteRamDif(pCurFileToExtract->hRamDifWork);
                    pCurFileToExtract->hRamDifWork = hRamDifResult;
                    if (errClose != DSERR_OK)
                        dfError = DFS_ERROR_BAD_PARAMETER;
                    fNoWriteSizeControl = TRUE;
                }

            if (((pCurFileToExtract->KindExtracting == KIND_EXTRACTING_COMBINERAMDIF_APPLY_INPLACE) ||
                 (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_COMBINERAMDIF_APPLY_INPLACE_NOCHECKSUM)) &&
                   (hFileContentReadBuf != NULL))
                {
                    CloseFileFullContentBuffer(hFileContentReadBuf);

                    hFileContentReadBuf=NULL;
                    if (dfError == DFS_SUCCESS)
                      if (!CallCallBack
                        (pProgressCallBack, &ProgressCallBackInfo,
                        DFCBM_AFTERCLOSINGPREVIOUSFILE, NULL))
                        dfError = DFS_STOP_REQUESTED;
                }

                if ((dfFileContentStoreMethod == TYPEDIR_PATCHFROMPREVIOUS) &&
                    (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_COMBINERAMDIF_APPLY))
                {
                    int err,retclose;

                    DfsClearStruct(&ApplyRamDifStr, 0, sizeof(APPLYRAMDIF_STREAM));
                    err = ApplyRamDifInit(pCurFileToExtract->hRamDifWork,&ApplyRamDifStr,FALSE);

                    ApplyRamDifStr.nbOrig = 1;
                    ApplyRamDifStr.OrigDataPtr = pOrgDataFilled;
                    ApplyRamDifStr.out_data_stream.next_out = (dfvoidp) BufWrite;
                    ApplyRamDifStr.out_data_stream.avail_out = dfBufWriteAvailOut = dfBufWriteSize;

                    if (err != DSERR_OK)
                        dfError = DFS_ERROR_MEMORY_ERROR;
                    else
                        fApplyRamDifInitialised = TRUE;

                    ProgressCallBackInfo.filename_previousversion =
                      pCurFileToExtract->filename_ondisk_previous_to_read;
                    if (dfError == DFS_SUCCESS)
                    if (!CallCallBack
                        (pProgressCallBack, &ProgressCallBackInfo,
                        DFCBM_BEFOREOPENPREVIOUSFILE, NULL))
                        dfError = DFS_STOP_REQUESTED;

                    // create FMIO
                    if (dfError == DFS_SUCCESS)
                        if ((pCurFileToExtract->KindExtracting != KIND_EXTRACTING_COMBINERAMDIF_APPLY_INPLACE) &&
                            (pCurFileToExtract->KindExtracting != KIND_EXTRACTING_COMBINERAMDIF_APPLY_INPLACE_NOCHECKSUM))
                        {
                            if (GetFileFullContentBuffer(ProgressCallBackInfo.
                                                    filename_previousversion,FILEFULLCONTENTBUFFER_READ,0,
                                                    &hFileContentReadBuf,
                                                    &orgData,pei))
                                ApplyRamDifStr.OrigDataPtr = pOrgDataFilled = &orgData;
                            else
                                dfError = DFS_ERROR_ERRORIO;
                        }

                    //now the apply work


                    while (dfError == DFS_SUCCESS)
                    {
                        int retApply;
                        dfuLong32 dfSizeWrite,dfWriteThis;

                        ApplyRamDifStr.out_data_stream.next_out = (dfvoidp) BufWrite;
                        ApplyRamDifStr.out_data_stream.avail_out = dfBufWriteSize;

                        retApply = DoApplyRamDifWork(&ApplyRamDifStr);
                        if ((retApply != DSERR_OK) && (retApply != DSERR_END))
                        {
                            dfError = DFS_ERROR_ERRORIO;
                        }

                        dfWriteThis = dfBufWriteSize - ApplyRamDifStr.out_data_stream.avail_out;

                        dfSizeWrite = 0;
                        if (dfWriteThis != 0)
                        {
                            dfSizeWrite = LowLevelWrite(llfw, BufWrite, dfWriteThis, pei);
                            if (dfSizeWrite != dfWriteThis)
                              dfError = DFS_ERROR_ERRORIO;
                            if (fComputeCrc32)
                                dfCrc32 = (dfuLong32)crc32(dfCrc32, BufWrite, dfWriteThis);
                            if (fComputeMD5)
                                svf_md5_append(&md5_ctx, BufWrite, dfWriteThis);
                            if (fComputeSHA1)
                                svf_sha1_append(&sha1_ctx, BufWrite, dfWriteThis);
                            if (fComputeSHA256)
                                svf_sha256_append(&sha256_ctx, BufWrite, dfWriteThis);

                            dfAllWrite += dfSizeWrite;


                            ProgressCallBackInfo.dfDirOrigDone += dfSizeWrite;
                            ProgressCallBackInfo.dfFileOrigDone += dfSizeWrite;

                            if (dfError == DFS_SUCCESS)
                                if (!CallCallBack
                                    (pProgressCallBack, &ProgressCallBackInfo,
                                    DFCBM_PROGRESSWORKINGFILE, NULL))
                                {
                                dfError = DFS_STOP_REQUESTED;
                                break;
                                }
                        }
                        if ((retApply == DSERR_END) || (dfSizeWrite==0))
                            break;
                    }
                    retclose = CloseRamApplyDif(&ApplyRamDifStr,NULL);
                    if (retclose != DSERR_OK)
                        dfError = DFS_ERROR_ERRORIO;
                    DeleteRamDif(pCurFileToExtract->hRamDifWork);
                    pCurFileToExtract->hRamDifWork = NULL;
                }


                /* wrote KIND_EXTRACTING_COMBINERAMDIF_APPLY_INPLACE / KIND_EXTRACTING_COMBINERAMDIF_APPLY_INPLACE_NOCHECKSUMhere */
                if ((dfFileContentStoreMethod == TYPEDIR_PATCHFROMPREVIOUS) &&
                    ((pCurFileToExtract->KindExtracting == KIND_EXTRACTING_COMBINERAMDIF_APPLY_INPLACE) ||
                     (pCurFileToExtract->KindExtracting == KIND_EXTRACTING_COMBINERAMDIF_APPLY_INPLACE_NOCHECKSUM)))
                {
                    int err,retclose;
                    dfuLong64 curPosMap = 0;
                    dfuLong64 dfFileSizeContentUncompressedToDo=DfsFilePropertiesInfo.dfFileSizeContentUncompressed;

                    DfsClearStruct(&ApplyRamDifInPlaceStr, 0, sizeof(APPLYRAMDIFINPLACE_STREAM));
                    err = ApplyRamDifInPlaceInit(pCurFileToExtract->hRamDifWork,&ApplyRamDifInPlaceStr,FALSE);
                    //return 0;
                    //ApplyRamDifInPlaceStr.nbOrig = 1;
                    //ApplyRamDifInPlaceStr.OrigDataPtr = &org;

                    if (err != DSERR_OK)
                        dfError = DFS_ERROR_MEMORY_ERROR;
                    else
                        fApplyRamDifInitialised = TRUE;

                    ProgressCallBackInfo.filename_previousversion =
                      pCurFileToExtract->filename_ondisk_previous_to_read;
                    if (dfError == DFS_SUCCESS)
                    if (!CallCallBack
                        (pProgressCallBack, &ProgressCallBackInfo,
                        DFCBM_BEFOREOPENPREVIOUSFILE, NULL))
                        dfError = DFS_STOP_REQUESTED;

                    // we must TODO LowLevelSetFileSize ProgressCallBackInfo.filename_previousversion to DfsFilePropertiesInfo.dfFileSizeContentUncompressed



                    // create FMIO
                    if (dfError == DFS_SUCCESS)
                    {
                        // if ((pCurFileToExtract->KindExtracting != KIND_EXTRACTING_COMBINERAMDIF_APPLY_INPLACE) || (pCurFileToExtract->KindExtracting != KIND_EXTRACTING_COMBINERAMDIF_APPLY_INPLACE_NOCHECKSUM))
                        if (GetFileFullContentBuffer(ProgressCallBackInfo.
                            filename_previousversion,
                            FILEFULLCONTENTBUFFER_READWRITE | FILEFULLCONTENTBUFFER_RESIZE |
                            (fFlushWrite ? FILEFULLCONTENTBUFFER_PROGRESSIVE_FLUSH : 0),
                            DfsFilePropertiesInfo.dfFileSizeContentUncompressed,
                            &hFileContentReadBuf,
                            pOrgDataFilled, pei))
                        {
                            fMustRenameAfterClose = TRUE;
                        }
                        else
                        {
                            dfError = DFS_ERROR_ERRORIO;
                        }
                    }
                    ///ApplyRamDifInPlaceStr.out_data_stream.next_out = (dfvoidp) GetOrigDataPtrpDataBySizeView(&org,0,dfBufWriteSize);
                    ///ApplyRamDifInPlaceStr.out_data_stream.avail_out = dfBufWriteAvailOut = dfBufWriteSize;

                    //now the apply work
                    dfError+=0;
                    /*
                    */


                    while (dfError == DFS_SUCCESS)
                    {
                        int retApply;
                        dfuLong32 dfSizeWrite,dfWriteThis;
                        dfvoidp BufStartThis;
                        dfuLong32 dfBufWriteSizeThis = dfBufWriteSize;
                        BOOL fDirtyFlag=FALSE;
                        dfuLong64 lastPosCallProgress = 0;
                        dfuLong64 dfProgressCallStep = dfBufWriteSize;
                        if ((NeedManualSetDataPtrDirty(pOrgDataFilled)) && (dfBufWriteSize > OPTIMAL_BLOCK_SIZE_FOR_MANUAL_DIRTY))
                            dfBufWriteSize = OPTIMAL_BLOCK_SIZE_FOR_MANUAL_DIRTY;

                        if (dfBufWriteSizeThis > dfFileSizeContentUncompressedToDo)
                            dfBufWriteSizeThis = (dfuLong32)dfFileSizeContentUncompressedToDo;

                        ApplyRamDifInPlaceStr.out_data_stream.next_out = BufStartThis = (dfvoidp)(((dfbytep) GetOrigDataPtrpDataBySizeView(pOrgDataFilled,curPosMap,dfBufWriteSizeThis))+curPosMap);
                        ApplyRamDifInPlaceStr.out_data_stream.avail_out = dfBufWriteSizeThis;


                        //return 0;
                        retApply = DoApplyRamDifInPlaceWorkWithDirtyFlag(&ApplyRamDifInPlaceStr, &fDirtyFlag);
                        if ((retApply != DSERR_OK) && (retApply != DSERR_END))
                        {
                            dfError = DFS_ERROR_ERRORIO;
                        }

                        /* not sure ! */
                        dfWriteThis = dfBufWriteSizeThis - ApplyRamDifInPlaceStr.out_data_stream.avail_out;

                        if (fDirtyFlag)
                        {
                            SetDataPtrDirtyByPosSize(pOrgDataFilled, curPosMap, dfWriteThis);
                        }


                        curPosMap += dfWriteThis;
                        dfFileSizeContentUncompressedToDo -= dfWriteThis;

                        dfSizeWrite = 0;
                        if (dfWriteThis != 0)
                        {
                            // = LowLevelWrite(llfw, BufWrite, dfWriteThis, pei);

                            dfSizeWrite = dfWriteThis;

                            if (dfSizeWrite != dfWriteThis)
                              dfError = DFS_ERROR_ERRORIO;
                            if (fComputeCrc32)
                                dfCrc32 = (dfuLong32)crc32(dfCrc32, (dfbytep)BufStartThis, dfWriteThis);
                            if (fComputeMD5)
                                svf_md5_append(&md5_ctx, (dfbytep)BufStartThis, dfWriteThis);
                            if (fComputeSHA1)
                                svf_sha1_append(&sha1_ctx, (dfbytep)BufStartThis, dfWriteThis);
                            if (fComputeSHA256)
                                svf_sha256_append(&sha256_ctx, (dfbytep)BufStartThis, dfWriteThis);

                            dfAllWrite += dfSizeWrite;


                            ProgressCallBackInfo.dfDirOrigDone += dfSizeWrite;
                            ProgressCallBackInfo.dfFileOrigDone += dfSizeWrite;

                            if (((curPosMap - lastPosCallProgress) >= dfProgressCallStep) || (dfFileSizeContentUncompressedToDo==0))
                            {
                                lastPosCallProgress = curPosMap;
                                if (dfError == DFS_SUCCESS)
                                    if (!CallCallBack
                                        (pProgressCallBack, &ProgressCallBackInfo,
                                        DFCBM_PROGRESSWORKINGFILE, NULL))
                                    {
                                        dfError = DFS_STOP_REQUESTED;
                                        break;
                                    }
                            }
                        }
                        if ((retApply == DSERR_END) || (dfSizeWrite==0))
                            break;
                    }
                    retclose = CloseRamApplyInPlaceDif(&ApplyRamDifInPlaceStr,NULL);
                    if (retclose != DSERR_OK)
                        dfError = DFS_ERROR_ERRORIO;
                    DeleteRamDif(pCurFileToExtract->hRamDifWork);
                    pCurFileToExtract->hRamDifWork = NULL;
                }




                // must check crc, read post file


                if (fComputeMD5)
                  svf_md5_finish(&md5_ctx,bMd5);
                if (fComputeSHA1)
                  svf_sha1_finish(&sha1_ctx,bSha1);
                if (fComputeSHA256)
                    svf_sha256_finish(&sha256_ctx, bSha256);


                if (pCurFileToExtract->fSetNewDate)
                {
                    ConvertDfsInfoDateToDfsTm(&pCurFileToExtract->dfsInfoDate, &dfsTm);
                }
                else
                {
                    //if ((!fDateRead) && (pDfsFileProp->))
                    if (fDateRead)
                    {
                        ConvertDfsInfoDateToDfsTm(&dfsInfoDate, &dfsTm);
                    }
                }

                if (llfw != NULL)
                {
                    BOOL fDateChanged = FALSE;
                    LowLevelCloseChangeDateTime(llfw, fFlushWrite, &dfsTm, &fDateChanged, pei);
                    if (!fDateChanged)
                        ChangeDateTimeFile(ProgressCallBackInfo.filename_onwork, &dfsTm);
                }

                if (dfError == DFS_SUCCESS)
                    if (!CallCallBack
                        (pProgressCallBack, &ProgressCallBackInfo,
                        DFCBM_AFTERCLOSINGWORKINGFILE, NULL))
                    dfError = DFS_STOP_REQUESTED;

                if (hFileContentReadBuf != NULL)
                {
                    CloseFileFullContentBuffer(hFileContentReadBuf);
                    hFileContentReadBuf=NULL;

                    if (fMustRenameAfterClose && (dfError == DFS_SUCCESS))
                    {
                        BOOL fRenameNeeded = dfUnicodeStrcmp(pCurFileToExtract->filename_ondisk_previous_to_read, pCurFileToExtract->filename_ondisk_to_write) != 0 ;
                        if (fRenameNeeded)
                            if (!SvfMoveFile(pCurFileToExtract->filename_ondisk_previous_to_read, pCurFileToExtract->filename_ondisk_to_write))
                            {
                            dfError = DFS_ERROR_ERRORIO;
                            }

                        if (dfError == DFS_SUCCESS)
                        {
                            ChangeDateTimeFile(ProgressCallBackInfo.filename_onwork, &dfsTm);
                        }
                    }
                    if (dfError == DFS_SUCCESS)
                        if (!CallCallBack
                            (pProgressCallBack, &ProgressCallBackInfo,
                            DFCBM_AFTERCLOSINGPREVIOUSFILE, NULL))
                            dfError = DFS_STOP_REQUESTED;
                }
            }

            // TO DO BETTER 2014/05/16
            pCurFileToExtract->fCorrectlyDone = TRUE;
            if ((dfUncompressedSize != VALUE_UNKNOWN) && (pCurFileToExtract->fCorrectlyDone) && (!fNoWriteSizeControl))
                pCurFileToExtract->fCorrectlyDone =
                    (dfUncompressedSize == dfAllWrite);

            /* fix GV 2014/05/15 : add test for KIND_EXTRACTING_EXTRACTRAWSTREAMPATCHFILE
               (else, we compared CRC when compressed size and uncompressed size where same!)
             */
            if ((pDfsCrcInfoParam != NULL) &&
                (pCurFileToExtract->fCorrectlyDone) &&
                (pCurFileToExtract->KindExtracting != KIND_EXTRACTING_EXTRACTRAWSTREAMPATCHFILE))
            {
                if ((pDfsCrcInfoParam->dfBeginPos == 0)
                    && (pDfsCrcInfoParam->dfEndPos == dfAllWrite))
                {
                    BOOL dfIsGoodCrc = TRUE;
                    if (dfIsGoodCrc && fComputeCrc32)
                       dfIsGoodCrc = (pDfsCrcInfoParam->dfCrc32Value == dfCrc32);
                    if (dfIsGoodCrc && fComputeMD5 && pDfsCrcInfoParam->fMd5)
                       dfIsGoodCrc = DfsMemcmp(pDfsCrcInfoParam->bMd5,bMd5,16)==0;
                    if (dfIsGoodCrc && fComputeSHA1 && pDfsCrcInfoParam->fSha1)
                       dfIsGoodCrc = DfsMemcmp(pDfsCrcInfoParam->bSha1,bSha1,20)==0;
                    if (dfIsGoodCrc && fComputeSHA256 && pDfsCrcInfoParam->fSha256)
                        dfIsGoodCrc = DfsMemcmp(pDfsCrcInfoParam->bSha256, bSha256, 32) == 0;

                    if (!dfIsGoodCrc)
                    {
                        ERROR_INFO_ITEM eii;
                        dfuLong32 dfNbErrorItem = 0;
                        if (dfError == DFS_SUCCESS)
                            dfError = DFS_ERROR_BAD_CHECKSUM;

                        if (pCurFileToExtract->filename_ondisk_to_write != NULL)
                        {
                                eii.dfInfoTag = DFS_ERRORTAG_FILENAME;
                                eii.pInfo = (dfbytep)pCurFileToExtract->filename_ondisk_to_write;
                                eii.dfSize = (dfUnicodeStrlen(pCurFileToExtract->filename_ondisk_to_write) + 1) * 2;
                                dfNbErrorItem = 1;
                        }

                        if (pei != NULL)
                            if ((*pei) == NULL)
                                *pei = CreateErrorInfoBlock(DFS_ERROR_BAD_CHECKSUM, 0, 1, &eii);
                        pCurFileToExtract->fCorrectlyDone = FALSE;
                    }
                }
            }

            if (pCurFileToExtract->fCorrectlyDone)
                ewei.dfSuccess = 0;
            else
            {
                ewei.dfSuccess = (dfError == 0) ? 0xffffffff : dfError;
            }
        }

        if (pExtractingFileWorkingEvent!=NULL)
          (*pExtractingFileWorkingEvent)(EXTR_WORK_EVENT_EXTRACTING_FILE_FINISHED,&ewei,dfUserPtrWorkingEvent);

        if (pDfsCrcInfoParam != NULL)
          DfsFree(pDfsCrcInfoParam);
      }
    i++;
    pCurFileToExtract++;
  }

  //DfsRFreeAllDirTag(TagListDirCopy, psTagListFileInDirCopy, pei);

  if (dfError == DFS_SUCCESS)
    if (!CallCallBack
        (pProgressCallBack, &ProgressCallBackInfo,
         DFCBM_AFTERCLOSINGDIRECTORY, NULL))
      dfError = DFS_STOP_REQUESTED;

  DfsFree(BufAlloc);

  return dfError;
}

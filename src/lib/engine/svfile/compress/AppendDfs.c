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

/*
#ifdef _DEBUG
#define MSGOUTTEST
#endif
*/


#if defined(_DEBUG) && defined(MSGOUTTEST)
//#include <windows. h>
#endif

#if defined(_DEBUG) && defined(MSGOUTTEST)
#include <shlobj.h>
#endif
#include <stdio.h>



#include "../../patchstream/common/difbasic.h"

#include "../../patchstream/common/DfsTlTyp.h"
#include "../../patchstream/common/difstool.h"
#include "../common/DfsStruc.h"
#include "../common/DfsTagDf.h"
#include "../common/DfsTagMg.h"
#include "../common/DfsTagBlockFloatEnd.h"

#include "../common/DfsIntf.h"
#include "../common/DfsTagBlockFloatTool.h"
#include "../common/DfsSet.h"

#include "../../patchstream/common/DfsOrigMemoryMap.h"

#include "../../patchstream/rebuild/RamDifWk.h"
#include "../../patchstream/rebuild/RamDifTl.h"
#include "../../patchstream/rebuild/RamDifWS.h"

#include "DfsWrSet.h"


#include "../decompress/DfsRdSet.h"
#include "../common/ArrayTl.h"


#include "../../patchstream/common/DfsIoHlp.h"

#include "../common/DirSet.h"
#include "../decompress/DoExtracting.h"
#include "../../patchstream/common/ltoolsc.h"

#include "AppendDfs.h"




typedef struct
{
    dfuLong32 dwMinProgress;
    dfuLong32 dwMaxProgress;
    tSetExtractPosCallBack pSetExtractPosCallBack;
    dfvoidp dfuPtrSetExtractPosCallBack;

    tConfirmBeforeCreatingFile pConfirmBeforeCreatingFile;
    dfvoidp dfUserPtrBeforeCreatingFile;
} PROGRESSCBPARAMEAPPEND;

BOOL DFSCALLBACK ProgressCallBackDoAppendDfsWork(PROGRESSCALLBACKINFO * pProgressCallBackInfo)
{
    PROGRESSCBPARAMEAPPEND* ppcp;
    dfuLong32 dwProgressWidth ;

    if ((pProgressCallBackInfo->dfEvent==DFCBM_BEFOREOPENWORKINGFILE) && (!pProgressCallBackInfo->fWillIgnoreFile))
    {
        dfuLong32 dfConfirmResult = CONFIRM_BEFORE_CREATING_FILE_OK;
        ppcp = (PROGRESSCBPARAMEAPPEND*)(pProgressCallBackInfo->dfUserPtr);
        if ((ppcp->pConfirmBeforeCreatingFile != NULL) && (pProgressCallBackInfo->fTemporaryFilename!=TRUE))
            dfConfirmResult = ppcp->pConfirmBeforeCreatingFile(pProgressCallBackInfo->filename_onwork,NULL,
                                           ppcp->dfUserPtrBeforeCreatingFile);
        if (dfConfirmResult != CONFIRM_BEFORE_CREATING_FILE_OK)
            pProgressCallBackInfo->fWillIgnoreFile = TRUE;
        return (dfConfirmResult != CONFIRM_BEFORE_CREATING_FILE_STOP);
    }

    if ((pProgressCallBackInfo->dfEvent==DFCBM_PROGRESSWORKINGFILE) && (pProgressCallBackInfo->dfUserPtr != NULL))
    {
      ppcp = (PROGRESSCBPARAMEAPPEND*)(pProgressCallBackInfo->dfUserPtr);
      dwProgressWidth = ppcp->dwMaxProgress - ppcp->dwMinProgress;
      if (ppcp->dwMaxProgress>0)
          if ((pProgressCallBackInfo->dfDirOrigSize>0) && (dwProgressWidth > 0))
          {
              dfuLong32 dwPos ;
              dwPos = CalculateRatio(pProgressCallBackInfo->dfDirOrigDone,
                                     pProgressCallBackInfo->dfDirOrigSize,
                                     dwProgressWidth) +
                      (ppcp->dwMinProgress);

              //ppcp->pGuiItem ->SetProgressPos(dwPos);
              if (ppcp->pSetExtractPosCallBack!=NULL)
                ppcp->pSetExtractPosCallBack(dwPos,0,ppcp->dfuPtrSetExtractPosCallBack);
          }
    }
    return TRUE;
}


dfuLong32 SVFAPI DoAppendDfs(DFSFILE DfsFileWhereAppend,
                      dfuLong32 dfNbVersionOriginalInDfsWhereAppend,
                      /*
                      const PCDIRINFO * pDirInfoDfsFileWhereAppend,*/
                      DFSFILE DfsFileWhereRead,
                      /*dfuLong32 dfNbVersionOnDfsWhereRead_,*/
                      const PCDIRINFO * pDirInfoDfsFileWhereRead,
                      dfuLong32 dfFirstVersionToRead,
                      dfuLong32 dfNbVersionToRead,
                      dfuLong32 *dfFirstVersionPositionMap,dfuLong32 dfNbFileinFirstVersionPositionMap,
                      tSetExtractPosCallBack pSetExtractPosCallBack,
                      dfvoidp dfUserPtr, dfuLong32 dwMinProgress, dfuLong32 dwMaxProgress,
                      H_ERROR_INFO* pei)
{
    dfuLong32 i;
    dfuLong32 dfError = DFS_SUCCESS;
    dfuLong32 dfProgressStep = 0;

    PROGRESSCBPARAMEAPPEND pcp;

    pcp.pSetExtractPosCallBack = pSetExtractPosCallBack;
    pcp.dfuPtrSetExtractPosCallBack = dfUserPtr;

    pcp.pConfirmBeforeCreatingFile = NULL;
    pcp.dfUserPtrBeforeCreatingFile = NULL;

    if (dfNbVersionToRead>0)
        dfProgressStep = (dwMaxProgress - dwMinProgress) / (dfNbVersionToRead*2);

    pcp.dwMinProgress = dwMinProgress;
    pcp.dwMaxProgress = dwMinProgress + dfProgressStep;


    for (i=0;i<dfNbVersionToRead;i++)
    {
        FILETOEXTRACT *pFileToExtract = NULL;
        dfuLong32 j;

        const PCDIRINFO pCurDirReadInfo = *(pDirInfoDfsFileWhereRead+i+dfFirstVersionToRead);
        dfuLong32 dfNbFileInVersionInOldDfs = pCurDirReadInfo->dfNbFile;

        if (i==0)
            if (dfFirstVersionPositionMap == NULL)
                    dfError = DFS_ERROR_BAD_PARAMETER;

        if (dfError == DFS_SUCCESS)
        {
           pFileToExtract = (FILETOEXTRACT *)DfsMalloc(sizeof(FILETOEXTRACT) * (dfNbFileInVersionInOldDfs + 1));
           if (pFileToExtract == NULL)
               dfError = DFS_ERROR_MEMORY_ERROR;
        }
        for (j = 0; (j < dfNbFileInVersionInOldDfs) && (dfError == DFS_SUCCESS); j++)
        {
            //(pFileToExtract+j)->dfsInfoDate=NULL;
            (pFileToExtract + j)->fCorrectlyDone = TRUE;
            (pFileToExtract + j)->fIgnore = TRUE;
            (pFileToExtract + j)->filename_ondisk_previous_to_read = NULL;
            (pFileToExtract + j)->filename_ondisk_to_write = NULL;
            ///(pFileToExtract + j)->dfuSizeProjected = 0;


//            (pFileToExtract + j)->fRawExtracting = TRUE;
            (pFileToExtract + j)->KindExtracting = KIND_EXTRACTING_EXTRACTRAWSTREAMPATCHFILE;
            (pFileToExtract + j)->hRamDifWork = NULL;

            (pFileToExtract + j)->fSetNewDate = FALSE;
            (pFileToExtract + j)->fTemporaryFile=TRUE;
        }

        for (j = 0; (j < dfNbFileInVersionInOldDfs && (dfError == DFS_SUCCESS)); j++)
        {
            // we build an extraction map for directory iBrowsing in old DFS
            //const FILE_IN_NEW_DFS_INFO *pfindicur = (pdindicur->pfindi) + j;

            {
                dfwchar szTempFN[1024];
                dfuLong32 dfPosInOldDfsIBrowsing;

                const FILEINDIRINFO* pFileInDirInfo ;
                dfPosInOldDfsIBrowsing = j;

                pFileInDirInfo = (pCurDirReadInfo->pFileInDirInfo) + (dfPosInOldDfsIBrowsing);

                // size for temp is pFileInDirInfo->dfSize
                GetTemporaryFilename(GetUnicodeSVFPrefix(), szTempFN, (sizeof(szTempFN) / sizeof(dfwchar)) - 1,TRUE,
                                     pFileInDirInfo->dfFileEncodedSize);


#if defined(_DEBUG) && defined(MSGOUTTEST)
            lstrcatW(szTempFN,((pFileToExtract+dfPosInOldDfsIBrowsing)->fRawExtracting) ? L"_reMix_Raw" : L"_reMix_noraw");
            if (!(pFileToExtract+dfPosInOldDfsIBrowsing)->fRawExtracting)
            {
                lstrcatW(szTempFN, L"_!");
            }
#endif

                (pFileToExtract +dfPosInOldDfsIBrowsing)->filename_ondisk_to_write = dfUnicodeCopyAlloc(szTempFN);
                ///(pFileToExtract +dfPosInOldDfsIBrowsing)->dfuSizeProjected = fooo();

                (pFileToExtract + dfPosInOldDfsIBrowsing)->fIgnore = FALSE;
                // set TRUE in Extraction map
            }
        }



        if (dfError == DFS_SUCCESS)
        {
            dfuLong32 j;
            FILETOADD * pFileToAdd = (FILETOADD *) DfsMalloc(sizeof(FILETOADD) * (dfNbFileInVersionInOldDfs + 1));
            if (pFileToAdd == NULL)
                dfError = DFS_ERROR_MEMORY_ERROR;

            for (j = 0; j < dfNbFileInVersionInOldDfs; j++)
                if (pFileToAdd != NULL)
                    (pFileToAdd + j)->hAddTags = NULL;

            for (j = 0; (j < (dfNbFileInVersionInOldDfs) && (dfError == DFS_SUCCESS)); j++)
            {
                dfuLong32 dfPosInOldDfsIBrowsing = j;
                const FILEINDIRINFO* pFileInDirInfo ;
                FILETOADD* pCurFileToAdd = pFileToAdd + j;
                dfvoidp TagBuf;
                dfuLong32 TagSize;
                memset(pCurFileToAdd,0,sizeof(FILETOADD));
                pFileInDirInfo = (pCurDirReadInfo->pFileInDirInfo) + (dfPosInOldDfsIBrowsing);

                pCurFileToAdd->filename_tostore = pFileInDirInfo->FileName;
                pCurFileToAdd->filename_prevversionondisk=NULL;
                pCurFileToAdd->hRamDifToFlushPatch = NULL;
                pCurFileToAdd->fIgnore=FALSE;
                pCurFileToAdd->hAddTags = NULL;   //((pfsDest->pFileItem)+i)->fAddNewTag;

                if (GetTag(*(pCurDirReadInfo->TagFile + j), DFSTAG_STORAGESTATUS, &TagBuf, &TagSize))
                {
                    pCurFileToAdd->dfFileStatusForRaw = ConvertuLongIntelToLong(*(dfuLong32Intel *) TagBuf);
                }

                if (GetTag(*(pCurDirReadInfo->TagFile + j), DFSTAG_STORAGEPATCHINFO, &TagBuf, &TagSize))
                {
                    if (pCurFileToAdd->hAddTags == NULL)
                        pCurFileToAdd->hAddTags = AllocNewTagList();

                    if (pCurFileToAdd->hAddTags != NULL)
                        AddTag(pCurFileToAdd->hAddTags, DFSTAG_STORAGEPATCHINFO, TagBuf, TagSize);
                }


                if (pCurFileToAdd->dfFileStatusForRaw == DFS_STORAGESTATUS_IDENTICAL)
                {
                    if ((pFileToExtract + j)->filename_ondisk_to_write != NULL)
                        DfsFree((pFileToExtract + j)->filename_ondisk_to_write);
                    (pFileToExtract + j)->filename_ondisk_to_write = NULL;
                    (pFileToExtract + j)->fIgnore=TRUE;
                }

                pCurFileToAdd->dfPreviousVersionFilePosition = VALUE_UNKNOWN;
                if (GetTag(*(pCurDirReadInfo->TagFile + j), DFSTAG_PREVIOUSVERSIONINFO, &TagBuf, &TagSize))
                    if (TagSize >= sizeof(DFSPREVIOUSVERSIONINFO))
                    {
                        dfuLong32 dfPreviousVersionFileNumber, dfPreviousVersionFilePosition;
                        const DFSPREVIOUSVERSIONINFO *pDfsPreviousVersionInfo =
                                                (const DFSPREVIOUSVERSIONINFO *) TagBuf;


                        dfPreviousVersionFileNumber =
                            ConvertuLongIntelToLong(pDfsPreviousVersionInfo->dfPreviousVersionFileNumber);
                        dfPreviousVersionFilePosition =
                            ConvertuLongIntelToLong(pDfsPreviousVersionInfo->dfPreviousVersionFilePosition);

                        if (dfPreviousVersionFileNumber == 1)
                        {
                          if ((i==0) && (dfFirstVersionPositionMap != NULL))
                          {
                              if (dfPreviousVersionFilePosition > dfNbFileinFirstVersionPositionMap)
                              {
                                  dfError = DFS_ERROR_BAD_PARAMETER;
                              }
                              else
                                pCurFileToAdd->dfPreviousVersionFilePosition = *(dfFirstVersionPositionMap+dfPreviousVersionFilePosition);
                          }
                          else
                              pCurFileToAdd->dfPreviousVersionFilePosition = dfPreviousVersionFilePosition;
                        }
                    }

                pCurFileToAdd->fForceRecopyPrevious = (pCurFileToAdd->dfFileStatusForRaw) == DFS_STORAGESTATUS_IDENTICAL;


                if (GetTag(*(pCurDirReadInfo->TagFile + j), DFSTAG_DATE, &TagBuf, &TagSize))
                    if (TagSize == sizeof(DFSINFODATE))
                {
                    pCurFileToAdd->fForceDate = TRUE;
                    pCurFileToAdd->dfsInfoDate = *(DFSINFODATE *)TagBuf;
                }


                pCurFileToAdd->fWritingRaw = TRUE;

                pCurFileToAdd->dfForceRecopyOrRawCopySize = pFileInDirInfo ->dfSize;
                pCurFileToAdd->dfForceRecopyOrRawCopyCrc32 = pFileInDirInfo ->dfCrc32;

                pCurFileToAdd->fForceRecopyOrRawCopyMd5Present = pFileInDirInfo ->fMd5Filled ;
                pCurFileToAdd->fForceRecopyOrRawCopySha1Present = pFileInDirInfo ->fSha1Filled ;
                pCurFileToAdd->fForceRecopyOrRawCopySha256Present = pFileInDirInfo->fSha256Filled;

                if (pCurFileToAdd->fForceRecopyOrRawCopyMd5Present)
                    DfsMemcpy(pCurFileToAdd->bMd5,pFileInDirInfo->bMd5,16);

                if (pCurFileToAdd->fForceRecopyOrRawCopySha1Present)
                    DfsMemcpy(pCurFileToAdd->bSha1,pFileInDirInfo->bSha1,20);

                if (pCurFileToAdd->fForceRecopyOrRawCopySha256Present)
                    DfsMemcpy(pCurFileToAdd->bSha256, pFileInDirInfo->bSha256, 32);


/*
#define DFS_STORAGESTATUS_IDENTICAL (0x00000002)
#define DFS_STORAGESTATUS_MODIFIED  (0x00000003)
#define DFS_STORAGESTATUS_NEW       (0x00000004)
#define DFS_STORAGESTATUS_NEWSTORED (0x00000005)
*/


                {
                    // We are "stolen" the allocated temp filename with raw extracted content
                    pCurFileToAdd->filename_ondisk = (
                        (pFileToExtract + j)->filename_ondisk_to_write);


#if defined(_DEBUG) && defined(MSGOUTTEST)
                    printf("---> crc = %08x, size=%u, dfConstrinfo=%08x for file %ws\n",
                            pFileInDirInfo ->dfCrc32,pFileInDirInfo ->dfSize,
                            pfiodirawcur->dfConstructionInfo,pFileInDirInfo->FileName);
                    printf("        stored as %ws\n",pCurFileToAdd->filename_ondisk);
#endif
                }
            }

            //extract raw with the extraction map with ExtractDirectory (ExtractPatch show usage)

            if (dfError == DFS_SUCCESS)
                dfError = ExtractDirectory(DfsFileWhereRead, i+dfFirstVersionToRead,pCurDirReadInfo,
                                        dfNbFileInVersionInOldDfs,
                                        pFileToExtract,

                                        //NULL, NULL,
                                        ProgressCallBackDoAppendDfsWork, &pcp,

                                        NULL, NULL,
                                        FALSE, pei);

            pcp.dwMinProgress += dfProgressStep;
            pcp.dwMaxProgress += dfProgressStep;

            if (i == dfNbVersionToRead-1)
                pcp.dwMaxProgress = dwMaxProgress;

            if (dfError == DFS_SUCCESS)
                            dfError =
                                InsertDirectoryinDfsFile(DfsFileWhereAppend,
                                                TYPEDIR_PATCHFROMPREVIOUS,
                                                dfNbFileInVersionInOldDfs, pFileToAdd,TRUE,
                                                NULL, NULL,

                                                //pCprParam, ProgressCallBackDoRemixDfsWork, &pcp,
                                                NULL, ProgressCallBackDoAppendDfsWork, &pcp,

                                                pei);

            for (j = 0; j < dfNbFileInVersionInOldDfs; j++)
            {
                if ((pFileToAdd + j)->hAddTags != NULL)
                {
                    CloseTagList((pFileToAdd + j)->hAddTags);
                }
            }

            DfsFree(pFileToAdd);

            pcp.dwMinProgress += dfProgressStep;
            pcp.dwMaxProgress += dfProgressStep;

            if (dfError == DFS_SUCCESS)
                    DuplicateDirectoryFloatBlock(DfsFileWhereRead, DfsFileWhereAppend,
                                    i+dfFirstVersionToRead,i + dfNbVersionOriginalInDfsWhereAppend, pei);
        }
// cleanup to do


        if (pFileToExtract != NULL)
        {
            for (j = 0; (j < dfNbFileInVersionInOldDfs); j++)
            {
                dfwcharp filename_ondisk_to_write = (pFileToExtract + j)->filename_ondisk_to_write ;
                if (filename_ondisk_to_write != NULL)
                {
                    DeleteTempFile(filename_ondisk_to_write,NULL);
                    DfsFree(filename_ondisk_to_write);
                }
                //(pFileToExtract +dfPosInOldDfsIBrowsing)->filename_ondisk_to_write = dfUnicodeCopyAlloc(szTempFN);
            }

            DfsFree(pFileToExtract);
        }
    }

    return dfError;
}


typedef struct
{
    const FILEINDIRINFO* pFileInDirInfo;
    dfuLong32 dfPos;
} ITEM_FILE_IN_DIR_INFO;



long DFSCALLBACK fncCompareItemFileInDirInfo(const void *lpElem1, const void *lpElem2)
{
  const ITEM_FILE_IN_DIR_INFO *pfi1 = (const ITEM_FILE_IN_DIR_INFO *) lpElem1;
  const ITEM_FILE_IN_DIR_INFO *pfi2 = (const ITEM_FILE_IN_DIR_INFO *) lpElem2;
  int icmp;
  if (pfi1->pFileInDirInfo->dfSize < pfi2->pFileInDirInfo->dfSize)
      return -1;
  if (pfi1->pFileInDirInfo->dfSize > pfi2->pFileInDirInfo->dfSize)
      return 1;

  if (pfi1->pFileInDirInfo->dfCrc32 < pfi2->pFileInDirInfo->dfCrc32)
      return -1;
  if (pfi1->pFileInDirInfo->dfCrc32 > pfi2->pFileInDirInfo->dfCrc32)
      return 1;

  if ((pfi1 -> pFileInDirInfo -> fMd5Filled) && (pfi2 -> pFileInDirInfo -> fMd5Filled))
  {
      icmp = DfsMemcmp(pfi1  -> pFileInDirInfo -> bMd5, pfi2 -> pFileInDirInfo -> bMd5, 16);
      if (icmp != 0)
        return icmp;
  }

  if ((pfi1 -> pFileInDirInfo -> fSha1Filled) && (pfi2 -> pFileInDirInfo -> fSha1Filled))
  {
      icmp = DfsMemcmp(pfi1  -> pFileInDirInfo -> bSha1, pfi2 -> pFileInDirInfo -> bSha1, 20);
      if (icmp != 0)
        return icmp;
  }

  if ((pfi1 -> pFileInDirInfo -> fSha256Filled) && (pfi2 -> pFileInDirInfo -> fSha256Filled))
  {
      icmp = DfsMemcmp(pfi1  -> pFileInDirInfo -> bSha256, pfi2 -> pFileInDirInfo -> bSha256, 32);
      if (icmp != 0)
        return icmp;
  }

  icmp = dfUnicodeStrcmpi(pfi1->pFileInDirInfo -> FileName,pfi2->pFileInDirInfo -> FileName);
  return icmp;
}

BOOL DFSCALLBACK fncDestructorItemFileInDirInfo(const void *lpElem)
{
  return TRUE;
}

dfuLong32* SVFAPI GetPositionConversionMapList(PCDIRINFO pDirOriginal,PCDIRINFO pDirToConvert,dfuLong32* pdfNbItemConversionMapList)
{
    dfuLong32 i;
    dfuLong32* pRet;
    STATIC_ARRAY_C sa;
    if (pDirOriginal->dfNbFile > pDirToConvert-> dfNbFile)
        return NULL;
    pRet = (dfuLong32*)DfsMalloc(sizeof(dfuLong32) * (pDirOriginal->dfNbFile));
    if (pRet==NULL)
        return NULL;

    sa = InitStaticArray_C(sizeof(ITEM_FILE_IN_DIR_INFO),0x100);
    SetFuncCompareDataSA(sa, fncCompareItemFileInDirInfo);
    SetFuncDestructorSA(sa, fncDestructorItemFileInDirInfo);

    for (i=0;i<pDirToConvert->dfNbFile;i++)
    {
        ITEM_FILE_IN_DIR_INFO ifidiAdd;
        ifidiAdd.pFileInDirInfo = (pDirToConvert->pFileInDirInfo)+i;
        ifidiAdd.dfPos = i;
        InsertSortedSA(sa,&ifidiAdd);
    }

    for (i=0;i<pDirOriginal->dfNbFile;i++)
    {
        ITEM_FILE_IN_DIR_INFO ifidiSearch;
        const ITEM_FILE_IN_DIR_INFO *pFound;
        dfuLong32 dfPosFound;
        ifidiSearch.dfPos = 0;
        ifidiSearch.pFileInDirInfo = (pDirOriginal->pFileInDirInfo)+i;
        if (!FindSameElemPosSA(sa, &ifidiSearch, &dfPosFound))
            break;
        pFound = (const ITEM_FILE_IN_DIR_INFO *)GetElemPtrSA(sa,dfPosFound);
        *(pRet+i) = pFound -> dfPos;
    }

    DeleteStaticArray_C(sa);

    if (i != pDirOriginal->dfNbFile)
    {
        DfsFree(pRet);
        pRet=NULL;
    }


    if ((pRet != NULL) && (pdfNbItemConversionMapList!=NULL))
        *pdfNbItemConversionMapList = pDirOriginal->dfNbFile;

    return pRet;
}

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

#if defined(_DEBUG) && defined(MSGTEST) && (defined(WIN32) || defined(_WIN32))
#include <windows.h>
#define WIN_H_INCLUDED
#endif

//#include <shlobj.h>
#include <stdio.h>



#include "../../patchstream/common/difbasic.h"

#include "../../patchstream/common/DfsTlTyp.h"
#include "../../patchstream/common/difstool.h"
#include "../common/DfsStruc.h"
#include "../common/DfsTagDf.h"
#include "../common/DfsTagMg.h"
#include "../common/DfsTagBlockFloatEnd.h"

#include "../common/DfsIntf.h"

#include "../common/ArrayTl.h"


#include "../../patchstream/common/DfsOrigMemoryMap.h"
#include "../../patchstream/rebuild/RamDifWk.h"

#include "../../patchstream/common/DfsIoHlp.h"

#include "../common/DfsSet.h"
#include "DfsRdSet.h"

#include "zlib.h"
#include "zip.h"



#include "../common/DirSet.h"
#include "DoExtracting.h"


#define _TRYACCELERATE
#ifndef T_SET_EXTRACT_POS_CALLBACK_DEFINED
#define T_SET_EXTRACT_POS_CALLBACK_DEFINED
typedef BOOL(DFSCALLBACK * tSetExtractPosCallBack) (dfuLong32 dwPos,
                                                    dfvoidp dfpAdditionnalInfo,
                                                    dfvoidp dfUserPtr);
#endif

typedef enum
{
    PROGRESS_CALC_METHOD_BY_ORIG,
    PROGRESS_CALC_METHOD_BY_ENCODED,
    PROGRESS_CALC_METHOD_BY_ORIG_AND_ENCODED_ADDING,
    PROGRESS_CALC_METHOD_BY_ORIG_AND_ENCODED_EQUILIBRATE
} PROGRESS_CALC_METHOD;

typedef struct
{
    dfuLong32 dwMinProgress;
    dfuLong32 dwMaxProgress;
    tSetExtractPosCallBack pSetExtractPosCallBack;
    dfvoidp dfuPtrSetExtractPosCallBack;

    tConfirmBeforeCreatingFile pConfirmBeforeCreatingFile;
    dfvoidp dfUserPtrBeforeCreatingFile;

    PROGRESS_CALC_METHOD calcProgress;
} PROGRESSCBPARAMEP;

BOOL DFSCALLBACK ProgressCallBackDoExtracting(PROGRESSCALLBACKINFO * pProgressCallBackInfo)
{
    PROGRESSCBPARAMEP* ppcp;
    dfuLong32 dwProgressWidth ;

    if ((pProgressCallBackInfo->dfEvent==DFCBM_BEFOREOPENWORKINGFILE) && (!pProgressCallBackInfo->fWillIgnoreFile))
    {
        dfuLong32 dfConfirmResult = CONFIRM_BEFORE_CREATING_FILE_OK;
        ppcp = (PROGRESSCBPARAMEP*)(pProgressCallBackInfo->dfUserPtr);
        if ((ppcp->pConfirmBeforeCreatingFile != NULL) && (pProgressCallBackInfo->fTemporaryFilename!=TRUE))
            dfConfirmResult = ppcp->pConfirmBeforeCreatingFile(pProgressCallBackInfo->filename_onwork,NULL,
                                           ppcp->dfUserPtrBeforeCreatingFile);
        if (dfConfirmResult != CONFIRM_BEFORE_CREATING_FILE_OK)
            pProgressCallBackInfo->fWillIgnoreFile = TRUE;
        return (dfConfirmResult != CONFIRM_BEFORE_CREATING_FILE_STOP);
    }

    if ((pProgressCallBackInfo->dfEvent==DFCBM_PROGRESSWORKINGFILE) && (pProgressCallBackInfo->dfUserPtr != NULL))
    {
      ppcp = (PROGRESSCBPARAMEP*)(pProgressCallBackInfo->dfUserPtr);
      dwProgressWidth = ppcp->dwMaxProgress - ppcp->dwMinProgress;
      if (ppcp->dwMaxProgress>0)
          if ((pProgressCallBackInfo->dfDirOrigSize>0) && (dwProgressWidth > 0))
          {
              dfuLong32 dwPos ;
              dfuLong64 dfDone=0;
              dfuLong64 dfSize=0;
              switch(ppcp->calcProgress)
              {
              case PROGRESS_CALC_METHOD_BY_ORIG:
                  dfDone=pProgressCallBackInfo->dfDirOrigDone;
                  dfSize=pProgressCallBackInfo->dfDirOrigSize;
                  break;

              case PROGRESS_CALC_METHOD_BY_ENCODED:
                  dfDone=pProgressCallBackInfo->dfDirEncodedDone;
                  dfSize=pProgressCallBackInfo->dfDirEncodedSize;
                  break;

              case PROGRESS_CALC_METHOD_BY_ORIG_AND_ENCODED_ADDING:
                  dfDone = pProgressCallBackInfo->dfDirOrigDone + pProgressCallBackInfo->dfDirEncodedDone ;
                  dfSize = pProgressCallBackInfo->dfDirOrigSize + pProgressCallBackInfo->dfDirEncodedSize ;
                  break;

              default:
                  break;
              }

              if (ppcp->calcProgress==PROGRESS_CALC_METHOD_BY_ORIG_AND_ENCODED_EQUILIBRATE)
              {
                  dwPos =
                      CalculateRatio(pProgressCallBackInfo->dfDirOrigDone,pProgressCallBackInfo->dfDirOrigSize,dwProgressWidth/2) +
                      CalculateRatio(pProgressCallBackInfo->dfDirEncodedDone,pProgressCallBackInfo->dfDirEncodedSize,dwProgressWidth - (dwProgressWidth/2)) +
                       (ppcp->dwMinProgress);
              }
              else
              {
                  dwPos = CalculateRatio(dfDone,dfSize,dwProgressWidth) + (ppcp->dwMinProgress);
              }

              //ppcp->pGuiItem ->SetProgressPos(dwPos);
              if (ppcp->pSetExtractPosCallBack!=NULL)
                ppcp->pSetExtractPosCallBack(dwPos,0,ppcp->dfuPtrSetExtractPosCallBack);
          }
    }
    return TRUE;
}

BOOL ApplyExtractingMap(FILESET* pfs,dfuLong32 dwMapFileNumber,const EXTRACTINGMAPITEM* lpExtractingMap,BOOL fRevert,BOOL fInPlaceAlwaysCombine,BOOL*pfUsingExtractInPlace)
{
    BOOL fUsingExtractInPlace=FALSE;
    BOOL fRet=FALSE;
    dfuLong32 i;
    if ((lpExtractingMap!=NULL) && (pfs!=NULL))
        if (dwMapFileNumber==pfs->dfNbFileItem)
    {
        fRet=TRUE;
        for (i=0;i<pfs->dfNbFileItem;i++)
        {
            EXTRACTINGMAPITEM emi = *(lpExtractingMap+i);
            if (fRevert)
            {
                if (emi == ExtractClassic)
                    ((pfs->pFileItem)+i)->ExtAction=ExtActionExtractContent;
                if (emi == ExtractInPlace)
                {
                    ((pfs->pFileItem)+i)->ExtAction=ExtActionExtractContentInPlace;
                    fUsingExtractInPlace=TRUE;
                }
                if (emi == ExtractInPlaceNoChecksum)
                {
                    ((pfs->pFileItem)+i)->ExtAction=ExtActionExtractContentInPlaceNoChecksum;
                    fUsingExtractInPlace=TRUE;
                }

                if (emi == ExtractByMerging)
                {
                    ((pfs->pFileItem) + i)->ExtAction = ExtActionExtractContentByMerging;
                    fUsingExtractInPlace=TRUE;
                }
                // for TODO 2014/08/27 : change ExtActionExtractContentInPlace and ExtActionExtractContentInPlaceNoChecksum to ExtActionMergeRamDif
                if ((fInPlaceAlwaysCombine) && ((emi == ExtractInPlace) || (emi == ExtractInPlaceNoChecksum)))
                {
                    ((pfs->pFileItem) + i)->ExtAction = ExtActionMergeRamDif;
                }
            }
            else
            {
                if (emi == ExtractNone)
                    ((pfs->pFileItem)+i)->ExtAction=ExtActionIgnore;
                if ((emi == ExtractInPlace) && (((pfs->pFileItem)+i)->ExtAction==ExtActionExtractContent))
                {
                    ((pfs->pFileItem)+i)->ExtAction=ExtActionExtractContentInPlace;
                    fUsingExtractInPlace=TRUE;
                }

                if ((emi == ExtractInPlaceNoChecksum) && (((pfs->pFileItem) + i)->ExtAction == ExtActionExtractContent))
                {
                    ((pfs->pFileItem) + i)->ExtAction = ExtActionExtractContentInPlaceNoChecksum;
                    fUsingExtractInPlace = TRUE;
                }
                if ((emi == ExtractByMerging) && (((pfs->pFileItem) + i)->ExtAction == ExtActionExtractContent))
                {
                    ((pfs->pFileItem) + i)->ExtAction = ExtActionExtractContentByMerging;
                    fUsingExtractInPlace = TRUE;
                }
            }
        }
    }
    if (pfUsingExtractInPlace!=NULL)
        *pfUsingExtractInPlace=fUsingExtractInPlace;
    return fRet;
}


BOOL SVFAPI DoMultiExtracting(DFSFILE DfsFile,dfwcharpc wchBaseDirExtract,
                       FILESET** ppfsDest,BOOL fTempDestExtr,dfuLong32 dfDirExtr,const PDIRINFO* pDirInfo,
                       BOOL fBaseDirectorySelected,dfuLong32 dfBaseDirNum,dfwcharpc wchBaseDirectory,const FILESET* pFileSetBase,
                       dfuLong32 dwMapFileNumber,const EXTRACTINGMAPITEM* lpExtractingMap,
                       EXTRACTINGMAPINFO* pExtractingMapInfo,
                       tSetExtractPosCallBack pSetExtractPosCallBack,dfvoidp dfUserPtr,
                       tConfirmBeforeCreatingFile pConfirmBeforeCreatingFile, dfvoidp dfUserPtrBeforeCreatingFile,
                       tExtractingFileWorkingEvent pExtractingFileWorkingEvent, dfvoidp dfUserExtractingFileWorkingEvent,
                       dfuLong32 dwMinProgress,dfuLong32 dwMaxProgress,BOOL fFlatExtracting, BOOL fFlushWrite, H_ERROR_INFO * pei)
{
    //BOOL fBaseOk = FALSE;
    BOOL fBaseNeeded = FALSE;
    dfuLong32 dfDirStartExtr=0;
    BOOL fUseCombine=TRUE;



    #if defined(_DEBUG) && defined(MSGTEST)
    HWND hWndMain=GetDesktopWindow();
    #endif


    if (ppfsDest != NULL)
        *ppfsDest = NULL;

    fBaseNeeded = ((*pDirInfo)->dfTypeDir == TYPEDIR_FILECRCONLY);
    if (fBaseNeeded)
    {
        /* we need dir selected */
        BOOL fBaseOk = FALSE;
        if (fBaseDirectorySelected)
            if (dfBaseDirNum <= dfDirExtr)
            {
                fBaseOk = TRUE;
                dfDirStartExtr = dfBaseDirNum;
            }

       if (!fBaseOk)
       {
           return FALSE;
       }
    }

    if (pExtractingMapInfo != NULL)
    {
        EXTRACTINGMAPINFO* pExtractingMapInfoLast = pExtractingMapInfo +
                                               (dfDirExtr-dfBaseDirNum);
        dwMapFileNumber = pExtractingMapInfoLast->dfMapFileNumber;
        lpExtractingMap = pExtractingMapInfoLast->pExtractingMap;
    }

    //ConvertTCharToUnicode(pszBaseDirExtract,(dfwcharp)wchBaseDirExtract,lstrlen(pszBaseDirExtract)+0x8);

    /* ((fBaseNeeded) && (dfDirStartExtr == dfDirExtr)) : just do a copy */
    if ((fBaseNeeded) && (dfDirStartExtr == dfDirExtr))
    {
        dfuLong32 dfError = DFS_SUCCESS;
        FILESET* pfsOrg = NULL;
        FILESET* pfsDest = NULL;
        dfuLong32 dfTypeDir = 0;
        BOOL fAllIsGood1=FALSE;
        BOOL fAllIsGood2=FALSE;
        BOOL fRet=FALSE;

        pfsOrg = (FILESET*)DfsMalloc(sizeof(FILESET));
        InitFileSet(pfsOrg);
        pfsDest = (FILESET*)DfsMalloc(sizeof(FILESET));
        InitFileSet(pfsDest);

        dfError = CreateFileSetForVersionInDirectory(DfsFile,pfsOrg,
                            dfDirExtr,wchBaseDirectory,pFileSetBase,
                            *(pDirInfo+dfDirExtr),
                            &fAllIsGood1,FALSE,TRUE,TRUE,TRUE,&dfTypeDir,FALSE,FALSE,pei);

        if (dfError == DFS_SUCCESS)
          dfError = CreateFileSetForVersionInDirectory(DfsFile,pfsDest,
                            dfDirExtr,wchBaseDirExtract,pFileSetBase,
                            *(pDirInfo+dfDirExtr),
                            &fAllIsGood2,FALSE,!fTempDestExtr,FALSE,FALSE,&dfTypeDir,FALSE,fFlatExtracting,pei);
        ApplyExtractingMap(pfsDest,dwMapFileNumber,lpExtractingMap,FALSE,FALSE,NULL);

        if ((dfError == DFS_SUCCESS) && fAllIsGood1 && fAllIsGood2 && (pfsOrg->dfNbFileItem==pfsDest->dfNbFileItem))
        {
            dfuLong32 i;
            DFSINFODATE dfsInfoDate;
            BOOL fDateRead=FALSE;
            DFSTM dfsTm;
            fRet = TRUE;

            for (i=0;(i<pfsDest->dfNbFileItem);i++)
                if (((pfsDest->pFileItem + i)->ExtAction)!=ExtActionIgnore)
            {
                if (dfUnicodeStrcmpi((pfsOrg->pFileItem+i)->FileNameOnArchive,(pfsDest->pFileItem+i)->FileNameOnArchive) !=0)
                {
                    fRet=FALSE;
                    break;
                }


                {
                    PDIRINFO pDirExtInfo = *(pDirInfo+ dfDirExtr);
                    dfvoidp pTagBuf;
                    dfuLong32 TagSize;

                    if (GetTag(*((pDirExtInfo->TagFile)+i), DFSTAG_DATE, &pTagBuf, &TagSize))
                    {
                        fDateRead = (TagSize>=sizeof(DFSINFODATE));
                        if (fDateRead)
                            dfsInfoDate = *((DFSINFODATE*)pTagBuf);

                        {
                        ConvertDfsInfoDateToDfsTm(&dfsInfoDate,&dfsTm);
                        #if ((!defined(_WINDOWS)) && ((defined(_DEBUG)) && defined(OUTTEXT)))
                        printf(" %02u/%02u/%04u %02u:%02u:%02u ",dfsTm.df_mday,dfsTm.df_mon,dfsTm.df_year,
                                dfsTm.df_hour,dfsTm.df_min,dfsTm.df_sec);
                        #endif
                        }
                    }
                }

                if ((pfsDest->pFileItem+i)->FileNameOnDisk==NULL)
                {
                  if ((pfsOrg->pFileItem + i)->fTempFile == TRUE)
                  {
                      dfwchar szTempFN[1024];

#if ((defined(WIN32) || defined(_WIN32))) && defined(WIN_H_INCLUDED)
                      MessageBox(0,"This message box must never be seen.\nYou see it, this mean Gilles Vollant do not understand his own code","Internal",MB_ICONERROR|MB_OK);
#endif
                      GetTemporaryFilename(GetUnicodeSVFPrefix(),szTempFN,(sizeof(szTempFN) / sizeof(dfwchar))-1,FALSE,
                          (((*(pDirInfo+ dfDirExtr))->pFileInDirInfo)+i)->dfSize);

#if  ((defined(WIN32) || defined(_WIN32))) && defined(WIN_H_INCLUDED) && defined(_DEBUG)
                      //lstrcatW(szTempFN,((pFileToExtract+dfPosInOldDfsIBrowsing)->fRawExtracting) ? L"_reMix_Raw" : L"_reMix_noraw");
                      lstrcatW(szTempFN,L"_DoMultiExtracting");
#endif

                      (pfsDest->pFileItem + i)->FileNameOnDisk =
                        dfUnicodeCopyConcatAlloc(szTempFN, NULL);
                      (pfsDest->pFileItem + i)->fTempFile = TRUE;

                      if (!MyCopyFile((pfsOrg->pFileItem+i)->FileNameOnDisk,(pfsDest->pFileItem+i)->FileNameOnDisk,FALSE,fFlushWrite,pei))
                      {
                          fRet=FALSE;
                          break;
                      }
                      else
                      if (fDateRead)
                        ChangeDateTimeFile((pfsDest->pFileItem+i)->FileNameOnDisk,&dfsTm);
                  }
                  else
                  {
                      (pfsDest->pFileItem+i)->FileNameOnDisk =
                        dfUnicodeCopyConcatAlloc((pfsOrg->pFileItem+i)->FileNameOnDisk, NULL);
                      (pfsDest->pFileItem + i)->fTempFile = FALSE;
                      (pfsDest->pFileItem + i)->fIdenticalPreviousVersion=FALSE;
                  }
                }
                else
                {
                    if (!MyCopyFile((pfsOrg->pFileItem+i)->FileNameOnDisk,(pfsDest->pFileItem+i)->FileNameOnDisk,FALSE,fFlushWrite,pei))
                    {
                        fRet=FALSE;
                        break;
                    }
                    else
                      if (fDateRead)
                        ChangeDateTimeFile((pfsDest->pFileItem+i)->FileNameOnDisk,&dfsTm);
                }
            }

            if (dfError==DFS_SUCCESS)
                fRet=TRUE;
        }



        FreeFileSet(pfsOrg,FALSE);
        DfsFree(pfsOrg);

        if ((ppfsDest != NULL) && fRet)
            *ppfsDest = pfsDest;
        else
        {
          FreeFileSet(pfsDest,FALSE);
          DfsFree(pfsDest);
        }

        #if defined(_DEBUG) && defined(MSGTEST)
        if (!fRet)
            if (hwndMain!=NULL)
              MessageBox(hwndMain,"We will just copy file from source dir",fRet ? "TRUE":"FALSE",MB_OK|MB_ICONERROR);
        #endif

        if ((fRet) && (dwMaxProgress!=0) && (pSetExtractPosCallBack!=NULL))
            pSetExtractPosCallBack(dwMaxProgress,NULL,dfUserPtr);
            //guiItem.SetProgressPos(dwMaxProgress);

        return fRet;
    }

    /* ((!fBaseNeeded) && (dfDirExtr == 0)) : just one unzipping */
    if ((!fBaseNeeded) && (dfDirExtr == 0))
    {
        dfuLong32 dfError = DFS_SUCCESS;
        FILESET* pfsDest = NULL;
        dfuLong32 dfTypeDir = 0;
        BOOL fAllIsGood=FALSE;
        BOOL fRet=FALSE;

        pfsDest = (FILESET*)DfsMalloc(sizeof(FILESET));
        InitFileSet(pfsDest);
        dfError = CreateFileSetForVersionInDirectory(DfsFile,pfsDest,
                            dfDirExtr,wchBaseDirExtract,pFileSetBase,
                            *(pDirInfo+dfDirExtr),
                            &fAllIsGood,FALSE,!fTempDestExtr,FALSE,FALSE,&dfTypeDir,FALSE,fFlatExtracting,pei);
        ApplyExtractingMap(pfsDest,dwMapFileNumber,lpExtractingMap,FALSE,FALSE,NULL);

        if ((dfError == DFS_SUCCESS) && (fAllIsGood==TRUE))
        {

            PROGRESSCBPARAMEP pcp;
            pcp.dwMinProgress = dwMinProgress;
            pcp.dwMaxProgress = dwMaxProgress;
            //pcp.pGuiItem = &guiItem;
            pcp.pSetExtractPosCallBack = pSetExtractPosCallBack;
            pcp.dfuPtrSetExtractPosCallBack = dfUserPtr;
            pcp.calcProgress=PROGRESS_CALC_METHOD_BY_ORIG;

              if (pfsDest->fTempFile)
              {
                pcp.pConfirmBeforeCreatingFile = NULL;
                pcp.dfUserPtrBeforeCreatingFile = NULL;
              }
              else
              {
                pcp.pConfirmBeforeCreatingFile = pConfirmBeforeCreatingFile;
                pcp.dfUserPtrBeforeCreatingFile = dfUserPtrBeforeCreatingFile;
              }

            dfError=ExtractPatch(DfsFile,NULL,
                pfsDest,dfDirExtr,*(pDirInfo+dfDirExtr),
                fTempDestExtr,fTempDestExtr,fFlushWrite /* && (!fTempDestExtr) */,
                wchBaseDirExtract,
                ProgressCallBackDoExtracting, &pcp,
                pExtractingFileWorkingEvent, dfUserExtractingFileWorkingEvent,
                pei);
            if (dfError==DFS_SUCCESS)
                fRet=TRUE;
        }


        if ((ppfsDest != NULL) && fRet)
            *ppfsDest = pfsDest;
        else
        {
          FreeFileSet(pfsDest,FALSE);
          DfsFree(pfsDest);
        }


        #if defined(_DEBUG) && defined(MSGTEST)
        if (!fRet)
            if (hwndMain!=NULL)
              MessageBox(hwndMain,"We have just extract one without patching",fRet ? "TRUE":"FALSE",MB_OK|MB_ICONERROR);
        #endif

        if ((fRet) && (dwMaxProgress!=0) && (pSetExtractPosCallBack!=NULL))
            pSetExtractPosCallBack(dwMaxProgress,NULL,dfUserPtr);
            //guiItem.SetProgressPos(dwMaxProgress);

        return fRet;
    }


    /* (dfDirExtr>=dfDirStartExtr) : apply several patching */
    if (dfDirExtr>=dfDirStartExtr)
    {
        dfuLong32 dfError = DFS_SUCCESS;
        FILESET* pfsOrg = NULL;
        FILESET* pfsDest = NULL;
        dfuLong32 dfTypeDir = 0;
        //BOOL fAllIsGood1=FALSE;
        //BOOL fAllIsGood2=FALSE;
        BOOL fRet=TRUE;
        BOOL fFileOrgToDelete;
        dfuLong32 i;
        BOOL fUsingInPlaceExtracting = FALSE;

        #ifdef _TRYACCELERATE
        FILESET** ppFileSetCollection;
        ppFileSetCollection = (FILESET**)DfsMalloc((dfDirExtr+1)*(sizeof(FILESET*)));

        for (i=0;i<=dfDirExtr;i++)
          *(ppFileSetCollection+i)=NULL;

        /* the main extracting loop */
        for (i=dfDirStartExtr;i<=dfDirExtr;i++)
        {
            BOOL fFirstBase = ((i==dfDirStartExtr) && (fBaseNeeded));
            BOOL fLastFinal = (i==dfDirExtr);
            BOOL fAllIsGood=FALSE;
            FILESET* pfsCur;

            pfsCur=(FILESET*)DfsMalloc(sizeof(FILESET));
            *(ppFileSetCollection+i)=pfsCur;

            InitFileSet(pfsCur);

            dfError =
                CreateFileSetForVersionInDirectory(DfsFile,pfsCur,
                                    i,
                                    fFirstBase ? wchBaseDirectory : wchBaseDirExtract,
                                    fFirstBase ? pFileSetBase : NULL,
                                    *(pDirInfo+i),
                                    &fAllIsGood,FALSE,
                                    (fLastFinal) ? (!fTempDestExtr) : (FALSE),
                                    fFirstBase,fFirstBase,&dfTypeDir,FALSE,
                                    fFirstBase ? FALSE:fFlatExtracting,pei);
            if ((dfError != DFS_SUCCESS) || (!fAllIsGood))
            {
                fRet=FALSE;
                break;
            }
            if ((fRet) && (i==dfDirExtr))
              ApplyExtractingMap(pfsCur,dwMapFileNumber,lpExtractingMap,FALSE,FALSE,&fUsingInPlaceExtracting);
        }

        /* Now, we revert the Extracting map */
        //// TODO 2014/08/27 : change ExtActionExtractContentInPlace and ExtActionExtractContentInPlaceNoChecksum to ExtActionMergeRamDif - obsolete comment
        for (i=dfDirExtr;(i>dfDirStartExtr) && fRet;i--)
        {
            dfuLong32 j;
            FILESET* pfsDest = *(ppFileSetCollection+i);
            FILESET* pfsPrevious = *(ppFileSetCollection+(i-1));
            for (j=0;j<pfsPrevious->dfNbFileItem;j++)
                ((pfsPrevious->pFileItem)+j)->ExtAction=ExtActionIgnore;

            if ((pExtractingMapInfo != NULL) && (i-1>=dfBaseDirNum))
            {
                EXTRACTINGMAPINFO* pExtractingMapCur = pExtractingMapInfo +
                                                        ((i-1)-dfBaseDirNum);
                ApplyExtractingMap(pfsPrevious,
                                   pExtractingMapCur->dfMapFileNumber,
                                   pExtractingMapCur->pExtractingMap,
                                   TRUE,TRUE,NULL);
            }

            for (j = 0; j < pfsDest->dfNbFileItem; j++)
            {
                dfuLong32 dfPrevious = ((pfsDest->pFileItem) + j)->dfPreviousVersion;
                EXTACTION ExtAction = ((pfsDest->pFileItem) + j)->ExtAction;
                BOOL fIgnore = (ExtAction == ExtActionIgnore);
                BOOL fSet = FALSE;

                /* * /

                ExtActionExtractContent,
                    ExtActionMergeRamDif,
                    ExtActionExtractContentInPlace,
                    ExtActionExtractContentInPlaceNoChecksum
                    */

                if (dfPrevious < pfsPrevious->dfNbFileItem)
                {
                    if (ExtAction == ExtActionExtractContent)
                    {
                        fSet = TRUE;
                        ((pfsPrevious->pFileItem) + dfPrevious)->ExtAction = ExtActionExtractContent;
                    }
                    if (ExtAction == ExtActionMergeRamDif)
                    {
                        fSet = TRUE;
                        ((pfsPrevious->pFileItem) + dfPrevious)->ExtAction = ExtActionMergeRamDif;
                    }
                    if (ExtAction == ExtActionExtractContentInPlace)
                    {
                        fSet = TRUE;
                        ((pfsPrevious->pFileItem) + dfPrevious)->ExtAction = ExtActionMergeRamDif;
                    }
                    if (ExtAction == ExtActionExtractContentInPlaceNoChecksum)
                    {
                        fSet = TRUE;
                        ((pfsPrevious->pFileItem) + dfPrevious)->ExtAction = ExtActionMergeRamDif;
                    }
                    if (ExtAction == ExtActionExtractContentByMerging)
                    {
                        fSet = TRUE;
                        ((pfsPrevious->pFileItem) + dfPrevious)->ExtAction = ExtActionMergeRamDif;
                    }
                    if ((dfPrevious != VALUE_UNKNOWN) && (!fIgnore) && (!fSet))
                    {
                        ((pfsPrevious->pFileItem) + dfPrevious)->ExtAction = ExtActionExtractContent;
                    }
                }
            }
        }
        #endif


        /*
        DEBUG code
        */

        for (i=dfDirStartExtr;(i<=dfDirExtr) && fRet;i++)
        {
            BOOL fTempDest;
            BOOL fAllIsGood=FALSE;
            fFileOrgToDelete = TRUE;



            /* the Combine stuff */
            if ((i>dfDirStartExtr) && (i<dfDirExtr) && (fUseCombine))
            {
                PDIRINFO pCurDirInfo = *(pDirInfo+i);
                FILESET* pfsCur = *(ppFileSetCollection+i);
                if (((*(pDirInfo+i))->dfTypeDir) == TYPEDIR_PATCHFROMPREVIOUS)
                {
                    dfuLong32 j;
                    for (j=0;j<pfsCur->dfNbFileItem;j++)
                    {
                        //const FILEINDIRINFO *pfi =((*(pDirInfo+i))->pFileInDirInfo)+j;
                        FILEITEM* pFileItemDest = (pfsCur->pFileItem)+j;
                        dfvoidp TagBuf;
                        dfuLong32 TagSize;

                          if (GetTag
                              (*(pCurDirInfo->TagFile + j), DFSTAG_STORAGESTATUS, &TagBuf, &TagSize))
                          {
                            dfuLong32 dfFileIdentical =
                              ConvertuLongIntelToLong(*(dfuLong32Intel *) TagBuf);
                            if ((dfFileIdentical == DFS_STORAGESTATUS_IDENTICAL) || (dfFileIdentical == DFS_STORAGESTATUS_MODIFIED))
                              pFileItemDest->ExtAction=ExtActionMergeRamDif;
                          }
                    }
                }
            }


            if ((i==dfDirStartExtr) && (fBaseNeeded))
            {
                BOOL fAllIsGood = FALSE;
                i++;

                #ifdef _TRYACCELERATE
                fAllIsGood = TRUE;
                pfsOrg = *(ppFileSetCollection+dfDirStartExtr);
                *(ppFileSetCollection+dfDirStartExtr)=NULL;
                #else
                pfsOrg = (FILESET*)DfsMalloc(sizeof(FILESET));
                InitFileSet(pfsOrg);
                dfError = CreateFileSetForVersionInDirectory(DfsFile,pfsOrg,
                                    dfDirStartExtr/*dfDirExtr*/,wchBaseDirectory,pFileSetBase,
                                    &fAllIsGood,FALSE,!fTempDestExtr,TRUE,TRUE,&dfTypeDir,FALSE);
                #endif

                if ((dfError != DFS_SUCCESS) || (!fAllIsGood))
                {
                    fRet=FALSE;
                    break;
                }
                fFileOrgToDelete = FALSE;
            }
            else pfsOrg = pfsDest ; // Get Previous Dest

/* do build and extradt pfsDest */


            fTempDest = (i==dfDirExtr) ? fTempDestExtr : TRUE;
            #ifdef _TRYACCELERATE
            fAllIsGood = TRUE;
            pfsDest = *(ppFileSetCollection+i);
            *(ppFileSetCollection+i)=NULL;
            #else
            pfsDest = (FILESET*)DfsMalloc(sizeof(FILESET));
            InitFileSet(pfsDest);
            if (!fTempDest)
                dfError = CreateFileSetForVersionInDirectory(DfsFile,pfsDest,
                                    i,wchBaseDirExtract,
                                    &fAllIsGood,FALSE,!fTempDest,FALSE,FALSE,&dfTypeDir,fFlatExtracting);
            else
                dfError = CreateFileSetForVersionInDirectory(DfsFile,pfsDest,
                                    i,wchBaseDirExtract,
                                    &fAllIsGood,FALSE,!fTempDest,FALSE,FALSE,&dfTypeDir,fFlatExtracting);
            if (i==dfDirExtr)
              ApplyExtractingMap(pfsDest,dwMapFileNumber,lpExtractingMap,FALSE,NULL);
            #endif

            pfsDest->fTempFile = fTempDest;


            if ((dfError != DFS_SUCCESS) || (!fAllIsGood))
            {
                fRet=FALSE;
                break;
            }

            {

              PROGRESSCBPARAMEP pcp;
              pcp.dwMinProgress = pcp.dwMaxProgress = 0;
              pcp.calcProgress=PROGRESS_CALC_METHOD_BY_ORIG;
              if (fUsingInPlaceExtracting)
                 pcp.calcProgress=PROGRESS_CALC_METHOD_BY_ORIG_AND_ENCODED_EQUILIBRATE;
              if ((dwMaxProgress!=0))
              {
                  dfuLong32 nbVersionToProcess=(dfDirExtr+1)-dfDirStartExtr;
                  dfuLong32 posVersionToProcessInVersionSet=i-dfDirStartExtr;
                  if (fBaseNeeded)
                  {
                      nbVersionToProcess--;
                      if (posVersionToProcessInVersionSet>0)
                         posVersionToProcessInVersionSet--;
                  }
                  pcp.dwMinProgress = dwMinProgress +
                                      (((dwMaxProgress-dwMinProgress)*(posVersionToProcessInVersionSet))/(nbVersionToProcess));
                  pcp.dwMaxProgress = dwMinProgress +
                                      (((dwMaxProgress-dwMinProgress)*(posVersionToProcessInVersionSet+1))/(nbVersionToProcess));
              }
              pcp.pSetExtractPosCallBack = pSetExtractPosCallBack;
              pcp.dfuPtrSetExtractPosCallBack = dfUserPtr;
              if (pfsDest->fTempFile)
              {
                pcp.pConfirmBeforeCreatingFile = NULL;
                pcp.dfUserPtrBeforeCreatingFile = NULL;
              }
              else
              {
                pcp.pConfirmBeforeCreatingFile = pConfirmBeforeCreatingFile;
                pcp.dfUserPtrBeforeCreatingFile = dfUserPtrBeforeCreatingFile;
              }
              //pcp.pGuiItem = &guiItem;
              {
                BOOL fOldFileCanBeCapturedInExtractPatch = fTempDest;
                dfError = ExtractPatch(DfsFile,pfsOrg,pfsDest,i,*(pDirInfo+i),
                                     fTempDest,fOldFileCanBeCapturedInExtractPatch,fFlushWrite /* && (!fTempDest) */,
                                     wchBaseDirExtract,ProgressCallBackDoExtracting, &pcp,
                                     pExtractingFileWorkingEvent, dfUserExtractingFileWorkingEvent,
                                     pei);
              }
            }

            if (pfsOrg != NULL)
            {
              FreeFileSet(pfsOrg,fFileOrgToDelete);
              DfsFree(pfsOrg);
            }

            if ((dfError != DFS_SUCCESS))
            {
                fRet = (dfError == DFS_STOP_REQUESTED);
                break;
            }
            if ((fRet) && (dwMaxProgress!=0) && (dfDirExtr+1>dfDirStartExtr))
            {
               dfuLong32 dwNewPos;
               dwNewPos = dwMinProgress +
                                      (((dwMaxProgress-dwMinProgress)*((i+1)-dfDirStartExtr))/((dfDirExtr+1)-dfDirStartExtr));
               #if defined(_DEBUG) && defined(WIN_H_INCLUDED)
               OutputDebugString("$");
               #endif

               //guiItem.SetProgressPos(dwNewPox);
               if (pSetExtractPosCallBack!=NULL)
                   pSetExtractPosCallBack(dwNewPos,0,dfUserPtr);
            }
        }

        if ((ppfsDest != NULL) && fRet)
            *ppfsDest = pfsDest;
        else
        {
            if (pfsDest != NULL)
            {
              FreeFileSet(pfsDest,TRUE);
              DfsFree(pfsDest);
              pfsDest=NULL;
            }
        }

        #ifdef _TRYACCELERATE
        if (ppFileSetCollection!=NULL)
        {
            for (i=dfDirStartExtr;i<=dfDirExtr;i++)
                 if (*(ppFileSetCollection+i)!=NULL)
                 {
                     FreeFileSet(*(ppFileSetCollection+i),FALSE);
                     DfsFree(*(ppFileSetCollection+i));
                 }
            DfsFree(ppFileSetCollection);
        }
        #endif

        #if defined(_DEBUG) && defined(MSGTEST)
        if (!fRet)
            if (hwndMain!=NULL)
              MessageBox(hwndMain,"We do patching",fRet ? "TRUE":"FALSE",MB_OK|MB_ICONERROR);
        #endif

        if ((fRet) && (dwMaxProgress!=0) && (pSetExtractPosCallBack!=NULL))
            pSetExtractPosCallBack(dwMaxProgress,0,dfUserPtr);
            //guiItem.SetProgressPos(dwMaxProgress);

        return fRet;
    }

    return FALSE;
}





/************/
#ifndef MY_VERSIONMADEBY

#ifdef WIN32
# define MY_VERSIONMADEBY   (0x0b00) /* platform depedent */
#else
# define MY_VERSIONMADEBY   (0x0) /* platform depedent */
#endif
#endif


BOOL SVFAPI BuildZipFileFromFileSet(FILESET* pfsDest,dfwcharpc dfwcZipFileName,
                                    int opt_compress_level, /* Z_DEFAULT_COMPRESSION;*/
                                    dfwcharp dfwVersionName,
                                    tSetExtractPosCallBack pSetExtractPosCallBack,dfvoidp dfUserPtr,
                                    dfuLong32 dwMinProgress,dfuLong32 dwMaxProgress, H_ERROR_INFO * pei)
{
    BOOL fRet=TRUE;
    //int opt_compress_level=Z_DEFAULT_COMPRESSION;
    dfuLong32 dfSizeBuffer = 0x8000*1*4;
    unsigned char* pcdfBuffer = (unsigned char*)DfsMalloc(dfSizeBuffer);
    char* globalComment=NULL;
    if (pcdfBuffer==NULL)
        fRet = FALSE;

    if (fRet)
    {
        zipFile zf;
        dfuLong32 i;
        dfuLong64 dfSizeAllFileToZip=0;
        dfuLong64 dfSizeTotalDone=0;
        int errclose=ZIP_OK;
        char szZipFileName[MAX_PATH_LENGTH +1]="";

        ConvertUnicodeToTChar(dfwcZipFileName,szZipFileName,MAX_PATH_LENGTH );
        zf=zipOpen (szZipFileName,0);
        fRet = zf != NULL;

        for (i=0;(i<pfsDest->dfNbFileItem) && (fRet);i++)
        {
            if (((pfsDest->pFileItem + i)->ExtAction) != ExtActionIgnore)
            {
                dfuLong64 dfSizeFile ;
                dfSizeFile = GetFileSizeByName((pfsDest->pFileItem + i)->FileNameOnDisk,NULL,pei);
                if (dfSizeFile==FILE_SIZE_NOT_EXIST)
                {
                    fRet=FALSE;
                    break;
                }
                dfSizeAllFileToZip+=dfSizeFile;
            }
        }

        for (i=0;(i<pfsDest->dfNbFileItem) && (fRet);i++)
        {
            if (((pfsDest->pFileItem + i)->ExtAction) != ExtActionIgnore)
            {
                int err=0;
                char szFileNameInZip[MAX_PATH_LENGTH+1];
                LOWLEVELFILE llr=NULL;
                dfuLong64 dfSizeFile=FILE_SIZE_NOT_EXIST;
                DFSTM DfsTm;
                int zip64;
                zip_fileinfo zi;

                //ConvertUnicodeToTChar((pfsDest->pFileItem + i)->FileNameOnArchive,szFileNameInZip,MAX_PATH_LENGTH);


                //ConvertUnicodeToTChar((pfsDest->pFileItem + i)->FileNameOnArchive,szFileNameInZip,MAX_PATH_LENGTH);
                //ConvertUnicodeToOem((pfsDest->pFileItem + i)->FileNameOnArchive,szFileNameInZip,MAX_PATH_LENGTH);
                ConvertUnicodeToAnsi((pfsDest->pFileItem + i)->FileNameOnArchive,szFileNameInZip,MAX_PATH_LENGTH);


                zi.dosDate = 0;
                zi.internal_fa = 0;
                zi.external_fa = 0;
                //filetime(filenameinzip,&zi.tmz_date,&zi.dosDate);
                dfSizeFile = GetFileSizeByName((pfsDest->pFileItem + i)->FileNameOnDisk,&DfsTm,pei);
                if ((pfsDest->pFileItem + i)->fForceDate)
                    ConvertDfsInfoDateToDfsTm(&(pfsDest->pFileItem + i)->dfsInfoDate,&DfsTm);
                zi.tmz_date.tm_sec = DfsTm.df_sec;
                zi.tmz_date.tm_min = DfsTm.df_min;
                zi.tmz_date.tm_hour = DfsTm.df_hour;
                zi.tmz_date.tm_mday = DfsTm.df_mday;
                zi.tmz_date.tm_mon = DfsTm.df_mon-1;
                zi.tmz_date.tm_year = DfsTm.df_year;

                if (dfSizeFile==FILE_SIZE_NOT_EXIST)
                {
                    fRet=FALSE;
                    break;
                }
                zip64=0;
                err=zipOpenNewFileInZip4_64(zf,szFileNameInZip,&zi,
                           NULL,0,NULL,0,NULL /* comment*/,
                           (opt_compress_level != 0) ? Z_DEFLATED : 0,
                           opt_compress_level, 0,
                                 -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
                                 NULL, 0, MY_VERSIONMADEBY, 0, zip64);
                if (err != ZIP_OK)
                {
                    fRet=FALSE;
                    break;
                }

                llr = OpenLowLevel((pfsDest->pFileItem + i)->FileNameOnDisk,OPEN_READ, FALSE,FALSE,0,pei);
                if (llr==NULL)
                if (err != ZIP_OK)
                {
                    fRet=FALSE;
                    break;
                }
                while ((dfSizeFile>0) && (fRet))
                {
                    int err=0;
                    dfuLong32 dfPosProgress;
                    dfuLong32 dfuDoThis ;

                    if (dfSizeFile < dfSizeBuffer)
                        dfuDoThis = (dfuLong32)dfSizeFile;
                    else
                        dfuDoThis = dfSizeBuffer;

                    if (LowLevelRead(llr,pcdfBuffer,dfuDoThis,pei) != dfuDoThis)
                        fRet=FALSE;
                    else
                    err = zipWriteInFileInZip (zf,pcdfBuffer,dfuDoThis);
                    if (err<0)
                        fRet=FALSE;
                    dfSizeFile -= dfuDoThis;
                    dfSizeTotalDone+=dfuDoThis;

                    dfPosProgress = dwMinProgress +
                        CalculateRatio(dfSizeTotalDone,dfSizeAllFileToZip,
                                            dwMaxProgress-dwMinProgress);

                    if (pSetExtractPosCallBack!=NULL)
                      pSetExtractPosCallBack(dfPosProgress,0,dfUserPtr);
                }
                LowLevelClose(llr,pei);
                err = zipCloseFileInZip(zf);
                if (err!=ZIP_OK)
                    fRet=FALSE;
            }

        }

        if (dfwVersionName != NULL)
        {
            dfuLong32 dfUniLen = dfUnicodeStrlen(dfwVersionName);
            if (dfUniLen>0)
                globalComment=(char*)DfsMalloc((dfUniLen*4)+0x10);
            if (globalComment!=NULL)
            {
                *globalComment='\0';
                ConvertUnicodeToTChar(dfwVersionName,globalComment,(dfUniLen*2)+4);
            }
        }
        if (zf!=NULL)
            errclose = zipClose(zf,globalComment);
        if (errclose != ZIP_OK)
            fRet=FALSE;
    }
    if (globalComment != NULL)
        DfsFree(globalComment);

    if (pcdfBuffer!=NULL)
        DfsFree(pcdfBuffer);

    if (pSetExtractPosCallBack!=NULL)
      pSetExtractPosCallBack(dwMaxProgress,0,dfUserPtr);

    return fRet;
}

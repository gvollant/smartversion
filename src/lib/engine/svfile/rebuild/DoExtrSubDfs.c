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
#include "../compress/DfsWrSet.h"
#include "../decompress/DfsRdSet.h"
//#include "DfsCdLin.h"

#include "zlib.h"
#include "zip.h"



#include "../common/DirSet.h"
#include "../decompress/DoExtracting.h"
#include "DoExtrSubDfs.h"


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

#ifndef SVF_EXTRACT_ONLY
BOOL DoInsertDirectoryFileSet(DFSFILE DfsFileWrite,dfuLong32 dfTypeDir,
                              FILESET * pfsOrg, FILESET * pfsDest,
                              dfwcharpc dfwVersionName,dfwcharpc dfwVersionComment,
                              const COMPRESSIONPARAM* pCprParam,
                              tProgressCallBack pProgressCallBack,dfvoidp dfUserPtr, H_ERROR_INFO * pei)
{
    BOOL fRet=TRUE;
    FILETOADD * pFileToAdd;
    dfuLong32 i;
    pFileToAdd=(FILETOADD *)DfsMalloc((pfsDest->dfNbFileItem+1)*sizeof(FILETOADD));
    if (pFileToAdd==NULL)
        return FALSE;

    for (i=0;i<pfsDest->dfNbFileItem;i++)
    {
        dfuLong32 j;
        (pFileToAdd+i)->fIgnore=FALSE;
        (pFileToAdd+i)->fForceDate = ((pfsDest->pFileItem)+i)->fForceDate;
        (pFileToAdd + i)->hAddTags = ((pfsDest->pFileItem) + i)->hAddTags;
        (pFileToAdd+i)->dfsInfoDate = ((pfsDest->pFileItem)+i)->dfsInfoDate;
        (pFileToAdd+i)->pReserved = ((pfsDest->pFileItem)+i)->pReserved;
        (pFileToAdd+i)->fWritingRaw = FALSE;

        (pFileToAdd+i)->filename_tostore = ((pfsDest->pFileItem)+i)->FileNameOnArchive;
        (pFileToAdd+i)->filename_ondisk = ((pfsDest->pFileItem)+i)->FileNameOnDisk;
        (pFileToAdd+i)->fForceRecopyPrevious = FALSE;
        (pFileToAdd+i)->dfForceRecopyOrRawCopySize = 0;
        (pFileToAdd+i)->dfForceRecopyOrRawCopyCrc32 = 0;


        (pFileToAdd+i)->dfPreviousVersionFilePosition=VALUE_UNKNOWN;
        (pFileToAdd+i)->filename_prevversionondisk=NULL;
        (pFileToAdd+i)->hRamDifToFlushPatch = NULL;

        if (pfsOrg!=NULL)
          for (j=0;j<pfsOrg->dfNbFileItem;j++)
            {
                dfwcharpc dfwFta=(pFileToAdd+i)->filename_tostore;
                dfwcharpc dfwPrevFileName=((pfsOrg->pFileItem)+j)->FileNameOnArchive;

                if (dfUnicodeStrcmpi(dfwPrevFileName,dfwFta)==0)
                {
                    (pFileToAdd+i)->dfPreviousVersionFilePosition = j;
                    (pFileToAdd+i)->filename_prevversionondisk =
                                      ((pfsOrg->pFileItem)+j)->FileNameOnDisk;

                    break;
                }
            }
    }
    if (dfwVersionName==NULL) dfwVersionName=GetUnicodeStringEmpty();
    fRet = InsertDirectoryinDfsFile(DfsFileWrite,dfTypeDir,
                                    pfsDest->dfNbFileItem,pFileToAdd,FALSE,
                                    dfwVersionName,dfwVersionComment,
                                    pCprParam,
                                    pProgressCallBack,dfUserPtr,pei) == DFS_SUCCESS;
    DfsFree(pFileToAdd);
    return fRet;
}


BOOL SVFAPI DoGenerateSubDfs(DFSFILE DfsFileRead,/*dfuLong32 dfNbDir,*/PDIRINFO* pDirInfo,
                      DFSFILE *pDfsFileWrite,dfwcharpc dfWritingDfsFileName, /*BOOL fZipFile,*/
                      //BOOL fStripIdenticalBody,
                      const DFSFEATUREPARAM* pFeatureParam,
                      BOOL fBaseDirectorySelected,dfuLong32 dfBaseDirNum,
                      dfwcharpc wchBaseDirectory,const FILESET* pFileSetBase,
                      dfuLong32 dwNbMapVersionMap,const BOOL* lpVersionMap,
                      BOOL fFirstVersionAsReference,BOOL fReuseOldPatch /* future*/,
                      const COMPRESSIONPARAM* pCprParam,
                      tSetExtractPosCallBack pSetExtractPosCallBack,dfvoidp dfUserPtr,
                      dfuLong32 dwMinProgress,dfuLong32 dwMaxProgress,
                      H_ERROR_INFO * pei)
{
    //BOOL fBaseOk = FALSE;
    BOOL fBaseNeeded = FALSE;
    BOOL fRet=TRUE;
    dfuLong32 dfDirStartExtr=0;
    dfuLong32 dfLastDirExtr=0;
    dfuLong32 dfFirstDirExtr=VALUE_UNKNOWN;
    dfuLong32 dfVersionToWrite=0;
    dfuLong32 i;
    FILESET** ppFileSetCollection;
    dfwcharpc wchBaseDirExtract=GetUnicodeStringEmpty();
    DFSFILE DfsFileWrite=NULL;
    dfuLong32 dfProgressStep=0;
    dfuLong32 dfProgressStartcurrentStep=dwMinProgress;
    dfuLong32 dfNbVersionWritten=0;
    #if defined(_DEBUG) && defined(MSGTEST)
    HWND hWndMain=GetDesktopWindow();
    #endif

    if (pDfsFileWrite!=NULL)
        *pDfsFileWrite=NULL;



    for (i=0;i<dwNbMapVersionMap;i++)
        if (*(lpVersionMap+i))
        {
            if (dfFirstDirExtr==VALUE_UNKNOWN)
                dfFirstDirExtr=i;
            dfLastDirExtr=i;
            dfVersionToWrite++;
        }

    if ((dfVersionToWrite+(1+dfLastDirExtr-dfDirStartExtr))>0)
      dfProgressStep = (dwMaxProgress-dwMinProgress)/(dfVersionToWrite+(1+dfLastDirExtr-dfDirStartExtr));

    /* return if no dir to extract */
    if (dfFirstDirExtr==VALUE_UNKNOWN)
        return FALSE;

    fBaseNeeded = ((*pDirInfo)->dfTypeDir == TYPEDIR_FILECRCONLY);
    if (fBaseNeeded)
    {
        /* we need dir selected */
        BOOL fBaseOk = FALSE;
        if (fBaseDirectorySelected)
            if (dfBaseDirNum <= dfFirstDirExtr)
            {
                fBaseOk = TRUE;
                dfDirStartExtr = dfBaseDirNum;
            }

       if (!fBaseOk)
       {
           return FALSE;
       }
    }


    {
        DFSFILEINFOPARAM DfsFileParam;

        DfsFileParam.sizeStruct = sizeof(DfsFileParam);
        DfsFileParam.dfStatus = DFS_NEWFILE | DFS_WRITABLE;

        DfsFileParam.filename = dfWritingDfsFileName;

        DfsFileOpen(&DfsFileParam, &DfsFileWrite,pei);
        if (pDfsFileWrite!=NULL)
        {
            *pDfsFileWrite=DfsFileWrite;
            //SetDfsExtendedMode(DfsFileWrite,fStripIdenticalBody);
            if (pFeatureParam!=NULL)
                SetDfsFeatureParam(DfsFileWrite,pFeatureParam);
        }
        if (DfsFileWrite==NULL)
            return FALSE;
    }

    ppFileSetCollection = (FILESET**)DfsMalloc((dfLastDirExtr+1)*(sizeof(FILESET*)));
    for (i=0;i<=dfLastDirExtr;i++)
        *(ppFileSetCollection+i)=NULL;

    for (i=dfDirStartExtr;i<=dfLastDirExtr;i++)
        {
            BOOL fFirstBase = ((i==dfDirStartExtr) && (fBaseNeeded));
            BOOL fAllIsGood=FALSE;
            FILESET* pfsCur;
            dfuLong32 dfError,dfTypeDir;
            dfError = dfTypeDir = 0;

            pfsCur=(FILESET*)DfsMalloc(sizeof(FILESET));
            *(ppFileSetCollection+i)=pfsCur;

            InitFileSet(pfsCur);


            dfError =
                CreateFileSetForVersionInDirectory(DfsFileRead,pfsCur,
                                    i,
                                    fFirstBase ? wchBaseDirectory : wchBaseDirExtract,
                                    fFirstBase ? pFileSetBase : NULL,
                                    *(pDirInfo+i),
                                    &fAllIsGood,FALSE,FALSE /* fFillNameOnDisk*/ ,
                                    fFirstBase,fFirstBase,&dfTypeDir,TRUE,FALSE,pei);


            if ((dfError != DFS_SUCCESS) || (!fAllIsGood))
            {
                fRet=FALSE;
                break;
            }
        }




    {
        FILESET* pfsOrg = NULL;
        FILESET* pfsOrgNewDfs = NULL;
        BOOL fOrgNewDfsToDelete=FALSE;
        FILESET* pfsDest = NULL;
        dfuLong32 dfError=DFS_SUCCESS;


        for (i=dfDirStartExtr;(i<=dfLastDirExtr) && fRet;i++)
        {
            BOOL fFileOrgToDelete = TRUE;

            if ((i==dfDirStartExtr) && (fBaseNeeded))
            {
                pfsOrg = *(ppFileSetCollection+dfDirStartExtr);
                *(ppFileSetCollection+dfDirStartExtr)=NULL;

                if (dfError != DFS_SUCCESS)
                {
                    fRet=FALSE;
                    break;
                }
                fFileOrgToDelete = FALSE;

                if (*(lpVersionMap+dfDirStartExtr))
                {
                    BOOL fInsert;
                    PROGRESSCBPARAMEP pcp;
                    dfuLong32 dfTypeDir=(fFirstVersionAsReference ?
                                              TYPEDIR_FILECRCONLY:TYPEDIR_FILEINSERTING_DEFLATE);

                    // dfs ... do build
                    //do InsertDirectoryinDfsFile or zip

                    pcp.dwMinProgress = dfProgressStartcurrentStep;
                    pcp.dwMaxProgress = dfProgressStartcurrentStep+dfProgressStep;

                    pcp.pSetExtractPosCallBack = pSetExtractPosCallBack;
                    pcp.dfuPtrSetExtractPosCallBack = dfUserPtr;

                    pcp.pConfirmBeforeCreatingFile = NULL;
                    pcp.dfUserPtrBeforeCreatingFile = NULL;

                    pcp.calcProgress=PROGRESS_CALC_METHOD_BY_ORIG;

                    fInsert=DoInsertDirectoryFileSet(DfsFileWrite,dfTypeDir,
                              NULL, pfsOrg,
                              /*dfwVersionName*/NULL,NULL,
                              pCprParam,
                              ProgressCallBackDoExtracting, &pcp, pei);


                    {
                        dfvoidp TagBuf;
                        dfuLong32 TagSize;
                        DFTAGBLOCKFLOAT TagBlockFloat;
                        TagBlockFloat = GetDfsTagBlockFloat(DfsFileRead, pei);
                        if (TagBlockFloat != NULL)
                        {
                            if (GetTagBlockFloat(TagBlockFloat,i,FLOATNUM_NOSPECIFIC,DFSTAG_DIR_NAME_FLOAT,&TagBuf, &TagSize))
                            {
                                DFTAGBLOCKFLOAT TagBlockFloatWriting;
                                TagBlockFloatWriting = GetDfsTagBlockFloat(DfsFileWrite, pei);
                                if (TagBlockFloatWriting != NULL)
                                    AddTagBlockFloat(TagBlockFloatWriting,dfNbVersionWritten,
                                                     FLOATNUM_NOSPECIFIC,DFSTAG_DIR_NAME_FLOAT,
                                                     TagBuf, TagSize);
                                SetDfsTagBlockFloatDirty(DfsFileWrite, pei);
                                //DfsFlushWriteDfsFile(DfsFileWrite, pei);
                            }

                            if (GetTagBlockFloat(TagBlockFloat,i,FLOATNUM_NOSPECIFIC,DFSTAG_DIR_COMMENT_FLOAT,&TagBuf, &TagSize))
                            {
                                DFTAGBLOCKFLOAT TagBlockFloatWriting;
                                TagBlockFloatWriting = GetDfsTagBlockFloat(DfsFileWrite, pei);
                                if (TagBlockFloatWriting != NULL)
                                    AddTagBlockFloat(TagBlockFloatWriting,dfNbVersionWritten,
                                                     FLOATNUM_NOSPECIFIC,DFSTAG_DIR_COMMENT_FLOAT,
                                                     TagBuf, TagSize);
                                SetDfsTagBlockFloatDirty(DfsFileWrite, pei);
                                //DfsFlushWriteDfsFile(DfsFileWrite, pei);
                            }

                        }
                    }
                    if (!fInsert)
                        fRet=FALSE;
                    else
                        dfNbVersionWritten++;

                    pfsOrgNewDfs = pfsOrg;
                    fOrgNewDfsToDelete=FALSE;
                    dfProgressStartcurrentStep+=dfProgressStep;
                }
                i++;
                if (i>dfLastDirExtr)
                    break; /*+++*/
            }
            else
            {
                pfsOrg = pfsDest ; // Get Previous Dest
                if (i>dfDirStartExtr)
                    if (*(lpVersionMap+i-1))
                    {
                        pfsOrgNewDfs=pfsDest;
                        fOrgNewDfsToDelete=TRUE;
                    }
            }

/* do build and extract pfsDest */


            pfsDest = *(ppFileSetCollection+i);
            *(ppFileSetCollection+i)=NULL;

            pfsDest->fTempFile = TRUE;


            if (dfError != DFS_SUCCESS)
            {
                fRet=FALSE;
                break;
            }

            {

              PROGRESSCBPARAMEP pcp;
              pcp.dwMinProgress = dfProgressStartcurrentStep;
              pcp.dwMaxProgress = dfProgressStartcurrentStep+dfProgressStep;

              pcp.pSetExtractPosCallBack = pSetExtractPosCallBack;
              pcp.dfuPtrSetExtractPosCallBack = dfUserPtr;
              pcp.pConfirmBeforeCreatingFile = NULL;
              pcp.dfUserPtrBeforeCreatingFile = NULL;

              pcp.calcProgress=PROGRESS_CALC_METHOD_BY_ORIG;

              dfError = ExtractPatch(DfsFileRead,pfsOrg,pfsDest,i,*(pDirInfo+i),
                                     TRUE,FALSE,FALSE,
                                     wchBaseDirExtract,ProgressCallBackDoExtracting, &pcp,
                                     NULL, NULL,
                                     pei);
              dfProgressStartcurrentStep+=dfProgressStep ;
            }

            if ((dfError == DFS_SUCCESS) && (*(lpVersionMap+i)))
            {
                // dfs ... do build
                //do InsertDirectoryinDfsFile or zip
                {
                    BOOL fInsert;
                    PROGRESSCBPARAMEP pcp;
                    dfuLong32 dfTypeDir;

                    pcp.dwMinProgress = dfProgressStartcurrentStep;
                    pcp.dwMaxProgress = dfProgressStartcurrentStep+dfProgressStep;

                    pcp.pSetExtractPosCallBack = pSetExtractPosCallBack;
                    pcp.dfuPtrSetExtractPosCallBack = dfUserPtr;
                    pcp.pConfirmBeforeCreatingFile = NULL;
                    pcp.dfUserPtrBeforeCreatingFile = NULL;

                    pcp.calcProgress=PROGRESS_CALC_METHOD_BY_ORIG;

                    dfTypeDir=(pfsOrgNewDfs != NULL)  ? TYPEDIR_PATCHFROMPREVIOUS :
                                          (fFirstVersionAsReference ?
                                              TYPEDIR_FILECRCONLY:TYPEDIR_FILEINSERTING_DEFLATE);

                    // dfs ... do build
                    //do InsertDirectoryinDfsFile or zip

                                          /*
                    {
                        dfvoidp TagBuf;
                        dfuLong32 TagSize;
                        DFTAGBLOCKFLOAT TagBlockFloat;
                        TagBlockFloat = GetDfsTagBlockFloat(DfsFileRead);
                        if (TagBlockFloat != NULL)
                            if (GetTagBlockFloat(TagBlockFloat,i,FLOATNUM_NOSPECIFIC,DFSTAG_DIR_NAME_FLOAT,&TagBuf, &TagSize))
                            {
                                DFTAGBLOCKFLOAT TagBlockFloatWriting;
                                TagBlockFloatWriting = GetDfsTagBlockFloat(DfsFileWrite);
                                if (TagBlockFloatWriting != NULL)
                                    AddTagBlockFloat(TagBlockFloatWriting,dfNbVersionWritten,
                                                     FLOATNUM_NOSPECIFIC,DFSTAG_DIR_NAME_FLOAT,
                                                     TagBuf, TagSize);
                            }

                    }*/

                    fInsert=DoInsertDirectoryFileSet(DfsFileWrite,dfTypeDir,
                              pfsOrgNewDfs, pfsDest,
                              /*dfwVersionName*/NULL,NULL,
                              pCprParam,
                              ProgressCallBackDoExtracting, &pcp, pei);

                    {
                        dfvoidp TagBuf;
                        dfuLong32 TagSize;
                        DFTAGBLOCKFLOAT TagBlockFloat;
                        TagBlockFloat = GetDfsTagBlockFloat(DfsFileRead, pei);
                        if (TagBlockFloat != NULL)
                        {
                            if (GetTagBlockFloat(TagBlockFloat,i,FLOATNUM_NOSPECIFIC,DFSTAG_DIR_NAME_FLOAT,&TagBuf, &TagSize))
                            {
                                DFTAGBLOCKFLOAT TagBlockFloatWriting;
                                TagBlockFloatWriting = GetDfsTagBlockFloat(DfsFileWrite, pei);
                                if (TagBlockFloatWriting != NULL)
                                    AddTagBlockFloat(TagBlockFloatWriting,dfNbVersionWritten,
                                                     FLOATNUM_NOSPECIFIC,DFSTAG_DIR_NAME_FLOAT,
                                                     TagBuf, TagSize);
                                SetDfsTagBlockFloatDirty(DfsFileWrite, pei);
                            }

                            if (GetTagBlockFloat(TagBlockFloat,i,FLOATNUM_NOSPECIFIC,DFSTAG_DIR_COMMENT_FLOAT,&TagBuf, &TagSize))
                            {
                                DFTAGBLOCKFLOAT TagBlockFloatWriting;
                                TagBlockFloatWriting = GetDfsTagBlockFloat(DfsFileWrite, pei);
                                if (TagBlockFloatWriting != NULL)
                                    AddTagBlockFloat(TagBlockFloatWriting,dfNbVersionWritten,
                                                     FLOATNUM_NOSPECIFIC,DFSTAG_DIR_COMMENT_FLOAT,
                                                     TagBuf, TagSize);
                                SetDfsTagBlockFloatDirty(DfsFileWrite, pei);
                            }
                        }
                    }



                    if (!fInsert)
                        fRet=FALSE;
                    else
                        dfNbVersionWritten++;
                    //pfsOrgNewDfs = pfsDest;
                    //fOrgNewDfsToDelete=FALSE;
                    dfProgressStartcurrentStep+=dfProgressStep;
                }

                if (pfsOrg==pfsOrgNewDfs)
                    pfsOrgNewDfs=NULL;

                if (pfsOrgNewDfs != NULL)
                {
                  FreeFileSet(pfsOrgNewDfs,fOrgNewDfsToDelete);
                  DfsFree(pfsOrgNewDfs);
                  pfsOrgNewDfs=NULL;
                }
            }

            if ((dfError != DFS_SUCCESS))
            {
                fRet=FALSE;
                break;
            }

            if ((pfsOrg != NULL) && (pfsOrg!=pfsOrgNewDfs))
            {/*
                BOOL fPreviousVersionNeededLater=FALSE;
                if (i>0)
                    if ((*(lpVersionMap+i-1)) && (!(*(lpVersionMap+i-1))))
                        fPreviousVersionNeededLater=TRUE;
                if (!fPreviousVersionNeededLater)*/
                {
                  FreeFileSet(pfsOrg,fFileOrgToDelete);
                  DfsFree(pfsOrg);
                }
            }
            pfsOrg=NULL;

            //dfProgressStartcurrentStep+=dfProgressStep ;
            if ((fRet) && (dwMaxProgress!=0) && (dfLastDirExtr+1>dfDirStartExtr))
            {
               dfuLong32 dwNewPos;
               dwNewPos = dfProgressStartcurrentStep;
               #if defined(_DEBUG) && defined(WIN_H_INCLUDED)
               OutputDebugString("$");
               #endif


               if (pSetExtractPosCallBack!=NULL)
                 pSetExtractPosCallBack(dwNewPos,0,dfUserPtr);
            }
        }


        if ((pfsOrgNewDfs != NULL))
        {
          FreeFileSet(pfsOrgNewDfs,fOrgNewDfsToDelete);
          DfsFree(pfsOrgNewDfs);
          if (pfsOrgNewDfs == pfsDest)
              pfsDest=NULL;
          pfsOrgNewDfs=NULL;
        }


        {
            if (pfsDest != NULL)
            {
              FreeFileSet(pfsDest,TRUE);
              DfsFree(pfsDest);
              pfsDest=NULL;
            }
        }


        if (ppFileSetCollection!=NULL)
        {
            for (i=dfDirStartExtr;i<=dfLastDirExtr;i++)
                 if (*(ppFileSetCollection+i)!=NULL)
                 {
                     FreeFileSet(*(ppFileSetCollection+i),FALSE);
                     DfsFree(*(ppFileSetCollection+i));
                 }
            DfsFree(ppFileSetCollection);
        }


        #if defined(_DEBUG) && defined(MSGTEST)
        if (!fRet)
            if (hwndMain!=NULL)
              MessageBox(hwndMain,"We do subgenerating",fRet ? "TRUE":"FALSE",MB_OK|MB_ICONERROR);
        #endif

        if ((fRet) && (dwMaxProgress!=0))
            if (pSetExtractPosCallBack != NULL)
              pSetExtractPosCallBack(dwMaxProgress,0,dfUserPtr);
            //guiItem.SetProgressPos(dwMaxProgress);


    }

    if (pDfsFileWrite==NULL)
        DfsClose(DfsFileWrite, pei);
    return fRet;
}

#endif

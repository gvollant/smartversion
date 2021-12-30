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

#ifdef _DEBUG
#define MSGOUTTEST
#endif

#include <stdio.h>



#include "../../engine/patchstream/common/difbasic.h"

#include "../../engine/patchstream/common/DfsTlTyp.h"
#include "../../engine/patchstream/common/difstool.h"
#include "../../engine/svfile/common/DfsStruc.h"
#include "../../engine/svfile/common/DfsTagDf.h"
#include "../../engine/svfile/common/DfsTagMg.h"
#include "../../engine/svfile/common/DfsTagBlockFloatEnd.h"

#include "../../engine/patchstream/common/DfsOrigMemoryMap.h"
#include "../../engine/patchstream/rebuild/RamDifWk.h"


#include "../../engine/svfile/common/DfsIntf.h"
#include "../../engine/svfile/common/DfsSet.h"

#include "../../engine/svfile/decompress/DfsRdSet.h"
#include "../../engine/svfile/compress/DfsWrSet.h"
#include "../../engine/svfile/common/ArrayTl.h"

#include "../../engine/patchstream/common/DfsIoHlp.h"



#include "../../engine/svfile/common/DirSet.h"
#include "../../engine/svfile/decompress/DoExtracting.h"
#include "../../engine/svfile/rebuild/ReMixDfs.h"
#include "../../engine/svfile/common/FileNameIndexer.h"

#include "ReMixHelper.h"

BOOL SVFAPI FreeVersionToAdd(VERSIONTOADD_REMIX *pVersionRemix,dfuLong32 dfNbVersion)
{
    dfuLong32 i,j;
    BOOL fDeleteStringToAdd=FALSE;
    if (pVersionRemix==NULL)
        return FALSE;
    for (i = 0; i < dfNbVersion; i++)
    {
        VERSIONTOADD_REMIX *pFileCopyInfoCur = pVersionRemix + i;
        if (pFileCopyInfoCur->pFileCopyInfo != NULL)
          DfsFree(pFileCopyInfoCur->pFileCopyInfo);
        pFileCopyInfoCur->pFileCopyInfo = NULL;

        if (pFileCopyInfoCur->pfta!=NULL)
        {
            if (fDeleteStringToAdd)
                for (j=0;j<pFileCopyInfoCur->dfNbFileToAdd;j++)
                {
                    FILETOADD_REMIX * pftacur = (pFileCopyInfoCur->pfta)+j;
                    if (pftacur->filename_tostore!=NULL)
                        DfsFree((void*)pftacur->filename_tostore);
                    if (pftacur->filename_ondisk!=NULL)
                        DfsFree((void*)pftacur->filename_ondisk);
                }

            DfsFree(pFileCopyInfoCur->pfta);
            pFileCopyInfoCur->pfta=NULL;
        }
    }
    DfsFree(pVersionRemix);
    return TRUE;
}


VERSIONTOADD_REMIX * SVFAPI BuildRecopyVersionToAdd(dfuLong32 dfNbVersionInDfs, PDIRINFO* pDirInfo)
{
        VERSIONTOADD_REMIX *pVersionRemix=NULL;
        dfuLong32 dfNbDirDfsRead = dfNbVersionInDfs;
        dfuLong32 dwNbMapVersionMap = dfNbDirDfsRead;
        dfuLong32 i, j;
        BOOL fRet=TRUE;

        pVersionRemix =
            (VERSIONTOADD_REMIX *) DfsMalloc(sizeof(VERSIONTOADD_REMIX) * (dwNbMapVersionMap + 1));
        if (pVersionRemix==NULL)
            return NULL;

        for (i = 0; i < dwNbMapVersionMap; i++)
        {
            VERSIONTOADD_REMIX *pFileCopyInfoCur = pVersionRemix + i;
            pFileCopyInfoCur->pFileCopyInfo = NULL;
            pFileCopyInfoCur->pfta = NULL;
        }

        // Fill a "full recopy" REMIX structure
        for (i = 0; (i < dwNbMapVersionMap) && fRet; i++)
        {
            VERSIONTOADD_REMIX *pFileCopyInfoCur = pVersionRemix + i;
            PDIRINFO pDirOrg = *((pDirInfo) + i);
            pFileCopyInfoCur->dfNumVersionPreviousSvf = i;
            pFileCopyInfoCur->dfNbPreviousFileInMask = pDirOrg->dfNbFile;
            pFileCopyInfoCur->dfNbFileToAdd = 0;

            pFileCopyInfoCur->pFileCopyInfo = (FILETOCOPYINFO_REMIX *)
                DfsMalloc(sizeof(FILETOCOPYINFO_REMIX) *
                              (pDirOrg->dfNbFile +
                               /*pInsertFileInVersionData->dfNbFileToAddInVersion*/ + 1));
            if (pFileCopyInfoCur->pFileCopyInfo==NULL)
            {
                fRet=FALSE;
                break;
            }


            for (j = 0; j < pDirOrg->dfNbFile; j++)
            {
                FILETOCOPYINFO_REMIX *pftci = (pFileCopyInfoCur->pFileCopyInfo) + j;
                pftci->dfReferenceItem = FTCI_REFERENCE_UNMODIFIED;
                pftci->fIsReferenceInAddedFile = FALSE;
            }
        }
        // Now, we will have to insert, delete or what we want on file !

        if (!fRet)
            {
                for (i = 0; i < dwNbMapVersionMap; i++)
                {
                    FILETOCOPYINFO_REMIX *pFileCopyInfo = (pVersionRemix + i)->pFileCopyInfo;
                    if (pFileCopyInfo!=NULL)
                      DfsFree(pFileCopyInfo);
                }
                DfsFree(pVersionRemix);
                pVersionRemix=NULL;
            }


        return pVersionRemix;
}


BOOL RemoveAllReferenceFromOnItemInPreviousVersion(VERSIONTOADD_REMIX *pVersionRemix,dfuLong32 dfVersionWhereRemove,
                                                   BOOL fItemReferedToRemoveInInsertedFiles,dfuLong32 dfItemReferedNumber)
{
    VERSIONTOADD_REMIX *pCurVersionRemix;
    dfuLong32 i;
    if (dfVersionWhereRemove==0)
        return FALSE;
    pCurVersionRemix = pVersionRemix + dfVersionWhereRemove;
    for (i=0;i< pCurVersionRemix ->dfNbPreviousFileInMask;i++)
    {
        FILETOCOPYINFO_REMIX* pCurFileCopyInfo = (pCurVersionRemix -> pFileCopyInfo)+i;
        if ((pCurFileCopyInfo ->dfReferenceItem == dfItemReferedNumber) &&
            (pCurFileCopyInfo ->fIsReferenceInAddedFile == fItemReferedToRemoveInInsertedFiles))
        {
            pCurFileCopyInfo ->fIsReferenceInAddedFile = FALSE;
            pCurFileCopyInfo ->dfReferenceItem = FTCI_REFERENCE_INDEPENDANT;
        }
    }

    for (i=0;i< pCurVersionRemix ->dfNbFileToAdd;i++)
    {
        FILETOADD_REMIX * pftaCur = (pCurVersionRemix -> pfta) + i;
        if ((pftaCur ->dfPreviousVersionFilePositionItem == dfItemReferedNumber) &&
            (pftaCur ->fIsReferenceInFileToAdd == fItemReferedToRemoveInInsertedFiles))
        {
            pftaCur ->fIsReferenceInFileToAdd = FALSE;
            pftaCur ->dfPreviousVersionFilePositionItem = FTCI_REFERENCE_INDEPENDANT;
        }
    }
    return TRUE;
}


#define VERSION_AFTER_INSTERTED_FILED_POSITION_NOT_INSERTED ((dfuLong32)(0xffffffffUL))

typedef struct
{
    dfuLong32 dfPosition;
    BOOL fIsInserted;
} VERSIONAFTERINSTERTEDFILEDPOSITION;


VERSIONTOADD_REMIX * SVFAPI CreateInsertingForReplaceCurrentDfs(dfuLong32 dfNbVersionInDfs, PDIRINFO* pDirInfo,
                                                         INSERTFILEINVERSION_DATA* pInsertFileInVersionData,
                                                         dfuLong32 dfVersionSelected,dfuLong32 * pdfError,
                                                         tCallBackAskReplacing pCallBackAskReplacing,dfvoidp dfUserPtr)
{
        VERSIONTOADD_REMIX *pVersionRemix=NULL;
        dfuLong32 dfNbDirDfsRead = dfNbVersionInDfs;
        dfuLong32 dwNbMapVersionMap = dfNbDirDfsRead;
        dfuLong32 i;
        //dfuLong32 dfError=DFS_SUCCESS;
        dfuLong32 dfNbVersionRemix=dwNbMapVersionMap;
        BOOL fVersionPreviousHasInsertedFile;
        VERSIONAFTERINSTERTEDFILEDPOSITION* pVersionAfterInstertedPositionAlloc;
        VERSIONAFTERINSTERTEDFILEDPOSITION* pVersionAfterInstertedPositionBrowseCurrentVersion;
        VERSIONAFTERINSTERTEDFILEDPOSITION* pVersionAfterInstertedPositionBrowsePreviousVersion;
        BOOL fRet=TRUE;
        REPLACEMENTFILEOPTION ReplacementFileOption = pInsertFileInVersionData->ReplacementFileOption;
        FILEINDEXER fixr;
        dfuLong32 dfNbFileToAddInVersion = pInsertFileInVersionData->dfNbFileToAddInVersion ;

        pVersionRemix = BuildRecopyVersionToAdd(dfNbVersionInDfs,pDirInfo);

        pVersionAfterInstertedPositionAlloc =
             (VERSIONAFTERINSTERTEDFILEDPOSITION*)DfsMalloc(sizeof(VERSIONAFTERINSTERTEDFILEDPOSITION) *
                        (dfNbFileToAddInVersion + 1) * 2);

        pVersionAfterInstertedPositionBrowseCurrentVersion = pVersionAfterInstertedPositionAlloc;
        pVersionAfterInstertedPositionBrowsePreviousVersion = pVersionAfterInstertedPositionAlloc + dfNbFileToAddInVersion + 1;

        fixr = InitFileRefIndexer(FALSE,pInsertFileInVersionData->dfNbFileToAddInVersion);
        for (i=0;i<pInsertFileInVersionData->dfNbFileToAddInVersion;i++)
        {
            AddFileRefIndexer(fixr,((pInsertFileInVersionData->pFileToAddInVersions)+i)->dfFileNameToStore,i,NULL);
        }

        // Fill a "full recopy" REMIX structure
        for (i = 0; i < dwNbMapVersionMap; i++)
        {
            //PDIRINFO pDirOrg = *((pDirInfo) + i);
            VERSIONTOADD_REMIX *pFileCopyInfoCur = pVersionRemix + i;
            pFileCopyInfoCur->pfta=(FILETOADD_REMIX *)DfsMalloc(sizeof(FILETOADD_REMIX) * (dfNbFileToAddInVersion+1));
        }
        // Now, we will have to insert file !

        for (i=0;(i<dfNbFileToAddInVersion) && (fRet);i++)
        {
          ((pVersionAfterInstertedPositionBrowsePreviousVersion)+i)->fIsInserted =
              FALSE;
          ((pVersionAfterInstertedPositionBrowsePreviousVersion)+i)->dfPosition =
              VERSION_AFTER_INSTERTED_FILED_POSITION_NOT_INSERTED;
        }
        fVersionPreviousHasInsertedFile=FALSE;


        for (i = 0; i < dwNbMapVersionMap; i++)
        {
            dfuLong32 j,k;
            PDIRINFO pCurDirOrg = *((pDirInfo) + i);
            VERSIONTOADD_REMIX *pFileCopyInfoCur = pVersionRemix + i;
            BOOL fInsertInCurrentVersion;
            dfuLong32 dfNbFileInserted = 0;
            //BOOL fNeedAdding=TRUE;

            fInsertInCurrentVersion = *((pInsertFileInVersionData->pVersionMap)+i);

            {
                for (j=0;(j<dfNbFileToAddInVersion) && (fRet);j++)
                {
                    (pVersionAfterInstertedPositionBrowseCurrentVersion+j)->fIsInserted = FALSE;
                    (pVersionAfterInstertedPositionBrowseCurrentVersion+j)->dfPosition = VERSION_AFTER_INSTERTED_FILED_POSITION_NOT_INSERTED;
                }
                for (k=0;(k<pCurDirOrg->dfNbFile) && fRet;k++)
                {
                    if (GetFileRefIndexer(fixr,(pCurDirOrg->pFileInDirInfo + k)->FileName,&j))
                    {
                        (pVersionAfterInstertedPositionBrowseCurrentVersion+j)->dfPosition = k;
                    }
                }
            }

            if ((!fInsertInCurrentVersion) && (fVersionPreviousHasInsertedFile))
            {
                // we had to "re link"

                for (k=0;(k<pCurDirOrg->dfNbFile) && fRet;k++)
                {
                    if (GetFileRefIndexer(fixr,(pCurDirOrg->pFileInDirInfo + k)->FileName,&j))
                    {
                        VERSIONAFTERINSTERTEDFILEDPOSITION VersionAfterInsertedFilePositionInPreviousVersion;
                        VersionAfterInsertedFilePositionInPreviousVersion =
                                     *(pVersionAfterInstertedPositionBrowsePreviousVersion+j);

                        if (VersionAfterInsertedFilePositionInPreviousVersion.dfPosition !=
                                     VERSION_AFTER_INSTERTED_FILED_POSITION_NOT_INSERTED)
                        {
                            RemoveAllReferenceFromOnItemInPreviousVersion(pVersionRemix,i,
                                            VersionAfterInsertedFilePositionInPreviousVersion.fIsInserted,
                                            VersionAfterInsertedFilePositionInPreviousVersion.dfPosition);

                            (pFileCopyInfoCur->pFileCopyInfo + k)->dfReferenceItem =
                                   VersionAfterInsertedFilePositionInPreviousVersion.dfPosition;
                            (pFileCopyInfoCur->pFileCopyInfo + k)->fIsReferenceInAddedFile =
                                   VersionAfterInsertedFilePositionInPreviousVersion.fIsInserted;
                        }
                    }
                }
            }

            if (fInsertInCurrentVersion)
            {
              for (j=0;(j<dfNbFileToAddInVersion) && (fRet);j++)
              {
                FILETOADDINVERSIONS* pFileToAddInVersionsCur = (pInsertFileInVersionData->pFileToAddInVersions)+j;
                // now we will search if the file exist already. This code must be optimized later
                VERSIONAFTERINSTERTEDFILEDPOSITION VersionAfterInsertedFilePositionInPreviousVersion;
                VERSIONAFTERINSTERTEDFILEDPOSITION VersionAfterInsertedFilePositionInCurrentVersion;
                BOOL fReplaceAsked = TRUE;
                dfuLong32 dfItemToReplace ;

                VersionAfterInsertedFilePositionInCurrentVersion = *(pVersionAfterInstertedPositionBrowseCurrentVersion+j);
                VersionAfterInsertedFilePositionInPreviousVersion = *(pVersionAfterInstertedPositionBrowsePreviousVersion+j);

                dfItemToReplace = VersionAfterInsertedFilePositionInCurrentVersion.dfPosition ;

                if (dfItemToReplace != VERSION_AFTER_INSTERTED_FILED_POSITION_NOT_INSERTED)
                {
                    fReplaceAsked = ReplacementFileOption != REPLACEMENTFILEOPTION_NOREPLACE;

                    if ((ReplacementFileOption == REPLACEMENTFILEOPTION_UIASKING) && (pCallBackAskReplacing!=NULL))
                    {
                        ASKREPL_CBINFO AskRepl_CbInfo;
                        dfuLong32 dfRet;

                        AskRepl_CbInfo.dfSizeStruct = sizeof(ASKREPL_CBINFO);
                        AskRepl_CbInfo.pInsertFileInVersionData = pInsertFileInVersionData;
                        AskRepl_CbInfo.dfVersion = i;
                        AskRepl_CbInfo.dfItemToReplace = dfItemToReplace ;
                        AskRepl_CbInfo.dfItemInInsertList=j;
                        AskRepl_CbInfo.dfFileNameToStore = pFileToAddInVersionsCur->dfFileNameToStore;
                        AskRepl_CbInfo.dfFileNameOnDisk = pFileToAddInVersionsCur->dfFileNameOnDisk;
                        AskRepl_CbInfo.pDirInfo = pDirInfo;

                        dfRet = (*pCallBackAskReplacing)(&AskRepl_CbInfo,dfUserPtr);
                        switch (dfRet)
                        {
                        case CBASKREPLACING_ANSWER_REPLACE :
                            fReplaceAsked = TRUE;
                            break;

                        case CBASKREPLACING_ANSWER_REPLACEALL :
                            fReplaceAsked = TRUE;
                            ReplacementFileOption = REPLACEMENTFILEOPTION_REPLACE;
                            break;

                        case CBASKREPLACING_ANSWER_NOREPLACE :
                            fReplaceAsked = FALSE;
                            break;

                        case CBASKREPLACING_ANSWER_NOREPLACEALL :
                            fReplaceAsked = FALSE;
                            ReplacementFileOption = REPLACEMENTFILEOPTION_NOREPLACE;
                            break;

                        case CBASKREPLACING_ANSWER_CANCEL:
                            fReplaceAsked = FALSE;
                            fRet=FALSE;
                            break;
                        }
                    }

                    if (fReplaceAsked)
                    {
                        ((pFileCopyInfoCur->pFileCopyInfo)+dfItemToReplace)->dfReferenceItem = FTCI_REFERENCE_DELETE;
                        if ((i+1) < dwNbMapVersionMap)
                            RemoveAllReferenceFromOnItemInPreviousVersion(pVersionRemix,i+1,
                                            FALSE,
                                            dfItemToReplace);
                    }
                }

                if (fReplaceAsked)
                {
                    FILETOADD_REMIX* pftaCur = (pFileCopyInfoCur->pfta)+dfNbFileInserted;
                    DfsClearStruct(pftaCur,0,sizeof(FILETOADD_REMIX));

                    pftaCur->filename_tostore = pFileToAddInVersionsCur->dfFileNameToStore;
                    pftaCur->filename_ondisk = pFileToAddInVersionsCur->dfFileNameOnDisk;
/*
                    (pVersionAfterInstertedPosition+j)->fIsInserted = TRUE;
                    (pVersionAfterInstertedPosition+j)->dfPosition = dfNbFileInserted;
*/
                    pftaCur->fForceRecopyPrevious=FALSE;
                    pftaCur->fForceDate=FALSE;
                    pftaCur->fAddNewTag=FALSE;

                    pftaCur->pReserved = NULL;

                    pftaCur->dfPreviousVersionFilePositionItem = FTCI_REFERENCE_INDEPENDANT;
                    pftaCur->fIsReferenceInFileToAdd=FALSE;
                            /* can be FTCI_REFERENCE_INDEPENDANT */
                    if (fVersionPreviousHasInsertedFile)
                    {
                        if (VersionAfterInsertedFilePositionInPreviousVersion.dfPosition !=
                                                VERSION_AFTER_INSTERTED_FILED_POSITION_NOT_INSERTED)
                        {
                            RemoveAllReferenceFromOnItemInPreviousVersion(pVersionRemix,i,
                                                VersionAfterInsertedFilePositionInPreviousVersion.fIsInserted,
                                                VersionAfterInsertedFilePositionInPreviousVersion.dfPosition);

                            pftaCur->dfPreviousVersionFilePositionItem =
                                VersionAfterInsertedFilePositionInPreviousVersion.dfPosition;
                            pftaCur->fIsReferenceInFileToAdd =
                                VersionAfterInsertedFilePositionInPreviousVersion.fIsInserted;
                        }
                    }
                    else if (i>0)
                    {
                        PDIRINFO pPrevCurDirOrg = *((pDirInfo) + i - 1);

                        for (k=0;(k<pPrevCurDirOrg->dfNbFile) && fRet;k++)
                        {
                            if (dfUnicodeStrcmpi((pPrevCurDirOrg->pFileInDirInfo + k)->FileName,
                                (pFileToAddInVersionsCur->dfFileNameToStore)) == 0)
                            {
                                RemoveAllReferenceFromOnItemInPreviousVersion(pVersionRemix,i,
                                                    FALSE,k);

                                pftaCur->fIsReferenceInFileToAdd = FALSE;
                                pftaCur->dfPreviousVersionFilePositionItem = k;
                                break;
                            }
                        }
                    }

                    VersionAfterInsertedFilePositionInCurrentVersion.dfPosition = dfNbFileInserted;
                    VersionAfterInsertedFilePositionInCurrentVersion.fIsInserted = TRUE;

                    dfNbFileInserted++;
                    pFileCopyInfoCur->dfNbFileToAdd=dfNbFileInserted;
                }
                else
                {
                    // we keep the current version
                    VersionAfterInsertedFilePositionInCurrentVersion.dfPosition = dfItemToReplace;
                    VersionAfterInsertedFilePositionInCurrentVersion.fIsInserted = FALSE;


                    if (fVersionPreviousHasInsertedFile)
                    {
                        if (dfItemToReplace != VERSION_AFTER_INSTERTED_FILED_POSITION_NOT_INSERTED)
                        {
                            (pFileCopyInfoCur->pFileCopyInfo + dfItemToReplace)->dfReferenceItem =
                                   VersionAfterInsertedFilePositionInPreviousVersion.dfPosition;
                            (pFileCopyInfoCur->pFileCopyInfo + dfItemToReplace)->fIsReferenceInAddedFile =
                                   VersionAfterInsertedFilePositionInPreviousVersion.fIsInserted;
                        }
                    }
                }
/*
                    if (fVersionPreviousHasInsertedFile)
                    {
                        // VersionAfterInsertedFilePositionOfCurrentFile
                        if (VersionAfterInsertedFilePositionOfCurrentFile.fIsInserted)
                        {
                            FILETOADD_REMIX* pftaInVerAfter = ((pVersionRemix + i + 1)->pfta) +
                                 (VersionAfterInsertedFilePositionOfCurrentFile.dfPosition);
                            pftaInVerAfter->fIsReferenceInFileToAdd = fNeedAdding;
                            if (fNeedAdding)
                              pftaInVerAfter->dfPreviousVersionFilePositionItem = (dfNbFileInserted-1);
                            else
                              pftaInVerAfter->dfPreviousVersionFilePositionItem = k;
                        }
                        else
                        {
                            FILETOCOPYINFO_REMIX* pftciInVerAfter = ((pVersionRemix + i + 1)->pFileCopyInfo) +
                                 (VersionAfterInsertedFilePositionOfCurrentFile.dfPosition);
                            pftciInVerAfter->fIsReferenceInAddedFile = fNeedAdding;
                            if (fNeedAdding)
                              pftciInVerAfter->dfReferenceItem = (dfNbFileInserted-1);
                            else
                              pftciInVerAfter->dfReferenceItem = k;
                        }
                    }
*/
                *(pVersionAfterInstertedPositionBrowseCurrentVersion+j) = VersionAfterInsertedFilePositionInCurrentVersion;
              //
              }
            }



            fVersionPreviousHasInsertedFile=fInsertInCurrentVersion;

            {
                // swap pVersionAfterInstertedPositionBrowseCurrentVersion;
                //          and pVersionAfterInstertedPositionBrowsePreviousVersion;
                VERSIONAFTERINSTERTEDFILEDPOSITION* pSwap;
                pSwap = pVersionAfterInstertedPositionBrowseCurrentVersion;
                pVersionAfterInstertedPositionBrowseCurrentVersion = pVersionAfterInstertedPositionBrowsePreviousVersion;
                pVersionAfterInstertedPositionBrowsePreviousVersion = pSwap;
            }
        }

     if (!fRet)
        {
            for (i = 0; i < dfNbVersionRemix; i++)
            {
                FILETOCOPYINFO_REMIX *pFileCopyInfo = (pVersionRemix + i)->pFileCopyInfo;
                DfsFree(pFileCopyInfo);
            }
            DfsFree(pVersionRemix);
            pVersionRemix=NULL;
        }

    if (pVersionAfterInstertedPositionAlloc!=NULL)
      DfsFree(pVersionAfterInstertedPositionAlloc);
    DeleteFileRefIndexer(fixr);

    return pVersionRemix;
}


VERSIONTOADD_REMIX * CreateInsertingForReplaceCurrentDlsOLD(dfuLong32 dfNbVersionInDfs, PDIRINFO* pDirInfo,
                                                         INSERTFILEINVERSION_DATA* pInsertFileInVersionData,
                                                         dfuLong32 dfVersionSelected,dfuLong32 * pdfError,
                                                         tCallBackAskReplacing pCallBackAskReplacing,dfvoidp dfUserPtr)
{
        VERSIONTOADD_REMIX *pVersionRemix=NULL;
        dfuLong32 dfNbDirDfsRead = dfNbVersionInDfs;
        dfuLong32 dwNbMapVersionMap = dfNbDirDfsRead;
        dfuLong32 i;
        //dfuLong32 dfError=DFS_SUCCESS;
        dfuLong32 dfNbVersionRemix=dwNbMapVersionMap;
        BOOL fVersionAfterHasInsertedFile=FALSE;
        VERSIONAFTERINSTERTEDFILEDPOSITION* pVersionAfterInstertedPosition;
        BOOL fRet=TRUE;
        REPLACEMENTFILEOPTION ReplacementFileOption = pInsertFileInVersionData->ReplacementFileOption;
        dfuLong32 dfNbFileToAddInVersion = pInsertFileInVersionData->dfNbFileToAddInVersion ;

        pVersionRemix = BuildRecopyVersionToAdd(dfNbVersionInDfs,pDirInfo);

        pVersionAfterInstertedPosition =
             (VERSIONAFTERINSTERTEDFILEDPOSITION*)DfsMalloc(sizeof(VERSIONAFTERINSTERTEDFILEDPOSITION) *
                        (dfNbFileToAddInVersion + 1));

        // Fill a "full recopy" REMIX structure
        for (i = 0; i < dwNbMapVersionMap; i++)
        {
            //PDIRINFO pDirOrg = *((pDirInfo) + i);
            VERSIONTOADD_REMIX *pFileCopyInfoCur = pVersionRemix + i;
            pFileCopyInfoCur->pfta=(FILETOADD_REMIX *)DfsMalloc(sizeof(FILETOADD_REMIX) * (dfNbFileToAddInVersion+1));
        }
        // Now, we will have to insert file !

        if (dwNbMapVersionMap>0)
          i = dwNbMapVersionMap-1;
        else i=0;

        for (;;)
        {
            dfuLong32 j,k;
            PDIRINFO pCurDirOrg = *((pDirInfo) + i);
            VERSIONTOADD_REMIX *pFileCopyInfoCur = pVersionRemix + i;
            BOOL fInsertInCurrentVersion;
            dfuLong32 dfNbFileInserted = 0;
            BOOL fNeedAdding=TRUE;

            fInsertInCurrentVersion = *((pInsertFileInVersionData->pVersionMap)+i);
            if (fInsertInCurrentVersion)
              for (j=0;(j<dfNbFileToAddInVersion) && (fRet);j++)
              {
                FILETOADDINVERSIONS* pFileToAddInVersionsCur = (pInsertFileInVersionData->pFileToAddInVersions)+j;
                // now we will search if the file exist already. This code must be optimized later
                VERSIONAFTERINSTERTEDFILEDPOSITION VersionAfterInsertedFilePositionOfCurrentFile;

                VersionAfterInsertedFilePositionOfCurrentFile = *(pVersionAfterInstertedPosition+j);
                    /*
                VersionAfterInsertedFilePosition.fIsInserted = TRUE;
                VersionAfterInsertedFilePosition.dfPosition = dfNbFileInserted;

*/
                for (k=0;(k<pCurDirOrg->dfNbFile) && fRet;k++)
                {
                    if (dfUnicodeStrcmpi((pCurDirOrg->pFileInDirInfo + k)->FileName,
                        (pFileToAddInVersionsCur->dfFileNameToStore)) == 0)
                    {
                        // found !!
                        BOOL fReplaceAsked = TRUE ; // userinterface will be done;

                        switch(ReplacementFileOption)
                        {
                        case REPLACEMENTFILEOPTION_REPLACE :
                            fReplaceAsked = TRUE;
                            break;

                        case REPLACEMENTFILEOPTION_NOREPLACE :
                            fReplaceAsked = FALSE;
                            break;

                        case REPLACEMENTFILEOPTION_UIASKING:
                            {
                                if (pCallBackAskReplacing==NULL)
                                    fReplaceAsked=TRUE;

                                else
                                {
                                    ASKREPL_CBINFO AskRepl_CbInfo;
                                    dfuLong32 dfRet ;
                                    AskRepl_CbInfo.dfSizeStruct = sizeof(ASKREPL_CBINFO);
                                    AskRepl_CbInfo.pInsertFileInVersionData = pInsertFileInVersionData;
                                    AskRepl_CbInfo.dfVersion = i;
                                    AskRepl_CbInfo.dfItemToReplace=k;
                                    AskRepl_CbInfo.dfItemInInsertList=j;
                                    AskRepl_CbInfo.dfFileNameToStore = pFileToAddInVersionsCur->dfFileNameToStore;
                                    AskRepl_CbInfo.dfFileNameOnDisk = pFileToAddInVersionsCur->dfFileNameOnDisk;
                                    AskRepl_CbInfo.pDirInfo = pDirInfo;

                                    dfRet = (*pCallBackAskReplacing)(&AskRepl_CbInfo,dfUserPtr);
                                    switch (dfRet)
                                    {
                                    case CBASKREPLACING_ANSWER_REPLACE :
                                        fReplaceAsked = TRUE;
                                        break;

                                    case CBASKREPLACING_ANSWER_REPLACEALL :
                                        fReplaceAsked = TRUE;
                                        ReplacementFileOption = REPLACEMENTFILEOPTION_REPLACE;
                                        break;

                                    case CBASKREPLACING_ANSWER_NOREPLACE :
                                        fReplaceAsked = FALSE;
                                        break;

                                    case CBASKREPLACING_ANSWER_NOREPLACEALL :
                                        fReplaceAsked = FALSE;
                                        ReplacementFileOption = REPLACEMENTFILEOPTION_NOREPLACE;
                                        break;

                                    case CBASKREPLACING_ANSWER_CANCEL:
                                        fReplaceAsked = FALSE;
                                        fRet=FALSE;
                                        break;
                                    }
                                }
                            }
                            break;
                        }

                        if (fReplaceAsked)
                        {
                            ((pFileCopyInfoCur->pFileCopyInfo)+k)->dfReferenceItem = FTCI_REFERENCE_DELETE;
                        }
                        else
                        {
                            fNeedAdding=FALSE;
                            (pVersionAfterInstertedPosition+j)->fIsInserted = FALSE;
                            (pVersionAfterInstertedPosition+j)->dfPosition = k;
                        }
                        break;
                    }
                }

                    if (fNeedAdding)
                    {
                        FILETOADD_REMIX* pftaCur = (pFileCopyInfoCur->pfta)+dfNbFileInserted;
                        DfsClearStruct(pftaCur,0,sizeof(FILETOADD_REMIX));

                        pftaCur->filename_tostore = pFileToAddInVersionsCur->dfFileNameToStore;
                        pftaCur->filename_ondisk = pFileToAddInVersionsCur->dfFileNameOnDisk;

                        pftaCur->dfPreviousVersionFilePositionItem = FTCI_REFERENCE_INDEPENDANT;
                        pftaCur->fIsReferenceInFileToAdd=FALSE;
                             /* can be FTCI_REFERENCE_INDEPENDANT */
                        if (fVersionAfterHasInsertedFile)
                        {
                            //pftaCur->dfPreviousVersionFilePositionItem = (pVersionAfterInstertedPosition+j)->dfPosition;
                            //pftaCur->fIsReferenceInFileToAdd = (pVersionAfterInstertedPosition+j)->fIsInserted;
                        }

                        (pVersionAfterInstertedPosition+j)->fIsInserted = TRUE;
                        (pVersionAfterInstertedPosition+j)->dfPosition = dfNbFileInserted;

                        pftaCur->fForceRecopyPrevious=FALSE;
                        pftaCur->fForceDate=FALSE;
                        pftaCur->fAddNewTag=FALSE;

                        pftaCur->pReserved = NULL;

                        dfNbFileInserted++;
                    }

                    if (fVersionAfterHasInsertedFile)
                    {
                        // VersionAfterInsertedFilePositionOfCurrentFile
                        if (VersionAfterInsertedFilePositionOfCurrentFile.fIsInserted)
                        {
                            FILETOADD_REMIX* pftaInVerAfter = ((pVersionRemix + i + 1)->pfta) +
                                 (VersionAfterInsertedFilePositionOfCurrentFile.dfPosition);
                            pftaInVerAfter->fIsReferenceInFileToAdd = fNeedAdding;
                            if (fNeedAdding)
                              pftaInVerAfter->dfPreviousVersionFilePositionItem = (dfNbFileInserted-1);
                            else
                              pftaInVerAfter->dfPreviousVersionFilePositionItem = k;

                        }
                        else
                        {
                            FILETOCOPYINFO_REMIX* pftciInVerAfter = ((pVersionRemix + i + 1)->pFileCopyInfo) +
                                 (VersionAfterInsertedFilePositionOfCurrentFile.dfPosition);
                            pftciInVerAfter->fIsReferenceInAddedFile = fNeedAdding;
                            if (fNeedAdding)
                              pftciInVerAfter->dfReferenceItem = (dfNbFileInserted-1);
                            else
                              pftciInVerAfter->dfReferenceItem = k;
                        }
                    }
              //
              }

            pFileCopyInfoCur->dfNbFileToAdd=dfNbFileInserted;

            // work adding version
            if (i==0)
                break;

            fVersionAfterHasInsertedFile=fInsertInCurrentVersion;
            i--;
        }

     if (!fRet)
        {
            for (i = 0; i < dfNbVersionRemix; i++)
            {
                FILETOCOPYINFO_REMIX *pFileCopyInfo = (pVersionRemix + i)->pFileCopyInfo;
                DfsFree(pFileCopyInfo);
            }
            DfsFree(pVersionRemix);
            pVersionRemix=NULL;
        }

    if (pVersionAfterInstertedPosition!=NULL)
      DfsFree(pVersionAfterInstertedPosition);

    return pVersionRemix;
}


BOOL SVFAPI PropagateDelete(VERSIONTOADD_REMIX *pVersionRemix,dfuLong32 dfNbVersionInDfs, PDIRINFO* pDirInfo,
                     dfuLong32 dfVersionWithDeleteInfo,
                     dfuLong32 dfFirstDeleteVersion,dfuLong32 dfLastDeleteVersion,
                     BOOL fSuppressRenamed)
{
    dfuLong32 i,j;

    i = dfVersionWithDeleteInfo;//dfVersionInListView;
    for (;;)
    {
        VERSIONTOADD_REMIX *pFileCopyInfoCur;
        VERSIONTOADD_REMIX *pFileCopyInfoCurNext;
        PDIRINFO pDirCur;
        PDIRINFO pDirCurNext;

        if ((i + 1) >= (dfNbVersionInDfs))
            break;
        if (i+1>dfLastDeleteVersion)
            break;
        /*
        if (!(*(DeleteOptionDlgParam.pfVersionSelectedMap + i + 1)))
            break;
*/
        pFileCopyInfoCur = pVersionRemix + i;
        pFileCopyInfoCurNext = pVersionRemix + i + 1;
        pDirCur = *((pDirInfo) + i);
        pDirCurNext = *((pDirInfo) + i + 1);
        // now propagate deletion from i to i+1

        for (j = 0; j < pDirCurNext->dfNbFile; j++)
        {
            dfvoidp TagBuf;
            dfuLong32 TagSize;

            dfuLong32 dfStorageStatus = 0;

            if (GetTag(*((pDirCurNext)->TagFile + j), DFSTAG_STORAGESTATUS, &TagBuf, &TagSize))
            {
                dfStorageStatus = ConvertuLongIntelToLong(*(dfuLong32Intel *) TagBuf);
            }

            if (GetTag(*((pDirCurNext)->TagFile + j), DFSTAG_PREVIOUSVERSIONINFO, &TagBuf, &TagSize))
            {
                if (TagSize >= sizeof(DFSPREVIOUSVERSIONINFO))
                {
                    dfuLong32 dfPreviousVersionFileNumber, dfPreviousVersionFilePosition;
                    const DFSPREVIOUSVERSIONINFO *pDfsPreviousVersionInfo
                        = (const DFSPREVIOUSVERSIONINFO *) TagBuf;
                    dfPreviousVersionFileNumber =
                        ConvertuLongIntelToLong(pDfsPreviousVersionInfo->dfPreviousVersionFileNumber);
                    dfPreviousVersionFilePosition =
                        ConvertuLongIntelToLong(pDfsPreviousVersionInfo->
                                                dfPreviousVersionFilePosition);

                    if (dfPreviousVersionFileNumber == 1)
                    {
                        BOOL fIsLinkDeleted = FALSE;
                        if ((dfStorageStatus == DFS_STORAGESTATUS_MODIFIED) ||
                            (dfStorageStatus == DFS_STORAGESTATUS_IDENTICAL))
                            if (dfPreviousVersionFilePosition < pDirCur->dfNbFile)
                                if (((pFileCopyInfoCur->pFileCopyInfo) +
                                        dfPreviousVersionFilePosition)->dfReferenceItem ==
                                    FTCI_REFERENCE_DELETE)
                                    fIsLinkDeleted = TRUE;

                        if ((!fSuppressRenamed) && fIsLinkDeleted)
                        {
                            if (dfUnicodeStrcmpi((pDirCurNext->pFileInDirInfo + j)->FileName,
                                                    (pDirCur->pFileInDirInfo +
                                                    dfPreviousVersionFilePosition)->FileName) != 0)
                                fIsLinkDeleted = FALSE;
                        }

                        if (fIsLinkDeleted)
                        {
                            ((pFileCopyInfoCurNext->pFileCopyInfo) + j)->dfReferenceItem =
                                FTCI_REFERENCE_DELETE;
                        }
                    }
                }
            }
        }
        i++;
    }

    i = dfVersionWithDeleteInfo;//dfVersionInListView;
    for (;;)
    {
        VERSIONTOADD_REMIX *pFileCopyInfoCur;
        VERSIONTOADD_REMIX *pFileCopyInfoCurPrev;
        PDIRINFO pDirCur;
        PDIRINFO pDirCurPrev;
        if (i == 0)
            break;
        /*
        if (!(*(DeleteOptionDlgParam.pfVersionSelectedMap + i - 1)))
            break;
            */
        if (i-1<dfFirstDeleteVersion)
            break;
        // now propagate deletion from i to i-1

        pFileCopyInfoCur = pVersionRemix + i;
        pFileCopyInfoCurPrev = pVersionRemix + i - 1;
        pDirCur = *((pDirInfo) + i);
        pDirCurPrev = *((pDirInfo) + i - 1);
        // now propagate deletion from i to i-1

        for (j = 0; j < pDirCur->dfNbFile; j++)
            if (((pFileCopyInfoCur->pFileCopyInfo) + j)->dfReferenceItem == FTCI_REFERENCE_DELETE)
            {
                dfvoidp TagBuf;
                dfuLong32 TagSize;

                dfuLong32 dfStorageStatus = 0;

                if (GetTag(*((pDirCur)->TagFile + j), DFSTAG_STORAGESTATUS, &TagBuf, &TagSize))
                {
                    dfStorageStatus = ConvertuLongIntelToLong(*(dfuLong32Intel *) TagBuf);
                }

                if (GetTag(*((pDirCur)->TagFile + j), DFSTAG_PREVIOUSVERSIONINFO, &TagBuf, &TagSize))
                    if (TagSize >= sizeof(DFSPREVIOUSVERSIONINFO))
                    {
                        dfuLong32 dfPreviousVersionFileNumber, dfPreviousVersionFilePosition;
                        const DFSPREVIOUSVERSIONINFO *pDfsPreviousVersionInfo
                            = (const DFSPREVIOUSVERSIONINFO *) TagBuf;
                        dfPreviousVersionFileNumber =
                            ConvertuLongIntelToLong(pDfsPreviousVersionInfo->
                                                    dfPreviousVersionFileNumber);
                        dfPreviousVersionFilePosition =
                            ConvertuLongIntelToLong(pDfsPreviousVersionInfo->
                                                    dfPreviousVersionFilePosition);

                        if (dfPreviousVersionFileNumber == 1)
                        {
                            BOOL fIsLinkDeleted = FALSE;
                            if ((dfStorageStatus == DFS_STORAGESTATUS_MODIFIED) ||
                                (dfStorageStatus == DFS_STORAGESTATUS_IDENTICAL))
                                if (dfPreviousVersionFilePosition < pDirCurPrev->dfNbFile)
                                    fIsLinkDeleted = TRUE;

                            if ((!fSuppressRenamed) && fIsLinkDeleted)
                            {
                                if (dfUnicodeStrcmpi((pDirCur->pFileInDirInfo + j)->FileName,
                                                        (pDirCurPrev->pFileInDirInfo +
                                                        dfPreviousVersionFilePosition)->FileName) != 0)
                                    fIsLinkDeleted = FALSE;
                            }

                            if (fIsLinkDeleted)
                            {
                                ((pFileCopyInfoCurPrev->pFileCopyInfo) +
                                    dfPreviousVersionFilePosition)->dfReferenceItem =
                                            FTCI_REFERENCE_DELETE;
                            }
                        }
                    }
            }

        i--;
    }
    return TRUE;
}

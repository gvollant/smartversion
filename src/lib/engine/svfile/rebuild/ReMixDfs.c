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
/*
verifier suppression d'un fichier dans une version
veririer mettre deux fois la meme version
gestion des erreurs
et enfin ajout d'un fichier dans une version

verifier date extraction pour identique (avec nom fichier) <-> capture final...

ameliorer gestion recompress premiere version, avec fOldDfsIsFirstRef ET fNewDlsIsFirstReg . -> FAIT 95%
progress bar  -> FAIT
*/

#if defined(_DEBUG) && (!defined(MSGOUTTEST))
#define MSGOUTTEST_nono
#endif

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

#include "../../patchstream/common/dfsOrigMemoryMap.h"

#include "../../patchstream/rebuild/RamDifWk.h"
#include "../../patchstream/rebuild/RamDifTl.h"
#include "../../patchstream/rebuild/RamDifWS.h"

#include "../compress/DfsWrSet.h"


#include "../decompress/DfsRdSet.h"
//#include "DfsCdLin.h"
#include "../common/ArrayTl.h"



#include "../../patchstream/common/DfsIoHlp.h"

#include "zlib.h"
#include "zip.h"



#include "../common/DirSet.h"
#include "../decompress/DoExtracting.h"
#include "ReMixDfs.h"
#include "../common/FileRefCounter.h"

/* #define _TRYACCELERATE */

/* dfConstructionInfo contain the number of file in previous version from which
       we made patch, of one of these constant : */

#define FIODI_CONSTR_JOKER_BASE     ((dfuLong32)(0xffffff00UL))

/* for file stored only as reference, with CRC (in first version) :*/
#define FIODI_CONSTR_REFERENCE      ((dfuLong32)(0xffffff01UL))

/* for file stored with content, uncompressed or compressed :*/
#define FIODI_CONSTR_NEW            ((dfuLong32)(0xffffff02UL))


/* dfParameter can be a combination of */
#define FIODI_PARAM_PATCH_MODIFED   ((dfuLong32)0x00000001)
#define FIODI_PARAM_PATCH_IDENTICAL ((dfuLong32)0x00000002)

/* sometime, we absolutely request a rebuild */
#define FIODI_PARAM_PATCH_REBUILD   ((dfuLong32)0x00000003)

/* FIODI_PARAM_PATCH_MASK is for do a OR (|) test on patch */
#define FIODI_PARAM_PATCH_MASK      ((dfuLong32)0x00000003)

#define FIODI_PARAM_REFERENCE       ((dfuLong32)0x00000004)
#define FIODI_PARAM_NEW             ((dfuLong32)0x00000008)

#define FIODI_PARAM_STOREINFO_MASK  ((dfuLong32)0x000000ff)

/* are filled when we select the file we want extract */
#define FIODI_NEEDED_FOR_EXTRACTING         ((dfuLong32)0x00010000)
#define FIODI_NEEDED_FOR_BLDNEWDFS_TARGET   ((dfuLong32)0x00020000)
#define FIODI_NEEDED_FOR_BLDNEWDFS_ORIGIN   ((dfuLong32)0x00040000)
#define FIODI_CONTENT_NEEDED_MASK           ((dfuLong32)0x00070000)

// for BUGBUG
#define FIODI_RAMDIF_COMBINING_NEEDED       ((dfuLong32)0x00080000)

typedef struct
{
    dfuLong32 dfConstructionInfo;
    dfuLong32 dfParameter;
    dfuLong32 dfStorageStatus;    /* the DFSTAG_STORAGESTATUS tag value (method) */
}
FILE_IN_OLD_DFS_INFO;

typedef struct
{
    dfuLong32 dfNbFile;
    FILE_IN_OLD_DFS_INFO *pfiodi;
}
DIR_IN_OLD_DFS_INFO;

/*******************************************************************/



typedef struct
{
    /* dfOldDfsFileItem contain the item of the file in the Old DFS
       (the number stored in old DFS file),
       or FINDI_PREVDFS_FILEADDED for an added file */
    dfuLong32 dfOldDfsFileItem;
    dfuLong32 dfBldParameter;


    /* dfPreviousVersionItemInOldSvf contain the item of previous version, in the
       previous directory WE COPY of old DFS or FINDI_PREVVERINNEWDFS_FILEADDED */
    dfuLong32 dfPreviousVersionItemInOldSvf;

    /* dfPreviousVersionItemInNewSvf contain the item of previous version, in the
       previous directory of new DFS, for store in the directory we will build */
    dfuLong32 dfPreviousVersionItemInNewSvf;

    /* location of previous version in previous DFS (useful for REBUILD PATCH) */
    //dfuLong32 dfOldDfsPreviousDirLocation;
    //dfuLong32 dfOldDfsPreviousFileLocation;

    /* if we copy the raw compressed stream (FINDI_PARAM_MASK_RAWCOPY), location of the stream */
    dfuLong32 dfOldDfsRawCopyDirLocation;
    dfuLong32 dfOldDfsRawCopyFileLocation;
    const FILETOADD_REMIX *pftaPreviousItem;
    const FILETOADD_REMIX *pfta;
}
FILE_IN_NEW_DFS_INFO;


#define FINDI_PREVDFS_FILEADDED         ((dfuLong32)(0xffffffffUL))

#define FINDI_PREVVERINNEWDFS_FILEADDED ((dfuLong32)(0xffffffffUL))

#define FINDI_PREVVERINOLDDFS_FILEADDED ((dfuLong32)(0xffffffffUL))

/* dfBldParameter is combination of*/
/* 1: we select the kind and construction method */
#define FINDI_PARAM_REFERENCE       ((dfuLong32)0x00000004)

#define FINDI_PARAM_NEW             ((dfuLong32)0x00000008)
#define FINDI_PARAM_NEW_RAWCOPY     ((dfuLong32)0x00000018)
#define FINDI_PARAM_NEW_RECOMPRESS  ((dfuLong32)0x00000028)

#define FINDI_PARAM_PATCH           ((dfuLong32)0x00000001)
#define FINDI_PARAM_PATCH_RAWCOPY   ((dfuLong32)0x00000011)
#define FINDI_PARAM_PATCH_REBUILD   ((dfuLong32)0x00000021)

#define FINDI_PARAM_PATCH_COMBINE   ((dfuLong32)0x00000041)


#define FINDI_PARAM_PATCH_IDENTICAL ((dfuLong32)0x00000002)

#define FINDI_PARAM_PATCH_MASK      ((dfuLong32)0x00000003)


#define FINDI_PARAM_MASK_RAWCOPY    ((dfuLong32)0x00000010)
#define FINDI_PARAM_MASK_REBUILD    ((dfuLong32)0x00000020)

#define FINDI_PARAM_MASK_BUILDMETH  ((dfuLong32)0x000000ff)


#define AFTER_LAST_DIRECTORY        ((dfuLong32)(0xffffffffUL))

typedef struct
{
    dfuLong32 dfNbFile;
    dfuLong32 dfNbFileReuse;
    dfuLong32 dfNbFileAdding;

    dfuLong32 dfNumDirVersionOldSvf;
    FILE_IN_NEW_DFS_INFO *pfindi;
}
DIR_IN_NEW_DFS_INFO;


static DIR_IN_OLD_DFS_INFO *BuildDirInOldArray(dfuLong32 dfNbDirDfsRead, const PCDIRINFO * pDirInfo)
{
    DIR_IN_OLD_DFS_INFO *pDiodi;
    BOOL fBaseNeeded;
    dfuLong32 dfNumDirOldDfs;
    dfuLong32 i, j;
    /* Todo : First, checking consistency of parameter */

    fBaseNeeded = ((*pDirInfo)->dfTypeDir == TYPEDIR_FILECRCONLY);
    dfNumDirOldDfs = dfNbDirDfsRead;
    pDiodi = (DIR_IN_OLD_DFS_INFO *)DfsMalloc(sizeof(DIR_IN_OLD_DFS_INFO) * (dfNumDirOldDfs + 1));

    (pDiodi + dfNumDirOldDfs)->dfNbFile = AFTER_LAST_DIRECTORY;
    (pDiodi + dfNumDirOldDfs)->pfiodi = NULL;

    for (i = 0; i < dfNumDirOldDfs; i++)
    {
        dfuLong32 dfNbFileCurDir = (*(pDirInfo + i))->dfNbFile;
        (pDiodi + i)->dfNbFile = dfNbFileCurDir;
        (pDiodi + i)->pfiodi =
            (FILE_IN_OLD_DFS_INFO *) DfsMalloc(sizeof(FILE_IN_OLD_DFS_INFO) * (dfNbFileCurDir + 1));

        for (j = 0; j < dfNbFileCurDir; j++)
        {
            FILE_IN_OLD_DFS_INFO *pFiodiCur = (((pDiodi + i)->pfiodi)) + j;
            //const FILEINDIRINFO *pCurFileInDirInfo = (((*(pDirInfo + i))->pFileInDirInfo) + j);
            dfvoidp TagBuf;
            dfuLong32 TagSize;

            pFiodiCur->dfConstructionInfo = (fBaseNeeded && (i == 0)) ? FIODI_CONSTR_REFERENCE : FIODI_CONSTR_NEW;      // TO BE CHECKED
            pFiodiCur->dfParameter = (fBaseNeeded && (i == 0)) ? FIODI_PARAM_REFERENCE : FIODI_PARAM_NEW;
            pFiodiCur->dfStorageStatus = 0;

            if (GetTag(*((*(pDirInfo + i))->TagFile + j), DFSTAG_STORAGESTATUS, &TagBuf, &TagSize))
            {
                pFiodiCur->dfStorageStatus = ConvertuLongIntelToLong(*(dfuLong32Intel *) TagBuf);
            }

            if (i != 0)
            {
                if (GetTag(*((*(pDirInfo + i))->TagFile + j), DFSTAG_PREVIOUSVERSIONINFO, &TagBuf, &TagSize))
                {
                    if (TagSize >= sizeof(DFSPREVIOUSVERSIONINFO))
                    {
                        dfuLong32 dfPreviousVersionFileNumber, dfPreviousVersionFilePosition;
                        const DFSPREVIOUSVERSIONINFO *pDfsPreviousVersionInfo
                            = (const DFSPREVIOUSVERSIONINFO *) TagBuf;
                        dfPreviousVersionFileNumber =
                            ConvertuLongIntelToLong(pDfsPreviousVersionInfo->dfPreviousVersionFileNumber);
                        dfPreviousVersionFilePosition =
                            ConvertuLongIntelToLong(pDfsPreviousVersionInfo->dfPreviousVersionFilePosition);

                        pFiodiCur->dfConstructionInfo = dfPreviousVersionFilePosition;

                        if (dfPreviousVersionFileNumber == 1)
                        {
                            if (pFiodiCur->dfStorageStatus == DFS_STORAGESTATUS_MODIFIED)
                                pFiodiCur->dfParameter = FIODI_PARAM_PATCH_MODIFED;

                            if (pFiodiCur->dfStorageStatus == DFS_STORAGESTATUS_IDENTICAL)
                                pFiodiCur->dfParameter = FIODI_PARAM_PATCH_IDENTICAL;

/* bug on identic beg */


                            if (pFiodiCur->dfStorageStatus == DFS_STORAGESTATUS_IDENTICAL)
                            {
                            dfvoidp TagBuf;
                            dfuLong32 TagSize;
                            PCDIRINFO pDirInfoCur=*(pDirInfo + i);
                            PCDIRINFO pDirInfoPrev=*(pDirInfo + i-1);
                            FILEINDIRINFO* pFileInDirInfoCur = (pDirInfoCur->pFileInDirInfo+j);

                                if (GetTag(*(pDirInfoCur->TagFile + j), DFSTAG_STORAGESTATUS, &TagBuf, &TagSize))
                                {
                                    if (TagSize==sizeof(dfuLong32Intel))
                                    {
                                        dfuLong32 dfFileIdentical = ConvertuLongIntelToLong(*(dfuLong32Intel*)TagBuf);
                                        if (dfFileIdentical==DFS_STORAGESTATUS_IDENTICAL)
                                            if (GetTag
                                                (*(pDirInfoCur->TagFile + j), DFSTAG_PREVIOUSVERSIONINFO, &TagBuf,
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
                                                if ((pFileInDirInfoCur->dfSize != pFileInDirInfoPrev->dfSize) && (pFileInDirInfoCur->dfFileEncodedSize>0))
                                                {
                                                    pFiodiCur->dfParameter = FIODI_PARAM_PATCH_REBUILD;
                                                }
                                                if ((pFileInDirInfoCur->dfSize != pFileInDirInfoPrev->dfSize))
                                                {
                                                    pFiodiCur->dfParameter = FIODI_PARAM_PATCH_REBUILD;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
/* bug on identic end */


                        }
                    }
                }
            }
#if defined(_DEBUG) && defined(MSGOUTTEST) && defined(TCHAR)
            {
                dfvoidp TagBuf;
                dfuLong32 TagSize;
                TCHAR szLine[MAX_PATH];
                szLine[0] = '\0';
                if (GetTag(*((*(pDirInfo + i))->TagFile + j), DFSTAG_FILENAME, &TagBuf, &TagSize))
                    wsprintf(szLine, "(%02x,%03x),'%ws' %lx %lx\n", i, j,
                             TagBuf, pFiodiCur->dfConstructionInfo, pFiodiCur->dfParameter);
                //OutputDebugString(szLine);
            }
#endif

        }
    }

    return pDiodi;
}


static void FreeDirInOldArray(DIR_IN_OLD_DFS_INFO * pDiodi)
{
    if (pDiodi != NULL)
    {
        dfuLong32 i = 0;
        while ((pDiodi + i)->dfNbFile != AFTER_LAST_DIRECTORY)
        {
            DfsFree((pDiodi + i)->pfiodi);
            i++;
        }
        DfsFree(pDiodi);
    }
}

/************************************************/

static DIR_IN_NEW_DFS_INFO *BuildDirInNewArray(
                                        dfuLong32 dfNbDirDfsRead,
                                        const DIR_IN_OLD_DFS_INFO * pDiodi,
                                        dfuLong32 dfNbVersionRemix,
                                        const VERSIONTOADD_REMIX *
                                        pVersionRemix, BOOL fFirstVersionAsReferenceNewDfs,
                                        BOOL fBaseDirectorySelected, dfuLong32 dfBaseDirNum,
                                        BOOL fReuseOldPatch,BOOL fRawAccepted)
{
    DIR_IN_NEW_DFS_INFO *pDindi;
    dfuLong32 i;
    dfuLong32 dfSizeArray;
    dfuLong32 dfNumDirPreviousVersionOldSvf;
    dfuLong32 dfNbFileReusePreviousVersion = 0;

    /* FINDI_PARAM_PATCH_REBUILD or FINDI_PARAM_PATCH_COMBINE */
    dfuLong32 dfF_Param_Patch_RebuildOrCombine = fReuseOldPatch ? FINDI_PARAM_PATCH_COMBINE : FINDI_PARAM_PATCH_REBUILD;

// to force classic stuff
//dfF_Param_Patch_RebuildOrCombine=FINDI_PARAM_PATCH_REBUILD;

    dfSizeArray = sizeof(DIR_IN_NEW_DFS_INFO) * (dfNbVersionRemix + 1);
    pDindi = (DIR_IN_NEW_DFS_INFO *)DfsMalloc(dfSizeArray);
    DfsClearStruct(pDindi, 0, dfSizeArray);
    (pDindi + dfNbVersionRemix)->dfNbFile = AFTER_LAST_DIRECTORY;

#define FIRST_VERSION ((dfuLong32)(0xffffffffUL))
    dfNumDirPreviousVersionOldSvf = FIRST_VERSION;

    for (i = 0; i < dfNbVersionRemix; i++)
    {
        const VERSIONTOADD_REMIX *pCurVersionRemix = pVersionRemix + i;
        dfuLong32 dfNbFileReuse = 0;
        dfuLong32 dfNbFileAdding;
        dfuLong32 dfNbFileCurDir;
        dfuLong32 dfNumDirVersionOldSvf = (pVersionRemix + i)->dfNumVersionPreviousSvf;
        dfuLong32 dfCurFile;
        dfuLong32 x;

        for (x = 0; x < pCurVersionRemix->dfNbPreviousFileInMask; x++)
        {
            const FILETOCOPYINFO_REMIX *pFileCopyInfo = ((pCurVersionRemix->pFileCopyInfo) + x);

            if (pFileCopyInfo->dfReferenceItem != FTCI_REFERENCE_DELETE)
                dfNbFileReuse++;
        }
        dfNbFileAdding = pCurVersionRemix->dfNbFileToAdd;
        dfNbFileCurDir = dfNbFileReuse + dfNbFileAdding;

        dfSizeArray = sizeof(FILE_IN_NEW_DFS_INFO) * (dfNbFileCurDir + 1);
        (pDindi + i)->pfindi = (FILE_IN_NEW_DFS_INFO *) DfsMalloc(dfSizeArray);
        DfsClearStruct((pDindi + i)->pfindi, 0, dfSizeArray);

        (pDindi + i)->dfNbFileReuse = dfNbFileReuse;
        (pDindi + i)->dfNbFileAdding = dfNbFileAdding;
        (pDindi + i)->dfNbFile = dfNbFileCurDir;
        (pDindi + i)->dfNumDirVersionOldSvf = dfNumDirVersionOldSvf;

        dfCurFile = 0;

        for (x = 0; x < pCurVersionRemix->dfNbPreviousFileInMask; x++)
        {
            dfuLong32 dfReferenceItemAsked;
            FILE_IN_NEW_DFS_INFO *pfindiCur = ((pDindi + i)->pfindi) + dfCurFile;
            const FILETOCOPYINFO_REMIX *pFileCopyInfo = ((pCurVersionRemix->pFileCopyInfo) + x);
            BOOL fDeleteItemAsked = (pFileCopyInfo->dfReferenceItem == FTCI_REFERENCE_DELETE);
            dfReferenceItemAsked = pFileCopyInfo->dfReferenceItem;

            pfindiCur->pfta = NULL;
            pfindiCur->pftaPreviousItem = NULL;


            if (!fDeleteItemAsked)
            {/*
                pfindiCur->dfBldParameter = FINDI_PARAM_NEW_RECOMPRESS;

                if (i == 0)
                {
                    if (fFirstVersionAsReferenceNewDfs)
                    {
                        pfindiCur->dfBldParameter = FINDI_PARAM_REFERENCE ;
                    }
                    else
                        pfindiCur->dfBldParameter = FINDI_PARAM_NEW_RECOMPRESS ;
                }
*/
                pfindiCur->dfOldDfsFileItem = x;
            }

            if ((i == 0) && (!fDeleteItemAsked))
            {
                if (fFirstVersionAsReferenceNewDfs)
                {
                    pfindiCur->dfBldParameter = FINDI_PARAM_REFERENCE ;
                    pfindiCur->dfOldDfsRawCopyDirLocation = pVersionRemix->dfNumVersionPreviousSvf;
                    pfindiCur->dfOldDfsRawCopyFileLocation = x ;
                }
                else
                {
                    if (fBaseDirectorySelected)
                        pfindiCur->dfBldParameter = FINDI_PARAM_NEW_RECOMPRESS;
                    else
                    {
                        // new SVF need a compressed version, and old SVF contain also compressed version

                        if ((pVersionRemix->dfNumVersionPreviousSvf == 0) && (fRawAccepted))
                        {
                            pfindiCur->dfBldParameter = FINDI_PARAM_NEW_RAWCOPY;
                            pfindiCur->dfOldDfsRawCopyDirLocation = 0;
                            pfindiCur->dfOldDfsRawCopyFileLocation = x ;
                        }
                        else
                        {
                            pfindiCur->dfBldParameter = FINDI_PARAM_NEW_RECOMPRESS;
                            // can be better : we can check if all patch are identical patch
                        }
                    }
                }
            }

            if ((i > 0) && (!fDeleteItemAsked) && ((pFileCopyInfo->fIsReferenceInAddedFile)))
            {
                pfindiCur->dfBldParameter = FINDI_PARAM_PATCH_REBUILD;
                pfindiCur->dfPreviousVersionItemInNewSvf = dfNbFileReusePreviousVersion + dfReferenceItemAsked;
                pfindiCur->dfPreviousVersionItemInOldSvf = FINDI_PREVVERINNEWDFS_FILEADDED;
                pfindiCur->pftaPreviousItem =
                        ((pVersionRemix + i -1)->pfta)+dfReferenceItemAsked;
            }

            if ((i > 0) && (!fDeleteItemAsked) && (!(pFileCopyInfo->fIsReferenceInAddedFile)))
            {
                //dfuLong32 dfPreviousVersionItemInNewSvf = FINDI_PREVVERINNEWDFS_FILEADDED;


                /* we will determinate the item of previous version, in the
                   previous directory of new DFS, for store in the directory we will build */
                dfuLong32 dfNbRealPatch = 0;
                dfuLong32 dfFilePositionPreviousInOldSvf = 0;
                dfuLong32 dfDirOldSvf = dfNumDirVersionOldSvf;
                dfuLong32 dfOldDfsRawCopyDirLocation, dfOldDfsRawCopyFileLocation;
                BOOL fRawCopyPossible = TRUE;
                BOOL fIsPatch = TRUE;

                dfFilePositionPreviousInOldSvf = x;
                dfOldDfsRawCopyDirLocation = dfOldDfsRawCopyFileLocation = FIRST_VERSION;

                while ((dfDirOldSvf > dfNumDirPreviousVersionOldSvf) &&
                       (dfFilePositionPreviousInOldSvf != FINDI_PREVVERINNEWDFS_FILEADDED))
                {
                    FILE_IN_OLD_DFS_INFO *pfiodi;

                    if (dfFilePositionPreviousInOldSvf > ((pDiodi + dfDirOldSvf)->dfNbFile))
                    {
                        return NULL;    // Internal error, we must free memory
                    }

                    // TODO : replace BOOL map by rich map, support added file
                    pfiodi = (((pDiodi + dfDirOldSvf)->pfiodi) + dfFilePositionPreviousInOldSvf);
                    if (((pfiodi->dfParameter) & FIODI_PARAM_STOREINFO_MASK) == FIODI_PARAM_PATCH_MODIFED)
                    {
                        dfNbRealPatch++;
                        if ((dfNbRealPatch == 1) && fReuseOldPatch)
                        {
                            /* useful only if dfNbRealPatch = 1 */
                            dfOldDfsRawCopyDirLocation = dfDirOldSvf;
                            dfOldDfsRawCopyFileLocation = dfFilePositionPreviousInOldSvf;
                        }
                        else
                            fRawCopyPossible = FALSE;
                    }

                    // FIODI_PARAM_PATCH_REBUILD is for identical bug in 1.0
                    if (((pfiodi->dfParameter) & FIODI_PARAM_STOREINFO_MASK) == FIODI_PARAM_PATCH_REBUILD)
                    {
                        fRawCopyPossible = FALSE;
                        dfNbRealPatch = 0xff;
                    }

                    if ((((pfiodi->dfParameter) & FIODI_PARAM_STOREINFO_MASK) == FIODI_PARAM_NEW) ||
                        (((pfiodi->dfParameter) & FIODI_PARAM_STOREINFO_MASK) == FIODI_PARAM_REFERENCE))
                    {
                        fIsPatch = FALSE;
                        if (dfNbRealPatch == 0)
                        {
                            // we will rawcopy the compress info
                            dfOldDfsRawCopyDirLocation = dfDirOldSvf;
                            dfOldDfsRawCopyFileLocation = dfFilePositionPreviousInOldSvf;
                        }
                        else
                            fRawCopyPossible = FALSE;
                        break;  // do not search before!
                    }

                    dfFilePositionPreviousInOldSvf = pfiodi->dfConstructionInfo;
                    if (dfFilePositionPreviousInOldSvf >= FIODI_CONSTR_JOKER_BASE)
                    {
                        dfFilePositionPreviousInOldSvf = FINDI_PREVVERINNEWDFS_FILEADDED;
                        //pfindiCur->pftaPreviousItem=NULL;
                        fIsPatch = FALSE;
                    }
                    dfDirOldSvf--;
                }

                if (fIsPatch)
                {
                    if (dfNbRealPatch == 0)
                        pfindiCur->dfBldParameter = FINDI_PARAM_PATCH_IDENTICAL;
                    else if (fRawCopyPossible && fRawAccepted)
                        pfindiCur->dfBldParameter = FINDI_PARAM_PATCH_RAWCOPY;
                    else
                        pfindiCur->dfBldParameter = dfF_Param_Patch_RebuildOrCombine;
                }
                else
                {
                    if (fRawCopyPossible && fRawAccepted)
                        pfindiCur->dfBldParameter = FINDI_PARAM_NEW_RAWCOPY;
                    else
                        pfindiCur->dfBldParameter = FINDI_PARAM_NEW_RECOMPRESS;
                }

                // fix the bug by remplace dfFilePosition by pfiodi
                //  dfFilePosition is a bad variable name
                ///pfindiCur->dfPreviousVersionItemInOldSvf = dfFilePosition;
                pfindiCur->dfPreviousVersionItemInOldSvf = dfFilePositionPreviousInOldSvf;

#if defined(MSGOUTTEST) && defined(_DEBUG)
                printf
                    (" -> previous version %u,file %u, dfFilePositionInOldSvf=%u ,dfFilePositionPreviousInOldSvf=%u\n",
                     i, x, dfFilePositionPreviousInOldSvf, dfFilePositionPreviousInOldSvf);
#endif

                if (dfReferenceItemAsked == FTCI_REFERENCE_INDEPENDANT)
                    pfindiCur->dfBldParameter = FINDI_PARAM_NEW_RECOMPRESS;

                if (dfReferenceItemAsked == FTCI_REFERENCE_UNMODIFIED)
                    dfReferenceItemAsked = dfFilePositionPreviousInOldSvf;

                if ((dfReferenceItemAsked != dfFilePositionPreviousInOldSvf) &&
                    (((pfindiCur->dfBldParameter) & FINDI_PARAM_PATCH_MASK) != 0))
                {
                    pfindiCur->dfPreviousVersionItemInOldSvf =
                        (dfReferenceItemAsked < ((pDiodi + dfNumDirPreviousVersionOldSvf)->dfNbFile)) ?
                        dfReferenceItemAsked : FINDI_PREVVERINNEWDFS_FILEADDED;

                    pfindiCur->dfBldParameter = dfF_Param_Patch_RebuildOrCombine;
                    #if defined(MSGOUTTEST) && defined(_DEBUG)
                    printf("rebuild different link\n");
                    #endif
                }

                if ((pfindiCur->dfBldParameter & FINDI_PARAM_PATCH_MASK) != 0)
                {
                    const VERSIONTOADD_REMIX *pBeforCurVersionRemix = pVersionRemix + i - 1;

                    /* if we delete the previous file, we must store it as "new" */
                    if (pfindiCur->dfPreviousVersionItemInOldSvf != FINDI_PREVVERINNEWDFS_FILEADDED)
                        if (((pBeforCurVersionRemix->pFileCopyInfo) +
                             (dfReferenceItemAsked))->dfReferenceItem == FTCI_REFERENCE_DELETE)
                        {
                            pfindiCur->dfBldParameter = FINDI_PARAM_NEW_RECOMPRESS;
                        }
                }

                if ((pfindiCur->dfBldParameter & FINDI_PARAM_PATCH_MASK) != 0)
                {
                    const VERSIONTOADD_REMIX *pBeforCurVersionRemix = pVersionRemix + i - 1;
                    dfuLong32 k;
                    pfindiCur->dfPreviousVersionItemInOldSvf = dfReferenceItemAsked;

                    if (dfReferenceItemAsked < ((pDiodi + dfNumDirPreviousVersionOldSvf)->dfNbFile))
                    {
                        pfindiCur->dfPreviousVersionItemInNewSvf = 0;
                        for (k = 0; k < dfReferenceItemAsked; k++)
                        {
                            const FILETOCOPYINFO_REMIX *pFileCopyInfo =
                                ((pBeforCurVersionRemix->pFileCopyInfo) + k);

                            if (pFileCopyInfo->dfReferenceItem != FTCI_REFERENCE_DELETE)
                                (pfindiCur->dfPreviousVersionItemInNewSvf)++;
                        }
                    }
                    else
                    {
                        dfuLong32 dfNbFileInPreviousInOldDfs =
                            ((pDiodi + dfNumDirPreviousVersionOldSvf)->dfNbFile);
                        pfindiCur->dfPreviousVersionItemInNewSvf = dfReferenceItemAsked;
                        for (k = 0; k < dfNbFileInPreviousInOldDfs; k++)
                        {
                            const FILETOCOPYINFO_REMIX *pFileCopyInfo =
                                ((pBeforCurVersionRemix->pFileCopyInfo) + k);

                            if (pFileCopyInfo->dfReferenceItem == FTCI_REFERENCE_DELETE)
                                (pfindiCur->dfPreviousVersionItemInNewSvf)--;
                        }
                    }
                }


                /* if rawcopy, store rawcopy info */
                if ((pfindiCur->dfBldParameter & FINDI_PARAM_MASK_RAWCOPY) != 0)
                {
                    pfindiCur->dfOldDfsRawCopyDirLocation = dfOldDfsRawCopyDirLocation;
                    pfindiCur->dfOldDfsRawCopyFileLocation = dfOldDfsRawCopyFileLocation;
                }
            }
            if (!fDeleteItemAsked)
                dfCurFile++;
        }

        for (x = 0; x < pCurVersionRemix->dfNbFileToAdd; x++)
        {
            dfuLong32 dfReferenceItemAsked;
            FILE_IN_NEW_DFS_INFO *pFindiCur = ((pDindi + i)->pfindi) + dfCurFile;
            const FILETOADD_REMIX *pftaCur = ((pCurVersionRemix->pfta) + x);

            dfReferenceItemAsked = pftaCur->dfPreviousVersionFilePositionItem;

            pFindiCur->dfOldDfsFileItem = FINDI_PREVDFS_FILEADDED;
            pFindiCur->dfPreviousVersionItemInOldSvf = FINDI_PREVVERINNEWDFS_FILEADDED;

            pFindiCur->pfta = pftaCur;
            if ((i == 0) || (dfReferenceItemAsked == FTCI_REFERENCE_INDEPENDANT))
            {
                pFindiCur->dfBldParameter =
                    (fFirstVersionAsReferenceNewDfs && (i == 0)) ?
                    FINDI_PARAM_REFERENCE : FINDI_PARAM_NEW_RECOMPRESS;
            }
            else
            {
                dfuLong32 k;
                dfuLong32 dfNbFileInPreviousInOldDfs = ((pDiodi + dfNumDirPreviousVersionOldSvf)->dfNbFile);

//#error must test fIsReferenceInFileToAdd

                pFindiCur->dfBldParameter = dfF_Param_Patch_RebuildOrCombine;
                if (!pftaCur->fIsReferenceInFileToAdd)
                {
                    pFindiCur->dfPreviousVersionItemInOldSvf = dfReferenceItemAsked;
                    pFindiCur->dfPreviousVersionItemInNewSvf = dfReferenceItemAsked;

                    for (k = 0; k < dfNbFileInPreviousInOldDfs; k++)
                    {
                        const VERSIONTOADD_REMIX *pBeforCurVersionRemix = pVersionRemix + i - 1;
                        const FILETOCOPYINFO_REMIX *pFileCopyInfo = ((pBeforCurVersionRemix->pFileCopyInfo) + k);

                        if (pFileCopyInfo->dfReferenceItem == FTCI_REFERENCE_DELETE)
                            (pFindiCur->dfPreviousVersionItemInNewSvf)--;
                    }
                }
                else
                {
                    BOOL fIsFileNameOnDiskSame = FALSE;
                    pFindiCur->dfPreviousVersionItemInNewSvf = dfNbFileReusePreviousVersion + dfReferenceItemAsked;
                    pFindiCur->dfPreviousVersionItemInOldSvf = FINDI_PREVVERINNEWDFS_FILEADDED;

                    pFindiCur->pftaPreviousItem =
                        ((pVersionRemix + i -1)->pfta)+dfReferenceItemAsked;

                    // compare pfindiCur->pfta->filename_ondisk and pfindiCur->pftaPreviousItem->filename_ondisk

                    if (dfUnicodeStrcmp(pFindiCur->pfta->filename_ondisk,pFindiCur->pftaPreviousItem->filename_ondisk)==0)
                        fIsFileNameOnDiskSame = TRUE;
                    if (fIsFileNameOnDiskSame == TRUE)
                        pFindiCur->dfBldParameter = FINDI_PARAM_PATCH_IDENTICAL;
                }
            }

            dfCurFile++;
        }

        dfNumDirPreviousVersionOldSvf = dfNumDirVersionOldSvf;
        dfNbFileReusePreviousVersion = dfNbFileReuse;
    }

    (pDindi + dfNbVersionRemix)->dfNbFile = AFTER_LAST_DIRECTORY;

    return pDindi;
}


void FreeDirInNewArray(DIR_IN_NEW_DFS_INFO * pDindi)
{
    if (pDindi != NULL)
    {
        dfuLong32 i = 0;
        while ((pDindi + i)->dfNbFile != AFTER_LAST_DIRECTORY)
        {
            DfsFree((pDindi + i)->pfindi);
            i++;
        }
        DfsFree(pDindi);
    }
}

static BOOL BuildDependecyInOldDfs(dfuLong32 dfNbDirDfsRead,
                                   DIR_IN_OLD_DFS_INFO * pDiodi,
                                   dfuLong32 dfNbVersionRemix, const DIR_IN_NEW_DFS_INFO * pDindi)
{
    dfuLong32 i, j;


    /* first : we look for each REBUILD or RECOMPRESS item in NEW_DFS
       what item need to be extracted from old SVF */
    for (i = 0; i < dfNbVersionRemix; i++)
    {
        const DIR_IN_NEW_DFS_INFO *pDindiCur = pDindi + i;
        dfuLong32 dfOldVersionNumber = pDindiCur->dfNumDirVersionOldSvf;

        /* to known where to start combining */
        dfuLong32 dfFirstVersionForCombining = 0;
        if (i>0)
            dfFirstVersionForCombining = ((pDindi + i-1)->dfNumDirVersionOldSvf)+1;

        for (j = 0; j < pDindiCur->dfNbFile; j++)
        {
            const FILE_IN_NEW_DFS_INFO *pFindiCur = (pDindiCur->pfindi) + j;
            dfuLong32 dfBldParameterItem = pFindiCur->dfBldParameter;
            if (dfBldParameterItem == FINDI_PARAM_PATCH_COMBINE)
            {
                dfuLong32 dfOldDfsFileItem = pFindiCur->dfOldDfsFileItem;
                if (dfOldDfsFileItem != FINDI_PREVDFS_FILEADDED)
                {
                    dfuLong32 dfDirOldSvfBrowse,dfFilePositionPreviousInOldSvfBrowse ;
                    FILE_IN_OLD_DFS_INFO *pdiodi =
                        (((pDiodi + dfOldVersionNumber)->pfiodi) + dfOldDfsFileItem);
                    pdiodi->dfParameter |= FIODI_RAMDIF_COMBINING_NEEDED; // BUGBUG pehaps

                    dfDirOldSvfBrowse = dfOldVersionNumber;
                    dfFilePositionPreviousInOldSvfBrowse = dfOldDfsFileItem;

                    while (dfDirOldSvfBrowse >= dfFirstVersionForCombining)
                    {
                        FILE_IN_OLD_DFS_INFO *pfiodiBrowse ;
                        if (dfFilePositionPreviousInOldSvfBrowse >= FIODI_CONSTR_JOKER_BASE)
                            break;
                        pfiodiBrowse = (((pDiodi + dfDirOldSvfBrowse)->pfiodi) + dfFilePositionPreviousInOldSvfBrowse);
                        pfiodiBrowse->dfParameter |= FIODI_RAMDIF_COMBINING_NEEDED;


                        dfFilePositionPreviousInOldSvfBrowse = pfiodiBrowse->dfConstructionInfo;
                        dfDirOldSvfBrowse--;
                    }

                }
            }

            if ((dfBldParameterItem == FINDI_PARAM_NEW_RECOMPRESS) ||
                (dfBldParameterItem == FINDI_PARAM_PATCH_REBUILD))
            {
                /* dfOldDfsFileItem contain the item of the file in the Old DFS
                   (the number stored in old DFS file),
                   or FINDI_PREVDFS_FILEADDED for an added file */
                dfuLong32 dfOldDfsFileItem = pFindiCur->dfOldDfsFileItem;
                if (dfOldDfsFileItem != FINDI_PREVDFS_FILEADDED)
                {
                    FILE_IN_OLD_DFS_INFO *pdiodi =
                        (((pDiodi + dfOldVersionNumber)->pfiodi) + dfOldDfsFileItem);
                    pdiodi->dfParameter |= FIODI_NEEDED_FOR_BLDNEWDFS_TARGET;
                }

                if ((dfBldParameterItem == FINDI_PARAM_PATCH_REBUILD) &&
                    (pFindiCur->dfPreviousVersionItemInOldSvf != FINDI_PREVVERINNEWDFS_FILEADDED) && (i > 0))
                {
                    const DIR_IN_NEW_DFS_INFO *pDindiPrevCur = pDindi + i - 1;
                    dfuLong32 dfOldPreviousVersionNumber = pDindiPrevCur->dfNumDirVersionOldSvf;
                    dfuLong32 dfPreviousVersionItemInOldSvf = pFindiCur->dfPreviousVersionItemInOldSvf;
                    FILE_IN_OLD_DFS_INFO *pdiodi =
                        (((pDiodi + dfOldPreviousVersionNumber)->pfiodi) + dfPreviousVersionItemInOldSvf);
                    pdiodi->dfParameter |= FIODI_NEEDED_FOR_BLDNEWDFS_ORIGIN;
                }
            }
        }
    }

    for (i = dfNbDirDfsRead/*dfNbVersionRemix*/ - 1; i > 0; i--)
    {
        const DIR_IN_OLD_DFS_INFO *pDiodiCur;
        const DIR_IN_OLD_DFS_INFO *pDiodiPrevCur;

        /* i>0 and i<=dfNbVersionRemix */

        pDiodiCur = pDiodi + i;
        pDiodiPrevCur = pDiodi + i - 1;
        for (j = 0; j < pDiodiCur->dfNbFile; j++)
        {
            FILE_IN_OLD_DFS_INFO *pfiodicur = (pDiodiCur->pfiodi) + j;
            if (((pfiodicur->dfParameter & FIODI_CONTENT_NEEDED_MASK) != 0) &&
                (pfiodicur->dfConstructionInfo < FIODI_CONSTR_JOKER_BASE))
            {
                dfuLong32 dfConstructionInfo = pfiodicur->dfConstructionInfo;
                ((pDiodiPrevCur->pfiodi) + (dfConstructionInfo))->dfParameter |= FIODI_NEEDED_FOR_EXTRACTING;
            }
        }
    }

    return TRUE;
}



////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

typedef struct
{
    dfuLong32 dwMinProgress;
    dfuLong32 dwMaxProgress;
    tSetExtractPosCallBack pSetExtractPosCallBack;
    dfvoidp dfuPtrSetExtractPosCallBack;

    tConfirmBeforeCreatingFile pConfirmBeforeCreatingFile;
    dfvoidp dfUserPtrBeforeCreatingFile;
}
PROGRESSCBPARAMEP;


BOOL DFSCALLBACK ProgressCallBackDoExtracting(PROGRESSCALLBACKINFO * pProgressCallBackInfo);

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////


void CopyTagBuf(dfuLong32 i, dfuLong32 dfNbVersionWritten, DFSFILE DfsFileRead,
                       DFSFILE DfsFileWrite, H_ERROR_INFO * pei)
{
    dfvoidp TagBuf;
    dfuLong32 TagSize;
    DFTAGBLOCKFLOAT TagBlockFloat;
    TagBlockFloat = GetDfsTagBlockFloat(DfsFileRead, pei);
    if (TagBlockFloat != NULL)
    {
        if (GetTagBlockFloat(TagBlockFloat, i, FLOATNUM_NOSPECIFIC, DFSTAG_DIR_NAME_FLOAT, &TagBuf, &TagSize))
        {
            DFTAGBLOCKFLOAT TagBlockFloatWriting;
            TagBlockFloatWriting = GetDfsTagBlockFloat(DfsFileWrite, pei);
            if (TagBlockFloatWriting != NULL)
                AddTagBlockFloat(TagBlockFloatWriting, dfNbVersionWritten,
                                 FLOATNUM_NOSPECIFIC, DFSTAG_DIR_NAME_FLOAT, TagBuf, TagSize);
            SetDfsTagBlockFloatDirty(DfsFileWrite, pei);
        }

        if (GetTagBlockFloat
            (TagBlockFloat, i, FLOATNUM_NOSPECIFIC, DFSTAG_DIR_COMMENT_FLOAT, &TagBuf, &TagSize))
        {
            DFTAGBLOCKFLOAT TagBlockFloatWriting;
            TagBlockFloatWriting = GetDfsTagBlockFloat(DfsFileWrite, pei);
            if (TagBlockFloatWriting != NULL)
                AddTagBlockFloat(TagBlockFloatWriting, dfNbVersionWritten,
                                 FLOATNUM_NOSPECIFIC, DFSTAG_DIR_COMMENT_FLOAT, TagBuf, TagSize);
            SetDfsTagBlockFloatDirty(DfsFileWrite, pei);
        }
    }
}

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/

/* DoMultiExtracting is a source of idea */


typedef struct
{
    DFSFILE DfsFileRead;
    const DIR_IN_OLD_DFS_INFO *pDiodi_;
    FILEREFCOUNTER frc;
    dfuLong32 dfCurrentVersion;
    FILESET *pfsCurrentVersion;
    /////////////
    dfuLong32 dfBaseDirNum;
    BOOL fBaseDirectorySelected;
    dfwcharpc wchBaseDirectory;
    const FILESET* pFileSetBase;
    dfwcharpc wchBaseDirExtract;
} DFSOLDEXTR;


static BOOL ApplyExtractingMapDiodi(FILESET * pfs, const DIR_IN_OLD_DFS_INFO * pDiodi,
                                    BOOL fSecondPassForRamDifDual, dfuLong32* p_nb_file_to_work)
{
    BOOL fRet = FALSE;
    dfuLong32 i;
    dfuLong32 nb_file_to_work=0;
    if ((pDiodi != NULL) && (pfs != NULL))
        if (pDiodi->dfNbFile == pfs->dfNbFileItem)
        {
            fRet = TRUE;
            for (i = 0; i < pDiodi->dfNbFile; i++)
            {
                BOOL fContentNeeded,fRamDifNeeded;
                FILE_IN_OLD_DFS_INFO *pfiodiCur = (pDiodi->pfiodi + i);
                fContentNeeded = (pfiodiCur->dfParameter & FIODI_CONTENT_NEEDED_MASK) != 0;
                fRamDifNeeded = (pfiodiCur->dfParameter & FIODI_RAMDIF_COMBINING_NEEDED) != 0;

                // GV 15 may 2014 : very strange

                if (fContentNeeded && fRamDifNeeded)
                    puts("---- DUAL EXTRACTING NEEDED : Content and ramdif\n");

                /*
                if (((pfs->pFileItem) + i)->hRamDifOfPatch != NULL)
                    puts("---- hRamDifOfPatch preloaded for merge\n");
                    */
                //((pfs->pFileItem) + i)->fIgnore = !fNeeded;
                ((pfs->pFileItem) + i)->ExtAction = fContentNeeded ? ExtActionExtractContent : ExtActionIgnore;


                ((pfs->pFileItem) + i)->ExtAction = ExtActionIgnore;
                if (!fSecondPassForRamDifDual)
                {
                    if (fContentNeeded)
                        ((pfs->pFileItem) + i)->ExtAction = ExtActionExtractContent ;
                    else
                        if (fRamDifNeeded)
                            ((pfs->pFileItem) + i)->ExtAction = ExtActionMergeRamDif ;
                }

                if (fSecondPassForRamDifDual && fContentNeeded && fRamDifNeeded)
                    ((pfs->pFileItem) + i)->ExtAction = ExtActionMergeRamDif;

                if (((pfs->pFileItem) + i)->ExtAction != ExtActionIgnore)
                    nb_file_to_work++;
            }
        }
    if (p_nb_file_to_work != NULL)
        *p_nb_file_to_work = nb_file_to_work;
    return fRet;
}



static dfuLong32 InitOldDfsExtraction(
                             DFSOLDEXTR ** ppDfsoeResult,
                             DFSFILE DfsFileRead,
                             const DIR_IN_OLD_DFS_INFO * pDiodi,
                             FILEREFCOUNTER frc, BOOL fBaseDirectorySelected,
                             dfuLong32 dfBaseDirNum, dfwcharpc wchBaseDirectory,const FILESET* pFileSetBase,
                             dfwcharpc wchBaseDirExtractTemp)
{
    dfuLong32 dfError = DFS_SUCCESS;
    DFSOLDEXTR *pDfsoe;

    pDfsoe = (DFSOLDEXTR *) DfsMalloc(sizeof(DFSOLDEXTR));
    if (pDfsoe != NULL)
    {
        pDfsoe->DfsFileRead = DfsFileRead;
        pDfsoe->pDiodi_ = pDiodi;
        pDfsoe->frc = frc;

        pDfsoe->dfBaseDirNum = dfBaseDirNum;
        pDfsoe->fBaseDirectorySelected = fBaseDirectorySelected;
        pDfsoe->wchBaseDirectory = dfUnicodeCopyAlloc(wchBaseDirectory);
        pDfsoe->wchBaseDirExtract = dfUnicodeCopyAlloc(wchBaseDirExtractTemp);
        pDfsoe->pFileSetBase = pFileSetBase;


        pDfsoe->dfCurrentVersion = fBaseDirectorySelected ? dfBaseDirNum : 0;
        pDfsoe->pfsCurrentVersion = NULL;
    }

    *ppDfsoeResult = pDfsoe;
    return dfError;
}


static BOOL ClearFileSetCurrentVersion(DFSOLDEXTR * pDfsoe)
{
    dfuLong32 j;
    BOOL fRet = TRUE;
    if (pDfsoe->pfsCurrentVersion != NULL)
    {
        for (j = 0; j < pDfsoe->pfsCurrentVersion->dfNbFileItem; j++)
        {
            FILEITEM *pFileItemCur = (pDfsoe->pfsCurrentVersion->pFileItem) + j;
            if (pFileItemCur->fTempFile)
                if (!SubFileRefCounter(pDfsoe->frc, pFileItemCur->FileNameOnDisk, NULL, TRUE))
                    fRet = FALSE;
        }

        FreeFileSet(pDfsoe->pfsCurrentVersion, FALSE);
        DfsFree(pDfsoe->pfsCurrentVersion);
        pDfsoe->pfsCurrentVersion = NULL;
    }
    return fRet;
}




typedef struct
{
    dfuLong32 dwMinProgress;
    dfuLong32 dwMaxProgress;
    tSetExtractPosCallBack pSetExtractPosCallBack;
    dfvoidp dfuPtrSetExtractPosCallBack;

    tConfirmBeforeCreatingFile pConfirmBeforeCreatingFile;
    dfvoidp dfUserPtrBeforeCreatingFile;
} PROGRESSCBPARAMEREMIX;

static BOOL DFSCALLBACK ProgressCallBackDoRemixDfsWork(PROGRESSCALLBACKINFO * pProgressCallBackInfo)
{
    PROGRESSCBPARAMEREMIX* ppcp;
    dfuLong32 dwProgressWidth ;

    if ((pProgressCallBackInfo->dfEvent==DFCBM_BEFOREOPENWORKINGFILE) && (!pProgressCallBackInfo->fWillIgnoreFile))
    {
        dfuLong32 dfConfirmResult = CONFIRM_BEFORE_CREATING_FILE_OK;
        ppcp = (PROGRESSCBPARAMEREMIX*)(pProgressCallBackInfo->dfUserPtr);
        if ((ppcp->pConfirmBeforeCreatingFile != NULL) && (pProgressCallBackInfo->fTemporaryFilename!=TRUE))
            dfConfirmResult = ppcp->pConfirmBeforeCreatingFile(pProgressCallBackInfo->filename_onwork,NULL,
                                           ppcp->dfUserPtrBeforeCreatingFile);
        if (dfConfirmResult != CONFIRM_BEFORE_CREATING_FILE_OK)
            pProgressCallBackInfo->fWillIgnoreFile = TRUE;
        return (dfConfirmResult != CONFIRM_BEFORE_CREATING_FILE_STOP);
    }

    if ((pProgressCallBackInfo->dfEvent==DFCBM_PROGRESSWORKINGFILE) && (pProgressCallBackInfo->dfUserPtr != NULL))
    {
      ppcp = (PROGRESSCBPARAMEREMIX*)(pProgressCallBackInfo->dfUserPtr);
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





static dfuLong32 ExtractVersionInOldDfs(DFSOLDEXTR * pDfsoe, dfuLong32 dfVersion, const PCDIRINFO * pDirInfo,
                                        dfuLong32* pdfProgressStartcurrentStep, dfuLong32 dfProgressStep,
                                        tSetExtractPosCallBack pSetExtractPosCallBack,dfvoidp dfUserPtr,H_ERROR_INFO* pei)
{
    BOOL fContinue = TRUE;
    dfuLong32 dfError = DFS_SUCCESS;
    dfuLong32 i = pDfsoe->dfCurrentVersion;
    BOOL fFirstLoop = ((pDfsoe->pfsCurrentVersion == NULL) && (i == pDfsoe->dfBaseDirNum));

    // we must handle HRAMDIF here to fix BUGBUG
    while ((fContinue) && ((i < dfVersion) || fFirstLoop) && (dfError == DFS_SUCCESS))
    {
        FILESET *pfsNextVersion = NULL;
        dfuLong32 dfTypeDir = 0;
        BOOL fAllIsGood = FALSE;
        BOOL fRet = FALSE;
        BOOL fFirstBase = FALSE;
        PROGRESSCBPARAMEREMIX pcp;
        dfuLong32 nb_file_to_work=0;

        /*if ((pDfsoe->pfsCurrentVersion == NULL)
           && (i == pDfsoe->dfBaseDirNum)) */
        if (fFirstLoop)
            fFirstBase = (pDfsoe->fBaseDirectorySelected);
        else
            i++;

        fFirstLoop = FALSE;


        /* to do : callback stuff */

        pcp.dwMinProgress = (*pdfProgressStartcurrentStep);
        pcp.dwMaxProgress = (*pdfProgressStartcurrentStep)+dfProgressStep;

        pcp.pSetExtractPosCallBack = pSetExtractPosCallBack;
        pcp.dfuPtrSetExtractPosCallBack = dfUserPtr;

        pcp.pConfirmBeforeCreatingFile = NULL;
        pcp.dfUserPtrBeforeCreatingFile = NULL;


        pfsNextVersion = (FILESET *) DfsMalloc(sizeof(FILESET));
        InitFileSet(pfsNextVersion);
        dfError =
            CreateFileSetForVersionInDirectory(pDfsoe->DfsFileRead,
                                               pfsNextVersion, i,
                                               fFirstBase ? pDfsoe->wchBaseDirectory : pDfsoe->wchBaseDirExtract,
                                               fFirstBase ? pDfsoe->pFileSetBase : NULL,
                                               *(pDirInfo+i),
                                               &fAllIsGood,
                                               FALSE, FALSE /* fFillNameOnDisk */ ,
                                               fFirstBase, fFirstBase,
                                               &dfTypeDir, TRUE, TRUE /* fFlatExtracting */,
                                               pei);

        pfsNextVersion->fTempFile = TRUE;

        // we fill ExtAction as ExtActionExtractContent, ExtActionIgnore, or ExtActionMergeRamDif
        ApplyExtractingMapDiodi(pfsNextVersion,pDfsoe->pDiodi_ + i,FALSE,&nb_file_to_work);


        if ((dfError == DFS_SUCCESS) || (fAllIsGood))
        {
            dfuLong32 dfLoop=0;


            if (i==2)
            {

                i+=0;
            }


            // we can APPLYSTREAMPATCH or
            // we can do ExtActionExtractContent, ExtActionMergeRamDif
            for (;;)
            {
                if ((!fFirstBase) && (nb_file_to_work>0))
                  dfError =
                    ExtractPatch(pDfsoe->DfsFileRead, pDfsoe->pfsCurrentVersion,
                                 pfsNextVersion, i, *(pDirInfo+i),
                                 TRUE, TRUE /* fOldFileCanBeCaptured */ , FALSE,
                                 pDfsoe->wchBaseDirExtract, ProgressCallBackDoRemixDfsWork, &pcp,
                                 NULL, NULL, pei);
                if (dfLoop>0)
                    break;
                dfLoop=1;
                ApplyExtractingMapDiodi(pfsNextVersion,pDfsoe->pDiodi_ + i,TRUE,&nb_file_to_work);
            }

            (*pdfProgressStartcurrentStep) += dfProgressStep;

            /*ProgressCallBackDoExtracting, &pcp); */
            if (dfError == DFS_SUCCESS)
            {
                dfuLong32 j;

                for (j = 0; j < pfsNextVersion->dfNbFileItem; j++)
                {
                    FILEITEM *pFileItemCur = (pfsNextVersion->pFileItem) + j;
                    if (pFileItemCur->fTempFile)
                        if (!AddFileRefCounter(pDfsoe->frc, pFileItemCur->FileNameOnDisk, NULL))
                            fRet = FALSE;
                }

                ClearFileSetCurrentVersion(pDfsoe);
                pDfsoe->pfsCurrentVersion = pfsNextVersion;
                pDfsoe->dfCurrentVersion = i;
            }
        }
    }
    return dfError;
}



static BOOL FreeOldDfsExtraction(DFSOLDEXTR * pDfsoe)
{
    if (pDfsoe == NULL)
        return FALSE;

    ClearFileSetCurrentVersion(pDfsoe);

    if (pDfsoe->wchBaseDirectory != NULL)
        DfsFree((dfwcharp) pDfsoe->wchBaseDirectory);

    if (pDfsoe->wchBaseDirExtract != NULL)
        DfsFree((dfwcharp) pDfsoe->wchBaseDirExtract);

    DfsFree(pDfsoe);
    return TRUE;
}

/***************************************************************************/

#if defined(MSGOUTTEST)

BOOL DisplayDirInOldDfsInfo(const DIR_IN_OLD_DFS_INFO * pDiodi,
                            dfuLong32 dfNumDirOldDfs, const PCDIRINFO * pDirInfo);

BOOL DisplayDirInNewDfsInfo(const DIR_IN_NEW_DFS_INFO * pDindi,
                            const PCDIRINFO * pDirInfo,
                            dfuLong32 dfNbDirDfsRead,
                            const DIR_IN_OLD_DFS_INFO * pDiodi,
                            dfuLong32 dfNbVersionRemix, const VERSIONTOADD_REMIX * pVersionRemix);
#endif


/*+++++++++++++++++++++++++++*/




void ClearFileToAddArray(FILETOADD* pFileToAdd,dfuLong32 dfMaxNbFileToAdd)
{
    dfuLong32 j;
    for (j=0;j<dfMaxNbFileToAdd;j++)
    {
        FILETOADD* pFileToAddCur = pFileToAdd+j;

        if (pFileToAddCur->filename_ondisk!=NULL)
            DfsFree((dfwcharp)pFileToAddCur->filename_ondisk);
        if (pFileToAddCur->filename_prevversionondisk!=NULL)
            DfsFree((dfwcharp)pFileToAddCur->filename_prevversionondisk);
        if (pFileToAddCur->filename_tostore!=NULL)
            DfsFree((dfwcharp)pFileToAddCur->filename_tostore);

        DeleteRamDif(pFileToAddCur->hRamDifToFlushPatch); // TESTDBG++
        /*
        if (pFileToAddCur->hRamDifToFlushPatch!=NULL)
            {};
        */
    }
    DfsFree(pFileToAdd);
}


dfuLong32 SVFAPI DoReMixDfs(DFSFILE DfsFileRead, dfuLong32 dfNbDirDfsRead,
    const PCDIRINFO * pDirInfo, DFSFILE * pDfsFileWrite,
    dfwcharpc dfWritingDfsFileName,        // BOOL fZipFile,
    const DFSFEATUREPARAM* pFeatureParam,
    BOOL fBaseDirectorySelected, dfuLong32 dfBaseDirNum,
    dfwcharpc wchBaseDirectory,
    const FILESET* pFileSetBase,
    dfuLong32 dfNbVersionRemix,
    const VERSIONTOADD_REMIX * pVersionRemix,
    BOOL fFirstVersionAsReferenceNewDfs, BOOL fReuseOldPatch,
    const COMPRESSIONPARAM * pCprParam,
    tSetExtractPosCallBack pSetExtractPosCallBack,
    dfvoidp dfUserPtr, dfuLong32 dwMinProgress, dfuLong32 dwMaxProgress,
    H_ERROR_INFO* pei)
{
    BOOL fRawAccepted = fReuseOldPatch;
    return DoReMixDfsEx(DfsFileRead, dfNbDirDfsRead,
        pDirInfo, pDfsFileWrite,
        dfWritingDfsFileName,        // BOOL fZipFile,
        pFeatureParam,
        fBaseDirectorySelected, dfBaseDirNum,
        wchBaseDirectory,
        pFileSetBase,
        dfNbVersionRemix,
        pVersionRemix,
        fFirstVersionAsReferenceNewDfs, fReuseOldPatch, fRawAccepted,
        pCprParam,
        pSetExtractPosCallBack,
        dfUserPtr, dwMinProgress, dwMaxProgress,
        pei);
}

dfuLong32 SVFAPI DoReMixDfsEx(DFSFILE DfsFileRead, dfuLong32 dfNbDirDfsRead,
                const PCDIRINFO * pDirInfo, DFSFILE * pDfsFileWrite,
                dfwcharpc dfWritingDfsFileName,        // BOOL fZipFile,
                const DFSFEATUREPARAM* pFeatureParam,
                BOOL fBaseDirectorySelected, dfuLong32 dfBaseDirNum,
                dfwcharpc wchBaseDirectory,
                const FILESET* pFileSetBase,
                dfuLong32 dfNbVersionRemix,
                const VERSIONTOADD_REMIX * pVersionRemix,
                BOOL fFirstVersionAsReferenceNewDfs, BOOL fReuseOldPatch, BOOL fRawAccepted,
                const COMPRESSIONPARAM * pCprParam,
                tSetExtractPosCallBack pSetExtractPosCallBack,
                dfvoidp dfUserPtr, dfuLong32 dwMinProgress, dfuLong32 dwMaxProgress,
                H_ERROR_INFO* pei)
{
    DIR_IN_OLD_DFS_INFO *pDiodi = NULL;
    DIR_IN_NEW_DFS_INFO *pDindi = NULL;
    BOOL fBaseNeeded;
    DFSFEATUREPARAM DfsFeatureParamUse ;
    const DFSFEATUREPARAM* pFeatureParamUse=NULL;

    dfuLong32 dfNumDirOldDfs;
    //dfuLong32 dfNbVersionWritten = 0;
    dfuLong32 dfProgressStep = 0;
//    dfuLong32 dfLastDirExtr,dfDirStartExtr;
    DFSFILE DfsFileWrite = NULL;
    //FILESET **ppFileSetCollection=NULL;
    dfuLong32 i;
    BOOL *lpVersionMap = NULL;
    dfuLong32 dfError = DFS_SUCCESS;
    dfuLong32 dfProgressStartcurrentStep=dwMinProgress;
    //dfwcharpc wchBaseDirExtract = GetUnicodeStringEmpty();  // to modify
    /* Todo : First, checking consistency of parameter */




    if (fReuseOldPatch)
    {
        DFSFEATUREPARAM DfsFeatureParamAdapt;

        if (pFeatureParam !=NULL)
        {
            DfsFeatureParamUse = *pFeatureParam;
        }
        else
        {
            DfsFeatureParamUse.fComputeMd5 = TRUE;
            DfsFeatureParamUse.fComputeSha1 = FALSE;
            DfsFeatureParamUse.fComputeSha256 = FALSE;
            DfsFeatureParamUse.fStripIdenticalBody = TRUE;
        }

        DfsFeatureParamAdapt = DfsFeatureParamUse ;
        AdaptDfsFeature(pDirInfo,dfNbDirDfsRead,&DfsFeatureParamAdapt);
        DfsFeatureParamUse.fComputeMd5 = DfsFeatureParamAdapt.fComputeMd5;
        DfsFeatureParamUse.fComputeSha1 = DfsFeatureParamAdapt.fComputeSha1;
        DfsFeatureParamUse.fComputeSha256  = DfsFeatureParamAdapt.fComputeSha256;
        pFeatureParamUse = &DfsFeatureParamUse;
    }
    else
        pFeatureParamUse = pFeatureParam;


    if (pDfsFileWrite != NULL)
        *pDfsFileWrite = NULL;

    fBaseNeeded = ((*pDirInfo)->dfTypeDir == TYPEDIR_FILECRCONLY);
    dfNumDirOldDfs = dfNbDirDfsRead;

    if (dfNbVersionRemix==0)
        dfError = DFS_ERROR_BAD_PARAMETER;

    if ((fBaseDirectorySelected) && (dfNbVersionRemix>0))
    {
        dfuLong32 dfFirstVersionAsked = pVersionRemix->dfNumVersionPreviousSvf;
        if (dfFirstVersionAsked < dfBaseDirNum)
            dfError = DFS_ERROR_BAD_PARAMETER;
    }

    if (dfError == DFS_SUCCESS)
    {
        lpVersionMap = (BOOL*)DfsMalloc(sizeof(BOOL) * (dfNbDirDfsRead + 1));

        if (lpVersionMap == NULL)
            dfError = DFS_ERROR_MEMORY_ERROR;
    }


    if (dfError == DFS_SUCCESS)
    {
        dfuLong32 dfPrevVersionMarked=0;
        for (i = 0; i < dfNbDirDfsRead; i++)
            *(lpVersionMap + i) = FALSE;
        for (i = 0; i < dfNbVersionRemix; i++)
        {
            dfuLong32 dfVer = (pVersionRemix + i)->dfNumVersionPreviousSvf;
            if (i > 0)
                if ((dfVer <= dfPrevVersionMarked) || (dfVer >= dfNbDirDfsRead))
                {
                    dfError = DFS_ERROR_BAD_PARAMETER;
                    break;
                }
            *(lpVersionMap + i) = TRUE;
            dfPrevVersionMarked = dfVer;
        }

        if ((dfNbVersionRemix+dfPrevVersionMarked)>0)
          dfProgressStep = (dwMaxProgress-dwMinProgress)/(dfNbVersionRemix+dfPrevVersionMarked);
    }

    if (dfError == DFS_SUCCESS)
        pDiodi = BuildDirInOldArray(dfNbDirDfsRead, pDirInfo);

    if ((dfError == DFS_SUCCESS) && (pDiodi != NULL))
        pDindi = BuildDirInNewArray(dfNbDirDfsRead, pDiodi,
                                    dfNbVersionRemix, pVersionRemix,
                                    fFirstVersionAsReferenceNewDfs,
                                    fBaseDirectorySelected, dfBaseDirNum,
                                    fReuseOldPatch,fRawAccepted);

    if (((pDindi == NULL) || (pDiodi == NULL)) && (dfError == DFS_SUCCESS))
        dfError = DFS_ERROR_BAD_PARAMETER;

#if defined(MSGOUTTEST)
    //DisplayDirInOldDfsInfo(pDiodi, dfNbDirDfsRead, pDirInfo);
#endif
    if (dfError == DFS_SUCCESS)
        if (!BuildDependecyInOldDfs(dfNbDirDfsRead, pDiodi, dfNbVersionRemix, pDindi))
            dfError = DFS_ERROR_BAD_PARAMETER;

#if defined(MSGOUTTEST)
    if (dfError == DFS_SUCCESS)
    {
        DisplayDirInOldDfsInfo(pDiodi, dfNbDirDfsRead, pDirInfo);
        DisplayDirInNewDfsInfo(pDindi, pDirInfo, dfNbDirDfsRead, pDiodi, dfNbVersionRemix, pVersionRemix);
    }
#endif
    /* now the preparation work is done, we do the real work ! */
/*
    dfDirStartExtr = dfBaseDirNum;
    dfLastDirExtr = dfNbDirDfsRead-1; // can be better : do not extract last version if they are not used

    if ((dfNbVersionRemix + (1 + dfLastDirExtr - dfDirStartExtr)) > 0)
        dfProgressStep =
        (dwMaxProgress - dwMinProgress) / (dfNbVersionRemix +
                                            (1 + dfLastDirExtr -
                                            dfDirStartExtr));
*/

    /* opening DFSWrite */
    if (dfError == DFS_SUCCESS)
    {
        DFSFILEINFOPARAM DfsFileParam;

        DfsFileParam.sizeStruct = sizeof(DfsFileParam);
        DfsFileParam.dfStatus = DFS_NEWFILE | DFS_WRITABLE;

        DfsFileParam.filename = dfWritingDfsFileName;

        DfsFileOpen(&DfsFileParam, &DfsFileWrite, pei);
        if (pDfsFileWrite != NULL)
        {
            *pDfsFileWrite=DfsFileWrite;
        }

        if (DfsFileWrite!=NULL)
        {
            //SetDfsExtendedMode(DfsFileWrite,fStripIdenticalBody);
            if (pFeatureParamUse!=NULL)
                SetDfsFeatureParam(DfsFileWrite,pFeatureParamUse);
        }
        if (DfsFileWrite == NULL)
            dfError = DFS_ERROR_ERRORIO;
    }

    if (dfError == DFS_SUCCESS)
    {
        DFSOLDEXTR *pDfsoe = NULL;
        dfwcharpc wchBaseDirExtract = GetUnicodeStringEmpty();

        FILEREFCOUNTER frc = NULL;
        FILETOADD* pFileToAddPrevious = NULL;
        dfuLong32 dfMaxNbFileToAddPrevious = 0;
        frc = InitFileRefCounter();
        if (frc == NULL)
            dfError = DFS_ERROR_MEMORY_ERROR;
        else
            dfError = InitOldDfsExtraction(&pDfsoe,
                                           DfsFileRead, pDiodi,
                                           frc,
                                           fBaseDirectorySelected,
                                           dfBaseDirNum, wchBaseDirectory, pFileSetBase, wchBaseDirExtract);

        i = 0;
        while ((i < dfNbVersionRemix) && (dfError == DFS_SUCCESS) && (pDfsoe != NULL))
        {
            dfuLong32 dfNumVersionPreviousSvf_PreviousFile = 0;
            dfuLong32 dfNumVersionPreviousSvf_CurrentFile = 0;
            dfuLong32 j;
            dfuLong32 dfNbFileInVersionInserted = 0;
            dfuLong32 dfMaxNbFileToAdd;
            FILETOADD *pFileToAdd = NULL;
            BOOL *pArrayTempSituationAdding = NULL;
            BOOL *pArrayTempSituationPreviousFile = NULL;
            BOOL *pArrayTempSituationCurrentFile = NULL;
            const DIR_IN_NEW_DFS_INFO *pdindicur = pDindi + i;
            dfuLong32 iBrowsing;

            dfMaxNbFileToAdd = pdindicur->dfNbFile;

            pFileToAdd = (FILETOADD *) DfsMalloc(sizeof(FILETOADD) * (dfMaxNbFileToAdd + 1));
            for (j=0;j<dfMaxNbFileToAdd;j++)
            {
                (pFileToAdd + j)->filename_ondisk=NULL;
                (pFileToAdd + j)->filename_prevversionondisk=NULL;
                (pFileToAdd + j)->filename_tostore=NULL;
                (pFileToAdd + j)->hRamDifToFlushPatch = NULL;
            }

            if (pFileToAdd != NULL)
                pArrayTempSituationAdding = (BOOL *) DfsMalloc((sizeof(BOOL) * 2 * (dfMaxNbFileToAdd + 1)));

            if (pArrayTempSituationAdding == NULL)
                dfError = DFS_ERROR_MEMORY_ERROR;
            else
            {
                pArrayTempSituationPreviousFile = pArrayTempSituationAdding;
                pArrayTempSituationCurrentFile = pArrayTempSituationAdding + dfMaxNbFileToAdd;
            }

            dfNumVersionPreviousSvf_CurrentFile = (pVersionRemix + i)->dfNumVersionPreviousSvf;
            if ((i > 0) && (dfError == DFS_SUCCESS))
            {
                dfNumVersionPreviousSvf_PreviousFile = (pVersionRemix + i - 1)->dfNumVersionPreviousSvf;
                dfError = ExtractVersionInOldDfs(pDfsoe, dfNumVersionPreviousSvf_PreviousFile,
                               pDirInfo,
                               &dfProgressStartcurrentStep, dfProgressStep,
                               pSetExtractPosCallBack, dfUserPtr,pei);
                if (dfError == DFS_SUCCESS)
                {

                }
            }

            // now : we must doing the work that need old file in version to add

            for (j = 0; (j < (pdindicur->dfNbFile) && (dfError == DFS_SUCCESS)); j++)
            {
                const FILE_IN_NEW_DFS_INFO *pfindicur = (pdindicur->pfindi) + j;
                dfuLong32 dfBldParameterBuildMethod = (pfindicur->dfBldParameter) & FINDI_PARAM_MASK_BUILDMETH;
                *(pArrayTempSituationPreviousFile + j) = FALSE;

                /* value possible for dfBldParameterBuildMethod


                   #define FINDI_PARAM_REFERENCE       ((dfuLong32)0x00000004)

                   #define FINDI_PARAM_NEW             ((dfuLong32)0x00000008) // testmask
                   #define FINDI_PARAM_NEW_RAWCOPY     ((dfuLong32)0x00000018)
                   #define FINDI_PARAM_NEW_RECOMPRESS  ((dfuLong32)0x00000028)

                   #define FINDI_PARAM_PATCH           ((dfuLong32)0x00000001) // testmask
                   #define FINDI_PARAM_PATCH_RAWCOPY   ((dfuLong32)0x00000011)
                   #define FINDI_PARAM_PATCH_REBUILD   ((dfuLong32)0x00000021)

                   #define FINDI_PARAM_PATCH_COMBINE   ((dfuLong32)0x00000041)

                   #define FINDI_PARAM_PATCH_IDENTICAL ((dfuLong32)0x00000002)

                   // mask:
                   #define FINDI_PARAM_PATCH_MASK      ((dfuLong32)0x00000003)
                   #define FINDI_PARAM_MASK_RAWCOPY    ((dfuLong32)0x00000010)
                   #define FINDI_PARAM_MASK_REBUILD    ((dfuLong32)0x00000020)
                 */


                (pFileToAdd + j)->fIgnore = FALSE;
                (pFileToAdd + j)->fForceDate = FALSE;   //((pfsDest->pFileItem)+i)->fForceDate;
                (pFileToAdd + j)->hAddTags = NULL;   //((pfsDest->pFileItem)+i)->fAddNewTag;
                //(pFileToAdd+j)->dfsInfoDate = NULL;//((pfsDest->pFileItem)+i)->dfsInfoDate;
                (pFileToAdd + j)->pReserved = NULL;     //((pfsDest->pFileItem)+i)->pReserved;
                (pFileToAdd + j)->fWritingRaw = FALSE;

                (pFileToAdd + j)->hRamDifToFlushPatch = NULL;

                (pFileToAdd + j)->filename_tostore = NULL;      //((pfsDest->pFileItem)+i)->FileNameOnArchive;
                (pFileToAdd + j)->filename_ondisk = NULL;       //((pfsDest->pFileItem)+i)->FileNameOnDisk;
                (pFileToAdd + j)->fForceRecopyPrevious = FALSE;
                (pFileToAdd + j)->dfForceRecopyOrRawCopySize = 0;
                (pFileToAdd + j)->dfForceRecopyOrRawCopyCrc32 = 0;
                (pFileToAdd + j)->fForceRecopyOrRawCopyMd5Present = FALSE;
                (pFileToAdd + j)->fForceRecopyOrRawCopySha1Present = FALSE;
                (pFileToAdd + j)->fForceRecopyOrRawCopySha256Present = FALSE;

                (pFileToAdd + j)->dfFileStatusForRaw =
                    (dfBldParameterBuildMethod == FINDI_PARAM_NEW_RAWCOPY)
                    || (dfBldParameterBuildMethod == FINDI_PARAM_PATCH_RAWCOPY);

                *(pArrayTempSituationPreviousFile + j) = FALSE;
                (pFileToAdd + j)->filename_prevversionondisk = NULL;
                (pFileToAdd + j)->dfPreviousVersionFilePosition = VALUE_UNKNOWN;


                if ((dfBldParameterBuildMethod & FINDI_PARAM_PATCH_MASK) != 0)
                {
                    (pFileToAdd + j)->dfPreviousVersionFilePosition=
                        pfindicur->dfPreviousVersionItemInNewSvf;
                }

                // filename_prevversionondisk is only used with FINDI_PARAM_PATCH_REBUILD
                if (dfBldParameterBuildMethod == FINDI_PARAM_PATCH_REBUILD)
                {
                    const FILESET *pfsPreviousVersionOldSvf = pDfsoe->pfsCurrentVersion;
                    dfuLong32 dfPreviousVersionItemInOldSvf = pfindicur->dfPreviousVersionItemInOldSvf;


                    /// already filled // (pFileToAdd+j)->filename_prevversionondisk=NULL;

                    if (dfPreviousVersionItemInOldSvf == FINDI_PREVVERINNEWDFS_FILEADDED)
                    {
                        // the prevois version is a file added by user
                        (pFileToAdd + j)->filename_prevversionondisk =
                            dfUnicodeCopyAlloc(pfindicur->pftaPreviousItem->filename_ondisk);
                    }
                    else
                    {
                        (pFileToAdd + j)->filename_prevversionondisk =
                            dfUnicodeCopyAlloc(((pfsPreviousVersionOldSvf->
                                                 pFileItem) + dfPreviousVersionItemInOldSvf)->FileNameOnDisk);

                        if (((pfsPreviousVersionOldSvf->pFileItem) +
                             dfPreviousVersionItemInOldSvf)->fTempFile)
                        {
                            *(pArrayTempSituationPreviousFile + j) = TRUE;
                            AddFileRefCounter(pDfsoe->frc,
                                              (pFileToAdd + j)->filename_prevversionondisk, NULL);
                        }
                    }
                }


                // filename_prevversionondisk is only used with FINDI_PARAM_PATCH_REBUILD
                if (dfBldParameterBuildMethod == FINDI_PARAM_PATCH_COMBINE)
                {
                    ///const FILESET *pfsPreviousVersionOldSvf = pDfsoe->pfsCurrentVersion;
                    ///(pFileToAdd + j)->hRamDifToFlushPatch=???;
                }
                //pFileToAdd->filename_tostore =
                //FILETOCOPYINFO_REMIX* pFileCopyInfoCur = ((pVersionRemix+i)->pFileCopyInfo)+j;
                //if (pFileCopyInfoCur -> fIsReferenceInAddedFile)
                //{
                //}
            }

            for (j = 0; (j < (pdindicur->dfNbFile) && (dfError == DFS_SUCCESS)); j++)
            {
                (pFileToAdd + j)->filename_ondisk = NULL;
                *(pArrayTempSituationCurrentFile + j) = FALSE;
            }


            // now : we must do operation for FINDI_PARAM_*_RAWCOPY or FINDI_PARAM_PATCH_IDENTICAL



            for (j = 0; (j < (pdindicur->dfNbFile) && (dfError == DFS_SUCCESS)); j++)
            {
                // we build an extraction map for directory iBrowsing in old DFS
                const FILE_IN_NEW_DFS_INFO *pfindicur = (pdindicur->pfindi) + j;

                //if (((pfindicur->dfBldParameter) & FINDI_PARAM_PATCH_IDENTICAL) != 0)
                if (((pfindicur->dfBldParameter) & FINDI_PARAM_MASK_BUILDMETH) == FINDI_PARAM_PATCH_IDENTICAL)
                {
                    if (pfindicur->dfPreviousVersionItemInOldSvf == FINDI_PREVVERINNEWDFS_FILEADDED)
                    {
                        (pFileToAdd + j)->dfForceRecopyOrRawCopySize =
                            (pFileToAddPrevious + pfindicur->dfPreviousVersionItemInNewSvf)->dfForceRecopyOrRawCopySize;
                        (pFileToAdd + j)->dfForceRecopyOrRawCopyCrc32 =
                            (pFileToAddPrevious + pfindicur->dfPreviousVersionItemInNewSvf)->dfForceRecopyOrRawCopyCrc32;
                        (pFileToAdd + j)->dfsInfoDate =
                            (pFileToAddPrevious + pfindicur->dfPreviousVersionItemInNewSvf)->dfsInfoDate;

                        (pFileToAdd + j)->fForceRecopyOrRawCopyMd5Present =
                            (pFileToAddPrevious + pfindicur->dfPreviousVersionItemInNewSvf)->fForceRecopyOrRawCopyMd5Present;
                        if ((pFileToAdd + j)->fForceRecopyOrRawCopyMd5Present)
                            DfsMemcpy((pFileToAdd + j)->bMd5,(pFileToAddPrevious + pfindicur->dfPreviousVersionItemInNewSvf)->bMd5,16);

                        (pFileToAdd + j)->fForceRecopyOrRawCopySha1Present =
                            (pFileToAddPrevious + pfindicur->dfPreviousVersionItemInNewSvf)->fForceRecopyOrRawCopySha1Present;
                        if ((pFileToAdd + j)->fForceRecopyOrRawCopySha1Present)
                            DfsMemcpy((pFileToAdd + j)->bSha1,(pFileToAddPrevious + pfindicur->dfPreviousVersionItemInNewSvf)->bSha1,20);

                        (pFileToAdd + j)->fForceRecopyOrRawCopySha256Present =
                            (pFileToAddPrevious + pfindicur->dfPreviousVersionItemInNewSvf)->fForceRecopyOrRawCopySha256Present;
                        if ((pFileToAdd + j)->fForceRecopyOrRawCopySha256Present)
                            DfsMemcpy((pFileToAdd + j)->bSha256,(pFileToAddPrevious + pfindicur->dfPreviousVersionItemInNewSvf)->bSha256,32);
                    }
                    else
                    {
                        const FILEINDIRINFO* pFileInDirInfo = ((*(pDirInfo + (dfNumVersionPreviousSvf_PreviousFile)))->pFileInDirInfo) +
                                                 (pfindicur->dfPreviousVersionItemInOldSvf);

                        (pFileToAdd + j)->dfForceRecopyOrRawCopySize = pFileInDirInfo ->dfSize+0;
                        (pFileToAdd + j)->dfForceRecopyOrRawCopyCrc32 = pFileInDirInfo->dfCrc32;
                        (pFileToAdd + j)->fForceRecopyOrRawCopyMd5Present = pFileInDirInfo ->fMd5Filled ;
                        if ((pFileToAdd + j)->fForceRecopyOrRawCopyMd5Present)
                            DfsMemcpy((pFileToAdd + j)->bMd5,pFileInDirInfo ->bMd5,16);

                        (pFileToAdd + j)->fForceRecopyOrRawCopySha1Present = pFileInDirInfo ->fSha1Filled ;
                        if ((pFileToAdd + j)->fForceRecopyOrRawCopySha1Present)
                            DfsMemcpy((pFileToAdd + j)->bSha1,pFileInDirInfo ->bSha1,20);

                        (pFileToAdd + j)->fForceRecopyOrRawCopySha256Present = pFileInDirInfo ->fSha256Filled ;
                        if ((pFileToAdd + j)->fForceRecopyOrRawCopySha256Present)
                            DfsMemcpy((pFileToAdd + j)->bSha256,pFileInDirInfo ->bSha256,32);

#if defined(_DEBUG) && defined(MSGOUTTEST) && defined(TCHAR)
                        else
                        {
                            OutputDebugString("bif");
                        }
#endif
                    }

                    // We must do the identical stuff
                    (pFileToAdd + j)->fForceRecopyPrevious = TRUE;
                }
            }

            // RAWCOPY STUFF (must add ramdif here)
            for (iBrowsing = dfNumVersionPreviousSvf_PreviousFile;
                 iBrowsing <= dfNumVersionPreviousSvf_CurrentFile; iBrowsing++)
            {
                FILETOEXTRACT *pFileToExtract;
                dfuLong32 dfNbFileInVersionInOldDfs = (pDiodi + iBrowsing)->dfNbFile;
                pFileToExtract = (FILETOEXTRACT *)DfsMalloc(sizeof(FILETOEXTRACT) * (dfNbFileInVersionInOldDfs + 1));
                for (j = 0; j < dfNbFileInVersionInOldDfs; j++)
                {
                    //(pFileToExtract+j)->dfsInfoDate=NULL;
                    (pFileToExtract + j)->fCorrectlyDone = TRUE;
                    (pFileToExtract + j)->fIgnore = TRUE;
                    (pFileToExtract + j)->filename_ondisk_previous_to_read = NULL;
                    (pFileToExtract + j)->filename_ondisk_to_write = NULL;
                    ///(pFileToExtract + j)->dfuSizeProjected = 0;
                    (pFileToExtract + j)->KindExtracting = KIND_EXTRACTING_EXTRACTRAWSTREAMPATCHFILE;
                    (pFileToExtract + j)->fSetNewDate = FALSE;
                    (pFileToExtract + j)->fTemporaryFile=TRUE;

                    (pFileToExtract + j)->hRamDifWork=NULL;
                    //(pFileToExtract + j)->hRamDifResult=NULL;
                }

                for (j = 0; (j < (pdindicur->dfNbFile) && (dfError == DFS_SUCCESS)); j++)
                {
                    // we build an extraction map for directory iBrowsing in old DFS
                    const FILE_IN_NEW_DFS_INFO *pfindicur = (pdindicur->pfindi) + j;

                    // todo : RamDif work

                    if ((((pfindicur->
                           dfBldParameter) & FINDI_PARAM_MASK_RAWCOPY) != 0)
                        && (pfindicur->dfOldDfsRawCopyDirLocation == iBrowsing))
                    {
                        dfwchar szTempFN[1024];
                        const FILEINDIRINFO* pFileInDirInfo ;
                        dfuLong32 dfPosInOldDfsIBrowsing;
                        dfPosInOldDfsIBrowsing = pfindicur->dfOldDfsRawCopyFileLocation;

                        pFileInDirInfo = ((*(pDirInfo + (iBrowsing)))->pFileInDirInfo) +
                                                 (dfPosInOldDfsIBrowsing);

                        // size for temp is pFileInDirInfo->dfSize
                        GetTemporaryFilename(GetUnicodeSVFPrefix(), szTempFN, (sizeof(szTempFN) / sizeof(dfwchar)) - 1,TRUE,
                            ((pFileToExtract+dfPosInOldDfsIBrowsing)->KindExtracting == KIND_EXTRACTING_EXTRACTRAWSTREAMPATCHFILE) ?
                                   pFileInDirInfo->dfFileEncodedSize : pFileInDirInfo->dfSize);


#if defined(_DEBUG) && defined(MSGOUTTEST) && defined(TCHAR)
                  lstrcatW(szTempFN,((pFileToExtract+dfPosInOldDfsIBrowsing)->KindExtracting ==
                                  KIND_EXTRACTING_EXTRACTRAWSTREAMPATCHFILE) ? L"_reMix_Raw" : L"_reMix_noraw");
                  if ((pFileToExtract+dfPosInOldDfsIBrowsing)->KindExtracting != KIND_EXTRACTING_EXTRACTRAWSTREAMPATCHFILE)
                  {
                      lstrcatW(szTempFN, L"_!");
                  }
#endif

                        (pFileToExtract + dfPosInOldDfsIBrowsing)->filename_ondisk_to_write = dfUnicodeCopyAlloc(szTempFN);
                        ///(pFileToExtract +dfPosInOldDfsIBrowsing)->dfuSizeProjected = fooo();

                        (pFileToExtract + dfPosInOldDfsIBrowsing)->fIgnore = FALSE;
                        // set TRUE in Extraction map
                    }


                    /* pehaps check identical */

                    /* we skip dfNumVersionPreviousSvf_PreviousFile because we don't care how build dfNumVersionPreviousSvf_PreviousFile
                       we cant only stage from dfNumVersionPreviousSvf_PreviousFile -> dfNumVersionPreviousSvf_CurrentFile */

                    /*
                    if ((iBrowsing > dfNumVersionPreviousSvf_PreviousFile) &&
                        ((((pfindicur->
                           dfBldParameter) & FINDI_PARAM_MASK_BUILDMETH) == FINDI_PARAM_PATCH_COMBINE)))
                    {
                        dfuLong32 dfPosInOldDfsIBrowsing;
                        dfPosInOldDfsIBrowsing = pfindicur->dfOldDfsRawCopyFileLocation;

                        (pFileToExtract + dfPosInOldDfsIBrowsing)->fIgnore = FALSE;
                        (pFileToExtract + j)->KindExtracting = KIND_EXTRACTING_COMBINERAMDIF_TORAMDIF;
                    }

                    */


                }


                for (j = 0; (j < (pdindicur->dfNbFile) && (dfError == DFS_SUCCESS)); j++)
                {
                    const FILE_IN_NEW_DFS_INFO *pfindicur = (pdindicur->pfindi) + j;
/*
                        if ((iBrowsing > dfNumVersionPreviousSvf_PreviousFile) &&
                            (((pfindicur->dfBldParameter) & FINDI_PARAM_MASK_BUILDMETH) == FINDI_PARAM_PATCH_COMBINE))
*/
                        if ((iBrowsing == dfNumVersionPreviousSvf_CurrentFile) &&
                            (((pfindicur->dfBldParameter) & FINDI_PARAM_MASK_BUILDMETH) == FINDI_PARAM_PATCH_COMBINE))
                        {
                            const FILEINDIRINFO* pFileInDirInfo;
                            //const PCDIRINFO * pCurDirInfo = (pDirInfo + (iBrowsing));
#ifdef _DEBUG
                            dfuLong32 dfNbFile = (*(pDirInfo + (iBrowsing)))->dfNbFile;

                            /* BOG ? */
                            if (pfindicur->dfOldDfsFileItem>=dfNbFile)
                                dfNbFile+=0;
#endif

                            pFileInDirInfo = ((*(pDirInfo + (iBrowsing)))->pFileInDirInfo) +
                                       (pfindicur->dfOldDfsFileItem);

                            (pFileToAdd + j)->dfForceRecopyOrRawCopySize = pFileInDirInfo ->dfSize+0;
                            (pFileToAdd + j)->dfForceRecopyOrRawCopyCrc32 = pFileInDirInfo->dfCrc32;
                            (pFileToAdd + j)->fForceRecopyOrRawCopyMd5Present = pFileInDirInfo ->fMd5Filled ;
                            if ((pFileToAdd + j)->fForceRecopyOrRawCopyMd5Present)
                                DfsMemcpy((pFileToAdd + j)->bMd5,pFileInDirInfo ->bMd5,16);

                            (pFileToAdd + j)->fForceRecopyOrRawCopySha1Present = pFileInDirInfo ->fSha1Filled ;
                            if ((pFileToAdd + j)->fForceRecopyOrRawCopySha1Present)
                                DfsMemcpy((pFileToAdd + j)->bSha1,pFileInDirInfo ->bSha1,20);

                            (pFileToAdd + j)->fForceRecopyOrRawCopySha256Present = pFileInDirInfo ->fSha256Filled ;
                            if ((pFileToAdd + j)->fForceRecopyOrRawCopySha256Present)
                                DfsMemcpy((pFileToAdd + j)->bSha256,pFileInDirInfo ->bSha256,32);

                            (pFileToAdd + j)->fForceRecopyPrevious = FALSE;
                        }
                }


                //extract raw patch with the extraction map with ExtractDirectory (ExtractPatch show usage)
                if (dfError == DFS_SUCCESS)
                    dfError = ExtractDirectory(DfsFileRead, iBrowsing,*(pDirInfo+iBrowsing),
                                               (pDiodi + iBrowsing)->dfNbFile, pFileToExtract,
                                               NULL, NULL, NULL, NULL,
                                               FALSE, pei);


#if defined(_DEBUG) && defined(MSGOUTTEST) && defined(TCHAR)
            for (j = 0; j < dfNbFileInVersionInOldDfs; j++)
                if ((((pFileToExtract + j)->KindExtracting != KIND_EXTRACTING_EXTRACTRAWSTREAMPATCHFILE)) &&  (((pFileToExtract + j)->fIgnore)==FALSE))
                {
                    char sz[800];
                    wsprintf(sz,"not raw = %ws \n",(pFileToExtract + j)->filename_ondisk_to_write);
                    OutputDebugString(sz);
                }
#endif

                // we must check error

                for (j = 0; (j < (pdindicur->dfNbFile) && (dfError == DFS_SUCCESS)); j++)
                {
                    const FILE_IN_NEW_DFS_INFO *pfindicur = (pdindicur->pfindi) + j;

                    if ((((pfindicur->dfBldParameter) & (FINDI_PARAM_MASK_RAWCOPY|FINDI_PARAM_REFERENCE)) != 0) &&
                        (pfindicur->dfOldDfsRawCopyDirLocation == iBrowsing))
                    {
                        const FILEINDIRINFO* pFileInDirInfo = ((*(pDirInfo + (pfindicur->dfOldDfsRawCopyDirLocation)))->pFileInDirInfo) +
                                                 (pfindicur->dfOldDfsRawCopyFileLocation);


                        (pFileToAdd + j)->fWritingRaw = TRUE;

                        (pFileToAdd + j)->dfForceRecopyOrRawCopySize = pFileInDirInfo ->dfSize+0;
                        (pFileToAdd + j)->dfForceRecopyOrRawCopyCrc32 = pFileInDirInfo ->dfCrc32;

                        (pFileToAdd + j)->fForceRecopyOrRawCopyMd5Present = pFileInDirInfo ->fMd5Filled ;
                        if ((pFileToAdd + j)->fForceRecopyOrRawCopyMd5Present)
                        {
                            DfsMemcpy((pFileToAdd + j)->bMd5,pFileInDirInfo ->bMd5,16);
                        }

                        (pFileToAdd + j)->fForceRecopyOrRawCopySha1Present = pFileInDirInfo ->fSha1Filled ;
                        if ((pFileToAdd + j)->fForceRecopyOrRawCopySha1Present)
                        {
                            DfsMemcpy((pFileToAdd + j)->bSha1,pFileInDirInfo ->bSha1,20);
                        }

                        (pFileToAdd + j)->fForceRecopyOrRawCopySha256Present = pFileInDirInfo ->fSha256Filled ;
                        if ((pFileToAdd + j)->fForceRecopyOrRawCopySha256Present)
                        {
                            DfsMemcpy((pFileToAdd + j)->bSha256,pFileInDirInfo ->bSha256,32);
                        }

                        if (((pfindicur->dfBldParameter) & FINDI_PARAM_REFERENCE) != 0)
                        {
                            (pFileToAdd + j)->dfFileStatusForRaw=DFS_STORAGESTATUS_REFERENCE;
                        }

                        if (((pfindicur->dfBldParameter) & FINDI_PARAM_MASK_RAWCOPY) != 0)
                        {
                            const FILE_IN_OLD_DFS_INFO* pfiodirawcur=((pDiodi+(pfindicur->dfOldDfsRawCopyDirLocation))->pfiodi)+(pfindicur->dfOldDfsRawCopyFileLocation);
                            dfuLong32 dfPosInOldDfsIBrowsing;
                            dfPosInOldDfsIBrowsing = pfindicur->dfOldDfsRawCopyFileLocation;
                            // We are "stolen" the allocated temp filename with raw extracted content
                            (pFileToAdd + j)->filename_ondisk = (
                                (pFileToExtract + dfPosInOldDfsIBrowsing)->filename_ondisk_to_write);
                            (pFileToExtract + dfPosInOldDfsIBrowsing)->filename_ondisk_to_write = NULL;
                            ///(pFileToExtract + dfPosInOldDfsIBrowsing)->dfuSizeProjected = 0;
                            (pFileToAdd + j)->dfFileStatusForRaw = ((pfiodirawcur->dfParameter & FIODI_PARAM_PATCH_MASK)==0) ?
                                                                        DFS_STORAGESTATUS_NEW : DFS_STORAGESTATUS_MODIFIED;

#if defined(_DEBUG) && defined(MSGOUTTEST)
                            printf("---> crc = %08x, size=%u, dfConstrinfo=%08x for file %ws\n",
                                    pFileInDirInfo ->dfCrc32,
                                    (dfuLong32)pFileInDirInfo ->dfSize,
                                    pfiodirawcur->dfConstructionInfo,pFileInDirInfo->FileName);
                            printf("        stored as %ws\n",(pFileToAdd + j)->filename_ondisk);
#endif

                            AddFileRefCounter(pDfsoe->frc, (pFileToAdd + j)->filename_ondisk, NULL);
                        }
                    }
/*
                    if (((pfindicur->dfBldParameter) & FINDI_PARAM_PATCH_COMBINE) != 0)
                    {
                            dfuLong32 dfPosInOldDfsIBrowsing;
                            dfPosInOldDfsIBrowsing = pfindicur->dfOldDfsRawCopyFileLocation; // fully bad BUGBUGBUG BUG !!

                            if ((pFileToAdd + j)->hRamDifToFlushPatch != NULL)
                            {
                                DeleteRamDif ((pFileToAdd + j)->hRamDifToFlushPatch);
                                // todo : free it
                            }
                            (pFileToAdd + j)->hRamDifToFlushPatch = (pFileToExtract + dfPosInOldDfsIBrowsing)->hRamDifResult;
                    }
*/

                }


                for (j = 0; j < dfNbFileInVersionInOldDfs; j++)
                {
                    if ((pFileToExtract + j)->filename_ondisk_previous_to_read != NULL)
                        DfsFree((dfwcharp)(pFileToExtract + j)->filename_ondisk_previous_to_read);
                    if ((pFileToExtract + j)->filename_ondisk_to_write != NULL)
                        DfsFree((dfwcharp)(pFileToExtract + j)->filename_ondisk_to_write);
                }

                DfsFree(pFileToExtract);
            }


            // with this ExtractVersionInOldDfs, we get new file in version to add
            // we extract content and process ramdif
            dfError = ExtractVersionInOldDfs(pDfsoe, dfNumVersionPreviousSvf_CurrentFile,
                               pDirInfo,
                               &dfProgressStartcurrentStep, dfProgressStep,
                               pSetExtractPosCallBack, dfUserPtr, pei);
            if (dfError == DFS_SUCCESS)
            {
                //const FILESET* pFileSetVersionInOldDfs = pDfsoe->pfsCurrentVersion;
            }

            // now : we must doing the work that need new file in version to add
            for (j = 0; (j < (pdindicur->dfNbFile) && (dfError == DFS_SUCCESS)); j++)
            {
                const FILE_IN_NEW_DFS_INFO *pfindicur = (pdindicur->pfindi) + j;
                dfuLong32 dfBldParameterBuildMethod = (pfindicur->dfBldParameter) & FINDI_PARAM_MASK_BUILDMETH;

                /* value possible for dfBldParameterBuildMethod


                   #define FINDI_PARAM_REFERENCE       ((dfuLong32)0x00000004)

                   #define FINDI_PARAM_NEW             ((dfuLong32)0x00000008) // testmask
                   #define FINDI_PARAM_NEW_RAWCOPY     ((dfuLong32)0x00000018)
                   #define FINDI_PARAM_NEW_RECOMPRESS  ((dfuLong32)0x00000028)

                   #define FINDI_PARAM_PATCH           ((dfuLong32)0x00000001) // testmask
                   #define FINDI_PARAM_PATCH_RAWCOPY   ((dfuLong32)0x00000011)
                   #define FINDI_PARAM_PATCH_REBUILD   ((dfuLong32)0x00000021)

                   #define FINDI_PARAM_PATCH_IDENTICAL ((dfuLong32)0x00000002)

                   // mask:
                   #define FINDI_PARAM_PATCH_MASK      ((dfuLong32)0x00000003)
                   #define FINDI_PARAM_MASK_RAWCOPY    ((dfuLong32)0x00000010)
                   #define FINDI_PARAM_MASK_REBUILD    ((dfuLong32)0x00000020)
                 */


                if (pfindicur->dfOldDfsFileItem == FINDI_PREVDFS_FILEADDED)
                {
                    (pFileToAdd + j)->filename_tostore =
                        dfUnicodeCopyAlloc(pfindicur->pfta->filename_tostore);
                }
                else
                {
                    const FILESET *pfsCurrentVersionOldSvf = pDfsoe->pfsCurrentVersion;

                    /**** BUG: pfsCurrentVersionOldSvf IS NULL !!!! !-( :-( */
                    dfuLong32 dfOldDfsFileItem = pfindicur->dfOldDfsFileItem;

                    (pFileToAdd + j)->filename_tostore =
                        dfUnicodeCopyAlloc(((pfsCurrentVersionOldSvf->
                                             pFileItem) + dfOldDfsFileItem)->FileNameOnArchive);

                    (pFileToAdd + j)->fForceDate = TRUE;
                    (pFileToAdd + j)->dfsInfoDate =
                        ((pfsCurrentVersionOldSvf->pFileItem) + dfOldDfsFileItem)->dfsInfoDate;
                }


                if (dfBldParameterBuildMethod == FINDI_PARAM_PATCH_COMBINE)
                {
                    const FILESET *pfsCurrentVersionOldSvf = pDfsoe->pfsCurrentVersion;
                    dfuLong32 dfOldDfsFileItem = pfindicur->dfOldDfsFileItem;
                    (pFileToAdd + j)->hRamDifToFlushPatch = ((pfsCurrentVersionOldSvf->
                                                 pFileItem) + dfOldDfsFileItem)->hRamDifOfPatch;
                    //IncrementUsageRamDif((pFileToAdd + j)->hRamDifToFlushPatch); // ++ TESTDBG

                    ((pfsCurrentVersionOldSvf->pFileItem) + dfOldDfsFileItem)->hRamDifOfPatch = NULL;
                }

                if ((dfBldParameterBuildMethod == FINDI_PARAM_PATCH_REBUILD)
                    || (dfBldParameterBuildMethod == FINDI_PARAM_NEW_RECOMPRESS))
                {
                    const FILESET *pfsCurrentVersionOldSvf = pDfsoe->pfsCurrentVersion;
                    dfuLong32 dfOldDfsFileItem = pfindicur->dfOldDfsFileItem;
                    if (dfOldDfsFileItem == FINDI_PREVDFS_FILEADDED)
                    {
                        (pFileToAdd + j)->filename_ondisk =
                            dfUnicodeCopyAlloc(pfindicur->pfta->filename_ondisk);
                    }
                    else
                    {
                        (pFileToAdd + j)->filename_ondisk =
                            dfUnicodeCopyAlloc(((pfsCurrentVersionOldSvf->
                                                 pFileItem) + dfOldDfsFileItem)->FileNameOnDisk);

                        if (((pfsCurrentVersionOldSvf->pFileItem) + dfOldDfsFileItem)->fTempFile)
                        {
                            *(pArrayTempSituationCurrentFile + j) = TRUE;
                            AddFileRefCounter(pDfsoe->frc, (pFileToAdd + j)->filename_ondisk, NULL);
                        }
                    }
                }

                //pFileToAdd->filename_tostore =
                //FILETOCOPYINFO_REMIX* pFileCopyInfoCur = ((pVersionRemix+i)->pFileCopyInfo)+j;
                //if (pFileCopyInfoCur -> fIsReferenceInAddedFile)
                //{
                //}
            }



            // now : we must doing the work with InsertDirectoryinDfsFile

            /*
               dfuLong32 InsertDirectoryinDfsFile(DFSFILE DfsFile, dfuLong32 dfTypeDir,
               dfuLong32 dfNbFile, FILETOADD * pFileToAdd,
               dfwcharpc dfwVersionName,
               dfwcharpc dfwVersionComment,
               const COMPRESSIONPARAM* pCprParam,
               tProgressCallBack pProgressCallBack,
               dfvoidp dfUserPtr) */
            if (dfError == DFS_SUCCESS)
            {
                PROGRESSCBPARAMEREMIX pcp;
                /* to do : callback stuff */

                pcp.dwMinProgress = dfProgressStartcurrentStep;
                pcp.dwMaxProgress = dfProgressStartcurrentStep+dfProgressStep;

                pcp.pSetExtractPosCallBack = pSetExtractPosCallBack;
                pcp.dfuPtrSetExtractPosCallBack = dfUserPtr;

                pcp.pConfirmBeforeCreatingFile = NULL;
                pcp.dfUserPtrBeforeCreatingFile = NULL;


                dfNbFileInVersionInserted = pdindicur->dfNbFile;
                dfError =
                    InsertDirectoryinDfsFile(DfsFileWrite,
                                             (i != 0) ? TYPEDIR_PATCHFROMPREVIOUS :
                                             (fFirstVersionAsReferenceNewDfs ?
                                              TYPEDIR_FILECRCONLY :
                                              TYPEDIR_FILEINSERTING_DEFLATE),
                                             dfNbFileInVersionInserted, pFileToAdd,TRUE,
                                             NULL, NULL, pCprParam, ProgressCallBackDoRemixDfsWork, &pcp, pei);

                dfProgressStartcurrentStep+=dfProgressStep;

                if (dfError == DFS_SUCCESS)
                    DuplicateDirectoryFloatBlock(DfsFileRead,DfsFileWrite,dfNumVersionPreviousSvf_CurrentFile,i,pei);
            }


            if (pFileToAdd != NULL)
            {
                for (j = 0; j < dfNbFileInVersionInserted; j++)
                {
                    if ((pFileToAdd + j)->hRamDifToFlushPatch != NULL)
                    {
                        DeleteRamDif((pFileToAdd + j)->hRamDifToFlushPatch);
                        (pFileToAdd + j)->hRamDifToFlushPatch=NULL;
                    }

                    if (*(pArrayTempSituationPreviousFile + j))
                        if (!SubFileRefCounter
                            (pDfsoe->frc, (pFileToAdd + j)->filename_prevversionondisk, NULL, TRUE))
                            dfError = DFS_ERROR_MEMORY_ERROR;

                    if (*(pArrayTempSituationCurrentFile + j))
                        if (!SubFileRefCounter(pDfsoe->frc, (pFileToAdd + j)->filename_ondisk, NULL, TRUE))
                            dfError = DFS_ERROR_MEMORY_ERROR;
                }



                if (pFileToAddPrevious !=NULL)
                {
                    ClearFileToAddArray(pFileToAddPrevious,dfMaxNbFileToAddPrevious);
                    pFileToAddPrevious=NULL;
                }
                pFileToAddPrevious=pFileToAdd;
                dfMaxNbFileToAddPrevious=dfMaxNbFileToAdd;
            }

            if (pArrayTempSituationAdding!=NULL)
                DfsFree(pArrayTempSituationAdding);

            i++;
        }

        if (pFileToAddPrevious!=NULL)
        {
            ClearFileToAddArray(pFileToAddPrevious,dfMaxNbFileToAddPrevious);
            pFileToAddPrevious=NULL;
        }

        if (pDfsoe != NULL)
            ClearFileSetCurrentVersion(pDfsoe);

        #if defined(MSGOUTTEST) && defined(_DEBUG)
        printf("do DeleteFileRefCounter\n");
        #endif

        if (frc != NULL)
            DeleteFileRefCounter(frc, TRUE);
        if (pDfsoe != NULL)
        {
            FreeOldDfsExtraction(pDfsoe);
        }
    }


    if ((pDfsFileWrite == NULL) && (DfsFileWrite != NULL))
        DfsClose(DfsFileWrite, pei);

    FreeDirInNewArray(pDindi);
    FreeDirInOldArray(pDiodi);
    if (lpVersionMap != NULL)
        DfsFree(lpVersionMap);

    return dfError ;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//BOOL fReuseOldPatch, BOOL fRawAccepted,



BOOL SVFAPI DoGenerateSubDfsEmul(DFSFILE DfsFileRead,  /*dfuLong32 dfNbDir, */
    PDIRINFO * pDirInfo, DFSFILE * pDfsFileWrite, dfwcharpc dfWritingDfsFileName, /*BOOL fZipFile, */
    const DFSFEATUREPARAM* pFeatureParam,
    BOOL fBaseDirectorySelected, dfuLong32 dfBaseDirNum,
    dfwcharpc wchBaseDirectory,
    const FILESET* pFileSetBase,
    dfuLong32 dwNbMapVersionMap,
    const BOOL * lpVersionMap,
    BOOL fFirstVersionAsReference, BOOL fReuseOldPatch /* future */,
    const COMPRESSIONPARAM * pCprParam,
    tSetExtractPosCallBack pSetExtractPosCallBack,
    dfvoidp dfUserPtr, dfuLong32 dwMinProgress, dfuLong32 dwMaxProgress, H_ERROR_INFO* pei)
{
    BOOL fRawAccepted = fReuseOldPatch;
    return DoGenerateSubDfsEmulEx(DfsFileRead,  /*dfuLong32 dfNbDir, */
        pDirInfo, pDfsFileWrite, dfWritingDfsFileName, /*BOOL fZipFile, */
        pFeatureParam,
        fBaseDirectorySelected, dfBaseDirNum,
        wchBaseDirectory,
        pFileSetBase,
        dwNbMapVersionMap,
        lpVersionMap,
        fFirstVersionAsReference, fReuseOldPatch, fRawAccepted,
        pCprParam,
        pSetExtractPosCallBack,
        dfUserPtr, dwMinProgress, dwMaxProgress, pei);
}

BOOL SVFAPI DoGenerateSubDfsEmulEx(DFSFILE DfsFileRead,  /*dfuLong32 dfNbDir, */
                          PDIRINFO * pDirInfo, DFSFILE * pDfsFileWrite, dfwcharpc dfWritingDfsFileName, /*BOOL fZipFile, */
                          const DFSFEATUREPARAM* pFeatureParam,
                          BOOL fBaseDirectorySelected, dfuLong32 dfBaseDirNum,
                          dfwcharpc wchBaseDirectory,
                          const FILESET* pFileSetBase,
                          dfuLong32 dwNbMapVersionMap,
                          const BOOL * lpVersionMap,
                          BOOL fFirstVersionAsReference, BOOL fReuseOldPatch, BOOL fRawAccepted,
                          const COMPRESSIONPARAM * pCprParam,
                          tSetExtractPosCallBack pSetExtractPosCallBack,
                          dfvoidp dfUserPtr, dfuLong32 dwMinProgress, dfuLong32 dwMaxProgress,H_ERROR_INFO* pei)
{
    dfuLong32 dfNbVersionRemix;
    VERSIONTOADD_REMIX *pVersionRemix;
    dfuLong32 dfNbDirDfsRead = dwNbMapVersionMap;
    dfuLong32 i, j,dfError;

    pVersionRemix = (VERSIONTOADD_REMIX*)DfsMalloc(sizeof(VERSIONTOADD_REMIX) * (dwNbMapVersionMap + 1));
    dfNbVersionRemix = 0;
    for (i = 0; i < dwNbMapVersionMap; i++)
        if (*(lpVersionMap + i))
        {
            VERSIONTOADD_REMIX *pFileCopyInfoCur = pVersionRemix + dfNbVersionRemix;
            PDIRINFO pDirOrg = *(pDirInfo + i);
            pFileCopyInfoCur->dfNumVersionPreviousSvf = i;
            pFileCopyInfoCur->dfNbPreviousFileInMask = pDirOrg->dfNbFile;
            pFileCopyInfoCur->dfNbFileToAdd = 0;
            pFileCopyInfoCur->pFileCopyInfo = (FILETOCOPYINFO_REMIX*)
                DfsMalloc(sizeof(FILETOCOPYINFO_REMIX) * (pDirOrg->dfNbFile + 1));
            pFileCopyInfoCur->pfta = NULL;
            for (j = 0; j < pDirOrg->dfNbFile; j++)
            {
                FILETOCOPYINFO_REMIX *pftci = (pFileCopyInfoCur->pFileCopyInfo) + j;
                pftci->dfReferenceItem = FTCI_REFERENCE_UNMODIFIED;
                pftci->fIsReferenceInAddedFile = FALSE;
            }
            dfNbVersionRemix++;
        }

    dfError = DoReMixDfsEx(DfsFileRead, dfNbDirDfsRead, (const PCDIRINFO*)pDirInfo, pDfsFileWrite,
                            dfWritingDfsFileName,       // BOOL fZipFile,
                            pFeatureParam,
                            fBaseDirectorySelected, dfBaseDirNum, wchBaseDirectory,pFileSetBase,
                            dfNbVersionRemix, pVersionRemix,
                            fFirstVersionAsReference, fReuseOldPatch, fRawAccepted,       // future
                            pCprParam, pSetExtractPosCallBack, dfUserPtr, dwMinProgress, dwMaxProgress, pei);

    for (i = 0; i < dfNbVersionRemix; i++)
    {
        FILETOCOPYINFO_REMIX *pFileCopyInfo = (pVersionRemix + i)->pFileCopyInfo;
        DfsFree(pFileCopyInfo);
    }
    DfsFree(pVersionRemix);
    return dfError == DFS_SUCCESS;
}


#if defined(MSGOUTTEST)

BOOL DisplayDirInOldDfsInfo(const DIR_IN_OLD_DFS_INFO * pDiodi,
                            dfuLong32 dfNumDirOldDfs, const PCDIRINFO * pDirInfo)
{
    dfuLong32 i, j;

    printf("\n\n" "DisplayDirInOldDfsInfo report !\n");
    printf("number of version : %u \n", dfNumDirOldDfs);
    for (i = 0; i < dfNumDirOldDfs; i++)
    {
        const DIR_IN_OLD_DFS_INFO *pDiodiCur = pDiodi + i;
        printf("\n\nVersion %u,number of file: %u\n", i, pDiodiCur->dfNbFile);
        for (j = 0; j < pDiodiCur->dfNbFile; j++)
        {
            const FILE_IN_OLD_DFS_INFO *pFiodiCur = (pDiodiCur->pfiodi) + j;
            const FILEINDIRINFO *pCurFileInDirInfo = (((*(pDirInfo + i))->pFileInDirInfo) + j);
            dfvoidp TagBuf;
            dfuLong32 TagSize;


            if (GetTag(*((*(pDirInfo + i))->TagFile + j), DFSTAG_FILENAME, &TagBuf, &TagSize))
            {
                printf("Filename : %ws\n", TagBuf);
            }

            printf
                (" dfConstrucInfo=0x%08x, dfParameter=0x%08x, dfStorageStatus=0x%08x\n",
                 pFiodiCur->dfConstructionInfo, pFiodiCur->dfParameter, pFiodiCur->dfStorageStatus);
        }
    }

    return TRUE;
}


const char* GetBldParamString(dfuLong32 dfBldParam)
{
    dfuLong32 dfBldParamMeth = dfBldParam & FINDI_PARAM_MASK_BUILDMETH;
    switch (dfBldParamMeth)
    {
        case FINDI_PARAM_REFERENCE : return "FINDI_PARAM_REFERENCE";
        case FINDI_PARAM_NEW : return "FINDI_PARAM_NEW";
        case FINDI_PARAM_NEW_RAWCOPY : return "FINDI_PARAM_NEW_RAWCOPY";
        case FINDI_PARAM_NEW_RECOMPRESS  : return "FINDI_PARAM_NEW_RECOMPRESS";

        case FINDI_PARAM_PATCH   : return "FINDI_PARAM_PATCH";
        case FINDI_PARAM_PATCH_RAWCOPY   : return "FINDI_PARAM_PATCH_RAWCOPY";
        case FINDI_PARAM_PATCH_REBUILD   : return "FINDI_PARAM_PATCH_REBUILD";
        case FINDI_PARAM_PATCH_COMBINE   : return "FINDI_PARAM_PATCH_COMBINE";

        case FINDI_PARAM_PATCH_IDENTICAL : return "FINDI_PARAM_PATCH_IDENTICAL";

        case FINDI_PARAM_MASK_RAWCOPY    : return "FINDI_PARAM_MASK_RAWCOPY";
        case FINDI_PARAM_MASK_REBUILD    : return "FINDI_PARAM_MASK_REBUILD";
    }
    return "(-)";
}

BOOL DisplayDirInNewDfsInfo(const DIR_IN_NEW_DFS_INFO * pDindi,
                            const PCDIRINFO * pDirInfo, dfuLong32 dfNbDirDfsRead,
                            const DIR_IN_OLD_DFS_INFO * pDiodi,
                            dfuLong32 dfNbVersionRemix, const VERSIONTOADD_REMIX * pVersionRemix)
{
    dfuLong32 i, j;

    printf("\n\n" "DisplayDirInNewDfsInfo report !\n");

    for (i = 0; i < dfNbVersionRemix; i++)
    {
        const DIR_IN_NEW_DFS_INFO *pDindiCur = pDindi + i;
        printf("\n\nVersion %u : \n", i);
        printf
            ("dfNbFile=%u, dfNbFileAdding=%u, dfNbFileReuse=%u ,dfNumDirVersionOldSvf=%u\n",
             pDindiCur->dfNbFile, pDindiCur->dfNbFileAdding,
             pDindiCur->dfNbFileReuse, pDindiCur->dfNumDirVersionOldSvf);
        for (j = 0; j < pDindiCur->dfNbFile; j++)
        {
            const FILE_IN_NEW_DFS_INFO *pfindicur = (pDindiCur->pfindi) + j;
            printf(" dfOldDfsFileItem = %u \n", pfindicur->dfOldDfsFileItem);
            printf("    dfBldParameter = 0x%08x - %s\n",
                          pfindicur->dfBldParameter,GetBldParamString(pfindicur->dfBldParameter));
            printf("    dfPreviousVersionItemInOldSvf = %u \n", pfindicur->dfPreviousVersionItemInOldSvf);
            printf("    dfPreviousVersionItemInNewSvf = %u\n", pfindicur->dfPreviousVersionItemInNewSvf);
            if ((pfindicur->dfBldParameter & FINDI_PARAM_MASK_RAWCOPY) != 0)
            {
                //printf("Copy the raw compressed stream (FINDI_PARAM_MASK_RAWCOPY), stream location:\n");
                printf
                    ("   _RAWCOPY: dfOldDfsRawCopyDirLocation, dfOldDfsRawCopyFileLocation = %u,%u\n",
                     pfindicur->dfOldDfsRawCopyDirLocation, pfindicur->dfOldDfsRawCopyFileLocation);
            }

            if (pfindicur->pftaPreviousItem != NULL)
            {
                printf("    Previous Item is a inserted file :");
                printf("tostore=%ws , ondisk=%ws\n",
                       pfindicur->pftaPreviousItem->filename_tostore,
                       pfindicur->pftaPreviousItem->filename_ondisk);
            }

            if (pfindicur->pfta != NULL)
            {
                printf("    Item is a inserted file : ");
                printf("tostore=%ws , ondisk=%ws\n",
                       pfindicur->pfta->filename_tostore, pfindicur->pfta->filename_ondisk);
            }
        }
    }

    printf("\n\n");
    return TRUE;
}


/*
// verbose version
BOOL DisplayDirInNewDfsInfo(const DIR_IN_NEW_DFS_INFO* pDindi,
                            const PDIRINFO *pDirInfo,dfuLong32 dfNbDirDfsRead,
                            const DIR_IN_OLD_DFS_INFO * pDiodi,
                            dfuLong32 dfNbVersionRemix,
                            const VERSIONTOADD_REMIX *pVersionRemix)
{
    dfuLong32 i,j;

    printf("\n\n"\
           "DisplayDirInNewDfsInfo report !\n");

    for (i=0;i<dfNbVersionRemix;i++)
    {
        const DIR_IN_NEW_DFS_INFO* pDindiCur=pDindi+i;
        printf("\n\nVersion %u : \n",i);
        printf("dfNbFile=%u, dfNbFileAdding=%u, dfNbFileReuse=%u ,dfNumDirVersionOldSvf=%u\n",
                     pDindiCur->dfNbFile,pDindiCur->dfNbFileAdding,
                     pDindiCur->dfNbFileReuse,pDindiCur->dfNumDirVersionOldSvf);
        for (j=0;j<pDindiCur->dfNbFile;j++)
        {
            const FILE_IN_NEW_DFS_INFO *pfindicur=(pDindiCur->pfindi)+j;
            printf("dfOldDfsFileItem = %u : contain the item of the file in the Old DFS\n" \
                   "(the number stored in old DFS file),or FINDI_PREVDFS_FILEADDED for added file\n",
                   pfindicur->dfOldDfsFileItem);
            printf("dfBldParameter = 0x%08x\n",pfindicur->dfBldParameter);
            printf("dfPreviousVersionItemInOldSvf = %u : contain the item of previous version,\n"\
                   "in the previous directory WE COPY of old DFS or FINDI_PREVVERINNEWDFS_FILEADDED\n",
                   pfindicur->dfPreviousVersionItemInOldSvf);
            printf("dfPreviousVersionItemInNewSvf = %u : contain the item of previous version, in \n" \
                   "the previous directory of new DFS, for store in the directory we will build\n",
                   pfindicur->dfPreviousVersionItemInNewSvf);
            if ((pfindicur->dfBldParameter & FINDI_PARAM_MASK_RAWCOPY)!=0)
            {
                printf("Copy the raw compressed stream (FINDI_PARAM_MASK_RAWCOPY), stream location:\n");
                printf("dfOldDfsRawCopyDirLocation, dfOldDfsRawCopyFileLocation = %u,%u\n",
                    pfindicur->dfOldDfsRawCopyDirLocation, pfindicur->dfOldDfsRawCopyFileLocation);
            }

            if (pfindicur->pftaPreviousItem!=NULL)
            {
                printf("Previous Item is a inserted file\n");
                printf("tostore=%ws , ondisk=%ws\n",pfindicur->pftaPreviousItem->filename_tostore,
                    pfindicur->pftaPreviousItem->filename_ondisk);
            }

            if (pfindicur->pfta!=NULL)
            {
                printf("Item is a inserted file\n");
                printf("tostore=%ws , ondisk=%ws\n",pfindicur->pfta->filename_tostore,
                    pfindicur->pfta->filename_ondisk);
            }
        }
    }

    return TRUE;
}
*/
#endif

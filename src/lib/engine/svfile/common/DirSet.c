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

//#include <windows. h>

#include "../../patchstream/common/difbasic.h"

#include "../../patchstream/common/DfsTlTyp.h"
#include "../../patchstream/common/difstool.h"
#include "DfsStruc.h"
#include "DfsTagDf.h"
#include "DfsTagMg.h"
#include "DfsTagBlockFloatEnd.h"

#include "DfsIntf.h"
#include "../../patchstream/common/DfsOrigMemoryMap.h"
#include "../../patchstream/rebuild/RamDifWk.h"

#include "DfsSet.h"
#include "../compress/DfsWrSet.h"
#include "../decompress/DfsRdSet.h"

#include "ArrayTl.h"



#include "../../patchstream/common/DfsIoHlp.h"

#include "DirSet.h"


#include "../../patchstream/rebuild/RamDifTl.h"

void SVFAPI FreeFileSet(FILESET * pfs, BOOL fDeleteFile)
{
  dfuLong32 i;
  if (pfs->dfNbFileItem == 0)
    return;
  if (pfs->pFileItem == NULL)
    return;


  for (i = 0; i < pfs->dfNbFileItem; i++)
  {
    FILEITEM *pfi = (pfs->pFileItem) + i;
    if (pfi->FileNameOnArchive != NULL)
      DfsFree((dfwcharp) (pfi->FileNameOnArchive));
    if (pfi->FileNameOnDisk != NULL)
    {
      if (fDeleteFile && (pfi->fTempFile))
        DeleteTempFile(pfi->FileNameOnDisk, NULL);

      DfsFree((dfwcharp) (pfi->FileNameOnDisk));
      DeleteRamDif(pfi->hRamDifOfPatch);
      pfi->hRamDifOfPatch = NULL;
    }
  }
  DeleteArray(pfs->pFileItem);
  pfs->pFileItem = NULL;
  pfs->dfFileItemAllocated = 0;
  pfs->dfNbFileItem = 0;
  pfs->fTempFile = FALSE;
}

void SVFAPI InitFileSet(FILESET * pfs)
{
  pfs->pFileItem = NULL;
  pfs->dfFileItemAllocated = 0;
  pfs->dfNbFileItem = 0;
  pfs->dfFileItemStepAlloc = 0x10;
  pfs->fTempFile = FALSE;
}



BOOL SVFAPI AddItemToFileSet(FILESET * pfs, const FILEITEM * lpfi,
                      dfuLong32 dfNbFileItem)
{
  pfs->pFileItem =
    (FILEITEM *) AddArrayElem(pfs->pFileItem, &pfs->dfNbFileItem,
                              &pfs->dfFileItemAllocated,
                              pfs->dfFileItemStepAlloc, sizeof(FILEITEM),
                              lpfi, dfNbFileItem);
  return TRUE;
}

long DFSCALLBACK fncCompareFileItem(const void *lpElem1, const void *lpElem2)
{
  const FILEITEM *pfi1 = (const FILEITEM *) lpElem1;
  const FILEITEM *pfi2 = (const FILEITEM *) lpElem2;
  return dfUnicodeStrcmpi(pfi1->FileNameOnArchive, pfi2->FileNameOnArchive);
}

BOOL DFSCALLBACK fncDestructorFileItem(const void *lpElem)
{
  FILEITEM *pfi = (FILEITEM *) lpElem;
  if (pfi->FileNameOnArchive != NULL)
    DfsFree((dfwcharp) pfi->FileNameOnArchive);
  if (pfi->FileNameOnDisk != NULL)
    DfsFree((dfwcharp) pfi->FileNameOnDisk);

  return TRUE;
}

BOOL SVFAPI ExtractPatch(DFSFILE DfsFile, FILESET * pfsOrg, FILESET * pfsDest,
                  dfuLong32 dfNumDirBase,PCDIRINFO pDirInfo,
                  BOOL fNewFileSetTemp, BOOL fOldFileCanBeCaptured,BOOL fFlushWrite,
                  dfwcharpc szBasePath,
                  tProgressCallBack pProgressCallBack, dfvoidp dfUserPtr,
                  tExtractingFileWorkingEvent pExtractingFileWorkingEvent,dfvoidp dfUserPtrWorkingEvent,
                  H_ERROR_INFO* pei)
{
  /* Note : if fNewFileSetTemp is FALSE, we use the filename in (pfsDest+#)->FileNameOnDisk
     Else we build a temp filename
   */
  dfuLong32 i;
  dfuLong32 dfError = DFS_SUCCESS;
  FILETOEXTRACT *pftx;
  EXTR_WORK_EVENT_INFO ewei;

  ewei.dfNumVersion = dfNumDirBase;
  ewei.dfNumFile = 0;
  ewei.dfFileNameOnDisk = NULL;
  ewei.dfSuccess = 0;
  ewei.fTempFile = fNewFileSetTemp;
  if (pExtractingFileWorkingEvent!=NULL)
      (*pExtractingFileWorkingEvent)(EXTR_WORK_EVENT_BEFORE_EXTRACTING_VERSION,&ewei,dfUserPtrWorkingEvent);

  pftx =
    (FILETOEXTRACT *) DfsMalloc(sizeof(FILETOEXTRACT) *
                                (pfsDest->dfNbFileItem + 1));

  for (i = 0; i < pfsDest->dfNbFileItem; i++)
  {
    dfuLong32 dfPreviousVersion;
    (pftx + i)->fCorrectlyDone = FALSE;
    (pftx + i)->fSkipUserRequested = FALSE;
    (pftx + i)->fIgnore = ((pfsDest->pFileItem + i)->ExtAction) == ExtActionIgnore ;

    (pftx + i)->hRamDifWork = NULL;

//    (pftx + i)->fRawExtracting = FALSE;
    (pftx + i)->KindExtracting = KIND_EXTRACTING_APPLYSTREAMPATCH;



    (pftx + i)->filename_ondisk_to_write=NULL;
    (pftx + i)->filename_ondisk_previous_to_read = NULL;
    ///(pftx + i)->dfuSizeProjected = 0;
    (pftx + i)->fSetNewDate = FALSE;
    (pftx + i)->fTemporaryFile = FALSE;

    if (!(pftx + i)->fIgnore)
    {
        BOOL fCanCapturePrevious = FALSE;

        dfPreviousVersion = (pfsDest->pFileItem + i)->dfPreviousVersion;

        if (((((pfsDest->pFileItem + i)->ExtAction==ExtActionExtractContentInPlace)) || (((pfsDest->pFileItem + i)->ExtAction==ExtActionExtractContentInPlaceNoChecksum)))
                 && (fOldFileCanBeCaptured) && (dfPreviousVersion != VALUE_UNKNOWN) && (pfsOrg != NULL))
            fCanCapturePrevious = TRUE;

        ///(pftx + i)->dfuSizeProjected = (pfsDest->pFileItem + i)->dfFileSizeUncompressed;

        if (!fNewFileSetTemp)
        {
          (pftx + i)->filename_ondisk_to_write =
            dfUnicodeCopyConcatAlloc((pfsDest->pFileItem + i)->FileNameOnDisk,
                                     NULL);
          ///(pftx + i)->dfuSizeProjected = (pfsDest->pFileItem + i)->dfFileSizeUncompressed;

          if (((pfsDest->pFileItem + i)->fForceDate))
          {
              (pftx + i)->fSetNewDate = TRUE;
              (pftx + i)->dfsInfoDate = (pfsDest->pFileItem + i)->dfsInfoDate;
          }
        }
        else
        {
            /*
          fCanCapturePrevious =
              (fNewFileSetTemp && fOldFileCanBeCaptured
                && ((pfsDest->pFileItem + i)->fIdenticalPreviousVersion)
                && (dfPreviousVersion != VALUE_UNKNOWN) && (pfsOrg != NULL));
                */
          if  (fNewFileSetTemp && fOldFileCanBeCaptured
                && ((pfsDest->pFileItem + i)->fIdenticalPreviousVersion)
                && (dfPreviousVersion != VALUE_UNKNOWN) && (pfsOrg != NULL))
                fCanCapturePrevious=TRUE;

          if ((fCanCapturePrevious) &&
              ((pfsDest->pFileItem + i)->ExtAction==ExtActionExtractContent))
              if (((pfsOrg->pFileItem) + dfPreviousVersion)->hRamDifOfPatch != NULL)
                  fCanCapturePrevious=FALSE;//

          if ((fCanCapturePrevious) || ((pfsDest->pFileItem + i)->ExtAction==ExtActionMergeRamDif))
          {
              if (fCanCapturePrevious)
              {
                  (pftx + i)->fIgnore = TRUE;
                  (pfsDest->pFileItem + i)->hRamDifOfPatch = ((pfsOrg->pFileItem) + dfPreviousVersion)->hRamDifOfPatch;
                  ((pfsOrg->pFileItem) + dfPreviousVersion)->hRamDifOfPatch = NULL;
              }
              else
                  (pfsDest->pFileItem + i)->fIdenticalPreviousVersion = FALSE;

              (pfsDest->pFileItem + i)->FileNameOnDisk =
                  dfUnicodeCopyConcatAlloc(((pfsOrg->pFileItem) +
                                      dfPreviousVersion)->FileNameOnDisk, NULL);
              (pftx + i)->fTemporaryFile = (pfsDest->pFileItem + i)->fTempFile =
                              ((pfsOrg->pFileItem) + dfPreviousVersion)->fTempFile;
              ((pfsOrg->pFileItem) + dfPreviousVersion)->fTempFile=FALSE;
          }
          else
          { /*
            dfwchar szTempFN[1024];
            {
            dfvoidp TagBufProperties;
            dfuLong32 TagSizeProperties;
            DFTAGLIST TagListCurrentFileInDir = *(psTagListFileInDirCopy + i);

             if (GetTag
           (TagListCurrentFileInDir, DFSTAG_FILEPOSPROPERTIES,
            &TagBufProperties, &TagSizeProperties))
            }*/
  /*
            GetTemporaryFilename(L"SVF",szTempFN,(sizeof(szTempFN) / sizeof(dfwchar))-1);

            (pftx + i)->filename_ondisk_to_write =
              dfUnicodeCopyAlloc(szTempFN);*/
            (pftx + i)->filename_ondisk_to_write = NULL;
            (pftx + i)->fTemporaryFile=TRUE;

            if ((pfsDest->pFileItem + i)->FileNameOnDisk != NULL)
              DfsFree((dfwcharp) (pfsDest->pFileItem + i)->FileNameOnDisk);
            /*
            (pfsDest->pFileItem + i)->FileNameOnDisk =
              dfUnicodeCopyConcatAlloc(szTempFN, NULL);*/
            (pfsDest->pFileItem + i)->fTempFile = TRUE;
            (pfsDest->pFileItem + i)->fIdenticalPreviousVersion = FALSE;
          }
        }
        if ((dfPreviousVersion != VALUE_UNKNOWN) && (pfsOrg != NULL))
          (pftx + i)->filename_ondisk_previous_to_read =
            dfUnicodeCopyAlloc(((pfsOrg->pFileItem) +
                                      dfPreviousVersion)->FileNameOnDisk);
//printf("write ext:");DispOutUnicodeString( (pfsDest->pFileItem + i)->FileNameOnDisk);

        if ((dfPreviousVersion != VALUE_UNKNOWN) && (pfsOrg != NULL))
        {
            (pftx + i)->hRamDifWork = ((pfsOrg->pFileItem) + dfPreviousVersion)->hRamDifOfPatch;
            ((pfsOrg->pFileItem) + dfPreviousVersion)->hRamDifOfPatch = NULL;
        }

        if ((pftx + i)->hRamDifWork != NULL)
        {
            (pftx + i)->KindExtracting = KIND_EXTRACTING_COMBINERAMDIF_APPLY;
//            if (/*(fCanCapturePrevious) ||*/ ((pfsDest->pFileItem + i)->ExtAction==ExtActionExtractContentInPlace))
            if ((pfsDest->pFileItem + i)->ExtAction==ExtActionExtractContentInPlace)
                (pftx + i)->KindExtracting = KIND_EXTRACTING_COMBINERAMDIF_APPLY_INPLACE;
//            if (/*(fCanCapturePrevious) ||*/ ((pfsDest->pFileItem + i)->ExtAction==ExtActionExtractContentInPlaceNoChecksum))
            if ((pfsDest->pFileItem + i)->ExtAction==ExtActionExtractContentInPlaceNoChecksum)
                (pftx + i)->KindExtracting = KIND_EXTRACTING_COMBINERAMDIF_APPLY_INPLACE_NOCHECKSUM;
        }

        /**/
        if ((pftx + i)->hRamDifWork == NULL)
        {
//            if (/*(fCanCapturePrevious) || */((pfsDest->pFileItem + i)->ExtAction==ExtActionExtractContentInPlace))
            if ((pfsDest->pFileItem + i)->ExtAction==ExtActionExtractContentInPlace)
                (pftx + i)->KindExtracting = KIND_EXTRACTING_COMBINERAMDIF_APPLY_INPLACE;
//            if (/*(fCanCapturePrevious) || */((pfsDest->pFileItem + i)->ExtAction==ExtActionExtractContentInPlaceNoChecksum))
            if ((pfsDest->pFileItem + i)->ExtAction==ExtActionExtractContentInPlaceNoChecksum)
                (pftx + i)->KindExtracting = KIND_EXTRACTING_COMBINERAMDIF_APPLY_INPLACE_NOCHECKSUM;
        }
        /**/


        if  (((pfsDest->pFileItem + i)->ExtAction) == ExtActionMergeRamDif)
            (pftx + i)->KindExtracting = KIND_EXTRACTING_COMBINERAMDIF_TORAMDIF;
    }
  }

  dfError =
    ExtractDirectory(DfsFile, dfNumDirBase + 0, (pDirInfo),
                     pfsDest->dfNbFileItem, pftx,
                     pProgressCallBack, dfUserPtr,
                     pExtractingFileWorkingEvent,dfUserPtrWorkingEvent,fFlushWrite,pei);
  ewei.dfSuccess = dfError;
  if (pExtractingFileWorkingEvent!=NULL)
      (*pExtractingFileWorkingEvent)(EXTR_WORK_EVENT_EXTRACTING_VERSION_FINISHED,&ewei,dfUserPtrWorkingEvent);

  for (i = 0; i < pfsDest->dfNbFileItem; i++)
  {
      if ((!(pftx + i)->fIgnore) && ((pfsDest->pFileItem + i)->FileNameOnDisk == NULL) &&
          ((pfsDest->pFileItem + i)->fTempFile))
          (pfsDest->pFileItem + i)->FileNameOnDisk = dfUnicodeCopyAlloc((pftx + i)->filename_ondisk_to_write);

      //if ((!(pftx + i)->fIgnore) && ((pfsDest->pFileItem + i)->hRamDifOfPatch!=NULL))
      if ((!(pftx + i)->fIgnore) && ((pftx+i)->hRamDifWork != NULL))
          (pfsDest->pFileItem + i)->hRamDifOfPatch = (pftx+i)->hRamDifWork;
  }

  for (i = 0; (i < pfsDest->dfNbFileItem) && (dfError == DFS_SUCCESS); i++)
    if ((!(pftx + i)->fCorrectlyDone) && (!(pftx + i)->fIgnore) && (!(pftx + i)->fSkipUserRequested))
      dfError = DFS_ERROR_ERRORIO;

#if defined(_DEBUG) && defined(_CONSOLE)
  for (i = 0; i < pfsDest->dfNbFileItem; i++)
  {
    if ((!(pftx + i)->fCorrectlyDone) && (!(pftx + i)->fIgnore))
      printf("File '%ws'-'%ws' skipped or badly extracted on version %u\n",
             ((pfsDest->pFileItem) + i)->FileNameOnArchive,
             (pftx + i)->filename_ondisk_to_write, dfNumDirBase);
  }
#endif
#if defined(_DEBUG) && (!defined(_CONSOLE)) && defined (OUTPUTEXTRACTINGDEBUG)
  for (i = 0; i < pfsDest->dfNbFileItem; i++)
  {
    if ((!(pftx + i)->fCorrectlyDone) && (!(pftx + i)->fIgnore))
    {
        char sz[1024];
        sprintf(sz,"\nFile '%ws'-'%ws' (from old %ws) badly extracted on version %u\n",
             ((pfsDest->pFileItem) + i)->FileNameOnArchive,
             (pftx + i)->filename_ondisk_to_write,
             ((pftx + i)->filename_ondisk_previous_to_read != NULL) ? (pftx + i)->filename_ondisk_previous_to_read : L"(no)",
             dfNumDirBase);
        OutputDebugString(sz);
    }
  }
#endif


  for (i = 0; i < pfsDest->dfNbFileItem; i++)
  {
    if ((pftx + i)->filename_ondisk_to_write != NULL)
      DfsFree((dfwcharp) (pftx + i)->filename_ondisk_to_write);
    if ((pftx + i)->filename_ondisk_previous_to_read != NULL)
      DfsFree((dfwcharp) (pftx + i)->filename_ondisk_previous_to_read);
  }
  DfsFree(pftx);
/*
  if (pfsOrg != NULL)
  {
    //FreeFileSet(pfsOrg,TRUE);
    FreeFileSet(pfsOrg, pfsOrg->fTempFile);
  }
*/
  return dfError;
}


dfuLong32 SVFAPI CreateFileSetForVersionInDirectory(DFSFILE DfsFile,
                                             FILESET * pfs, dfuLong32 dfNumDir, dfwcharpc szDirBase,const FILESET* pFileSetBase,
                                             PCDIRINFO pDirInfoAlreadyProvided,
                                             BOOL * pfComplete,    // Will receive BOOL to known if all is good
                                             BOOL fVerboseNotFound,       // if must do printf for error
                                             BOOL fFillNameOnDisk,
                                             BOOL fTryFindFileOnDiskIfNotInsertingInDfs,
                                             BOOL fTryFindFileOnDiskIfInsertingInDfs,
                                             dfuLong32 * pdfTypeDir,
                                             BOOL fFillForceFileDateFromDirectoryInDfs,
                                             BOOL fFlatFileSet,H_ERROR_INFO* pei)
{
  dfuLong32 dfError;
  PCDIRINFO pDirInfo = NULL;
  PDIRINFO pDirInfoLoadInFunction=NULL;
  dfwcharp szDirBaseUse;
  dfuLong32 dfDirBaseLen = dfUnicodeStrlen(szDirBase);
  BOOL fTryFindFileOnDisk = FALSE;

  if (pfComplete != NULL)
    *pfComplete = TRUE;

  if (dfDirBaseLen == 0)
    szDirBaseUse = dfUnicodeCopyConcatAlloc(NULL, NULL);
  else
  {
    dfuLong32 dfDirBaseLen = dfUnicodeStrlen(szDirBase);
    if (((*(szDirBase + dfDirBaseLen - 1)) == '\\') ||
        ((*(szDirBase + dfDirBaseLen - 1)) == '/') ||
        ((*(szDirBase + dfDirBaseLen - 1)) == ':'))
      szDirBaseUse = dfUnicodeCopyAlloc(szDirBase);
    else
      szDirBaseUse = dfUnicodeCopyConcatAlloc(szDirBase, GetUnicodeStringDirectorySeparator());
  }

  FreeFileSet(pfs, FALSE);

  dfError = DFS_SUCCESS;
  if (pDirInfoAlreadyProvided != NULL)
      pDirInfo = pDirInfoAlreadyProvided;
  else
  {
      dfError = ReadDirectoryInfo(DfsFile, dfNumDir, &pDirInfoLoadInFunction, NULL, NULL,pei);
      pDirInfo = pDirInfoLoadInFunction;
  }

  if (dfError == DFS_SUCCESS)
  {
    if (pdfTypeDir != NULL)
      *pdfTypeDir = pDirInfo->dfTypeDir;
    if ((pDirInfo->dfTypeDir == TYPEDIR_FILEINSERTING_DEFLATE) ||
        (pDirInfo->dfTypeDir == TYPEDIR_FILEINSERTING_STORE))
      fTryFindFileOnDisk = fTryFindFileOnDiskIfInsertingInDfs;
    else
      fTryFindFileOnDisk = fTryFindFileOnDiskIfNotInsertingInDfs;
  }




  if (dfError == DFS_SUCCESS)
  {
    dfuLong32 i;
    dfuLong32 dfGetNbFile = pDirInfo->dfNbFile;
    for (i = 0; i < dfGetNbFile; i++)
    {
      FILEITEM fi;
      dfwcharp FileNameOnDisk=NULL;
      dfuLong64 dfSize;
      dfvoidp TagBuf;
      dfuLong32 TagSize;
      BOOL fFoundInFileSetBase=FALSE;

//      fi.dfFileSizeUncompressed = 0;

      fi.fTempFile = FALSE;
      fi.fIdenticalPreviousVersion = FALSE;
      fi.ExtAction=ExtActionExtractContent ;
      fi.fForceDate=FALSE;
      fi.hAddTags = NULL;
      fi.pReserved = NULL;
      fi.FileNameOnArchive =
        dfUnicodeCopyConcatAlloc((pDirInfo->pFileInDirInfo + i)->FileName,
                                 NULL);
      fi.FileNameOnDisk = NULL;
      fi.dfPreviousVersion = VALUE_UNKNOWN;
      fi.hRamDifOfPatch = NULL;
      //

      if (GetTag
          (*(pDirInfo->TagFile + i), DFSTAG_DATE, &TagBuf, &TagSize))
          if (TagSize == sizeof(DFSINFODATE))
      {
          fi.fForceDate = fFillForceFileDateFromDirectoryInDfs;
          fi.dfsInfoDate = *(DFSINFODATE *)TagBuf;
      }

      if (GetTag
          (*(pDirInfo->TagFile + i), DFSTAG_STORAGESTATUS, &TagBuf, &TagSize))
      {
        dfuLong32 dfFileIdentical =
          ConvertuLongIntelToLong(*(dfuLong32Intel *) TagBuf);
        if (dfFileIdentical == DFS_STORAGESTATUS_IDENTICAL)
          fi.fIdenticalPreviousVersion = TRUE;
      }
      if (GetTag
          (*(pDirInfo->TagFile + i), DFSTAG_PREVIOUSVERSIONINFO, &TagBuf,
           &TagSize))
      {
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
            fi.dfPreviousVersion = dfPreviousVersionFilePosition;
        }
      }


      if ((fTryFindFileOnDisk || fFillNameOnDisk) && (pFileSetBase!=NULL))
      {
          dfuLong32 i;
          for (i=0;i<pFileSetBase->dfNbFileItem;i++)
          {
              const FILEITEM* pfi=((pFileSetBase->pFileItem) + i);
              if (dfUnicodeStrcmpi(pfi->FileNameOnArchive,fi.FileNameOnArchive)==0)
              {
                  fFoundInFileSetBase=TRUE;
                  fi.FileNameOnDisk = dfUnicodeCopyAlloc(pfi->FileNameOnDisk);
              }
          }
      }

      if ((fTryFindFileOnDisk || fFillNameOnDisk) && (!fFoundInFileSetBase))
      {
        dfwcharpc dfwFileNameOnArchiveUse=fi.FileNameOnArchive;
        if (fFlatFileSet)
        {
            dfuLong32 i=0;
            while (fi.FileNameOnArchive[i] != 0)
            {
                if ((fi.FileNameOnArchive[i] == '\\') || (fi.FileNameOnArchive[i] == '/'))
                    dfwFileNameOnArchiveUse=((dfwcharpc)fi.FileNameOnArchive)+i+1;
                i++;
            }
        }

        FileNameOnDisk =
          dfUnicodeCopyConcatAlloc(szDirBaseUse, dfwFileNameOnArchiveUse);
        if (fTryFindFileOnDisk)
        {
          dfSize = GetFileSizeByName(FileNameOnDisk, NULL, pei);
          if (dfSize == FILE_SIZE_NOT_EXIST)
          {
            if (dfError == DFS_SUCCESS)
              dfError = DFS_ERROR_ERRORIO;

            if (fVerboseNotFound)
            {
                printf("file ");
                DispOutUnicodeString(FileNameOnDisk);
                printf(" not found\n");
            }
            if (pfComplete != NULL)
              *pfComplete = FALSE;

            if (pei != NULL)
                if ((*pei) == NULL)
            {
                ERROR_INFO_ITEM eii;
                eii.dfInfoTag = DFS_ERRORTAG_FILENAME;
                eii.pInfo = (dfbytep)FileNameOnDisk;
                eii.dfSize = (dfUnicodeStrlen(FileNameOnDisk) + 1) * 2;

                *pei = CreateErrorInfoBlock(DFS_ERROR_FILE_NOT_FOUND, 0, 1, &eii);
            }


            DfsFree(FileNameOnDisk);
          }
          else
          if (dfSize != (pDirInfo->pFileInDirInfo + i)->dfSize)
          {

              if (dfError == DFS_SUCCESS)
                  dfError = DFS_ERROR_ERRORIO;

              if (fVerboseNotFound)
              {
                  printf("file ");
                  DispOutUnicodeString(FileNameOnDisk);
                  printf(" has bad size\n");
              }
              if (pfComplete != NULL)
                  *pfComplete = FALSE;

              if (pei != NULL)
                  if ((*pei) == NULL)
              {
                  ERROR_INFO_ITEM eii;
                  eii.dfInfoTag = DFS_ERRORTAG_FILENAME;
                  eii.pInfo = (dfbytep)FileNameOnDisk;
                  eii.dfSize = (dfUnicodeStrlen(FileNameOnDisk) + 1) * 2;

                  *pei = CreateErrorInfoBlock(DFS_ERROR_BAD_CHECKSUM, 0, 1, &eii);
              }

              DfsFree(FileNameOnDisk);
          }
          else
          {
            fi.FileNameOnDisk = FileNameOnDisk;
            ///fi.dfFileSizeUncompressed = dfSize;
          }
        }
        else
          fi.FileNameOnDisk = FileNameOnDisk;
        ConvertFileNameAndPath(fi.FileNameOnArchive, NULL, 0, FALSE);
      }
      //printf("add %ws,%ws\n",fi.FileNameOnArchive,fi.FileNameOnDisk);
      if (!AddItemToFileSet(pfs, &fi, 1))
      {
        dfError = DFS_ERROR_MEMORY_ERROR;
        break;
      }
    }
  }
  DfsFree(szDirBaseUse);
  if (pDirInfoLoadInFunction != NULL)
    FreeDirectoryInfo(&pDirInfoLoadInFunction,pei);
  return dfError;
}

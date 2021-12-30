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

//#include <windows. h>
/*
#include <stdlib.h>
#include <string.h>
*/
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "zlib.h"
#include "../../patchstream/common/difbasic.h"

#include "../../patchstream/common/DfsTlTyp.h"
#include "../../patchstream/common/difstool.h"
#include "../../svfile/common/DfsMFile.h"
#include "../../patchstream/common/DfsType.h"
#include "DfsTagDf.h"
#include "DfsTagMg.h"
#include "DfsTagBlockFloatEnd.h"

#include "DfsIntf.h"
#include "DfsStruc.h"

#include "../../svfile/common/DfsMtStr.h" // to remove soon
#include "../../../../SvfVersion.h"

#include "DfsIntfInternal.h"

void ClearDirInWorkMember(DFSFILEINTERNAL * DfsFileInternal,
                          BOOL fClearMemBlock)
{
  if (fClearMemBlock)
    if (DfsFileInternal->CurDirFileTagInBuildingPtr != NULL)
      DfsFree(DfsFileInternal->CurDirFileTagInBuildingPtr);
  DfsFileInternal->CurDirFileTagInBuildingPtr = NULL;
  DfsFileInternal->CurDirFileTagInBuildingAllocated = 0;
  DfsFileInternal->CurDirFileTagInBuildingSize = 0;
  DfsFileInternal->CurDirStepAlloc = 0x1000;


  DfsFileInternal->dfNumberFilesInDir = VALUE_UNKNOWN;
  DfsFileInternal->dfTypeCurDir = 0;

  /*
     if ((DfsFileInternal->dfTagListCurDirInfo!=NULL) && fClearMemBlock)
     DfsFree(DfsFileInternal->dfTagListCurDirInfo);
   */
  DfsFileInternal->dfTagListCurDirInfo = NULL;
  DfsFileInternal->dfTagListCurDirInfoIntro = NULL;
}

dfuLong32 SVFAPI DfsOpen(DFSFILEINFOPARAM DfsFileParam, DFSFILE* DfsFile, H_ERROR_INFO* pei)
{
    return DfsFileOpen(&DfsFileParam, DfsFile, pei);
}

dfuLong32 SVFAPI GetSvfVersion()
{
    return (((dfuLong32)SVF_VERSION_MAJOR) * ((dfuLong32)0x01000000)) |
           (((dfuLong32)SVF_MINOR_DIGIT_1) * ((dfuLong32)0x010000)) |
           (((dfuLong32)SVF_MINOR_DIGIT_2) * ((dfuLong32)0x0100));

}

dfuLong32 SVFAPI GetSvfVersionDate()
{
    return (((dfuLong32)SVF_VERSION_YEAR) * ((dfuLong32)0x010000));

}

dfuLong32 SVFAPI DfsFileOpen(const DFSFILEINFOPARAM *pDfsFileParam, DFSFILE* DfsFile, H_ERROR_INFO* pei)
{
  DFSFILEINTERNAL *DfsFileInternal = NULL;
  dfuLong32 dfError = DFS_SUCCESS;
  BOOL fNeedParseDirectoryForOldComment=FALSE;
  BOOL fCreateNewFile, fOpenExisting;
  //dfuLong32 error = 0;
  dfbytep pBlockEnd = NULL;
  dfuLong32 dfBlockType = 0;

  *DfsFile = NULL;
  fCreateNewFile = fOpenExisting = FALSE;

  if (pDfsFileParam->dfStatus == (DFS_NEWFILE | DFS_WRITABLE))
    fCreateNewFile = TRUE;
  if (pDfsFileParam->dfStatus == DFS_WRITABLE)
    fOpenExisting = TRUE;
  if (pDfsFileParam->dfStatus == DFS_READABLE)
    fOpenExisting = TRUE;

  if ((!fCreateNewFile) && (!fOpenExisting))
    dfError = DFS_ERROR_BAD_PARAMETER;

  if (dfError == DFS_SUCCESS)
  {
    DfsFileInternal = (DFSFILEINTERNAL *) DfsMalloc(sizeof(DFSFILEINTERNAL));
    DfsFileInternal->DfsDirListHead = NULL;
  }



  if (DfsFileInternal == NULL)
    dfError = DFS_ERROR_MEMORY_ERROR;
  else
  {
    DfsFileInternal->dfDirInfoInternal = NULL;
    //DfsFileInternal->dfNbDir_unused = 0;
    DfsFileInternal->dfNbDirAllocated = 0;
    DfsFileInternal->dfDirStepAlloc = 0x10;

    DfsFileInternal->dfNumberOfDir = 0;
    DfsFileInternal->dfTagBlockFloat = NULL;
    DfsFileInternal->DfsFeatureParam.fStripIdenticalBody=FALSE;
    DfsFileInternal->DfsFeatureParam.fComputeMd5 = TRUE;
    DfsFileInternal->DfsFeatureParam.fComputeSha1 = FALSE;
    DfsFileInternal->DfsFeatureParam.fComputeSha256 = FALSE;

    DfsFileInternal->CurDfsDirListNbDir = 0;

    DfsFileInternal->CurDfsDirListHeadSize = sizeof(DFSLISTDIRHEADHEADER64);
    DfsFileInternal->CurDfsDirListHeadStepAlloc = sizeof(DFSDIRINFO64) * 4;
    DfsFileInternal->CurDfsDirListHeadAllocated =
      DfsFileInternal->CurDfsDirListHeadSize;
    DfsFileInternal->DfsDirListHead =
      (DFSLISTDIRHEAD64 *) DfsMalloc(DfsFileInternal->
                                   CurDfsDirListHeadAllocated);

    if (DfsFileInternal->DfsDirListHead == NULL)
      dfError = DFS_ERROR_MEMORY_ERROR;


    ClearDirInWorkMember(DfsFileInternal, FALSE);
    DfsFileInternal->dfCurState = CURSTATE_NODIROPENED_ATENDOFDATA;
    DfsFileInternal->dfPosOfLastUnfloatDataWhenAtEndOfDataHigh =
    DfsFileInternal->dfPosOfLastUnfloatDataWhenAtEndOfDataLow = INVALID_POSITION;//DfsTell(DfsFileInternal->DfsFileWrap,&error);

    DfsFileInternal->fModifyingDone = FALSE;
    DfsFileInternal->fModifyingDoneNoFlushed = FALSE;

    DfsFileInternal->dfTagListCurFileInFinalDir = NULL;
    DfsFileInternal->dfTagListCurFileInPreFileInfoDir = NULL;

    DfsFileInternal->dfStatus = pDfsFileParam->dfStatus;
    DfsFileInternal->dfPosBlockDataEndHigh =
    DfsFileInternal->dfPosBlockDataEndLow = 0;


    if (dfError == DFS_SUCCESS)
    {
      DFSFILEINFOPARAMINTERNAL DfsFileParamInternal;
      DfsFileParamInternal.sizeStruct = sizeof(DFSFILEINFOPARAMINTERNAL);
      DfsFileParamInternal.dfStatus = pDfsFileParam->dfStatus;
      DfsFileParamInternal.filename = pDfsFileParam->filename;
      DfsFileInternal->DfsFileWrap = DfsOpenFile(DfsFileParamInternal,pei);
      if (DfsFileInternal->DfsFileWrap == NULL)
        dfError = DFS_ERROR_ERRORIO;
    }
    /* todo : if open, read listdir */
    if ((fOpenExisting) && (dfError == DFS_SUCCESS))
    {
      DFSLISTDIREND DfsListDirEnd;
      dfuLong32 error = 0;
      dfuLong32 dfFileSizeLow,dfFileSizeHigh;
      DfsGetSize(DfsFileInternal->DfsFileWrap,&dfFileSizeLow,&dfFileSizeHigh,pei);

      if ((dfFileSizeLow < sizeof(DFSLISTDIREND)) && (dfFileSizeHigh==0))
        dfError = DFS_ERROR_ERRORIO;
      else
      {
        dfuLong32 dfFilePosLow=dfFileSizeLow;
        dfuLong32 dfFilePosHigh=dfFileSizeHigh;
        dfSub64(&dfFilePosLow,&dfFilePosHigh,sizeof(DFSLISTDIREND),0);
        error =
          DfsSeek(DfsFileInternal->DfsFileWrap,dfFilePosLow,dfFilePosHigh,pei);
      }
      if (error != 0)
        dfError = DFS_ERROR_ERRORIO;

      if (dfError == DFS_SUCCESS)
      {
        DfsRead(DfsFileInternal->DfsFileWrap, &DfsListDirEnd,
                sizeof(DfsListDirEnd), &error,pei);
        if (error != 0)
          dfError = DFS_ERROR_ERRORIO;
      }

      if (dfError == DFS_SUCCESS)
      {
        DfsFileInternal->CurDfsDirListHeadSize =
          ConvertuLongIntelToLong(DfsListDirEnd.dfBlockSize);
        /* finish and read the end */

        if (((DfsFileInternal->CurDfsDirListHeadSize +
              sizeof(DfsListDirEnd)) > dfFileSizeLow) && (dfFileSizeHigh==0))
          dfError = DFS_ERROR_ERRORIO;
      }

      if (dfError == DFS_SUCCESS)
      {
          pBlockEnd = (dfbytep)DfsMalloc(DfsFileInternal->CurDfsDirListHeadSize+1);
          if (pBlockEnd == NULL)
              dfError = DFS_ERROR_MEMORY_ERROR;
      }

      if (dfError == DFS_SUCCESS)
      {
        dfuLong32 dfPosBlockDataEndUnAlignLow = dfFileSizeLow;
        dfuLong32 dfPosBlockDataEndUnAlignHigh = dfFileSizeHigh;
        dfSub64(&dfPosBlockDataEndUnAlignLow,&dfPosBlockDataEndUnAlignHigh,
                DfsFileInternal->CurDfsDirListHeadSize+sizeof(DFSLISTDIREND),0);

        error =
          DfsSeek(DfsFileInternal->DfsFileWrap,
                  dfPosBlockDataEndUnAlignLow,dfPosBlockDataEndUnAlignHigh,pei);

        DfsTellDivAlign(DfsFileInternal->DfsFileWrap,
                        &DfsFileInternal->dfPosBlockDataEndLow, &DfsFileInternal->dfPosBlockDataEndHigh, &error,pei);

        if (error == 0)
          DfsRead(DfsFileInternal->DfsFileWrap,
                  pBlockEnd,
                  DfsFileInternal->CurDfsDirListHeadSize, &error, pei);

        if (error != 0)
          dfError = DFS_ERROR_ERRORIO;
      }

      if (dfError == DFS_SUCCESS)
      {
          dfBlockType = ConvertuLongIntelToLong(((DFSLISTDIRHEAD64*)pBlockEnd)->DfsListDirHeadHeader.dfBlocType);
          if ((dfBlockType != BLOCKTYPE_LISTDIR) && (dfBlockType != BLOCKTYPE_LISTDIR64))
              dfError = DFS_ERROR_BAD_PARAMETER;
      }

      if ((dfError == DFS_SUCCESS) && (dfBlockType == BLOCKTYPE_LISTDIR))
      {
        DFSLISTDIRHEADOLD* pDfsListDirHeadOld = (DFSLISTDIRHEADOLD*)pBlockEnd;
        dfuLong32 dfNbDir = ConvertuLongIntelToLong(pDfsListDirHeadOld -> DfsListDirHeadHeader.dfNbDir);
        DfsFileInternal->CurDfsDirListHeadSize = sizeof(DFSLISTDIRHEAD64) +
                    (sizeof(DFSDIRINFO64) * dfNbDir);
        if (!DfsCheckAllocatedMemory((dfvoidp*)&DfsFileInternal->DfsDirListHead,
                                     &DfsFileInternal->
                                     CurDfsDirListHeadAllocated,
                                     DfsFileInternal->
                                     CurDfsDirListHeadSize,
                                     DfsFileInternal->
                                     CurDfsDirListHeadStepAlloc))
          dfError = DFS_ERROR_MEMORY_ERROR;
        else
        {
            dfuLong32 i;
            DfsClearStruct(DfsFileInternal->DfsDirListHead,0,DfsFileInternal->CurDfsDirListHeadSize);
            DfsFileInternal->DfsDirListHead->DfsListDirHeadHeader.dfBlocType = ConvertuLongToLongIntel(BLOCKTYPE_LISTDIR64);
            DfsFileInternal->DfsDirListHead->DfsListDirHeadHeader.dfNbDir = pDfsListDirHeadOld -> DfsListDirHeadHeader.dfNbDir;
            DfsFileInternal->DfsDirListHead->DfsListDirHeadHeader.dfTypeDfsFile = pDfsListDirHeadOld -> DfsListDirHeadHeader.dfTypeDfsFile;
            DfsFileInternal->DfsDirListHead->DfsListDirHeadHeader.dfOffsetPostFileTagLow =
                       pDfsListDirHeadOld -> DfsListDirHeadHeader.dfOffsetPostFileTag;
            DfsFileInternal->DfsDirListHead->DfsListDirHeadHeader.dfOffsetPostFileTagHigh =
                       ConvertuLongToLongIntel(0);
            for (i=0;i<dfNbDir;i++)
            {
                DfsFileInternal->DfsDirListHead->dfsDirInfo[i].dfTypeDir = pDfsListDirHeadOld->dfsDirInfo[i].dfTypeDir;
                DfsFileInternal->DfsDirListHead->dfsDirInfo[i].dfOffsetDirIntroLow = pDfsListDirHeadOld->dfsDirInfo[i].dfOffsetDirIntro;
                DfsFileInternal->DfsDirListHead->dfsDirInfo[i].dfOffsetDirIntroHigh = ConvertuLongToLongIntel(0);
                DfsFileInternal->DfsDirListHead->dfsDirInfo[i].dfOffsetDirLow = pDfsListDirHeadOld->dfsDirInfo[i].dfOffsetDir;
                DfsFileInternal->DfsDirListHead->dfsDirInfo[i].dfOffsetDirHigh = ConvertuLongToLongIntel(0);
                DfsFileInternal->DfsDirListHead->dfsDirInfo[i].dfBlockDirSize = pDfsListDirHeadOld->dfsDirInfo[i].dfBlockDirSize;
            }
        }
      }

      if ((dfError == DFS_SUCCESS) && (dfBlockType == BLOCKTYPE_LISTDIR64))
      {
        if (!DfsCheckAllocatedMemory((dfvoidp*)&DfsFileInternal->DfsDirListHead,
                                     &DfsFileInternal->
                                     CurDfsDirListHeadAllocated,
                                     DfsFileInternal->
                                     CurDfsDirListHeadSize,
                                     DfsFileInternal->
                                     CurDfsDirListHeadStepAlloc))
          dfError = DFS_ERROR_MEMORY_ERROR;
        else
          DfsMemcpy(DfsFileInternal->DfsDirListHead,pBlockEnd,DfsFileInternal->CurDfsDirListHeadSize);
      }

      if (dfError == DFS_SUCCESS)
      {
        if (error != 0)
          dfError = DFS_ERROR_ERRORIO;
        if (dfError == DFS_SUCCESS)
        {
          dfuLong32 dfOffsetPostFileTagLow,dfOffsetPostFileTagHigh;
          DfsFileInternal->dfCurState = CURSTATE_RD_BEFOREREAD_BLOCKTYPE;
          DfsFileInternal->dfNumberOfDir =
          DfsFileInternal->CurDfsDirListNbDir =
            ConvertuLongIntelToLong(DfsFileInternal->DfsDirListHead->DfsListDirHeadHeader.dfNbDir);
          dfOffsetPostFileTagLow = ConvertuLongIntelToLong(DfsFileInternal->DfsDirListHead->DfsListDirHeadHeader.dfOffsetPostFileTagLow);
          dfOffsetPostFileTagHigh = ConvertuLongIntelToLong(DfsFileInternal->DfsDirListHead->DfsListDirHeadHeader.dfOffsetPostFileTagHigh);
          if ((dfOffsetPostFileTagLow != 0) || (dfOffsetPostFileTagHigh != 0))
          {
              dfuLong32 errorTagFloat ;
              dfvoidp ptrUncprFloat=NULL;
              dfuLong32 dfSizeCptFloat=0;
              DFSENDFILETAGHEADER DfsEndFileTagHeader;

              errorTagFloat = DfsSeekMulAlign(DfsFileInternal->DfsFileWrap,dfOffsetPostFileTagLow,dfOffsetPostFileTagHigh,pei);
              if (errorTagFloat == DFS_SUCCESS)
              {
                  {
                    DfsTellDivAlign(DfsFileInternal->DfsFileWrap,
                                    &DfsFileInternal->dfPosBlockDataEndLow, &DfsFileInternal->dfPosBlockDataEndHigh,
                                    &error,pei);
                  }

                  DfsRead(DfsFileInternal->DfsFileWrap,&DfsEndFileTagHeader,sizeof(DFSENDFILETAGHEADER),&errorTagFloat,pei);
              }
              if (errorTagFloat == DFS_SUCCESS)
              {
                  dfSizeCptFloat = ConvertuLongIntelToLong(DfsEndFileTagHeader.dfTotalSizeCompressedFloatTag);
                  ptrUncprFloat = DfsMalloc(dfSizeCptFloat+0x10);
              }

              if (ptrUncprFloat!=NULL)
              {
                  dfuLong32 dfSizeRead = DfsRead(DfsFileInternal->DfsFileWrap,ptrUncprFloat,dfSizeCptFloat,&errorTagFloat,pei);
                  if ((dfSizeRead==dfSizeCptFloat) && (errorTagFloat==DFS_SUCCESS))
                  {
                      DfsFileInternal->dfTagBlockFloat = ReadDfTagBlockFloatSizeLimited(ptrUncprFloat,NULL,dfSizeCptFloat);
                  }
                  DfsFree(ptrUncprFloat);
              }
          }
          else fNeedParseDirectoryForOldComment=TRUE;
        }
        DfsSeek(DfsFileInternal->DfsFileWrap, 0, 0,pei);
      }

      if ((dfError == DFS_SUCCESS) && (DfsFileInternal->dfTagBlockFloat==NULL))
      {
          DfsFileInternal->dfTagBlockFloat = BuildEmptyBlockFloat();
          AddTaguLongBlockFloat(DfsFileInternal->dfTagBlockFloat,FLOATNUM_NOSPECIFIC,FLOATNUM_NOSPECIFIC,DFSTAG_NOFLOATFILLED,1);
      }
    }
    /* build empty dfTagBlockFloat for new file */
    if ((dfError == DFS_SUCCESS) && (DfsFileInternal->dfTagBlockFloat==NULL))
        DfsFileInternal->dfTagBlockFloat = BuildEmptyBlockFloat();
  }


  if ((dfError != DFS_SUCCESS) && ((DfsFileInternal) != NULL))
  {
    if (DfsFileInternal->DfsFileWrap != NULL)
      DfsCloseFile(DfsFileInternal->DfsFileWrap,NULL);
    if (DfsFileInternal->dfTagBlockFloat != NULL)
      CloseDfTagBlockFloat(DfsFileInternal->dfTagBlockFloat);
    if (DfsFileInternal->DfsDirListHead != NULL)
      DfsFree(DfsFileInternal->DfsDirListHead);
    DfsFree(DfsFileInternal);
    DfsFileInternal = NULL;
  }

  if (dfError == DFS_SUCCESS)
    *DfsFile = (DFSFILE) DfsFileInternal;

  if (pBlockEnd != NULL)
      DfsFree(pBlockEnd);

  return dfError;
}

void SVFAPI SetDfsExtendedMode(DFSFILE DfsFile, BOOL fStripIdenticalBody)
{
  DFSFILEINTERNAL *DfsFileInternal = (DFSFILEINTERNAL *) DfsFile;
  if (DfsFileInternal == NULL)
    return ;
  DfsFileInternal->DfsFeatureParam.fStripIdenticalBody=fStripIdenticalBody;
}

void SVFAPI GetDfsExtendedMode(DFSFILE DfsFile, BOOL* pfStripIdenticalBody)
{
  DFSFILEINTERNAL *DfsFileInternal = (DFSFILEINTERNAL *) DfsFile;
  if (DfsFileInternal == NULL)
    return ;
  *pfStripIdenticalBody = DfsFileInternal->DfsFeatureParam.fStripIdenticalBody;
}

void SVFAPI SetDfsFeatureParam(DFSFILE DfsFile, const DFSFEATUREPARAM* pFill)
{
  DFSFILEINTERNAL *DfsFileInternal = (DFSFILEINTERNAL *) DfsFile;
  if (DfsFileInternal == NULL)
    return ;
  DfsFileInternal->DfsFeatureParam = *pFill;
}

void SVFAPI GetDfsFeatureParam(DFSFILE DfsFile, DFSFEATUREPARAM* pFill)
{
  DFSFILEINTERNAL *DfsFileInternal = (DFSFILEINTERNAL *) DfsFile;
  if (DfsFileInternal == NULL)
    return ;
  *pFill = DfsFileInternal->DfsFeatureParam;
}


dfuLong32 SVFAPI DfsClose(DFSFILE DfsFile, H_ERROR_INFO * pei)
{
  /* todo: write the final directory table, (DfsDirListHead) cleanup */
  dfuLong32 dfError = DFS_SUCCESS;
  //dfuLong32 error = 0;
  DFSFILEINTERNAL *DfsFileInternal = (DFSFILEINTERNAL *) DfsFile;
  if (DfsFileInternal == NULL)
    return DFS_ERROR_BAD_PARAMETER;

#ifndef SVF_EXTRACT_ONLY
  dfError = DfsFlushWriteDfsFile(DfsFile,pei);
  if (dfError != DFS_SUCCESS)
      return dfError;
#endif

  DfsFree(DfsFileInternal->DfsDirListHead);

  if (DfsCloseFile(DfsFileInternal->DfsFileWrap,pei) != 0)
    dfError = DFS_ERROR_ERRORIO;

  if (DfsFileInternal->dfTagBlockFloat != NULL)
      CloseDfTagBlockFloat(DfsFileInternal->dfTagBlockFloat);


  if (dfError == DFS_SUCCESS)
  {
    DfsFree(DfsFileInternal);
  }

  return dfError;
}





/****************************************************************************/
/* We must work with state: */
/*
#define CURSTATE_RD_BEFOREREAD_BLOCKTYPE                (0x00000020)
#define CURSTATE_RD_BEFOREREAD_BLOCK_FILE               (0x00000021)
#define CURSTATE_RD_BEFOREREAD_BLOCK_POSTFILE           (0x00000022)
#define CURSTATE_RD_BEFOREREAD_BLOCK_DIRINTRO           (0x00000023)
#define CURSTATE_RD_BEFOREREAD_BLOCK_DIR                (0x00000024)
#define CURSTATE_RD_BEFOREREAD_BLOCK_LISTDIR            (0x00000025)

#define CURSTATE_RD_READINGFILECONTENT                  (0x0000002f)

*/
/****************************************************************************/


static dfuLong32 DfsRReadDfBlockType(DFSFILE DfsFile, dfuLong32 * pdfBlockType, H_ERROR_INFO * pei)
{
  dfuLong32 dfError = DFS_SUCCESS;
  dfuLong32 error = 0;
  dfuLong32Intel dfBlockTypeIntel;
  DFSFILEINTERNAL *DfsFileInternal = (DFSFILEINTERNAL *) DfsFile;
  if (DfsFileInternal == NULL)
    return DFS_ERROR_BAD_PARAMETER;

  if (DfsFileInternal->dfCurState != CURSTATE_RD_BEFOREREAD_BLOCKTYPE)
    return DFS_ERROR_BAD_PARAMETER;

  DfsReadOrWriteFillAlign(DfsFileInternal->DfsFileWrap, FALSE, &error,pei);
  if (error == 0)
    DfsRead(DfsFileInternal->DfsFileWrap, &dfBlockTypeIntel,
            sizeof(dfBlockTypeIntel), &error, pei);

  if (error != 0)
    dfError = DFS_ERROR_ERRORIO;
  else
  {
    dfuLong32 dfBlockType = ConvertuLongIntelToLong(dfBlockTypeIntel);
    DfsFileInternal->dfBlockTypeJustRead = dfBlockType;
    if (pdfBlockType != NULL)
      *pdfBlockType = dfBlockType;
    switch (dfBlockType)
    {
    case BLOCKTYPE_FILE:
      DfsFileInternal->dfCurState = CURSTATE_RD_BEFOREREAD_BLOCK_FILE;
      break;

    case BLOCKTYPE_DIRINTRO:
      DfsFileInternal->dfCurState = CURSTATE_RD_BEFOREREAD_BLOCK_DIRINTRO;
      break;

    case BLOCKTYPE_DIR:
      DfsFileInternal->dfCurState = CURSTATE_RD_BEFOREREAD_BLOCK_DIR;
      break;

    case BLOCKTYPE_LISTDIR:
      DfsFileInternal->dfCurState = CURSTATE_RD_BEFOREREAD_BLOCK_LISTDIR;
      break;

    default:
      dfError = DFS_ERROR_ERRORIO;
    }
  }
  return dfError;
}

static dfuLong32 DfsRReadNextHeaderBlockAfterBlockType(DFSFILE DfsFile,
                                                     void *Buf, dfuLong32 Size,
                                                     dfuLong32 * errorCode, H_ERROR_INFO * pei)
{
  dfuLong32 dfError = DFS_SUCCESS;
  dfuLong32 error = 0;

  DFSFILEINTERNAL *DfsFileInternal = (DFSFILEINTERNAL *) DfsFile;
  if (DfsFileInternal == NULL)
    return DFS_ERROR_BAD_PARAMETER;

  DfsRead(DfsFileInternal->DfsFileWrap,
          ((dfbytep) Buf) + sizeof(dfuLong32Intel), Size - sizeof(dfuLong32Intel),
          &error,pei);

  if (error != 0)
    dfError = DFS_ERROR_ERRORIO;
  else
    *((dfuLong32Intel *) Buf) =
      ConvertuLongToLongIntel(DfsFileInternal->dfBlockTypeJustRead);
  return dfError;
}

/****************************************************************************/

dfuLong32 DfsRGetNextBlockType(DFSFILE DfsFile, dfuLong32 * dfBlockType, H_ERROR_INFO * pei)
{
  dfuLong32 dfError = DFS_SUCCESS;
  dfError = DfsRReadDfBlockType(DfsFile, dfBlockType,pei);
  return dfError;
}

dfuLong32 DfsROpenNextFileAndTagBeforeFile(DFSFILE DfsFile,
                                         dfuLong32 * pdfContentStoreMethod,
                                         dfuLong64 * pdfContentEncodedSize,
                                         dfuLong64 * pdfContentUncompressedSize,
                                         DFTAGLIST * TagListFileCopy,
                                         H_ERROR_INFO * pei)
{
  dfuLong32 dfError = DFS_SUCCESS;
  dfuLong32 error = 0;
  DFSFILEINTERNAL *DfsFileInternal = (DFSFILEINTERNAL *) DfsFile;
  DFSFILEHEADER64 DfsFileHeader;
  dfvoidp ptrDirTag = NULL;
  dfuLong32 dfTagListSize = 0;
  dfuLong32 dfuSizeHeader ;

  if (DfsFileInternal == NULL)
    return DFS_ERROR_BAD_PARAMETER;

  if (DfsFileInternal->dfCurState != CURSTATE_RD_BEFOREREAD_BLOCK_FILE)
    return DFS_ERROR_BAD_PARAMETER;

  DfsFileInternal->dfEncodedSizeExecuted = 0;
  DfsFileInternal->dfEncodedCrc32 = 0;

  memset(&DfsFileHeader,0,sizeof(DfsFileHeader));

  DfsRReadNextHeaderBlockAfterBlockType(DfsFile, &DfsFileHeader,
                                        sizeof(DFSFILEHEADER32), &error, pei);
  if (error != 0)
    dfError = DFS_ERROR_ERRORIO;

  dfuSizeHeader = ConvertuLongIntelToLong(DfsFileHeader.dfsFileHeaderSize);

  if (dfError == DFS_SUCCESS)
    if ((dfuSizeHeader > sizeof(DFSFILEHEADER64)) || (dfuSizeHeader < sizeof(DFSFILEHEADER32)))
      dfError = DFS_ERROR_ERRORIO;

  if ((dfError == DFS_SUCCESS) && (dfuSizeHeader > sizeof(DFSFILEHEADER32)))
  {
      DfsRead(DfsFileInternal->DfsFileWrap,
            ((dfbytep) &DfsFileHeader) + sizeof(DFSFILEHEADER32), dfuSizeHeader - sizeof(DFSFILEHEADER32),
            &error,pei);

      if (error != 0)
        dfError = DFS_ERROR_ERRORIO;
  }


  if (dfError == DFS_SUCCESS)
  {
    if (pdfContentEncodedSize != NULL)
      *pdfContentEncodedSize =
        ConvertDualuLongIntelTouLong64(DfsFileHeader.dfContentEncodedSizeLow,DfsFileHeader.dfContentEncodedSizeHigh);

    if (pdfContentStoreMethod != NULL)
      *pdfContentStoreMethod =
        ConvertuLongIntelToLong(DfsFileHeader.dfContentStoreMethod);
    if (pdfContentUncompressedSize != NULL)
      *pdfContentUncompressedSize =
        ConvertDualuLongIntelTouLong64(DfsFileHeader.dfContentUncompressedSizeLow,DfsFileHeader.dfContentUncompressedSizeHigh);

    dfTagListSize = ConvertuLongIntelToLong(DfsFileHeader.dfTagListSize);
    ptrDirTag = DfsMalloc(dfTagListSize + 1);
    if (ptrDirTag == NULL)
      dfError = DFS_ERROR_MEMORY_ERROR;
  }

  if (dfError == DFS_SUCCESS)
  {
    DfsRead(DfsFileInternal->DfsFileWrap, ptrDirTag, dfTagListSize, &error, pei);
    if (error != 0)
      dfError = DFS_ERROR_ERRORIO;

    if ((TagListFileCopy != NULL) && (dfError == DFS_SUCCESS))
      *TagListFileCopy = ReadTagList(ptrDirTag, &dfTagListSize);
  }

  if (ptrDirTag != NULL)
    DfsFree(ptrDirTag);

  if (dfError == DFS_SUCCESS)
    DfsFileInternal->dfCurState = CURSTATE_RD_READINGFILECONTENT;

  return dfError;
}

dfuLong32 DfsRReadFileEncoded(DFSFILE
                            DfsFile,
                            void *Buf, dfuLong32 Size, dfuLong32 * pReadSize,
                            H_ERROR_INFO * pei)
{
  dfuLong32 dfError = DFS_SUCCESS;
  dfuLong32 error = 0;
  dfuLong32 dfReadSize;
  DFSFILEINTERNAL *DfsFileInternal = (DFSFILEINTERNAL *) DfsFile;
  if (DfsFileInternal == NULL)
    return DFS_ERROR_BAD_PARAMETER;
  if (DfsFileInternal->dfCurState != CURSTATE_RD_READINGFILECONTENT)
    return DFS_ERROR_BAD_PARAMETER;
  dfReadSize = DfsRead(DfsFileInternal->DfsFileWrap, Buf, Size, &error, pei);
  if (error != 0)
    dfError = DFS_ERROR_ERRORIO;
  else if (pReadSize != NULL)
    *pReadSize = dfReadSize;

  DfsFileInternal->dfEncodedSizeExecuted += dfReadSize;
  DfsFileInternal->dfEncodedCrc32 =
    (dfuLong32)crc32(DfsFileInternal->dfEncodedCrc32, (dfbytep)Buf, dfReadSize);
  return dfError;
}

dfuLong32 DfsRReadGetPostFile(DFSFILE DfsFile,
                            dfuLong64 * pdfFileContentUncompressedSize,
                            dfuLong32 * pCrcInfoNumber,
                            DFSCRCINFOPARAM ** ppDfsCrcInfoParam, H_ERROR_INFO * pei)
{
  dfuLong32 dfError = DFS_SUCCESS;
  dfuLong32 error = 0;
  DFSFILEINTERNAL *DfsFileInternal = (DFSFILEINTERNAL *) DfsFile;
  DFSPOSTFILEINFO DfsPostFileInfo;
  dfuLong32 dfCrcInfoNumber = 0;
  DFSCRCINFO_FULLSIZESTRUCTURE *pDfsCrcInfo = NULL;
  if (DfsFileInternal == NULL)
    return DFS_ERROR_BAD_PARAMETER;
  if ((DfsFileInternal->dfCurState != CURSTATE_RD_BEFOREREAD_BLOCK_POSTFILE)
      && (DfsFileInternal->dfCurState != CURSTATE_RD_READINGFILECONTENT))
    return DFS_ERROR_BAD_PARAMETER;
/** TODO **/

  DfsRead(DfsFileInternal->DfsFileWrap, &DfsPostFileInfo,
          sizeof(DfsPostFileInfo), &error, pei);
  if (error != 0)
    dfError = DFS_ERROR_ERRORIO;

  if (dfError == DFS_SUCCESS)
  {

    dfCrcInfoNumber =
      ConvertuLongIntelToLong(DfsPostFileInfo.dfNumberCrc32Info);
    pDfsCrcInfo =
      (DFSCRCINFO_FULLSIZESTRUCTURE *) DfsMalloc((dfCrcInfoNumber + 1) * sizeof(DFSCRCINFO_FULLSIZESTRUCTURE));
    if (pDfsCrcInfo == NULL)
      dfError = DFS_ERROR_MEMORY_ERROR;
  }

  if ((dfError == DFS_SUCCESS) && (pdfFileContentUncompressedSize != NULL))
    *pdfFileContentUncompressedSize =
      ConvertDualuLongIntelTouLong64(DfsPostFileInfo.dfFileContentUncompressedSizeLow,
                                     DfsPostFileInfo.dfFileContentUncompressedSizeHigh);


  if (dfError == DFS_SUCCESS)
    if (DfsFileInternal->dfEncodedCrc32 !=
        ConvertuLongIntelToLong(DfsPostFileInfo.dfFileContentCompressedCrc32))
      dfError = DFS_ERROR_ERRORIO;

  if (dfError == DFS_SUCCESS)
  {
    dfuLong32 dfSizeCrcInfoItem=0;
    dfuLong32 dfSizeCrcInfoToGetSize=0;
    if (dfCrcInfoNumber > 0)
    {
      dfuLong32 dfSizeMinimal = dfCrcInfoNumber * sizeof(DFSCRCINFOONLYCRC32_32);

      dfuLong32 dfSizeCrcInfo=0 ;

      DfsRead(DfsFileInternal->DfsFileWrap, pDfsCrcInfo,
              dfSizeMinimal , &error, pei);

      if (error == 0)
          dfSizeCrcInfoToGetSize=GetCrcInfoSizeNeededToGetSize(pDfsCrcInfo);

      if (dfSizeCrcInfoToGetSize>sizeof(DFSCRCINFOONLYCRC32_32))
      {
          DfsRead(DfsFileInternal->DfsFileWrap, ((dfbytep)pDfsCrcInfo)+dfSizeMinimal,
              dfSizeCrcInfoToGetSize-dfSizeMinimal , &error, pei);
          if (error == 0)
              dfSizeMinimal=dfSizeCrcInfoToGetSize;
      }

      if (error==0)
        dfSizeCrcInfoItem=GetCrcInfoSize(pDfsCrcInfo);

      //dfuLong32 GetCrcInfoSizeNeededToGetSize(const DFSCRCINFO_FULLSIZESTRUCTURE * pDfsCrcInfo);



      if (dfSizeCrcInfoItem>sizeof(DFSCRCINFOONLYCRC32_32))
      {
          dfSizeCrcInfo = dfSizeCrcInfoItem*dfCrcInfoNumber;
          if (error == 0)
          DfsRead(DfsFileInternal->DfsFileWrap, ((dfbytep)pDfsCrcInfo)+dfSizeMinimal,
              dfSizeCrcInfo-dfSizeMinimal , &error, pei);
      }
      else
          dfSizeCrcInfo=dfSizeMinimal;
    }
    if (error != 0)
      dfError = DFS_ERROR_ERRORIO;

    if ((dfError == DFS_SUCCESS) && (pCrcInfoNumber != NULL))
      *pCrcInfoNumber = dfCrcInfoNumber;

    if ((dfError == DFS_SUCCESS) && (ppDfsCrcInfoParam != NULL))
    {
      dfuLong32 i;
      DFSCRCINFOPARAM *pDfsCrcInfoParam;
      pDfsCrcInfoParam =
        (DFSCRCINFOPARAM *) DfsMalloc((dfCrcInfoNumber + 1) *
                                      sizeof(DFSCRCINFOPARAM));
      *ppDfsCrcInfoParam = pDfsCrcInfoParam;

      for (i = 0; i < dfCrcInfoNumber; i++)
      {
        const DFSCRCINFO_FULLSIZESTRUCTURE* pCurDfsCrcInfo=(const DFSCRCINFO_FULLSIZESTRUCTURE*)(((dfbytep)pDfsCrcInfo)+(dfSizeCrcInfoItem*i));
        ConvertDfsCrcInfoToDfsCrcInfoParam(pCurDfsCrcInfo,
                                           pDfsCrcInfoParam + i);
      }
    }

  }
  if (pDfsCrcInfo != NULL)
    DfsFree(pDfsCrcInfo);

  if (dfError == DFS_SUCCESS)
    DfsFileInternal->dfCurState = CURSTATE_RD_BEFOREREAD_BLOCKTYPE;
  return dfError;
}

/*
dfuLong32 DfsRCloseFile(DFSFILE DfsFile, const void *Buf, dfuLong32 Size,
                      dfuLong32 * pReadSize)
{
  dfuLong32 dfError = DFS_SUCCESS;
  dfuLong32 error = 0;

  DFSFILEINTERNAL *DfsFileInternal = (DFSFILEINTERNAL *) DfsFile;
  if (DfsFileInternal == NULL)
    return DFS_ERROR_BAD_PARAMETER;

  if (DfsFileInternal->dfCurState != CURSTATE_RD_BEFOREREAD_BLOCK_POSTFILE)
    return DFS_ERROR_BAD_PARAMETER;


  return DFS_ERROR_BAD_PARAMETER;
}
*/
dfuLong32 DfsRGetNextDirIntro(DFSFILE DfsFile,
                            dfuLong32 * pdfTypedir,
                            DFTAGLIST * pTagListDirIntroCopy, H_ERROR_INFO * pei)
{
  dfuLong32 dfError = DFS_SUCCESS;
  dfuLong32 error = 0;
  DFSFILEINTERNAL *DfsFileInternal = (DFSFILEINTERNAL *) DfsFile;
  DFSDIRINTROHEADER DfsDirIntroHeader;
  dfvoidp ptrDirTag = NULL;
  dfuLong32 dfTagListSize = 0;

  if (DfsFileInternal == NULL)
    return DFS_ERROR_BAD_PARAMETER;

  if (DfsFileInternal->dfCurState != CURSTATE_RD_BEFOREREAD_BLOCK_DIRINTRO)
    return DFS_ERROR_BAD_PARAMETER;

  DfsFileInternal->dfEncodedSizeExecuted = 0;
  DfsFileInternal->dfEncodedCrc32 = 0;

  DfsRReadNextHeaderBlockAfterBlockType(DfsFile, &DfsDirIntroHeader,
                                        sizeof(DfsDirIntroHeader), &error, pei);
  if (error != 0)
    dfError = DFS_ERROR_ERRORIO;

  if (dfError == DFS_SUCCESS)
  {
    dfTagListSize = ConvertuLongIntelToLong(DfsDirIntroHeader.dfTagSize);
    ptrDirTag = DfsMalloc(dfTagListSize + 1);
    if (ptrDirTag == NULL)
      dfError = DFS_ERROR_MEMORY_ERROR;
  }

  if (dfError == DFS_SUCCESS)
  {
    if (pdfTypedir != NULL)
      *pdfTypedir = ConvertuLongIntelToLong(DfsDirIntroHeader.dfTypeDir);
    DfsRead(DfsFileInternal->DfsFileWrap, ptrDirTag, dfTagListSize, &error, pei);
    if (error != 0)
      dfError = DFS_ERROR_ERRORIO;

    if ((pTagListDirIntroCopy != NULL) && (dfError == DFS_SUCCESS))
      *pTagListDirIntroCopy = ReadTagList(ptrDirTag, &dfTagListSize);
  }

  if (dfError == DFS_SUCCESS)
    DfsFileInternal->dfCurState = CURSTATE_RD_BEFOREREAD_BLOCKTYPE;
  if (ptrDirTag != NULL)
    DfsFree(ptrDirTag);
  return dfError;
}

dfuLong32 DfsRGetNextDir(DFSFILE DfsFile,
                       dfuLong32 * pdfTypedir,
                       DFTAGLIST * pTagListDirCopy,
                       dfuLong32 * pdfGetNbFile,
                       DFTAGLIST ** ppsTagListFileInDirCopy, H_ERROR_INFO * pei)
{
  dfuLong32 dfError = DFS_SUCCESS;
  dfuLong32 error = 0;
  DFSFILEINTERNAL *DfsFileInternal = (DFSFILEINTERNAL *) DfsFile;
  DFSDIRHEADER DfsDirHeader;
  dfvoidp ptrTagArray = NULL;
  dfvoidp ptrTagArrayCompressed = NULL;
  dfuLong32 dfSizeDirTag = 0;
  dfuLong32 dfSizeFilesTag = 0;
  dfuLong32 dfSizeAllTags = 0;
  dfuLong32 dfSizeAllTagsCompressed = 0;
  dfuLong32 dfNbFileInDir = 0;
  dfuLong32 dfStoreMethod = 0;


  if (DfsFileInternal == NULL)
    return DFS_ERROR_BAD_PARAMETER;

  if (DfsFileInternal->dfCurState != CURSTATE_RD_BEFOREREAD_BLOCK_DIR)
    return DFS_ERROR_BAD_PARAMETER;

  DfsFileInternal->dfEncodedSizeExecuted = 0;
  DfsFileInternal->dfEncodedCrc32 = 0;

  DfsRReadNextHeaderBlockAfterBlockType(DfsFile, &DfsDirHeader,
                                        sizeof(DfsDirHeader), &error, pei);
  if (error != 0)
    dfError = DFS_ERROR_ERRORIO;

  if (dfError == DFS_SUCCESS)
  {
    if (pdfTypedir != NULL)
      *pdfTypedir = ConvertuLongIntelToLong(DfsDirHeader.dfTypeDir);
    dfNbFileInDir = ConvertuLongIntelToLong(DfsDirHeader.dfNumberFilesInDir);

    if (pdfGetNbFile != NULL)
      *pdfGetNbFile = dfNbFileInDir;

    dfSizeDirTag = ConvertuLongIntelToLong(DfsDirHeader.dfSizeDirTag);
    dfSizeFilesTag = ConvertuLongIntelToLong(DfsDirHeader.dfSizeFilesTag);
    dfSizeAllTagsCompressed = ConvertuLongIntelToLong(DfsDirHeader.dfSizeCompressed);
    ptrTagArrayCompressed = DfsMalloc(dfSizeAllTagsCompressed + 0x10);
    dfSizeAllTags = dfSizeDirTag + dfSizeFilesTag;
    dfStoreMethod = ConvertuLongIntelToLong(DfsDirHeader.dfStoreMethod);

    ptrTagArray = DfsMalloc(dfSizeAllTags + 1);
    if (ptrTagArray == NULL)
      dfError = DFS_ERROR_MEMORY_ERROR;
  }

  /*WE MUST UNCOMPRESS HERE */
  if (dfError == DFS_SUCCESS)
  {
    dfuLong32 dfTagListSize = 0;
    dfuLong32 dfCrc32;
    DfsRead(DfsFileInternal->DfsFileWrap, ptrTagArrayCompressed, dfSizeAllTagsCompressed, &error, pei);
    if (error != 0)
      dfError = DFS_ERROR_ERRORIO;

    if ((dfStoreMethod == DFSMETHOD_STORE) && (dfSizeAllTagsCompressed==dfSizeAllTags))
    {
        DfsMemcpy(ptrTagArray,ptrTagArrayCompressed,dfSizeAllTags);
    }
    else
    if (dfStoreMethod == DFSMETHOD_DEFLATE)
    {
          z_stream zstr;
          DfsClearStruct(&zstr, 0, sizeof(z_stream));
          //if (inflateInit(&zstr) != Z_OK)
          if (inflateInit2(&zstr, -MAX_WBITS) != Z_OK)
              dfError = DFS_ERROR_MEMORY_ERROR;
          else
          {
              int err;
              zstr.next_out=(dfbytep)ptrTagArray;
              zstr.avail_out=dfSizeAllTags;
              zstr.next_in=(dfbytep)ptrTagArrayCompressed;
              zstr.avail_in=dfSizeAllTagsCompressed;
              err = inflate(&zstr,Z_SYNC_FLUSH);
              if (! (((err==Z_STREAM_END) || (err==Z_OK)) && (zstr.total_out == dfSizeAllTags)))
                  dfError = DFS_ERROR_ERRORIO;
              inflateEnd(&zstr);
          }

    }
    else
        dfError = DFS_ERROR_ERRORIO;

    DfsFree(ptrTagArrayCompressed);
    dfCrc32 = (dfuLong32)crc32(0,(dfbytep)ptrTagArray,dfSizeAllTags);
    if (dfError == DFS_SUCCESS)
        if (dfCrc32 != ConvertuLongIntelToLong(DfsDirHeader.dfCrc32Uncompressed))
            dfError = DFS_ERROR_ERRORIO;

    if (((pTagListDirCopy != NULL) || (ppsTagListFileInDirCopy != NULL))
        && (dfError == DFS_SUCCESS))
    {
      DFTAGLIST TagListDirCopy;
      dfuLong32 i;
      TagListDirCopy =
        ReadTagListSizeLimited(ptrTagArray, &dfTagListSize, dfSizeAllTags);
      if (pTagListDirCopy != NULL)
        *pTagListDirCopy = TagListDirCopy;
      else
        CloseTagList(TagListDirCopy);

      if ((ppsTagListFileInDirCopy != NULL) && (TagListDirCopy != NULL))
      {
        DFTAGLIST *dfTagListArray;
        dfuLong32 dfPosInTagArray = dfTagListSize;

        dfTagListArray =
          (DFTAGLIST *) DfsMalloc(sizeof(DFTAGLIST *) * (dfNbFileInDir + 1));
        *ppsTagListFileInDirCopy = dfTagListArray;

        for (i = 0; i < dfNbFileInDir + 1; i++)
          *(dfTagListArray + i) = NULL;

        for (i = 0;
             (i < dfNbFileInDir) && (dfPosInTagArray < dfSizeAllTags); i++)
        {
          dfuLong32 dfThisTagSize = 0;

          *(dfTagListArray + i) =
            ReadTagListSizeLimited(((dfbytep) ptrTagArray) +
                                   dfPosInTagArray, &dfThisTagSize,
                                   dfSizeAllTags - dfPosInTagArray);
          dfPosInTagArray += dfThisTagSize;
        }
      }

    }
  }
  if (dfError == DFS_SUCCESS)
    DfsFileInternal->dfCurState = CURSTATE_RD_BEFOREREAD_BLOCKTYPE;
  if (ptrTagArray != NULL)
    DfsFree(ptrTagArray);
  return dfError;
}

dfuLong32 DfsRFreeAllDirTag(DFTAGLIST TagListDirCopy,
                          DFTAGLIST * psTagListFileInDirCopy,H_ERROR_INFO * pei)
{
  dfuLong32 dfError = DFS_SUCCESS;


  if (TagListDirCopy != NULL)
    CloseTagList(TagListDirCopy);

  if (psTagListFileInDirCopy != NULL)
  {
    DFTAGLIST *pBrowseTagListArray = psTagListFileInDirCopy;
    while ((*pBrowseTagListArray) != NULL)
    {
      CloseTagList(*pBrowseTagListArray);
      pBrowseTagListArray++;
    }
    DfsFree((dfvoidp) psTagListFileInDirCopy);
  }
  return dfError;
}

/****************************************/
/* a Dir contain a pointer to a file    */
/* function not compatible with stream  */
/****************************************/

dfuLong32 DfsGotoFilePointer(DFSFILE DfsFile, dfuLong32 dfPointerLow, dfuLong32 dfPointerHigh, H_ERROR_INFO * pei)
{
  dfuLong32 dfError = DFS_SUCCESS;
  //dfuLong32 error = 0;

  DFSFILEINTERNAL *DfsFileInternal = (DFSFILEINTERNAL *) DfsFile;
  if (DfsFileInternal == NULL)
    return DFS_ERROR_BAD_PARAMETER;

  if ((DfsFileInternal->dfCurState != CURSTATE_RD_BEFOREREAD_BLOCKTYPE) &&
      (DfsFileInternal->dfCurState != CURSTATE_NODIROPENED_ATENDOFDATA))
    return DFS_ERROR_BAD_PARAMETER;

  if (dfError == DFS_SUCCESS)
    DfsSeekMulAlign(DfsFileInternal->DfsFileWrap, dfPointerLow, dfPointerHigh, pei);
  return dfError;
}

dfuLong32 DfsGotoDir(DFSFILE DfsFile, dfuLong32 dfNumDir, H_ERROR_INFO * pei)
{
  dfuLong32 dfError = DFS_SUCCESS;
 //dfuLong32 error = 0;
  DFSDIRINFO64 *pDfsCurDirInfo;

  DFSFILEINTERNAL *DfsFileInternal = (DFSFILEINTERNAL *) DfsFile;
  if (DfsFileInternal == NULL)
    return DFS_ERROR_BAD_PARAMETER;

  if ((DfsFileInternal->dfCurState != CURSTATE_RD_BEFOREREAD_BLOCKTYPE) &&
      (DfsFileInternal->dfCurState != CURSTATE_NODIROPENED_ATENDOFDATA))
    return DFS_ERROR_BAD_PARAMETER;

  if (dfNumDir > DfsFileInternal->dfNumberOfDir)
    return DFS_ERROR_BAD_PARAMETER;

  pDfsCurDirInfo =
    (DFSDIRINFO64 *) (((dfbytep) DfsFileInternal->DfsDirListHead) +
                    sizeof(DFSLISTDIRHEADHEADER64) + (dfNumDir * sizeof(DFSDIRINFO64)));

  if (dfError == DFS_SUCCESS)
  {
    DfsSeekMulAlign(DfsFileInternal->DfsFileWrap,
            ConvertuLongIntelToLong(pDfsCurDirInfo->dfOffsetDirLow),
            ConvertuLongIntelToLong(pDfsCurDirInfo->dfOffsetDirHigh), pei);
    DfsFileInternal->dfCurState = CURSTATE_RD_BEFOREREAD_BLOCKTYPE;
  }
  return dfError;
}


dfuLong32 SVFAPI DfsGetDirType(DFSFILE DfsFile, dfuLong32 dfNumDir, dfuLong32 * pdfDirType, H_ERROR_INFO * pei)
{
  dfuLong32 dfError = DFS_SUCCESS;
  //dfuLong32 error = 0;
  DFSDIRINFO64 *pDfsCurDirInfo;

  DFSFILEINTERNAL *DfsFileInternal = (DFSFILEINTERNAL *) DfsFile;
  if (DfsFileInternal == NULL)
    return DFS_ERROR_BAD_PARAMETER;

  if (DfsFileInternal->dfCurState != CURSTATE_RD_BEFOREREAD_BLOCKTYPE)
    return DFS_ERROR_BAD_PARAMETER;

  if (dfNumDir > DfsFileInternal->dfNumberOfDir)
    return DFS_ERROR_BAD_PARAMETER;

  pDfsCurDirInfo =
    (DFSDIRINFO64 *) (((dfbytep) DfsFileInternal->DfsDirListHead) +
                    sizeof(DFSLISTDIRHEADHEADER64) + (dfNumDir * sizeof(DFSDIRINFO64)));
  if ((dfError == DFS_SUCCESS) && (pdfDirType!=NULL))
      *pdfDirType = ConvertuLongIntelToLong(pDfsCurDirInfo->dfTypeDir);
  return dfError;
}


dfuLong32 DfsGotoDirIntro(DFSFILE DfsFile, dfuLong32 dfNumDir, H_ERROR_INFO * pei)
{
  dfuLong32 dfError = DFS_SUCCESS;
  //dfuLong32 error = 0;
  DFSDIRINFO64 *pDfsCurDirInfo;

  DFSFILEINTERNAL *DfsFileInternal = (DFSFILEINTERNAL *) DfsFile;
  if (DfsFileInternal == NULL)
    return DFS_ERROR_BAD_PARAMETER;

  if ((DfsFileInternal->dfCurState != CURSTATE_RD_BEFOREREAD_BLOCKTYPE) &&
      (DfsFileInternal->dfCurState != CURSTATE_NODIROPENED_ATENDOFDATA))
    return DFS_ERROR_BAD_PARAMETER;

  if (dfNumDir > DfsFileInternal->dfNumberOfDir)
    return DFS_ERROR_BAD_PARAMETER;

  pDfsCurDirInfo =
    (DFSDIRINFO64 *) (((dfbytep) DfsFileInternal->DfsDirListHead) +
                    sizeof(DFSLISTDIRHEADHEADER64) + (dfNumDir * sizeof(DFSDIRINFO64)));
  if (dfError == DFS_SUCCESS)
    DfsSeekMulAlign(DfsFileInternal->DfsFileWrap,
            ConvertuLongIntelToLong(pDfsCurDirInfo->dfOffsetDirIntroLow),
            ConvertuLongIntelToLong(pDfsCurDirInfo->dfOffsetDirIntroHigh),pei);
  return dfError;
}

dfuLong32 SVFAPI DfsGetNbDir(DFSFILE DfsFile, dfuLong32 * pdfNbDir, H_ERROR_INFO * pei)
{
  DFSFILEINTERNAL *DfsFileInternal = (DFSFILEINTERNAL *) DfsFile;
  if (DfsFileInternal == NULL)
    return DFS_ERROR_BAD_PARAMETER;

  if (pdfNbDir != NULL)
    *pdfNbDir = DfsFileInternal->dfNumberOfDir;
  return DFS_SUCCESS;
}

dfuLong32 DfsGetCurrentPointer(DFSFILE DfsFile, dfuLong32 * pdfGetCurPtrLow,dfuLong32 * pdfGetCurPtrHigh, H_ERROR_INFO * pei)   /* useful? */
{
  dfuLong32 dfError = DFS_SUCCESS;
  dfuLong32 error = 0;

  DFSFILEINTERNAL *DfsFileInternal = (DFSFILEINTERNAL *) DfsFile;
  if (DfsFileInternal == NULL)
    return DFS_ERROR_BAD_PARAMETER;

  if ((pdfGetCurPtrLow != NULL) && (pdfGetCurPtrHigh != NULL))
  {
    DfsTellDivAlign(DfsFileInternal->DfsFileWrap, pdfGetCurPtrLow, pdfGetCurPtrHigh, &error, pei);
    if (error != 0)
      dfError = DFS_ERROR_ERRORIO;
  }

  return DFS_SUCCESS;
}

DFTAGBLOCKFLOAT SVFAPI SVFAPI GetDfsTagBlockFloat(DFSFILE DfsFile, H_ERROR_INFO * pei)
{
  //dfuLong32 dfError = DFS_SUCCESS;
  //dfuLong32 error = 0;

  DFSFILEINTERNAL *DfsFileInternal = (DFSFILEINTERNAL *) DfsFile;
  if (DfsFileInternal == NULL)
    return NULL;
  return DfsFileInternal ->dfTagBlockFloat;
}

BOOL SVFAPI SetDfsTagBlockFloatDirty(DFSFILE DfsFile, H_ERROR_INFO * pei)
{
  BOOL fPreviousDirty;
  //dfuLong32 dfError = DFS_SUCCESS;
  //dfuLong32 error = 0;

  DFSFILEINTERNAL *DfsFileInternal = (DFSFILEINTERNAL *) DfsFile;
  if (DfsFileInternal == NULL)
    return FALSE;

  if ((DfsFileInternal->dfStatus & DFS_WRITABLE) == 0)
  {
    return FALSE;
  }
  fPreviousDirty = DfsFileInternal->fModifyingDoneNoFlushed ;

  DfsFileInternal->fModifyingDone = TRUE;
  DfsFileInternal->fModifyingDoneNoFlushed = TRUE;

  return fPreviousDirty;
}

/************************************************************************/
// todo : write a summary about dir properties before file

// status : CURSTATE_NODIROPENED_ATENDOFDATA -> CURSTATE_NODIROPENED + A end of written data

// readstate : after each read function

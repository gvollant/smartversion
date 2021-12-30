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
#include "../common/DfsTagDf.h"
#include "../common/DfsTagMg.h"
#include "../common/DfsTagBlockFloatEnd.h"
#include "../common/DfsTagBlockFloatEndStruct.h"

#include "../common/DfsIntf.h"
#include "../common/DfsStruc.h"

#include "../../svfile/common/DfsMtStr.h" // to remove soon
#include "../../../../SvfVersion.h"

#include "../common/DfsIntfInternal.h"
#include "../common/DfsIntfWr.h"
#include "../compress/DfsTagMgWr.h"
#include "../compress/DfsTagBlockFloatEndWr.h"

static dfuLong32 DfsWriteAndEraseTag(DFSFILEINTERNAL * DfsFileInternal,
                                   dfvoidp * pptrDirTag,
                                   dfuLong32 * psizeDirTag, H_ERROR_INFO * pei)
{
  dfuLong32 dfError = DFS_SUCCESS;
  dfuLong32 error = 0;

  if (dfError == DFS_SUCCESS)
  {
    if (dfError == DFS_SUCCESS)
      DfsWrite(DfsFileInternal->DfsFileWrap, *pptrDirTag, *psizeDirTag,
               &error,pei);
    if (error != 0)
      dfError = DFS_ERROR_ERRORIO;
    FreeTagPtr(*pptrDirTag);
    *pptrDirTag = NULL;
    *psizeDirTag = 0;
  }

  return dfError;
}

static dfuLong32 DfsWriteDirIntroduction(DFSFILEINTERNAL * DfsFileInternal, H_ERROR_INFO * pei)
{
  dfuLong32 dfError = DFS_SUCCESS;
  dfuLong32 error = 0;

  if (DfsFileInternal->dfCurState !=
      CURSTATE_WR_AFTERCREATEDIR_BEFOREWRITEINTRO)
    return DFS_ERROR_BAD_PARAMETER;


  if (dfError == DFS_SUCCESS)
  {
    DFSDIRINTROHEADER DfsDirIntroHeader;
    dfvoidp ptrDirTag = NULL;
    dfuLong32 sizeDirTag = 0;
    if (!EndBuildTagList
        (DfsFileInternal->dfTagListCurDirInfoIntro, &ptrDirTag, &sizeDirTag,
         TRUE))
      dfError = DFS_ERROR_MEMORY_ERROR;

    if (dfError == DFS_SUCCESS)
    {
      DfsFileInternal->dfTagListCurDirInfoIntro = NULL;
      DfsDirIntroHeader.dfBlocType =
        ConvertuLongToLongIntel(BLOCKTYPE_DIRINTRO);
      DfsDirIntroHeader.dfTypeDir =
        ConvertuLongToLongIntel(DfsFileInternal->dfTypeCurDir);
      DfsDirIntroHeader.dfNumberOfnDir =
        ConvertuLongToLongIntel(DfsFileInternal->dfNumberOfDir);
      DfsDirIntroHeader.dfTagSize = ConvertuLongToLongIntel(sizeDirTag);

      if (dfError == DFS_SUCCESS)
        DfsReadOrWriteFillAlign(DfsFileInternal->DfsFileWrap, TRUE, &error,pei);
      if (error == 0)
        DfsTellDivAlign(DfsFileInternal->DfsFileWrap, &DfsFileInternal->dfDirIntroPosLow, &DfsFileInternal->dfDirIntroPosHigh, &error,pei);

      if (error != 0)
        dfError = DFS_ERROR_ERRORIO;

      if (dfError == DFS_SUCCESS)
        DfsWrite(DfsFileInternal->DfsFileWrap, &DfsDirIntroHeader,
                 sizeof(DFSDIRINTROHEADER), &error, pei);
      if (error != 0)
        dfError = DFS_ERROR_ERRORIO;

      if (dfError == DFS_SUCCESS)

        dfError =
          DfsWriteAndEraseTag(DfsFileInternal, &ptrDirTag, &sizeDirTag, pei);
    }
  }


  DfsFileInternal->dfCurState = CURSTATE_WR_AFTERCREATEDIR_AFTERWRITEINTRO;
  return dfError;
}

dfuLong32 DfsWCloseFlushCurrentDir(DFSFILE DfsFile, H_ERROR_INFO * pei)
{
  dfuLong32 dfError = DFS_SUCCESS;
  dfuLong32 dfBlockDirSize = 0;
  dfuLong32 error=0;
  DFSFILEINTERNAL *DfsFileInternal = (DFSFILEINTERNAL *) DfsFile;
  if (DfsFileInternal == NULL)
    return DFS_ERROR_BAD_PARAMETER;

  if (DfsFileInternal->dfCurState ==
      CURSTATE_WR_AFTERCREATEDIR_BEFOREWRITEINTRO)
  {
    dfError = DfsWriteDirIntroduction(DfsFileInternal, pei);
  }

  if (DfsFileInternal->dfCurState !=
      CURSTATE_WR_AFTERCREATEDIR_AFTERWRITEINTRO)
    return DFS_ERROR_BAD_PARAMETER;

  if ((DfsFileInternal->dfStatus & DFS_WRITABLE) == 0)
  {
    dfError = DFS_ERROR_BAD_PARAMETER;
    return dfError;
  }

  //if (DfsFileInternal->CurDirFileTagInBuildingSize != 0)
  if (DfsFileInternal->dfNumberFilesInDir != VALUE_UNKNOWN)
  {
    DFSDIRHEADER DfsDirHeader;
    dfvoidp ptrDirTag;
    dfuLong32 sizeDirTag;
    dfuLong32 error = 0;
    dfuLong32 crc32v;
    DFSDIRINFO64 *pDfsCurDirInfo;
    dfuLong32 dfWriteUncompressedDataSize;
    dfuLong32 dfBufWriteSize;
    dfuLong32 dfWriteSize=0;
    dfbytep dfBufWrite;
    BOOL fCompress=TRUE;

    if (!EndBuildTagList
        (DfsFileInternal->dfTagListCurDirInfo, &ptrDirTag, &sizeDirTag,
         FALSE))
      dfError = DFS_ERROR_MEMORY_ERROR;
    DfsFileInternal->dfTagListCurDirInfo = NULL;

    dfWriteUncompressedDataSize=sizeDirTag +
                              DfsFileInternal->CurDirFileTagInBuildingSize;
    dfBufWriteSize = dfWriteUncompressedDataSize + (dfWriteUncompressedDataSize/0x10)+0x100;
    dfBufWrite = (dfbytep)DfsMalloc(dfBufWriteSize);

    DfsDirHeader.dfBlocType = ConvertuLongToLongIntel(BLOCKTYPE_DIR);
    DfsDirHeader.dfTypeDir =
      ConvertuLongToLongIntel(DfsFileInternal->dfTypeCurDir);

    crc32v = crc32(0, (dfbytep)ptrDirTag, sizeDirTag);
    crc32v =
      crc32(crc32v, (dfbytep)DfsFileInternal->CurDirFileTagInBuildingPtr,
            DfsFileInternal->CurDirFileTagInBuildingSize);

    DfsDirHeader.dfCrc32Uncompressed = ConvertuLongToLongIntel(crc32v);

    DfsDirHeader.dfNumberOfnDir =
      ConvertuLongToLongIntel(DfsFileInternal->dfNumberOfDir);
    DfsDirHeader.dfNumberFilesInDir =
      ConvertuLongToLongIntel(DfsFileInternal->dfNumberFilesInDir);

    DfsDirHeader.dfSizeDirTag = ConvertuLongToLongIntel(sizeDirTag);
    DfsDirHeader.dfSizeFilesTag =
      ConvertuLongToLongIntel(DfsFileInternal->CurDirFileTagInBuildingSize);

    DfsFileInternal->dfNumberOfDir++;

    if (fCompress)
    {
          z_stream zstr;
          int err;
          BOOL fSuccess=TRUE;
          DfsClearStruct(&zstr, 0, sizeof(z_stream));
          //deflateInit(&zstr,Z_DEFAULT_COMPRESSION);
          deflateInit2(&zstr, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, DEF_MEM_LEVEL, 0);
          zstr.avail_out = dfBufWriteSize;
          zstr.next_out = dfBufWrite;
          zstr.next_in =  (dfbytep)ptrDirTag;
          zstr.avail_in = sizeDirTag;
          err = deflate(&zstr,Z_NO_FLUSH);
          if ((err != Z_OK) || (zstr.avail_in !=0))
          {
              fSuccess=FALSE;
          }
          else
          {
            zstr.avail_in =  DfsFileInternal->CurDirFileTagInBuildingSize;
            zstr.next_in = (dfbytep)DfsFileInternal->CurDirFileTagInBuildingPtr;
            if (deflate(&zstr, Z_FINISH) != Z_STREAM_END)
                fSuccess=FALSE;
          }
          dfWriteSize = zstr.total_out;
          deflateEnd(&zstr);
          DfsDirHeader.dfStoreMethod = ConvertuLongToLongIntel(DFSMETHOD_DEFLATE);
          if (!fSuccess)
              fCompress=FALSE;
    }


    if ((!fCompress) || (dfWriteSize > sizeDirTag+DfsFileInternal->CurDirFileTagInBuildingSize))
    {
        DfsMemcpy(dfBufWrite,ptrDirTag,sizeDirTag);
        DfsMemcpy(dfBufWrite+sizeDirTag,DfsFileInternal->CurDirFileTagInBuildingPtr,
                  DfsFileInternal->CurDirFileTagInBuildingSize);
        dfWriteSize = sizeDirTag+DfsFileInternal->CurDirFileTagInBuildingSize;
        DfsDirHeader.dfStoreMethod = ConvertuLongToLongIntel(DFSMETHOD_STORE);
    }
    DfsDirHeader.dfSizeCompressed = ConvertuLongToLongIntel(dfWriteSize) ;

    if (dfError == DFS_SUCCESS)
      DfsReadOrWriteFillAlign(DfsFileInternal->DfsFileWrap, TRUE, &error, pei);

    if (error == 0)
      DfsTellDivAlign(DfsFileInternal->DfsFileWrap, &DfsFileInternal->dfDirPosLow, &DfsFileInternal->dfDirPosHigh, &error, pei);
    if (error != 0)
      dfError = DFS_ERROR_ERRORIO;

    /* Write the DFSDIRHEADER header */
    if (dfError == DFS_SUCCESS)
      DfsWrite(DfsFileInternal->DfsFileWrap, &DfsDirHeader,
               sizeof(DFSDIRHEADER), &error, pei);
    if (error != 0)
      dfError = DFS_ERROR_ERRORIO;
    else
      dfBlockDirSize += sizeof(DFSDIRHEADER);

    /* WE MUST COMPRESS HERE */
    /* Write the Tags (general property of the directory) */
    if (dfError == DFS_SUCCESS)
      DfsWrite(DfsFileInternal->DfsFileWrap, dfBufWrite, dfWriteSize, &error, pei);
    if (error != 0)
      dfError = DFS_ERROR_ERRORIO;
    else
      dfBlockDirSize += dfWriteSize;

    /* Write the All the Tags Set */
    /*
    if (dfError == DFS_SUCCESS)
      DfsWrite(DfsFileInternal->DfsFileWrap,
               DfsFileInternal->CurDirFileTagInBuildingPtr,
               DfsFileInternal->CurDirFileTagInBuildingSize, &error);
    if (error != 0)
      dfError = DFS_ERROR_ERRORIO;
    else
      dfBlockDirSize += DfsFileInternal->CurDirFileTagInBuildingSize;
      */
    DfsFree(dfBufWrite);


    FreeTagPtr(ptrDirTag);

    if (dfError == DFS_SUCCESS)
    {

      DfsFileInternal->CurDfsDirListNbDir++;
      DfsFileInternal->CurDfsDirListHeadSize += sizeof(DFSDIRINFO64);
      if (!DfsCheckAllocatedMemory((dfvoidp*)&DfsFileInternal->DfsDirListHead,
                                   &DfsFileInternal->
                                   CurDfsDirListHeadAllocated,
                                   DfsFileInternal->CurDfsDirListHeadSize,
                                   DfsFileInternal->
                                   CurDfsDirListHeadStepAlloc))
        dfError = DFS_ERROR_MEMORY_ERROR;
    }
    if (dfError == DFS_SUCCESS)
    {
      pDfsCurDirInfo =
        (DFSDIRINFO64 *) (((dfbytep) DfsFileInternal->DfsDirListHead) +
                        sizeof(DFSLISTDIRHEADHEADER64) +
                        ((DfsFileInternal->CurDfsDirListNbDir -
                          1) * sizeof(DFSDIRINFO64)));
      pDfsCurDirInfo->dfOffsetDirIntroLow =
        ConvertuLongToLongIntel(DfsFileInternal->dfDirIntroPosLow);
      pDfsCurDirInfo->dfOffsetDirIntroHigh =
        ConvertuLongToLongIntel(DfsFileInternal->dfDirIntroPosHigh);
      pDfsCurDirInfo->dfOffsetDirLow =
        ConvertuLongToLongIntel(DfsFileInternal->dfDirPosLow);
      pDfsCurDirInfo->dfOffsetDirHigh =
        ConvertuLongToLongIntel(DfsFileInternal->dfDirPosHigh);
      pDfsCurDirInfo->dfTypeDir =
        ConvertuLongToLongIntel(DfsFileInternal->dfTypeCurDir);
      //pDfsCurDirInfo->dfNbDir= ConvertuLongToLongIntel(DfsFileInternal->dfNumberOfDir);
      pDfsCurDirInfo->dfBlockDirSize =
        ConvertuLongToLongIntel(dfBlockDirSize);
    }

    DfsFileInternal->dfCurState = CURSTATE_NODIROPENED_ATENDOFDATA;
    DfsTellDivAlign(DfsFileInternal->DfsFileWrap,
                    &DfsFileInternal->dfPosOfLastUnfloatDataWhenAtEndOfDataLow,
                    &DfsFileInternal->dfPosOfLastUnfloatDataWhenAtEndOfDataHigh,
                    &error, pei);
  }


  ClearDirInWorkMember(DfsFileInternal, TRUE);

  {
    dfuLong32 error = 0;
    if (dfError == DFS_SUCCESS)
      DfsReadOrWriteFillAlign(DfsFileInternal->DfsFileWrap, TRUE, &error, pei);

    if (error == 0)
      DfsTellDivAlign(DfsFileInternal->DfsFileWrap,
                      &DfsFileInternal->dfPosBlockDataEndLow,
                      &DfsFileInternal->dfPosBlockDataEndHigh,
                      &error, pei);
    if (error != 0)
      dfError = DFS_ERROR_ERRORIO;
  }

  DfsFileInternal->dfCurState = CURSTATE_NODIROPENED_ATENDOFDATA;
  DfsTellDivAlign(DfsFileInternal->DfsFileWrap,
                  &DfsFileInternal->dfPosOfLastUnfloatDataWhenAtEndOfDataLow,
                  &DfsFileInternal->dfPosOfLastUnfloatDataWhenAtEndOfDataHigh,
                  &error, pei);

  return dfError;
}

dfuLong32 DfsWCreateNewDir(DFSFILE DfsFile, dfuLong32 dfTypeDir, H_ERROR_INFO * pei)
{
  dfuLong32 dfError = DFS_SUCCESS;
  dfuLong32 error=0;
  DFSFILEINTERNAL *DfsFileInternal = (DFSFILEINTERNAL *) DfsFile;

  if (DfsFileInternal == NULL)
    return DFS_ERROR_BAD_PARAMETER;

  if (DfsFileInternal->dfCurState == CURSTATE_RD_BEFOREREAD_BLOCKTYPE)
  {
    dfError=DfsSeekMulAlign(DfsFileInternal->DfsFileWrap,
                            DfsFileInternal->dfPosBlockDataEndLow,
                            DfsFileInternal->dfPosBlockDataEndHigh,
                            pei);
    if (dfError != DFS_SUCCESS)
        return dfError;
    DfsFileInternal->dfCurState = CURSTATE_NODIROPENED_ATENDOFDATA;
    DfsTellDivAlign(DfsFileInternal->DfsFileWrap,
                    &DfsFileInternal->dfPosOfLastUnfloatDataWhenAtEndOfDataLow,
                    &DfsFileInternal->dfPosOfLastUnfloatDataWhenAtEndOfDataHigh,
                    &error,
                    pei);
  }

  if (DfsFileInternal->dfCurState != CURSTATE_NODIROPENED_ATENDOFDATA)
    return DFS_ERROR_BAD_PARAMETER;

  if ((DfsFileInternal->dfStatus & DFS_WRITABLE) == 0)
  {
    dfError = DFS_ERROR_BAD_PARAMETER;
    return dfError;
  }

  if (DfsFileInternal->CurDirFileTagInBuildingSize != 0)
  {
    dfError = DfsWCloseFlushCurrentDir(DfsFile, pei);
    if (dfError != DFS_SUCCESS)
      return dfError;
  }
/*
  if (dfError == DFS_SUCCESS)
    {
      DfsFileInternal->dfTagListCurDirInfo = AllocNewTagList();
      if (DfsFileInternal->dfTagListCurDirInfo == NULL)
        dfError = DFS_ERROR_MEMORY_ERROR;
    }
*/
  if (dfError == DFS_SUCCESS)
  {
    DfsFileInternal->dfCurState = CURSTATE_WR_AFTERCREATEDIR_BEFOREWRITEINTRO;

    DfsFileInternal->dfNumberFilesInDir = 0;
    DfsFileInternal->dfTypeCurDir = dfTypeDir;
    DfsFileInternal->dfTagListCurDirInfo = AllocNewTagList();
    DfsFileInternal->dfTagListCurDirInfoIntro = AllocNewTagList();

    DfsFileInternal->fModifyingDone = TRUE;
    DfsFileInternal->fModifyingDoneNoFlushed = TRUE;
  }
  return dfError;
}



dfuLong32 DfsWCreateNewFileInDir(DFSFILE DfsFile, dfuLong32 dfContentStoreMethod,
                                 dfuLong64 dfContentUncompressedSize, H_ERROR_INFO * pei)
{
  DFSFILEINTERNAL *DfsFileInternal = (DFSFILEINTERNAL *) DfsFile;
  dfuLong32 dfError = DFS_SUCCESS;
  if (DfsFileInternal == NULL)
    return DFS_ERROR_BAD_PARAMETER;


  if (DfsFileInternal->dfCurState ==
      CURSTATE_WR_AFTERCREATEDIR_BEFOREWRITEINTRO)
  {
    dfError = DfsWriteDirIntroduction(DfsFileInternal, pei);
  }

  // If a dir is not opened, error
  if (DfsFileInternal->dfCurState !=
      CURSTATE_WR_AFTERCREATEDIR_AFTERWRITEINTRO)
    dfError = DFS_ERROR_BAD_PARAMETER;


  if (dfError == DFS_SUCCESS)
  {
    DfsFileInternal->dfTagListCurFileInFinalDir = AllocNewTagList();
    if (DfsFileInternal->dfTagListCurFileInFinalDir == NULL)
      dfError = DFS_ERROR_MEMORY_ERROR;
  }

  if (dfError == DFS_SUCCESS)
  {
    DfsFileInternal->dfTagListCurFileInPreFileInfoDir = AllocNewTagList();
    if (DfsFileInternal->dfTagListCurFileInPreFileInfoDir == NULL)
    {
      dfError = DFS_ERROR_MEMORY_ERROR;
      EndBuildTagList(DfsFileInternal->dfTagListCurFileInFinalDir, NULL,
                      NULL, FALSE);
    }
  }

  if (dfError == DFS_SUCCESS)
  {
    DfsFileInternal->dfCurState = CURSTATE_WR_AFTERCREATEFILE;

    DfsFileInternal->dfsFileHeader.dfsFileHeaderSize =
      ConvertuLongToLongIntel(sizeof(DFSFILEHEADER64));

    if (dfContentUncompressedSize < (0xff000000UL))
        DfsFileInternal->dfsFileHeader.dfsFileHeaderSize =
          ConvertuLongToLongIntel(sizeof(DFSFILEHEADER32));


    ConvertuLong64ToDualuLongIntel(dfContentUncompressedSize,
                                   &DfsFileInternal->dfsFileHeader.dfContentUncompressedSizeLow,
                                   &DfsFileInternal->dfsFileHeader.dfContentUncompressedSizeHigh);

    DfsFileInternal->dfsFileHeader.dfContentStoreMethod =
      ConvertuLongToLongIntel(DFSMETHOD_STORE);

    DfsFileInternal->dfsFileHeader.dfContentEncodedSizeLow =
    DfsFileInternal->dfsFileHeader.dfContentEncodedSizeHigh =
      ConvertuLongToLongIntel(VALUE_UNKNOWN);

    DfsFileInternal->dfContentUncompressedSizeDeclared =
      dfContentUncompressedSize;
    DfsFileInternal->dfEncodedSizeExecuted = 0;
    DfsFileInternal->dfEncodedCrc32 = 0;

    DfsFileInternal->dfFileEncodedSizeWithPreAndPostHeader = 0;
    DfsFileInternal->dfFileEncodedSizeWithoutPreAndPostHeader = 0;
    DfsFileInternal->dfsFilePosProperties.dfContentStoreMethod =
      (dfContentStoreMethod);
  }

  /* todo : ask in parameter pCrcTableInterval and store the CRC Interval (optional, if NULL : CRC in one block all file */
  return dfError;
}

dfuLong32 DfsWAddTagInNewFileInDir(DFSFILE DfsFile, dfuLong32 TagNumber,
                                 dfvoidpc TagBuf, dfuLong32 TagSize,
                                 BOOL fStoreTagBeforeFile,
                                 BOOL fStoreTagInDirectory,H_ERROR_INFO * pei)
{
  DFSFILEINTERNAL *DfsFileInternal = (DFSFILEINTERNAL *) DfsFile;
  dfuLong32 dfError = DFS_SUCCESS;
  if (DfsFileInternal == NULL)
    return DFS_ERROR_BAD_PARAMETER;


  if (!((DfsFileInternal->dfCurState == CURSTATE_WR_AFTERCREATEFILE) ||
        ((DfsFileInternal->dfCurState == CURSTATE_WRITINGFILECONTENT)
         && (!fStoreTagBeforeFile))))
  {
    dfError = DFS_ERROR_BAD_PARAMETER;
  }

  if ((dfError == DFS_SUCCESS) && fStoreTagInDirectory)
  {
    if (!AddTag
        (DfsFileInternal->dfTagListCurFileInFinalDir, TagNumber, TagBuf,
         TagSize))
      dfError = DFS_ERROR_MEMORY_ERROR;
  }

  if ((dfError == DFS_SUCCESS) && fStoreTagBeforeFile)
  {
    if (!AddTag
        (DfsFileInternal->dfTagListCurFileInPreFileInfoDir, TagNumber,
         TagBuf, TagSize))
      dfError = DFS_ERROR_MEMORY_ERROR;
  }
  return dfError;
}

dfuLong32 DfsWAddTagInDir(DFSFILE DfsFile, dfuLong32 TagNumber,
                        dfvoidpc TagBuf, dfuLong32 TagSize,
                        BOOL fWriteInDirIntroduction)
{
  DFSFILEINTERNAL *DfsFileInternal = (DFSFILEINTERNAL *) DfsFile;
  dfuLong32 dfError = DFS_SUCCESS;

  if ((fWriteInDirIntroduction)
      && (DfsFileInternal->dfCurState !=
          CURSTATE_WR_AFTERCREATEDIR_BEFOREWRITEINTRO))
    dfError = DFS_ERROR_BAD_PARAMETER;

  if ((DfsFileInternal->dfCurState !=
       CURSTATE_WR_AFTERCREATEDIR_BEFOREWRITEINTRO)
      && (DfsFileInternal->dfCurState !=
          CURSTATE_WR_AFTERCREATEDIR_AFTERWRITEINTRO)
      && (DfsFileInternal->dfCurState != CURSTATE_WR_AFTERCREATEFILE)
      && (DfsFileInternal->dfCurState != CURSTATE_WRITINGFILECONTENT))
    dfError = DFS_ERROR_BAD_PARAMETER;

  if (dfError == DFS_SUCCESS)
    if (!AddTag
        (DfsFileInternal->dfTagListCurDirInfo, TagNumber, TagBuf, TagSize))
      dfError = DFS_ERROR_MEMORY_ERROR;

  if ((dfError == DFS_SUCCESS) && (fWriteInDirIntroduction))
    if (!AddTag
        (DfsFileInternal->dfTagListCurDirInfoIntro, TagNumber, TagBuf,
         TagSize))
      dfError = DFS_ERROR_MEMORY_ERROR;

  return dfError;
}

dfuLong32 DfsWQuickCloseWriteFileForStripped(DFSFILE DfsFile)
{
  DFSFILEINTERNAL *DfsFileInternal = (DFSFILEINTERNAL *) DfsFile;
  if (DfsFileInternal->dfTagListCurFileInPreFileInfoDir != NULL)
    CloseTagList(DfsFileInternal->dfTagListCurFileInPreFileInfoDir);
  DfsFileInternal->dfTagListCurFileInPreFileInfoDir = NULL;

  return DFS_SUCCESS;
}

dfuLong32 DfsWWriteFileEncoded(DFSFILE DfsFile, const void *Buf, dfuLong32 Size, H_ERROR_INFO * pei)
{
  DFSFILEINTERNAL *DfsFileInternal = (DFSFILEINTERNAL *) DfsFile;
  dfuLong32 dfError = DFS_SUCCESS;
  if (DfsFileInternal == NULL)
    return DFS_ERROR_BAD_PARAMETER;

  if ((DfsFileInternal->dfCurState != CURSTATE_WRITINGFILECONTENT)
      && (DfsFileInternal->dfCurState != CURSTATE_WR_AFTERCREATEFILE))
    return DFS_ERROR_BAD_PARAMETER;

  if (DfsFileInternal->dfCurState == CURSTATE_WR_AFTERCREATEFILE)
  {
    dfuLong32 error = 0;
    dfvoidp ptrDirTag = NULL;
    dfuLong32 sizeDirTag = 0;


    DfsFileInternal->dfCurState = CURSTATE_WRITINGFILECONTENT;
    /* todo : writing the DFSFILEHEADER and the DFSTAGLIST
       (EndBuildTagList on dfTagListCurFileInPreFileInfoDir) */

    if (!EndBuildTagList
        (DfsFileInternal->dfTagListCurFileInPreFileInfoDir, &ptrDirTag,
         &sizeDirTag, TRUE))
      dfError = DFS_ERROR_MEMORY_ERROR;

    DfsFileInternal->dfTagListCurFileInPreFileInfoDir=NULL;

    if (dfError == DFS_SUCCESS)
      DfsReadOrWriteFillAlign(DfsFileInternal->DfsFileWrap, TRUE, &error, pei);

    if (error != 0)
      dfError = DFS_ERROR_ERRORIO;

    if (dfError == DFS_SUCCESS)
      DfsTellDivAlign(DfsFileInternal->DfsFileWrap,
                      &DfsFileInternal->dfFilePosLow,
                      &DfsFileInternal->dfFilePosHigh,
                      &error, pei);

    if (dfError == DFS_SUCCESS)
    {
      dfuLong32 dfsFilePosPropertiesLow,dfsFilePosPropertiesHigh;
      DfsTellDivAlign(DfsFileInternal->DfsFileWrap, &dfsFilePosPropertiesLow, &dfsFilePosPropertiesHigh, &error, pei);
      DfsFileInternal->dfsFilePosProperties.dfFilePos = dfsFilePosPropertiesLow | (((dfuLong64)dfsFilePosPropertiesHigh)<<32);

      if (error != 0)
        dfError = DFS_ERROR_ERRORIO;
      DfsFileInternal->dfsFileHeader.dfBlocType =
        ConvertuLongToLongIntel(BLOCKTYPE_FILE);
      DfsFileInternal->dfsFileHeader.dfTagListSize =
        ConvertuLongToLongIntel(sizeDirTag);

      DfsTellDivAlign(DfsFileInternal->DfsFileWrap,
                      &DfsFileInternal->DfsFileHeaderPosLow,
                      &DfsFileInternal->DfsFileHeaderPosHigh,
                      &error, pei);

      DfsWrite(DfsFileInternal->DfsFileWrap,
               &DfsFileInternal->dfsFileHeader,
               ConvertuLongIntelToLong(DfsFileInternal->dfsFileHeader.dfsFileHeaderSize), &error, pei);
      if (error != 0)
        dfError = DFS_ERROR_ERRORIO;
      else
        DfsFileInternal->dfFileEncodedSizeWithPreAndPostHeader +=
          ConvertuLongIntelToLong(DfsFileInternal->dfsFileHeader.dfsFileHeaderSize);
    }

    if (dfError == DFS_SUCCESS)
    {
      if (dfError == DFS_SUCCESS)
      {
        DfsWrite(DfsFileInternal->DfsFileWrap, ptrDirTag, sizeDirTag, &error, pei);
        if (error != 0)
          dfError = DFS_ERROR_ERRORIO;
        else
          DfsFileInternal->dfFileEncodedSizeWithPreAndPostHeader +=
            sizeDirTag;
      }
    }
    if (ptrDirTag != NULL)
      FreeTagPtr(ptrDirTag);
  }

  if ((dfError == DFS_SUCCESS) && (Size > 0))
  {
    dfuLong32 error = 0;
    dfuLong32 dfWrite =
      DfsWrite(DfsFileInternal->DfsFileWrap, Buf, Size, &error, pei);

    DfsFileInternal->dfEncodedSizeExecuted += dfWrite;
    DfsFileInternal->dfEncodedCrc32 =
      crc32(DfsFileInternal->dfEncodedCrc32, (dfbytep)Buf, dfWrite);

    if (error != 0)
      dfError = DFS_ERROR_ERRORIO;
    else
    {
      DfsFileInternal->dfFileEncodedSizeWithPreAndPostHeader += dfWrite;
      DfsFileInternal->dfFileEncodedSizeWithoutPreAndPostHeader += dfWrite;
    }
  }

  return dfError;
}

dfuLong32 DfsWCloseFileInDir(DFSFILE DfsFile,
                           dfuLong64 dfFileContentUncompressedSize,
                           dfuLong32 CrcInfoNumber,
                           const DFSCRCINFOPARAM * pDfsCrcInfoParam,
                           const DFSPATCHANALYSEINFO_MEMORY* pDfsPatchAnalyseInfo,
                           BOOL fEraseContent, H_ERROR_INFO * pei)
{
  dfuLong32 dfError = DFS_SUCCESS;
  DFSFILEINTERNAL *DfsFileInternal = (DFSFILEINTERNAL *) DfsFile;
  if (DfsFileInternal == NULL)
    return DFS_ERROR_BAD_PARAMETER;

  if ((fEraseContent) && (DfsFileInternal->dfCurState == CURSTATE_WRITINGFILECONTENT))
  {
    if ((DfsSeekMulAlign
                (DfsFileInternal->DfsFileWrap,
                DfsFileInternal->DfsFileHeaderPosLow,
                DfsFileInternal->DfsFileHeaderPosHigh,pei)) != DFS_SUCCESS)
      fEraseContent=FALSE;
    else
      DfsFileInternal->dfCurState = CURSTATE_WR_AFTERCREATEFILE;
  }

  if (fEraseContent)
    if (DfsFileInternal->dfCurState != CURSTATE_WR_AFTERCREATEFILE)
        fEraseContent=FALSE;

  if (fEraseContent)
  {
      DfsFileInternal->dfEncodedSizeExecuted = 0;
      DfsFileInternal->dfEncodedCrc32 = 0;
      DfsWQuickCloseWriteFileForStripped(DfsFile);
  }
  else
  {
    if (DfsFileInternal->dfCurState == CURSTATE_WR_AFTERCREATEFILE)
        dfError = DfsWWriteFileEncoded(DfsFile, NULL, 0,pei);

    if ((dfError == DFS_SUCCESS)
        && (DfsFileInternal->dfCurState != CURSTATE_WRITINGFILECONTENT))
        dfError = DFS_ERROR_BAD_PARAMETER;
  }

  if (DfsFileInternal->dfContentUncompressedSizeDeclared == VALUE_UNKNOWN)
    DfsFileInternal->dfContentUncompressedSizeDeclared =
      dfFileContentUncompressedSize;

  if (dfFileContentUncompressedSize !=
      DfsFileInternal->dfContentUncompressedSizeDeclared)
    dfError = DFS_ERROR_BAD_PARAMETER;

  ConvertuLong64ToDualuLongIntel(DfsFileInternal->dfEncodedSizeExecuted,
                                 &DfsFileInternal->dfsFileHeader.dfContentEncodedSizeLow,
                                 &DfsFileInternal->dfsFileHeader.dfContentEncodedSizeHigh);

  /* GO BACK, ONLY IF NOT STREAM */
  if (!fEraseContent)
  {
    dfuLong32 error = 0;
    dfuLong32 dfCurPosLow, dfCurPosHigh ;
    DfsTell(DfsFileInternal->DfsFileWrap, &dfCurPosLow, &dfCurPosHigh, &error, pei);
    if (DfsSeekMulAlign
        (DfsFileInternal->DfsFileWrap,
         DfsFileInternal->DfsFileHeaderPosLow,
         DfsFileInternal->DfsFileHeaderPosHigh, pei) != DFS_ERROR_BAD_PARAMETER)
    {
      dfuLong32 error = 0;
      DfsWrite(DfsFileInternal->DfsFileWrap,
               &DfsFileInternal->dfsFileHeader,
               ConvertuLongIntelToLong(DfsFileInternal->dfsFileHeader.dfsFileHeaderSize), &error, pei);
      if (error != 0)
        dfError = DFS_ERROR_ERRORIO;
      DfsSeek(DfsFileInternal->DfsFileWrap, dfCurPosLow,dfCurPosHigh,pei);
    }
  }

  DfsFileInternal->dfsFileHeader.dfContentEncodedSizeLow =
  DfsFileInternal->dfsFileHeader.dfContentEncodedSizeHigh =
    ConvertuLongToLongIntel(VALUE_UNKNOWN);

  if (dfError == DFS_SUCCESS)
  {
    DFSPOSTFILEINFO DfsFilePostInfo;
    DFSCRCINFO_FULLSIZESTRUCTURE *pDfsCrcInfo;
    dfuLong32 dfAllocCrcInfo=(sizeof(DFSCRCINFO_FULLSIZESTRUCTURE) * (CrcInfoNumber + 1));
    pDfsCrcInfo = (DFSCRCINFO_FULLSIZESTRUCTURE *) DfsMalloc(dfAllocCrcInfo);
    if (pDfsCrcInfo == NULL)
      dfError = DFS_ERROR_MEMORY_ERROR;
    if (dfError == DFS_SUCCESS)
    {
      dfuLong32 error = 0;
      dfuLong32 i;
      dfuLong32 dfPosInCrcInfo=0;

      DfsClearStruct(pDfsCrcInfo,0,dfAllocCrcInfo);

      ConvertuLong64ToDualuLongIntel(DfsFileInternal->dfContentUncompressedSizeDeclared,
                            &DfsFilePostInfo.dfFileContentUncompressedSizeLow,
                            &DfsFilePostInfo.dfFileContentUncompressedSizeHigh);
      DfsFilePostInfo.dfFileContentCompressedCrc32 =
        ConvertuLongToLongIntel(DfsFileInternal->dfEncodedCrc32);
      DfsFilePostInfo.dfNumberCrc32Info =
        ConvertuLongToLongIntel(CrcInfoNumber);

      for (i = 0; i < CrcInfoNumber; i++)
      {
        DFSCRCINFO_FULLSIZESTRUCTURE* pCurCrcInfo=(DFSCRCINFO_FULLSIZESTRUCTURE*)(((char*)pDfsCrcInfo)+dfPosInCrcInfo);
        ConvertDfsCrcInfoParamToDfsCrcInfo(pDfsCrcInfoParam + i,
                                           pCurCrcInfo);
        dfPosInCrcInfo += GetCrcInfoSize((const DFSCRCINFO_FULLSIZESTRUCTURE*)pCurCrcInfo);
      }

      if (!fEraseContent)
        DfsWrite(DfsFileInternal->DfsFileWrap, &DfsFilePostInfo,
                sizeof(DfsFilePostInfo), &error, pei);
      if (error != 0)
        dfError = DFS_ERROR_ERRORIO;
      else
        DfsFileInternal->dfFileEncodedSizeWithPreAndPostHeader +=
          sizeof(DfsFilePostInfo);
      if ((dfError == DFS_SUCCESS) && (!fEraseContent))
        DfsWrite(DfsFileInternal->DfsFileWrap, pDfsCrcInfo,
                 dfPosInCrcInfo, &error, pei);
      if (error != 0)
        dfError = DFS_ERROR_ERRORIO;
      else
        DfsFileInternal->dfFileEncodedSizeWithPreAndPostHeader +=
          dfPosInCrcInfo;

      DfsFileInternal->dfsFilePosProperties.
        dfFileSizeContentUncompressed =
        (DfsFileInternal->
                                dfContentUncompressedSizeDeclared);


      DfsFileInternal->dfsFilePosProperties.
        dfFileEncodedSizeWithoutPreAndPostHeader =
        (DfsFileInternal->
                                dfFileEncodedSizeWithoutPreAndPostHeader);
      DfsFileInternal->dfsFilePosProperties.
        dfFileEncodedSizeWithPreAndPostHeader =
        (DfsFileInternal->
                                dfFileEncodedSizeWithPreAndPostHeader);

      if (fEraseContent)
      {
          DfsFileInternal->dfsFilePosProperties.dfFilePos = (0);
          DfsFileInternal->dfsFilePosProperties.dfFilePos = (0);
          DfsFileInternal->dfsFilePosProperties.dfFileEncodedSizeWithoutPreAndPostHeader = (0);
          DfsFileInternal->dfsFilePosProperties.dfFileEncodedSizeWithPreAndPostHeader = (0);
      }

      if (dfError == DFS_SUCCESS)
        if (!AddTag
            (DfsFileInternal->dfTagListCurFileInFinalDir, DFSTAG_CRCINFO,
             pDfsCrcInfo, dfPosInCrcInfo))
          dfError = DFS_ERROR_MEMORY_ERROR;

      if (dfError == DFS_SUCCESS)
      {
          DFSFILEPOSPROPERTIES6464 DfsFilePosProperties;
          dfuLong32 dfSizeTag ;
          dfSizeTag = ConvertDfsFilePropertiesToTag(&DfsFileInternal->dfsFilePosProperties,
                                &DfsFilePosProperties,sizeof(DfsFilePosProperties));

          if (!AddTag
                (DfsFileInternal->dfTagListCurFileInFinalDir,
                 DFSTAG_FILEPOSPROPERTIES,
                 &DfsFilePosProperties,
                 dfSizeTag))
            dfError = DFS_ERROR_MEMORY_ERROR;
      }
      DfsFree(pDfsCrcInfo);
    }
  }

  if (dfError == DFS_SUCCESS)
  {
    DfsFileInternal->dfCurState = CURSTATE_WR_AFTERCREATEDIR_AFTERWRITEINTRO;

    /* Now we will add the tag about the file we closed in the Tag array of current dir */
    /* to do : just before, add 1 or 2 tag about size, position... */
    {
      dfvoidp ptrDirTag;
      dfuLong32 sizeDirTag;
      if (!EndBuildTagList
          (DfsFileInternal->dfTagListCurFileInFinalDir, &ptrDirTag,
           &sizeDirTag, FALSE))
        dfError = DFS_ERROR_MEMORY_ERROR;
      DfsFileInternal->dfTagListCurFileInFinalDir = NULL;

      DfsFileInternal->dfNumberFilesInDir++;

      if (dfError == DFS_SUCCESS)
        if (!
            (DfsCheckAllocatedMemory
             (&DfsFileInternal->CurDirFileTagInBuildingPtr,
              &DfsFileInternal->CurDirFileTagInBuildingAllocated,
              DfsFileInternal->CurDirFileTagInBuildingSize + sizeDirTag,
              DfsFileInternal->CurDirStepAlloc)))
          dfError = DFS_ERROR_MEMORY_ERROR;
      if (dfError == DFS_SUCCESS)
      {
        DfsMemcpy(((dfbytep) DfsFileInternal->
                   CurDirFileTagInBuildingPtr) +
                  DfsFileInternal->CurDirFileTagInBuildingSize,
                  ptrDirTag, sizeDirTag);
        DfsFileInternal->CurDirFileTagInBuildingSize += sizeDirTag;
      }

      FreeTagPtr(ptrDirTag);
    }
        /***/
  }

  return dfError;
}

dfuLong32 DfsWCloseDir(DFSFILE DfsFile, H_ERROR_INFO * pei)
{
  /* near DfsWCloseFlushCurrentDir ... */
  /*
     dfuLong32 dfError = DFS_SUCCESS;
     DFSFILEINTERNAL* DfsFileInternal = (DFSFILEINTERNAL*)DfsFile;

     if (dfError==DFS_SUCCESS)
     {
     DfsFileInternal->dfCurState = CURSTATE_NODIROPENED_ATENDOFDATA;
     }

     return dfError; */
  return DfsWCloseFlushCurrentDir(DfsFile, pei);
}

dfuLong32 SVFAPI DfsFlushWriteDfsFile(DFSFILE DfsFile, H_ERROR_INFO * pei)
{
  /* todo: write the final directory table, (DfsDirListHead) cleanup */
  dfuLong32 dfFlush;
  dfuLong32 dfError = DFS_SUCCESS;
  dfuLong32 error = 0;

  DFSFILEINTERNAL *DfsFileInternal = (DFSFILEINTERNAL *) DfsFile;
  if (DfsFileInternal == NULL)
    return DFS_ERROR_BAD_PARAMETER;

  if (DfsFileInternal->dfCurState == CURSTATE_RD_BEFOREREAD_BLOCKTYPE)
  {
    dfError=DfsSeekMulAlign(DfsFileInternal->DfsFileWrap,
                            DfsFileInternal->dfPosBlockDataEndLow,
                            DfsFileInternal->dfPosBlockDataEndHigh, pei);
    if (dfError != DFS_SUCCESS)
        return dfError;
    DfsFileInternal->dfCurState = CURSTATE_NODIROPENED_ATENDOFDATA;
    DfsTellDivAlign(DfsFileInternal->DfsFileWrap,
                    &DfsFileInternal->dfPosOfLastUnfloatDataWhenAtEndOfDataLow,
                    &DfsFileInternal->dfPosOfLastUnfloatDataWhenAtEndOfDataHigh,
                    &error,pei);
  }

  if ((DfsFileInternal->dfCurState != CURSTATE_NODIROPENED_ATENDOFDATA)
      && (DfsFileInternal->fModifyingDoneNoFlushed))
    return DFS_ERROR_BAD_PARAMETER;

  if (DfsFileInternal->fModifyingDoneNoFlushed)
  {
      dfuLong32 dfPosTagFloatLow = 0;
      dfuLong32 dfPosTagFloatHigh = 0;

      if (dfError == DFS_SUCCESS)
      {
          if ((DfsFileInternal->dfPosOfLastUnfloatDataWhenAtEndOfDataLow!=INVALID_POSITION) ||
              (DfsFileInternal->dfPosOfLastUnfloatDataWhenAtEndOfDataHigh!=INVALID_POSITION))
            DfsSeekMulAlign(DfsFileInternal->DfsFileWrap,
                            DfsFileInternal->dfPosOfLastUnfloatDataWhenAtEndOfDataLow,
                            DfsFileInternal->dfPosOfLastUnfloatDataWhenAtEndOfDataHigh,pei);

          DfsReadOrWriteFillAlign(DfsFileInternal->DfsFileWrap, TRUE, &error,pei);
          DfsTellDivAlign(DfsFileInternal->DfsFileWrap,
                          &DfsFileInternal->dfPosOfLastUnfloatDataWhenAtEndOfDataLow,
                          &DfsFileInternal->dfPosOfLastUnfloatDataWhenAtEndOfDataHigh,
                          &error,pei);
      }

      if ((dfError == DFS_SUCCESS) && (DfsFileInternal->dfTagBlockFloat != NULL))
      {
          dfvoidp ptrTagFloat = NULL;
          dfuLong32 dfuSizeTagFlostStruct = 0;


          if (FlushTagBlockFloat(DfsFileInternal->dfTagBlockFloat, &ptrTagFloat, &dfuSizeTagFlostStruct,TRUE))
          {
              DFSENDFILETAGHEADER DfsEndFileTagHeader;
              dfuLong32 error=0;
              DfsTellDivAlign(DfsFileInternal->DfsFileWrap, &dfPosTagFloatLow, &dfPosTagFloatHigh, &error,pei);
              if (error != 0)
                dfError = DFS_ERROR_ERRORIO;

              DfsEndFileTagHeader.dfBlocType= ConvertuLongToLongIntel(BLOCKTYPE_ENDFILETAGANDINFO);
              DfsEndFileTagHeader.dfTotalSizeEndFileInfo =
                       ConvertuLongToLongIntel(sizeof(DfsEndFileTagHeader)+dfuSizeTagFlostStruct);
              DfsEndFileTagHeader.dfTotalSizeCompressedFloatTag =
                       ConvertuLongToLongIntel(dfuSizeTagFlostStruct);
              DfsEndFileTagHeader.dfReserved = ConvertuLongToLongIntel(0);

              DfsWrite(DfsFileInternal->DfsFileWrap, &DfsEndFileTagHeader,
                            sizeof(DfsEndFileTagHeader), &error,pei);
              if (error != 0)
                dfError = DFS_ERROR_ERRORIO;

              DfsWrite(DfsFileInternal->DfsFileWrap, ptrTagFloat,
                            dfuSizeTagFlostStruct, &error,pei);
              if (error != 0)
                dfError = DFS_ERROR_ERRORIO;

          }
          FreeTagPtr(ptrTagFloat);
      }

    DfsFileInternal->DfsDirListHead->DfsListDirHeadHeader.dfBlocType =
      ConvertuLongToLongIntel(BLOCKTYPE_LISTDIR64);
    DfsFileInternal->DfsDirListHead->DfsListDirHeadHeader.dfNbDir =
      ConvertuLongToLongIntel(DfsFileInternal->dfNumberOfDir);
    DfsFileInternal->DfsDirListHead->DfsListDirHeadHeader.dfTypeDfsFile =
      ConvertuLongToLongIntel(0);
    DfsFileInternal->DfsDirListHead->DfsListDirHeadHeader.dfOffsetPostFileTagLow = ConvertuLongToLongIntel(dfPosTagFloatLow);
    DfsFileInternal->DfsDirListHead->DfsListDirHeadHeader.dfOffsetPostFileTagHigh = ConvertuLongToLongIntel(dfPosTagFloatHigh);

    if (dfError == DFS_SUCCESS)
      DfsReadOrWriteFillAlign(DfsFileInternal->DfsFileWrap, TRUE, &error,pei);
    if (error != 0)
      dfError = DFS_ERROR_ERRORIO;

    if (dfError == DFS_SUCCESS)
      DfsWrite(DfsFileInternal->DfsFileWrap,
               DfsFileInternal->DfsDirListHead,
               DfsFileInternal->CurDfsDirListHeadSize, &error,pei);
    if (error != 0)
      dfError = DFS_ERROR_ERRORIO;

    if (dfError == DFS_SUCCESS)
    {
      DFSLISTDIREND DfsListDirEnd;
      DfsListDirEnd.dfBlockSize =
        ConvertuLongToLongIntel(DfsFileInternal->CurDfsDirListHeadSize);
      DfsListDirEnd.dfBlockCrc32 =
        ConvertuLongToLongIntel(crc32
                                (0,
                                 (dfbytep) DfsFileInternal->
                                 DfsDirListHead,
                                 DfsFileInternal->CurDfsDirListHeadSize));
      DfsWrite(DfsFileInternal->DfsFileWrap, &DfsListDirEnd,
               sizeof(DFSLISTDIREND), &error, pei);
      if (error != 0)
        dfError = DFS_ERROR_ERRORIO;
      DfsMarkEndNow(DfsFileInternal->DfsFileWrap,&error,pei);
      if (error != 0)
        dfError = DFS_ERROR_ERRORIO;
      DfsFileInternal->dfCurState = CURSTATE_NODIROPENED_ATENDOFDATA;
      if ((DfsFileInternal->dfPosOfLastUnfloatDataWhenAtEndOfDataLow!=INVALID_POSITION) ||
          (DfsFileInternal->dfPosOfLastUnfloatDataWhenAtEndOfDataHigh!=INVALID_POSITION))
        DfsSeekMulAlign(DfsFileInternal->DfsFileWrap,
                        DfsFileInternal->dfPosOfLastUnfloatDataWhenAtEndOfDataLow,
                        DfsFileInternal->dfPosOfLastUnfloatDataWhenAtEndOfDataHigh,
                        pei);

      DfsFileInternal->fModifyingDoneNoFlushed=FALSE;
    }
  }

  dfFlush = DfsFlushWriteFile(DfsFileInternal->DfsFileWrap,pei);

  if (dfError == DFS_SUCCESS)
      dfError = dfFlush;


  return dfError;
}

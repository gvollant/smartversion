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
#include "../../patchstream/common/DfsIoHlp.h"
#include "../common/DfsSet.h"
#include "../common/DfsSetTl.h"

#include "../../patchstream/rebuild/RamDifWk.h"
#include "../../patchstream/rebuild/RamDifTl.h"
#include "../../patchstream/rebuild/RamDifWS.h"

#include "DfsWrSet.h"

#include "../../patchstream/common/difstrm.h"
#include "../../patchstream/compress/makdifst.h"

#include "../../patchstream/compress/abstractCompress.h"
#include "zlib.h"
#include "../../patchstream/common/compress_store.h"

#include "../../../hash/svf_md5.h"
#include "../../../hash/svf_sha.h"
#include "../../../hash/svf_sha256.h"


#define SIZE_BUFFER (0x8000*1*4)

dfuLong32 SVFAPI InsertDirectoryinDfsFile(DFSFILE DfsFile, dfuLong32 dfTypeDir,
                                 dfuLong32 dfNbFile, FILETOADD * pFileToAdd,
                                 BOOL fWriteCrcAndSizeInFileToAddArray,
                                 dfwcharpc dfwVersionName,
                                 dfwcharpc dfwVersionComment,
                                 const COMPRESSIONPARAM* pCprParam,
                                 tProgressCallBack pProgressCallBack,
                                 dfvoidp dfUserPtr,
                                 H_ERROR_INFO* pei)
{
  dfuLong32 dfFlush;
  dfuLong32 dfError = DFS_SUCCESS;
  dfuLong32 dfError2 = DFS_SUCCESS;
  dfuLong32 dfRetCall;
  dfuLong32 i ;
  FILETOADD *pCurFileToAdd;
  dfbytep BufAlloc;
  dfbytep BufRead, BufWrite;
  dfuLong32 dfBufReadSize, dfBufWriteSize;
  dfuLong32 dfBufWriteAvailOut=0;
  PROGRESSCALLBACKINFO ProgressCallBackInfo;
  BOOL fTagInfoBeforeFile=FALSE;
  dfuLong32 dfNbDir=0;
  dfuLong32 dfNbFileGood = 0;
  BOOL fDirOpen = FALSE;
  BOOL fStripIdenticalBody=FALSE;
  DFSFEATUREPARAM DfsFeatureParam;

  GetDfsExtendedMode(DfsFile, &fStripIdenticalBody);
  GetDfsFeatureParam(DfsFile, &DfsFeatureParam);
//fStripIdenticalBody=TRUE;

  DfsGetNbDir(DfsFile, &dfNbDir, pei);
  //dfuLong32 dfSizeOrigDone, dfSizeEncodedDone, dfFileDone;

  //dfFileDone = dfSizeOrigDone = dfSizeEncodedDone = 0;

  InitProgressCallBackInfo(&ProgressCallBackInfo, DfsFile, dfUserPtr);


  dfBufWriteSize = SIZE_BUFFER;
  dfBufReadSize = SIZE_BUFFER*4;
  BufAlloc = (dfbytep) DfsMalloc(dfBufReadSize + dfBufWriteSize);
  if (BufAlloc == NULL)
    return DFS_ERROR_MEMORY_ERROR;
  BufRead = BufAlloc;
  BufWrite = BufAlloc + dfBufReadSize;

  ProgressCallBackInfo.dfDirType = dfTypeDir;
  ProgressCallBackInfo.dfDirOrigDone = 0;
  ProgressCallBackInfo.dfDirEncodedDone = 0;
  ProgressCallBackInfo.dfDirOrigSize = 0;
  ProgressCallBackInfo.dfDirEncodedSize = 0;




  for (i=0,pCurFileToAdd = pFileToAdd;((i < dfNbFile) && (dfError == DFS_SUCCESS));i++,pCurFileToAdd++)
  {
    BOOL fForceRecopyReference = pFileToAdd->fWritingRaw &&
             (pFileToAdd->dfFileStatusForRaw == DFS_STORAGESTATUS_REFERENCE);

    if ((!pCurFileToAdd->fIgnore) && ((pCurFileToAdd->fForceRecopyPrevious) || (fForceRecopyReference)))
        dfNbFileGood++;

    if ((!pCurFileToAdd->fIgnore) && (!((pCurFileToAdd->fForceRecopyPrevious) || (fForceRecopyReference))))
    {
        ProgressCallBackInfo.filename_onwork = (dfwcharp)pCurFileToAdd->filename_ondisk;
        ProgressCallBackInfo.filename_stored = (dfwcharp)pCurFileToAdd->filename_tostore;
        ProgressCallBackInfo.fTemporaryFilename = FALSE;

        ProgressCallBackInfo.fWillIgnoreFile = FALSE;
        ProgressCallBackInfo.dfFileOrigDone = 0;
        ProgressCallBackInfo.dfFileEncodedDone = 0;
        ProgressCallBackInfo.dfFileOrigSize = 0;
        ProgressCallBackInfo.dfFileEncodedSize = 0;
        ProgressCallBackInfo.dfFileNumber = i;


        if (dfError == DFS_SUCCESS)
          if (!CallCallBack
              (pProgressCallBack, &ProgressCallBackInfo,
               DFCBM_BEFOREOPENWORKINGFILEPREPARSE, NULL))
            dfError = DFS_STOP_REQUESTED;

        if (ProgressCallBackInfo.fWillIgnoreFile)
          continue;


        if (ProgressCallBackInfo.filename_onwork == NULL)
            dfNbFileGood++;
        if ((dfError == DFS_SUCCESS) && (ProgressCallBackInfo.filename_onwork != NULL))
        {
          DFSTM dfsTm;
          dfuLong64 dfFileSize =
            GetFileSizeByName(ProgressCallBackInfo.filename_onwork, &dfsTm, pei);

          if (dfFileSize == FILE_SIZE_NOT_EXIST)
          {
            dfError = DFS_ERROR_ERRORIO;
            break;
          }
          else
          {
              ProgressCallBackInfo.dfDirOrigSize += dfFileSize;
              dfNbFileGood++;
          }
        }
    }
  }

  /* We do not accept a directory without valid file */
  if ((dfNbFileGood == 0) && (dfError == DFS_SUCCESS))
      dfError = DFS_ERROR_BAD_PARAMETER;

  if (dfError == DFS_SUCCESS)
    if (!CallCallBack
        (pProgressCallBack, &ProgressCallBackInfo, DFCBM_BEFOREOPENDIRECTORY,
         NULL))
      dfError = DFS_STOP_REQUESTED;

  if (dfError == DFS_SUCCESS)
  {
    dfError = DfsWCreateNewDir(DfsFile, dfTypeDir, pei);
    if (dfError == DFS_SUCCESS)
        fDirOpen = TRUE;
  }

  if (dfError == DFS_SUCCESS)
    if (!CallCallBack
        (pProgressCallBack, &ProgressCallBackInfo, DFCBM_AFTEROPENDIRECTORY,
         NULL))
      dfError = DFS_STOP_REQUESTED;

  if (((dfwVersionName != NULL) || (dfwVersionComment != NULL)) && (dfError == DFS_SUCCESS))
  {
      DFTAGBLOCKFLOAT TagBlockFloat;
      TagBlockFloat = GetDfsTagBlockFloat(DfsFile, pei);
      /*
      if (dfwVersionName!=NULL)
        dfError=DfsWAddTagInDir(DfsFile,DFSTAG_DIR_NAME,
                        dfwVersionName,(dfUnicodeStrlen(dfwVersionName) + 1) * 2,TRUE);
*/
      if (dfwVersionName!=NULL)
        AddTagBlockFloat(TagBlockFloat,dfNbDir,FLOATNUM_NOSPECIFIC,DFSTAG_DIR_NAME_FLOAT,
                        dfwVersionName,(dfUnicodeStrlen(dfwVersionName) + 1) * 2);
      if (dfwVersionComment!=NULL)
        AddTagBlockFloat(TagBlockFloat,dfNbDir,FLOATNUM_NOSPECIFIC,DFSTAG_DIR_COMMENT_FLOAT,
                        dfwVersionComment,(dfUnicodeStrlen(dfwVersionComment) + 1) * 2);
  }

  for (i=0,pCurFileToAdd = pFileToAdd;((i < dfNbFile) && (dfError == DFS_SUCCESS));i++,pCurFileToAdd++)
  {
    BOOL fForceRecopyReference = pFileToAdd->fWritingRaw &&
             (pFileToAdd->dfFileStatusForRaw == DFS_STORAGESTATUS_REFERENCE);
    const DFSPATCHANALYSEINFO_MEMORY* pDfsPatchAnalyseInfo=NULL;
    DFSPATCHANALYSEINFO_MEMORY dfsPatchAnalyseInfoToFill;

    dfsPatchAnalyseInfoToFill.total_size_insert = dfsPatchAnalyseInfoToFill.total_size_depl_in_place =
        dfsPatchAnalyseInfoToFill.total_size_depl_out_place = 0;

    if ((!pCurFileToAdd->fIgnore) && ((pCurFileToAdd->fForceRecopyPrevious) || fForceRecopyReference))
    {
        DFSINFODATE dfsInfoDate;
        DFSCRCINFOPARAM DfsCrcInfoParam;
        dfuLong32 dfBufWriteSize=0;
        dfuLong32Intel dfFileStatusIntel;
        BOOL fNewFileInDirCreated = FALSE;
        dfuLong32 dfNbTagToAdd, loopTag;

        dfuLong64 dfFileSize=pCurFileToAdd->dfForceRecopyOrRawCopySize;

        dfError = DfsWCreateNewFileInDir(DfsFile, dfTypeDir, dfFileSize, pei);

        if (dfError == DFS_SUCCESS)
            fNewFileInDirCreated = TRUE;

        dfNbTagToAdd = GetCountOfTags(pCurFileToAdd->hAddTags);
        for (loopTag = 0; loopTag < dfNbTagToAdd; loopTag++)
        {
            dfuLong32 tagNumber = GetTagNumberAtPos(pCurFileToAdd->hAddTags, loopTag);
            dfuLong32 tagSize = 0;
            dfvoidp tagBuf = NULL;
            if (GetTag(pCurFileToAdd->hAddTags, tagNumber, &tagBuf, &tagSize))
            {
                DfsWAddTagInNewFileInDir(DfsFile, tagNumber, tagBuf, tagSize,
                    fTagInfoBeforeFile, TRUE, pei);
            }
        }

        dfsInfoDate = pCurFileToAdd->dfsInfoDate;

        DfsWAddTagInNewFileInDir(DfsFile, DFSTAG_FILENAME,
                                 (dfwcharp)pCurFileToAdd->filename_tostore,
                                 (dfUnicodeStrlen(pCurFileToAdd->
                                                  filename_tostore) + 1) * 2,
                                 fTagInfoBeforeFile, TRUE, pei);

        DfsWAddTagInNewFileInDir(DfsFile, DFSTAG_DATE,
                                 &dfsInfoDate, sizeof(dfsInfoDate), fTagInfoBeforeFile, TRUE, pei);




        {
            DFSPREVIOUSVERSIONINFO DfsPreviousVersionInfo;
            DfsPreviousVersionInfo.dfPreviousVersionFileNumber = ConvertuLongToLongIntel(1);
            DfsPreviousVersionInfo.dfPreviousVersionFilePosition =
                ConvertuLongToLongIntel(pCurFileToAdd->dfPreviousVersionFilePosition);
            DfsWAddTagInNewFileInDir(DfsFile, DFSTAG_PREVIOUSVERSIONINFO,
                                 &DfsPreviousVersionInfo, sizeof(DFSPREVIOUSVERSIONINFO),
                                 fTagInfoBeforeFile, TRUE, pei);
        }

        if ((!fStripIdenticalBody) && (!fForceRecopyReference))
        {
            char BufWrite[0x200];
            dfBufWriteSize = MakeDifStFileIdentical(BufWrite,0x200,pCurFileToAdd->dfForceRecopyOrRawCopySize);
            dfError = DfsWWriteFileEncoded(DfsFile, BufWrite, dfBufWriteSize, pei);
        }

        dfFileStatusIntel = ConvertuLongToLongIntel(fForceRecopyReference ? DFS_STORAGESTATUS_REFERENCE : DFS_STORAGESTATUS_IDENTICAL) ;

        DfsWAddTagInNewFileInDir(DfsFile, DFSTAG_STORAGESTATUS,
                                    &dfFileStatusIntel,
                                    sizeof(dfuLong32Intel),
                                    FALSE, TRUE, pei);

        DfsCrcInfoParam.dfBeginPos = 0;
        DfsCrcInfoParam.dfEndPos = dfFileSize;
        DfsCrcInfoParam.dfCrc32Value = pCurFileToAdd ->dfForceRecopyOrRawCopyCrc32;
        DfsCrcInfoParam.fMd5 = pCurFileToAdd -> fForceRecopyOrRawCopyMd5Present;
        DfsCrcInfoParam.fSha1 = pCurFileToAdd -> fForceRecopyOrRawCopySha1Present;
        DfsCrcInfoParam.fSha256 = pCurFileToAdd->fForceRecopyOrRawCopySha256Present;
        if (!fForceRecopyReference)
            DfsCrcInfoParam.fSha256 = DfsCrcInfoParam.fSha1 = DfsCrcInfoParam.fMd5 = FALSE;

        if (DfsCrcInfoParam.fMd5)
            DfsMemcpy(DfsCrcInfoParam.bMd5,pCurFileToAdd ->bMd5,16);
        if (DfsCrcInfoParam.fSha1)
            DfsMemcpy(DfsCrcInfoParam.bSha1,pCurFileToAdd ->bSha1,20);
        if (DfsCrcInfoParam.fSha256)
            DfsMemcpy(DfsCrcInfoParam.bSha256, pCurFileToAdd->bSha256,32);

        DfsCrcInfoParam.dfTypeCrcAndMd5Info = DfsCrcInfoParam.fMd5 ? TYPECRCINFO_CRC32MD5 : TYPECRCINFO_CRC32;
        if (DfsCrcInfoParam.fSha1)
            DfsCrcInfoParam.dfTypeCrcAndMd5Info |= TYPECRCINFO_SHA1;
        if (DfsCrcInfoParam.fSha256)
            DfsCrcInfoParam.dfTypeCrcAndMd5Info |= TYPECRCINFO_SHA256;

        dfError2 = DFS_SUCCESS;
        if (fNewFileInDirCreated)
        {
          if ((!fStripIdenticalBody) && (!fForceRecopyReference))
            dfError2 = DfsWCloseFileInDir(DfsFile, dfFileSize, 1, &DfsCrcInfoParam,pDfsPatchAnalyseInfo,FALSE, pei);
          else
            dfError2 = DfsWCloseFileInDir(DfsFile, dfFileSize, 1, &DfsCrcInfoParam,pDfsPatchAnalyseInfo,TRUE, pei);

          fNewFileInDirCreated = FALSE;

          if (dfError == DFS_SUCCESS)
              dfError = dfError2;
        }
    }
    else
    if (!pCurFileToAdd->fIgnore)
    {
        dfuLong64 dfFileSize=0;
        dfuLong32 dfCrc32;
        BOOL fComputeMd5 = DfsFeatureParam.fComputeMd5;
        BOOL fComputeSha1 = DfsFeatureParam.fComputeSha1;
        BOOL fComputeSha256 = DfsFeatureParam.fComputeSha256;
        SVF_MD5_INTERNAL md5_ctx;
        SVF_SHA1_INTERNAL sha1_ctx;
        SVF_SHA256_INTERNAL sha256_ctx;
        dfuLong64 dfSizeToRead, dfSizeReadDone;
        LOWLEVELFILE llfr=NULL;
        DFSCRCINFOPARAM DfsCrcInfoParam;
        dfuLong32 dfTypeFile = dfTypeDir;
        DFSTM dfsTm;
        DFSINFODATE dfsInfoDate;
        BOOL fWstrInitialized, fZstrInitialized, fPsbFrdInitialized, fZstrInitializedErzatzRaw;
        WRITEDIF_STREAM wstr;
        abstract_compress_stream zstrm;
        ORIGDATA org;
        HFILECONTENTREADBUF hFileContentReadBuf = NULL;
        BOOL fFlushingTerminated;
        BOOL fFileIdentical=FALSE;
        //dfuLong64 total_size_insert=0;
        //dfuLong64 total_size_depl_in_place=0;
        //dfuLong64 total_size_depl_out_place=0;

        dfuLong32 dfFileStatus = 0;
        dfuLong32Intel dfFileStatusIntel ;
        BOOL fNewFileInDirCreated = FALSE;
        dfuLong64 dfFileSizeUncompressedReal=0;
        HRAMDIF hRamDifToFlushPatch = NULL;
        PATCHSTREAMBLD_FROM_RAMDIF psbFrd ;
        BOOL fRamDifContinue;
        BOOL fWriteStoragePatchInfo = FALSE;
        dfuLong32 dfNbTagToAdd, loopTag;

        if (pCurFileToAdd->fWritingRaw)
            dfTypeFile = TYPEDIR_FILEINSERTING_STORE;

        if (pCurFileToAdd->hRamDifToFlushPatch != NULL)
            if (pCurFileToAdd->filename_ondisk == NULL)
            {
                hRamDifToFlushPatch = pCurFileToAdd->hRamDifToFlushPatch;
                dfTypeFile = TYPEDIR_PATCHFROMPREVIOUS;
            }

        fWstrInitialized = fZstrInitialized = fZstrInitializedErzatzRaw = fPsbFrdInitialized = FALSE;

        ProgressCallBackInfo.filename_onwork = (dfwcharp)pCurFileToAdd->filename_ondisk;
        ProgressCallBackInfo.filename_stored = (dfwcharp)pCurFileToAdd->filename_tostore;
        ProgressCallBackInfo.fTemporaryFilename = FALSE;

        ProgressCallBackInfo.fWillIgnoreFile = FALSE;
        ProgressCallBackInfo.dfFileOrigDone = 0;
        ProgressCallBackInfo.dfFileEncodedDone = 0;
        ProgressCallBackInfo.dfFileOrigSize = 0;
        ProgressCallBackInfo.dfFileEncodedSize = 0;
        ProgressCallBackInfo.dfFileNumber = i;

        if (ProgressCallBackInfo.filename_onwork != NULL)
        {
            if (dfError == DFS_SUCCESS)
              if (!CallCallBack
                  (pProgressCallBack, &ProgressCallBackInfo,
                   DFCBM_BEFOREOPENWORKINGFILE, NULL))
                dfError = DFS_STOP_REQUESTED;

            if (ProgressCallBackInfo.fWillIgnoreFile)
              continue;

            if (dfError == DFS_SUCCESS)
            {
              dfFileSize =
                GetFileSizeByName(ProgressCallBackInfo.filename_onwork, &dfsTm, pei);

              if (dfFileSize == FILE_SIZE_NOT_EXIST)
              {
                dfError = DFS_ERROR_ERRORIO;
                break;
              }
              else
                  ProgressCallBackInfo.dfFileOrigSize = dfFileSize;

              llfr = OpenLowLevel(ProgressCallBackInfo.filename_onwork, OPEN_READ,FALSE,FALSE,0,pei);
              if (llfr == NULL)
              {
                dfError = DFS_ERROR_ERRORIO;
                break;
              }
            }
        }

        if (dfError == DFS_SUCCESS)
          {
            //dfError = DfsWCreateNewFileInDir(DfsFile, dfTypeDir, dfFileSize);

            dfFileSizeUncompressedReal = (pCurFileToAdd->fWritingRaw || (hRamDifToFlushPatch!=NULL)) ?
                          pCurFileToAdd->dfForceRecopyOrRawCopySize: dfFileSize;
            dfError = DfsWCreateNewFileInDir(DfsFile, dfTypeDir, dfFileSizeUncompressedReal, pei);

            if (dfError == DFS_SUCCESS)
                fNewFileInDirCreated = TRUE;
          }

        if (!pCurFileToAdd->fForceDate)
            ConvertDfsTmToDfsInfoDate(&dfsTm, &dfsInfoDate);
        else
            dfsInfoDate = pCurFileToAdd->dfsInfoDate;

        if ((dfTypeFile == TYPEDIR_PATCHFROMPREVIOUS) &&
            (pCurFileToAdd->filename_prevversionondisk == NULL) && (hRamDifToFlushPatch == NULL))
          dfTypeFile = TYPEDIR_FILEINSERTING_DEFLATE;
/* **++-- if ((dfTypeFile == TYPEDIR_PATCHFROMPREVIOUS) && (dfFileSize==0)) dfTypeFile = TYPEDIR_FILEINSERTING_DEFLATE;*/
        if ((dfTypeFile == TYPEDIR_PATCHFROMPREVIOUS) && (hRamDifToFlushPatch != NULL))
        {

            dfuLong32 CompressRatio = 1;
            dfuLong32 dfSizeButStreamKB = DEFAULT_SIZE_BUF_STREAM_KB;

            if (pCprParam != NULL)
            {
                // strange logic from makdifst.c for select compression ratio in patch
                //if (pCprParam->uZlibCompressRatio >= 11)
                if (pCprParam->uZlibCompressRatio >= 1)
                    CompressRatio = pCprParam->uZlibCompressRatio;
                dfSizeButStreamKB = pCprParam->dfSizeButStreamKB;
            }

            if (PatchOutStreamBldFromRamDif(&psbFrd, pCurFileToAdd->dfForceRecopyOrRawCopySize, hRamDifToFlushPatch, CompressRatio, dfSizeButStreamKB) == DSERR_OK)
                fPsbFrdInitialized = TRUE;
            else
                dfError = DFS_ERROR_BAD_PARAMETER;
        }

        if ((dfTypeFile == TYPEDIR_PATCHFROMPREVIOUS) && (hRamDifToFlushPatch == NULL))
        {
          int err;
          //int CompressRatio = 1;

//          org.givePtr = NULL;
//          org.freePtr = NULL;

          ProgressCallBackInfo.filename_previousversion =
            (dfwcharp)pCurFileToAdd->filename_prevversionondisk;
          if (dfError == DFS_SUCCESS)
            if (!CallCallBack
                (pProgressCallBack, &ProgressCallBackInfo,
                 DFCBM_BEFOREOPENPREVIOUSFILE, NULL))
              dfError = DFS_STOP_REQUESTED;

          if (dfError == DFS_SUCCESS)
          { /* create the FMIO */
            if (!
              GetFileFullContentBuffer(ProgressCallBackInfo.
                                           filename_previousversion,FILEFULLCONTENTBUFFER_READ,0,
                                           &hFileContentReadBuf,
                                           &org,pei))
                dfError = DFS_ERROR_ERRORIO;
          }

          DfsClearStruct(&wstr, 0, sizeof(WRITEDIF_STREAM));

          if (dfError == DFS_SUCCESS)
          {
            wstr.nbOrig = 1;
            wstr.OrigDataPtr = &org;
          }
          else
          {
            wstr.nbOrig = 0;
            wstr.OrigDataPtr = NULL;
          }

          err = MakeDifInit(&wstr, pCprParam);  // 5 = compress ratio
          if (err != DSERR_OK)
            dfError = DFS_ERROR_MEMORY_ERROR;
          //wstr.nbOrig = 1;
          //wstr.OrigDataPtr = &org;

          wstr.pPerformOrigDataAnalysisCB = NULL;
          wstr.dfUserPtr = NULL;

          wstr.out_data_stream.next_out = (dfvoidp) BufWrite;
          wstr.out_data_stream.avail_out = dfBufWriteAvailOut = dfBufWriteSize;

          if (err != DSERR_OK)
            dfError = DFS_ERROR_MEMORY_ERROR;
          else
            fWstrInitialized = TRUE;

    #ifdef DO_STATIS
          if (fWstrInitialized)
            EnableStatis(&wstr, 512);
    #endif
        }


        if (fPsbFrdInitialized)
        {
          psbFrd.out_data_stream.total_out = 0;
          psbFrd.out_data_stream.next_out = BufWrite;
          psbFrd.out_data_stream.avail_out = dfBufWriteAvailOut = dfBufWriteSize;
        }

        if ((dfTypeFile == TYPEDIR_FILEINSERTING_STORE) ||
            (dfTypeFile == TYPEDIR_FILEINSERTING_DEFLATE))
        {
          DfsClearStruct(&zstrm, 0, sizeof(abstract_compress_stream));
          zstrm.next_out = BufWrite;
          zstrm.avail_out = dfBufWriteAvailOut = dfBufWriteSize;
          if (pCurFileToAdd->fWritingRaw)
          {
              fZstrInitialized = fZstrInitializedErzatzRaw = TRUE;
          }
          else
          {
            int abstract_init_ret_value;
            unsigned int CompressRatio = 0;
            if (pCprParam != NULL)
                CompressRatio = (unsigned int)pCprParam->uZlibCompressRatio;

            if (CompressRatio >= 21)
            {
             abstract_init_ret_value = abstract_init_compress_autoselect(&zstrm,CompressRatio,0);
            }
            else
             abstract_init_ret_value = abstract_init_compress_inflate_withoutNegMaxWBits(&zstrm,
                  (dfTypeFile == TYPEDIR_FILEINSERTING_DEFLATE) ?
                                    ABSTR_COMPRESS_Z_DEFAULT_COMPRESSION : 0);


            if (abstract_init_ret_value != ABSTR_COMPRESS_Z_OK)
                dfError = DFS_ERROR_MEMORY_ERROR;
            else
                fZstrInitialized = TRUE;
          }
        }


        dfSizeToRead = dfFileSize;
        dfSizeReadDone = 0;
        dfCrc32 = 0;
        if (fComputeMd5)
          svf_md5_init(&md5_ctx);
        if (fComputeSha1)
          svf_sha1_init(&sha1_ctx);
        if (fComputeSha256)
            svf_sha256_init(&sha256_ctx);


        dfNbTagToAdd = GetCountOfTags(pCurFileToAdd->hAddTags);
        for (loopTag = 0; loopTag < dfNbTagToAdd; loopTag++)
        {
            dfuLong32 tagNumber = GetTagNumberAtPos(pCurFileToAdd->hAddTags, loopTag);
            dfuLong32 tagSize = 0;
            dfvoidp tagBuf = NULL;
            if (GetTag(pCurFileToAdd->hAddTags, tagNumber, &tagBuf, &tagSize))
            {
                DfsWAddTagInNewFileInDir(DfsFile, tagNumber, tagBuf, tagSize,
                    fTagInfoBeforeFile, TRUE, pei);
            }
        }

        DfsWAddTagInNewFileInDir(DfsFile, DFSTAG_FILENAME,
                                 (dfwcharp)pCurFileToAdd->filename_tostore,
                                 (dfUnicodeStrlen(pCurFileToAdd->
                                                  filename_tostore) + 1) * 2,
                                 fTagInfoBeforeFile, TRUE, pei);

        DfsWAddTagInNewFileInDir(DfsFile, DFSTAG_DATE,
                                 &dfsInfoDate, sizeof(dfsInfoDate), fTagInfoBeforeFile, TRUE, pei);



        if ((fWstrInitialized) || fPsbFrdInitialized ||
            (((pCurFileToAdd->fWritingRaw) &&
             (pCurFileToAdd->dfFileStatusForRaw == DFS_STORAGESTATUS_MODIFIED))))
        {
            DFSPREVIOUSVERSIONINFO DfsPreviousVersionInfo;
            DfsPreviousVersionInfo.dfPreviousVersionFileNumber = ConvertuLongToLongIntel(1);
            DfsPreviousVersionInfo.dfPreviousVersionFilePosition =
                ConvertuLongToLongIntel(pCurFileToAdd->dfPreviousVersionFilePosition);
            DfsWAddTagInNewFileInDir(DfsFile, DFSTAG_PREVIOUSVERSIONINFO,
                                 &DfsPreviousVersionInfo, sizeof(DFSPREVIOUSVERSIONINFO),
                                 fTagInfoBeforeFile, TRUE, pei);
        }

        fRamDifContinue = hRamDifToFlushPatch != NULL;

        while (((dfSizeToRead > 0) || (fRamDifContinue)) && (dfError == DFS_SUCCESS))
        {
          dfuLong32 dfSizeStep=0;
          dfuLong32 dfSizeRead=0;
          dfuLong32 dfBufWriteAvailIn = 0;
          BOOL fNoInDataContinue = fPsbFrdInitialized;

          dfSizeStep = dfBufReadSize;
          if (dfSizeToRead < dfSizeStep)
            dfSizeStep = (dfuLong32)dfSizeToRead;

          if (dfError == DFS_SUCCESS)
            if (!CallCallBack
                (pProgressCallBack, &ProgressCallBackInfo,
                 DFCBM_PROGRESSWORKINGFILE, NULL))
            {
              dfError = DFS_STOP_REQUESTED;
              break;
            }

          if (hRamDifToFlushPatch==NULL)
          {
              dfSizeRead = LowLevelRead(llfr, BufRead, dfSizeStep,pei);
              ProgressCallBackInfo.dfDirOrigDone += dfSizeRead;
              ProgressCallBackInfo.dfFileOrigDone += dfSizeRead;

              if ((dfSizeRead != dfSizeStep))
                dfError = DFS_ERROR_ERRORIO;
          }

          dfCrc32 = crc32(dfCrc32, BufRead, dfSizeRead);
          if (fComputeMd5)
            svf_md5_append(&md5_ctx, BufRead, dfSizeRead);
          if (fComputeSha1)
            svf_sha1_append(&sha1_ctx, BufRead, dfSizeRead);
          if (fComputeSha256)
              svf_sha256_append(&sha256_ctx, BufRead, dfSizeRead);

          dfSizeToRead -= dfSizeRead;
          dfSizeReadDone += dfSizeRead;
          dfBufWriteAvailIn = dfSizeRead;

          if (fWstrInitialized)
          {
            wstr.in_data_stream.next_in = (dfvoidp) BufRead;
            wstr.in_data_stream.avail_in = dfBufWriteAvailIn;
          }

          if (fZstrInitialized)
          {
            zstrm.next_in = (dfbytep) BufRead;
            zstrm.avail_in = dfBufWriteAvailIn;
          }

          if ((fPsbFrdInitialized) || (fZstrInitialized) || (fWstrInitialized))
            do
            {
              if (fPsbFrdInitialized)
              {
                  int err;
                  err=DoPatchOutStreamBldFromRamDif(&psbFrd);
                  if (err!=DSERR_OK)
                      fNoInDataContinue = FALSE; /* for END or error */
                  dfBufWriteAvailOut = (dfuLong32)psbFrd.out_data_stream.avail_out;
                  dfBufWriteAvailIn = 0;
                  if (err == DSERR_END)
                  {
                      fRamDifContinue = FALSE;
                  }
                  else
                  if (err != DSERR_OK)
                    dfError = DFS_ERROR_MEMORY_ERROR;
              }

              if (fWstrInitialized)
              {
                int err;
                //wstr.in_data_stream.next_in = (dfvoidp) BufRead;
                wstr.in_data_stream.avail_in = dfBufWriteAvailIn;
                err = DoMakeDifWork(&wstr);




                  #if defined (_DEBUG) && defined (SHOWCRC)
                  {
                      dfuLong32 crcin =  crc32(0,((dfbytep)wstr.in_data_stream.next_in)-wstr.done_latest_in,wstr.done_latest_in);
                      dfuLong32 crcout = crc32(0,((dfbytep)wstr.out_data_stream.next_out)-wstr.done_latest_out,wstr.done_latest_out);
                      printf("crc in,out=%lx,%lx for length %u,%u\n",crcin,crcout,wstr.done_latest_in,wstr.done_latest_out);
                  }
                  #endif

                dfBufWriteAvailOut = (dfuLong32)wstr.out_data_stream.avail_out;
                dfBufWriteAvailIn = (dfuLong32)wstr.in_data_stream.avail_in;
                if (err != 0)
                  dfError = DFS_ERROR_MEMORY_ERROR;
              }

              if (fZstrInitialized)
              {
                int err;
                //zstr.next_in = (dfvoidp) BufRead;
                zstrm.avail_in = dfBufWriteAvailIn;
                if (fZstrInitializedErzatzRaw)
                    err = XflateStore_compress(&zstrm, ABSTR_COMPRESS_Z_NO_FLUSH);
                else
                    err = abstract_compress(&zstrm, ABSTR_COMPRESS_Z_NO_FLUSH);

                dfBufWriteAvailOut = zstrm.avail_out;
                dfBufWriteAvailIn = zstrm.avail_in;
                if (err != ABSTR_COMPRESS_Z_OK)
                  dfError = DFS_ERROR_MEMORY_ERROR;
              }

              if (((fWstrInitialized) || (fZstrInitialized) || (fPsbFrdInitialized))
                  && ((dfBufWriteAvailOut==0)))
              {

                dfError = DfsWWriteFileEncoded(DfsFile, BufWrite, dfBufWriteSize, pei);
                if (dfError == DFS_SUCCESS)
                {
                  ProgressCallBackInfo.dfDirEncodedDone += dfBufWriteSize;
                  ProgressCallBackInfo.dfFileEncodedDone += dfBufWriteSize;
                }

                if (fPsbFrdInitialized)
                {
                  psbFrd.out_data_stream.next_out = (dfvoidp) BufWrite;
                  psbFrd.out_data_stream.avail_out = dfBufWriteAvailOut = dfBufWriteSize;
                }

                if (fWstrInitialized)
                {
                  wstr.out_data_stream.next_out = (dfvoidp) BufWrite;
                  wstr.out_data_stream.avail_out = dfBufWriteAvailOut = dfBufWriteSize;
                }

                if (fZstrInitialized)
                {
                  zstrm.next_out = BufWrite;
                  zstrm.avail_out = dfBufWriteAvailOut = dfBufWriteSize;
                }
              }
            }
            while (((dfBufWriteAvailIn > 0) || (fNoInDataContinue)) && (dfError == DFS_SUCCESS));
        }

        fFlushingTerminated = FALSE;


        if (fPsbFrdInitialized)
        {
            if ((dfError == DFS_SUCCESS) && (dfBufWriteAvailOut < dfBufWriteSize))
              dfError =
                DfsWWriteFileEncoded(DfsFile, BufWrite,
                                     dfBufWriteSize - dfBufWriteAvailOut, pei);
        }

        if ((fWstrInitialized) || (fZstrInitialized))
          do
          {
            if ((fWstrInitialized) && (wstr.out_data_stream.avail_out>0))
            {
              int err = FlushMakeDif(&wstr);


          #if defined (_DEBUG) && defined (SHOWCRC)
          {
              dfuLong32 crcin =  crc32(0,((dfbytep)wstr.in_data_stream.next_in)-wstr.done_latest_in,wstr.done_latest_in);
              dfuLong32 crcout = crc32(0,((dfbytep)wstr.out_data_stream.next_out)-wstr.done_latest_out,wstr.done_latest_out);
              printf("crc in,out=%lx,%lx for length %u,%u\n",crcin,crcout,wstr.done_latest_in,wstr.done_latest_out);
          }
          #endif


              dfBufWriteAvailOut = (dfuLong32)wstr.out_data_stream.avail_out;
              if (err == DSERR_END)
                fFlushingTerminated = TRUE;
              else if (err != DSERR_OK)
                dfError = DFS_ERROR_MEMORY_ERROR;
            }

            if ((fZstrInitialized) && (zstrm.avail_out>0))
            {
              int err ;

              if (fZstrInitializedErzatzRaw)
                  err = XflateStore_compress(&zstrm, ABSTR_COMPRESS_Z_FINISH);
              else
                  err = abstract_compress(&zstrm, ABSTR_COMPRESS_Z_FINISH);

              dfBufWriteAvailOut = zstrm.avail_out;
              if (err == ABSTR_COMPRESS_Z_STREAM_END)
                fFlushingTerminated = TRUE;
              else if (err != ABSTR_COMPRESS_Z_OK)
                dfError = DFS_ERROR_MEMORY_ERROR;
            }

            if ((dfError == DFS_SUCCESS) && (dfBufWriteAvailOut < dfBufWriteSize))
              dfError =
                DfsWWriteFileEncoded(DfsFile, BufWrite,
                                     dfBufWriteSize - dfBufWriteAvailOut, pei);

            if (fWstrInitialized)
            {
              wstr.out_data_stream.next_out = (dfvoidp) BufWrite;
              wstr.out_data_stream.avail_out = dfBufWriteAvailOut = dfBufWriteSize;
            }
            if (fZstrInitialized)
            {
              zstrm.next_out = BufWrite;
              zstrm.avail_out = dfBufWriteAvailOut = dfBufWriteSize;
            }
          } while ((!fFlushingTerminated) && (dfError == DFS_SUCCESS));

        DfsCrcInfoParam.dfBeginPos = 0;
        DfsCrcInfoParam.dfTypeCrcAndMd5Info = TYPECRCINFO_CRC32;

        if ((pCurFileToAdd->fWritingRaw) || fPsbFrdInitialized)
        {
            if (fPsbFrdInitialized)
                dfFileStatus = DFS_STORAGESTATUS_MODIFIED;
            else
                dfFileStatus = pCurFileToAdd->dfFileStatusForRaw;
            DfsCrcInfoParam.dfEndPos = pCurFileToAdd->dfForceRecopyOrRawCopySize;
            DfsCrcInfoParam.dfCrc32Value = pCurFileToAdd ->dfForceRecopyOrRawCopyCrc32;
            DfsCrcInfoParam.fMd5 = pCurFileToAdd -> fForceRecopyOrRawCopyMd5Present;
            DfsCrcInfoParam.fSha1 = pCurFileToAdd -> fForceRecopyOrRawCopySha1Present;
            DfsCrcInfoParam.fSha256 = pCurFileToAdd->fForceRecopyOrRawCopySha256Present;
            if (DfsCrcInfoParam.fMd5)
            {
                DfsMemcpy(DfsCrcInfoParam.bMd5,pCurFileToAdd ->bMd5,16);
                DfsCrcInfoParam.dfTypeCrcAndMd5Info = TYPECRCINFO_CRC32MD5 ;
            }
            DfsCrcInfoParam.dfTypeCrcAndMd5Info = DfsCrcInfoParam.fMd5 ? TYPECRCINFO_CRC32MD5 : TYPECRCINFO_CRC32;

            if (DfsCrcInfoParam.fSha1)
            {
                DfsMemcpy(DfsCrcInfoParam.bSha1,pCurFileToAdd ->bSha1,20);
                DfsCrcInfoParam.dfTypeCrcAndMd5Info |= TYPECRCINFO_SHA1 ;
            }


            if (DfsCrcInfoParam.fSha256)
            {
                DfsMemcpy(DfsCrcInfoParam.bSha256, pCurFileToAdd->bSha256,32);
                DfsCrcInfoParam.dfTypeCrcAndMd5Info |= TYPECRCINFO_SHA256;
            }

            if (fPsbFrdInitialized)
            {
                dfFileStatus = DFS_STORAGESTATUS_MODIFIED;

                if (ClosePatchStreamBldFromRamDif(&psbFrd, &dfsPatchAnalyseInfoToFill) == DSERR_OK)
                    fWriteStoragePatchInfo = TRUE;
                else
                    dfError = DFS_ERROR_MEMORY_ERROR;
            }
        }
        else
        {
            DfsCrcInfoParam.dfEndPos = dfSizeReadDone;
            DfsCrcInfoParam.dfCrc32Value = dfCrc32;

            //fComputeMD5 = FALSE ; // crippleBeta remove it

            DfsCrcInfoParam.fMd5 = fComputeMd5;
            DfsCrcInfoParam.fSha1 = fComputeSha1;
            DfsCrcInfoParam.fSha256 = fComputeSha256;
            //++SHA256 DfsCrcInfoParam.fSha256 = fComputeSha256;

            if (fComputeMd5)
            {
                svf_md5_finish(&md5_ctx,DfsCrcInfoParam.bMd5);
                DfsCrcInfoParam.dfTypeCrcAndMd5Info = TYPECRCINFO_CRC32MD5 ;
#if defined(_DEBUG) && defined(SHOW_MD5_COMPUTE)
                { int i; printf("md5 is :"); for (i=0;i<16;i++) printf("%02x",DfsCrcInfoParam.bMd5[i]); printf("\n") ;};
#endif
            }

            if (fComputeSha1)
            {
                svf_sha1_finish(&sha1_ctx,DfsCrcInfoParam.bSha1);
                DfsCrcInfoParam.dfTypeCrcAndMd5Info |= TYPECRCINFO_SHA1;
            }

            if (fComputeSha256)
            {
                svf_sha256_finish(&sha256_ctx,DfsCrcInfoParam.bSha256);
                DfsCrcInfoParam.dfTypeCrcAndMd5Info |= TYPECRCINFO_SHA256 ;
            }

            if (fWriteCrcAndSizeInFileToAddArray)
            {
                pCurFileToAdd->dfForceRecopyOrRawCopySize = dfSizeReadDone;
                pCurFileToAdd->dfForceRecopyOrRawCopyCrc32 = DfsCrcInfoParam.dfCrc32Value ;
                pCurFileToAdd->fForceRecopyOrRawCopyMd5Present = DfsCrcInfoParam.fMd5;
                if (pCurFileToAdd->fForceRecopyOrRawCopyMd5Present)
                    DfsMemcpy(pCurFileToAdd->bMd5,DfsCrcInfoParam.bMd5,16);

                pCurFileToAdd->fForceRecopyOrRawCopySha1Present = DfsCrcInfoParam.fSha1;
                if (pCurFileToAdd->fForceRecopyOrRawCopySha1Present)
                    DfsMemcpy(pCurFileToAdd->bSha1,DfsCrcInfoParam.bSha1,20);

                pCurFileToAdd->fForceRecopyOrRawCopySha256Present = DfsCrcInfoParam.fSha256;
                if (pCurFileToAdd->fForceRecopyOrRawCopySha256Present)
                    DfsMemcpy(pCurFileToAdd->bSha256,DfsCrcInfoParam.bSha256, 32);

                if (!pCurFileToAdd->fForceDate)
                    ConvertDfsTmToDfsInfoDate(&dfsTm, &pCurFileToAdd->dfsInfoDate);
            }

            if (dfTypeFile == TYPEDIR_FILECRCONLY)
                dfFileStatus = DFS_STORAGESTATUS_REFERENCE;

            if (fZstrInitialized)
            {
                dfFileStatus = DFS_STORAGESTATUS_NEW;

                if (!fZstrInitializedErzatzRaw)
                  abstract_compress_end(&zstrm);
            }

            if (dfTypeFile == TYPEDIR_FILEINSERTING_STORE)
                dfFileStatus = DFS_STORAGESTATUS_NEWSTORED;

            if (fWstrInitialized)
            {
                CloseMakeDifEx(&wstr, NULL,&fFileIdentical,
                    &dfsPatchAnalyseInfoToFill);
                if (!fFileIdentical)
                    pDfsPatchAnalyseInfo = &dfsPatchAnalyseInfoToFill;

                dfFileStatus = fFileIdentical ? DFS_STORAGESTATUS_IDENTICAL:DFS_STORAGESTATUS_MODIFIED;
                if (fFileIdentical)
                    DfsCrcInfoParam.dfTypeCrcAndMd5Info = TYPECRCINFO_CRC32;
                else
                    fWriteStoragePatchInfo = TRUE;
            }
        }


        dfFileStatusIntel = ConvertuLongToLongIntel(dfFileStatus) ;
        /* depl DEBUG 2014-10-26
        DfsWAddTagInNewFileInDir(DfsFile, DFSTAG_STORAGESTATUS,
                                    &dfFileStatusIntel,
                                    sizeof(dfuLong32Intel),
                                    FALSE, TRUE, pei);
                                    */
        if (fWriteStoragePatchInfo)
        {
            DFSSTORAGEPATCHINFOINTEL DfsStoragePatchInfoIntel;
            DfsStoragePatchInfoIntel.dfSizeDeplInPlaceIntel = ConvertuLongToLongIntel64(dfsPatchAnalyseInfoToFill.total_size_depl_in_place);
            DfsStoragePatchInfoIntel.dfSizeDeplOutPlaceIntel = ConvertuLongToLongIntel64(dfsPatchAnalyseInfoToFill.total_size_depl_out_place);
            DfsWAddTagInNewFileInDir(DfsFile, DFSTAG_STORAGEPATCHINFO,
                                    &DfsStoragePatchInfoIntel,
                                    sizeof(DfsStoragePatchInfoIntel),
                                    FALSE, TRUE, pei);
        }

        DfsWAddTagInNewFileInDir(DfsFile, DFSTAG_STORAGESTATUS,
            &dfFileStatusIntel,
            sizeof(dfuLong32Intel),
            FALSE, TRUE, pei);

        dfError2 = DFS_SUCCESS;
        if (fNewFileInDirCreated)
        {
          if (!fStripIdenticalBody)
          dfError2 = DfsWCloseFileInDir(DfsFile, dfFileSizeUncompressedReal, 1, &DfsCrcInfoParam,pDfsPatchAnalyseInfo,FALSE,pei);
          else
          dfError2 = DfsWCloseFileInDir(DfsFile, dfFileSizeUncompressedReal, 1, &DfsCrcInfoParam,pDfsPatchAnalyseInfo,fFileIdentical,pei);

          fNewFileInDirCreated = FALSE;
          if (dfError == DFS_SUCCESS)
              dfError = dfError2;
        }


        if (llfr != NULL)
          LowLevelClose(llfr,pei);

        if (dfError == DFS_SUCCESS)
          if (!CallCallBack
              (pProgressCallBack, &ProgressCallBackInfo,
               DFCBM_AFTERCLOSINGWORKINGFILE, NULL))
            dfError = DFS_STOP_REQUESTED;


        if (hFileContentReadBuf != NULL)
        {
          CloseFileFullContentBuffer(hFileContentReadBuf);
          if (dfError == DFS_SUCCESS)
            if (!CallCallBack
                (pProgressCallBack, &ProgressCallBackInfo,
                 DFCBM_AFTERCLOSINGPREVIOUSFILE, NULL))
              dfError = DFS_STOP_REQUESTED;
        }
    }
  }

  if (fDirOpen)
  {
    dfRetCall = DfsWCloseDir(DfsFile,pei);
    if (dfError == DFS_SUCCESS)
        dfError = dfRetCall;
    fDirOpen = FALSE;
  }

  if (dfError == DFS_SUCCESS)
    if (!CallCallBack
        (pProgressCallBack, &ProgressCallBackInfo,
         DFCBM_AFTERCLOSINGDIRECTORY, NULL))
      dfError = DFS_STOP_REQUESTED;


  DfsFree(BufAlloc);
  dfFlush = DfsFlushWriteDfsFile(DfsFile, pei);
  if (dfError == DFS_SUCCESS)
      dfError = dfFlush ;
  return dfError;
}

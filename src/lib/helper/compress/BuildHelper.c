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
 *  BuildHelper.cpp
 *
 */


/* this file can be compiled as C file */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>


#include "../../engine/patchstream/common/difbasic.h"
#include "../../../misc/svfdll.h"
#include "../../../cli/DfsCdLin.h"

#include "../../engine/patchstream/common/ltoolsc.h"
#include "../../engine/svfile/compress/AddingTool.h"


#include "../../hash/svf_sha256.h"
#include "../../hash/svf_md5.h"
#include "../../hash/svf_sha.h"


#include "BuildHelper.h"

BOOL DFSCALLBACK ProgressCallBackCreatePatchHelper(PROGRESSCALLBACKINFO * pProgressCallBackInfo)
{
    if (pProgressCallBackInfo->dfEvent==DFCBM_BEFOREOPENWORKINGFILE)
    {
        //printf("processing ");
        //DispOutUnicodeString(pProgressCallBackInfo->filename_stored);
    }

    if (pProgressCallBackInfo->dfEvent==DFCBM_AFTERCLOSINGWORKINGFILE)
    {
        //printf("\n");
    }
    return TRUE;
}




static BOOL AddTextInBuf(char**szText,int* BufSize,const char* szTextAdd)
{
    int len=(int)strlen(szTextAdd);
    if ((len+1) >= (*BufSize))
        return FALSE;
    strcpy(*szText,szTextAdd);
    (*szText)+=len;
    (*BufSize)-=len;
    return TRUE;
}

static BOOL AddUnicTextInBuf(char**szText,int* BufSize,dfwcharp dfTextAdd)
{
    if (!ConvertUnicodeToAnsi(dfTextAdd,*szText,*BufSize))
        return FALSE;
    int len=(int)strlen(*szText);
    (*szText)+=len;
    (*BufSize)-=len;
    return TRUE;
}

static BOOL GetErrorText(H_ERROR_INFO hei,char*szText,int BufSize)
{
    dfwcharp dfFileName=NULL;
    dfwcharp dfErrorMsg=NULL;
    dfuLong32 dfSizeFileName,dfSizeErrorMsg;
    BOOL fRet = TRUE;

    if (hei==NULL)
    {
        if ((szText!=NULL) && (BufSize>0))
            *szText=0;
        return TRUE;
    }
    if (BufSize<0x20)
        return FALSE;
    AddTextInBuf(&szText,&BufSize,"error detected : ");
    BufSize -= (int)strlen(szText);
    szText += strlen(szText);

    GetErrorInfoItemByTag(hei,DFS_ERRORTAG_FILENAME,(dfbytep*)(&dfFileName),&dfSizeFileName);
    GetErrorInfoItemByTag(hei,DFS_ERRORTAG_ERRORMSG,(dfbytep*)(&dfErrorMsg),&dfSizeErrorMsg);

    if (dfFileName!=NULL)
    {
        AddTextInBuf(&szText,&BufSize,"filename : ");
        AddUnicTextInBuf(&szText,&BufSize,dfFileName);
        AddTextInBuf(&szText,&BufSize,"\n");
    }


    if (dfErrorMsg!=NULL)
    {
        AddTextInBuf(&szText,&BufSize,"message : ");
        AddUnicTextInBuf(&szText,&BufSize,dfErrorMsg);
        AddTextInBuf(&szText,&BufSize,"\n");
    }

    return fRet;
}

int SVFAPI BuildPatchFromTwoFile(const char*DfsToBuild,const char*FirstFilename,const char*SecondFileName)
{
    return BuildPatchFromTwoFileEx(DfsToBuild,FirstFilename,SecondFileName,NULL,0);
}


int SVFAPI BuildPatchFromTwoFileEx(const char*DfsToBuild, const char*FirstFilename, const char*SecondFileName, char*errText, dfuLong32 dwErrTextSize)
{
    return BuildPatchFromTwoFileEx2(DfsToBuild, FirstFilename, SecondFileName, errText, dwErrTextSize, -1, -1, -1);
}

int SVFAPI BuildPatchFromTwoFileEx2(const char*DfsToBuild, const char*FirstFilename, const char*SecondFileName, char*errText, dfuLong32 dwErrTextSize,
    signed long CompressRatio, signed long HashBits, signed long BlockSize)
{
    return BuildPatchFromTwoFileEx3(DfsToBuild, FirstFilename, SecondFileName, errText, dwErrTextSize,
        CompressRatio, HashBits, BlockSize, 0);
}

// duplicated in BuildHelper.cpp and DfsCdLin.cpp
static dfuLong32 GetPhysSizeKbUsable()
{
  dfuLong32 sizeReported = GetPhysicalMemoryKb();
  dfuLong32 totalLess2GB = (sizeReported > (2048 * 1024)) ? (GetPhysicalMemoryKb() - (2048 * 1024)) : 0;
  dfuLong32 totalDivide4 = sizeReported / 4;
  dfuLong32 sizeSelected = (totalLess2GB > totalDivide4) ? totalLess2GB : totalDivide4;
  return sizeSelected;
}


int SVFAPI BuildPatchFromTwoFileExCprParam(const char*DfsToBuild, const char*FirstFilename, const char*SecondFileName, char*errText, dfuLong32 dwErrTextSize,
    COMPRESSIONPARAM* pCprParamToBuildFnc, signed long supp_flags)
{
    int ret=0;
    dfwchar szDfsFileName[MAX_PATH_LENGTH];
    dfwchar szFirstFileName[MAX_PATH_LENGTH];
    dfwchar szFirstFileNameGeneric[MAX_PATH_LENGTH];
    dfwchar szFirstFileNamePath[MAX_PATH_LENGTH];
    dfwchar szFirstFileNameFileName[MAX_PATH_LENGTH];
    dfwchar szSecondFileName[MAX_PATH_LENGTH];
    dfwchar szSecondFileNameGeneric[MAX_PATH_LENGTH];
    dfwchar szSecondFileNamePath[MAX_PATH_LENGTH];
    dfwchar szSecondFileNameFileName[MAX_PATH_LENGTH];
    BOOL fComputeMd5 = ((supp_flags & FLAG_NO_COMPUTE_MD5) != 0) ? FALSE : TRUE;
    BOOL fComputeSha1 = ((supp_flags & FLAG_NO_COMPUTE_SHA1) != 0) ? FALSE : TRUE;
    BOOL fComputeSha256 = ((supp_flags & FLAG_NO_COMPUTE_SHA256) != 0) ? FALSE : TRUE;
    H_ERROR_INFO hei=NULL;
    DFSFEATUREPARAM DfsFeatureParam;

    if ((errText != NULL) && (dwErrTextSize != 0))
        *errText='\0';

   DfsFeatureParam.fComputeMd5 = fComputeMd5;
   DfsFeatureParam.fComputeSha1 = fComputeSha1;
   DfsFeatureParam.fComputeSha256 = fComputeSha256;
   DfsFeatureParam.fStripIdenticalBody = TRUE;
   int FirstAsCompressed=0;
   if (FirstFilename!=NULL)
       if ((*FirstFilename)=='\0')
           FirstFilename=NULL;

   if (FirstFilename==NULL)
       {
           FirstFilename=SecondFileName;
           SecondFileName=NULL;
           FirstAsCompressed=1;
       }

    ConvertAnsiToUnicode(DfsToBuild, szDfsFileName, MAX_PATH_LENGTH);
    if (FirstFilename!=NULL)
        ConvertAnsiToUnicode(FirstFilename, szFirstFileName, MAX_PATH_LENGTH);
    else
        szFirstFileName[0]=0;

    if (SecondFileName!=NULL)
        ConvertAnsiToUnicode(SecondFileName, szSecondFileName, MAX_PATH_LENGTH);
    else
        szSecondFileName[0]=0;

    ConvertFileNameAndPath(szFirstFileName,szFirstFileNameGeneric,MAX_PATH_LENGTH,FALSE);
    SplitFileNameAndPath(szFirstFileNameGeneric,
                          szFirstFileNamePath,MAX_PATH_LENGTH-1,
                          szFirstFileNameFileName,MAX_PATH_LENGTH-1,
                          TRUE);

    ConvertFileNameAndPath(szSecondFileName,szSecondFileNameGeneric,MAX_PATH_LENGTH,FALSE);
    SplitFileNameAndPath(szSecondFileNameGeneric,
                          szSecondFileNamePath,MAX_PATH_LENGTH-1,
                          szSecondFileNameFileName,MAX_PATH_LENGTH-1,
                          TRUE);

      DFSFILE DfsFile = NULL;
      DFSFILEINFOPARAM DfsFileParam;
      DfsFileParam.sizeStruct = sizeof(DfsFileParam);
      DfsFileParam.dfStatus = 0;
      DfsFileParam.dfStatus = DFS_NEWFILE | DFS_WRITABLE;
      DfsFileParam.filename = szDfsFileName;

      DfsFileOpen(&DfsFileParam, &DfsFile,&hei);

      ConvertOldDirectoryCommentStorage(DfsFile,NULL);
      SetDfsFeatureParam(DfsFile,&DfsFeatureParam);
      FILETOADD fta;

              fta.filename_ondisk = szFirstFileName;
              fta.filename_tostore = szFirstFileNameFileName;
              fta.filename_prevversionondisk = NULL;
              fta.hRamDifToFlushPatch = NULL;
              fta.dfPreviousVersionFilePosition = 0;
              fta.fForceRecopyPrevious = FALSE;
              fta.fIgnore = FALSE;
              fta.fForceDate = FALSE;
              fta.hAddTags = NULL;
              fta.pReserved = NULL;
              fta.dfForceRecopyOrRawCopySize = 0;
              fta.dfForceRecopyOrRawCopyCrc32 = 0;
              fta.fForceRecopyOrRawCopyMd5Present = FALSE;
              fta.fForceRecopyOrRawCopySha1Present = FALSE;
              fta.fForceRecopyOrRawCopySha256Present = FALSE;
              fta.fWritingRaw = FALSE;

              dfuLong32 dfNbFileToAdd=1;
              FILETOADD* pFileToAdd=&fta;

              dfwcharpc lpszVersionName=szFirstFileNameFileName;
              dfwcharpc lpszVersionComment=NULL;

              dfuLong32 dfErr ;
              dfErr = InsertDirectoryinDfsFile(DfsFile,
                                               (FirstAsCompressed==0) ? TYPEDIR_FILECRCONLY:TYPEDIR_FILEINSERTING_DEFLATE,
                                               dfNbFileToAdd, pFileToAdd, FALSE,
                                               lpszVersionName,
                                               lpszVersionComment,
                                               pCprParamToBuildFnc,
                                               ProgressCallBackCreatePatchHelper, NULL,
                                               &hei);

          if (szSecondFileName[0] != 0)
          {
              FILETOADD fta2;

              //            cprParam.dfBlockCalcSizeSearch=lValue;
              //  cprParam.dfNbHashBit=lValue;


              fta2.filename_ondisk = szSecondFileName;
              fta2.filename_tostore = szSecondFileNameFileName;
              fta2.filename_prevversionondisk = szFirstFileName;
              fta2.hRamDifToFlushPatch = NULL;
              fta2.dfPreviousVersionFilePosition = 0;
              fta2.fForceRecopyPrevious = FALSE;
              fta2.fIgnore = FALSE;
              fta2.fForceDate = FALSE;
              fta2.hAddTags = NULL;
              fta2.pReserved = NULL;
              fta2.dfForceRecopyOrRawCopySize = 0;
              fta2.dfForceRecopyOrRawCopyCrc32 = 0;
              fta2.fForceRecopyOrRawCopyMd5Present = FALSE;
              fta2.fForceRecopyOrRawCopySha1Present = FALSE;
              fta2.fForceRecopyOrRawCopySha256Present = FALSE;
              fta2.fWritingRaw = FALSE;


              dfwcharpc lpszVersionName2=szSecondFileNameFileName;
              dfwcharpc lpszVersionComment2=NULL;

              dfErr = InsertDirectoryinDfsFile(DfsFile,
                                               TYPEDIR_PATCHFROMPREVIOUS,
                                               dfNbFileToAdd, &fta2, FALSE,
                                               lpszVersionName2,
                                               lpszVersionComment2,
                                               pCprParamToBuildFnc,
                                               ProgressCallBackCreatePatchHelper, NULL,
                                               &hei);
          }

          DfsClose(DfsFile,&hei);


    if (hei!=NULL)
    {
        char errBufTxt[1024]="";
        int errBufSize=1024-1;

        if ((errText!=NULL) && (dwErrTextSize!=0))
            GetErrorText(hei,errText,dwErrTextSize);


        GetErrorText(hei,errBufTxt,errBufSize);
        //printf("%s\n",errBufTxt);
        ret=-1;
        FreeErrorInfoBlock(hei);
        hei=NULL;
    }

    return ret;
}


int SVFAPI BuildPatchFromTwoFileEx3(const char*DfsToBuild, const char*FirstFilename, const char*SecondFileName, char*errText, dfuLong32 dwErrTextSize,
  signed long CompressRatio, signed long HashBits, signed long BlockSize, signed long supp_flags)
{

  BOOL custCompression;
  COMPRESSIONPARAM cprParam;
  COMPRESSIONPARAM* pCprParamToBuildFnc;

  custCompression = (CompressRatio != -1) || (HashBits != -1) || (BlockSize != -1);


  InitDefaultCompressionParam(&cprParam);
  cprParam.dfPhysicalMemoryKB = GetPhysSizeKbUsable();

  if (CompressRatio != -1)
    cprParam.uZlibCompressRatio = (dfuLong32)CompressRatio;

  if (HashBits != -1)
    cprParam.dfNbHashBit = (dfuLong32)HashBits;

  if (BlockSize != -1)
    cprParam.dfBlockCalcSizeSearch = (dfuLong32)BlockSize;

  pCprParamToBuildFnc = custCompression ? (&cprParam) : NULL;
  return BuildPatchFromTwoFileExCprParam(DfsToBuild, FirstFilename, SecondFileName, errText, dwErrTextSize,
    pCprParamToBuildFnc, supp_flags);
}

static void FillErrorText(const char* errorTextOrg, char*errText, dfuLong32 dwErrTextSize)
{
    if ((errorTextOrg == NULL) || (dwErrTextSize == 0))
        return;

    if (errText == NULL)
    {
        *errText = '\0';
        return;
    }

    if (strlen(errText) >= dwErrTextSize)
    {
        *errText = '\0';
        return;
    }

    strcpy(errText, errorTextOrg);
}


int SVFAPI AppendPatch(const char* szDfsWork, const char* szDfsToAppend, char*errText, dfuLong32 dwErrTextSize)
{
    int ret = BUILDERR_ERR_RECOMPRESS;

    dfwchar szDfsWorkFileName[MAX_PATH_LENGTH];
    dfwchar szDfsToAppendFileName[MAX_PATH_LENGTH];



    if ((errText != NULL) && (dwErrTextSize != 0))
        *errText = '\0';

    if (!ConvertAnsiToUnicode(szDfsWork, szDfsWorkFileName, MAX_PATH_LENGTH))
    {
        FillErrorText("source file error", errText, dwErrTextSize);
        return BUILDERR_ERR_SOURCE_FILE_ERROR;
    }

    if (!ConvertAnsiToUnicode(szDfsToAppend, szDfsToAppendFileName, MAX_PATH_LENGTH))
    {
        return BUILDERR_ERR_DEST_FILE_ERROR;
    }

    {
        DFSFILE DfsFileWhereRead = NULL;
        DFSFILEINFOPARAM DfsFileParamWhereRead;
        DfsFileParamWhereRead.sizeStruct = sizeof(DfsFileParamWhereRead);
        DfsFileParamWhereRead.dfStatus = DFS_READABLE;


        DFSFILE DfsFile = NULL;
        DFSFILEINFOPARAM DfsFileParam;
        DfsFileParam.sizeStruct = sizeof(DfsFileParam);
        //DfsFileParam.dfStatus = 0;
        DfsFileParam.dfStatus = DFS_WRITABLE;
        DfsFileParam.filename = szDfsWorkFileName;

        DfsFileOpen(&DfsFileParam, &DfsFile, NULL);

        /*
        if (!DfsFeatureParam.fStripIdenticalBody)
        SetDfsExtendedMode(DfsFile,TRUE);
        */


        if (DfsFile == NULL)
        {
            FillErrorText("source file error", errText, dwErrTextSize);
            return BUILDERR_ERR_SOURCE_FILE_ERROR;
        }


        ConvertOldDirectoryCommentStorage(DfsFile, NULL);
        {
            dfuLong32 dfNbDirWhereAdd = 0;
            dfuLong32 dfNbDirWhereRead = 0;
            PDIRINFO *pDirInfoWhereAdd = NULL;
            PDIRINFO *pDirInfoDfsFileWhereRead = NULL;
            DfsFileParamWhereRead.filename = szDfsToAppendFileName;
            DfsFileOpen(&DfsFileParamWhereRead, &DfsFileWhereRead, NULL);



            if (DfsFileWhereRead != NULL)
            {
                ConvertOldDirectoryCommentStorage(DfsFileWhereRead, NULL);
                pDirInfoWhereAdd = ReadAllDirInfo(DfsFile, &dfNbDirWhereAdd, READ_ALL_DIR, NULL);
                AdaptDfsFileFeature(DfsFile, (const PCDIRINFO*)pDirInfoWhereAdd, dfNbDirWhereAdd);
            }

            if (pDirInfoWhereAdd != NULL)
                pDirInfoDfsFileWhereRead = ReadAllDirInfo(DfsFileWhereRead, &dfNbDirWhereRead, READ_ALL_DIR, NULL);


            if (DfsFileWhereRead == NULL)
            {
                FillErrorText("source file error", errText, dwErrTextSize);
                DfsClose(DfsFile, NULL);
                return BUILDERR_ERR_SOURCE_FILE_ERROR;
            }
            else
                if ((pDirInfoWhereAdd == NULL) || (dfNbDirWhereAdd == 0))
                {
                FillErrorText("source file error", errText, dwErrTextSize);
                DfsClose(DfsFile, NULL);
                DfsClose(DfsFileWhereRead, NULL);
                return BUILDERR_ERR_SOURCE_FILE_ERROR;
                }
                else
                    if ((pDirInfoDfsFileWhereRead == NULL) || (dfNbDirWhereRead == 0))
                    {
                FillErrorText("source file error", errText, dwErrTextSize);
                DfsClose(DfsFile, NULL);
                DfsClose(DfsFileWhereRead, NULL);
                return BUILDERR_ERR_SOURCE_FILE_ERROR;
                    }
                    else
                    {
                        dfuLong32 i;

                        dfuLong32 dfError = DFS_SUCCESS;



                        for (i = 0; (i + 1<dfNbDirWhereRead); i++)
                        {
                            //PDIRINFO pDirInfoWhereReadTry = NULL;
                            dfuLong32 dfNbItemConversionMapList;
                            dfuLong32* pPositionConversionMapList;


                            dfNbItemConversionMapList = 0;

                            pPositionConversionMapList = GetPositionConversionMapList(*(pDirInfoDfsFileWhereRead + i),
                                *(pDirInfoWhereAdd + dfNbDirWhereAdd - 1), &dfNbItemConversionMapList);


                            if (pPositionConversionMapList != NULL)
                            {
                                //printf("version number %u of ", i);
                                ///DispOutUnicodeString(DfsFileParamWhereRead.filename);
                                //printf(" is compatible with latest version of ");
                                ///DispOutUnicodeString(DfsFileParam.filename);
                                //printf("\n");
                                // DoAppendDfs
                                dfError = DoAppendDfs(DfsFile,
                                    dfNbDirWhereAdd,/*
                                                    pDirInfoWhereAdd,*/
                                                    DfsFileWhereRead,
                                                    /*dfNbDirWhereRead,*/
                                                    (const PCDIRINFO*)pDirInfoDfsFileWhereRead,
                                                    i + 1,
                                                    dfNbDirWhereRead - (i + 1),
                                                    pPositionConversionMapList, dfNbItemConversionMapList,
                                                    NULL, NULL,
                                                    0, 0,
                                                    NULL);

                                DfsFree(pPositionConversionMapList);
                                if (dfError == DFS_SUCCESS)
                                    ret = 0;
                                break;
                            }
                        }
                        if ((i + 1) == dfNbDirWhereRead)
                            FillErrorText("no compatible version found", errText, dwErrTextSize);
                    }

            if (pDirInfoWhereAdd != NULL)
                FreeAllDirInfo(pDirInfoWhereAdd, dfNbDirWhereAdd);


            if (pDirInfoDfsFileWhereRead != NULL)
                FreeAllDirInfo(pDirInfoDfsFileWhereRead, dfNbDirWhereRead);


            if (DfsFileWhereRead != NULL)
            {
                DfsClose(DfsFileWhereRead, NULL);
                DfsFileWhereRead = NULL;
            }
        }
        DfsClose(DfsFile, NULL);
    }

    return ret;
}

int SVFAPI RecompressPatch(const char* DfsOrg, const char*DfsToBuild, char*errText, dfuLong32 dwErrTextSize,
    signed long CompressRatio, signed long HashBits, signed long BlockSize, signed long supp_flags)
{

    int ret = 0;
    dfwchar szDfsOrgFileName[MAX_PATH_LENGTH];
    dfwchar szDfsDestFileName[MAX_PATH_LENGTH];
    BOOL fComputeMd5 = ((supp_flags & FLAG_NO_COMPUTE_MD5) != 0) ? FALSE : TRUE;
    BOOL fComputeSha1 = ((supp_flags & FLAG_NO_COMPUTE_SHA1) != 0) ? FALSE : TRUE;
    BOOL fComputeSha256 = ((supp_flags & FLAG_NO_COMPUTE_SHA256) != 0) ? FALSE : TRUE;
    H_ERROR_INFO hei = NULL;
    BOOL custCompression;
    BOOL fStripIdentical = TRUE;
    BOOL fRawAccepted = FALSE;
    BOOL fReuseOldPatch = TRUE;
    COMPRESSIONPARAM cprParam;
    COMPRESSIONPARAM* pCprParamToBuildFnc;
    DFSFILEINFOPARAM DfsFileParam;
    DFSFEATUREPARAM DfsFeatureParam;
    DFSFILE DfsFile = NULL;
    DFSFILE DfsFileOut = NULL;
    dfuLong32 dfNumDir;
    dfuLong32 dfNbDir = 0;
    BOOL* lpVersionMap = NULL;
    PDIRINFO* pDirInfo = NULL;
    BOOL fFirstVersionAsReferenceNewDfs = TRUE;
    BOOL fRet = TRUE;
    dfuLong32 dfError = 0;
    int retValue = 0;
    BOOL fIncludeAllVersion = FALSE;

    if ((errText != NULL) && (dwErrTextSize != 0))
        *errText = '\0';
    custCompression = (CompressRatio != -1) || (HashBits != -1) || (BlockSize != -1);


    InitDefaultCompressionParam(&cprParam);
    cprParam.dfPhysicalMemoryKB = GetPhysicalMemoryKb() / 4;

    if (CompressRatio != -1)
        cprParam.uZlibCompressRatio = (dfuLong32)CompressRatio;

    if (HashBits != -1)
        cprParam.dfNbHashBit = (dfuLong32)HashBits;

    if (BlockSize != -1)
        cprParam.dfBlockCalcSizeSearch = (dfuLong32)BlockSize;

    pCprParamToBuildFnc = custCompression ? (&cprParam) : NULL;


    DfsFeatureParam.fComputeMd5 = fComputeMd5;
    DfsFeatureParam.fComputeSha1 = fComputeSha1;
	DfsFeatureParam.fComputeSha256 = fComputeSha256;
    DfsFeatureParam.fStripIdenticalBody = TRUE;




    if (!ConvertAnsiToUnicode(DfsOrg, szDfsOrgFileName, MAX_PATH_LENGTH))
    {
        FillErrorText("source file error", errText, dwErrTextSize);
        return BUILDERR_ERR_SOURCE_FILE_ERROR;
    }

    if (!ConvertAnsiToUnicode(DfsToBuild, szDfsDestFileName, MAX_PATH_LENGTH))
    {
        return BUILDERR_ERR_DEST_FILE_ERROR;
    }



    cprParam.dfPhysicalMemoryKB = GetPhysicalMemoryKb() / 4;

    DfsFileParam.sizeStruct = sizeof(DfsFileParam);
    DfsFileParam.dfStatus = DFS_READABLE;

    if (szDfsOrgFileName != NULL)
    {
        DfsFileParam.filename = szDfsOrgFileName;
        if (DfsFileOpen(&DfsFileParam, &DfsFile, NULL) != DFS_SUCCESS)
        {
            FillErrorText("source file error", errText, dwErrTextSize);
            return BUILDERR_ERR_SOURCE_FILE_ERROR;
        }
    }


    ConvertOldDirectoryCommentStorage(DfsFile, NULL);
    DfsGetNbDir(DfsFile, &dfNbDir, NULL);

    lpVersionMap = (BOOL *)DfsMalloc(sizeof(BOOL) * (dfNbDir + 1));


    pDirInfo = ReadAllDirInfo(DfsFile, &dfNbDir, READ_ALL_DIR, NULL);

    if (dfNbDir>0)
        fFirstVersionAsReferenceNewDfs = ((*pDirInfo)->dfTypeDir == TYPEDIR_FILECRCONLY);

    for (dfNumDir = 0; dfNumDir < dfNbDir; dfNumDir++)
        *(lpVersionMap + dfNumDir) = fIncludeAllVersion ? TRUE : FALSE;
    *(lpVersionMap + 0) = TRUE;
    if (dfNbDir>0)
        *(lpVersionMap + dfNbDir - 1) = TRUE;

    AdaptDfsFileFeature(DfsFile, (const PCDIRINFO *)pDirInfo, dfNbDir);
    if (pDirInfo == NULL)
        fRet = FALSE;




    if (fRet)
    {
        VERSIONTOADD_REMIX *pVersionRemix;
        dfuLong32 dwNbMapVersionMap = dfNbDir;
        dfuLong32 i, j, dfNbVersionRemix;
        BOOL fBaseDirectorySelected = FALSE;
        dfwcharp szDfsBaseDirectory = NULL;


        pVersionRemix =
            (VERSIONTOADD_REMIX *)DfsMalloc(sizeof(VERSIONTOADD_REMIX) * (dwNbMapVersionMap + 1));
        dfNbVersionRemix = 0;
        for (i = 0; i < dwNbMapVersionMap; i++)
            if (*(lpVersionMap + i))
            {
            VERSIONTOADD_REMIX *pFileCopyInfoCur = pVersionRemix + dfNbVersionRemix;
            PDIRINFO pDirOrg = *(pDirInfo + i);
            pFileCopyInfoCur->dfNumVersionPreviousSvf = i;
            pFileCopyInfoCur->dfNbPreviousFileInMask = pDirOrg->dfNbFile;
            pFileCopyInfoCur->dfNbFileToAdd = 0;
            pFileCopyInfoCur->pFileCopyInfo = (FILETOCOPYINFO_REMIX *)
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


        dfError = DoReMixDfsEx(DfsFile, dfNbDir, (const PCDIRINFO *)pDirInfo, &DfsFileOut,
            szDfsDestFileName,
            &DfsFeatureParam,

            //fBaseDirectorySelected, dfBaseDirNum, wchBaseDirectory,
            fBaseDirectorySelected, 0, fBaseDirectorySelected ? szDfsBaseDirectory : NULL, NULL,

            dfNbVersionRemix, pVersionRemix,

            //fFirstVersionAsReference, fReuseOldPatch,        // future
            fFirstVersionAsReferenceNewDfs, fReuseOldPatch, fRawAccepted,

            &cprParam, NULL, NULL, 0, 100, &hei);
        //pCprParam, pSetExtractPosCallBack, dfUserPtr, dwMinProgress, dwMaxProgress, NULL);

        if (dfError != DFS_SUCCESS)
            fRet = FALSE;

        {
            dfuLong32 i;
            for (i = 0; i < dfNbVersionRemix; i++)
            {
                FILETOCOPYINFO_REMIX *pFileCopyInfo = (pVersionRemix + i)->pFileCopyInfo;
                DfsFree(pFileCopyInfo);
            }
        }
        DfsFree(pVersionRemix);
    }

    FreeAllDirInfo(pDirInfo, dfNbDir);
    pDirInfo = NULL;

    DfsFree(lpVersionMap);
    DfsClose(DfsFile, NULL);
    DfsClose(DfsFileOut, NULL);


    if (hei != NULL)
    {
        GetErrorText(hei, errText, dwErrTextSize);
        retValue = (int)GetErrorNumber(hei);
    }
    FreeErrorInfoBlock(hei);

    int altRetValue = ((retValue == 0) ? BUILDERR_ERR_RECOMPRESS : retValue);
    int retFinalValue = fRet ? 0 : altRetValue;

    return retFinalValue;
}

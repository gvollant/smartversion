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
#include <stdio.h>

#include "../lib/engine/patchstream/common/difbasic.h"
#include "../lib/engine/svfile/common/DfsMFile.h"
#include "../lib/engine/patchstream/common/DfsTlTyp.h"
#include "../lib/engine/patchstream/common/difstool.h"
#include "../lib/engine/svfile/common/DfsStruc.h"
#include "../lib/engine/svfile/common/DfsTagDf.h"
#include "../lib/engine/svfile/common/DfsTagMg.h"
#include "../lib/engine/svfile/common/DfsTagBlockFloatEnd.h"

#include "../lib/engine/patchstream/common/difstrm.h"

#include "../lib/engine/patchstream/common/DfsOrigMemoryMap.h"
#include "../lib/engine/patchstream/rebuild/RamDifWk.h"

#include "../lib/engine/patchstream/compress/makdifst.h"

#include "../lib/engine/svfile/common/DfsIntf.h"
#include "../lib/engine/svfile/common/DfsIntfL.h"
#include "../lib/engine/svfile/common/DfsSet.h"
#include "../lib/engine/svfile/compress/DfsWrSet.h"
#include "../lib/engine/svfile/decompress/DfsRdSet.h"

#include "../cli/DfsCdLin.h"

#include "../lib/engine/svfile/common/DirSet.h"
#include "../lib/engine/svfile/decompress/DoExtracting.h"
#include "../lib/engine/svfile/rebuild/ReMixDfs.h"


#if defined(SMARTVERSION_USE_WIN32)

#include <windows.h>


#define tryCall(fnc) \
                { dfuLong32 dfErrorTC=fnc; \
                  if (dfErrorTC!=DFS_SUCCESS) printf("TC:**error %u %s\n",dfErrorTC,#fnc); else printf("TC:success %s\n",#fnc); }


#define tryCallcp(fnc,vcp) \
                { dfuLong32 dfErrorTC=fnc; \
                  if (dfErrorTC!=DFS_SUCCESS) printf("TC:**error %u %s\n",dfErrorTC,#fnc); else printf("TC:success %s\n",#fnc); \
                  *(vcp)=dfErrorTC;}

/*
#define tryCallcp(fnc,vcp) \
                { dfuLong32 dfErrorTC=fnc; \
                    *(vcp)=dfErrorTC ; \
                  if (dfErrorTC!=DFS_SUCCESS) printf("TC:**error %u %s\n",dfErrorTC,#fnc); else printf("TC:success %s\n",#fnc); }
*/
void TestBuildDfs()
{
  DFSFILEINFOPARAM DfsFileParam;
  DFSFILE DfsFile;
  dfuLong32 dfBlockType;
  dfuLong32 dfError;




  DfsFileParam.sizeStruct = sizeof(DfsFileParam);
  DfsFileParam.dfStatus = DFS_NEWFILE | DFS_WRITABLE;
  DfsFileParam.filename = (L"c:\\avir_bld.bin");

  //DeleteFile(DfsFileParam.filename);
  SetFileAttributesW(DfsFileParam.filename, FILE_ATTRIBUTE_NORMAL);

  printf("**********************************************************\n");
  printf("begin write %ws\n", (LPCWSTR)DfsFileParam.filename);

  tryCall(DfsFileOpen(&DfsFileParam, &DfsFile,NULL));
  printf("      Open result=%s\n",(DfsFile==NULL) ? "(null)" : "handle");

  {
    FILETOADD FileToAdd[] = { {(L"Config.sys"), (L"c:\\cOnfig.sys"), NULL, 0}
    ,
    {(L"boot.ini"), (L"c:\\boot.iNi"), NULL, 0}
    ,
    {(L"unimdm.fra"), (L"c:\\gil\\unimdm.fra"), NULL, 0}
    ,
    {(L"unimdm.bin"), (L"c:\\gil\\unimdm.fra"), (L"c:\\gil\\unimdm.usa"), 3}
    };
    //tryCall(InsertDirectoryinDfsFile(DfsFile,TYPEDIR_FILEINSERTING_DEFLATE,

    tryCall(InsertDirectoryinDfsFile
            (DfsFile, TYPEDIR_FILECRCONLY, 2, &FileToAdd[0], FALSE, NULL, NULL, NULL, NULL, NULL, NULL));
    tryCall(InsertDirectoryinDfsFile
            (DfsFile, TYPEDIR_FILEINSERTING_DEFLATE, 2, &FileToAdd[0], FALSE, NULL, NULL,
             NULL, NULL, NULL, NULL));
    tryCall(InsertDirectoryinDfsFile
            (DfsFile, TYPEDIR_FILEINSERTING_STORE, 2, &FileToAdd[0], FALSE, NULL, NULL,
             NULL, NULL, NULL, NULL));
    tryCall(InsertDirectoryinDfsFile
            (DfsFile, TYPEDIR_PATCHFROMPREVIOUS, 4, &FileToAdd[0], FALSE, NULL, NULL,
             NULL, NULL, NULL, NULL));
  }
  tryCall(DfsClose(DfsFile,NULL));

  printf("end write %ws\n\n", (LPCWSTR)DfsFileParam.filename);
  printf("**********************************************************\n");




/*********************************************************************************/


  {
    FILETOADD FileToAdd2[] = {
      {(L"winimage.exe"), (L"l:\\difstrm\\tst\\v2\\winimage.exe"), NULL, 0}
      ,
      {(L"winimaus.hlp"), (L"l:\\difstrm\\tst\\v2\\winimaus.hlp"), NULL, 1}
    };

    FILETOADD FileToAdd3[] = {
      {(L"winimage.exe"), (L"l:\\difstrm\\tst\\v3\\winimage.exe"),
       (L"l:\\difstrm\\tst\\v2\\winimage.exe"), 0}
      ,
      {(L"winimaus.hlp"), (L"l:\\difstrm\\tst\\v3\\winimaus.hlp"),
       (L"l:\\difstrm\\tst\\v2\\winimaus.hlp"), 1}
    };

    FILETOADD FileToAdd4[] = {
      {(L"winimage.exe"), (L"l:\\difstrm\\tst\\v4\\winimage.exe"),
       (L"l:\\difstrm\\tst\\v3\\winimage.exe"), 0}
      ,
      {(L"winimaus.hlp"), (L"l:\\difstrm\\tst\\v4\\winimaus.hlp"),
       (L"l:\\difstrm\\tst\\v3\\winimaus.hlp"), 1}
    };

    FILETOADD FileToAdd5[] = {
      {(L"winimage.exe"), (L"l:\\difstrm\\tst\\v5\\winimage.exe"),
       (L"l:\\difstrm\\tst\\v4\\winimage.exe"), 0}
      ,
      {(L"winimaus.hlp"), (L"l:\\difstrm\\tst\\v5\\winimaus.hlp"),
       (L"l:\\difstrm\\tst\\v4\\winimaus.hlp"), 1}
    };

    FILETOADD FileToAdd6[] = {
      {(L"winimage.exe"), (L"l:\\difstrm\\tst\\v5009\\winimage.exe"),
       (L"l:\\difstrm\\tst\\v5\\winimage.exe"), 0}
      ,
      {(L"winimaus.hlp"), (L"l:\\difstrm\\tst\\v5009\\winimaus.hlp"),
       (L"l:\\difstrm\\tst\\v5\\winimaus.hlp"), 1}
    };

    DfsFileParam.sizeStruct = sizeof(DfsFileParam);
    DfsFileParam.dfStatus = DFS_NEWFILE | DFS_WRITABLE;
    DfsFileParam.filename = (L"l:\\difstrm\\tst\\vercr.svf");
    printf("begin write %ws\n", (LPCWSTR)DfsFileParam.filename);

    tryCall(DfsFileOpen(&DfsFileParam, &DfsFile,NULL));
    printf("      Open result=%s\n",(DfsFile==NULL) ? "(null)" : "handle");

    tryCall(InsertDirectoryinDfsFile
            (DfsFile, TYPEDIR_FILECRCONLY, 2, &FileToAdd2[0], FALSE, NULL,
             NULL, NULL, NULL, NULL, NULL));
    tryCall(InsertDirectoryinDfsFile
            (DfsFile, TYPEDIR_PATCHFROMPREVIOUS, 2, &FileToAdd3[0], FALSE, NULL,
             NULL, NULL, NULL, NULL, NULL));
    tryCall(InsertDirectoryinDfsFile
            (DfsFile, TYPEDIR_PATCHFROMPREVIOUS, 2, &FileToAdd4[0], FALSE, NULL,
             NULL, NULL, NULL, NULL, NULL));
    tryCall(InsertDirectoryinDfsFile
            (DfsFile, TYPEDIR_PATCHFROMPREVIOUS, 2, &FileToAdd5[0], FALSE, NULL,
             NULL, NULL, NULL, NULL, NULL));
    tryCall(InsertDirectoryinDfsFile
            (DfsFile, TYPEDIR_PATCHFROMPREVIOUS, 2, &FileToAdd6[0], FALSE, NULL,
             NULL, NULL, NULL, NULL, NULL));
    tryCall(DfsClose(DfsFile,NULL));

    printf("end write %ws\n\n", (LPCWSTR)DfsFileParam.filename);

    DfsFileParam.sizeStruct = sizeof(DfsFileParam);
    DfsFileParam.dfStatus = DFS_NEWFILE | DFS_WRITABLE;
    DfsFileParam.filename = (L"l:\\difstrm\\tst\\vercz.svf");
    printf("begin write %ws\n", (LPCWSTR)DfsFileParam.filename);

    tryCall(DfsFileOpen(&DfsFileParam, &DfsFile, NULL));
    printf("      Open result=%s\n",(DfsFile==NULL) ? "(null)" : "handle");

    tryCall(InsertDirectoryinDfsFile
            (DfsFile, TYPEDIR_FILEINSERTING_DEFLATE, 2, &FileToAdd2[0], FALSE, NULL,
             NULL, NULL, NULL, NULL, NULL));
    tryCall(InsertDirectoryinDfsFile
            (DfsFile, TYPEDIR_PATCHFROMPREVIOUS, 2, &FileToAdd3[0], FALSE, NULL,
             NULL, NULL, NULL, NULL, NULL));
    tryCall(InsertDirectoryinDfsFile
            (DfsFile, TYPEDIR_PATCHFROMPREVIOUS, 2, &FileToAdd4[0], FALSE, NULL,
             NULL, NULL, NULL, NULL, NULL));
    tryCall(InsertDirectoryinDfsFile
            (DfsFile, TYPEDIR_PATCHFROMPREVIOUS, 2, &FileToAdd5[0], FALSE, NULL,
             NULL, NULL, NULL, NULL, NULL));
    tryCall(InsertDirectoryinDfsFile
            (DfsFile, TYPEDIR_PATCHFROMPREVIOUS, 2, &FileToAdd6[0], FALSE, NULL,
             NULL, NULL, NULL, NULL, NULL));
    tryCall(DfsClose(DfsFile,NULL));

/*
    DfsFileParam.sizeStruct = sizeof(DfsFileParam);
    DfsFileParam.dfStatus = DFS_NEWFILE | DFS_WRITABLE;
    DfsFileParam.filename = (L"l:\\difstrm\\tst\\verczhlp.svf");
    printf("begin write %s\n", DfsFileParam.filename);

    tryCall(DfsFileOpen(&DfsFileParam, &DfsFile));
    printf("      Open result=%x\n", DfsFile);

    tryCall(InsertDirectoryinDfsFile
            (DfsFile, TYPEDIR_FILEINSERTING_DEFLATE, 1, &FileToAdd2[1], NULL,
             NULL, NULL, NULL));
    tryCall(InsertDirectoryinDfsFile
            (DfsFile, TYPEDIR_PATCHFROMPREVIOUS, 1, &FileToAdd3[1], NULL,
             NULL, NULL, NULL));
    tryCall(InsertDirectoryinDfsFile
            (DfsFile, TYPEDIR_PATCHFROMPREVIOUS, 1, &FileToAdd4[1], NULL,
             NULL, NULL, NULL));
    tryCall(InsertDirectoryinDfsFile
            (DfsFile, TYPEDIR_PATCHFROMPREVIOUS, 1, &FileToAdd5[1], NULL,
             NULL, NULL, NULL));
    tryCall(InsertDirectoryinDfsFile
            (DfsFile, TYPEDIR_PATCHFROMPREVIOUS, 1, &FileToAdd6[1], NULL,
             NULL, NULL, NULL));
    tryCall(DfsClose(DfsFile));
*/

    printf("end write %ws\n\n", (LPCWSTR)DfsFileParam.filename);

    printf("**********************************************************\n");
  }

/*********************************************************************************/


  DfsFileParam.sizeStruct = sizeof(DfsFileParam);
  DfsFileParam.dfStatus = DFS_READABLE;
  DfsFileParam.filename = (L"c:\\avir_bld_old.bin");
  DfsFileParam.filename = (L"c:\\avir_bld.bin");

  printf("\nbegin read %ws\n", (LPCWSTR)DfsFileParam.filename);
  tryCall(DfsFileOpen(&DfsFileParam, &DfsFile,NULL));

  //DfsRGetNextBlockType(DfsFile, &dfBlockType);

  /////////tryCall(DfsGotoDir(DfsFile, 1));


  tryCallcp(DfsRGetNextBlockType(DfsFile, &dfBlockType,NULL), &dfError);
  printf("BlockType = %u, err code=%u\n", dfBlockType, dfError);

  while ((dfError == DFS_SUCCESS) && (dfBlockType != BLOCKTYPE_LISTDIR))
  {
    switch (dfBlockType)
    {

    case BLOCKTYPE_FILE:
      {
        dfuLong64 dfUncompressedSize = 0;
        dfuLong64 dfSizeEncoded = 0;
        dfvoidp bufread;
        dfvoidp pTagBuf;
        dfuLong32 TagSize;
        DFTAGLIST DfTagList;

        tryCall(DfsROpenNextFileAndTagBeforeFile
                (DfsFile, NULL, &dfSizeEncoded, NULL, &DfTagList, NULL));

        printf("         size encoded=%lu", (unsigned long)dfSizeEncoded);
        //tryCall(DfsRReadFileEncoded(DfsFile,buf,9+2,NULL));
        if (GetTag(DfTagList, DFSTAG_FILENAME, &pTagBuf, &TagSize))
        {
          printf(" filename = '%ws' ", (LPCWSTR)pTagBuf);
        }
        printf("\n");

        CloseTagList(DfTagList);

        bufread = DfsMalloc((size_t)dfSizeEncoded + 1);
        //tryCall(DfsRReadFileEncoded(DfsFile, buf, 4 + 2, NULL));
        //tryCall(DfsRReadFileEncoded(DfsFile, buf, 5, NULL));
        tryCall(DfsRReadFileEncoded(DfsFile, bufread, (dfuLong32)dfSizeEncoded, NULL, NULL));
        DfsFree(bufread);

        tryCall(DfsRReadGetPostFile
                (DfsFile, &dfUncompressedSize, NULL, NULL, NULL));
      }
      break;

    case BLOCKTYPE_DIRINTRO:
      {
        dfuLong32 dfTypeDir = 0;
        tryCall(DfsRGetNextDirIntro(DfsFile, &dfTypeDir, NULL, NULL));
        printf("TypeDir = %u\n", dfTypeDir);
      }
      break;

    case BLOCKTYPE_DIR:
      {
        dfuLong32 dfTypeDir = 0;
        DFTAGLIST TagListDirCopy = NULL;
        DFTAGLIST *psTagListFileInDirCopy = NULL;
        dfuLong32 dfNbFile = 0;
        dfuLong32 i;

        tryCall(DfsRGetNextDir
                (DfsFile, &dfTypeDir, &TagListDirCopy, &dfNbFile,
                 &psTagListFileInDirCopy,NULL));

        printf("TypeDir = %u, Nbfile=%u\n\n", dfTypeDir, dfNbFile);
        for (i = 0; i < dfNbFile; i++)
        {
          dfvoidp TagBuf = NULL;
          dfuLong32 TagSize = 0;
          printf("file %u :", i);
          if (GetTag
              (*(psTagListFileInDirCopy + i), DFSTAG_FILENAME, &TagBuf,
               &TagSize))
          {
            printf("filename: '%ws'  ", (LPCWSTR)TagBuf);
          }
          if (GetTag
              (*(psTagListFileInDirCopy + i), DFSTAG_DATE, &TagBuf, &TagSize))
          {
            DFSTM dfsTm;
            ConvertDfsInfoDateToDfsTm((DFSINFODATE *) TagBuf, &dfsTm);
            printf("%02u/%02u/%04u %02u:%02u:%02u ", dfsTm.df_mday,
                   dfsTm.df_mon, dfsTm.df_year, dfsTm.df_hour,
                   dfsTm.df_min, dfsTm.df_sec);
          }
          if (GetTag
              (*(psTagListFileInDirCopy + i), DFSTAG_CRCINFO, &TagBuf,
               &TagSize))
          {
            const DFSCRCINFO_FULLSIZESTRUCTURE *pDfsCrcInfo = (DFSCRCINFO_FULLSIZESTRUCTURE *) TagBuf;
            DFSCRCINFOPARAM DfsCrcInfoParam;

            ConvertDfsCrcInfoToDfsCrcInfoParam(pDfsCrcInfo, &DfsCrcInfoParam);

            printf("crc (%lu,%lu) = 0x%lx ",
                   (unsigned long)DfsCrcInfoParam.dfBeginPos,
                   (unsigned long)DfsCrcInfoParam.dfEndPos, (unsigned long)DfsCrcInfoParam.dfCrc32Value);
            // need show MD5 MD5++
          }
          if (GetTag
              (*(psTagListFileInDirCopy + i), DFSTAG_FILEPOSPROPERTIES,
               &TagBuf, &TagSize))
          {
            const DFSFILEPOSPROPERTIES32 *pDfsFileProp =
              (DFSFILEPOSPROPERTIES32 *) TagBuf;

            printf("size Uncompres=%lu ",
                   ConvertuLongIntelToLong(pDfsFileProp->
                                           dfFileSizeContentUncompressed));
          }
          printf("\n");
        }
        tryCall(DfsRFreeAllDirTag(TagListDirCopy, psTagListFileInDirCopy,NULL));

      }
      break;

    case BLOCKTYPE_LISTDIR:
      printf("end\n");
      break;

    }


    tryCallcp(DfsRGetNextBlockType(DfsFile, &dfBlockType,NULL), &dfError);
    printf("BlockType = %u, err code=%u\n", dfBlockType, dfError);


  }
  tryCall(DfsClose(DfsFile,NULL));
  printf("work done\n");


/*************************************************************************/


  printf("////////////////////////////////////\n\n");
  {
    PDIRINFO pDirInfo=NULL;
    FILETOCHECK FileToCheck[] = { {(L"c:\\cOnfig.sys"), FALSE, FALSE}
    ,
    {(L"c:\\boot.iNi"), FALSE, FALSE}
    ,
    {(L"c:\\gil\\unimdm.fra"), FALSE, FALSE}
    ,
    {(L"c:\\gil\\unimdm.fra"), FALSE, FALSE}
    };

    FILETOEXTRACT FileToExtract2[] =
      { {(L"c:\\cOnfig.sys.ext"), NULL, FALSE, FALSE,FALSE}
    ,
    {(L"c:\\boot.iNi.ext"), NULL, FALSE, FALSE,FALSE}
    };

    FILETOEXTRACT FileToExtract3[] =
      { {(L"c:\\cOnfig.sys.ext3"), NULL, FALSE, FALSE}
    ,
    {(L"c:\\boot.iNi.ext3"), NULL, FALSE, FALSE}
    ,
    {(L"c:\\unimdmfra.ext3"), NULL, FALSE, FALSE}
    ,
    {(L"c:\\unimdmfra.dif.ext3"), (L"c:\\gil\\unimdm.usa"), FALSE, FALSE}
    };

    DfsFileParam.sizeStruct = sizeof(DfsFileParam);
    DfsFileParam.dfStatus = DFS_READABLE;
    DfsFileParam.filename = (L"c:\\avir_bld_old.bin");
    DfsFileParam.filename = (L"c:\\avir_bld.bin");

    printf("\nbegin read %ws\n", (LPCWSTR)DfsFileParam.filename);
    tryCall(DfsFileOpen(&DfsFileParam, &DfsFile,NULL));
    tryCall(CheckDirectoryCrcWithRealFileSet
            (DfsFile, 3, 4, FileToCheck, NULL, NULL, NULL));
    {
      dfuLong32 i;
      for (i = 0; i < 4; i++)
        printf("check %ws : %s\n", FileToCheck[i].filename_ondisk_tocompare,
               FileToCheck[i].fIsIdentical ? "TRUE" : "FALSE");
    }

    //printf("\n\nExtract 2 <--\n");
    //tryCall(ExtractDirectory(DfsFile, 2, 2, FileToExtract2, NULL, NULL));
    printf("\n\nExtract 3 <--\n");

    tryCall(ReadDirectoryInfo(DfsFile, 3, (&pDirInfo) , NULL, NULL, NULL));

    tryCall(ExtractDirectory(DfsFile, 3, pDirInfo,4, FileToExtract3, NULL, NULL, NULL, NULL, FALSE, NULL));
    if (pDirInfo!=NULL)
              FreeDirectoryInfo((&pDirInfo), NULL);

    {
      dfuLong32 i;
      for (i = 0; i < 4; i++)
        printf("check %ws : %s\n", FileToExtract3[i].filename_ondisk_to_write,
               FileToExtract3[i].fCorrectlyDone ? "TRUE" : "FALSE");
    }
    // CheckDirectoryCrcWithRealFileSet
    tryCall(DfsClose(DfsFile,NULL));
  }
  printf("work done\n");
  PrintDfsList((L"c:\\avir_bld.bin"),VALUE_UNKNOWN,VALUE_UNKNOWN);
}


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/




/*

typedef struct
{
  dfwcharpc FileName;
  dfuLong32 dfSize;
  dfuLong32 dfCrc32;
  dfuLong32 dfFileEncodedSize;
  BOOL fCrc32Filled;
  BOOL fDateFilled;
  DFSTM dfsTm;
} FILEINDIRINFO;

typedef struct
{
  dfuLong32 dfNumDir;
  dfuLong32 dfNbFile;
  dfuLong32 dfTypeDir;
  FILEINDIRINFO* pFileInDirInfo;
  DFTAGLIST TagDir;
  DFTAGLIST*TagFile;
} DIRINFO;
typedef DIRINFO* PDIRINFO;

typedef struct
{
    dfuLong32 dfNumVersionPreviousSvf;

    dfuLong32 dfNbPreviousFileInMask;
    FILETOCOPYINFO_REMIX* pFileCopyInfo;

    dfuLong32 dfNbFileToAdd;
    FILETOADD_REMIX * pfta;
} VERSIONTOADD_REMIX;

*/

void FillOneVersionFullCopy(VERSIONTOADD_REMIX * pVersionRemix,DIRINFO* pDirInfo)
{
    FILETOCOPYINFO_REMIX* pFileCopyInfo;
    dfuLong32 i;
    pVersionRemix->dfNbFileToAdd=0;
    pVersionRemix->pfta=NULL;
    pVersionRemix->dfNbPreviousFileInMask = pDirInfo->dfNbFile;

    pFileCopyInfo = (FILETOCOPYINFO_REMIX*)DfsMalloc(sizeof(FILETOCOPYINFO_REMIX)*(pVersionRemix->dfNbPreviousFileInMask+1));
    pVersionRemix->pFileCopyInfo=pFileCopyInfo;
    for (i=0;i<pVersionRemix->dfNbPreviousFileInMask;i++)
    {
        (pFileCopyInfo+i)->dfReferenceItem=FTCI_REFERENCE_UNMODIFIED;
        (pFileCopyInfo+i)->fIsReferenceInAddedFile=FALSE;
    }
}

void FreeOneVersionFullCopy(VERSIONTOADD_REMIX * pVersionRemix)
{
    DfsFree(pVersionRemix->pFileCopyInfo);
}

#if defined(_DEBUG) && (!defined (USE_SVF_DLL))
extern dfuLong32 dfTotalAlloc ;
extern dfuLong32 dfTotalNbAlloc ;
#else
static dfuLong32 dfTotalAlloc =0;
static dfuLong32 dfTotalNbAlloc =0;
#endif

BOOL TestRemix(dfuLong32 dfParam,dfwcharpc dffnOrig,dfwcharpc dffnDest,dfuLong32 dfnbVer,const dfuLong32 * pdfVerTab)
{
  DFSFILE DfsFileIn = NULL;
  DFSFILEINFOPARAM DfsFileParamIn;

  //DFSFILEINFOPARAM DfsFileParamOut;
  WCHAR swzoutput[MAX_PATH+1];
  dfwcharpc pswzoutput;

  PDIRINFO* pDirInfo=NULL;//(PDIRINFO*)DfsMalloc(sizeof(PDIRINFO)*(DfsFileAndInfo.dfNbDir+1));
  dfuLong32 dfNbDir;
  dfuLong32 dfNumDir;
  wsprintfW(swzoutput,L"Y:\\avir\\winimage_history_out_%u.svf",dfParam);
  pswzoutput = (dffnDest!=NULL) ?  dffnDest : swzoutput;

  DfsFileParamIn.sizeStruct = sizeof(DfsFileParamIn);
  DfsFileParamIn.dfStatus = DFS_READABLE;
  DfsFileParamIn.filename = (dffnOrig != NULL) ? dffnOrig : (L"Y:\\avir\\winimage_history_work.svf");
  tryCall(DfsFileOpen(&DfsFileParamIn, &DfsFileIn, NULL));

  printf("remix start param %u %ws to %ws\n",dfParam,DfsFileParamIn.filename,pswzoutput);

  if (dfnbVer>0)
  {
      dfuLong32 i;
      for (i=0;i<dfnbVer;i++)
        printf("    integrate version %u\n",*(pdfVerTab+i));
  }


  DfsGetNbDir(DfsFileIn, &dfNbDir, NULL);
  pDirInfo = (PDIRINFO*)DfsMalloc(sizeof(PDIRINFO)*(dfNbDir+1));

  for (dfNumDir = 0; dfNumDir < dfNbDir; dfNumDir++)
  {
      dfuLong32 dfError = DFS_SUCCESS;
      dfError = ReadDirectoryInfo(DfsFileIn, dfNumDir, (pDirInfo)+dfNumDir, NULL, NULL, NULL);
      if (dfNumDir > 0)
      {
          PDIRINFO pDirInfoPrev=*((pDirInfo)+dfNumDir-1);
          PDIRINFO pDirInfoCur=*((pDirInfo)+dfNumDir);

          FixIndenticalDifferentSizeInReadDirectoryInfo(pDirInfoPrev,pDirInfoCur);
      }

      if (dfError != DFS_SUCCESS)
      {
          //fRet = FALSE;
          break;
      }
  }



    {
    dfuLong32 dfError = DFS_SUCCESS;
    dfuLong32 dfBlockType = 0;
    dfuLong32 dfNbDir = 0;
    dfuLong32 dfNumDir;

    ConvertOldDirectoryCommentStorage(DfsFileIn,NULL);
    DfsGetNbDir(DfsFileIn, &dfNbDir, NULL);


    for (dfNumDir = 0; dfNumDir < dfNbDir; dfNumDir++)
        {
        dfuLong32 i = 0;
        PDIRINFO pDirInfo = NULL;
        dfuLong32 dfGetNbFile = 0;

        dfError = ReadDirectoryInfo(DfsFileIn, dfNumDir, &pDirInfo, NULL, NULL, NULL);
        if (pDirInfo != NULL)
            {
            dfGetNbFile = pDirInfo->dfNbFile;
            printf("\nDirectory Number %u, type %u\n", dfNumDir + 0,
                    (pDirInfo->dfTypeDir));

            printf
                ("Name                                Date   Time       Size    Packed  CRC-32\n"
                "--------------------------------- -------- -----  --------  -------- --------\n");

            while ((i < dfGetNbFile) && (dfError == DFS_SUCCESS))
                {
                dfvoidp TagBuf;
                dfuLong32 TagSize;
                {
                    printf("%-32ws  ", (pDirInfo->pFileInDirInfo + i)->FileName);
                }
                if (GetTag
                    (*(pDirInfo->TagFile + i), DFSTAG_DATE, &TagBuf, &TagSize))
                    {
                    DFSTM dfsTm;
                    ConvertDfsInfoDateToDfsTm((DFSINFODATE *) TagBuf, &dfsTm);
                    printf("%02u/%02u/%02u %02u:%02u ", dfsTm.df_mday,
                            dfsTm.df_mon, dfsTm.df_year % 100, dfsTm.df_hour,
                            dfsTm.df_min/*, dfsTm.df_sec*/);
                    }
                printf("%9lu %9lu ", (unsigned long)((pDirInfo->pFileInDirInfo + i)->dfSize),
                        (unsigned long)((pDirInfo->pFileInDirInfo + i)->dfFileEncodedSize));

                if ((pDirInfo->pFileInDirInfo + i)->fCrc32Filled)
                    printf("%08lx", (pDirInfo->pFileInDirInfo + i)->dfCrc32);
                printf("\n");

                i++;
                }


            }
            FreeDirectoryInfo(&pDirInfo,NULL);
        }

    }

  {
      dfuLong32 dfNbVersionRemix=0;
      VERSIONTOADD_REMIX * pVersionRemix=NULL;
      COMPRESSIONPARAM cprParam;
      dfuLong32 dfTotalAllocSav,dfTotalNbAllocSav;
      DFSFILE DfsFileOut = NULL;
      dfuLong32 dfErr;

      InitDefaultCompressionParam(&cprParam);
      cprParam.uZlibCompressRatio=1;
      cprParam.dfBlockCalcSizeSearch=28;

      if (dfnbVer>0)
      {
          dfuLong32 i;
          dfNbVersionRemix = dfnbVer;

          pVersionRemix = (VERSIONTOADD_REMIX*)DfsMalloc(sizeof(VERSIONTOADD_REMIX)*(dfNbVersionRemix+1));

          for (i=0;i<dfnbVer;i++)
          {
              (pVersionRemix+i)->dfNumVersionPreviousSvf=*(pdfVerTab+i);
              FillOneVersionFullCopy((pVersionRemix+i),*(pDirInfo+(pVersionRemix+i)->dfNumVersionPreviousSvf));
          }

          if (dfParam==5)
          {
              (((pVersionRemix+1)->pFileCopyInfo)+5)->dfReferenceItem = FTCI_REFERENCE_DELETE;
              (((pVersionRemix+2)->pFileCopyInfo)+7)->dfReferenceItem = FTCI_REFERENCE_DELETE;
          }
      }
      else
      if (dfParam!=0)
      {
        dfNbVersionRemix = 2;

        pVersionRemix = (VERSIONTOADD_REMIX*)DfsMalloc(sizeof(VERSIONTOADD_REMIX)*(dfNbVersionRemix+1));
        (pVersionRemix+0)->dfNumVersionPreviousSvf=0;
        (pVersionRemix+1)->dfNumVersionPreviousSvf=dfParam;
        FillOneVersionFullCopy((pVersionRemix+0),*(pDirInfo+(pVersionRemix+0)->dfNumVersionPreviousSvf));
        FillOneVersionFullCopy((pVersionRemix+1),*(pDirInfo+(pVersionRemix+1)->dfNumVersionPreviousSvf));
      }
      else
      {
        dfNbVersionRemix = 3;

        pVersionRemix = (VERSIONTOADD_REMIX*)DfsMalloc(sizeof(VERSIONTOADD_REMIX)*(dfNbVersionRemix+1));
        (pVersionRemix+0)->dfNumVersionPreviousSvf=0;
        (pVersionRemix+1)->dfNumVersionPreviousSvf=1;
        (pVersionRemix+2)->dfNumVersionPreviousSvf=4;

        FillOneVersionFullCopy((pVersionRemix+0),*(pDirInfo+(pVersionRemix+0)->dfNumVersionPreviousSvf));
        FillOneVersionFullCopy((pVersionRemix+1),*(pDirInfo+(pVersionRemix+1)->dfNumVersionPreviousSvf));
        FillOneVersionFullCopy((pVersionRemix+2),*(pDirInfo+(pVersionRemix+2)->dfNumVersionPreviousSvf));
      }

      dfTotalAllocSav=dfTotalAlloc;
      dfTotalNbAllocSav=dfTotalNbAlloc;

      dfErr=
        DoReMixDfsEx(DfsFileIn, dfNbDir, pDirInfo,
                    &DfsFileOut,pswzoutput,      // BOOL fZipFile,
                    FALSE,
                    FALSE,// fBaseDirectorySelected
                    0, //dfBaseDirNum
                    NULL, //wchBaseDirectory
                    NULL,
                    dfNbVersionRemix,
                    pVersionRemix,
                    FALSE, // fFirstVersionAsReference
                    TRUE, //BOOL fReuseOldPatch,  // future
                    TRUE, // BOOL fRawAccepted
                    &cprParam, //   const COMPRESSIONPARAM * pCprParam
                    NULL,//   tSetExtractPosCallBack pSetExtractPosCallBack
                    NULL,   //dfvoidp dfUserPtr
                    0,0, //dfuLong32 dwMinProgress,dfuLong32 dwMaxProgress
                    NULL);

      tryCall(DfsClose(DfsFileOut,NULL));
      printf("dfTotalAllocSav=%u,dfTotalNbAllocSav=%u \n",dfTotalAllocSav,dfTotalNbAllocSav); ;

      printf("dfTotalAlloc=%u,dfTotalNbAlloc=%u \n",dfTotalAlloc,dfTotalNbAlloc); ;

      {
          dfuLong32 i;
          for (i=0;i<dfNbVersionRemix;i++)
              FreeOneVersionFullCopy((pVersionRemix+i));
      }
      DfsFree(pVersionRemix);
  }

  for (dfNumDir = 0; dfNumDir < dfNbDir; dfNumDir++)
  {
      FreeDirectoryInfo(pDirInfo+dfNumDir,NULL);
  }
  DfsFree(pDirInfo);


  tryCall(DfsClose(DfsFileIn,NULL));

  printf("final: dfTotalAlloc=%u,dfTotalNbAlloc=%u \n",dfTotalAlloc,dfTotalNbAlloc);
  return TRUE;
}

#endif

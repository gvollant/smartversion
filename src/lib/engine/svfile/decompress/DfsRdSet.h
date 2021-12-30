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

/* DfsRdSet.h */
/* function to extract a version into SVF file, based on previous version */

#ifndef DFS_RD_SET_H_INCLUDED
#define DFS_RD_SET_H_INCLUDED


#if defined(__cplusplus) && (!defined(ALLINCPP))
extern "C" {
#endif


typedef struct
{
  dfwcharpc filename_ondisk_tocompare;
  dfwcharpc filename_archive;
  BOOL fIgnore;
  BOOL fIsIdentical;
}
FILETOCHECK;

/*
extracting mode :
1) apply patch from original file + patch in SVF -> result file
2) extracting raw patch file content from SVF

3) read raw patch from SVF, combine previous patch in RamDif -> RamDif
4) read raw patch from SVF, combine previous patch in RamDif -> raw patch file (only with writing tools)

5) apply patch from original file + latest patch in SVF AND combine previous patch in RamDif -> result file

*/

typedef enum
{
    KIND_EXTRACTING_APPLYSTREAMPATCH = 1,
    KIND_EXTRACTING_EXTRACTRAWSTREAMPATCHFILE,

    KIND_EXTRACTING_COMBINERAMDIF_TORAMDIF,
    KIND_EXTRACTING_COMBINERAMDIF_APPLY

    ,KIND_EXTRACTING_COMBINERAMDIF_BUILDSTREAMPATCHFILE

    ,KIND_EXTRACTING_COMBINERAMDIF_APPLY_INPLACE
    ,KIND_EXTRACTING_COMBINERAMDIF_APPLY_INPLACE_NOCHECKSUM
} KIND_EXTRACTING;



typedef struct
{
  dfwcharp filename_ondisk_to_write;
  dfwcharp filename_ondisk_previous_to_read;
  BOOL fTemporaryFile;
//  dfuLong64 dfuSizeProjected;
  BOOL fIgnore;
  //BOOL fRawExtractingg;
  // KindExtracting will replace fRawExtractingg
  KIND_EXTRACTING KindExtracting;
  //HRAMDIF hRamDifBaseCombine;
  //HRAMDIF hRamDifResult;
  HRAMDIF hRamDifWork;

  BOOL fCorrectlyDone;
  BOOL fSkipUserRequested;
  BOOL fSetNewDate;
  DFSINFODATE dfsInfoDate;
} FILETOEXTRACT;


typedef struct
{
  dfwcharpc FileName;
  dfuLong64 dfSize;
  dfuLong32 dfCrc32;
  dfuLong64 dfFileEncodedSize;
  BOOL fCrc32Filled;
  BOOL fDateFilled;
  DFSTM dfsTm;
  BOOL   fMd5Filled;
  dfbyte bMd5[16];
  BOOL   fSha1Filled;
  dfbyte bSha1[20];
  BOOL   fSha256Filled;
  dfbyte bSha256[32];
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
typedef const DIRINFO* PCDIRINFO;

dfuLong32 SVFAPI ReadDirectoryInfo(DFSFILE DfsFile, dfuLong32 dfNumDir,
                            PDIRINFO* ppDirInfo,
                            tProgressCallBack pProgressCallBack,
                            dfvoidp dfUserPtr,
                            H_ERROR_INFO* pei);

void SVFAPI FreeDirectoryInfo(PDIRINFO* ppDirInfo, H_ERROR_INFO * pei);

void SVFAPI FixIndenticalDifferentSizeInReadDirectoryInfo(const DIRINFO* pDirInfoPrev,PDIRINFO pDirInfoCur);

#define READ_ALL_DIR (0xffffffff)
PDIRINFO* SVFAPI ReadAllDirInfo(DFSFILE DfsFile,dfuLong32 *pdfNbDir,dfuLong32 dfLimitReadDir,dfuLong32 *pdfError);

void SVFAPI FreeAllDirInfo(PDIRINFO* pDirInfo,dfuLong32 dfNbDir);

typedef struct
{
  BOOL fStripIdenticalFound;
  BOOL fNoStripIdenticalFound;
  BOOL fMd5PresentFound;
  BOOL fMd5NoPresentFound;
  BOOL fSha1PresentFound;
  BOOL fSha1NoPresentFound;
  BOOL fSha256PresentFound;
  BOOL fSha256NoPresentFound;
} ANALYSE_DFS_FEATURE_USED;

void SVFAPI AnalyseDfsFeature(const PCDIRINFO* pDirInfo,dfuLong32 dfNbDir,ANALYSE_DFS_FEATURE_USED* pAnalyseDfsFeature);
BOOL SVFAPI AdaptDfsFeature(const PCDIRINFO* pDirInfo,dfuLong32 dfNbDir,DFSFEATUREPARAM* pDfsFeatureParam);
BOOL SVFAPI AdaptDfsFileFeature(DFSFILE DfsFile,const PCDIRINFO* pDirInfo,dfuLong32 dfNbDir);

BOOL SVFAPI ConvertOldDirectoryCommentStorage(DFSFILE DfsFile, H_ERROR_INFO * pei);

dfuLong32 SVFAPI CheckDirectoryCrcWithRealFileSet(DFSFILE DfsFile, dfuLong32 dfNumDir,
                                         dfuLong32 dfNbFile,
                                         FILETOCHECK * pFileToCheck,
                                         tProgressCallBack pProgressCallBack,
                                         dfvoidp dfUserPtr,
                                         H_ERROR_INFO* pei);

#define EXTR_WORK_EVENT_BEFORE_EXTRACTING_VERSION   (0x00001000)
#define EXTR_WORK_EVENT_BEFORE_EXTRACTING_FILE      (0x00002000)
#define EXTR_WORK_EVENT_EXTRACTING_FILE_FINISHED    (0x00003000)
#define EXTR_WORK_EVENT_EXTRACTING_VERSION_FINISHED (0x00004000)

typedef struct
{
    dfwcharpc dfFileNameOnDisk;
    BOOL fTempFile;
    dfuLong32 dfNumVersion;
    dfuLong32 dfNumFile;
    dfuLong32 dfSuccess;
} EXTR_WORK_EVENT_INFO;

typedef dfuLong32(DFSCALLBACK* tExtractingFileWorkingEvent)(dfuLong32 dfEvent,
                                                            const EXTR_WORK_EVENT_INFO* pEventPtr,
                                                            dfvoidp dfUserPtr);

dfuLong32 SVFAPI ExtractDirectory(DFSFILE DfsFile, dfuLong32 dfNumDir,
                           PCDIRINFO pDirInfo,
                           dfuLong32 dfNbFile,
                           FILETOEXTRACT * pFileToExtract,
                           tProgressCallBack pProgressCallBack,
                           dfvoidp dfUserPtr,
                           tExtractingFileWorkingEvent pExtractingFileWorkingEvent,
                           dfvoidp dfUserPtrWorkingEvent,
                           BOOL fFlushWrite,
                           H_ERROR_INFO* pei);

/*
typedef struct
{
    dfuLong32 dfTypeInserting;
    dfuLong32 dfNbFile;
} -*/



/*

#define DF_PROGRESS_EVENT_NEWDIR    (1)
#define DF_PROGRESS_EVENT_NEWFILE   (2)
#define DF_PROGRESS_EVENT_PROGRESS  (3)
*/

#define DF_NUMDIR_NOGOTO (0xffffffff)

#if defined(__cplusplus) && (!defined(ALLINCPP))
}
#endif

#endif

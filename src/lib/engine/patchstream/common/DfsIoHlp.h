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

/* DfsIOhlp.h */
/* Basic low level disk function, including memory mapped */

#ifndef DFSIOHLP_INCLUDED
#define DFSIOHLP_INCLUDED

#ifndef MAX_PATH_LENGTH
#define MAX_PATH_LENGTH (256+32)
#endif

#if defined(__cplusplus) && (!defined(ALLINCPP))
extern "C" {
#endif

typedef enum
{
  OPEN_READ,
  OPEN_READWRITE,
  OPEN_CREATE
}
TYPEOPEN;

typedef enum
{
  TYPESEEK_BEGIN,
  TYPESEEK_END
}
TYPESEEK;

typedef BOOL(DFSCALLBACK * tProgressCopyFileCallBack) (dfuLong64 dwPos,
                                                    dfuLong64 dwTotalSize,
                                                    dfvoidp dfUserPtr);

BOOL SVFAPI dfsInitVirtualFileNameSpace();
BOOL SVFAPI dfsSetVirtualFileNameMaximumMemory(BOOL fLimitSize,dfuLong64 dfMaxSize);
BOOL SVFAPI dfsUnInitVirtualFileNameSpace(BOOL fClearMaximumMemoryValue);

BOOL SVFAPI PerformInitAndMutexForMemFS();

DECLARE_DFHANDLE(LOWLEVELFILE);

BOOL SVFAPI GetCanonicalFileName(dfwcharpc FileName,
                          dfwcharp szBuffer, dfuLong32 dfBufSizeInWChar,
                          dfuLong32* pdfBufSizeNeededInWChar);


LOWLEVELFILE SVFAPI OpenLowLevel(dfwcharpc FileName, TYPEOPEN TypeOpen,
                          BOOL fTemporaryFile, BOOL fProjectedSize, dfuLong64 dfProjectedSize,
                          H_ERROR_INFO* pei);

LOWLEVELFILE memOpenOrBuildPermanentLowLevel(dfwcharpc FileName, TYPEOPEN TypeOpen,
                          BOOL fTemporaryFile, BOOL fProjectedSize, dfuLong64 dfProjectedSize,const void*bufPermanent,
                          H_ERROR_INFO * pei);

BOOL memDeleteFile(dfwcharpc lpFileName, H_ERROR_INFO* pei);

BOOL myRenameFile(dfwcharpc lpFileName, dfwcharpc lpszCopyFileName, H_ERROR_INFO* pei);


dfuLong32 SVFAPI LowLevelWrite(LOWLEVELFILE llfFile, const void *Buf, dfuLong32 size, H_ERROR_INFO* pei);

dfuLong32 SVFAPI LowLevelRead(LOWLEVELFILE llfFile, void *Buf, dfuLong32 size, H_ERROR_INFO* pei);


BOOL SVFAPI LowLevelSeek(LOWLEVELFILE llfFile, dfuLong32 PosLow, dfuLong32 PosHigh, TYPESEEK TypeSeek, H_ERROR_INFO* pei);

void SVFAPI LowLevelTell(LOWLEVELFILE llfFile, dfuLong32 *PosLow, dfuLong32 *PosHigh);

void SVFAPI LowLevelGetSize(LOWLEVELFILE llfFile, dfuLong32 *PosLow, dfuLong32 *PosHigh);

BOOL SVFAPI LowLevelSetFileSize(LOWLEVELFILE llfFile, dfuLong32 PosLow, dfuLong32 PosHigh, H_ERROR_INFO* pei);

BOOL SVFAPI LowLevelSeek64(LOWLEVELFILE llfFile, dfuLong64 Pos, TYPESEEK TypeSeek, H_ERROR_INFO* pei);

void SVFAPI LowLevelTell64(LOWLEVELFILE llfFile, dfuLong64 *Pos);

void SVFAPI LowLevelGetSize64(LOWLEVELFILE llfFile, dfuLong64 *Pos);

BOOL SVFAPI LowLevelSetFileSize64(LOWLEVELFILE llfFile, dfuLong64 Pos, H_ERROR_INFO* pei);

BOOL SVFAPI LowLevelClose(LOWLEVELFILE llfFile, H_ERROR_INFO* pei);

BOOL SVFAPI LowLevelCloseEx(LOWLEVELFILE llFile, BOOL fFlushWrite, H_ERROR_INFO* pei);

BOOL SVFAPI DeleteTempFile(dfwcharpc FileName, H_ERROR_INFO* pei);

BOOL SVFAPI CopyFileCutSizeEx(dfwcharpc lpExistingFileName,dfwcharpc lpNewFileName,BOOL bFailIfExists, H_ERROR_INFO * pei,
                     BOOL fTempWrite,BOOL fCutSize, BOOL fFlushWrite, BOOL* lpfErrorOnOpeningDest, dfuLong64 dfMaxSize,
                     tProgressCopyFileCallBack pProgressCopyFileCallBack, dfvoidp dfUserPtr);

BOOL SVFAPI CopyFileCutSize(dfwcharpc lpExistingFileName,dfwcharpc lpNewFileName,BOOL bFailIfExists, H_ERROR_INFO * pei,
                     BOOL fTempWrite,BOOL fCutSize, BOOL fFlushWrite, BOOL* lpfErrorOnOpeningDest, dfuLong64 dfMaxSize);

BOOL SVFAPI MyCopyFile(dfwcharpc lpExistingFileName, dfwcharpc lpNewFileName, BOOL bFailIfExists, BOOL fFlushWrite, H_ERROR_INFO* pei);
BOOL SVFAPI MyDeleteFile(dfwcharpc lpFileName, H_ERROR_INFO* pei);

BOOL SVFAPI SetTempDirectory(dfwcharpc lpDirectory);
BOOL SVFAPI GetTempDirectorySystem(dfwcharp lpDirectory,dfuLong32 dfSizeBufferinwChar);
BOOL GetTempDirectory(dfwcharp lpDirectory,dfuLong32 dfSizeBufferinwChar,BOOL fGiveDefaultIfNoPreset);

BOOL SVFAPI GetTemporaryFilename(dfwcharpc lpTreeLetter,dfwcharp lpBuffer, dfuLong32 dfSizeBufferinwChar,BOOL fTempMemPossible,dfuLong64 dfFileSizeProjected);


BOOL SVFAPI MyMkdir(dfwcharpc DirectoryName, H_ERROR_INFO* pei);

BOOL SVFAPI CheckIfFileSameA(const char*lpFile1,const char*lpFile2);
BOOL SVFAPI CheckIfFileSame(dfwcharpc lpFile1,dfwcharpc lpFile2);

dfwchar SVFAPI My_getch_console();
char SVFAPI My_getch_console_char();

/*
#ifdef WIN32
#ifdef FILE_ATTRIBUTE_NORMAL
HANDLE MyCreateFileW(dfwcharpc FileName,        // file name
                     DWORD dwDesiredAccess,     // access mode
                     DWORD dwShareMode, // share mode
                     LPSECURITY_ATTRIBUTES lpSecurityAttributes,        // SD
                     DWORD dwCreationDisposition,       // how to create
                     DWORD dwFlagsAndAttributes,        // file attributes
                     HANDLE hTemplateFile       // handle to template file
  );
#endif
#endif
*/

BOOL SVFAPI SplitFileNameAndPath(dfwcharpc dfFullPathName,
                          dfwcharp dfOnlyPath,dfuLong32 dfPathSize,
                          dfwcharp dfOnlyFileName,dfuLong32 dfFileNameSize,
                          BOOL fOperatingSystemSeparatorInPath);

BOOL SVFAPI ConvertFileNameAndPath(dfwcharpc dfFullPathName,
                          dfwcharp dfFullPathNameAdapted,dfuLong32 dfPathSize,
                          BOOL fConvertToOperatingSystem);

BOOL SVFAPI SvfGetFullPathName(dfwcharpc lpFileName,dfuLong32 nBufferLength,dfwcharp lpBuffer,dfwcharp *lpFilePart);

BOOL SVFAPI SvfGetTempFileName(dfwcharpc lpPathName,dfwcharpc lpPrefixString,dfuLong32 uUnique,dfwcharp lpTempFileName);

BOOL SVFAPI SvfMoveFile(dfwcharpc szTempFileName,dfwcharpc lpszCopyFileName);

/* #define FILE_SIZE_NOT_EXIST ((dfuLong64)0xffffffffffffffff) */
#define FILE_SIZE_NOT_EXIST ((((dfuLong64)0xffffffff)<<32) | (dfuLong64)0xffffffff)

dfuLong64 SVFAPI GetFileSizeByName(dfwcharpc FileName, DFSTM * pDfsTm, H_ERROR_INFO* pei);
BOOL SVFAPI ChangeDateTimeFile(dfwcharpc FileName, const DFSTM * pDfsTm);


BOOL SVFAPI LowLevelCloseChangeDateTime(LOWLEVELFILE llFile, BOOL fFlushWrite, const DFSTM* pDfsTm, BOOL* pDateTimeChanged, H_ERROR_INFO* pei);

DECLARE_DFHANDLE(HFILESEARCHING);

typedef struct
{
    dfwcharpc FileName;
    dfuLong64 FileSize;
    BOOL fIsDirectory;
} FILESEARCHITEMFOUND;
HFILESEARCHING SVFAPI InitFileSearching(dfwcharpc szWildcardSearch);
FILESEARCHITEMFOUND* SVFAPI GetNextItemContent(HFILESEARCHING);
BOOL SVFAPI CloseFileSearching(HFILESEARCHING);


DECLARE_DFHANDLE(HFILECONTENTREADBUF);

#define FILEFULLCONTENTBUFFER_ACCESSMASK 0x07
#define FILEFULLCONTENTBUFFER_READ 0x01
#define FILEFULLCONTENTBUFFER_WRITE 0x02
#define FILEFULLCONTENTBUFFER_READWRITE 0x03
#define FILEFULLCONTENTBUFFER_RESIZE 0x08
#define FILEFULLCONTENTBUFFER_PROGRESSIVE_FLUSH 0x10

BOOL SVFAPI GetFileFullContentBuffer(dfwcharpc FileName,dfuLong32 dfAccessFlag,dfuLong64 dfSizeRequested,
                                  HFILECONTENTREADBUF* hFileContentReadBuf,
                                  ORIGDATA* pOrg, H_ERROR_INFO* pei);

BOOL SVFAPI CloseFileFullContentBuffer(HFILECONTENTREADBUF);

BOOL SVFAPI CloseFileFullContentBufferEx(HFILECONTENTREADBUF,BOOL,H_ERROR_INFO*);

SYNC_DIF_MUTEX_OBJECT SVFAPI GetVirtualFileNameSpaceMutex();
BOOL SVFAPI SetVirtualFileNameSpaceMutex(SYNC_DIF_MUTEX_OBJECT SyncDifMutexSet,SYNC_DIF_MUTEX_OBJECT* pSyncDifMutexSetPrev,BOOL fTryClear,BOOL fClearMaximumMemoryValue);

/* */
#ifdef INVALID_HANDLE_VALUE
BOOL SVFAPI FillWinErrorFileName(dfwcharpc dfFileName,dfuLong32 dwErr,H_ERROR_INFO * pei);

HANDLE SVFAPI MyCreateFileW(dfwcharpc FileName,        // file name
                     DWORD dwDesiredAccess,     // access mode
                     DWORD dwShareMode, // share mode
                     LPSECURITY_ATTRIBUTES lpSecurityAttributes,        // SD
                     DWORD dwCreationDisposition,       // how to create
                     DWORD dwFlagsAndAttributes,        // file attributes
                     HANDLE hTemplateFile       // handle to template file
  );
#endif

#if defined(__cplusplus) && (!defined(ALLINCPP))
}
#endif

#endif

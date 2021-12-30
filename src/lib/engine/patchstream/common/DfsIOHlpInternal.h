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


/* only for DfsIoHlpW.c or posix */
BOOL IsFileNameForMemorySpace(const dfwchar* FileName);
BOOL ManualCopyFile(dfwcharpc lpExistingFileName,dfwcharpc lpNewFileName,BOOL bFailIfExists, BOOL fFlushWrite, H_ERROR_INFO * pei);

/* only in DfsIoHlpW.c or posix */
//


typedef struct
{
    dfuLong64 dfFileSize;
    dfuLong64 posLastWrite;
    dfwcharpc dfVirtualFileName;
    dfbytep   Buf;

    DFSTM*    pVirtualFileDate;

    BOOL      fIsPermanentReadOnlyBuffer;
    size_t    allocatedBufSize;
} VIRTUALFILEITEMINFO;

typedef struct
{
    VIRTUALFILEITEMINFO* pvfi;
    dfuLong64 pos;
} VIRTUALFILEITEMFILEOPEN;

typedef struct
{
    dfwcharp dfFileName;
    VIRTUALFILEITEMFILEOPEN* pvfio;
    union
    {
        const void* hlFile;
        int filedes;
    } u;
} LOWLEVELINTERNAL;




typedef struct
{
  BOOL fVirtualMemFile;
  BOOL fWritePossible;

  dfuLong64 dwSize;
  void* lpBuf;

  union
  {
      struct
      {
          void* hFile;
          void* hFileMap;
      } wh ;
      int filedes;
  } u;
  struct
  {
      void* allocatedInternalBufferData;
      void* allocatedInternalBufferDirty;
      dfuIntPtr allocatedInternalBufferSize;
      dfuIntPtr allocatedInternalBufferDirtySize;

      dfuIntPtr nbDirtyUnitPerGranul;
  } no_map;
  BOOL      fPartialMapping;
  dfuLong64 dfBeginMapPos;
  dfuIntPtr dfMapSize;
  BOOL      fProgressiveFlush;

  dfuLong32 dfGranularityMapping;
}
FILECONTENTREADBUFINTERNAL;


dfuLong32 diskLowLevelWrite(LOWLEVELFILE llFile, void const *Buf, dfuLong32 size, H_ERROR_INFO * pei);
BOOL diskLowLevelSetFileSize(LOWLEVELFILE llFile, dfuLong32 PosLow, dfuLong32 PosHigh, H_ERROR_INFO * pei);
dfuLong32 diskLowLevelRead(LOWLEVELFILE llFile, void *Buf, dfuLong32 size, H_ERROR_INFO* pei);
BOOL diskLowLevelSeek(LOWLEVELFILE llFile, dfuLong32 PosLow, dfuLong32 PosHigh, TYPESEEK TypeSeek, H_ERROR_INFO* pei);
void diskLowLevelTell(LOWLEVELFILE llFile, dfuLong32 *PosLow, dfuLong32 *PosHigh);
void diskLowLevelGetSize(LOWLEVELFILE llFile, dfuLong32 *PosLow, dfuLong32 *PosHigh);
BOOL diskLowLevelClose(LOWLEVELFILE llFile,BOOL fFlushWrite, H_ERROR_INFO* pei);
BOOL diskLowLevelCloseChangeDateTime(LOWLEVELFILE llFile, BOOL fFlushWrite, const DFSTM* pDfsTm, BOOL* pDateTimeChanged, H_ERROR_INFO* pei);
dfuLong64 diskGetFileSizeByName(dfwcharpc FileName, DFSTM * pDfsTm, H_ERROR_INFO* pei);

BOOL diskChangeDateTimeFile(dfwcharpc FileName, const DFSTM * pDfsTm);

LOWLEVELFILE diskOpenLowLevel(dfwcharpc FileName, TYPEOPEN TypeOpen,
                          BOOL fTemporaryFile, BOOL fProjectedSize, dfuLong64 dfProjectedSize,
                          H_ERROR_INFO * pei);

BOOL diskGetFileFullContentBuffer(dfwcharpc FileName,dfuLong32 dfAccessFlag,dfuLong64 dfSizeRequested,//BOOL fReadOnly,
                                  HFILECONTENTREADBUF* phFileContentReadBuf,
                                  ORIGDATA* pOrg,
                                  H_ERROR_INFO* pei);

BOOL diskCloseFileFullContentBuffer(FILECONTENTREADBUFINTERNAL *pFileContentReadBufInternal,BOOL fFlushWrite, H_ERROR_INFO* pei);





BOOL CheckIfVirtualFilePossible(dfuLong64 dfFileSize);
BOOL GetTempMemoryTmpFileName(dfwcharp dfFileName,dfuLong32 dfSizeFileName,dfuLong64 dfFileSize);

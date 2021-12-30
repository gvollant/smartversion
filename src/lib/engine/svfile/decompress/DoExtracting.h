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

/* DoExtracting.h */
/* Extract several version in one time */

#ifndef DO_EXTRACTING_H_INCLUDED
#define DO_EXTRACTING_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif


#ifndef T_SET_EXTRACT_POS_CALLBACK_DEFINED
#define T_SET_EXTRACT_POS_CALLBACK_DEFINED
typedef BOOL(DFSCALLBACK * tSetExtractPosCallBack) (dfuLong32 dwPos,
                                                    dfvoidp dfpAdditionnalInfo,
                                                    dfvoidp dfUserPtr);
#endif
BOOL DFSCALLBACK ProgressCallBackDoExtracting(PROGRESSCALLBACKINFO * pProgressCallBackInfo);

typedef dfuLong32(DFSCALLBACK * tConfirmBeforeCreatingFile)(dfwcharpc dfFileName,
                                                          dfvoidp dfpAdditionnalInfo,
                                                          dfvoidp dfUserPtr);


#define DFCBXTR_AFTERVERSIONEXTRACTED         (0x00000100)
#define DFCBXTR_BEFOREDELETEPREVIOUS          (0x00000101)


#define CONFIRM_BEFORE_CREATING_FILE_OK (0x0)
#define CONFIRM_BEFORE_CREATING_FILE_SKIP (0x1)
#define CONFIRM_BEFORE_CREATING_FILE_STOP (0x2)

typedef enum
{
    ExtractNone = 0,
    ExtractClassic,
    ExtractInPlace,
    ExtractInPlaceNoChecksum,
    ExtractByMerging,
} EXTRACTINGMAPITEM;

typedef struct
{
    dfuLong32 dfMapFileNumber;
    const EXTRACTINGMAPITEM*   pExtractingMap;
//    const BOOL*   pExtractingMap;
} EXTRACTINGMAPINFO;


typedef struct
{
  dfuLong32 dfSizeStruct;
  DFSFILE DfsFile;

  dfvoidp dfDataEventPtr;
  dfvoidp dfReserved;
} DFCBXTR_CALLBACKINFO;

BOOL SVFAPI DoMultiExtracting(DFSFILE DfsFile,dfwcharpc wchBaseDirExtract,
                              FILESET** ppfsDest,BOOL fTempDestExtr,dfuLong32 dfDirExtr,
                              const PDIRINFO* pDirInfo,
                              BOOL fBaseDirectorySelected,dfuLong32 dfBaseDirNum,dfwcharpc wchBaseDirectory,const FILESET* pFileSetBase,
                              dfuLong32 dwMapFileNumber,const EXTRACTINGMAPITEM* lpExtractingMap,
                              EXTRACTINGMAPINFO* pExtractingMapInfo,
                              tSetExtractPosCallBack pSetExtractPosCallBack,dfvoidp dfUserPtr,
                              tConfirmBeforeCreatingFile pConfirmBeforeCreatingFile, dfvoidp dfUserPtrBeforeCreatingFile,
                              tExtractingFileWorkingEvent pExtractingFileWorkingEvent, dfvoidp dfUserExtractingFileWorkingEvent,
                              dfuLong32 dwMinProgress,dfuLong32 dwMaxProgress,BOOL fFlatExtracting, BOOL fFlushWrite, H_ERROR_INFO * pei);





BOOL SVFAPI BuildZipFileFromFileSet(FILESET* pfsDest,dfwcharpc dfwcZipFileName,int opt_compress_level,
                             dfwcharp dfwVersionName,
                             tSetExtractPosCallBack pSetExtractPosCallBack,dfvoidp dfUserPtr,
                             dfuLong32 dwMinProgress,dfuLong32 dwMaxProgress, H_ERROR_INFO * pei);


/************************************************************************/

#ifdef __cplusplus
}
#endif

#endif

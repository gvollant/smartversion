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

/* DfsIntlL.h */
/* CPPPTST
#ifdef __cplusplus
extern "C" {
#endif
*/

dfuLong32 DfsWCloseFlushCurrentDir(DFSFILE DfsFile, H_ERROR_INFO * pei);

dfuLong32 DfsWCreateNewDir(DFSFILE DfsFile, dfuLong32 dfTypeDir, H_ERROR_INFO * pei);

dfuLong32 DfsWAddTagInDir(DFSFILE DfsFile, dfuLong32 TagNumber,
                        dfvoidpc TagBuf, dfuLong32 TagSize,
                        BOOL fWriteInDirIntroduction, H_ERROR_INFO * pei);

dfuLong32 DfsWCreateNewFileInDir(DFSFILE DfsFile, dfuLong32 dfContentStoreMethod,
                                 dfuLong64 dfContentUncompressedSize, H_ERROR_INFO * pei);

dfuLong32 DfsWAddTagInNewFileInDir(DFSFILE DfsFile, dfuLong32 TagNumber,
                                 dfvoidpc TagBuf, dfuLong32 TagSize,
                                 BOOL fStoreTagBeforeFile,
                                 BOOL fStoreTagInDirectory, H_ERROR_INFO * pei);

dfuLong32 DfsWQuickCloseWriteFileForStripped(DFSFILE DfsFile);

dfuLong32 DfsWWriteFileEncoded(DFSFILE DfsFile, const void *Buf, dfuLong32 Size, H_ERROR_INFO * pei);

dfuLong32 DfsWCloseFileInDir(DFSFILE DfsFile,
                           dfuLong64 dfFileContentUncompressedSize,
                           dfuLong32 CrcInfoNumber,
                           const DFSCRCINFOPARAM * pDfsCrcInfoParam,
                           const DFSPATCHANALYSEINFO_MEMORY* pDfsPatchAnalyseInfo,
                           BOOL fEraseContent, H_ERROR_INFO * pei);

dfuLong32 DfsWCloseDir(DFSFILE DfsFile, H_ERROR_INFO * pei);


/****************************************************************************/

dfuLong32 DfsRGetNextBlockType(DFSFILE DfsFile, dfuLong32 * dfBlockType, H_ERROR_INFO * pei);

dfuLong32 DfsROpenNextFileAndTagBeforeFile(DFSFILE DfsFile,
                                         dfuLong32 * pdfContentStoreMethod,
                                         dfuLong64 * pdfContentEncodedSize,
                                         dfuLong64 * pdfContentUncompressedSize,
                                         DFTAGLIST * TagListFileCopy,
                                         H_ERROR_INFO * pei);

dfuLong32 DfsRReadFileEncoded(DFSFILE DfsFile, void *Buf, dfuLong32 Size,
                            dfuLong32 * pReadSize, H_ERROR_INFO * pei);

dfuLong32 DfsRReadGetPostFile(DFSFILE DfsFile,
                            dfuLong64 * pdfFileContentUncompressedSize,
                            dfuLong32 * pCrcInfoNumber,
                            DFSCRCINFOPARAM ** ppDfsCrcInfoParam,
                            H_ERROR_INFO * pei);
/*
dfuLong32 DfsRCloseFile(DFSFILE DfsFile, const void *Buf, dfuLong32 Size,
                      dfuLong32 * pReadSize, H_ERROR_INFO * pei);
*/
dfuLong32 DfsRGetNextDirIntro(DFSFILE DfsFile, dfuLong32 * pdfTypedir,
                            DFTAGLIST * pTagListDirIntroCopy, H_ERROR_INFO * pei);

dfuLong32 DfsRGetNextDir(DFSFILE DfsFile, dfuLong32 * pdfTypedir,
                       DFTAGLIST * pTagListDirCopy, dfuLong32 * pdfGetNbFile,
                       DFTAGLIST ** ppsTagListFileInDirCopy, H_ERROR_INFO * pei);

dfuLong32 DfsRFreeAllDirTag(DFTAGLIST TagListDirCopy,
                          DFTAGLIST * psTagListFileInDirCopy, H_ERROR_INFO * pei);

/* a Dir contain a pointer to a file*/
/* function not compatible with stream */

dfuLong32 DfsGotoFilePointer(DFSFILE DfsFile, dfuLong32 dfPointerLow,dfuLong32 dfPointerHigh, H_ERROR_INFO * pei);

dfuLong32 DfsGotoDir(DFSFILE DfsFile, dfuLong32 dfNumDir, H_ERROR_INFO * pei);

dfuLong32 DfsGotoDirIntro(DFSFILE DfsFile, dfuLong32 dfNumDir, H_ERROR_INFO * pei);

dfuLong32 DfsGetCurrentPointer(DFSFILE DfsFile, dfuLong32 * pdfGetCurPtrLow,dfuLong32 * pdfGetCurPtrHigh, H_ERROR_INFO * pei);  /* useful? */

/************************************************************************/

/*
#ifdef __cplusplus
}
#endif
*/
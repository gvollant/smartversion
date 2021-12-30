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

#ifndef DO_EXTR_SUBDFS_H_INCLUDED
#define DO_EXTR_SUBDFS_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif



BOOL SVFAPI DoGenerateSubDfs(DFSFILE DfsFileRead,/*dfuLong32 dfNbDir,*/PDIRINFO* pDirInfo,
                             DFSFILE *pDfsFileWrite,dfwcharpc dfWritingDfsFileName, /*BOOL fZipFile,*/
                             const DFSFEATUREPARAM* pFeatureParam,
                             BOOL fBaseDirectorySelected,dfuLong32 dfBaseDirNum,dfwcharpc wchBaseDirectory,const FILESET* pFileSetBase,
                             dfuLong32 dwNbMapVersionMap,const BOOL* lpVersionMap,
                             BOOL fFirstVersionAsReference,BOOL fReuseOldPatch /* future*/,
                             const COMPRESSIONPARAM* pCprParam,
                             tSetExtractPosCallBack pSetExtractPosCallBack,dfvoidp dfUserPtr,
                             dfuLong32 dwMinProgress,dfuLong32 dwMaxProgress,
                             H_ERROR_INFO * pei);




/************************************************************************/

#ifdef __cplusplus
}
#endif

#endif

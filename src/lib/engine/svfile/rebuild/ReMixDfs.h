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

/* ReMixDfs.h */
/* Modify version in a Dfs */

#ifndef REMIX_DFS_H_INCLUDED
#define REMIX_DFS_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif


/*

typedef struct
{
  dfwcharpc filename_tostore;
  dfwcharpc filename_ondisk;
  dfwcharpc filename_prevversionondisk;
  dfuLong32   dfPreviousVersionFilePosition;
  BOOL      fIgnore;

  BOOL      fForceRecopyPrevious;
  BOOL      fForceDate;
  BOOL      fAddNewTag; // to be used later
  DFSINFODATE dfsInfoDate;
  dfvoidp   pReserved;

  dfuLong32   dfPreviousForceRecopySize;
  dfuLong32   dfPreviousForceRecopyCrc32;
}
FILETOADD_dfswrset.h;

*/

typedef struct
{
    dfwcharpc filename_tostore;
    dfwcharpc filename_ondisk;
    dfuLong32   dfPreviousVersionFilePositionItem; /* can be FTCI_REFERENCE_INDEPENDANT */
    BOOL      fIsReferenceInFileToAdd;

    BOOL      fForceRecopyPrevious;
    BOOL      fForceDate;
    BOOL      fAddNewTag; // to be used later
    DFSINFODATE dfsInfoDate;
    dfvoidp   pReserved;

    dfuLong32   dfPreviousForceRecopySize;
    dfuLong32   dfPreviousForceRecopyCrc32;
} FILETOADD_REMIX;

typedef struct
{
    dfuLong32 dfReferenceItem;
    BOOL fIsReferenceInAddedFile;
} FILETOCOPYINFO_REMIX ;

#define FTCI_REFERENCE_DELETE       ((dfuLong32)(0xffffffffUL))
#define FTCI_REFERENCE_UNMODIFIED   ((dfuLong32)(0xfffffffeUL))
#define FTCI_REFERENCE_INDEPENDANT  ((dfuLong32)(0xfffffffdUL))
#define FTCI_REFERENCE_JOKERBASE    ((dfuLong32)(0xfffffff0UL))

typedef struct
{
    dfuLong32 dfNumVersionPreviousSvf;

    dfuLong32 dfNbPreviousFileInMask;
    FILETOCOPYINFO_REMIX* pFileCopyInfo;

    dfuLong32 dfNbFileToAdd;
    FILETOADD_REMIX * pfta;
} VERSIONTOADD_REMIX;

/*
typedef struct
{
  dfuLong32 dfNbVersionToAdd;
  VERSIONTOADD_REMIX* pVersionToAdd;
} FRAMEWORK_REMIX;
*/

/*


BOOL SVFAPI DoGenerateSubDfs(DFSFILE DfsFileRead,PDIRINFO* pDirInfo,
                      DFSFILE *pDfsFileWrite,dfwcharpc dfWritingDfsFileName, // BOOL fZipFile,
                      const DFSFEATUREPARAM* pFeatureParam,
                      BOOL fBaseDirectorySelected,dfuLong32 dfBaseDirNum,dfwcharpc wchBaseDirectory,
                      dfuLong32 dwNbMapVersionMap,BOOL* lpVersionMap,
                      BOOL fFirstVersionAsReference,BOOL fReuseOldPatch, // future
                      const COMPRESSIONPARAM* pCprParam,
                      tSetExtractPosCallBack pSetExtractPosCallBack,dfvoidp dfUserPtr,
                      dfuLong32 dwMinProgress,dfuLong32 dwMaxProgress);
*/

BOOL SVFAPI DoGenerateSubDfsEmul(DFSFILE DfsFileRead,/*dfuLong32 dfNbDir,*/PDIRINFO* pDirInfo,
                      DFSFILE *pDfsFileWrite,dfwcharpc dfWritingDfsFileName, /*BOOL fZipFile,*/
                      const DFSFEATUREPARAM* pFeatureParam,
                      BOOL fBaseDirectorySelected,dfuLong32 dfBaseDirNum,dfwcharpc wchBaseDirectory,
                      const FILESET* pFileSetBase,
                      dfuLong32 dwNbMapVersionMap,const BOOL* lpVersionMap,
                      BOOL fFirstVersionAsReference,BOOL fReuseOldPatch /* future*/,
                      const COMPRESSIONPARAM* pCprParam,
                      tSetExtractPosCallBack pSetExtractPosCallBack,dfvoidp dfUserPtr,
                      dfuLong32 dwMinProgress,dfuLong32 dwMaxProgress,
                      H_ERROR_INFO* pei);


BOOL SVFAPI DoGenerateSubDfsEmulEx(DFSFILE DfsFileRead,/*dfuLong32 dfNbDir,*/PDIRINFO* pDirInfo,
                      DFSFILE *pDfsFileWrite,dfwcharpc dfWritingDfsFileName, /*BOOL fZipFile,*/
                      const DFSFEATUREPARAM* pFeatureParam,
                      BOOL fBaseDirectorySelected,dfuLong32 dfBaseDirNum,dfwcharpc wchBaseDirectory,
                      const FILESET* pFileSetBase,
                      dfuLong32 dwNbMapVersionMap,const BOOL* lpVersionMap,
                      BOOL fFirstVersionAsReference,BOOL fReuseOldPatch, BOOL fRawAccepted,
                      const COMPRESSIONPARAM* pCprParam,
                      tSetExtractPosCallBack pSetExtractPosCallBack,dfvoidp dfUserPtr,
                      dfuLong32 dwMinProgress,dfuLong32 dwMaxProgress,
                      H_ERROR_INFO* pei);


dfuLong32 SVFAPI DoReMixDfs(DFSFILE DfsFileRead,dfuLong32 dfNbDirDfsRead,const PCDIRINFO* pDirInfo,
                DFSFILE *pDfsFileWrite,dfwcharpc dfWritingDfsFileName, // BOOL fZipFile,
                const DFSFEATUREPARAM* pFeatureParam,
                BOOL fBaseDirectorySelected,dfuLong32 dfBaseDirNum,dfwcharpc wchBaseDirectory,
                const FILESET* pFileSetBase,
                dfuLong32 dfNbVersionRemix, const VERSIONTOADD_REMIX * pVersionRemix,
                BOOL fFirstVersionAsReference,BOOL fReuseOldPatch, // future
                const COMPRESSIONPARAM* pCprParam,
                tSetExtractPosCallBack pSetExtractPosCallBack,dfvoidp dfUserPtr,
                dfuLong32 dwMinProgress,dfuLong32 dwMaxProgress,H_ERROR_INFO* pei);

dfuLong32 SVFAPI DoReMixDfsEx(DFSFILE DfsFileRead, dfuLong32 dfNbDirDfsRead,
                const PCDIRINFO * pDirInfo, DFSFILE * pDfsFileWrite,
                dfwcharpc dfWritingDfsFileName,        // BOOL fZipFile,
                const DFSFEATUREPARAM* pFeatureParam,
                BOOL fBaseDirectorySelected, dfuLong32 dfBaseDirNum,
                dfwcharpc wchBaseDirectory,
                const FILESET* pFileSetBase,
                dfuLong32 dfNbVersionRemix,
                const VERSIONTOADD_REMIX * pVersionRemix,
				BOOL fFirstVersionAsReferenceNewDfs, BOOL fReuseOldPatch, BOOL fRawAccepted,
                const COMPRESSIONPARAM * pCprParam,
                tSetExtractPosCallBack pSetExtractPosCallBack,
                dfvoidp dfUserPtr, dfuLong32 dwMinProgress, dfuLong32 dwMaxProgress,
                H_ERROR_INFO* pei);

#ifdef __cplusplus
}
#endif

#endif

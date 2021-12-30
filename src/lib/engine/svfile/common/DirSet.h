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

/* DirSet.h */
/* basic structure and tools to extract several version at a time */

#ifndef DIR_SET_H_INCLUDED
#define DIR_SET_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    ExtActionIgnore,
    ExtActionExtractContent,
    ExtActionMergeRamDif,
    ExtActionExtractContentInPlace,
    ExtActionExtractContentInPlaceNoChecksum,
    ExtActionExtractContentByMerging
} EXTACTION;

typedef struct
{
  dfwcharpc FileNameOnDisk;
  dfwcharpc FileNameOnArchive;
  ///dfuLong64 dfFileSizeUncompressed;
  BOOL      fTempFile;
  dfuLong32 dfPreviousVersion;
  BOOL      fIdenticalPreviousVersion;
  //BOOL      fIgnore;// replace with enum
  EXTACTION ExtAction;
  BOOL      fForceDate;
  DFTAGLIST   hAddTags;
  DFSINFODATE dfsInfoDate;
  dfvoidp   pReserved;
  HRAMDIF   hRamDifOfPatch ;
}
FILEITEM;
typedef FILEITEM *PFILEITEM;


typedef struct
{
  dfuLong32 dfNbFileItem;
  dfuLong32 dfFileItemStepAlloc;
  dfuLong32 dfFileItemAllocated;
  BOOL fTempFile;
  FILEITEM *pFileItem;
}
FILESET;

void SVFAPI InitFileSet(FILESET * pfs);
void SVFAPI FreeFileSet(FILESET * pfs, BOOL fDeleteFile);

BOOL SVFAPI AddItemToFileSet(FILESET * pfs, const FILEITEM * lpfi,
                      dfuLong32 dfNbFileItem);





BOOL SVFAPI ExtractPatch(DFSFILE DfsFile, FILESET * pfsOrg, FILESET * pfsDest,
                  dfuLong32 dfNumDirBase,PCDIRINFO pDirInfo,
                  BOOL fNewFileSetTemp, BOOL fOldFileCanBeCaptured,BOOL fFlushWrite,
                  dfwcharpc szBasePath,
                  tProgressCallBack pProgressCallBack,dfvoidp dfUserPtr,
                  tExtractingFileWorkingEvent pExtractingFileWorkingEvent,dfvoidp dfUserPtrWorkingEvent,
                  H_ERROR_INFO* pei);

/*

Create a FILESET Array for a version and a directory

  pfComplete : Will receive BOOL to known if all is good
  fVerboseNotFound : if must do printf for error
  fFillNameOnDisk : Always fill the FileNameOnDisk item and search the file
  fTryFindFileOnDiskIfNotInsertingInDfs : if TRUE, fill the FileNameOnDisk
                    item and search the file and NOT Inserting (Patch or CRC)
  fTryFindFileOnDiskIfInsertingInDfs : if TRUE, fill the FileNameOnDisk
                    item and search the file and Inserting (DEFLATE or uncompressed)

  pdfTypeDir : Will receive the type of the dir


*/

dfuLong32 SVFAPI CreateFileSetForVersionInDirectory(DFSFILE DfsFile, FILESET * pfs,
                                           dfuLong32 dfNumDir,
                                           dfwcharpc szDirBase,const FILESET* pFileSetBase,
                                           PCDIRINFO pDirInfoAlreadyProvided,
                                           BOOL * pfComplete,
                                           BOOL fVerboseNotFound,
                                           BOOL fFillNameOnDisk,
                                           BOOL
                                           fTryFindFileOnDiskIfNotInsertingInDfs,
                                           BOOL
                                           fTryFindFileOnDiskIfInsertingInDfs,
                                           dfuLong32 * pdfTypeDir,
                                           BOOL fFillForceFileDateFromDirectoryInDfs,
                                           BOOL fFlatFileSet,H_ERROR_INFO* pei);


long DFSCALLBACK fncCompareFileItem(const void* lpElem1, const void* lpElem2);
BOOL DFSCALLBACK fncDestructorFileItem(const void* lpElem);





#ifdef __cplusplus
}
#endif

#endif

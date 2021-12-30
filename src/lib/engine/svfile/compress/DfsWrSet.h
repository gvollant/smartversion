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

/* DfsWrSet.h */
/* function to write a new version into SVF file */

#ifndef DFS_WR_SET_H_INCLUDED
#define DFS_WR_SET_H_INCLUDED


#if defined(__cplusplus) && (!defined(ALLINCPP))
extern "C" {
#endif


typedef struct
{
  dfwcharpc   filename_tostore;
  dfwcharpc   filename_ondisk;
  dfwcharpc   filename_prevversionondisk;
  dfuLong32   dfPreviousVersionFilePosition;
  BOOL        fIgnore;

  BOOL        fForceRecopyPrevious;
  BOOL        fWritingRaw;
  dfuLong32   dfFileStatusForRaw;
  BOOL        fForceDate;
  DFTAGLIST   hAddTags; /* to be used later */
  DFSINFODATE dfsInfoDate;
  dfvoidp     pReserved; /* for future use */

  dfuLong64     dfForceRecopyOrRawCopySize;
  dfuLong32     dfForceRecopyOrRawCopyCrc32;
  BOOL          fForceRecopyOrRawCopyMd5Present;
  dfbyte        bMd5[16];
  BOOL          fForceRecopyOrRawCopySha1Present;
  dfbyte        bSha1[20];
  BOOL          fForceRecopyOrRawCopySha256Present;
  dfbyte        bSha256[32];

  HRAMDIF       hRamDifToFlushPatch;
}
FILETOADD;

/*
typedef struct
{
    dfuLong32 dfTypeInserting;
    dfuLong32 dfNbFile;
} -*/

#define TYPEDIR_FILECRCONLY             (1)
#define TYPEDIR_FILEINSERTING_STORE     (2)
#define TYPEDIR_FILEINSERTING_DEFLATE   (3)
#define TYPEDIR_PATCHFROMPREVIOUS       (4)


dfuLong32 SVFAPI InsertDirectoryinDfsFile(DFSFILE DfsFile, dfuLong32 dfTypeDir,
                                 dfuLong32 dfNbFile, FILETOADD * pFileToAdd,
                                 BOOL fWriteCrcAndSizeInFileToAddArray,
                                 dfwcharpc dfwVersionName,
                                 dfwcharpc dfwVersionComment,
                                 const COMPRESSIONPARAM* pCprParam,
                                 tProgressCallBack pProgressCallBack,
                                 dfvoidp dfUserPtr,
                                 H_ERROR_INFO* pei);

#define DF_PROGRESS_EVENT_NEWDIR    (1)
#define DF_PROGRESS_EVENT_NEWFILE   (2)
#define DF_PROGRESS_EVENT_PROGRESS  (3)

#if defined(__cplusplus) && (!defined(ALLINCPP))
}
#endif

#endif

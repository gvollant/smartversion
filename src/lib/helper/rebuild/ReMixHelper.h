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

/* ReMixHelper.h */
/* function to be used with Remix */

#ifndef REMIX_HELPER_H_INCLUDED
#define REMIX_HELPER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    dfwcharpc dfFileNameOnDisk;
    dfwcharpc dfFileNameToStore;
} FILETOADDINVERSIONS;

typedef enum
{
    REPLACEMENTFILEOPTION_UIASKING=1,
    REPLACEMENTFILEOPTION_REPLACE,
    REPLACEMENTFILEOPTION_NOREPLACE
} REPLACEMENTFILEOPTION;

typedef struct
{
    FILETOADDINVERSIONS* pFileToAddInVersions;
    dfuLong32 dfNbFileToAddInVersion;
    dfuLong32 dfNbVersion;
    BOOL* pVersionMap;
    REPLACEMENTFILEOPTION ReplacementFileOption;
    BOOL fAskLinkRenamedVersionBefore;
    BOOL fAskLinkRenamedVersionAfter;
} INSERTFILEINVERSION_DATA;


typedef struct
{
  dfuLong32 dfSizeStruct;
  INSERTFILEINVERSION_DATA* pInsertFileInVersionData;

  dfuLong32 dfVersion;
  dfuLong32 dfItemToReplace;
  dfuLong32 dfItemInInsertList;
  dfwcharpc dfFileNameToStore;
  dfwcharpc dfFileNameOnDisk;
  const PDIRINFO* pDirInfo;
} ASKREPL_CBINFO;

#define CBASKREPLACING_ANSWER_REPLACE       ((dfuLong32)(0x00000001UL))
#define CBASKREPLACING_ANSWER_REPLACEALL    ((dfuLong32)(0x00000005UL))
#define CBASKREPLACING_ANSWER_NOREPLACE     ((dfuLong32)(0x00000002UL))
#define CBASKREPLACING_ANSWER_NOREPLACEALL  ((dfuLong32)(0x00000006UL))
#define CBASKREPLACING_ANSWER_CANCEL        ((dfuLong32)(0x00000008UL))

typedef dfuLong32(DFSCALLBACK * tCallBackAskReplacing) (const ASKREPL_CBINFO* pAskRepl_CbInfo,
                                                      dfvoidp dfUserPtr);

VERSIONTOADD_REMIX * SVFAPI BuildRecopyVersionToAdd(dfuLong32 dfNbVersionInDfs, PDIRINFO* pDirInfo);

VERSIONTOADD_REMIX * SVFAPI CreateInsertingForReplaceCurrentDfs(dfuLong32 dfNbDir, PDIRINFO* pDirInfo,
                                                         INSERTFILEINVERSION_DATA* pInsertFileInVersionData,
                                                         dfuLong32 dfVersionSelected,dfuLong32 * pdfError,
                                                         tCallBackAskReplacing pCallBackAskReplacing,dfvoidp dfUserPtr);

BOOL SVFAPI FreeVersionToAdd(VERSIONTOADD_REMIX *pVersionRemix,dfuLong32 dfNbVersion);

BOOL SVFAPI PropagateDelete(VERSIONTOADD_REMIX *pVersionRemix,dfuLong32 dfNbVersionInDfs, PDIRINFO* pDirInfo,
                     dfuLong32 dfVersionWithDeleteInfo,
                     dfuLong32 dfFirstDeleteVersion,dfuLong32 dfLastDeleteVersion,
                     BOOL fSuppressRenamed);

#ifdef __cplusplus
}
#endif

#endif

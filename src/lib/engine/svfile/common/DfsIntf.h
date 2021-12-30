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

/* DfsIntl.h */
/* function and structure to work on SVF file */


#ifndef DFS_INTL_H_INCLUDED
#define DFS_INTL_H_INCLUDED

#if defined(__cplusplus) && (!defined(ALLINCPP))
extern "C" {
#endif

typedef struct
{
  dfuLong32 sizeStruct;
  dfuLong32 dfStatus;
  dfwcharpc filename;            /* filename or other info for open callback */
}
DFSFILEINFOPARAM;


typedef struct
{
  BOOL fStripIdenticalBody;
  BOOL fComputeMd5;
  BOOL fComputeSha1;
  BOOL fComputeSha256;
} DFSFEATUREPARAM;


DECLARE_DFHANDLE(DFSFILE);

//#define MAX_PATH_LENGTH (260)

/****************************************************************************/

dfuLong32 SVFAPI GetSvfVersion();
dfuLong32 SVFAPI GetSvfVersionDate();

dfuLong32 SVFAPI DfsOpen(DFSFILEINFOPARAM DfsFileParam, DFSFILE * DfsFile, H_ERROR_INFO * pei);

dfuLong32 SVFAPI DfsFileOpen(const DFSFILEINFOPARAM *pDfsFileParam, DFSFILE* DfsFile, H_ERROR_INFO* pei);

dfuLong32 SVFAPI DfsClose(DFSFILE, H_ERROR_INFO * pei);

dfuLong32 SVFAPI DfsFlushWriteDfsFile(DFSFILE, H_ERROR_INFO * pei);

/****************************************************************************/

void SVFAPI SetDfsExtendedMode(DFSFILE DfsFile, BOOL fStripIdenticalBody);
void SVFAPI GetDfsExtendedMode(DFSFILE DfsFile, BOOL* pfStripIdenticalBody);

void SVFAPI SetDfsFeatureParam(DFSFILE DfsFile, const DFSFEATUREPARAM*);
void SVFAPI GetDfsFeatureParam(DFSFILE DfsFile, DFSFEATUREPARAM*);

/****************************************************************************/

dfuLong32 SVFAPI DfsGetDirType(DFSFILE DfsFile, dfuLong32 dfNumDir, dfuLong32 * pdfDirType, H_ERROR_INFO * pei);

dfuLong32 SVFAPI DfsGetNbDir(DFSFILE DfsFile, dfuLong32 * pdfNbDir, H_ERROR_INFO * pei);

DFTAGBLOCKFLOAT SVFAPI GetDfsTagBlockFloat(DFSFILE DfsFile, H_ERROR_INFO * pei);

BOOL SVFAPI SetDfsTagBlockFloatDirty(DFSFILE DfsFile, H_ERROR_INFO * pei);

/************************************************************************/

#if defined(__cplusplus) && (!defined(ALLINCPP))
}
#endif

#endif

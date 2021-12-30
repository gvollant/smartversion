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

/* makdifst.h */
/* function to build a stream patch from two file */

#ifndef DFS_MAK_DIFST_H_INCLUDED
#define DFS_MAK_DIFST_H_INCLUDED


#ifdef __cplusplus
extern "C" {
#endif


/*DECLARE_DFHANDLE(HWRITEDIF);*/
DECLARE_DFHANDLE(HWRITEDIF_INTERNALSTATE);

typedef dfuLong32(DFSCALLBACK * tPerformOrigDataAnalysisCB)(dfuLong32 dfEvent,
                                                            dfuLong64 PosInOrig,
                                                            dfuLong64 TotalOrig,
                                                            dfvoidp dfUserPtr);



typedef struct
{
  dfuLong32 nbOrig;               /* must be initialised before call init */
  ORIGDATAPTR OrigDataPtr;      /* must be initialised before call init */
  dfuLong32 uDoChecksum;          /* 1 if we compute checksum in InData */

  IN_DATA_STREAM in_data_stream;
  OUT_DATA_STREAM out_data_stream;

  tPerformOrigDataAnalysisCB pPerformOrigDataAnalysisCB;
  dfvoidp dfUserPtr;

  HWRITEDIF_INTERNALSTATE state;        /* not visible by applications */
}
WRITEDIF_STREAM;


dfuLong32 SVFAPI GetCompressionParamSize OF(());
void SVFAPI InitDefaultCompressionParam OF((COMPRESSIONPARAM* pCprParam));
int SVFAPI MakeDifInit OF((WRITEDIF_STREAM * write_stream, const COMPRESSIONPARAM* pCprParam));
int SVFAPI DoMakeDifWork OF((WRITEDIF_STREAM * write_stream));

/* when call InsertBytesFromOrig, avail_in MUST be 0 */
/*
int SVFAPI InsertBytesFromOrig OF((WRITEDIF_STREAM * write_stream, dfuLong32 posOrig,
                            dfuLong32 size));
*/
int SVFAPI FlushMakeDif OF((WRITEDIF_STREAM * write_stream));
int SVFAPI CloseMakeDif OF((WRITEDIF_STREAM * write_stream, dfuLong32 * uChecksum, BOOL* pfFileIdentical));

int SVFAPI CloseMakeDifEx OF((WRITEDIF_STREAM * write_stream, dfuLong32 * uChecksum, BOOL* pfFileIdentical,
  DFSPATCHANALYSEINFO_MEMORY* pDfsPatchAnalyseInfo));

#ifdef DO_STATIS
void SVFAPI EnableStatis OF((WRITEDIF_STREAM * write_stream, dfuLong32 max_value));
int SVFAPI FlushStatis
OF((WRITEDIF_STREAM * write_stream, int numStream, const char *fn,
    const char *title));
void SVFAPI FlushHuff OF((WRITEDIF_STREAM * write_stream));
#endif
/****************************************************************************/

#ifdef __cplusplus
}
#endif

#endif

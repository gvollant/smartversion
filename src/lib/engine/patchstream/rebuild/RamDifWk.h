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

/* RamDifWk.h */
/*  */

#ifndef RAM_DIF_WK_H_INCLUDED
#define RAM_DIF_WK_H_INCLUDED



DECLARE_DFHANDLE(HRAMDIF);

/*****************************************************************************************/

DECLARE_DFHANDLE(HRAMDIFBLD_FROM_DATA_STREAM);




typedef int (DFSCALLBACK * tWriteInsertBytesFnc)(dfvoidp pUserPtr,
                                               dfvoidp data, dfuLong64 size);

typedef int (DFSCALLBACK * tWriteInsertDeplFnc)(dfvoidp pUserPtr,
                                            dfuLong64 posIntPtr, dfuLong64 size);

typedef int (DFSCALLBACK * tDoFlushOut)(dfvoidp pUserPtr,int final_flush);

typedef struct
{
    dfvoidp pUserPtr;
    tWriteInsertBytesFnc fncWriteInsertBytesFnc;
    tWriteInsertDeplFnc fncWriteInsertDeplFnc;
    tDoFlushOut fncDoFlushOut;
    dfuLong32 dfMaxSizeWriteInsByteArray;
} CLASS_OUT_PATCH;

typedef struct
{
  IN_DATA_STREAM in_data_stream;

  HRAMDIFBLD_FROM_DATA_STREAM state;        /* not visible by applications */
} RAMDIFBLD_FROM_DATA_STREAM;

int SVFAPI ClosePatchBldFromDataStream (RAMDIFBLD_FROM_DATA_STREAM * pDifbld,DFSPATCHANALYSEINFO_MEMORY *p_dfsPatchAnalyseInfoToFill);
int SVFAPI DoPatchBldFromDataStream (RAMDIFBLD_FROM_DATA_STREAM * pDifbld);
int SVFAPI PatchBldFromDataStreamAndRamDifInit (RAMDIFBLD_FROM_DATA_STREAM * pDifbld,
                                                const CLASS_OUT_PATCH* pClass_Out_PatchSet,
                                                 dfuLong64 dfExpectedSizeOfPatchResult,
                                                 ORIGDATAPTR OrigDataPtrForInsertNoInPlace,
                                                 HRAMDIF hRamDifBase);

/**************************************************************************/

int SVFAPI RamDifBldFromDataStreamAndRamDifInit OF((RAMDIFBLD_FROM_DATA_STREAM * pDifbld,
                                                    dfuLong64 dfExpectedSizeOfPatchResult,
                                                    ORIGDATAPTR OrigDataPtrForInsertNoInPlace,
                                                    HRAMDIF hRamDifBase));
int SVFAPI RamDifBldFromDataStreamInit OF((RAMDIFBLD_FROM_DATA_STREAM * pDifbld, dfuLong64 dfExpectedSizeOfPatchResult));
int SVFAPI DoRamDifBldFromDataStream (RAMDIFBLD_FROM_DATA_STREAM * pDifbld);
int SVFAPI CloseRamDifBldFromDataStream OF((RAMDIFBLD_FROM_DATA_STREAM * pDifbld,HRAMDIF *phRamDifBld,DFSPATCHANALYSEINFO_MEMORY *p_dfsPatchAnalyseInfoToFill));

/*****************************************************************************************/



DECLARE_DFHANDLE(HAPPLYRAMDIF_INTERNALSTATE);

typedef struct
{
  dfuLong32 nbOrig;               /* must be initialised before call init */
  ORIGDATAPTR OrigDataPtr;      /* must be initialised before call init */
  dfuLong32 uDoChecksum;          /* 1 if we compute checksum in OutData */

  OUT_DATA_STREAM out_data_stream;

  HAPPLYRAMDIF_INTERNALSTATE state;        /* not visible by applications */
}
APPLYRAMDIF_STREAM;

int SVFAPI ApplyRamDifInit OF((HRAMDIF hRamDifBld, APPLYRAMDIF_STREAM * apply_ramdif_stream, BOOL fComputeChecksum));
int SVFAPI DoApplyRamDifWork OF((APPLYRAMDIF_STREAM * apply_ramdif_stream));
int SVFAPI CloseRamApplyDif OF((APPLYRAMDIF_STREAM * apply_ramdif_stream, dfuLong32 * uChecksum));


/*****************************************************************************************/



DECLARE_DFHANDLE(HAPPLYRAMDIFINPLACE_INTERNALSTATE);

typedef struct
{
  dfuLong32 uDoChecksum;          /* 1 if we compute checksum in OutData */

  OUT_DATA_STREAM out_data_stream;

  HAPPLYRAMDIFINPLACE_INTERNALSTATE state;        /* not visible by applications */
}
APPLYRAMDIFINPLACE_STREAM;

int SVFAPI ApplyRamDifInPlaceInit OF((HRAMDIF hRamDifBld, APPLYRAMDIFINPLACE_STREAM * apply_ramdif_inplace_stream, BOOL fComputeChecksum));
int SVFAPI DoApplyRamDifInPlaceWork OF((APPLYRAMDIFINPLACE_STREAM * apply_ramdif_inplace_stream));
int SVFAPI DoApplyRamDifInPlaceWorkWithDirtyFlag OF((APPLYRAMDIFINPLACE_STREAM * apply_ramdif_inplace_stream, BOOL *pfDirty));
int SVFAPI CloseRamApplyInPlaceDif OF((APPLYRAMDIFINPLACE_STREAM * apply_ramdif_inplace_stream, dfuLong32 * uChecksum));

#endif

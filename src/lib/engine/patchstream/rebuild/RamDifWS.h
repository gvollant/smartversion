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

/* RamDifWS.h */
/*  */

#ifndef RAM_DIF_WS_H_INCLUDED
#define RAM_DIF_WS_H_INCLUDED

DECLARE_DFHANDLE(HPATCHSTREAMBLD_FROM_DATA_STREAM);


typedef struct
{
  IN_DATA_STREAM in_data_stream;
  OUT_DATA_STREAM out_data_stream;

  HPATCHSTREAMBLD_FROM_DATA_STREAM statee;        /* not visible by applications */
} PATCHSTREAMBLD_FROM_DATA_STREAM;


int SVFAPI PatchOutStreamBldFromDataStreamAndRamDifInit(PATCHSTREAMBLD_FROM_DATA_STREAM * pStrmBld,
                                                         dfuLong64 dfExpectedSizeOfPatchResult,
                                                         HRAMDIF hRamDifBase,
                                                         dfuLong32 CompressRatio, dfuLong32 dfSizeButStreamKB);

int SVFAPI PatchOutStreamBldFromDataStreamAndRamDifInit_ReplaceOrigOutPlace(PATCHSTREAMBLD_FROM_DATA_STREAM * pStrmBld,
                                                         dfuLong64 dfExpectedSizeOfPatchResult,
                                                         HRAMDIF hRamDifBase,
                                                         ORIGDATAPTR OrigDataPtr,
                                                         dfuLong32 CompressRatio, dfuLong32 dfSizeButStreamKB);

int SVFAPI DoPatchOutStreamBldFromDataStream (PATCHSTREAMBLD_FROM_DATA_STREAM * pStrmBld);

int SVFAPI FlushPatchOutStreamBldFromDataStreamAndRamDif(PATCHSTREAMBLD_FROM_DATA_STREAM * pStrmBld);

int SVFAPI ClosePatchOutStreamBldFromDataStreamAndRamDif(PATCHSTREAMBLD_FROM_DATA_STREAM * pStrmBld,
    DFSPATCHANALYSEINFO_MEMORY *p_dfsPatchAnalyseInfoToFill);

/************************************************************************************************/

DECLARE_DFHANDLE(HPATCHSTREAMBLD_FROM_RAMDIF);

typedef struct
{
  OUT_DATA_STREAM out_data_stream;

  HPATCHSTREAMBLD_FROM_RAMDIF state_;        /* not visible by applications */
} PATCHSTREAMBLD_FROM_RAMDIF;

int SVFAPI PatchOutStreamBldFromRamDif(PATCHSTREAMBLD_FROM_RAMDIF * pStrmBldRd,
                                       dfuLong64 dfExpectedSizeOfPatchResult,
                                       HRAMDIF hRamDifBase,
                                       dfuLong32 CompressRatio, dfuLong32 dfSizeButStreamKB);

int SVFAPI PatchOutStreamBldFromRamDif_ReplaceOrigOutPlace(PATCHSTREAMBLD_FROM_RAMDIF * pStrmBldRd,
                                       dfuLong64 dfExpectedSizeOfPatchResult,
                                       HRAMDIF hRamDifBase,
                                       ORIGDATAPTR OrigDataPtr,
                                       dfuLong32 CompressRatio, dfuLong32 dfSizeButStreamKB);

int SVFAPI DoPatchOutStreamBldFromRamDif (PATCHSTREAMBLD_FROM_RAMDIF * pStrmBldRd);

int SVFAPI FlushPatchOutStreamBldFromRamDif(PATCHSTREAMBLD_FROM_RAMDIF * pStrmBldRd);

int SVFAPI ClosePatchStreamBldFromRamDif(PATCHSTREAMBLD_FROM_RAMDIF * pStrmBldRd,
    DFSPATCHANALYSEINFO_MEMORY *p_dfsPatchAnalyseInfoToFill);

#endif

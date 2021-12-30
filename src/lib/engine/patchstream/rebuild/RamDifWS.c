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

#include <stddef.h>
#include <stdio.h>

#include <memory.h>

#include "../common/difbasic.h"
#include "../compress/abstractCompress.h"
//#include "zlib.h"




#include "../common/difstrm.h"
#include "../common/difstrmi.h"
#include "../common/DfsTlTyp.h"
#include "../common/difstool.h"
#include "../compress/MkDifStm.h"


//#include "ApDifStm.h"

#include "../common/DfsOrigMemoryMap.h"

#include "RamDifWk.h"
#include "RamDifTl.h"

#include "RamDifWS.h"




typedef struct
{
    OUT_MULTI_BUFFER OutPatchStream;
    OUT_DATA_STREAM* pOutDataStream;
    dfuLong64 posCurOrg;
    RAMDIFBLD_FROM_DATA_STREAM RamDifBld_From_Data_Stream;
    dfuLong32 in_final_flush;
} PATCH_OUT_STREAM_INTERNAL;



int DFSCALLBACK PatchOutWriteInsertBytesFnc(dfvoidp pUserPtr,
                                               dfvoidp data, dfuLong64 size)
{
    PATCH_OUT_STREAM_INTERNAL* pPatch_Out_Stream_Internal;
    pPatch_Out_Stream_Internal = (PATCH_OUT_STREAM_INTERNAL*)pUserPtr;
    pPatch_Out_Stream_Internal->posCurOrg += size;
    return WriteInsertBytesInStream(&pPatch_Out_Stream_Internal->OutPatchStream,
                                    pPatch_Out_Stream_Internal->pOutDataStream,
                                    data, size);

}

int DFSCALLBACK PatchOutWriteInsertDeplFnc(dfvoidp pUserPtr,
                                            dfuLong64 posIntPtr, dfuLong64 size)
{
    int err;
    PATCH_OUT_STREAM_INTERNAL* pPatch_Out_Stream_Internal;
    pPatch_Out_Stream_Internal = (PATCH_OUT_STREAM_INTERNAL*)pUserPtr;
    err = WriteInsertDeplInStream(&pPatch_Out_Stream_Internal->OutPatchStream,
                                   pPatch_Out_Stream_Internal->pOutDataStream,
                                   pPatch_Out_Stream_Internal->posCurOrg,
                                   posIntPtr, size);
    pPatch_Out_Stream_Internal->posCurOrg = posIntPtr + size;
    return err;
}

int DFSCALLBACK PatchOutDoFlushOut(dfvoidp pUserPtr,int final_flush)
{
    PATCH_OUT_STREAM_INTERNAL* pPatch_Out_Stream_Internal;
    pPatch_Out_Stream_Internal = (PATCH_OUT_STREAM_INTERNAL*)pUserPtr;

    /*
    if (final_flush == 0)
        return 0;
*/
    FlushOutInStream(&pPatch_Out_Stream_Internal->OutPatchStream,
                     pPatch_Out_Stream_Internal->pOutDataStream);

    if (final_flush == 1)
    {
        int err = FinalFlushOutData(&pPatch_Out_Stream_Internal->OutPatchStream,
                     pPatch_Out_Stream_Internal->pOutDataStream);
        if (err == DSERR_END)
            return 0;
        else
            return 1;
    }
    return NeedMoreOutSpaceForFlushing(&pPatch_Out_Stream_Internal->OutPatchStream,
                     pPatch_Out_Stream_Internal->pOutDataStream);
}



int SVFAPI PatchOutStreamBldFromDataStreamAndRamDifInit(PATCHSTREAMBLD_FROM_DATA_STREAM * pStrmBld,
                                                         dfuLong64 dfExpectedSizeOfPatchResult,
                                                         HRAMDIF hRamDifBase,
                                                         dfuLong32 CompressRatio, dfuLong32 dfSizeButStreamKB)
{
    return PatchOutStreamBldFromDataStreamAndRamDifInit_ReplaceOrigOutPlace(pStrmBld,
                                                         dfExpectedSizeOfPatchResult,
                                                         hRamDifBase,
                                                         NULL,
                                                         CompressRatio, dfSizeButStreamKB);
}

int SVFAPI PatchOutStreamBldFromDataStreamAndRamDifInit_ReplaceOrigOutPlace(PATCHSTREAMBLD_FROM_DATA_STREAM * pStrmBld,
                                                         dfuLong64 dfExpectedSizeOfPatchResult,
                                                         HRAMDIF hRamDifBase,
                                                         ORIGDATAPTR OrigDataPtr,
                                                         dfuLong32 CompressRatio, dfuLong32 dfSizeButStreamKB)
{
    PATCH_OUT_STREAM_INTERNAL* pPatch_Out_Stream_Internal;
    CLASS_OUT_PATCH Class_Out_Patch_Internal;
    int err;
    pPatch_Out_Stream_Internal = (PATCH_OUT_STREAM_INTERNAL*)DfsMalloc(sizeof(PATCH_OUT_STREAM_INTERNAL));
    if (pPatch_Out_Stream_Internal == NULL)
        return DSERR_INTERNAL;

    pPatch_Out_Stream_Internal->pOutDataStream = &pStrmBld->out_data_stream;

    if (InitOutPatchStream(&pPatch_Out_Stream_Internal->OutPatchStream,CompressRatio, dfSizeButStreamKB) != DSERR_OK)
    {
        DfsFree(pPatch_Out_Stream_Internal);
        return DSERR_INTERNAL;
    }

    Class_Out_Patch_Internal.fncWriteInsertBytesFnc = PatchOutWriteInsertBytesFnc;
    Class_Out_Patch_Internal.fncWriteInsertDeplFnc = PatchOutWriteInsertDeplFnc;
    Class_Out_Patch_Internal.fncDoFlushOut = PatchOutDoFlushOut;
    Class_Out_Patch_Internal.pUserPtr = (dfvoidp)pPatch_Out_Stream_Internal;
    Class_Out_Patch_Internal.dfMaxSizeWriteInsByteArray = 0x4000;

    pPatch_Out_Stream_Internal->RamDifBld_From_Data_Stream.in_data_stream = pStrmBld->in_data_stream;
    pPatch_Out_Stream_Internal->in_final_flush = 0;
    pPatch_Out_Stream_Internal->posCurOrg = 0;

    err = PatchBldFromDataStreamAndRamDifInit(&pPatch_Out_Stream_Internal->RamDifBld_From_Data_Stream,
                                            &Class_Out_Patch_Internal,
                                            dfExpectedSizeOfPatchResult,OrigDataPtr,hRamDifBase);
    pStrmBld->in_data_stream = pPatch_Out_Stream_Internal->RamDifBld_From_Data_Stream.in_data_stream ;

    if (err != DSERR_OK)
    {
        ReleaseOutPatchStream(&pPatch_Out_Stream_Internal->OutPatchStream);
        DfsFree(pPatch_Out_Stream_Internal);
    }
    pStrmBld->statee = (HPATCHSTREAMBLD_FROM_DATA_STREAM)pPatch_Out_Stream_Internal;
    return err;
}



int SVFAPI DoPatchOutStreamBldFromDataStream (PATCHSTREAMBLD_FROM_DATA_STREAM * pStrmBld)
{
    int err;
    PATCH_OUT_STREAM_INTERNAL* pPatch_Out_Stream_Internal;
    pPatch_Out_Stream_Internal = (PATCH_OUT_STREAM_INTERNAL*)pStrmBld->statee;


    pPatch_Out_Stream_Internal->RamDifBld_From_Data_Stream.in_data_stream = pStrmBld->in_data_stream;
    pPatch_Out_Stream_Internal->pOutDataStream = &pStrmBld->out_data_stream;

    err = DoPatchBldFromDataStream(&pPatch_Out_Stream_Internal->RamDifBld_From_Data_Stream);
    pStrmBld->in_data_stream = pPatch_Out_Stream_Internal->RamDifBld_From_Data_Stream.in_data_stream ;

    return err;
}


int SVFAPI FlushPatchOutStreamBldFromDataStreamAndRamDif(PATCHSTREAMBLD_FROM_DATA_STREAM * pStrmBld)
{
  int err = DSERR_OK;
  PATCH_OUT_STREAM_INTERNAL* pPatch_Out_Stream_Internal;
  pPatch_Out_Stream_Internal = (PATCH_OUT_STREAM_INTERNAL*)pStrmBld->statee;

  pPatch_Out_Stream_Internal->pOutDataStream = &pStrmBld->out_data_stream;

  if (pPatch_Out_Stream_Internal->in_final_flush == 0)
  {
      if (pStrmBld->in_data_stream.avail_in != 0)
      {
          pPatch_Out_Stream_Internal->RamDifBld_From_Data_Stream.in_data_stream = pStrmBld->in_data_stream;
          err = DoPatchBldFromDataStream(&pPatch_Out_Stream_Internal->RamDifBld_From_Data_Stream);
          pStrmBld->in_data_stream = pPatch_Out_Stream_Internal->RamDifBld_From_Data_Stream.in_data_stream ;

          if ((err != DSERR_OK) && (err != DSERR_END))
              return err;
      }
      if (pStrmBld->in_data_stream.avail_in == 0)
          pPatch_Out_Stream_Internal->in_final_flush = 1;
  }

  FlushOutInStream(&pPatch_Out_Stream_Internal->OutPatchStream,
                     pPatch_Out_Stream_Internal->pOutDataStream);

  if (err == DSERR_OK)
  {
      err = FinalFlushOutData(&pPatch_Out_Stream_Internal->OutPatchStream,
                     pPatch_Out_Stream_Internal->pOutDataStream);
  }

  return err;
}


int SVFAPI ClosePatchOutStreamBldFromDataStreamAndRamDif(PATCHSTREAMBLD_FROM_DATA_STREAM * pStrmBld,DFSPATCHANALYSEINFO_MEMORY *p_dfsPatchAnalyseInfoToFill)
{
  int err = DSERR_OK;
  PATCH_OUT_STREAM_INTERNAL* pPatch_Out_Stream_Internal;
  DFSPATCHANALYSEINFO_MEMORY dfsPatchAnamysisInfoAlt;
  pPatch_Out_Stream_Internal = (PATCH_OUT_STREAM_INTERNAL*)pStrmBld->statee;

  pPatch_Out_Stream_Internal->pOutDataStream = &pStrmBld->out_data_stream;

  err = ClosePatchBldFromDataStream (&pPatch_Out_Stream_Internal->RamDifBld_From_Data_Stream,p_dfsPatchAnalyseInfoToFill);


  GetSteamAnalysis(&pPatch_Out_Stream_Internal->OutPatchStream, &dfsPatchAnamysisInfoAlt);
  ReleaseOutPatchStream(&pPatch_Out_Stream_Internal->OutPatchStream);
  DfsFree(pPatch_Out_Stream_Internal);
  return err;
}

/***********************************************************************************************/

typedef struct
{
  PATCHSTREAMBLD_FROM_DATA_STREAM StrmBld;
  dfbyte dfIdenticalStreamArray[MAX_IDENTICAL_SIZE];
} PATCHSTREAMBLD_FROM_RAMDIF_INTERNAL;


int SVFAPI PatchOutStreamBldFromRamDif(PATCHSTREAMBLD_FROM_RAMDIF * pStrmBldRd,
                                       dfuLong64 dfExpectedSizeOfPatchResult,
                                       HRAMDIF hRamDifBase,
                                       dfuLong32 CompressRatio, dfuLong32 dfSizeButStreamKB)
{
    return PatchOutStreamBldFromRamDif_ReplaceOrigOutPlace(pStrmBldRd,
                                       dfExpectedSizeOfPatchResult,
                                       hRamDifBase,
                                       NULL,
                                       CompressRatio, dfSizeButStreamKB);
}

int SVFAPI PatchOutStreamBldFromRamDif_ReplaceOrigOutPlace(PATCHSTREAMBLD_FROM_RAMDIF * pStrmBldRd,
                                       dfuLong64 dfExpectedSizeOfPatchResult,
                                       HRAMDIF hRamDifBase,
                                       ORIGDATAPTR OrigDataPtr,
                                       dfuLong32 CompressRatio, dfuLong32 dfSizeButStreamKB)
{
    PATCHSTREAMBLD_FROM_RAMDIF_INTERNAL* pPatchStrm_Bld_FromRamDif_Internal;
    int err;
    pPatchStrm_Bld_FromRamDif_Internal = (PATCHSTREAMBLD_FROM_RAMDIF_INTERNAL*)DfsMalloc(sizeof(PATCHSTREAMBLD_FROM_RAMDIF_INTERNAL));
    if (pPatchStrm_Bld_FromRamDif_Internal == NULL)
        return DSERR_INTERNAL;


    pPatchStrm_Bld_FromRamDif_Internal->StrmBld.in_data_stream.total_in = 0;
    pPatchStrm_Bld_FromRamDif_Internal->StrmBld.in_data_stream.next_in =
             pPatchStrm_Bld_FromRamDif_Internal->dfIdenticalStreamArray;
    pPatchStrm_Bld_FromRamDif_Internal->StrmBld.in_data_stream.done_latest_in = 0;
    pPatchStrm_Bld_FromRamDif_Internal->StrmBld.in_data_stream.avail_in =
      MakeDifStFileIdentical( pPatchStrm_Bld_FromRamDif_Internal->dfIdenticalStreamArray,MAX_IDENTICAL_SIZE,dfExpectedSizeOfPatchResult);

    pPatchStrm_Bld_FromRamDif_Internal->StrmBld.out_data_stream = pStrmBldRd->out_data_stream ;
    err = PatchOutStreamBldFromDataStreamAndRamDifInit(&pPatchStrm_Bld_FromRamDif_Internal->StrmBld,
                                                       dfExpectedSizeOfPatchResult,hRamDifBase,CompressRatio, dfSizeButStreamKB);
    pStrmBldRd->out_data_stream = pPatchStrm_Bld_FromRamDif_Internal->StrmBld.out_data_stream ;

    if (err != DSERR_OK)
    {
        DfsFree(pPatchStrm_Bld_FromRamDif_Internal);
    }
    else
    {
        pStrmBldRd->state_ = (HPATCHSTREAMBLD_FROM_RAMDIF)pPatchStrm_Bld_FromRamDif_Internal;
    }
    return err;
}

int SVFAPI DoPatchOutStreamBldFromRamDif (PATCHSTREAMBLD_FROM_RAMDIF * pStrmBldRd)
{
    int err;
    PATCHSTREAMBLD_FROM_RAMDIF_INTERNAL* pPatchStrm_Bld_FromRamDif_Internal;
    pPatchStrm_Bld_FromRamDif_Internal = (PATCHSTREAMBLD_FROM_RAMDIF_INTERNAL*)pStrmBldRd->state_;

    pPatchStrm_Bld_FromRamDif_Internal->StrmBld.out_data_stream = pStrmBldRd->out_data_stream ;
    err = DoPatchOutStreamBldFromDataStream (&pPatchStrm_Bld_FromRamDif_Internal->StrmBld);
    pStrmBldRd->out_data_stream = pPatchStrm_Bld_FromRamDif_Internal->StrmBld.out_data_stream ;
    return err;
}

int SVFAPI FlushPatchOutStreamBldFromRamDif(PATCHSTREAMBLD_FROM_RAMDIF * pStrmBldRd)
{
    int err;
    PATCHSTREAMBLD_FROM_RAMDIF_INTERNAL* pPatchStrm_Bld_FromRamDif_Internal;
    pPatchStrm_Bld_FromRamDif_Internal = (PATCHSTREAMBLD_FROM_RAMDIF_INTERNAL*)pStrmBldRd->state_;

    pPatchStrm_Bld_FromRamDif_Internal->StrmBld.out_data_stream = pStrmBldRd->out_data_stream ;
    err = FlushPatchOutStreamBldFromDataStreamAndRamDif (&pPatchStrm_Bld_FromRamDif_Internal->StrmBld);
    pStrmBldRd->out_data_stream = pPatchStrm_Bld_FromRamDif_Internal->StrmBld.out_data_stream ;
    return err;
}

int SVFAPI ClosePatchStreamBldFromRamDif(PATCHSTREAMBLD_FROM_RAMDIF * pStrmBldRd,DFSPATCHANALYSEINFO_MEMORY *p_dfsPatchAnalyseInfoToFill)
{
    int err;
    PATCHSTREAMBLD_FROM_RAMDIF_INTERNAL* pPatchStrm_Bld_FromRamDif_Internal;
    pPatchStrm_Bld_FromRamDif_Internal = (PATCHSTREAMBLD_FROM_RAMDIF_INTERNAL*)pStrmBldRd->state_;

    pPatchStrm_Bld_FromRamDif_Internal->StrmBld.out_data_stream = pStrmBldRd->out_data_stream ;
    err = ClosePatchOutStreamBldFromDataStreamAndRamDif (&pPatchStrm_Bld_FromRamDif_Internal->StrmBld,p_dfsPatchAnalyseInfoToFill);
    pStrmBldRd->out_data_stream = pPatchStrm_Bld_FromRamDif_Internal->StrmBld.out_data_stream ;
    DfsFree(pPatchStrm_Bld_FromRamDif_Internal);
    return err;
}

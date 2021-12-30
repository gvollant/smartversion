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
#include "zlib.h"

#include "../common/difbasic.h"

#include "../common/difstrm.h"
#include "../common/DfsOrigMemoryMap.h"
#include "../common/difstrmi.h"
#include "../common/DfsTlTyp.h"
#include "../common/difstool.h"

#include "makdifst.h"

#include "MkDifStm.h"
#include "FindSeq.h"

#ifdef  DO_STATIS
#include "statis.h"
#include "tstahuff.h"
#define NB_STATIS (3+1)
#define NB_AHUFF (11)
#endif

#ifndef local
#define local static
#endif

#ifndef DEF_MEM_LEVEL
#if MAX_MEM_LEVEL >= 8
#  define DEF_MEM_LEVEL 8
#else
#  define DEF_MEM_LEVEL  MAX_MEM_LEVEL
#endif
#endif

//#define SHOWSTREAM
//#define NOQUIET
//#define NOQUIEST_ALIGN
/*
TODO :
--- real Multi OrigPtr support ---,crc-adler
- test streaming
- improve speed
*/

//#define SHOWSEQDEBUG
//#define SHOWSTREAM
#define SHOWSTREAMEND




//#define SIZE_PRE_READ_DATA (1024)//(1024/32)
//#define SIZE_PRE_READ_DATA_AFTER_FLUSH (32)
#define MDSAroundUpper(dwValue,dwModulo) (((((dwValue)) + ((dwModulo)) -1) / ((dwModulo))) * (dwModulo))

#define MAX_SEARCH_PARAMETERS (0x40)
#define SIZE_PRE_READ_DATA_AFTER_FLUSH (16)

#define INS_DEPL_POS_NONE  ((dfuLong64)-1)


typedef struct
{
/* constant (only depend of compression level) : */
  dfuLong32 between_search_ins;
  dfuLong32 min_size_depl;
  dfuLong32 min_size_search_depl;
  dfuLong32 min_before_search;

  dfuLong32 searchBeforeWindow;
  dfuLong32 searchAfterWindow;
  dfuLong32 continue_inplace;
  dfuLong32 align_possible;
}
SEARCH_PARAMETERS;

typedef struct
{
  dfuLong32 dfAlignValue ;
  dfuLong64 dfCountAligned ;
  dfuLong64 dfCountAlignedSize ;
  dfuLong64 dfCountUnaligned ;
  dfuLong64 dfCountUnalignedSize ;
} ALIGN_STATISTIQ;
#define NB_MAX_ALIGN_STATISTIQ (4)



typedef struct
{
  OUT_MULTI_BUFFER outPatchStream;

  BOOL    fFileIdentical;
  dfuLong32 uLastAdler32;

  dfuLong32 SearchInitDone;
  HFINDSEQINTL HFindSeqIntl;
  dfuLong64 curPosIns;
  dfuLong64 sizeorg;
  dfuLong64 curPos_newwritten;

  /* pre_read_data contain data read, we think to write byte insert.
     if pos_in_pre_read>0, new_ins_depl_pos == INS_DEPL_POS_NONE */
  dfbytep pre_read_data;
  dfuLong32 pre_read_data_size;
  dfuLong32 pos_in_pre_read;
  dfuLong32 pre_read_data_size_after_flush;

  dfuLong64 total_in_internal;

  dfuLong64 total_size_insert;
  dfuLong64 total_size_depl_in_place;
  dfuLong64 total_size_depl_out_place;
  //dfuLong64 total_out_internal;
  //dfuLong64 total_size_orig;

/* if the beginning of pre_read_data is in depl */

  dfuLong64 new_ins_depl_pos;
  dfuLong64 new_ins_depl_size;

  dfuLong32 size_before_next_search;
  dfuLong32 size_next_search;
  dfuLong32 cur_min_before_search ;

  dfuLong32 step_search;

  dfuLong32 dfStatiqDist[2][64];
  dfuLong32 dfCountAlignedSearch;
  dfuLong32 dfMaxAlignedSearchNumber;
  dfuLong32 dfAlignValue;

/*
  dfuLong32 dfAlignValue ;
  dfuLong64 dfCountAligned ;
  dfuLong64 dfCountAlignedSize ;
  dfuLong64 dfCountUnaligned ;
  dfuLong64 dfCountUnalignedSize ;
*/
  dfuLong32 dfNbAlignStatistiq;
  ALIGN_STATISTIQ AlignStatistiq[NB_MAX_ALIGN_STATISTIQ];


#ifdef DO_STATIS
  STATIS *statis[NB_STATIS];
  TSTHUFOUT *tsthufout[NB_AHUFF];
#endif

  dfuLong32 nb_search_parameters;
  dfuLong32 cur_search_parameters;

  COMPRESSIONPARAM cprParam;
  dfuLong32 dfBlockCalcSizeSearch;
  dfuLong32 dfMinimalAlign;
  SEARCH_PARAMETERS SearchParameters[MAX_SEARCH_PARAMETERS];

  dfbyte pre_read_data_tab[1];
}
INTERNAL_WRITESTATE;




dfuLong32 SVFAPI GetCompressionParamSize()
{
  return sizeof(COMPRESSIONPARAM);
}

void SVFAPI InitDefaultCompressionParam(COMPRESSIONPARAM* pCprParam)
{
    pCprParam->dfStructSize = sizeof(COMPRESSIONPARAM);
    pCprParam->uZlibCompressRatio = 9;
    pCprParam->dfBlockCalcSizeSearch = COMPRESSIONPARAM_AUTOVALUE;

    pCprParam->dfGeneralCompressionRatio = COMPRESSIONPARAM_AUTOVALUE;
    pCprParam->dfBlockSizeInFatLikeTable = COMPRESSIONPARAM_AUTOVALUE;
    pCprParam->dfBlockSizeInReduceTable = COMPRESSIONPARAM_AUTOVALUE;
    pCprParam->dfAlignPredict = COMPRESSIONPARAM_AUTOVALUE;
    pCprParam->dfSizeFactorAlignAcceptance = COMPRESSIONPARAM_AUTOVALUE;
    pCprParam->dfMaxAlignedSearchNumber = COMPRESSIONPARAM_AUTOVALUE;

    pCprParam->dfPhysicalMemoryKB = COMPRESSIONPARAM_AUTOVALUE;

    pCprParam->dfNbHashBit = COMPRESSIONPARAM_AUTOVALUE;

    pCprParam->dfMinimalSearchAlignement = COMPRESSIONPARAM_AUTOVALUE;
    pCprParam->dfSizeButStreamKB = DEFAULT_SIZE_BUF_STREAM_KB;
}

#define INTERNALRECOPYBUF (0x200)
/*
dfuLong32 MakeDifStFileIdentical(dfvoidp ptr, dfuLong32 sizeBuffer, dfuLong64 size64)
{
  int err;
  z_stream zstr;
  dfuLong32 size = (dfuLong32)size64;

  dfbyte tabStream0uncompressed[INTERNALRECOPYBUF];
  dfuLong32 sizeStream0uncompressed;
  dfbyte tabStream0compressed[INTERNALRECOPYBUF];
  dfuLong32 sizeStream0compressed;
  dfbyte tabResult[INTERNALRECOPYBUF];
  dfuLong32 dfResultSize;
  BOOL fFast=TRUE;

  if ((size == 0))
    {
      sizeStream0uncompressed = 0;
    }
  else if ((size < 0x1b))
    {
      tabStream0uncompressed[0] = 0x40 + 0x20 + (dfbyte) (size - 1);
      sizeStream0uncompressed = 1;
    }
  else if ((size < 0x100))
    {
      tabStream0uncompressed[0] = 0x40;
      tabStream0uncompressed[1] = (dfbyte) (size - 1);
      sizeStream0uncompressed = 2;
    }
  else
    {
      dfuLong32Intel sizeIntel;
      dfuLong32Intel posIntel = ConvertuLongToLongIntel(0);
      dfbyte bNbByteNb;
      bNbByteNb = GetNbByteNumber(size);

      tabStream0uncompressed[0] =
        ((dfbyte) (bNbByteNb + 0x7b)) | ((dfbyte) 0x80);

      sizeIntel = ConvertuLongToLongIntel(size);
      sizeStream0uncompressed = bNbByteNb + 1 + 4;
      DfsMemcpy(&tabStream0uncompressed[1], &sizeIntel, bNbByteNb);
      DfsMemcpy(&tabStream0uncompressed[1 + bNbByteNb], &posIntel, 4);
    }

  ClearZStream(&zstr);

  if (fFast)
  {
      dfuLong32 i;
      sizeStream0compressed = sizeStream0uncompressed + 5;
      for (i=0;i<sizeStream0compressed;i++)
          tabStream0compressed[i+5]=tabStream0uncompressed[i];
      tabStream0compressed[0]=1;
      tabStream0compressed[1]=((dfbyte)sizeStream0uncompressed-0);
      tabStream0compressed[2]=0;
      tabStream0compressed[3]=(dfbyte)(0xff - ((dfbyte)sizeStream0uncompressed));
      tabStream0compressed[4]=0xff;
  }
  else
  {
    err = deflateInit2(&zstr,
                        (int) Z_BEST_COMPRESSION,//Z_NO_COMPRESSION,//Z_BEST_SPEED, // Z_BEST_COMPRESSION,
                        Z_DEFLATED, -MAX_WBITS, DEF_MEM_LEVEL, 0);
    zstr.avail_in = sizeStream0uncompressed;
    zstr.next_in = tabStream0uncompressed;


    zstr.avail_out = INTERNALRECOPYBUF;
    zstr.next_out = tabStream0compressed;
    deflate(&zstr, Z_FINISH);
    sizeStream0compressed = zstr.total_out;
    deflateEnd(&zstr);
  }

  tabResult[0] =
    SMALLHEADER_SIZE_VALUE1 | SMALLHEADER_SIGN_VALUE |
    SMALLHEADER_ENDSTREAM_MASK;
  tabResult[1] = (dfbyte) sizeStream0compressed;
  DfsMemcpy(&tabResult[2], &tabStream0compressed[0], sizeStream0compressed);
  tabResult[sizeStream0compressed + 2] =
    SMALLHEADER_SIZE_VALUE0 | SMALLHEADER_SIGN_VALUE |
    SMALLHEADER_ENDSTREAM_MASK | 1;
  dfResultSize = sizeStream0compressed + 3;


  if (size == 0)
    {
      tabResult[0] =
        SMALLHEADER_SIZE_VALUE0 | SMALLHEADER_SIGN_VALUE |
        SMALLHEADER_ENDSTREAM_MASK;
      tabResult[1] =
        SMALLHEADER_SIZE_VALUE0 | SMALLHEADER_SIGN_VALUE |
        SMALLHEADER_ENDSTREAM_MASK | 1;
      dfResultSize = 2;
    }

    if (sizeBuffer<dfResultSize)
        return 0;
    DfsMemcpy(ptr,tabResult,dfResultSize);
  return dfResultSize;
}
*/

/****************************************************************/

int SVFAPI MakeDifInit(WRITEDIF_STREAM * write_stream, const COMPRESSIONPARAM* pCprParam)
{
  dfuLong32 i,j;
  dfbytep ptrAlloc;
  dfuLong32 CompressRatio=1;
  dfuLong32 dfBlockCalcSizeSearch;
  dfuLong32 dfAdd_sizep_pre_read_data;
  INTERNAL_WRITESTATE *internal;
  dfuLong64 total_size_orig;
  dfuLong32 nbBasisSearchParameter;

  // configure xz or gzip

  if (pCprParam!=NULL)
  {
      dfBlockCalcSizeSearch = pCprParam->dfBlockCalcSizeSearch;
      if (pCprParam->uZlibCompressRatio>=11)
          CompressRatio = pCprParam->uZlibCompressRatio;
  }
  else
      dfBlockCalcSizeSearch = COMPRESSIONPARAM_AUTOVALUE;

  total_size_orig = 0;
  for (j = 0; j < write_stream->nbOrig; j++)
      total_size_orig += ((write_stream->OrigDataPtr) + j)->size;

  if (dfBlockCalcSizeSearch == COMPRESSIONPARAM_AUTOVALUE)
  {
      if (total_size_orig < (1024 * 1024))
        dfBlockCalcSizeSearch = 8 ;
      else
      if (total_size_orig < (32 * 1024 * 1024))
        dfBlockCalcSizeSearch = 16 ;
      else
      if (total_size_orig < (32 * 1024 * 1024))
        dfBlockCalcSizeSearch = 32 ;
      else
      if (total_size_orig < (256 * 1024 * 1024))
        dfBlockCalcSizeSearch = 64 ;
      else
      if (total_size_orig < (1024 * 1024 * 1024))
        dfBlockCalcSizeSearch = 128 ;
      else
        dfBlockCalcSizeSearch = 256 ;

      // to not break default compatibility, to remove later
      if (dfBlockCalcSizeSearch > 0x40)
        dfBlockCalcSizeSearch = 0x40;
  }

  if ((total_size_orig >= (256 * 1024 * 1024)) && (dfBlockCalcSizeSearch < 32))
      dfBlockCalcSizeSearch = 32 ;

  if (dfBlockCalcSizeSearch < 4)
      dfBlockCalcSizeSearch = 4;

  if (dfBlockCalcSizeSearch > 0x40)
  {
    dfBlockCalcSizeSearch = (dfBlockCalcSizeSearch / 0x10) * 0x10;

    if (dfBlockCalcSizeSearch < 4)
      dfBlockCalcSizeSearch = 4;

    if (dfBlockCalcSizeSearch > 0x40)
    {
      dfuLong32 dfExpected = dfBlockCalcSizeSearch;
      dfBlockCalcSizeSearch = 0x40;
      while ((dfBlockCalcSizeSearch<dfExpected) && (dfBlockCalcSizeSearch<4096))
        dfBlockCalcSizeSearch*=2;
    }
  }

  while ((total_size_orig / dfBlockCalcSizeSearch) > 0xffff0000)
      dfBlockCalcSizeSearch = dfBlockCalcSizeSearch*2;

  if (dfBlockCalcSizeSearch > 0x2000)
      dfBlockCalcSizeSearch = 0x2000;



  dfAdd_sizep_pre_read_data = 0x400;
  if (dfBlockCalcSizeSearch>=0x80)
      dfAdd_sizep_pre_read_data = 0x300 + (dfBlockCalcSizeSearch * 4);

  dfAdd_sizep_pre_read_data = MDSAroundUpper(dfAdd_sizep_pre_read_data,0x100);

  write_stream->uDoChecksum = 1;


  ptrAlloc = (dfbytep) DfsMalloc(MDSAroundUpper(sizeof(INTERNAL_WRITESTATE)+dfAdd_sizep_pre_read_data,0x10));
  internal = (INTERNAL_WRITESTATE *) ptrAlloc;
  write_stream->state = (HWRITEDIF_INTERNALSTATE) internal;
  if (internal == NULL)
    return DSERR_INTERNAL;
/**+++*/
  write_stream->in_data_stream.next_in = NULL;
  write_stream->in_data_stream.avail_in = 0;
  write_stream->in_data_stream.total_in = 0;
  write_stream->in_data_stream.done_latest_in = 0;

  write_stream->out_data_stream.next_out = NULL;
  write_stream->out_data_stream.avail_out = 0;
  write_stream->out_data_stream.total_out = 0;
  write_stream->out_data_stream.done_latest_out = 0;
/**+++*/


  {
      int errInit = InitOutPatchStream(&internal->outPatchStream,CompressRatio, pCprParam->dfSizeButStreamKB) ;
      if (errInit != DSERR_OK)
      {
        DfsFree(internal);
        return DSERR_INTERNAL;
      }
  }/**+++*/

  internal->sizeorg = total_size_orig;
  internal->fFileIdentical = TRUE;

  internal->uLastAdler32 = adler32(0L, 0, 0);

  internal->curPosIns = 0;

  internal->total_in_internal = 0;
  //internal->total_out_internal = 0;


  internal->total_size_insert = 0;
  internal->total_size_depl_in_place = 0;
  internal->total_size_depl_out_place = 0;

  internal->pre_read_data = internal->pre_read_data_tab;
  internal->pre_read_data_size = dfAdd_sizep_pre_read_data;
  internal->pre_read_data_size_after_flush = dfmin(SIZE_PRE_READ_DATA_AFTER_FLUSH,dfBlockCalcSizeSearch);
  internal->pre_read_data_size_after_flush *= 1;
  internal->pos_in_pre_read = 0;

  internal->new_ins_depl_pos = INS_DEPL_POS_NONE;
  internal->new_ins_depl_size = 0;

  internal->curPos_newwritten = 0;


  InitDefaultCompressionParam(&internal->cprParam);
  /*
  internal->cprParam.uZlibCompressRatio = COMPRESSIONPARAM_AUTOVALUE;
  internal->cprParam.dfBlockCalcSizeSearch = COMPRESSIONPARAM_AUTOVALUE;
  internal->cprParam.dfGeneralCompressionRatio = COMPRESSIONPARAM_AUTOVALUE;
  internal->cprParam.dfBlockSizeInFatLikeTable = COMPRESSIONPARAM_AUTOVALUE;
  internal->cprParam.dfBlockSizeInReduceTable = COMPRESSIONPARAM_AUTOVALUE;
  internal->cprParam.dfAlignPredict = COMPRESSIONPARAM_AUTOVALUE;
  internal->cprParam.dfPhysicalMemoryKB = COMPRESSIONPARAM_AUTOVALUE;
  internal->cprParam.dfSizeFactorAlignAcceptance = COMPRESSIONPARAM_AUTOVALUE;
  internal->cprParam.dfMaxAlignedSearchNumber = COMPRESSIONPARAM_AUTOVALUE;
  */

  if (pCprParam!=NULL)
      internal->cprParam = *pCprParam;

  internal->dfBlockCalcSizeSearch = dfBlockCalcSizeSearch;

  if ((((int)internal->cprParam.uZlibCompressRatio)>=0) && (internal->cprParam.uZlibCompressRatio<=9))
      CompressRatio = internal->cprParam.uZlibCompressRatio;

#define SIZEFACTORALIGNACCEPTANCE (6+2)
#define MAXALIGNEDSEARCHNUMBER (8)

  if (internal->cprParam.dfSizeFactorAlignAcceptance == COMPRESSIONPARAM_AUTOVALUE)
      internal->cprParam.dfSizeFactorAlignAcceptance = SIZEFACTORALIGNACCEPTANCE;

  if (internal->cprParam.dfMaxAlignedSearchNumber == COMPRESSIONPARAM_AUTOVALUE)
      internal->cprParam.dfMaxAlignedSearchNumber = MAXALIGNEDSEARCHNUMBER;

  for (i = 0; i < 64; i++)
    internal->dfStatiqDist[0][i] = internal->dfStatiqDist[1][i] = 0;

/* constant (only depend of compression level) : */
  internal->SearchParameters[0].between_search_ins = 0;//12 - 5;
  internal->SearchParameters[0].min_size_depl = (1*internal->dfBlockCalcSizeSearch)-(0*1);
  //internal->SearchParameters[0].min_size_search_depl = 10 - 2;
  internal->SearchParameters[0].min_size_search_depl = internal->SearchParameters[0].min_size_depl;
  internal->SearchParameters[0].min_size_search_depl += internal->SearchParameters[0].min_size_depl-1; // new compo


  internal->SearchParameters[0].min_before_search = 3 - 1;
  internal->SearchParameters[0].searchBeforeWindow = 0;
  internal->SearchParameters[0].searchAfterWindow = 0;
  internal->SearchParameters[0].continue_inplace = 0;

  internal->step_search = 0;

  internal->dfCountAlignedSearch = 0;
  internal->dfMaxAlignedSearchNumber = 1;
  internal->dfAlignValue = 1;

  internal->dfMinimalAlign = 1;

  internal->dfNbAlignStatistiq = 0;

  if (dfBlockCalcSizeSearch >= 0x10)
  {
      internal->AlignStatistiq[0].dfAlignValue = 0x10;
      internal->dfNbAlignStatistiq = 1;
  }

  if (dfBlockCalcSizeSearch >= 0x40)
  {
      internal->AlignStatistiq[1].dfAlignValue = dfBlockCalcSizeSearch;
      internal->dfNbAlignStatistiq = 2;
  }

  for (i=0;i<(internal->dfNbAlignStatistiq);i++)
  {
    internal->AlignStatistiq[i].dfCountAligned = 0;
    internal->AlignStatistiq[i].dfCountAlignedSize = 0;
    internal->AlignStatistiq[i].dfCountUnaligned = 0;
    internal->AlignStatistiq[i].dfCountUnalignedSize = 0;
  }


  if (internal->cprParam.dfMinimalSearchAlignement != COMPRESSIONPARAM_AUTOVALUE)
  {
    internal->dfMinimalAlign = dfmin(dfBlockCalcSizeSearch,internal->cprParam.dfMinimalSearchAlignement);
  }

  internal->nb_search_parameters = 1;
  internal->cur_search_parameters = 0;


  internal->SearchParameters[1] = internal->SearchParameters[0];
  internal->nb_search_parameters = 2;
  internal->SearchParameters[1].continue_inplace = 1;
  internal->SearchParameters[0].searchBeforeWindow = 0x1000;
  internal->SearchParameters[0].searchAfterWindow = 0x2000;
  internal->SearchParameters[0].align_possible = 0;

  internal->SearchParameters[2] = internal->SearchParameters[0];
  internal->SearchParameters[1] = internal->SearchParameters[0];
  internal->nb_search_parameters = 3;
  internal->SearchParameters[2].continue_inplace = 1;
  internal->SearchParameters[1].continue_inplace = 1;
  internal->SearchParameters[0].searchBeforeWindow = 0x200*1;
  internal->SearchParameters[0].searchAfterWindow = 0x200*1;
  internal->SearchParameters[1].searchBeforeWindow = 0x200*1;
  internal->SearchParameters[1].searchAfterWindow = 0x200*1;
  internal->SearchParameters[2].searchBeforeWindow = 0x0;
  internal->SearchParameters[2].searchAfterWindow = 0x0;
  internal->SearchParameters[2].align_possible = 1;


  internal->size_next_search =
    internal->SearchParameters[internal->cur_search_parameters].min_before_search +
    internal->SearchParameters[internal->cur_search_parameters].min_size_search_depl;
  internal->size_before_next_search = internal->size_next_search;
  internal->cur_min_before_search = internal->SearchParameters[internal->cur_search_parameters].min_before_search;


  if (CompressRatio > 9)
    CompressRatio = 9;
  if (CompressRatio < 1)
    CompressRatio = 3;

  nbBasisSearchParameter=3;
  /*
  if (dfBlockCalcSizeSearch > 0x20)
  {
    internal->SearchParameters[0] = internal->SearchParameters[1];
    internal->SearchParameters[1] = internal->SearchParameters[2];
    nbBasisSearchParameter=2;
  }
*/

  for (i = 0; i < ((MAX_SEARCH_PARAMETERS / 3) - 4); i++)
  {
    dfuLong32 dfByteWindow;
    dfuLong32 pos = (i + 1) * nbBasisSearchParameter;
    // dfByteWindow = (i*8)+14; // VERY SLOW
    // dfByteWindow = (i*3)+14; // SLOW
    // dfByteWindow = ((i/2))+12; // VERY FAST
    dfuLong32 factI, addByteWindow,j;

    if (CompressRatio >= 3)
      addByteWindow = 14;
    else if (CompressRatio == 2)
      addByteWindow = 13;
    else
      addByteWindow = 12;

    if (CompressRatio >= 9)
      factI = 8;
    else if (CompressRatio >= 3)
      factI = (CompressRatio - 1) / 2;
    else
      factI = 0;

    dfByteWindow =
      (factI > 0) ? ((i * factI) + addByteWindow) : ((i / 2) + addByteWindow);


    internal->SearchParameters[pos - 1].searchBeforeWindow =
      internal->SearchParameters[pos - 1].searchAfterWindow =
      (1 << dfByteWindow);
    if ((dfByteWindow > 30) /* || (internal->SearchParameters[pos - 1].searchAfterWindow >= total_size_orig) */)
    {
      internal->SearchParameters[pos - 1].searchBeforeWindow =
        internal->SearchParameters[pos - 1].searchAfterWindow = 0;
      internal->nb_search_parameters = pos;
      break;
    }
    for (j=0;j<nbBasisSearchParameter;j++)
        internal->SearchParameters[pos+j] = internal->SearchParameters[(pos+j) - nbBasisSearchParameter];

    internal->SearchParameters[pos + nbBasisSearchParameter -1].searchBeforeWindow =
      internal->SearchParameters[pos + nbBasisSearchParameter -1].searchAfterWindow = 0;
    internal->nb_search_parameters = pos + nbBasisSearchParameter;
  }


  {
      dfuLong32 i;

      for (i=0;i+1<internal->nb_search_parameters;i++)
      {
          if (internal->SearchParameters[i+1].continue_inplace==1)
          {
              dfuLong32 j,dif=0;
              const unsigned char* lpCmp1=(const unsigned char*)&(internal->SearchParameters[i]);
              const unsigned char* lpCmp2=(const unsigned char*)&(internal->SearchParameters[i+1]);
              for (j=0;j<sizeof(SEARCH_PARAMETERS);j++)
                  if ((*(lpCmp1++))!=(*(lpCmp2++)))
                  {
                      dif=1;
                      break;
                  }
             if (dif==0)
             {
                 dfuLong32 j;
                  for (j=i;j+1<internal->nb_search_parameters;j++)
                      internal->SearchParameters[j]=internal->SearchParameters[j+1];

                  internal->nb_search_parameters--;
             }
          }
/*
          if ((internal->SearchParameters[i]==internal->SearchParameters[i+1]) &&
              (internal->SearchParameters[i+1].continue_inplace==1))
*/
      }
  }

  internal->SearchParameters[0].continue_inplace = 1;
  internal->SearchParameters[1].continue_inplace = 0;
  internal->SearchParameters[0].searchBeforeWindow = 0;
  internal->SearchParameters[0].searchAfterWindow = 0;
  internal->SearchParameters[1].searchBeforeWindow = 0;
  internal->SearchParameters[1].searchAfterWindow = 0;
  internal->SearchParameters[2].searchBeforeWindow = 0x0;
  internal->SearchParameters[2].searchAfterWindow = 0x0;
  internal->SearchParameters[0].align_possible = 0;
  internal->SearchParameters[1].align_possible = 0;
  internal->nb_search_parameters = 2;

  internal->SearchParameters[0].continue_inplace = 0;




  internal->SearchParameters[0].min_size_search_depl += 0x0;
  for (i=0;i<6;i++)
  {
        if (i>0)
        {
          internal->SearchParameters[i] = internal->SearchParameters[i-1];
          internal->SearchParameters[i-1].align_possible = 1;
        }
        internal->SearchParameters[i].continue_inplace = 0;
        internal->SearchParameters[i].searchBeforeWindow = 0;
        internal->SearchParameters[i].searchAfterWindow = 0;
        internal->SearchParameters[i].align_possible = 0;
  }
  internal->nb_search_parameters = i;


  internal->SearchInitDone = 0;
  internal->HFindSeqIntl = NULL;

#ifdef DO_STATIS
  for (i = 0; i < NB_STATIS; i++)
    internal->statis[i] = NULL;
  for (i = 0; i < NB_AHUFF; i++)
    internal->tsthufout[i] = NULL;
#endif


  return DSERR_OK;
}



/****++*/



int WriteInsertBytes(WRITEDIF_STREAM *write_stream, dfvoidp data, dfuLong64 size)
{
    OUT_MULTI_BUFFER *pOutPatchStream;
    OUT_DATA_STREAM* pOutDataStream;
    INTERNAL_WRITESTATE* internal;

    internal = (INTERNAL_WRITESTATE*)(write_stream -> state);
    pOutDataStream = &(write_stream ->out_data_stream);
    pOutPatchStream = &(internal->outPatchStream);
    if (size>0)
    {
        internal ->fFileIdentical = FALSE;
        internal -> curPosIns += size;      /*+++* */
        internal -> curPos_newwritten += size;
        internal -> total_size_insert += size;
    }
    return WriteInsertBytesInStream(pOutPatchStream,pOutDataStream,data,size);
}

int WriteInsertDepl(WRITEDIF_STREAM *write_stream, dfuLong64 posIntPtr, dfuLong64 size)
{
    OUT_MULTI_BUFFER *pOutPatchStream;
    OUT_DATA_STREAM* pOutDataStream;
    INTERNAL_WRITESTATE* internal;
    dfuLong64 posCurOrg;
    dfuLong64 pos= (dfuLong64)posIntPtr; //6432 // 3264
    internal = (INTERNAL_WRITESTATE*)(write_stream -> state);
    posCurOrg = internal->curPosIns;

    if (size == 0)
        return DSERR_OK;

    if (internal->fFileIdentical != FALSE)
    {
        if (pos != posCurOrg)
            internal->fFileIdentical = FALSE;
    }

    if (pos == internal->curPos_newwritten)
        internal -> total_size_depl_in_place += size;
    else
        internal -> total_size_depl_out_place += size;



    pOutDataStream = &(write_stream ->out_data_stream);
    pOutPatchStream = &(internal->outPatchStream);


    internal->curPosIns = pos + size;
    internal->curPos_newwritten += size;
    return WriteInsertDeplInStream(pOutPatchStream,pOutDataStream,posCurOrg,posIntPtr,size);
}

//void FlushOutInStream(OUT_MULTI_BUFFER *pOutPatchStream,OUT_DATA_STREAM* pOutDataStream)
void FlushOut(WRITEDIF_STREAM *write_stream)
{
    OUT_MULTI_BUFFER *pOutPatchStream;
    OUT_DATA_STREAM* pOutDataStream;
    INTERNAL_WRITESTATE* internal;

    internal = (INTERNAL_WRITESTATE*)(write_stream -> state);
    pOutDataStream = &(write_stream ->out_data_stream);
    pOutPatchStream = &(internal->outPatchStream);
    FlushOutInStream(pOutPatchStream,pOutDataStream);
}


/*
local int WriteSeqData (write_stream, nbStream, data, size)
    WRITEDIF_STREAM *write_stream;
    int nbStream;
    dfvoidp data;
    dfuLong32 size;
{
    return 0;
}
*/

/*
int
FlushMakeDif (WRITEDIF_STREAM *write_stream)
{
    int err = DSERR_OK;
    INTERNAL_WRITESTATE* internal;

    internal = (INTERNAL_WRITESTATE*)(write_stream -> state);

    FlushOut(write_stream);
    while ((internal -> nextStreamToFlush <= LAST_STREAM) &&
           (write_stream->out_data_stream.avail_out > 0))
    {
        FlushWriteStreamData(write_stream, internal -> nextStreamToFlush,
                                Z_FINISH);
        internal -> nextStreamToFlush++;
        FlushOut(write_stream);
    }

    if ((internal -> nextStreamToFlush > LAST_STREAM) && (err == DSERR_OK) &&
        (internal -> outBufferPos == 0))
        err = DSERR_END;
    return err;
}
*/
int SVFAPI CloseMakeDif(WRITEDIF_STREAM * write_stream, dfuLong32 * uChecksum,BOOL* pfFileIdentical)
{
    return  CloseMakeDifEx(write_stream, uChecksum,pfFileIdentical,NULL);
}

int SVFAPI CloseMakeDifEx(WRITEDIF_STREAM * write_stream, dfuLong32 * uChecksum, BOOL* pfFileIdentical,
  DFSPATCHANALYSEINFO_MEMORY* pDfsPatchAnalyseInfo)
{
  int i;
  int err = DSERR_OK;
  INTERNAL_WRITESTATE *internal;

  internal = (INTERNAL_WRITESTATE *) (write_stream->state);

#if defined(NOQUIET)
  {
      char szInfo[0x200];
      sprintf(szInfo,"insert=%lu, inplace=%lu outplace=%lu for total=%lu\n",
          (unsigned long)internal->total_size_insert,(unsigned long)internal->total_size_depl_in_place,(unsigned long)internal->total_size_depl_out_place,
          (unsigned long)internal->total_size_insert+(unsigned long)internal->total_size_depl_in_place+(unsigned long)internal->total_size_depl_out_place);
      puts(szInfo);
  }
#endif

  if (pDfsPatchAnalyseInfo != NULL)
  {
      //DFSPATCHANALYSEINFO_MEMORY
      pDfsPatchAnalyseInfo->total_size_insert = internal->total_size_insert;
      pDfsPatchAnalyseInfo->total_size_depl_in_place = internal->total_size_depl_in_place;
      pDfsPatchAnalyseInfo->total_size_depl_out_place = internal->total_size_depl_out_place;
  }

  if (pfFileIdentical != NULL)
  {
      *pfFileIdentical = FALSE;
      if (internal->fFileIdentical)
      {
          dfuLong32 j;
          dfuLong64 total_size_orig;

          total_size_orig = 0;
          for (j = 0; j < write_stream->nbOrig; j++)
              total_size_orig += ((write_stream->OrigDataPtr) + j)->size;

          if (internal->total_in_internal == total_size_orig)
              *pfFileIdentical = TRUE;
      }
  }
  if (uChecksum != NULL)
    *uChecksum = internal->uLastAdler32;

  {
    dfuLong32 tot = 0;

    //dfuLong32 totdisp = 0;
    for (i = 0; i < 64; i++)
      tot += internal->dfStatiqDist[0][i] + internal->dfStatiqDist[1][i];


#if defined(NOQUIET) || defined(NOQUIEST_ALIGN)
    for (i=0;i<(int)(internal->dfNbAlignStatistiq);i++)
      printf("Align on %u= %u, unalign= %u /By Size: align= %u, unalign= %u\n",
        internal->AlignStatistiq[i].dfAlignValue,
        internal->AlignStatistiq[i].dfCountAligned,
        internal->AlignStatistiq[i].dfCountUnaligned,
        internal->AlignStatistiq[i].dfCountAlignedSize,
        internal->AlignStatistiq[i].dfCountUnalignedSize);
#endif

#if defined(NOQUIET)
    {
      dfuLong32 thisval;
      for (i = 0; i < 64; i++)
        if ((thisval =
             (internal->dfStatiqDist[0][i] + internal->dfStatiqDist[1][i])) >
            0)
          printf("%x,0x%x\t%u\t%u\t%u %% %u %%\n", i, 1 << i,
                 internal->dfStatiqDist[0][i], internal->dfStatiqDist[1][i],
                 (thisval * 100) / tot, ((totdisp += thisval) * 100) / tot);
    }
#endif
  }

  //{
  //  DFSPATCHANALYSEINFO_MEMORY dfsPatchAnamysisInfoAlt;
  //  GetSteamAnalysis(&internal->outPatchStream, &dfsPatchAnamysisInfoAlt);
  //}

  ReleaseOutPatchStream(&internal->outPatchStream);

#ifdef DO_STATIS
  for (i = 0; i < NB_STATIS; i++)
    if (internal->statis[i] != NULL)
      DeleteStatis(internal->statis[i]);
  for (i = 0; i < NB_AHUFF; i++)
    if (internal->tsthufout[i] != NULL)
      CloseHufOut(internal->tsthufout[i]);
#endif

  DfsFree(internal);
  return err;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

/*
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
int DoMakeDifWork OF((WRITEDIF_STREAM *write_stream));

int DoMakeDifWork (write_stream)
    WRITEDIF_STREAM *write_stream;
{
    int err = DSERR_OK;
    int i=0;
    INTERNAL_WRITESTATE* internal;

    internal = (INTERNAL_WRITESTATE*)(write_stream -> state);

    FlushOut(write_stream);


    for (i=0;i<16*2;i++)
    {
        char szTst[512];
        sprintf(szTst,"Test_ONE %u,",i);
        WriteStreamData(write_stream,0,szTst,strlen(szTst));

        sprintf(szTst,"Test_two %u,",i);
        WriteStreamData(write_stream,1,szTst,strlen(szTst));

        sprintf(szTst,"Test_ONE %u bis,",i);
        WriteStreamData(write_stream,0,szTst,strlen(szTst));
    }

    FlushOut(write_stream);
DFASSERT(0==1);

    return err;
}
*/




/* write beginning of the sequence
    highBit = 0 : WriteInsert Bytes
    highBit = 1 : WriteInsert Depl
    */

local BOOL
GetOrigForPos(WRITEDIF_STREAM * write_stream, dfuLong64 pos_wanted,
              dfuLong32 * num_orig, dfuLong64 * pos_of_orig)
{
  dfuLong32 i;
  dfuLong64 dfCurPos = 0;
  for (i = 0; i < write_stream->nbOrig; i++)
  {
    ORIGDATAPTR curOrigData = (write_stream->OrigDataPtr) + i;
    if ((pos_wanted >= dfCurPos) &&
        (pos_wanted < dfCurPos + curOrigData->size))
    {
      *num_orig = i;
      *pos_of_orig = dfCurPos;
      return TRUE;
    }

    dfCurPos += curOrigData->size; // 6432
  }
  return FALSE;
}

/*
dfvoidp GetDataPtrFromOrigData(ORIGDATA* pOrigData)
{
    return GetOrigDataPtrpData(pOrigData);
}
*/
/*
Find the sequence that begin at lpSeqRech, size dwMaxRech
we find in lpFileOrg, size dwSizeOrg
dwBeginSearch is the offset of lpFileOrg where we begin search
dwMaxSearchOrg is the offset of lpFileOrg where we stop search
(if dwMaxSearchOrg == 0, we search until end)
dwLatestOrg : latest org found (to be closest possible if no change size)
dwMinInter : the minimum size we want see
*posOrg will contain the position
*dwSizeOrgRcp will contain the size
return 1 if OK

BOOL FindSeq(HFINDSEQINTL hfsq,
             dfbytep lpSeqRech,dfuLong32 dwMaxRech,
             dfbytep lpFileOrg,dfuLong32 dwSizeOrg,
             dfuLong32 dwBeginSearch,dfuLong32 dwMaxSearchOrg,
             dfuLong32 dwLatestOrg,dfuLong32 dwMinInter, // for info-optimize
             dfuLong32 dfAlign, dfuLong32 *dfSkipBeginSearch,
             dfuLong32 *posOrg,dfuLong32 *dwSizeOrgRcp)
BOOL StupidFindSeq(HFINDSEQINTL hfsq,
                   dfbytep lpSeqRech, dfuLong32 dwMaxRech,
                   dfbytep lpFileOrg, dfuLong32 dwSizeOrg,
                   dfuLong32 dwBeginSearch, dfuLong32 dwMaxSearchOrg,
                   dfuLong32 dwLatestOrg, dfuLong32 dwMinInter,
                   dfuLong32 dfAlign, dfuLong32 *dfSkipBeginSearch,
                   dfuLong32 * posOrg, dfuLong32 * dwSizeOrgRcp);
*/


static BOOL
FindSeqOrigPtr(HFINDSEQINTL hfsq,
               dfbytep lpSeqRech, dfuLong32 dwMaxRech,
               WRITEDIF_STREAM * write_stream,
               dfuLong64 dwBeginSearch, dfuLong64 dwMaxSearchOrg,
               dfuLong64 dwLatestOrg, dfuLong32 dwMinInter,
               dfuLong32 dfLengthStopSearch,
               dfuLong32 dfAlign, dfuLong32 *dfSkipBeginSearch,
               dfuLong64 * posOrg, dfuLong32 * dwSizeOrgRcp)
{
  BOOL fRet = FALSE;
  dfuLong32 num_orig;
  dfuLong64 pos_of_orig;
  if (GetOrigForPos(write_stream, dwBeginSearch, &num_orig, &pos_of_orig))
  {
    //dfbytep ptr_;
    dfuLong64 size;
    dfuLong64 sizeofsearching;

    size = (dfuLong64)(((write_stream->OrigDataPtr) + num_orig)->size);
    if ((size > dwMaxSearchOrg) && (dwMaxSearchOrg != 0))
      size = dwMaxSearchOrg;
    if ((size < dwMaxSearchOrg) && (dwMaxSearchOrg != 0))
      dwMaxSearchOrg = size;
    if (dwLatestOrg >= size)
        dwLatestOrg = (size==0) ? 0 : (size-1);
    //ptr_ = (dfbytep) GetDataPtrFromOrigData(((write_stream->OrigDataPtr) + num_orig));
    *posOrg = *dwSizeOrgRcp = 0;


    if (dwMaxSearchOrg == 0)
      sizeofsearching = size - dwBeginSearch;
    else
      sizeofsearching = dwMaxSearchOrg - dwBeginSearch;

    if (sizeofsearching > 0x000000)
      fRet = FindSeq(hfsq,
                     lpSeqRech, dwMaxRech,
                     (write_stream->OrigDataPtr) + num_orig, size, dwBeginSearch, dwMaxSearchOrg,
                     dwLatestOrg, dwMinInter,   // for info-optimize
                     dfLengthStopSearch,
                     dfAlign, dfSkipBeginSearch,
                     posOrg, dwSizeOrgRcp);
  }
  return fRet;
}


local dfuLong32
CheckSameByteThanInDepl(WRITEDIF_STREAM * write_stream,
                        dfvoidp newdata, dfuLong64 dwBeginSearch,
                        dfuLong32 maxsize, dfuLong32 direction)
{
  dfuLong32 sizeRet = 0;
  dfuLong32 num_orig;
  dfuLong64 pos_of_orig;

  if (GetOrigForPos(write_stream, dwBeginSearch, &num_orig, &pos_of_orig))
  {
    dfbytep ptr=NULL;
    ORIGDATA* pOrg=(write_stream->OrigDataPtr) + num_orig;

    if (direction == 1)
    {
      dfuLong64 pos_in_cur_org;
      dfuLong64 size_end_org;
      dfuLong32 i;
      dfuLong64 maxsize_check;
      pos_in_cur_org = dwBeginSearch - pos_of_orig;
      size_end_org = (dfuLong64)(((write_stream->OrigDataPtr) + num_orig)->size -
        pos_in_cur_org); //6432
      maxsize_check = maxsize;
      if (maxsize_check > size_end_org)
        maxsize_check = size_end_org;

      for (i = 0; i < maxsize_check; i++)
      {
        if ((i & GetMaxOrigDataExigibleSizeViewMask(pOrg)) == 0)
        //if ((i==0) || (GetMaxOrigDataExigibleSizeView(pOrg)<maxsize_check))
          ptr = (dfbytep)GetOrigDataPtrpDataBySizeView(pOrg,
                          pos_in_cur_org + i,
                          dfmin(GetMaxOrigDataExigibleSizeView(pOrg),maxsize_check-i));

        if ((*(((dfbytep) (ptr)) + pos_in_cur_org + i)) !=
            (*(((dfbytep) newdata) + i)))
          break;
      }

      sizeRet = i;
#if (defined (DEBUG) || defined (_DEBUG)) && defined(CHECK_CMP)
      if (sizeRet>0)
        if (memcmp(newdata, ((dfbytep) GetOrigDataPtrpDataBySizeView(pOrg,dwBeginSearch,sizeRet)+ dwBeginSearch),
                    sizeRet) != 0)
            printf("ERRcmp1\t");
#endif
    }

    if (direction == 0)
    {
      dfuLong64 pos_in_cur_org;
      dfuLong32 i;
      dfuLong32 maxsize_check;
      pos_in_cur_org = dwBeginSearch - pos_of_orig;
      maxsize_check = maxsize;
      if (maxsize_check > (pos_in_cur_org + 1))
        maxsize_check = (dfuLong32)(pos_in_cur_org + 1);

      for (i = 0; i < maxsize_check; i++)
      {
        if ((i & GetMaxOrigDataExigibleSizeViewMask(pOrg)) == 0)
        {
            dfuLong32 dfLenWorkWindow = (dfuLong32)(dfmin(GetMaxOrigDataExigibleSizeView(pOrg),maxsize_check));
            //dfLenWorkWindow = dfmin(dfLenWorkWindow,pos_in_cur_org+1);
            ptr = (dfbytep)GetOrigDataPtrpDataBySizeView(pOrg,
                        pos_in_cur_org - (dfLenWorkWindow - 1),dfLenWorkWindow);
        }

        if ((*(((dfbytep) (ptr)) + pos_in_cur_org - i)) !=
            (*(((dfbytep) newdata) - i)))
          break;
      }

      sizeRet = i;

#if (defined (DEBUG) || defined (_DEBUG)) && defined(CHECK_CMP)
      if (sizeRet>0)
        if (memcmp(((dfbytep) newdata)-(sizeRet - 1),
            (((dfbytep) GetOrigDataPtrpDataBySizeView(pOrg,dwBeginSearch-(sizeRet - 1),sizeRet))+ dwBeginSearch-(sizeRet - 1)),
                    sizeRet) != 0)
            printf("ERRcmp0\t");
#endif
    }
  }


  return sizeRet;
}

local dfuLong32
CheckSameByteThanInDeplSoon(WRITEDIF_STREAM * write_stream,
                            dfvoidp newdata, dfuLong64 dwBeginSearch,
                            dfuLong32 maxsize, dfuLong32 minsize, dfuLong32 * pos)
{
  dfuLong32 sizeRet = 0;
  dfuLong32 num_orig;
  dfuLong64 pos_of_orig;

  if (GetOrigForPos(write_stream, dwBeginSearch, &num_orig, &pos_of_orig))
  {
    dfbytep ptr=NULL;
    dfuLong64 pos_in_cur_org;
    dfuLong64 size_end_org;
    dfuLong32 i;
    dfuLong32 maxsize_check;
    dfuLong32 pos_begin_ok = 0;
    ORIGDATA* pOrg;

    *pos = 0;

    pos_in_cur_org = dwBeginSearch - pos_of_orig;
    size_end_org = (dfuLong64)(((write_stream->OrigDataPtr) + num_orig)->size -
      pos_in_cur_org); // 6432
    maxsize_check = maxsize;
    if (maxsize_check > size_end_org)
      maxsize_check = (dfuLong32)size_end_org;

    pOrg = (write_stream->OrigDataPtr) + num_orig;

    for (i = 0; i < maxsize_check; i++)
    {
      if ((i & GetMaxOrigDataExigibleSizeViewMask(pOrg)) == 0)
          ptr = (dfbytep)GetOrigDataPtrpDataBySizeView(pOrg,
                        pos_in_cur_org + i,
                        dfmin(GetMaxOrigDataExigibleSizeView(pOrg),maxsize_check-i));

      if ((*(((dfbytep) (ptr + pos_in_cur_org + i)))) != (*(((dfbytep) newdata) + i)))
      {
        if (sizeRet >= minsize)
          break;
        else
        {
          sizeRet = 0;
          pos_begin_ok = i + 1;
        }
      }
      else
        sizeRet++;
    }
    if (sizeRet >= minsize)
      *pos = pos_begin_ok;
    else
      sizeRet = 0;
  }
  return sizeRet;
}

void PreReadDataFlushIfNeeded(WRITEDIF_STREAM * write_stream,INTERNAL_WRITESTATE *internal)
{
    if (internal->size_before_next_search +
        internal->pos_in_pre_read >= internal->pre_read_data_size)
    {
        dfuLong32 i, j;
        dfuLong32 dfKeep =
            dfmin(internal->pre_read_data_size_after_flush,
                internal->pos_in_pre_read);
        WriteInsertBytes(write_stream,
                            internal->pre_read_data,
                            internal->pos_in_pre_read - dfKeep);
        for (i = 0, j = internal->pos_in_pre_read - dfKeep;
                i < dfKeep; i++, j++)
            internal->pre_read_data_tab[i] =
            internal->pre_read_data_tab[j];
        internal->pos_in_pre_read = dfKeep;
    }
}


// to examine alignement, compare internal->curPos_newwritten and internal->new_ins_depl_pos
void CheckComputeStatistiqAlignement(INTERNAL_WRITESTATE * internal,dfuLong64 pos,dfuLong64 size, BOOL fAlignExpected)
{
    dfuLong32 i;
/*
    if (internal->dfAlignValue == 1)
    {
                    internal->dfMaxAlignedSearchNumber = internal->cprParam.dfMaxAlignedSearchNumber;
                    internal->dfAlignValue = internal->AlignStatistiq[0].dfAlignValue;
                    internal->dfCountAlignedSearch = 0;
    }
*/
    if (internal->dfNbAlignStatistiq == 0)
        return ;

    if ((internal->AlignStatistiq[0].dfCountAlignedSize + internal->AlignStatistiq[0].dfCountUnalignedSize) > 128*(dfuLong64)1024)
    {
        internal->dfCountAlignedSearch = 0;
        internal->dfMaxAlignedSearchNumber = 1;
        internal->dfAlignValue = 1;

        //i = internal->dfNbAlignStatistiq;
        //while (i>0)
        if (internal->cprParam.dfSizeFactorAlignAcceptance > 0)
            for (i=0;i<internal->dfNbAlignStatistiq;i++)
            {
                //i--;

                //internal->AlignStatistiq[i].dfCountAligned >>=1;
                internal->AlignStatistiq[i].dfCountAlignedSize >>=1;
                //internal->AlignStatistiq[i].dfCountUnaligned>>=1;
                internal->AlignStatistiq[i].dfCountUnalignedSize>>=1;

                if ((internal->AlignStatistiq[i].dfCountAlignedSize / internal->cprParam.dfSizeFactorAlignAcceptance) >
                        internal->AlignStatistiq[i].dfCountUnalignedSize)
                {
                    internal->dfMaxAlignedSearchNumber = internal->cprParam.dfMaxAlignedSearchNumber;
                    internal->dfAlignValue = internal->AlignStatistiq[i].dfAlignValue;
                }
            }
    }

    for (i=0;i<internal->dfNbAlignStatistiq;i++)
    {
        dfuLong32 dfAlignValue = internal->AlignStatistiq[i].dfAlignValue;
        BOOL fSameAlign ;
        fSameAlign = ((internal->curPos_newwritten) % dfAlignValue) == ((pos) % dfAlignValue);
        if (fSameAlign)
        {
            internal->AlignStatistiq[i].dfCountAligned++;
            internal->AlignStatistiq[i].dfCountAlignedSize += size;
        }
        else
        {
            internal->AlignStatistiq[i].dfCountUnaligned++;
            internal->AlignStatistiq[i].dfCountUnalignedSize+= size;
        }
    }
}


int SVFAPI DoMakeDifWork(WRITEDIF_STREAM * write_stream)
{
  int err = DSERR_OK;
  //int i = 0;
  INTERNAL_WRITESTATE *internal;
  dfuLong32 dwWindowSmallSearchUnit, dwStepSearch, dwDoFast;


  write_stream->in_data_stream.done_latest_in = 0;
  write_stream->out_data_stream.done_latest_out = 0;




  dwDoFast = 5;

  dwWindowSmallSearchUnit = (dwDoFast < 4) ? 0x40 :
    ((dwDoFast < 7) ? 0x200 : 0x400);
  dwStepSearch = (dwDoFast < 3) ? 5 : ((dwDoFast < 5) ? 3 : 3);
  dwStepSearch = 2 + 1;



  internal = (INTERNAL_WRITESTATE *) (write_stream->state);

  //FlushOut(write_stream);


  while ((write_stream->in_data_stream.avail_in > 0) && (write_stream->out_data_stream.avail_out > 0) &&
         (err == DSERR_OK))
  {
    // stupid code:
    //if (internal -> pre_read_data_size==0)
    //    internal -> new_ins_depl_pos = INS_DEPL_POS_NONE;

    FlushOut(write_stream);
    if (internal->new_ins_depl_pos != INS_DEPL_POS_NONE)
    {                           /* we are in a ins depl */
      dfuLong32 dfHow = CheckSameByteThanInDepl(write_stream,
                                              write_stream->in_data_stream.next_in,
                                              internal->new_ins_depl_pos + internal->new_ins_depl_size,
                                              write_stream->in_data_stream.avail_in,
                                              1);       // 1 = down

      /*if (inadler) */
      internal->uLastAdler32 = adler32(internal->uLastAdler32,
                                       (dfbytep)write_stream->in_data_stream.next_in, dfHow);
      internal->new_ins_depl_size += dfHow;
      write_stream->in_data_stream.avail_in -= dfHow;
      write_stream->in_data_stream.next_in = ((dfbytep) write_stream->in_data_stream.next_in) + dfHow;
      write_stream->in_data_stream.total_in += dfHow;
      internal->total_in_internal += dfHow;
      write_stream->in_data_stream.done_latest_in += dfHow;

      // if we must stop the ins depl
      if (write_stream->in_data_stream.avail_in > 0)
      {
        dfuLong32 cur_search_parameters = 0;

        // to examine alignement, compare internal->curPos_newwritten and internal->new_ins_depl_pos
        CheckComputeStatistiqAlignement(internal,internal->new_ins_depl_pos,internal->new_ins_depl_size,FALSE);
        err = WriteInsertDepl(write_stream,
                              internal->new_ins_depl_pos,
                              internal->new_ins_depl_size);

        internal->new_ins_depl_pos = INS_DEPL_POS_NONE;
        internal->new_ins_depl_size = 0;
        internal->cur_min_before_search = internal->SearchParameters[internal->cur_search_parameters].min_before_search;
        //fix_align_cur_min_before_search(internal);
        /* cur_min_before_search alignAdapt ++ */

        internal->size_next_search =
          internal->cur_min_before_search +
          internal->SearchParameters[cur_search_parameters].
          min_size_search_depl +0; // +0 parasit
        internal->size_before_next_search = internal->size_next_search;
        //internal -> cur_search_parameters = cur_search_parameters;
      }
    }
    else                        // we are not in insert depl
    {
      //int found = 0;
      dfuLong32 add = 0;
      //BOOL fDone = FALSE;
      BOOL fTryFound = TRUE;

      // do
      {
        dfuLong32 i;
        dfuLong64 posOrg;
        dfuLong32 sizeOrgRcp;

        if (internal->step_search == 0)
          add = dwStepSearch;   //internal -> size_before_next_search ;
        else
          add = internal->step_search;

        internal->step_search = add;


        if (add > internal->size_before_next_search)
        {
          add = internal->size_before_next_search;
          internal->step_search = 0;
        }

        if (add > write_stream->in_data_stream.avail_in)
        {
          add = write_stream->in_data_stream.avail_in;
          internal->step_search -= add;
          fTryFound = FALSE;
        }
        else
          internal->step_search = 0;

        DFASSERT(internal->pos_in_pre_read + add <
                 internal->pre_read_data_size);

#ifdef _DEBUG
        if (internal->pos_in_pre_read + add >= internal->pre_read_data_size)
        {
          printf("assert %u,%u,%u\n", internal->pos_in_pre_read, add,
                 internal->pre_read_data_size);
        }
#endif
        for (i = 0; i < add; i++)
          *((internal->pre_read_data) + (internal->pos_in_pre_read) + i) =
            *(((dfbytep) (write_stream->in_data_stream.next_in)) + i);

        /*if (inadler) */
        internal->uLastAdler32 = adler32(internal->uLastAdler32,
                                         (dfbytep)write_stream->in_data_stream.next_in, add);

        write_stream->in_data_stream.avail_in -= add;
        write_stream->in_data_stream.next_in = ((dfbytep) write_stream->in_data_stream.next_in) + add;
        write_stream->in_data_stream.total_in += add;
        internal->total_in_internal += add;
        write_stream->in_data_stream.done_latest_in += add;
        internal->pos_in_pre_read += add;
        internal->size_before_next_search -= add;
        //printf("#");


        //if (internal -> size_before_next_search==0)
        {
          BOOL fFound = FALSE;
          dfuLong32 dfSkipBeginSearch;
          dfuLong32 cur_search_parameters = internal->cur_search_parameters;
          dfuLong32 pos_search_in_pre_read = internal->pos_in_pre_read -
            (internal->size_next_search -
             internal->cur_min_before_search);

          sizeOrgRcp = 0;

#define ADVTEST (4)
#define MINEQ   (2)


          /* Trying search prev place ! */
          if ((!fFound) && fTryFound)
            //if ((!fFound) && (internal -> size_before_next_search==0))
          {
            dfuLong32 pos_found;
            sizeOrgRcp = CheckSameByteThanInDeplSoon(write_stream,
                                                     ((dfbytep) internal->pre_read_data),
                                                     internal->curPosIns,
                                                     dfmin(internal->pos_in_pre_read, 6),       //,internal -> min_before_search),
                                                     2, // min size
                                                     &pos_found);

            //if (internal -> size_before_next_search!=0)
            //    sizeOrgRcp=0;
//sizeOrgRcp=0;
            if (sizeOrgRcp > 0)
            {
              fFound = TRUE;
              dfSkipBeginSearch = 0;
              posOrg = internal->curPosIns + pos_found;
              pos_search_in_pre_read = pos_found;
            }
          }
          /* Trying search prev place done ! */

          /* Trying search inplace ! (must be optional) */
          if ((!fFound) && (internal->size_before_next_search == 0) && (internal->curPos_newwritten!=internal->curPosIns))
          {
            dfuLong32 pos_found;
            sizeOrgRcp = CheckSameByteThanInDeplSoon(write_stream,
                                                     ((dfbytep) internal->pre_read_data),
                                                     internal->curPos_newwritten,
                                                     dfmin(internal->pos_in_pre_read, 6),       //,internal -> min_before_search),
                                                     2, // min size
                                                     &pos_found);

            //if (internal -> size_before_next_search!=0)
            //    sizeOrgRcp=0;
//sizeOrgRcp=0;
            if (sizeOrgRcp > 0)
            {
              fFound = TRUE;
              dfSkipBeginSearch = 0;
              posOrg = internal->curPos_newwritten + pos_found;
              pos_search_in_pre_read = pos_found;
            }
          }
          /* Trying search inplace ! (must be optional)  done */


/*
#define ADVTEST2 (2)
#define MINEQ2   (2)

*/




          if ((!fFound) && (internal->size_before_next_search == 0))
          {
            dfuLong32 continue_search;

            do
            {
              dfuLong64 dwBeginSearch, dwMaxSearchOrg;
              dfuLong32 dfSuggested_size_before_next_search;
              if (internal->SearchParameters[cur_search_parameters].
                  searchBeforeWindow == 0)
                dwBeginSearch = 0;
              else
                dwBeginSearch =
                  (internal->SearchParameters[cur_search_parameters].
                   searchBeforeWindow > internal->curPosIns) ? 0 :
                  (internal->curPosIns -
                   internal->SearchParameters[cur_search_parameters].
                   searchBeforeWindow);

              if (internal->SearchParameters[cur_search_parameters].
                  searchAfterWindow == 0)
                dwMaxSearchOrg = 0;
              else
                dwMaxSearchOrg = internal->curPosIns + internal->
                  SearchParameters[cur_search_parameters].searchAfterWindow;
              /* begin init the search engine */
              if (internal->SearchInitDone == 0)
              {
                  dfuLong32 dfSizeClusterReduced = 0;
                  dfuLong64 dfArrayLength, dfNbArray ;
                  dfuLong32 dfNbHashBit = COMPRESSIONPARAM_AUTOVALUE;
                  dfuLong32 i;
                  dfuLong32 dfBlockCalcSizeSearch = internal->dfBlockCalcSizeSearch;
                  dfuLong32 dfPreselectedNbArray = COMPRESSIONPARAM_AUTOVALUE;


                  if (((dfBlockCalcSizeSearch >= 0x10) && (internal->sizeorg >= 128*1024*1024)) ||
                      (dfBlockCalcSizeSearch >= 0x20))
                    dfSizeClusterReduced = dfBlockCalcSizeSearch / 2;


                  if (internal->sizeorg < 1024*1024)
                      dfSizeClusterReduced=0;

/*
                  if ((dfBlockCalcSizeSearch >= 0x10) || (dfBlockCalcSizeSearch >= 0x40))
                      if (internal->sizeorg >= 128*1024*1024)*/
                  dfSizeClusterReduced=0;
                  if (((dfBlockCalcSizeSearch%2)==0) && (dfBlockCalcSizeSearch>=0x8*4))
                        dfSizeClusterReduced = dfBlockCalcSizeSearch / 2;
                  else if (dfBlockCalcSizeSearch>=0x10)
                      dfSizeClusterReduced = dfBlockCalcSizeSearch ;
                        //dfBlockCalcSizeSearch = dfSizeClusterReduced  * 2;

                  dfArrayLength = dfBlockCalcSizeSearch*(0x100000/(16));

                  if (internal->cprParam.dfNbHashBit != COMPRESSIONPARAM_AUTOVALUE)
                  {
                    dfNbHashBit = internal->cprParam.dfNbHashBit & 0xff;
                    if ((dfNbHashBit == 0) || (dfNbHashBit > 31)) dfNbHashBit = COMPRESSIONPARAM_AUTOVALUE;
                    if (((internal->cprParam.dfNbHashBit / 0x100) > 0) && ((internal->cprParam.dfNbHashBit / 0x100) <= 0x0f) && (internal->sizeorg>0))
                    {
                      dfPreselectedNbArray = internal->cprParam.dfNbHashBit / 0x100;

                      dfArrayLength = internal->sizeorg / dfPreselectedNbArray;
                    }
                  }

                  if (dfNbHashBit == COMPRESSIONPARAM_AUTOVALUE)
                  {
                    dfuLong32 dfDivide = dfBlockCalcSizeSearch;
                    dfNbHashBit = 16;
                    while (dfDivide >= (0x10 / 2))
                    {
                      dfNbHashBit++;
                      dfDivide /= 2;
                    }
                  }

                  if (dfPreselectedNbArray != COMPRESSIONPARAM_AUTOVALUE)
                    dfNbArray = dfPreselectedNbArray;
                  else
                  {
                    for (i = 16 + 2; i<dfNbHashBit; i++)
                      dfArrayLength *= 2;

                    dfNbArray = (dfuLong32)((internal->sizeorg + (dfArrayLength - 1)) / dfArrayLength);
                  }

                  if (internal->cprParam.dfPhysicalMemoryKB != COMPRESSIONPARAM_AUTOVALUE)
                  {
                      dfuLong64 dfSizeReduced = 0 ;
                      dfuLong64 dfSizeBHFLArray = (dfuLong64)((internal->sizeorg / (dfBlockCalcSizeSearch))*4);
                      dfuLong64 dfSizeArray = (((dfuLong64)1) << dfNbHashBit)*4;
                      dfuLong32 dfNbArrayMemory=2;

                      if (dfSizeClusterReduced>0)
                          dfSizeReduced = (dfuLong64)((internal->sizeorg / dfSizeClusterReduced)*2);


                      if (((dfSizeReduced+dfSizeBHFLArray + (dfSizeArray*dfNbArrayMemory*2))/1024) < internal->cprParam.dfPhysicalMemoryKB)
                      {
                          dfuLong32 j;

                          j=0;
                          while ((((dfSizeReduced+dfSizeBHFLArray + (dfSizeArray*dfNbArrayMemory*2))/1024) < internal->cprParam.dfPhysicalMemoryKB) && (j<2))
                          {
                              j++;
                              dfSizeArray*=2;
                              dfNbHashBit++;
                          }
                      }
                      else
                      {
                          dfuLong32 j;

                          j=0;
                          while ((((dfSizeReduced+dfSizeBHFLArray + (dfSizeArray*dfNbArrayMemory*2))/1024) > (internal->cprParam.dfPhysicalMemoryKB)) && (j<2))
                          {
                              j++;
                              dfSizeArray/=2;
                              dfNbHashBit--;
                          }
                      }
                  }

#if defined(_DEBUG) && defined(VERBOSE_PATCH_BUILD)
                  printf("dfBlockCalcSizeSearch=%u, dfNbHashBit=%u, dfSizeClusterReduced=%u, dfArrayLength=%u KB\n",
                      dfBlockCalcSizeSearch,dfNbHashBit,dfSizeClusterReduced,((dfuLong32)(dfArrayLength/1024)));
#endif
                  if (dfPreselectedNbArray == COMPRESSIONPARAM_AUTOVALUE)
                    dfNbArray = 1;

                  if (dfArrayLength>0)
                    dfNbArray = (dfuLong32)((internal->sizeorg + (dfArrayLength - 1)) / dfArrayLength);
                  if (dfNbArray == 0)
                      dfNbArray = 1;
                  dfArrayLength = ((internal->sizeorg + dfBlockCalcSizeSearch) / dfNbArray);
                  dfArrayLength -= (dfArrayLength % dfBlockCalcSizeSearch);

                  InitFindSeq(&internal->HFindSeqIntl,
                              write_stream->nbOrig,
                              write_stream->OrigDataPtr,
                              dfArrayLength,
                              dfBlockCalcSizeSearch,
                              dfNbHashBit,
                              dfSizeClusterReduced);

                  internal->SearchInitDone = 1;
              }
              /* end init the search engine */

              dfSkipBeginSearch = 0;
              dfSuggested_size_before_next_search = 0;
              fFound = TRUE;
              {
                dfuLong32 i;
                dfbyte cLatest;
                dfuLong32 iCountRepeat=1;
                dfbytepc lpSeqRech=internal->pre_read_data + pos_search_in_pre_read;
                dfuLong32 dwMaxRech=internal->pos_in_pre_read - pos_search_in_pre_read;
                dfuLong32 dwMinInter=internal->SearchParameters[cur_search_parameters].min_size_depl;

                if (pos_search_in_pre_read>0xff000000)
                    pos_search_in_pre_read-=0;
                for (i=0;i<dwMaxRech;i++)
                {
                    dfbyte c;
                    c = *(lpSeqRech+i);
                    if (i==0)
                        cLatest = c;
                    else
                        if (cLatest != c)
                        {
                            cLatest = c;
                            iCountRepeat=1;
                        }
                        else
                        {
                            iCountRepeat++;
                            if (iCountRepeat>=dwMinInter)
                            {
                                //internal->size_before_next_search = i;
                                while ((i+1)<dwMaxRech)
                                    if ((*(lpSeqRech+i+1))==cLatest)
                                        i++;
                                    else
                                        break;
                                dfSuggested_size_before_next_search = i;
                                fFound=FALSE;
                                break;
                            }
                        }
                }
              }

              if (dfSuggested_size_before_next_search>0)
              {
                 //internal->size_before_next_search+=dfSuggested_size_before_next_search;
                  internal->cur_min_before_search+=dfSuggested_size_before_next_search;
                  /* cur_min_before_search alignAdapt ++ */
                  //fix_align_cur_min_before_search(internal);
                  PreReadDataFlushIfNeeded(write_stream,internal);
                  break;
              }



              if (fFound)
              {
                dfuLong32 dfSkipAlignSrch = 0;
                dfuLong32 dfAlignThis;
                dfuLong32 dfModulo=0;
                internal->dfCountAlignedSearch++;

                dfAlignThis = internal->dfMinimalAlign;

                if ((internal->SearchParameters[cur_search_parameters].align_possible)!=0)
                {
                    internal->dfCountAlignedSearch++;
                    if (internal->dfCountAlignedSearch == internal->dfMaxAlignedSearchNumber)
                    {
                        internal->dfCountAlignedSearch = 0;
                    }
                    else
                        dfAlignThis = internal->dfAlignValue;


                    /* reset */
                    if (/*(internal->SearchParameters[cur_search_parameters].searchBeforeWindow == 0) &&*/
                        internal->SearchParameters[cur_search_parameters].searchAfterWindow==0)
                    {
                        internal->dfCountAlignedSearch = 0;
                        dfAlignThis = internal->dfMinimalAlign;
                    }
                }

                if (dfAlignThis < internal->dfMinimalAlign)
                  dfAlignThis = internal->dfMinimalAlign;

//dfAlignThis =0x10;
               if (dfAlignThis != 1)
                  dfModulo = (dfuLong32)((((pos_search_in_pre_read + internal->curPos_newwritten))) % dfAlignThis);

               if (dfModulo != 0)
                    dfSkipAlignSrch = (dfuLong32)(dfAlignThis - dfModulo);

               fFound =
                (FindSeqOrigPtr
                 (internal->HFindSeqIntl,
                  internal->pre_read_data + (pos_search_in_pre_read + dfSkipAlignSrch),
                  internal->pos_in_pre_read - (pos_search_in_pre_read + dfSkipAlignSrch),
                  write_stream, dwBeginSearch, dwMaxSearchOrg,
                  internal->curPosIns,

                  internal->SearchParameters[cur_search_parameters].min_size_depl,
                  internal->SearchParameters[cur_search_parameters].min_size_depl,

                  dfAlignThis, &dfSkipBeginSearch,
                  &posOrg, &sizeOrgRcp));

               if (fFound && dfSkipAlignSrch>0)
               {
                   if (posOrg<dfAlignThis)
                       fFound=FALSE;
                   else
                   {
                        dfuLong32 dfRemoveSkip = 0;
                        posOrg-=dfSkipAlignSrch;
                        if (dfSkipBeginSearch==0)
                            dfRemoveSkip= CheckSameByteThanInDepl(write_stream,
                                                    ((dfbytep) internal-> pre_read_data) +pos_search_in_pre_read + dfSkipAlignSrch- 1,
                                                    posOrg + dfSkipAlignSrch - 1,
                                                    dfSkipAlignSrch, 0);

                        dfSkipBeginSearch += (dfSkipAlignSrch-dfRemoveSkip);
                     }
                   }
              }


if ((fFound) && (sizeOrgRcp>4))
{
//dfSkipBeginSearch=1*0;
}


              if (fFound)
              {
                if (posOrg > internal->curPosIns)
                  internal->
                    dfStatiqDist[0][dfmin
                                    (MyLogLong64
                                     (posOrg - internal->curPosIns), 31)]++;
                else
                  internal->
                    dfStatiqDist[1][dfmin
                                    (MyLogLong64
                                     (internal->curPosIns - posOrg), 31)]++;
              }
              continue_search = 0;
              if ((!fFound)
                  && (cur_search_parameters + 1 <
                      internal->nb_search_parameters))
                if ((internal->
                     SearchParameters[cur_search_parameters +
                                     1].continue_inplace != 0) && (dfSuggested_size_before_next_search == 0))
                {
                  continue_search = 1;
                  cur_search_parameters++;
                  internal->cur_search_parameters = cur_search_parameters;
                }
            }
            while (continue_search == 1);
          }
          if (fFound)
          {
            //dfuLong32 dfAddBefore;
            dfsLong32 dfAddBefore;

            internal->cur_search_parameters = cur_search_parameters = 0;
            // we search before
            if (dfSkipBeginSearch>0)
                dfAddBefore = -((dfsLong32)dfSkipBeginSearch);
            else
            dfAddBefore = CheckSameByteThanInDepl(write_stream,
                                                  ((dfbytep) internal->
                                                   pre_read_data) +
                                                  pos_search_in_pre_read
                                                  - 1, posOrg - 1,
                                                  pos_search_in_pre_read, 0);

            WriteInsertBytes(write_stream, internal->pre_read_data,
                             pos_search_in_pre_read - dfAddBefore);
            //posOrg-=dfAddBefore;
            //sizeOrgRcp+=dfAddBefore;


            // if we can hope continue after
            if (sizeOrgRcp ==
                internal->pos_in_pre_read - pos_search_in_pre_read)
            {
              internal->pos_in_pre_read = 0;
              internal->new_ins_depl_pos = posOrg - dfAddBefore;
              internal->new_ins_depl_size = sizeOrgRcp + dfAddBefore;
            }
            else
            {
              dfuLong32 i;
#ifdef _DEBUG
              if (sizeOrgRcp + pos_search_in_pre_read >
                  internal->pos_in_pre_read)
              {
                printf("error2\t");
              }
#endif
              // to examine alignement, compare internal->curPos_newwritten and posOrg - dfAddBefore
              internal->pos_in_pre_read -=
                sizeOrgRcp + pos_search_in_pre_read;
              CheckComputeStatistiqAlignement(internal,posOrg - dfAddBefore,sizeOrgRcp + dfAddBefore,FALSE);
              err =
                WriteInsertDepl(write_stream, posOrg - dfAddBefore,
                                sizeOrgRcp + dfAddBefore);
//if ((sizeOrgRcp + dfAddBefore)==6) printf("6b sizeOrgRcp,pos_in_pre_read,pos_search_in_pre_read=%u %u %u\n",sizeOrgRcp,internal->pos_in_pre_read,pos_search_in_pre_read); // @@@
              for (i = 0; i < internal->pos_in_pre_read; i++)
                *((internal->pre_read_data) + i) =
                  *((internal->pre_read_data) + i +
                    (sizeOrgRcp + pos_search_in_pre_read));
              // prepare next search
              internal->new_ins_depl_pos = INS_DEPL_POS_NONE;
              internal->new_ins_depl_size = 0;
              internal->cur_min_before_search = internal->SearchParameters[internal->cur_search_parameters].min_before_search;
              //fix_align_cur_min_before_search(internal);
              /* cur_min_before_search alignAdapt ++ */

              internal->size_next_search =
                internal->cur_min_before_search +
                internal->SearchParameters[cur_search_parameters].
                min_size_search_depl;
              internal->size_before_next_search = internal->size_next_search;

              PreReadDataFlushIfNeeded(write_stream,internal);
              /*
              if (internal->size_before_next_search +
                  internal->pos_in_pre_read >= internal->pre_read_data_size)
              {
                dfuLong32 i, j;
                dfuLong32 dfKeep =
                  dfmin(internal->pre_read_data_size_after_flush,
                        internal->pos_in_pre_read);
                WriteInsertBytes(write_stream,
                                 internal->pre_read_data,
                                 internal->pos_in_pre_read - dfKeep);
                for (i = 0, j = internal->pos_in_pre_read - dfKeep;
                     i < dfKeep; i++, j++)
                  internal->pre_read_data_tab[i] =
                    internal->pre_read_data_tab[j];
                internal->pos_in_pre_read = dfKeep;
              }*/
            }


            //sizeOrgRcp
            //fDone=TRUE;
          }
          else
          {
            if (internal->size_before_next_search == 0)
            {
              internal->cur_search_parameters++;
              if (internal->cur_search_parameters >=
                  internal->nb_search_parameters)
                internal->cur_search_parameters = 0;
              cur_search_parameters = internal->cur_search_parameters;
              /*
                 internal -> cur_search_parameters = cur_search_parameters =
                 dfmin(internal -> cur_search_parameters+1,
                 internal -> nb_search_parameters-1);
               */
            }
            if (internal->size_before_next_search == 0)
            {                   // we have not found!
              internal->new_ins_depl_pos = INS_DEPL_POS_NONE;
              internal->new_ins_depl_size = 0;
              internal->cur_min_before_search = internal->SearchParameters[internal->cur_search_parameters].min_before_search;

              //fix_align_cur_min_before_search(internal);
              /* cur_min_before_search alignAdapt ++ */

              internal->size_next_search =
                internal->cur_min_before_search +
                internal->SearchParameters[cur_search_parameters].
                min_size_search_depl;
              internal->size_before_next_search = internal->size_next_search;
              if (internal->size_before_next_search +
                  internal->pos_in_pre_read >= internal->pre_read_data_size)
              {
                dfuLong32 i, j;
                dfuLong32 dfKeep =
                  dfmin(internal->pre_read_data_size_after_flush,
                        internal->pos_in_pre_read);
                WriteInsertBytes(write_stream,
                                 internal->pre_read_data,
                                 internal->pos_in_pre_read - dfKeep);
                for (i = 0, j = internal->pos_in_pre_read - dfKeep;
                     i < dfKeep; i++, j++)
                  internal->pre_read_data_tab[i] =
                    internal->pre_read_data_tab[j];
                internal->pos_in_pre_read = dfKeep;
              }
            }
          }
        }
      }
    }                           // of elseif
  }                             // of while

  FlushOut(write_stream);


  return 0;
}

int SVFAPI FlushMakeDif(WRITEDIF_STREAM * write_stream)
{
  int err = DSERR_OK;
  INTERNAL_WRITESTATE *internal;

  internal = (INTERNAL_WRITESTATE *) (write_stream->state);

  write_stream->in_data_stream.done_latest_in = 0;
  write_stream->out_data_stream.done_latest_out = 0;


  if (IsFinalFlush(&(internal->outPatchStream)) == 0)
  {
    BOOL fJustDiscoverIdentical = FALSE;
    err = DoMakeDifWork(write_stream);
    if (err != DSERR_OK)
        return err;

    if (write_stream->in_data_stream.avail_in > 0)
      return DSERR_OK;

    if ((internal->new_ins_depl_pos == INS_DEPL_POS_NONE) &&
        (internal->fFileIdentical))
    {
        fJustDiscoverIdentical = CheckSameByteThanInDepl(write_stream,internal->pre_read_data,0,internal->pos_in_pre_read,1) ==
             internal->pos_in_pre_read;
    }

    if (fJustDiscoverIdentical)
    {
            err = WriteInsertDepl(write_stream,
                                  0,
                                  internal->pos_in_pre_read);
    }
    else
    {
        if (internal->new_ins_depl_pos != INS_DEPL_POS_NONE)
            err = WriteInsertDepl(write_stream,
                                internal->new_ins_depl_pos,
                                internal->new_ins_depl_size);

        WriteInsertBytes(write_stream, internal->pre_read_data,
                        internal->pos_in_pre_read);
    }

    SetInFinalFlush(&(internal->outPatchStream));

    if (internal->SearchInitDone == 1)
    {
      CloseFinqSeqInternal(internal->HFindSeqIntl);
      internal->SearchInitDone = 0;
    }
    internal->HFindSeqIntl = NULL;
  }




  FlushOut(write_stream);
  if (err == DSERR_OK)
  {
      err=FinalFlushOutData(&internal->outPatchStream, &write_stream->out_data_stream);
  }

  return err;
}


#ifdef DO_STATIS
void EnableStatis(WRITEDIF_STREAM * write_stream, dfuLong32 max_value)
{
  dfuLong32 i;
  INTERNAL_WRITESTATE *internal;

  internal = (INTERNAL_WRITESTATE *) (write_stream->state);
  for (i = 0; i < NB_STATIS; i++)
    internal->statis[i] = InitStatis(max_value);
  for (i = 0; i < NB_AHUFF; i++)
    internal->tsthufout[i] = InitHufOut();
}

int
FlushStatis(WRITEDIF_STREAM * write_stream, int numStream, const char *fn,
            const char *title)
{
  int ret = 0;
  INTERNAL_WRITESTATE *internal;

  internal = (INTERNAL_WRITESTATE *) (write_stream->state);
  if (numStream < NB_STATIS)
    if (internal->statis[numStream] != NULL)
      ret = WriteStatis(internal->statis[numStream], fn, title);
  return ret;
}

void FlushHuff(WRITEDIF_STREAM * write_stream)
{
  INTERNAL_WRITESTATE *internal;
  dfuLong32 i, sumhuf;
  sumhuf = 0;

  internal = (INTERNAL_WRITESTATE *) (write_stream->state);
  for (i = 0; i < NB_AHUFF; i++)
    if (internal->tsthufout[i] != NULL)
    {
      unsigned long hufsize = CloseHufOut(internal->tsthufout[i]);
      internal->tsthufout[i] = NULL;
      //printf("huffman number %d size = %lu\n",i,hufsize);
      if (i != 0)
        sumhuf += hufsize;
    }
  //printf("hufman>0 sum = %lu\n",sumhuf);
}
#endif

/*
endian check
adapt FindSeq for multi OrigData
todo : write CheckSameByteThanInDepl for check same data (before or after)
crc ,, InsertBytesFromOrig
*/

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
#include <string.h>

#include <memory.h>

#include "zlib.h"

#include "../common/difbasic.h"

#include "../common/difstrm.h"
#include "../common/difstrmi.h"
#include "../common/DfsTlTyp.h"
#include "../common/difstool.h"
#include "../common/DfsOrigMemoryMap.h"



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



#define MDSAroundUpper(dwValue,dwModulo) (((((dwValue)) + ((dwModulo)) -1) / ((dwModulo))) * (dwModulo))

#define MAX_SEARCH_PARAMETERS (0x40)
#define SIZE_PRE_READ_DATA_AFTER_FLUSH (16)

#define INS_DEPL_POS_NONE  ((dfuLong64)-1)


/**/

int GetNbByteNumberMDI(dfuLong32 size)
{
  return (size < 0x100) ? 1 : (size < 0x10000) ? 2 :
    (size < 0x1000000) ? 3 : 4;
}


int GetNbByteNumber64MDI(dfuLong64 value64)
{
    dfuLong32 valueLow = (dfuLong32)value64;
    dfuLong32 valueHigh = (dfuLong32)(value64>>32);
    if (valueHigh != 0)
        return GetNbByteNumberMDI(valueHigh)+4;
    else
        return GetNbByteNumberMDI(valueLow);
}

#define INTERNALRECOPYBUF (MAX_IDENTICAL_SIZE)

dfuLong32 SVFAPI MakeDifStFileIdenticalCompressRatio(dfvoidp ptr, dfuLong32 sizeBuffer,
                                                     dfuLong64 size64, int iZlibCompressRatio)
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

  if (size64 == 0)
    {
      sizeStream0uncompressed = 0;
    }
  else if ((size64 < 0x1b))
    {
      tabStream0uncompressed[0] = 0x40 + 0x20 + (dfbyte) (size - 1);
      sizeStream0uncompressed = 1;
    }
  else if ((size64 < 0x100))
    {
      tabStream0uncompressed[0] = 0x40;
      tabStream0uncompressed[1] = (dfbyte) (size - 1);
      sizeStream0uncompressed = 2;
    }
  else if ((size64 >> 32) == 0)
    {
      dfuLong32Intel sizeIntel;
      dfuLong32Intel posIntel = ConvertuLongToLongIntel(0);
      dfbyte bNbByteNb;
      bNbByteNb = (dfbyte)GetNbByteNumberMDI(size);

      tabStream0uncompressed[0] =
        ((dfbyte) (bNbByteNb + 0x7b)) | ((dfbyte) 0x80);

      sizeIntel = ConvertuLongToLongIntel(size);
      sizeStream0uncompressed = bNbByteNb + 1 + 4;
      DfsMemcpy(&tabStream0uncompressed[1], &sizeIntel, bNbByteNb);
      DfsMemcpy(&tabStream0uncompressed[1 + bNbByteNb], &posIntel, 4);
    }
  else
    {
      dfbyte dfNbBytesSize,dfNbBytesPos;
      dfuLong64 pos=0;
      dfuLong64Intel dfSizeIntel64 = ConvertuLongToLongIntel64(size64) ;
      dfuLong64Intel dfPosIntel64 = ConvertuLongToLongIntel64(pos) ;


      tabStream0uncompressed[0] = 0x7b;
      dfNbBytesSize=(dfbyte)GetNbByteNumber64MDI(size64);
      dfNbBytesPos=(dfbyte)GetNbByteNumber64MDI(pos);
      /*
      if (dfNbBytesSize==0)
          dfNbBytesSize++;*/
      tabStream0uncompressed[1] = (dfbyte)(((dfbyte) 0x80) | (((dfNbBytesSize-1) & 0x07) << 4) | (dfNbBytesPos & 0x0f));
      DfsMemcpy(&tabStream0uncompressed[2],&dfSizeIntel64,dfNbBytesSize);
      DfsMemcpy(&tabStream0uncompressed[2+dfNbBytesSize],&dfPosIntel64,dfNbBytesPos);

      sizeStream0uncompressed = 2 + dfNbBytesSize + dfNbBytesPos;
    }


  ClearZStream(&zstr);

  if (iZlibCompressRatio == 0)
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
                        (int) iZlibCompressRatio,//Z_NO_COMPRESSION,//Z_BEST_SPEED, // /*Z_BEST_SPEED,/ / */Z_BEST_COMPRESSION,
                        Z_DEFLATED, -MAX_WBITS, DEF_MEM_LEVEL, 0);
    zstr.avail_in = sizeStream0uncompressed;
    zstr.next_in = tabStream0uncompressed;


    zstr.avail_out = INTERNALRECOPYBUF;
    zstr.next_out = tabStream0compressed;
    deflate(&zstr, Z_FINISH);
    sizeStream0compressed = (dfuLong32)zstr.total_out;
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

    /*
  {
    dfuLong32 i;
    printf("size = %6u : ", size);
    for (i = 0; i < dfResultSize; i++)
      printf("%02x  ", tabResult[i]);
    printf("\n");
  }
  */

    if (sizeBuffer<dfResultSize)
        return 0;
    DfsMemcpy(ptr,tabResult,dfResultSize);
  return dfResultSize;
}


dfuLong32 SVFAPI MakeDifStFileIdentical(dfvoidp ptr, dfuLong32 sizeBuffer, dfuLong64 size64)
{
    return MakeDifStFileIdenticalCompressRatio(ptr, sizeBuffer, size64, 0);
}
/****************************************************************/

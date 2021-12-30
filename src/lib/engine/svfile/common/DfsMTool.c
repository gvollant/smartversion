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

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include "zlib.h"
#include "../../patchstream/common/difbasic.h"
#include "../../patchstream/common/DfsTlTyp.h"
#include "../../patchstream/common/difstool.h"
#include "DfsMFile.h"
#include "DfsStruc.h"

/*
typedef struct
{
  dfbyte dfsInfoDateByte[8];
} DFSINFODATE;


typedef struct
{
  dfuLong32 df_msec;           // milisecond [0..999] 10 bits /
  dfuLong32 df_sec;            // seconds after the minute - [0..59]  6 bits/
  dfuLong32 df_min;            // minutes after the hour - [0..59] 6 bits/
  dfuLong32 df_hour;           // hours since midnight - [0..23]  5 bits/
  dfuLong32 df_mday;           // day of the month - [1..31] 5 bits/
  dfuLong32 df_mon;            // months since January - [1..12] 4 bits/
  dfuLong32 df_year;           // years - [0..4095] 12 bits/
  dfuLong32 df_timezone_bias;  // bias [0..255] 8 bits. this is ((bias in minute/15)+128)
} DFSTM;

*/



void ConvertDfsInfoDateToDfsTm(const DFSINFODATE * dfsInfoDate, DFSTM * dfsTm)
{
  dfsTm->df_msec =
    dfsInfoDate->
    dfsInfoDateByte[0] | ((dfsInfoDate->dfsInfoDateByte[1] & 0x03) << 8);
  dfsTm->df_sec = (dfsInfoDate->dfsInfoDateByte[1] & 0xfc) >> 2;
  dfsTm->df_min = (dfsInfoDate->dfsInfoDateByte[2] & 0x3f);
  dfsTm->df_hour = (dfsInfoDate->dfsInfoDateByte[3] & 0x1f);

  dfsTm->df_mday = (dfsInfoDate->dfsInfoDateByte[4] & 0x1f);
  dfsTm->df_mon =
    (((dfsInfoDate->
       dfsInfoDateByte[4] & 0xe0) >> 5) | (((dfsInfoDate->
                                             dfsInfoDateByte[5] & 0x01) <<
                                            3)));
  dfsTm->df_year =
    (((dfsInfoDate->dfsInfoDateByte[5] & 0xfe) >> 1) | ((dfsInfoDate->
                                                         dfsInfoDateByte[6] &
                                                         0x1f) << 7));
  dfsTm->df_timezone_bias =
    (((dfsInfoDate->dfsInfoDateByte[6] & 0xe0) >> 5) | ((dfsInfoDate->
                                                         dfsInfoDateByte[7] &
                                                         0xff) << 3));
  dfsTm->df_timezone_bias = dfsTm->df_timezone_bias & 0xff;
}

void ConvertDfsTmToDfsInfoDate(const DFSTM * dfsTm, DFSINFODATE * dfsInfoDate)
{
  dfsInfoDate->dfsInfoDateByte[0] = (dfbyte) (dfsTm->df_msec & 0xff);
  dfsInfoDate->dfsInfoDateByte[1] =
    (dfbyte) (((dfsTm->df_msec & 0x300) >> 8) | (dfsTm->df_sec << 2));
  dfsInfoDate->dfsInfoDateByte[2] = (dfbyte) (dfsTm->df_min & 0x3f);
  dfsInfoDate->dfsInfoDateByte[3] = (dfbyte) (dfsTm->df_hour & 0x1f);

  dfsInfoDate->dfsInfoDateByte[4] =
    (dfbyte) (((dfsTm->df_mday & 0x1f)) | ((dfsTm->df_mon & 0x07) << 5));
  dfsInfoDate->dfsInfoDateByte[5] =
    (dfbyte) ((((dfsTm->df_mon & 0x8) >> 3)) | ((dfsTm->
                                                 df_year & 0x7f) << 1));
  dfsInfoDate->dfsInfoDateByte[6] =
    (dfbyte) (((dfsTm->df_year & 0xf80) >> 7) | ((dfsTm->df_timezone_bias & 0x07) << 5));
  dfsInfoDate->dfsInfoDateByte[7] = (dfbyte) ((dfsTm->df_timezone_bias & 0x7f8) >>3);
}

dfuLong32 GetCrcInfoSizeNeededToGetSize(const DFSCRCINFO_FULLSIZESTRUCTURE * pDfsCrcInfo)
{
    dfuLong32 dfType = ConvertuLongIntelToLong(((DFSCRCINFOWITHMD5_64*)pDfsCrcInfo)->dfTypeCrcAndMd5Info);
    if ((dfType & TYPECRCINFO_STRUCTSIZEPRESENT)!=0)
    {
        DFSCRCINFOWITHMD5SHA1SHA256_64* pDfsCrcInfoStructSizePresend = (DFSCRCINFOWITHMD5SHA1SHA256_64*)pDfsCrcInfo;
        return (dfuLong32)(((dfbytep)&(pDfsCrcInfoStructSizePresend->dfStructSize)) - ((dfbytep)(pDfsCrcInfoStructSizePresend)))
            + sizeof(dfuLong32);
    }
    else
        return sizeof(dfuLong32);
}

dfuLong32 GetCrcInfoSize(const DFSCRCINFO_FULLSIZESTRUCTURE * pDfsCrcInfo)
{
    dfuLong32 dfType = ConvertuLongIntelToLong(((DFSCRCINFOWITHMD5_64*)pDfsCrcInfo)->dfTypeCrcAndMd5Info);
    if ((dfType & TYPECRCINFO_STRUCTSIZEPRESENT)!=0)
    {
        dfuLong32 dfStructSize = ConvertuLongIntelToLong(((DFSCRCINFOWITHMD5SHA1_64*)pDfsCrcInfo)->dfStructSize);
        return dfStructSize;
    }
    else
    if ((dfType & TYPECRCINFO_MD5)==0)
    {
        if ((dfType & TYPECRCINFO_64BITSPOS)==0)
          return sizeof(DFSCRCINFOONLYCRC32_32);
        else
          return sizeof(DFSCRCINFOONLYCRC32_64);
    }
    else
        if ((dfType & TYPECRCINFO_64BITSPOS)==0)
          return sizeof(DFSCRCINFOWITHMD5_32);
        else
          return sizeof(DFSCRCINFOWITHMD5_64);
}

dfuLong32 GetNbCrcInfo(const DFSCRCINFO_FULLSIZESTRUCTURE *pDfsCrcInfo,dfuLong32 dfSizeBuf)
{
    return GetCrcInfoSize(pDfsCrcInfo)/dfSizeBuf;
}


void ConvertDfsCrcInfoParamToDfsCrcInfo(const DFSCRCINFOPARAM *
                                        pDfsCrcInfoParam,
                                        DFSCRCINFO_FULLSIZESTRUCTURE * pDfsCrcInfoP)
{
  DFSCRCINFOWITHMD5_32* pDfsCrcInfo=(DFSCRCINFOWITHMD5_32*)pDfsCrcInfoP;
  pDfsCrcInfo->dfTypeCrcAndMd5Info =
    ConvertuLongToLongIntel((pDfsCrcInfoParam)->dfTypeCrcAndMd5Info);
  pDfsCrcInfo->dfBeginPos =
    ConvertuLongToLongIntel((dfuLong32)((pDfsCrcInfoParam)->dfBeginPos));
  pDfsCrcInfo->dfEndPos =
    ConvertuLongToLongIntel((dfuLong32)((pDfsCrcInfoParam)->dfEndPos));
  pDfsCrcInfo->dfCrcValue =
    ConvertuLongToLongIntel((pDfsCrcInfoParam)->dfCrc32Value);

  if (((((pDfsCrcInfoParam->dfTypeCrcAndMd5Info) & TYPECRCINFO_SHA1) != 0)) || ((((pDfsCrcInfoParam->dfTypeCrcAndMd5Info) & TYPECRCINFO_SHA256) != 0)))
  {
      DFSCRCINFOWITHMD5SHA1_64* pDfsCrcInfo64Sha1 = (DFSCRCINFOWITHMD5SHA1_64*)pDfsCrcInfoP;
      dfuLong32 dfNewTypeCrcAndMd5Info = ((pDfsCrcInfoParam)->dfTypeCrcAndMd5Info) | TYPECRCINFO_64BITSPOS | TYPECRCINFO_STRUCTSIZEPRESENT;
      BOOL fFillMD5, fFillSHA1, fFillSHA256;
      dfuLong32 dfStructSize;

      if (pDfsCrcInfoParam->fMd5)
          dfNewTypeCrcAndMd5Info |= TYPECRCINFO_MD5;

      if (pDfsCrcInfoParam->fSha1)
          dfNewTypeCrcAndMd5Info |= TYPECRCINFO_SHA1;

      if (pDfsCrcInfoParam->fSha256)
          dfNewTypeCrcAndMd5Info |= TYPECRCINFO_SHA256;

      pDfsCrcInfo64Sha1->dfTypeCrcAndMd5Info =
          ConvertuLongToLongIntel(dfNewTypeCrcAndMd5Info);

      pDfsCrcInfo64Sha1->dfBeginPosHigh = ConvertuLongToLongIntel((dfuLong32)(((pDfsCrcInfoParam)->dfBeginPos) >> 32));
      pDfsCrcInfo64Sha1->dfEndPosHigh = ConvertuLongToLongIntel((dfuLong32)(((pDfsCrcInfoParam)->dfEndPos) >> 32));
      dfStructSize = (dfuLong32)sizeof(DFSCRCINFOWITHMD5SHA1_64);

      fFillSHA256 = ((((pDfsCrcInfoParam->dfTypeCrcAndMd5Info) & TYPECRCINFO_SHA256) != 0) &&
          (((pDfsCrcInfoParam->dfTypeCrcAndMd5Info) & TYPECRCINFO_SHA256EMPTY) == 0));

      fFillSHA1 = ((((pDfsCrcInfoParam->dfTypeCrcAndMd5Info) & TYPECRCINFO_SHA1) != 0) &&
          (((pDfsCrcInfoParam->dfTypeCrcAndMd5Info) & TYPECRCINFO_SHA1EMPTY) == 0));

      fFillMD5 = ((((pDfsCrcInfoParam->dfTypeCrcAndMd5Info) & TYPECRCINFO_MD5) != 0) &&
          (((pDfsCrcInfoParam->dfTypeCrcAndMd5Info) & TYPECRCINFO_MD5EMPTY) == 0));

      if (!fFillSHA256)
      {

          if ((!fFillMD5) && (!fFillSHA1))
          {
              DfsClearStruct(pDfsCrcInfo64Sha1->bBinDataPart1, 0, 16);
              dfStructSize = (dfuLong32)sizeof(DFSCRCINFOWITHMD5SHA1_64) - 20;
          }
          else
          if ((fFillMD5) && (!fFillSHA1))
          {
              DfsMemcpy(pDfsCrcInfo64Sha1->bBinDataPart1, pDfsCrcInfoParam->bMd5, 16);
              dfStructSize = (dfuLong32)sizeof(DFSCRCINFOWITHMD5SHA1_64) - 20;
          }
          else
          if ((!fFillMD5) && (fFillSHA1))
          {
              DfsMemcpy(pDfsCrcInfo64Sha1->bBinDataPart1, pDfsCrcInfoParam->bSha1, 16);
              DfsMemcpy(pDfsCrcInfo64Sha1->bBinDataPart2, ((dfbytepc)pDfsCrcInfoParam->bSha1) + 16, 4);
              dfStructSize = (dfuLong32)sizeof(DFSCRCINFOWITHMD5SHA1_64) - 16;
          }
          else
          if ((fFillMD5) && (fFillSHA1))
          {
              DfsMemcpy(pDfsCrcInfo64Sha1->bBinDataPart1, pDfsCrcInfoParam->bMd5, 16);
              DfsMemcpy(pDfsCrcInfo64Sha1->bBinDataPart2, pDfsCrcInfoParam->bSha1, 20);
              dfStructSize = (dfuLong32)sizeof(DFSCRCINFOWITHMD5SHA1_64);
          }
      }
      else
      {
          // we have sha256
          if ((!fFillMD5) && (!fFillSHA1))
          {
              DfsMemcpy(pDfsCrcInfo64Sha1->bBinDataPart1, pDfsCrcInfoParam->bSha256, 16);
              DfsMemcpy(pDfsCrcInfo64Sha1->bBinDataPart2, ((dfbytepc)pDfsCrcInfoParam->bSha256) + 16, 16);
              dfStructSize = (dfuLong32)sizeof(DFSCRCINFOWITHMD5SHA1SHA256_64) - (20+16);
          }
          else
          if ((fFillMD5) && (!fFillSHA1))
          {
              DfsMemcpy(pDfsCrcInfo64Sha1->bBinDataPart1, pDfsCrcInfoParam->bMd5, 16);
              DfsMemcpy(pDfsCrcInfo64Sha1->bBinDataPart2, ((dfbytepc)pDfsCrcInfoParam->bSha256), 32);
              dfStructSize = (dfuLong32)sizeof(DFSCRCINFOWITHMD5SHA1SHA256_64) - 20;
          }
          else
          if ((!fFillMD5) && (fFillSHA1))
          {
              DfsMemcpy(pDfsCrcInfo64Sha1->bBinDataPart1, pDfsCrcInfoParam->bSha1, 16);
              DfsMemcpy(pDfsCrcInfo64Sha1->bBinDataPart2, ((dfbytepc)pDfsCrcInfoParam->bSha1) + 16, 4);
              DfsMemcpy(pDfsCrcInfo64Sha1->bBinDataPart2 + 4, ((dfbytepc)pDfsCrcInfoParam->bSha256), 32);
              dfStructSize = (dfuLong32)sizeof(DFSCRCINFOWITHMD5SHA1SHA256_64) - 16;
          }
          else
          if ((fFillMD5) && (fFillSHA1))
          {
              DfsMemcpy(pDfsCrcInfo64Sha1->bBinDataPart1, pDfsCrcInfoParam->bMd5, 16);
              DfsMemcpy(pDfsCrcInfo64Sha1->bBinDataPart2, pDfsCrcInfoParam->bSha1, 20);
              DfsMemcpy(pDfsCrcInfo64Sha1->bBinDataPart2+20, pDfsCrcInfoParam->bSha256, 32);
              dfStructSize = (dfuLong32)sizeof(DFSCRCINFOWITHMD5SHA1SHA256_64);
          }
      }
      pDfsCrcInfo64Sha1->dfStructSize = ConvertuLongToLongIntel(dfStructSize);
  }
  else
  if ((((pDfsCrcInfoParam->dfBeginPos) >> 32) != 0) || (((pDfsCrcInfoParam->dfEndPos) >> 32) != 0))
  {
      pDfsCrcInfo->dfTypeCrcAndMd5Info =
        ConvertuLongToLongIntel(((pDfsCrcInfoParam)->dfTypeCrcAndMd5Info) | TYPECRCINFO_64BITSPOS);

      if ((((pDfsCrcInfoParam->dfTypeCrcAndMd5Info) & TYPECRCINFO_MD5)!=0))
      {
          DFSCRCINFOWITHMD5_64* pDfsCrcInfo64=(DFSCRCINFOWITHMD5_64*)pDfsCrcInfoP;
          {
              DfsMemcpy(pDfsCrcInfo64->bMd5,pDfsCrcInfoParam->bMd5,16);
          }
          pDfsCrcInfo64->dfBeginPosHigh = ConvertuLongToLongIntel((dfuLong32)(((pDfsCrcInfoParam)->dfBeginPos)>>32));
          pDfsCrcInfo64->dfEndPosHigh = ConvertuLongToLongIntel((dfuLong32)(((pDfsCrcInfoParam)->dfEndPos)>>32));
      }
      else
      {
          DFSCRCINFOONLYCRC32_64* pDfsCrcInfo64=(DFSCRCINFOONLYCRC32_64*)pDfsCrcInfoP;
          pDfsCrcInfo64->dfBeginPosHigh = ConvertuLongToLongIntel((dfuLong32)(((pDfsCrcInfoParam)->dfBeginPos)>>32));
          pDfsCrcInfo64->dfEndPosHigh = ConvertuLongToLongIntel((dfuLong32)(((pDfsCrcInfoParam)->dfEndPos)>>32));
      }
  }
  else
  {
    if ((((pDfsCrcInfoParam->dfTypeCrcAndMd5Info) & TYPECRCINFO_MD5)!=0))
        DfsMemcpy(pDfsCrcInfo->bMd5,pDfsCrcInfoParam->bMd5,16);
  }
}

BOOL ConvertDfsCrcInfoToDfsCrcInfoParam(const DFSCRCINFO_FULLSIZESTRUCTURE * pDfsCrcInfoP,
                                        DFSCRCINFOPARAM * pDfsCrcInfoParam)
{
  const DFSCRCINFOWITHMD5_32* pDfsCrcInfo=(const DFSCRCINFOWITHMD5_32*)pDfsCrcInfoP;
  dfuLong32 dfTypeCrcAndMd5Info;
  BOOL fRet=TRUE;

  pDfsCrcInfoParam->dfTypeCrcAndMd5Info = dfTypeCrcAndMd5Info =
    ConvertuLongIntelToLong((pDfsCrcInfo)->dfTypeCrcAndMd5Info);
  pDfsCrcInfoParam->dfBeginPos =
    ConvertuLongIntelToLong((pDfsCrcInfo)->dfBeginPos);
  pDfsCrcInfoParam->dfEndPos =
    ConvertuLongIntelToLong((pDfsCrcInfo)->dfEndPos);
  pDfsCrcInfoParam->dfCrc32Value =
    ConvertuLongIntelToLong((pDfsCrcInfo)->dfCrcValue);

  pDfsCrcInfoParam->fMd5 = (pDfsCrcInfoParam->dfTypeCrcAndMd5Info & TYPECRCINFO_MD5)!=0;
  pDfsCrcInfoParam->fSha1 = FALSE;
  pDfsCrcInfoParam->fSha256 = FALSE;

  if ((pDfsCrcInfoParam->dfTypeCrcAndMd5Info & TYPECRCINFO_STRUCTSIZEPRESENT)!=0)
  {
      BOOL fFilledSHA256,fFilledSHA1,fFilledMD5;
      DFSCRCINFOWITHMD5SHA1_64* pDfsCrcInfoSha64 = (DFSCRCINFOWITHMD5SHA1_64*)pDfsCrcInfo;
      //dfuLong32 dfStructSize = ConvertuLongIntelToLong((pDfsCrcInfoSha64)->dfStructSize);
      dfuLong32 dfStructSizeMinimal;

      fFilledSHA256 = ((((dfTypeCrcAndMd5Info) & TYPECRCINFO_SHA256)!=0) &&
                    (((dfTypeCrcAndMd5Info) & TYPECRCINFO_SHA256EMPTY)==0));

      fFilledSHA1 = ((((dfTypeCrcAndMd5Info) & TYPECRCINFO_SHA1)!=0) &&
                    (((dfTypeCrcAndMd5Info) & TYPECRCINFO_SHA1EMPTY)==0));

      fFilledMD5 = ((((dfTypeCrcAndMd5Info) & TYPECRCINFO_MD5)!=0) &&
                     (((dfTypeCrcAndMd5Info) & TYPECRCINFO_MD5EMPTY)==0));

      pDfsCrcInfoParam->dfBeginPos |=
            ((dfuLong64)(ConvertuLongIntelToLong((pDfsCrcInfoSha64)->dfBeginPosHigh))) << 32;
      pDfsCrcInfoParam->dfEndPos |=
            ((dfuLong64)(ConvertuLongIntelToLong((pDfsCrcInfoSha64)->dfEndPosHigh))) << 32;

      pDfsCrcInfoParam->fMd5 = fFilledMD5;
      pDfsCrcInfoParam->fSha1 = fFilledSHA1;
      pDfsCrcInfoParam->fSha256 = fFilledSHA256;

      dfStructSizeMinimal = sizeof(DFSCRCINFOWITHMD5SHA1_64);

      if ((!fFilledMD5) && (!fFilledSHA1))
      {
          dfStructSizeMinimal -= 20;
      }
      else
      if ((fFilledMD5) && (!fFilledSHA1))
      {
          dfStructSizeMinimal -= 20;
          DfsMemcpy(pDfsCrcInfoParam->bMd5,pDfsCrcInfoSha64->bBinDataPart1,16);
      }
      else
      if ((!fFilledMD5) && (fFilledSHA1))
      {
          dfStructSizeMinimal -= 16;
          DfsMemcpy(pDfsCrcInfoParam->bSha1,pDfsCrcInfoSha64->bBinDataPart1,16);
          DfsMemcpy(((dfbytep)pDfsCrcInfoParam->bSha1)+16,pDfsCrcInfoSha64->bBinDataPart2,4);
      }
      else
      if ((fFilledMD5) && (fFilledSHA1))
      {
          DfsMemcpy(pDfsCrcInfoParam->bMd5,pDfsCrcInfoSha64->bBinDataPart1,16);
          DfsMemcpy(pDfsCrcInfoParam->bSha1,pDfsCrcInfoSha64->bBinDataPart2,20);
      }

      if (fFilledSHA256)
      {
          dfStructSizeMinimal = sizeof(DFSCRCINFOWITHMD5SHA1SHA256_64);
          dfuLong32 nbDataBefore = 0;
          nbDataBefore += (fFilledMD5 ? 16 : 0);
          nbDataBefore += (fFilledSHA1 ? 20 : 0);
          if (nbDataBefore < 16)
          {
              DfsMemcpy(pDfsCrcInfoParam->bSha256, pDfsCrcInfoSha64->bBinDataPart1 + nbDataBefore, 16-nbDataBefore);
              DfsMemcpy(((dfbytep)pDfsCrcInfoParam->bSha256) + (16 - nbDataBefore), pDfsCrcInfoSha64->bBinDataPart2, 32 - (16 - nbDataBefore));
              dfStructSizeMinimal = (dfuLong32)((((dfbytep)pDfsCrcInfoSha64->bBinDataPart2) + (32 - (16 - nbDataBefore))) - ((dfbytep)pDfsCrcInfoSha64));
          }
          else
          {
              DfsMemcpy(((dfbytep)pDfsCrcInfoParam->bSha256), pDfsCrcInfoSha64->bBinDataPart2 + (nbDataBefore - 16), 32);
              dfStructSizeMinimal = (dfuLong32)((((dfbytep)pDfsCrcInfoSha64->bBinDataPart2) + (nbDataBefore - 16) + 32) - ((dfbytep)pDfsCrcInfoSha64));
          }
      }
  }
  else
  if ((pDfsCrcInfoParam->dfTypeCrcAndMd5Info & TYPECRCINFO_64BITSPOS) != 0)
  {
      if (pDfsCrcInfoParam->fMd5)
      {
          const DFSCRCINFOWITHMD5_64* pDfsCrcInfo64=(const DFSCRCINFOWITHMD5_64*)pDfsCrcInfoP;
          pDfsCrcInfoParam->dfBeginPos |=
              ((dfuLong64)(ConvertuLongIntelToLong((pDfsCrcInfo64)->dfBeginPosHigh))) << 32;
          pDfsCrcInfoParam->dfEndPos |=
              ((dfuLong64)(ConvertuLongIntelToLong((pDfsCrcInfo64)->dfEndPosHigh))) << 32;
          if (pDfsCrcInfoParam->dfTypeCrcAndMd5Info & TYPECRCINFO_MD5EMPTY)
              pDfsCrcInfoParam->fMd5 = FALSE;
          else
              DfsMemcpy(pDfsCrcInfoParam->bMd5,pDfsCrcInfo64->bMd5,16);
      }
      else
      {
          const DFSCRCINFOONLYCRC32_64* pDfsCrcInfo64=(const DFSCRCINFOONLYCRC32_64*)pDfsCrcInfoP;
          pDfsCrcInfoParam->dfBeginPos |=
              ((dfuLong64)(ConvertuLongIntelToLong((pDfsCrcInfo64)->dfBeginPosHigh))) << 32;
          pDfsCrcInfoParam->dfEndPos |=
              ((dfuLong64)(ConvertuLongIntelToLong((pDfsCrcInfo64)->dfEndPosHigh))) << 32;
      }
  }
  else
  {
    if (pDfsCrcInfoParam->fMd5)
        DfsMemcpy(pDfsCrcInfoParam->bMd5,pDfsCrcInfo->bMd5,16);
  }
  return fRet;
}

BOOL ConvertDfsFileProperties(DFSFILEPOSPROPERTIESINFO* pDfsFilePosPropertiesInfo,
                              dfvoidp TagBufProperties,dfuLong32 TagSizeProperties)
{
    DFSFILEPOSPROPERTIES6464 DfsFilePosProperties6464;
    DFSFILEPOSPROPERTIES6464* pDfsFileProperties=NULL;
    memset(&DfsFilePosProperties6464,0,sizeof(DfsFilePosProperties6464));
    if ((TagSizeProperties != sizeof(DFSFILEPOSPROPERTIES6464)) &&
        (TagSizeProperties != sizeof(DFSFILEPOSPROPERTIES3264)) &&
        (TagSizeProperties != sizeof(DFSFILEPOSPROPERTIES32)))
        return FALSE;
    memcpy(&DfsFilePosProperties6464,TagBufProperties,TagSizeProperties);
    if (pDfsFileProperties != NULL)
        memcpy(pDfsFileProperties,&DfsFilePosProperties6464,sizeof(DFSFILEPOSPROPERTIES6464));

    if (pDfsFilePosPropertiesInfo != NULL)
    {
        pDfsFilePosPropertiesInfo->dfContentStoreMethod = ConvertuLongIntelToLong(DfsFilePosProperties6464.dfContentStoreMethod);

        pDfsFilePosPropertiesInfo->dfFilePos =
            ConvertDualuLongIntelTouLong64(DfsFilePosProperties6464.dfFilePosLow,
                                           DfsFilePosProperties6464.dfFilePosHigh);

        pDfsFilePosPropertiesInfo->dfFileEncodedSizeWithoutPreAndPostHeader =
            ConvertDualuLongIntelTouLong64(DfsFilePosProperties6464.dfFileEncodedSizeWithoutPreAndPostHeaderLow,
                                           DfsFilePosProperties6464.dfFileEncodedSizeWithoutPreAndPostHeaderHigh);

        pDfsFilePosPropertiesInfo->dfFileEncodedSizeWithPreAndPostHeader =
            ConvertDualuLongIntelTouLong64(DfsFilePosProperties6464.dfFileEncodedSizeWithPreAndPostHeaderLow,
                                           DfsFilePosProperties6464.dfFileEncodedSizeWithPreAndPostHeaderHigh);

        pDfsFilePosPropertiesInfo->dfFileSizeContentUncompressed =
            ConvertDualuLongIntelTouLong64(DfsFilePosProperties6464.dfFileSizeContentUncompressedLow,
                                           DfsFilePosProperties6464.dfFileSizeContentUncompressedHigh);
    }


    return TRUE;
}

dfuLong32 ConvertDfsFilePropertiesToTag(const DFSFILEPOSPROPERTIESINFO* pDfsFilePosPropertiesInfo,
                                        dfvoidp TagBufProperties,dfuLong32 TagSizeProperties)
{
    DFSFILEPOSPROPERTIES6464 DfsFilePosProperties6464;
    dfuLong32 dfRet;

    DfsFilePosProperties6464.dfContentStoreMethod = ConvertuLongToLongIntel(pDfsFilePosPropertiesInfo->dfContentStoreMethod);

    ConvertuLong64ToDualuLongIntel(
        pDfsFilePosPropertiesInfo->dfFilePos,
        &DfsFilePosProperties6464.dfFilePosLow,
        &DfsFilePosProperties6464.dfFilePosHigh);

    ConvertuLong64ToDualuLongIntel(
        pDfsFilePosPropertiesInfo->dfFileEncodedSizeWithoutPreAndPostHeader,
        &DfsFilePosProperties6464.dfFileEncodedSizeWithoutPreAndPostHeaderLow,
        &DfsFilePosProperties6464.dfFileEncodedSizeWithoutPreAndPostHeaderHigh);

    ConvertuLong64ToDualuLongIntel(
        pDfsFilePosPropertiesInfo->dfFileEncodedSizeWithPreAndPostHeader,
        &DfsFilePosProperties6464.dfFileEncodedSizeWithPreAndPostHeaderLow,
        &DfsFilePosProperties6464.dfFileEncodedSizeWithPreAndPostHeaderHigh);

    ConvertuLong64ToDualuLongIntel(
        pDfsFilePosPropertiesInfo->dfFileSizeContentUncompressed,
        &DfsFilePosProperties6464.dfFileSizeContentUncompressedLow,
        &DfsFilePosProperties6464.dfFileSizeContentUncompressedHigh);

    if ((((pDfsFilePosPropertiesInfo->dfFileEncodedSizeWithoutPreAndPostHeader) >> 32) != 0) ||
        (((pDfsFilePosPropertiesInfo->dfFileEncodedSizeWithPreAndPostHeader) >> 32) != 0) ||
        (((pDfsFilePosPropertiesInfo->dfFileSizeContentUncompressed) >> 32) != 0))
        dfRet = sizeof(DFSFILEPOSPROPERTIES6464);
    else
    if (((pDfsFilePosPropertiesInfo->dfFileSizeContentUncompressed) >> 32) != 0)
        dfRet = sizeof(DFSFILEPOSPROPERTIES3264);
    else
        //dfRet = sizeof(DFSFILEPOSPROPERTIES32);
        dfRet = sizeof(DFSFILEPOSPROPERTIES3264);

    if (dfRet > TagSizeProperties)
        return 0;

    memcpy(TagBufProperties,&DfsFilePosProperties6464,dfRet);
    return dfRet;
}

int CompareDfsTm(const DFSTM* pdfsTm1,const DFSTM* pdfsTm2)
{
  int iRet = 0;
  if (pdfsTm1->df_year != pdfsTm2->df_year)
      iRet = (pdfsTm1->df_year < pdfsTm2->df_year) ? -1 : 1;
  else
  if (pdfsTm1->df_mon != pdfsTm2->df_mon)
      iRet = (pdfsTm1->df_mon < pdfsTm2->df_mon) ? -1 : 1;
  else
  if (pdfsTm1->df_mday != pdfsTm2->df_mday)
      iRet = (pdfsTm1->df_mday < pdfsTm2->df_mday) ? -1 : 1;
  else
  if (pdfsTm1->df_hour != pdfsTm2->df_hour)
      iRet = (pdfsTm1->df_hour < pdfsTm2->df_hour) ? -1 : 1;
  else
  if (pdfsTm1->df_min != pdfsTm2->df_min)
      iRet = (pdfsTm1->df_min < pdfsTm2->df_min) ? -1 : 1;
  else
  if (pdfsTm1->df_sec != pdfsTm2->df_sec)
      iRet = (pdfsTm1->df_sec < pdfsTm2->df_sec) ? -1 : 1;
  else
  if (pdfsTm1->df_msec != pdfsTm2->df_msec)
      iRet = (pdfsTm1->df_msec < pdfsTm2->df_msec) ? -1 : 1;

  return iRet;
}

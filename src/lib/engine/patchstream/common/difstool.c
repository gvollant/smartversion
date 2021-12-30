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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "zlib.h"
#include "difbasic.h"
#include "DfsTlTyp.h"
#include "difstool.h"


dfuLong32 DfsCrc32(dfuLong32 previousCrc,const void* ptr, dfuLong32 sizelen)
{
  return (dfuLong32)crc32(previousCrc, ptr, sizelen);
}

dfuLong32 DfsAdler32(dfuLong32 previousAdler,const void* ptr, dfuLong32 sizelen)
{
  return (dfuLong32)adler32(previousAdler, ptr, sizelen);
}

void DfsClearStruct(void *ptrStruct, dfbyte fillbyte, size_t sizeStruct)
{
  memset(ptrStruct, (int) ((unsigned int) fillbyte), sizeStruct);
}

//#define VERBOSE_BIG_ALLOC

#ifdef _DEBUG
#include <stdio.h>
dfuLong32 dfTotalAlloc = 0;
dfuLong32 dfTotalNbAlloc = 0;

BOOL CheckEmptyAlloc()
{
  if ((dfTotalAlloc == 0) && (dfTotalNbAlloc == 0))
    return TRUE;

  printf("\nMEMORY STILL ALLOCATED\n");
  return FALSE;
}

void *DfsRealloc(void *ptr, size_t size)
{
  void *ptrRet=NULL;
  dfuLong32 *ptrOld = NULL;

#if ((defined(_DEBUG)) && (defined(VERBOSE_BIG_ALLOC)))
  if (size > 1024*1024*1)
  {
      dfuLong32 dfSizeMega = (dfuLong32)(size/(1024*1024));
      printf("Big alloc %u MB\n",dfSizeMega);
  }
#endif

  if (ptr != NULL)
  {
    ptrOld = ((dfuLong32 *) ptr) - 1;
    dfTotalAlloc -= *((dfuLong32 *) ptrOld);
  }

  if ((ptr != NULL) && (size != 0))
  {
    ptrRet = ((dfuLong32 *) realloc(ptrOld, size + 4)) ;
    if (ptrRet != NULL)
        ptrRet = (((dfuLong32 *) ptrRet) + 1) ;
  }
  else if ((ptr == NULL) && (size != 0))
  {
    ptrRet = ((dfuLong32 *) malloc(size + 4)) ;
    if (ptrRet != NULL)
        ptrRet = (((dfuLong32 *) ptrRet) + 1) ;
    dfTotalNbAlloc++;
  }
  else if ((ptr != NULL))
  {
    free(ptrOld);
    dfTotalNbAlloc--;
    ptrRet = NULL;
  }

  if ((ptrRet == NULL) && (size>0))
  {
      // this is a big problem
      return NULL;

  }

  if (ptrRet != NULL)
  {
    *(((dfuLong32 *) ptrRet) - 1) = (dfuLong32)size;
    dfTotalAlloc += *(((dfuLong32 *) ptrRet) - 1);
  }
  return ptrRet;
}

#else
BOOL CheckEmptyAlloc()
{
	return TRUE;
}

void *DfsRealloc(void *ptr, size_t size)
{
  if ((ptr != NULL) && (size != 0))
    return realloc(ptr, size);
  else if ((ptr == NULL) && (size != 0))
    return malloc(size);
  else if ((ptr != NULL))
    free(ptr);
  return NULL;
}
#endif

void *DfsMalloc(size_t size)
{
  return DfsRealloc(NULL, size);
}

void DfsFree(void *ptr)
{
  DfsRealloc(ptr, 0);
}



#define DEFAULT_STEP (0x1000)
BOOL DfsCheckAllocatedMemory(dfvoidp * ptr, dfuLong32 * sizeAllocated,
                             dfuLong32 sizeNeeded, dfuLong32 step)
{
  dfuLong32 sizeTryAlloc;
  dfvoidp newPtr;
  if (sizeNeeded <= *sizeAllocated)
    return TRUE;

  /* note : here sizeNeeded > 0 ! */
  if (step == 0)
    step = DEFAULT_STEP;

  sizeTryAlloc = ((sizeNeeded + (step - 1)) / step) * step;
  newPtr = DfsRealloc(*ptr, sizeTryAlloc);
  if (newPtr == NULL)
  {
    return FALSE;
  }
  else
  {
    *ptr = newPtr;
    *sizeAllocated = sizeTryAlloc;
    return TRUE;
  }
}

#ifdef REDEFINED_MEMFNC
void DfsMemcpy(void *ptrdest, const void *ptrsrc, dfuLong32 size)
{
    if (size>0)
        memcpy(ptrdest, ptrsrc, size);
}

void DfsMemmove(void *ptrdest, const void *ptrsrc, dfuLong32 size)
{
    if (size>0)
        memmove(ptrdest, ptrsrc, size);
}

int DfsMemcmp(const void *ptrsrc1, const void *ptrsrc2, dfuLong32 size)
{
  dfuLong32 i;
  const dfbyte *ptsrc1parc = (const dfbyte *) ptrsrc1;
  const dfbyte *ptsrc2parc = (const dfbyte *) ptrsrc2;
  for (i = 0; i < size; i++)
  {
    dfbyte c1 = *ptsrc1parc;
    dfbyte c2 = *ptsrc2parc;
    if (c1 < c2)
      return -1;
    if (c1 > c2)
      return 1;
    ptsrc1parc++;
    ptsrc2parc++;
  }
  return 0;
}
#endif

dfuLong32 MyLogLong(dfuLong32 value)
{
  dfuLong32 ret = 0;

  while (value > 0x1000)
  {
    ret += 4 * 3;
    value >>= (4 * 3);
  }
  while (value > 0x10)
  {
    ret += 4;
    value >>= 4;
  }
  while (value > 0x0)
  {
    ret += 1;
    value >>= 1;
  }
  return ret;
}

dfuLong32 MyLogLongPtr(dfuIntPtr value)
{
  dfuLong32 ret = 0;

  while (value > 0x1000)
  {
    ret += 4 * 3;
    value >>= (4 * 3);
  }
  while (value > 0x10)
  {
    ret += 4;
    value >>= 4;
  }
  while (value > 0x0)
  {
    ret += 1;
    value >>= 1;
  }
  return ret;
}

dfuLong32 MyLogLong64(dfuLong64 value)
{
  dfuLong32 ret = 0;

  while (value > 0x1000)
  {
    ret += 4 * 3;
    value >>= (4 * 3);
  }
  while (value > 0x10)
  {
    ret += 4;
    value >>= 4;
  }
  while (value > 0x0)
  {
    ret += 1;
    value >>= 1;
  }
  return ret;
}

void difstr_putValue_inmemory(
     void *dest,
     dfuLong32 x,
     dfuInt nbByte)
{
  unsigned char *buf = (unsigned char *) dest;
  dfuInt n;
  for (n = 0; n < nbByte; n++)
  {
    buf[n] = (unsigned char) (x & 0xff);
    x >>= 8;
  }
}

dfuLong32 difstr_getValue_inmemory(
     void *src,
     dfuInt nbByte)
{
  unsigned char *buf = (unsigned char *) src;
  dfuInt n;
  dfuLong32 x = 0;
  for (n = 0; n < nbByte; n++)
    x |= (((unsigned char) (buf[n])) << (n * 8));

  return x;
}

dfuInt difstr_getNeededByte_forValue(dfuLong32 x)
{
    dfuInt n;
    dfuInt ret = 0;
    for (n=0; n < 4; n++)
    {
        if ((x & 0xff) != 0)
            ret = n + 1;
        x >>= 8;
    }
    return ret;
}

void ClearZStream(
     z_streamp strm)
{
  strm->next_in = NULL;
  strm->avail_in = 0;
  strm->total_in = 0;

  strm->next_out = NULL;
  strm->avail_out = 0;
  strm->total_out = 0;

  strm->zalloc = (alloc_func) 0;
  strm->zfree = (free_func) 0;
  strm->opaque = (voidpf) 0;
}

#if ((defined(WIN32) || defined(_WIN32) || defined (_WIN64) || defined (_M_IX86)  || \
      defined(__i386) || defined(__i386__) || defined(__x86_64) || defined(__x86_64__) || \
      defined(TARGET_CPU_X86) || defined(TARGET_CPU_X86_64) || defined(__LITTLE_ENDIAN__)   \
           ) && (!(defined(INTEL_X86_LIKE_LITTLE_ENDIAN))))
#define INTEL_X86_LIKE_LITTLE_ENDIAN 1
#endif

#ifdef INTEL_X86_LIKE_LITTLE_ENDIAN

dfuInt16Intel ConvertuInt16TouInt16Intel(dfuInt16 value)
{
  return *((dfuInt16Intel *) & value);
}

dfuInt16 ConvertuInt16IntelTouInt16(dfuInt16Intel value)
{
  return (*((dfuInt16 *) & value)) & (0xffff);
}

dfuLong32Intel ConvertuLongToLongIntel(dfuLong32 value)
{
  return *((dfuLong32Intel *) & value);
}

dfuLong32 ConvertuLongIntelToLong(dfuLong32Intel value)
{
  return (*((dfuLong32 *) & value)) & (0xffffffff);
}


dfuLong64 ConvertDualuLongIntelTouLong64(dfuLong32Intel valueLow, dfuLong32Intel valueHigh)
{
    dfuLong32 uLow = *((dfuLong32 *) & valueLow);
    dfuLong32 uHigh = *((dfuLong32 *) & valueHigh);
    return uLow | (((dfuLong64)uHigh)<<32);
}

void ConvertuLong64ToDualuLongIntel(dfuLong64 value, dfuLong32Intel* pLow, dfuLong32Intel* pHigh)
{
    dfuLong32 uLow = (dfuLong32)value;
    dfuLong32 uHigh = (dfuLong32)(value>>32);
    if (pLow!=NULL)
      *pLow = *((dfuLong32Intel *) (&uLow));
    if (pHigh!=NULL)
      *pHigh = *((dfuLong32Intel *) (&uHigh));
}

dfuLong64Intel ConvertuLongToLongIntel64(dfuLong64 value)
{
  return *((dfuLong64Intel *) & value);
}

dfuLong64 ConvertuLongIntelToLong64(dfuLong64Intel value)
{
  return *((dfuLong64 *) & value);
}
#else


dfuInt16Intel ConvertuInt16TouInt16Intel(dfuInt16 value)
{
    dfuInt16Intel retv;
    retv.tab[0]= (dfbyte)value;
    retv.tab[1]= (dfbyte)(value>>8);
  return retv;
}

dfuInt16 ConvertuInt16IntelTouInt16(dfuInt16Intel value)
{
  dfuInt16 retv;
  retv = (value.tab[0]) | (((dfuInt16)value.tab[1])<<8);
  return retv;
}

dfuLong32Intel ConvertuLongToLongIntel(dfuLong32 value)
{
    dfuLong32Intel retv;
    retv.tab[0]= (dfbyte)value;
    retv.tab[1]= (dfbyte)(value>>8);
    retv.tab[2]= (dfbyte)(value>>16);
    retv.tab[3]= (dfbyte)(value>>24);
  return retv;
}

dfuLong64 ConvertDualuLongIntelTouLong64(dfuLong32Intel valueLow, dfuLong32Intel valueHigh)
{
    dfuLong64 retv;
    //dfuLong32 uLow = *((dfuLong32 *) & valueLow);
    //dfuLong32 uHigh = *((dfuLong32 *) & valueHigh);
    //return uLow | (((dfuLong64)uHigh)<<32);
    retv = (valueLow.tab[0]) | (((dfuLong32)valueLow.tab[1])<<8) | (((dfuLong32)valueLow.tab[2])<<16) | (((dfuLong32)valueLow.tab[3])<<24) |
           (((dfuLong64)valueHigh.tab[0])<<32) | (((dfuLong64)valueHigh.tab[1])<<40) | (((dfuLong64)valueHigh.tab[2])<<48) | (((dfuLong64)valueHigh.tab[3])<<56);
    return retv;
}

void ConvertuLong64ToDualuLongIntel(dfuLong64 value, dfuLong32Intel* pLow, dfuLong32Intel* pHigh)
{
    dfuLong32Intel Low,High;
    Low.tab[0]= (dfbyte)value ;
    Low.tab[1]= (dfbyte)(value>>8);
    Low.tab[2]= (dfbyte)(value>>16);
    Low.tab[3]= (dfbyte)(value>>24);
    High.tab[0]= (dfbyte)(value>>32) ;
    High.tab[1]= (dfbyte)(value>>40);
    High.tab[2]= (dfbyte)(value>>48);
    High.tab[3]= (dfbyte)(value>>56);
    *pLow=Low;
    *pHigh=High;
}

dfuLong32 ConvertuLongIntelToLong(dfuLong32Intel value)
{
  dfuLong32 retv;
  retv = (value.tab[0]) | (((dfuLong32)value.tab[1])<<8) | (((dfuLong32)value.tab[2])<<16) | (((dfuLong32)value.tab[3])<<24);
  return retv;
}

dfuLong64Intel ConvertuLongToLongIntel64(dfuLong64 value)
{
    dfuLong64Intel retv;
    retv.tab[0]= (dfbyte)value ;
    retv.tab[1]= (dfbyte)(value>>8);
    retv.tab[2]= (dfbyte)(value>>16);
    retv.tab[3]= (dfbyte)(value>>24);
    retv.tab[4]= (dfbyte)(value>>32) ;
    retv.tab[5]= (dfbyte)(value>>40);
    retv.tab[6]= (dfbyte)(value>>48);
    retv.tab[7]= (dfbyte)(value>>56);
    return retv;
}

dfuLong64 ConvertuLongIntelToLong64(dfuLong64Intel value)
{
  dfuLong64 retv;
  retv = (value.tab[0]) | (((dfuLong32)value.tab[1])<<8) | (((dfuLong32)value.tab[2])<<16) | (((dfuLong32)value.tab[3])<<24) |
         (((dfuLong64)value.tab[4])<<32) | (((dfuLong64)value.tab[5])<<40) | (((dfuLong64)value.tab[6])<<48) | (((dfuLong64)value.tab[7])<<56);
  return retv;
}

#endif

dfuLong64 AdduLong64(dfuLong64 a,dfuLong64 b)
{
    return a+b;
}

dfuLong64 SubuLong64(dfuLong64 a,dfuLong64 b)
{
    return a-b;
}

dfuLong32 GetuLong64Low32(dfuLong64 a)
{
    return (dfuLong32)(a & 0xffffffffUL);
}

dfuLong32 GetuLong64High32(dfuLong64 a)
{
    return (dfuLong32)(((dfuLong64)a) >> 32);
}

dfuLong64 MakeuLong64(dfuLong32 dfLow,dfuLong32 dfHigh)
{
    return ((dfuLong64)(dfLow & 0xffffffffUL)) | (((dfuLong64)(dfHigh & 0xffffffffUL)) << 32);
}



#define USEARITH64CPLX

#ifdef USEARITH64CPLX

void dfAdd64(dfuLong32 *dwLow,dfuLong32 *dwHigh,dfuLong32 dwAddLow,dfuLong32 dwAddHigh)
{
    dfuLong64 a,b,c;
    a = (*dwLow) | (((dfuLong64)(*dwHigh))<<32);
    b = dwAddLow | (((dfuLong64)dwAddHigh)<<32);
    c=a+b;
    *dwLow = (dfuLong32)(c);
    *dwHigh = (dfuLong32)(c>>32);
}

void dfMultiply64(dfuLong32 *pdwLow,dfuLong32 *pdwHigh,dfuLong32 dwLow2,dfuLong32 dwHigh2)
{
    dfuLong64 a,b,c;
    a = (*pdwLow) | (((dfuLong64)(*pdwHigh))<<32);
    b = dwLow2 | (((dfuLong64)dwHigh2)<<32);
    c=a*b;
    *pdwLow = (dfuLong32)(c);
    *pdwHigh = (dfuLong32)(c>>32);
}

void dfSub64(dfuLong32 *dwLow,dfuLong32 *dwHigh,dfuLong32 dwSubLow,dfuLong32 dwSubHigh)
{
    dfuLong64 a,b,c;
    a = (*dwLow) | (((dfuLong64)(*dwHigh))<<32);
    b = dwSubLow | (((dfuLong64)dwSubHigh)<<32);
    c=a-b;
    *dwLow = (dfuLong32)(c);
    *dwHigh = (dfuLong32)(c>>32);
}

#else

void dfAdd64(dfuLong32 *dwLow,dfuLong32 *dwHigh,dfuLong32 dwAddLow,dfuLong32 dwAddHigh)
{
        if (((*dwLow)+dwAddLow<dwAddLow) || ((*dwLow)+dwAddLow<(*dwLow)))
                *dwHigh++;
        (*dwLow)+=dwAddLow;
        (*dwHigh)+=dwAddHigh;
}

#define Mul64_PR_BIT(n)       ((dfuLong32)1 << (n))
#define Mul64_PR_BITMASK(n)   (Mul64_PR_BIT(n) - 1)

#define Mul64__lo16(a)        ((a) & Mul64_PR_BITMASK(16))
#define Mul64__hi16(a)        ((a) >> 16)

void dfMultiply64(dfuLong32 *pdwLow,dfuLong32 *pdwHigh,dfuLong32 dwLow2,dfuLong32 dwHigh2)
{
    dfuLong32 dwRLow,dwRHigh;
    dfuLong32 _a1, _a0, _b1, _b0, _y0, _y1, _y2, _y3;
    dfuLong32 dwLow = *pdwLow;
    dfuLong32 dwHigh= *pdwHigh;


     _a1 = Mul64__hi16(dwLow);
     _a0 = Mul64__lo16(dwLow);
     _b1 = Mul64__hi16(dwLow2);
     _b0 = Mul64__lo16(dwLow2);
     _y0 = _a0 * _b0;
     _y1 = _a0 * _b1;
     _y2 = _a1 * _b0;
     _y3 = _a1 * _b1;
     _y1 += Mul64__hi16(_y0);                         /* can't carry */
     _y1 += _y2;                                /* might carry */
     if (_y1 < _y2)
        _y3 += (dfuLong32)(Mul64_PR_BIT(16));  /* propagate */
     dwRLow = (Mul64__lo16(_y1) << 16) + Mul64__lo16(_y0);
     dwRHigh = _y3 + Mul64__hi16(_y1);

    dwRHigh += dwHigh * dwLow2 + dwLow * dwHigh2;

    *pdwLow= dwRLow;
    *pdwHigh= dwRHigh;
}

void dfSub64(dfuLong32 *dwLow,dfuLong32 *dwHigh,dfuLong32 dwSubLow,dfuLong32 dwSubHigh)
{
    (*dwHigh) -= dwSubHigh;
    if ((*dwLow)>=dwSubLow)
    {
        (*dwLow) -= dwSubLow;
    }
    else
    {
        (*dwHigh) --;
        (*dwLow) -= dwSubLow;
    }
}
#endif

int  dfCompareValue64(dfuLong32 dwLow1,dfuLong32 dwHigh1,dfuLong32 dwLow2,dfuLong32 dwHigh2)
{
    if (dwHigh1>dwHigh2)
        return 1;
    else
        if (dwHigh1<dwHigh2)
            return -1;
        else
            if (dwLow1>dwLow2)
                return 1;
            else
                if (dwLow1<dwLow2)
                    return -1;
                else
                    return 0;
}
/*
void dfDivide64(dfuLong32 *dwLow,dfuLong32 *dwHigh,dfuLong32 dwLow2,dfuLong32 dwHigh2)
{
    if (((*dwHigh)==0) && (dwHigh2==0))
        *dwLow = (*dwLow)/dwLow2;
    else
    {
        /////----
    }
}
*/

#ifndef Round8
#define Round8(x) ((((x)+7)/8)*8)
#endif

typedef struct
{
  dfuLong32 dfSizeStruct;
  dfuLong32 dfNumError;
  dfuLong32 dfSubError;
  dfuLong32 dfNbErrorInfo;
  dfuLong32 dfPosOffsetList;
  dfuLong32 dfPosSizeList;
  dfuLong32 dfPosTagList;
  dfuLong32 dfPtrList;
} INTERNAL_ERROR_INFO;

H_ERROR_INFO SVFAPI CreateErrorInfoBlock(dfuLong32 dfError,dfuLong32 dfSubError,
                                  dfuLong32 dfNbErrorItem,const ERROR_INFO_ITEM* pErrorInfoItem)
{
    dfuLong32 i;
    dfuLong32 dfSizeStruct;
    INTERNAL_ERROR_INFO* pei ;
    dfSizeStruct = Round8(sizeof(INTERNAL_ERROR_INFO)) +
                   Round8(sizeof(dfuLong32)*dfNbErrorItem)*3 ;

    for (i=0;i<dfNbErrorItem;i++)
        dfSizeStruct += Round8(((pErrorInfoItem+i)->dfSize)+4);


    pei = (INTERNAL_ERROR_INFO*)DfsMalloc(dfSizeStruct);

    if (pei != NULL)
    {
        dfuLong32 dfCurPosData ;
        dfuLong32 * pdflOffsetList;
        dfuLong32 * pdflSizeList;
        dfuLong32 * pdflTagList;

        DfsClearStruct(pei,0,dfSizeStruct);
        pei->dfSizeStruct = dfSizeStruct;
        pei->dfNumError = dfError;
        pei->dfSubError = dfSubError;
        pei->dfNbErrorInfo = dfNbErrorItem ;

        pei->dfPosOffsetList = Round8(sizeof(INTERNAL_ERROR_INFO)) ;
        pdflOffsetList = (dfuLong32*) (((dfbytep)pei)+pei->dfPosOffsetList);

        pei->dfPosSizeList = pei->dfPosOffsetList + Round8(sizeof(dfuLong32)*dfNbErrorItem);
        pdflSizeList = (dfuLong32*) (((dfbytep)pei)+pei->dfPosSizeList);

        pei->dfPosTagList = pei->dfPosSizeList + Round8(sizeof(dfuLong32)*dfNbErrorItem);
        pdflTagList = (dfuLong32*) (((dfbytep)pei)+pei->dfPosTagList);

        dfCurPosData = pei->dfPosTagList + Round8(sizeof(dfuLong32)*dfNbErrorItem);

        for (i=0;i<dfNbErrorItem;i++)
        {
            DfsMemcpy(((dfbytep)pei)+dfCurPosData,(pErrorInfoItem+i)->pInfo,(pErrorInfoItem+i)->dfSize);
            *(pdflOffsetList+i)=dfCurPosData ;
            *(pdflSizeList+i)=(pErrorInfoItem+i)->dfSize;
            *(pdflTagList+i)=(pErrorInfoItem+i)->dfInfoTag;
            dfCurPosData += Round8(((pErrorInfoItem+i)->dfSize)+4);
        }
    }

    return (H_ERROR_INFO)pei;
}


BOOL SVFAPI FreeErrorInfoBlock(H_ERROR_INFO hErrorInfo)
{
    INTERNAL_ERROR_INFO* pei = (INTERNAL_ERROR_INFO*)hErrorInfo;
    if (pei != NULL)
      DfsFree(pei);
    return TRUE;
}


dfuLong32 SVFAPI GetErrorNumber(H_ERROR_INFO hErrorInfo)
{
    dfuLong32 dfRet = 0;
    if (hErrorInfo!=NULL)
        dfRet =  ((INTERNAL_ERROR_INFO*)hErrorInfo)->dfNumError;
    return dfRet;
}

dfuLong32 SVFAPI GetErrorSubNumber(H_ERROR_INFO hErrorInfo)
{
    dfuLong32 dfRet = 0;
    if (hErrorInfo!=NULL)
        dfRet =  ((INTERNAL_ERROR_INFO*)hErrorInfo)->dfSubError;
    return dfRet;
}

dfuLong32 SVFAPI GetNbErrorInfo(H_ERROR_INFO hErrorInfo)
{
    dfuLong32 dfRet = 0;
    if (hErrorInfo!=NULL)
        dfRet =  ((INTERNAL_ERROR_INFO*)hErrorInfo)->dfNbErrorInfo;
    return dfRet;
}

BOOL SVFAPI GetErrorInfoItem(H_ERROR_INFO hErrorInfo,dfuLong32 dfNbItem,dfbytep* ptr,dfuLong32* pdfSize,dfuLong32* pdfInfoTag)
{
    BOOL fRet=FALSE;
    if (hErrorInfo!=NULL)
    {
        INTERNAL_ERROR_INFO* pei = (INTERNAL_ERROR_INFO*)hErrorInfo;
        if (dfNbItem < pei->dfNbErrorInfo)
        {
            dfuLong32* pdflOffsetList = (dfuLong32*) (((dfbytep)pei)+(pei->dfPosOffsetList));
            dfuLong32* pdflSizeList = (dfuLong32*) (((dfbytep)pei)+(pei->dfPosSizeList));
            dfuLong32* pdflTagList = (dfuLong32*) (((dfbytep)pei)+(pei->dfPosTagList));

            *ptr = ((dfbytep)(pei)) + (*(pdflOffsetList+dfNbItem));
            *pdfSize = (*(pdflSizeList+dfNbItem));
            *pdfInfoTag = (*(pdflTagList+dfNbItem));
        }
    }

    return fRet;
}

BOOL SVFAPI GetErrorInfoItemByTag(H_ERROR_INFO hErrorInfo,dfuLong32 dfTag,dfbytep* ptr,dfuLong32* pdfSize)
{
    BOOL fRet=FALSE;
    if (hErrorInfo!=NULL)
    {
        dfuLong32 i;
        INTERNAL_ERROR_INFO* pei = (INTERNAL_ERROR_INFO*)hErrorInfo;
        for (i=0;i<pei->dfNbErrorInfo;i++)
        {
            dfuLong32* pdflOffsetList = (dfuLong32*) (((dfbytep)pei)+(pei->dfPosOffsetList));
            dfuLong32* pdflSizeList = (dfuLong32*) (((dfbytep)pei)+(pei->dfPosSizeList));
            dfuLong32* pdflTagList = (dfuLong32*) (((dfbytep)pei)+(pei->dfPosTagList));

            if ((*(pdflTagList+i)) == dfTag)
            {
                *ptr = ((dfbytep)(pei)) + (*(pdflOffsetList+i));
                *pdfSize = (*(pdflSizeList+i));
                fRet=TRUE;
                break;
            }
        }
    }

    return fRet;
}

long SVFAPI ConvertUnicodeStringToLong(dfwcharpc str)
{
    char szVal[256+10];
    long lValue;


    ConvertUnicodeToAnsi(str,szVal,256);
    szVal[256]=0;
    lValue=atol(szVal);
    return lValue;
}


dfwcharpc SVFAPI GetUnicodeStringEmpty()
{
    static const dfbyte dfByteArray[] = { '\0','\0' };
    return (dfwcharpc)dfByteArray ;
}

dfwcharpc SVFAPI GetUnicodeStringSlashSeparator()
{
    static const dfbyte dfByteArray[] = { '/','\0','\0','\0' };
    return (dfwcharpc)dfByteArray ;
}

dfwcharpc SVFAPI GetUnicodeStringAntiSlashSeparator()
{
    static const dfbyte dfByteArray[] = { '\\','\0','\0','\0' };
    return (dfwcharpc)dfByteArray ;
}

dfwcharpc SVFAPI GetUnicodeStringStar()
{
    static const dfbyte dfByteArray[] = { '*','\0','\0','\0' };
    return (dfwcharpc)dfByteArray ;
}

dfwcharpc SVFAPI GetUnicodeStringDot()
{
    static const dfbyte dfByteArray[] = { '.','\0','\0','\0' };
    return (dfwcharpc)dfByteArray ;
}

dfwcharpc SVFAPI GetUnicodeStringDotDot()
{
    static const dfbyte dfByteArray[] = { '.','\0','.','\0','\0','\0' };
    return (dfwcharpc)dfByteArray ;
}

dfwcharpc SVFAPI GetUnicodeSVFPrefix()
{
    static const dfbyte dfByteArray[] = { 'S','\0','V','\0','F','\0','\0','\0' };
    return (dfwcharpc)dfByteArray ;
}

dfwcharpc SVFAPI GetUnicodeLowerCaseSvf()
{
    static const dfbyte dfByteArray[] = { 's','\0','v','\0','f','\0','\0','\0' };
    return (dfwcharpc)dfByteArray ;
}

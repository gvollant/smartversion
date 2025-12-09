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

//#ifdef _DEBUG
#define VERBOSE_BUILDEVENT_no
//#endif

//#define VERIFY_BHFL_HASH
//#endif

#define USE_OLDBHFL_RED_no

/* PreWkFnd.c */
#include <stddef.h>

#include <memory.h>


#include "../common/difbasic.h"
#include "../common/DfsTlTyp.h"
#include "../common/difstool.h"
#include "../common/difstrm.h"
#include "../common/DfsOrigMemoryMap.h"
#include "PreWkFnd.h"

#include "zlib.h"


#ifndef local
#define local static
#endif

#define PWFAroundLower(dwValue,dwModulo) ((((dwValue)) / ((dwModulo))) * (dwModulo))
#define PWFAroundUpper(dwValue,dwModulo) (((((dwValue)) + ((dwModulo)) -1) / ((dwModulo))) * (dwModulo))
#define PWFDivideUpper(dwValue,dwModulo) (((((dwValue)) + ((dwModulo)) -1) / ((dwModulo))))

#define myMax(a,b)            (((a) > (b)) ? (a) : (b))
#define myMin(a,b)            (((a) < (b)) ? (a) : (b))


typedef dfuLong32 dfpwfPos;
#define NOTHING_POS ((dfpwfPos)0xffffffff)



#ifdef _DEBUG
#define CHECK_MAXORG_FIND
#endif

#if defined(VERBOSE_BUILDEVENT)||defined(CHECK_MAXORG_FIND)
#include <stdio.h>
#endif

#if defined(_DEBUG) || defined(VERBOSE_BUILDEVENT)
#include <windows.h>
#endif

#ifndef UNALIGNED
#if defined(_M_MRX000) || defined(_M_ALPHA) || defined(_M_PPC) || defined(_M_IA64) || defined(_M_AMD64)
#define UNALIGNED __unaligned
#else
#define UNALIGNED
#endif
#endif

typedef struct
{
  dfpwfPos *pdfpwfPos;
}
ARRAYFIRSTPOS;


typedef struct
{
  dfuLong32 dfSizeCluster;
  dfuLong32 dfSizeReducedCluster;
  dfuLong32 dfReducedMask;
  dfvoidp   pReducedData;
  dfuLong64 dfNbCells;
#ifdef USECRC16
  dfuInt16  Crc16Table[256];
#endif
} REDUCEDFULLFILE ;

#define ARRAYPOS_FIRSTPOS   (0)
#define ARRAYPOS_LASTPOS    (1)

typedef struct tagBLOCKHASH_FATLIKE
{
  dfuLong32 dfBlockCalcSize;
  dfuLong32 dfMinSearchSize;
  dfuLong32 dfHashBits;           /* < 32 */
  dfuLong32 dfAndOperator;       /* (2^dfHashBits)-1 */
  dfuLong32 dfArrayItem;          /* (2^dfHashBits) */

  dfuLong64 dfArrayLength;
  dfuLong32 dfNbArray;

  dfpwfPos *pLikeFatArrayBld;
  dfpwfPos *pSortedArray;
  const z_crc_t* pcrc32_table;
#ifdef USECRC16
  dfuInt16 Crc16Table[256];
#endif
  //tGetHashFatLikeValue fncGetHashFatLikeValue;

  dfuLong32 (* fncGetHashFatLikeValue)(const struct tagBLOCKHASH_FATLIKE* pBhfl, dfvoidpc pPos);

  ARRAYFIRSTPOS ArrayPosValue[2];
  dfvoidp pDualUsageBuffer;
  dfuIntPtr dfDualUsageBufferSize;
  dfuIntPtr dfSortedArrayBufferSize;
  dfuIntPtr dfArrayPosValueBufferSize;
} BLOCKHASH_FATLIKE;

//#define DOGETHASHFATLIKEVALUE(pBhfl,pPos) (((pBhfl)->fncGetHashFatLikeValue)((pBhfl),(pPos)))
//#define DOGETHASHFATLIKEVALUE(pBhfl,pPos) (GetHashFatLikeValueCrc16((pBhfl),(pPos)))
#define DOGETHASHFATLIKEVALUE(pBhfl,pPos) (GetHashFatLikeValueCrc32((pBhfl),(pPos)))


typedef dfuLong32 (* tGetHashFatLikeValue)(const BLOCKHASH_FATLIKE * pBhfl, dfvoidpc pPos);

typedef BLOCKHASH_FATLIKE* PBLOCKHASH_FATLIKE;

typedef struct
{
//  dfvoidpc pData;
  ORIGDATA* pOrg;
  dfuLong64 sizeData;

  dfvoidp   pWorkDataBuffer;
  dfuLong32 dfDataBufferSize;


  dfuLong32 dfNbHashedFullFile;
  REDUCEDFULLFILE *pHashedFullFile;

  dfuLong32 dfNbBlockHashFatLike;
  PBLOCKHASH_FATLIKE pbhflArray[1];

  //BLOCKHASH_FATLIKE bhfl;
} PREWKFND;

//extern unsigned long FAR crc_table[1][256];

/* ========================================================================= */
/*
#define DOLIT4 c ^= *buf4++; \
        c = crc_table[3][c & 0xff] ^ crc_table[2][(c >> 8) & 0xff] ^ \
            crc_table[1][(c >> 16) & 0xff] ^ crc_table[0][c >> 24]

*/
//#define pusecrc32_table (((const unsigned long*)(&(crc_table[0][0]))))
#define DOLIT4 c ^= *buf4++; \
        c = (u4)((pusecrc32_table[(3*0x100) | (c & 0xff)]) ^ (pusecrc32_table[(2*0x100)|((c >> 8) & 0xff)]) ^ \
                 (pusecrc32_table[(1*0x100) | ((c >> 16) & 0xff)]) ^ (pusecrc32_table[c >> 24]))

#define DOLIT16 DOLIT4; DOLIT4; DOLIT4; DOLIT4
#define DOLIT32 DOLIT4; DOLIT4; DOLIT4; DOLIT4; DOLIT4; DOLIT4; DOLIT4; DOLIT4


typedef z_crc_t u4;


/* ========================================================================= */
//local __inline unsigned long crc32_little_fast(const unsigned char  *buf, unsigned len)
// dfuLong32 GetHashFatLikeValueCrc32(const BLOCKHASH_FATLIKE * pBhfl, dfvoidpc pBuf)
dfuLong32 GetHashFatLikeValueCrc32_rmv(const BLOCKHASH_FATLIKE * pBhfl, dfvoidpc pBuf)
{
    register u4 c;
    register const u4  *buf4;
    const unsigned char  *buf = (const unsigned char  *)pBuf;
    const z_crc_t* pusecrc32_table = (pBhfl->pcrc32_table);
    dfuLong32 len;
    len = pBhfl->dfBlockCalcSize;


    c = ~(0);
    while (len && ((ptrdiff_t)buf & 3)) {
        c = (u4)(pusecrc32_table[(c ^ *buf++) & 0xff] ^ (c >> 8));
        len--;
    }

    buf4 = (const u4 FAR *)(const void  *)buf;
    while (len >= 16) {
        DOLIT16;
        len -= 16;
    }
    while (len >= 4) {
        DOLIT4;
        len -= 4;
    }
    buf = (const unsigned char  *)buf4;

    if (len) do {
        c = (u4)(pusecrc32_table[(c ^ *buf++) & 0xff] ^ (c >> 8));
    } while (--len);
    //c = ~c;

    {
        dfuLong32 dfCrc = c;
        dfCrc ^= (dfCrc >> 16);
        return dfCrc & pBhfl->dfAndOperator;
    }
}

//dfuLong32 GetHashFatLikeValueCrc32_slow(const BLOCKHASH_FATLIKE * pBhfl, dfvoidpc pBuf)
dfuLong32 GetHashFatLikeValueCrc32(const BLOCKHASH_FATLIKE * pBhfl, dfvoidpc pBuf)
{
  dfuLong32 dfCrc;
  dfCrc = crc32(0, (dfbytep) pBuf, pBhfl->dfBlockCalcSize);
  //dfCrc=crc32_little_fast((dfbytep) pBuf, pBhfl->dfBlockCalcSize);
  //dfCrc = adler32(0, (dfbytep) pBuf, pBhfl->dfBlockCalcSize);
  //dfCrc ^= adler32(0, (dfbytep) pBuf, pBhfl->dfBlockCalcSize);

  dfCrc ^= (dfCrc >> 16);
/*
  if ((pBhfl->dfAndOperator & 0xffff0000)==0)
  {
      dfuLong32 dfHigh = (dfCrc >> 16);
      //dfuLong32 dfHighInversed = ((dfHigh & 0xff)<<8) | (dfHigh >> 8);
      dfCrc = (dfCrc & 0xffff) ^ dfHigh;
  }
  else
      dfCrc ^= (dfCrc >> 16);
*/


  return dfCrc & pBhfl->dfAndOperator;
}

#ifdef USECRC16
#define CRCCCITT        0x1021          // CCITT polynomial (for XMODEM etc.)
#define CRC16           0x8005          // CRC16 polynomial
#define CRC16_REV       0xA001          // Reverse CRC16 polynomial (for DEX/UCS etc.)

dfuInt16 crc_hware(dfuInt16 data, dfuInt16 genpoly, dfuInt16 accum) {
    int i ;

    data <<= 8 ;
    for ( i = 8 ; i > 0 ; i-- )  {
        if ( (data ^ accum) & 0x8000 ) {
            accum = (accum << 1) ^ genpoly ;
        }
        else {
            accum <<= 1 ;
        }
        data <<= 1 ;
    }

    return(accum) ;
}

// "Reverse" Hardware bitshift simulation of CRC
dfuInt16 crc_revhware(dfuInt16 data, dfuInt16 genpoly, dfuInt16 accum) {
    int i ;

    data <<= 1 ;
    for ( i = 8 ; i > 0 ; i-- )  {
        data >>= 1 ;
        if ( (data ^ accum) & 0x0001 ) {
            accum = (accum >> 1) ^ genpoly ;
        }
        else {
            accum >>= 1 ;
        }
    }

    return(accum) ;
}

void make_crctbl(dfuInt16 *ptable, dfuInt16 poly, dfuInt16 (*crcfn)(dfuInt16, dfuInt16, dfuInt16)) {
    int i ;
    for ( i = 0 ; i < 256 ; i++ )
    {
        ptable[i] = (*crcfn)(i, poly, 0) ;
    }
}

void crc16_crc_init(dfuInt16 *ptable) {
    // Create the CRC table - DYLs cannot have static data
    make_crctbl(ptable, CRC16_REV, crc_revhware) ;
}

#define DOCRC16Step { nTemp = *nData++ ^ wCRCWord; wCRCWord >>= 8; wCRCWord ^= (*pCrcTable+nTemp); };
dfuLong32 GetCrc16(dfuLong32 wLength,const dfuInt16* pCrcTable, dfvoidpc pPos)
{
   dfbyte nTemp;
   dfuInt16 wCRCWord = 0xFFFF;

   dfbytepc nData=(dfbytepc) pPos;
   while (wLength>=8)
   {
       DOCRC16Step ; DOCRC16Step ; DOCRC16Step ; DOCRC16Step ;
       DOCRC16Step ; DOCRC16Step ; DOCRC16Step ; DOCRC16Step ;
       wLength-=8;
   }
   while (wLength--)
   {
      DOCRC16Step;
   }
   return wCRCWord ;
}

dfuLong32 GetHashFatLikeValueCrc16(const BLOCKHASH_FATLIKE * pBhfl, dfvoidpc pPos)
{
   return GetCrc16(pBhfl->dfBlockCalcSize,&pBhfl->Crc16Table[0],pPos) & pBhfl->dfAndOperator;
}
#endif

void ReduceHash(const REDUCEDFULLFILE *pHashedFullFile,
                dfvoidpc pData, dfuLong32 sizeCluster,dfvoidpc pHash,dfuLong32 dfSizeReduced,dfuLong32 number)
{
    dfuLong32 i;

        //crc=crc32(0,pData,sizeCluster);
        //crc=adler32(0,(const Bytef *)pData,sizeCluster)/* & 0xffffff*/;
        //crc=adler32(0,(const Bytef *)pData,sizeCluster) & 0xfffff;
        //crc=adler32(0,(const Bytef *)pData,sizeCluster) ^crc32(0,pData,sizeCluster);;
        ///++crc=adler32(0,(const Bytef *)pData,sizeCluster) ;
        //crc = GetCrc16(sizeCluster,pHashedFullFile->Crc16Table,pData);
//crc=adler32(0,(const Bytef *)pData,sizeCluster) ;
////crc =(crc & 0xffff) ^ (crc >> 16);

        if (dfSizeReduced==2)
        {
            for (i=0;i<number;i++)
            {
                dfuInt16 crc;

    /*
                dfuLong32 j;
                //crc =(crc & 0xffff) ^ ((((crc>>16) & 0xff)<<8) | (crc >> 24));
                crc=0xffff;
                for (j=0;j<sizeCluster/2;j++)
                {
                    dfuInt16 c;
                    c = *((dfuInt16*)(((dfuInt16*)pData)+(j)));
                    pData=((const Bytef *)pData)+2;
                    crc = (crc ^ c);
                }
                if ((sizeCluster & 1)!=0)
                {
                    crc = crc ^  *((((const Bytef *)pData)));
                    pData=((const Bytef *)pData)+1;
                }

*/
                crc=(dfuInt16)adler32(0,(const Bytef *)pData,sizeCluster) ;


                *((dfuInt16*)pHash)=(dfuInt16)crc;
                pData = (dfvoidpc)(((dfbytepc)pData) + sizeCluster);
                pHash = (dfvoidp)(((dfbytep)pHash) + dfSizeReduced);
            }
        }
        else
        {
            for (i=0;i<number;i++)
            {
                dfuLong32 crc;
                crc=adler32(0,(const Bytef *)pData,sizeCluster) ;
                pData = (dfvoidpc)(((dfbytepc)pData) + sizeCluster);
                pHash = (dfvoidp)(((dfbytep)pHash) + dfSizeReduced);
                *((dfuLong32*)pHash)=crc;
            }
        }

        // SIZE = 4 and align warning!


        //pData = (dfvoidpc)(((dfbytepc)pData) + sizeCluster);


}

HPREWKFND BuildPreWKFnd(ORIGDATA* pOrig, dfuLong64 sizeData,
                        dfuLong64 dfArrayLength_,dfuLong32 dfBlockCalcSize_,
                        dfuLong32 dfNbHashBit,
                        dfuLong32 dfSizeClusterReduced_)
{
  PREWKFND *pPreWkFnd = NULL;
  dfuLong32 dfAddForLatestPos = 1;
  dfuLong32 dfNbBlockHash = 1;
  BOOL fCancelAlloc = FALSE;


  #ifdef VERBOSE_BUILDEVENT
  DWORD dwBeginTime = GetTickCount();
  #endif

  if ((sizeData != 0) && (pOrig != NULL))
  {
    pPreWkFnd =
      (PREWKFND *) DfsMalloc(sizeof(PREWKFND) +
                             ((dfNbBlockHash + 1) * ((size_t)sizeof(PBLOCKHASH_FATLIKE))));

    if (pPreWkFnd != NULL)
    {
        dfuLong32 i;
        for (i=0;i<dfNbBlockHash;i++)
        {
          dfuLong32 dfNbArray = 0;
          dfuLong64 dfArrayLengthThis;

//#define INCBITTESTTABLELENGHT (4+2)
//#define INCBITTEST (4+4+(2*0)) // TESTCODE

#define INCBITTEST (0) // TESTCODE
#define INCBITTESTTABLELENGHT (0)
    dfNbHashBit+=INCBITTEST;
//// dfArrayLengthThis=dfArrayLength_ =dfArrayLength_*(1<<INCBITTESTTABLELENGHT);
//dfSizeClusterReduced_ =1*dfBlockCalcSize_/2; // TESTCODE

//dfArrayLengthThis = dfArrayLength_ =(dfuLong32)(sizeData  / 1);
//dfNbHashBit=24;
          dfArrayLengthThis=dfArrayLength_ ;
          if (dfArrayLengthThis != 0)
              dfNbArray = (dfuLong32)((sizeData + (dfArrayLengthThis - 1)) / dfArrayLengthThis);

          if (dfNbArray == 0)
          {
            dfNbArray = 1;
            dfArrayLengthThis = (dfuLong64)sizeData;
          }

          pPreWkFnd->pbhflArray[i]=(BLOCKHASH_FATLIKE*)DfsMalloc(sizeof(BLOCKHASH_FATLIKE));
          if (pPreWkFnd->pbhflArray[i] == NULL)
              break;
          else
          {
              dfuLong32 k;
              BLOCKHASH_FATLIKE* pBhfl = pPreWkFnd->pbhflArray[i];

              pBhfl->dfBlockCalcSize = dfBlockCalcSize_;
              pBhfl->dfMinSearchSize = (pBhfl->dfBlockCalcSize * 2) - 1;

              pBhfl->dfMinSearchSize = (pBhfl->dfBlockCalcSize * 1) - 0; /*+++++*/

              pBhfl->dfHashBits = dfNbHashBit;
              pBhfl->dfAndOperator = (1 << (pBhfl->dfHashBits)) - 1;
              pBhfl->dfArrayItem = 1 << (pBhfl->dfHashBits);

              pBhfl->dfNbArray = dfNbArray;

              pBhfl->dfArrayLength = dfArrayLengthThis;

              pBhfl->pLikeFatArrayBld=NULL;
              pBhfl->pSortedArray=NULL;
              for (k=ARRAYPOS_FIRSTPOS;k<=ARRAYPOS_LASTPOS;k++)
                  (pBhfl)->ArrayPosValue[k].pdfpwfPos = NULL;

              pBhfl->fncGetHashFatLikeValue = &GetHashFatLikeValueCrc32;

              pBhfl->pDualUsageBuffer = NULL;
              pBhfl->dfDualUsageBufferSize = 0;
              pBhfl->dfSortedArrayBufferSize = 0;
              pBhfl->dfArrayPosValueBufferSize = 0;
              crc32(0,(dfbytep)" ",1); // to init table
              pBhfl->pcrc32_table = get_crc_table();

#ifdef USECRC16
              crc16_crc_init(&pBhfl->Crc16Table[0]);
              pBhfl->fncGetHashFatLikeValue = &GetHashFatLikeValueCrc16;
#endif

#ifdef VERBOSE_BUILDEVENT
               printf("\n    BuildPreWKFnd %u bits,%u arrays, blocksize of %u bytes\nalloc of %u byte for pPreWkFnd\n",
                   pBhfl->dfHashBits,pBhfl->dfNbArray,pBhfl->dfBlockCalcSize,
                   sizeof(PREWKFND) +((pBhfl->dfNbArray + 1) * sizeof(ARRAYFIRSTPOS)));
#endif
          }
        }

        if (i!=dfNbBlockHash)
        {
            dfuLong32 j;
            for (j=0;j<i;j++)
                DfsFree(pPreWkFnd->pbhflArray[j]);
            DfsFree(pPreWkFnd);
            pPreWkFnd=NULL;
        }
        else
            pPreWkFnd->dfNbBlockHashFatLike = dfNbBlockHash;
    }
  }

  if (pPreWkFnd != NULL)
  {
    dfuLong32 i;
    //BLOCKHASH_FATLIKE* pBhfl = &(pPreWkFnd->bhfl);
    pPreWkFnd->sizeData = sizeData;
    pPreWkFnd->pOrg = pOrig;
//    pPreWkFnd->pData = pData;

    pPreWkFnd->dfNbHashedFullFile = 0;
    pPreWkFnd->pHashedFullFile = NULL;

    pPreWkFnd->pWorkDataBuffer = NULL;
    pPreWkFnd->dfDataBufferSize = 0;
    /*
    */


    for (i=0;i<pPreWkFnd->dfNbBlockHashFatLike;i++)
    {
        BLOCKHASH_FATLIKE* pBhfl = pPreWkFnd->pbhflArray[i];


        pBhfl->dfSortedArrayBufferSize = (dfuIntPtr)(((sizeData / pBhfl->dfBlockCalcSize) + 0x10) * sizeof(dfpwfPos));
        pBhfl->dfArrayPosValueBufferSize = (sizeof(dfpwfPos) * ((pBhfl->dfArrayItem)+1));

        pBhfl->dfDualUsageBufferSize = myMax(pBhfl->dfSortedArrayBufferSize,pBhfl->dfArrayPosValueBufferSize);

        /* First big alloc */
        pBhfl->pLikeFatArrayBld =
          (dfpwfPos *)DfsMalloc((dfuIntPtr)(((sizeData / pBhfl->dfBlockCalcSize) + 0x10) * sizeof(dfpwfPos)));
        if (pBhfl->pLikeFatArrayBld != NULL)
        {
            /* Second big alloc */
            pBhfl->pDualUsageBuffer = (dfvoidp)DfsMalloc(pBhfl->dfDualUsageBufferSize);
            pBhfl->pSortedArray = (dfpwfPos *)(pBhfl->pDualUsageBuffer);
        }
#ifdef VERBOSE_BUILDEVENT
        printf("alloc TWICE of %u byte for pLikeFatArrayBld and pSortedArray\n",
            ((sizeData / pBhfl->dfBlockCalcSize) + 1) *sizeof(dfpwfPos));
#endif
        if (pBhfl->pSortedArray == NULL)
        {
            fCancelAlloc=TRUE;
            break;
        }
    }
  }


  if (pPreWkFnd != NULL)
  {
      dfuLong32 j;
      if (!fCancelAlloc)
        for (j=0;j<pPreWkFnd->dfNbBlockHashFatLike;j++)
      {
        BLOCKHASH_FATLIKE* pBhfl = pPreWkFnd->pbhflArray[j];

        //    BLOCKHASH_FATLIKE* pBhfl = &(pPreWkFnd->bhfl);
        dfuLong32 i;

        for (i=ARRAYPOS_FIRSTPOS;i<=ARRAYPOS_LASTPOS;i++)
            (pBhfl)->ArrayPosValue[i].pdfpwfPos = NULL;

    #ifdef VERBOSE_BUILDEVENT
        printf("alloc of %u (array) * %u bytes = %u for pdfpwfPos\n",
            pBhfl->dfNbArray + dfAddForLatestPos,1 + (sizeof(dfpwfPos) * (pBhfl->dfArrayItem)),
            (pBhfl->dfNbArray + dfAddForLatestPos)*(1 + (sizeof(dfpwfPos) * (pBhfl->dfArrayItem))));
    #endif

        for (i=ARRAYPOS_FIRSTPOS;i<=ARRAYPOS_LASTPOS;i++)
        {
          dfuIntPtr dfSize;
          dfuLong32 j;
          dfSize = (sizeof(dfpwfPos) * (dfuIntPtr)((pBhfl->dfArrayItem)+1));

          if (i!=ARRAYPOS_LASTPOS)
            (pBhfl->ArrayPosValue[i]).pdfpwfPos =
                (dfpwfPos *) DfsMalloc(1 + dfSize); /* third big alloc */
          else
            (pBhfl->ArrayPosValue[i]).pdfpwfPos = (dfpwfPos *)(pBhfl->pDualUsageBuffer);
          if ((pBhfl->ArrayPosValue[i]).pdfpwfPos == NULL)
          {
            fCancelAlloc = TRUE;
            break;
          }
          else
              for (j = 0; j < pBhfl->dfArrayItem; j++)
                *(((pBhfl->ArrayPosValue[i]).pdfpwfPos) + j) = NOTHING_POS;
        }
      }

    if (fCancelAlloc)
    {
      dfuLong32 j;
      for (j=0;j<pPreWkFnd->dfNbBlockHashFatLike;j++)
      {
        dfuLong32 i;
        BLOCKHASH_FATLIKE* pBhfl = pPreWkFnd->pbhflArray[j];

        for (i=ARRAYPOS_FIRSTPOS;i<=ARRAYPOS_LASTPOS;i++)
        {
            if (i != ARRAYPOS_LASTPOS)
                if (((pBhfl->ArrayPosValue[i]).pdfpwfPos != NULL))
                {
                    DfsFree((pBhfl->ArrayPosValue[i]).pdfpwfPos);
                    (pBhfl->ArrayPosValue[i]).pdfpwfPos=NULL;
                }
        }
        if (pBhfl->pLikeFatArrayBld != NULL)
          DfsFree(pBhfl->pLikeFatArrayBld);

        if (pBhfl->pDualUsageBuffer != NULL)
          DfsFree(pBhfl->pDualUsageBuffer);


        DfsFree(pBhfl);
      }

      for (j=0;j<pPreWkFnd->dfNbHashedFullFile;j++)
          if (((pPreWkFnd->pHashedFullFile)+j)->pReducedData!=NULL)
            DfsFree(((pPreWkFnd->pHashedFullFile)+j)->pReducedData);
      if (pPreWkFnd->pHashedFullFile != NULL)
            DfsFree(pPreWkFnd->pHashedFullFile);

      if (pPreWkFnd->pWorkDataBuffer != NULL)
            DfsFree(pPreWkFnd->pWorkDataBuffer);

      DfsFree(pPreWkFnd);
      pPreWkFnd = NULL;
    }


    /* must be more secure */
    if ((dfSizeClusterReduced_ > 0) && (!fCancelAlloc))
    {
        dfuLong32 i;
        pPreWkFnd->dfNbHashedFullFile = 1;
        pPreWkFnd->pHashedFullFile = (REDUCEDFULLFILE*)DfsMalloc(sizeof(REDUCEDFULLFILE)*pPreWkFnd->dfNbHashedFullFile);
        if (pPreWkFnd->pHashedFullFile == NULL)
            pPreWkFnd->dfNbHashedFullFile = 0;
        else
        for (i=0;i<pPreWkFnd->dfNbHashedFullFile;i++)
        {
            REDUCEDFULLFILE* pCurHashedFullFile = (pPreWkFnd->pHashedFullFile)+i;
            pCurHashedFullFile->dfSizeCluster = dfSizeClusterReduced_;
            pCurHashedFullFile->dfSizeReducedCluster = 4/2;
            pCurHashedFullFile->dfReducedMask = (1 << (pCurHashedFullFile->dfSizeReducedCluster*8))-1;
            pCurHashedFullFile->dfNbCells = sizeData / (pCurHashedFullFile->dfSizeCluster);
#ifdef USECRC16
            crc16_crc_init(&pCurHashedFullFile->Crc16Table[0]);
#endif
            /* fourh big alloc */
            pCurHashedFullFile->pReducedData = (dfvoidp)DfsMalloc((dfuIntPtr)((
                    (pCurHashedFullFile)->dfSizeReducedCluster * ((pCurHashedFullFile)->dfNbCells +0x100))+0x10));
            if (pCurHashedFullFile->pReducedData == NULL)
            {
                dfuLong32 l;
                for (l=0;l<i;l++)
                {
                    DfsFree(pCurHashedFullFile->pReducedData);
                    pCurHashedFullFile->pReducedData=NULL;
                }
                DfsFree(pPreWkFnd->pHashedFullFile);
                pPreWkFnd->pHashedFullFile = NULL;
                pPreWkFnd->dfNbHashedFullFile = 0;
                break;
            }

#ifdef VERBOSE_BUILDEVENT
        printf("\n  ->alloc of %u byte for pReducedData\n",
            (pCurHashedFullFile)->dfSizeReducedCluster * ((pCurHashedFullFile)->dfNbCells +2));
#endif
        }
    }
  }

  if (pPreWkFnd != NULL)
  {
    dfuLong64 iPosData;
    dfuLong32 i, k;
    dfuLong32 incr;
    dfuLong64 iPosDataLimit;
    dfuIntPtr dfNbIncrByFileView;
    dfuIntPtr dfIncrOnFileViewPos;
    dfvoidp   pData=NULL;

    incr = 1;
    if (pPreWkFnd->dfNbBlockHashFatLike>0)
        incr = (pPreWkFnd->pbhflArray[0])->dfBlockCalcSize;


    for (k=0;k<pPreWkFnd->dfNbHashedFullFile;k++)
    {
        dfuLong32 dfCurSizeCluster = ((pPreWkFnd->pHashedFullFile)+k)->dfSizeCluster;
        if (incr > dfCurSizeCluster)
        {
            if ((incr % dfCurSizeCluster) != 0)
                incr = 1;
            else
                incr = dfCurSizeCluster;
        }
        else
        {
            if ((dfCurSizeCluster % incr) != 0)
                incr = 1;
        }
    }

    for (k=0;k<pPreWkFnd->dfNbBlockHashFatLike;k++)
    {
        dfuLong32 dfCurSizeCluster = (pPreWkFnd->pbhflArray[k])->dfBlockCalcSize;
        if (incr > dfCurSizeCluster)
        {
            if ((incr % dfCurSizeCluster) != 0)
                incr = 1;
            else
                incr = dfCurSizeCluster;
        }
        else
        {
            if ((dfCurSizeCluster % incr) != 0)
                incr = 1;
        }
    }

    iPosDataLimit = (sizeData - 1) ;
    for (k=0;k<pPreWkFnd->dfNbBlockHashFatLike;k++)
    {
        dfuLong64 iPosDataLimitThis ;
        BLOCKHASH_FATLIKE* pBhfl = pPreWkFnd->pbhflArray[k];
        iPosDataLimitThis = (sizeData - pBhfl->dfBlockCalcSize) ;
        if (iPosDataLimitThis < iPosDataLimit)
            iPosDataLimit = iPosDataLimitThis;
    }


    dfNbIncrByFileView = GetMaxOrigDataExigibleSizeView(pPreWkFnd->pOrg) / incr;
    dfIncrOnFileViewPos = dfNbIncrByFileView ;

    /* now the loop !!! */

    for (iPosData=0;iPosData <= iPosDataLimit;iPosData+=incr)
    {
        if (dfIncrOnFileViewPos == dfNbIncrByFileView)
        {
            dfIncrOnFileViewPos = 0;
            pData = GetOrigDataPtrpDataBySizeView(pPreWkFnd->pOrg,iPosData,GetMaxOrigDataExigibleSizeView(pPreWkFnd->pOrg));
        }
        dfIncrOnFileViewPos ++;

        for (k=0;k<pPreWkFnd->dfNbHashedFullFile;k++)
            if ((iPosData % ((pPreWkFnd->pHashedFullFile)+k)->dfSizeCluster)==0)
            {
                REDUCEDFULLFILE* pHashedFullFile = ((pPreWkFnd->pHashedFullFile)+k);
                dfuLong32 dfNumCell = (dfuLong32)((iPosData / pHashedFullFile->dfSizeCluster));

                dfvoidp pCellPtr = (dfvoidp)(((dfbytep)(pHashedFullFile->pReducedData)) +
                              (pHashedFullFile->dfSizeReducedCluster * ((dfuLong64)dfNumCell)));
                ReduceHash(pHashedFullFile,
                           ((dfbytepc)pData)+iPosData,pHashedFullFile->dfSizeCluster,
                           pCellPtr,pHashedFullFile->dfSizeReducedCluster,1);
            }

        for (k=0;k<pPreWkFnd->dfNbBlockHashFatLike;k++)
        {
            BLOCKHASH_FATLIKE* pBhfl = pPreWkFnd->pbhflArray[k];

            if ((iPosData % pBhfl->dfBlockCalcSize)==0)
            {
                dfuLong32 dfHash;
                dfuLong32 dfLastArrayFill;

                //dfuLong32 dfSearchChainToSearch = NOTHING_POS;

                // i contain the cell item number
                i = (dfuLong32)(iPosData / pBhfl->dfBlockCalcSize);
                *((pBhfl->pLikeFatArrayBld) + i) = NOTHING_POS;

                dfHash = DOGETHASHFATLIKEVALUE(pBhfl, ((dfbytepc) pData) + (iPosData));

                dfLastArrayFill = (dfuLong32)(iPosData / pBhfl->dfArrayLength);
                if ((*(((pBhfl->ArrayPosValue[ARRAYPOS_FIRSTPOS]).pdfpwfPos) + dfHash)) == NOTHING_POS)
                {
                    *(((pBhfl->ArrayPosValue[ARRAYPOS_FIRSTPOS]).pdfpwfPos) + dfHash) = i;
                }
                else
                {
                    dfuLong32 dfLastPos ;
                    dfLastPos = *(((pBhfl->ArrayPosValue[ARRAYPOS_LASTPOS]).pdfpwfPos) + dfHash);
                    *((pBhfl->pLikeFatArrayBld) + dfLastPos) = i;
                }
                *(((pBhfl->ArrayPosValue[ARRAYPOS_LASTPOS]).pdfpwfPos) + dfHash) = i;
            }
        }
    }


    for (k=0;k<pPreWkFnd->dfNbBlockHashFatLike;k++)
    {
        BLOCKHASH_FATLIKE* pBhfl = pPreWkFnd->pbhflArray[k];
        dfuLong32 dfCurPosNewArray;
        dfuLong32 i;
        if (dfAddForLatestPos == 1)
            {
              //DfsFree((pBhfl->ArrayPosValue[ARRAYPOS_LASTPOS]).pdfpwfPos);
              (pBhfl->ArrayPosValue[ARRAYPOS_LASTPOS]).pdfpwfPos=NULL;
              pBhfl->pDualUsageBuffer = NULL;

              if (pBhfl->dfDualUsageBufferSize != pBhfl->dfSortedArrayBufferSize)
                  pBhfl->pSortedArray = DfsRealloc(pBhfl->pSortedArray,pBhfl->dfSortedArrayBufferSize);
            }
        // think of recycle buffer between pBhfl->ArrayFirstPos[ARRAYPOS_LASTPOS] and pBhfl->pSortedArray


        *((pBhfl->pSortedArray) + 0) = 0xFFFFFFFF;
        dfCurPosNewArray = 1;
        for (i=0;i<pBhfl->dfArrayItem;i++)
        {
            dfuLong32 dfPosItemForCurrentHash;
            dfPosItemForCurrentHash = *((pBhfl->ArrayPosValue[ARRAYPOS_FIRSTPOS].pdfpwfPos)+i);
            *((pBhfl->ArrayPosValue[ARRAYPOS_FIRSTPOS].pdfpwfPos)+i) = dfCurPosNewArray;
            while (dfPosItemForCurrentHash != NOTHING_POS)
            {
                *((pBhfl->pSortedArray) + dfCurPosNewArray) = dfPosItemForCurrentHash ;
                dfCurPosNewArray++;
                dfPosItemForCurrentHash = *((pBhfl->pLikeFatArrayBld) + dfPosItemForCurrentHash) ;
            }
        }
        // the latest pos at
        *((pBhfl->ArrayPosValue[ARRAYPOS_FIRSTPOS].pdfpwfPos)+i) = dfCurPosNewArray;
        *((pBhfl->pSortedArray) + dfCurPosNewArray) = 0xffffffff;
        DfsFree(pBhfl->pLikeFatArrayBld);
        pBhfl->pLikeFatArrayBld=NULL;
    }
  }

#ifdef VERBOSE_BUILDEVENT
   dwBeginTime = GetTickCount()-dwBeginTime ;
   printf("time elapsed for BuildPreWKFnd = %u msec (%u sec)\n\n",dwBeginTime,dwBeginTime/1000);
#endif
  return (HPREWKFND) pPreWkFnd;
}

#ifdef VERBOSE_BUILDEVENT
dfuLong32 dfCountTrueCompare=0;
dfuLong32 dfCountTrueCompareOk=0;
dfuLong32 dfCountTrueCompareOkInCrcZone=0;
dfuLong32 dfCountTrueCompareFailInSkip=0;
dfuLong32 dfCountTrueCompareOkWithSkip=0;
dfuLong32 dfCountSkipNeededInTrueCompare=0;
dfuLong32 dfCountTrueCompareMinAcceptable=0;
dfuLong32 dfReduceCmp=0;
dfuLong32 dfReduceCmpOk=0;
dfuLong32 dfReduceCmpOkTrue=0;
dfuLong32 dfReduceCmpOkFalse=0;
dfuLong32 dfStepNewReturnTrue=0;
#endif
#ifdef VERIFY_BHFL_HASH
dfuLong32 dfCountBHFLHashCollide=0;
dfuLong32 dfCountBHFLHashNotCollide=0;
#endif

BOOL FreePreWKFnd(HPREWKFND hPreWkFnd)
{
  dfuLong32 i,k;
  PREWKFND *pPreWkFnd = (PREWKFND *) hPreWkFnd;

  if (pPreWkFnd == NULL)
    return FALSE;

  for (k=0;k<pPreWkFnd->dfNbBlockHashFatLike;k++)
  {
    BLOCKHASH_FATLIKE* pBhfl = pPreWkFnd->pbhflArray[k];

    for (i=ARRAYPOS_FIRSTPOS;i<=ARRAYPOS_LASTPOS;i++)
    {
        if ((pBhfl->ArrayPosValue[i]).pdfpwfPos != NULL)
          DfsFree((pBhfl->ArrayPosValue[i]).pdfpwfPos);
    }

    if (pBhfl->pLikeFatArrayBld != NULL)
      DfsFree(pBhfl->pLikeFatArrayBld);
    DfsFree(pBhfl->pSortedArray);
    DfsFree(pBhfl);
  }


  for (i=0;i<pPreWkFnd->dfNbHashedFullFile;i++)
      if (((pPreWkFnd->pHashedFullFile)+i)->pReducedData!=NULL)
        DfsFree(((pPreWkFnd->pHashedFullFile)+i)->pReducedData);
  if (pPreWkFnd->pHashedFullFile != NULL)
      DfsFree(pPreWkFnd->pHashedFullFile);

  if (pPreWkFnd->pWorkDataBuffer != NULL)
      DfsFree(pPreWkFnd->pWorkDataBuffer);

  DfsFree(pPreWkFnd);

#ifdef VERBOSE_BUILDEVENT

  #ifdef USE_OLDBHFL_RED
  printf("Using old BHFL method\n");
  #else
  printf("Using new BHFL method\n");
  #endif
  printf("True comp total=%u, ok=%u, ok with skip=%u, ok in crczone=%u,\n        fail in skip=%u, skip need=%u, CompareMinAcceptable=%u\n",
            dfCountTrueCompare,dfCountTrueCompareOk,dfCountTrueCompareOkWithSkip,
            dfCountTrueCompareOkInCrcZone,dfCountTrueCompareFailInSkip,
            dfCountSkipNeededInTrueCompare,
            dfCountTrueCompareMinAcceptable);

  if (dfCountTrueCompare>100)
      printf("->%: ok = %u %%, ok with skip = %u %%, ok in crczone= %u %%,  fail in skip=%u %%, %u %%, cmpminacc=%u %%\n",
            dfCountTrueCompareOk / (dfCountTrueCompare / 100),
            dfCountTrueCompareOkWithSkip/ (dfCountTrueCompare / 100),
            dfCountTrueCompareOkInCrcZone / (dfCountTrueCompare / 100),
            dfCountTrueCompareFailInSkip / (dfCountTrueCompare / 100),
            dfCountSkipNeededInTrueCompare/ (dfCountTrueCompare / 100),
            dfCountTrueCompareMinAcceptable/ (dfCountTrueCompare / 100));
  //if (dfReduceCmpOk>100)
  printf("\n-> Reduce %u, OK: %u, Ok True: %u, True: %u %%, ok false: %u, ret1: %u\n",
                dfReduceCmp,
                dfReduceCmpOk,
                dfReduceCmpOkTrue,
                (dfReduceCmpOk>100) ? (dfReduceCmpOkTrue / (dfReduceCmpOk / 100)):(((dfReduceCmpOkTrue+1)*100) / (dfReduceCmpOk +1)),
                dfReduceCmpOkFalse,
                dfStepNewReturnTrue);
#endif

#ifdef VERIFY_BHFL_HASH
      printf("-->Hash Fatlike: collide %u, not collide %u\n",dfCountBHFLHashCollide,dfCountBHFLHashNotCollide);
#endif

  return TRUE;
}


BOOL EnlargeInternalDataBuffer(PREWKFND *pPreWkFnd, dfuLong32 dfSizeNeeded)
{
    dfvoidp pWorkNewBuffer;
    if (dfSizeNeeded <= pPreWkFnd->dfDataBufferSize)
        return TRUE;

    if (pPreWkFnd->pWorkDataBuffer == NULL)
        pWorkNewBuffer = DfsMalloc(dfSizeNeeded);
    else
        pWorkNewBuffer = DfsRealloc(pPreWkFnd->pWorkDataBuffer,dfSizeNeeded);

    if (pWorkNewBuffer == NULL)
        return FALSE;

#ifdef _DEBUG
    if (pPreWkFnd->dfDataBufferSize==0)
        memset(pWorkNewBuffer,0xf5,dfSizeNeeded);
#endif
    pPreWkFnd->pWorkDataBuffer = pWorkNewBuffer;
    pPreWkFnd->dfDataBufferSize = dfSizeNeeded;
    return TRUE;
}

/* minimal in first */
dfuLong32 GetMinimalInterPreWkSrch(HPREWKFND hPreWkFnd)
{
  PREWKFND *pPreWkFnd = (PREWKFND *) hPreWkFnd;

  if (pPreWkFnd != NULL)
    if (pPreWkFnd->dfNbBlockHashFatLike>0)
  {
    BLOCKHASH_FATLIKE* pBhfl ;
    pBhfl = pPreWkFnd->pbhflArray[0];
    return pBhfl -> dfMinSearchSize;
  }

  return 0;
}


dfuLong32 GetSmallestReduceSize(HPREWKFND hPreWkFnd)
{
  PREWKFND *pPreWkFnd = (PREWKFND *) hPreWkFnd;

  if (pPreWkFnd != NULL)
      if (pPreWkFnd->dfNbHashedFullFile > 0)
      {
          const REDUCEDFULLFILE* pHashedFullFile ;
          pHashedFullFile = ((pPreWkFnd->pHashedFullFile)+0);
          if (pHashedFullFile != NULL)
              return pHashedFullFile ->dfSizeCluster;
      }

  return 0;
}

dfuLong32 GetSmallestBlockCalcSize(HPREWKFND hPreWkFnd)
{
  PREWKFND *pPreWkFnd = (PREWKFND *) hPreWkFnd;

  if (pPreWkFnd != NULL)
    if (pPreWkFnd->dfNbBlockHashFatLike>0)
  {
    BLOCKHASH_FATLIKE* pBhfl ;
    pBhfl = pPreWkFnd->pbhflArray[0];
    return pBhfl -> dfBlockCalcSize;
  }

  return 0;
}

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
*/
#define MAX_NBHASHEDFULLFILE (0x10)

#ifdef OLDFindSeqPreWK
BOOL FindSeqPreWK(HPREWKFND hPreWkFnd,
             dfbytep lpSeqRech, dfuLong32 dwMaxRech,
             dfbytep lpFileOrg, dfuLong64 dwSizeOrg,
             dfuLong64 dwBeginSearch, dfuLong64 dwMaxSearchOrg,
             dfuLong64 dwLatestOrg, dfuLong32 dwMinInter,
             dfuLong32 dfLengthStopSearch,
             dfuLong32 dfAlign, dfuLong32 *pdfSkipBeginSearch,
             dfuLong64 * posOrg, dfuLong32 * dwSizeOrgRcp,
             BOOL * pfIsMinEnough)
{
  PREWKFND *pPreWkFnd = (PREWKFND *) hPreWkFnd;
  dfuLong32 i;
  dfuLong64 dfArrayPos;
  dfuLong32 dwSizeOrgBetter, posOrgBetter;
  BOOL fFound;
  dfvoidpc pData = pPreWkFnd->pData;
  const BLOCKHASH_FATLIKE* pBhfl ;
  dfuLong32 dfBlockCalcSize ;
  dfuLong32 dwMaxRechHash;
  dfuLong32 dfNbCellCmp[MAX_NBHASHEDFULLFILE];
  BOOL fNbCellHashCmpRecalc;
  BOOL fReduceSearchString=FALSE;
  dfuLong32 dfMinSizeCluster,dfMaxSizeCluster,dfMaxSizeReducedStringSearch,dfMaxSizeReducedCluster;
  dfuLong32 dfSkipBeginSearch;
  BOOL fCanSkipBeginSearch;
//  dfuLong32 dwMaxRechAroundLower4 = dwMaxRech & 0xfffffffc;

  fCanSkipBeginSearch = FALSE;
  if (pdfSkipBeginSearch != NULL)
      fCanSkipBeginSearch = TRUE;

  if (pPreWkFnd == NULL)
    return FALSE;

  pBhfl = pPreWkFnd->pbhflArray[0];
  dfBlockCalcSize = pBhfl->dfBlockCalcSize;


  if (dfAlign>1)
  {
      dfuLong32 dfModulo = (dfuLong32)(dwBeginSearch % dfAlign);
      if (dfModulo>0)
          dwBeginSearch += (dfAlign-dfModulo);
  }

  if (pfIsMinEnough != NULL)
    *pfIsMinEnough = (dwMinInter >= pBhfl->dfMinSearchSize);


  if (dwMaxSearchOrg == 0)
    dwMaxSearchOrg = dwSizeOrg;

  if (dwMinInter < pBhfl->dfMinSearchSize)
  {
    return FALSE;
  }

  dwSizeOrgBetter = 0;
  posOrgBetter = 0;
  dfSkipBeginSearch = 0;
  fFound = FALSE;
  dfMaxSizeReducedStringSearch = 0;

  fNbCellHashCmpRecalc = TRUE;

  if (pPreWkFnd->dfNbHashedFullFile > 0)
     //if ((dwMaxSearchOrg - dwBeginSearch) >= (0x1000))
  {
      dfuLong32 k,l;
      dfMinSizeCluster = ((pPreWkFnd->pHashedFullFile)+0)->dfSizeCluster;
      dfMaxSizeCluster = ((pPreWkFnd->pHashedFullFile)+0)->dfSizeCluster;
      dfMaxSizeReducedCluster = ((pPreWkFnd->pHashedFullFile)+0) ->dfSizeReducedCluster ;
      for (k = 1;k < pPreWkFnd->dfNbHashedFullFile;k++)
      {
          dfMinSizeCluster = myMin(dfMinSizeCluster,((pPreWkFnd->pHashedFullFile)+k)->dfSizeCluster);
          dfMaxSizeCluster = myMax(dfMinSizeCluster,((pPreWkFnd->pHashedFullFile)+k)->dfSizeCluster);
          dfMaxSizeReducedCluster = myMax(dfMaxSizeReducedCluster,((pPreWkFnd->pHashedFullFile)+k) ->dfSizeReducedCluster);
      }

#define MAXRECHHASH (0x300)
      dwMaxRechHash = myMin(dwMaxRech,MAXRECHHASH);


      dfMaxSizeReducedStringSearch = ((dwMaxRechHash / dfMinSizeCluster) + 1) * dfMaxSizeReducedCluster;
      dfMaxSizeReducedStringSearch = PWFAroundUpper(dfMaxSizeReducedStringSearch,8)+8;

      fReduceSearchString = EnlargeInternalDataBuffer(pPreWkFnd,
                      (pPreWkFnd->dfNbHashedFullFile) * dfMaxSizeCluster*dfMaxSizeReducedStringSearch);

      if (fReduceSearchString)
      for (k = 0;k < pPreWkFnd->dfNbHashedFullFile;k++)
      {
          const REDUCEDFULLFILE* pHashedFullFile = ((pPreWkFnd->pHashedFullFile)+k);
          for (l=0;(l<pHashedFullFile->dfSizeCluster) && (l < dwMaxRechHash);l++)
          {
              dfuLong32 dfNbCellHash = 0;
              dfvoidp pWorkDataCurrentShiftedReduction ;
              pWorkDataCurrentShiftedReduction = (((dfbytep)pPreWkFnd->pWorkDataBuffer) +
                      (((k * dfMaxSizeCluster)  + l) * dfMaxSizeReducedStringSearch));

              if (l < dwMaxRechHash)
                  dfNbCellHash = ((dwMaxRechHash - l) / (pHashedFullFile->dfSizeCluster));
#if defined(_DEBUG) && defined(VERBOSE_BUILDEVENT)
              if ( (((k * dfMaxSizeCluster)  + l) * dfMaxSizeReducedStringSearch) > pPreWkFnd->dfDataBufferSize)
                  printf("over1\n");
              if (((((k * dfMaxSizeCluster)  + l) * dfMaxSizeReducedStringSearch) +(dfNbCellHash*pHashedFullFile->dfSizeReducedCluster)) > pPreWkFnd->dfDataBufferSize)
                  printf("over2\n");
#endif

              ReduceHash(pHashedFullFile,
                         ((dfbytepc)lpSeqRech)+l,pHashedFullFile->dfSizeCluster,
                         pWorkDataCurrentShiftedReduction,pHashedFullFile->dfSizeReducedCluster,dfNbCellHash);
          }
      }
  }

  /* now searching */
  dfArrayPos = dwBeginSearch / pBhfl->dfArrayLength;
  for (i = 0; i < dfBlockCalcSize - 0; i += dfAlign)
  {
    dfuLong32 dfChainPos, dfHashSearch;
    //dfHashSearch = (pBhfl->fncGetHashFatLikeValue)(pBhfl, lpSeqRech + i);
    dfHashSearch = DOGETHASHFATLIKEVALUE(pBhfl, lpSeqRech + i);
    dfChainPos =
      *(((pBhfl->ArrayFirstPos[dfArrayPos]).pdfpwfPos) + dfHashSearch);
    while (dfChainPos != NOTHING_POS)
    {
      /* */
      if ((dfChainPos * (dfBlockCalcSize) >= dwBeginSearch + i))
      {
        dfuLong32 dfPosTest;
        dfuLong32 j,k;
        BOOL fBreak=FALSE;
        dfuLong32 dfNeedSkipMin=0;

        if (dfChainPos * (dfBlockCalcSize) + dwMinInter >= dwMaxSearchOrg + i)
        {
          break;
        }

        dfPosTest = dfChainPos * (dfBlockCalcSize) - i;



        // here , verify on hash
        if (fReduceSearchString)
        {
            if (fNbCellHashCmpRecalc)
            {
              dfuLong32 dfMinimalLengthEqual;
              dfMinimalLengthEqual = myMin(myMax(dwMinInter,dwSizeOrgBetter),dwMaxRechHash);

              for (k = 0;k < pPreWkFnd->dfNbHashedFullFile;k++)
              {
                REDUCEDFULLFILE* pHashedFullFile = ((pPreWkFnd->pHashedFullFile)+k);
                dfNbCellCmp[k] = (dfMinimalLengthEqual - (pHashedFullFile ->dfSizeCluster - 1)) /
                      (pHashedFullFile ->dfSizeCluster) ;
              }
              fNbCellHashCmpRecalc = FALSE;
            }

            for (k = 0;k < pPreWkFnd->dfNbHashedFullFile;k++)
            {
                REDUCEDFULLFILE* pHashedFullFile = ((pPreWkFnd->pHashedFullFile)+k);
                dfuLong32 dfSizeCluster=pHashedFullFile->dfSizeCluster;
                dfuLong32 dfModulo ;
                dfuLong32 dfNeedSkip ;
                dfuLong32 dfNbCellReduceCompare;
                //dfuLong32 dfMinimalLengthEqual;

                dfModulo = (dfuLong32)(dfPosTest % dfSizeCluster);
                if (dfModulo > 0)
                    dfNeedSkip = dfSizeCluster - dfModulo;
                else
                    dfNeedSkip = 0;

                if (k==0)
                    dfNeedSkipMin = dfNeedSkip;
                else
                    dfNeedSkipMin = myMin(dfNeedSkipMin,dfNeedSkip);

                dfNbCellReduceCompare = dfNbCellCmp[k];
                // now calculate hash
                {
                    dfvoidp pWorkDataCurrentShiftedReduction ;
                    dfvoidp pSearchStringShiftedReduced;
                    dfuLong32* pWorkDataCurrentShiftedReductionBrowse ;
                    dfuLong32* pSearchStringShiftedReducedBrowse;
                    dfuLong32 l;
                    pWorkDataCurrentShiftedReduction = (((dfbytep)pPreWkFnd->pWorkDataBuffer) +
                        (((k * dfMaxSizeCluster)  + dfNeedSkip) * dfMaxSizeReducedStringSearch));
                    pSearchStringShiftedReduced = ((dfbytep)pHashedFullFile->pReducedData) +
                                (((dfPosTest+dfNeedSkip) / pHashedFullFile->dfSizeCluster) * pHashedFullFile->dfSizeReducedCluster);

                    pWorkDataCurrentShiftedReductionBrowse = (dfuLong32*)pWorkDataCurrentShiftedReduction;
                    pSearchStringShiftedReducedBrowse = (dfuLong32*)pSearchStringShiftedReduced;


    #if defined(_DEBUG) && defined(NO_CONFIDENT_REDUCEHASH)
                    {
                    dfbyte dfHash[SIZEHASHPROVBUF];
                    memset(dfHash,0x66,SIZEHASHPROVBUF);
                    ReduceHash(pHashedFullFile,
                               ((dfbytepc)lpSeqRech)+dfNeedSkip,pHashedFullFile->dfSizeCluster,
                               dfHash,pHashedFullFile->dfSizeReducedCluster,dfNbCellReduceCompare);

                        if (DfsMemcmp(dfHash,
                                    pWorkDataCurrentShiftedReduction,
                                    dfNbCellReduceCompare * pHashedFullFile->dfSizeReducedCluster) != 0)
                        {
                            printf("BAD HASH PROPOSITION\n");
                        }
                    }
    #endif


                    for (l=0;l<dfNbCellReduceCompare;l++)
                    {
                        dfuLong32 a,b;
                        a = (*pWorkDataCurrentShiftedReductionBrowse) & (pHashedFullFile->dfReducedMask);
                        b = (*pSearchStringShiftedReducedBrowse) & (pHashedFullFile->dfReducedMask);
                        if (a!=b)
                        {
                            fBreak=TRUE;
                            break;
                        }
                        pSearchStringShiftedReducedBrowse = (dfuLong32*)
                            (((dfbytep)pSearchStringShiftedReducedBrowse)+pHashedFullFile->dfSizeReducedCluster);
                        pWorkDataCurrentShiftedReductionBrowse = (dfuLong32*)
                            (((dfbytep)pWorkDataCurrentShiftedReductionBrowse)+pHashedFullFile->dfSizeReducedCluster);
                    }

                    if (fBreak)
                        break;
                }
#ifdef VERBOSE_BUILDEVENT
                {
                    dfuLong32 l;
                    for (l=0;l<dfNeedSkip;l++)
                        if ((*(((dfbytepc) pData) + dfPosTest + l)) != (*(lpSeqRech + l)))
                        {
                            dfCountTrueCompareFailInSkip++;
                            break;
                        }
                }
#endif
            }
        }






        if (fBreak)
            j=0;
        else
        {
            dfuLong32 dfSkipBeginRech = 0;

/*
            for (j = 0; j < dwMaxRech; j++)
            {
            if ((*(((dfbytepc) pData) + dfPosTest + j)) != (*(lpSeqRech + j)))
                break;
            }
            */
            if (fCanSkipBeginSearch)
            {
                dfsLong32 m;
                for (m=((dfsLong32)dfNeedSkipMin)-1;m>=0;m--)
                {
                    if ((*(((dfbytepc) pData) + dfPosTest + m)) != (*(lpSeqRech + m)))
                        break;
                }
                dfSkipBeginRech = (dfuLong32)(m+1);
#ifdef VERBOSE_BUILDEVENT
                if (dfSkipBeginRech>0)
                  dfCountSkipNeededInTrueCompare++;

                if (dfSkipBeginRech + dwMaxRech < dwMinInter)
                    printf("skip too big\n");
#endif
            }

            {
                const dfuLong32 UNALIGNED * pDataBrowse32 = (const dfuLong32*)(((dfbytepc) (pData)) + dfPosTest + dfSkipBeginRech);
                const dfuLong32 UNALIGNED * pSeqRechBrowse32= (const dfuLong32*)(lpSeqRech+dfSkipBeginRech);

                j=0;

                {
                    dfuLong32 dwMaxRechAroundLower4 ;
                    dfuLong32 dwMaxRechThis ;
                    if (dfPosTest + dwMaxRech > dwMaxSearchOrg)
                        dwMaxRechThis = (dfuLong32)(dwMaxSearchOrg-dfPosTest);
                    else
                        dwMaxRechThis=dwMaxRech;
                    dwMaxRechThis -= dfSkipBeginRech;
                    dwMaxRechAroundLower4 = dwMaxRechThis & 0xfffffffc;

                    for (;;)
                    {
                        if ((*pSeqRechBrowse32) != (*pDataBrowse32))
                        {
                            dfbytepc pSeqRechBrowse = (dfbytepc)pSeqRechBrowse32;
                            dfbytepc pDataBrowse = (dfbytepc)pDataBrowse32;
                            if ((*(pSeqRechBrowse)) == (*(pDataBrowse)))
                            {
                                j++;
                                if ((*(pSeqRechBrowse+1)) == (*(pDataBrowse+1)))
                                {
                                    j++;
                                    if ((*(pSeqRechBrowse+2)) == (*(pDataBrowse+2)))
                                        j++;
                                }
                            }
                            break;
                        }
                        pSeqRechBrowse32++;
                        pDataBrowse32++;
                        j+=4;

                        if (j>=dwMaxRechAroundLower4)
                        {
                            dfbytepc pSeqRechBrowse = (dfbytepc)pSeqRechBrowse32;
                            dfbytepc pDataBrowse = (dfbytepc)pDataBrowse32;
                            if (j<dwMaxRechThis)
                            if (((*(pSeqRechBrowse)) == (*(pDataBrowse))))
                            {
                                j++;
                                if (j<dwMaxRechThis)
                                if (((*(pSeqRechBrowse+1)) == (*(pDataBrowse+1))))
                                {
                                    j++;
                                    if (j<dwMaxRechThis)
                                    if (((*(pSeqRechBrowse+2)) == (*(pDataBrowse+2))))
                                        j++;
                                }
                            }
                            break;
                        }
                    }
                }
            }

#ifdef VERBOSE_BUILDEVENT
            if (pPreWkFnd->dfNbHashedFullFile >= 1)
              if (j<(dfNbCellCmp[0] * (((pPreWkFnd->pHashedFullFile)+0)->dfSizeCluster)))
                  dfCountTrueCompareOkInCrcZone++;
            if (j >= dwMinInter)
                dfCountTrueCompareMinAcceptable++;
#endif
            if ((j >= dwMinInter) && (j >= dwSizeOrgBetter))
            {
                //if ((j > dwSizeOrgBetter) || (posOrgBetter < dwLatestOrg))
                if ((j > dwSizeOrgBetter) ||
                    ((posOrgBetter < dwLatestOrg) && (dfPosTest > posOrgBetter)) ||
                    (dfPosTest >= dwLatestOrg) && (posOrgBetter > dfPosTest))
                {
                    dwSizeOrgBetter = j;
                    posOrgBetter = dfPosTest;
                    fFound = TRUE;
                    fNbCellHashCmpRecalc=TRUE;
                    dfSkipBeginSearch = dfSkipBeginRech;

#ifdef VERBOSE_BUILDEVENT
                    dfCountTrueCompareOk++;
                    if (dfSkipBeginRech > 0)
                        dfCountTrueCompareOkWithSkip++;
#endif
                }
                /*dwMaxBeginString = dwMaxSearchOrg - dwSizeMinFind; */
            }
#ifdef VERBOSE_BUILDEVENT
            dfCountTrueCompare++;
#endif
        }
      }
      dfChainPos = *((pBhfl->pLikeFatArray) + dfChainPos);
    }
  }

  if (fFound)
  {

    *posOrg = posOrgBetter;
    *dwSizeOrgRcp = dwSizeOrgBetter;
  }


  if (!(fFound && (dwSizeOrgBetter > dwMinInter)))
      return FALSE;

  if (pdfSkipBeginSearch!=NULL)
        *pdfSkipBeginSearch = dfSkipBeginSearch;

#ifdef _DEBUG
  {
      dfuLong32 i;
      for (i=0;i<dwSizeOrgBetter;i++)
      {
          if ((*(((dfbytepc) pData) + posOrgBetter + i + dfSkipBeginSearch)) != (*(lpSeqRech + i + dfSkipBeginSearch)))
          {
              printf("Error in cmp\n");
              break;
          }
          if (dwSizeOrgBetter+dfSkipBeginSearch >dwMaxRech)
              printf("after bound srch\n");
          if (posOrgBetter+dfSkipBeginSearch+dwSizeOrgBetter > dwMaxSearchOrg)
              printf("after bound org\n");
      }
  }
#endif

  return TRUE;
}
#endif

#ifdef USE_OLDBHFL_RED


typedef struct
{
    dfuLong32 dfHashValue;
} CACHE_HASH_INFO;

BOOL NewFindSeqPreWKStep(HPREWKFND hPreWkFnd,
             dfbytep lpSeqRech, dfuLong32 dwMaxRech,
             dfbytep lpFileOrg,
             //dfuLong64 dwBeginSearch, dfuLong64 dwMaxSearchOrg,
             dfuLong64 dfChainPosMinAcceptable,dfuLong64 dfChainPosMaxAcceptable,
             dfuLong32 dwMinInter,
             dfuLong32 dfAlign, dfuLong32 *pdfSkipBeginSearch,
             dfuLong64 * posOrg, dfuLong32 * dwSizeOrgRcp,


             const REDUCEDFULLFILE* pHashedFullFile,
             dfbytepc pBufForReduceHashCache,
             dfuLong32 dfMaxSizeReducedStringSearch,
             dfuLong32* pReducedHashCacheSize,

             const BLOCKHASH_FATLIKE* pBhfl,
             const CACHE_HASH_INFO* pBufCacheHashFatLikeSearch,

             BOOL fChooseFirst,BOOL fChooseLast,BOOL fDirectionNeg
#ifdef CHECK_MAXORG_FIND
             ,dfuLong64 dwMaxSearchOrg
#endif
             )
{
  dfuLong32 i;
  dfuLong32 dfBlockCalcSize = pBhfl->dfBlockCalcSize;
  dfuLong64 dfArrayPos;
  //dfuLong32 dfNbCellCmp;
  dfuLong32 dwSizeOrgBetter = 0;
  dfuLong64 posOrgBetter = 0;
  dfuLong32 dfSkipBeginSearch;
  BOOL fCanSkipBeginSearch ;
  PREWKFND *pPreWkFnd = (PREWKFND *) hPreWkFnd;
//  dfvoidpc pData = pPreWkFnd->pData;
  ORIGDATA* pOrg = pPreWkFnd->pOrg;
  BOOL fFound = FALSE;
  dfuLong32 dfNbCellReduceCompare,dfNbByteReduceCompare;

  //dfuLong32 dfChainPosMinAcceptable,dfChainPosMaxAcceptable;
//pHashedFullFile=NULL;
  fCanSkipBeginSearch = FALSE;
  if (pdfSkipBeginSearch != NULL)
      fCanSkipBeginSearch = TRUE;

#ifdef CHECK_MAXORG_FIND
  if ((dfBlockCalcSize + dwMinInter) > dwMaxSearchOrg)
  {
      printf("*1\n");
      return FALSE;
  }
#endif

  /*
  dfChainPosMinAcceptable = (dwBeginSearch+dfBlockCalcSize) / dfBlockCalcSize;
  dfChainPosMaxAcceptable = (dwMaxSearchOrg - (dfBlockCalcSize + dwMinInter)) / dfBlockCalcSize;
*/

#if defined(_DEBUG) && defined(VERBOSE_BUILDEVENT)
//  { char sz[200]; wsprintf(sz,"search (%u , %u)\n",(dfuLong32)dfChainPosMinAcceptable,(dfuLong32)dfChainPosMaxAcceptable); OutputDebugString(sz);}
#endif

  dfArrayPos = dfChainPosMinAcceptable / (pBhfl->dfArrayLength/pBhfl->dfBlockCalcSize);

  if (dfArrayPos >= pBhfl->dfNbArray)
      dfArrayPos = pBhfl->dfNbArray -1 ;
  if (pHashedFullFile != NULL)
  {
      dfNbCellReduceCompare = dfBlockCalcSize / pHashedFullFile->dfSizeCluster;
      dfNbByteReduceCompare = dfNbCellReduceCompare * pHashedFullFile->dfSizeReducedCluster;
  }

  for (i = 0; i < dfBlockCalcSize - 0; i += dfAlign)
  {
    dfuLong32 dfChainPos;
    dfuLong32 dfHashSearch = (pBufCacheHashFatLikeSearch+i)->dfHashValue;
    dfvoidpc pWorkDataCurrentShiftedReduction ;
    dfuLong32 dfFirstItemPossibleCurrentHashValue;
    dfuLong32 dfLastItemPossibleCurrentHashValue;
    dfuLong32 dfItemSortedArrayBrowse,dfItemSortedArrayLimit;
    signed int iItemSortedArrayIncr;
    dfuLong32 dfStepDichFirst ;

    dfFirstItemPossibleCurrentHashValue =
        *((pBhfl->ArrayPosValue[ARRAYPOS_FIRSTPOS].pdfpwfPos)+dfHashSearch) ;
    dfLastItemPossibleCurrentHashValue =
        *((pBhfl->ArrayPosValue[ARRAYPOS_FIRSTPOS].pdfpwfPos)+dfHashSearch + 1) ;
    if (dfLastItemPossibleCurrentHashValue == dfFirstItemPossibleCurrentHashValue)
        continue;

    if (pBufForReduceHashCache != NULL)
        pWorkDataCurrentShiftedReduction = (dfvoidpc)(((pBufForReduceHashCache) +
                                (i * dfMaxSizeReducedStringSearch)));
#ifdef VERIFY_BHFL_HASH
    BOOL fCmpVerifyHash;
    if (GetHashFatLikeValueCrc32(pBhfl,lpSeqRech+i) != dfHashSearch)
        printf("|");
#endif


    dfStepDichFirst = 1;
    while (dfStepDichFirst < ((dfLastItemPossibleCurrentHashValue)-dfFirstItemPossibleCurrentHashValue))
        dfStepDichFirst = dfStepDichFirst << 1;

    while (dfStepDichFirst>0)
    {
        if ((dfFirstItemPossibleCurrentHashValue+dfStepDichFirst) <= dfLastItemPossibleCurrentHashValue)
        {
            dfuLong32 dfChainPosTest = *((pBhfl->pSortedArray) + dfFirstItemPossibleCurrentHashValue + dfStepDichFirst-1);
            if (dfChainPosTest < dfChainPosMinAcceptable)
            {
                dfFirstItemPossibleCurrentHashValue += dfStepDichFirst;
            }
        }
        dfStepDichFirst = dfStepDichFirst >> 1;
    }


#if defined(_DEBUG)
    { // Verify first dichotomia
        dfuLong32 dfItemSortedArrayTryNoDichos = *((pBhfl->ArrayPosValue[ARRAYPOS_FIRSTPOS].pdfpwfPos)+dfHashSearch) ;
        dfuLong32 dfChainPosTry = *((pBhfl->pSortedArray) + dfItemSortedArrayTryNoDichos);

        while ((dfChainPosTry < dfChainPosMinAcceptable) && (dfItemSortedArrayTryNoDichos < dfLastItemPossibleCurrentHashValue))
        {
            dfItemSortedArrayTryNoDichos++;
            dfChainPosTry = *((pBhfl->pSortedArray) + dfItemSortedArrayTryNoDichos);
        }
        if ((dfItemSortedArrayTryNoDichos != dfFirstItemPossibleCurrentHashValue) ||
            (dfChainPosTry != (*((pBhfl->pSortedArray) + dfFirstItemPossibleCurrentHashValue))))
            printf("error first dicos%%\n");

    }
#endif

    dfChainPos = *((pBhfl->pSortedArray) + dfFirstItemPossibleCurrentHashValue);
    if ((dfChainPos >= dfChainPosMaxAcceptable) ||
        (dfFirstItemPossibleCurrentHashValue  == dfLastItemPossibleCurrentHashValue))
        continue;


    {
        dfuLong32 dfStepDichLast;

        dfStepDichLast = 1;
        while (dfStepDichLast < ((dfLastItemPossibleCurrentHashValue-1)-dfFirstItemPossibleCurrentHashValue))
            dfStepDichLast = dfStepDichLast << 1;
        while (dfStepDichLast > 0)
        {
            if (dfLastItemPossibleCurrentHashValue >= dfFirstItemPossibleCurrentHashValue + dfStepDichLast)
                if ((*((pBhfl->pSortedArray) + dfLastItemPossibleCurrentHashValue - dfStepDichLast)) >= dfChainPosMaxAcceptable)
                dfLastItemPossibleCurrentHashValue -= dfStepDichLast;
            dfStepDichLast = dfStepDichLast >> 1;
        }
    }

#if defined(_DEBUG)
    {
        dfuLong32 dfLastItemTryDichos;
        dfLastItemTryDichos=*((pBhfl->ArrayPosValue[ARRAYPOS_FIRSTPOS].pdfpwfPos)+dfHashSearch + 1);
        while ((*((pBhfl->pSortedArray) + dfLastItemTryDichos - 1)) >= dfChainPosMaxAcceptable)
            dfLastItemTryDichos--;
        if  (dfLastItemPossibleCurrentHashValue != dfLastItemTryDichos)
          printf("error last dichos\n");
    }
#endif

#if defined(_DEBUG)
    if (dfFirstItemPossibleCurrentHashValue == dfLastItemPossibleCurrentHashValue)
    {
        printf(".");
    }
#endif

    if (!fDirectionNeg)
    //if (!fChooseLast)
    //if (1)
    {
        dfItemSortedArrayBrowse = dfFirstItemPossibleCurrentHashValue;
        dfItemSortedArrayLimit=dfLastItemPossibleCurrentHashValue;
        iItemSortedArrayIncr=1;
    }
    else
    {
        dfItemSortedArrayBrowse = dfLastItemPossibleCurrentHashValue-1;
        dfItemSortedArrayLimit = dfFirstItemPossibleCurrentHashValue-1;
        iItemSortedArrayIncr=-1;
    }

    //while ((dfItemSortedArrayBrowse < dfLastItemPossibleCurrentHashValue) && (dfChainPos < dfChainPosMaxAcceptable))
    for (;;)
    {
      {
        dfuLong64 dfPosTest;
        dfuLong64 dfPosTestWithMinimalSkipforHash;
        dfuLong32 j;
        BOOL fBreak = FALSE;
        dfuLong32 dfNeedSkip = i;

        dfPosTestWithMinimalSkipforHash = (dfChainPos * ((dfuLong64)dfBlockCalcSize)) ;
        dfPosTest = (dfChainPos * ((dfuLong64)dfBlockCalcSize)) - i;
        //dfPosTest = (dfChainPos * ((dfuLong64)dfBlockCalcSize)) + i; // NEW TEST

        /*
        if (i!=0)
            dfNeedSkip = dfBlockCalcSize - i;
*/

#ifdef VERIFY_BHFL_HASH
    if (GetHashFatLikeValueCrc32(pBhfl,((dfbytepc)pData)+ (dfChainPos*(dfuLong64)dfBlockCalcSize)) != dfHashSearch)
        printf("-");
    fCmpVerifyHash = DfsMemcmp(((dfbytepc)pData)+ (dfChainPos*(dfuLong64)dfBlockCalcSize),lpSeqRech+i,pBhfl->dfBlockCalcSize) == 0;
    if (!fCmpVerifyHash)
        dfCountBHFLHashCollide++;
    else
        dfCountBHFLHashNotCollide++;
#endif

/*if ((dfPosTest % dfAlign) != 0)
printf("#");*/
        // here , verify on hash

#define IGNORE_HASHED_FULL_FILE_
#ifndef IGNORE_HASHED_FULL_FILE
        if ((pHashedFullFile!=NULL))
        {
            //dfNbCellCmp = dfNbCellReduceCmp;

            //if (pHashedFullFile != NULL)
            {
                dfuLong32 dfSizeCluster=pHashedFullFile->dfSizeCluster;


                // now calculate hash
                {
                    //dfvoidpc pWorkDataCurrentShiftedReduction ;
                    dfvoidpc pSearchStringShiftedReduced;
                    dfuLong32 dfDataCurrentShiftedReductionSize = *(pReducedHashCacheSize+dfNeedSkip);

                    //dfNbCellReduceCompare = dfDataCurrentShiftedReductionSize ;

                    //dfNbCellReduceCompare = dfBlockCalcSize / pHashedFullFile->dfSizeCluster;
/*
if ((dfNeedSkip % dfAlign) != 0)
printf("$");
*/
#ifdef CHECK_MAXORG_FIND
                    if (dfNeedSkip + dfmax(dfNbCellReduceCompare*pHashedFullFile->dfSizeCluster,dwMaxRech) + dfPosTest >= dwMaxSearchOrg)
                    {
                        printf("*2\n");
                        break;
                    }
#endif
/* */
                    /*
                    pWorkDataCurrentShiftedReduction = (dfvoidpc)(((pBufForReduceHashCache) +
                            (dfNeedSkip * dfMaxSizeReducedStringSearch)));*/
                    //pSearchStringShiftedReduced = ((dfbytep)pHashedFullFile->pReducedData) +
                    //            (((dfPosTestWithMinimalSkipforHash) / pHashedFullFile->dfSizeCluster) * pHashedFullFile->dfSizeReducedCluster);
                    pSearchStringShiftedReduced = (((dfbytep)pHashedFullFile->pReducedData) +
                                                (dfNbByteReduceCompare*dfChainPos));


#define NO_CONFIDENT_REDUCEHASH_no
    #if defined(_DEBUG) && defined(NO_CONFIDENT_REDUCEHASH)
                    {
                    #define SIZEHASHPROVBUF 0x400
                    dfbyte dfHash[SIZEHASHPROVBUF];
                    dfuLong32 dfNbCellReduceCompareVerifyHash = dfNbCellReduceCompare;
                    memset(dfHash,0x66,SIZEHASHPROVBUF);
                    //dfReduceCmp++;

                    ReduceHash(pHashedFullFile,
                               ((dfbytepc)lpSeqRech)+dfNeedSkip,pHashedFullFile->dfSizeCluster,
                               dfHash,pHashedFullFile->dfSizeReducedCluster,dfNbCellReduceCompareVerifyHash);
                    if (DfsMemcmp(dfHash,
                                pWorkDataCurrentShiftedReduction,
                                dfNbCellReduceCompareVerifyHash * pHashedFullFile->dfSizeReducedCluster) != 0)
                        printf("BAD HASH PROPOSITION for seq rech skip=%u\n",dfNeedSkip);

                    ReduceHash(pHashedFullFile,
                               ((dfbytepc)pData)+dfPosTest+dfNeedSkip,pHashedFullFile->dfSizeCluster,
                               dfHash,pHashedFullFile->dfSizeReducedCluster,dfNbCellReduceCompareVerifyHash);
                    if (DfsMemcmp(dfHash,
                                pSearchStringShiftedReducedBrowse,
                                dfNbCellReduceCompareVerifyHash * pHashedFullFile->dfSizeReducedCluster) != 0)
                        printf("BAD HASH PROPOSITION for pData\n");

                    }
    #endif

                    if (dfNbByteReduceCompare==4)
                    {
                            dfuLong32 a,b;
                            a = *((dfuLong32*)pWorkDataCurrentShiftedReduction);
                            b = *((dfuLong32*)pSearchStringShiftedReduced);
                            fBreak = a != b;
                    }
                    else if (dfNbByteReduceCompare==2)
                    {
                            dfuInt16 a,b;
                            a = *((dfuInt16*)pWorkDataCurrentShiftedReduction);
                            b = *((dfuInt16*)pSearchStringShiftedReduced);
                            fBreak = a != b;
                    }
                    else
                    #define USE_MEMCMP
                    #ifndef USE_MEMCMP
                    {
                        dfuLong32 l;

                        dfuLong32* pWorkDataCurrentShiftedReductionBrowse ;
                        dfuLong32* pSearchStringShiftedReducedBrowse;
                        pWorkDataCurrentShiftedReductionBrowse = (dfuLong32*)pWorkDataCurrentShiftedReduction;
                        pSearchStringShiftedReducedBrowse = (dfuLong32*)pSearchStringShiftedReduced;

                        for (l=0;l<dfNbCellReduceCompare;l++)
                        {
                            dfuLong32 a,b;
                            a = (*pWorkDataCurrentShiftedReductionBrowse) & (pHashedFullFile->dfReducedMask);
                            b = (*pSearchStringShiftedReducedBrowse) & (pHashedFullFile->dfReducedMask);
                            if (a!=b)
                            {
                                fBreak=TRUE;
                                break;
                            }
                            pSearchStringShiftedReducedBrowse = (dfuLong32*)
                                (((dfbytep)pSearchStringShiftedReducedBrowse)+pHashedFullFile->dfSizeReducedCluster);
                            pWorkDataCurrentShiftedReductionBrowse = (dfuLong32*)
                                (((dfbytep)pWorkDataCurrentShiftedReductionBrowse)+pHashedFullFile->dfSizeReducedCluster);
                        }
                    }
                    #else
                    {
                    fBreak = DfsMemcmp(pWorkDataCurrentShiftedReduction,pSearchStringShiftedReduced,
                                        pHashedFullFile->dfSizeReducedCluster * dfNbCellReduceCompare) != 0;
                    }
                    #endif

#ifdef VERIFY_BHFL_HASH
                    /*
                    if ((fBreak) && (fCmpVerifyHash))
                        printf(".");
                        */
                    if ((fBreak) && (DfsMemcmp(
                                        ((dfbytepc)pData)+dfPosTest+dfNeedSkip,
                                        ((dfbytepc)lpSeqRech)+dfNeedSkip,
                                        pBhfl->dfBlockCalcSize) == 0))
                                        printf("!");


#endif

#ifdef VERBOSE_BUILDEVENT
                    dfReduceCmp++;
                    if (!fBreak)
                    {
                        dfvoidp pData;
                        pData = GetOrigDataPtrpDataBySizeView(pOrg,dfPosTest,dwMinInter+dfNeedSkip);

                        dfReduceCmpOk++;


                        for (j = dfNeedSkip; j < dwMinInter+dfNeedSkip; j++)
                            {
                              if ((*(((dfbytepc) pData) + dfPosTest + j)) != (*(lpSeqRech + j)))
                              {
                                  dfReduceCmpOkFalse++;
                                  break;
                              }
                            }
                        if (j==dwMinInter+dfNeedSkip)
                            dfReduceCmpOkTrue++;
                    }
#endif
/*
                    if (fBreak)
                        break;*/
                }

#if defined(VERBOSE_BUILDEVENT) && defined(COUNT_FAILSKIP)
                {
                    dfuLong32 l;
                    for (l=0;l<dfNeedSkip;l++)
                        if ((*(((dfbytepc) pData) + dfPosTest + l)) != (*(lpSeqRech + l)))
                        {
                            dfCountTrueCompareFailInSkip++;
                            break;
                        }
                }
#endif

            }
        }
#endif


        if (fBreak)
            j=0;
        else
        {
            dfuLong32 dfSkipBeginRech = 0;
            dfvoidp pData;
            pData = GetOrigDataPtrpDataBySizeView(pOrg,dfPosTest,dfNeedSkip+dwMaxRech);

            if (fCanSkipBeginSearch)
            {
                dfsLong32 m;
                for (m=((dfsLong32)dfNeedSkip)-1;m>=0;m--)
                {
                    if ((*(((dfbytepc) pData) + dfPosTest + m)) != (*(lpSeqRech + m)))
                        break;
                }
                dfSkipBeginRech = (dfuLong32)(m+1);

#ifdef VERBOSE_BUILDEVENT
                if (dfSkipBeginRech>0)
                  dfCountSkipNeededInTrueCompare++;

                if (dfSkipBeginRech + dwMaxRech < dwMinInter)
                    printf("skip too big\n");
#endif
            }

            {
                const dfuLong32 UNALIGNED * pDataBrowse32 = (const dfuLong32*)(((dfbytepc) (pData)) + dfPosTest + dfSkipBeginRech);
                const dfuLong32 UNALIGNED * pSeqRechBrowse32= (const dfuLong32*)(lpSeqRech+dfSkipBeginRech);

                j=0;

                {
                    dfuLong32 dwMaxRechAroundLower4 ;
                    dfuLong32 dwMaxRechThis ;

                    dwMaxRechThis=dwMaxRech;

#ifdef CHECK_MAXORG_FIND
                    if (dfPosTest + dwMaxRech > dwMaxSearchOrg)
                        dwMaxRechThis = (dfuLong32)(dwMaxSearchOrg-dfPosTest);

                    if (dwMaxRechThis != dwMaxRech)
                        printf("*5\n");
#endif

                    dwMaxRechThis -= dfSkipBeginRech;
                    dwMaxRechAroundLower4 = dwMaxRechThis & 0xfffffffc;

                    for (;;)
                    {
                        if ((*pSeqRechBrowse32) != (*pDataBrowse32))
                        {
                            dfbytepc pSeqRechBrowse = (dfbytepc)pSeqRechBrowse32;
                            dfbytepc pDataBrowse = (dfbytepc)pDataBrowse32;
                            if ((*(pSeqRechBrowse)) == (*(pDataBrowse)))
                            {
                                j++;
                                if ((*(pSeqRechBrowse+1)) == (*(pDataBrowse+1)))
                                {
                                    j++;
                                    if ((*(pSeqRechBrowse+2)) == (*(pDataBrowse+2)))
                                        j++;
                                }
                            }
                            break;
                        }
                        pSeqRechBrowse32++;
                        pDataBrowse32++;
                        j+=4;

                        if (j>=dwMaxRechAroundLower4)
                        {
                            dfbytepc pSeqRechBrowse = (dfbytepc)pSeqRechBrowse32;
                            dfbytepc pDataBrowse = (dfbytepc)pDataBrowse32;
                            if (j<dwMaxRechThis)
                            if (((*(pSeqRechBrowse)) == (*(pDataBrowse))))
                            {
                                j++;
                                if (j<dwMaxRechThis)
                                if (((*(pSeqRechBrowse+1)) == (*(pDataBrowse+1))))
                                {
                                    j++;
                                    if (j<dwMaxRechThis)
                                    if (((*(pSeqRechBrowse+2)) == (*(pDataBrowse+2))))
                                        j++;
                                }
                            }
                            break;
                        }
                    }
                }
            }

#ifdef VERBOSE_BUILDEVENT
            if (pHashedFullFile!=NULL)
              if (j<(dfNbCellReduceCompare * (((pPreWkFnd->pHashedFullFile)+0)->dfSizeCluster)))
                  dfCountTrueCompareOkInCrcZone++;
            if (j >= dwMinInter)
                dfCountTrueCompareMinAcceptable++;
#endif
            /*
            if ((j >= dwMinInter) && (j >= dwSizeOrgBetter))
            {
                //if ((j > dwSizeOrgBetter) || (posOrgBetter < dwLatestOrg))
                if ((j > dwSizeOrgBetter) ||
                    ((posOrgBetter < dwLatestOrg) && (dfPosTest > posOrgBetter)) ||
                    (dfPosTest >= dwLatestOrg) && (posOrgBetter > dfPosTest))
                {
                    dwSizeOrgBetter = j;
                    posOrgBetter = dfPosTest;
                    fFound = TRUE;
                    fNbCellHashCmpRecalc=TRUE;
                    dfSkipBeginSearch = dfSkipBeginRech;
#ifdef VERBOSE_BUILDEVENT
                    dfCountTrueCompareOk++;
                    if (dfSkipBeginRech > 0)
                        dfCountTrueCompareOkWithSkip++;
#endif
                }
                // dwMaxBeginString = dwMaxSearchOrg - dwSizeMinFind;
            }
            */

            if ((j >= dwMinInter) /*&& (j >= dwSizeOrgBetter)*/)
            {
                BOOL fRecordThis;
                if ((!fChooseFirst) && (!fChooseLast))
                {
                    *posOrg = dfPosTest;
                    *dwSizeOrgRcp = j;

                    if (pdfSkipBeginSearch!=NULL)
                        *pdfSkipBeginSearch = dfSkipBeginRech;
                    #ifdef VERBOSE_BUILDEVENT
                    dfStepNewReturnTrue++;
                    #endif
                    return TRUE;
                }

                fRecordThis = (j >= dwSizeOrgBetter) || (!fFound);
                if (fChooseFirst && (dfPosTest < posOrgBetter))
                    fRecordThis = TRUE;

                if (fChooseLast && (dfPosTest > posOrgBetter))
                    fRecordThis = TRUE;

                if (fRecordThis)
                {
                    dwSizeOrgBetter = j;
                    posOrgBetter = dfPosTest;
                    fFound = TRUE;
                    //fNbCellHashCmpRecalc=TRUE;
                    dfSkipBeginSearch = dfSkipBeginRech;

#ifdef VERBOSE_BUILDEVENT
if (dfSkipBeginRech+dwSizeOrgBetter>dwMaxRech)
  printf("after bound org probably 2\n");
#endif

#ifdef VERBOSE_BUILDEVENT
                    dfCountTrueCompareOk++;
                    if (dfSkipBeginRech > 0)
                        dfCountTrueCompareOkWithSkip++;
#endif
                }
            }
#ifdef VERBOSE_BUILDEVENT
            dfCountTrueCompare++;
#endif
        }
      }

        dfItemSortedArrayBrowse+=iItemSortedArrayIncr;
        if (dfItemSortedArrayBrowse == dfItemSortedArrayLimit)
            break;
        dfChainPos = *((pBhfl->pSortedArray) + dfItemSortedArrayBrowse);
    }
  }

  if (fFound)
  {

    *posOrg = posOrgBetter;
    *dwSizeOrgRcp = dwSizeOrgBetter;
  }


  if (!(fFound && (dwSizeOrgBetter > dwMinInter)))
      return FALSE;

  if (pdfSkipBeginSearch!=NULL)
        *pdfSkipBeginSearch = dfSkipBeginSearch;

#ifdef _DEBUG
  {
      dfuLong32 i;
      dfvoidp pData;
      pData = GetOrigDataPtrpDataBySizeView(pOrg,posOrgBetter + dfSkipBeginSearch,dwSizeOrgBetter);

      for (i=0;i<dwSizeOrgBetter;i++)
      {
          if ((*(((dfbytepc) pData) + posOrgBetter + i + dfSkipBeginSearch)) != (*(lpSeqRech + i + dfSkipBeginSearch)))
          {
              printf("Error in cmp\n");
              break;
          }
          if (dwSizeOrgBetter+dfSkipBeginSearch >dwMaxRech)
              printf("after bound srch\n");

          if (posOrgBetter+dfSkipBeginSearch+dwSizeOrgBetter > dwMaxSearchOrg)
              printf("*7:after bound org\n");

      }
  }
#endif

#ifdef VERBOSE_BUILDEVENT
  dfStepNewReturnTrue++;
#endif
  return TRUE;
}



BOOL FindSeqPreWKS(HPREWKFND hPreWkFnd,
             dfbytep lpSeqRech, dfuLong32 dwMaxRech,
             ORIGDATA* pOrig, dfuLong64 dwSizeOrg,
             dfuLong64 dwBeginSearch, dfuLong64 dwMaxSearchOrg,
             dfuLong64 dwLatestOrg, dfuLong32 dwMinInter,
             dfuLong32 dfLengthStopSearch,
             dfuLong32 dfAlign, dfuLong32 *pdfSkipBeginSearch,
             dfuLong64 * posOrg, dfuLong32 * dwSizeOrgRcp)
{
  const BLOCKHASH_FATLIKE* pBhfl = NULL ;
  PREWKFND *pPreWkFnd = (PREWKFND *) hPreWkFnd;
  dfuLong32 dfBlockCalcSize = 0 ;
  const REDUCEDFULLFILE* pHashedFullFile = NULL;//((pPreWkFnd->pHashedFullFile)+k);
  dfuLong32 dfSkipBeginSearch;
  BOOL fCanSkipBeginSearch;
  dfuLong32 dwSizeOrgBetter, posOrgBetter;
  BOOL fFound;
  dfuLong32 dfMinSizeCluster,dfMaxSizeCluster,dfMaxSizeReducedStringSearch,dfMaxSizeReducedCluster;
  dfuLong32 dwMaxRechHash ;
  dfuLong32 dfSizeCacheHashFatLikeSearch ;
  dfuLong32 dfNbHashFatLikeValue;
  BOOL fReduceSearchString = FALSE;
  dfbytep pBufForReduceHashCache = NULL;
  CACHE_HASH_INFO* pBufCacheHashFatLikeSearch = NULL;
  dfuLong32* pReducedHashCacheSize=NULL;
  dfuLong32 dfNbCellReduceCmp=0;
  dfuLong32 i ;
  BOOL fRet=FALSE;
  BOOL * pfIsMinEnough=NULL;

  fCanSkipBeginSearch = FALSE;
  if (pdfSkipBeginSearch != NULL)
      fCanSkipBeginSearch = TRUE;


  /* now search if some bytes are repeat */
  /*
  {
      dfuLong32 i;
      dfbyte cLatest;
      dfuLong32 iCountRepeat=1;
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
                  iCountRepeat++;
      }

      if (iCountRepeat>=dwMinInter)
          return FALSE;
  }*/

  if (pPreWkFnd == NULL)
    return FALSE;

  pBhfl = pPreWkFnd->pbhflArray[0];
  dfBlockCalcSize = pBhfl->dfBlockCalcSize;


  dfSizeCacheHashFatLikeSearch = PWFAroundUpper(dfBlockCalcSize * sizeof(CACHE_HASH_INFO),0x10);
  if (!EnlargeInternalDataBuffer(pPreWkFnd,dfSizeCacheHashFatLikeSearch))
      return FALSE;

  if (dfAlign>1)
  {
      dfuLong32 dfModulo = (dfuLong32)(dwBeginSearch % dfAlign);
      if (dfModulo>0)
          dwBeginSearch += (dfAlign-dfModulo);
  }

  if (pfIsMinEnough != NULL)
    *pfIsMinEnough = (dwMinInter >= pBhfl->dfMinSearchSize);


  if (dwMaxSearchOrg == 0)
    dwMaxSearchOrg = dwSizeOrg;

  if (dwMinInter < pBhfl->dfMinSearchSize)
  {
    return FALSE;
  }

  dwSizeOrgBetter = 0;
  posOrgBetter = 0;
  dfSkipBeginSearch = 0;
  fFound = FALSE;
  dfMaxSizeReducedStringSearch = 0;


  /* to do : better selection */
  pBhfl = pPreWkFnd->pbhflArray[0];
  dfBlockCalcSize = pBhfl->dfBlockCalcSize;


  for (i=0;i<pPreWkFnd->dfNbHashedFullFile;i++)
  {
      pHashedFullFile = ((pPreWkFnd->pHashedFullFile)+i);
      if (pHashedFullFile->dfSizeCluster > dwMinInter)
          pHashedFullFile=NULL;
      else
          if (((dfBlockCalcSize) % (pHashedFullFile->dfSizeCluster))!=0)
              pHashedFullFile=NULL;
      if (pHashedFullFile!=NULL)
          break;
  }

  /* prepare hashing */

  if (pHashedFullFile!=NULL)
  {
      dfuLong32 l;
      dfuLong32 dfSizeBufForReduceHashCache=0;
      dfuLong32 dfSizeBufForReducedHashCacheSizeItem;
      dfMinSizeCluster = pHashedFullFile->dfSizeCluster;
      dfMaxSizeCluster = pHashedFullFile->dfSizeCluster;
      dfMaxSizeReducedCluster = pHashedFullFile->dfSizeReducedCluster ;

#define MAXRECHHASH (0x400)
      dwMaxRechHash = myMin(dwMaxRech,MAXRECHHASH);


      dfMaxSizeReducedStringSearch = ((dwMaxRechHash / dfMinSizeCluster) + 1) * dfMaxSizeReducedCluster;
      dfMaxSizeReducedStringSearch = PWFAroundUpper(dfMaxSizeReducedStringSearch,8)+8;

      dfSizeBufForReduceHashCache = dfBlockCalcSize*dfMaxSizeReducedStringSearch;
      dfSizeBufForReduceHashCache = PWFAroundUpper(dfSizeBufForReduceHashCache,0x10);

      dfSizeBufForReducedHashCacheSizeItem = dfBlockCalcSize*sizeof(dfuLong32);

      fReduceSearchString = EnlargeInternalDataBuffer(pPreWkFnd,dfSizeCacheHashFatLikeSearch+
                                                      dfSizeBufForReduceHashCache + dfSizeBufForReducedHashCacheSizeItem);
      if (!fReduceSearchString)
          pHashedFullFile = NULL;
      else
      {
          dfuLong32 dfMinimalLengthEqual,lStep;
          pBufForReduceHashCache = ((dfbytep)pPreWkFnd->pWorkDataBuffer) + dfSizeCacheHashFatLikeSearch;
          pReducedHashCacheSize = (dfuLong32*)(((dfbytep)pPreWkFnd->pWorkDataBuffer) +
                                          dfSizeBufForReduceHashCache + dfSizeCacheHashFatLikeSearch);

          dfMinimalLengthEqual = myMin(dwMinInter,dwMaxRechHash);



          dfNbCellReduceCmp = (dfMinimalLengthEqual - (pHashedFullFile ->dfSizeCluster - 1)) /
                          (pHashedFullFile ->dfSizeCluster) ;


          lStep = 1;
          if ((dfBlockCalcSize % dfAlign) == 0)
              lStep=dfAlign;

          for (l=0;(l<dfBlockCalcSize) && (l < dwMaxRechHash);l+=lStep)
          {
              dfuLong32 dfNbCellHash = 0;
              dfvoidp pWorkDataCurrentShiftedReduction ;
              pWorkDataCurrentShiftedReduction = ((pBufForReduceHashCache) +
                      (l * dfMaxSizeReducedStringSearch));

              if (l < dwMaxRechHash)
                  dfNbCellHash = ((dwMaxRechHash - l) / (pHashedFullFile->dfSizeCluster));
              *(pReducedHashCacheSize+l)=dfNbCellHash;

              ReduceHash(pHashedFullFile,((dfbytepc)lpSeqRech)+l,pHashedFullFile->dfSizeCluster,
                         pWorkDataCurrentShiftedReduction,pHashedFullFile->dfSizeReducedCluster,dfNbCellHash);
          }

          for (;l<pHashedFullFile->dfSizeCluster;l++)
                  *(pReducedHashCacheSize+l)=0;
      }
  }

  pBufCacheHashFatLikeSearch = (CACHE_HASH_INFO*)(pPreWkFnd->pWorkDataBuffer);
  if (dfBlockCalcSize>dwMaxRech)
      dfNbHashFatLikeValue = 0;
  else
      dfNbHashFatLikeValue = myMin(dfBlockCalcSize,(dwMaxRech+1)-dfBlockCalcSize);


  for (i = 0; i < dfBlockCalcSize; i ++)
      (pBufCacheHashFatLikeSearch+i)->dfHashValue = NOTHING_POS;

  //for (i = 0; i < dfBlockCalcSize - 0; i += dfAlign)
  //for (i=0;i<dfNbHashFatLikeValue;i++)
  for (i=0;i<dfNbHashFatLikeValue;i += dfAlign)
  {/*
      if (dfBlockCalcSize + i > dwMaxRech)
          break;*/
      //*(pBufCacheHashFatLikeSearch+i) = (pBhfl->fncGetHashFatLikeValue)(pBhfl,((dfbytepc)lpSeqRech)+i);
      (pBufCacheHashFatLikeSearch+i)->dfHashValue = DOGETHASHFATLIKEVALUE(pBhfl,((dfbytepc)lpSeqRech)+i);
  }

  // @@@



#define DO_TEST_DICHNEW_no
#ifdef DO_TEST_DICHNEW

  for (i=0;i<dfNbHashFatLikeValue;i += dfAlign)
  {
      dfuLong32 dfHashValue;
      dfuLong32 dfFirstItemPossibleCurrentHashValue,dfLastItemPossibleCurrentHashValue;
      dfuLong32 dfStepDichFirst ;
      dfuLong32 dfStepDich ;
      dfuLong32 dfChainPosMinAcceptable,dfChainPosMaxAcceptable;
      dfuLong32 dfFirstSearchPosit;
      dfuLong32 dfFirstValid,dfLastValid;
      dfuLong32 dfChainPosLastOrg = (dfuLong32)(dwLatestOrg / dfBlockCalcSize);// @@+
      dfChainPosMinAcceptable=dfChainPosMaxAcceptable=0;

      dfChainPosMinAcceptable = (dfuLong32)((dwBeginSearch+dfBlockCalcSize) / dfBlockCalcSize); // @@+
      dfChainPosMaxAcceptable = (dfuLong32)((dwMaxSearchOrg - (dfBlockCalcSize + dwMinInter)) / dfBlockCalcSize); // @@+



      dfHashValue = DOGETHASHFATLIKEVALUE(pBhfl,((dfbytepc)lpSeqRech)+i);
      (pBufCacheHashFatLikeSearch+i)->dfHashValue = dfHashValue;
      dfFirstItemPossibleCurrentHashValue = dfFirstValid =
        *((pBhfl->ArrayPosValue[ARRAYPOS_FIRSTPOS].pdfpwfPos)+dfHashValue) ;
      dfLastItemPossibleCurrentHashValue = dfLastValid =
        (*((pBhfl->ArrayPosValue[ARRAYPOS_FIRSTPOS].pdfpwfPos)+dfHashValue + 1))-1 ;

      if (dfFirstItemPossibleCurrentHashValue > dfLastItemPossibleCurrentHashValue)
           continue;


      dfStepDichFirst = 1;
      while (dfStepDichFirst < ((dfLastItemPossibleCurrentHashValue+1)-dfFirstItemPossibleCurrentHashValue))
          dfStepDichFirst = dfStepDichFirst << 1;



      if (dfChainPosMinAcceptable!=0)
      {
        dfStepDich = dfStepDichFirst*1;
        while (dfStepDich > 0)
        {
            dfStepDich = dfStepDich >> 1;
            if ((dfFirstItemPossibleCurrentHashValue+dfStepDich-0) <= (dfLastItemPossibleCurrentHashValue))
            {
                dfuLong32 dfChainPosTest = *((pBhfl->pSortedArray) + dfFirstItemPossibleCurrentHashValue + dfStepDich-0);
                if (dfChainPosTest < dfChainPosMinAcceptable)
                {
                    dfFirstItemPossibleCurrentHashValue += dfStepDich+1;
                }
            }
        }
      }

      if ((dfChainPosMaxAcceptable!=0) && (dfChainPosMaxAcceptable!=0xffffffff))
      {
        dfuLong32 dfLastItemPossibleCurrentHashValueLimited=dfLastValid;
        dfStepDich = dfStepDichFirst;
        while (dfStepDich > 0)
        {
            dfStepDich = dfStepDich >> 1;
            // in other word :
            // if ((dfLastItemPossibleCurrentHashValueLimited-dfStepDich) >= dfFirstItemPossibleCurrentHashValue)
            if ((dfLastItemPossibleCurrentHashValueLimited) >= dfFirstItemPossibleCurrentHashValue+dfStepDich)
                if ((*((pBhfl->pSortedArray) + (dfLastItemPossibleCurrentHashValueLimited - dfStepDich))) > dfChainPosMaxAcceptable)
                dfLastItemPossibleCurrentHashValueLimited -= (dfStepDich+1) ;
        }
        dfLastItemPossibleCurrentHashValue=dfLastItemPossibleCurrentHashValueLimited;

        if (dfLastItemPossibleCurrentHashValue==dfFirstItemPossibleCurrentHashValue)
            if (!((*((pBhfl->pSortedArray) + (dfLastItemPossibleCurrentHashValue))) <= dfChainPosMaxAcceptable))
                dfLastItemPossibleCurrentHashValue-=0;
      }



      {
          dfStepDich = dfStepDichFirst*1;
          dfFirstSearchPosit = dfFirstItemPossibleCurrentHashValue;

          while (dfStepDich > 0)
          {
           dfStepDich = dfStepDich >> 1;
           if ((dfFirstSearchPosit+dfStepDich-00) <= (dfLastItemPossibleCurrentHashValue))
            {
                dfuLong32 dfChainPosTest = *((pBhfl->pSortedArray) + dfFirstSearchPosit + dfStepDich-00);
                if (dfChainPosTest <= dfChainPosLastOrg)
                {
                    dfFirstSearchPosit += dfStepDich+1;
                }
            }

            //dfStepDich = dfStepDich >> 1;
          }

      }


      //(pBufCacheHashFatLikeSearch+i)->dfFirstItemPossibleCurrentHashValue = dfFirstItemPossibleCurrentHashValue;
      //(pBufCacheHashFatLikeSearch+i)->dfLastItemPossibleCurrentHashValue = dfLastItemPossibleCurrentHashValue;

      //(pBufCacheHashFatLikeSearch+i)->dfBrowsePositive = dfFirstSearchPosit;
      //(pBufCacheHashFatLikeSearch+i)->fBrowsePositive = (dfFirstSearchPosit<=dfLastItemPossibleCurrentHashValue) &&
      //           (dfFirstItemPossibleCurrentHashValue <= dfLastItemPossibleCurrentHashValue)
      //(pBufCacheHashFatLikeSearch+i)->dfBrowseNeg = dfFirstSearchPosit-1;
      //(pBufCacheHashFatLikeSearch+i)->fBrowseNeg = ((dfFirstSearchPosit-1)>=dfFirstItemPossibleCurrentHashValue) &&
      //           (dfFirstItemPossibleCurrentHashValue <= dfLastItemPossibleCurrentHashValue)

#if defined(_DEBUG) || defined(DO_TEST_DICHNEW)
      {
          dfuLong32 i;
          dfuLong32 dfFirstItemSlow=*((pBhfl->ArrayPosValue[ARRAYPOS_FIRSTPOS].pdfpwfPos)+dfHashValue);
          dfuLong32 dfLastItemSlow=dfFirstItemSlow;
          dfuLong32 dfFirstSearchPositSlow=dfFirstItemSlow;
          BOOL fBrowseNegSlow,fBrowsePositSlow;
          BOOL fBrowseNeg ,fBrowsePositive ;

          fBrowsePositive = (dfFirstSearchPosit<=dfLastItemPossibleCurrentHashValue)
               && (dfFirstItemPossibleCurrentHashValue <= dfLastItemPossibleCurrentHashValue) ;
          fBrowseNeg = ((dfFirstSearchPosit-1)>=dfFirstItemPossibleCurrentHashValue)
               && (dfFirstItemPossibleCurrentHashValue <= dfLastItemPossibleCurrentHashValue) ;

          dfLastItemSlow=(*((pBhfl->ArrayPosValue[ARRAYPOS_FIRSTPOS].pdfpwfPos)+dfHashValue ))-1;
          dfFirstSearchPositSlow = *((pBhfl->ArrayPosValue[ARRAYPOS_FIRSTPOS].pdfpwfPos)+dfHashValue);

          for (i=(*((pBhfl->ArrayPosValue[ARRAYPOS_FIRSTPOS].pdfpwfPos)+dfHashValue));
               i<(*((pBhfl->ArrayPosValue[ARRAYPOS_FIRSTPOS].pdfpwfPos)+dfHashValue + 1));i++)
          {
              dfuLong32 dfChainPosTest = *((pBhfl->pSortedArray) + i);
              if (dfChainPosMinAcceptable!=0)
                  if (dfChainPosTest<dfChainPosMinAcceptable)
                      dfFirstItemSlow = i+1;

              if (dfChainPosTest<=dfChainPosLastOrg)
                  dfFirstSearchPositSlow=i+1;

              if ((dfChainPosMaxAcceptable!=0) && (dfChainPosMaxAcceptable!=0xffffffff))
              {
                  if (dfChainPosTest<=dfChainPosMaxAcceptable)
                      dfLastItemSlow=i;
              }
              else
                  dfLastItemSlow=i;
          }
          fBrowseNegSlow=fBrowsePositSlow=FALSE;
          if (dfFirstItemSlow<=dfLastItemSlow)
          {
              fBrowseNegSlow=(dfFirstSearchPositSlow-1)>=dfFirstItemSlow;
              fBrowsePositSlow=dfFirstSearchPositSlow<=dfLastItemSlow;
          }
          if (fBrowseNegSlow != fBrowseNeg)
              printf("neg error\n");
          if (fBrowsePositSlow != fBrowsePositive)
              printf("posit error\n");

          if (fBrowseNegSlow || fBrowsePositSlow)
              if (dfFirstItemSlow != dfFirstItemPossibleCurrentHashValue)
                  printf("first error\n");

          if (fBrowseNegSlow || fBrowsePositSlow)
              if (dfLastItemSlow != dfLastItemPossibleCurrentHashValue)
                  printf("last error %u %u\n",dfLastItemSlow ,dfLastItemPossibleCurrentHashValue);

          if (fBrowseNegSlow || fBrowsePositSlow)
              if (dfFirstSearchPositSlow != dfFirstSearchPosit)
                  printf("search posit error\n");
          //printf(".");
      }
#endif

  }
#endif


  // @@@

  /**/
  {
    //dfuLong64 dfLastArrayPos = dwMaxSearchOrg / pBhfl->dfArrayLength;
    //dfuLong64 dfFirstArrayPos = dwBeginSearch / pBhfl->dfArrayLength;
    dfuLong64 dfCurrentArrayPos = dwLatestOrg / pBhfl->dfArrayLength;

    dfuLong32 dfNbStepArrayOrder=2;
    dfuLong64 dfFirstSearchAfterArrayPosLimit = dfCurrentArrayPos + dfNbStepArrayOrder;

    dfuLong64 dfFirstSearchBeforeArrayPosLimit=0;

    dfuLong64 dfChainPosMinAcceptable = ((dwBeginSearch+dfBlockCalcSize) / dfBlockCalcSize);
    dfuLong64 dfChainPosMaxAcceptable = ((dwMaxSearchOrg - (dfBlockCalcSize + dwMinInter)) / dfBlockCalcSize);
    dfuLong32 dfReservEndFile = dfBlockCalcSize + dwMinInter + dfMaxSizeReducedStringSearch ;
    dfuLong32 dfChainPosMaxSecure = (dfuLong32)((dwSizeOrg - dfReservEndFile) / dfBlockCalcSize);
    dfuLong32 dfNbBlockByArray = (dfuLong32)(pBhfl->dfArrayLength / dfBlockCalcSize);
    dfuLong32 k1,k2,sumK;

    if (dfReservEndFile>dwSizeOrg)
        dfChainPosMaxSecure = 0;
    if (dfChainPosMaxAcceptable>dfChainPosMaxSecure)
        dfChainPosMaxAcceptable=dfChainPosMaxSecure;

    if (dfCurrentArrayPos > dfNbStepArrayOrder)
        dfFirstSearchBeforeArrayPosLimit = dfCurrentArrayPos - dfNbStepArrayOrder;


    if (dfNbStepArrayOrder>0)
    {
#if defined(_DEBUG) && defined(VERBOSE_BUILDEVENT)
//        { char sz[200] ; wsprintf(sz,"\n******************** pos = %u - %u\n",
//                   (dfuLong32)dwLatestOrg,(dfuLong32)(dwLatestOrg / dfBlockCalcSize)); OutputDebugString(sz);}
#endif

        fRet=NewFindSeqPreWKStep(hPreWkFnd,
                lpSeqRech, dwMaxRech,
                lpFileOrg,
                myMax(dfChainPosMinAcceptable,dwLatestOrg / dfBlockCalcSize),
                myMin((dfCurrentArrayPos+1)*dfNbBlockByArray,dfChainPosMaxAcceptable),

                //dwLatestOrg, myMin(dwMaxSearchOrg,dfFirstSearchAfterArrayPosLimit*pBhfl->dfArrayLength),
                dwMinInter,
                dfAlign, pdfSkipBeginSearch,
                posOrg, dwSizeOrgRcp,

                pHashedFullFile,
                pBufForReduceHashCache,
                dfMaxSizeReducedStringSearch,
                pReducedHashCacheSize,

                pBhfl,pBufCacheHashFatLikeSearch,
                TRUE /*fChooseFirst*/,FALSE /*fChooseLast*/, FALSE /* fDirectionNeg*/
#ifdef CHECK_MAXORG_FIND
                ,dwMaxSearchOrg
#endif
                );

        if (!fRet)
        fRet=NewFindSeqPreWKStep(hPreWkFnd,
                lpSeqRech, dwMaxRech,
                lpFileOrg,
                //myMax(dwBeginSearch,dfFirstSearchBeforeArrayPosLimit*pBhfl->dfArrayLength),dwLatestOrg,
                myMax(dfChainPosMinAcceptable,(dfCurrentArrayPos+0)*dfNbBlockByArray),
                myMin(dwLatestOrg / dfBlockCalcSize,dfChainPosMaxAcceptable),

                dwMinInter,
                dfAlign, pdfSkipBeginSearch,
                posOrg, dwSizeOrgRcp,

                pHashedFullFile,
                pBufForReduceHashCache,
                dfMaxSizeReducedStringSearch,
                pReducedHashCacheSize,

                pBhfl,pBufCacheHashFatLikeSearch,
                FALSE /*fChooseFirst*/,TRUE /*fChooseLast*/, TRUE /* fDirectionNeg*/
#ifdef CHECK_MAXORG_FIND
                ,dwMaxSearchOrg
#endif
                );
    }


    k1=k2=sumK=0;
    while (!fRet)
    {
        dfuLong64 dfChainPosMinInSearch,dfChainPosMaxInSearch;
        if (k1==0)
        {
            dfChainPosMinInSearch = myMax(dfChainPosMinAcceptable,(dfCurrentArrayPos+(k2+1))*dfNbBlockByArray);
            dfChainPosMaxInSearch = myMin((dfCurrentArrayPos+(k2+2))*dfNbBlockByArray,dfChainPosMaxAcceptable);
        }
        else
        {
            if (dfCurrentArrayPos >= k2+1)
            {
              dfChainPosMinInSearch = myMax(dfChainPosMinAcceptable,(dfCurrentArrayPos-(k2+1))*dfNbBlockByArray);
              dfChainPosMaxInSearch = myMin((dfCurrentArrayPos-(k2))*dfNbBlockByArray,dfChainPosMaxAcceptable);
            }
            else
                dfChainPosMinInSearch = dfChainPosMaxInSearch = 0;
        }

        if (dfChainPosMinInSearch < dfChainPosMaxInSearch)
        {
            sumK++;

            fRet=NewFindSeqPreWKStep(hPreWkFnd,
                lpSeqRech, dwMaxRech,
                lpFileOrg,
                //myMax(dwBeginSearch,dfFirstSearchBeforeArrayPosLimit*pBhfl->dfArrayLength),dwLatestOrg,
                dfChainPosMinInSearch,dfChainPosMaxInSearch,
                dwMinInter,
                dfAlign, pdfSkipBeginSearch,
                posOrg, dwSizeOrgRcp,

                pHashedFullFile,
                pBufForReduceHashCache,
                dfMaxSizeReducedStringSearch,
                pReducedHashCacheSize,

                pBhfl,pBufCacheHashFatLikeSearch,
                (k1==0) && (k2<1) /*fChooseFirst*/,
                (k1==1) && (k2<1) /*fChooseLast*/,
                (k1==1)
#ifdef CHECK_MAXORG_FIND
                ,dwMaxSearchOrg
#endif
                );
        }

        if (k1==0)
            k1=1;
        else
        {
            if (sumK==0)
                break;

            k1=0;
            k2++;
            sumK=0;
        }
    }

/*
    if ((!fRet) && (dfFirstSearchAfterArrayPosLimit*pBhfl->dfArrayLength<dwMaxSearchOrg))
    fRet=NewFindSeqPreWKStep(hPreWkFnd,
             lpSeqRech, dwMaxRech,
             lpFileOrg,
             //myMin(dwMaxSearchOrg,dfFirstSearchAfterArrayPosLimit*pBhfl->dfArrayLength),dwMaxSearchOrg,
             dfFirstSearchAfterArrayPosLimit*pBhfl->dfArrayLength,dwMaxSearchOrg,
             dwMinInter,
             dfAlign, pdfSkipBeginSearch,
             posOrg, dwSizeOrgRcp,

             pHashedFullFile,
             pBufForReduceHashCache,
             dfMaxSizeReducedStringSearch,
             pReducedHashCacheSize,

             pBhfl,pBufCacheHashFatLikeSearch,
             FALSE ,FALSE );

    if ((!fRet) && (dwBeginSearch <dfFirstSearchBeforeArrayPosLimit*pBhfl->dfArrayLength))
    fRet=NewFindSeqPreWKStep(hPreWkFnd,
             lpSeqRech, dwMaxRech,
             lpFileOrg,
             //dwBeginSearch,myMax(dwBeginSearch,dfFirstSearchBeforeArrayPosLimit*pBhfl->dfArrayLength),
             dwBeginSearch,dfFirstSearchBeforeArrayPosLimit*pBhfl->dfArrayLength,
             dwMinInter,
             dfAlign, pdfSkipBeginSearch,
             posOrg, dwSizeOrgRcp,

             pHashedFullFile,
             pBufForReduceHashCache,
             dfMaxSizeReducedStringSearch,
             pReducedHashCacheSize,

             pBhfl,pBufCacheHashFatLikeSearch,
             FALSE ,FALSE );
             */
  }

  return fRet;
}
#else

typedef struct
{
    dfuLong32 dfHashValue;
    dfuLong32 dfFirstItemPossibleCurrentHashValue;
    dfuLong32 dfBrowseNeg;
    BOOL      fBrowseNeg;
    dfuLong32 dfLastItemPossibleCurrentHashValue;
    dfuLong32 dfBrowsePositive;
    dfuLong32 dfValueToTestItem;
    BOOL      fBrowsePositive;

    BOOL      fReduceCalcDone;
    dfuLong32 dfNbCellReduce;
    dfbyte    ReducedBlock[1];
} CACHE_HASH_INFO;




BOOL FindSeqPreWKS(HPREWKFND hPreWkFnd,
             dfbytep lpSeqRech, dfuLong32 dwMaxRech,
             ORIGDATA* pOrg,dfuLong64 dwSizeOrg,
             dfuLong64 dwBeginSearch, dfuLong64 dwMaxSearchOrg,
             dfuLong64 dwLatestOrg, dfuLong32 dwMinInter,
             dfuLong32 dfLengthStopSearch,
             dfuLong32 dfAlign, dfuLong32 *pdfSkipBeginSearch,
             dfuLong64 * posOrg, dfuLong32 * dwSizeOrgRcp
             /*,BOOL * pfIsMinEnough*/)
{
  const BLOCKHASH_FATLIKE* pBhfl = NULL ;
  PREWKFND *pPreWkFnd = (PREWKFND *) hPreWkFnd;
  dfuLong32 dfBlockCalcSize = 0 ;
  const REDUCEDFULLFILE* pHashedFullFile = NULL;//((pPreWkFnd->pHashedFullFile)+k);
  dfuLong32 dfSkipBeginSearch;
  BOOL fCanSkipBeginSearch;
  BOOL fFound;
  dfuLong32 dfMinSizeCluster,dfMaxSizeCluster,dfMaxSizeReducedStringSearch,dfMaxSizeReducedCluster;
  dfuLong32 dwMaxRechHash ;
  dfuLong32 dfSizeCacheHashFatLikeSearch ;
  dfuLong32 dfNbHashFatLikeValue;
  //BOOL fReduceSearchString = FALSE;
  //dfbytep pBufForReduceHashCache = NULL;
  CACHE_HASH_INFO* pBufCacheHashFatLikeSearch = NULL;
  CACHE_HASH_INFO* pBufCacheHashFatLikeSearchBrowseFill;
  //dfuLong32* pReducedHashCacheSize=NULL;
  dfuLong32 dfNbCellReduceCmp=0;
  dfuLong32 i ;
  dfuLong32 dfChainPosLastOrg ;
  dfuLong32 dfChainPosMinAcceptable,dfChainPosMaxAcceptable;
//  BOOL fRet=FALSE;
  dfuLong32 dwSizeOrgBetter = 0;
  dfuLong64 posOrgBetter = 0;
  dfuLong32 dfNbCellReduceCompare=0;
  dfuLong32 dfNbByteReduceCompare=0;
  dfuLong32 dfDistanceMinimalForBreakAlwaysAfterFind ;
  dfuLong32 dfSizeReducedCluster = 0;
  dfuLong32 dfNbCacheHashValue;
  dfuLong32 dfCacheHashItemSize;
  //ORIGDATA* pOrg = pPreWkFnd->pOrg;
  const dfpwfPos *pSortedArray;
  BOOL* pfIsMinEnough=NULL;

  #ifdef VERBOSE_BUILDEVENT
  if (dfAlign==1)
      if (dwMaxRech < ((2*dwMinInter)-1))
          printf("insuficcient size\n");
  #endif


  fCanSkipBeginSearch = FALSE;
  if (pdfSkipBeginSearch != NULL)
      fCanSkipBeginSearch = TRUE;

//printf("%07x ",(dwLatestOrg-dwBeginSearch));
if ((dwLatestOrg-dwBeginSearch)==0x200)
return FALSE;


  /* now search if some bytes are repeat */
  /*
  {
      dfuLong32 i;
      dfbyte cLatest;
      dfuLong32 iCountRepeat=1;
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
                  iCountRepeat++;
      }

      if (iCountRepeat>=dwMinInter)
          return FALSE;
  }*/

  if (pPreWkFnd == NULL)
    return FALSE;

  pBhfl = pPreWkFnd->pbhflArray[0];
  dfBlockCalcSize = pBhfl->dfBlockCalcSize;



  dfSizeCacheHashFatLikeSearch = PWFAroundUpper(dfBlockCalcSize * sizeof(CACHE_HASH_INFO),0x10);
  if (!EnlargeInternalDataBuffer(pPreWkFnd,dfSizeCacheHashFatLikeSearch))
      return FALSE;

  if (dfAlign>1)
  {
      dfuLong32 dfModulo = (dfuLong32)(dwBeginSearch % dfAlign);
      if (dfModulo>0)
          dwBeginSearch += (dfAlign-dfModulo);
  }

  if (pfIsMinEnough != NULL)
    *pfIsMinEnough = (dwMinInter >= pBhfl->dfMinSearchSize);


  if (dwMaxSearchOrg == 0)
    dwMaxSearchOrg = dwSizeOrg;

  if (dwMinInter < pBhfl->dfMinSearchSize)
  {
    return FALSE;
  }

  dwSizeOrgBetter = 0;
  posOrgBetter = 0;
  dfSkipBeginSearch = 0;
  fFound = FALSE;
  dfMaxSizeReducedStringSearch = 0;
  pSortedArray = pBhfl->pSortedArray;


  /* to do : better selection */
  pBhfl = pPreWkFnd->pbhflArray[0];
  dfBlockCalcSize = pBhfl->dfBlockCalcSize;
  dfChainPosLastOrg = (dfuLong32)(dwLatestOrg / dfBlockCalcSize);
  dfChainPosMinAcceptable = (dfuLong32)((dwBeginSearch+dfBlockCalcSize) / dfBlockCalcSize); // @@+
  dfChainPosMaxAcceptable = (dfuLong32)((dwMaxSearchOrg - (dfBlockCalcSize + dwMinInter)) / dfBlockCalcSize); // @@+
  dfDistanceMinimalForBreakAlwaysAfterFind = ((dfuLong32)(dfmin(pBhfl->dfArrayLength,1024*1024) / dfBlockCalcSize));

  for (i=0;i<pPreWkFnd->dfNbHashedFullFile;i++)
  {
      pHashedFullFile = ((pPreWkFnd->pHashedFullFile)+i);
      if (pHashedFullFile->dfSizeCluster > dwMinInter)
          pHashedFullFile=NULL;
      else
          if (((dfBlockCalcSize) % (pHashedFullFile->dfSizeCluster))!=0)
              pHashedFullFile=NULL;
      if (pHashedFullFile!=NULL)
          break;
  }

  /* prepare hashing */

  dfCacheHashItemSize = sizeof(CACHE_HASH_INFO);

  if (pHashedFullFile!=NULL)
  {
      //dfuLong32 l;
      //dfuLong32 dfSizeBufForReduceHashCache=0;
      //dfuLong32 dfSizeBufForReducedHashCacheSizeItem;

      dfMinSizeCluster = pHashedFullFile->dfSizeCluster;
      dfMaxSizeCluster = pHashedFullFile->dfSizeCluster;
      dfMaxSizeReducedCluster = pHashedFullFile->dfSizeReducedCluster ;
      dfSizeReducedCluster = pHashedFullFile->dfSizeReducedCluster;

      dfNbCellReduceCompare = dfBlockCalcSize / pHashedFullFile->dfSizeCluster;
      dfNbByteReduceCompare = dfNbCellReduceCompare * dfSizeReducedCluster;
      dfCacheHashItemSize += dfNbByteReduceCompare;



#define MAXRECHHASH (0x400)
      dwMaxRechHash = myMin(dwMaxRech,MAXRECHHASH);
/*

      dfMaxSizeReducedStringSearch = ((dwMaxRechHash / dfMinSizeCluster) + 1) * dfMaxSizeReducedCluster;
      dfMaxSizeReducedStringSearch = PWFAroundUpper(dfMaxSizeReducedStringSearch,8)+8;

      dfSizeBufForReduceHashCache = dfBlockCalcSize*dfMaxSizeReducedStringSearch;
      dfSizeBufForReduceHashCache = PWFAroundUpper(dfSizeBufForReduceHashCache,0x10);

      dfSizeBufForReducedHashCacheSizeItem = dfBlockCalcSize*sizeof(dfuLong32);

      fReduceSearchString = EnlargeInternalDataBuffer(pPreWkFnd,dfSizeCacheHashFatLikeSearch+
                                                      dfSizeBufForReduceHashCache + dfSizeBufForReducedHashCacheSizeItem);
      if (!fReduceSearchString)
          pHashedFullFile = NULL;*/
  }
  dfCacheHashItemSize = PWFAroundUpper(dfCacheHashItemSize,sizeof(dfvoidp));
  if (!EnlargeInternalDataBuffer(pPreWkFnd,dfCacheHashItemSize * dfBlockCalcSize))
                                                      return 0;

  if (pHashedFullFile != NULL)
  {
          //dfuLong32 lStep;
          dfuLong32 dfMinimalLengthEqual;


          dfMinimalLengthEqual = myMin(dwMinInter,dwMaxRechHash);



          dfNbCellReduceCmp = (dfMinimalLengthEqual - (pHashedFullFile ->dfSizeCluster - 1)) /
                          (pHashedFullFile ->dfSizeCluster) ;

/*
          pBufForReduceHashCache = ((dfbytep)pPreWkFnd->pWorkDataBuffer) + dfSizeCacheHashFatLikeSearch;
          pReducedHashCacheSize = (dfuLong32*)(((dfbytep)pPreWkFnd->pWorkDataBuffer) +
                                          dfSizeBufForReduceHashCache + dfSizeCacheHashFatLikeSearch);


          lStep = 1;
          if ((dfBlockCalcSize % dfAlign) == 0)
              lStep=dfAlign;

          for (l=0;(l<dfBlockCalcSize) && (l < dwMaxRechHash);l+=lStep)
          {
              dfuLong32 dfNbCellHash = 0;
              dfvoidp pWorkDataCurrentShiftedReduction ;
              pWorkDataCurrentShiftedReduction = ((pBufForReduceHashCache) +
                      (l * dfMaxSizeReducedStringSearch));

              if (l < dwMaxRechHash)
                  dfNbCellHash = ((dwMaxRechHash - l) / (pHashedFullFile->dfSizeCluster));
              //dfNbCellHash=dfmin(dfNbCellReduceCompare,dfNbCellHash);
              *(pReducedHashCacheSize+l)=dfNbCellHash;

              ReduceHash(pHashedFullFile,((dfbytepc)lpSeqRech)+l,pHashedFullFile->dfSizeCluster,
                         pWorkDataCurrentShiftedReduction,dfSizeReducedCluster,dfNbCellHash);
          }

          for (;l<pHashedFullFile->dfSizeCluster;l++)
                  *(pReducedHashCacheSize+l)=0;
  */
  }



  pBufCacheHashFatLikeSearch = (CACHE_HASH_INFO*)(pPreWkFnd->pWorkDataBuffer);
  pBufCacheHashFatLikeSearchBrowseFill = pBufCacheHashFatLikeSearch ;
  if (dfBlockCalcSize>dwMaxRech)
      dfNbHashFatLikeValue = 0;
  else
      dfNbHashFatLikeValue = myMin(dfBlockCalcSize,(dwMaxRech+1)-dfBlockCalcSize);

/*
  for (i = 0; i < dfBlockCalcSize; i ++)
  {
      (pBufCacheHashFatLikeSearch+i)->dfHashValue = NOTHING_POS;
      (pBufCacheHashFatLikeSearch+i)->fBrowseNeg = FALSE;
      (pBufCacheHashFatLikeSearch+i)->fBrowsePositive = FALSE;
  }*/

  dfNbCacheHashValue = 0;
  for (i=0;i<dfNbHashFatLikeValue;i += dfAlign/*,dfNbCacheHashValue+=dfAlign*/)
  {
      dfuLong32 dfHashValue;
      dfuLong32 dfFirstItemPossibleCurrentHashValue,dfLastItemPossibleCurrentHashValue;
      dfuLong32 dfStepDichFirst ;
      dfuLong32 dfStepDich ;
      //dfuLong32 dfChainPosMinAcceptable,dfChainPosMaxAcceptable;
      dfuLong32 dfFirstSearchPosit;
      dfuLong32 dfFirstValid,dfLastValid;
      BOOL fBrowseNeg,fBrowsePosit;
      //dfuLong32 dfChainPosLastOrg = (dfuLong32)(dwLatestOrg / dfBlockCalcSize);// @@+
      //dfChainPosMinAcceptable=dfChainPosMaxAcceptable=0;

      //dfChainPosMinAcceptable = (dfuLong32)((dwBeginSearch+dfBlockCalcSize) / dfBlockCalcSize); // @@+
      //dfChainPosMaxAcceptable = (dfuLong32)((dwMaxSearchOrg - (dfBlockCalcSize + dwMinInter)) / dfBlockCalcSize); // @@+

/*
      (pBufCacheHashFatLikeSearch+i)->fBrowseNeg = FALSE;
      (pBufCacheHashFatLikeSearch+i)->fBrowsePositive = FALSE;
*/
      dfHashValue = DOGETHASHFATLIKEVALUE(pBhfl,((dfbytepc)lpSeqRech)+i);

      /*
      for (dfStepDich=0;dfStepDich<10;dfStepDich++)
        dfHashValue = DOGETHASHFATLIKEVALUE(pBhfl,((dfbytepc)lpSeqRech)+i);
      */
      dfFirstItemPossibleCurrentHashValue = dfFirstValid =
        *((pBhfl->ArrayPosValue[ARRAYPOS_FIRSTPOS].pdfpwfPos)+dfHashValue) ;
      dfLastItemPossibleCurrentHashValue = dfLastValid =
        (*((pBhfl->ArrayPosValue[ARRAYPOS_FIRSTPOS].pdfpwfPos)+dfHashValue + 1)) ;

      if (dfFirstValid == dfLastValid)
      {
           continue;
      }




      dfStepDichFirst = 1;
      while (dfStepDichFirst < ((dfLastValid)-dfFirstValid))
          dfStepDichFirst = dfStepDichFirst << 1;

      dfFirstItemPossibleCurrentHashValue = dfFirstValid ;
      dfLastValid--;
      dfLastItemPossibleCurrentHashValue = dfLastValid ;


      if (dfChainPosMinAcceptable!=0)
      {
        dfStepDich = dfStepDichFirst;
        while (dfStepDich > 0)
        {
            dfStepDich = dfStepDich >> 1;
            if ((dfFirstItemPossibleCurrentHashValue+dfStepDich-0) <= (dfLastItemPossibleCurrentHashValue))
            {
                dfuLong32 dfChainPosTest = *((pSortedArray) + dfFirstItemPossibleCurrentHashValue + dfStepDich-0);
                if (dfChainPosTest < dfChainPosMinAcceptable)
                {
                    dfFirstItemPossibleCurrentHashValue += dfStepDich+1;
                }
            }
        }
      }

      if ((dfChainPosMaxAcceptable!=0) && (dfChainPosMaxAcceptable!=0xffffffff))
      {
        dfuLong32 dfLastItemPossibleCurrentHashValueLimited=dfLastValid;
        dfStepDich = dfStepDichFirst;
        while (dfStepDich > 0)
        {
            dfStepDich = dfStepDich >> 1;
            // in other word :
            // if ((dfLastItemPossibleCurrentHashValueLimited-dfStepDich) >= dfFirstItemPossibleCurrentHashValue)
            if ((dfLastItemPossibleCurrentHashValueLimited) >= dfFirstItemPossibleCurrentHashValue+dfStepDich)
                if ((*((pSortedArray) + (dfLastItemPossibleCurrentHashValueLimited - dfStepDich))) > dfChainPosMaxAcceptable)
                dfLastItemPossibleCurrentHashValueLimited -= (dfStepDich+1) ;
        }
        dfLastItemPossibleCurrentHashValue=dfLastItemPossibleCurrentHashValueLimited;
      }



      {
          dfStepDich = dfStepDichFirst;
          dfFirstSearchPosit = dfFirstItemPossibleCurrentHashValue;

          while (dfStepDich > 0)
          {
           dfStepDich = dfStepDich >> 1;
           if ((dfFirstSearchPosit+dfStepDich) <= (dfLastItemPossibleCurrentHashValue))
            {
                dfuLong32 dfChainPosTest = *((pSortedArray) + dfFirstSearchPosit + dfStepDich);
                if (dfChainPosTest <= dfChainPosLastOrg)
                    dfFirstSearchPosit += dfStepDich+1;
            }
          }
      }


      fBrowseNeg = ((dfFirstSearchPosit-1)>=dfFirstItemPossibleCurrentHashValue);
      fBrowsePosit = (dfFirstSearchPosit<=dfLastItemPossibleCurrentHashValue);

      if ((!fBrowseNeg) && (!fBrowsePosit))
          continue;

      pBufCacheHashFatLikeSearchBrowseFill->dfFirstItemPossibleCurrentHashValue = dfFirstItemPossibleCurrentHashValue;
      pBufCacheHashFatLikeSearchBrowseFill->dfLastItemPossibleCurrentHashValue = dfLastItemPossibleCurrentHashValue;

      pBufCacheHashFatLikeSearchBrowseFill->dfBrowsePositive = dfFirstSearchPosit;
      pBufCacheHashFatLikeSearchBrowseFill->fBrowsePositive = fBrowsePosit;
      pBufCacheHashFatLikeSearchBrowseFill->dfBrowseNeg = dfFirstSearchPosit-1;
      pBufCacheHashFatLikeSearchBrowseFill->fBrowseNeg = fBrowseNeg;
      pBufCacheHashFatLikeSearchBrowseFill->dfValueToTestItem = i;
      pBufCacheHashFatLikeSearchBrowseFill->dfHashValue = dfHashValue;
      pBufCacheHashFatLikeSearchBrowseFill->fReduceCalcDone = FALSE;

      pBufCacheHashFatLikeSearchBrowseFill = (CACHE_HASH_INFO*)(((dfbytep)pBufCacheHashFatLikeSearchBrowseFill) + dfCacheHashItemSize);
      dfNbCacheHashValue++;

#if defined(_DEBUG) || defined(DO_TEST_DICHNEW)
      {
          dfuLong32 i;
          dfuLong32 dfFirstItemSlow=*((pBhfl->ArrayPosValue[ARRAYPOS_FIRSTPOS].pdfpwfPos)+dfHashValue);
          dfuLong32 dfLastItemSlow=dfFirstItemSlow;
          dfuLong32 dfFirstSearchPositSlow=dfFirstItemSlow;
          BOOL fBrowseNegSlow,fBrowsePositSlow;
          BOOL fBrowseNeg ,fBrowsePositive ;

          fBrowsePositive = (dfFirstSearchPosit<=dfLastItemPossibleCurrentHashValue)
               && (dfFirstItemPossibleCurrentHashValue <= dfLastItemPossibleCurrentHashValue) ;
          fBrowseNeg = ((dfFirstSearchPosit-1)>=dfFirstItemPossibleCurrentHashValue)
               && (dfFirstItemPossibleCurrentHashValue <= dfLastItemPossibleCurrentHashValue) ;

          dfLastItemSlow=(*((pBhfl->ArrayPosValue[ARRAYPOS_FIRSTPOS].pdfpwfPos)+dfHashValue ))-1;
          dfFirstSearchPositSlow = *((pBhfl->ArrayPosValue[ARRAYPOS_FIRSTPOS].pdfpwfPos)+dfHashValue);

          for (i=(*((pBhfl->ArrayPosValue[ARRAYPOS_FIRSTPOS].pdfpwfPos)+dfHashValue));
               i<(*((pBhfl->ArrayPosValue[ARRAYPOS_FIRSTPOS].pdfpwfPos)+dfHashValue + 1));i++)
          {
              dfuLong32 dfChainPosTest = *((pSortedArray) + i);
              if (dfChainPosMinAcceptable!=0)
                  if (dfChainPosTest<dfChainPosMinAcceptable)
                      dfFirstItemSlow = i+1;

              if (dfChainPosTest<=dfChainPosLastOrg)
                  dfFirstSearchPositSlow=i+1;

              if ((dfChainPosMaxAcceptable!=0) && (dfChainPosMaxAcceptable!=0xffffffff))
              {
                  if (dfChainPosTest<=dfChainPosMaxAcceptable)
                      dfLastItemSlow=i;
              }
              else
                  dfLastItemSlow=i;
          }
          fBrowseNegSlow=fBrowsePositSlow=FALSE;
          if (dfFirstItemSlow<=dfLastItemSlow)
          {
              fBrowseNegSlow=(dfFirstSearchPositSlow-1)>=dfFirstItemSlow;
              fBrowsePositSlow=dfFirstSearchPositSlow<=dfLastItemSlow;
          }
          if (fBrowseNegSlow != fBrowseNeg)
              printf("neg error\n");
          if (fBrowsePositSlow != fBrowsePositive)
              printf("posit error\n");

          if (fBrowseNegSlow || fBrowsePositSlow)
              if (dfFirstItemSlow != dfFirstItemPossibleCurrentHashValue)
                  printf("first error\n");

          if (fBrowseNegSlow || fBrowsePositSlow)
              if (dfLastItemSlow != dfLastItemPossibleCurrentHashValue)
                  printf("last error %u %u\n",dfLastItemSlow ,dfLastItemPossibleCurrentHashValue);

          if (fBrowseNegSlow || fBrowsePositSlow)
              if (dfFirstSearchPositSlow != dfFirstSearchPosit)
                  printf("search posit error\n");
          //printf(".");
      }
#endif

  }


  for (;;)
  {
      dfuLong32 i;

      dfuLong32 dfBetterDistanceFromLatestOrgInBlock = 0xffffffff; // maxi
      dfuLong32 dfValueToTestItem;
      dfuLong32 dfValueToTestPosit=0;
      BOOL fBetterValueToTestIsNeg=0;
      CACHE_HASH_INFO *pCurEvolution=NULL;
      CACHE_HASH_INFO *pCurBrowse;

      pCurBrowse = pBufCacheHashFatLikeSearch;
      for (i=0;i<dfNbCacheHashValue;i ++)
      {
          CACHE_HASH_INFO *pCur = (pCurBrowse);
          if (pCur->fBrowseNeg)
          {
              dfuLong32 dfChainPosTest = *((pSortedArray) + (pCur->dfBrowseNeg));
              dfuLong32 dfDistance = (dfChainPosLastOrg - dfChainPosTest);
              /*
              BOOL fSelect=FALSE;
              if (dfDistance < dfBetterDistanceFromLatestOrgInBlock)
                  fSelect = TRUE;

              if (dfDistance == dfBetterDistanceFromLatestOrgInBlock)
                  if ((pCurEvolution->dfValueToTestItem >= pCur->dfValueToTestItem) && fBetterValueToTestIsNeg)
                      fSelect=TRUE;

              if (fSelect)*/
              if (dfDistance < dfBetterDistanceFromLatestOrgInBlock)
              {
                  dfBetterDistanceFromLatestOrgInBlock = dfDistance;
                  fBetterValueToTestIsNeg = TRUE;
                  dfValueToTestPosit = dfChainPosTest;
                  pCurEvolution = pCur;
              }
          }

          if (pCur->fBrowsePositive)
          {
              dfuLong32 dfChainPosTest = *((pSortedArray) + (pCur->dfBrowsePositive));
              dfuLong32 dfDistance = (dfChainPosTest - dfChainPosLastOrg);

              BOOL fSelect=FALSE;


              if (dfDistance < dfBetterDistanceFromLatestOrgInBlock)
                  fSelect=TRUE;

              if (dfDistance == dfBetterDistanceFromLatestOrgInBlock)
                  if (pCurEvolution->dfValueToTestItem >= pCur->dfValueToTestItem)
                      fSelect=TRUE;

              if (fSelect)
              //if (dfDistance <= dfBetterDistanceFromLatestOrgInBlock)
              {
                  dfBetterDistanceFromLatestOrgInBlock = dfDistance;
                  fBetterValueToTestIsNeg = FALSE;
                  dfValueToTestPosit = dfChainPosTest;
                  pCurEvolution = pCur;
              }
          }
          pCurBrowse = (CACHE_HASH_INFO*)(((dfbytep)pCurBrowse) + dfCacheHashItemSize);
      }

      if (dfBetterDistanceFromLatestOrgInBlock==0xffffffff)
          break;

      if ((dwSizeOrgBetter>0) &&
          (dfBetterDistanceFromLatestOrgInBlock >= dfDistanceMinimalForBreakAlwaysAfterFind))
          break;

      dfValueToTestItem = pCurEvolution->dfValueToTestItem ;



      // now test position dfValueToTestPosit with with hash dfValueToTestItem
      {
        dfuLong64 dfPosTest;
        //dfuLong64 dfPosTestWithMinimalSkipforHash;
        dfuLong32 j;
        dfuLong32 i = dfValueToTestItem;
        BOOL fBreak = FALSE;
        dfuLong32 dfNeedSkip = i;
        dfuLong32 dfChainPos = dfValueToTestPosit;;

        //dfPosTestWithMinimalSkipforHash = (dfChainPos * ((dfuLong64)dfBlockCalcSize)) ;
        dfPosTest = (dfChainPos * ((dfuLong64)dfBlockCalcSize)) - i;

//+++

#define IGNORE_HASHED_FULL_FILE_
#ifndef IGNORE_HASHED_FULL_FILE
        //pHashedFullFile=NULL;
        if (pHashedFullFile!=NULL)
        {
            dfvoidp pWorkDataCurrentShiftedReduction = pCurEvolution->ReducedBlock;
            dfuLong32 dfReducedMask = pHashedFullFile->dfReducedMask;

            if (!pCurEvolution->fReduceCalcDone)
            {
                ReduceHash(pHashedFullFile,((dfbytepc)lpSeqRech)+dfValueToTestItem,
                    pHashedFullFile->dfSizeCluster,
                    pWorkDataCurrentShiftedReduction,dfSizeReducedCluster,dfNbCellReduceCompare);

                pCurEvolution->fReduceCalcDone = TRUE;
                pCurEvolution->dfNbCellReduce = dfNbCellReduceCompare;
            }

            {

                // now calculate hash
                {
                    dfvoidpc pSearchStringShiftedReduced;
                    //dfuLong32 dfDataCurrentShiftedReductionSizeInCell = (pCurEvolution->dfNbCellReduce);

                    pSearchStringShiftedReduced = (((dfbytep)pHashedFullFile->pReducedData) +
                                                (dfNbByteReduceCompare*dfChainPos));

                    #ifdef PREDEFINED_MEMCMP
                    if (dfNbByteReduceCompare==4)
                    {
                            dfuLong32 a,b;
                            a = *((dfuLong32*)pWorkDataCurrentShiftedReduction);
                            b = *((dfuLong32*)pSearchStringShiftedReduced);
                            fBreak = a != b;
                    }
                    else if (dfNbByteReduceCompare==2)
                    {
                            dfuInt16 a,b;
                            a = *((dfuInt16*)pWorkDataCurrentShiftedReduction);
                            b = *((dfuInt16*)pSearchStringShiftedReduced);
                            fBreak = a != b;
                    }
                    else
                    #endif
                    //#define USE_MEMCMP
                    #ifndef USE_MEMCMP
                    {
                        dfuLong32 l;

                        dfuLong32* pWorkDataCurrentShiftedReductionBrowse ;
                        dfuLong32* pSearchStringShiftedReducedBrowse;
                        pWorkDataCurrentShiftedReductionBrowse = (dfuLong32*)pWorkDataCurrentShiftedReduction;
                        pSearchStringShiftedReducedBrowse = (dfuLong32*)pSearchStringShiftedReduced;

                        /* very cpu intensive here */
                        for (l=0;l<dfNbCellReduceCompare;l++)
                        {
                            dfuLong32 a,b;
                            a = (*pWorkDataCurrentShiftedReductionBrowse) & (dfReducedMask);
                            b = (*pSearchStringShiftedReducedBrowse) & (dfReducedMask);
                            if (a!=b)
                            {
                                fBreak=TRUE;
                                break;
                            }
                            pSearchStringShiftedReducedBrowse = (dfuLong32*)
                                (((dfbytep)pSearchStringShiftedReducedBrowse)+pHashedFullFile->dfSizeReducedCluster);
                            pWorkDataCurrentShiftedReductionBrowse = (dfuLong32*)
                                (((dfbytep)pWorkDataCurrentShiftedReductionBrowse)+pHashedFullFile->dfSizeReducedCluster);
                        }
                    }
                    #else
                    {
                    fBreak = DfsMemcmp(pWorkDataCurrentShiftedReduction,pSearchStringShiftedReduced,
                                        dfSizeReducedCluster * dfNbCellReduceCompare) != 0;
                    }
                    #endif


#ifdef VERBOSE_BUILDEVENT
                    dfReduceCmp++;
                    if (!fBreak)
                    {
                        dfvoidp pData;
                        pData = GetOrigDataPtrpDataBySizeView(pOrg,dfPosTest,dwMinInter+dfNeedSkip);

                        dfReduceCmpOk++;


                        for (j = dfNeedSkip; j < dwMinInter+dfNeedSkip; j++)
                            {
                              if ((*(((dfbytepc) pData) + dfPosTest + j)) != (*(lpSeqRech + j)))
                              {
                                  dfReduceCmpOkFalse++;
                                  break;
                              }
                            }
                        if (j==dwMinInter+dfNeedSkip)
                            dfReduceCmpOkTrue++;
                    }
#endif
                    /*
                    if (fBreak)
                        continue;
                        */
                }

#if defined(VERBOSE_BUILDEVENT) && defined(COUNT_FAILSKIP)
                {
                    dfuLong32 l;
                    for (l=0;l<dfNeedSkip;l++)
                        if ((*(((dfbytepc) pData) + dfPosTest + l)) != (*(lpSeqRech + l)))
                        {
                            dfCountTrueCompareFailInSkip++;
                            break;
                        }
                }
#endif

            }
        }
#endif


        if (!fBreak)
        {
            dfuLong32 dfSkipBeginRech = 0;
            dfvoidp pData;
            pData = GetOrigDataPtrpDataBySizeView(pOrg,dfPosTest,dfNeedSkip+dwMaxRech);

            if (fCanSkipBeginSearch)
            {
                dfsLong32 m;
                for (m=((dfsLong32)dfNeedSkip)-1;m>=0;m--)
                {
                    if ((*(((dfbytepc) pData) + dfPosTest + m)) != (*(lpSeqRech + m)))
                        break;
                }
                dfSkipBeginRech = (dfuLong32)(m+1);

#ifdef VERBOSE_BUILDEVENT
                if (dfSkipBeginRech>0)
                  dfCountSkipNeededInTrueCompare++;

                if (dfSkipBeginRech + dwMaxRech < dwMinInter)
                    printf("skip too big\n");
#endif
            }

            {
                const dfuLong32 UNALIGNED * pDataBrowse32 = (const dfuLong32*)(((dfbytepc) (pData)) + dfPosTest + dfSkipBeginRech);
                const dfuLong32 UNALIGNED * pSeqRechBrowse32= (const dfuLong32*)(lpSeqRech+dfSkipBeginRech);

                j=0;
                /*
                if (DfsMemcmp(pDataBrowse32,pSeqRechBrowse32,dwMinInter)==0)
                {
                    j=dwMinInter;
                    pDataBrowse32+=(j / sizeof(dfuLong32));
                    pSeqRechBrowse32+=(j / sizeof(dfuLong32));
                }*/

                {
                    dfuLong32 dwMaxRechAroundLower4 ;
                    dfuLong32 dwMaxRechThis ;

                    dwMaxRechThis=dwMaxRech;

#ifdef CHECK_MAXORG_FIND
                    if (dfPosTest + dwMaxRech > dwMaxSearchOrg)
                        dwMaxRechThis = (dfuLong32)(dwMaxSearchOrg-dfPosTest);

                    if (dwMaxRechThis != dwMaxRech)
                        printf("*5\n");
#endif

                    dwMaxRechThis -= dfSkipBeginRech;
                    dwMaxRechAroundLower4 = dwMaxRechThis & 0xfffffffc;

                    for (;;)
                    {
                        if ((*pSeqRechBrowse32) != (*pDataBrowse32))
                        {
                            dfbytepc pSeqRechBrowse = (dfbytepc)pSeqRechBrowse32;
                            dfbytepc pDataBrowse = (dfbytepc)pDataBrowse32;
                            if ((*(pSeqRechBrowse)) == (*(pDataBrowse)))
                            {
                                j++;
                                if ((*(pSeqRechBrowse+1)) == (*(pDataBrowse+1)))
                                {
                                    j++;
                                    if ((*(pSeqRechBrowse+2)) == (*(pDataBrowse+2)))
                                        j++;
                                }
                            }
                            break;
                        }
                        pSeqRechBrowse32++;
                        pDataBrowse32++;
                        j+=4;

                        if (j>=dwMaxRechAroundLower4)
                        {
                            dfbytepc pSeqRechBrowse = (dfbytepc)pSeqRechBrowse32;
                            dfbytepc pDataBrowse = (dfbytepc)pDataBrowse32;
                            if (j<dwMaxRechThis)
                            if (((*(pSeqRechBrowse)) == (*(pDataBrowse))))
                            {
                                j++;
                                if (j<dwMaxRechThis)
                                if (((*(pSeqRechBrowse+1)) == (*(pDataBrowse+1))))
                                {
                                    j++;
                                    if (j<dwMaxRechThis)
                                    if (((*(pSeqRechBrowse+2)) == (*(pDataBrowse+2))))
                                        j++;
                                }
                            }
                            break;
                        }
                    }
                }
            }

#ifdef VERBOSE_BUILDEVENT
            if (pHashedFullFile!=NULL)
              if (j<(dfNbCellReduceCompare * (((pPreWkFnd->pHashedFullFile)+0)->dfSizeCluster)))
                  dfCountTrueCompareOkInCrcZone++;
            if (j >= dwMinInter)
                dfCountTrueCompareMinAcceptable++;
#endif
            /*
            if ((j >= dwMinInter) && (j >= dwSizeOrgBetter))
            {
                //if ((j > dwSizeOrgBetter) || (posOrgBetter < dwLatestOrg))
                if ((j > dwSizeOrgBetter) ||
                    ((posOrgBetter < dwLatestOrg) && (dfPosTest > posOrgBetter)) ||
                    (dfPosTest >= dwLatestOrg) && (posOrgBetter > dfPosTest))
                {
                    dwSizeOrgBetter = j;
                    posOrgBetter = dfPosTest;
                    fFound = TRUE;
                    fNbCellHashCmpRecalc=TRUE;
                    dfSkipBeginSearch = dfSkipBeginRech;
#ifdef VERBOSE_BUILDEVENT
                    dfCountTrueCompareOk++;
                    if (dfSkipBeginRech > 0)
                        dfCountTrueCompareOkWithSkip++;
#endif
                }
                // dwMaxBeginString = dwMaxSearchOrg - dwSizeMinFind;
            }
            */

            if ((j >dwSizeOrgBetter) /*&& (j >= dwSizeOrgBetter)*/)
            {
                dwSizeOrgBetter = j;
                posOrgBetter = dfPosTest;
                dfSkipBeginSearch = dfSkipBeginRech;

                #ifdef _DEBUG
                    if (posOrgBetter + dfSkipBeginSearch+dwSizeOrgBetter > pOrg->size)
                        printf("**findseq overflow\n");
                #endif

                if (dwSizeOrgBetter == dwMaxRech)
                    break;
            }
#ifdef VERBOSE_BUILDEVENT
            dfCountTrueCompare++;
#endif
        }
      }


#define REPACK_CACHE_ASH_INFO_ARRAY
      if (fBetterValueToTestIsNeg)
      {
          pCurEvolution->dfBrowseNeg--;
          if (pCurEvolution->dfBrowseNeg < pCurEvolution -> dfFirstItemPossibleCurrentHashValue)
          {
              pCurEvolution -> fBrowseNeg = FALSE;

#ifdef REPACK_CACHE_ASH_INFO_ARRAY
              if (!(pCurEvolution -> fBrowsePositive))
              {
                  CACHE_HASH_INFO* pLatest ;
                  dfNbCacheHashValue--;
                  pLatest = ((CACHE_HASH_INFO*)(((dfbytep)pBufCacheHashFatLikeSearch)+(dfNbCacheHashValue*dfCacheHashItemSize)));
				  DfsMemmove(pCurEvolution, pLatest, dfCacheHashItemSize);
              }
#endif
          }
      }
      else
      {
          pCurEvolution->dfBrowsePositive++;
          if (pCurEvolution->dfBrowsePositive > pCurEvolution -> dfLastItemPossibleCurrentHashValue)
          {
              pCurEvolution -> fBrowsePositive = FALSE;

#ifdef REPACK_CACHE_ASH_INFO_ARRAY
              if (!(pCurEvolution -> fBrowseNeg))
              {
                  CACHE_HASH_INFO* pLatest ;
                  dfNbCacheHashValue--;
                  pLatest = ((CACHE_HASH_INFO*)(((dfbytep)pBufCacheHashFatLikeSearch)+(dfNbCacheHashValue*dfCacheHashItemSize)));
				  DfsMemmove(pCurEvolution, pLatest, dfCacheHashItemSize);
              }
#endif
          }
      }


  }



/// +++

  if (dwSizeOrgBetter>=dwMinInter)
  {
    *posOrg = posOrgBetter;
    *dwSizeOrgRcp = dwSizeOrgBetter;


    if (pdfSkipBeginSearch!=NULL)
            *pdfSkipBeginSearch = dfSkipBeginSearch;

    #ifdef _DEBUG
        if (posOrgBetter + dfSkipBeginSearch+dwSizeOrgBetter > pOrg->size)
            printf("**+findseq overflow\n");
    #endif

    #ifdef VERBOSE_BUILDEVENT
    dfStepNewReturnTrue++;
    #endif

    return TRUE;
  }
  else
      return FALSE;
}

#endif

/*
nouveau FindSeqPreWK
couche 1 : selection des BLOCKHASH_FATLIKE et REDUCEDFULLFILE (optionel)
  - calcul des valeurs de reduction pour les 2
  - execution par etapes de BLOCKHASH_FATLIKE (le découpage suit le nbArray)
*/


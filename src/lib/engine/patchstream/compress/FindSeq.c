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

#define FINDSEQ_REPORT_no


#include "../common/difbasic.h"
#include "../common/difstrm.h"
#include "../common/DfsTlTyp.h"
#include "../common/difstool.h"
#include "../common/DfsOrigMemoryMap.h"
#include "FindSeq.h"
#include "FSeqRept.h"
#include "PreWkFnd.h"

#ifdef COMPAREFIND
//#include <windows. h>
#endif

#define COMPAREFIND_no
#ifdef _DEBUG
#define COMPAREFIND_nono
#endif

#ifndef local
#define local static
#endif

/*
#pragma optimiz e("agt",on)
#pragma optimiz e("gityb2",on)
*/

BOOL InitFindSeq(HFINDSEQINTL * phfsq,
                 dfuLong32 nbOrig, ORIGDATAPTR OrigDataPtr,
                 dfuLong64 dfArrayLength,dfuLong32 dfBlockCalcSize,
                 dfuLong32 dfNbHashBit,
                 dfuLong32 dfSizeClusterReduced)
{
  OpenFindSeq();
//return 1; //++++++++--------*******@@@@@@@@@@@@@@@@@@@@@@@@v

  {
    HPREWKFND hpk = BuildPreWKFnd((OrigDataPtr),
                                  (dfuLong64)OrigDataPtr->size,
                                  dfArrayLength, dfBlockCalcSize,
                                  dfNbHashBit,
                                  dfSizeClusterReduced);
    *phfsq = (HFINDSEQINTL)hpk ;
  }
  return TRUE;
}

BOOL CloseFinqSeqInternal(HFINDSEQINTL hfsq)
{
    {
      HPREWKFND hpk = (HPREWKFND)hfsq;
      FreePreWKFnd(hpk);
    }
  CloseReportFindSeq();
  return TRUE;
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
BOOL StupidFindSeq(HFINDSEQINTL hfsq,
                   dfbytep lpSeqRech, dfuLong32 dwMaxRech,
                   ORIGDATA* pOrig, dfuLong64 dwSizeOrg,
                   dfuLong64 dwBeginSearch, dfuLong64 dwMaxSearchOrg,
                   dfuLong64 dwLatestOrg, dfuLong32 dwMinInter,
                   dfuLong32 dfLengthStopSearch,
                   dfuLong32 dfAlign, dfuLong32 *dfSkipBeginSearch,
                   dfuLong64 * posOrg, dfuLong32 * dwSizeOrgRcp)
{
  dfuLong64 iCount;
  dfuLong32 j;
  dfuLong32 dwSizeMinFind;
  dfuLong64 dwMaxBeginString;
  dfuLong32 dwSizeOrgBetter;
  dfuLong64 posOrgBetter;
  dfbyte firstbyte;
  dfbytep lpFileOrg=NULL;
  dfuLong32 dfMinSpaceInView;
  dfuLong64 dfCountPosRenewView;
  //dfbytep lpFileParc, lpFileMaxBeginString;
  dwSizeMinFind = dwMinInter;

  if (dwMaxSearchOrg == 0)
    dwMaxSearchOrg = dwSizeOrg;

  if (dfAlign>1)
  {
      dfuLong32 dfModulo = (dfuLong32)(dwBeginSearch % dfAlign);
      if (dfModulo>0)
          dwBeginSearch += (dfAlign-dfModulo);
  }

  dwMaxBeginString = dwMaxSearchOrg - dwSizeMinFind;
  dwSizeOrgBetter = 0;
  posOrgBetter = 0;
  firstbyte = *lpSeqRech;
/*
  lpFileMaxBeginString = lpFileOrg + dwMaxBeginString;

  for (lpFileParc = lpFileOrg + dwBeginSearch;
       lpFileParc < lpFileMaxBeginString; lpFileParc+=dfAlign)*/
  dfMinSpaceInView = 6 + dwMaxRech + 0;
  dfCountPosRenewView = 0;
  for (iCount=0;iCount<dwMaxBeginString-dwBeginSearch;iCount+=dfAlign)
  {
    if (iCount == dfCountPosRenewView)
    {
        dfuIntPtr dfLengthPtr = (dfuIntPtr)dfmin(GetMaxOrigDataExigibleSizeView(pOrig),
                                        ((dwMaxBeginString-dwBeginSearch)+dfMinSpaceInView)-iCount);
        lpFileOrg = GetOrigDataPtrpDataBySizeView(pOrig,
                          dwBeginSearch + iCount,
                          dfLengthPtr);
        dfCountPosRenewView = ((iCount + dfLengthPtr) - dfMinSpaceInView);
    }


    if ((iCount+6) < dwMaxBeginString)
    // unroll
    {
      if ((*(lpFileOrg+dwBeginSearch+iCount)) != firstbyte)
        iCount++;
      if ((*(lpFileOrg+dwBeginSearch+iCount)) != firstbyte)
        iCount++;
      if ((*(lpFileOrg+dwBeginSearch+iCount)) != firstbyte)
        iCount++;
      if ((*(lpFileOrg+dwBeginSearch+iCount)) != firstbyte)
        iCount++;
      if ((*(lpFileOrg+dwBeginSearch+iCount)) != firstbyte)
        iCount++;
      if ((*(lpFileOrg+dwBeginSearch+iCount)) != firstbyte)
        iCount++;
    }

    if ((*(lpFileOrg+dwBeginSearch+iCount)) == firstbyte)
    {
      j = 1;
      for (;;)
      {
        if ((j < dwMaxRech)
            && (((dfuLong64)(iCount + dwBeginSearch)) + j < dwMaxSearchOrg)
            && ((*(lpFileOrg + iCount + dwBeginSearch + j)) == (*(lpSeqRech + j))))
          j++;
        else
        {
          if ((j >= dwSizeMinFind) && (j >= dwSizeOrgBetter))
          {
            if ((j > dwSizeOrgBetter) || (posOrgBetter < dwLatestOrg))
            {
              dwSizeOrgBetter = j;
              posOrgBetter = (dfuLong64)(iCount + dwBeginSearch);
            }
            dwMaxBeginString = dwMaxSearchOrg - dwSizeMinFind;
          }
          break;
        }
      }
      if ((dwSizeOrgBetter == dwMaxRech) && (posOrgBetter >= dwLatestOrg))
        break;
    }
  }
  *posOrg = posOrgBetter;
  *dwSizeOrgRcp = dwSizeOrgBetter;

  return (dwSizeOrgBetter > dwMinInter);
}


BOOL StupidFindSeqSimple(HFINDSEQINTL hfsq,
                         dfbytep lpSeqRech, dfuLong32 dwMaxRech,
                         ORIGDATA* pOrig, dfuLong64 dwSizeOrg,
                         dfuLong64 dwBeginSearch, dfuLong64 dwMaxSearchOrg,
                         dfuLong64 dwLatestOrg, dfuLong32 dwMinInter,
                         dfuLong32 dfLengthStopSearch,
                         dfuLong32 dfAlign, dfuLong32 *dfSkipBeginSearch,
                         dfuLong64 * posOrg, dfuLong32 * dwSizeOrgRcp)
{
  dfuLong32 j;
  dfuLong32 dwSizeMinFind;
  dfuLong64 dwMaxBeginString;
  dfuLong32 dwSizeOrgBetter;
  dfuLong64 posOrgBetter;
  dfbyte firstbyte;
  //dfbytep lpFileParc, lpFileMaxBeginString;
  dfbytep lpFileOrg=NULL;
  dfuLong64 dfCountPosRenewView;
  dfuLong32 dfMinSpaceInView;
  dfuLong64 iCount;
  dwSizeMinFind = dwMinInter;

  if (dwMaxSearchOrg == 0)
    dwMaxSearchOrg = dwSizeOrg;

  dwMaxBeginString = dwMaxSearchOrg - dwSizeMinFind;
  dwSizeOrgBetter = 0;
  posOrgBetter = 0;
  firstbyte = *lpSeqRech;

//  lpFileMaxBeginString = lpFileOrg + dwMaxBeginString;

  if (dfAlign>1)
  {
      dfuLong32 dfModulo = (dfuLong32)(dwBeginSearch % dfAlign);
      if (dfModulo>0)
          dwBeginSearch += (dfAlign-dfModulo);
  }


  dfMinSpaceInView = dwMaxRech + 0;
  dfCountPosRenewView = 0;
  for (iCount=0;iCount<dwMaxBeginString-dwBeginSearch;iCount+=dfAlign)
  {
    if (iCount == dfCountPosRenewView)
    {
        dfuIntPtr dfLengthPtr = (dfuIntPtr)dfmin(GetMaxOrigDataExigibleSizeView(pOrig),
                                        ((dwMaxBeginString-dwBeginSearch)+dfMinSpaceInView)-iCount);
        lpFileOrg = GetOrigDataPtrpDataBySizeView(pOrig,
                          dwBeginSearch + iCount,
                          dfLengthPtr);
        dfCountPosRenewView = ((iCount + dfLengthPtr) - dfMinSpaceInView);
    }
    if ((*(lpFileOrg+dwBeginSearch+iCount)) == firstbyte)
    {
      j = 1;
      for (;;)
      {
        if ((j < dwMaxRech)
            && (((dfuLong64)(dwBeginSearch + iCount)) + j < dwMaxSearchOrg)
            && ((*(lpFileOrg + iCount + dwBeginSearch + j)) == (*(lpSeqRech + j))))
          j++;
        else
        {
          if ((j >= dwSizeMinFind) && (j >= dwSizeOrgBetter))
          {

            if ((j > dwSizeOrgBetter) || (posOrgBetter < dwLatestOrg))
            {
              dwSizeOrgBetter = j;
              posOrgBetter = (dfuLong64)(dwBeginSearch+iCount);
            }
            dwMaxBeginString = dwMaxSearchOrg - dwSizeMinFind;
          }
          break;
        }
      }
      if ((dwSizeOrgBetter == dwMaxRech) && (posOrgBetter >= dwLatestOrg))
        break;
    }
  }
  *posOrg = posOrgBetter;
  *dwSizeOrgRcp = dwSizeOrgBetter;

  return (dwSizeOrgBetter > dwMinInter);
}

local void FillTable(dfuLong32 * lpdwTable, dfbytep lpSeqRech,
                     dfuLong32 dwSizeRech)
{
  dfuLong32 i;

  for (i = 0; i < 0x100; i++)
    *(lpdwTable + i) = dwSizeRech;
  for (i = 0; i < dwSizeRech; i++)
    *(lpdwTable + *(lpSeqRech + i)) = dwSizeRech - (i + 1);
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
BOOL IntelligentFindSeq(HFINDSEQINTL hfsq,
                        dfbytep lpSeqRech, dfuLong32 dwMaxRech,
                        ORIGDATA* pOrig, dfuLong64 dwSizeOrg,
                        dfuLong64 dwBeginSearch, dfuLong64 dwMaxSearchOrg,
                        dfuLong64 dwLatestOrg, dfuLong32 dwMinInter,
                        dfuLong32 dfLengthStopSearch,
                        dfuLong32 dfAlign, dfuLong32 *dfSkipBeginSearch,
                        dfuLong64 * posOrg, dfuLong32 * dwSizeOrgRcp)
{
  dfuLong64 i;
  dfuLong32 posSeq = 0;
  dfuLong64 posOrgBetter = 0;
  dfuLong32 dwSizeOrgBetter;
  dfuLong32 dwSizeOrgBetterTable;
  dfuLong32 dwTable[0x100];
  dfuLong32 stop_verify;
  dfuLong64 dfCountPosRenewView;
  dfuLong32 dfMinSpaceInView;
  dfuLong32 dfMinimalSpaceInViewForSpeedSearch=0;
  dfbytep lpfoorg=NULL;
  dfbytep lpFileOrg=NULL;
  dfbytep lpfoOrgLim;
  dfbytep lpEndView=NULL;
  BOOL fFullView=FALSE;
  BOOL fReadaptFileView=TRUE;

#ifdef BOUND_CHECKER
  {
    dfuLong32 i;
    dfbyte vch = 0;
    for (i = 0; i < dwSizeOrg; i++)
      vch += *(lpFileOrg + i);
  }
#endif

  if (dwMinInter > dwMaxRech)
  {
    //printf("\n\nerrinter\n");
    return FALSE;
  }
  dwSizeOrgBetter = dwSizeOrgBetterTable = dwMinInter;
  //memset(dwTable,0,0x100*sizeof(dfuLong32));
  //for (i=0;i<MININTER;i++) bTable[*(lpSeqRech+i)]=1;


  //if (dwMaxRech>2) dwMaxRech-=2;// bug to be fixed
  if (dwMaxRech < dwMinInter)
    return FALSE;

  BeginFindSeq();
  FillTable(dwTable, lpSeqRech, dwSizeOrgBetterTable);

  if (dwMaxSearchOrg == 0)
    dwMaxSearchOrg = dwSizeOrg;

  if (dfAlign>1)
  {
      dfuLong32 dfModulo = (dfuLong32)(dwBeginSearch % dfAlign);
      if (dfModulo>0)
          dwBeginSearch += (dfAlign-dfModulo);
  }

  dfMinSpaceInView = dwSizeOrgBetterTable;
  dfCountPosRenewView = dwBeginSearch;
  lpfoOrgLim=NULL;

  for (i = dwBeginSearch; i < dwMaxSearchOrg; i++)
  {
    dfuLong64 dwA = dwSizeOrg - dwSizeOrgBetterTable;

    //if (i >= dfCountPosRenewView)
    //if ((lpfoOrgLim == NULL) || (lpfo>=lpfoOrgLim))
    if (fReadaptFileView)
    {
        dfuIntPtr dfLengthPtr ;
        fFullView = (dwMaxSearchOrg-i) <= GetMaxOrigDataExigibleSizeView(pOrig);
        if (fFullView)
            dfLengthPtr = (dfuIntPtr)(dwMaxSearchOrg-i);
        else
            dfLengthPtr = GetMaxOrigDataExigibleSizeView(pOrig);

        lpFileOrg = GetOrigDataPtrpDataBySizeView(pOrig, i, dfLengthPtr);
        lpEndView = lpFileOrg + i + dfLengthPtr;
        dfMinimalSpaceInViewForSpeedSearch = (9 * dwSizeOrgBetterTable)+dwMaxRech;
        //lpfoOrgLim = lpFileOrg+dfMin(MaxSearchOrg,i+dfLengthPtr)- ((9 * dwSizeOrgBetterTable) + dwMaxRech);
        dfCountPosRenewView = (i + dfLengthPtr) - dfMinSpaceInView;
        lpfoorg = lpFileOrg + dwSizeOrgBetterTable - 1;
        fReadaptFileView=FALSE;
    }


    // boyer moor speeding
    if ((i - 1 < dwA) && (posSeq == 0))
    {
      //dfuLong64 lLim = dwMaxSearchOrg - 9 * dwSizeOrgBetterTable;
      dfuLong32 dwLa = 1;
      dfbytep lpfo;


      lpfo = lpfoorg + i;


      //while ((((dfuLong64)(lpfo - lpfoorg)) < lLim) && (dwLa > 0))
      //while ((((lpfo )) < lpfoOrgLim) && (dwLa > 0))
      while (((lpfo + dfMinimalSpaceInViewForSpeedSearch) < lpEndView) && (dwLa > 0))
      {
        dwLa = dwTable[*(lpfo)];
         /**/ if (dwLa == 0)
          break;
        lpfo += dwLa;

        dwLa = dwTable[*(lpfo)];
        if (dwLa == 0)
          break;
        lpfo += dwLa;

        dwLa = dwTable[*(lpfo)];
        //**/if (dwLa==0) break;
        lpfo += dwLa;

        dwLa = dwTable[*(lpfo)];
        if (dwLa == 0)
          break;
        lpfo += dwLa;

#ifdef BOUND_CHECKER
        {
          dfbyte lpfov = *(lpfo);
          lpfov = lpfov * 1;
        }
#endif
        dwLa = dwTable[*(lpfo)];
        //**/if (dwLa==0) break;
        lpfo += dwLa;

        dwLa = dwTable[*(lpfo)];
        if (dwLa == 0)
          break;
        lpfo += dwLa;

        dwLa = dwTable[*(lpfo)];
        //if (dwLa==0) break;
        lpfo += dwLa;

        dwLa = dwTable[*(lpfo)];
        //if (dwLa==0) break;
        lpfo += dwLa;
      }
      i = (dfuLong32)(lpfo - lpfoorg);



      if (dwLa > 0)
      {
          if (!fFullView)
          {
              fReadaptFileView=TRUE;
              continue;
          }

          while ((dwLa =
                    dwTable[*(lpFileOrg + i + dwSizeOrgBetterTable - 1)]) != 0)
            if (((i + dwLa < dwA)))
                i += dwLa;
            else
                break;
      }
    }

    stop_verify = 0;

    if ((*(lpFileOrg + i) == *(lpSeqRech + posSeq)) && (posSeq < dwMaxRech))
      posSeq++;
    else
      stop_verify = 1;


    if (stop_verify != 0)
      if (posSeq > 0)
      {
        dfuLong32 dfAlignOk = 1;
        if (dfAlign>1)
            if (((i - posSeq) % dfAlign)>0)
                dfAlignOk = 0;

        if (dfAlignOk==1)
        {
            if (posSeq > dwSizeOrgBetter)
            {
                dwSizeOrgBetter = posSeq;
                posOrgBetter = i - posSeq;
                dwSizeOrgBetterTable = dwSizeOrgBetter;
                //lpfoorg = lpFileOrg + dwSizeOrgBetterTable - 1;
                lpfoOrgLim=NULL;
                FillTable(dwTable, lpSeqRech, dwSizeOrgBetterTable);
                fReadaptFileView=TRUE;
            }
            else if ((posSeq == dwSizeOrgBetter) && (posOrgBetter < dwLatestOrg))
            {
                posOrgBetter = i - posSeq;
                if ((posOrgBetter >= dwLatestOrg) && (posSeq == dwMaxRech))
                    break;
            }
        }
        i -= posSeq - 0;

        posSeq = 0;
      }
  }



  if (posSeq > dwSizeOrgBetter)
  {
    if (((i - posSeq) % dfAlign)==0)
    {
      dwSizeOrgBetter = posSeq;
      posOrgBetter = i - posSeq;
    }
  }
  else
      if ((posSeq == dwSizeOrgBetter) && (posOrgBetter < dwLatestOrg))
         if (((i - posSeq) % dfAlign)==0)
            posOrgBetter = i - posSeq;

#if (defined (DEBUG) || defined (_DEBUG)) && defined(CHECK_CMP)
  if (dwSizeOrgBetter > dwMinInter)
  {
    //if (memcmp(posOrgBetter + lpFileOrg, lpSeqRech, dwSizeOrgBetter) != 0)
    if (memcmp(((dfbytep)GetOrigDataPtrpDataBySizeView(pOrig,posOrgBetter,dwSizeOrgBetter))+posOrgBetter, lpSeqRech, dwSizeOrgBetter) != 0)
      printf("error_\t");

    if (dwSizeOrgBetter > dwMaxRech)
    {
      dwSizeOrgBetter = dwMaxRech;
      printf("ERROR\t");
    }

    //if (memcmp(posOrgBetter + lpFileOrg, lpSeqRech, dwSizeOrgBetter) != 0)
    if (memcmp(((dfbytep)GetOrigDataPtrpDataBySizeView(pOrig,posOrgBetter,dwSizeOrgBetter))+posOrgBetter, lpSeqRech, dwSizeOrgBetter) != 0)
      printf("error*\t");
  }
#endif

  *posOrg = posOrgBetter;
  *dwSizeOrgRcp = dwSizeOrgBetter;
/*
  if ((dwSizeOrgBetter>dwMinInter) && ((*lpSeqRech) != (*(lpFileOrg+(*posOrg)))))
      puts("bug");
*/
  ReportFindSeq("FindSeq", dwBeginSearch, dwMaxSearchOrg, dwMinInter,
                dwSizeOrg, dwLatestOrg);
  return (dwSizeOrgBetter > dwMinInter);
}

#if defined (_DEBUG) && defined (COMPAREFIND)
#include <stdio.h>
BOOL FindSeq(HFINDSEQINTL hfsq,
             dfbytep lpSeqRech, dfuLong32 dwMaxRech,
             dfbytep lpFileOrg, dfuLong32 dwSizeOrg,
             dfuLong32 dwBeginSearch, dfuLong32 dwMaxSearchOrg,
             dfuLong32 dwLatestOrg, dfuLong32 dwMinInter,
             dfuLong32 dfLengthStopSearch,
             dfuLong32 dfAlign, dfuLong32 *dfSkipBeginSearch,
             dfuLong32 * posOrg, dfuLong32 * dwSizeOrgRcp)
{
  BOOL fRet1, fRet2 , fRet3;
  dfuLong32 posOrg1;
  dfuLong32 dwSizeOrgRcp1;
  dfuLong32 posOrg2;
  dfuLong32 dwSizeOrgRcp2;
  dfuLong32 posOrg3;
  dfuLong32 dwSizeOrgRcp3;


  dfSkipBeginSearch = NULL;

  posOrg1=dwSizeOrgRcp1=0;
  posOrg2=dwSizeOrgRcp2=0;
  posOrg3=dwSizeOrgRcp3=0;

  fRet1 = IntelligentFindSeq(hfsq,
                             lpSeqRech, dwMaxRech,
                             lpFileOrg, dwSizeOrg,
                             dwBeginSearch, dwMaxSearchOrg,
                             dwLatestOrg, dwMinInter,
                             dfLengthStopSearch,
                             dfAlign, dfSkipBeginSearch,
                             &posOrg1,
                             &dwSizeOrgRcp1);
  fRet2 =
    StupidFindSeq(hfsq, lpSeqRech, dwMaxRech, lpFileOrg, dwSizeOrg,
                  dwBeginSearch, dwMaxSearchOrg, dwLatestOrg, dwMinInter,
                  dfLengthStopSearch,
                  dfAlign, dfSkipBeginSearch,
                  &posOrg2, &dwSizeOrgRcp2);

  if ((fRet1 != fRet2) ||
      (fRet1) && ((posOrg1 != posOrg2) || (dwSizeOrgRcp1 != dwSizeOrgRcp2)))
  {
    printf("\n*\n************ERROR\n*\n");
    //OutputDebugString("\n\n***error ret \n");
  }


  if (hfsq!=NULL)
  {
      BOOL fEnough=FALSE;
      posOrg3=dwSizeOrgRcp3=0;
      fRet3 =
        FindSeqPreWK((HPREWKFND)hfsq, lpSeqRech, dwMaxRech, lpFileOrg, dwSizeOrg,
                      dwBeginSearch, dwMaxSearchOrg, dwLatestOrg, dwMinInter,
                      dfLengthStopSearch,
                      dfAlign, dfSkipBeginSearch,
                      &posOrg3, &dwSizeOrgRcp3,&fEnough);
      if ((fRet1 == fRet3) &&
          (fRet1) && ((posOrg1 == posOrg3) && (dwSizeOrgRcp1 == dwSizeOrgRcp3)))
          printf(".");

      if ((fRet1 != fRet3) ||
          (fRet1) && ((posOrg1 != posOrg3) || (dwSizeOrgRcp1 != dwSizeOrgRcp3)))
      {
        printf("\n*\n*++++++++*ERROR\n*\n");
      //OutputDebugString("\n\n***error val \n");

      fRet3 =
        FindSeqPreWK((HPREWKFND)hfsq, lpSeqRech, dwMaxRech, lpFileOrg, dwSizeOrg,
                      dwBeginSearch, dwMaxSearchOrg, dwLatestOrg, dwMinInter,
                      dfLengthStopSearch,
                      dfAlign, dfSkipBeginSearch,
                      &posOrg3, &dwSizeOrgRcp3,&fEnough);
      }
  }
  if (dfAlign>1)
      if ((posOrg1 % dfAlign) != 0)
          printf("align problem\n");

  *posOrg = posOrg1;
  *dwSizeOrgRcp = dwSizeOrgRcp1;
  return fRet1;
}

#else

BOOL FindSeq(HFINDSEQINTL hfsq,
             dfbytep lpSeqRech, dfuLong32 dwMaxRech,
             ORIGDATA* pOrig, dfuLong64 dwSizeOrg,
             dfuLong64 dwBeginSearch, dfuLong64 dwMaxSearchOrg,
             dfuLong64 dwLatestOrg, dfuLong32 dwMinInter,
             dfuLong32 dfLengthStopSearch,
             dfuLong32 dfAlign, dfuLong32 *dfSkipBeginSearch,
             dfuLong64 * posOrg, dfuLong32 * dwSizeOrgRcp)
{
  BOOL fRet;
  BOOL fUseIntelligentFindSeq ;

  //dfuLong64 dfSizeSearch = ((dwMaxSearchOrg == 0) ? dwMaxSearchOrg : dwSizeOrg) - dwBeginSearch;
  /*
  if (dfSizeSearch > 0x500)
    fUseIntelligentFindSeq = (dwMinInter<GetMinimalInterPreWkSrch((HPREWKFND)hfsq));
  else
    fUseIntelligentFindSeq = 01;
*/
  dfuLong32 minimalInterPreWkSrch = GetMinimalInterPreWkSrch((HPREWKFND)hfsq);
  fUseIntelligentFindSeq = (dwMinInter < minimalInterPreWkSrch);
//fUseIntelligentFindSeq=1;
  if (fUseIntelligentFindSeq)
      fRet = IntelligentFindSeq(hfsq,
//      fRet = StupidFindSeqSimple(hfsq,
                            lpSeqRech, dwMaxRech,
                            pOrig, dwSizeOrg,
                            dwBeginSearch, dwMaxSearchOrg,
                            dwLatestOrg, dwMinInter,
                            dfLengthStopSearch,
                            dfAlign, dfSkipBeginSearch,
                            posOrg, dwSizeOrgRcp);
  else
  {
  fRet = FindSeqPreWKS((HPREWKFND)hfsq,
                            lpSeqRech, dwMaxRech,
                            pOrig, dwSizeOrg,
                            dwBeginSearch, dwMaxSearchOrg,
                            dwLatestOrg, dwMinInter,
                            dfLengthStopSearch,
                            dfAlign, dfSkipBeginSearch,
                            posOrg, dwSizeOrgRcp);
  /* bis */
  /*
  fRet = FindSeqPreWKS((HPREWKFND)hfsq,
                            lpSeqRech, dwMaxRech,
                            pOrig, dwSizeOrg,
                            dwBeginSearch, dwMaxSearchOrg,
                            dwLatestOrg, dwMinInter,
                            dfLengthStopSearch,
                            dfAlign, dfSkipBeginSearch,
                            posOrg, dwSizeOrgRcp);
							*/
  }
#if (defined (DEBUG) || defined (_DEBUG)) && defined(CHECK_CMP)
  if ((fRet) && ((*dwSizeOrgRcp)>0))
  {
    dfuLong32 dfSkipBeginSearchFound = 0;
    if (dfSkipBeginSearch!=NULL)
        dfSkipBeginSearchFound = *dfSkipBeginSearch;
    if (memcmp((((dfbytep)GetOrigDataPtrpDataBySizeView(pOrig,
                 (*posOrg)+dfSkipBeginSearchFound,*dwSizeOrgRcp))+(*posOrg)+dfSkipBeginSearchFound),
               lpSeqRech+dfSkipBeginSearchFound, (*dwSizeOrgRcp)) != 0)
      printf("error_\t");
  }
#endif
  return fRet;
}

#endif

#pragma optimize( "", on )

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
#include <string.h>


#include "difbasic.h"
#include "ltoolsc.h"


typedef struct
{
  dfuLong32 dwNbUsed;
  dfuLong32 dwNbAlloc;
  dfuLong32 dwSizeElem;
  dfuLong32 dwStepAlloc;
  LPBYTE lpData;
  tCompareDataC fCompareData;
  tDestructorDataC fDestructorData;
} STATIC_ARRAY_INTERNAL;

STATIC_ARRAY_C InitStaticArray_C(dfuLong32 dwSizeElem, dfuLong32 dwStepAllocNew)
{
  STATIC_ARRAY_INTERNAL* pSAi = (STATIC_ARRAY_INTERNAL*)malloc(sizeof(STATIC_ARRAY_INTERNAL));
  if (pSAi == NULL)
    return NULL;
  pSAi->dwNbUsed = pSAi->dwNbAlloc = 0;
  pSAi->dwSizeElem = dwSizeElem;
  pSAi->dwStepAlloc = dwStepAllocNew;
  pSAi->lpData = NULL;
  pSAi->fCompareData = NULL;
  pSAi->fDestructorData = NULL;

  return (STATIC_ARRAY_C)pSAi;
}

void DeleteStaticArray_C(STATIC_ARRAY_C sac)
{
  if (sac == NULL)
    return;

  STATIC_ARRAY_INTERNAL* pSAi = (STATIC_ARRAY_INTERNAL*)sac;
  dfuLong32 i;
  if (pSAi->fDestructorData != NULL)
    for (i = 0; i < pSAi->dwNbUsed; i++)
    {
      const void* ptrItem = GetElemPtrSA(sac, i);
      (*(pSAi->fDestructorData))(ptrItem);
    }
  if (pSAi->lpData != NULL)
  {
    free(pSAi->lpData);
  }
  free(pSAi);
    //STATIC_ARRAY* psa = (STATIC_ARRAY*)sac;
    //delete(psa);
}

static BOOL SetNbAlloc(STATIC_ARRAY_INTERNAL* pSAi, dfuLong32 dwNbAllocNew)
{
  BOOL fRes;
  dfuLong32 dwNewSize = (dwNbAllocNew*pSAi->dwSizeElem) + 0x10;
  if (pSAi->lpData == NULL)
  {
    // + 0x10 : when dwNbAllocNew==0 we will not have NULL
    pSAi->lpData = (LPBYTE)malloc(dwNewSize);
    fRes = (pSAi->lpData != NULL);
  }
  else
  {
    LPBYTE lpNewData = (LPBYTE)realloc(pSAi->lpData, dwNewSize);
    fRes = (lpNewData != NULL);
    if (fRes)
      pSAi->lpData = lpNewData;
  }
  if (fRes)
    pSAi->dwNbAlloc = dwNbAllocNew;
  return fRes;
}

static BOOL CheckNbAlloc(STATIC_ARRAY_INTERNAL* pSAi, dfuLong32 dwNbAllocNeeded)
{
  dfuLong32 dwNbExactAlloc;
  BOOL fRes = TRUE;
  dwNbExactAlloc = ((dwNbAllocNeeded + pSAi->dwStepAlloc - 1) / pSAi->dwStepAlloc)*pSAi->dwStepAlloc;
  if (dwNbExactAlloc != pSAi->dwNbAlloc)
    fRes = SetNbAlloc(pSAi,dwNbExactAlloc);
  return fRes;
}

BOOL ReservAllocationSA(STATIC_ARRAY_C sac,dfuLong32 dwNbReserv)
{
  STATIC_ARRAY_INTERNAL* pSAi = (STATIC_ARRAY_INTERNAL*)sac;

  BOOL fRet = TRUE;
  if (dwNbReserv > pSAi->dwNbAlloc)
    fRet = CheckNbAlloc(pSAi,dwNbReserv);
  return fRet;
}

dfuLong32 GetNbElemSA(STATIC_ARRAY_C sac)
{
  STATIC_ARRAY_INTERNAL* pSAi = (STATIC_ARRAY_INTERNAL*)sac;
    return pSAi->dwNbUsed;
}

BOOL InitStaticArraySA(STATIC_ARRAY_C sac,
                    dfuLong32 dwSizeElemNew,
                    dfuLong32 dwStepAllocNew)
{
  STATIC_ARRAY_INTERNAL* pSAi = (STATIC_ARRAY_INTERNAL*)sac;
    //STATIC_ARRAY* psa = (STATIC_ARRAY*)sac;
    //return psa->InitStaticArray(dwSizeElem,dwStepAlloc);

    BOOL fRes = FALSE;
    if (pSAi->dwNbAlloc == 0)
    {
      pSAi->dwSizeElem = dwSizeElemNew;
      pSAi->dwStepAlloc = dwStepAllocNew;
      fRes = TRUE;
      //if (dwFirstAlloc>0)
      //  fRes = ReservAllocationSA(dwFirstAlloc);
    }
    return fRes;
}

BOOL DeleteElemSA(STATIC_ARRAY_C sac, dfuLong32 dwPos, dfuLong32 dwNbElem)
{
  STATIC_ARRAY_INTERNAL* pSAi = (STATIC_ARRAY_INTERNAL*)sac;

  BOOL fRes = FALSE;
  if (dwPos + dwNbElem <= pSAi->dwNbUsed)
  {
    dfuLong32 i;
    if (pSAi->fDestructorData != NULL)
      for (i = dwPos; i < dwPos + dwNbElem; i++)
      {
        const void* ptrItem = GetElemPtrSA(sac, i);
        (*(pSAi->fDestructorData))(ptrItem);
      }

    memmove(pSAi->lpData + ((dwPos)*pSAi->dwSizeElem), pSAi->lpData + ((dwPos + dwNbElem)*pSAi->dwSizeElem),
      (pSAi->dwNbUsed - (dwPos + dwNbElem))*pSAi->dwSizeElem);
    pSAi->dwNbUsed -= dwNbElem;
    fRes = CheckNbAlloc(pSAi,pSAi->dwNbUsed);
  }

  return fRes;
}



BOOL SetElemSA(STATIC_ARRAY_C sac, dfuLong32 dwPos, LPCVOID lpDataElem)
{
  STATIC_ARRAY_INTERNAL* pSAi = (STATIC_ARRAY_INTERNAL*)sac;
  BOOL fRes = dwPos < pSAi->dwNbUsed;
  if (dwPos == pSAi->dwNbUsed)
    if (CheckNbAlloc(pSAi,pSAi->dwNbUsed + 1))
    {
      pSAi->dwNbUsed++;
      fRes = TRUE;
    }

  if (fRes)
  {
    memcpy(pSAi->lpData + (dwPos*pSAi->dwSizeElem), lpDataElem, pSAi->dwSizeElem);
  }
  return fRes;
}

BOOL InsertElemSA(STATIC_ARRAY_C sac, dfuLong32 dwPos, LPCVOID lpDataElem)
{
  STATIC_ARRAY_INTERNAL* pSAi = (STATIC_ARRAY_INTERNAL*)sac;

    BOOL fRes = FALSE;
    if (dwPos <= pSAi->dwNbUsed)
      if (CheckNbAlloc(pSAi,pSAi->dwNbUsed + 1))
      {
        memmove(pSAi->lpData + ((dwPos + 1)*pSAi->dwSizeElem), pSAi->lpData + (dwPos*pSAi->dwSizeElem),
          (pSAi->dwNbUsed - dwPos)*pSAi->dwSizeElem);
        pSAi->dwNbUsed++;
        fRes = SetElemSA(sac, dwPos, lpDataElem);
      }
    return fRes;
}


BOOL GetElemSA(STATIC_ARRAY_C sac, dfuLong32 dwPos, LPVOID lpDataElem)
{
  STATIC_ARRAY_INTERNAL* pSAi = (STATIC_ARRAY_INTERNAL*)sac;
  BOOL fRes = dwPos < pSAi->dwNbUsed;
  if (fRes)
  {
    memcpy(lpDataElem, pSAi->lpData + (dwPos*pSAi->dwSizeElem), pSAi->dwSizeElem);
  }
  return fRes;
}

LPCVOID GetElemPtrSA(STATIC_ARRAY_C sac, dfuLong32 dwPos)
{

  STATIC_ARRAY_INTERNAL* pSAi = (STATIC_ARRAY_INTERNAL*)sac;
  LPCVOID lpRet = NULL;
  if (dwPos < pSAi->dwNbUsed)
  {
    lpRet = pSAi->lpData + (dwPos*pSAi->dwSizeElem);
  }
  return lpRet;
}

BOOL AddEndElemSA(STATIC_ARRAY_C sac, LPCVOID lpDataElem)
{
  STATIC_ARRAY_INTERNAL* pSAi = (STATIC_ARRAY_INTERNAL*)sac;

  BOOL fRes = FALSE;
  if (CheckNbAlloc(pSAi, pSAi->dwNbUsed + 1))
  {
    pSAi->dwNbUsed++;
    fRes = SetElemSA(sac,pSAi->dwNbUsed - 1, lpDataElem);
  }
  return fRes;
}

BOOL SetFuncCompareDataSA(STATIC_ARRAY_C sac, tCompareDataC fCompareDataSet)
{
  STATIC_ARRAY_INTERNAL* pSAi = (STATIC_ARRAY_INTERNAL*)sac;

  BOOL fRet = (pSAi->dwNbUsed<2);
  if (fRet)
    pSAi->fCompareData = fCompareDataSet;
  return fRet;
}

BOOL SetFuncDestructorSA(STATIC_ARRAY_C sac, tDestructorDataC fDestructorDataSet)
{
  STATIC_ARRAY_INTERNAL* pSAi = (STATIC_ARRAY_INTERNAL*)sac;

  BOOL fRet = TRUE;
  pSAi->fDestructorData = fDestructorDataSet;
  return fRet;
}

#ifndef lt_min
#define lt_min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

BOOL InsertSortedSA(STATIC_ARRAY_C sac, LPCVOID lpDataElem)
{
  STATIC_ARRAY_INTERNAL* pSAi = (STATIC_ARRAY_INTERNAL*)sac;
    //STATIC_ARRAY* psa = (STATIC_ARRAY*)sac;
    //return psa->InsertSortedSA(lpData);
  BOOL fRet;
  if ((pSAi->fCompareData == NULL) || (pSAi->dwNbUsed == 0))
    fRet = AddEndElemSA(sac,lpDataElem);
  else
  {
    dfuLong32 dwStep = pSAi->dwNbUsed / 2;
    dfuLong32 i = 0;
    BOOL fContinueLoop = TRUE;

    do
    {
      dfuLong32 dwTest = lt_min(pSAi->dwNbUsed - 1, i + dwStep);

      if ((*(pSAi->fCompareData))(lpDataElem, GetElemPtrSA(sac,dwTest)) >= 0)
        i = dwTest + 1;
      if (i == pSAi->dwNbUsed + 1)
        fContinueLoop = FALSE;

      if (dwStep>0)
        dwStep = (dwStep) / 2;
      else
        fContinueLoop = FALSE;
    } while (fContinueLoop);

    fRet = InsertElemSA(sac, i, lpDataElem);
  }
  return fRet;
}

BOOL FindSameElemPosSA(STATIC_ARRAY_C sac, LPCVOID lpDataElem, dfuLong32* lpdwPos)
{
  STATIC_ARRAY_INTERNAL* pSAi = (STATIC_ARRAY_INTERNAL*)sac;
   // STATIC_ARRAY* psa = (STATIC_ARRAY*)sac;
   // return psa->FindSameElemPosSA(lpDataElem,lpdwPos);
  BOOL fRet = FALSE;
  if ((pSAi->fCompareData != NULL) && (pSAi->dwNbUsed>0))
  {
    dfuLong32 dwStep = pSAi->dwNbUsed / 2;
    dfuLong32 i = 0;
    BOOL fContinueLoop = TRUE;

    do
    {
      dfuLong32 dwTest = lt_min(pSAi->dwNbUsed - 1, i + dwStep);

      long iComp = (*(pSAi->fCompareData))(lpDataElem, GetElemPtrSA(sac,dwTest));

      if (iComp == 0)
      {
        i = dwTest;
        fContinueLoop = FALSE;
      }

      if (iComp>0)
        i = dwTest + 1;

      if (i == pSAi->dwNbUsed + 1)
        fContinueLoop = FALSE;

      if (dwStep>0)
        dwStep = (dwStep) / 2;
      else
        fContinueLoop = FALSE;
    } while (fContinueLoop);

    i = lt_min(pSAi->dwNbUsed - 1, i);
    fRet = ((*(pSAi->fCompareData))(lpDataElem, GetElemPtrSA(sac,i))) == 0;
    if (fRet && (lpdwPos != NULL))
      *lpdwPos = i;
  }
  return fRet;
}

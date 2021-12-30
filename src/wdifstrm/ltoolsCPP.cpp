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

//#include <windows. h>
//#include <windowsx.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../lib/engine/patchstream/common/difbasic.h"
#include "ltoolsCPP.h"

#define STEPALLOC (0x10)




BOOL STATIC_ARRAY::InitStaticArray(dfuLong32 dwSizeElemNew,dfuLong32 dwStepAllocNew, dfuLong32 dwFirstAlloc)
{
    BOOL fRes = FALSE;
    if (dwNbAlloc == 0)
    {
        dwSizeElem = dwSizeElemNew ;
        dwStepAlloc = dwStepAllocNew ;
        fRes = TRUE;
        if (dwFirstAlloc>0)
          fRes = ReservAllocationSA(dwFirstAlloc);
    }
    return fRes;
}

STATIC_ARRAY::STATIC_ARRAY(dfuLong32 dwSizeElemNew,dfuLong32 dwStepAllocNew,dfuLong32 dwFirstAlloc) :
    dwNbUsed(0),
    dwNbAlloc(0),
    dwSizeElem(dwSizeElemNew),
    dwStepAlloc(dwStepAllocNew),
    lpData(NULL),
    fCompareData(NULL),
    fDestructorData(NULL)
{
  if (dwFirstAlloc>0)
    ReservAllocationSA(dwFirstAlloc);
}

BOOL STATIC_ARRAY::ReservAllocationSA(dfuLong32 dwNbReserv)
{
  BOOL fRet = TRUE;
  if (dwNbReserv > dwNbAlloc)
    fRet = CheckNbAlloc(dwNbReserv);
  return fRet;
}

BOOL STATIC_ARRAY::SetFuncCompareDataSA(tCompareData fCompareDataSet, BOOL fForce)
{
    BOOL fRet = (dwNbUsed<2) || fForce;
    if (fRet)
        fCompareData=fCompareDataSet;
    return fRet;
}

BOOL STATIC_ARRAY::SetFuncDestructorSA(tDestructorData fDestructorDataSet)
{
    fDestructorData = fDestructorDataSet;
    return TRUE;
}

/*
// slow version
BOOL STATIC_ARRAY::InsertSortedSA(LPCVOID lpDataElem)
{
    BOOL fRet;
    if (fCompareData==NULL)
        fRet = AddEndElemSA(lpDataElem);
    else
    {
        dfuLong32 i=0;
        BOOL fBreak=FALSE;
        while ((i<dwNbUsed) && (!fBreak))
        {
            if ((*fCompareData)(lpDataElem,GetElemPtrSA(i))<=0)
                fBreak=TRUE;
            else
                i++;
        }
        fRet=InsertElemSA(i,lpDataElem);
    }
    return fRet;
}
*/


BOOL STATIC_ARRAY::InsertSortedSA(LPCVOID lpDataElem)
{
    BOOL fRet;
    if ((fCompareData==NULL) || (dwNbUsed==0))
        fRet = AddEndElemSA(lpDataElem);
    else
    {
        dfuLong32 dwStep = dwNbUsed/2;
        dfuLong32 i=0;
        BOOL fContinueLoop=TRUE;

        do
        {
            dfuLong32 dwTest=min(dwNbUsed-1,i+dwStep);

            if ((*fCompareData)(lpDataElem,GetElemPtrSA(dwTest))>=0)
                i=dwTest+1;
            if (i==dwNbUsed+1)
                fContinueLoop=FALSE;

            if (dwStep>0)
                dwStep = (dwStep)/2;
            else
                fContinueLoop=FALSE;
        } while (fContinueLoop);

        fRet=InsertElemSA(i,lpDataElem);
    }
    return fRet;
}

BOOL STATIC_ARRAY::FindSameElemPosSA(LPCVOID lpDataElem,dfuLong32* lpdwPos) const
{
    BOOL fRet=FALSE;
    if ((fCompareData!=NULL) && (dwNbUsed>0))
    {
        dfuLong32 dwStep = dwNbUsed/2;
        dfuLong32 i=0;
        BOOL fContinueLoop=TRUE;

        do
        {
            dfuLong32 dwTest=min(dwNbUsed-1,i+dwStep);

            long iComp=(*fCompareData)(lpDataElem,GetElemPtrSA(dwTest));

            if (iComp==0)
            {
                i=dwTest;
                fContinueLoop=FALSE;
            }

            if (iComp>0)
                i=dwTest+1;

            if (i==dwNbUsed+1)
                fContinueLoop=FALSE;

            if (dwStep>0)
                dwStep = (dwStep)/2;
            else
                fContinueLoop=FALSE;
        } while (fContinueLoop);

        i=min(dwNbUsed-1,i);
        fRet=((*fCompareData)(lpDataElem,GetElemPtrSA(i)))==0;
        if (fRet && (lpdwPos!=NULL))
            *lpdwPos=i;
    }
    return fRet;
}

STATIC_ARRAY::~STATIC_ARRAY()
{
    dfuLong32 i;
    if (fDestructorData!=NULL)
        for (i=0;i<dwNbUsed;i++)
            (*fDestructorData)(GetElemPtrSA(i));
    if (lpData != NULL)
        free(lpData);
}

dfuLong32 STATIC_ARRAY::GetNbElemSA() const
{
    return dwNbUsed;
}

BOOL STATIC_ARRAY::DeleteElemSA(dfuLong32 dwPos,dfuLong32 dwNbElem)
{
    BOOL fRes = FALSE;
    if (dwPos+dwNbElem <= dwNbUsed)
    {
        dfuLong32 i;
        if (fDestructorData!=NULL)
            for (i=dwPos;i<dwPos+dwNbElem;i++)
                (*fDestructorData)(GetElemPtrSA(i));
        memmove(lpData+((dwPos)*dwSizeElem),lpData+((dwPos+dwNbElem)*dwSizeElem),
                        (dwNbUsed-(dwPos+dwNbElem))*dwSizeElem);
        dwNbUsed -= dwNbElem;
        fRes = CheckNbAlloc(dwNbUsed);
    }
    return fRes;
}

BOOL STATIC_ARRAY::SetElemSA(dfuLong32 dwPos, LPCVOID lpDataElem)
{
    BOOL fRes = dwPos < dwNbUsed ;
    if (dwPos == dwNbUsed)
        if (CheckNbAlloc(dwNbUsed+1))
    {
        dwNbUsed++;
        fRes=TRUE;
    }

    if (fRes)
    {
        memcpy(lpData+(dwPos*dwSizeElem),lpDataElem,dwSizeElem);
    }
    return fRes;
}

BOOL STATIC_ARRAY::InsertElemSA(dfuLong32 dwPos, LPCVOID lpDataElem)
{
    BOOL fRes = FALSE;
    if (dwPos <= dwNbUsed)
        if (CheckNbAlloc(dwNbUsed+1))
    {
        memmove(lpData+((dwPos+1)*dwSizeElem),lpData+(dwPos*dwSizeElem),
                        (dwNbUsed-dwPos)*dwSizeElem);
        dwNbUsed ++ ;
        fRes = SetElemSA(dwPos, lpDataElem);
    }
    return fRes;
}

BOOL STATIC_ARRAY::GetElemSA(dfuLong32 dwPos, LPVOID lpDataElem) const
{
    BOOL fRes = dwPos < dwNbUsed ;
    if (fRes)
    {
        memcpy(lpDataElem,lpData+(dwPos*dwSizeElem),dwSizeElem);
    }
    return fRes;
}

LPCVOID STATIC_ARRAY::GetElemPtrSA(dfuLong32 dwPos) const
{
    LPCVOID lpRet=NULL;
    if (dwPos < dwNbUsed)
    {
        lpRet = lpData+(dwPos*dwSizeElem) ;
    }
    return lpRet;
}

BOOL STATIC_ARRAY::AddEndElemSA(LPCVOID lpDataElem)
{
    BOOL fRes = FALSE;
    if (CheckNbAlloc(dwNbUsed+1))
    {
        dwNbUsed ++ ;
        fRes = SetElemSA(dwNbUsed-1, lpDataElem);
    }
    return fRes;
}

BOOL STATIC_ARRAY::SetNbAlloc(dfuLong32 dwNbAllocNew)
{
    BOOL fRes;
    dfuLong32 dwNewSize = (dwNbAllocNew*dwSizeElem) + 0x10;
    if (lpData == NULL)
    {
        // + 0x10 : when dwNbAllocNew==0 we will not have NULL
        lpData = (LPBYTE)malloc(dwNewSize);
        fRes = (lpData != NULL);
    }
    else
    {
        LPBYTE lpNewData = (LPBYTE)realloc(lpData,dwNewSize);
        fRes = (lpNewData != NULL);
        if (fRes)
            lpData = lpNewData;
    }
    if (fRes)
        dwNbAlloc = dwNbAllocNew;
    return fRes;
}

BOOL STATIC_ARRAY::CheckNbAlloc(dfuLong32 dwNbAllocNeeded)
{
    dfuLong32 dwNbExactAlloc;
    BOOL fRes = TRUE;
    dwNbExactAlloc = ((dwNbAllocNeeded + dwStepAlloc - 1)/dwStepAlloc)*dwStepAlloc;
    if (dwNbExactAlloc != dwNbAlloc)
        fRes = SetNbAlloc(dwNbExactAlloc);
    return fRes;
}

/*************************************************************************/

/*
// List test code
#include <time.h>
#include <conio.h>


long funcCompTst(LPCVOID lpElem1, LPCVOID lpElem2)
{
    char* str1=*(char**)lpElem1;
    char* str2=*(char**)lpElem2;
    return strcmp(str1,str2);
}

BOOL funcDestr(LPCVOID lpElem)
{
    char* str1=*(char**)lpElem;
    free(str1);
    return TRUE;
}

#define SIZETEST (4096)
void main(int argc,char *argv[])
{
    STATIC_ARRAY sa;
    BOOL fOk=TRUE;
    dfuLong32 i,j,nb,dwT1,dwT2;

    sa.SetFuncCompareDataSA(funcCompTst);
    sa.SetFuncDestructorSA(funcDestr);

    srand( (unsigned)time( NULL ) );
    //srand( 4 );

    nb=5000;


    printf("nb=%d\n",nb);

    dwT1=GetTickCount();
    for (i=0;i<nb;i++)
    {
        char szTxt[SIZETEST+16];
        LPSTR lpName;
        dfuLong32 dwS;

        for (j=0;j<SIZETEST;j++)
            szTxt[j]='A'+((rand()*25)/RAND_MAX);
        szTxt[j]='\0';


        lpName=(LPSTR)malloc(strlen(szTxt)+2);
        strcpy(lpName,szTxt);
        sa.InsertSortedSA(&lpName);

        if (!sa.FindSameElemPosSA(&lpName,&dwS))
            printf("not found %d\n",i);
        else
        {
            char *rs2;
            sa.GetElemSA(dwS,&rs2);
            if (funcCompTst(&rs2,&lpName)!=0)
                printf("bad foud\n");
        }
    }
    dwT2=GetTickCount();

    for (i=0;i<nb;i++)
    {
        char szTxt[SIZETEST+16];
        LPSTR lpName=szTxt;
        char *rs;
        dfuLong32 dwS;

        sa.GetElemSA(i,&rs);

        strcpy(lpName,rs);


        if (!sa.FindSameElemPosSA(&lpName,&dwS))
            printf("not found %d\n",i);
        else
        {
            char *rs2;
            sa.GetElemSA(dwS,&rs2);
            if (funcCompTst(&rs2,&lpName)!=0)
                printf("bad foud\n");
        }
    }

    for (i=0;i<sa.GetNbElemSA();i++)
    {
        char *rs;
        sa.GetElemSA(i,&rs);
        //printf("%s\n",rs);
        if (i>0)
        {
            char *rs2;
            sa.GetElemSA(i-1,&rs2);
            if (funcCompTst(&rs2,&rs)>0)
            {
                printf("\n****\n");
                fOk=FALSE;
            }
        }
    }
    if (!fOk) printf("\n**++ BAD ++\n");
    printf("time = %u\n",dwT2-dwT1);
    getchar();

    sa.DeleteElemSA(0,sa.GetNbElemSA());
    printf("free\n");
    getchar();
    printf("end\n");
}
*/

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

#include <windows.h>
#include <windowsx.h>


#ifndef WDIFSTRM
#include "winx31ad.h"
#endif
#include <commdlg.h>
#include <shellapi.h>
#include <dos.h>
#include <stdlib.h>
#include <stdio.h>
#include <direct.h>
#include <string.h>
#include <commctrl.h>
#include <shlobj.h>

#include "miscutil.h"

#include "LoadIcon.h"
#include "../../lib/engine/patchstream/common/difbasic.h"

#include "ltoolsCPP.h"
#include "extinfo.h"

typedef struct
{
    LPCTSTR lpszExt;
    BOOL fIconFilled;
    BOOL fExtNameFilled;
    int iIcon;
    LPCTSTR lpszExtDesc;
} EXTITEM;



long DFSCALLBACK fncCompareExtItem(const void *lpElem1, const void *lpElem2)
{
  const EXTITEM *pei1 = (const EXTITEM *) lpElem1;
  const EXTITEM *pei2 = (const EXTITEM *) lpElem2;
  return lstrcmpi(pei1->lpszExt,pei2->lpszExt);
}

BOOL DFSCALLBACK fncDestructorExtItem(const void *lpElem)
{
  const EXTITEM *pei = (const EXTITEM *) lpElem;
  if (pei->lpszExt != NULL)
    free((void*)pei->lpszExt);
  if (pei->lpszExtDesc != NULL)
    free((void*)pei->lpszExtDesc);

  return TRUE;
}

EXTINFOCACHE::EXTINFOCACHE()
{
    saExt.InitStaticArray(sizeof(EXTITEM), 0x80);
    saExt.SetFuncCompareDataSA(fncCompareExtItem);
    saExt.SetFuncDestructorSA(fncDestructorExtItem);
}

EXTINFOCACHE::~EXTINFOCACHE()
{
    Init(TRUE);
}

BOOL EXTINFOCACHE::Init(BOOL fClear)
{
    if (fClear)
    {
        saExt.DeleteElemSA(0, saExt.GetNbElemSA());
    }
    return TRUE;
}

LPCTSTR EXTINFOCACHE::GetExtensionFromName(LPCTSTR lpszName)
{
    LPCTSTR lpRes = lpszName;
    TCHAR c;
    DWORD i=0;
    do
    {
        c=*(lpszName);
        if ((c==':') || (c=='.') || (c=='\\') || (c=='/'))
            lpRes = CharNext(lpszName);

        lpszName=CharNext(lpszName);
    } while (c != '\0');
    return lpRes;
}


void EXTINFOCACHE::ClearExtInfo()
{
    Init(TRUE);
}

BOOL EXTINFOCACHE::CreateOrGetExtEntry(LPCTSTR lpExt,dfuLong32 &dwPos)
{
    BOOL fRet;
    EXTITEM eItemSrch;
    eItemSrch.lpszExt=lpExt;
    dwPos=0;

    fRet = saExt.FindSameElemPosSA(&eItemSrch,&dwPos);

    if (!fRet)
    {
        EXTITEM eItemInsert;
        LPTSTR lpszAllocCopyExt;
        lpszAllocCopyExt=(LPTSTR)malloc((lstrlen(lpExt)+1)*sizeof(TCHAR));
        lstrcpy(lpszAllocCopyExt,lpExt);
        eItemInsert.lpszExt=lpszAllocCopyExt;
        eItemInsert.fExtNameFilled=FALSE;
        eItemInsert.fIconFilled=FALSE;
        eItemInsert.iIcon=0;
        eItemInsert.lpszExtDesc=NULL;

        fRet = saExt.InsertSortedSA(&eItemInsert);
        if (fRet)
            fRet = saExt.FindSameElemPosSA(&eItemSrch,&dwPos);
    }
    return fRet;
}

int EXTINFOCACHE::GetItemIndexImageCached(LPCTSTR lpExt,BOOL fOnlyExt)
{
    dfuLong32 dwPos=0;
    EXTITEM* pExtItem;
    if (!fOnlyExt)
        lpExt=GetExtensionFromName(lpExt);

    if (!CreateOrGetExtEntry(lpExt,dwPos))
        return 0;

    pExtItem = (EXTITEM*)saExt.GetElemPtrSA(dwPos);
    if (!pExtItem->fIconFilled)
    {
        pExtItem->iIcon=GetItemIndexImage((LPTSTR)lpExt,TRUE);
        pExtItem->fIconFilled=TRUE;
    }
    return pExtItem->iIcon;
}

LPCTSTR EXTINFOCACHE::GetExtensionDescFromRegistryCached(LPCTSTR lpExt,BOOL fOnlyExt)
{
    dfuLong32 dwPos=0;
    EXTITEM* pExtItem;
    if (!fOnlyExt)
        lpExt=GetExtensionFromName(lpExt);

    if (!CreateOrGetExtEntry(lpExt,dwPos))
        return 0;

    pExtItem = (EXTITEM*)saExt.GetElemPtrSA(dwPos);
    if (!pExtItem->fExtNameFilled)
    {
        TCHAR szDescription[MAX_PATH*2];

        if (GetExtensionFromExtInRegistry((LPTSTR)lpExt,szDescription))
        {
            pExtItem->fExtNameFilled=TRUE;
            pExtItem->lpszExtDesc=(LPTSTR)malloc((lstrlen(szDescription)+1)*sizeof(TCHAR));
            lstrcpy((LPTSTR)pExtItem->lpszExtDesc,szDescription);
        }
        //pExtItem->iIcon=GetItemIndexImage((LPTSTR)lpExt,TRUE);
        //pExtItem->fIconFilled=TRUE;
    }

    if (pExtItem->fExtNameFilled)
      return pExtItem->lpszExtDesc;
    else
        return NULL;
}

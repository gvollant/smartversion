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

/* LruMenu.cpp */

#include <windows.h>
#include <windowsx.h>
#ifndef WIN32
#include "winx31ad.h"
#endif
#include <commdlg.h>
#include <shellapi.h>
#include <dos.h>
#include <stdlib.h>
#include <stdio.h>
#include <direct.h>
#include <string.h>
#include <tchar.h>

#include "miscutil.h"

#include <dos.h>
#include <direct.h>
#include <errno.h>

#ifdef __BORLANDC__
#include <dir.h>
#define _chdir chdir
#define _mkdir mkdir
#define _getcwd getcwd
#define _strupr strupr
#ifdef WIN32
#define hmemcpy memcpy
#endif
#endif



#include "LruMenu.h"


LRUMENU::LRUMENU (WORD wNbLruShowInit,WORD wNbLruMenuInit,WORD wMaxSizeLruItemInit)
{
  wNbItemFill = 0;
  wNbLruMenu = wNbLruMenuInit;
  wNbLruShow = wNbLruShowInit;
  wMaxSizeLruItem = wMaxSizeLruItemInit;
  wIdLruEntry = IDLRUENT;
  lpLRU = (LPSTR)malloc(wNbLruMenu*(UINT)wMaxSizeLruItem);
}

LRUMENU::~LRUMENU (void)
{
  free(lpLRU);
}

void LRUMENU::SetNbLruShow(WORD wNbLruShowInit)
{
  wNbLruShow = min(wNbLruShowInit,wNbLruMenu);
}

BOOL LRUMENU::SetMenuItem(WORD wItem,LPSTR lpItem)
{
  if (wItem >= NBLRUMENU) return FALSE;
  _fstrncpy(lpLRU + (wMaxSizeLruItem * (UINT)wItem),lpItem,wMaxSizeLruItem-1);
  wNbItemFill = max(wNbItemFill,wItem+1);
  return TRUE;
}

BOOL LRUMENU::GetMenuItem(WORD wItem,BOOL fIDMBased,LPSTR lpItem,UINT uiSize)
{
  if (fIDMBased) wItem -= (wIdLruEntry + 1);
  if (wItem >= wNbItemFill) return FALSE;
  _fstrncpy(lpItem,lpLRU + (wMaxSizeLruItem * (UINT)wItem),uiSize);
  return TRUE;
}

void LRUMENU::AddNewItem(LPSTR lpItemParam)
{
WORD i,j;
TCHAR szConvertedItem[(MAX_PATH*2)+0x10]="";
LPTSTR lpFilePart=NULL;

  DWORD dwBufferSize = (MAX_PATH * 2) + 2;
  DWORD dwResult=GetFullPathName(lpItemParam, dwBufferSize,szConvertedItem,&lpFilePart);
  if (dwResult < dwBufferSize)
    lpItemParam = szConvertedItem;
  for (i=0;i<wNbItemFill;i++)
    if (_fstrcmp(szConvertedItem,lpLRU + (wMaxSizeLruItem * (UINT)i)) == 0)
      {
    // decal puis 1er
    for (j=i;j>0;j--)
      lstrcpy(lpLRU + (wMaxSizeLruItem * (UINT)j),
          lpLRU + (wMaxSizeLruItem * (UINT)(j-1)));
    _fstrncpy(lpLRU,lpItemParam,wMaxSizeLruItem-1);
    return ;
      }
  wNbItemFill = min(wNbItemFill+1,wNbLruMenu);
  for (i=wNbItemFill-1;i>0;i--)
     lstrcpy(lpLRU + (wMaxSizeLruItem * (UINT)i),
         lpLRU + (wMaxSizeLruItem * (UINT)(i-1)));
  _fstrncpy(lpLRU,lpItemParam,wMaxSizeLruItem-1);
}


BOOL LRUMENU::DelMenuItem(WORD wItem,BOOL fIDMBased)
{
WORD i;
  if (fIDMBased) wItem -= (wIdLruEntry + 1);
  if (wNbItemFill <= wItem) return FALSE;
  wNbItemFill--;
  for (i=wItem;i<wNbItemFill;i++)
     lstrcpy(lpLRU + (wMaxSizeLruItem * (UINT)i),
         lpLRU + (wMaxSizeLruItem * (UINT)(i+1)));
  return TRUE;
}

void LRUMENU::CleanMenu(HMENU hMenu)
{
WORD i;
  for (i=0;i<=wNbLruMenu;i++)
    RemoveMenu(hMenu,i+wIdLruEntry,MF_BYCOMMAND);
}

void LRUMENU::PlaceMenuLRUItem(HMENU hMenu,UINT uiItem)
{
int i;
WORD wNbShow;
  if (hMenu == NULL) return;
  CleanMenu(hMenu);
  if (wNbItemFill == 0) return;
  MyInsertMenu(hMenu,uiItem,MF_SEPARATOR,wIdLruEntry,NULL);

  //for (i=0;i<wNbItemFill;i++)
  wNbShow = min(wNbItemFill,wNbLruShow);
  for (i=(int)wNbShow-1;i>=0;i--)
  {
  LPSTR lpTxt;
    lpTxt = (LPSTR)malloc(wMaxSizeLruItem + 20);
    wsprintf(lpTxt,"&%lu %s",(DWORD)(i+1),lpLRU + (wMaxSizeLruItem*(UINT)i));
    MyInsertMenu(hMenu,(((WORD)i)!=(wNbShow-1)) ? (wIdLruEntry+i+2) : wIdLruEntry,
          MF_STRING,wIdLruEntry+i+1,lpTxt);
    free(lpTxt);
  }
}

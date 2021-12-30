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

#include <windows.h>            // required for all Windows applications
#include <windowsx.h>
#include <commctrl.h>
#include <shlobj.h>
#include <stdio.h>

#include "SaveParam.h"

BOOL IsSmartVersionKeyFoundInLocalMachine()
{
  BOOL fTry;
  HKEY hTryKey = NULL;
  DWORD dwType = 0;
  char sz[256]="";
  DWORD dwSizeBuf=10;

  fTry = (RegOpenKeyEx(HKEY_LOCAL_MACHINE,"Software\\SmartVersion",0,
                            KEY_READ,&hTryKey)
                            == ERROR_SUCCESS);

  if (fTry)
      fTry = (RegQueryValueEx(hTryKey,"SmartVersionUseRegistry",NULL,&dwType,(LPBYTE)sz,&dwSizeBuf) ==
                  ERROR_SUCCESS);


  if (hTryKey != NULL)
      RegCloseKey(hTryKey);

  return fTry;
}

HKEY GetSmartVersionKeyBase()
{
    if (IsSmartVersionKeyFoundInLocalMachine())
        return HKEY_LOCAL_MACHINE;
    else
        return HKEY_CURRENT_USER;
}

SAVEPARAM::SAVEPARAM()
{
    hParamKey = NULL;
    fWriteEnabled=FALSE;
}

SAVEPARAM::~SAVEPARAM()
{
    CloseRegKey();
}



BOOL SAVEPARAM::OpenRegKey(HKEY hBaseKey,LPCTSTR lpszNameBranch,BOOL fWriteEnabled)
{
    BOOL fOpened;
    fOpened = (RegCreateKeyEx(hBaseKey,lpszNameBranch,0,NULL,
                  REG_OPTION_NON_VOLATILE,
                  fWriteEnabled ? KEY_ALL_ACCESS : KEY_READ,NULL,&hParamKey,NULL)
                            == ERROR_SUCCESS);
    return fOpened;
}

BOOL SAVEPARAM::DoReadTxtParam(TXTSAVEPARAM* lpTxtSaveParam,DWORD dwNbParam)
{
    DWORD i;
    if (hParamKey == NULL)
        return FALSE;

    for (i=0;i<dwNbParam;i++)
    {
        DWORD dwSizeBuf = (lpTxtSaveParam+i)->uiSize;
        DWORD dwType = 0;
        BOOL fGoodQuery;
        fGoodQuery = RegQueryValueEx(hParamKey,(lpTxtSaveParam+i)->lpszNameEntry,NULL,&dwType,(LPBYTE)(lpTxtSaveParam+i)->lpszValue,&dwSizeBuf) ==
                  ERROR_SUCCESS;
        if ((fGoodQuery) && (dwType==REG_SZ))
            *(((lpTxtSaveParam+i)->lpszValue)+dwSizeBuf) = '\0';
        else
        {
            if ((lpTxtSaveParam+i)->lpDefValue == NULL)
                lstrcpy((lpTxtSaveParam+i)->lpszValue,"");
            else
                lstrcpy((lpTxtSaveParam+i)->lpszValue,(lpTxtSaveParam+i)->lpDefValue);
        }
    }
    return TRUE;
}

BOOL SAVEPARAM::DoWriteTxtParam(TXTSAVEPARAM* lpTxtSaveParam,DWORD dwNbParam)
{
    DWORD i;
    if (hParamKey == NULL)
        return FALSE;

    for (i=0;i<dwNbParam;i++)
    {
        RegSetValueEx(hParamKey,(lpTxtSaveParam+i)->lpszNameEntry,0,REG_SZ,
                      (LPBYTE)(lpTxtSaveParam+i)->lpszValue,lstrlen((lpTxtSaveParam+i)->lpszValue)+1);
    }
    return TRUE;
}

BOOL SAVEPARAM::DoReadLongParam(LONGSAVEPARAM* lpLongSaveParam,DWORD dwNbParam)
{
    DWORD i;
    if (hParamKey == NULL)
        return FALSE;

    for (i=0;i<dwNbParam;i++)
    {
        TCHAR szRead[MAX_PATH+2];
        DWORD dwSizeBuf = MAX_PATH;
        DWORD dwType = 0;
        BOOL fGoodQuery;
        fGoodQuery = RegQueryValueEx(hParamKey,(lpLongSaveParam+i)->lpszNameEntry,NULL,&dwType,(LPBYTE)szRead,&dwSizeBuf) ==
                  ERROR_SUCCESS;
        if ((fGoodQuery) && (dwType==REG_SZ))
            *((lpLongSaveParam+i)->plValue) = atol(szRead);
        else
        if ((fGoodQuery) && (dwType==REG_DWORD))
            memcpy(((lpLongSaveParam+i)->plValue),szRead,sizeof(DWORD));
        else
            *((lpLongSaveParam+i)->plValue) = (lpLongSaveParam+i)->lDefValue;

    }

    return TRUE;
}

BOOL SAVEPARAM::DoWriteLongParam(LONGSAVEPARAM* lpLongSaveParam,DWORD dwNbParam)
{
    DWORD i;
    if (hParamKey == NULL)
        return FALSE;

    for (i=0;i<dwNbParam;i++)
    {
        RegSetValueEx(hParamKey,(lpLongSaveParam+i)->lpszNameEntry,0,REG_DWORD,
                      (LPBYTE)(lpLongSaveParam+i)->plValue,sizeof(DWORD));
    }
    return TRUE;
}

BOOL SAVEPARAM::CloseRegKey()
{
    if (hParamKey != NULL)
        RegCloseKey(hParamKey);
    hParamKey = NULL;
    return TRUE;
}

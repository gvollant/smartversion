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

#include "resource.h"
#include "global.h"            // prototypes specific to this application

#include "ltoolsCPP.h"
#include "ExtInfo.h"
#include "GuiItem.h"
#include "../../lib/engine/svfile/common/DirSet.h"

#include "../../lib/engine/patchstream/common/difstrm.h"
#include "../../lib/engine/patchstream/common/DfsOrigMemoryMap.h"
#include "../../lib/engine/svfile/decompress/DoExtracting.h"
#include "../../lib/engine/patchstream/compress/makdifst.h"

#include "RegCode.h"
#include "uiMain.h"

//#include "MiscUtilWDFS.h"
#include "MiscUtil.h"

#include "LoadIcon.h"

#include "../../lib/engine/svfile/common/ArrayTl.h"
#include "../../lib/engine/patchstream/common/DfsOrigMemoryMap.h"
#include "../../lib/engine/patchstream/common/DfsIoHlp.h"

#include "Droputil.h"
#include "DrpOleUt.h"

UINT GetUiResDescTypeDir(dfuLong32 dfTypeDir)
{
UINT uID=0;

      switch (dfTypeDir)
      {
      case TYPEDIR_FILECRCONLY:
          uID = IDS_TYPEDIR_FILECRCONLY;
          break;

      case TYPEDIR_FILEINSERTING_STORE:
          uID = IDS_TYPEDIR_FILEINSERTING_STORE;
          break;

      case TYPEDIR_FILEINSERTING_DEFLATE:
          uID = IDS_TYPEDIR_FILEINSERTING_DEFLATE;
          break;

      case TYPEDIR_PATCHFROMPREVIOUS:
          uID = IDS_TYPEDIR_PATCHFROMPREVIOUS;
          break;
      }
      return uID;
}

/*
BOOL BuildStrDate(const DFSTM* pdfsTm,LPTSTR lpszString,UINT uiSize)
{
  DFSTM dfsTm = *pdfsTm;
  wsprintf(lpszString,"%02u/%02u/%02u %02u:%02u:%02u ", dfsTm.df_mday,
         dfsTm.df_mon, dfsTm.df_year % 100, dfsTm.df_hour,
         dfsTm.df_min, dfsTm.df_sec);
  return TRUE;
}
*/


// Date and Time
// Must be change for proper Internationalisation
// under MS-DOS : int 21h , AH=38h
// under Microsoft Windows operationg system : Win.ini [intl]

/*
BOOL BuildStrDate(const DFSTM* pdfsTm,LPTSTR lpszString,UINT uiSize)
{
  static TCHAR szShortDate[64];
  static TCHAR szTime1159[64];
  static TCHAR szTime2359[64];
  static TCHAR sziTime[64];
  static BOOL fShortDateLoaded = FALSE;
  DFSTM dfsTm = *pdfsTm;
  BOOL fLongYear=FALSE;
  int year = dfsTm.df_year;
  if (!fLongYear)
      year = year %100;

  if (!fShortDateLoaded)
  {
    DWORD dwType=0;
    HKEY hKey=NULL;
    szShortDate[0] = 0;
    szTime1159[0] = 0;
    szTime2359[0] = 0;
    sziTime[0] = 0;


    if (RegOpenKey(HKEY_CURRENT_USER, "Control Panel\\International", &hKey)==ERROR_SUCCESS)
    {
      DWORD lDataBufSize ;

      lDataBufSize= sizeof(szShortDate);
      RegQueryValueEx(hKey, "sShortDate", NULL, &dwType, (LPBYTE)szShortDate, &lDataBufSize);

      lDataBufSize= sizeof(szTime1159);
      RegQueryValueEx(hKey, "s1159", NULL, &dwType, (LPBYTE)szTime1159, &lDataBufSize);

      lDataBufSize= sizeof(szTime2359);
      RegQueryValueEx(hKey, "s2359", NULL, &dwType, (LPBYTE)szTime2359, &lDataBufSize);

      lDataBufSize= sizeof(sziTime);
      RegQueryValueEx(hKey, "iTime", NULL, &dwType, (LPBYTE)sziTime, &lDataBufSize);

      RegCloseKey(hKey);
    }

    fShortDateLoaded=TRUE;
  }
  if ((szShortDate[0] == 'M') || (szShortDate[0] == 'm'))
      wsprintf(lpszString,fLongYear ? "%02lu/%02lu/%04lu" : "%02lu/%02lu/%02lu",dfsTm.df_mon,dfsTm.df_mday,year);
   else if ((szShortDate[0] == 'Y') || (szShortDate[0] == 'y'))
       wsprintf(lpszString,fLongYear ? "%04lu/%02lu/%02lu" : "%02lu/%02lu/%02lu",year,dfsTm.df_mon,dfsTm.df_mday);
   else wsprintf(lpszString,fLongYear ? "%02lu/%02lu/%04lu" : "%02lu/%02lu/%02lu",dfsTm.df_mday,dfsTm.df_mon,year);


  if (sziTime[0] != '0')
     wsprintf(lpszString+lstrlen(lpszString)," %02lu:%02lu:%02lu",dfsTm.df_hour,dfsTm.df_min,dfsTm.df_sec);
   else
   {
     int hr = dfsTm.df_hour;
     int hr12 = hr;
     if (hr12>12)
         hr12 -= 12;
     wsprintf(lpszString+lstrlen(lpszString)," %02lu:%02lu:%02lu %s",hr12,dfsTm.df_min,dfsTm.df_sec,(hr <12) ? ((LPSTR)szTime1159)
                                                           : ((LPSTR)szTime2359));
   }
   return TRUE;
}
*/

BOOL BuildStrDate(const DFSTM* pDfsTm,LPTSTR lpszString,UINT uiSize)
{
    SYSTEMTIME SystemTime;
    UINT uiDateSize;

    SystemTime.wMilliseconds = (WORD) pDfsTm->df_msec;    /* milisecond [0..999] 10 bits */
    SystemTime.wSecond = (WORD) pDfsTm->df_sec;   /* seconds after the minute - [0..59]  6 bits */
    SystemTime.wMinute = (WORD) pDfsTm->df_min;   /* minutes after the hour - [0..59] 6 bits */
    SystemTime.wHour = (WORD) pDfsTm->df_hour;    /* hours since midnight - [0..23]  5 bits */
    SystemTime.wDayOfWeek = (WORD) 0;
    SystemTime.wDay = (WORD) pDfsTm->df_mday;     /* day of the month - [1..31] 5 bits */
    SystemTime.wMonth = (WORD) pDfsTm->df_mon;    /* months since January - [1..12] 4 bits */
    SystemTime.wYear = (WORD) pDfsTm->df_year;    /* years - [0..4095] 12 bits */


    if (GetDateFormat(LOCALE_USER_DEFAULT,DATE_SHORTDATE,&SystemTime,NULL,lpszString,uiSize-1) == 0)
        return FALSE;
    lstrcat(lpszString," ");
    uiDateSize=lstrlen(lpszString);
    if (uiDateSize >= uiSize)
        return FALSE;

    if (GetTimeFormat(LOCALE_USER_DEFAULT,TIME_NOSECONDS,&SystemTime,NULL,lpszString+uiDateSize,uiSize-uiDateSize) == 0)
        return FALSE;

    return TRUE;
}
// --------------------------------------------------------------------------
//  FUNCTION    WinLongToStr
//  PURPOSE     copy long to string using INTL windows settings
// --------------------------------------------------------------------------
//  INPUT       lNum            long integer to copy
//              lpszStr         pointer to string to copy to
//              cbStr           size of string to copy to
//
//  OUTPUT      int             number of char. written to lpsz
// --------------------------------------------------------------------------
//  COMMENTS    will only work for POSITIVE numbers - sign is not considered
// --------------------------------------------------------------------------
/*
int WinLongToStr(long lNum, LPSTR lpszStr, int cbStr)
{
  long lNumDiv, lNumMod;
  static TCHAR  szThousand[10];
  int   ncIn, ncOut, i;
  BOOL  fEmptySeparator;
  LPSTR lpszBuf;
  LPSTR lpchCur;
  LPSTR lpchStr;
  static BOOL  fszThousandFilled=FALSE;

  if (!fszThousandFilled)
  {
    DWORD dwType=0;
    HKEY hKey=NULL;
    szThousand[0] = 0;

    if (RegOpenKey(HKEY_CURRENT_USER, "Control Panel\\International", &hKey)==ERROR_SUCCESS)
    {
      DWORD lDataBufSize ;

      lDataBufSize= sizeof(szThousand);
      RegQueryValueEx(hKey, "sThousand", NULL, &dwType, (LPBYTE)szThousand, &lDataBufSize);

      RegCloseKey(hKey);
    }

    fszThousandFilled = TRUE;
  }


  {
      //TCHAR szBuffer[MAX_PATH];
      //wsprintf(szBuffer, ("%lu"), lNum);

      //ncOut = GetNumberFormat(LOCALE_USER_DEFAULT, 0, szBuffer, NULL, lpszStr, cbStr);
      //if (ncOut>0)
      //    return ncOut;
  }

  // allocate memory for temporary string buffer

  lpszBuf = (LPSTR)GlobalAllocPtr(GHND, cbStr + 10);
  if(!lpszBuf) return 0;                        // no char copied to out string

  // get windows settings
  //GetProfileString("intl", "sThousand", " ", (LPSTR)szThousand, 2);
  fEmptySeparator = (strlen(szThousand) == 0);

  // initialization
  lpchCur = lpszBuf;
  lNumDiv = lNum;
  ncIn    = 0;
  ncOut   = 0;

  // loop to generate reverse order string from integral part of lNum
  while(lNumDiv >= 10)
  {
    ncIn++;
    ncOut++;

    lNumMod = lNumDiv % 10;
    *lpchCur = (char)('0' + lNumMod);

    lpchCur++;
    lNumDiv = lNumDiv / 10;

    // insert thousand separator if needed
    if(!fEmptySeparator && ((ncIn % 3) == 0) && (lNumDiv > 0))
    {
      *lpchCur = szThousand[0];
      lpchCur++;
      ncOut++;
    }
  }
  // copy last digit to buffer
  *lpchCur = (char)('0' + lNumDiv);
  ncOut++;

  // copy temporary buffer to output string in reverse order
  lpchStr = lpszStr;
  for(i = 0; i < ncOut; i++)
    *lpchStr++ = *lpchCur--;

  // terminate string and adjust return value
  *lpchStr   = '\0';
  ncOut += 1;

  // free temporary buffer and return number of char written to output string
  GlobalFreePtr(lpszBuf);
  return ncOut;
}
*/

/*
int WinLongToStr(long lNum, LPTSTR lpszStr, int cbStr)
{
    TCHAR szRawNumber[MAX_PATH+1];
    TCHAR szFormattedNumber[MAX_PATH+1+0x10];
    TCHAR szDecimalSep[0x11];
    LPTSTR lpLastDecimalSeparator=NULL;
    LPTSTR lpszBrowseFormatted;
    UINT uiszDecimalSepLenInByte;

    wsprintf(szRawNumber,"%lu",lNum);
    GetNumberFormat(LOCALE_USER_DEFAULT,0,szRawNumber,NULL,szFormattedNumber,MAX_PATH);
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL,szDecimalSep, 0x10);
    uiszDecimalSepLenInByte = lstrlen(szDecimalSep)*sizeof(TCHAR);

    lpszBrowseFormatted = szFormattedNumber;
    while ((*lpszBrowseFormatted) != 0)
    {
        if (memcmp(szDecimalSep,lpszBrowseFormatted,uiszDecimalSepLenInByte)==0)
            lpLastDecimalSeparator = lpszBrowseFormatted;
        lpszBrowseFormatted = CharNext(lpszBrowseFormatted);
    }
    if (lpLastDecimalSeparator!=NULL)
        *lpLastDecimalSeparator=0;

    if (lstrlen(szFormattedNumber)>=cbStr)
        return 0;
    lstrcpy(lpszStr,szFormattedNumber);
    return lstrlen(lpszStr);
}
*/


int WinLongRawSzToStr(LPTSTR szRawNumber, LPTSTR lpszStr, int cbStr)
{
    TCHAR szFormattedNumber[MAX_PATH+1+0x10];
    TCHAR szDecimalSep[0x11];
    LPTSTR lpLastDecimalSeparator=NULL;
    LPTSTR lpszBrowseFormatted;
    UINT uiszDecimalSepLenInByte;

    GetNumberFormat(LOCALE_USER_DEFAULT,0,szRawNumber,NULL,szFormattedNumber,MAX_PATH);
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL,szDecimalSep, 0x10);
    uiszDecimalSepLenInByte = lstrlen(szDecimalSep)*sizeof(TCHAR);

    lpszBrowseFormatted = szFormattedNumber;
    while ((*lpszBrowseFormatted) != 0)
    {
        if (memcmp(szDecimalSep,lpszBrowseFormatted,uiszDecimalSepLenInByte)==0)
            lpLastDecimalSeparator = lpszBrowseFormatted;
        lpszBrowseFormatted = CharNext(lpszBrowseFormatted);
    }
    if (lpLastDecimalSeparator!=NULL)
        *lpLastDecimalSeparator=0;

    if (lstrlen(szFormattedNumber)>=cbStr)
        return 0;
    lstrcpy(lpszStr,szFormattedNumber);
    return lstrlen(lpszStr);
}


int WinLongToStr(long lNum, LPTSTR lpszStr, int cbStr)
{
    TCHAR szRawNumber[MAX_PATH];
    wsprintf(szRawNumber,"%lu",lNum);
    return WinLongRawSzToStr(szRawNumber,lpszStr,cbStr);
}


int WinLongLongToStr(DWORD dwNumLow,long lNumHigh, LPSTR lpszStr, int cbStr)
{
    TCHAR szRawNumber[MAX_PATH];
    if (lNumHigh==0)
        wsprintf(szRawNumber,"%lu",dwNumLow);
    else
#if defined(WIN64) || defined(_WIN64)
        wsprintf(szRawNumber,"%I64u",dwNumLow | (((dfuLong64)lNumHigh)<<32));
#else
        wsprintf(szRawNumber,"%I64u",dwNumLow,lNumHigh);
#endif
    return WinLongRawSzToStr(szRawNumber,lpszStr,cbStr);
}


int WinLong64ToStr(dfuLong64 dfl64Num, LPSTR lpszStr, int cbStr)
{
    LARGE_INTEGER li;

    li.QuadPart = dfl64Num;
    DWORD dwNumLow=li.LowPart;
    long lNumHigh=li.HighPart;
    return WinLongLongToStr(dwNumLow,lNumHigh, lpszStr, cbStr);
}


GUIITEM::GUIITEM()
{
    hwndMain = hwndTreeView = hwndLV = hwndToolTip = NULL;
    hwndTB = hwndSB = NULL;
    hwndProgress = NULL;
    hPopupMenu = NULL;
    hdcSplit = NULL;
    cxSplitter = 0;
    dwLastPosProgress = 0;

    dfListViewNbItem = 0;
    pdfwListViewSortMap = NULL;

    fDirty = FALSE;
    fFileNameExist=FALSE;

    fColumnInitialised=FALSE;
    fCurrentColumnDirList=FALSE;

    szFileName[0] = 0;
    szDefaultDirExtract[0] = 0;
    szDefaultDirAddVersion[0] = 0;
    szDefaultDirPreviousVersion[0] = 0;

    InitDefaultCompressionParam(&compressionParam);

    compressionParam.uZlibCompressRatio=6;
    compressionParam.dfBlockCalcSizeSearch=4;//8;
    compressionParam.dfPhysicalMemoryKB = GetPhysicalMemoryKb()/4;

    fOverwriteExtracting=FALSE;
    fRegistered=FALSE;
    fListViewFocused=FALSE;

    fSelectTempMemSize=FALSE;
    dwTempMemSize=0;
    fSelectTempPath=FALSE;
    szTempPath[0]=0;
    pfStripIdentical=FALSE;
    fMd5 = TRUE;
    fSha1 = FALSE;
	fSha256 = FALSE;

    sizeTB.cx = sizeTB.cy = 0;
    dwLangUISelect=LANGUI_SELECT_AUTO;

    InitializeDragOle(GetModuleHandle(NULL));
    fInvert=FALSE;
}

GUIITEM::~GUIITEM()
{
    EraseListView();
    UninitializeDragOle();
}

BOOL InitListViewImageLists(HWND hwndLV)
{
    HIMAGELIST himlSmall;
    HIMAGELIST himlLarge;
    SHFILEINFO sfi;
    BOOL       bSuccess=TRUE;

    himlSmall = (HIMAGELIST)SHGetFileInfo((LPCSTR)"C:\\",
                                           FILE_ATTRIBUTE_DIRECTORY,
                                           &sfi,
                                           sizeof(SHFILEINFO),
                                           SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);

    himlLarge = (HIMAGELIST)SHGetFileInfo((LPCSTR)"C:\\",
                                           FILE_ATTRIBUTE_DIRECTORY,
                                           &sfi,
                                           sizeof(SHFILEINFO),
                                           SHGFI_SYSICONINDEX | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES);

    if (himlSmall && himlLarge)
    {
       ListView_SetImageList(hwndLV, himlSmall, LVSIL_SMALL);
       ListView_SetImageList(hwndLV, himlLarge, LVSIL_NORMAL);
    }
    else
       bSuccess = FALSE;

    return bSuccess;
}


BOOL GUIITEM::InstallProgressBar(DWORD dwMaxSet)
{
    RECT rcClient;
    int cyVScroll ;

    // cyVScroll = GetSystemMetrics(SM_CYVSCROLL);
    GetWindowRect(hwndSB,&rcClient);
    cyVScroll = rcClient.bottom - rcClient.top;

    GetClientRect(hwndMain, &rcClient);
    ShowWindow(hwndSB,FALSE);



    if (hwndProgress==NULL)
    {
            //InitCommonControls();

            hwndProgress = CreateWindowEx(0, PROGRESS_CLASS, (LPSTR) NULL,
                WS_CHILD | WS_VISIBLE, rcClient.left,
                rcClient.bottom - cyVScroll,
                rcClient.right, cyVScroll,
                hwndMain, (HMENU) 0, ghInst, NULL);
           // SetWindowPos(hwndProgress,HWND_TOP,0,0,0,0,SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOREPOSITION);
            dwLastPosProgress = 0;
    }
    dwMaxProgress=dwMaxSet;
    SendMessage(hwndProgress, PBM_SETRANGE, 0, MAKELPARAM(0, dwMaxProgress));
    SendMessage(hwndProgress, PBM_SETSTEP, (WPARAM) 1, 0);
    SetCursor(LoadCursor(NULL,IDC_WAIT));
    return TRUE;
}

BOOL GUIITEM::SetProgressPos(DWORD dwSet)
{
    BOOL fRet=FALSE;
    if (hwndProgress!=NULL)
    {
        if (dwLastPosProgress != dwSet)
        {
            SendMessage(hwndProgress, PBM_SETPOS, dwSet, 0);
            dwLastPosProgress = dwSet;
            #ifdef _DEBUG
            {
                TCHAR szText[MAX_PATH];
                wsprintf(szText,"-%03u- %t",dwSet);
                if (dwSet>500)
                {
                    OutputDebugString("##");
                }
                OutputDebugString(szText);
            }
            #endif
        }
        fRet=TRUE;
    }
    return fRet;
}

BOOL GUIITEM::RemoveProgressBar()
{
    if (hwndProgress!=NULL)
    {
        DestroyWindow(hwndProgress);
        hwndProgress=NULL;
        dwLastPosProgress=0;
    }

    SetCursor(LoadCursor(NULL,IDC_ARROW));
    ShowWindow(hwndSB,TRUE);

    return TRUE;
}

BOOL GUIITEM::RemovePopupMenu()
{
    if (hPopupMenu == NULL)
        return FALSE;

    DestroyMenu(hPopupMenu);
    hPopupMenu = NULL;

    return TRUE;
}

static BOOL IsMenuItemEnabled(HMENU hMainMenu,UINT uiID)
{
    UINT flag;
    flag = GetMenuState(hMainMenu,uiID,MF_BYCOMMAND);
    return (!(flag & (MF_DISABLED | MF_GRAYED)));
}

static BOOL CopyMenuItem(HMENU hMainMenu,HMENU hPopupMenu,UINT uiID)
{
    TCHAR szText[MAX_PATH+1];
    //MENUITEMINFO mii;
    if (!IsMenuItemEnabled(hMainMenu,uiID))
        return FALSE;
    GetMenuString(hMainMenu,uiID,
                    szText,MAX_PATH,MF_BYCOMMAND);

    InsertMenu(hPopupMenu,0xFFFFFFFF,MF_BYPOSITION | MF_STRING,uiID,szText);
    //mii.cbSize = sizeof(mii);
    //InsertMenuItem(hPopupMenu,0xFFFFFFFF,MF_BYPOSITION | MF_STRING,&mii);
    return TRUE;
}


BOOL GUIITEM::DoPopupMenu(const POINT* pt,BOOL fIsTreeView,BOOL fIfListViewIsDir)
{
    HMENU hMainMenu = GetMenu(hwndMain);
    DWORD dwNbItem = 0;
    RemovePopupMenu();

    hPopupMenu = CreatePopupMenu();
    //if (!fIsTreeView)
    {
        if (CopyMenuItem(hMainMenu,hPopupMenu,IDM_VERSION_EXTRACT))
            dwNbItem++;
        if (CopyMenuItem(hMainMenu,hPopupMenu,IDM_VERSION_ZIPFILE))
            dwNbItem++;
        if ((!fIfListViewIsDir) && (!fIsTreeView))
            if (CopyMenuItem(hMainMenu,hPopupMenu,IDM_FILEPROPERTIES))
                dwNbItem++;
    }

    if ((IsMenuItemEnabled(hMainMenu,IDM_FILESDELETE) ||
         IsMenuItemEnabled(hMainMenu,IDM_INSERTFILES)) && (!fIfListViewIsDir) && (!fIsTreeView))
    {
        InsertMenu(hPopupMenu,0xFFFFFFFF,MF_BYPOSITION | MF_SEPARATOR,0,NULL);
        if (CopyMenuItem(hMainMenu,hPopupMenu,IDM_FILESDELETE))
            dwNbItem++;
        if (CopyMenuItem(hMainMenu,hPopupMenu,IDM_INSERTFILES))
            dwNbItem++;
    }

    if (fIsTreeView || fIfListViewIsDir)
    {
      if (CopyMenuItem(hMainMenu,hPopupMenu,IDM_VERSION_DELETE))
            dwNbItem++;
      if (CopyMenuItem(hMainMenu,hPopupMenu,IDM_VERSION_VERSIONPROPERTIES))
          dwNbItem++;
    }

    if (fIsTreeView)
      if (CopyMenuItem(hMainMenu,hPopupMenu,IDM_VERSION_ADDNEWVERSION))
          dwNbItem++;

    if ((!fIsTreeView) && (fIfListViewIsDir))
    {
        if (CopyMenuItem(hMainMenu,hPopupMenu,IDM_VERSION_GENERATESUBDFS))
            dwNbItem++;
    }
    if (dwNbItem==0)
    {
        DestroyMenu(hPopupMenu);
        hPopupMenu = NULL;
    }
    else
    {
      TrackPopupMenuEx(hPopupMenu,0,pt->x,pt->y,hwndMain,NULL);
    }
    return (dwNbItem>0);
}

BOOL GUIITEM::SetListViewStyle(UINT uiStyle)
{
  DWORD dwStyle ;
  int i;
  HMENU hMenu =  GetMenu(hwndMain);
  HWND hWndLB = hwndLV;

  for (i=IDM_VIEW_LARGEICON;i<=IDM_VIEW_LIST;i++)
    {
    BOOL fHere ;
    //BYTE bState ;
      fHere = (unsigned)(i - IDM_VIEW_LARGEICON) == (unsigned)(uiStyle - LVS_ICON);
      CheckMenuItem(hMenu,i,fHere ? MF_CHECKED : MF_UNCHECKED);
      SendMessage(hwndTB,TB_PRESSBUTTON,i,MAKELONG(fHere,0));

      //bState = fHere ? BTNS_PUSHED : BTNS_RAISED ;
      //SetBtnState(hWndTB,i,bState);
    }

  dwStyle = GetWindowLong(hWndLB, GWL_STYLE);
  if ((dwStyle & LVS_TYPEMASK) != uiStyle)
                          SetWindowLong(hWndLB, GWL_STYLE,
                            (dwStyle & ~LVS_TYPEMASK) | uiStyle);
  uiStyleLv = uiStyle;
  return TRUE;
}

BOOL GUIITEM::GetListViewStyle(UINT &uiStyle) const
{
   HWND hWndLB = hwndLV;

   uiStyle = GetWindowLong(hWndLB, GWL_STYLE) & LVS_TYPEMASK ;
   return TRUE;
}




typedef struct
{
    int iColumn;
    BOOL fInvert;
    PDIRINFO pCurDirInfo;
    EXTINFOCACHE* pExtInfoCache;
} PARAMSORTCOLUMN;

int CompareTypeOfFileName(EXTINFOCACHE* pExtInfoCache,dfwcharpc dfFileName1,dfwcharpc dfFileName2)
{
    int iRet;
        LPCTSTR lpszRegisteredType1,lpszRegisteredType2;
        char szFileName1[MAX_PATH];
        char szFileName2[MAX_PATH];
        LPCTSTR lpExt1,lpExt2;

        wsprintf(szFileName1,"%ws",dfFileName1);
        lpExt1=pExtInfoCache->GetExtensionFromName(szFileName1);
        lpszRegisteredType1 = pExtInfoCache->GetExtensionDescFromRegistryCached(lpExt1,TRUE);
        if (lpszRegisteredType1==NULL)
            lpszRegisteredType1="";

        wsprintf(szFileName2,"%ws",dfFileName2);
        lpExt2=pExtInfoCache->GetExtensionFromName(szFileName2);
        lpszRegisteredType2 = pExtInfoCache->GetExtensionDescFromRegistryCached(lpExt2,TRUE);
        if (lpszRegisteredType2==NULL)
            lpszRegisteredType2="";

        if (lstrcmpi(lpExt1,lpExt2)==0)
            iRet=0;
        else
            iRet = lstrcmpi(lpszRegisteredType1,lpszRegisteredType2);
        return iRet;
}

int CALLBACK CompareFuncListView(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    int iRet = 0;
    PARAMSORTCOLUMN* ppsc;

    if (lParam1 == lParam2)
        return 0;

    ppsc = (PARAMSORTCOLUMN*)lParamSort;
    EXTINFOCACHE* pExtInfoCache=ppsc->pExtInfoCache;

    if (ppsc->iColumn == 1)
        {
            iRet = CompareTypeOfFileName(pExtInfoCache,
                                         (ppsc->pCurDirInfo->pFileInDirInfo + lParam1)->FileName,
                                         (ppsc->pCurDirInfo->pFileInDirInfo + lParam2)->FileName);
        }

    if (ppsc->iColumn == 2)
    {
        if (((ppsc->pCurDirInfo->pFileInDirInfo + lParam1)->dfSize) <
            ((ppsc->pCurDirInfo->pFileInDirInfo + lParam2)->dfSize))
            iRet = -1;

        if (((ppsc->pCurDirInfo->pFileInDirInfo + lParam1)->dfSize) >
            ((ppsc->pCurDirInfo->pFileInDirInfo + lParam2)->dfSize))
            iRet = 1;
    }


    if (ppsc->iColumn == 3)
    {
        dfvoidp TagBuf1;
        dfuLong32 TagSize1;
        dfvoidp TagBuf2;
        dfuLong32 TagSize2;

        if ((GetTag(*(ppsc->pCurDirInfo->TagFile + lParam1), DFSTAG_DATE, &TagBuf1, &TagSize1)) &&
            (GetTag(*(ppsc->pCurDirInfo->TagFile + lParam2), DFSTAG_DATE, &TagBuf2, &TagSize2)))
        {
          DFSTM dfsTm1,dfsTm2;
          ConvertDfsInfoDateToDfsTm((DFSINFODATE *) TagBuf1, &dfsTm1);
          ConvertDfsInfoDateToDfsTm((DFSINFODATE *) TagBuf2, &dfsTm2);

          iRet = CompareDfsTm(&dfsTm1,&dfsTm2);
        }
    }

    if (ppsc->iColumn == 4)
    {
        signed long dfRatio1 = 0;
        signed long dfRatio2 = 0;
        if ((ppsc->pCurDirInfo->pFileInDirInfo + lParam1)->dfSize>0)
          dfRatio1 = ((((signed)(ppsc->pCurDirInfo->pFileInDirInfo + lParam1)->dfSize) - (signed)(ppsc->pCurDirInfo->pFileInDirInfo + lParam1)->dfFileEncodedSize)*100) / (signed)((ppsc->pCurDirInfo->pFileInDirInfo + lParam1)->dfSize);
        if ((ppsc->pCurDirInfo->pFileInDirInfo + lParam2)->dfSize>0)
          dfRatio2 = ((((signed)(ppsc->pCurDirInfo->pFileInDirInfo + lParam2)->dfSize) - (signed)(ppsc->pCurDirInfo->pFileInDirInfo + lParam2)->dfFileEncodedSize)*100) / (signed)((ppsc->pCurDirInfo->pFileInDirInfo + lParam2)->dfSize);
        if (dfRatio1 < dfRatio2)
            iRet = -1;
        if (dfRatio1 > dfRatio2)
            iRet = 1;
    }


    if (ppsc->iColumn == 5)
    {
        if (((ppsc->pCurDirInfo->pFileInDirInfo + lParam1)->dfFileEncodedSize) <
            ((ppsc->pCurDirInfo->pFileInDirInfo + lParam2)->dfFileEncodedSize))
            iRet = -1;

        if (((ppsc->pCurDirInfo->pFileInDirInfo + lParam1)->dfFileEncodedSize) >
            ((ppsc->pCurDirInfo->pFileInDirInfo + lParam2)->dfFileEncodedSize))
            iRet = 1;
    }


    if (ppsc->iColumn == 6)
    {
        dfuLong32 dfCrc32_1,dfCrc32_2;
        dfCrc32_1 = dfCrc32_2 = 0;

        if ((ppsc->pCurDirInfo->pFileInDirInfo + lParam1)->fCrc32Filled)
          dfCrc32_1 = (ppsc->pCurDirInfo->pFileInDirInfo + lParam1)->dfCrc32;

        if ((ppsc->pCurDirInfo->pFileInDirInfo + lParam2)->fCrc32Filled)
          dfCrc32_2 = (ppsc->pCurDirInfo->pFileInDirInfo + lParam2)->dfCrc32;
        if (dfCrc32_1 < dfCrc32_2)
            iRet = -1;
        if (dfCrc32_1 > dfCrc32_2)
            iRet = 1;
    }

    if (ppsc->iColumn == 7)
    {

        dfvoidp TagBuf1;
        dfuLong32 TagSize1;
        dfvoidp TagBuf2;
        dfuLong32 TagSize2;

        if ((GetTag(*(ppsc->pCurDirInfo->TagFile + lParam1), DFSTAG_STORAGESTATUS, &TagBuf1, &TagSize1)) &&
            (GetTag(*(ppsc->pCurDirInfo->TagFile + lParam2), DFSTAG_STORAGESTATUS, &TagBuf2, &TagSize2)))
            if ((TagSize1 == sizeof(dfuLong32Intel)) && (TagSize2 == sizeof(dfuLong32Intel)))
        {
                dfuLong32 dfFileIdentical1 = ConvertuLongIntelToLong(*(dfuLong32Intel*)TagBuf1);
                dfuLong32 dfFileIdentical2 = ConvertuLongIntelToLong(*(dfuLong32Intel*)TagBuf2);
                if (dfFileIdentical1<dfFileIdentical2)
                    iRet = -1;
                if (dfFileIdentical1>dfFileIdentical2)
                    iRet = 1;
        }
    }



    if ((ppsc->iColumn == 0) || (ppsc->iColumn == 8) || (iRet==0))
    {
        dfwchar dfwOnlyFileName1[MAX_PATH+0x10];
        dfwchar dfwOnlyFilePath1[MAX_PATH+0x10];
        dfwchar dfwOnlyFileName2[MAX_PATH+0x10];
        dfwchar dfwOnlyFilePath2[MAX_PATH+0x10];

        dfwOnlyFilePath1[0] = dfwOnlyFileName1[0] = '\0';
        SplitFileNameAndPath((ppsc->pCurDirInfo->pFileInDirInfo + lParam1)->FileName,
                             dfwOnlyFilePath1,MAX_PATH,
                             dfwOnlyFileName1,MAX_PATH,
                             TRUE);

        dfwOnlyFilePath2[0] = dfwOnlyFileName2[0] = '\0';
        SplitFileNameAndPath((ppsc->pCurDirInfo->pFileInDirInfo + lParam2)->FileName,
                             dfwOnlyFilePath2,MAX_PATH,
                             dfwOnlyFileName2,MAX_PATH,
                             TRUE);

        if (ppsc->iColumn == 8)
            iRet = dfUnicodeStrcmpi(dfwOnlyFilePath1,dfwOnlyFilePath2);
        if (iRet == 0)
            iRet = dfUnicodeStrcmpi(dfwOnlyFileName1,dfwOnlyFileName2);
    }

    if (iRet == 0)
        iRet = (lParam1 < lParam2) ? -1 : 1;
    if (ppsc->fInvert)
        iRet = -iRet;
    return iRet;
}



void DoSort(LPDWORD lpSortMap,DWORD dwNbElem,PFNLVCOMPARE Compare,LPARAM lParamSort)
{
DWORD i,j;
DWORD diPivot;
BOOL fOnePermut,fMoveIJ;
  if (dwNbElem < 2) return;
  diPivot = *(lpSortMap+(dwNbElem/2));
  i=0;
  j=dwNbElem-1;
  fOnePermut = FALSE;
/*
  if (dwNbElem==2)
  {
      OutputDebugString("2");
  }
*/
  do
    {
      fMoveIJ=FALSE;

      while (Compare(*(lpSortMap+i),diPivot,lParamSort) < 0)
      {
          i++;
          fMoveIJ=TRUE;
      }

      while ((j!=0) && (Compare(*(lpSortMap+j),diPivot,lParamSort) > 0))
      {
          j--;
          fMoveIJ=TRUE;
      }
      if (i <= j)  // if i==j , only i++ j--
        {
        DWORD diPermut;

          diPermut = *(lpSortMap+i);
          *(lpSortMap+i) = *(lpSortMap+j);
          *(lpSortMap+j) = diPermut;
          if (!fOnePermut)
            if ((Compare(*(lpSortMap+i),diPivot,lParamSort)!=0))
              fOnePermut=TRUE;
        }
    } while ((i < j) && (fMoveIJ));

  if (fOnePermut || (j+1<dwNbElem))
    if (j>=1) DoSort(lpSortMap,j+1,Compare,lParamSort);
  if (fOnePermut || (i>0))
    if (i<dwNbElem-1) DoSort(lpSortMap+i,dwNbElem-i,Compare,lParamSort);

  if ((i==0) && (j==0) && (!fOnePermut) && (dwNbElem>1))
      DoSort(lpSortMap+1,dwNbElem-1,Compare,lParamSort);
  else if ((i==dwNbElem-1) && (j==i) && (!fOnePermut) && (dwNbElem>1))
      DoSort(lpSortMap,dwNbElem-1,Compare,lParamSort);
}

#ifndef HDF_SORTUP
#define HDF_SORTUP              0x0400
#define HDF_SORTDOWN            0x0200
#endif

BOOL ShowSortColumnTitleBitmap(HWND hWndLV,int iColumn,BOOL fEraseOtherSort,BOOL fInvert)
{
    HWND hTitleBar = ListView_GetHeader(hWndLV);
    DWORD dwHeadFmtSortDown,dwHeadFmtSortUp,dwHeadFmtSortUpDown  ;
    HBITMAP hBitmapUpDown;
    BOOL fSortUp = !fInvert;

    dwHeadFmtSortUp = dwHeadFmtSortDown = HDF_BITMAP | HDF_BITMAP_ON_RIGHT;

    if (IsWhistler())
    {
      dwHeadFmtSortDown = HDF_SORTDOWN;
      dwHeadFmtSortUp = HDF_SORTUP;
    }

    if (fSortUp)
    {
        dwHeadFmtSortUpDown = dwHeadFmtSortUp;
        hBitmapUpDown = hBitmapUp;
    }
    else
    {
        dwHeadFmtSortUpDown = dwHeadFmtSortDown;
        hBitmapUpDown = hBitmapDown;
    }

    if (hTitleBar ==NULL)
        return FALSE;

    int iCount=Header_GetItemCount(hTitleBar);
    int i;

    if (fEraseOtherSort)
      for (i=0;i<iCount;i++)
          if (i!=iColumn)
          {
              HDITEM headerItem;
              BOOL fSetItem=FALSE;
              headerItem.mask = HDI_FORMAT | HDI_BITMAP;
              Header_GetItem(hTitleBar,i, &headerItem);
              if (((headerItem.fmt & (dwHeadFmtSortDown)) == dwHeadFmtSortDown) /*|| (headerItem.hbm != 0)*/)
              {
                  headerItem.fmt &= ~(dwHeadFmtSortDown);
                  fSetItem=TRUE;
              }
              else
              if (((headerItem.fmt & (dwHeadFmtSortUp)) == dwHeadFmtSortUp) /*|| (headerItem.hbm != 0)*/)
              {
                  headerItem.fmt &= ~(dwHeadFmtSortUp);
                  fSetItem=TRUE;
              }

              if (fSetItem)
              {
                if (headerItem.hbm != 0)
                {
                    //DeleteObject(headerItem.hbm);
                    headerItem.hbm = 0;
                }
                Header_SetItem(hTitleBar,i, &headerItem);
              }
          }


    if (iColumn<=iCount)
    {
        HDITEM headerItem;
        headerItem.mask = HDI_FORMAT | HDI_BITMAP;
        if (Header_GetItem(hTitleBar,iColumn, &headerItem))
        {
            if (headerItem.hbm != 0)
            {
                //DeleteObject(headerItem.hbm);
                headerItem.hbm = 0;
            }
            if (((headerItem.fmt & (dwHeadFmtSortDown)) == dwHeadFmtSortDown) /*|| (headerItem.hbm != 0)*/)
                headerItem.fmt &= ~(dwHeadFmtSortDown);
            else
            if (((headerItem.fmt & (dwHeadFmtSortUp)) == dwHeadFmtSortUp) /*|| (headerItem.hbm != 0)*/)
                headerItem.fmt &= ~(dwHeadFmtSortUp);

            headerItem.fmt |= dwHeadFmtSortUpDown;/*
            headerItem.hbm = (HBITMAP)LoadImage(AfxGetInstanceHandle(),
                MAKEINTRESOURCE(m_atSortArrow), IMAGE_BITMAP, 0, 0,
                LR_LOADMAP3DCOLORS);*/
            headerItem.hbm = hBitmapUpDown;
            Header_SetItem(hTitleBar,iColumn, &headerItem);
        }
    }

   return TRUE;
}

BOOL GUIITEM::DoSortColumn(DFSFILEANDINFO &DfsFileAndInfo,int iColumn,BOOL fInvert)
{
    static PARAMSORTCOLUMN psc;
    psc.iColumn = iColumn;
    psc.fInvert = fInvert;
    psc.pExtInfoCache = &ExtInfoCache;


    if ((DfsFileAndInfo.DfsFile == NULL) ||
        (((DWORD)DfsFileAndInfo.dfCurDir) == TVITEMPARAM_ROOT) ||
        (DfsFileAndInfo.pDirInfo==NULL))
        return FALSE;

    psc.pCurDirInfo = *(DfsFileAndInfo.pDirInfo+DfsFileAndInfo.dfCurDir);

    {
      LPBOOL lpfExtractItemMap;
      dfuLong32 i;
      lpfExtractItemMap = (BOOL*)DfsMalloc(sizeof(BOOL)*(dfListViewNbItem + 1));

      for (i=0;i<dfListViewNbItem;i++)
          *(lpfExtractItemMap+i)=FALSE;

      for (i=0;i<dfListViewNbItem;i++)
      {
          dfuLong32 iItem = *(pdfwListViewSortMap+i);
          *(lpfExtractItemMap+iItem) =
            ListView_GetItemState(hwndLV,i,LVIS_SELECTED) != 0;
      }

      DoSort(pdfwListViewSortMap,dfListViewNbItem,CompareFuncListView,(LPARAM)&psc);
      ListView_SetItemCount(hwndLV, 0);

      ShowSortColumnTitleBitmap(hwndLV,iColumn,TRUE,fInvert);



      ListView_SetItemCount(hwndLV, dfListViewNbItem);

      for (i=0;i<dfListViewNbItem;i++)
      {
          dfuLong32 iItem = *(pdfwListViewSortMap+i);
          BOOL fFlag = *(lpfExtractItemMap+iItem) ;
          ListView_SetItemState(hwndLV,i, fFlag ? LVIS_SELECTED : 0,LVIS_SELECTED);
      }
      DfsFree(lpfExtractItemMap);
    }

    return TRUE;
}

LRESULT DoSearch(PDIRINFO pCurDirInfo,LPDWORD pdfwListViewSortMap,LPCTSTR lpsz,int iStart,BOOL fWrap)
{
    int i;
    dfwchar dfwSearch[MAX_PATH+2];
    dfuLong32 dflnSrch;
    ConvertTCharToUnicode(lpsz,dfwSearch,MAX_PATH);
    dfwSearch[MAX_PATH]=0;
    dflnSrch = dfUnicodeStrlen(dfwSearch);
    if (dflnSrch>MAX_PATH)
        dflnSrch=MAX_PATH;


    for (i=iStart;i<(int)pCurDirInfo->dfNbFile;i++)
    {
        dfwchar dfwOnlyFileName[MAX_PATH+2];
        dfwchar dfwOnlyFilePath[MAX_PATH+2];
        DWORD iItem = *(pdfwListViewSortMap+i);
        dfwOnlyFilePath[0] = dfwOnlyFileName[0] = '\0';
        SplitFileNameAndPath((pCurDirInfo->pFileInDirInfo + iItem)->FileName,
                             dfwOnlyFilePath,MAX_PATH,
                             dfwOnlyFileName,MAX_PATH,
                             TRUE);
        dfwOnlyFileName[dflnSrch]=0;
        if (dfUnicodeStrcmpi(dfwOnlyFileName,dfwSearch)==0)
        {
            return i;
        }
    }

    if (fWrap)
      for (i=0;i<iStart;i++)
    {
        dfwchar dfwOnlyFileName[MAX_PATH+2];
        dfwchar dfwOnlyFilePath[MAX_PATH+2];
        DWORD iItem = *(pdfwListViewSortMap+i);
        dfwOnlyFilePath[0] = dfwOnlyFileName[0] = '\0';
        SplitFileNameAndPath((pCurDirInfo->pFileInDirInfo + iItem)->FileName,
                             dfwOnlyFilePath,MAX_PATH,
                             dfwOnlyFileName,MAX_PATH,
                             TRUE);
        dfwOnlyFileName[dflnSrch]=0;
        if (dfUnicodeStrcmpi(dfwOnlyFileName,dfwSearch)==0)
        {
            return i;
        }
    }

    return -1;
}

LRESULT DoNotifyHeaderOfListView(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,NMHDR* pnmHdr)
{
    return 0;
}


LRESULT DoDrawItemHeaderOfListView(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,LPDRAWITEMSTRUCT lpdis)
{
    return 0;
}


BOOL ComputeSelection(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo)
{
    BOOL fModified=FALSE;
    LPBOOL lpfExtractItemMap=NULL;
    dfuLong32 i;
    dfuLong32 dfNbSelected=0;
    dfuLong64 dfSizeSelected=0;
    dfuLong64 dfSizeTotal=0;
    dfuLong32 dfNbFileTotal=0;

    if (DfsFileAndInfo.dfCurDir != TVITEMPARAM_ROOT)
    {
        PDIRINFO pCurDirInfo = *(DfsFileAndInfo.pDirInfo+ DfsFileAndInfo.dfCurDir);
        lpfExtractItemMap = (BOOL*)DfsMalloc(sizeof(BOOL)*(pCurDirInfo ->dfNbFile + 1));

        dfNbFileTotal = pCurDirInfo ->dfNbFile ;

        for (i=0;i<pCurDirInfo ->dfNbFile;i++)
            *(lpfExtractItemMap+i)=TRUE;

        for (i=0;i<pCurDirInfo ->dfNbFile;i++)
        {
            dfuLong32 iItem = *(guiItem.pdfwListViewSortMap+i);
            *(lpfExtractItemMap+iItem) =
                ListView_GetItemState(guiItem.hwndLV,i,LVIS_SELECTED) != 0;
        }

        dfNbSelected=0;
        for (i=0;i<pCurDirInfo ->dfNbFile;i++)
        {
            if (*(lpfExtractItemMap+i))
            {
                dfNbSelected++;
                dfSizeSelected += (pCurDirInfo->pFileInDirInfo + i)->dfSize;
            }
            dfSizeTotal += (pCurDirInfo->pFileInDirInfo + i)->dfSize;
        }

        if (dfNbSelected==0)
            for (i=0;i<pCurDirInfo ->dfNbFile;i++)
                *(lpfExtractItemMap+i)=TRUE;
        DfsFree(lpfExtractItemMap);
    }
    if ((DfsFileAndInfo.dfNbFileSelectedCurDir != dfNbSelected) ||
        (DfsFileAndInfo.dfSizeSelectedCurDir != dfSizeSelected) ||
        (DfsFileAndInfo.dfSizeTotalCurDir != dfSizeTotal) ||
        (DfsFileAndInfo.dfNbFileTotalCurDir != dfNbFileTotal))
    {
        DfsFileAndInfo.dfNbFileSelectedCurDir = dfNbSelected;
        DfsFileAndInfo.dfSizeSelectedCurDir = dfSizeSelected;
        DfsFileAndInfo.dfSizeTotalCurDir = dfSizeTotal;
        DfsFileAndInfo.dfNbFileTotalCurDir = dfNbFileTotal;
        fModified=TRUE;
    }

    return fModified;
}

LRESULT DoNotifyListViewListFile(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,NMHDR* pnmHdr)
{
    NM_LISTVIEW * pnmlv=(NM_LISTVIEW *)pnmHdr;



    if (pnmHdr->code == NM_SETFOCUS)
    {
        guiItem.SetListViewFocused(TRUE);
    }

    if (pnmHdr->code == LVN_KEYDOWN)
    {
        LPNMLVKEYDOWN pnkd = (LPNMLVKEYDOWN) pnmHdr ;
        if (pnkd ->wVKey == VK_F6)
        {
            SetFocus(guiItem.hwndTreeView);
        }
    }

    if ((pnmHdr->code == LVN_GETDISPINFO) && (guiItem.pdfwListViewSortMap!=NULL))
    {
        NMLVDISPINFO* lpVDISPINFO = (NMLVDISPINFO*)pnmlv;
        //int iItem=lpVDISPINFO->item.iItem;
        int iItem;
        TCHAR szTxt[MAX_PATH*4];
        LPTSTR lpszTxt = szTxt;
        TCHAR szDate[MAX_PATH]="";
        dfwchar dfwOnlyFileName[MAX_PATH];
        dfwchar dfwOnlyFilePath[MAX_PATH];
        TCHAR szStatus[MAX_PATH]="";
        dfuLong32 dfCrc32=0;



        PDIRINFO pCurDirInfo = *(DfsFileAndInfo.pDirInfo+DfsFileAndInfo.dfCurDir);
        dfvoidp TagBuf;
        dfuLong32 TagSize;
        dfsLong32 dfPackedRatio=0;

        iItem = *(guiItem.pdfwListViewSortMap + lpVDISPINFO->item.iItem);

        if (GetTag(*(pCurDirInfo->TagFile + iItem), DFSTAG_STORAGESTATUS, &TagBuf, &TagSize))
        {
            if (TagSize==sizeof(dfuLong32Intel))
            {
                dfuLong32 dfFileIdentical = ConvertuLongIntelToLong(*(dfuLong32Intel*)TagBuf);
                UINT uiRes=0;
                switch (dfFileIdentical)
                {
                    case DFS_STORAGESTATUS_IDENTICAL:
                        uiRes = IDS_STATUS_IDENTICAL;
                        break;

                    case DFS_STORAGESTATUS_MODIFIED:
                        uiRes = IDS_STATUS_MODIFIED;
                        break;

                    case DFS_STORAGESTATUS_NEW:
                    case DFS_STORAGESTATUS_NEWSTORED:
                        uiRes = IDS_STATUS_NEW;
                        break;

                    case DFS_STORAGESTATUS_REFERENCE:
                        uiRes = IDS_STATUS_REFERENCE;
                        break;
                }

                if (uiRes != 0)
                    LoadString(ghInstRes,uiRes,
                     szStatus,(sizeof(szStatus)/sizeof(TCHAR))-1);
            }
        }
        else lstrcpy(szStatus,"not found");


        if ((pCurDirInfo->pFileInDirInfo + iItem)->fCrc32Filled)
          dfCrc32 = (pCurDirInfo->pFileInDirInfo + iItem)->dfCrc32;

        if ((pCurDirInfo->pFileInDirInfo + iItem)->dfSize>0)
        {
            dfsLong64 dfPackSize = ((dfsLong64)(pCurDirInfo->pFileInDirInfo + iItem)->dfSize) - ((signed)(pCurDirInfo->pFileInDirInfo + iItem)->dfFileEncodedSize);
            if (dfPackSize<0)
            {
                if ((pCurDirInfo->pFileInDirInfo + iItem)->dfSize>0)
                  dfPackedRatio = (dfsLong32)((dfPackSize*100)/((dfsLong64)((pCurDirInfo->pFileInDirInfo + iItem)->dfSize)));
                else
                  dfPackedRatio = 100;
            }
            else
                dfPackedRatio = CalculateRatio((dfuLong64)dfPackSize,(pCurDirInfo->pFileInDirInfo + iItem)->dfSize,100);
        }



        dfwOnlyFilePath[0] = dfwOnlyFileName[0] = '\0';
        SplitFileNameAndPath((pCurDirInfo->pFileInDirInfo + iItem)->FileName,
                             dfwOnlyFilePath,MAX_PATH,
                             dfwOnlyFileName,MAX_PATH,
                             TRUE);

        if(lpVDISPINFO->item.mask & LVIF_IMAGE)
        {
            char szFileName[MAX_PATH];
            wsprintf(szFileName,"%ws",dfwOnlyFileName);


            lpVDISPINFO->item.iImage=guiItem.ExtInfoCache.GetItemIndexImageCached(szFileName,FALSE);
        }



        if(lpVDISPINFO->item.mask & LVIF_TEXT)
        {
            if (lpVDISPINFO->item.iSubItem==0)
                wsprintf(lpVDISPINFO->item.pszText,"%ws",dfwOnlyFileName);

            if (lpVDISPINFO->item.iSubItem==1)
            {
                LPCTSTR lpszRegisteredType = "";
                char szFileName[MAX_PATH];
                wsprintf(szFileName,"%ws",dfwOnlyFileName);
                lpszRegisteredType = guiItem.ExtInfoCache.GetExtensionDescFromRegistryCached(szFileName,FALSE);

                if (lpszRegisteredType==NULL)
                    lpszRegisteredType="";

                wsprintf(lpVDISPINFO->item.pszText,"%s",lpszRegisteredType);
            }

            if (lpVDISPINFO->item.iSubItem==2)
                WinLong64ToStr((pCurDirInfo->pFileInDirInfo + iItem)->dfSize,lpVDISPINFO->item.pszText,MAX_PATH);

            if (lpVDISPINFO->item.iSubItem==3)
            {
                if (GetTag(*(pCurDirInfo->TagFile + iItem), DFSTAG_DATE, &TagBuf, &TagSize))
                {
                  DFSTM dfsTm;
                  ConvertDfsInfoDateToDfsTm((DFSINFODATE *) TagBuf, &dfsTm);
                  BuildStrDate(&dfsTm,lpVDISPINFO->item.pszText,MAX_PATH);
                }
            }

            if (lpVDISPINFO->item.iSubItem==4)
                wsprintf(lpVDISPINFO->item.pszText,"%02d %%",dfPackedRatio);

            if (lpVDISPINFO->item.iSubItem==5)
                WinLong64ToStr((pCurDirInfo->pFileInDirInfo + iItem)->dfFileEncodedSize,lpVDISPINFO->item.pszText,MAX_PATH);

            if (lpVDISPINFO->item.iSubItem==6)
            {
                wsprintf(lpVDISPINFO->item.pszText,"%08x",dfCrc32);
#ifdef _DEBUG
                if (((pCurDirInfo->pFileInDirInfo + iItem)->fCrc32Filled) && ((pCurDirInfo->pFileInDirInfo + iItem)->fMd5Filled))
                {
                    dfuLong32 i;
                    LPTSTR lpszCrcEnd;
                    lstrcat(lpVDISPINFO->item.pszText," ");
                    lpszCrcEnd = lpVDISPINFO->item.pszText + lstrlen(lpVDISPINFO->item.pszText);
                    for (i=0;i<16;i++)
                        wsprintf(lpszCrcEnd+(i*2),"%02x",(pCurDirInfo->pFileInDirInfo + iItem)->bMd5[i]);
                }
#endif
            }

            if (lpVDISPINFO->item.iSubItem==7)
                wsprintf(lpVDISPINFO->item.pszText,"%s",szStatus);

            if (lpVDISPINFO->item.iSubItem==8)
                wsprintf(lpVDISPINFO->item.pszText,"%ws",dfwOnlyFilePath);

        }
    }


    if (pnmHdr->code == NM_RCLICK)
    {
        {
            POINT pt;
            GetCursorPos(&pt);
            guiItem.DoPopupMenu(&pt,FALSE,FALSE);
        }
        return 0;
    }

   if (pnmHdr->code == LVN_BEGINLABELEDIT)
   {
       return TRUE;
   }

   if (pnmHdr->code == LVN_ODCACHEHINT)
   {
      LPNMLVCACHEHINT   lpCacheHint = (LPNMLVCACHEHINT)pnmHdr;
      /*
      This sample doesn't use this notification, but this is sent when the
      ListView is about to ask for a range of items. On this notification,
      you should load the specified items into your local cache. It is still
      possible to get an LVN_GETDISPINFO for an item that has not been cached,
      therefore, your application must take into account the chance of this
      occurring.
      */
      return 0;
   }


   if (pnmHdr->code ==  LVN_ODFINDITEM)
   {
      LPNMLVFINDITEM lpFindItem = (LPNMLVFINDITEM)pnmHdr;
      LRESULT lResult=-1;
      /*
      This sample doesn't use this notification, but this is sent when the
      ListView needs a particular item. Return -1 if the item is not found.
      */
      if (lpFindItem->lvfi.flags & LVFI_STRING)
          //if (lstrlen(lpFindItem->lvfi.psz)==1)
              lResult = DoSearch(*(DfsFileAndInfo.pDirInfo+DfsFileAndInfo.dfCurDir),
                                 guiItem.pdfwListViewSortMap,
                                 lpFindItem->lvfi.psz,lpFindItem->iStart,
                                 (lpFindItem->lvfi.flags & LVFI_WRAP)!=0);

      return lResult;
   }

    if (pnmHdr->code == LVN_COLUMNCLICK)
    {
        if (pnmlv->iSubItem != guiItem.GetColumnSort())
          guiItem.SetColumnSort(pnmlv->iSubItem,FALSE);
        else
          guiItem.SetColumnSort(pnmlv->iSubItem,!guiItem.GetColumnSortInvert());
        guiItem.DoSortColumn(DfsFileAndInfo,guiItem.GetColumnSort(),guiItem.GetColumnSortInvert());
    }

   if (pnmHdr->code ==  LVN_ITEMCHANGED) //LVN_ITEMACTIVATE)
   //if (pnmHdr->code ==  LVN_ODSTATECHANGED)
   {
       if (ComputeSelection(guiItem,DfsFileAndInfo))
       {
            guiItem.RefreshNormalStatusBar(DfsFileAndInfo);
       }
      RefreshGrayingMenu(guiItem,DfsFileAndInfo);
   }


    if (pnmHdr->code == LVN_BEGINDRAG)
    {
        DoSrvDrop(guiItem,DfsFileAndInfo);
        return TRUE;
    }

   return 0;
}


BOOL DoDblClickOrReturnListViewListDir(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,NMHDR* pnmHdr,BOOL fTrueMouseAction)
{
   LV_HITTESTINFO lvhti;
   UINT uiSel=0;
     if (fTrueMouseAction)
     {
            GetCursorPos(&lvhti.pt);
            ScreenToClient(guiItem.hwndLV, &lvhti.pt);
        uiSel = (UINT)ListView_HitTest(guiItem.hwndLV, &lvhti);
     }
     else
     {
     DWORD i;
     DWORD dwNbListViewItem = ListView_GetItemCount(guiItem.hwndLV);
         for (i=0;i<dwNbListViewItem;i++/*,lpdiParc++*/)
         {
             if (ListView_GetItemState(guiItem.hwndLV,i,LVIS_FOCUSED))
                 uiSel=(UINT)i;
         }
     }


     {
         UINT i;
         HTREEITEM hti = TreeView_GetFirstVisible(guiItem.hwndTreeView);
         hti=TreeView_GetChild(guiItem.hwndTreeView,hti);
         for (i=0;i<uiSel;i++)
             hti =TreeView_GetNextItem(guiItem.hwndTreeView,hti,TVGN_NEXT);
         TreeView_SelectItem(guiItem.hwndTreeView,hti);

     }

     return TRUE;
}

void DoGetDispInfoListViewDir(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,NMHDR* pnmlv)
{
        NMLVDISPINFO* lpVDISPINFO = (NMLVDISPINFO*)pnmlv;
        dfuLong32 iItem = lpVDISPINFO->item.iItem;
        PDIRINFO pCurDirInfo = (((*(DfsFileAndInfo.pDirInfo+iItem))));

        if(lpVDISPINFO->item.mask & LVIF_IMAGE)
          lpVDISPINFO->item.iImage = GetItemIndexImage(NULL,FALSE);



        if ((lpVDISPINFO->item.mask & LVIF_TEXT) && (lpVDISPINFO->item.iSubItem==0))
        {
          dfwcharpc pDirName=(dfwcharp)L"";
          dfvoidp TagBuf;
          dfuLong32 TagSize;
          DFTAGBLOCKFLOAT TagBlockFloat;

          /*
          if (GetTag(pCurDirInfo->TagDir, DFSTAG_DIR_NAME, &TagBuf, &TagSize))
              pDirName = (dfwcharp)TagBuf;
          */

          TagBlockFloat = GetDfsTagBlockFloat(DfsFileAndInfo.DfsFile,NULL);
          if (TagBlockFloat != NULL)
              if (GetTagBlockFloat(TagBlockFloat,iItem,FLOATNUM_NOSPECIFIC,DFSTAG_DIR_NAME_FLOAT,&TagBuf, &TagSize))
                  pDirName = (dfwcharp)TagBuf;

          wsprintf(lpVDISPINFO->item.pszText,"%ws",pDirName);
        }

        if ((lpVDISPINFO->item.mask & LVIF_TEXT) && (lpVDISPINFO->item.iSubItem==1))
        {
            wsprintf(lpVDISPINFO->item.pszText,"%u",iItem);
        }


        if ((lpVDISPINFO->item.mask & LVIF_TEXT) && (lpVDISPINFO->item.iSubItem==2))
        {
          UINT uID = GetUiResDescTypeDir(pCurDirInfo->dfTypeDir);
          if (uID == 0)
              *lpVDISPINFO->item.pszText='\0';
          else
          LoadString(ghInstRes,uID,
                     lpVDISPINFO->item.pszText,MAX_PATH);

        }

        if ((lpVDISPINFO->item.mask & LVIF_TEXT) &&
            ((lpVDISPINFO->item.iSubItem==3) ||
             (lpVDISPINFO->item.iSubItem==4) ||
             (lpVDISPINFO->item.iSubItem==5) ||
             (lpVDISPINFO->item.iSubItem==6)))
        {
            dfuLong32 i;
            dfuLong64 dfTotalSize=0;
            dfuLong64 dfTotalPacked = 0;
            dfuLong32 dfTotalRatio=0;
            DFSTM dfsTm;

            DfsClearStruct(&dfsTm,0,sizeof(dfsTm));

            for (i=0;i<pCurDirInfo->dfNbFile;i++)
            {
                dfvoidp TagBuf;
                dfuLong32 TagSize;
                dfTotalSize += (pCurDirInfo->pFileInDirInfo + i)->dfSize;
                dfTotalPacked +=(pCurDirInfo->pFileInDirInfo + i)->dfFileEncodedSize;

                if (lpVDISPINFO->item.iSubItem==4)
                    if (GetTag(*(pCurDirInfo->TagFile + i), DFSTAG_DATE, &TagBuf, &TagSize))
                    {
                      DFSTM dfsTmThis;
                      ConvertDfsInfoDateToDfsTm((DFSINFODATE *) TagBuf, &dfsTmThis);
                      if (CompareDfsTm(&dfsTm,&dfsTmThis)==-1)
                          dfsTm=dfsTmThis;
                    }
            }
            //dfTotalRatio = CalculateRatio(dfTotalPacked,dfTotalSize,100);

        {
            dfsLong64 dfPackSize = ((dfsLong64)(dfTotalSize)) - ((dfsLong64)dfTotalPacked);
            if (dfPackSize<0)
            {
                if (dfTotalSize>0)
                  dfTotalRatio = (dfsLong32)((dfPackSize*100)/dfTotalSize);
                else
                    dfTotalRatio = 100;
            }
            else
                dfTotalRatio = CalculateRatio((dfuLong64)dfPackSize,dfTotalSize,100);
        }

            if (lpVDISPINFO->item.iSubItem==3)
                WinLong64ToStr(dfTotalSize,lpVDISPINFO->item.pszText,MAX_PATH);
                //wsprintf(lpVDISPINFO->item.pszText,"%u",dfTotalSize);

            if (lpVDISPINFO->item.iSubItem==4)
                BuildStrDate(&dfsTm,lpVDISPINFO->item.pszText,MAX_PATH);

            if (lpVDISPINFO->item.iSubItem==5)
                wsprintf(lpVDISPINFO->item.pszText,"%u %%",dfTotalRatio);

            if (lpVDISPINFO->item.iSubItem==6)
                WinLong64ToStr(dfTotalPacked,lpVDISPINFO->item.pszText,MAX_PATH);
                //wsprintf(lpVDISPINFO->item.pszText,"%u",dfTotalPacked);
        }

        if ((lpVDISPINFO->item.mask & LVIF_TEXT) && (lpVDISPINFO->item.iSubItem==7))
        {
            if (DfsFileAndInfo.fBaseDirectorySelected && (DfsFileAndInfo.dfBaseDirNum == iItem))
              wsprintf(lpVDISPINFO->item.pszText,"%s",DfsFileAndInfo.lpBaseDirectory);
            else
                *lpVDISPINFO->item.pszText='\0';
        }
}

LRESULT DoNotifyListViewListDir(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,NMHDR* pnmHdr)
{
    NM_LISTVIEW * pnmlv=(NM_LISTVIEW *)pnmHdr;



    if (pnmHdr->code == NM_SETFOCUS)
    {
        guiItem.SetListViewFocused(TRUE);
    }

    if (pnmHdr->code == LVN_KEYDOWN)
    {
        LPNMLVKEYDOWN pnkd = (LPNMLVKEYDOWN) pnmHdr ;
        if (pnkd ->wVKey == VK_F6)
        {
            SetFocus(guiItem.hwndTreeView);
        }
    }

    if (pnmHdr->code == LVN_ITEMCHANGED)
    {
      RefreshGrayingMenu(guiItem,DfsFileAndInfo);
    }


    if (pnmHdr->code == NM_RCLICK)
    {
        {
            POINT pt;
            GetCursorPos(&pt);
            guiItem.DoPopupMenu(&pt,FALSE,TRUE);
        }
        return 0;
    }


    if (pnmHdr->code == NM_RETURN)
    {
        DoDblClickOrReturnListViewListDir(guiItem,DfsFileAndInfo,pnmHdr,FALSE);
    }


    if (pnmHdr->code == NM_DBLCLK)
    {
        DoDblClickOrReturnListViewListDir(guiItem,DfsFileAndInfo,pnmHdr,TRUE);
    }

    if (pnmHdr->code == LVN_GETDISPINFO)
    {
        DoGetDispInfoListViewDir(guiItem,DfsFileAndInfo,pnmHdr);
    }

    if (pnmHdr->code == LVN_ENDLABELEDIT)
    {
        NMLVDISPINFO* lpVDISPINFO = (NMLVDISPINFO*)pnmlv;

        #ifdef _DEBUG
        OutputDebugString("edit vername = '");
        OutputDebugString((lpVDISPINFO->item.pszText==NULL) ? "(null)" : lpVDISPINFO->item.pszText);
        OutputDebugString("'\n");
        #endif

        if (lpVDISPINFO->item.pszText!=NULL)
        {
            dfwcharp pNewDirName;
            dfuLong32 dfNewDirNameUnicodeLen;
            DFTAGBLOCKFLOAT TagBlockFloat;
            TagBlockFloat = GetDfsTagBlockFloat(DfsFileAndInfo.DfsFile,NULL);
            if (TagBlockFloat == NULL)
                return FALSE;


            pNewDirName = (dfwcharp)DfsMalloc((lstrlen(lpVDISPINFO->item.pszText)+0x10) *4);
            if (pNewDirName == NULL)
                return FALSE;
            ConvertTCharToUnicode(lpVDISPINFO->item.pszText,pNewDirName,(lstrlen(lpVDISPINFO->item.pszText)+0x04) *2);
            dfNewDirNameUnicodeLen = dfUnicodeStrlen(pNewDirName);

            if (dfNewDirNameUnicodeLen>0)
                AddTagBlockFloat(TagBlockFloat,lpVDISPINFO->item.iItem,FLOATNUM_NOSPECIFIC,DFSTAG_DIR_NAME_FLOAT,
                                pNewDirName,(dfNewDirNameUnicodeLen + 1) * 2);
            else
                RemoveTagBlockFloat(TagBlockFloat,lpVDISPINFO->item.iItem,FLOATNUM_NOSPECIFIC,DFSTAG_DIR_NAME_FLOAT);

            DfsFree(pNewDirName);
            SetDfsTagBlockFloatDirty(DfsFileAndInfo.DfsFile,NULL);
            //guiItem.FillTreeView(DfsFileAndInfo);
            guiItem.UpdateTreeViewItem(DfsFileAndInfo,lpVDISPINFO->item.iItem);

            return TRUE;
        }
        return TRUE;
    }


    return 0;
}

LRESULT DoNotifyListView(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,NMHDR* pnmHdr)
{
    if (guiItem.fCurrentColumnDirList)
        return DoNotifyListViewListDir(guiItem,DfsFileAndInfo,pnmHdr);
    else
        return DoNotifyListViewListFile(guiItem,DfsFileAndInfo,pnmHdr);
}

BOOL GUIITEM::DoSaveColumnWidth()
{
  BOOL fRet=FALSE;
  dfuLong32 i;
  if (fColumnInitialised && (!fCurrentColumnDirList))
  {
      for (i=0;i<NBLVCOLUMNFILELIST;i++)
      {
          iColSizeFileList[i] = ListView_GetColumnWidth(hwndLV,i);
      }
      fRet=TRUE;
  }
  if (fColumnInitialised && fCurrentColumnDirList)
  {
      for (i=0;i<NBLVCOLUMNDIRLIST;i++)
      {
          iColSizeDirList[i] = ListView_GetColumnWidth(hwndLV,i);
      }
      fRet=TRUE;
  }
  return fRet;
}

BOOL GUIITEM::InitListViewColumn(HWND hwndLV,BOOL fDirList,BOOL fMainAppListView)
{
  int i;

  if (fMainAppListView)
  {
    if (fColumnInitialised)
        DoSaveColumnWidth();

    EraseListView();

    for (i=0;i<max(NBLVCOLUMNDIRLIST,NBLVCOLUMNFILELIST);i++)
        ListView_DeleteColumn(hwndLV,0);
  }
/*
  {
      DWORD dwStyle = GetWindowLong(hwndLV, GWL_STYLE);
      if (fDirList)
          dwStyle = dwStyle | LVS_SINGLESEL;
      else
          dwStyle = dwStyle & (LVS_SINGLESEL ^ ((DWORD)0xffffffffL));
      SetWindowLong(hwndLV, GWL_STYLE,dwStyle);
  }
*/

  if (fDirList)
  {
      /*
      DWORD dwStyle = GetWindowLong(hwndLV, GWL_STYLE);
      //dwStyle = (dwStyle | LVS_NOSORTHEADER|LVS_NOCOLUMNHEADER)-(LVS_NOSORTHEADER|LVS_NOCOLUMNHEADER);
      dwStyle = dwStyle | LVS_NOSORTHEADER;
      SetWindowLong(hwndLV, GWL_STYLE,dwStyle);*/
      {
          HWND hWndHeader = ListView_GetHeader(hwndLV);
          DWORD dwStyle=GetWindowLong(hWndHeader, GWL_STYLE);
          dwStyle &= (HDS_BUTTONS ^ ((dfuLong32)0xffffffffL));
          SetWindowLong(hWndHeader, GWL_STYLE,dwStyle);
      }

      for (i=0;i<NBLVCOLUMNDIRLIST;i++)
      {
            LV_COLUMN lvC;
            TCHAR szTitle[MAX_PATH];


            lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            lvC.fmt = ((i==1) || (i==3) || (i==5) || (i==6)) ? LVCFMT_RIGHT : LVCFMT_LEFT;  // left-align column
            lvC.cx = 75;            // width of column in pixels
//            if (i==0) lvC.cx /=2 ;
//            if (i==1) lvC.cx *=2 ;
            if (i==0) lvC.cx *=2 ;
            if (i==1) lvC.cx /=2 ;
            if (i==4) lvC.cx = (lvC.cx*5)/3 ;
            if (i==5) lvC.cx = (lvC.cx*2)/3 ;
            if (i==7) lvC.cx = (lvC.cx*3)/2 ;
            lvC.pszText = szTitle;



          ShowSortColumnTitleBitmap(hwndLV,-1,TRUE,FALSE);
          LoadInternatString(IDS_TITLECOLUMNDIRLIST1+i,szTitle,sizeof(szTitle)-1);
          //szTitle[0]='$';
          ListView_InsertColumn(hwndLV, i, &lvC);

          if (iColSizeDirList[i]!=COLUMNWIDTH_UNKNOWN)
            ListView_SetColumnWidth(hwndLV,i,iColSizeDirList[i]);
      }
  }
  else
  {
      /*
      DWORD dwStyle = GetWindowLong(hwndLV, GWL_STYLE);
      dwStyle = dwStyle | LVS_NOSORTHEADER;
      SetWindowLong(hwndLV, GWL_STYLE,dwStyle);*/

      {
          HWND hWndHeader = ListView_GetHeader(hwndLV);
          DWORD dwStyle=GetWindowLong(hWndHeader, GWL_STYLE);
          dwStyle |= HDS_BUTTONS ;
          SetWindowLong(hWndHeader, GWL_STYLE,dwStyle);
      }

      for (i=0;i<NBLVCOLUMNFILELIST;i++)
      {

            LV_COLUMN lvC;
            TCHAR szTitle[MAX_PATH];

            lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            lvC.fmt = ((i==2) || (i==4) || (i==5)) ? LVCFMT_RIGHT : LVCFMT_LEFT;  // left-align column
            lvC.cx = 75;            // width of column in pixels
            if (i==0) lvC.cx *=2 ;
            if (i==3) lvC.cx = (lvC.cx*5)/3 ;
            if (i==4) lvC.cx = (lvC.cx*2)/3 ;
            if (i==8) lvC.cx = (lvC.cx*3)/2 ;
            lvC.pszText = szTitle;


          ShowSortColumnTitleBitmap(hwndLV,GetColumnSort(),FALSE,GetColumnSortInvert());
          LoadInternatString(IDS_TITLECOLUMNFILELIST1+i,szTitle,sizeof(szTitle)-1);
          ListView_InsertColumn(hwndLV, i, &lvC);

          if (iColSizeFileList[i]!=COLUMNWIDTH_UNKNOWN)
            ListView_SetColumnWidth(hwndLV,i,iColSizeFileList[i]);
      }
  }
  if (fMainAppListView)
  {
    fColumnInitialised=TRUE;
    fCurrentColumnDirList=fDirList;
  }
  return TRUE;
}

BOOL GUIITEM::InitListView(HWND hwndLV)
{
  InitListViewColumn(hwndLV,FALSE);
  InitListViewImageLists(hwndLV);
  return TRUE;
}

BOOL GUIITEM::EraseListView()
{
  ListView_DeleteAllItems(hwndLV);

  //if (fListViewVirtual)
      ListView_SetItemCount(hwndLV, 0);
      /*
  else
      ListView_DeleteAllItems(hwndLV);*/

  if (pdfwListViewSortMap != NULL)
      DfsFree(pdfwListViewSortMap);
  pdfwListViewSortMap = NULL;
  dfListViewNbItem = 0;

  return TRUE;
}

void GUIITEM::TreeViewFillItem(DFSFILEANDINFO &DfsFileAndInfo,TVINSERTSTRUCT& tv,dfuLong32 dfNumDir,LPTSTR lpszTxtBuf, int sizeTxtBuf)
{
      memset(&tv,0,sizeof(tv));
      tv.hParent=NULL;
      tv.hInsertAfter=TVI_LAST;
      tv.item.mask=TVIF_TEXT|TVIF_PARAM|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
      tv.item.lParam=dfNumDir;

      TCHAR szFmtVersion[256]="";
      TCHAR szVersionInfo[256]="";
      PDIRINFO pCurDirInfo = (((*(DfsFileAndInfo.pDirInfo+dfNumDir))));
      dfuLong32 dfTypeDir = pCurDirInfo->dfTypeDir;
      UINT uID;
      dfwcharpc pDirName=(dfwcharp)L"";
      dfvoidp TagBuf;
      dfuLong32 TagSize;
      DFTAGBLOCKFLOAT TagBlockFloat;

      uID = GetUiResDescTypeDir(dfTypeDir);

      if (uID!=0)
          LoadString(ghInstRes,uID,
                     szVersionInfo,sizeof(szVersionInfo)/sizeof(TCHAR));


      TagBlockFloat = GetDfsTagBlockFloat(DfsFileAndInfo.DfsFile,NULL);
      if (TagBlockFloat != NULL)
          if (GetTagBlockFloat(TagBlockFloat,dfNumDir,FLOATNUM_NOSPECIFIC,DFSTAG_DIR_NAME_FLOAT,&TagBuf, &TagSize))
              pDirName = (dfwcharp)TagBuf;

      if (dfUnicodeStrlen(pDirName)==0)
      {
        LoadString(ghInstRes,IDS_UNNAMEDVERSIONAME,
                    szFmtVersion,sizeof(szFmtVersion)/sizeof(TCHAR));

        wsprintf(lpszTxtBuf,szFmtVersion,dfNumDir);
      }
      else
          wsprintf(lpszTxtBuf,"%ws",pDirName);

      /*
      LoadString(ghInstRes,IDS_FMTVERSION,
                 szFmtVersion,sizeof(szFmtVersion)/sizeof(TCHAR));

      wsprintf(lpszTxtBuf,szFmtVersion,dfNumDir,pDirName,szVersionInfo);
      */

      if (DfsFileAndInfo.fBaseDirectorySelected && (DfsFileAndInfo.dfBaseDirNum == dfNumDir))
      {
          wsprintf(lpszTxtBuf+lstrlen(lpszTxtBuf)," %s",DfsFileAndInfo.lpBaseDirectory);
      }
      tv.item.pszText =lpszTxtBuf;
      tv.item.iImage = GetItemIndexImage(NULL,FALSE);

      tv.item.iImage = I_IMAGECALLBACK ;
      tv.item.iSelectedImage= I_IMAGECALLBACK ;
}

BOOL GUIITEM::FillTreeView(DFSFILEANDINFO &DfsFileAndInfo,dfuLong32 dfSelectVersion)
{
  dfuLong32 dfNumDir;
  HTREEITEM htiRoot;

  TreeView_DeleteAllItems(hwndTreeView);
  DfsFileAndInfo.dfCurDir = 0;
  DfsFileAndInfo.dfNbFileSelectedCurDir = 0;
  DfsFileAndInfo.dfSizeSelectedCurDir = 0;
  if ((DfsFileAndInfo.dfNbDir == 0) || (DfsFileAndInfo.DfsFile == NULL))
      return FALSE;

  {
      TVINSERTSTRUCT tv;
      tv.item.mask=TVIF_PARAM|TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
      tv.item.lParam=TVITEMPARAM_ROOT;
      tv.hParent=NULL;
      tv.hInsertAfter=TVI_LAST;
      tv.item.pszText = (LPTSTR)GetszFileName();
      tv.item.iImage = GetItemIndexImage(NULL,FALSE);

      tv.item.iImage = I_IMAGECALLBACK ;
      tv.item.iSelectedImage= I_IMAGECALLBACK ;

      htiRoot=TreeView_InsertItem(hwndTreeView,&tv);
  }

  if (dfSelectVersion==VERSION_NUMBER_DEFAULT)
      dfSelectVersion=DfsFileAndInfo.dfNbDir-1;

  for (dfNumDir = 0; dfNumDir < DfsFileAndInfo.dfNbDir; dfNumDir++)
  {
      TVINSERTSTRUCT tv;
      HTREEITEM hti;
      TCHAR szTxt[MAX_PATH+1];
      TreeViewFillItem(DfsFileAndInfo,tv,dfNumDir,szTxt,MAX_PATH);
      tv.hParent=htiRoot;
      hti=TreeView_InsertItem(hwndTreeView,&tv);
      if (dfNumDir==dfSelectVersion)
          TreeView_SelectItem(hwndTreeView,hti);
  }
  return TRUE;
}

BOOL GUIITEM::UpdateTreeViewItem(DFSFILEANDINFO &DfsFileAndInfo,dfuLong32 dfItemParam)
{
    HTREEITEM hti;
    UINT flag=TVGN_CHILD;
    if (dfItemParam==TVITEMPARAM_ROOT) return FALSE;
    hti = TreeView_GetRoot(hwndTreeView);
    while (hti != NULL)
    {
        TVITEM tv;
        tv.mask=TVIF_PARAM;
        tv.hItem = hti;
        tv.lParam = 0xffffffff;

        if (TreeView_GetItem(hwndTreeView, &tv))
          if (((dfuLong32)tv.lParam) == dfItemParam)
        {
            TVINSERTSTRUCT tvi;
            TCHAR szTxt[MAX_PATH+1];
            TreeViewFillItem(DfsFileAndInfo,tvi,dfItemParam,szTxt,MAX_PATH);
            tvi.item.hItem = hti;
            tvi.item.mask = TVIF_TEXT;
            TreeView_SetItem(hwndTreeView,&tvi.item);
            return TRUE;
        }
        hti = TreeView_GetNextItem(hwndTreeView, hti, flag);
        flag = TVGN_NEXT;
    }
    return FALSE;
}

void GUIITEM::UpdateStatusBar(HWND hWndStatusbar,LPSTR lpszStatusString, WORD partNumber, WORD displayFlags)
{
    SendMessage(hWndStatusbar,
                SB_SETTEXT,
                partNumber | displayFlags,
                (LPARAM)lpszStatusString);
}

BOOL GUIITEM::RefreshNormalStatusBar(DFSFILEANDINFO &DfsFileAndInfo)
{
    TCHAR  szBuffer[MAX_PATH];
    TCHAR  szFormat[MAX_PATH];
    UINT   nStringID = 0;

    if (DfsFileAndInfo.DfsFile == NULL)
    {
        nStringID = IDS_DESCRIPTION;
        LoadString(ghInstRes, nStringID, szBuffer, sizeof(szBuffer));
    }
    else
    {
        if (((DWORD)DfsFileAndInfo.dfCurDir) == TVITEMPARAM_ROOT)
        {
            LoadString(ghInstRes, IDS_STATUSBAR_INFO_ROOT, szFormat, sizeof(szFormat));
            wsprintf(szBuffer,szFormat,DfsFileAndInfo.dfNbDir);
        }
        else
        {
            TCHAR szSize[MAX_PATH+1];
            if (DfsFileAndInfo.dfNbFileSelectedCurDir>0)
            {
                LoadString(ghInstRes, IDS_STATUSBAR_INFO_SELECTION, szFormat, sizeof(szFormat));
                WinLong64ToStr(DfsFileAndInfo.dfSizeSelectedCurDir,szSize,MAX_PATH);
                wsprintf(szBuffer,szFormat,DfsFileAndInfo.dfNbFileSelectedCurDir,szSize);
            }
            else
            {
                LoadString(ghInstRes, IDS_STATUSBAR_INFO_NOSELECTION, szFormat, sizeof(szFormat));
                WinLong64ToStr(DfsFileAndInfo.dfSizeTotalCurDir,szSize,MAX_PATH);
                wsprintf(szBuffer,szFormat,DfsFileAndInfo.dfNbFileTotalCurDir,szSize);
            }
        }
    }
    UpdateStatusBar(hwndSB,szBuffer, 0, 0);
    return TRUE;
}



BOOL GUIITEM::FillListView(DFSFILEANDINFO &DfsFileAndInfo,BOOL fAlwaysFullRebuild)
{
  PDIRINFO pCurDirInfo;
  dfuLong32 i = 0;
  dfuLong32 dfError = DFS_SUCCESS;
  dfuLong32 dfNumDir = DfsFileAndInfo.dfCurDir;
  BOOL fDirList = (((DWORD)dfNumDir) == TVITEMPARAM_ROOT);


  //InitListView(guiItem);
  if (pdfwListViewSortMap != NULL)
      DfsFree(pdfwListViewSortMap);
  pdfwListViewSortMap=0;
  dfListViewNbItem = 0;

  if ((fDirList != fCurrentColumnDirList) || (fAlwaysFullRebuild))
  {
      dfuLong32 dfNbItem=0;
      DoSaveColumnWidth();
      InitListViewColumn(hwndLV,fDirList);

      if ((DfsFileAndInfo.dfNbDir != 0) && (DfsFileAndInfo.DfsFile != NULL))
          dfNbItem=DfsFileAndInfo.dfNbDir;
      ListView_SetItemCount(hwndLV, dfNbItem);
  }
  else
  {
      EraseListView();
  }

  if (fDirList)
  {
      return TRUE;
  }

  if ((DfsFileAndInfo.dfNbDir <= dfNumDir) || (DfsFileAndInfo.DfsFile == NULL))
  {
      return FALSE;
  }
  pCurDirInfo = *(DfsFileAndInfo.pDirInfo+dfNumDir);

  dfListViewNbItem = pCurDirInfo ->dfNbFile;


  ListView_SetItemCount(hwndLV, dfListViewNbItem);


  pdfwListViewSortMap = (LPDWORD)DfsMalloc((dfListViewNbItem+5)*sizeof(LPDWORD));
  for (i=0;i< dfListViewNbItem;i++)
      *(pdfwListViewSortMap+i)=i;

  return TRUE;
}

LRESULT DoNotifyTreeView(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,NMHDR* pnmHdr)
{
    NMTREEVIEW* pNmTreeView=(NMTREEVIEW*)pnmHdr;

    if (pnmHdr->code == TVN_KEYDOWN)
    {
        NMTVKEYDOWN* ptvkd = (LPNMTVKEYDOWN) pnmHdr ;
        if (ptvkd ->wVKey == VK_F6)
        {
            SetFocus(guiItem.hwndLV);
        }
    }

    if (pnmHdr->code == TVN_GETDISPINFO)
    {
        LPNMTVDISPINFO lptvdi = (LPNMTVDISPINFO) pnmHdr ;
        if ((lptvdi->item.mask & TVIF_IMAGE) != 0)
            lptvdi->item.iImage = GetItemIndexImage(NULL,FALSE);
        if ((lptvdi->item.mask & TVIF_SELECTEDIMAGE ) != 0)
            lptvdi->item.iSelectedImage = GetItemIndexImage(NULL,TRUE);
    }

    if (pnmHdr->code == TVN_SELCHANGED)
    {
        DfsFileAndInfo.dfCurDir=(dfuLong32)pNmTreeView->itemNew.lParam;
        DfsFileAndInfo.dfNbFileSelectedCurDir = 0;
        DfsFileAndInfo.dfSizeSelectedCurDir = 0;
        guiItem.FillListView(DfsFileAndInfo);
        guiItem.DoSortColumn(DfsFileAndInfo,guiItem.GetColumnSort(),guiItem.GetColumnSortInvert());
        ComputeSelection(guiItem,DfsFileAndInfo);
        RefreshGrayingMenu(guiItem,DfsFileAndInfo);
    }

    if (pnmHdr->code == NM_SETFOCUS)
    {
        guiItem.SetListViewFocused(FALSE);
    }

    if (pnmHdr->code == NM_RCLICK)
    {
        {
            POINT pt;
            GetCursorPos(&pt);


            {
                HTREEITEM htiClicked=NULL;
                HTREEITEM hti = TreeView_GetFirstVisible(guiItem.hwndTreeView);
                while (hti != NULL)
                {
                    RECT rc;
                    POINT ptClient=pt;
                    ScreenToClient(guiItem.hwndTreeView,&ptClient);
                    if (TreeView_GetItemRect(guiItem.hwndTreeView,hti,&rc,FALSE))
                    {
                        if (PtInRect(&rc,ptClient))
                        {
                            htiClicked=hti;
                            break;
                        }
                    }

                    hti = TreeView_GetNextVisible(guiItem.hwndTreeView,hti);
                }

                if (htiClicked!=NULL)
                {
                    if (TreeView_GetSelection(guiItem.hwndTreeView) != htiClicked)
                        TreeView_SelectItem(guiItem.hwndTreeView,htiClicked);
                }


            }
            guiItem.DoPopupMenu(&pt,TRUE,FALSE);
        }
        return 0;
    }

    return 0;
}

LRESULT DoNotifyToolTip(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,NMHDR*)
{
    return 0;
}

BOOL GUIITEM::SetDirtyFlag(BOOL fNewFlag)
{
    fDirty = fNewFlag;
    return DisplayNewTitleBar();
}

BOOL GUIITEM::SetszFileName(LPCTSTR lpszFileName,BOOL fChangeDirty,BOOL fNewFlag)
{
    if (fChangeDirty)
        fDirty = fNewFlag;
    fFileNameExist = (lpszFileName != NULL);
    lstrcpy(szFileName,fFileNameExist ? lpszFileName : (""));

#ifdef _DEBUG
    if (fFileNameExist)
    {
        TCHAR szFullPath[MAX_PATH+2];
        TCHAR szMsg[MAX_PATH*3];
        LPTSTR lpszFilePart=NULL;
        DWORD dwSizePath=GetFullPathName(lpszFileName,0,NULL,NULL);
        GetFullPathName(lpszFileName,MAX_PATH,szFullPath,&lpszFilePart);
        wsprintf(szMsg,"%u char :param='%s', FullPath='%s', filepart='%s'\n",dwSizePath,lpszFileName,szFullPath,lpszFilePart);
        OutputDebugString(szMsg);
    }
#endif

    return DisplayNewTitleBar();
}

BOOL GUIITEM::DisplayNewTitleBar()
{
    UINT uiFmt;
    TCHAR szFmt[MAX_PATH*2];
    TCHAR szTitle[MAX_PATH*4];

    if (!fFileNameExist)
        uiFmt = fRegistered ? IDS_TITLE_NOFILE : IDS_TITLE_UNREG_NOFILE;
    else if (lstrlen(szFileName)==0)
        uiFmt = fRegistered ? IDS_TITLE_NONAMED : IDS_TITLE_UNREG_NONAMED;
    else
        uiFmt = fRegistered ? IDS_TITLE_FMTFILE : IDS_TITLE_UNREG_FMTFILE;
    LoadString(ghInstRes, uiFmt, szFmt, sizeof(szFmt));
    wsprintf(szTitle,szFmt,szFileName);
    SetWindowText(hwndMain,szTitle);
    return TRUE;
}

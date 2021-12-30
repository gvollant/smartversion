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
#include "RegCode.h"
#include "DoExtracting.h"
#include "../../lib/engine/patchstream/common/DfsIoHlp.h"
#include "uiMain.h"
#include "DlgLnkNM.h"

//#include "MiscUtilWDFS.h"
#include "MiscUtil.h"

//#include "LoadIcon.h"
#include "../../lib/engine/svfile/common/ArrayTl.h"



#include "../../lib/engine/svfile/rebuild/ReMixDfs.h"
#include "../../lib/helper/rebuild/ReMixHelper.h"
#include "../../lib/engine/svfile/compress/AppendDfs.h"

#include "IOConfig.h"

#include "zlib.h"
#include "unzip.h"
#include "../../lib/hash/svf_sha256.h"
#include "../../lib/hash/svf_md5.h"
#include "../../lib/hash/svf_sha.h"


extern "C"
{
#include "iowin32.h"
}

#ifndef NO_HTMLHELP
#include "Htmlhelp.h"
#endif

BOOL PaintAppIcon(HWND hwnd,HINSTANCE hInstance,LPCSTR lpBitmapName,UINT x,UINT y)
{
  HDC         hdc;
  HBITMAP     hbmp;
  PAINTSTRUCT ps;
  COLORREF    oldClr;

  hbmp    = LoadBitmap(hInstance, lpBitmapName);
  if (hbmp == NULL)
      return FALSE;
  hdc     = BeginPaint(hwnd, &ps);

  oldClr = SetBkColor(hdc,GetSysColor(COLOR_BTNFACE));

  //TransparentBlt(hdc,4,10,hbmp,RGB(255,0,0),0,0);
  TransparentBlt(hdc,x,y,hbmp,RGB(255,0,0),0,0,0,0);
  //PaintBitmap(hdc,4,10,78,76,hbmp,0,0);

  oldClr = SetBkColor(hdc,oldClr);
  DeleteBitmap(hbmp);
  EndPaint(hwnd, &ps);
  return TRUE;
}

BOOL About_PaintBitmap(HWND hwnd)
{
    return PaintAppIcon(hwnd,GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_APPICONABOUT),10,10);
}

BOOL CALLBACK AboutDlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
    switch (message) {

        case WM_COMMAND:    ///IDC_WEBSITELINK
            {
                WORD wId = GET_WM_COMMAND_ID(wParam,lParam);

                if ((wId == IDOK) || (wId == IDCANCEL))
                {
                    EndDialog(hDlg, (wId == IDOK));
                    return (TRUE);
                }
                break;
            }

        case WM_PAINT:
            {
            About_PaintBitmap(hDlg);
            return TRUE;
            }



    }
    return (FALSE);                           /* Didn't process a message    */
}


BOOL DoAbout(const GUIITEM &guiItem)
{
    DialogBox(ghInstRes, MAKEINTRESOURCE(IDD_ABOUT), guiItem.GetHwndMain() , (DLGPROC)AboutDlgProc);
    return TRUE;
}

typedef struct
{
  BOOL fRegistered;
  DWORD dwCurDayUse;
  DWORD dwTotalDayUseAllowed;
} SPLASHPARAM;



BOOL CALLBACK SplashDlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
    switch (message) {
        case WM_INITDIALOG:
            {
                SPLASHPARAM* pSplashParam=(SPLASHPARAM*)lParam;
                if (pSplashParam->fRegistered)
                {
                    SetDlgItemText(hDlg,IDC_REGLINE1,"");
                    SetDlgItemText(hDlg,IDC_REGLINE2,"");
                }
                else
                {
                    TCHAR szLine1[MAX_PATH+0x40] = "";
                    TCHAR szLine2[MAX_PATH+0x40] = "";
                    TCHAR szFormat1[MAX_PATH+1];
                    LoadString(ghInstRes,IDS_LINE1SPLASHFMT,szFormat1,sizeof(szFormat1)/sizeof(TCHAR));
                    wsprintf(szLine1,szFormat1,pSplashParam->dwCurDayUse,pSplashParam->dwTotalDayUseAllowed);
                    if (pSplashParam->dwCurDayUse > pSplashParam->dwTotalDayUseAllowed)
                    {
                        LoadString(ghInstRes,IDS_LINE2EVALFINISHED,szLine2,sizeof(szLine2)/sizeof(TCHAR));
                    }
                    SetDlgItemText(hDlg,IDC_REGLINE1,szLine1);
                    SetDlgItemText(hDlg,IDC_REGLINE2,szLine2);
                }
            }

        case WM_COMMAND:    ///IDC_WEBSITELINK
            {
                WORD wId = GET_WM_COMMAND_ID(wParam,lParam);

                if ((wId == IDOK) || (wId == IDCANCEL))
                {
                    EndDialog(hDlg, (wId == IDOK));
                    return (TRUE);
                }
                break;
            }

        case WM_PAINT:
            {
            PaintAppIcon(hDlg,GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_APPICONABOUT),10,10);
            return TRUE;
            }



    }
    return (FALSE);                           /* Didn't process a message    */
}


BOOL DoSplash(const GUIITEM &guiItem,BOOL fRegistered,DWORD dwCurDayUse,DWORD dwTotalDayUseAllowed)
{
    SPLASHPARAM SplashParam;
    SplashParam.dwCurDayUse = dwCurDayUse;
    SplashParam.dwTotalDayUseAllowed = dwTotalDayUseAllowed;
    SplashParam.fRegistered = fRegistered;

    DialogBoxParam(ghInstRes, MAKEINTRESOURCE(IDD_SPLASH), guiItem.GetHwndMain() , (
                   DLGPROC)SplashDlgProc,(LPARAM)&SplashParam);
    return TRUE;
}

BOOL DoHelpContents(HWND hWnd,LPTSTR lpszAddUrl)
{
    TCHAR szModuleFileName[MAX_PATH+1];
    TCHAR szChmFileName[MAX_PATH+1];
    LPTSTR lpFileNamePart=NULL;
    GetModuleFileName(NULL,szModuleFileName,MAX_PATH);
    GetFullPathName(szModuleFileName,MAX_PATH,szChmFileName,&lpFileNamePart);
    if (lpFileNamePart != NULL)
    {
        TCHAR szChmName[MAX_PATH+2];
        LoadString(ghInstRes,IDS_CHMFILENAME,szChmName,MAX_PATH);
        //lstrcpy(lpFileNamePart,"smartvs.chm");
        lstrcpy(lpFileNamePart,szChmName);

        //lstrcat(lpFileNamePart,"::/What_are_Patches_.htm");
        //lstrcat(lpFileNamePart,"::/Ordering_SmartVersion.htm");
        if (lpszAddUrl!=NULL)
            lstrcat(lpFileNamePart,lpszAddUrl);
#ifndef NO_HTMLHELP
        HWND hwndHelp =
        HtmlHelp(
                    hWnd,
                    szChmFileName,
                    HH_DISPLAY_TOPIC,
                    NULL) ;
#endif
    }
    return TRUE;
}

BOOL DoVisitWebSite(const GUIITEM &guiItem,UINT uiResUrl)
{
    TCHAR szUrl[MAX_PATH+1];
    LoadString(ghInstRes,uiResUrl,szUrl,sizeof(szUrl)/sizeof(TCHAR));

    HINSTANCE result = ::ShellExecute(NULL, ("open"), szUrl,
         NULL, NULL, SW_SHOWNORMAL);
    return (((LRESULT)result)>32);
}

typedef struct
{
    COMPRESSIONPARAM* pCompressionParam;
    BOOL* pfOverwriteExtracting;

    BOOL* pfSelectTempMemSize;
    DWORD* pdwTempMemSize;
    BOOL* pfSelectTempPath;
    LPTSTR lpszTempPath;
    DWORD dwTempPathBufSize;
    BOOL * pfStripIdentical;
    //BOOL * pfCompressLzma;
    BOOL * pfMd5;
    BOOL * pfSha1;
	BOOL * pfSha256;
    BOOL * pfUserSelectCprParam;
    DWORD* pdwLangageValue;
} SETTINGDLGPARAM;

void SetTempPathProperties(HWND hDlg,BOOL fTempPathSpecified)
{
    if (!fTempPathSpecified)
        {
            WCHAR szwWinTemp[MAX_PATH+1];
            szwWinTemp[0]=0;
            GetTempDirectorySystem((dfwcharp)szwWinTemp,MAX_PATH);
            #ifdef UNICODE
            SetDlgItemText(hDlg,IDC_EDITTEMPDIR,szwWinTemp);
            #else
            {
                TCHAR szWinTemp[(MAX_PATH*2)+1];
                ConvertUnicodeToAnsi((dfwcharp)szwWinTemp,szWinTemp,MAX_PATH*2);
                SetDlgItemText(hDlg,IDC_EDITTEMPDIR,szWinTemp);
            }
            #endif
        }

    EnableWindow(GetDlgItem(hDlg,IDC_EDITTEMPDIR),fTempPathSpecified);
    EnableWindow(GetDlgItem(hDlg,IDC_BROWSETEMPDIR),fTempPathSpecified);
}

static int CALLBACK GDE_BrowseCallbackProc(HWND hwnd,UINT uMsg,LPARAM lParam,LPARAM lpData);

#ifndef BIF_USENEWUI
#define BIF_NEWDIALOGSTYLE     0x0040   // Use the new dialog layout with the ability to resize
                                        // Caller needs to call OleInitialize() before using this API
#define BIF_USENEWUI           (BIF_NEWDIALOGSTYLE | BIF_EDITBOX)
#endif
BOOL BrowseForFolderForTempDir(HWND hDlg)
{
  LPMALLOC pMalloc;
  BOOL     fReturn = FALSE;

  if(SUCCEEDED(::SHGetMalloc(&pMalloc)))
  {
    LPITEMIDLIST pidlBrowse;
    BROWSEINFO   bi;
    //TCHAR szText[MAX_PATH+1];
    TCHAR szPath[MAX_PATH+1];

    //::LoadString(ghinst, IDS_FOLDER_BROWSE, szText, MAX_PATH);
    ::ZeroMemory(szPath, sizeof(szPath));
    ::ZeroMemory(&bi, sizeof(BROWSEINFO));
    //mFolderPath.GetText(szPath,MAX_PATH);
    GetDlgItemText(hDlg,IDC_EDITTEMPDIR,szPath,MAX_PATH);

    bi.hwndOwner      = hDlg;
    bi.pidlRoot       = NULL;
    bi.pszDisplayName = szPath;
    bi.lpszTitle      = NULL;//szText;
    bi.ulFlags        = BIF_RETURNONLYFSDIRS | BIF_RETURNFSANCESTORS | BIF_USENEWUI | BIF_VALIDATE;
    bi.lpfn           = GDE_BrowseCallbackProc;//NULL;
    bi.lParam         = (LPARAM)szPath;
    bi.iImage         = 0;

    pidlBrowse = ::SHBrowseForFolder(&bi);
    if(pidlBrowse)
    {
      if(::SHGetPathFromIDList(pidlBrowse, szPath))
          SetDlgItemText(hDlg,IDC_EDITTEMPDIR,szPath);
    }
      // clean-up
    pMalloc->Free(pidlBrowse);
    pMalloc->Release();
  }

  return TRUE;
}

void AdaptEnableAndSlideValue(HWND hDlg)
{
    HWND hCtlSlider;
    BOOL fEnabled = IsWindowEnabled(GetDlgItem(hDlg,IDC_SLIDERCOMPRESSION));
    BOOL fEnableNeeded =
        IsDlgButtonChecked(hDlg,IDC_MANUALYSELECTCPRPREF) == BST_CHECKED;

    if (fEnabled != fEnableNeeded)
        EnableWindow(GetDlgItem(hDlg,IDC_SLIDERCOMPRESSION),fEnableNeeded);

    if ((hCtlSlider=GetDlgItem(hDlg,IDC_SLIDERCOMPRESSION))!=NULL)
    {
        TCHAR szOldText[MAX_PATH+8]="";
        TCHAR szNewText[MAX_PATH+8];
        DWORD dwSlideValue = (DWORD)SendMessage(hCtlSlider,TBM_GETPOS,0,0);
        if (fEnableNeeded)
            wsprintf(szNewText,"%u",dwSlideValue);
        else
            szNewText[0] = 0;
        GetDlgItemText(hDlg,IDC_STATICBLOCKSIZE,szOldText,MAX_PATH);
        if (lstrcmp(szOldText,szNewText)!=0)
            SetDlgItemText(hDlg,IDC_STATICBLOCKSIZE,szNewText);
    }


	if ((hCtlSlider = GetDlgItem(hDlg, IDC_SLIDECOMPRPREF)) != NULL)
	{

		TCHAR szNewText[MAX_PATH + 8];
		DWORD dwSlideValue = (DWORD)SendMessage(hCtlSlider, TBM_GETPOS, 0, 0);
		wsprintf(szNewText, "%u", dwSlideValue);
		SetDlgItemText(hDlg, IDC_STATICCOMPPREF, szNewText);
	}
}



BOOL CALLBACK SettingDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  SETTINGDLGPARAM* psdp;
  if (message == WM_INITDIALOG)
    MySetWindowLongPtr(hDlg, DWLP_USER, lParam);
  psdp = (SETTINGDLGPARAM*)MyGetWindowLongPtr(hDlg, DWLP_USER);
  switch (message) {


  case WM_INITDIALOG:
  {
    dfuLong32 compressionRatio = psdp->pCompressionParam->uZlibCompressRatio;
    BOOL fLZma = (compressionRatio >= 31) && (compressionRatio <= 69);
    BOOL fLZ4 = (compressionRatio >= 71) && (compressionRatio <= 129);
    BOOL fZstd = (compressionRatio >= 171) && (compressionRatio <= 222);
    BOOL fLzham = (compressionRatio >= 231) && (compressionRatio <= 269);
    BOOL fZlib = !(fLZma || fLZ4 || fZstd || fLzham);

    HWND hCtl;


    if ((hCtl = GetDlgItem(hDlg, IDC_OVERWRITE)) != NULL)
    {
      CheckDlgButton(hDlg, IDC_OVERWRITE, (!(*(psdp->pfOverwriteExtracting))) ? BST_CHECKED : BST_UNCHECKED);
    }


    if ((hCtl = GetDlgItem(hDlg, IDC_CHECKCUSTVALUEMEMTEMPCACHE)) != NULL)
    {
      BOOL fSelectTempMemSize;
      fSelectTempMemSize = ((*(psdp->pfSelectTempMemSize)));
      CheckDlgButton(hDlg, IDC_CHECKCUSTVALUEMEMTEMPCACHE, fSelectTempMemSize ? BST_CHECKED : BST_UNCHECKED);
      EnableWindow(GetDlgItem(hDlg, IDC_EDITCUSTVALUEMEMTEMPCACHE), fSelectTempMemSize);
      EnableWindow(GetDlgItem(hDlg, IDC_STATICCUSTVALUEMEMTEMPCACHE), fSelectTempMemSize);
      if (fSelectTempMemSize)
        SetDlgItemInt(hDlg, IDC_EDITCUSTVALUEMEMTEMPCACHE, *psdp->pdwTempMemSize, FALSE);
      else
        SetDlgItemText(hDlg, IDC_EDITCUSTVALUEMEMTEMPCACHE, "0");
    }

    if ((hCtl = GetDlgItem(hDlg, IDC_CHECKSELECTTEMPDIR)) != NULL)
    {
      BOOL fSelectTempPath = *(psdp->pfSelectTempPath);
      CheckDlgButton(hDlg, IDC_CHECKSELECTTEMPDIR, fSelectTempPath ? BST_CHECKED : BST_UNCHECKED);
      EnableWindow(GetDlgItem(hDlg, IDC_EDITTEMPDIR), fSelectTempPath);
      EnableWindow(GetDlgItem(hDlg, IDC_BROWSETEMPDIR), fSelectTempPath);
    }

    if ((hCtl = GetDlgItem(hDlg, IDC_MANUALYSELECTCPRPREF)) != NULL)
    {
      CheckDlgButton(hDlg, IDC_MANUALYSELECTCPRPREF, ((*(psdp->pfUserSelectCprParam))) ? BST_CHECKED : BST_UNCHECKED);
    }

    if ((hCtl = GetDlgItem(hDlg, IDC_STRIPIDENTICAL)) != NULL)
    {
      CheckDlgButton(hDlg, IDC_STRIPIDENTICAL, ((*(psdp->pfStripIdentical))) ? BST_CHECKED : BST_UNCHECKED);
    }

    if ((hCtl = GetDlgItem(hDlg, IDC_MD5)) != NULL)
    {
      CheckDlgButton(hDlg, IDC_MD5, ((*(psdp->pfMd5))) ? BST_CHECKED : BST_UNCHECKED);
    }

    if ((hCtl = GetDlgItem(hDlg, IDC_SHA1)) != NULL)
    {
      CheckDlgButton(hDlg, IDC_SHA1, ((*(psdp->pfSha1))) ? BST_CHECKED : BST_UNCHECKED);
    }

    if ((hCtl = GetDlgItem(hDlg, IDC_SHA256)) != NULL)
    {
      CheckDlgButton(hDlg, IDC_SHA256, ((*(psdp->pfSha256))) ? BST_CHECKED : BST_UNCHECKED);
    }


    if ((hCtl = GetDlgItem(hDlg, IDC_SLIDECOMPRPREF)) != NULL)
    {
      SendMessage(hCtl, TBM_SETRANGE, (WPARAM)TRUE, MAKELONG(0, 9));
      unsigned int ratio = (psdp->pCompressionParam->uZlibCompressRatio % 10);
      if (fZstd) ratio = ratio / 2;
      SendMessage(hCtl, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(ratio));
      TCHAR szNewText[0x20];
      wsprintf(szNewText, "%u", (unsigned int)(psdp->pCompressionParam->uZlibCompressRatio % 10));
      SetDlgItemText(hDlg, IDC_STATICCOMPPREF, szNewText);
      //SetDlgItemTextLoadString(hDlg,IDC_COMPRESSINFO,IDS_COMP0+iLevelSet);
      //iLatestShowedLevel=iLevelSet;
    }

    {
      CheckRadioButton(hDlg, IDC_COMPRESSLZ4, IDC_COMPRESSLZMA,
        fLZma ? IDC_COMPRESSLZMA :
        (fLZ4 ? IDC_COMPRESSLZ4 :
        (fZstd ? IDC_COMPRESSZSTD :
          (fLzham ? IDC_COMPRESSLZHAM : IDC_COMPRESSDEFLATE))));
    }

    if ((hCtl = GetDlgItem(hDlg, IDC_SLIDERCOMPRESSION)) != NULL)
    {
      int iLevelSet = psdp->pCompressionParam->dfBlockCalcSizeSearch;

      SendMessage(hCtl, TBM_SETRANGE, (WPARAM)TRUE, MAKELONG(8, 512));
      SendMessage(hCtl, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)iLevelSet);
      //SetDlgItemTextLoadString(hDlg,IDC_COMPRESSINFO,IDS_COMP0+iLevelSet);
      //iLatestShowedLevel=iLevelSet;
    }

    BOOL fTempPathSpecified = (*(psdp->lpszTempPath)) != 0;
    if (fTempPathSpecified)
    {
      SetDlgItemText(hDlg, IDC_EDITTEMPDIR, psdp->lpszTempPath);
    }

    SetTempPathProperties(hDlg, fTempPathSpecified);

    {
      WORD i;
      HWND hCtlCombo = GetDlgItem(hDlg, IDC_COMBOLANGAGESELECT);
      for (i = 0; i<GetNbLang(); i++)
      {
        TCHAR szItemName[MAX_PATH];
        GetLangName(szItemName, i);
        int iItem = ComboBox_AddString(hCtlCombo, szItemName);
        ComboBox_SetItemData(hCtlCombo, iItem, i);
        if (i == GetCurLang())
          ComboBox_SetCurSel(hCtlCombo, iItem);
      }
    }

    AdaptEnableAndSlideValue(hDlg);
    break;
  }

  case WM_NOTIFY:
  {
    AdaptEnableAndSlideValue(hDlg);
    break;
  }

  case WM_COMMAND:    ///IDC_WEBSITELINK
  {
    WORD wId = GET_WM_COMMAND_ID(wParam, lParam);

    if ((wId == IDC_MANUALYSELECTCPRPREF) || (wId == IDC_SLIDERCOMPRESSION) || (wId == IDC_SLIDECOMPRPREF))
      AdaptEnableAndSlideValue(hDlg);



    if ((wId == IDC_COMPRESSLZMA) || (wId == IDC_COMPRESSDEFLATE) || (wId == IDC_COMPRESSLZ4) || (wId == IDC_COMPRESSZSTD) || (wId == IDC_COMPRESSLZHAM))
    {
      CheckRadioButton(hDlg, IDC_COMPRESSLZ4, IDC_COMPRESSLZMA, wId);
    }




    if (wId == IDC_CHECKCUSTVALUEMEMTEMPCACHE)
    {
      BOOL fSelectTempMemSize = (IsDlgButtonChecked(hDlg, IDC_CHECKCUSTVALUEMEMTEMPCACHE) == BST_CHECKED);

      EnableWindow(GetDlgItem(hDlg, IDC_EDITCUSTVALUEMEMTEMPCACHE), fSelectTempMemSize);
      EnableWindow(GetDlgItem(hDlg, IDC_STATICCUSTVALUEMEMTEMPCACHE), fSelectTempMemSize);
    }

    if (wId == IDC_CHECKSELECTTEMPDIR)
    {
      BOOL fSelectTempPath = (IsDlgButtonChecked(hDlg, IDC_CHECKSELECTTEMPDIR) == BST_CHECKED);

      EnableWindow(GetDlgItem(hDlg, IDC_EDITTEMPDIR), fSelectTempPath);
      EnableWindow(GetDlgItem(hDlg, IDC_BROWSETEMPDIR), fSelectTempPath);
    }

    if (wId == IDC_CHECKSELECTTEMPDIR)
    {
      BOOL fTempPathSpecified = IsDlgButtonChecked(hDlg, IDC_CHECKSELECTTEMPDIR) == BST_CHECKED;
      SetTempPathProperties(hDlg, fTempPathSpecified);
    }

    if (wId == IDC_GETHELP)
    {
      DoHelpContents(hDlg,//*pADDCONTENTDFSDLGParam->pGuiItem,
        "::/Settings.htm");
    }

    if (wId == IDC_BROWSETEMPDIR)
      BrowseForFolderForTempDir(hDlg);

    if (wId == IDOK)
    {
      HWND hCtl;
      BOOL fSelectTempPath = (IsDlgButtonChecked(hDlg, IDC_CHECKSELECTTEMPDIR) == BST_CHECKED);
      BOOL fSelectTempMemSize = (IsDlgButtonChecked(hDlg, IDC_CHECKCUSTVALUEMEMTEMPCACHE) == BST_CHECKED);
      BOOL fLzma = FALSE;
      BOOL fLz4 = FALSE;
      BOOL fZstd = FALSE;
      BOOL fLzham = FALSE;
      BOOL fDeflate;
      dfuLong32 ratio;
      dfuLong32 uZlibCompressRatio = psdp->pCompressionParam->uZlibCompressRatio;

      if ((hCtl = GetDlgItem(hDlg, IDC_COMPRESSLZMA)) != NULL)
        fLzma = IsDlgButtonChecked(hDlg, IDC_COMPRESSLZMA) == BST_CHECKED;

      if ((hCtl = GetDlgItem(hDlg, IDC_COMPRESSLZ4)) != NULL)
        fLz4 = IsDlgButtonChecked(hDlg, IDC_COMPRESSLZ4) == BST_CHECKED;

      if ((hCtl = GetDlgItem(hDlg, IDC_COMPRESSLZ4)) != NULL)
        fZstd = IsDlgButtonChecked(hDlg, IDC_COMPRESSZSTD) == BST_CHECKED;

      if ((hCtl = GetDlgItem(hDlg, IDC_COMPRESSLZ4)) != NULL)
        fLzham = IsDlgButtonChecked(hDlg, IDC_COMPRESSLZHAM) == BST_CHECKED;

      fDeflate = !(fLzma || fLz4 || fZstd || fLzham);

      if ((hCtl = GetDlgItem(hDlg, IDC_SLIDERCOMPRESSION)) != NULL)
        psdp->pCompressionParam->dfBlockCalcSizeSearch = (dfuLong32)SendMessage(hCtl, TBM_GETPOS, 0, 0);

      if ((hCtl = GetDlgItem(hDlg, IDC_OVERWRITE)) != NULL)
        (*(psdp->pfOverwriteExtracting)) = IsDlgButtonChecked(hDlg, IDC_OVERWRITE) != BST_CHECKED;


      if ((hCtl = GetDlgItem(hDlg, IDC_SLIDECOMPRPREF)) != NULL)
        ratio = (dfuLong32)SendMessage(hCtl, TBM_GETPOS, 0, 0);


      if (fLzma)
        ratio += 40;

      if (fLz4)
        ratio += 110;

      if (fLzham)
        ratio += 250;

      if (fZstd) // zstd between 171 and 190
      {
        ratio = (ratio * 2) + 170;
        if (ratio >= 188) ratio = 190;
      }
      psdp->pCompressionParam->uZlibCompressRatio = ratio;

      if ((hCtl = GetDlgItem(hDlg, IDC_STRIPIDENTICAL)) != NULL)
        (*(psdp->pfStripIdentical)) = IsDlgButtonChecked(hDlg, IDC_STRIPIDENTICAL) == BST_CHECKED;

      if ((hCtl = GetDlgItem(hDlg, IDC_MD5)) != NULL)
        (*(psdp->pfMd5)) = IsDlgButtonChecked(hDlg, IDC_MD5) == BST_CHECKED;

      if ((hCtl = GetDlgItem(hDlg, IDC_SHA1)) != NULL)
        (*(psdp->pfSha1)) = IsDlgButtonChecked(hDlg, IDC_SHA1) == BST_CHECKED;
      if ((hCtl = GetDlgItem(hDlg, IDC_SHA256)) != NULL)
        (*(psdp->pfSha256)) = IsDlgButtonChecked(hDlg, IDC_SHA256) == BST_CHECKED;

      *(psdp->pfSelectTempPath) = fSelectTempPath;
      if (fSelectTempPath)
      {
        GetDlgItemText(hDlg, IDC_EDITTEMPDIR, psdp->lpszTempPath, psdp->dwTempPathBufSize);
      }
      else *(psdp->lpszTempPath) = 0;

      *(psdp->pfSelectTempMemSize) = fSelectTempMemSize;
      if (fSelectTempMemSize)
        *psdp->pdwTempMemSize = GetDlgItemInt(hDlg, IDC_EDITCUSTVALUEMEMTEMPCACHE, NULL, FALSE);
      else
        *psdp->pdwTempMemSize = 0;

      if ((hCtl = GetDlgItem(hDlg, IDC_MANUALYSELECTCPRPREF)) != NULL)
        *psdp->pfUserSelectCprParam = IsDlgButtonChecked(hDlg, IDC_MANUALYSELECTCPRPREF) == BST_CHECKED;

      if ((hCtl = GetDlgItem(hDlg, IDC_COMBOLANGAGESELECT)) != NULL)
        *psdp->pdwLangageValue = (DWORD)(ComboBox_GetItemData(hCtl, ComboBox_GetCurSel(hCtl)));
    }

    if ((wId == IDOK) || (wId == IDCANCEL))
    {
      EndDialog(hDlg, (wId == IDOK));
      return (TRUE);
    }
  }
  break;
  }
  return (FALSE);                           /* Didn't process a message    */
}


BOOL DoApplicationSettings(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,LRUMENU &lrum)
{
    SETTINGDLGPARAM sdp;
    BOOL fRet;
    BOOL fUserSelectCprParam;
    DWORD dwLangageValue=0;
    sdp.pCompressionParam = &guiItem.compressionParam;
    sdp.pfOverwriteExtracting = &guiItem.fOverwriteExtracting;
    sdp.lpszTempPath= guiItem.szTempPath;
    sdp.dwTempPathBufSize = sizeof(guiItem.szTempPath)/sizeof(TCHAR);
    sdp.pdwTempMemSize = &guiItem.dwTempMemSize;
    sdp.pfSelectTempPath = &guiItem.fSelectTempPath ;
    sdp.pfSelectTempMemSize = &guiItem.fSelectTempMemSize;
    sdp.pfStripIdentical = &guiItem.pfStripIdentical;
    sdp.pfMd5 = &guiItem.fMd5;
    sdp.pfSha1 = &guiItem.fSha1;
	sdp.pfSha256 = &guiItem.fSha256;

    fUserSelectCprParam = guiItem.compressionParam.dfBlockCalcSizeSearch != COMPRESSIONPARAM_AUTOVALUE;
    sdp.pfUserSelectCprParam = &fUserSelectCprParam;
    dwLangageValue = GetCurLang();
    sdp.pdwLangageValue = &dwLangageValue;

    fRet = (BOOL)DialogBoxParam(ghInstRes,
                           MAKEINTRESOURCE(IDD_COMPRESSION_PREF),
                           guiItem.GetHwndMain(),
                           (DLGPROC)SettingDlgProc,(LPARAM)(&sdp));

    if (fRet)
    {
        if (!fUserSelectCprParam)
            guiItem.compressionParam.dfBlockCalcSizeSearch = COMPRESSIONPARAM_AUTOVALUE;
        if (DfsFileAndInfo.DfsFile != NULL)
        {
            DFSFEATUREPARAM DfsFeatureParam;

            DfsFeatureParam.fComputeMd5 = guiItem.fMd5;
            DfsFeatureParam.fComputeSha1 = guiItem.fSha1;
			DfsFeatureParam.fComputeSha256 = guiItem.fSha256;
            DfsFeatureParam.fStripIdenticalBody = guiItem.pfStripIdentical;
			// guiItem.pfCompressLzma;
            SetDfsFeatureParam(DfsFileAndInfo.DfsFile,&DfsFeatureParam);
            AdaptDfsFileFeature(DfsFileAndInfo.DfsFile,DfsFileAndInfo.pDirInfo,DfsFileAndInfo.dfNbDir);
        }

        if (!guiItem.fSelectTempPath)
            SetTempDirectory(NULL);
        else
        {
            dfwchar szTempPathW[MAX_PATH+0x10];
            ConvertAnsiToUnicode(guiItem.szTempPath,szTempPathW,MAX_PATH);
            SetTempDirectory(szTempPathW);
        }

        IOConfig_SetVirtualFileNameMaximumMemory(guiItem.fSelectTempMemSize,(unsigned long)guiItem.dwTempMemSize);
        if (dwLangageValue != GetCurLang())
        {
            TryLoadLang(guiItem.hwndMain,(WORD)dwLangageValue,TRUE);
            lrum.PlaceMenuLRUItem(GetSubMenu(GetMenu(guiItem.hwndMain),0),IDM_EXIT);
            InvalidateRect(guiItem.hwndMain,NULL,TRUE);
            RefreshGrayingMenu(guiItem,DfsFileAndInfo);
            guiItem.RefreshNormalStatusBar(DfsFileAndInfo);
            guiItem.dwLangUISelect = dwLangageValue;
            guiItem.DisplayNewTitleBar();
            guiItem.FillListView(DfsFileAndInfo,TRUE);
        }
    }
    return fRet;
}

/*****************************************************************************/

class DLGPROCANDPARAM
{
public:
    DLGPROCANDPARAM() {};
    virtual ~DLGPROCANDPARAM() {} ;
    virtual BOOL DlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
               { return FALSE; } ;
} ;



LONG CALLBACK DlgProcAndParamDlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
    DLGPROCANDPARAM* pDlgProcAndParam;
    if (message==WM_INITDIALOG)
        MySetWindowLongPtr(hDlg,DWLP_USER,lParam);

    pDlgProcAndParam=(DLGPROCANDPARAM*)MyGetWindowLongPtr(hDlg,DWLP_USER);

    if (pDlgProcAndParam != NULL)
      return pDlgProcAndParam->DlgProc(hDlg,message,wParam,lParam);
    else
      return 0;
}

/*****************************************************************************/

typedef struct
{
    TCHAR szName[MAX_PATH];
    TCHAR szRegCode[MAX_PATH];
} DLGREGISTERPARAM;

class DLGREGISTERPROCANDPARAM:public DLGPROCANDPARAM
{
public:
    DLGREGISTERPROCANDPARAM(DLGREGISTERPARAM* pDlgRegisterParamSet)
      { pDlgRegisterParam = pDlgRegisterParamSet; } ;

    ~DLGREGISTERPROCANDPARAM() { };
    BOOL DlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
private:
    DLGREGISTERPARAM* pDlgRegisterParam;
} ;

BOOL DLGREGISTERPROCANDPARAM::DlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
    switch (message)
    {
      case WM_INITDIALOG:
          SetDlgItemText(hDlg,IDC_EDITNAMEREG, pDlgRegisterParam->szName);
          SetDlgItemText(hDlg,IDC_EDITCRCREG, pDlgRegisterParam->szRegCode);
          return TRUE;

      case WM_COMMAND:
            {
                WORD wId = GET_WM_COMMAND_ID(wParam,lParam);

                if (wId == IDC_GETHELP)
                {
                    DoHelpContents(hDlg,//*pADDCONTENTDFSDLGParam->pGuiItem,
                                   "::/Ordering_SmartVersion.htm");
                    return TRUE;
                }

                if (wId == IDOK)
                {
                    GetDlgItemText(hDlg,IDC_EDITNAMEREG,pDlgRegisterParam->szName,
                            sizeof(pDlgRegisterParam->szName)/sizeof(TCHAR));
                    GetDlgItemText(hDlg,IDC_EDITCRCREG,pDlgRegisterParam->szRegCode,
                            sizeof(pDlgRegisterParam->szRegCode)/sizeof(TCHAR));
                }

                if ((wId == IDOK) || (wId == IDCANCEL))
                {
                    EndDialog(hDlg, (wId == IDOK));
                    return (TRUE);
                }
            }
            break;

    }
    return FALSE;
}

BOOL DoApplicationRegister(REGCODE &RegCode,GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo)
{
    BOOL fRet;
    DLGREGISTERPARAM DlgRegisterParam;
    DLGREGISTERPROCANDPARAM DlgRegisterProcAndParam(&DlgRegisterParam);
    lstrcpy(DlgRegisterParam.szName,RegCode.GetNameCode());
    lstrcpy(DlgRegisterParam.szRegCode,RegCode.GetCrcCode());
    fRet = (BOOL)DialogBoxParam(ghInstRes,
                           MAKEINTRESOURCE(IDD_REGISTER),
                           guiItem.GetHwndMain(),
                           (DLGPROC)DlgProcAndParamDlgProc,(LPARAM)(&DlgRegisterProcAndParam));
    if (fRet)
    {
        BOOL fRegister,fRegisterPro;
        TCHAR szMsg[MAX_PATH*2]="";
        TCHAR szTitle[MAX_PATH*2]="";

        //fRegister=RegCode.TestRegCode(DlgRegisterParam.szName,DlgRegisterParam.szRegCode,&fRegisterPro);
        fRegister=RegCode.SetRegCode(DlgRegisterParam.szName,DlgRegisterParam.szRegCode,&fRegisterPro);
        guiItem.SetRegisteredMode(RegCode.IsRegistered());
        guiItem.DisplayNewTitleBar();


        LoadString(ghInstRes,fRegister ? IDS_GOODREGISTER : IDS_BADREGISTER,szMsg,sizeof(szMsg)/sizeof(TCHAR));
        LoadString(ghInstRes,IDS_REGISTER,szTitle,sizeof(szTitle)/sizeof(TCHAR));
        MessageBox(guiItem.hwndMain,szMsg,szTitle,MB_OK|MB_TASKMODAL| ((fRegister ? MB_ICONINFORMATION : MB_ICONERROR)));
    }
    return fRet;
}

BOOL DoOpeningDfs(const GUIITEM &guiItem,TCHAR* pszFn,UINT uiSize)
{
TCHAR szFilter[MAX_PATH*2]="";
MYOPENFILENAME ofn;



    InitOpenFileName((MYOPENFILENAME*)&ofn,guiItem.GetHwndMain(),IDS_TYPEOPENDFS,szFilter,sizeof(szFilter)-1,
                          pszFn,uiSize,NULL,0,0);
    if (!GetOpenFileName((OPENFILENAME*)&ofn))
        return FALSE;

    return TRUE;
}

BOOL FreeDfsFileAndInfo(DFSFILEANDINFO& DfsFileAndInfo,BOOL fCloseFile, H_ERROR_INFO * pei)
{
    if (DfsFileAndInfo.pDirInfo != NULL)
    {
        dfuLong32 i;
        for (i=0;i<DfsFileAndInfo.dfNbDir;i++)
            FreeDirectoryInfo((DfsFileAndInfo.pDirInfo)+i,NULL);
        DfsFree(DfsFileAndInfo.pDirInfo);
        DfsFileAndInfo.pDirInfo = NULL;
    }

    if ((DfsFileAndInfo.DfsFile != NULL) && fCloseFile)
    {
        DfsClose(DfsFileAndInfo.DfsFile,pei);
        DfsFileAndInfo.DfsFile = NULL;
        if (DfsFileAndInfo.lpszDfsFileName != NULL)
            DfsFree((void*)DfsFileAndInfo.lpszDfsFileName);
        DfsFileAndInfo.lpszDfsFileName = NULL;
    }

    DfsFileAndInfo.fBaseDirectorySelected = FALSE;
    DfsFileAndInfo.fBaseDirectoryNeeded = FALSE;
    if (DfsFileAndInfo.lpBaseDirectory != NULL)
      DfsFree((void*)DfsFileAndInfo.lpBaseDirectory);
    DfsFileAndInfo.lpBaseDirectory = NULL;
    DfsFileAndInfo.dfBaseDirNum = 0;

    DfsFileAndInfo.dfNbDir = 0;
    DfsFileAndInfo.dfCurDir = 0;
    DfsFileAndInfo.dfNbFileSelectedCurDir = 0;
    DfsFileAndInfo.dfSizeSelectedCurDir = 0;
    DfsFileAndInfo.dfNbFileTotalCurDir = 0;
    DfsFileAndInfo.dfSizeTotalCurDir = 0;
    return TRUE;
}


BOOL DoCloseDfs(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo)
{
    DfsFileAndInfo.fNotifyAccepted = FALSE;
    guiItem.ExtInfoCache.ClearExtInfo();

    TreeView_DeleteAllItems(guiItem.hwndTreeView);
    guiItem.EraseListView();
    DfsFileAndInfo.fNotifyAccepted = TRUE;

    FreeDfsFileAndInfo(DfsFileAndInfo,TRUE,NULL);
    guiItem.SetszFileName(NULL,TRUE,FALSE);
    RefreshGrayingMenu(guiItem,DfsFileAndInfo);
    return TRUE;
}

#ifndef BIF_USENEWUI
#define  BIF_USENEWUI   0x50      // new flag value for IE5 and higher
#endif

static int CALLBACK GDE_BrowseCallbackProc(
    HWND hwnd,
    UINT uMsg,
    LPARAM lParam,
    LPARAM lpData
   )
{
    if (uMsg == BFFM_INITIALIZED)
    {
        SendMessage(hwnd,BFFM_SETSELECTION,TRUE,lpData);
    }
    return 0;
}


typedef enum
{
    COMPARE_GOOD,
    COMPARE_NOTFOUND,
    COMPARE_BADSIZE,
    COMPARE_PROBLEMREAD,
    COMPARE_BADCRC,
    COMPARE_UNDEFINED
} COMPARERES;

/*
BOOL TryCompareBaseFile(GUIITEM &guiItem,LPCSTR lpszDirectory,const FILEINDIRINFO* pCurFileInDirInfo,BOOL fVerboseAsk,BOOL &fCancel)
{
    TCHAR szFileNameToCheckPrefix[MAX_PATH*2];
    int iln;
    BOOL fFound=FALSE;
    BOOL fGoodSize=FALSE;
    BOOL fGoodCrc=FALSE;
    HANDLE hFile=NULL;
    COMPARERES CompareRes=COMPARE_UNDEFINED;
    dfwcharp dfwFileNameToOpen = NULL;
    dfwchar dfwCharPrefix[MAX_PATH*2];


    lstrcpy(szFileNameToCheckPrefix,lpszDirectory);
    iln = lstrlen(szFileNameToCheckPrefix);
    if (iln>0)
        if (szFileNameToCheckPrefix[iln-1] != '\\')
            lstrcat(szFileNameToCheckPrefix,"\\");

    ConvertTCharToUnicode(szFileNameToCheckPrefix,dfwCharPrefix,MAX_PATH*2);

    dfwFileNameToOpen = dfUnicodeCopyConcatAlloc(dfwCharPrefix,pCurFileInDirInfo -> FileName);
    hFile = MyCreateFileW(dfwFileNameToOpen,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);

    if (hFile!=NULL)
    {
        DWORD dwFileFoundSize;
        fFound = TRUE;
        dwFileFoundSize = GetFileSize(hFile,NULL);
        fGoodSize = dwFileFoundSize == pCurFileInDirInfo -> dfSize;
    }
    else CompareRes = COMPARE_NOTFOUND ;

    if (fGoodSize)
    {
        if (pCurFileInDirInfo->fCrc32Filled)
        {
            DWORD dwCrc32=0;
            DWORD dwSizeToDo = pCurFileInDirInfo -> dfSize;
            DWORD dwSizeBuffer = min(0x10000,dwSizeToDo+1);
            BOOL fProblemReading = FALSE;

            void* ptr = DfsMalloc(dwSizeBuffer);

            if ((hFile!=NULL) && (ptr != NULL))
            {
                while (dwSizeToDo>0)
                {
                    DWORD dwDone=0;
                    if (!ReadFile(hFile,ptr,min(dwSizeToDo,dwSizeBuffer),&dwDone,NULL))
                    {
                        fProblemReading = TRUE;
                        CompareRes = COMPARE_PROBLEMREAD;
                        break;
                    }
                    dwSizeToDo -= dwDone;
                    dwCrc32 = crc32(dwCrc32, (const unsigned char*)ptr, dwDone);
                }

            }
            else
            {
                CompareRes = COMPARE_PROBLEMREAD;
                fProblemReading = TRUE;
            }

            if ((dwCrc32 == pCurFileInDirInfo->dfCrc32) && (!fProblemReading))
            {
                CompareRes = COMPARE_GOOD;
            }
            else
                CompareRes = COMPARE_BADCRC;

            if (ptr != NULL)
                DfsFree(ptr);
        }
        else
        {
            fGoodCrc = TRUE;
            CompareRes = COMPARE_GOOD;
        }
    }
    else
        if (CompareRes == COMPARE_UNDEFINED)
            CompareRes = COMPARE_BADSIZE;

    if ((CompareRes != COMPARE_GOOD) && fVerboseAsk)
    {
        UINT uiId = 0;
        TCHAR szErrDesc[MAX_PATH]="";
        TCHAR szErrFmt[MAX_PATH]="";
        TCHAR szMsg[MAX_PATH*4]="";
        if (CompareRes == COMPARE_NOTFOUND)
            uiId = IDS_ERROR_COMPARE_NOTFOUD;

        if (CompareRes == COMPARE_BADSIZE)
            uiId = IDS_ERROR_COMPARE_BADSIZE;

        if (CompareRes == COMPARE_PROBLEMREAD)
            uiId = IDS_ERROR_COMPARE_PROBLEMREAD;

        if (CompareRes == COMPARE_BADCRC)
            uiId = IDS_ERROR_COMPARE_BADCRC;
        if (uiId != 0)
            LoadString(ghInstRes,uiId,szErrDesc,sizeof(szErrDesc)/sizeof(TCHAR));
        LoadString(ghInstRes,IDS_TXTERRORCOMPARE,szErrFmt,sizeof(szErrFmt)/sizeof(TCHAR));
        wsprintf(szMsg,szErrFmt,dfwFileNameToOpen,pCurFileInDirInfo -> FileName,szErrDesc);

        if (MessageBox(guiItem.hwndMain,szMsg,NULL,MB_ICONERROR|MB_OKCANCEL) == IDCANCEL)
            fCancel = TRUE;
    }

    if (hFile != NULL)
        CloseHandle(hFile);
    if (dfwFileNameToOpen != NULL)
      DfsFree(dfwFileNameToOpen);
    return (CompareRes == COMPARE_GOOD);
}
*/


typedef struct
{
dfwcharpc dfwFileName;
dfuLong32 dfCrc32;
dfuLong32 dwFileFoundSize;
BOOL    fReadOk;
BOOL    fFound;
BOOL    fMD5Computed;
BOOL    fSHA1Computed;
BOOL    fSHA256Computed;
BYTE    bMD5[16];
BYTE    bSHA1[20];
BYTE    bSHA256[32];
}
FILECRC32;

long DFSCALLBACK fncCompareFileItemCrc32(const void *lpElem1, const void *lpElem2)
{
  const FILECRC32 *pfcrc1 = (const FILECRC32 *) lpElem1;
  const FILECRC32 *pfcrc2 = (const FILECRC32 *) lpElem2;
  return dfUnicodeStrcmpi(pfcrc1->dfwFileName, pfcrc2->dfwFileName);
}

BOOL DFSCALLBACK fncDestructorFileItemCrc32(const void *lpElem)
{
  const FILECRC32 *pfcrc = (const FILECRC32 *) lpElem;
  if (pfcrc->dfwFileName != NULL)
    DfsFree((dfwcharp) pfcrc->dfwFileName);

  return TRUE;
}


BOOL ReportCompareError(GUIITEM &guiItem,COMPARERES CompareRes,dfwcharpc dfwDfsName,dfwcharpc dfwFileCmp)
{
    UINT uiId = 0;
    TCHAR szErrDesc[MAX_PATH]="";
    TCHAR szErrFmt[MAX_PATH]="";
    TCHAR szMsg[MAX_PATH*4]="";
    BOOL fCancel=FALSE;
    if (CompareRes == COMPARE_NOTFOUND)
        uiId = IDS_ERROR_COMPARE_NOTFOUD;

    if (CompareRes == COMPARE_BADSIZE)
        uiId = IDS_ERROR_COMPARE_BADSIZE;

    if (CompareRes == COMPARE_PROBLEMREAD)
        uiId = IDS_ERROR_COMPARE_PROBLEMREAD;

    if (CompareRes == COMPARE_BADCRC)
        uiId = IDS_ERROR_COMPARE_BADCRC;
    if (uiId != 0)
        LoadString(ghInstRes,uiId,szErrDesc,sizeof(szErrDesc)/sizeof(TCHAR));
    LoadString(ghInstRes,IDS_TXTERRORCOMPARE,szErrFmt,sizeof(szErrFmt)/sizeof(TCHAR));
    wsprintf(szMsg,szErrFmt,dfwDfsName,dfwFileCmp,szErrDesc);

    if (MessageBox(guiItem.hwndMain,szMsg,NULL,MB_ICONERROR|MB_OKCANCEL) == IDCANCEL)
        fCancel = TRUE;
    return fCancel;
}


COMPARERES ComputeHashFile(dfwcharpc dfwFileNameToOpen,dfuLong64 dfExpectedSize,
                           BOOL fCompareCrc32,dfuLong32& dwCrc32,
                           BOOL fCompareMD5,LPBYTE bMD5,BOOL fCompareSHA1,LPBYTE bSHA1, BOOL fCompareSHA256, LPBYTE bSHA256,
                           H_ERROR_INFO* pei)
{
    LOWLEVELFILE llFile;
    SVF_MD5_INTERNAL md5Ctx;
    SVF_SHA1_INTERNAL sha1Ctx;
    SVF_SHA256_INTERNAL sha256Ctx;
    COMPARERES CompareRes;
    BOOL fGoodSize=FALSE;

    if (fCompareMD5)
        svf_md5_init(&md5Ctx) ;
    if (fCompareSHA1)
        svf_sha1_init(&sha1Ctx) ;
	if (fCompareSHA256)
		svf_sha256_init(&sha256Ctx);
    dwCrc32=0;

    llFile = OpenLowLevel(dfwFileNameToOpen,OPEN_READ,FALSE,FALSE,0,pei);

    if (llFile!=NULL)
    {
        dfuLong64 dfFileFoundSize ;
        LowLevelGetSize64(llFile,&dfFileFoundSize);

        fGoodSize = (dfFileFoundSize == dfExpectedSize);
        if (!fGoodSize)
            CompareRes = COMPARE_BADSIZE;
    }
    else CompareRes = COMPARE_NOTFOUND ;

    if (fGoodSize)
    {
        if (fCompareCrc32 || fCompareMD5 || fCompareSHA1)
        {
            dfuLong64 dwSizeToDo = dfExpectedSize;
            DWORD dwSizeBuffer = (DWORD)(min(0x10000,dwSizeToDo+1));
            BOOL fProblemReading = FALSE;

            void* ptr = DfsMalloc(dwSizeBuffer);

            if ((llFile!=NULL) && (ptr != NULL))
            {
                while (dwSizeToDo>0)
                {
                    DWORD dwDone=0;
                    DWORD dwToDoThis = (DWORD)(min(dwSizeToDo,dwSizeBuffer));
                    //if (!ReadFile(hFile,ptr,min(dwSizeToDo,dwSizeBuffer),&dwDone,NULL))
                    dwDone = LowLevelRead(llFile,ptr,dwToDoThis,pei);
                    if (dwDone!=dwToDoThis)
                    {
                        fProblemReading = TRUE;
                        CompareRes = COMPARE_PROBLEMREAD;
                        break;
                    }
                    dwSizeToDo -= dwToDoThis;
                    if (fCompareCrc32)
                        dwCrc32 = crc32(dwCrc32, (const unsigned char*)ptr, dwDone);
                    if (fCompareMD5)
                        svf_md5_append(&md5Ctx, (const unsigned char*)ptr, dwDone);
                    if (fCompareSHA1)
                        svf_sha1_append(&sha1Ctx, (const unsigned char*)ptr, dwDone);
					if (fCompareSHA256)
						svf_sha256_append(&sha256Ctx, (const unsigned char*)ptr, dwDone);
                }
            }
            else
            {
                CompareRes = COMPARE_PROBLEMREAD;
                fProblemReading = TRUE;
            }

            if (!fProblemReading)
                CompareRes = COMPARE_GOOD;

            if (fCompareMD5)
                svf_md5_finish(&md5Ctx,bMD5);

            if (fCompareSHA1)
                svf_sha1_finish(&sha1Ctx,bSHA1);
			if (fCompareSHA256)
				svf_sha256_finish(&sha256Ctx, bSHA256);

            if (ptr != NULL)
                DfsFree(ptr);
        }
        else
        {
            CompareRes = COMPARE_GOOD;
        }
    }

    if (llFile != NULL)
        LowLevelClose(llFile,pei);
    return CompareRes;
}


#define UIMAroundUpper(dwValue,dwModulo) \
    (((((dwValue)) + ((dwModulo)) -1) / ((dwModulo))) * (dwModulo))


COMPARERES ComputeHashFileOverlapIO(dfwcharpc dfwFileNameToOpen,
                                    dfuLong64 dfExpectedSize,
                                    BOOL fCompareCrc32, dfuLong32 & dwCrc32,
                                    BOOL fCompareMD5, LPBYTE bMD5,
                                    BOOL fCompareSHA1, LPBYTE bSHA1,
                                    BOOL fCompareSHA256, LPBYTE bSHA256,
                                    H_ERROR_INFO * pei)
{
  HANDLE hf;
  BOOL fNoBuffering ;
  BOOL fAskSequential = TRUE;
  COMPARERES CompareRes = COMPARE_GOOD;
  BOOL fGoodSize = FALSE;
  DWORD dwSizeBlock ;

  fNoBuffering = FALSE;
  dwSizeBlock = 0x10000;

  hf = MyCreateFileW(dfwFileNameToOpen, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL |
                     (fNoBuffering ? FILE_FLAG_NO_BUFFERING : 0) |
                     FILE_FLAG_OVERLAPPED |
                     (fAskSequential ? FILE_FLAG_SEQUENTIAL_SCAN : 0), NULL);

  if (hf == INVALID_HANDLE_VALUE)
    hf = NULL;

  if (hf != NULL)
  {
    dfuLong64 dfFileFoundSize;
    DWORD dwSizeLow, dwSizeHigh;
    dwSizeHigh = 0;
    dwSizeLow = GetFileSize(hf, &dwSizeHigh);
    dfFileFoundSize = dwSizeLow | (((dfuLong64) dwSizeHigh) << 32);

    fGoodSize = (dfFileFoundSize == dfExpectedSize);
    if (!fGoodSize)
      CompareRes = COMPARE_BADSIZE;
  }
  else
    CompareRes = COMPARE_NOTFOUND;

  if ((fGoodSize) && (fCompareCrc32 || fCompareMD5 || fCompareSHA1))
    {
      OVERLAPPED ovr;
      unsigned char *ptra;
      unsigned char *ptrb;
      unsigned char *ptrn;
      unsigned char *ptr;
      unsigned char *ptrBlock;
      dfuLong64 dwTot = 0;
      dfuLong64 dfComputed = 0;
      dfuLong64 dfReadAskToDo = dfExpectedSize;
      DWORD dwLast = 0;
      BOOL fParity = FALSE;
      SVF_MD5_INTERNAL md5Ctx;
      SVF_SHA1_INTERNAL sha1Ctx;
      SVF_SHA256_INTERNAL sha256Ctx;
      BOOL fReadPending = FALSE;

      dwCrc32 = 0;
      if (fCompareMD5)
          svf_md5_init(&md5Ctx);
      if (fCompareSHA1)
          svf_sha1_init(&sha1Ctx);
	  if (fCompareSHA256)
		  svf_sha256_init(&sha256Ctx);

      if (dwSizeBlock > dfExpectedSize)
        dwSizeBlock = ((((DWORD) dfExpectedSize) / 0x4000) + 1) * 0x4000;

      memset(&ovr, 0, sizeof(ovr));
      ptrBlock = (unsigned char *) DfsMalloc(dwSizeBlock * 2);
      ptra = ptrBlock;
      ptrb = ptrBlock + dwSizeBlock;
      ptr = ptrBlock;

      do
      {
        DWORD dwrf;
        DWORD dwDummy;
        DWORD dwReadAskThisRealData;
        ptrn = fParity ? ptrb : ptra;

        ovr.Offset = (DWORD)dwTot;
        ovr.OffsetHigh = (DWORD)((dwTot)>>32);
        if (dfReadAskToDo>=dwSizeBlock)
            dwReadAskThisRealData = dwSizeBlock;
        else
            dwReadAskThisRealData = (DWORD)dfReadAskToDo;

        fReadPending = (dwReadAskThisRealData>0);
        if (fReadPending)
        {
            BOOL frf;
            frf = ReadFile(hf, ptrn, dwSizeBlock, &dwDummy, &ovr);
            dwrf = GetLastError();

            dfReadAskToDo-=dwReadAskThisRealData;

            if ((!frf) && (dwrf != ERROR_IO_PENDING))
            {
                CompareRes = COMPARE_PROBLEMREAD;
                FillWinErrorFileName(dfwFileNameToOpen,dwrf,pei);
                break;
            }
        }

        //DoTheCpuWork(ptr,dwLast);
        if (dwLast>0)
        {
            if (fCompareCrc32)
                dwCrc32 = crc32(dwCrc32, (const unsigned char *) ptr, dwLast);

            if (fCompareMD5)
                svf_md5_append(&md5Ctx, (const unsigned char *) ptr, dwLast);

            if (fCompareSHA1)
                svf_sha1_append(&sha1Ctx, (const unsigned char *) ptr, dwLast);

			if (fCompareSHA256)
				svf_sha256_append(&sha256Ctx, (const unsigned char *)ptr, dwLast);

            dfComputed += dwLast;
        }

        if (fReadPending)
        {
            BOOL fOverLap;
            fOverLap = GetOverlappedResult(hf, &ovr, &dwLast, TRUE);
            if (!fOverLap)
            {
                DWORD dwErr ;
                CompareRes = COMPARE_PROBLEMREAD;
                dwErr = GetLastError();
                FillWinErrorFileName(dfwFileNameToOpen,dwErr,pei);
                break;
            }
        }
        else
            dwLast = 0;

        ptr = ptrn;
        fParity = !fParity;

        dwTot += dwLast;
      } while (dwLast > 0);

      DfsFree(ptrBlock);
      if (CompareRes == COMPARE_GOOD)
          if (dfComputed != dfExpectedSize)
              CompareRes = COMPARE_PROBLEMREAD;

      if ((CompareRes == COMPARE_GOOD) && (fCompareMD5))
          svf_md5_finish(&md5Ctx,bMD5);

      if ((CompareRes == COMPARE_GOOD) && (fCompareSHA1))
          svf_sha1_finish(&sha1Ctx,bSHA1);

	  if ((CompareRes == COMPARE_GOOD) && (fCompareSHA256))
		  svf_sha256_finish(&sha256Ctx, bSHA256);
    }

    if (hf != NULL)
      CloseHandle(hf);

  return CompareRes;
}






BOOL TryCompareBaseFile(GUIITEM &guiItem,LPCSTR lpszDirectory,
                        STATIC_ARRAY &saCrcFile,
                        const FILEINDIRINFO* pCurFileInDirInfo,BOOL fVerboseAsk,
                        BOOL fCompareMD5,BOOL fCompareSHA1,
                        BOOL &fCancel, H_ERROR_INFO* pei)
{
    TCHAR szFileNameToCheckPrefix[MAX_PATH*2];
    int iln;
    BOOL fFound=FALSE;
    LOWLEVELFILE llFile=NULL;
    COMPARERES CompareRes=COMPARE_UNDEFINED;
    dfwcharp dfwFileNameToOpen = NULL;
    dfwchar dfwCharPrefix[MAX_PATH*2];
    //DWORD dwCrc32=0;
    BOOL fFoundInArray=FALSE;

    lstrcpy(szFileNameToCheckPrefix,lpszDirectory);
    iln = lstrlen(szFileNameToCheckPrefix);
    if (iln>0)
        if (szFileNameToCheckPrefix[iln-1] != '\\')
            lstrcat(szFileNameToCheckPrefix,"\\");

    ConvertTCharToUnicode(szFileNameToCheckPrefix,dfwCharPrefix,MAX_PATH*2);

    dfwFileNameToOpen = dfUnicodeCopyConcatAlloc(dfwCharPrefix,pCurFileInDirInfo -> FileName);
    //hFile = MyCreateFileW(dfwFileNameToOpen,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);

    {
        FILECRC32 fcrc32;
        dfuLong32 dwPos=0;
        fcrc32.dfwFileName = dfwFileNameToOpen;
        fcrc32.dfCrc32 = 0;
        fcrc32.fMD5Computed = FALSE;
        fcrc32.fSHA1Computed = FALSE;
        if (saCrcFile.FindSameElemPosSA(&fcrc32,&dwPos))
        {
            const FILECRC32* lpAlready=(const FILECRC32*)saCrcFile.GetElemPtrSA(dwPos);
            fFoundInArray = TRUE;
            //dwCrc32=lpAlready->dfCrc32_;
            CompareRes = COMPARE_GOOD;
            if (!lpAlready->fFound)
                CompareRes = COMPARE_NOTFOUND;
            else
            if (!lpAlready->fReadOk)
                CompareRes = COMPARE_PROBLEMREAD;
            else
            if (lpAlready->dwFileFoundSize != pCurFileInDirInfo -> dfSize)
                CompareRes = COMPARE_BADCRC;
            else
            if ((lpAlready->dfCrc32) == (pCurFileInDirInfo->dfCrc32))
            {
                CompareRes = COMPARE_GOOD;
            }
            else
                CompareRes = COMPARE_BADCRC;

            if (CompareRes == COMPARE_GOOD)
            {
                if ((lpAlready->fMD5Computed) && (pCurFileInDirInfo->fMd5Filled))
                    if (DfsMemcmp(lpAlready->bMD5,pCurFileInDirInfo->bMd5,16)!=0)
                        CompareRes = COMPARE_BADCRC;
            }

            if (CompareRes == COMPARE_GOOD)
            {
                if ((lpAlready->fSHA1Computed) && (pCurFileInDirInfo->fSha1Filled))
                    if (DfsMemcmp(lpAlready->bSHA1,pCurFileInDirInfo->bSha1,20)!=0)
                        CompareRes = COMPARE_BADCRC;
            }
        }
    }
    if (!fFoundInArray)
    {
        BOOL fGoodSize=FALSE;
        BOOL fGoodCrc=FALSE;
        DWORD dwFileFoundSizeLow=0;
        DWORD dwFileFoundSizeHigh=0;
        dfuLong32 dwCrc32=0;
        BYTE bMD5[16];
        BYTE bSHA1[20];
		BYTE bSHA256[32];
        BOOL fComputeCRC,fComputeMD5,fComputeSHA1, fComputeSHA256;

        fComputeMD5 = fCompareMD5 || (pCurFileInDirInfo->fMd5Filled);
        fComputeSHA1 = fCompareSHA1 || (pCurFileInDirInfo->fSha1Filled);
		fComputeSHA256 = fCompareSHA1 || (pCurFileInDirInfo->fSha256Filled);
        fComputeCRC = (pCurFileInDirInfo->fCrc32Filled) || fComputeMD5 || fComputeSHA1;



        if (GetWin32Kind()==WINNT)
            CompareRes = ComputeHashFileOverlapIO(dfwFileNameToOpen,pCurFileInDirInfo->dfSize,
                                                fComputeCRC,dwCrc32,
                                                fComputeMD5,bMD5,
                                                fComputeSHA1,bSHA1, fComputeSHA256, bSHA256, pei);
        else
            CompareRes = ComputeHashFile(dfwFileNameToOpen,pCurFileInDirInfo->dfSize,
                                        fComputeCRC,dwCrc32,
                                        fComputeMD5,bMD5,
                                        fComputeSHA1,bSHA1, fComputeSHA256, bSHA256, pei);


        if ((CompareRes == COMPARE_GOOD) && (pCurFileInDirInfo->fCrc32Filled))
            if ((dwCrc32 != pCurFileInDirInfo->dfCrc32))
                CompareRes = COMPARE_BADCRC;

        if ((CompareRes == COMPARE_GOOD) && (pCurFileInDirInfo->fMd5Filled) && fComputeMD5)
            if (DfsMemcmp(bMD5,pCurFileInDirInfo->bMd5,16)!=0)
                CompareRes = COMPARE_BADCRC;

        if ((CompareRes == COMPARE_GOOD) && (pCurFileInDirInfo->fSha1Filled) && fComputeSHA1)
            if (DfsMemcmp(bSHA1,pCurFileInDirInfo->bSha1,20)!=0)
                CompareRes = COMPARE_BADCRC;

        if ((CompareRes == COMPARE_GOOD) && (pCurFileInDirInfo->fSha256Filled) && fComputeSHA256)
            if (DfsMemcmp(bSHA256,pCurFileInDirInfo->bSha256,32)!=0)
                CompareRes = COMPARE_BADCRC;

        if (((CompareRes == COMPARE_BADCRC) || (CompareRes == COMPARE_GOOD) ||
             (CompareRes == COMPARE_NOTFOUND) || (CompareRes == COMPARE_PROBLEMREAD)) &&
             (fComputeCRC))
        {
            FILECRC32 fCrc32;
            fCrc32.dfwFileName = dfUnicodeCopyConcatAlloc(dfwFileNameToOpen,NULL);
            fCrc32.dfCrc32=dwCrc32;

            fCrc32.fMD5Computed = fComputeMD5;
            fCrc32.fSHA1Computed = fComputeSHA1;
			fCrc32.fSHA256Computed = fComputeSHA256;

            if (fCrc32.fMD5Computed)
                DfsMemcpy(fCrc32.bMD5,bMD5,16);

            if (fCrc32.fSHA1Computed)
                DfsMemcpy(fCrc32.bSHA1,bSHA1,20);

            if (fCrc32.fSHA256Computed)
                DfsMemcpy(fCrc32.bSHA256,bSHA256,32);

            fCrc32.dwFileFoundSize = dwFileFoundSizeLow;
            fCrc32.fReadOk=CompareRes != COMPARE_PROBLEMREAD;
            fCrc32.fFound=CompareRes != COMPARE_NOTFOUND;
            saCrcFile.InsertSortedSA(&fCrc32);
        }
    }


    if ((CompareRes != COMPARE_GOOD) && fVerboseAsk)
        if (ReportCompareError(guiItem,CompareRes,dfwFileNameToOpen,pCurFileInDirInfo -> FileName))
            fCancel=TRUE;


    if (dfwFileNameToOpen != NULL)
      DfsFree(dfwFileNameToOpen);
    return (CompareRes == COMPARE_GOOD);
}

BOOL TrySetDfsOneBaseDirectory(LPCSTR lpszDirectory,GUIITEM &guiItem,
                               DFSFILEANDINFO &DfsFileAndInfo,dfuLong32 dfNumDir,
                               BOOL fVerboseAsk,BOOL &fCancel,STATIC_ARRAY &saCrcFile,
                               BOOL fCompareMD5,BOOL fCompareSHA1)
{
    PDIRINFO pCurDirInfo;
    dfuLong32 i;
    pCurDirInfo = (((*(DfsFileAndInfo.pDirInfo+dfNumDir))));
    BOOL fRet=TRUE;

    SetCursor(LoadCursor(NULL,IDC_WAIT));

    for (i=0;i<pCurDirInfo->dfNbFile && (!fCancel) && fRet;i++)
    {
        FILEINDIRINFO* pCurFileInDirInfo;

        BOOL fTry;

        pCurFileInDirInfo = (pCurDirInfo->pFileInDirInfo+i);

        fTry = TryCompareBaseFile(guiItem,lpszDirectory,saCrcFile,
                                  pCurFileInDirInfo,fVerboseAsk,
                                  fCompareMD5,fCompareSHA1,
                                  fCancel,NULL);
        /*
        fTry = TryCompareBaseFile(guiItem,lpszDirectory,saCrcFile,
                                  pCurFileInDirInfo,
                                  fCompareMD5,fCompareSHA1,
                                  fVerboseAsk,fCancel);
                                  */
        fRet = fRet && fTry;
    }
    if (fCancel)
        fRet = FALSE;

    if (fRet)
    {
        DfsFileAndInfo.fBaseDirectorySelected = TRUE;
        DfsFileAndInfo.dfBaseDirNum = dfNumDir;

        if (DfsFileAndInfo.lpBaseDirectory != NULL)
          DfsFree((void*)DfsFileAndInfo.lpBaseDirectory);


        DfsFileAndInfo.lpBaseDirectory = (LPTSTR)DfsMalloc((strlen(lpszDirectory)+8)*2);
        lstrcpy((LPTSTR)DfsFileAndInfo.lpBaseDirectory,lpszDirectory);
        lstrcpy(guiItem.szDefaultDirPreviousVersion,lpszDirectory);
    }
    SetCursor(LoadCursor(NULL,IDC_ARROW));
    return fRet;
}

/* can be replaced by CheckDirectoryCrcWithRealFileSet pehaps */
BOOL TrySetDfsBaseDirectory(LPCSTR lpszDirectory,GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,
                            dfuLong32 dfNumDir,BOOL fTryOtherDir,BOOL fVerboseAsk,BOOL &fCancel)
{
    BOOL fRet = TRUE;
    BOOL fCompareMD5,fCompareSHA1;
    STATIC_ARRAY saCrcFile;
    dfuLong32 i;
    ANALYSE_DFS_FEATURE_USED AnalyseDfsFeatureUsed;

    AnalyseDfsFeature(DfsFileAndInfo.pDirInfo,DfsFileAndInfo.dfNbDir,&AnalyseDfsFeatureUsed);
    fCompareMD5=AnalyseDfsFeatureUsed.fMd5PresentFound;
    fCompareSHA1=AnalyseDfsFeatureUsed.fSha1PresentFound;

    saCrcFile.InitStaticArray(sizeof(FILECRC32), 0x80);
    saCrcFile.SetFuncCompareDataSA(fncCompareFileItemCrc32);
    saCrcFile.SetFuncDestructorSA(fncDestructorFileItemCrc32);
    if (!fTryOtherDir)
        return TrySetDfsOneBaseDirectory(lpszDirectory,guiItem,DfsFileAndInfo,dfNumDir,
                                         fVerboseAsk,fCancel,saCrcFile,
                                         fCompareMD5,fCompareSHA1);

    fRet = TrySetDfsOneBaseDirectory(lpszDirectory,guiItem,DfsFileAndInfo,dfNumDir,FALSE,fCancel,saCrcFile,fCompareMD5,fCompareSHA1);
    if (!fRet)
        for (i=0;i<DfsFileAndInfo.dfNbDir;i++)
        {
            if (i!=dfNumDir)
                fRet = TrySetDfsOneBaseDirectory(lpszDirectory,guiItem,DfsFileAndInfo,i,FALSE,fCancel,saCrcFile,fCompareMD5,fCompareSHA1);
            if (fRet)
                return TRUE;
        }

    if (fVerboseAsk && (!fRet))
      fRet = TrySetDfsOneBaseDirectory(lpszDirectory,guiItem,DfsFileAndInfo,dfNumDir,fVerboseAsk,fCancel,saCrcFile,fCompareMD5,fCompareSHA1);

    return fRet ;
}

BOOL DoAskAndSetDfsBaseDirectory(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,LPCTSTR lpActualCurDir,BOOL &fCancel)
{
    BOOL fRet=FALSE;
    LPMALLOC pMalloc;
    BOOL     fReturn = FALSE;

    if(SUCCEEDED(::SHGetMalloc(&pMalloc)))
    {
        LPITEMIDLIST pidlBrowse;
        BROWSEINFO   bi;

        TCHAR szPath[MAX_PATH+1];


        ::ZeroMemory(szPath, sizeof(szPath));
        ::ZeroMemory(&bi, sizeof(BROWSEINFO));


        wsprintf(szPath,lpActualCurDir);

        bi.hwndOwner      = guiItem.hwndMain;
        bi.pidlRoot       = NULL;
        bi.pszDisplayName = szPath;
        bi.lpszTitle      = NULL;//szText;
        bi.ulFlags        = BIF_RETURNONLYFSDIRS | BIF_RETURNFSANCESTORS | BIF_USENEWUI | BIF_VALIDATE;
        bi.lpfn           = GDE_BrowseCallbackProc;//NULL;
        bi.lParam         = (LPARAM)szPath;
        bi.iImage         = 0;

        pidlBrowse = ::SHBrowseForFolder(&bi);
        if(pidlBrowse)
        {
          if(::SHGetPathFromIDList(pidlBrowse, szPath))
          {
              if (!fRet)
                fRet = TrySetDfsBaseDirectory(szPath,guiItem,DfsFileAndInfo,0,TRUE,TRUE,fCancel);
          }
        }

        // clean-up
        pMalloc->Free(pidlBrowse);
        pMalloc->Release();
    }

    return fRet;
}






BOOL RefreshDfs(DFSFILE DfsFile,GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,LPCTSTR lpszFileNameDfs)
{
  BOOL fRet = TRUE;
  BOOL fLoadNewDfs;

  dfuLong32 dfNumDir ;
  BOOL fPreviousBaseDirSelected=FALSE;
  LPCTSTR lpOldBaseDirectory = NULL;
  dfuLong32 dfPreviousBaseDirNum = 0;
  dfuLong32 dfTypeFirstDir ;

  fLoadNewDfs = DfsFile != DfsFileAndInfo.DfsFile;

  if ((DfsFileAndInfo.fBaseDirectorySelected) && (!fLoadNewDfs))
  {
      fPreviousBaseDirSelected = TRUE;
      lpOldBaseDirectory = DfsFileAndInfo.lpBaseDirectory;
      DfsFileAndInfo.lpBaseDirectory = NULL;
      dfPreviousBaseDirNum=DfsFileAndInfo.dfBaseDirNum;
  }

  if (fLoadNewDfs)
      guiItem.ExtInfoCache.ClearExtInfo();
  FreeDfsFileAndInfo(DfsFileAndInfo,fLoadNewDfs,NULL);

  DfsFileAndInfo.DfsFile = DfsFile;
  if (DfsFileAndInfo.lpszDfsFileName != NULL)
      DfsFree((void*)DfsFileAndInfo.lpszDfsFileName);

  DfsFileAndInfo.lpszDfsFileName = NULL;
  if (lpszFileNameDfs!=NULL)
  {
      LPTSTR lpszDfsFileName;
      LPTSTR lpszDfsFileNameFilePart;
      dfuLong32 dfSizeName = GetFullPathName(lpszFileNameDfs,0,NULL,NULL) + 4;
      lpszDfsFileName = (LPTSTR)DfsMalloc(dfSizeName + 2);
      GetFullPathName(lpszFileNameDfs,dfSizeName,lpszDfsFileName,&lpszDfsFileNameFilePart);
      DfsFileAndInfo.lpszDfsFileName = lpszDfsFileName ;
  }

  DfsGetNbDir(DfsFile, &DfsFileAndInfo.dfNbDir,NULL);
  DfsFileAndInfo.dfCurDir = 0;
  DfsFileAndInfo.dfNbFileSelectedCurDir = 0;
  DfsFileAndInfo.dfSizeSelectedCurDir = 0;
  DfsFileAndInfo.pDirInfo = (PDIRINFO*)DfsMalloc(sizeof(PDIRINFO)*(DfsFileAndInfo.dfNbDir+1));

  for (dfNumDir = 0; dfNumDir < DfsFileAndInfo.dfNbDir; dfNumDir++)
  {
      dfuLong32 dfError = DFS_SUCCESS;
      dfError = ReadDirectoryInfo(DfsFile, dfNumDir, (DfsFileAndInfo.pDirInfo)+dfNumDir, NULL, NULL,NULL);
      if (dfError != DFS_SUCCESS)
      {
          fRet = FALSE;
          break;
      }
      if (dfNumDir > 0)
      {
          PDIRINFO pDirInfoPrev=*((DfsFileAndInfo.pDirInfo)+dfNumDir-1);
          PDIRINFO pDirInfoCur=*((DfsFileAndInfo.pDirInfo)+dfNumDir);

          FixIndenticalDifferentSizeInReadDirectoryInfo(pDirInfoPrev,pDirInfoCur);
      }
  }


  if (!fRet)
      FreeDfsFileAndInfo(DfsFileAndInfo,TRUE,NULL);
  if (fRet)
    AdaptDfsFileFeature(DfsFileAndInfo.DfsFile,DfsFileAndInfo.pDirInfo,DfsFileAndInfo.dfNbDir);

  if (DfsFileAndInfo.dfNbDir>0)
    dfTypeFirstDir = (((*(DfsFileAndInfo.pDirInfo+0))))->dfTypeDir;
  DfsFileAndInfo.fBaseDirectoryNeeded = dfTypeFirstDir == TYPEDIR_FILECRCONLY;


  if (fRet)
  {
      BOOL fCancel=FALSE;
      if (fPreviousBaseDirSelected)
        TrySetDfsBaseDirectory(lpOldBaseDirectory,guiItem,DfsFileAndInfo,dfPreviousBaseDirNum,FALSE,TRUE,fCancel);
      /*guiItem.FillTreeView(DfsFileAndInfo);*/
      guiItem.FillListView(DfsFileAndInfo);
      guiItem.DoSortColumn(DfsFileAndInfo,guiItem.GetColumnSort(),guiItem.GetColumnSortInvert());
  }


  if (lpOldBaseDirectory!=NULL)
    DfsFree((void*)lpOldBaseDirectory);

  RefreshGrayingMenu(guiItem,DfsFileAndInfo);
  return fRet;
}


DWORD GetListViewSelectionStatus(HWND hWndLV,dfuLong32* pdwPosFirstSelected)
{
    DWORD i;
    DWORD dwNbItem = ListView_GetItemCount(hWndLV);
    DWORD dwNbSelected=0;
    DWORD dwPosFirstSelected;
    dwPosFirstSelected=0xffffffff;
    for (i=0;i<dwNbItem;i++)
    {
        UINT uiState = ListView_GetItemState(hWndLV,i,LVIS_SELECTED |LVIS_FOCUSED );
        if ((uiState & LVIS_SELECTED)!=0)
            dwNbSelected++;
        if ((((uiState & LVIS_SELECTED)!=0) && (dwPosFirstSelected==0xffffffff)) ||
            ((uiState & LVIS_FOCUSED)!=0))
            dwPosFirstSelected=i;
    }
    if (pdwPosFirstSelected!=NULL)
        *pdwPosFirstSelected=dwPosFirstSelected;
    return dwNbSelected;
}

BOOL RefreshGrayingMenu(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo)
{
    HMENU hMenu;
    BOOL fDfsOpened = DfsFileAndInfo.DfsFile != NULL;
    BOOL fBaseVersionSelectable = DfsFileAndInfo.fBaseDirectoryNeeded;
    BOOL fVersionSelected=FALSE;
    BOOL fExtractEnabled;
    BOOL fEnableSubDfs=FALSE;
    BOOL fEnableAppendDfs=FALSE;
    BOOL fDeletePossible;
    BOOL fDelVersionEnabled;
    BOOL fFilePropertiesEnabled;

    if (fDfsOpened)
        fVersionSelected = DfsFileAndInfo.dfCurDir!=TVITEMPARAM_ROOT;
    hMenu = GetMenu(guiItem.hwndMain);

    fExtractEnabled=(fVersionSelected && fDfsOpened);

    fExtractEnabled=FALSE;
    if (fVersionSelected && fDfsOpened)
        fExtractEnabled=TRUE;
    if ((!fVersionSelected) && fDfsOpened)
    {
        fExtractEnabled = GetListViewSelectionStatus(guiItem.hwndLV,NULL)==1;
    }

    if ((!fVersionSelected) && fDfsOpened)
    {
        fEnableSubDfs = GetListViewSelectionStatus(guiItem.hwndLV,NULL)>=1;
    }

    fFilePropertiesEnabled = FALSE;
    if ((fVersionSelected) && fDfsOpened)
    {
        fFilePropertiesEnabled = GetListViewSelectionStatus(guiItem.hwndLV,NULL)==1;
    }

    fEnableAppendDfs = fDfsOpened;

    fDelVersionEnabled = fEnableSubDfs || fVersionSelected;

    EnableMenuItem(hMenu,IDM_VERSION_EXTRACT,fExtractEnabled ? MF_ENABLED:MF_GRAYED);
    EnableMenuItem(hMenu,IDM_VERSION_VERSIONPROPERTIES,fExtractEnabled ? MF_ENABLED:MF_GRAYED);

    EnableMenuItem(hMenu,IDM_VERSION_DELETE,fDelVersionEnabled ? MF_ENABLED:MF_GRAYED);
    SendMessage(guiItem.hwndTB,TB_ENABLEBUTTON,IDM_VERSION_DELETE,MAKELONG(fDelVersionEnabled,0));

    EnableMenuItem(hMenu,IDM_FILEPROPERTIES,fFilePropertiesEnabled ? MF_ENABLED:MF_GRAYED);
    SendMessage(guiItem.hwndTB,TB_ENABLEBUTTON,IDM_FILEPROPERTIES,MAKELONG(fFilePropertiesEnabled,0));

    SendMessage(guiItem.hwndTB,TB_ENABLEBUTTON,IDM_VERSION_EXTRACT,MAKELONG(fExtractEnabled,0));

    EnableMenuItem(hMenu,IDM_VERSION_ZIPFILE,fExtractEnabled ? MF_ENABLED:MF_GRAYED);
    SendMessage(guiItem.hwndTB,TB_ENABLEBUTTON,IDM_VERSION_ZIPFILE,MAKELONG(fExtractEnabled,0));


    fDeletePossible=FALSE;

    if (((DWORD)DfsFileAndInfo.dfCurDir) != TVITEMPARAM_ROOT)
    {
        dfuLong32 dfNbSel;
        //dfNbSel = GetListViewSelectionStatus(guiItem.hwndLV,NULL);
        dfNbSel =ListView_GetSelectedCount(guiItem.hwndLV);
        if (dfNbSel >= 1)
           fDeletePossible=TRUE;
    }

    EnableMenuItem(hMenu,IDM_FILESDELETE,fDeletePossible ? MF_ENABLED:MF_GRAYED);

    EnableMenuItem(hMenu,IDM_APPENDSVFTOSVF,fEnableAppendDfs ? MF_ENABLED:MF_GRAYED);
    SendMessage(guiItem.hwndTB,TB_ENABLEBUTTON,IDM_APPENDSVFTOSVF,MAKELONG(fEnableAppendDfs,0));

    EnableMenuItem(hMenu,IDM_VERSION_GENERATESUBDFS,fEnableSubDfs ? MF_ENABLED:MF_GRAYED);
    SendMessage(guiItem.hwndTB,TB_ENABLEBUTTON,IDM_VERSION_GENERATESUBDFS,MAKELONG(fEnableSubDfs,0));

    EnableMenuItem(hMenu,IDM_VERSION_ADDNEWVERSION,fDfsOpened ? MF_ENABLED:MF_GRAYED);
    EnableMenuItem(hMenu,IDM_VERSION_ADDNEWVERSIONFROMZIP,fDfsOpened ? MF_ENABLED:MF_GRAYED);
    SendMessage(guiItem.hwndTB,TB_ENABLEBUTTON,IDM_VERSION_ADDNEWVERSION,MAKELONG(fDfsOpened,0));

    EnableMenuItem(hMenu,IDM_VERSION_SPECIFYBASEDIR,(fDfsOpened && fBaseVersionSelectable) ? MF_ENABLED:MF_GRAYED);
    EnableMenuItem(hMenu,IDM_FILE_CLOSE,fDfsOpened ? MF_ENABLED:MF_GRAYED);

    guiItem.RefreshNormalStatusBar(DfsFileAndInfo);
    return TRUE;
}

BOOL DisplayErrorMessage(HWND hWnd,UINT uId,H_ERROR_INFO & hei,BOOL fDisplayMessageIfNoInfo)
{
      dfwcharp dfFileName=NULL;
      dfwcharp dfErrorMsg=NULL;
      dfwchar dfEmpty[2];
      dfuLong32 dfSizeFileName,dfSizeErrorMsg;
      TCHAR szErrFmt[MAX_PATH+2] = "";
      TCHAR szErrMsg[(MAX_PATH*4)+2] = "";
      dfEmpty[0]=dfEmpty[1]=0;

      if ((hei == NULL) && (!fDisplayMessageIfNoInfo))
          return TRUE;

      if (hei != NULL)
      {
        GetErrorInfoItemByTag(hei,DFS_ERRORTAG_FILENAME,(dfbytep*)(&dfFileName),&dfSizeFileName);
        GetErrorInfoItemByTag(hei,DFS_ERRORTAG_ERRORMSG,(dfbytep*)(&dfErrorMsg),&dfSizeErrorMsg);
      }

      LoadString(ghInstRes,uId,szErrFmt,sizeof(szErrMsg)/sizeof(TCHAR));
      wsprintf(szErrMsg,szErrFmt,
          (dfFileName==NULL)?dfEmpty:dfFileName,
          (dfErrorMsg==NULL)?dfEmpty:dfErrorMsg);
      return MessageBox(hWnd,szErrMsg,NULL,MB_ICONERROR|MB_OK)==MB_OK;
}

BOOL CompareRelativeFileName(LPCTSTR lpszFileName1,LPCTSTR lpszFileName2)
{
TCHAR szConvertedItem1[(MAX_PATH*2)+4];
TCHAR szConvertedItem2[(MAX_PATH*2)+4];
LPTSTR lpFilePart1=NULL;
LPTSTR lpFilePart2=NULL;

  szConvertedItem1[0]=szConvertedItem2[1]=0;
  GetFullPathName(lpszFileName1,MAX_PATH*2,szConvertedItem1,&lpFilePart1);
  GetFullPathName(lpszFileName2,MAX_PATH*2,szConvertedItem2,&lpFilePart1);
  return lstrcmpi(szConvertedItem1,szConvertedItem2)==0;
}

BOOL DoLoadDfs(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,LRUMENU &lrum,
               LPCTSTR lpszDfs,BOOL fAskBaseDir,LPCTSTR lpPreselectedBaseDir,
               dfuLong32 dfSelectVersion)
{
  DFSFILE DfsFile = NULL;
  DFSFILEINFOPARAM DfsFileParam;
  BOOL fRet;
  BOOL fNeedDfsBaseDirectory=FALSE;
  BOOL fBasePreselectAccepted=FALSE;
  BOOL fVerboseAsk=TRUE;
  BOOL fCancel=FALSE;
  H_ERROR_INFO hei=NULL;
  DFSFEATUREPARAM DfsFeatureParam;


  if (lstrlen(guiItem.GetszFileName())!=0)
  {
      if (CompareRelativeFileName(guiItem.GetszFileName(),lpszDfs))
      {
          TCHAR szErrMsgFmt[MAX_PATH] = "";
          TCHAR szErrMsg[MAX_PATH*2] = "";
          LoadString(ghInstRes,IDS_FILEALREADYOPEN,szErrMsgFmt,sizeof(szErrMsgFmt)/sizeof(TCHAR));
          wsprintf(szErrMsg,szErrMsgFmt,guiItem.GetszFileName());

          MessageBox(guiItem.GetHwndMain(),szErrMsg,NULL,MB_OK|MB_ICONERROR);
          return FALSE;
      }
  }
  DfsFileAndInfo.fBaseDirectorySelected = FALSE;

  DfsFileParam.sizeStruct = sizeof(DfsFileParam);
  DfsFileParam.dfStatus = DFS_READABLE | DFS_WRITABLE; /****+++---**/
  DfsFileParam.dfStatus = DFS_WRITABLE; /****+++---**/
  #ifdef UNICODE
  DfsFileParam.filename = FileName;
  #else
  WCHAR wFileName[MAX_PATH];
  ConvertAnsiToUnicode(lpszDfs,(dfwcharp)wFileName,sizeof(wFileName)/sizeof(WCHAR));
  DfsFileParam.filename = (dfwcharp)wFileName;
  #endif
  if (DfsFileOpen(&DfsFileParam, &DfsFile, &hei) != DFS_SUCCESS)
  {
      DisplayErrorMessage(guiItem.hwndMain,IDS_ERROROPENING,hei,TRUE);
      FreeErrorInfoBlock(hei);
      return FALSE;
  }



  DfsFeatureParam.fComputeMd5 = guiItem.fMd5;
  DfsFeatureParam.fComputeSha1 = guiItem.fSha1;
  DfsFeatureParam.fComputeSha256 = guiItem.fSha256;
  DfsFeatureParam.fStripIdenticalBody = guiItem.pfStripIdentical;
  SetDfsFeatureParam(DfsFile,&DfsFeatureParam);

  ConvertOldDirectoryCommentStorage(DfsFile,NULL);

  guiItem.SetszFileName(lpszDfs,TRUE,FALSE);
  fRet = RefreshDfs(DfsFile,guiItem,DfsFileAndInfo,lpszDfs);

  if (fRet)
  {
      lrum.AddNewItem((LPSTR)lpszDfs);
      lrum.PlaceMenuLRUItem(GetSubMenu(GetMenu(guiItem.GetHwndMain()),0),IDM_EXIT);
  }
  else
      guiItem.SetszFileName(NULL,TRUE,FALSE);

  if (fRet && (DfsFileAndInfo.dfNbDir>0) && fAskBaseDir)
  {
      fNeedDfsBaseDirectory = DfsFileAndInfo.fBaseDirectoryNeeded;
  }

  if ((fNeedDfsBaseDirectory) && (lpPreselectedBaseDir==NULL))
  {
      lpPreselectedBaseDir = guiItem.szDefaultDirPreviousVersion;
      fVerboseAsk=FALSE;
  }

  if (fRet && fNeedDfsBaseDirectory && (lpPreselectedBaseDir!=NULL))
  {
      fBasePreselectAccepted = TrySetDfsBaseDirectory(lpPreselectedBaseDir,guiItem,DfsFileAndInfo,0,TRUE,fVerboseAsk,fCancel);
  }

  if (fRet && fNeedDfsBaseDirectory && (!fBasePreselectAccepted))
  {
      int iRet;
      TCHAR szMsg[MAX_PATH*2]="";
      TCHAR szTitle[MAX_PATH*2]="";
      LoadString(ghInstRes,IDS_ASKBASEDIR,szMsg,sizeof(szMsg)/sizeof(TCHAR));
      LoadString(ghInstRes,IDS_ASKBASEDIRTITLE,szTitle,sizeof(szTitle)/sizeof(TCHAR));
      iRet = MessageBox(guiItem.hwndMain,szMsg,szTitle,MB_YESNO|MB_ICONQUESTION);



      if (iRet == IDYES)
      {
          LPTSTR lpFilePart=NULL;
          TCHAR szBasePath[MAX_PATH+8];
          GetFullPathName(lpszDfs,MAX_PATH,szBasePath,&lpFilePart);
          if (lpFilePart != NULL)
              *lpFilePart = 0;
          DoAskAndSetDfsBaseDirectory(guiItem,DfsFileAndInfo,szBasePath,fCancel);

          /* refill */
          /*guiItem.FillTreeView(DfsFileAndInfo);*/
      }
  }

  if (fCancel || (!fRet))
  {
      DoCloseDfs(guiItem,DfsFileAndInfo);
      fRet=FALSE;
  }
  guiItem.FillTreeView(DfsFileAndInfo,dfSelectVersion);
  RefreshGrayingMenu(guiItem,DfsFileAndInfo);
  FreeErrorInfoBlock(hei);
  return fRet;
}


typedef struct
{
    DWORD dwMinProgress;
    DWORD dwMaxProgress;
    GUIITEM * pGuiItem;
} PROGRESSCBPARAM;

BOOL DFSCALLBACK ProgressCallBackCreatePatching(PROGRESSCALLBACKINFO * pProgressCallBackInfo)
{
    PROGRESSCBPARAM* ppcp;
    DWORD dwProgressWidth ;

    if ((pProgressCallBackInfo->dfEvent==DFCBM_PROGRESSWORKINGFILE) && (pProgressCallBackInfo->dfUserPtr != NULL))
    {
      ppcp = (PROGRESSCBPARAM*)(pProgressCallBackInfo->dfUserPtr);
      dwProgressWidth = ppcp->dwMaxProgress - ppcp->dwMinProgress;
      if (ppcp->dwMaxProgress>0)
          if ((pProgressCallBackInfo->dfDirOrigSize>0) && (dwProgressWidth > 0))
          {
              DWORD dwPos ;
              dwPos = CalculateRatio(pProgressCallBackInfo->dfDirOrigDone,
                                     pProgressCallBackInfo->dfDirOrigSize,
                                     dwProgressWidth) +
                      (ppcp->dwMinProgress);

              ppcp->pGuiItem ->SetProgressPos(dwPos);
          }
    }
    return TRUE;
}

BOOL DFSCALLBACK SetPosCallBack(dfuLong32 dwPos,dfvoidp, dfvoidp dfUserPtr)
{
    GUIITEM* pGuiItem=(GUIITEM*)dfUserPtr;
    pGuiItem->SetProgressPos(dwPos);
    return TRUE;
}

/*
typedef dfuLong32(DFSCALLBACK * tConfirmBeforeCreatingFile)(dfwcharpc dfFileName,
                                                    dfvoidp dfpAdditionnalInfo,
                                                    dfvoidp dfUserPtr);
*/
typedef struct
{
    LPTSTR lpGlobalTextYNA;
    LPTSTR lpGlobalCaptYNA;
} DLGYESNOALLPARAM;
BOOL CALLBACK YesNoAllFmtProc(HWND hDlg,WORD message,
                                           WPARAM wParam,LPARAM lParam)
{
  switch (message)
    {
        case WM_INITDIALOG:                /* message: initialize dialog box */
            {
                DLGYESNOALLPARAM* pDlgYesNoAllParam=(DLGYESNOALLPARAM*)lParam;
                CenterWindow(hDlg);
                SetDlgItemText(hDlg,IDC_LINE1,pDlgYesNoAllParam->lpGlobalTextYNA);
                SetWindowText(hDlg,pDlgYesNoAllParam->lpGlobalCaptYNA);

                return (TRUE);
            }

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam,lParam))
               {
                case IDCANCEL:
                case IDC_ALL:
                case IDYES :
                case IDNO :
                case IDC_YESALL:
                case IDC_NOALL:
                    EndDialog(hDlg,GET_WM_COMMAND_ID(wParam,lParam));
                    return TRUE;
               }

    }
  return FALSE;
}

int MessageYesNoAll(HWND hWnd,UINT uiRes,UINT uiTitle,LPCSTR lpParam,BOOL fHasNoAll)
{
TCHAR szFmt[MAX_PATH*2];
TCHAR szMsg[MAX_PATH*2];
TCHAR szTitle[MAX_PATH*2];
DLGYESNOALLPARAM DlgYesNoAllParam;

  LoadString(ghInstRes,uiRes,szFmt,sizeof(szFmt)-1);
  wsprintf(szMsg,szFmt,lpParam);
  LoadString(ghInstRes,uiTitle,szTitle,sizeof(szTitle)-1);
  DlgYesNoAllParam.lpGlobalCaptYNA = szTitle;
  DlgYesNoAllParam.lpGlobalTextYNA = szMsg;
  return (int)DialogBoxParam(ghInstRes,
                        MAKEINTRESOURCE(IDD_YESNOCANCALL),
                        hWnd,(DLGPROC)YesNoAllFmtProc,
                        (LPARAM)&DlgYesNoAllParam);
}

typedef struct
{
    HWND hwndMain;
    BOOL fYesAll;
    BOOL fNoAll;
} OVERWRITEPARAM;

dfuLong32 DFSCALLBACK ConfirmBeforeCreatingFile(dfwcharpc dfFileName,
                                              dfvoidp dfpAdditionnalInfo,
                                              dfvoidp dfUserPtr)
{
    dfuLong32 ret= CONFIRM_BEFORE_CREATING_FILE_OK;
    OVERWRITEPARAM* powp = (OVERWRITEPARAM*)dfUserPtr;
    LOWLEVELFILE llr;
    if (powp ->fYesAll)
        return CONFIRM_BEFORE_CREATING_FILE_OK;

    llr=OpenLowLevel(dfFileName,OPEN_READ,FALSE,FALSE,0,NULL);
    if (llr != NULL)
    {
        TCHAR szTxt[MAX_PATH*2];
        int iMR;
        LowLevelClose(llr,NULL);
        if (powp ->fNoAll)
            return CONFIRM_BEFORE_CREATING_FILE_SKIP;

        wsprintf(szTxt,"%ws",dfFileName);
        iMR=MessageYesNoAll(powp->hwndMain,IDS_CONFIRMOVERWRITEEXTRFORMAT,IDS_CONFIRMOVERWRITEEXTRCAPT,szTxt,FALSE);
        /*
        wsprintf(szTxt,"overwrite %ws",dfFileName);
        iMR=MessageBox(powp->hwndMain,szTxt,"Overwrite",MB_YESNOCANCEL|MB_ICONQUESTION);
        */
        if ((iMR==IDC_ALL) || (iMR==IDC_YESALL))
            powp->fYesAll=TRUE;
        if (iMR==IDC_NOALL)
        {
            powp ->fNoAll = TRUE;
            ret=CONFIRM_BEFORE_CREATING_FILE_SKIP;
        }
        if (iMR==IDNO)
            ret=CONFIRM_BEFORE_CREATING_FILE_SKIP;
        if (iMR==IDCANCEL)
            ret=CONFIRM_BEFORE_CREATING_FILE_STOP;
    }
    return ret;
}

BOOL DoExtracting(HWND hwndMain,GUIITEM &guiItem,DWORD dwMinProgress,DWORD dwMaxProgress,
                  DFSFILE DfsFile,PTSTR pszBaseDirExtract,
                  FILESET** ppfsDest,BOOL fTempDestExtr,dfuLong32 dfDirExtr,PDIRINFO* pDirInfo,
                  BOOL fBaseDirectorySelected,dfuLong32 dfBaseDirNum,LPCTSTR lpBaseDirectory,
                  DWORD dwMapFileNumber,EXTRACTINGMAPITEM* lpExtractingMap,BOOL fOverwriteExtracting,
                  BOOL fFlatExtracting, H_ERROR_INFO * pei)
{
    BOOL fRet=FALSE;
    BOOL fBaseNeeded ;
    dfwchar wchBaseDirExtract[1024+(MAX_PATH*2)];
    dfwchar wchBaseDirectory[1024+(MAX_PATH*2)];
    OVERWRITEPARAM OverWriteParam;

    if (ppfsDest != NULL)
        *ppfsDest = NULL;



    fBaseNeeded = ((*pDirInfo)->dfTypeDir == TYPEDIR_FILECRCONLY);
    if (fBaseNeeded)
    {
        /* we need dir selected */
        if (!fBaseDirectorySelected)
           {
               TCHAR szErrMsg[MAX_PATH] = "";
               LoadString(ghInstRes,IDS_NOBASEDIR,szErrMsg,sizeof(szErrMsg)/sizeof(TCHAR));
               if (hwndMain!=NULL)
                 MessageBox(hwndMain,szErrMsg,NULL,MB_OK|MB_ICONERROR);
               return FALSE;
           }

        if (dfBaseDirNum>dfDirExtr)
           {
               TCHAR szErrMsg[MAX_PATH] = "";
               LoadString(ghInstRes,IDS_NOBASEDIRADAPTED,szErrMsg,sizeof(szErrMsg)/sizeof(TCHAR));
               if (hwndMain!=NULL)
                 MessageBox(hwndMain,szErrMsg,NULL,MB_OK|MB_ICONERROR);
               return FALSE;
           }
    }

    ConvertTCharToUnicode(pszBaseDirExtract,(dfwcharp)wchBaseDirExtract,lstrlen(pszBaseDirExtract)+0x8);
    wchBaseDirectory[0]='\0';
    if (fBaseDirectorySelected)
        ConvertTCharToUnicode(lpBaseDirectory,(dfwcharp)wchBaseDirectory,lstrlen(lpBaseDirectory)+0x8);

    OverWriteParam.hwndMain = hwndMain;
    OverWriteParam.fYesAll = fOverwriteExtracting;
    OverWriteParam.fNoAll = FALSE;
    FILESET* pFileSetBase=NULL;

    fRet= DoMultiExtracting(
                  DfsFile,wchBaseDirExtract,
                  ppfsDest,fTempDestExtr,dfDirExtr,pDirInfo,
                  fBaseDirectorySelected,dfBaseDirNum,wchBaseDirectory,pFileSetBase,
                  dwMapFileNumber,lpExtractingMap,
                  NULL,
                  SetPosCallBack,&guiItem,
                  (&ConfirmBeforeCreatingFile),&OverWriteParam,/***/
                  NULL,NULL,
                  dwMinProgress,dwMaxProgress,fFlatExtracting,FALSE,pei);
    return fRet;
}


BOOL AskDirectory(HWND hDlg,LPTSTR lpDir)
{
  LPMALLOC pMalloc;
  BOOL     fRet= FALSE;

  if(SUCCEEDED(::SHGetMalloc(&pMalloc)))
  {
    LPITEMIDLIST pidlBrowse;
    BROWSEINFO   bi;
    //TCHAR szText[MAX_PATH+1];
    TCHAR szPath[(MAX_PATH+1)*2];

    //::LoadString(ghinst, IDS_FOLDER_BROWSE, szText, MAX_PATH);
    ::ZeroMemory(szPath, sizeof(szPath));
    ::ZeroMemory(&bi, sizeof(BROWSEINFO));


    bi.hwndOwner      = hDlg;
    bi.pidlRoot       = NULL;
    bi.pszDisplayName = lpDir;
    bi.lpszTitle      = NULL;//szText;
    bi.ulFlags        = BIF_RETURNONLYFSDIRS | BIF_RETURNFSANCESTORS | BIF_USENEWUI | BIF_VALIDATE;
    bi.lpfn           = GDE_BrowseCallbackProc;//NULL;
    bi.lParam         = (LPARAM)lpDir;
    bi.iImage         = 0;

    pidlBrowse = ::SHBrowseForFolder(&bi);
    if(pidlBrowse)
    {
      if(::SHGetPathFromIDList(pidlBrowse, lpDir))
          fRet=TRUE;
    }
      // clean-up
    pMalloc->Free(pidlBrowse);
    pMalloc->Release();
  }

  return fRet;
}




BOOL DoExtract(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,BOOL fZipFileBuild)
{
    BOOL fRet;
    TCHAR szDirectory[(MAX_PATH+1)*2]="";
    TCHAR szZipFileName[(MAX_PATH+1)*2]="";
    EXTRACTINGMAPITEM* lpfExtractItemMap=NULL;
    PCDIRINFO pCurDirInfo;
    FILESET* pfsDest=NULL;
    BOOL fCancelDlg=FALSE;
    dfuLong32 i;
    dfuLong32 dfDirExtract;
    BOOL fUseListViewSelectInfo=TRUE;

    if (DfsFileAndInfo.DfsFile == NULL)
        return FALSE;
    if (((DWORD)DfsFileAndInfo.dfCurDir) == TVITEMPARAM_ROOT)
    {
       if (GetListViewSelectionStatus(guiItem.hwndLV,&dfDirExtract)!=1)
           return FALSE;
       fUseListViewSelectInfo=FALSE;
    }
    else
        dfDirExtract=DfsFileAndInfo.dfCurDir;



    GetCurrentDirectory(sizeof(szDirectory)/sizeof(TCHAR),szDirectory);
    if (guiItem.szDefaultDirExtract[0] != 0)
        lstrcpy(szDirectory,guiItem.szDefaultDirExtract);

    #if (!defined(_DEBUG)) || (!defined(IGNOREASKING))
    if (fZipFileBuild)
    {
        TCHAR szFilter[MAX_PATH*2]="";
        MYOPENFILENAME ofn;

        InitOpenFileName((MYOPENFILENAME*)&ofn,guiItem.GetHwndMain(),IDS_TYPEOPENZIP,szFilter,sizeof(szFilter)-1,
                              szZipFileName,MAX_PATH,NULL,0,0);
        ofn.lpstrDefExt="zip";
        //ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
        if (!GetSaveFileName((OPENFILENAME*)&ofn))
            return FALSE;
    }
    else
    {
        if (!AskDirectory(guiItem.hwndMain,szDirectory))
            return FALSE;
    }
    // --++**//###
    #else
      lstrcpy(szZipFileName,"c:\\tsext\\export.zip");
    #endif

    H_ERROR_INFO hei=NULL;
    lstrcpy(guiItem.szDefaultDirExtract,szDirectory);
    // TODO QUICK : use SHBrowseForFolder for asking directory
    guiItem.InstallProgressBar(fZipFileBuild ? 200 : 100);

    pCurDirInfo = *(DfsFileAndInfo.pDirInfo+ dfDirExtract);

    if (fUseListViewSelectInfo)
    {
        dfuLong32 dfNbSelected=0;
        lpfExtractItemMap = (EXTRACTINGMAPITEM*)DfsMalloc(sizeof(EXTRACTINGMAPITEM)*(pCurDirInfo ->dfNbFile + 1));

        for (i=0;i<pCurDirInfo ->dfNbFile;i++)
            *(lpfExtractItemMap+i)=ExtractClassic;

        for (i=0;i<pCurDirInfo ->dfNbFile;i++)
        {
            dfuLong32 iItem = *(guiItem.pdfwListViewSortMap+i);
            *(lpfExtractItemMap+iItem) =
              (ListView_GetItemState(guiItem.hwndLV,i,LVIS_SELECTED) != 0) ? ExtractClassic : ExtractNone;
        }

        for (i=0;i<pCurDirInfo ->dfNbFile;i++)
            if (*(lpfExtractItemMap+i))
                dfNbSelected++;

        if (dfNbSelected==0)
            for (i=0;i<pCurDirInfo ->dfNbFile;i++)
                *(lpfExtractItemMap+i)=ExtractClassic;
    }

    fRet = DoExtracting(guiItem.hwndMain,guiItem,0,100,
                  DfsFileAndInfo.DfsFile,
                  szDirectory,
                  fZipFileBuild ? &pfsDest : NULL,
                  fZipFileBuild, /* parameter : fTempDestExtr */
                  dfDirExtract,DfsFileAndInfo.pDirInfo,
                  DfsFileAndInfo.fBaseDirectorySelected,
                  DfsFileAndInfo.dfBaseDirNum,
                  DfsFileAndInfo.lpBaseDirectory,
                  fUseListViewSelectInfo ? pCurDirInfo ->dfNbFile : 0,
                  fUseListViewSelectInfo ? lpfExtractItemMap : NULL,
                  guiItem.fOverwriteExtracting,FALSE,&hei);

    if (lpfExtractItemMap!=NULL)
        DfsFree(lpfExtractItemMap);

    if (fZipFileBuild && fRet)
    {
        dfwchar dfwcZipFileName[MAX_PATH+1];
        dfwcharp pZipComment=NULL;

        ConvertTCharToUnicode(szZipFileName,dfwcZipFileName,MAX_PATH);


        {
            DFTAGBLOCKFLOAT TagBlockFloat;
            dfvoidp TagBufDirName,TagBufDirComment;
            dfuLong32 TagSizeDirName,TagSizeDirComment;
            TagSizeDirName=TagSizeDirComment=0;

            TagBlockFloat = GetDfsTagBlockFloat(DfsFileAndInfo.DfsFile,NULL);
            if (TagBlockFloat != NULL)
            {
                GetTagBlockFloat(TagBlockFloat,dfDirExtract,FLOATNUM_NOSPECIFIC,
                                     DFSTAG_DIR_NAME_FLOAT,&TagBufDirName, &TagSizeDirName);
                GetTagBlockFloat(TagBlockFloat,dfDirExtract,FLOATNUM_NOSPECIFIC,
                                     DFSTAG_DIR_COMMENT_FLOAT,&TagBufDirComment, &TagSizeDirComment);
            }
            if (TagSizeDirName + TagSizeDirComment>0)
            {
                pZipComment = (dfwcharp)DfsMalloc(TagSizeDirName + TagSizeDirComment + 0x10);
                dfuLong32 uln=0;
                if (TagSizeDirName>0)
                {
                  DfsMemcpy(pZipComment,TagBufDirName,TagSizeDirName);
                  uln = dfUnicodeStrlen(pZipComment);
                }
                if (TagSizeDirComment>0)
                {
                  dfwchar cr,lf;

                  cr = L'\x0d';
                  lf = L'\x0a';

                  *(pZipComment+uln) = cr;
                  *(pZipComment+uln+1) = lf;
                  DfsMemcpy(pZipComment+uln+2,TagBufDirComment,TagSizeDirComment);
                }
            }
        }

        for (i=0;(i<pfsDest->dfNbFileItem) && (fRet);i++)
            if (((pfsDest->pFileItem + i)->ExtAction != ExtActionIgnore))
                (pfsDest->pFileItem + i)->fForceDate=TRUE;

        fRet = BuildZipFileFromFileSet(pfsDest,dfwcZipFileName,Z_DEFAULT_COMPRESSION,
                                         pZipComment,
                                         SetPosCallBack,&guiItem,
                                         100,200,&hei);

        FreeFileSet(pfsDest,TRUE);
        DfsFree(pfsDest);

        if (pZipComment!=NULL)
            DfsFree(pZipComment);
    }

    guiItem.RemoveProgressBar();

    if (!fRet)
    {
        DisplayErrorMessage(guiItem.hwndMain,IDS_ERROR_EXTRACTING,hei,(!fRet) && (!fCancelDlg));
    }

    FreeErrorInfoBlock(hei);
    return fRet;
}

BOOL DoGenerateZip(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo)
{
    return DoExtract(guiItem,DfsFileAndInfo,TRUE);
}


BOOL DoSpecifyBaseDirVersion(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo)
{
    BOOL fRet=FALSE;
    if ((DfsFileAndInfo.DfsFile != NULL) && (DfsFileAndInfo.fBaseDirectoryNeeded))
    {
      int iRet;
      TCHAR szMsg[MAX_PATH*2]="";
      TCHAR szTitle[MAX_PATH*2]="";
      BOOL fCancel=FALSE;
      LoadString(ghInstRes,IDS_ASKBASEDIR,szMsg,sizeof(szMsg)/sizeof(TCHAR));
      LoadString(ghInstRes,IDS_ASKBASEDIRTITLE,szTitle,sizeof(szTitle)/sizeof(TCHAR));
      //iRet = MessageBox(guiItem.hwndMain,szMsg,szTitle,MB_YESNO|MB_ICONQUESTION);
      iRet=IDYES;
      fRet=TRUE;


      if (iRet == IDYES)
      {
          LPTSTR lpFilePart=NULL;
          TCHAR szBasePath[MAX_PATH+8];
          GetFullPathName(guiItem.GetszFileName(),MAX_PATH,szBasePath,&lpFilePart);
          if (lpFilePart != NULL)
              *lpFilePart = 0;
          DoAskAndSetDfsBaseDirectory(guiItem,DfsFileAndInfo,szBasePath,fCancel);

          /* refill */
          guiItem.FillTreeView(DfsFileAndInfo);
      }

      if (fCancel)
      {
          DoCloseDfs(guiItem,DfsFileAndInfo);
          fRet=FALSE;
      }
      RefreshGrayingMenu(guiItem,DfsFileAndInfo);
    }
    return fRet;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/*************************************************************************************/
// resize tool
#define RSZ_GRIP_OBJ    ("ResizableGrip")


RESIZABLEDLGHELP::RESIZABLEDLGHELP()
{
    m_hParent = NULL;
    m_wndGrip = NULL;

    pItemCtlInternal = NULL;
    dwNbCtlInternal = dwNbCtlInternalAllocated = 0;
    dwNbCtlInternalStep = 0x40;
    dwRatioShift=0x10000;
}

BOOL RESIZABLEDLGHELP::CheckAllocateNbCtlItem(DWORD dwNbNeeded)
{
    DWORD dwNbTryAllocate;
    ITEMCTLINTERNAL *pItemCtlInternalNew;
    BOOL fRet;
    if (dwNbNeeded <= dwNbCtlInternalAllocated)
        return TRUE;
    dwNbTryAllocate = ((dwNbNeeded + dwNbCtlInternalStep - 1) /
                           dwNbCtlInternalStep) * dwNbCtlInternalStep;
    DWORD dwSizeAllocate = (dwNbTryAllocate)*sizeof(ITEMCTLINTERNAL);
    if (pItemCtlInternal == NULL)
    {
        pItemCtlInternalNew = (ITEMCTLINTERNAL *)DfsMalloc(dwSizeAllocate);
    }
    else
    {
        pItemCtlInternalNew = (ITEMCTLINTERNAL *)DfsRealloc(pItemCtlInternal,dwSizeAllocate);
    }
    fRet = (pItemCtlInternalNew != NULL);
    if (fRet)
    {
        pItemCtlInternal = pItemCtlInternalNew;
        dwNbCtlInternalAllocated = dwNbTryAllocate;
    }

    return fRet;
}

BOOL RESIZABLEDLGHELP::Init(HWND hParent)
{
    RECT rcDlg;
    m_hParent = hParent;
    InitGrip();

    GetClientRect(m_hParent,&rcDlg);
    m_szInitialDlg.cx = rcDlg.right-rcDlg.left;
    m_szInitialDlg.cy = rcDlg.bottom-rcDlg.top;

    GetWindowRect(m_hParent,&rcDlg);
    m_sizeOriginalDialog.cx = rcDlg.right-rcDlg.left;
    m_sizeOriginalDialog.cy = rcDlg.bottom-rcDlg.top;

    return TRUE;
}

BOOL RESIZABLEDLGHELP::GetOriginalDialogSize(SIZE * ptSize)
{
    *ptSize = m_sizeOriginalDialog;
    return TRUE;
}

RESIZABLEDLGHELP::~RESIZABLEDLGHELP()
{
    if (pItemCtlInternal != NULL)
        DfsFree(pItemCtlInternal);
}

BOOL RESIZABLEDLGHELP::InitCtlList(const ITEMCTLINFO* pItemCtlInit,DWORD dwNbItem)
{
    DWORD i=0;

    if (dwNbItem!=NBITEM_UNSPECIFIED)
        if (!CheckAllocateNbCtlItem(dwNbCtlInternal+dwNbItem+1))
            return FALSE;

    for (;;)
    {
        HWND hCtl;
        ITEMCTLINTERNAL *pItemCtlInternalCur;
        RECT rcInitial;

        if (dwNbItem!=NBITEM_UNSPECIFIED)
            if (i>=dwNbItem)
                break;


        if (dwNbItem==NBITEM_UNSPECIFIED)
            if ((pItemCtlInit+i)->uiId==0)
            {
                break;
            }

        if (!CheckAllocateNbCtlItem(dwNbCtlInternal+1))
            return FALSE;

        pItemCtlInternalCur=pItemCtlInternal+dwNbCtlInternal;

        hCtl = GetDlgItem(m_hParent,(pItemCtlInit+i)->uiId);
        if (hCtl!=NULL)
        {
            pItemCtlInternalCur->ItemCtlInfo = *(pItemCtlInit+i);
            GetWindowRect(hCtl,&rcInitial);
            pItemCtlInternalCur->sizeInitial.cx = rcInitial.right - rcInitial.left;
            pItemCtlInternalCur->sizeInitial.cy = rcInitial.bottom - rcInitial.top;
            pItemCtlInternalCur->ptInitial.x = rcInitial.left;
            pItemCtlInternalCur->ptInitial.y = rcInitial.top;
            ::ScreenToClient(m_hParent, &pItemCtlInternalCur->ptInitial);
            dwNbCtlInternal++;
        }
        i++;
    }
    return TRUE;
}

BOOL RESIZABLEDLGHELP::InitRatio(DWORD dwRatioShiftSet)
{
    dwRatioShift=dwRatioShiftSet;
    return TRUE;
}

BOOL RESIZABLEDLGHELP::OnResize()
{
    if (m_wndGrip)
    {
        UpdateGripPos();
        ShowSizeGrip(TRUE);
    }
    MoveAndResizeDlgItem();
    return TRUE;
}

BOOL RESIZABLEDLGHELP::InitGrip()
{
    m_sizeGrip.cx = GetSystemMetrics(SM_CXVSCROLL);
    m_sizeGrip.cy = GetSystemMetrics(SM_CYHSCROLL);
    RECT rect = { 0 , 0, m_sizeGrip.cx, m_sizeGrip.cy };

    m_wndGrip = ::CreateWindowEx(0, ("SCROLLBAR"),
                                (LPSTR)NULL,
                                WS_CHILD | WS_CLIPSIBLINGS | SBS_SIZEGRIP,
                                rect.left, rect.top,
                                rect.right-rect.left,
                                rect.bottom-rect.top,
                                m_hParent,
                                (HMENU)0,
                                NULL,
                                NULL);

    if (m_wndGrip)
    {
        // set a triangular window region
        HRGN rgnGrip, rgn;
        rgn = ::CreateRectRgn(0,0,1,1);
        rgnGrip = ::CreateRectRgnIndirect(&rect);

        for (int y=0; y<m_sizeGrip.cy; y++)
        {
            ::SetRectRgn(rgn, 0, y, m_sizeGrip.cx-y, y+1);
            ::CombineRgn(rgnGrip, rgnGrip, rgn, RGN_DIFF);
        }
        ::SetWindowRgn(m_wndGrip, rgnGrip, FALSE);

        // subclass control
        /*
        ::SetProp(m_wndGrip, RSZ_GRIP_OBJ,
            (HANDLE)GetWindowLongPtr(m_wndGrip, GWLP_WNDPROC));
        ::SetWindowLong(m_wndGrip, GWL_WNDPROC, (LONG)GripWindowProc);
*/
        // force dialog styles (RESIZABLE BORDER, NO FLICKERING)


        /*
        ::SetWindowLong(m_hParent, GWL_STYLE,
            ::GetWindowLong(m_hParent, GWL_STYLE) | WS_THICKFRAME | WS_CLIPCHILDREN);
        */

        // update pos
        UpdateGripPos();
        ShowSizeGrip(TRUE);
    }

    return m_wndGrip!=NULL;
}

BOOL RESIZABLEDLGHELP::MoveAndResizeDlgItem()
{
    SIZE szDlgNew;
    RECT rcDlg;
    DWORD i;
    LONG lShiftDlgX,lShiftDlgY;

    GetClientRect(m_hParent,&rcDlg);
    szDlgNew.cx = rcDlg.right-rcDlg.left;
    szDlgNew.cy = rcDlg.bottom-rcDlg.top;

    lShiftDlgX = ((LONG)(szDlgNew.cx))- ((LONG)(m_szInitialDlg.cx));
    lShiftDlgY = ((LONG)(szDlgNew.cy))- ((LONG)(m_szInitialDlg.cy));

    for (i=0;i<dwNbCtlInternal;i++)
    {
        ITEMCTLINTERNAL ItemCtlInternalCur=*(pItemCtlInternal+i);
        HWND hCtrl = GetDlgItem(m_hParent,ItemCtlInternalCur.ItemCtlInfo.uiId);

        if (ItemCtlInternalCur.ItemCtlInfo.fInvalidateRect)
          InvalidateRect(hCtrl,NULL,FALSE);
          //InvalidateRect(hCtrl,NULL,FALSE);
        if ((ItemCtlInternalCur.ItemCtlInfo.dwRatioShiftX!=0) ||
            (ItemCtlInternalCur.ItemCtlInfo.dwRatioShiftY!=0))
        {
            RECT rcInWin;
            POINT pt1 ;
            GetWindowRect(hCtrl,&rcInWin);
            pt1.x = ItemCtlInternalCur.ptInitial.x;
            pt1.y = ItemCtlInternalCur.ptInitial.y;
            //::ScreenToClient(m_hParent, &pt1);
            LONG x = pt1.x +
                 MulDiv(lShiftDlgX,ItemCtlInternalCur.ItemCtlInfo.dwRatioShiftX,dwRatioShift);
            LONG y = pt1.y +
                 MulDiv(lShiftDlgY,ItemCtlInternalCur.ItemCtlInfo.dwRatioShiftY,dwRatioShift);
            ::SetWindowPos(hCtrl, NULL, x,y, 0,0, SWP_NOSIZE);
        }

        if ((ItemCtlInternalCur.ItemCtlInfo.dwRatioShiftCX!=0) ||
            (ItemCtlInternalCur.ItemCtlInfo.dwRatioShiftCY!=0))
        {
            LONG cx = (ItemCtlInternalCur.sizeInitial.cx) +
                 MulDiv(lShiftDlgX,ItemCtlInternalCur.ItemCtlInfo.dwRatioShiftCX,dwRatioShift);
            LONG cy = (ItemCtlInternalCur.sizeInitial.cy) +
                 MulDiv(lShiftDlgY,ItemCtlInternalCur.ItemCtlInfo.dwRatioShiftCY,dwRatioShift);
            ::SetWindowPos(hCtrl, NULL, 0,0, cx,cy, SWP_NOMOVE);
        }
    }
//InvalidateRect(m_hParent,NULL,TRUE);
    return TRUE;
}

void RESIZABLEDLGHELP::UpdateGripPos()
{
    // size-grip goes bottom right in the client area
    // (any right-to-left adjustment should go here)

    RECT rect;
    ::GetClientRect(m_hParent,&rect);

    rect.left = rect.right - m_sizeGrip.cx;
    rect.top = rect.bottom - m_sizeGrip.cy;

    // must stay below other children
    ::SetWindowPos(m_wndGrip,HWND_BOTTOM, rect.left, rect.top, 0, 0,
        SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREPOSITION);

    // maximized windows cannot be resized



    if ( ::IsZoomed(m_hParent) )
    {
        ::EnableWindow(m_wndGrip, FALSE);
        ShowSizeGrip(FALSE);
    }
    else
    {
        ::EnableWindow(m_wndGrip, TRUE);
        ShowSizeGrip(TRUE);
    }
}

void RESIZABLEDLGHELP::ShowSizeGrip(BOOL bShow)
{
    ::ShowWindow(m_wndGrip, bShow ? SW_SHOW : SW_HIDE);
}

/*************************************************************************************/
/*************************************************************************************/

typedef struct
{
    STATIC_ARRAY* psaFilesInsert;
    EXTINFOCACHE* pExtInfoCache;
} INSERTFILEUIPARAM;

typedef struct
{
    STATIC_ARRAY* psaFilesInsert ;
    dfuLong32 dfItem;
} ITEM_FILE_LIST_SORTING;


typedef struct
{
    dfwcharpc dfFileNameOnDisk;
    dfwcharpc dfFileNameToStore;
    dfuLong64 dfSize;
    DFSTM DfsTm;
    EXTINFOCACHE* pExtInfoCache;
} ITEM_FILE_LIST;

typedef ITEM_FILE_LIST* PITEM_FILE_LIST;

long DFSCALLBACK fncCompareItemFileList(const void *lpElem1, const void *lpElem2)
{
  long lRet;
  const ITEM_FILE_LIST *pfi1 = (const ITEM_FILE_LIST *) lpElem1;
  const ITEM_FILE_LIST *pfi2 = (const ITEM_FILE_LIST *) lpElem2;
  lRet = dfUnicodeStrcmpi(pfi1->dfFileNameToStore, pfi2->dfFileNameToStore);
  return lRet;
}

BOOL DFSCALLBACK fncDestructorItemFileList(const void *lpElem)
{
  ITEM_FILE_LIST *pfi = (ITEM_FILE_LIST *) lpElem;
  if (pfi->dfFileNameOnDisk != NULL)
    DfsFree((dfwcharp) pfi->dfFileNameOnDisk);
  if (pfi->dfFileNameToStore != NULL)
    DfsFree((dfwcharp) pfi->dfFileNameToStore);

  return TRUE;
}


BOOL DFSCALLBACK fncDestructorItemFileListSorting(const void *lpElem)
{
    return TRUE;
}

long DFSCALLBACK fncCompareItemFileListStoreName(const void *lpElem1, const void *lpElem2)
{
  long lRet;
  //const ITEM_FILE_LIST *pfi1 = ((const ITEM_FILE_LIST_SORTING *) lpElem1)->pItemFileList;
  //const ITEM_FILE_LIST *pfi2 = ((const ITEM_FILE_LIST_SORTING *) lpElem2)->pItemFileList;
  dfuLong32 dfItem1 = ((const ITEM_FILE_LIST_SORTING *) lpElem1)->dfItem;
  dfuLong32 dfItem2 = ((const ITEM_FILE_LIST_SORTING *) lpElem2)->dfItem;
  STATIC_ARRAY* psaFilesInsert1 = ((const ITEM_FILE_LIST_SORTING *) lpElem1)->psaFilesInsert;
  STATIC_ARRAY* psaFilesInsert2 = ((const ITEM_FILE_LIST_SORTING *) lpElem2)->psaFilesInsert;
  const ITEM_FILE_LIST *pfi1 = (const ITEM_FILE_LIST *)psaFilesInsert1->GetElemPtrSA(dfItem1);
  const ITEM_FILE_LIST *pfi2 = (const ITEM_FILE_LIST *)psaFilesInsert2->GetElemPtrSA(dfItem2);

  lRet = dfUnicodeStrcmpi(pfi1->dfFileNameToStore, pfi2->dfFileNameToStore);
  if (lRet == 0)
      lRet = dfUnicodeStrcmpi(pfi1->dfFileNameOnDisk, pfi2->dfFileNameOnDisk);
  return lRet;
}

long DFSCALLBACK fncCompareItemFileListNameOnDisk(const void *lpElem1, const void *lpElem2)
{
  long lRet;
  dfuLong32 dfItem1 = ((const ITEM_FILE_LIST_SORTING *) lpElem1)->dfItem;
  dfuLong32 dfItem2 = ((const ITEM_FILE_LIST_SORTING *) lpElem2)->dfItem;
  STATIC_ARRAY* psaFilesInsert1 = ((const ITEM_FILE_LIST_SORTING *) lpElem1)->psaFilesInsert;
  STATIC_ARRAY* psaFilesInsert2 = ((const ITEM_FILE_LIST_SORTING *) lpElem2)->psaFilesInsert;
  const ITEM_FILE_LIST *pfi1 = (const ITEM_FILE_LIST *)psaFilesInsert1->GetElemPtrSA(dfItem1);
  const ITEM_FILE_LIST *pfi2 = (const ITEM_FILE_LIST *)psaFilesInsert2->GetElemPtrSA(dfItem2);

  lRet = dfUnicodeStrcmpi(pfi1->dfFileNameOnDisk, pfi2->dfFileNameOnDisk);
  if (lRet == 0)
      lRet = dfUnicodeStrcmpi(pfi1->dfFileNameToStore, pfi2->dfFileNameToStore);
  return lRet;
}

long DFSCALLBACK fncCompareItemFileListSize(const void *lpElem1, const void *lpElem2)
{
  long lRet;
  dfuLong32 dfItem1 = ((const ITEM_FILE_LIST_SORTING *) lpElem1)->dfItem;
  dfuLong32 dfItem2 = ((const ITEM_FILE_LIST_SORTING *) lpElem2)->dfItem;
  STATIC_ARRAY* psaFilesInsert1 = ((const ITEM_FILE_LIST_SORTING *) lpElem1)->psaFilesInsert;
  STATIC_ARRAY* psaFilesInsert2 = ((const ITEM_FILE_LIST_SORTING *) lpElem2)->psaFilesInsert;
  const ITEM_FILE_LIST *pfi1 = (const ITEM_FILE_LIST *)psaFilesInsert1->GetElemPtrSA(dfItem1);
  const ITEM_FILE_LIST *pfi2 = (const ITEM_FILE_LIST *)psaFilesInsert2->GetElemPtrSA(dfItem2);

  if (pfi1->dfSize > pfi2->dfSize)
      lRet = 1;
  else
  if (pfi1->dfSize < pfi2->dfSize)
      lRet = -1;
  else
      lRet = fncCompareItemFileListStoreName(lpElem1,lpElem2);
  return lRet;
}

long DFSCALLBACK fncCompareItemFileListDate(const void *lpElem1, const void *lpElem2)
{
  long lRet;
  dfuLong32 dfItem1 = ((const ITEM_FILE_LIST_SORTING *) lpElem1)->dfItem;
  dfuLong32 dfItem2 = ((const ITEM_FILE_LIST_SORTING *) lpElem2)->dfItem;
  STATIC_ARRAY* psaFilesInsert1 = ((const ITEM_FILE_LIST_SORTING *) lpElem1)->psaFilesInsert;
  STATIC_ARRAY* psaFilesInsert2 = ((const ITEM_FILE_LIST_SORTING *) lpElem2)->psaFilesInsert;
  const ITEM_FILE_LIST *pfi1 = (const ITEM_FILE_LIST *)psaFilesInsert1->GetElemPtrSA(dfItem1);
  const ITEM_FILE_LIST *pfi2 = (const ITEM_FILE_LIST *)psaFilesInsert2->GetElemPtrSA(dfItem2);

  lRet = (long)CompareDfsTm(&pfi1->DfsTm,&pfi2->DfsTm);
  if (lRet==0)
      lRet = fncCompareItemFileListStoreName(lpElem1,lpElem2);
  return lRet;
}

long DFSCALLBACK fncCompareItemFileListExt(const void *lpElem1, const void *lpElem2)
{
  long lRet;
  EXTINFOCACHE* pExtInfoCache;
  dfuLong32 dfItem1 = ((const ITEM_FILE_LIST_SORTING *) lpElem1)->dfItem;
  dfuLong32 dfItem2 = ((const ITEM_FILE_LIST_SORTING *) lpElem2)->dfItem;
  STATIC_ARRAY* psaFilesInsert1 = ((const ITEM_FILE_LIST_SORTING *) lpElem1)->psaFilesInsert;
  STATIC_ARRAY* psaFilesInsert2 = ((const ITEM_FILE_LIST_SORTING *) lpElem2)->psaFilesInsert;
  const ITEM_FILE_LIST *pfi1 = (const ITEM_FILE_LIST *)psaFilesInsert1->GetElemPtrSA(dfItem1);
  const ITEM_FILE_LIST *pfi2 = (const ITEM_FILE_LIST *)psaFilesInsert2->GetElemPtrSA(dfItem2);

  pExtInfoCache = pfi1->pExtInfoCache;

  lRet = (long)CompareTypeOfFileName(pExtInfoCache,pfi1->dfFileNameToStore,pfi2->dfFileNameToStore);
  if (lRet==0)
      lRet = fncCompareItemFileListStoreName(lpElem1,lpElem2);
  return lRet;
}

/*********************************************************************/

class INSERTFILEUI
{
public:
    INSERTFILEUI()
        {
            psaFilesSorting = new STATIC_ARRAY(sizeof(ITEM_FILE_LIST_SORTING));
            psaFilesSorting->SetFuncDestructorSA(fncDestructorItemFileListSorting);
            psaFilesSorting->SetFuncCompareDataSA(fncCompareItemFileListStoreName);
            dfDfsCurrentFileName = NULL;
            lpszCurrentDirectory = NULL;
        } ;
    /*(GUIITEM &guiItemSet,DFSFILEANDINFO &DfsFileAndInfoSet) :
                  guiItem(guiItemSet),
                  DfsFileAndInfo(DfsFileAndInfoSet) { };*/

    ~INSERTFILEUI()
        {
            delete(psaFilesSorting);
            if (lpszCurrentDirectory!=NULL)
                DfsFree((void*)lpszCurrentDirectory);
        };

    BOOL InitListViewFileToInsertListColumn(HWND hDlg);
    BOOL ProcessDialogInsertFileButtons(HWND hDlg,WORD wId);
    LRESULT OnNotifyInsertListView(HWND hDlg,WPARAM idCtrl,LPNMHDR pnmHdr);
    BOOL SetCurrentDirectory(LPCTSTR lpszCurDirSet) ;
    LPCSTR GetCurrentDirectory()
      { return lpszCurrentDirectory ; }

    void SetDfsCurrentFileName(LPCTSTR dfDfsCurrentFileNameSet)
    {  dfDfsCurrentFileName = (dfDfsCurrentFileNameSet); }

    void SetInfoCache(EXTINFOCACHE* pExtInfoCacheSet)
     { pExtInfoCache = pExtInfoCacheSet ;};
    void SetFileUiParam(INSERTFILEUIPARAM* pInsertFileUiParamSet)
     { pInsertFileUiParam = pInsertFileUiParamSet; };

private:
    BOOL RefreshInsertList(HWND hWnd,int iColumnSort=-1);
    BOOL DoGetDispInfoInsertList(const NMHDR* pnmHdr);
    BOOL RemoveItemInsertList(dfuLong32 dfListViewSortedIntemNumber);
    BOOL RemoveAllItemInsertList(dfuLong32 dfListViewSortedIntemNumber);

    BOOL AddFiles(HWND hWnd);
    BOOL RemoveSelected(HWND hDlg,BOOL RemoveSelected=FALSE);
    BOOL AddDirectory(HWND hDlg);

    INSERTFILEUIPARAM* GetInsertFileUiParam() { return pInsertFileUiParam; };
    BOOL AddOneFileInFileInsertList(LPCSTR lpszFileNameOnDisk,
                                    dfwcharpc dfFileNameToStore,
                                    dfwcharpc dfFileNamePrefixToStore,
                                    BOOL &fStop);

    BOOL AddOneDirInFileInsertList(LPCSTR lpszFileNameOnDisk,
                                    dfwcharpc dfFileNameToStore,
                                    dfwcharpc dfFileNamePrefixToStore,
                                    BOOL &fStop);

    INSERTFILEUIPARAM* pInsertFileUiParam;
    EXTINFOCACHE* pExtInfoCache;
    STATIC_ARRAY* psaFilesSorting;
    LPCTSTR dfDfsCurrentFileName;
    LPCTSTR lpszCurrentDirectory;
};

BOOL INSERTFILEUI::SetCurrentDirectory(LPCTSTR lpszCurDirSet)
{
    LPTSTR lpszNewCurrentDirectory;
    lpszNewCurrentDirectory = (LPTSTR)DfsMalloc((lstrlen(lpszCurDirSet)+1)*sizeof(TCHAR));
    if (lpszNewCurrentDirectory == NULL)
        return FALSE;

    if (lpszCurrentDirectory!=NULL)
        DfsFree((void*)lpszCurrentDirectory);

    lstrcpy(lpszNewCurrentDirectory,lpszCurDirSet);
    lpszCurrentDirectory = lpszNewCurrentDirectory;

    return TRUE;
}

TCHAR GetLastCharOfString(LPCSTR lpszString)
{
    TCHAR cLast=0;
    while ((*lpszString)!=0)
    {
        cLast=*lpszString;
        lpszString=CharNext(lpszString);
    }
    return cLast;
}

#define NBLVCOLUMNFILETOINSERTLIST (4+1+0)

BOOL InitListViewControlFileToInsertListColumn(HWND hwndLV)
{
    int i;

    ListView_DeleteAllItems(hwndLV);
    ListView_SetItemCount(hwndLV, 0);
    InitListViewImageLists(hwndLV);

    {
        HWND hWndHeader = ListView_GetHeader(hwndLV);
        DWORD dwStyle=GetWindowLong(hWndHeader, GWL_STYLE);
        dwStyle |= HDS_BUTTONS ;
        SetWindowLong(hWndHeader, GWL_STYLE,dwStyle);
    }

    for (i=0;i<NBLVCOLUMNFILETOINSERTLIST;i++)
    {
        LV_COLUMN lvC;
        TCHAR szTitle[MAX_PATH];

        lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        lvC.fmt = ((i==2)) ? LVCFMT_RIGHT : LVCFMT_LEFT;  // left-align column
        lvC.cx = 75;            // width of column in pixels
        if (i==0) lvC.cx *=2 ;
        if (i==3) lvC.cx = (lvC.cx*5)/3 ;
        if (i==4) lvC.cx = (lvC.cx*2)/3 ;
        if (i==5)  lvC.cx *=2 ;
        lvC.pszText = szTitle;


        ShowSortColumnTitleBitmap(hwndLV,0,TRUE,FALSE);
        LoadInternatString(IDS_TITLECOLUMNFILELIST1+i,szTitle,sizeof(szTitle)-1);
        if (i==4)
            LoadInternatString(IDS_TITLECOLUMNFILELISTDISKLOCATION,szTitle,sizeof(szTitle)-1);
        ListView_InsertColumn(hwndLV, i, &lvC);

        //if (iColSizeFileList[i]!=COLUMNWIDTH_UNKNOWN)
        //  ListView_SetColumnWidth(hwndLV,i,iColSizeFileList[i]);
    }

  return TRUE;
}

BOOL INSERTFILEUI::InitListViewFileToInsertListColumn(HWND hDlg)
{
    HWND hwndLV = GetDlgItem(hDlg,IDC_LISTSELFILES);
    return ::InitListViewControlFileToInsertListColumn(hwndLV);
}

BOOL INSERTFILEUI::AddOneFileInFileInsertList(LPCSTR lpszFileNameOnDisk,
                                                dfwcharpc dfFileNameToStore,
                                                dfwcharpc dfFileNamePrefixToStore,
                                                BOOL &fStop)
{
    ITEM_FILE_LIST ItemFileList;
    dfwcharp dfFileNameOnDisk;
    dfuLong32 i;
    dfuLong32 dfSizeAlloc = lstrlen(lpszFileNameOnDisk);
    STATIC_ARRAY* psaFilesInsert = pInsertFileUiParam->psaFilesInsert;


    dfFileNameOnDisk = (dfwcharp)DfsMalloc(sizeof(dfwchar) * (dfSizeAlloc+4) * 2);
    ConvertTCharToUnicode(lpszFileNameOnDisk,dfFileNameOnDisk,dfSizeAlloc+2);


    if (dfDfsCurrentFileName!=NULL)
    {
        if (CheckIfFileSameA(lpszFileNameOnDisk,dfDfsCurrentFileName))
        {
            DfsFree(dfFileNameOnDisk);
            return FALSE;
        }
    }

    ItemFileList.dfSize = GetFileSizeByName(dfFileNameOnDisk,&ItemFileList.DfsTm,NULL);
    if (dfFileNameToStore==NULL)
    {
        dfwcharpc dfFileNameToStoreBrowse;
        dfFileNameToStore = dfFileNameOnDisk;
        dfFileNameToStoreBrowse = dfFileNameToStore;
        while ((*dfFileNameToStoreBrowse)!=0)
        {
            dfwchar c = *dfFileNameToStoreBrowse ;
            if ((c=='\\') || (c=='/') || (c==':'))
                dfFileNameToStore = dfFileNameToStoreBrowse +1 ;
            dfFileNameToStoreBrowse++;
        }
    }
    ItemFileList.dfFileNameOnDisk = dfUnicodeCopyAlloc(dfFileNameOnDisk);
    ItemFileList.dfFileNameToStore = dfUnicodeCopyConcatAlloc(dfFileNamePrefixToStore,dfFileNameToStore);

    ItemFileList.pExtInfoCache = pExtInfoCache;
    DfsFree(dfFileNameOnDisk);

    for (i=0;i<psaFilesInsert->GetNbElemSA();i++)
    {
        const ITEM_FILE_LIST* pItemFileListCmp;
        pItemFileListCmp=(const ITEM_FILE_LIST*)psaFilesInsert -> GetElemPtrSA(i);
        if (dfUnicodeStrcmpi(pItemFileListCmp->dfFileNameToStore,ItemFileList.dfFileNameToStore)==0)
        {
            RemoveItemInsertList(i);
        }
    }

    {

        ITEM_FILE_LIST_SORTING ifls;
        ifls.dfItem = psaFilesInsert ->GetNbElemSA();
        ifls.psaFilesInsert = psaFilesInsert;
        psaFilesInsert -> SetElemSA(psaFilesInsert->GetNbElemSA(),&ItemFileList);
        psaFilesSorting->InsertSortedSA(&ifls);
    }


    return TRUE;
}

BOOL INSERTFILEUI::RefreshInsertList(HWND hWnd,int iColumnSort/*=-1*/)
{
    HWND hwndLV = GetDlgItem(hWnd,IDC_LISTSELFILES);
    STATIC_ARRAY* psaFilesInsert = pInsertFileUiParam->psaFilesInsert;
    //STATIC_ARRAY* psaFilesSorting = pInsertFileUiParam->psaFilesSorting;

    ListView_SetItemCount(hwndLV, 0);

    if (iColumnSort!=-1)
    {
        dfuLong32 i;
        psaFilesSorting->DeleteElemSA(0,psaFilesSorting->GetNbElemSA());
        switch(iColumnSort)
        {
        case 0:
            psaFilesSorting->SetFuncCompareDataSA(fncCompareItemFileListStoreName);
            break;

        case 1:
            psaFilesSorting->SetFuncCompareDataSA(fncCompareItemFileListExt);
            break;

        case 2:
            psaFilesSorting->SetFuncCompareDataSA(fncCompareItemFileListSize);
            break;

        case 3:
            psaFilesSorting->SetFuncCompareDataSA(fncCompareItemFileListDate);
            break;

        case 4:
            psaFilesSorting->SetFuncCompareDataSA(fncCompareItemFileListNameOnDisk);
            break;
        }

        for (i=0;i<psaFilesInsert->GetNbElemSA();i++)
        {
            ITEM_FILE_LIST_SORTING ifls;
            ifls.dfItem = i;
            ifls.psaFilesInsert = psaFilesInsert;
            psaFilesSorting ->InsertSortedSA(&ifls);
        }

        ShowSortColumnTitleBitmap(hwndLV,iColumnSort,TRUE,FALSE);
    }

    ListView_SetItemCount(hwndLV, psaFilesInsert->GetNbElemSA());
    return TRUE;
}

BOOL INSERTFILEUI::DoGetDispInfoInsertList(const NMHDR* pnmHdr)
{
    NMLVDISPINFO* lpVDISPINFO = (NMLVDISPINFO*)pnmHdr;
    dfuLong32 iItem = lpVDISPINFO->item.iItem;
    dfuLong32 iSubItem = lpVDISPINFO->item.iSubItem;

    STATIC_ARRAY* psaFilesInsert = pInsertFileUiParam->psaFilesInsert;
    //STATIC_ARRAY* psaFilesSorting = pInsertFileUiParam->psaFilesSorting;
    const ITEM_FILE_LIST_SORTING *pifls;
    const ITEM_FILE_LIST* pifl;

    pifls=(const ITEM_FILE_LIST_SORTING *)psaFilesSorting->GetElemPtrSA(iItem);
    pifl=(const ITEM_FILE_LIST *)psaFilesInsert->GetElemPtrSA(pifls->dfItem);

    if(lpVDISPINFO->item.mask & LVIF_IMAGE)
    {
        TCHAR szFileName[MAX_PATH];
        wsprintf(szFileName,"%ws",pifl->dfFileNameOnDisk);

        lpVDISPINFO->item.iImage =
                   pExtInfoCache->GetItemIndexImageCached(szFileName,FALSE);
    }


    if ((lpVDISPINFO->item.mask & LVIF_TEXT))
    {
        switch(iSubItem)
        {
        case 0:
            wsprintf(lpVDISPINFO->item.pszText,"%ws",pifl->dfFileNameToStore);
            break;

        case 1:
            {
                LPCTSTR lpszRegisteredType = "";
                char szFileName[MAX_PATH];
                wsprintf(szFileName,"%ws",pifl->dfFileNameOnDisk);
                lpszRegisteredType = pExtInfoCache->GetExtensionDescFromRegistryCached(szFileName,FALSE);

                if (lpszRegisteredType==NULL)
                    lpszRegisteredType="";

                wsprintf(lpVDISPINFO->item.pszText,"%s",lpszRegisteredType);
                break;
            }
        case 2:
            WinLong64ToStr(pifl->dfSize,lpVDISPINFO->item.pszText,MAX_PATH);
            break;

        case 3:
            BuildStrDate(&pifl->DfsTm,lpVDISPINFO->item.pszText,MAX_PATH);
            break;

        case 4:
            wsprintf(lpVDISPINFO->item.pszText,"%ws",pifl->dfFileNameOnDisk);
            break;

        case 5:
            wsprintf(lpVDISPINFO->item.pszText,"%ws",pifl->dfFileNameToStore);
            break;
        }
    }

    return TRUE;
}

BOOL INSERTFILEUI::RemoveItemInsertList(dfuLong32 dfListViewSortedIntemNumber)
{
    dfuLong32 i;
    STATIC_ARRAY* psaFilesInsert = pInsertFileUiParam->psaFilesInsert;
    //STATIC_ARRAY* psaFilesSorting = pInsertFileUiParam->psaFilesSorting;
    psaFilesInsert -> DeleteElemSA(dfListViewSortedIntemNumber,1);

    for (i=0;i<psaFilesSorting->GetNbElemSA();i++)
    {
        ITEM_FILE_LIST_SORTING* pifls = (ITEM_FILE_LIST_SORTING*)psaFilesSorting->GetElemPtrSA(i);
        if (pifls->dfItem == dfListViewSortedIntemNumber)
        {
            psaFilesSorting->DeleteElemSA(i);
            if (i == psaFilesSorting->GetNbElemSA())
                break;
            else
                pifls = (ITEM_FILE_LIST_SORTING*)psaFilesSorting->GetElemPtrSA(i);
        }

        if (pifls->dfItem > dfListViewSortedIntemNumber)
        {
            pifls->dfItem--;
        }
    }

    return TRUE;
}

BOOL INSERTFILEUI::RemoveAllItemInsertList(dfuLong32 dfListViewSortedIntemNumber)
{
    STATIC_ARRAY* psaFilesInsert = pInsertFileUiParam->psaFilesInsert;
    //STATIC_ARRAY* psaFilesSorting = pInsertFileUiParam->psaFilesSorting;
    psaFilesSorting->DeleteElemSA(0,psaFilesInsert->GetNbElemSA());
    psaFilesInsert->DeleteElemSA(0,psaFilesInsert->GetNbElemSA());

    return TRUE;
}

BOOL INSERTFILEUI::AddFiles(HWND hDlg)
{
TCHAR szFilter[MAX_PATH*2]="";
//TCHAR szFiles[MAX_PATH*128]="";
MYOPENFILENAME ofn;
BOOL fMultipleFileSelected;
BOOL fRet=TRUE;
BOOL fFirst=TRUE;
LPTSTR lpszFiles;
DWORD dwszFileSizeInTchar;

    dwszFileSizeInTchar = MAX_PATH*32;
    lpszFiles = (LPTSTR)DfsMalloc((dwszFileSizeInTchar+0x10) * sizeof(TCHAR));
    if (lpszFiles == NULL)
        return FALSE;
    *lpszFiles=0;

    InitOpenFileName((MYOPENFILENAME*)&ofn,hDlg,IDS_TYPEOPENALL,szFilter,sizeof(szFilter)-1,
                          lpszFiles,dwszFileSizeInTchar,NULL,0,0);
    ofn.Flags |= OFN_ALLOWMULTISELECT|OFN_EXPLORER;
    ofn.lpstrInitialDir = GetCurrentDirectory();
    if (!GetOpenFileName((OPENFILENAME*)&ofn))
    {
        DfsFree(lpszFiles);
        return FALSE;
    }

    fMultipleFileSelected = ((*(lpszFiles+lstrlen(lpszFiles)+1))!='\0');

    if (fMultipleFileSelected)
    {
        LPTSTR lpszOnlyPath = lpszFiles;
        DWORD dwLenPath = lstrlen(lpszFiles);
        LPTSTR lpszBrowseFile = lpszFiles+dwLenPath +1;
        TCHAR szFullFileName[MAX_PATH*2];
        memcpy(szFullFileName,lpszFiles,dwLenPath);
        szFullFileName[dwLenPath]= ('\\');
        lpszBrowseFile = lpszFiles+lstrlen(lpszFiles)+1;
        BOOL fStop=FALSE;


        while ((*lpszBrowseFile) != ('\0'))
        {
            BOOL fResult=FALSE;
            lstrcpy(szFullFileName+dwLenPath+1,lpszBrowseFile);

            //for directory see anything like AddDirFtaArray AddOneDirInFileInsertList
            if ((GetFileAttributes(szFullFileName) & FILE_ATTRIBUTE_DIRECTORY) == 0)
            {
                if (AddOneFileInFileInsertList(szFullFileName,NULL,NULL,fStop))
                    fResult=TRUE;
                else if (fStop)
                    fResult=FALSE;
            }
            else
            {
                if (AddOneDirInFileInsertList(szFullFileName,NULL,NULL,fStop))
                    fResult=TRUE;
                else if (fStop)
                    fResult=FALSE;
            }
            if (!fResult)
            {
                fRet=FALSE;
                break;
            }
            else
            {
                fFirst=FALSE;
            }

            lpszBrowseFile += lstrlen(lpszBrowseFile)+1;
        }
        if (fRet)
            SetCurrentDirectory(lpszFiles);
    }
    else
    {
        BOOL fStop;
        fRet=AddOneFileInFileInsertList(lpszFiles,NULL,NULL,fStop);
        if (fRet)
        {
            DWORD dwPosBuf = (lstrlen(lpszFiles) & 0xfffff0) + 0x20;
            LPTSTR lpszBuf = lpszFiles+dwPosBuf;
            LPTSTR lpszFilePart=NULL;
            if (dwszFileSizeInTchar>dwPosBuf)
            {
                GetFullPathName(lpszFiles,dwszFileSizeInTchar-dwPosBuf,lpszBuf,&lpszFilePart);
                *lpszFilePart=0;
                if (lpszFilePart>lpszBuf)
                {
                    LPTSTR lpszPrev=CharPrev(lpszBuf,lpszFilePart);
                    TCHAR c=*lpszPrev;
                    if (lpszPrev>lpszBuf)
                    {
                        LPTSTR lpszPrev2=CharPrev(lpszBuf,lpszPrev);
                        TCHAR c2=*lpszPrev2;
                        if ((c2!='\\') && (c2!='/') && (c2!=':') && (c=='\\'))
                            *lpszPrev=0;
                    }
                }

                SetCurrentDirectory(lpszBuf);
            }
        }
    }
    RefreshInsertList(hDlg);
    DfsFree(lpszFiles);
    return fRet;
}


BOOL INSERTFILEUI::AddDirectory(HWND hDlg)
{
  LPMALLOC pMalloc;
  BOOL     fRet = FALSE;
  TCHAR szPath[MAX_PATH+1];

  if(SUCCEEDED(::SHGetMalloc(&pMalloc)))
  {
    LPITEMIDLIST pidlBrowse;
    BROWSEINFO   bi;
    //TCHAR szText[MAX_PATH+1];


    //::LoadString(ghinst, IDS_FOLDER_BROWSE, szText, MAX_PATH);
    ::ZeroMemory(szPath, sizeof(szPath));
    ::ZeroMemory(&bi, sizeof(BROWSEINFO));
    //mFolderPath.GetText(szPath,MAX_PATH);
    GetDlgItemText(hDlg,IDC_EDITDIR,szPath,MAX_PATH);
    if (GetCurrentDirectory()!=NULL)
        if (lstrlen(GetCurrentDirectory())<MAX_PATH)
          lstrcpy(szPath,GetCurrentDirectory());

    bi.hwndOwner      = hDlg;
    bi.pidlRoot       = NULL;
    bi.pszDisplayName = szPath;
    bi.lpszTitle      = NULL;//szText;
    bi.ulFlags        = BIF_RETURNONLYFSDIRS | BIF_RETURNFSANCESTORS | BIF_USENEWUI | BIF_VALIDATE;
    bi.lpfn           = GDE_BrowseCallbackProc;//NULL;
    bi.lParam         = (LPARAM)szPath;
    bi.iImage         = 0;

    pidlBrowse = ::SHBrowseForFolder(&bi);
    if(pidlBrowse)
    {
      if(::SHGetPathFromIDList(pidlBrowse, szPath))
          fRet=TRUE;
    }
      // clean-up
    pMalloc->Free(pidlBrowse);
    pMalloc->Release();
  }

  if (fRet)
  {
      BOOL fStop;
      if (AddOneDirInFileInsertList(szPath,NULL,NULL,fStop))
      {
          SetCurrentDirectory(szPath);
      }

      RefreshInsertList(hDlg);
  }
  return fRet;
}



BOOL INSERTFILEUI::AddOneDirInFileInsertList(LPCSTR lpszFileNameOnDisk,
                                dfwcharpc dfFileNameToStore,
                                dfwcharpc dfFileNamePrefixToStore,
                                BOOL& fStop)
{
    //for directory see anything like AddDirFtaArray

    TCHAR szWilcard[MAX_PATH*2];
    TCHAR szWilcardBaseNextDir[MAX_PATH*2];
    TCHAR szDirectory[MAX_PATH*2];
    HANDLE hSearch;
    int iln;
    WIN32_FIND_DATA FileData;
    BOOL fFinished = FALSE;
    BOOL fRet = TRUE;
    DWORD dwTotal=0;
    BOOL fSubDir=TRUE;

    lstrcpy(szDirectory,lpszFileNameOnDisk);
    iln = lstrlen(szDirectory);
    if (GetLastCharOfString(szDirectory) != '\\')
//        lstrcat(szDirectory,"/");
        lstrcat(szDirectory,"\\");

    lstrcpy(szWilcard,szDirectory);
    lstrcpy(szWilcardBaseNextDir,szWilcard);
    lstrcat(szWilcard,"*.*");
    hSearch = FindFirstFile(szWilcard,&FileData);
    if (hSearch == INVALID_HANDLE_VALUE)
        return FALSE;


    while (!fFinished)
    {
        TCHAR szNewPath[MAX_PATH*2];
        dfwchar szwAddNewPrefix[(MAX_PATH*2)+8];
        dfwcharpc dfszNewPrefix;
        BOOL fNeedAssBackslash=FALSE;

        lstrcpy(szNewPath, szDirectory);
        lstrcat(szNewPath, FileData.cFileName);

        dfuLong32 j=dfUnicodeStrlen(dfFileNamePrefixToStore);
        if (j>0)
            if ((dfFileNamePrefixToStore[j-1]!='\\') && (dfFileNamePrefixToStore[j-1]!='/'))
                fNeedAssBackslash=TRUE;

        //szwAddNewPrefix[0]='\\';
        szwAddNewPrefix[0]='/';
        ConvertTCharToUnicode(FileData.cFileName,szwAddNewPrefix + (fNeedAssBackslash ? 1 : 0),MAX_PATH*2);

        dfszNewPrefix = dfUnicodeCopyConcatAlloc(dfFileNamePrefixToStore,szwAddNewPrefix);

        if ((FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0)
        {
            BOOL fStop;
            if (AddOneFileInFileInsertList(szNewPath,dfszNewPrefix,NULL,fStop))
                dwTotal += FileData.nFileSizeLow;
        }
        else if (fSubDir)
            if ((lstrcmp(FileData.cFileName,".")!=0) && (lstrcmp(FileData.cFileName,"..")!=0))
        {
            TCHAR szNewDirSrch[MAX_PATH];
            TCHAR szNewDirPrevious[MAX_PATH]="";

            lstrcpy(szNewDirSrch,szWilcardBaseNextDir);
            lstrcat(szNewDirSrch,FileData.cFileName);
            BOOL fStop=FALSE;

            /*
            if (lpszAddPreviousDir!=NULL)
            {
                lstrcpy(szNewDirPrevious,lpszAddPreviousDir);
                lstrcat(szNewDirPrevious,"\\");
            }
            lstrcat(szNewDirPrevious,FileData.cFileName);
            */

            if (!AddOneDirInFileInsertList(szNewPath,NULL,dfszNewPrefix,fStop))
            {
                if (fStop)
                {
                    fRet = FALSE;
                    break;
                }
            }
        }
        DfsFree((void*)dfszNewPrefix);

        if (!FindNextFile(hSearch, &FileData))
                fFinished = TRUE;
    }

    FindClose(hSearch);
    return fRet;
}




BOOL INSERTFILEUI::RemoveSelected(HWND hDlg,BOOL fRemoveAll)
{
    STATIC_ARRAY* psaFilesInsert = pInsertFileUiParam->psaFilesInsert;
    //STATIC_ARRAY* psaFilesSorting = pInsertFileUiParam->psaFilesSorting;
    HWND hwndLV = GetDlgItem(hDlg,IDC_LISTSELFILES);

    if (fRemoveAll)
    {
        psaFilesSorting->DeleteElemSA(0,psaFilesSorting->GetNbElemSA());
        psaFilesInsert->DeleteElemSA(0,psaFilesInsert->GetNbElemSA());
    }
    else
    {
        dfuLong32 i;
        i = psaFilesSorting ->GetNbElemSA();

        while (i>0)
        {
            i--;
            if (ListView_GetItemState(hwndLV, i, LVIS_SELECTED) != 0)
            {
                const ITEM_FILE_LIST_SORTING * pifls;
                pifls=(const ITEM_FILE_LIST_SORTING *)psaFilesSorting->GetElemPtrSA(i);
                RemoveItemInsertList(pifls->dfItem);
            }
        }
    }
   ListView_SetItemCount(hwndLV, 0);
   ListView_SetItemCount(hwndLV, psaFilesInsert->GetNbElemSA());
   return TRUE;
}

BOOL INSERTFILEUI::ProcessDialogInsertFileButtons(HWND hDlg,WORD wId)
{
    switch (wId)
    {
        case IDC_ADDDIR:
            return AddDirectory(hDlg);

        case IDC_ADDFILES:
            return AddFiles(hDlg);

        case IDC_REMOVEFILES:
            return RemoveSelected(hDlg);

        case IDC_CLEARLISTFILE:
            return RemoveSelected(hDlg,TRUE);
    }
    return FALSE;
}

LRESULT INSERTFILEUI::OnNotifyInsertListView(HWND hDlg,WPARAM idCtrl,LPNMHDR pnmHdr)
{
    if (pnmHdr->hwndFrom == GetDlgItem(hDlg,IDC_LISTSELFILES))
    {
        NM_LISTVIEW * pnmlv=(NM_LISTVIEW *)pnmHdr;
        if (pnmHdr->code == LVN_GETDISPINFO)
        DoGetDispInfoInsertList(pnmHdr);

        if (pnmHdr->code == LVN_COLUMNCLICK)
        {
            RefreshInsertList(hDlg,pnmlv->iSubItem);
        }
    }

    return 0;
}

/*************************************************************************************/

/*******************/

typedef struct
{
  BOOL fCreateNewDfs;
  TCHAR szFileList[MAX_PATH*16];
  TCHAR szDirectory[MAX_PATH];
  TCHAR szVersionName[MAX_PATH];
  TCHAR szVersionComment[MAX_PATH];
  BOOL  fIsDirectoryUsed;
  BOOL  fStoreFullContent;
  BOOL  fStoreAdditionnalFile;
  BOOL  fAddPreviousRemovedAsIdentical;
  BOOL  fFileToAddFileAlreadyFilled;
  LPCTSTR dfDfsFileName;

  FILETOADDARRAY ftaArray;
  dfuLong64 dfTotalFileSize;

  EXTINFOCACHE* pExtInfoCache;
  //GUIITEM* pGuiItem;
} ADDCONTENTDFSDLGPARAM;




BOOL InsertFileToAddCollection(ADDCONTENTDFSDLGPARAM& ADDCONTENTDFSDLGParam,FILETOADD* pFileToAdd,dfuLong32 dfNbToInsert)
{
    BOOL fRet=TRUE;
    FILETOADD* pFileToAddBrowse=pFileToAdd;
    dfuLong32 i;
    for (i=0;i<dfNbToInsert;i++,pFileToAddBrowse++)
    {

        FILETOADD fta=*pFileToAddBrowse;
        if (pFileToAddBrowse->filename_tostore != NULL)
            fta.filename_tostore = dfUnicodeCopyConcatAlloc(pFileToAddBrowse->filename_tostore,NULL);
        if (pFileToAddBrowse->filename_ondisk != NULL)
            fta.filename_ondisk = dfUnicodeCopyConcatAlloc(pFileToAddBrowse->filename_ondisk,NULL);
        if (pFileToAddBrowse->filename_prevversionondisk != NULL)
            fta.filename_prevversionondisk = dfUnicodeCopyConcatAlloc(pFileToAddBrowse->filename_prevversionondisk,NULL);


        ADDCONTENTDFSDLGParam.ftaArray.pFileToAdd = (FILETOADD *)
        AddArrayElem(ADDCONTENTDFSDLGParam.ftaArray.pFileToAdd,
                     &ADDCONTENTDFSDLGParam.ftaArray.dfNbFileToAdd,
                     &ADDCONTENTDFSDLGParam.ftaArray.dfNbFileToAddAllocated,
                     ADDCONTENTDFSDLGParam.ftaArray.dfFileToAddStepAlloc,
                     sizeof(FILETOADD), &fta, 1);
        if (ADDCONTENTDFSDLGParam.ftaArray.pFileToAdd==NULL)
        {
            fRet=FALSE;
            break;
        }
    }
    return fRet;
}

class ADDCONTENTDFSDLG
{
public:
    ADDCONTENTDFSDLG(HWND hDlg,LPARAM dwInitParam);
    ~ADDCONTENTDFSDLG();
    BOOL DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
private:
    //BOOL BrowseForFolderForEditDir(HWND hDlg);
    //BOOL DoOpeningSrc(HWND hDlg);
    //BOOL AddFileDfsDlgParam(LPCTSTR lpszFileName, LPCSTR lpszAddPreviousDir=NULL);
    //BOOL AddDirDfsDlgParam(LPCTSTR lpszFileName,DWORD &dwTotal,BOOL fSubDir=FALSE,LPCSTR lpszAddPreviousDir=NULL);

    ADDCONTENTDFSDLGPARAM* pADDCONTENTDFSDLGParam;
    RESIZABLEDLGHELP ResizableDlgHelp;

    INSERTFILEUI * pInsertFileUI;
    INSERTFILEUIPARAM InsertFileUiParam;
    STATIC_ARRAY* psaFilesInsert;
};



ADDCONTENTDFSDLG::ADDCONTENTDFSDLG(HWND hDlg,LPARAM dwInitParam)
{
    psaFilesInsert = NULL;
    pInsertFileUI = NULL;
    InsertFileUiParam.pExtInfoCache = NULL;
    InsertFileUiParam.psaFilesInsert = NULL;
}

ADDCONTENTDFSDLG::~ADDCONTENTDFSDLG()
{
    if (psaFilesInsert != NULL)
        delete(psaFilesInsert);
    if (pInsertFileUI != NULL)
        delete(pInsertFileUI);
}


/*
BOOL ADDCONTENTDFSDLG::BrowseForFolderForEditDir(HWND hDlg)
{
  LPMALLOC pMalloc;
  BOOL     fReturn = FALSE;

  if(SUCCEEDED(::SHGetMalloc(&pMalloc)))
  {
    LPITEMIDLIST pidlBrowse;
    BROWSEINFO   bi;
    //TCHAR szText[MAX_PATH+1];
    TCHAR szPath[MAX_PATH+1];

    //::LoadString(ghinst, IDS_FOLDER_BROWSE, szText, MAX_PATH);
    ::ZeroMemory(szPath, sizeof(szPath));
    ::ZeroMemory(&bi, sizeof(BROWSEINFO));
    //mFolderPath.GetText(szPath,MAX_PATH);
    GetDlgItemText(hDlg,IDC_EDITDIR,szPath,MAX_PATH);

    bi.hwndOwner      = hDlg;
    bi.pidlRoot       = NULL;
    bi.pszDisplayName = szPath;
    bi.lpszTitle      = NULL;//szText;
    bi.ulFlags        = BIF_RETURNONLYFSDIRS | BIF_RETURNFSANCESTORS | BIF_USENEWUI | BIF_VALIDATE;
    bi.lpfn           = GDE_BrowseCallbackProc;//NULL;
    bi.lParam         = (LPARAM)szPath;
    bi.iImage         = 0;

    pidlBrowse = ::SHBrowseForFolder(&bi);
    if(pidlBrowse)
    {
      if(::SHGetPathFromIDList(pidlBrowse, szPath))
          SetDlgItemText(hDlg,IDC_EDITDIR,szPath);
    }
      // clean-up
    pMalloc->Free(pidlBrowse);
    pMalloc->Release();
  }

  return TRUE;
}
*/
/*
BOOL ADDCONTENTDFSDLG::AddFileDfsDlgParam(LPCTSTR lpszFileName,LPCSTR lpszAddPreviousDir)
{
    return AddFileFtaArray(&(pADDCONTENTDFSDLGParam->ftaArray),lpszFileName,lpszAddPreviousDir);
}

*/

BOOL AddDirFtaArray(FILETOADDARRAY* pftaArray,LPCTSTR lpszFileName,DWORD &dwTotal,BOOL fSubDir,LPCSTR lpszAddPreviousDir)
{
    TCHAR szWilcard[MAX_PATH*2];
    TCHAR szWilcardBaseNextDir[MAX_PATH*2];
    TCHAR szDirectory[MAX_PATH*2];
    HANDLE hSearch;
    int iln;
    WIN32_FIND_DATA FileData;
    BOOL fFinished = FALSE;
    BOOL fRet = TRUE;


    lstrcpy(szDirectory,lpszFileName);
    iln = lstrlen(szDirectory);
    if (GetLastCharOfString(szDirectory) != '\\')
        lstrcat(szDirectory,"\\");

    lstrcpy(szWilcard,szDirectory);
    lstrcpy(szWilcardBaseNextDir,szWilcard);
    lstrcat(szWilcard,"*.*");
    hSearch = FindFirstFile(szWilcard,&FileData);
    if (hSearch == INVALID_HANDLE_VALUE)
        return FALSE;


    while (!fFinished)
    {
        TCHAR szNewPath[MAX_PATH*2];

        lstrcpy(szNewPath, szDirectory);
        lstrcat(szNewPath, FileData.cFileName);

        if ((FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0)
        {
            if (AddFileFtaArray(pftaArray,szNewPath,lpszAddPreviousDir))
                dwTotal += FileData.nFileSizeLow;
        }
        else if (fSubDir)
            if ((lstrcmp(FileData.cFileName,".")!=0) && (lstrcmp(FileData.cFileName,"..")!=0))
        {
            TCHAR szNewDirSrch[MAX_PATH];
            TCHAR szNewDirPrevious[MAX_PATH]="";

            lstrcpy(szNewDirSrch,szWilcardBaseNextDir);
            lstrcat(szNewDirSrch,FileData.cFileName);

            if (lpszAddPreviousDir!=NULL)
            {
                lstrcpy(szNewDirPrevious,lpszAddPreviousDir);
                lstrcat(szNewDirPrevious,"\\");
            }
            lstrcat(szNewDirPrevious,FileData.cFileName);

            #ifdef _DEBUG
            {
                char sz[2024];
                wsprintf(sz,"srch=%s Prev=%s, this=%s, attr=%x\n   szNewDirSrch=%s; szNewDirPrev=%s\n",
                    lpszFileName,
                    lpszAddPreviousDir ? lpszAddPreviousDir:"null",FileData.cFileName,FileData.dwFileAttributes,
                    szNewDirSrch,szNewDirPrevious);
                OutputDebugString(sz);
            }
            #endif

            if (!AddDirFtaArray(pftaArray,szNewDirSrch,dwTotal,fSubDir,szNewDirPrevious))
            {
                fRet = FALSE;
                break;
            }
        }

        if (!FindNextFile(hSearch, &FileData))
                fFinished = TRUE;
    }

    FindClose(hSearch);
    return fRet;
}



BOOL BuildFileToAddFromRealFile(FILETOADD &fta,LPCTSTR lpszFileName,LPCSTR lpszAddPreviousDir)
{
    TCHAR szFullPath[MAX_PATH];
    LPTSTR lpszOnlyFile=NULL;
    DfsClearStruct(&fta,0,sizeof(FILETOADD));
    GetFullPathName(lpszFileName,MAX_PATH,szFullPath,&lpszOnlyFile);

    fta.filename_ondisk = (dfwcharpc)DfsMalloc((lstrlen(szFullPath)+0x10)*sizeof(dfwchar));
    ConvertTCharToUnicode(szFullPath,(dfwcharp)fta.filename_ondisk,lstrlen(szFullPath)+0x8);

    fta.fIgnore = FALSE;
    fta.fForceDate = FALSE;
    fta.hAddTags = NULL;
    fta.pReserved = NULL;
    fta.fForceRecopyPrevious = FALSE;
    fta.hRamDifToFlushPatch = NULL;

    fta.fWritingRaw=FALSE;
    fta.dfFileStatusForRaw=0;

    if (lpszAddPreviousDir == NULL)
    {
      fta.filename_tostore = (dfwcharpc)DfsMalloc((lstrlen(lpszOnlyFile)+0x10)*sizeof(dfwchar));
      ConvertTCharToUnicode(lpszOnlyFile,(dfwcharp)fta.filename_tostore,lstrlen(lpszOnlyFile)+0x8);
    }
    else
    {
      dfuLong32 iln;
      fta.filename_tostore = (dfwcharpc)DfsMalloc((lstrlen(lpszAddPreviousDir)+lstrlen(lpszOnlyFile)+0x10)*sizeof(dfwchar));
      ConvertTCharToUnicode(lpszAddPreviousDir,(dfwcharp)fta.filename_tostore,lstrlen(lpszAddPreviousDir)+0x4);
      iln = dfUnicodeStrlen(fta.filename_tostore);
      *(((dfwcharp)fta.filename_tostore)+iln) = '/';
      ConvertTCharToUnicode(lpszOnlyFile,((dfwcharp)fta.filename_tostore)+iln+1,lstrlen(lpszOnlyFile)+0x4);
    }
    ConvertFileNameAndPath(fta.filename_tostore,NULL,0,FALSE);

    fta.filename_prevversionondisk = NULL;
    fta.dfPreviousVersionFilePosition = 0;
    return TRUE;
}


BOOL AddFileFtaArray(FILETOADDARRAY* pftaArray,LPCTSTR lpszFileName,LPCSTR lpszAddPreviousDir)
{
    FILETOADD fta;
    BuildFileToAddFromRealFile(fta,lpszFileName,lpszAddPreviousDir);

    return AddFtaToFtaArray(pftaArray,&fta,1);
}

/*
BOOL ADDCONTENTDFSDLG::AddDirDfsDlgParam(LPCTSTR lpszFileName,DWORD &dwTotal,BOOL fSubDir,LPCSTR lpszAddPreviousDir)
{
    return AddDirFtaArray(&pADDCONTENTDFSDLGParam->ftaArray, lpszFileName,dwTotal,fSubDir, lpszAddPreviousDir);
}

BOOL ADDCONTENTDFSDLG::DoOpeningSrc(HWND hDlg)
{
TCHAR szFilter[MAX_PATH*2]="";
TCHAR szFiles[MAX_PATH*128]="";
TCHAR szFilesDisp[MAX_PATH*128]="";
MYOPENFILENAME ofn;
BOOL fMultipleFileSelected;
BOOL fRet=TRUE;
BOOL fFirst=TRUE;

    InitOpenFileName((MYOPENFILENAME*)&ofn,hDlg,IDS_TYPEOPENALL,szFilter,sizeof(szFilter)-1,
                          szFiles,sizeof(szFiles)/sizeof(TCHAR),NULL,0,0);
    ofn.Flags |= OFN_ALLOWMULTISELECT|OFN_EXPLORER;
    if (!GetOpenFileName((OPENFILENAME*)&ofn))
        return FALSE;

    ClearFileToAddArrayWithDelete(&(pADDCONTENTDFSDLGParam->ftaArray),TRUE,FALSE);
    pADDCONTENTDFSDLGParam->fStoreAdditionnalFile = TRUE;
    pADDCONTENTDFSDLGParam->fAddPreviousRemovedAsIdentical = FALSE;

    fMultipleFileSelected = (szFiles[lstrlen(szFiles)+1]!='\0');

    if (fMultipleFileSelected)
    {
        LPTSTR lpszOnlyPath = szFiles;
        DWORD dwLenPath = lstrlen(szFiles);
        LPTSTR lpszBrowseFile = szFiles+dwLenPath +1;
        TCHAR szFullFileName[MAX_PATH*2];
        memcpy(szFullFileName,szFiles,dwLenPath);
        szFullFileName[dwLenPath]= ('\\');
        lpszBrowseFile = szFiles+lstrlen(szFiles)+1;
        while ((*lpszBrowseFile) != ('\0'))
        {
            lstrcpy(szFullFileName+dwLenPath+1,lpszBrowseFile);
            if (!AddFileDfsDlgParam(szFullFileName))
            {
                fRet=FALSE;
                break;
            }
            else
            {
                if (!fFirst)
                    lstrcat(szFilesDisp," ");
                lstrcat(szFilesDisp,lpszBrowseFile);
                fFirst=FALSE;
            }

            lpszBrowseFile += lstrlen(lpszBrowseFile)+1;
        }
    }
    else
    {
        fRet=AddFileDfsDlgParam(szFiles);
        lstrcpy(szFilesDisp,szFiles);
    }


    SetDlgItemText(hDlg,IDC_EDITFILELIST,szFilesDisp);
    return fRet;
}
*/
BOOL ADDCONTENTDFSDLG::DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
       switch (message) {
       case WM_INITDIALOG:
           {
            HWND hCtl;
            HWND hwndLVF;

            pADDCONTENTDFSDLGParam=(ADDCONTENTDFSDLGPARAM*)lParam;
            CheckRadioButton(hDlg,IDC_STORECONTENT,IDC_STORECRC,pADDCONTENTDFSDLGParam->fStoreFullContent ? IDC_STORECONTENT:IDC_STORECRC);
            CheckRadioButton(hDlg,IDC_ADDFILE,IDC_ADDDIR,pADDCONTENTDFSDLGParam->fIsDirectoryUsed ? IDC_ADDDIR:IDC_ADDFILE);
            SetDlgItemText(hDlg,IDC_EDITFILELIST,pADDCONTENTDFSDLGParam->szFileList);
            SetDlgItemText(hDlg,IDC_EDITDIR,pADDCONTENTDFSDLGParam->szDirectory);
            CheckDlgButton(hDlg,IDC_STOREALSONEW,pADDCONTENTDFSDLGParam->fStoreAdditionnalFile ? BST_CHECKED:BST_UNCHECKED);
            CheckDlgButton(hDlg,IDC_STOREONLYPRESENT,(!pADDCONTENTDFSDLGParam->fStoreAdditionnalFile) ? BST_CHECKED:BST_UNCHECKED);

            CheckDlgButton(hDlg,IDC_INCLUDESUBDIR,BST_CHECKED);
            hCtl = GetDlgItem(hDlg,IDC_ADDPREVIOUSFILEREMOVEDASIDENTICAL);
                if (hCtl != NULL)
                    CheckDlgButton(hDlg,IDC_ADDPREVIOUSFILEREMOVEDASIDENTICAL,(pADDCONTENTDFSDLGParam->fAddPreviousRemovedAsIdentical) ? BST_CHECKED:BST_UNCHECKED);

            EnableWindow(GetDlgItem(hDlg,IDC_EDITFILELIST),FALSE);

            hwndLVF = GetDlgItem(hDlg,IDC_LISTSELFILES);
            if (hwndLVF != NULL)
            {
                pInsertFileUI = new INSERTFILEUI ;
                InsertFileUiParam.pExtInfoCache=  pADDCONTENTDFSDLGParam->pExtInfoCache;
                //InsertFileUiParam.psaFilesInsert=NULL;

                InsertFileUiParam.psaFilesInsert = new STATIC_ARRAY(sizeof(ITEM_FILE_LIST));
                InsertFileUiParam.psaFilesInsert->SetFuncDestructorSA(fncDestructorItemFileList);
                InsertFileUiParam.psaFilesInsert->SetFuncCompareDataSA(fncCompareItemFileList);
                //delete(InsertFileUiParam.psaFilesInsert); at end

                pInsertFileUI->SetInfoCache(InsertFileUiParam.pExtInfoCache);
                pInsertFileUI->SetFileUiParam(&InsertFileUiParam) ;
                pInsertFileUI->InitListViewFileToInsertListColumn(hDlg);

                pInsertFileUI->SetCurrentDirectory(pADDCONTENTDFSDLGParam->szDirectory);
                if (pADDCONTENTDFSDLGParam->dfDfsFileName != NULL)
                    pInsertFileUI->SetDfsCurrentFileName(pADDCONTENTDFSDLGParam->dfDfsFileName);
            }


              ResizableDlgHelp.Init(hDlg);
              ResizableDlgHelp.InitRatio(0x1000);

              if (hwndLVF != NULL)
              {
                  const ITEMCTLINFO ItemCtlInfo[] =
                  {
                    {IDC_STATICVERSIONNAME,0x0000,0x0000,0x1000,0x000,FALSE},
                    {IDC_EDITVERSIONNAME,0x0000,0x0000,0x1000,0x000,FALSE},
                    {IDC_STATICSEPARATOR1,0x0000,0x0000,0x1000,0x000,FALSE},
                    {IDC_STATICFILELIST,0x0000,0x0000,0x1000,0x000,FALSE},

                    {IDC_LISTSELFILES,0x0000,0x0000,0x1000,0x0a00,FALSE},
                    {IDC_ADDFILES,0x0800,0x0a00,0x0000,0x000,TRUE},
                    {IDC_ADDDIR,0x0800,0x0a00,0x0000,0x000,TRUE},
                    {IDC_REMOVEFILES,0x0800,0x0a00,0x0000,0x000,TRUE},
                    {IDC_CLEARLISTFILE,0x0800,0x0a00,0x0000,0x000,TRUE},

                    {IDC_STATICSEPARATOR2,0x0000,0x0a00,0x1000,0x000,FALSE},

                    {IDC_STATICSTORAGEFIRSTVERSION,0x0000,0x0a00,0x1000,0x000,TRUE},
                    {IDC_STORECONTENT,0x0000,0x0a00,0x1000,0x000,TRUE},
                    {IDC_STORECRC,0x0000,0x0a00,0x1000,0x000,TRUE},

                    {IDC_ADDPREVIOUSFILEREMOVEDASIDENTICAL,0x0000,0x0a00,0x1000,0x000,TRUE},
                    {IDC_STOREONLYPRESENT,0x0000,0x0a00,0x1000,0x000,TRUE},
                    {IDC_STOREALSONEW,0x0000,0x0a00,0x1000,0x000,TRUE},


                    {IDC_STATICSEPARATOR3,0x0000,0x0a00,0x1000,0x000,FALSE},
                    {IDC_STATICCOMMENT,0x0000,0x0a00,0x1000,0x000,TRUE},
                    {IDC_VERSIONPROPCOMMENT,0x0000,0x0a00,0x1000,0x600,FALSE},

                    {IDOK,0x1000,0x1000,0x0000,0x000,TRUE},
                    {IDCANCEL,0x1000,0x1000,0x0000,0x000,TRUE},
                    {IDC_GETHELP,0x1000,0x1000,0x0000,0x000,TRUE},

                    {0,0,0,0,0,FALSE}};

                    ResizableDlgHelp.InitCtlList(ItemCtlInfo);
              }
              else
              {
                  const ITEMCTLINFO ItemCtlInfo[] =
                  {
                    {IDC_GROUPFIRSTVER,0x0000,0x0000,0x1000,0x0000,FALSE},
                    {IDC_EDITVERSIONNAME,0x0000,0x0000,0x1000,0x0000,FALSE},
                    {IDC_GROUPPREF,0x0000,0x0000,0x1000,0x0000,FALSE},

                    {IDC_GROUPCOMMENT,0x0000,0x0000,0x1000,0x1000,FALSE},
                    {IDC_VERSIONPROPCOMMENT,0x0000,0x0000,0x1000,0x1000,FALSE},

                    {IDOK,0x1000,0x1000,0x0000,0x000,TRUE},
                    {IDCANCEL,0x1000,0x1000,0x0000,0x000,FALSE},
                    {IDC_GETHELP,0x1000,0x1000,0x0000,0x000,FALSE},
                    {0,0,0,0,0,FALSE}};

                    ResizableDlgHelp.InitCtlList(ItemCtlInfo);
              }

            return TRUE;
           }


      case WM_NOTIFY:
          {
              NMHDR* pnmHdr = (NMHDR*)lParam;
              if (pnmHdr->hwndFrom == GetDlgItem(hDlg,IDC_LISTSELFILES))
              {
                  if (pInsertFileUI!=NULL)
                    return (BOOL)pInsertFileUI->OnNotifyInsertListView(hDlg,wParam,pnmHdr);
              }
              break;
          }

        case WM_GETMINMAXINFO:
        {
            SIZE sz;
            MINMAXINFO* pMinMaxInfo = (MINMAXINFO*)lParam;
            ResizableDlgHelp.GetOriginalDialogSize(&sz);
            pMinMaxInfo->ptMinTrackSize.x=sz.cx;
            pMinMaxInfo->ptMinTrackSize.y=sz.cy;
            return 0;
        }

        case WM_SIZE:
            //SendMessage(hDlg,WM_SETREDRAW,FALSE,0);
            ResizableDlgHelp.OnResize();
            //SendMessage(hDlg,WM_SETREDRAW,TRUE,0);
            //InvalidateRect(hDlg,NULL,FALSE);
            return TRUE;

        case WM_COMMAND:    ///IDC_WEBSITELINK
            {
                WORD wId = GET_WM_COMMAND_ID(wParam,lParam);

                switch (wId)
                {

                    case IDC_ADDDIR:
                    case IDC_ADDFILES:
                    case IDC_REMOVEFILES:
                    case IDC_CLEARLISTFILE:
                        if (pInsertFileUI!=NULL)
                          pInsertFileUI->ProcessDialogInsertFileButtons(hDlg,wId);
                        break;

                    case IDC_GETHELP:
                        DoHelpContents(hDlg,
                                       pADDCONTENTDFSDLGParam->fCreateNewDfs  ?
                                          "::/Starting_with_an_Empty_Patch.htm" :
                                          "::/Adding_New_Versions_to_a_Patch.htm");
                        return TRUE;

                    case IDOK:
                    case IDCANCEL:
                    {
                        BOOL fRet=(wId==IDOK);
                        if (fRet)
                        {
                            BOOL fSubfolderChecked;
                            dfuLong64 dwTotal=0;
                            HWND hCtl;
                            BOOL fIsStoreContentBtn = GetDlgItem(hDlg,IDC_STORECONTENT)!=NULL;
                            pADDCONTENTDFSDLGParam->fStoreFullContent = IsDlgButtonChecked(hDlg,IDC_STORECONTENT);
                            pADDCONTENTDFSDLGParam->fIsDirectoryUsed = IsDlgButtonChecked(hDlg,IDC_ADDDIR);
                            pADDCONTENTDFSDLGParam->fStoreAdditionnalFile = IsDlgButtonChecked(hDlg,IDC_STOREALSONEW);

                            //GetDlgItemText(hDlg,IDC_EDITFILELIST,pADDCONTENTDFSDLGParam->szFileList,sizeof(pADDCONTENTDFSDLGParam->szFileList)/sizeof(TCHAR));
                            //GetDlgItemText(hDlg,IDC_EDITDIR,pADDCONTENTDFSDLGParam->szDirectory,sizeof(pADDCONTENTDFSDLGParam->szDirectory)/sizeof(TCHAR));
                            GetDlgItemText(hDlg,IDC_EDITVERSIONNAME,pADDCONTENTDFSDLGParam->szVersionName,sizeof(pADDCONTENTDFSDLGParam->szVersionName)/sizeof(TCHAR));
                            GetDlgItemText(hDlg,IDC_VERSIONPROPCOMMENT,pADDCONTENTDFSDLGParam->szVersionComment,sizeof(pADDCONTENTDFSDLGParam->szVersionComment)/sizeof(TCHAR));

                            hCtl = GetDlgItem(hDlg,IDC_ADDPREVIOUSFILEREMOVEDASIDENTICAL);
                                if (hCtl != NULL)
                                    pADDCONTENTDFSDLGParam->fAddPreviousRemovedAsIdentical = IsDlgButtonChecked(hDlg,IDC_ADDPREVIOUSFILEREMOVEDASIDENTICAL) == BST_CHECKED;

                            fSubfolderChecked=IsDlgButtonChecked(hDlg,IDC_INCLUDESUBDIR)==BST_CHECKED;

                            if ((InsertFileUiParam.psaFilesInsert != NULL) && (pInsertFileUI!=NULL))
                            {
                                dfuLong32 i;
                                STATIC_ARRAY* psaFileInsert=InsertFileUiParam.psaFilesInsert;
                                for (i=0;i<psaFileInsert->GetNbElemSA();i++)
                                {
                                    const ITEM_FILE_LIST* pItem =
                                          (const ITEM_FILE_LIST*)psaFileInsert->GetElemPtrSA(i);
                                    FILETOADD fta;
                                    DfsClearStruct(&fta,0,sizeof(FILETOADD));

                                    fta.filename_ondisk = dfUnicodeCopyAlloc(pItem->dfFileNameOnDisk);
                                    fta.filename_tostore = dfUnicodeCopyAlloc(pItem->dfFileNameToStore);

                                    fta.fIgnore = FALSE;
                                    fta.fForceDate = FALSE;
                                    fta.hAddTags = NULL;
                                    fta.pReserved = NULL;
                                    fta.fForceRecopyPrevious = FALSE;

                                    fta.fWritingRaw = FALSE;
                                    fta.dfFileStatusForRaw = 0;

                                    fta.hRamDifToFlushPatch = NULL;


                                    fta.filename_prevversionondisk = NULL;
                                    fta.dfPreviousVersionFilePosition = 0;


                                    if (AddFtaToFtaArray(&pADDCONTENTDFSDLGParam->ftaArray,&fta,1))
                                        dwTotal += pItem->dfSize;
                                }
                                pADDCONTENTDFSDLGParam->dfTotalFileSize = dwTotal;
                            }
                            if (InsertFileUiParam.psaFilesInsert != NULL)
                                delete(InsertFileUiParam.psaFilesInsert);
                            InsertFileUiParam.psaFilesInsert=NULL;


                            if (pInsertFileUI!=NULL)
                              if (pInsertFileUI->GetCurrentDirectory() != NULL)
                                //if ((!pADDCONTENTDFSDLGParam->fStoreFullContent) && fIsStoreContentBtn)
                                  if (lstrlen(pInsertFileUI->GetCurrentDirectory()) < ((sizeof(pADDCONTENTDFSDLGParam->szDirectory)/sizeof(TCHAR))-1))
                                    lstrcpy(pADDCONTENTDFSDLGParam->szDirectory,pInsertFileUI->GetCurrentDirectory());

/*
                            if ((pADDCONTENTDFSDLGParam->fIsDirectoryUsed) && (!pADDCONTENTDFSDLGParam->fFileToAddFileAlreadyFilled))
                            {
                                //TCHAR szDir[MAX_PATH];
                                AddDirDfsDlgParam(pADDCONTENTDFSDLGParam->szDirectory,dwTotal,fSubfolderChecked);
                                pADDCONTENTDFSDLGParam->dfTotalFileSize = dwTotal;
                            }
*/
                        }
                        EndDialog(hDlg, fRet);
                        return (TRUE);
                    }

                    case IDC_STORECONTENT:
                    case IDC_STORECRC:
                        CheckRadioButton(hDlg,IDC_STORECONTENT,IDC_STORECRC,wId);
                        return TRUE;

                    case IDC_STOREALSONEW:
                    case IDC_STOREONLYPRESENT:
                        CheckRadioButton(hDlg,IDC_STOREONLYPRESENT,IDC_STOREALSONEW,wId);
                        break;
/*
                    case IDC_ADDFILE:
                    case IDC_ADDDIR:
                        CheckRadioButton(hDlg,IDC_ADDFILE,IDC_ADDDIR,wId);
                        return TRUE;

                    case IDC_BROWSE_FILELIST:
                        SendMessage(hDlg,WM_COMMAND,
                            GET_WM_COMMAND_MPS (IDC_ADDFILE,hDlg,0));
                        return DoOpeningSrc(hDlg);

                    case IDC_BROWSE_DIR:
                        SendMessage(hDlg,WM_COMMAND,
                            GET_WM_COMMAND_MPS (IDC_ADDDIR,hDlg,0));
                        return BrowseForFolderForEditDir(hDlg);
                        */
                }

            }
            break;
    }
    return (FALSE);                           /* Didn't process a message    */
}

void fillCompressionParamForGuiItem(COMPRESSIONPARAM & CompressionParam, const GUIITEM &guiItem)
{
	CompressionParam = guiItem.compressionParam;
	dfuLong32 uZlibCompressRatio = guiItem.compressionParam.uZlibCompressRatio;


	CompressionParam.uZlibCompressRatio = uZlibCompressRatio;
}

BOOL CALLBACK AddContentDfsProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   BOOL fRet=FALSE;
    if (message==WM_INITDIALOG)
    {
      ADDCONTENTDFSDLG* pADDCONTENTDFSDLG = new ADDCONTENTDFSDLG(hwnd, lParam);
      if(!pADDCONTENTDFSDLG)
        return FALSE;
      MySetWindowLongPtr(hwnd,DWLP_USER,(DWORD_PTR)pADDCONTENTDFSDLG);
    }

    ADDCONTENTDFSDLG* pADDCONTENTDFSDLG = (ADDCONTENTDFSDLG*)MyGetWindowLongPtr(hwnd,DWLP_USER);
    if (pADDCONTENTDFSDLG!=NULL)
        fRet = pADDCONTENTDFSDLG -> DlgProc(hwnd,message,wParam,lParam);

    if (message==WM_DESTROY)
    {
        delete pADDCONTENTDFSDLG ;
        MySetWindowLongPtr(hwnd,DWLP_USER,0);
    }
    return fRet;
}




BOOL DoNewDfs(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,LRUMENU &lrum,FILETOADD* pFileToAdd,dfuLong32 dfNbToInsert)
{
    BOOL fRet=FALSE;
    ADDCONTENTDFSDLGPARAM ADDCONTENTDFSDLGParam;
    TCHAR szFileNameDfs[MAX_PATH]="";
    H_ERROR_INFO hei=NULL;

    ADDCONTENTDFSDLGParam.fIsDirectoryUsed=TRUE;
    ADDCONTENTDFSDLGParam.fStoreFullContent=TRUE;
    ADDCONTENTDFSDLGParam.szDirectory[0]='\0';
    ADDCONTENTDFSDLGParam.szFileList[0]='\0';
    ADDCONTENTDFSDLGParam.fCreateNewDfs = TRUE;
    ADDCONTENTDFSDLGParam.dfDfsFileName = NULL;
    ADDCONTENTDFSDLGParam.fFileToAddFileAlreadyFilled=dfNbToInsert>0;
    ADDCONTENTDFSDLGParam.pExtInfoCache = &guiItem.ExtInfoCache;
    //ADDCONTENTDFSDLGParam.pGuiItem = &guiItem;


    ClearFileToAddArrayWithDelete(&(ADDCONTENTDFSDLGParam.ftaArray),FALSE,FALSE);
    ADDCONTENTDFSDLGParam.fStoreAdditionnalFile = TRUE;
    ADDCONTENTDFSDLGParam.fAddPreviousRemovedAsIdentical = FALSE;

    InsertFileToAddCollection(ADDCONTENTDFSDLGParam,pFileToAdd,dfNbToInsert);

    GetCurrentDirectory(sizeof(ADDCONTENTDFSDLGParam.szDirectory)/sizeof(TCHAR),ADDCONTENTDFSDLGParam.szDirectory);

    if (guiItem.szDefaultDirAddVersion[0] != 0)
      lstrcpy(ADDCONTENTDFSDLGParam.szDirectory,guiItem.szDefaultDirAddVersion);

    fRet = (BOOL)(DialogBoxParam(ghInstRes,
                  (dfNbToInsert==0) ? MAKEINTRESOURCE(IDD_NEWVERSION) : MAKEINTRESOURCE(IDD_NEWVERSION_NOFILESELINFO),
                  guiItem.GetHwndMain(), (DLGPROC)AddContentDfsProc,(LPARAM)&ADDCONTENTDFSDLGParam));
    if (fRet && (ADDCONTENTDFSDLGParam.ftaArray.dfNbFileToAdd==0))
    {
        TCHAR szErrMsg[MAX_PATH] = "";
        LoadString(ghInstRes,IDS_ERRORNOFILETOADD,szErrMsg,sizeof(szErrMsg)/sizeof(TCHAR));
        MessageBox(guiItem.hwndMain,szErrMsg,NULL,MB_ICONERROR|MB_OK);
        fRet = FALSE;
    }
    if (fRet)
        lstrcpy(guiItem.szDefaultDirAddVersion,ADDCONTENTDFSDLGParam.szDirectory);


    if (fRet)
    {
        TCHAR szFilter[MAX_PATH*2]="";
        MYOPENFILENAME ofn;

        InitOpenFileName((MYOPENFILENAME*)&ofn,guiItem.GetHwndMain(),IDS_TYPEOPENDFS,szFilter,sizeof(szFilter)-1,
                              szFileNameDfs,MAX_PATH,NULL,0,0);
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
        ofn.lpstrDefExt="svf";
        if (!GetSaveFileName((OPENFILENAME*)&ofn))
            fRet= FALSE;

        if (fRet)
        {
            DFSFILE DfsFile = NULL;
            DFSFILEINFOPARAM DfsFileParam;
            dfwchar dfwDfsFileName[MAX_PATH];

            guiItem.InstallProgressBar(100);

            DfsFileParam.sizeStruct = sizeof(DfsFileParam);
            DfsFileParam.dfStatus = DFS_NEWFILE | DFS_WRITABLE;
            ConvertTCharToUnicode(szFileNameDfs,dfwDfsFileName,MAX_PATH);

            DfsFileParam.filename = dfwDfsFileName;

            DfsFileOpen(&DfsFileParam, &DfsFile, NULL);
            if (DfsFile == NULL)
            {
                  //printf("Can't open %ws\n", szDfsFileName);
                  fRet=FALSE;
            }
            else
            {
                dfuLong32 dfErr;
                PROGRESSCBPARAM pcp;
                dfwcharp dfwVersionName=NULL;
                dfuLong32 dfSizeVersionName;
                dfwcharp dfwVersionComment=NULL;
                dfuLong32 dfSizeVersionComment;
                DFSFEATUREPARAM DfsFeatureParam;

                pcp.dwMinProgress = 000;
                pcp.dwMaxProgress = 100;
                pcp.pGuiItem = &guiItem;



                DfsFeatureParam.fComputeMd5 = guiItem.fMd5;
                DfsFeatureParam.fComputeSha1 = guiItem.fSha1;
				DfsFeatureParam.fComputeSha256 = guiItem.fSha256;
                DfsFeatureParam.fStripIdenticalBody = guiItem.pfStripIdentical;
                SetDfsFeatureParam(DfsFile,&DfsFeatureParam);


                dfSizeVersionName = lstrlen(ADDCONTENTDFSDLGParam.szVersionName) + 0x10;
                dfwVersionName = (dfwcharp)DfsMalloc((dfSizeVersionName)*sizeof(dfwchar));
                if (dfwVersionName != NULL)
                    ConvertTCharToUnicode(ADDCONTENTDFSDLGParam.szVersionName,dfwVersionName,dfSizeVersionName);


                dfSizeVersionComment = lstrlen(ADDCONTENTDFSDLGParam.szVersionComment) + 0x10;
                dfwVersionComment = (dfwcharp)DfsMalloc((dfSizeVersionComment)*sizeof(dfwchar));
                if (dfwVersionComment != NULL)
                    ConvertTCharToUnicode(ADDCONTENTDFSDLGParam.szVersionComment,dfwVersionComment,dfSizeVersionComment);

#ifdef _DEBUG
                {
                    dfuLong32 i;
                    for (i=0;i<ADDCONTENTDFSDLGParam.ftaArray.dfNbFileToAdd;i++)
                    {
                        TCHAR sz[MAX_PATH*2];
                        wsprintf(sz,"%ws\n",(ADDCONTENTDFSDLGParam.ftaArray.pFileToAdd+i)->filename_tostore);
                        OutputDebugString(sz);
                    }
                }
#endif

	            COMPRESSIONPARAM CompressionParamAdapted;
	            fillCompressionParamForGuiItem(CompressionParamAdapted, guiItem);

                dfErr = InsertDirectoryinDfsFile(DfsFile,
                                       ADDCONTENTDFSDLGParam.fStoreFullContent ?
                                           TYPEDIR_FILEINSERTING_DEFLATE :
                                           TYPEDIR_FILECRCONLY,
                                       ADDCONTENTDFSDLGParam.ftaArray.dfNbFileToAdd,
                                       ADDCONTENTDFSDLGParam.ftaArray.pFileToAdd,
                                       FALSE,
                                       dfwVersionName,  //ADDCONTENTDFSDLGParam.szVersionName
                                       dfwVersionComment,
                                       &CompressionParamAdapted,/**/
                                       ProgressCallBackCreatePatching, &pcp,NULL);

                if (dfwVersionName != NULL)
                    DfsFree(dfwVersionName);

                if (dfwVersionComment != NULL)
                    DfsFree(dfwVersionComment);

                if (dfErr != DFS_SUCCESS)
                {
                    fRet=FALSE;
                    //printf("error if writing %ws\n",szDfsFileName);
                }
            }
            DfsClose(DfsFile,NULL);
            guiItem.RemoveProgressBar();

            if (!fRet)
            {
                TCHAR szErrMsg[MAX_PATH] = "";
                LoadString(ghInstRes,IDS_ERRORWRITING,szErrMsg,sizeof(szErrMsg)/sizeof(TCHAR));
                MessageBox(guiItem.hwndMain,szErrMsg,NULL,MB_ICONERROR|MB_OK);
                fRet = FALSE;
            }
        }

    }

    ClearFileToAddArrayWithDelete(&(ADDCONTENTDFSDLGParam.ftaArray),TRUE,FALSE);

    if (fRet)
    {
        DoLoadDfs(guiItem,DfsFileAndInfo,lrum,szFileNameDfs,TRUE,ADDCONTENTDFSDLGParam.szDirectory);
    }


    RefreshGrayingMenu(guiItem,DfsFileAndInfo);

    FreeErrorInfoBlock(hei);
    return fRet;
}


/****************************************************************/

BOOL DoAddingPreviousRemovedAsIdentical(PDIRINFO pCurDirInfo,
                                        FILESET* pfsDest,
                                        ADDCONTENTDFSDLGPARAM* pADDCONTENTDFSDLGParam)
{
    dfuLong32 j;
    dfuLong32 dfPreviousNbFileToAdd=pADDCONTENTDFSDLGParam->ftaArray.dfNbFileToAdd;

    for (j=0;j<pfsDest->dfNbFileItem;j++)
    {
        dfuLong32 i;
        BOOL fFound=FALSE;
        dfwcharpc dfwExtrFileName=((pfsDest->pFileItem)+j)->FileNameOnArchive;
        for (i=0;i<dfPreviousNbFileToAdd;i++)
        {
            dfwcharpc dfwFta=((pADDCONTENTDFSDLGParam->ftaArray.pFileToAdd)+i)->filename_tostore;

            if (dfUnicodeStrcmpi(dfwExtrFileName,dfwFta)==0)
            {
                fFound=TRUE;
                break;
            }
        }

        if (!fFound)
        {
            FILETOADD fta;
            dfvoidp TagBuf;
            dfuLong32 TagSize;
            DfsClearStruct(&fta,0,sizeof(fta));
            if (((pCurDirInfo->pFileInDirInfo)+j)->fCrc32Filled)
            {
                fta.dfForceRecopyOrRawCopyCrc32 = ((pCurDirInfo->pFileInDirInfo)+j)->dfCrc32;
                fta.fForceRecopyOrRawCopyMd5Present = ((pCurDirInfo->pFileInDirInfo)+j)->fMd5Filled;
                if (fta.fForceRecopyOrRawCopyMd5Present)
                    DfsMemcpy(fta.bMd5,((pCurDirInfo->pFileInDirInfo)+j)->bMd5,16);
                fta.fForceRecopyOrRawCopySha1Present = ((pCurDirInfo->pFileInDirInfo)+j)->fSha1Filled;
                if (fta.fForceRecopyOrRawCopySha1Present)
                    DfsMemcpy(fta.bSha1,((pCurDirInfo->pFileInDirInfo)+j)->bSha1,20);
            }
            else
            {
                fta.dfForceRecopyOrRawCopyCrc32 = 0;
                fta.fForceRecopyOrRawCopyMd5Present = FALSE;
                fta.fForceRecopyOrRawCopySha1Present = FALSE;
            }
            fta.dfForceRecopyOrRawCopySize = ((pCurDirInfo->pFileInDirInfo)+j)->dfSize;
            fta.dfPreviousVersionFilePosition = j;

            if (GetTag(*(pCurDirInfo->TagFile + j), DFSTAG_DATE, &TagBuf, &TagSize))
            {
                fta.dfsInfoDate = *((DFSINFODATE *) TagBuf);
                fta.fForceDate = TRUE;
            }
            else
                fta.fForceDate = FALSE;

            fta.hAddTags = NULL;
            fta.fForceRecopyPrevious = TRUE;
            fta.fWritingRaw = FALSE;
            fta.dfFileStatusForRaw = 0;

            fta.hRamDifToFlushPatch = NULL;

            fta.fIgnore = FALSE;
            fta.filename_ondisk = NULL;
            fta.filename_prevversionondisk = NULL;
            fta.filename_tostore = dfUnicodeCopyConcatAlloc(((pCurDirInfo->pFileInDirInfo)+j)->FileName,NULL);
            fta.pReserved = NULL;

            pADDCONTENTDFSDLGParam->ftaArray.pFileToAdd = (FILETOADD *)
            AddArrayElem(pADDCONTENTDFSDLGParam->ftaArray.pFileToAdd,
                        &pADDCONTENTDFSDLGParam->ftaArray.dfNbFileToAdd,
                        &pADDCONTENTDFSDLGParam->ftaArray.dfNbFileToAddAllocated,
                        pADDCONTENTDFSDLGParam->ftaArray.dfFileToAddStepAlloc,
                        sizeof(FILETOADD), &fta, 1);
        }
    }

    return TRUE;
}


BOOL DoAppendNewVersionBuildToList(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo)
{
    DFSFILE DfsFile = DfsFileAndInfo.DfsFile;
    BOOL fRet=TRUE;
    BOOL fReload=FALSE;
    {
        dfuLong32 dfNewNbDir;
        DfsGetNbDir(DfsFile, &dfNewNbDir,NULL);

        if (dfNewNbDir > DfsFileAndInfo.dfNbDir)
        {
            PDIRINFO* pNewDirInfo = (PDIRINFO*)DfsRealloc(DfsFileAndInfo.pDirInfo,sizeof(PDIRINFO)*(dfNewNbDir+1));
            if (pNewDirInfo != NULL)
            {
                dfuLong32 dfNumDir;
                DfsFileAndInfo.pDirInfo = pNewDirInfo;
                for (dfNumDir=DfsFileAndInfo.dfNbDir;dfNumDir<dfNewNbDir;dfNumDir++)
                    * ((DfsFileAndInfo.pDirInfo)+dfNumDir) = NULL;

                for (dfNumDir=DfsFileAndInfo.dfNbDir;dfNumDir<dfNewNbDir;dfNumDir++)
                {
                    dfuLong32 dfError = DFS_SUCCESS;
                    dfError = ReadDirectoryInfo(DfsFile, dfNumDir, (DfsFileAndInfo.pDirInfo)+dfNumDir, NULL, NULL,NULL);
                    if (dfError != DFS_SUCCESS)
                    {
                        fRet = FALSE;
                        fReload=TRUE;
                        break;
                    }
                    if (dfNumDir > 0)
                    {
                        PDIRINFO pDirInfoPrev=*((DfsFileAndInfo.pDirInfo)+dfNumDir-1);
                        PDIRINFO pDirInfoCur=*((DfsFileAndInfo.pDirInfo)+dfNumDir);

                        FixIndenticalDifferentSizeInReadDirectoryInfo(pDirInfoPrev,pDirInfoCur);
                    }
                }
                if (!fReload)
                {
                  DfsFileAndInfo.dfNbDir = dfNewNbDir;
                  guiItem.FillTreeView(DfsFileAndInfo);
                }
            }
            else
                fReload=TRUE;
        }
    }

    if (fReload)
    {
        LPTSTR lpszCopyFileName = (LPTSTR)DfsMalloc(lstrlen(DfsFileAndInfo.lpszDfsFileName)+4);
        lstrcpy(lpszCopyFileName,DfsFileAndInfo.lpszDfsFileName);
        fRet = RefreshDfs(DfsFileAndInfo.DfsFile,guiItem,DfsFileAndInfo,lpszCopyFileName);
        DfsFree(lpszCopyFileName);
        guiItem.FillTreeView(DfsFileAndInfo);
    }
    return fRet;
}


BOOL DoProcessInsertingVersion(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,
                               ADDCONTENTDFSDLGPARAM &ADDCONTENTDFSDLGParam,
                               LRUMENU &lrum,
                               BOOL fAddPreviousRemovedAsIdentical,
                               H_ERROR_INFO &hei)
{
    BOOL fRet=TRUE;
    TCHAR szFileNameDfs[MAX_PATH]="";

    if (fRet)
    {
        guiItem.InstallProgressBar(200);
        {
            FILESET* pfsDest=NULL;
            TCHAR szPathTemp[(MAX_PATH+1)*2]="";
            dfuLong32 i;

            GetTempPath(sizeof(szPathTemp)/sizeof(TCHAR),szPathTemp);

            guiItem.SetProgressPos(0);

            fRet = DoExtracting(guiItem.hwndMain,guiItem,0,100,
                          DfsFileAndInfo.DfsFile,
                          szPathTemp,&pfsDest,TRUE,
                          DfsFileAndInfo.dfNbDir-1,DfsFileAndInfo.pDirInfo,
                          DfsFileAndInfo.fBaseDirectorySelected,DfsFileAndInfo.dfBaseDirNum,
                          DfsFileAndInfo.lpBaseDirectory,
                          0,NULL,TRUE,FALSE,&hei);

            #if defined(_DEBUG) && defined(MSGTEST)
            if (!fRet)
                MessageBox(guiItem.hwndMain,fRet?"OK":"BAD","Extracted",MB_OK);
            #endif
            //guiItem.SetProgressPos(2);


            /* now : search match old <> new */


            /* +++---***$$$###5 */
            if (fRet)
            {
                dfuLong32 dfNbOldName=pfsDest->dfNbFileItem;
                dfuLong32 dfNbNewName=ADDCONTENTDFSDLGParam.ftaArray.dfNbFileToAdd;
                dfuLong32 i,j;
                dfuLong32 dfNbFileIdentical=0;
                dfwcharpc* pOldNameArray=(dfwcharpc*)DfsMalloc(sizeof(dfwcharpc)*(dfNbOldName+1));
                dfwcharpc* pNewNameArray=(dfwcharpc*)DfsMalloc(sizeof(dfwcharpc)*(dfNbNewName+1));
                dfuLong32* pOldNameToNewNameLink=(dfuLong32*)DfsMalloc(sizeof(dfuLong32)*(dfNbNewName+1));
                BOOL fAddManualIdentical=FALSE;

                for (i=0;i<dfNbNewName;i++)
                    *(pNewNameArray+i) = ((ADDCONTENTDFSDLGParam.ftaArray.pFileToAdd)+i)->filename_tostore;

                for (i=0;i<dfNbOldName;i++)
                    *(pOldNameArray+i) = ((pfsDest->pFileItem)+i)->FileNameOnArchive;

                for (i=0;i<dfNbNewName;i++)
                    *(pOldNameToNewNameLink+i) = NO_LINK_VALUE;

                if (!BuildLinkFileIdentical(pOldNameArray,dfNbOldName,
                                            pNewNameArray,dfNbNewName,
                                            pOldNameToNewNameLink,
                                            &dfNbFileIdentical))
                        fRet=FALSE;

                if (fRet)
                  if ((dfNbFileIdentical<dfNbOldName) && (dfNbFileIdentical<dfNbNewName))
                  {
                      fRet = EditLinkRenamedFile(guiItem.GetHwndMain(),
                                                 pOldNameArray,dfNbOldName,
                                                 pNewNameArray,dfNbNewName,
                                                 pOldNameToNewNameLink);
                      fAddManualIdentical = fRet;
                  }

                if (fRet)
                  for (i=0;i<dfNbNewName;i++)
                    {
                        j = *(pOldNameToNewNameLink+i);

                        if (j!=NO_LINK_VALUE)
                            {

                                ((ADDCONTENTDFSDLGParam.ftaArray.pFileToAdd)+i)->dfPreviousVersionFilePosition = j;
                                ((ADDCONTENTDFSDLGParam.ftaArray.pFileToAdd)+i)->filename_prevversionondisk =
                                    dfUnicodeCopyConcatAlloc(((pfsDest->pFileItem)+j)->FileNameOnDisk,NULL);


                                #ifdef _DEBUG
                                if (!fAddManualIdentical)
                                {
                                    dfwcharpc dfwFta=((ADDCONTENTDFSDLGParam.ftaArray.pFileToAdd)+i)->filename_tostore;
                                    dfwcharpc dfwExtrFileName=((pfsDest->pFileItem)+j)->FileNameOnArchive;

                                    if (dfUnicodeStrcmpi(dfwFta,
                                                        dfwExtrFileName)!=0)
                                    {
                                        TCHAR szTxt[(MAX_PATH*3)];
                                        wsprintf(szTxt,"'%ws' cmp '%ws' = %d\n",
                                                        dfwFta,dfwExtrFileName,
                                                        dfUnicodeStrcmpi(dfwFta,dfwExtrFileName));

                                        MessageBox(0,szTxt,"big error 2",MB_OK);
                                    }
                                }
                                #endif
                            }
                    }


                DfsFree(pOldNameArray);
                DfsFree(pNewNameArray);
                DfsFree(pOldNameToNewNameLink);
            }
            /* +++---***$$$###5 */

            if (fRet)
              if (!ADDCONTENTDFSDLGParam.fStoreAdditionnalFile)
                for (i=0;i<ADDCONTENTDFSDLGParam.ftaArray.dfNbFileToAdd;i++)
                {
                    if (((ADDCONTENTDFSDLGParam.ftaArray.pFileToAdd)+i)->filename_prevversionondisk == NULL)
                    {
                        ((ADDCONTENTDFSDLGParam.ftaArray.pFileToAdd)+i)->fIgnore = TRUE;
                    }
                }

            if (fRet && fAddPreviousRemovedAsIdentical)
                fRet = DoAddingPreviousRemovedAsIdentical((((*(DfsFileAndInfo.pDirInfo+DfsFileAndInfo.dfNbDir-1)))),
                                                          pfsDest,&ADDCONTENTDFSDLGParam);

            if (fRet)
            {
                DFSFILE DfsFile = DfsFileAndInfo.DfsFile;

                {
                    dfuLong32 dfErr;
                    PROGRESSCBPARAM pcp;
                    dfwcharp dfwVersionName=NULL;
                    dfwcharp dfwVersionComment=NULL;
                    dfuLong32 dfSizeVersionName;
                    dfuLong32 dfSizeVersionComment;
                    pcp.dwMinProgress = 100;
                    pcp.dwMaxProgress = 200;
                    pcp.pGuiItem = &guiItem;


                    dfSizeVersionName = lstrlen(ADDCONTENTDFSDLGParam.szVersionName) + 0x10;
                    dfwVersionName = (dfwcharp)DfsMalloc((dfSizeVersionName)*sizeof(dfwchar));
                    if (dfwVersionName != NULL)
                        ConvertTCharToUnicode(ADDCONTENTDFSDLGParam.szVersionName,dfwVersionName,dfSizeVersionName);


                    dfSizeVersionComment = lstrlen(ADDCONTENTDFSDLGParam.szVersionComment) + 0x10;
                    dfwVersionComment = (dfwcharp)DfsMalloc((dfSizeVersionComment)*sizeof(dfwchar));
                    if (dfwVersionComment != NULL)
                        ConvertTCharToUnicode(ADDCONTENTDFSDLGParam.szVersionComment,dfwVersionComment,dfSizeVersionComment);


	                COMPRESSIONPARAM CompressionParamAdapted;
	                fillCompressionParamForGuiItem(CompressionParamAdapted, guiItem);


                    dfErr = InsertDirectoryinDfsFile(DfsFile,
                                           TYPEDIR_PATCHFROMPREVIOUS,
                                           ADDCONTENTDFSDLGParam.ftaArray.dfNbFileToAdd,
                                           ADDCONTENTDFSDLGParam.ftaArray.pFileToAdd,
                                           FALSE,
                                           dfwVersionName,  //ADDCONTENTDFSDLGParam.szVersionName
                                           dfwVersionComment,
                                           &CompressionParamAdapted,
                                           ProgressCallBackCreatePatching, &pcp,&hei);

                    if (dfwVersionName != NULL)
                        DfsFree(dfwVersionName);

                    if (dfwVersionComment != NULL)
                        DfsFree(dfwVersionComment);

                    if (dfErr != DFS_SUCCESS)
                    {
                        fRet=FALSE;
                        //printf("error if writing %ws\n",szDfsFileName);
                    }
                }

            }

            if (pfsDest!=NULL)
            {
              FreeFileSet(pfsDest,TRUE);
              DfsFree(pfsDest);
              pfsDest = NULL;
            }
        }

        DoAppendNewVersionBuildToList(guiItem,DfsFileAndInfo);
/*
        if (fRet)
        {
            LPTSTR lpszCopyFileName = (LPTSTR)DfsMalloc(lstrlen(DfsFileAndInfo.lpszDfsFileName)+4);
            lstrcpy(lpszCopyFileName,DfsFileAndInfo.lpszDfsFileName);
            fRet = RefreshDfs(DfsFileAndInfo.DfsFile,guiItem,DfsFileAndInfo,lpszCopyFileName);
            DfsFree(lpszCopyFileName);
            guiItem.FillTreeView(DfsFileAndInfo);
        }
*/
        guiItem.RemoveProgressBar();

        if (!fRet)
        {
            TCHAR szErrMsg[MAX_PATH] = "";
            LoadString(ghInstRes,IDS_ERRORWRITING,szErrMsg,sizeof(szErrMsg)/sizeof(TCHAR));
            MessageBox(guiItem.hwndMain,szErrMsg,NULL,MB_ICONERROR|MB_OK);
            fRet = FALSE;
        }
    }

    ClearFileToAddArrayWithDelete(&(ADDCONTENTDFSDLGParam.ftaArray),TRUE,FALSE);

    return fRet;
}


BOOL DoInsertVersion(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,LRUMENU &lrum,FILETOADD* pFileToAdd,dfuLong32 dfNbToInsert)
{
    BOOL fRet=FALSE;
    BOOL fCancelDlg ;
    ADDCONTENTDFSDLGPARAM ADDCONTENTDFSDLGParam;
    TCHAR szFileNameDfs[MAX_PATH]="";

    if (DfsFileAndInfo.DfsFile == NULL)
        return FALSE;

    H_ERROR_INFO hei=NULL;

    ADDCONTENTDFSDLGParam.fIsDirectoryUsed=TRUE;
    ADDCONTENTDFSDLGParam.fStoreFullContent=TRUE;
    ADDCONTENTDFSDLGParam.szDirectory[0]='\0';
    ADDCONTENTDFSDLGParam.szFileList[0]='\0';
    ADDCONTENTDFSDLGParam.fCreateNewDfs = FALSE;
    ADDCONTENTDFSDLGParam.dfDfsFileName = DfsFileAndInfo.lpszDfsFileName;
    ADDCONTENTDFSDLGParam.fFileToAddFileAlreadyFilled=dfNbToInsert>0;
    ADDCONTENTDFSDLGParam.pExtInfoCache = &guiItem.ExtInfoCache;
    //ADDCONTENTDFSDLGParam.pGuiItem = &guiItem;

    ClearFileToAddArrayWithDelete(&(ADDCONTENTDFSDLGParam.ftaArray),FALSE,FALSE);
    ADDCONTENTDFSDLGParam.fStoreAdditionnalFile = TRUE;
    ADDCONTENTDFSDLGParam.fAddPreviousRemovedAsIdentical = FALSE;

    InsertFileToAddCollection(ADDCONTENTDFSDLGParam,pFileToAdd,dfNbToInsert);
    GetCurrentDirectory(sizeof(ADDCONTENTDFSDLGParam.szDirectory),ADDCONTENTDFSDLGParam.szDirectory);

    if (guiItem.szDefaultDirAddVersion[0] != 0)
      lstrcpy(ADDCONTENTDFSDLGParam.szDirectory,guiItem.szDefaultDirAddVersion);

    fRet = (BOOL)(DialogBoxParam(ghInstRes,
                        (dfNbToInsert==0) ? MAKEINTRESOURCE(IDD_ADDVERSION) : MAKEINTRESOURCE(IDD_ADDVERSION_NOFILESELINFO),
            guiItem.GetHwndMain(), (DLGPROC)AddContentDfsProc,(LPARAM)&ADDCONTENTDFSDLGParam));
    fCancelDlg = !fRet;

    if (fRet && (ADDCONTENTDFSDLGParam.ftaArray.dfNbFileToAdd==0))
    {
        TCHAR szErrMsg[MAX_PATH] = "";
        LoadString(ghInstRes,IDS_ERRORNOFILETOADD,szErrMsg,sizeof(szErrMsg)/sizeof(TCHAR));
        MessageBox(guiItem.hwndMain,szErrMsg,NULL,MB_ICONERROR|MB_OK);
        fRet = FALSE;
        fCancelDlg=TRUE;
    }

    if (fRet)
        lstrcpy(guiItem.szDefaultDirAddVersion,ADDCONTENTDFSDLGParam.szDirectory);


    if (fRet)
        fRet = DoProcessInsertingVersion(guiItem,DfsFileAndInfo,
                                         ADDCONTENTDFSDLGParam,lrum,
                                         ADDCONTENTDFSDLGParam.fAddPreviousRemovedAsIdentical,hei);
    DisplayErrorMessage(guiItem.hwndMain,IDS_ERROR_INSERTING,hei,(!fRet) && (!fCancelDlg));
    FreeErrorInfoBlock(hei);

    return fRet;
}


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

typedef struct
{
  BOOL fStoreFullContent;
  BOOL DoLoadNewDfsIfCurrentlyCreated;
  BOOL fRebuildCompression;
} GENERATESUBDFSDLGPARAM;

BOOL CALLBACK GenerateSubDfsDlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
    GENERATESUBDFSDLGPARAM* pgsdfsdp;
    if (message==WM_INITDIALOG)
        MySetWindowLongPtr(hDlg,DWLP_USER,lParam);
    pgsdfsdp=(GENERATESUBDFSDLGPARAM*)MyGetWindowLongPtr(hDlg,DWLP_USER);
    switch (message) {


    case WM_INITDIALOG:
        {
           CheckRadioButton(hDlg,IDC_STORECONTENT,IDC_STORECRC,pgsdfsdp->fStoreFullContent ? IDC_STORECONTENT:IDC_STORECRC);
           CheckDlgButton(hDlg,IDC_LOADNEWDFS,pgsdfsdp->DoLoadNewDfsIfCurrentlyCreated ? BST_CHECKED:BST_UNCHECKED);
           CheckDlgButton(hDlg,IDC_PERFORMNEWCOMPRESSION,pgsdfsdp->fRebuildCompression ? BST_CHECKED:BST_UNCHECKED);
        break;
        }

    case WM_COMMAND:    ///IDC_WEBSITELINK
            {
                WORD wId = GET_WM_COMMAND_ID(wParam,lParam);

                if (wId == IDC_GETHELP)
                {
                    DoHelpContents(hDlg,//*pADDCONTENTDFSDLGParam->pGuiItem,
                                   //"::/Building_Partial_Patches.htm");
                                   "::/Distributing_Version_Subsets.htm");
                    return TRUE;
                }

                if ((wId==IDC_STORECONTENT)|| (wId==IDC_STORECRC))
                {
                        CheckRadioButton(hDlg,IDC_STORECONTENT,IDC_STORECRC,wId);
                }

                if (wId == IDOK)
                {
                    pgsdfsdp->fStoreFullContent = IsDlgButtonChecked(hDlg,IDC_STORECONTENT);
                    pgsdfsdp->DoLoadNewDfsIfCurrentlyCreated =IsDlgButtonChecked(hDlg,IDC_LOADNEWDFS);
                    pgsdfsdp->fRebuildCompression = IsDlgButtonChecked(hDlg,IDC_PERFORMNEWCOMPRESSION);
                }

                if ((wId == IDOK) || (wId == IDCANCEL))
                {
                    EndDialog(hDlg, (wId == IDOK));
                    return (TRUE);
                }
            }
            break;
    }
    return (FALSE);                           /* Didn't process a message    */
}

BOOL DoRemixForReplaceCurrentDls(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,LRUMENU &lrum,
                                 dfuLong32 dfNbVersionRemix,const  VERSIONTOADD_REMIX *pVersionRemix,
                                 dfuLong32 dfVersionSelected,dfuLong32 * pdfError,H_ERROR_INFO & hei);

BOOL DoDeleteVersion(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,LRUMENU &lrum)
{
    dfuLong32 dfNbVersionRemix;
    VERSIONTOADD_REMIX *pVersionRemix;
    dfuLong32 i, j,dfError;
    dfuLong32 dfVersionSelected=0;
    BOOL fRet=TRUE;
    H_ERROR_INFO hei=NULL;
    dfuLong32 dwNbMapVersionMap ;
    BOOL*lpVersionMap ;
    dfuLong32 dfCountSelVersion=0;

    dwNbMapVersionMap = DfsFileAndInfo.dfNbDir;


    lpVersionMap = (BOOL*)DfsMalloc((dwNbMapVersionMap+1)*sizeof(BOOL));

    for (i=0;i<dwNbMapVersionMap;i++)
        *(lpVersionMap+i) = TRUE;

    dfuLong32 dfTreeViewSelected=DfsFileAndInfo.dfCurDir;
    HTREEITEM hti;

    hti = TreeView_GetSelection(guiItem.hwndTreeView);
    if (hti!=NULL)
    {
        TVITEM tv;
        tv.mask=TVIF_PARAM;
        tv.hItem = hti;
        tv.lParam = 0xffffffff;

        if (TreeView_GetItem(guiItem.hwndTreeView, &tv))
            dfTreeViewSelected = (dfuLong32)tv.lParam ;
    }

    if ((((DWORD)dfTreeViewSelected) == TVITEMPARAM_ROOT) || (dfTreeViewSelected>=dwNbMapVersionMap))
    {
        for (i=0;i<dwNbMapVersionMap;i++)
            *(lpVersionMap+i)=ListView_GetItemState(guiItem.hwndLV,i,LVIS_SELECTED) == 0;
    }
    else
        *(lpVersionMap+dfTreeViewSelected)=FALSE;

    for (i=0;i<dwNbMapVersionMap;i++)
        if (!(*(lpVersionMap+i)))
            dfCountSelVersion++;

    if (dfCountSelVersion==0)
    {
        DfsFree(lpVersionMap);
        return FALSE;
    }


    {
        TCHAR szFormat[MAX_PATH+2];
        TCHAR szMsg[MAX_PATH+2];
        TCHAR szCaption[MAX_PATH+2];
        int iMsg = IDNO;
        LoadString(ghInstRes,IDS_MESSAGECONFIRMDELVERSION,szFormat,MAX_PATH);
        LoadString(ghInstRes,IDS_TITLECONFIRMDELVERSION,szCaption,MAX_PATH);
        wsprintf(szMsg,szFormat,dfCountSelVersion);
        iMsg = MessageBox(guiItem.GetHwndMain(),szMsg,szCaption,MB_ICONQUESTION|MB_YESNO);
        if (iMsg != IDYES)
        {
            DfsFree(lpVersionMap);
            return FALSE;
        }
    }



    pVersionRemix = (VERSIONTOADD_REMIX*)DfsMalloc(sizeof(VERSIONTOADD_REMIX) * (dwNbMapVersionMap + 1));
    dfNbVersionRemix = 0;
    for (i = 0; i < dwNbMapVersionMap; i++)
        if (*(lpVersionMap + i))
        {
            VERSIONTOADD_REMIX *pFileCopyInfoCur = pVersionRemix + dfNbVersionRemix;
            PDIRINFO pDirOrg = *(DfsFileAndInfo.pDirInfo + i);
            pFileCopyInfoCur->dfNumVersionPreviousSvf = i;
            pFileCopyInfoCur->dfNbPreviousFileInMask = pDirOrg->dfNbFile;
            pFileCopyInfoCur->dfNbFileToAdd = 0;
            pFileCopyInfoCur->pFileCopyInfo = (FILETOCOPYINFO_REMIX*)
                DfsMalloc(sizeof(FILETOCOPYINFO_REMIX) * (pDirOrg->dfNbFile + 1));
            pFileCopyInfoCur->pfta = NULL;
            for (j = 0; j < pDirOrg->dfNbFile; j++)
            {
                FILETOCOPYINFO_REMIX *pftci = (pFileCopyInfoCur->pFileCopyInfo) + j;
                pftci->dfReferenceItem = FTCI_REFERENCE_UNMODIFIED;
                pftci->fIsReferenceInAddedFile = FALSE;
            }
            dfNbVersionRemix++;
        }

    if (fRet)
        fRet = DoRemixForReplaceCurrentDls(guiItem, DfsFileAndInfo, lrum,
                                            dfNbVersionRemix, pVersionRemix,
                                            dfVersionSelected,&dfError,hei);

    DfsFree(lpVersionMap);
//    guiItem.RemoveProgressBar();
/*
    for (i = 0; i < dfNbVersionRemix; i++)
    {
        FILETOCOPYINFO_REMIX *pFileCopyInfo = (pVersionRemix + i)->pFileCopyInfo;
        DfsFree(pFileCopyInfo);
    }
    DfsFree(pVersionRemix);
*/
    FreeVersionToAdd(pVersionRemix,dfNbVersionRemix);

    DisplayErrorMessage(guiItem.hwndMain,IDS_ERROR_DELETING,hei,!((dfError == DFS_SUCCESS) && fRet));
    FreeErrorInfoBlock(hei);

    return (dfError == DFS_SUCCESS) && fRet;
}

BOOL DoGenerateSubDfsUserSelected(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,LRUMENU &lrum)
{
    GENERATESUBDFSDLGPARAM gsdfsdp;
    BOOL fRet;
    TCHAR szFilter[MAX_PATH*2]="";
    MYOPENFILENAME ofn;
    TCHAR szFileNameDfs[MAX_PATH]="";
    BOOL fCancelDlg=FALSE;


    if (DfsFileAndInfo.pDirInfo==NULL)
        return FALSE;
    gsdfsdp.fRebuildCompression = FALSE;
    gsdfsdp.fStoreFullContent = ((*DfsFileAndInfo.pDirInfo)->dfTypeDir != TYPEDIR_FILECRCONLY);
    gsdfsdp.DoLoadNewDfsIfCurrentlyCreated=TRUE;
    fRet = (BOOL)DialogBoxParam(ghInstRes, MAKEINTRESOURCE(IDD_BUILDSUBVERSION),
        guiItem.GetHwndMain() , (DLGPROC)GenerateSubDfsDlgProc,(LPARAM)(&gsdfsdp));
    if (!fRet)
        return FALSE;

    H_ERROR_INFO hei=NULL;


    InitOpenFileName((MYOPENFILENAME*)&ofn,guiItem.GetHwndMain(),IDS_TYPEOPENDFS,szFilter,sizeof(szFilter)-1,
                          szFileNameDfs,MAX_PATH,NULL,0,0);
    ofn.lpstrDefExt="svf";
    #ifdef _DEBUG
    lstrcpy(szFileNameDfs,"g:\\gil\\tstsubdfs.svf");
    #else
    if (!GetSaveFileName((OPENFILENAME*)&ofn))
    {
        fRet= FALSE;
        fCancelDlg=TRUE;
    }
    #endif

    if (fRet)
    {
        dfwchar dfwNewDfsFileName[MAX_PATH];
        dfwchar wchBaseDirectory[1024+(MAX_PATH*2)];
        dfuLong32 dwNbMapVersionMap;
        BOOL* lpVersionMap;
        dfuLong32 i;
		BOOL fReuseOldPatch, fRawAccepted;
        DFSFEATUREPARAM DfsFeatureParam;
        DfsFeatureParam.fComputeMd5 = guiItem.fMd5;
        DfsFeatureParam.fComputeSha1 = guiItem.fSha1;
		DfsFeatureParam.fComputeSha256 = guiItem.fSha256;
        DfsFeatureParam.fStripIdenticalBody = guiItem.pfStripIdentical;


        ConvertTCharToUnicode(szFileNameDfs,dfwNewDfsFileName,MAX_PATH);
        wchBaseDirectory[0]='\0';
        if (DfsFileAndInfo.fBaseDirectorySelected)
            ConvertTCharToUnicode(DfsFileAndInfo.lpBaseDirectory,(dfwcharp)wchBaseDirectory,
                           MAX_PATH);
        dwNbMapVersionMap = DfsFileAndInfo.dfNbDir;
        lpVersionMap = (BOOL*)DfsMalloc((dwNbMapVersionMap+1)*sizeof(BOOL));
        for (i=0;i<dwNbMapVersionMap;i++)
            *(lpVersionMap+i)=ListView_GetItemState(guiItem.hwndLV,i,LVIS_SELECTED) != 0;


        guiItem.InstallProgressBar(2000);
        guiItem.SetProgressPos(0);
        FILESET* pFileSetBase=NULL;

        #if defined(_DEBUG) || defined(WIN32)

	    COMPRESSIONPARAM CompressionParamAdapted;
	    fillCompressionParamForGuiItem(CompressionParamAdapted, guiItem);

/*
        DoReMixDfsEx(DfsFileAndInfo.DfsFile,DfsFileAndInfo.dfNbDir,DfsFileAndInfo.pDirInfo,
                      NULL,dfwNewDfsFileName, // BOOL fZipFile,
                      guiItem.pfStripIdentical,
                      DfsFileAndInfo.fBaseDirectorySelected,
                      DfsFileAndInfo.dfBaseDirNum,
                      wchBaseDirectory,pFileSetBase,
                      dwNbMapVersionMap,
                      NULL, // VERSIONADD_REMIX
                      //lpVersionMap,
                      !gsdfsdp.fStoreFullContent,
                      FALSE,FALSE, //future
                      &CompressionParamAdapted,
                      SetPosCallBack,&guiItem,0,2000,

                      //tSetExtractPosCallBack pSetExtractPosCallBack,dfvoidp dfUserPtr,
                      //dfuLong32 dwMinProgress,dfuLong32 dwMaxProgress
                      &hei
                     );*/
        fReuseOldPatch =! gsdfsdp.fRebuildCompression;
		fRawAccepted = !gsdfsdp.fRebuildCompression;
        fRet = DoGenerateSubDfsEmulEx(DfsFileAndInfo.DfsFile,/*DfsFileAndInfo.dfNbDir,*/DfsFileAndInfo.pDirInfo,
                      NULL,dfwNewDfsFileName, /*BOOL fZipFile,*/
                      &DfsFeatureParam,
                      DfsFileAndInfo.fBaseDirectorySelected,
                      DfsFileAndInfo.dfBaseDirNum,
                      wchBaseDirectory,pFileSetBase,
                      dwNbMapVersionMap,
                      lpVersionMap,
					  !gsdfsdp.fStoreFullContent, fReuseOldPatch, fRawAccepted,
                      &CompressionParamAdapted,
                      SetPosCallBack,&guiItem,0,2000,&hei
                      /*
                      tSetExtractPosCallBack pSetExtractPosCallBack,dfvoidp dfUserPtr,
                      dfuLong32 dwMinProgress,dfuLong32 dwMaxProgress*/);
        #else
#error (oldstuf)
        fRet = DoGenerateSubDfs(DfsFileAndInfo.DfsFile,/*DfsFileAndInfo.dfNbDir,*/DfsFileAndInfo.pDirInfo,
                      NULL,dfwNewDfsFileName, /*BOOL fZipFile,*/
                      guiItem.pfStripIdentical,
                      DfsFileAndInfo.fBaseDirectorySelected,
                      DfsFileAndInfo.dfBaseDirNum,
                      wchBaseDirectory,pFileSetBase,
                      dwNbMapVersionMap,
                      lpVersionMap,
                      !gsdfsdp.fStoreFullContent,FALSE/* future*/,
                      &CompressionParamAdapted,
                      SetPosCallBack,&guiItem,0,2000,&hei
                      /*
                      tSetExtractPosCallBack pSetExtractPosCallBack,dfvoidp dfUserPtr,
                      dfuLong32 dwMinProgress,dfuLong32 dwMaxProgress*/);
        #endif
        DfsFree(lpVersionMap);
        guiItem.RemoveProgressBar();

/*
        if ((!fRet) && (hei==NULL))
        {
            TCHAR szErrMsg[MAX_PATH] = "";
            LoadString(ghInstRes,IDS_ERRORWRITING,szErrMsg,sizeof(szErrMsg)/sizeof(TCHAR));
            MessageBox(guiItem.hwndMain,szErrMsg,NULL,MB_ICONERROR|MB_OK);
            fRet = FALSE;
            fCancelDlg=TRUE;
        }
        */
    }


    if (fRet && gsdfsdp.DoLoadNewDfsIfCurrentlyCreated)
    {
        DoLoadDfs(guiItem,DfsFileAndInfo,lrum,szFileNameDfs,TRUE,NULL);
    }


    RefreshGrayingMenu(guiItem,DfsFileAndInfo);

    DisplayErrorMessage(guiItem.hwndMain,IDS_ERROR_WRITING,hei,(!fRet) && (!fCancelDlg));
    FreeErrorInfoBlock(hei);
    return fRet;
}


BOOL DoAppendSvfToSvf(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,LRUMENU &lrum)
{
    BOOL fRet = FALSE;
    dfuLong32 dfError = DFS_SUCCESS;
    dfuLong32 i;

    dfuLong32 dfNbDirWhereAdd=0;
    dfuLong32 dfNbDirWhereRead=0;
    PDIRINFO *pDirInfoWhereAdd = NULL;
    PDIRINFO *pDirInfoDfsFileWhereRead = NULL;

    DFSFILE DfsFileWhereRead = NULL;
    DFSFILEINFOPARAM DfsFileParamWhereRead;
    DFSFILE DfsFile;
    H_ERROR_INFO hei=NULL;

    DfsFileParamWhereRead.sizeStruct = sizeof(DFSFILEINFOPARAM);
    DfsFileParamWhereRead.dfStatus = DFS_READABLE;

    DfsFile = DfsFileAndInfo.DfsFile;


    {
        TCHAR szOpenFileName[MAX_PATH+0x20];

        szOpenFileName[0]=0;
        {
        TCHAR szFilter[MAX_PATH*2]="";
        MYOPENFILENAME ofn;

            InitOpenFileName((MYOPENFILENAME*)&ofn,guiItem.GetHwndMain(),IDS_TYPEOPENDFS,szFilter,sizeof(szFilter)-1,
                                szOpenFileName,MAX_PATH,NULL,0,0);
            if (!GetOpenFileName((OPENFILENAME*)&ofn))
                return FALSE;
        }


        {
            WCHAR wFileName[MAX_PATH];

            ConvertAnsiToUnicode(szOpenFileName,(dfwcharp)wFileName,sizeof(wFileName)/sizeof(WCHAR));
            DfsFileParamWhereRead.filename = (dfwcharp)wFileName;

            DfsFileOpen(&DfsFileParamWhereRead, &DfsFileWhereRead,NULL);
        }
    }
    ConvertOldDirectoryCommentStorage(DfsFileWhereRead,NULL);
    if (DfsFileWhereRead != NULL)
        pDirInfoDfsFileWhereRead = ReadAllDirInfo(DfsFileWhereRead,&dfNbDirWhereRead,READ_ALL_DIR,NULL);

    //if (pDirInfoWhereAdd != NULL)
        //pDirInfoDfsFileWhereRead = ReadAllDirInfo(DfsFileWhereRead,&dfNbDirWhereRead,READ_ALL_DIR,NULL);
    pDirInfoWhereAdd = DfsFileAndInfo.pDirInfo;
    dfNbDirWhereAdd = DfsFileAndInfo.dfNbDir;


    //printf("Appending version from %ws on %ws\n",DfsFileParamWhereRead.filename,DfsFileParam.filename);

    for (i=0;(i+1<dfNbDirWhereRead);i++)
    {
        PDIRINFO pDirInfoWhereReadTry = NULL;
        dfuLong32 dfNbItemConversionMapList;
        dfuLong32* pPositionConversionMapList;


        dfNbItemConversionMapList = 0;

        pPositionConversionMapList=GetPositionConversionMapList(*(pDirInfoDfsFileWhereRead+i),
                                            *(pDirInfoWhereAdd+dfNbDirWhereAdd-1),&dfNbItemConversionMapList);


        if (pPositionConversionMapList != NULL)
        {
            // DoAppendDfs
            guiItem.InstallProgressBar(2000);
            dfError = DoAppendDfs(DfsFile,
                                    dfNbDirWhereAdd,/*
                                    pDirInfoWhereAdd,*/
                                    DfsFileWhereRead,
                                    /*dfNbDirWhereRead,*/
                                    pDirInfoDfsFileWhereRead,
                                    i+1,
                                    dfNbDirWhereRead-(i+1),
                                    pPositionConversionMapList,dfNbItemConversionMapList,

                                    SetPosCallBack,&guiItem,0,2000,&hei);
            guiItem.RemoveProgressBar();

            DfsFree(pPositionConversionMapList);
            if (dfError == DFS_SUCCESS)
                fRet = TRUE;
            DisplayErrorMessage(guiItem.hwndMain,IDS_ERROR_WRITING,hei,(!fRet));
            FreeErrorInfoBlock(hei);
            break;
        }
    }
    if (i+1==dfNbDirWhereRead)
        DisplayErrorMessage(guiItem.hwndMain,IDS_NOCOMPATIBLEVERSIONFOUND,hei,TRUE);
        //printf("no compatible version found\n");

    if (pDirInfoDfsFileWhereRead != NULL)
        FreeAllDirInfo(pDirInfoDfsFileWhereRead,dfNbDirWhereRead);

    DoAppendNewVersionBuildToList(guiItem,DfsFileAndInfo);

    AdaptDfsFileFeature(DfsFileAndInfo.DfsFile,DfsFileAndInfo.pDirInfo,DfsFileAndInfo.dfNbDir);
    RefreshGrayingMenu(guiItem,DfsFileAndInfo);



    return fRet;
}

/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/

/*************************************************************************************/
/*************************************************************************************/

class GENERICDLG
{
public:
    GENERICDLG() {} ;
    virtual ~GENERICDLG() {};
    virtual BOOL DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) = 0;
};


BOOL CALLBACK GenericDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   BOOL fRet=FALSE;
    if (message==WM_INITDIALOG)
    {
      GENERICDLG* pGENERICDLG = (GENERICDLG*)(lParam);
      if(!pGENERICDLG)
        return FALSE;
      MySetWindowLongPtr(hwnd,DWLP_USER,(DWORD_PTR)pGENERICDLG);
    }

    GENERICDLG* pGENERICDLG = (GENERICDLG*)MyGetWindowLongPtr(hwnd,DWLP_USER);
    if (pGENERICDLG != NULL)
        fRet = pGENERICDLG -> DlgProc(hwnd,message,wParam,lParam);

    if (message==WM_DESTROY)
    {
        //delete pGENERICDLG ;
        MySetWindowLongPtr(hwnd,DWLP_USER,0);
    }
    return fRet;
}

class GENERICDLGEXTRINFO : public GENERICDLG
{
public:
    GENERICDLGEXTRINFO(GUIITEM &guiItemSet,DFSFILEANDINFO &DfsFileAndInfoSet) :
                  guiItem(guiItemSet),
                  DfsFileAndInfo(DfsFileAndInfoSet)
      {} ;
    virtual ~GENERICDLGEXTRINFO() { } ;
    //virtual SetInfo(GUIITEM &guiItemSet,DFSFILEANDINFO &DfsFileAndInfoSet);
protected:
    GUIITEM &guiItem;
    DFSFILEANDINFO &DfsFileAndInfo;
};

/****************************************************************************/


/*
      GetFullPathName(lpszFileNameDfs,dfSizeName,lpszDfsFileName,&lpszDfsFileNameFilePart);
      DfsFileAndInfo.lpszDfsFileName
*/

BOOL CreateTemporaryFileNameInSameDirectory(LPCTSTR lpszOriginalFile,LPTSTR lpszTempFileName,UINT uiSizeBuffer)
{
    TCHAR szDirectory[MAX_PATH+8];
    TCHAR szTempFileName[MAX_PATH+8];
    LPTSTR lpszFilePart=NULL;
    GetFullPathName(lpszOriginalFile,MAX_PATH,szDirectory,&lpszFilePart);
    *lpszFilePart=0;
    GetTempFileName(szDirectory,"SVF",0,szTempFileName);
    if (((unsigned)lstrlen(szTempFileName))>=uiSizeBuffer)
        return FALSE;
    lstrcpy(lpszTempFileName,szTempFileName);
    return TRUE;
}


#define DFVERSIONSELECTED_NONE ((dfuLong32)(0xffffffffUL))
class GENERICDLGEXTRINFOWITHVERSION : public GENERICDLGEXTRINFO
{
public:
    GENERICDLGEXTRINFOWITHVERSION(GUIITEM &guiItemSet,DFSFILEANDINFO &DfsFileAndInfoSet) :
          GENERICDLGEXTRINFO(guiItemSet,DfsFileAndInfoSet) {};
    virtual ~GENERICDLGEXTRINFOWITHVERSION() {};

protected:
    BOOL PrepareListViewVersion(HWND hCtl, const BOOL* pfVersionSelectedMap=NULL,
                                dfuLong32 dfVersionSelected=DFVERSIONSELECTED_NONE);
    BOOL ReadListVersionSelection(HWND hCtl, BOOL* pfVersionSelectedMap=NULL);
    UINT ComputeAndCheckVersionSelectionErrorMessage(const BOOL* pfVersionSelectedMap,
                                           dfuLong32 dfVersionSelected=DFVERSIONSELECTED_NONE,
                                           BOOL fCheckContiguous=TRUE,
                                           dfuLong32 *pdfFirstVersionSelected=NULL,
                                           dfuLong32 *pdfLasrVersionSelected=NULL);
};


BOOL GENERICDLGEXTRINFOWITHVERSION::PrepareListViewVersion(HWND hwndLV, const BOOL* pfVersionSelectedMap,
                                dfuLong32 dfVersionSelected)
{
    dfuLong32 i;

    guiItem.InitListViewColumn(hwndLV,TRUE,FALSE);
    InitListViewImageLists(hwndLV);

    dfuLong32 dfNbItem=DfsFileAndInfo.dfNbDir;
    ListView_SetItemCount(hwndLV, dfNbItem);


    for (i=0;i<DfsFileAndInfo.dfNbDir;i++)
        if (*(pfVersionSelectedMap+i))
          ListView_SetItemState(hwndLV,i,
            LVIS_SELECTED,LVIS_SELECTED);

    if (dfVersionSelected!=DFVERSIONSELECTED_NONE)
    {
      ListView_SetItemState(hwndLV,dfVersionSelected,
          LVIS_FOCUSED/*|LVIS_SELECTED*/,LVIS_FOCUSED/*|LVIS_SELECTED*/);
      ListView_EnsureVisible(hwndLV,dfVersionSelected,TRUE);
    }
    return TRUE;
}

BOOL GENERICDLGEXTRINFOWITHVERSION::ReadListVersionSelection(HWND hwndLV, BOOL* pfVersionSelectedMap)
{
    dfuLong32 i;
    for (i=0;i<DfsFileAndInfo.dfNbDir;i++)
        *(pfVersionSelectedMap+i) =
            ListView_GetItemState(hwndLV,i,LVIS_SELECTED)!=0;
    return TRUE;
}

UINT GENERICDLGEXTRINFOWITHVERSION::ComputeAndCheckVersionSelectionErrorMessage(
                                           const BOOL* pfVersionSelectedMap,
                                           dfuLong32 dfVersionSelected,
                                           BOOL fCheckContiguous,
                                           dfuLong32 *pdfFirstVersionSelected,
                                           dfuLong32 *pdfLastVersionSelected)
{
    UINT uiErrMsg=0;
    if (dfVersionSelected!=DFVERSIONSELECTED_NONE)
        if (!(*(pfVersionSelectedMap+dfVersionSelected)))
            uiErrMsg=IDS_CURVERSIONUNSELECTED;
    dfuLong32 dfFirstSelectedVersion, dfLastSelectedVersion;
    dfFirstSelectedVersion = dfLastSelectedVersion = 0;

    if ((uiErrMsg==0) && (fCheckContiguous))
    {
        dfuLong32 i;
        BOOL fFirstSet,fLastSet;
        fFirstSet=fLastSet=FALSE;
        for (i=0;i<DfsFileAndInfo.dfNbDir;i++)
        {
            BOOL fCurVerSelect=*(pfVersionSelectedMap+i);

            if (fCurVerSelect)
                dfLastSelectedVersion=i;
            if ((fCurVerSelect) && (!fFirstSet))
            {
                fFirstSet=TRUE;
                dfFirstSelectedVersion=i;
            }
            else
            if ((!fCurVerSelect) && (fFirstSet) && (!fLastSet))
            {
                fLastSet=TRUE;
            }
            else
            if ((fCurVerSelect) & (fLastSet))
            {
                uiErrMsg=IDS_NOTCONTIGUOUSVERSIONSELECTED;
            }
        }
    }

    if (pdfFirstVersionSelected!=NULL)
        *pdfFirstVersionSelected = dfFirstSelectedVersion;
    if (pdfLastVersionSelected!=NULL)
        *pdfLastVersionSelected = dfLastSelectedVersion;

    return uiErrMsg;
}
/*********************************************************************/

BOOL DoRemixForReplaceCurrentDls(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,LRUMENU &lrum,
                                 dfuLong32 dfNbVersionRemix,const  VERSIONTOADD_REMIX *pVersionRemix,
                                 dfuLong32 dfVersionSelected,dfuLong32 * pdfError,H_ERROR_INFO & hei)
{
    BOOL fRet=TRUE;
    dfuLong32 dwMinProgress,dwMaxProgress;
    dfwchar wchBaseDirectory[1024+(MAX_PATH*2)];
    dfuLong32 dfError=DFS_SUCCESS;

    TCHAR szTempFileName[MAX_PATH+8];
    WCHAR szwTempFileName[MAX_PATH+8];
    DFSFEATUREPARAM DfsFeatureParam;
    DfsFeatureParam.fComputeMd5 = guiItem.fMd5;
    DfsFeatureParam.fComputeSha1 = guiItem.fSha1;
	DfsFeatureParam.fComputeSha256 = guiItem.fSha256;
    DfsFeatureParam.fStripIdenticalBody = guiItem.pfStripIdentical;


        dwMinProgress=0;
        dwMaxProgress=2000;

        wchBaseDirectory[0]='\0';
        if (DfsFileAndInfo.fBaseDirectorySelected)
        ConvertTCharToUnicode(DfsFileAndInfo.lpBaseDirectory,(dfwcharp)wchBaseDirectory,
                        MAX_PATH);


        guiItem.InstallProgressBar(2000);
        guiItem.SetProgressPos(0);


        CreateTemporaryFileNameInSameDirectory(DfsFileAndInfo.lpszDfsFileName,szTempFileName,MAX_PATH);
        ConvertTCharToUnicode(szTempFileName,(dfwcharp)szwTempFileName,MAX_PATH+2);
        FILESET* pFileSetBase=NULL;


	    COMPRESSIONPARAM CompressionParamAdapted;
	    fillCompressionParamForGuiItem(CompressionParamAdapted, guiItem);
		BOOL fReuseOldPatch = TRUE;
		BOOL fRawAccepted = TRUE;
        dfError = DoReMixDfsEx(DfsFileAndInfo.DfsFile, DfsFileAndInfo.dfNbDir, DfsFileAndInfo.pDirInfo,
                            NULL /*pDfsFileWrite*/,(dfwcharp)szwTempFileName /* dfWritingDfsFileName*/,
                            &DfsFeatureParam,
                            DfsFileAndInfo.fBaseDirectorySelected,
                            DfsFileAndInfo.dfBaseDirNum,
                            wchBaseDirectory,pFileSetBase,
                            dfNbVersionRemix, pVersionRemix,
                            DfsFileAndInfo.fBaseDirectoryNeeded,
							fReuseOldPatch, fRawAccepted,
                            &CompressionParamAdapted,
                            SetPosCallBack /*pSetExtractPosCallBack*/,&guiItem/* dfUserPtr*/,
                            dwMinProgress, dwMaxProgress,
                            NULL);

        guiItem.RemoveProgressBar();

        if (dfError == DFS_SUCCESS)
        {
            LPTSTR lpszCopyFileName = (LPTSTR)DfsMalloc((lstrlen(DfsFileAndInfo.lpszDfsFileName)+4)*sizeof(TCHAR));
            lstrcpy(lpszCopyFileName,DfsFileAndInfo.lpszDfsFileName);

            BOOL fRenamed=FALSE;
            DWORD dwErr=0;
            DoCloseDfs(guiItem,DfsFileAndInfo);
            /* warning : MoveFileEx is not Win95/98 compatible */
            fRenamed = MoveFileEx(szTempFileName,lpszCopyFileName,
                                MOVEFILE_COPY_ALLOWED|MOVEFILE_REPLACE_EXISTING);
            if (!fRenamed)
                dwErr=GetLastError();

            if (dwErr == ERROR_CALL_NOT_IMPLEMENTED)
            {
                TCHAR szTempFileNameBis[MAX_PATH+8];
                CreateTemporaryFileNameInSameDirectory(lpszCopyFileName,szTempFileNameBis,MAX_PATH);
                DeleteFile(szTempFileNameBis);
                if (MoveFile(lpszCopyFileName,szTempFileNameBis))
                {
                    if (MoveFile(szTempFileName,lpszCopyFileName))
                    {
                        fRenamed=TRUE;
                        DeleteFile(szTempFileNameBis);
                    }
                }
            }

            if (!fRenamed)
                fRet=FALSE;

            if (fRet)
              fRet=DoLoadDfs(guiItem,DfsFileAndInfo,lrum,lpszCopyFileName,TRUE,NULL,dfVersionSelected);

            DfsFree(lpszCopyFileName);
        }
        else
        {
            DeleteFile(szTempFileName);
            fRet=FALSE;
        }


    if (pdfError!=NULL)
        *pdfError=dfError;

//    DisplayErrorMessage(guiItem.hwndMain,IDS_ERROR_WRITING,hei);
//    FreeErrorInfoBlock(hei);

    return fRet;
}

/*********************************************************************/
/*********************************************************************/
/* The delete dialog stuff */

typedef struct
{
    dfuLong32 dfNbFile;
    dfuLong32 dfDeletedSize;
    BOOL    fSuppressRenamed;
    BOOL*   pfVersionSelectedMap;
    dfuLong32 dfVersionSelected;
    dfuLong32 dfFirstVersionSelected;
    dfuLong32 dfLastVersionSelected;
} DELETEOPTIONDLGPARAM;

class FILEDELETEDLG : public GENERICDLGEXTRINFOWITHVERSION
{
public:
    FILEDELETEDLG(GUIITEM &guiItemSet,DFSFILEANDINFO &DfsFileAndInfoSet,
                  DELETEOPTIONDLGPARAM* pDeleteOptionDlgParamSet) :
         GENERICDLGEXTRINFOWITHVERSION(guiItemSet,DfsFileAndInfoSet)
      {  pDeleteOptionDlgParam = pDeleteOptionDlgParamSet ;};
    ~FILEDELETEDLG() {};
    BOOL DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    DELETEOPTIONDLGPARAM* pDeleteOptionDlgParam;
    RESIZABLEDLGHELP ResizableDlgHelp;
};


BOOL FILEDELETEDLG::DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
      case WM_INITDIALOG:
          {
              HWND hwndLV = GetDlgItem(hDlg,IDC_LISTSELVERSION);
              {
                  TCHAR szFmt[MAX_PATH+1];
                  TCHAR szText[MAX_PATH+1];
                  GetDlgItemText(hDlg,IDC_STATICDELETEINFO,szFmt,MAX_PATH);
                  wsprintf(szText,szFmt,pDeleteOptionDlgParam->dfNbFile,pDeleteOptionDlgParam->dfDeletedSize);
                  SetDlgItemText(hDlg,IDC_STATICDELETEINFO,szText);
              }
              PrepareListViewVersion(hwndLV, pDeleteOptionDlgParam->pfVersionSelectedMap,pDeleteOptionDlgParam->dfVersionSelected);

              CheckDlgButton(hDlg,IDC_ALSODELETERENAMED,
                             pDeleteOptionDlgParam->fSuppressRenamed ? BST_CHECKED : BST_UNCHECKED);

              ResizableDlgHelp.Init(hDlg);
              ResizableDlgHelp.InitRatio(0x1000);
              {
                  const ITEMCTLINFO ItemCtlInfo[] =
                  {

                    {IDC_LISTSELVERSION,0x000,0x000,0x1000,0x1000,FALSE},
                    {IDC_ALSODELETERENAMED,0x000,0x1000,0,0,FALSE},
                    {IDOK,0x1000,0x000,0,0,TRUE},
                    {IDCANCEL,0x1000,0x000,0,0,TRUE},
                    {IDC_SELECTALLVER,0x1000/6,0x1000,0,0,TRUE},
                    {IDC_UNSELECTALLVER,(3*0x1000)/6,0x1000,0,0,TRUE},
                    {IDC_GETHELP,(5*0x1000)/6,0x1000,0,0,TRUE},

                    {0,0,0,0,0,FALSE}};

                    ResizableDlgHelp.InitCtlList(ItemCtlInfo);
              }

              return TRUE;
          }

      case WM_GETMINMAXINFO:
      {
          SIZE sz;
          MINMAXINFO* pMinMaxInfo = (MINMAXINFO*)lParam;
          ResizableDlgHelp.GetOriginalDialogSize(&sz);
          pMinMaxInfo->ptMinTrackSize.x=sz.cx;
          pMinMaxInfo->ptMinTrackSize.y=sz.cy;
          return 0;
      }

      case WM_SIZE:
          ResizableDlgHelp.OnResize();
          return TRUE;

      case WM_NOTIFY:
          {
              NMHDR* pnmHdr = (NMHDR*)lParam;
              if (pnmHdr->hwndFrom == GetDlgItem(hDlg,IDC_LISTSELVERSION))
                  if (pnmHdr->code == LVN_GETDISPINFO)
                    DoGetDispInfoListViewDir(guiItem,DfsFileAndInfo,pnmHdr);
              break;
          }

      case WM_COMMAND:
            {
                WORD wId = GET_WM_COMMAND_ID(wParam,lParam);

                switch (wId)
                {
                    case IDC_GETHELP:
                        {
                            DoHelpContents(hDlg,//*pADDCONTENTDFSDLGParam->pGuiItem,
                                                //"::/Ordering_SmartVersion.htm");
                                                "::/adding_deleting.htm");

                            return TRUE;
                        }

                    case IDC_SELECTALLVER:
                    case IDC_UNSELECTALLVER:
                        {
                            HWND hwndLV = GetDlgItem(hDlg,IDC_LISTSELVERSION);
                            DWORD dwCount = ListView_GetItemCount(hwndLV);
                            DWORD i;
                            for (i=0;i<dwCount;i++)
                                ListView_SetItemState(hwndLV,i,
                                                      (wId==IDC_SELECTALLVER) ? LVIS_SELECTED : 0,
                                                      LVIS_SELECTED);
                            break;
                        }

                    case IDCANCEL:
                        EndDialog(hDlg, FALSE);
                        return (TRUE);

                    case IDOK:
                    {
                        HWND hwndLV = GetDlgItem(hDlg,IDC_LISTSELVERSION);
                        BOOL fRet=TRUE;
                        UINT uiErrMsg;

                        ReadListVersionSelection(hwndLV,pDeleteOptionDlgParam->pfVersionSelectedMap);

                        uiErrMsg=ComputeAndCheckVersionSelectionErrorMessage(pDeleteOptionDlgParam->pfVersionSelectedMap,
                                           pDeleteOptionDlgParam->dfVersionSelected,
                                           TRUE,
                                           &pDeleteOptionDlgParam->dfFirstVersionSelected,
                                           &pDeleteOptionDlgParam->dfLastVersionSelected);


                        if (uiErrMsg!=0)
                        {
                            TCHAR szMsg[MAX_PATH+2];
                            TCHAR szCaption[MAX_PATH+2];
                            LoadString(ghInstRes,uiErrMsg,szMsg,MAX_PATH);
                            LoadString(ghInstRes,IDS_CANNOTDELETEFILEINTODFS,szCaption,MAX_PATH);
                            MessageBox(guiItem.GetHwndMain(),szMsg,szCaption,MB_ICONERROR);
                            fRet=FALSE;
                        }

                        pDeleteOptionDlgParam->fSuppressRenamed =
                            IsDlgButtonChecked(hDlg,IDC_ALSODELETERENAMED)==BST_CHECKED;

                        EndDialog(hDlg, fRet);
                        return (TRUE);
                    }
                }
            }
            break;
    }

    return (FALSE);                           /* Didn't process a message    */
}



/****************************************************************************/
/****************************************************************************/




BOOL DoFileDelete(GUIITEM & guiItem, DFSFILEANDINFO & DfsFileAndInfo, LRUMENU & lrum)
{
    DELETEOPTIONDLGPARAM DeleteOptionDlgParam;
    FILEDELETEDLG FileDeleteDlg(guiItem, DfsFileAndInfo, &DeleteOptionDlgParam);
    BOOL fRet;
    dfuLong32 i;
    H_ERROR_INFO hei=NULL;
    BOOL fCancelDlg=TRUE;

    DfsClearStruct(&DeleteOptionDlgParam, 0, sizeof(DeleteOptionDlgParam));
    DeleteOptionDlgParam.fSuppressRenamed = TRUE;
    DeleteOptionDlgParam.dfVersionSelected = DfsFileAndInfo.dfCurDir;   //guiItem.;
    DeleteOptionDlgParam.pfVersionSelectedMap =
        (BOOL *) DfsMalloc(sizeof(BOOL) * (DfsFileAndInfo.dfNbDir + 1));

    DeleteOptionDlgParam.dfNbFile = ListView_GetSelectedCount(guiItem.hwndLV);
    DeleteOptionDlgParam.dfDeletedSize = 0;
    fRet = (DeleteOptionDlgParam.pfVersionSelectedMap != NULL);
    if (fRet)
    {
        for (i = 0; i < DfsFileAndInfo.dfNbDir; i++)
            *(DeleteOptionDlgParam.pfVersionSelectedMap + i) = FALSE;
        *(DeleteOptionDlgParam.pfVersionSelectedMap + DfsFileAndInfo.dfCurDir) = TRUE;

        fRet =(BOOL)DialogBoxParam(ghInstRes, MAKEINTRESOURCE(IDD_DELETESEVERALFILES), guiItem.GetHwndMain(),
                              (DLGPROC) GenericDlgProc, (LPARAM) (&FileDeleteDlg));

        fCancelDlg=!fRet;
    }

    if (fRet)
    {
        VERSIONTOADD_REMIX *pVersionRemix;
        dfuLong32 dfNbDirDfsRead = DfsFileAndInfo.dfNbDir;
        dfuLong32 dwNbMapVersionMap = dfNbDirDfsRead;
        //dfuLong32 i;
        dfuLong32 j;
        dfuLong32 dfError;
        dfuLong32 dfNbVersionRemix;
        dfuLong32 dfFirstDeleteVersion, dfLastDeleteVersion;
        dfFirstDeleteVersion = DeleteOptionDlgParam.dfFirstVersionSelected;
        dfLastDeleteVersion = DeleteOptionDlgParam.dfLastVersionSelected;

        pVersionRemix = BuildRecopyVersionToAdd(DfsFileAndInfo.dfNbDir, DfsFileAndInfo.pDirInfo);
         //   (VERSIONTOADD_REMIX *) DfsMalloc(sizeof(VERSIONTOADD_REMIX) * (dwNbMapVersionMap + 1));
        dfNbVersionRemix = DfsFileAndInfo.dfNbDir;

        // Now, we will have to delete file !
        {
            dfuLong32 dfVersionInListView = DfsFileAndInfo.dfCurDir;
            VERSIONTOADD_REMIX *pFileCopyInfoCur = pVersionRemix + dfNbVersionRemix;
            dfuLong32 dfNbDel = ListView_GetSelectedCount(guiItem.hwndLV);
            pFileCopyInfoCur = pVersionRemix + dfVersionInListView;

            for (j = 0; j < (*((DfsFileAndInfo.pDirInfo) + (dfVersionInListView)))->dfNbFile; j++)
            {
                if (ListView_GetItemState(guiItem.hwndLV, j, LVIS_SELECTED) != 0)
                {
                    dfuLong32 jItem = *(guiItem.pdfwListViewSortMap + j);
                    FILETOCOPYINFO_REMIX *pftci = (pFileCopyInfoCur->pFileCopyInfo) + jItem;
                    pftci->dfReferenceItem = FTCI_REFERENCE_DELETE;
                }
            }



            PropagateDelete(pVersionRemix,dfNbVersionRemix, DfsFileAndInfo.pDirInfo,
                            dfVersionInListView,
                            dfFirstDeleteVersion, dfLastDeleteVersion,
                            DeleteOptionDlgParam.fSuppressRenamed);


            if (fRet)
                fRet = DoRemixForReplaceCurrentDls(guiItem, DfsFileAndInfo, lrum,
                                                   dfNbVersionRemix, pVersionRemix,
                                                   DeleteOptionDlgParam.dfVersionSelected,&dfError,hei);
        }
/*
        for (i = 0; i < dfNbVersionRemix; i++)
        {
            FILETOCOPYINFO_REMIX *pFileCopyInfo = (pVersionRemix + i)->pFileCopyInfo;
            DfsFree(pFileCopyInfo);
        }
        DfsFree(pVersionRemix);*/
        FreeVersionToAdd(pVersionRemix,dfNbVersionRemix);


        if (DeleteOptionDlgParam.pfVersionSelectedMap != NULL)
            DfsFree(DeleteOptionDlgParam.pfVersionSelectedMap);
    }
    DisplayErrorMessage(guiItem.hwndMain,IDS_ERROR_DELETING,hei,(!fRet) && (!fCancelDlg));
    FreeErrorInfoBlock(hei);
    return fRet;
}



/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

/************/




/*************************************************************************************/
/*************************************************************************************/

typedef struct
{
    dfuLong32 dfVersionSelected ;
    BOOL*   pfVersionSelectedMap;
    dfuLong32 dfFirstVersionSelected,dfLastVersionSelected;
} INSERTFILESDLGPARAM;


class INSERTFILESDLG : public GENERICDLGEXTRINFOWITHVERSION
{
public:
    INSERTFILESDLG(GUIITEM &guiItemSet,DFSFILEANDINFO &DfsFileAndInfoSet,
                  INSERTFILESDLGPARAM* pInsertFilesDlgParamSet,
                  INSERTFILEUIPARAM* pInsertFileUiParamSet) :
         GENERICDLGEXTRINFOWITHVERSION(guiItemSet,DfsFileAndInfoSet)
            {  pInsertFilesDlgParam = pInsertFilesDlgParamSet ;
                InsertFileUI.SetInfoCache(&guiItemSet.ExtInfoCache);
                InsertFileUI.SetFileUiParam(pInsertFileUiParamSet) ;
                InsertFileUI.SetDfsCurrentFileName(DfsFileAndInfo.lpszDfsFileName);
         };

    ~INSERTFILESDLG() {};
    BOOL DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    RESIZABLEDLGHELP ResizableDlgHelp;
    INSERTFILEUI InsertFileUI;
    INSERTFILESDLGPARAM* pInsertFilesDlgParam;
};

BOOL INSERTFILESDLG::DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
      case WM_INITDIALOG:
          {
              HWND hwndLV = GetDlgItem(hDlg,IDC_LISTSELVERSION);
              PrepareListViewVersion(hwndLV, pInsertFilesDlgParam->pfVersionSelectedMap,pInsertFilesDlgParam->dfVersionSelected);
              InsertFileUI.InitListViewFileToInsertListColumn(hDlg);

              ResizableDlgHelp.Init(hDlg);
              ResizableDlgHelp.InitRatio(0x1000);
              {
                  const ITEMCTLINFO ItemCtlInfo[] =
                  {
                      /*
                    {IDC_GROUPLISTSELVERSION,0,0x000,0x1000,0x600,FALSE},
                    {IDC_LISTSELVERSION,0,0x000,0x1000,0x600,FALSE},
                    {IDC_STATICSELVERSION,0,0x000,0x1000,0x600,FALSE},*/
                    {IDOK,0x1000,0x000,0x0000,0x000,TRUE},
                    {IDCANCEL,0x1000,0x000,0x0000,0x000,TRUE},

                    {IDC_STATICSELFILES,0,0x0000,0x1000,0x000,TRUE},

                    //{IDC_GROUPLISTSELFILES,0,0x0000,0x1000,0xa00,FALSE},
                    {IDC_LISTSELFILES,0,0x0000,0x1000,0xa00,FALSE},


                    {IDC_ADDFILES,0x800,0xa00,0,0,TRUE},
                    {IDC_ADDDIR,0x800,0xa00,0,0,TRUE},
                    {IDC_REMOVEFILES,0x800,0xa00,0,0,TRUE},
                    {IDC_CLEARLISTFILE,0x800,0xa00,0,0,TRUE},

                    {IDC_STATICSEPARATOR,0,0xa00,0x1000,0,FALSE},

                    //{IDC_GROUPLISTSELVERSION,0,0xa00,0x1000,0x600,FALSE},
                    {IDC_LISTSELVERSION,0,0xa00,0x1000,0x600,FALSE},
                    {IDC_STATICSELVERSION,0,0xa00,0x1000,0x000,TRUE},

                    {IDC_SELECTALLVER,0x1000/2,0x1000,0,0,TRUE},
                    {IDC_UNSELECTALLVER,(3*0x1000)/6,0x1000,0,0,TRUE},
                    {IDC_GETHELP,(3*0x1000)/6,0x1000,0,0,TRUE},

                    {0,0,0,0,0,FALSE}};

                    ResizableDlgHelp.InitCtlList(ItemCtlInfo);
              }



              return TRUE;
          }

      case WM_GETMINMAXINFO:
      {
          SIZE sz;
          MINMAXINFO* pMinMaxInfo = (MINMAXINFO*)lParam;
          ResizableDlgHelp.GetOriginalDialogSize(&sz);
          pMinMaxInfo->ptMinTrackSize.x=sz.cx;
          pMinMaxInfo->ptMinTrackSize.y=sz.cy;
          return 0;
      }

      case WM_SIZE:
          ResizableDlgHelp.OnResize();
          return TRUE;

      case WM_NOTIFY:
          {
              NMHDR* pnmHdr = (NMHDR*)lParam;
              if (pnmHdr->hwndFrom == GetDlgItem(hDlg,IDC_LISTSELVERSION))
                  if (pnmHdr->code == LVN_GETDISPINFO)
                    DoGetDispInfoListViewDir(guiItem,DfsFileAndInfo,pnmHdr);

              if (pnmHdr->hwndFrom == GetDlgItem(hDlg,IDC_LISTSELFILES))
              {
                  return (BOOL)InsertFileUI.OnNotifyInsertListView(hDlg,wParam,pnmHdr);
              }
              break;
          }

      case WM_COMMAND:
            {
                WORD wId = GET_WM_COMMAND_ID(wParam,lParam);

                switch (wId)
                {
                    case IDC_GETHELP:
                        {
                            DoHelpContents(hDlg,//*pADDCONTENTDFSDLGParam->pGuiItem,
                                                //"::/Ordering_SmartVersion.htm");
                                                "::/adding_deleting.htm");
                            return TRUE;
                        }

                    case IDC_SELECTALLVER:
                    case IDC_UNSELECTALLVER:
                        {
                            HWND hwndLV = GetDlgItem(hDlg,IDC_LISTSELVERSION);
                            DWORD dwCount = ListView_GetItemCount(hwndLV);
                            DWORD i;
                            for (i=0;i<dwCount;i++)
                                ListView_SetItemState(hwndLV,i,
                                                      (wId==IDC_SELECTALLVER) ? LVIS_SELECTED : 0,
                                                      LVIS_SELECTED);
                            break;
                        }

                    case IDCANCEL:
                        EndDialog(hDlg, FALSE);
                        return (TRUE);

                    case IDC_ADDDIR:
                    case IDC_ADDFILES:
                    case IDC_REMOVEFILES:
                    case IDC_CLEARLISTFILE:
                        InsertFileUI.ProcessDialogInsertFileButtons(hDlg,wId);
                        break;

                    case IDOK:
                    {
                        HWND hwndLV = GetDlgItem(hDlg,IDC_LISTSELVERSION);
                        BOOL fRet=TRUE;
                        UINT uiErrMsg;

                        ReadListVersionSelection(hwndLV,pInsertFilesDlgParam->pfVersionSelectedMap);

                        uiErrMsg=ComputeAndCheckVersionSelectionErrorMessage(pInsertFilesDlgParam->pfVersionSelectedMap,
                                           pInsertFilesDlgParam->dfVersionSelected,
                                           TRUE,
                                           &pInsertFilesDlgParam->dfFirstVersionSelected,
                                           &pInsertFilesDlgParam->dfLastVersionSelected);


                        if (uiErrMsg!=0)
                        {
                            TCHAR szMsg[MAX_PATH+2];
                            TCHAR szCaption[MAX_PATH+2];
                            LoadString(ghInstRes,uiErrMsg,szMsg,MAX_PATH);
                            LoadString(ghInstRes,IDS_CANNOTDELETEFILEINTODFS,szCaption,MAX_PATH);
                            MessageBox(guiItem.GetHwndMain(),szMsg,szCaption,MB_ICONERROR);
                            fRet=FALSE;
                            return TRUE; /* do not quit */
                        }

                        EndDialog(hDlg, fRet);
                        return (TRUE);
                    }
                }
            }
            break;
    }

    return (FALSE);                           /* Didn't process a message    */
}


//cbarwInfo
typedef struct
{
    HWND hWnd;
} CALLBACKASKREPLACING_USERINFO;

dfuLong32 DFSCALLBACK CallBackAskReplacingWin(const ASKREPL_CBINFO* pAskRepl_CbInfo,
                                            dfvoidp dfUserPtr)
{
    const CALLBACKASKREPLACING_USERINFO* pcbarwInfo ;
    TCHAR szFmt[MAX_PATH*2];
    TCHAR szMsg[MAX_PATH*3];
    TCHAR szTitle[MAX_PATH*2];
    DLGYESNOALLPARAM DlgYesNoAllParam;
    INT_PTR rep;
    dfuLong32 dfRet = CBASKREPLACING_ANSWER_REPLACE;
    pcbarwInfo = (const CALLBACKASKREPLACING_USERINFO*)dfUserPtr;

// format contain Would you like to replace the existing file %ws in version %u\nby content of %ws

    LoadString(ghInstRes,IDS_CONFIRMREPLACEININSERTINGMSG,szFmt,sizeof(szFmt)-1);
    wsprintf(szMsg,szFmt,pAskRepl_CbInfo->dfFileNameToStore,pAskRepl_CbInfo->dfVersion,pAskRepl_CbInfo->dfFileNameOnDisk);
    LoadString(ghInstRes,IDS_CONFIRMREPLACEININSERTINGTITLE,szTitle,sizeof(szTitle)-1);
    DlgYesNoAllParam.lpGlobalCaptYNA = szTitle;
    DlgYesNoAllParam.lpGlobalTextYNA = szMsg;
    rep = DialogBoxParam(ghInstRes,
                        MAKEINTRESOURCE(IDD_YESNOCANCALL),
                        pcbarwInfo->hWnd,(DLGPROC)YesNoAllFmtProc,
                        (LPARAM)&DlgYesNoAllParam);

      switch (rep)
      {
        case IDYES :      dfRet = CBASKREPLACING_ANSWER_REPLACE;
                            break;

        case IDC_ALL:
        case IDC_YESALL : dfRet = CBASKREPLACING_ANSWER_REPLACEALL;
                            break;

        case IDC_NO     : dfRet = CBASKREPLACING_ANSWER_NOREPLACE;
                            break;

        case IDC_NOALL :  dfRet = CBASKREPLACING_ANSWER_NOREPLACEALL;
                            break;

        case IDCANCEL :   dfRet = CBASKREPLACING_ANSWER_CANCEL;
                            break;
      }

    return dfRet;
}

BOOL DoInsertFileInVersion(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,LRUMENU &lrum)
{
    INSERTFILESDLGPARAM InsertFilesDlgParam;
    INSERTFILEUIPARAM InsertFileUiParam;
    INSERTFILESDLG InsertFilesDlg(guiItem, DfsFileAndInfo, &InsertFilesDlgParam,&InsertFileUiParam);
    BOOL fRet=TRUE;
    BOOL fCancelDlg=TRUE;
    H_ERROR_INFO hei=NULL;

    DfsClearStruct(&InsertFilesDlgParam, 0, sizeof(InsertFilesDlgParam));
    InsertFilesDlgParam.dfVersionSelected = DfsFileAndInfo.dfCurDir;
    InsertFilesDlgParam.pfVersionSelectedMap =
        (BOOL *) DfsMalloc(sizeof(BOOL) * (DfsFileAndInfo.dfNbDir + 1));

    InsertFileUiParam.pExtInfoCache = &guiItem.ExtInfoCache;

    InsertFileUiParam.psaFilesInsert = new STATIC_ARRAY(sizeof(ITEM_FILE_LIST));
    InsertFileUiParam.psaFilesInsert->SetFuncDestructorSA(fncDestructorItemFileList);
    InsertFileUiParam.psaFilesInsert->SetFuncCompareDataSA(fncCompareItemFileList);

    fRet = (InsertFilesDlgParam.pfVersionSelectedMap != NULL);
    if (fRet)
    {
        dfuLong32 i;
        for (i = 0; i < DfsFileAndInfo.dfNbDir; i++)
            *(InsertFilesDlgParam.pfVersionSelectedMap + i) = FALSE;
        *(InsertFilesDlgParam.pfVersionSelectedMap + DfsFileAndInfo.dfCurDir) = TRUE;

        fRet =(BOOL)DialogBoxParam(ghInstRes, MAKEINTRESOURCE(IDD_INSERTFILES), guiItem.GetHwndMain(),
                              (DLGPROC) GenericDlgProc, (LPARAM) (&InsertFilesDlg));
        if (InsertFileUiParam.psaFilesInsert->GetNbElemSA()==0)
            fRet=FALSE;
        if (!fRet)
            fCancelDlg = TRUE;
    }


    if (fRet)
    {
        INSERTFILEINVERSION_DATA InsertFileInVersionData;
        FILETOADDINVERSIONS* pftaiv ;
        VERSIONTOADD_REMIX *pVersionToAddRemix=NULL;
        CALLBACKASKREPLACING_USERINFO cbarwInfo;
        dfuLong32 dfError = DFS_SUCCESS;
        pftaiv = (FILETOADDINVERSIONS*)DfsMalloc(sizeof(FILETOADDINVERSIONS) *
                          (InsertFileUiParam.psaFilesInsert->GetNbElemSA()+1));
        if (pftaiv == NULL)
            fRet=FALSE;
        else
        {
            dfuLong32 i;
            for (i=0;i<InsertFileUiParam.psaFilesInsert->GetNbElemSA();i++)
            {
                const ITEM_FILE_LIST* pItemFileListCmp;
                pItemFileListCmp=(const ITEM_FILE_LIST*)InsertFileUiParam.psaFilesInsert -> GetElemPtrSA(i);

                (pftaiv+i)->dfFileNameOnDisk = pItemFileListCmp->dfFileNameOnDisk;
                (pftaiv+i)->dfFileNameToStore = pItemFileListCmp->dfFileNameToStore;
            }
        }
        InsertFileInVersionData.dfNbFileToAddInVersion = InsertFileUiParam.psaFilesInsert->GetNbElemSA();
        InsertFileInVersionData.dfNbVersion = DfsFileAndInfo.dfNbDir;
        InsertFileInVersionData.fAskLinkRenamedVersionAfter = FALSE;
        InsertFileInVersionData.fAskLinkRenamedVersionBefore=FALSE;
        InsertFileInVersionData.pFileToAddInVersions=pftaiv;
        InsertFileInVersionData.pVersionMap = InsertFilesDlgParam.pfVersionSelectedMap ;
        InsertFileInVersionData.ReplacementFileOption=REPLACEMENTFILEOPTION_UIASKING;//REPLACEMENTFILEOPTION_REPLACE;

        cbarwInfo.hWnd = guiItem.GetHwndMain();
        pVersionToAddRemix =
            CreateInsertingForReplaceCurrentDfs(InsertFileInVersionData.dfNbVersion, DfsFileAndInfo.pDirInfo,
                                             &InsertFileInVersionData,
                                             DfsFileAndInfo.dfCurDir,&dfError,
                                             CallBackAskReplacingWin,&cbarwInfo);

        if ((pVersionToAddRemix != NULL) && (dfError == DFS_SUCCESS) && fRet)
        {
                fRet = DoRemixForReplaceCurrentDls(guiItem, DfsFileAndInfo, lrum,
                                                   DfsFileAndInfo.dfNbDir, pVersionToAddRemix,
                                                   DfsFileAndInfo.dfCurDir,&dfError,hei);
        }
        FreeVersionToAdd(pVersionToAddRemix,InsertFileInVersionData.dfNbVersion);
        DfsFree(pftaiv);
    }

    delete(InsertFileUiParam.psaFilesInsert);
    DfsFree(InsertFilesDlgParam.pfVersionSelectedMap);
    DisplayErrorMessage(guiItem.hwndMain,IDS_ERROR_INSERTING,hei,(!fRet) && (!fCancelDlg));
    FreeErrorInfoBlock(hei);
    return fRet;
}







/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/


typedef struct
{
    LPTSTR  lpName;
    LPTSTR  lpComment;
    dfuLong32 dfVersionNumber;
    dfuLong32 dfTypeDir;
    DFSTM   dfsTm;
    dfuLong64 dfTotalSize;
    dfuLong64 dfPackedSize;
    TCHAR   szBaseFolder[MAX_PATH+1];
} VERSIONPROPERTIESDLGPARAM;


class VERSIONPROPERTIESDLG
{
public:
    VERSIONPROPERTIESDLG(HWND hDlg,LPARAM dwInitParam);
    ~VERSIONPROPERTIESDLG();
    BOOL DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    VERSIONPROPERTIESDLGPARAM* pVersionPropertiesDlgParam;
    RESIZABLEDLGHELP ResizableDlgHelp;
};



VERSIONPROPERTIESDLG::VERSIONPROPERTIESDLG(HWND hDlg,LPARAM dwInitParam)
{
}

VERSIONPROPERTIESDLG::~VERSIONPROPERTIESDLG()
{
}



BOOL VERSIONPROPERTIESDLG::DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
       switch (message) {
       case WM_INITDIALOG:
           {
            TCHAR szText[MAX_PATH+1];
            pVersionPropertiesDlgParam=(VERSIONPROPERTIESDLGPARAM*)lParam;

            if (pVersionPropertiesDlgParam->lpName!=NULL)
                SetDlgItemText(hDlg,IDC_EDITVERSIONNAME,pVersionPropertiesDlgParam->lpName);

            if (pVersionPropertiesDlgParam->lpComment!=NULL)
                SetDlgItemText(hDlg,IDC_VERSIONPROPCOMMENT,pVersionPropertiesDlgParam->lpComment);

            wsprintf(szText,"%u",pVersionPropertiesDlgParam->dfVersionNumber);
            SetDlgItemText(hDlg,IDC_VERSIONPROPNUMBER,szText);

            BuildStrDate(&pVersionPropertiesDlgParam->dfsTm,szText,MAX_PATH);
            SetDlgItemText(hDlg,IDC_VERSIONPROPDATEMODIFIED,szText);

            WinLong64ToStr(pVersionPropertiesDlgParam->dfPackedSize,szText,MAX_PATH);
            SetDlgItemText(hDlg,IDC_VERSIONPROPPACKED,szText);

            WinLong64ToStr(pVersionPropertiesDlgParam->dfTotalSize,szText,MAX_PATH);
            SetDlgItemText(hDlg,IDC_VERSIONPROPSIZE,szText);



/*
            dfuLong32 dfTotalRatio = CalculateRatio(pVersionPropertiesDlgParam->dfPackedSize,
                                                  pVersionPropertiesDlgParam->dfTotalSize,
                                                  100);

*/
            //dfTotalRatio = CalculateRatio(dfTotalPacked,dfTotalSize,100);

            {
                dfuLong32  dfTotalRatio ;
                dfuLong64 dfTotalSize=pVersionPropertiesDlgParam->dfTotalSize;
                dfuLong64 dfTotalPacked=pVersionPropertiesDlgParam->dfPackedSize;
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
                wsprintf(szText,"%u %%",dfTotalRatio);
            }


            SetDlgItemText(hDlg,IDC_VERSIONPROPRATIO,szText);

            // IDC_VERSIONPROPBASE , IDC_VERSIONPROPTYPE
            UINT uID = GetUiResDescTypeDir(pVersionPropertiesDlgParam->dfTypeDir);
            if (uID == 0)
                szText[0]='\0';
            else
            LoadString(ghInstRes,uID,szText,MAX_PATH);
            SetDlgItemText(hDlg,IDC_VERSIONPROPTYPE,szText);

            SetDlgItemText(hDlg,IDC_VERSIONPROPBASE,pVersionPropertiesDlgParam->szBaseFolder);


            ResizableDlgHelp.Init(hDlg);
            ResizableDlgHelp.InitRatio(0x1000);
            {
                const ITEMCTLINFO ItemCtlInfo[] =
                {

                {IDC_VERSIONPROPCOMMENT,0x000,0x000,0x1000,0x1000,FALSE},
                {IDC_EDITVERSIONNAME,0x000,0x000,0x1000,0,FALSE},
                {IDOK,0x1000,0x1000,0,0,TRUE},
                {IDCANCEL,0x1000,0x1000,0,0,TRUE},
                {IDC_GETHELP,0x1000,0x1000,0,0,TRUE},
                {0,0,0,0,0,FALSE}};

                ResizableDlgHelp.InitCtlList(ItemCtlInfo);
            }

            return TRUE;
           }

            case WM_GETMINMAXINFO:
            {
                SIZE sz;
                MINMAXINFO* pMinMaxInfo = (MINMAXINFO*)lParam;
                ResizableDlgHelp.GetOriginalDialogSize(&sz);
                pMinMaxInfo->ptMinTrackSize.x=sz.cx;
                pMinMaxInfo->ptMinTrackSize.y=sz.cy;
                return 0;
            }

            case WM_SIZE:
                ResizableDlgHelp.OnResize();
                return TRUE;


        case WM_COMMAND:    ///IDC_WEBSITELINK
            {
                WORD wId = GET_WM_COMMAND_ID(wParam,lParam);

                switch (wId)
                {
                    case IDC_GETHELP:
                        {
                            DoHelpContents(hDlg,//*pADDCONTENTDFSDLGParam->pGuiItem,
                                                //"::/Ordering_SmartVersion.htm");
                                                NULL);
                            return TRUE;
                        }

                    case IDOK:
                    case IDCANCEL:
                    {
                        BOOL fRet=(wId==IDOK);
                        if (fRet)
                        {
                            dfuLong32 dfAlloc;
                            if (pVersionPropertiesDlgParam->lpName!=NULL)
                                DfsFree(pVersionPropertiesDlgParam->lpName);
                            dfAlloc=(GetWindowTextLength(GetDlgItem(hDlg,IDC_EDITVERSIONNAME))*
                                              sizeof(TCHAR))+0x10;
                            pVersionPropertiesDlgParam->lpName = (LPTSTR)DfsMalloc(dfAlloc*2);

                            GetDlgItemText(hDlg,IDC_EDITVERSIONNAME,pVersionPropertiesDlgParam->lpName,dfAlloc);


                            if (pVersionPropertiesDlgParam->lpComment!=NULL)
                                DfsFree(pVersionPropertiesDlgParam->lpComment);
                            dfAlloc=(GetWindowTextLength(GetDlgItem(hDlg,IDC_VERSIONPROPCOMMENT))*
                                              sizeof(TCHAR))+0x10;
                            pVersionPropertiesDlgParam->lpComment = (LPTSTR)DfsMalloc(dfAlloc*2);
                            GetDlgItemText(hDlg,IDC_VERSIONPROPCOMMENT,pVersionPropertiesDlgParam->lpComment,dfAlloc);
                        }
                        EndDialog(hDlg, fRet);
                        return (TRUE);
                    }

                }

            }
            break;
    }
    return (FALSE);                           /* Didn't process a message    */
}




BOOL CALLBACK VersionPropertiesDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   BOOL fRet=FALSE;
    if (message==WM_INITDIALOG)
    {
      VERSIONPROPERTIESDLG* pVERSIONPROPERTIESDLG = new VERSIONPROPERTIESDLG(hwnd, lParam);
      if(!pVERSIONPROPERTIESDLG)
        return FALSE;
      MySetWindowLongPtr(hwnd,DWLP_USER,(LONG_PTR)pVERSIONPROPERTIESDLG);
    }

    VERSIONPROPERTIESDLG* pVERSIONPROPERTIESDLG = (VERSIONPROPERTIESDLG*)MyGetWindowLongPtr(hwnd,DWLP_USER);
    if (pVERSIONPROPERTIESDLG != NULL)
        fRet = pVERSIONPROPERTIESDLG -> DlgProc(hwnd,message,wParam,lParam);

    if (message==WM_DESTROY)
    {
        delete pVERSIONPROPERTIESDLG ;
        MySetWindowLongPtr(hwnd,DWLP_USER,0);
    }
    return fRet;
}


static BOOL UpdateFloatItem(DFTAGBLOCKFLOAT TagBlockFloat, dfuLong32 dfDirNum, dfuLong32 dfFileNum, dfuLong32 TagNumber,LPTSTR lpNewValue)
{
    BOOL fModifiedDirName=TRUE;
    dfwcharp pNewDirName = NULL;
    dfuLong32 dfNewDirNameUnicodeLen=0;
    dfvoidp TagBuf;
    dfuLong32 TagSize;

    if (lpNewValue!=NULL)
    {
        fModifiedDirName=TRUE;
        pNewDirName = (dfwcharp)DfsMalloc((lstrlen(lpNewValue)+0x10) *4);

        if (pNewDirName == NULL)
            return FALSE;
        ConvertTCharToUnicode(lpNewValue,pNewDirName,(lstrlen(lpNewValue)+0x04) *4);
        dfNewDirNameUnicodeLen = dfUnicodeStrlen(pNewDirName);
    }

    if (GetTagBlockFloat(TagBlockFloat, dfDirNum,
                        dfFileNum, TagNumber,
                        &TagBuf, &TagSize))
    {
        dfuLong32 dfUnicLen = dfUnicodeStrlen (pNewDirName);
        if (dfUnicodeStrcmp(pNewDirName,(dfwcharp)TagBuf)==0)
            fModifiedDirName = FALSE;
    }
    else
        fModifiedDirName = (dfNewDirNameUnicodeLen!=0);

    if (fModifiedDirName)
    {
        if (dfNewDirNameUnicodeLen>0)
            AddTagBlockFloat(TagBlockFloat,dfDirNum,dfFileNum,TagNumber,
                            pNewDirName,(dfNewDirNameUnicodeLen + 1) * 2);
        else
            RemoveTagBlockFloat(TagBlockFloat,dfDirNum,dfFileNum,TagNumber);

        //SetDfsTagBlockFloatDirty(DfsFileAndInfo.DfsFile);
        //guiItem.UpdateTreeViewItem(DfsFileAndInfo,dfDirProperties);
    }

    if (pNewDirName!=NULL)
        DfsFree(pNewDirName);

    return fModifiedDirName;
}



BOOL DoVersionProperties(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,BOOL fSpecifyDir,dfuLong32 dfDirNum)
{
    BOOL fRet;
    VERSIONPROPERTIESDLGPARAM VersionPropertiesDlgParam;
    dfuLong32 i;
    dfuLong32 dfDirProperties;
    BOOL fUseListViewSelectInfo=TRUE;
    PDIRINFO pCurDirInfo;
    LPBOOL lpfPropertiesItemMap = NULL;
    dfuLong32 dfNbSelected=0;
    dfvoidp TagBuf;
    dfuLong32 TagSize;
    DFTAGBLOCKFLOAT TagBlockFloat;
    BOOL fModifyingDone=FALSE;



    if (DfsFileAndInfo.DfsFile == NULL)
        return FALSE;

    if (fSpecifyDir)
        dfDirProperties=dfDirNum;
    else
    {
        dfuLong32 dfTreeViewSelected=DfsFileAndInfo.dfCurDir;
        HTREEITEM hti;

        hti = TreeView_GetSelection(guiItem.hwndTreeView);
        if (hti!=NULL)
        {
            TVITEM tv;
            tv.mask=TVIF_PARAM;
            tv.hItem = hti;
            tv.lParam = 0xffffffff;

            if (TreeView_GetItem(guiItem.hwndTreeView, &tv))
              dfTreeViewSelected = (dfuLong32)tv.lParam ;
        }

        if (((DWORD)dfTreeViewSelected) == TVITEMPARAM_ROOT)
        {
        if (GetListViewSelectionStatus(guiItem.hwndLV,&dfDirProperties)!=1)
            return FALSE;
        fUseListViewSelectInfo=FALSE;
        }
        else
            dfDirProperties=dfTreeViewSelected;
    }

    pCurDirInfo = *(DfsFileAndInfo.pDirInfo+ dfDirProperties);


    if (fUseListViewSelectInfo)
    {
        lpfPropertiesItemMap = (BOOL*)DfsMalloc(sizeof(BOOL)*(pCurDirInfo ->dfNbFile + 1));

        for (i=0;i<pCurDirInfo ->dfNbFile;i++)
            *(lpfPropertiesItemMap+i)=TRUE;

        for (i=0;i<pCurDirInfo ->dfNbFile;i++)
        {
            dfuLong32 iItem = *(guiItem.pdfwListViewSortMap+i);
            *(lpfPropertiesItemMap+iItem) =
              ListView_GetItemState(guiItem.hwndLV,i,LVIS_SELECTED) != 0;
        }


        for (i=0;i<pCurDirInfo ->dfNbFile;i++)
            if (*(lpfPropertiesItemMap+i))
                dfNbSelected++;
    }

    if ((dfNbSelected==0) && (lpfPropertiesItemMap!=NULL))
        for (i=0;i<pCurDirInfo ->dfNbFile;i++)
            *(lpfPropertiesItemMap+i)=TRUE;

    if (lpfPropertiesItemMap!=NULL)
        DfsFree(lpfPropertiesItemMap);
    lpfPropertiesItemMap = NULL;

    VersionPropertiesDlgParam.lpComment = NULL;
    VersionPropertiesDlgParam.lpName = NULL;

    VersionPropertiesDlgParam.dfVersionNumber = dfDirProperties;

    VersionPropertiesDlgParam.dfTotalSize=0;
    VersionPropertiesDlgParam.dfPackedSize=0;
    VersionPropertiesDlgParam.dfTypeDir = pCurDirInfo->dfTypeDir;
    DfsClearStruct(&VersionPropertiesDlgParam.dfsTm,0,sizeof(VersionPropertiesDlgParam.dfsTm));

    for (i=0;i<pCurDirInfo->dfNbFile;i++)
    {
        dfvoidp TagBuf;
        dfuLong32 TagSize;
        VersionPropertiesDlgParam.dfTotalSize += (pCurDirInfo->pFileInDirInfo + i)->dfSize;
        VersionPropertiesDlgParam.dfPackedSize +=(pCurDirInfo->pFileInDirInfo + i)->dfFileEncodedSize;

        if (GetTag(*(pCurDirInfo->TagFile + i), DFSTAG_DATE, &TagBuf, &TagSize))
        {
            DFSTM dfsTmThis;
            ConvertDfsInfoDateToDfsTm((DFSINFODATE *) TagBuf, &dfsTmThis);
            if (CompareDfsTm(&VersionPropertiesDlgParam.dfsTm,&dfsTmThis)==-1)
                VersionPropertiesDlgParam.dfsTm=dfsTmThis;
        }
    }

    VersionPropertiesDlgParam.szBaseFolder[0]='\0';
    if (DfsFileAndInfo.fBaseDirectorySelected && (DfsFileAndInfo.dfBaseDirNum == dfDirProperties))
        wsprintf(VersionPropertiesDlgParam.szBaseFolder,"%s",DfsFileAndInfo.lpBaseDirectory);

    TagBlockFloat = GetDfsTagBlockFloat(DfsFileAndInfo.DfsFile,NULL);
    if (TagBlockFloat != NULL)
    {
        if (GetTagBlockFloat(TagBlockFloat, dfDirProperties,
                             FLOATNUM_NOSPECIFIC, DFSTAG_DIR_NAME_FLOAT,
                             &TagBuf, &TagSize))
        {
            dfwcharp pDirName = (dfwcharp)TagBuf;
            VersionPropertiesDlgParam.lpName = (LPTSTR)DfsMalloc((dfUnicodeStrlen(pDirName)*2)+0x100);
            wsprintf(VersionPropertiesDlgParam.lpName,"%ws",pDirName);
        }

        if (GetTagBlockFloat(TagBlockFloat, dfDirProperties,
                             FLOATNUM_NOSPECIFIC, DFSTAG_DIR_COMMENT_FLOAT,
                             &TagBuf, &TagSize))
        {
            dfwcharp pDirComment = (dfwcharp)TagBuf;
            VersionPropertiesDlgParam.lpComment = (LPTSTR)DfsMalloc((dfUnicodeStrlen(pDirComment)*2)+0x100);
            wsprintf(VersionPropertiesDlgParam.lpComment,"%ws",pDirComment);
        }
    }

    fRet = (BOOL)DialogBoxParam(ghInstRes, MAKEINTRESOURCE(IDD_VERSIONPROPERTIES), guiItem.GetHwndMain(),
                          (DLGPROC)VersionPropertiesDlgProc,(LONG_PTR)(&VersionPropertiesDlgParam));


    if (fRet)
    {
        if (UpdateFloatItem(TagBlockFloat, dfDirProperties,
                             FLOATNUM_NOSPECIFIC, DFSTAG_DIR_COMMENT_FLOAT,VersionPropertiesDlgParam.lpComment))
            fModifyingDone=TRUE;

        if (UpdateFloatItem(TagBlockFloat, dfDirProperties,
                             FLOATNUM_NOSPECIFIC, DFSTAG_DIR_NAME_FLOAT,VersionPropertiesDlgParam.lpName))
        {
            fModifyingDone=TRUE;
            guiItem.UpdateTreeViewItem(DfsFileAndInfo,dfDirProperties);
        }
    }

    if (VersionPropertiesDlgParam.lpName!=NULL)
        DfsFree(VersionPropertiesDlgParam.lpName);
    if (VersionPropertiesDlgParam.lpComment!=NULL)
        DfsFree(VersionPropertiesDlgParam.lpComment);

    if (fModifyingDone)
        SetDfsTagBlockFloatDirty(DfsFileAndInfo.DfsFile,NULL);
    return fRet;
}


/****************************************************************************/
/****************************************************************************/

typedef struct
{
} FILEPROPERTIESDLGPARAM;


class FILEPROPERTIESPROCANDPARAM:public DLGPROCANDPARAM
{
public:
    FILEPROPERTIESPROCANDPARAM(GUIITEM &guiItemSet,DFSFILEANDINFO &DfsFileAndInfoSet,FILEPROPERTIESDLGPARAM* pParamSet) :
                  guiItem(guiItemSet),
                  DfsFileAndInfo(DfsFileAndInfoSet)
      { pFilePropertiesParam = pParamSet; } ;

    ~FILEPROPERTIESPROCANDPARAM() { };
    BOOL DlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);

private:
    BOOL FillCurFile(HWND hDlg);

    BOOL GetPreviousPositionOfFile(dfuLong32 dfNumVersion,dfuLong32 dfNumFile,dfuLong32& dfPrevious,BOOL &fIdentical);
    BOOL GetNextPositionOfFile(dfuLong32 dfNumVersion,dfuLong32 dfNumFile,dfuLong32& dfNext,BOOL &fIdentical);

    BOOL GetPreviousPositionOfFileNotIdentical(dfuLong32 dfNumVersion,dfuLong32 dfNumFile,
                                               dfuLong32& dfPreviousVersion,dfuLong32 & dfPreviousPos);
    BOOL GetNextPositionOfFileNotIdentical(dfuLong32 dfNumVersion,dfuLong32 dfNumFile,
                               dfuLong32& dfNextVersion,dfuLong32 & dfNextPos);


    FILEPROPERTIESDLGPARAM* pFilePropertiesParam;
    GUIITEM &guiItem;
    DFSFILEANDINFO &DfsFileAndInfo;
    BOOL fOneFileSelect;
    dfuLong32 dfCurVersion;
    dfuLong32 dfCurFile;
} ;

BOOL FILEPROPERTIESPROCANDPARAM::GetPreviousPositionOfFile(dfuLong32 dfNumVersion,dfuLong32 dfNumFile,
                                                           dfuLong32& dfPrevious,BOOL &fIdentical)
{
    BOOL fFound = FALSE;
    dfvoidp TagBuf;
    dfuLong32 TagSize;
    BOOL fIdenticalFound=FALSE;

    if (dfNumVersion == 0)
        return FALSE;

    if (GetTag(*(((*(DfsFileAndInfo.pDirInfo+dfNumVersion))->TagFile + dfNumFile)), DFSTAG_STORAGESTATUS, &TagBuf, &TagSize))
    {
            if (TagSize==sizeof(dfuLong32Intel))
            {
                dfuLong32 dfFileIdentical = ConvertuLongIntelToLong(*(dfuLong32Intel*)TagBuf);
                fIdenticalFound = (dfFileIdentical==DFS_STORAGESTATUS_IDENTICAL);
            }
    }

    if (GetTag(*(((*(DfsFileAndInfo.pDirInfo+dfNumVersion))->TagFile + dfNumFile)), DFSTAG_PREVIOUSVERSIONINFO, &TagBuf, &TagSize))
    {
        dfuLong32 dfPreviousVersionFileNumber, dfPreviousVersionFilePosition;
        const DFSPREVIOUSVERSIONINFO *pDfsPreviousVersionInfo =
        (const DFSPREVIOUSVERSIONINFO *) TagBuf;
        dfPreviousVersionFileNumber =
            ConvertuLongIntelToLong(pDfsPreviousVersionInfo->
                                    dfPreviousVersionFileNumber);
        dfPreviousVersionFilePosition =
            ConvertuLongIntelToLong(pDfsPreviousVersionInfo->
                                    dfPreviousVersionFilePosition);
        if (dfPreviousVersionFileNumber == 1)
        {
            dfPrevious = dfPreviousVersionFilePosition;
            fFound = TRUE;
            fIdentical = fIdenticalFound;
        }
    }

    return fFound;
}

BOOL FILEPROPERTIESPROCANDPARAM::GetPreviousPositionOfFileNotIdentical(dfuLong32 dfNumVersion,dfuLong32 dfNumFile,
                                             dfuLong32& dfPreviousVersion,dfuLong32 & dfPreviousPos)
{
    dfuLong32 dfPrevPosFound;
    BOOL fIdentical;
    for (;;)
    {
        if (!GetPreviousPositionOfFile(dfNumVersion,dfNumFile,dfPrevPosFound,fIdentical))
            return FALSE;
        if (!fIdentical)
        {
            dfPreviousVersion = dfNumVersion -1;
            dfPreviousPos = dfPrevPosFound;
            return TRUE;
        }
        dfNumVersion--;
        dfNumFile = dfPrevPosFound;
    }
}


BOOL FILEPROPERTIESPROCANDPARAM::GetNextPositionOfFile(dfuLong32 dfNumVersion,dfuLong32 dfNumFile,
                                                       dfuLong32& dfNext,BOOL &fIdentical)
{
    BOOL fFound = FALSE;
    dfuLong32 j;
    if (dfNumVersion >= DfsFileAndInfo.dfNbDir-1)
        return FALSE;


    for (j=0;j<(*(DfsFileAndInfo.pDirInfo+dfNumVersion+1))->dfNbFile;j++)
    {
        dfvoidp TagBuf;
        dfuLong32 TagSize;
        BOOL fIdenticalFound=FALSE;
        if (GetTag(*(((*(DfsFileAndInfo.pDirInfo+dfNumVersion+1))->TagFile + j)), DFSTAG_STORAGESTATUS, &TagBuf, &TagSize))
        {
                if (TagSize==sizeof(dfuLong32Intel))
                {
                    dfuLong32 dfFileIdentical = ConvertuLongIntelToLong(*(dfuLong32Intel*)TagBuf);
                    fIdenticalFound = (dfFileIdentical==DFS_STORAGESTATUS_IDENTICAL);
                }
        }

        if (GetTag(*(((*(DfsFileAndInfo.pDirInfo+dfNumVersion+1))->TagFile + j)), DFSTAG_PREVIOUSVERSIONINFO, &TagBuf, &TagSize))
        {
            dfuLong32 dfPreviousVersionFileNumber, dfPreviousVersionFilePosition;
            const DFSPREVIOUSVERSIONINFO *pDfsPreviousVersionInfo =
            (const DFSPREVIOUSVERSIONINFO *) TagBuf;
            dfPreviousVersionFileNumber =
                ConvertuLongIntelToLong(pDfsPreviousVersionInfo->
                                        dfPreviousVersionFileNumber);
            dfPreviousVersionFilePosition =
                ConvertuLongIntelToLong(pDfsPreviousVersionInfo->
                                        dfPreviousVersionFilePosition);
            if (dfPreviousVersionFileNumber == 1)
                if (dfPreviousVersionFilePosition == dfNumFile)
            {
                dfNext = j;
                fFound = TRUE;
                fIdentical = fIdenticalFound;
                break;
            }
        }
    }
    return fFound;
}


BOOL FILEPROPERTIESPROCANDPARAM::GetNextPositionOfFileNotIdentical(dfuLong32 dfNumVersion,dfuLong32 dfNumFile,
                                  dfuLong32& dfNextVersion,dfuLong32 & dfNextPos)
{
    dfuLong32 dfNextPosFound;
    BOOL fIdentical;
    for (;;)
    {
        if (!GetNextPositionOfFile(dfNumVersion,dfNumFile,dfNextPosFound,fIdentical))
            return FALSE;
        if (!fIdentical)
        {
            dfNextVersion = dfNumVersion + 1;
            dfNextPos = dfNextPosFound;
            return TRUE;
        }
        dfNumVersion++;
        dfNumFile = dfNextPosFound;
    }
}

BOOL FILEPROPERTIESPROCANDPARAM::FillCurFile(HWND hDlg)
{
    const FILEINDIRINFO* pFileCur ;
    TCHAR szText[MAX_PATH*2];
    dfwchar dfwOnlyFileName[MAX_PATH];
    dfwchar dfwOnlyFilePath[MAX_PATH];
    if (!fOneFileSelect)
        return FALSE;

    wsprintf(szText,"%u",dfCurVersion);
    SetDlgItemText(hDlg,IDC_EDITFILEPROPVERNUM,szText);
    SetDlgItemText(hDlg,IDC_EDITFILEPROPVERNAME,"");


    {
        dfvoidp TagBuf;
        dfuLong32 TagSize;
        DFTAGBLOCKFLOAT TagBlockFloat;
        TagBlockFloat = GetDfsTagBlockFloat(DfsFileAndInfo.DfsFile, NULL);
        if (TagBlockFloat != NULL)
        {
            if (GetTagBlockFloat(TagBlockFloat,dfCurVersion,FLOATNUM_NOSPECIFIC,DFSTAG_DIR_NAME_FLOAT,&TagBuf, &TagSize))
            {
                wsprintf(szText,"%ws",TagBuf);
                SetDlgItemText(hDlg,IDC_EDITFILEPROPVERNAME,szText);
            }
/*
            if (fVerbose)
            if (GetTagBlockFloat(TagBlockFloat,dfNumDir,FLOATNUM_NOSPECIFIC,DFSTAG_DIR_COMMENT_FLOAT,&TagBuf, &TagSize))
            {
                printf("\n%ws",TagBuf);
            }*/

        }
    }


    pFileCur=((*((DfsFileAndInfo.pDirInfo) + (dfCurVersion)))->pFileInDirInfo) + dfCurFile;
    dfwOnlyFilePath[0] = dfwOnlyFileName[0] = '\0';
    SplitFileNameAndPath(pFileCur->FileName,
                            dfwOnlyFilePath,MAX_PATH,
                            dfwOnlyFileName,MAX_PATH,
                            TRUE);

    wsprintf(szText,"%ws",dfwOnlyFileName);
    SetDlgItemText(hDlg,IDC_EDITFILEPROPFILENAME,szText);
    wsprintf(szText,"%ws",dfwOnlyFilePath);
    SetDlgItemText(hDlg,IDC_EDITFILEPROPPATH,szText);

    {
        LPCTSTR lpszRegisteredType = "";
        wsprintf(szText,"%ws",dfwOnlyFileName);
        lpszRegisteredType = guiItem.ExtInfoCache.GetExtensionDescFromRegistryCached(szText,FALSE);

        if (lpszRegisteredType==NULL)
            lpszRegisteredType="";

        wsprintf(szText,"%s",lpszRegisteredType);
        SetDlgItemText(hDlg,IDC_EDITFILEPROPTYPE,szText);
    }

    WinLong64ToStr(pFileCur->dfSize,szText,MAX_PATH);
    SetDlgItemText(hDlg,IDC_EDITFILEPROPSIZE,szText);

    WinLong64ToStr(pFileCur->dfFileEncodedSize,szText,MAX_PATH);
    SetDlgItemText(hDlg,IDC_EDITFILEPROPPACKED,szText);


    {
        dfvoidp TagBuf;
        dfuLong32 TagSize;
        if (GetTag(*(((*(DfsFileAndInfo.pDirInfo+dfCurVersion))->TagFile + dfCurFile)), DFSTAG_STORAGESTATUS, &TagBuf, &TagSize))
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
                     szText,(sizeof(szText)/sizeof(TCHAR))-1);
            }
        }
        else lstrcpy(szText,"not found");
        SetDlgItemText(hDlg,IDC_EDITFILEPROPSTATUS,szText);
    }

    {
        dfvoidp TagBuf;
        dfuLong32 TagSize;
        if (GetTag(*(((*(DfsFileAndInfo.pDirInfo+dfCurVersion))->TagFile + dfCurFile)),DFSTAG_DATE, &TagBuf, &TagSize))
        {
            DFSTM dfsTm;
            ConvertDfsInfoDateToDfsTm((DFSINFODATE *) TagBuf, &dfsTm);
            BuildStrDate(&dfsTm,szText,MAX_PATH);
            SetDlgItemText(hDlg,IDC_EDITFILEPROPDATE,szText);
        }
    }

    {
        dfsLong32 dfPackedRatio ;
        if (pFileCur->dfSize>0)
        {
            dfsLong64 dfPackSize = ((dfsLong64)pFileCur->dfSize) - ((signed)pFileCur->dfFileEncodedSize);
            if (dfPackSize<0)
            {
                if (pFileCur->dfSize>0)
                  dfPackedRatio = (dfsLong32)((dfPackSize*100)/((dfsLong64)(pFileCur->dfSize)));
                else
                  dfPackedRatio = 100;
            }
            else
                dfPackedRatio = CalculateRatio((dfuLong64)dfPackSize,pFileCur->dfSize,100);
        }
        wsprintf(szText,"%02d %%",dfPackedRatio);
        SetDlgItemText(hDlg,IDC_EDITFILEPROPRATIO,szText);
    }


    wsprintf(szText,"%08x",pFileCur->dfCrc32);
    SetDlgItemText(hDlg,IDC_EDITFILEPROPCRC,szText);
    ShowWindow(GetDlgItem(hDlg,IDC_STATICFILEPROPMD5),(pFileCur->fMd5Filled) ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hDlg,IDC_EDITFILEPROPMD5),(pFileCur->fMd5Filled) ? SW_SHOW : SW_HIDE);

    if (pFileCur->fMd5Filled)
    {
        dfuLong32 k;
        for (k=0;k<16;k++)
            wsprintf(((LPTSTR)szText)+(k*2),"%02x",pFileCur->bMd5[k]);
        SetDlgItemText(hDlg,IDC_EDITFILEPROPMD5,szText);
    }


    ShowWindow(GetDlgItem(hDlg,IDC_STATICFILEPROPSHA1),(pFileCur->fSha1Filled) ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hDlg,IDC_EDITFILEPROPSHA1),(pFileCur->fSha1Filled) ? SW_SHOW : SW_HIDE);

	ShowWindow(GetDlgItem(hDlg, IDC_STATICFILEPROPSHA256), (pFileCur->fSha256Filled) ? SW_SHOW : SW_HIDE);
	ShowWindow(GetDlgItem(hDlg, IDC_EDITFILEPROPSHA256), (pFileCur->fSha256Filled) ? SW_SHOW : SW_HIDE);

    if (pFileCur->fSha1Filled)
    {
        dfuLong32 k;
        for (k=0;k<20;k++)
            wsprintf(((LPTSTR)szText)+(k*2),"%02x",pFileCur->bSha1[k]);
        SetDlgItemText(hDlg,IDC_EDITFILEPROPSHA1,szText);
    }

	if (pFileCur->fSha256Filled)
	{
		dfuLong32 k;
		for (k = 0; k<32; k++)
			wsprintf(((LPTSTR)szText) + (k * 2), "%02x", pFileCur->bSha256[k]);
		SetDlgItemText(hDlg, IDC_EDITFILEPROPSHA256, szText);
	}
    {
        dfuLong32 dfNumVer,dfNumFile;
        BOOL fIdentical;
        BOOL fPrevFound = GetPreviousPositionOfFile(dfCurVersion,dfCurFile,dfNumFile,fIdentical);
        BOOL fPrevFoundModified = GetPreviousPositionOfFileNotIdentical(dfCurVersion,dfCurFile,dfNumVer,dfNumFile);
        BOOL fNextFound = GetNextPositionOfFile(dfCurVersion,dfCurFile,dfNumFile,fIdentical);
        BOOL fNextFoundModified = GetNextPositionOfFileNotIdentical(dfCurVersion,dfCurFile,dfNumVer,dfNumFile);
        EnableWindow(GetDlgItem(hDlg,IDC_FILEPROPPREVIOUSBUTTON),fPrevFound);
        EnableWindow(GetDlgItem(hDlg,IDC_FILEPROPPREVIOUSMODIFIEDBUTTON),fPrevFoundModified);
        EnableWindow(GetDlgItem(hDlg,IDC_FILEPROPNEXTBUTTON),fNextFound);
        EnableWindow(GetDlgItem(hDlg,IDC_FILEPROPNEXTMODIFIEDBUTTON),fNextFoundModified);
    }
    return TRUE;
}

BOOL FILEPROPERTIESPROCANDPARAM::DlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
    switch (message)
    {
      case WM_INITDIALOG:

            if (DfsFileAndInfo.dfNbFileSelectedCurDir>0)
            {
                TCHAR szSize[MAX_PATH+1];
                TCHAR szBuffer[MAX_PATH+1];
                TCHAR szFormat[MAX_PATH+1];
                LoadString(ghInstRes, IDS_STATUSBAR_INFO_SELECTION, szFormat, sizeof(szFormat));
                WinLong64ToStr(DfsFileAndInfo.dfSizeSelectedCurDir,szSize,MAX_PATH);
                wsprintf(szBuffer,szFormat,DfsFileAndInfo.dfNbFileSelectedCurDir,szSize);
                SetDlgItemText(hDlg,IDC_EDITFILEPROPSIZE,szBuffer);
                dfCurVersion = DfsFileAndInfo.dfCurDir;
                fOneFileSelect = FALSE;
                if (ListView_GetSelectedCount(guiItem.hwndLV) == 1)
                {
                    dfuLong32 j;


                    for (j = 0; j < (*((DfsFileAndInfo.pDirInfo) + (dfCurVersion)))->dfNbFile; j++)
                    {
                        if (ListView_GetItemState(guiItem.hwndLV, j, LVIS_SELECTED) != 0)
                        {
                            dfCurFile = *(guiItem.pdfwListViewSortMap + j);
                            fOneFileSelect = TRUE;
                            break;
                        }
                    }
                }
                if (fOneFileSelect)
                    FillCurFile(hDlg);
            }

          return TRUE;

      case WM_COMMAND:
            {
                WORD wId = GET_WM_COMMAND_ID(wParam,lParam);

                switch (wId)
                {
                    case IDC_GETHELP:
                    {
                        /*
                        DoHelpContents(hDlg,// *pADDCONTENTDFSDLGParam->pGuiItem,
                                    "::/Ordering_SmartVersion.htm");
                                    */
                        return TRUE;
                    }

                    if (wId == IDOK)
                    {
                    }

                    case IDC_FILEPROPPREVIOUSBUTTON:
                        if (fOneFileSelect)
                        {
                            dfuLong32 dfNumFile;
                            BOOL fIdentical;
                            BOOL fPrevFound = GetPreviousPositionOfFile(dfCurVersion,dfCurFile,dfNumFile,fIdentical);
                            if (fPrevFound)
                            {
                                dfCurVersion--;
                                dfCurFile=dfNumFile;
                                FillCurFile(hDlg);
                            }
                        }
                        break;

                    case IDC_FILEPROPPREVIOUSMODIFIEDBUTTON:
                        if (fOneFileSelect)
                        {
                            dfuLong32 dfNumVer,dfNumFile;
                            BOOL fPrevFoundModified = GetPreviousPositionOfFileNotIdentical(dfCurVersion,dfCurFile,dfNumVer,dfNumFile);
                            if (fPrevFoundModified)
                            {
                                dfCurFile=dfNumFile;
                                dfCurVersion=dfNumVer;
                                FillCurFile(hDlg);
                            }
                        }
                        break;

                    case IDC_FILEPROPNEXTBUTTON:
                        if (fOneFileSelect)
                        {
                            dfuLong32 dfNumFile;
                            BOOL fIdentical;
                            BOOL fNextFound = GetNextPositionOfFile(dfCurVersion,dfCurFile,dfNumFile,fIdentical);
                            if (fNextFound)
                            {
                                dfCurVersion++;
                                dfCurFile=dfNumFile;
                                FillCurFile(hDlg);
                            }
                        }
                        break;

                    case IDC_FILEPROPNEXTMODIFIEDBUTTON:
                        if (fOneFileSelect)
                        {
                            dfuLong32 dfNumVer,dfNumFile;
                            BOOL fNextFoundModified = GetNextPositionOfFileNotIdentical(dfCurVersion,dfCurFile,dfNumVer,dfNumFile);
                            if (fNextFoundModified)
                            {
                                dfCurFile=dfNumFile;
                                dfCurVersion=dfNumVer;
                                FillCurFile(hDlg);
                            }
                        }
                        break;

                    case IDOK:
                    case IDCANCEL:
                    {
                        if (wId == IDOK)
                        {
                        }

                        EndDialog(hDlg, (wId == IDOK));
                        return (TRUE);
                    }
                }
            }
            break;

    }
    return FALSE;
}

BOOL DoFileProperties(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,LRUMENU &lrum)
{
    BOOL fRet;
    FILEPROPERTIESDLGPARAM FilePropertiesDlgParam;
    FILEPROPERTIESPROCANDPARAM FilePropertiesDlgProcAndParam(guiItem,DfsFileAndInfo,&FilePropertiesDlgParam);
    fRet = (BOOL)DialogBoxParam(ghInstRes,
                           MAKEINTRESOURCE(IDD_FILEPROPERTIES),
                           guiItem.GetHwndMain(),
                           (DLGPROC)DlgProcAndParamDlgProc,(LPARAM)(&FilePropertiesDlgProcAndParam));
    return fRet;
}


/****************************************************************************/
/****************************************************************************/

BOOL DoInsertVersionZipFile(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,LRUMENU &lrum,LPTSTR lpszZipFile)
{
TCHAR szFilter[MAX_PATH*2]="";
MYOPENFILENAME ofn;
FILETOADDARRAY ftaArray;
TCHAR szZipFileName[MAX_PATH+1]="";
BOOL fRet=TRUE;


    if (lpszZipFile==NULL)
    {
        InitOpenFileName((MYOPENFILENAME*)&ofn,guiItem.GetHwndMain(),IDS_TYPEOPENZIP,szFilter,sizeof(szFilter)-1,
                            szZipFileName,MAX_PATH,NULL,0,0);
        if (!GetOpenFileName((OPENFILENAME*)&ofn))
            return FALSE;
        lpszZipFile = szZipFileName;
    }

    ClearFileToAddArrayWithDelete(&ftaArray,FALSE,FALSE);
    if (!BuildFtaArrayFromZipfile(&ftaArray,lpszZipFile))
    {
        ClearFileToAddArrayWithDelete(&ftaArray,TRUE,TRUE);
        return FALSE;
    }
    fRet=DoInsertVersion(guiItem,DfsFileAndInfo,lrum,ftaArray.pFileToAdd,ftaArray.dfNbFileToAdd);
    ClearFileToAddArrayWithDelete(&ftaArray,TRUE,TRUE);
    return fRet;
}

BOOL DoNewDfsZipFile(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,LRUMENU &lrum,LPTSTR lpszZipFile)
{
TCHAR szFilter[MAX_PATH*2]="";
MYOPENFILENAME ofn;
FILETOADDARRAY ftaArray;
TCHAR szZipFileName[MAX_PATH+1]="";
BOOL fRet=TRUE;

    if (lpszZipFile==NULL)
    {
        InitOpenFileName((MYOPENFILENAME*)&ofn,guiItem.GetHwndMain(),IDS_TYPEOPENZIP,szFilter,sizeof(szFilter)-1,
                            szZipFileName,MAX_PATH,NULL,0,0);
        if (!GetOpenFileName((OPENFILENAME*)&ofn))
            return FALSE;
        lpszZipFile = szZipFileName;
    }

    ClearFileToAddArrayWithDelete(&ftaArray,FALSE,FALSE);
    if (!BuildFtaArrayFromZipfile(&ftaArray,lpszZipFile))
    {
        ClearFileToAddArrayWithDelete(&ftaArray,TRUE,TRUE);
        return FALSE;
    }
    fRet=DoNewDfs(guiItem,DfsFileAndInfo,lrum,ftaArray.pFileToAdd,ftaArray.dfNbFileToAdd);
    ClearFileToAddArrayWithDelete(&ftaArray,TRUE,TRUE);
    return fRet;
}

/********************************************************************************************/

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

/* wdifstrm.cpp */


#include <windows.h>            // required for all Windows applications
#include <windowsx.h>
#include <shlobj.h>



#include "resource.h"
#define S_EXTERN
#include "global.h"            // prototypes specific to this application


#include "ltoolsCPP.h"
#include "miscutil.h"
#include "ExtInfo.h"
#include "GuiItem.h"
#include "../../lib/engine/svfile/common/DirSet.h"
#include "RegCode.h"
#include "DoExtracting.h"
#include "uiMain.h"
#include "SaveParam.h"
#include "../../lib/engine/patchstream/common/DfsOrigMemoryMap.h"
#include "../../lib/engine/patchstream/common/DfsIoHlp.h"
#include "../../cli/DfsCdLin.h"

#include "IOConfig.h"

#include "zlib.h"
#include "zlib.h"
#include "unzip.h"

extern "C"
{
#include "iowin32.h"
}


//#include <uxTheme.h>
#define NOVERSIONRC "2.0"

char szAppName[256];              // The name of this application
char szTitle[256];               // The title bar text


WNDPROC lpOldHeaderProc=NULL;
HHOOK MessageFilterHook = NULL;

HBITMAP hBmpToolbar = NULL;
BOOL fGlobalHideRegisterOrUninstall=FALSE;




typedef struct
{
BOOL fInternal;
TCHAR szFileNameRes[MAX_PATH]; // = "" for internal
WORD wLANGID;  // FOR Win32 API
TCHAR szWinInisLangage[30]; // For saving in WinImage pref and Win 16 WIN.INI
TCHAR szSign[20];
TCHAR szNameLang[80];
} LANGAGEINFORMATION;
typedef LANGAGEINFORMATION FAR * LPLANGAGEINFORMATION;

LPLANGAGEINFORMATION lpli=NULL;
WORD wNbLang;
WORD wCur;

void AddLang(LPLANGAGEINFORMATION lpNewLang)
{
  lpli = (LPLANGAGEINFORMATION)
      ReallocOrAlloc(lpli,(wNbLang+1)*sizeof(LANGAGEINFORMATION));
  *(lpli+wNbLang) = *lpNewLang;
  wNbLang++;
}

void AddInternal(WORD wIndexStr)
{
LANGAGEINFORMATION li;
TCHAR szTxt[50];

  li.szNameLang[0] = '\0';
  li.fInternal = TRUE;

  if (LoadString(ghInst,wIndexStr + IDS_NAMELANGAGE,
           li.szNameLang,sizeof(li.szNameLang)-1)==0)
            return;
  LoadString(ghInst,wIndexStr + IDS_WININISLANGAGE,
           li.szWinInisLangage,sizeof(li.szWinInisLangage)-1);
  LoadString(ghInst,wIndexStr + IDS_SIGN,
           li.szSign,sizeof(li.szSign)-1);
  LoadString(ghInst,wIndexStr + IDS_LANGID,szTxt,sizeof(szTxt)-1);
  li.wLANGID = (WORD)atoi(szTxt);
  AddLang(&li);
}

void AddDll(LPSTR lpFn)
{
LANGAGEINFORMATION li;
HINSTANCE hInstDll;
//BOOL fAdd = TRUE;
TCHAR szTxt[50];
BOOL fVersionDllCompatible;

  if (GetWin32Kind() == WIN32S)
    hInstDll = LoadLibrary(lpFn);
   else
    hInstDll = LoadLibraryEx(lpFn,NULL,DONT_RESOLVE_DLL_REFERENCES/*|LOAD_LIBRARY_AS_DATAFILE*/);

  if (hInstDll == NULL) return ;
  lstrcpy(li.szFileNameRes,lpFn);
  li.fInternal = FALSE;

  LoadString(hInstDll,IDS_NAMELANGAGE,
           li.szNameLang,sizeof(li.szNameLang)-1);
  LoadString(hInstDll,IDS_WININISLANGAGE,
           li.szWinInisLangage,sizeof(li.szWinInisLangage)-1);
  LoadString(hInstDll,IDS_SIGN,
           li.szSign,sizeof(li.szSign)-1);
  LoadString(hInstDll,IDS_LANGID,szTxt,sizeof(szTxt)-1);
  li.wLANGID = (WORD)atoi(szTxt);
  LoadString(hInstDll,IDS_SMVVERSION,szTxt,sizeof(szTxt)-1);
  fVersionDllCompatible = FALSE;
  if (_fstrcmp(szTxt,NOVERSIONRC) == 0)
      fVersionDllCompatible=TRUE;

#ifdef NOVERSIONRC_COMPAT0
  if (_fstrcmp(szTxt,NOVERSIONRC_COMPAT0) == 0)
      fVersionDllCompatible=TRUE;
#endif
#ifdef NOVERSIONRC_COMPAT1
  if (_fstrcmp(szTxt,NOVERSIONRC_COMPAT1) == 0)
      fVersionDllCompatible=TRUE;
#endif
#ifdef NOVERSIONRC_COMPAT2
  if (_fstrcmp(szTxt,NOVERSIONRC_COMPAT2) == 0)
      fVersionDllCompatible=TRUE;
#endif
#ifdef NOVERSIONRC_COMPAT3
  if (_fstrcmp(szTxt,NOVERSIONRC_COMPAT3) == 0)
      fVersionDllCompatible=TRUE;
#endif
  FreeLibrary(hInstDll);
  if (fVersionDllCompatible)
      AddLang(&li);
}

BOOL InitLangInfo()
{
//LANGAGEINFORMATION li;
char szFN[MAX_PATH];
LPSTR lpName  ;
WIN32_FIND_DATA ffblk;
HANDLE hFind;
BOOL fResult=TRUE;



  ghInstRes = ghInst;
  lpli = NULL;
  wNbLang = 0;
  GetModuleFileName(ghInst,szFN,sizeof(szFN));
  lpName = (LPSTR)GetNameWithoutPath(szFN) ;
  AddInternal(0);


  lstrcpy(lpName,"smrtvs*.t32");

  if ((hFind = FindFirstFile(szFN,&ffblk)) != INVALID_HANDLE_VALUE)
       do
    {
      lstrcpy(lpName,ffblk.cFileName);
      AddDll(szFN);
    } while (FindNextFile(hFind,&ffblk));
  FindClose(hFind);
  return fResult;
}

void ReleaseLangInfo()
{
  GlobalFreePtr(lpli);
}

WORD GetNbLang()
{
  return wNbLang;
}

BOOL GetLangName(LPTSTR lpLN,WORD i)
{
  lstrcpy(lpLN,(lpli+i)->szNameLang);
  return TRUE;
}

LPCTSTR GetLangInisLang(DWORD i)
{
  return (lpli+i)->szWinInisLangage;
}

BOOL GetIndexForInisLang(DWORD* lpdwItem,LPCTSTR lpszLang)
{
    DWORD i;
    for (i=0;i<GetNbLang();i++)
        if (lstrcmpi(GetLangInisLang(i),lpszLang)==0)
        {
            *lpdwItem=i;
            return TRUE;
        }

    return FALSE;
}

DWORD GetBetterAutoLang()
{
  DWORD i;
  WORD wLangId = PRIMARYLANGID(GetUserDefaultLangID());
  for (i=0;i<GetNbLang();i++)
     if ((wLangId) == (lpli+i)->wLANGID)
       return i;
    return 0;
}

BOOL TryLoadLang(HWND hWndMain,DWORD dwIndexLang,BOOL fReplace)
{
HINSTANCE hNewInst;
LPLANGAGEINFORMATION lplil;

HMENU hMenuNew;
  if (dwIndexLang == LANGUI_SELECT_AUTO)
      dwIndexLang = GetBetterAutoLang();
  lplil = lpli + dwIndexLang ;

  if (lplil -> fInternal)
    {
      if ((ghInstRes != NULL) && (ghInstRes != ghInst))
        FreeLibrary(ghInstRes);
      ghInstRes = ghInst;

      //lstrcpy(szIntern,lplil -> szSign);

      wCur = (WORD)dwIndexLang;
    }
  else
  {
    if (GetWin32Kind() == WIN32S)
        hNewInst = LoadLibrary(lplil->szFileNameRes);
    else
        hNewInst = LoadLibraryEx(lplil->szFileNameRes,NULL,DONT_RESOLVE_DLL_REFERENCES/*|LOAD_LIBRARY_AS_DATAFILE*/);
    if (hNewInst == NULL) return FALSE;
    if ((ghInstRes != NULL) && (ghInstRes != ghInst))
        FreeLibrary(ghInstRes);
    ghInstRes = hNewInst;
  }


  wCur = (WORD)dwIndexLang;
  hMenuNew=LoadMenu(ghInstRes,MAKEINTRESOURCE(IDM_MAIN));
  if (hMenuNew != NULL)
  {
      HMENU hMenuOld=NULL;

      if (fReplace)
        hMenuOld=GetMenu(hWndMain);

      SetMenu(hWndMain,hMenuNew);
      if (hMenuOld!=NULL)
        DestroyMenu(hMenuOld);
  }
  SetMUResourceBase(ghInstRes,0);
  return TRUE;
}

/*
BOOL SetLangInfo(LPSTR lpLang)
{
WORD wLangId;
WORD i ;
  CnvMaj(lpLang);
  for (i=0;i<wNbLang;i++)
      if (_fstrcmp(lpLang,(lpli+i)->szSign) == 0)
    return TryLoadLang(i);

  wLangId = PRIMARYLANGID(GetUserDefaultLangID());
  for (i=0;i<wNbLang;i++)
     if ((wLangId) == (lpli+i)->wLANGID)
       return TryLoadLang(i);

  return TryLoadLang(0);
}
*/

WORD GetCurLang()
{
  return wCur;
}

BOOL GetLangInfo(LPTSTR lpLang)
{
  LoadInternatString(IDS_SIGN,lpLang,5);
  return TRUE;
}



/**********************************************************************/

/*
void RebuildMenu(HWND hWndMain,BOOL fReplacing)
{
  char szEntATOP[MAX_PATH];
  HMENU hOldMenu;
  HMENU hMenu;
  BOOL fAddReadCdItem;


  GetSystemMenu(hWndMain,TRUE);
  hMenuSys = GetSystemMenu(hWndMain,FALSE);

  hOldMenu = hMenu;
  if (fReplacing)
  {
  }
  hMenu = LoadMenu(ghInstRes,MAKEINTRESOURCE(IDM_MAIN));

  SetMenu(hWndMain,hMenu);
}
*/
/**********************************************************************/
/*
long CALLBACK HeaderWndProc(HWND hWnd, UINT message,
                         WPARAM wParam,LPARAM lParam)
{
long lr=0;
  if (lpOldHeaderProc!=NULL)
  {
      if (message==WM_DRAWITEM)
      {
          LPDRAWITEMSTRUCT lpdis=(LPDRAWITEMSTRUCT)lParam;
          return TRUE;
      }

      lr = CallWindowProc((WNDPROC)lpOldHeaderProc,hWnd,message,wParam,lParam);
  }
  return lr;
}*/

HWND WINAPI CreateListView(HWND hwndParent)
{
    HWND hwndLV;
    RECT rc;

    // Force the common controls DLL to be loaded.

    //InitCommonControls();

    // Create the control.

    GetClientRect(hwndParent, &rc);

    hwndLV = CreateWindowEx(WS_EX_CLIENTEDGE,
                            WC_LISTVIEW, "",WS_EX_CLIENTEDGE|
                            WS_VISIBLE | WS_CHILD | /*WS_BORDER | *//*LVS_EDITLABELS |*/
                            LVS_REPORT /*LVS_LIST*/| LVS_AUTOARRANGE | LVS_SHAREIMAGELISTS |
                            LVS_SHOWSELALWAYS |
                            LVS_OWNERDATA,
                            0, 0,
                            rc.right-rc.left, rc.bottom-rc.top,
                            hwndParent,
                            (HMENU)IDD_LISTVIEW,
                            ghInst,
                            NULL);
    /*
    if (hwndLV)
    {
        HWND hWndHeader=ListView_GetHeader(hwndLV);
        lpOldHeaderProc = (WNDPROC) GetWindowLong(hWndHeader,GWL_WNDPROC);
        SetWindowLong(hWndHeader,GWL_WNDPROC,(LONG)(FARPROC)HeaderWndProc);
    }*/

    return hwndLV;
}

#define NUM_BUTTONS (8+0)
#define ID_TOOLBAR   32000
#define IDM_STATUSBAR 32001

/*
HWND CreateAToolBarOld(HINSTANCE ghInst,HWND hWndParent)
{
   REBARINFO     rbi;
   REBARBANDINFO rbBand;
   //RECT          rc;
   HWND   hwndRB;
   DWORD  dwBtnSize;
   HWND hwndToolbar;
     //TBADDBITMAP tb;


   hwndRB = CreateWindowEx(WS_EX_TOOLWINDOW,
                           REBARCLASSNAME,
                           NULL,
                           WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|
                           WS_CLIPCHILDREN|RBS_VARHEIGHT|CCS_NODIVIDER,
                           0,0,0,0,
                           hWndParent,
                           NULL,
                           ghInst,
                           NULL);
   if(!hwndRB)
      return NULL;

   // Initialize and send the REBARINFO structure.
   rbi.cbSize = sizeof(REBARINFO);  // Required when using this struct.
   rbi.fMask  = 0;
   rbi.himl   = (HIMAGELIST)NULL;
   if(!SendMessage(hwndRB, RB_SETBARINFO, 0, (LPARAM)&rbi))
      return NULL;

   // Initialize structure members that both bands will share.
   rbBand.cbSize = sizeof(REBARBANDINFO);  // Required
   rbBand.fMask  = RBBIM_TEXT | RBBIM_STYLE | RBBIM_CHILD  | RBBIM_CHILDSIZE | RBBIM_SIZE;
   rbBand.fStyle = RBBS_CHILDEDGE;

   // Create the combo box control to be added.

    TBBUTTON tbButtons[] =
    {
    { 0,  IDM_FILE_NEW,       TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
    { 1,  IDM_FILE_OPEN,      TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
    { -1, 0,                  TBSTATE_ENABLED, TBSTYLE_SEP,    0L, 0},

    { 4, IDM_VIEW_LARGEICON,  TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
    { 5, IDM_VIEW_SMALLICONS, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
    { 6, IDM_VIEW_LIST,       TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
    { 7, IDM_VIEW_DETAILS,    TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
    { -1, 0,                  TBSTATE_ENABLED, TBSTYLE_SEP,    0L, 0},

  };

  // retrieve the appropriate bitmap and the correct values for size...
  int cxItem  = 24;
  int cyItem  = 24;
  int cBitmap = 480;
  int cItem   = sizeof(tbButtons) / sizeof(TBBUTTON);

  HBITMAP hbmp = (HBITMAP)::LoadImage(ghInst, MAKEINTRESOURCE(IDB_TOOLBAR_LARGE_TRUECOLOR),
                                      IMAGE_BITMAP, 480, cyItem, 0);

  hwndToolbar = ::CreateToolbarEx(hWndParent, WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS,
                                  ID_TOOLBAR, cItem, NULL, (UINT_PTR)hbmp, tbButtons, cItem,
                                  0, 0, 24, 24, sizeof(TBBUTTON));

   // Get the height of the toolbar.
   dwBtnSize = SendMessage(hwndToolbar, TB_GETBUTTONSIZE, 0,0);

   // Set values unique to the band with the toolbar.
   rbBand.lpText     = "ToolBar";
   rbBand.hwndChild  = hwndToolbar;
   rbBand.cxMinChild = LOWORD(dwBtnSize);
   rbBand.cyMinChild = HIWORD(dwBtnSize);
   rbBand.cx         = 1400;

   // Add the band that has the toolbar.
   SendMessage(hwndRB, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);
   return hwndToolbar;
}*/



HWND CreateAToolBar(HINSTANCE ghInst,HWND hWndParent)
{
   REBARINFO     rbi;
   REBARBANDINFO rbBand;
   //RECT          rc;
   HWND   hwndRB;
   DWORD  dwBtnSize;
   HWND hwndToolbar;
     //TBADDBITMAP tb;

   hwndRB = CreateWindowEx(WS_EX_TOOLWINDOW,
                           REBARCLASSNAME,
                           NULL,
                           WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|
                           WS_CLIPCHILDREN|RBS_VARHEIGHT|CCS_NODIVIDER,
                           0,0,0,0,
                           hWndParent,
                           NULL,
                           ghInst,
                           NULL);
   if(!hwndRB)
      return NULL;

   // Initialize and send the REBARINFO structure.
   rbi.cbSize = sizeof(REBARINFO);  // Required when using this struct.
   rbi.fMask  = 0;
   rbi.himl   = (HIMAGELIST)NULL;
   if(!SendMessage(hwndRB, RB_SETBARINFO, 0, (LPARAM)&rbi))
      return NULL;

   // Initialize structure members that both bands will share.
   rbBand.cbSize = sizeof(REBARBANDINFO);  // Required
   rbBand.fMask  = RBBIM_TEXT | RBBIM_STYLE | RBBIM_CHILD  | RBBIM_CHILDSIZE | RBBIM_SIZE;
   rbBand.fStyle = RBBS_CHILDEDGE;

   // Create the combo box control to be added.

    TBBUTTON tbButtons[] =
    {
    { 0,  IDM_FILE_NEW,       TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
    { 1,  IDM_FILE_OPEN,      TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},

    { -1, 0,                  TBSTATE_ENABLED, TBSTYLE_SEP,    0L, 0},
    { -1, 0,                  TBSTATE_ENABLED, TBSTYLE_SEP,    0L, 0},
    { 13, IDM_VERSION_EXTRACT,    TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
    { 14, IDM_VERSION_ADDNEWVERSION,    TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},

    { -1, 0,                  TBSTATE_ENABLED, TBSTYLE_SEP,    0L, 0},
    { -1, 0,                  TBSTATE_ENABLED, TBSTYLE_SEP,    0L, 0},

    { 4, IDM_VIEW_LARGEICON,  TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
    { 5, IDM_VIEW_SMALLICONS, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
    { 6, IDM_VIEW_LIST,       TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
    { 7, IDM_VIEW_DETAILS,    TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},

  };

  // retrieve the appropriate bitmap and the correct values for size...
  int cxItem  = 24;
  int cyItem  = 24;
  int cBitmap = 480;
  int cItem   = sizeof(tbButtons) / sizeof(TBBUTTON);

  // load the bitmap and create an image list
  HBITMAP     hbmp ;
  HIMAGELIST  hil  = ImageList_Create(cxItem, cyItem, ILC_COLOR24 | ILC_MASK, cItem, 0);

  if (hBmpToolbar != NULL)
      DeleteBitmap(hBmpToolbar);
  hbmp = hBmpToolbar = (HBITMAP)::LoadImage(ghInst, MAKEINTRESOURCE(IDB_TOOLBAR_LARGE_TRUECOLOR), IMAGE_BITMAP, 480, cyItem, 0);
  ImageList_AddMasked(hil, hbmp, RGB(0, 0, 0));

  hwndToolbar = ::CreateToolbarEx(hWndParent, WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS,
                                  ID_TOOLBAR, cItem, NULL, NULL, tbButtons, cItem,
                                  0, 0, 24, 24, sizeof(TBBUTTON));

  SendMessage(hwndToolbar, TB_SETIMAGELIST, 0, (LPARAM)hil);

   // Get the height of the toolbar.
   dwBtnSize = (DWORD)SendMessage(hwndToolbar, TB_GETBUTTONSIZE, 0,0);

   // Set values unique to the band with the toolbar.
   rbBand.lpText     = "ToolBar";
   rbBand.hwndChild  = hwndToolbar;
   rbBand.cxMinChild = LOWORD(dwBtnSize);
   rbBand.cyMinChild = HIWORD(dwBtnSize);
   rbBand.cx         = 1400;

   // Add the band that has the toolbar.
   SendMessage(hwndRB, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);
   return hwndToolbar;
}


/*
// francl -------------------------------------------------------------------
    HWND hwndToolbar;
    TBADDBITMAP tb;
    int index;
    BOOL fLargeIcon = TRUE;

    TBBUTTON tbButtons[] =
    {
    { 0, IDM_FILE_NEW,  TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
    { 1, IDM_FILE_OPEN, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
  };

  // retrieve the appropriate bitmap and the correct values for size...
  int cxItem  = 24;
  int cyItem  = 24;
  int cBitmap = 480;
  int cItem   = sizeof(tbButtons) / sizeof(TBBUTTON);

  HBITMAP hbmp = (HBITMAP)::LoadImage(ghInst, MAKEINTRESOURCE(IDB_TOOLBAR_LARGE_TRUECOLOR),
                                      IMAGE_BITMAP, 480, cyItem, LR_LOADTRANSPARENT);

  hwndToolbar = ::CreateToolbarEx(hWndParent, WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS,
                                  ID_TOOLBAR, cItem, NULL, (UINT_PTR)hbmp, tbButtons, cItem,
                                  0, 0, 24, 24, sizeof(TBBUTTON));
  return hwndToolbar;
// francl --- end
*/
/*
    TBADDBITMAP tb;
    int index, stdidx,stdidx2;
    BOOL fLargeIcon=FALSE;

    // Toolbar buttons
    TBBUTTON tbButtons [ ] =
    {
    {STD_FILENEW, IDM_FILE_NEW, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
    {STD_FILEOPEN, IDM_FILE_OPEN, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},

//  {STD_FILESAVE, IDM_FILE_CLOSE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
    {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
    {VIEW_LARGEICONS, IDM_VIEW_LARGEICON, TBSTATE_ENABLED, TBSTYLE_BUTTON,
    0L, 0},
    {VIEW_SMALLICONS, IDM_VIEW_SMALLICONS, TBSTATE_ENABLED, TBSTYLE_BUTTON,
    0L, 0},
    {VIEW_LIST, IDM_VIEW_LIST, TBSTATE_ENABLED, TBSTYLE_BUTTON,
    0L, 0},
    {VIEW_DETAILS, IDM_VIEW_DETAILS, TBSTATE_ENABLED, TBSTYLE_BUTTON,
    0L, 0},
    {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
//  {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
    };

    TBBUTTON tbButtons2 [ ] =
    {
    {0, IDM_VERSION_EXTRACT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
    {1, IDM_VERSION_ADDNEWVERSION, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
    };

    // Create the toolbar and add the first three buttons and a separator.
    hWndToolbar = CreateToolbarEx (hWndParent,
    WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS,
    ID_TOOLBAR, 11, (HINSTANCE)HINST_COMMCTRL,
    fLargeIcon ? IDB_STD_LARGE_COLOR : IDB_STD_SMALL_COLOR,
    (LPCTBBUTTON)&tbButtons, 4-1, 0, 0, 100, 30, sizeof (TBBUTTON));

    // Add the next four buttons
    tb.hInst = HINST_COMMCTRL;
    tb.nID = fLargeIcon ? IDB_VIEW_LARGE_COLOR : IDB_VIEW_SMALL_COLOR;
    stdidx = SendMessage (hWndToolbar, TB_ADDBITMAP, 12, (LPARAM)&tb);
    tb.hInst = ghInst;

    tb.nID = fLargeIcon ? IDB_LARGE_TOOLBAR_BITMAP : IDB_TOOLBAR_BITMAP;
    stdidx2 = SendMessage (hWndToolbar, TB_ADDBITMAP, 2, (LPARAM)&tb);

    for (index = 4-1; index < NUM_BUTTONS; index++)
      tbButtons[index].iBitmap += stdidx;

    SendMessage (hWndToolbar, TB_ADDBUTTONS, 4+1, (LONG) &tbButtons[4-1]);


    for (index = 0; index < 2; index++)
      tbButtons2[index].iBitmap += stdidx2;

    SendMessage (hWndToolbar, TB_ADDBUTTONS, 2, (LONG) &tbButtons2[0]);


    return hWndToolbar;
*/

void InitializeStatusBar(HWND hWndStatusbar,HWND hwndParent)
{
    const unsigned int cSpaceInBetween = 8;
    int   ptArray[3];   // Array defining the number of parts/sections
    SIZE  size;         // the Status bar will display.
    RECT  rect;
    HDC   hDC;

   /*
    * Fill in the ptArray...
    */

    hDC = GetDC(hwndParent);
    GetClientRect(hwndParent, &rect);

    ptArray[2] = rect.right;


    if (GetTextExtentPoint(hDC, "123,123", 8, &size))
        ptArray[1] = ptArray[2] - (size.cx) - cSpaceInBetween;
    else
        ptArray[1] = 0;

    if (GetTextExtentPoint(hDC, "Cursor Pos:", 12, &size))
        ptArray[0] = ptArray[1] - (size.cx) - cSpaceInBetween;
    else
        ptArray[0] = 0;

    ReleaseDC(hwndParent, hDC);

    SendMessage(hWndStatusbar,
                SB_SETPARTS,
                sizeof(ptArray)/sizeof(ptArray[0]),
                (LPARAM)(LPINT)ptArray);
/*
    UpdateStatusBar(guiItem.hwndSB,szAppName, 0, 0);
    UpdateStatusBar(guiItem.hwndSB,"Cursor Pos:", 1, SBT_POPOUT);
    UpdateStatusBar(guiItem.hwndSB,"", 3, SBT_POPOUT);*/
}


HWND CreateStatusBar(HINSTANCE,HWND hwndParent)
{

    HWND hWndStatusbar ;
    hWndStatusbar = CreateStatusWindow(WS_CHILD | WS_VISIBLE /*| WS_BORDER*/,
                                       szAppName,
                                       hwndParent,
                                       IDM_STATUSBAR);
    if(hWndStatusbar)
    {
        InitializeStatusBar(hWndStatusbar,hwndParent);
        //SetTimer(hwndParent, IDM_TIMER, TIMER_TIMEOUT, NULL);
        return hWndStatusbar ;
    }

    return NULL;
}

HWND CreateTreeView(HWND hwndParent, int iID, DWORD dwStyle)
{
    RECT rc;      // Client rect of parent
    HWND hwnd;    // Handle to TreeView
    SHFILEINFO    sfi;
    HIMAGELIST    hImageList=NULL ;         // Handle to systemn ImageList.

    // This registers the TreeView class.

    //InitCommonControls();

    // Get the client area of the parent.

    GetClientRect(hwndParent, &rc);

    // Create the TreeView control.

    hwnd = CreateWindowEx(WS_EX_CLIENTEDGE,         // Ex style
                          WC_TREEVIEW,              // Class
                          "",                       // Dummy Text
                          dwStyle,                  // Style
                          0, 0,                     // Use all of
                          rc.right/2, rc.bottom,    // client area.
                          hwndParent,               // Parent
                          (HMENU)(DWORD_PTR)iID,               // ID
                          ghInst,                    // Instance
                          NULL);                    // No extra

    // Grab a handle to the system image list, for our icons

    memset(&sfi,0,sizeof(SHFILEINFO));

    hImageList = (HIMAGELIST)SHGetFileInfo((LPCTSTR)"C:\\",
                                           FILE_ATTRIBUTE_DIRECTORY,
                                           &sfi,
                                           sizeof(SHFILEINFO),
                                           SHGFI_SYSICONINDEX | SHGFI_SMALLICON  | SHGFI_USEFILEATTRIBUTES);

    // Attach ImageList to TreeView

    if (hImageList && hwnd)
        TreeView_SetImageList(hwnd, hImageList, 0);

    // Return the window handle

    return hwnd;
}





/*********************************************************************************************/


void IsSmartVersionExeRegistrySet(BOOL & fActualVersionSet,BOOL & fPreviousVersionSet)
{
    TCHAR szKeyValue[MAX_PATH+1];
    long lSize = MAX_PATH;
    fActualVersionSet = fPreviousVersionSet = FALSE;
    if (RegQueryValue(HKEY_CLASSES_ROOT,"SmartVersion", szKeyValue,&lSize)==ERROR_SUCCESS)
        if (lSize>0)
            fActualVersionSet = fPreviousVersionSet = TRUE;
}

HKEY GetSmartVersionUnInstallKeyBase()
{
    BOOL fNoNt = (GetWin32Kind() != WINNT);
    if (fNoNt)
        return HKEY_LOCAL_MACHINE;
    else
        return GetSmartVersionKeyBase();
}



void RegisterSmartVersionExe()
{
    TCHAR szBuff[MAX_PATH+0x80];
    TCHAR szAppName[MAX_PATH+0x80];


    GetModuleFileName(NULL,((LPSTR)szAppName),MAX_PATH);

    szBuff[0]='"';

    GetModuleFileName(NULL,((LPSTR)szBuff)+1,MAX_PATH);
    wsprintf(szBuff,"\"%s\" \"%%1\"",szAppName);

    RegSetValue(HKEY_CLASSES_ROOT, "SmartVersion", REG_SZ, "SmartVersion", 0);
    RegSetValue(HKEY_CLASSES_ROOT, "SmartVersion\\shell\\open\\command",
     REG_SZ, szBuff, 0);
    GetModuleFileName(NULL,szBuff,MAX_PATH);
    wsprintf(szBuff,"%s,0",szAppName);
    RegSetValue(HKEY_CLASSES_ROOT, "SmartVersion\\DefaultIcon",
       REG_SZ, szBuff, 0);

    /*
    wsprintf(szBuff,"\"%s\" /e \"%%1\"",szAppName);
    RegSetValue(HKEY_CLASSES_ROOT, "SmartVersion\\shell\\Extract\\command",
     REG_SZ, szBuff, 0);
    */


            /***/

    RegSetValue(HKEY_CLASSES_ROOT, ".svf", REG_SZ, "SmartVersion", 0);

    {
    char szUninsKey[256+32];
        DWORD dwDisp;
        HKEY hCurKey;
        DWORD dw1=1;

        szUninsKey[0]='"';
        GetModuleFileName(NULL,szUninsKey+1,sizeof(szUninsKey)-24);
        lstrcat(szUninsKey,"\" /uninstall");
        RegCreateKeyEx(GetSmartVersionUnInstallKeyBase(),
                "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\SmartVersion",
                0,NULL,
                REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hCurKey,&dwDisp);

        RegSetValueEx(hCurKey,
            "DisplayName",
            0,REG_SZ,(LPBYTE)"SmartVersion",sizeof("SmartVersion"));

        RegSetValueEx(hCurKey,
            "UninstallString",
            0,REG_SZ,(LPBYTE)szUninsKey,lstrlen(szUninsKey)+1);

        RegSetValueEx(hCurKey,
            "NoModify",
            0,REG_DWORD,(LPBYTE)&dw1,sizeof(DWORD));
        RegSetValueEx(hCurKey,
            "NoRepair",
            0,REG_DWORD,(LPBYTE)&dw1,sizeof(DWORD));


        RegCloseKey(hCurKey);
    }

    //WriteWimaInt("SmartVersionVersion",ACTUALVERSION);
}


BOOL RegDeleteKeyAssociation(LPCSTR lpszAssociation)
{
    char szKeyName[MAX_PATH+1] = "";
    long lSize = sizeof(szKeyName)-1;
    RegQueryValue(HKEY_CLASSES_ROOT,lpszAssociation,szKeyName,&lSize);
    if (lstrlen(szKeyName)>0)
        if ((lstrcmpi(szKeyName,"SmartVersion")!=0) && (lstrcmpi(szKeyName,"DifStream")!=0))
            return FALSE;

    RegDeleteKeyNT(HKEY_CLASSES_ROOT, lpszAssociation);
    return TRUE;
}

#define REGKEY_UNINSTALL_ROOT  ("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall")

char* gszSmartVersionFiles[] =
{
    "ORDER.URL",
    "WHATSNEW.TXT",
    "FILE_ID.DIZ",
    "WASNEU.TXT",
    "QUOINEUF.TXT",
    "README.TXT",
    "AFAS.TXT",
    "Afas.chm",
    "Auteur.txt",
    "VENDOR.DOC",
    "WHATNEWS.TXT",
    "wdifstrm.chm",
    "wdifstrm.exe",
    "difstrm.exe",
    "smartvs.chm",
    "smartvs.exe",
    "smartvs.url",
    "wsmartvs.exe",
    "wsmartvs.chm",
    "smartvs.url",
    "smv.exe",
    "smartversion.exe",
    "smartversion.chm",
    "smartvs.exe",
    "smartvs.chm"

};

BOOL DoUninstalling()
{
    TCHAR szPath[(MAX_PATH+1)*2];
    LPSTR pch;
    LPSTR pchEOP;
    int i,cFiles;
    HKEY hKey;




    {
      if(ERROR_SUCCESS == RegOpenKeyEx(GetSmartVersionUnInstallKeyBase(),
            "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\SmartVersion", 0,
            KEY_ALL_ACCESS, &hKey))
        {
            RegCloseKey(hKey);
            RegDeleteKeyNT(GetSmartVersionUnInstallKeyBase(),"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\SmartVersion");
        }
      else
      {
          if (GetSmartVersionUnInstallKeyBase() != HKEY_LOCAL_MACHINE)
          {
            if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\SmartVersion", 0,
                    KEY_ALL_ACCESS, &hKey))
                {
                    RegCloseKey(hKey);
                    RegDeleteKeyNT(HKEY_LOCAL_MACHINE,"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\SmartVersion");
                }
          }
      }
    }

    RegDeleteKeyAssociation(".svf");

    RegDeleteKeyNT(HKEY_CLASSES_ROOT, "SmartVersion");




    GetModuleFileName(NULL, szPath, MAX_PATH);
    pch = pchEOP = szPath;
    while(*pch)
    {
        if(('\\') == *pch)
            pchEOP = (pch)+1;

        pch =CharNext(pch);
    }
    *pchEOP = ('\0');


    if (RegOpenKeyEx(GetSmartVersionKeyBase(),"Software\\SmartVersion",0,
                            KEY_READ,&hKey)
                            == ERROR_SUCCESS)
    {
        int iCount;
        char szKey[32];
        DWORD dwType;
        DWORD dwSizeBuf=255;
        char  szReadKey[256];
        DWORD dwSizeReadKey;

        dwSizeReadKey = sizeof(szReadKey)-1;
        if (RegQueryValueEx(hKey,"NumberOfDeleteWhenUninstall",NULL,&dwType,(LPBYTE)szReadKey,&dwSizeReadKey) ==
                  ERROR_SUCCESS)
        {
            int iMax=0;

            if (dwType==REG_SZ)
                iMax=atoi(szReadKey);

            if (dwType==REG_DWORD)
                iMax=(int) *((DWORD*)szReadKey);

            for (iCount=0;iCount<iMax;iCount++)
            {
                wsprintf(szKey,"DeleteWhenUninstall%u",(DWORD)iCount);
                dwSizeReadKey = sizeof(szReadKey)-1;
                if (RegQueryValueEx(hKey,szKey,NULL,&dwType,(LPBYTE)szReadKey,&dwSizeReadKey) !=
                      ERROR_SUCCESS)
                      break;

                SetFileAttributes(szReadKey, FILE_ATTRIBUTE_NORMAL);
                DeleteFile(szReadKey);
            }

            dwSizeReadKey = sizeof(szReadKey)-1;
            if (RegQueryValueEx(hKey,"RemoveDirWhenUninstall",NULL,&dwType,(LPBYTE)szReadKey,&dwSizeReadKey) ==
                  ERROR_SUCCESS)
                  RemoveDirectory(szReadKey);
        }

        for (iCount=0;;iCount++)
        {
            wsprintf(szKey,"SmartVersionFile%d",iCount);

            if (RegQueryValueEx(hKey,szKey,NULL,&dwType,(LPBYTE)pchEOP,&dwSizeBuf) !=
                  ERROR_SUCCESS)
                  break;

            SetFileAttributes(szPath, FILE_ATTRIBUTE_NORMAL);
            DeleteFile(szPath);
        }

        RegDeleteKeyNT(hKey, "SmartVersion");
        RegCloseKey(hKey);
    }

    RegDeleteKeyNT(GetSmartVersionKeyBase(),"Software\\SmartVersion");


    // delete files
    cFiles = sizeof(gszSmartVersionFiles) / sizeof(char *);
    for(i = 0; i < cFiles; i++)
    {
        strcpy(pchEOP, gszSmartVersionFiles[i]);

        // ensure read-only and hidden attributes are removed and then remove
        SetFileAttributes(szPath, FILE_ATTRIBUTE_NORMAL);
        DeleteFile(szPath);
    }

    char szDel[256];
    strcpy(pchEOP,"smrtvs*.*");
    strcpy(szDel,szPath);
    HANDLE hFind;
    WIN32_FIND_DATA FindFileData;
    #define MAXNBFILE 254
    LPSTR lpDelFN = (LPSTR)GlobalAllocPtr(GHND,(MAXNBFILE+1)*256);
    int j,iCount=0;

    hFind=FindFirstFile(szDel,&FindFileData);
    if (hFind!=INVALID_HANDLE_VALUE)
    {
        do
        {
            strcpy(pchEOP,FindFileData.cFileName);
            strcpy(lpDelFN+(iCount*256),szPath);
            iCount++;
            //SetFileAttributes(szPath, FILE_ATTRIBUTE_NORMAL);
            //DeleteFile(szPath);
        } while (FindNextFile(hFind,&FindFileData) && (iCount<MAXNBFILE));
        FindClose(hFind);

        for (j=0;j<iCount;j++)
        {
            LPSTR lpCur = lpDelFN+(j*256);
            SetFileAttributes(lpCur, FILE_ATTRIBUTE_NORMAL);
            DeleteFile(lpCur);
        }
        GlobalFreePtr(lpDelFN);
    }


    //fExit=TRUE;
    //iRet=0;

/*
    if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_UNINSTALL_ROOT, 0,
        KEY_ALL_ACCESS, &hKey))
    {
        RegDeleteKeyNT(hKey, "SmartVersion");
        RegCloseKey(hKey);
    }*/

    DeleteExecutableBF();

    return TRUE;
}
/**************************************************************************/
/**************************************************************************/

LRESULT CALLBACK HelpMessageFilterHook(int nCode,WPARAM wParam,LPARAM lParam)
{
LPMSG lpMsg = (LPMSG)lParam;
    if (nCode==MSGF_DIALOGBOX)
        if (lpMsg->message==WM_KEYDOWN && lpMsg->wParam==VK_F1)
        {
            HWND hTemp=NULL;
            HWND hParent=lpMsg->hwnd;

            while (hParent != NULL)
            {
                hTemp=hParent;
                if (!(GetWindowLong(hTemp, GWL_STYLE) & WS_CHILD))
                    break;
                hParent=GetParent(hParent);
            }

            if (hTemp)
                PostMessage(hTemp,WM_COMMAND,
                    GET_WM_COMMAND_MPS (IDC_GETHELP,hTemp,0));
            return TRUE;
        }
    return CallNextHookEx(MessageFilterHook, nCode, wParam, (LPARAM)lpMsg);
}


/***********************************************************************/

class WNDMAIN
{
  public:
       WNDMAIN(HWND,LPCREATESTRUCT) ;
       ~WNDMAIN() ;
       LRESULT WndProc(HWND,UINT,WPARAM,LPARAM);
  private:
       LRESULT MsgCreate(HWND,UINT,WPARAM,LPARAM);
       LRESULT MsgDestroy(HWND,UINT,WPARAM,LPARAM);
       LRESULT MsgSize(HWND,UINT,WPARAM,LPARAM);
       LRESULT MsgMouseMove(HWND,UINT,WPARAM,LPARAM);
       LRESULT MsgLButtonDown(HWND,UINT,WPARAM,LPARAM);
       LRESULT MsgLButtonUp(HWND,UINT,WPARAM,LPARAM);
       LRESULT MsgCommand(HWND,UINT,WPARAM,LPARAM);
       LRESULT MsgDropFiles(HWND,UINT,WPARAM,LPARAM);
       LRESULT MsgNotify(HWND,UINT,WPARAM,LPARAM);
       LRESULT MsgContextMenu(HWND hwnd, UINT uMessage, WPARAM wparam, LPARAM lparam);
       LRESULT MsgDrawItem(HWND,UINT,WPARAM,LPARAM);
       LRESULT MsgMenuSelect(HWND,UINT,WPARAM,LPARAM);
       LRESULT MsgSetFocus(HWND,UINT,WPARAM,LPARAM);
       LRESULT MsgPaint(HWND,UINT,WPARAM,LPARAM);

       BOOL    GetItemHigh(HWND hwnd,int &iTVLVHigh,int &iSBHigh, int &iTbHigh);
       BOOL    DoResize(HWND,BOOL fSetNewSplit=FALSE,int iNewSplit=0);
       BOOL    DoPaintWndItem(HWND hwnd,HDC hdc);

       BOOL    DoSavePref();
       BOOL    DoLoadPref();

       LRESULT DoNotifyTips(HWND hwnd, UINT uMessage, WPARAM wparam, LPARAM lparam);

       DFSFILEANDINFO DfsFileAndInfo;
       GUIITEM guiItem;
       LRUMENU lrum;

       REGCODE RegCode;
       DWORD   dwNbUseUnregistered;
       DWORD   dwLastDayOfUse;

       BOOL    fOnUninstall,fOnHideRegister;
};


WNDMAIN::WNDMAIN(HWND,LPCREATESTRUCT)
{
   DfsFileAndInfo.DfsFile = NULL;
   DfsFileAndInfo.lpszDfsFileName = NULL;
   DfsFileAndInfo.dfNbDir = 0;
   DfsFileAndInfo.dfCurDir = 0;
   DfsFileAndInfo.dfNbFileSelectedCurDir = 0;
   DfsFileAndInfo.dfSizeSelectedCurDir = 0;
   DfsFileAndInfo.pDirInfo = NULL;


   DfsFileAndInfo.fBaseDirectorySelected = FALSE;
   DfsFileAndInfo.fBaseDirectoryNeeded = FALSE;
   DfsFileAndInfo.fNotifyAccepted = TRUE;
   DfsFileAndInfo.lpBaseDirectory = NULL;
   DfsFileAndInfo.dfBaseDirNum = 0;

   dwNbUseUnregistered=0;
   dwLastDayOfUse=0;
   lrum.SetIdLruEntry(IDM_FILE_LRU0);
   fOnUninstall=FALSE;
   fOnHideRegister=FALSE;
}


WNDMAIN::~WNDMAIN()
{
    DoCloseDfs(guiItem,DfsFileAndInfo);
}

static DWORD GetDayDWord()
{
    SYSTEMTIME st;
    GetSystemTime(&st);
    return (st.wDay)+(32L*st.wMonth)+(32L*16L*st.wYear);
}
#define LASTDAYOFUSEUNREGISTERED "SettingTabInfo"
#define NBDAYOFUSEUNREGISTERED "CompressionHint"

BOOL WNDMAIN::DoLoadPref()
{
    SAVEPARAM SaveParam;
    BOOL fRegistrationInfoRead = FALSE;
    if (!SaveParam.OpenRegKey(GetSmartVersionKeyBase(),"Software\\SmartVersion",FALSE))
        return FALSE;

    {
        TCHAR szColEntryName[NBLVCOLUMNFILELIST][MAX_PATH];
        LONG  lColSize[NBLVCOLUMNFILELIST];
        DWORD i;
        LONGSAVEPARAM lsp[NBLVCOLUMNFILELIST];
        for (i=0;i<NBLVCOLUMNFILELIST;i++)
        {
            //lColSize[i]=ListView_GetColumnWidth(guiItem.hwndLV,i);
            wsprintf(szColEntryName[i],"Column File List Width %u",i);
            lsp[i].lpszNameEntry=szColEntryName[i];
            lsp[i].plValue = &lColSize[i];
            lsp[i].lDefValue=lColSize[i]=COLUMNWIDTH_UNKNOWN;
        }
        if (SaveParam.DoReadLongParam(lsp,NBLVCOLUMNFILELIST))
            for (i=0;i<NBLVCOLUMNFILELIST;i++)
            {
                if (lColSize[i] != 0)
                {
                    guiItem.iColSizeFileList[i]=lColSize[i];
                }
            }
    }

    {
        TCHAR szColEntryName[NBLVCOLUMNDIRLIST][MAX_PATH];
        LONG  lColSize[NBLVCOLUMNDIRLIST];
        DWORD i;
        LONGSAVEPARAM lsp[NBLVCOLUMNDIRLIST];
        for (i=0;i<NBLVCOLUMNDIRLIST;i++)
        {
            //lColSize[i]=ListView_GetColumnWidth(guiItem.hwndLV,i);
            wsprintf(szColEntryName[i],"Column Directory List Width %u",i);
            lsp[i].lpszNameEntry=szColEntryName[i];
            lsp[i].plValue = &lColSize[i];
            lsp[i].lDefValue=lColSize[i]=COLUMNWIDTH_UNKNOWN;
        }
        if (SaveParam.DoReadLongParam(lsp,NBLVCOLUMNDIRLIST))
            for (i=0;i<NBLVCOLUMNDIRLIST;i++)
            {
                if (lColSize[i] != 0)
                {
                    guiItem.iColSizeDirList[i]=lColSize[i];
                }
            }
    }
    {
        long int i;

        for (i=MAXLRU;i>=0;i--)
        {
            TXTSAVEPARAM lst;
            TCHAR szEntryName[MAX_PATH];
            TCHAR szEntryValue[MAX_PATH+2]="";
            wsprintf(szEntryName,"LRU File %d",i);
            lst.lpszNameEntry = szEntryName;
            lst.lpszValue=szEntryValue;
            lst.uiSize=MAX_PATH;
            lst.lpDefValue=NULL;

            if (SaveParam.DoReadTxtParam(&lst,1))
                if (lstrlen(szEntryValue)>0)
                    lrum.AddNewItem(/*(WORD)i+1,*/szEntryValue);

        }
        lrum.PlaceMenuLRUItem(GetSubMenu(GetMenu(guiItem.hwndMain),0),IDM_EXIT);
    }


    {
        LONGSAVEPARAM lsp[2] =
            {
                { "ZlibCompressRatio",(long*)&guiItem.compressionParam.uZlibCompressRatio,1 },
                { "BlockCalcSizeSearch",(long*)&guiItem.compressionParam.dfBlockCalcSizeSearch,0 },
            };
        SaveParam.DoReadLongParam(lsp,2);
        if (guiItem.compressionParam.dfBlockCalcSizeSearch <= 6)
            guiItem.compressionParam.dfBlockCalcSizeSearch = COMPRESSIONPARAM_AUTOVALUE;
    }

    {
        long lOverwrite = 0;
        long lSelectTempMemSize = 0;
        long lTempMemSize = 0;
        long lSelectTempPath = 0;
        long lStripIdentical = 0;
        long lMd5 = 0;
        long lSha1 = 0;
		long lSha256 = 0;
		long lLzma = 0;

        LONGSAVEPARAM lsp[9] =
        {
            { "OverwriteExtracting",(long*)&lOverwrite,0 },
            { "SelectTempMemSize",(long*)&lSelectTempMemSize,0 },
            { "TempMemSize",(long*)&lTempMemSize,0 },
            { "SelectTempPath",(long*)&lSelectTempPath,0 },
            { "StripIdentical",(long*)&lStripIdentical,1 },
            { "MD5",(long*)&lMd5 ,1 },
            { "SHA1",(long*)&lSha1,0 },
			{ "SHA256",(long*)&lSha256,0 },
            { "LZMA",(long*)&lLzma,0 }
        };
        SaveParam.DoReadLongParam(lsp,9);
        guiItem.fOverwriteExtracting = lOverwrite != 0;
        guiItem.fSelectTempMemSize = lSelectTempMemSize!=0;
        guiItem.dwTempMemSize = lTempMemSize;
        guiItem.fSelectTempPath = lSelectTempPath!=0;
        guiItem.pfStripIdentical = lStripIdentical!=0;
        guiItem.fMd5 = lMd5!=0;
        guiItem.fSha1 = lSha1!=0;
		guiItem.fSha256 = lSha256 != 0;
        //guiItem.pfCompressLzma = lLzma!=0;


        if (!guiItem.fSelectTempPath)
            SetTempDirectory(NULL);
        else
        {
            dfwchar szTempPathW[MAX_PATH+0x10];
            ConvertAnsiToUnicode(guiItem.szTempPath,szTempPathW,MAX_PATH);
            SetTempDirectory(szTempPathW);
        }
        IOConfig_SetVirtualFileNameMaximumMemory(guiItem.fSelectTempMemSize,(unsigned long)guiItem.dwTempMemSize);
    }


    {
            TCHAR szTempPath[MAX_PATH+8]="";


            TXTSAVEPARAM tRegParam[1] =
            {
                { "TempPath",szTempPath,MAX_PATH*sizeof(TCHAR),NULL },
            } ;

            SaveParam.DoReadTxtParam(tRegParam,1);
            lstrcpy(guiItem.szTempPath,szTempPath);
    }

    {
        WINSIZE gwsMain;

        LONGSAVEPARAM lsp[5] =
            {
                { "gwsMain.x",(long*)&gwsMain.x,0 },
                { "gwsMain.y",(long*)&gwsMain.y,0 },
                { "gwsMain.cx",(long*)&gwsMain.cx,0 },
                { "gwsMain.cy",(long*)&gwsMain.cy,0 },
                { "gwsMain.fMax",(long*)&gwsMain.fMax,0 }
            };
        SaveParam.DoReadLongParam(lsp,5);

        if (!gwsMain.fMax)
        {
            if ((gwsMain.cx==0) || (gwsMain.cy>0))
                if (!IsIconic(guiItem.hwndMain) && !IsZoomed(guiItem.hwndMain))
                {
                    RECT rect;
                    GetWindowRect(guiItem.hwndMain, &rect);
                    gwsMain.cx = rect.right - rect.left;
                    gwsMain.cy = rect.bottom - rect.top;
                    if (gwsMain.cx<630)
                        gwsMain.cx=630;
                }
            if ((gwsMain.cx>0) && (gwsMain.cy>0))
              SetWindowPos(guiItem.hwndMain,NULL,0,0,
                           gwsMain.cx,gwsMain.cy,
                           SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOMOVE);
        }
    }


    {
        LONG lListViewStyle=LVS_ICON;
        LONGSAVEPARAM lStyleParam[1] = { { "ListViewStyle",(long*)&lListViewStyle,LVS_REPORT } };


        SaveParam.DoReadLongParam(lStyleParam,1);
        guiItem.SetListViewStyle(lListViewStyle);
    }

    {
        LONG lColumnSort = 0;
        LONG lInvert = 0;

        LONGSAVEPARAM lColParam[2] = { { "ColumnSort",(long*)&lColumnSort,0 } ,
                                       { "ColomnSortInvert",(long*)&lInvert,0} };

        SaveParam.DoReadLongParam(lColParam,2);


        guiItem.SetColumnSort(lColumnSort,lInvert!=0);
    }

    {
        TCHAR szLangUI[MAX_PATH+1];
        TXTSAVEPARAM tLangParam[] =
           { { "LangageUISelect", szLangUI,MAX_PATH,NULL } } ;
        szLangUI[0]=0;
        SaveParam.DoReadTxtParam(tLangParam,1);
        guiItem.dwLangUISelect = LANGUI_SELECT_AUTO;
        if (!GetIndexForInisLang(&guiItem.dwLangUISelect,szLangUI))
            guiItem.dwLangUISelect = GetBetterAutoLang();
    }



    {
        TXTSAVEPARAM tDirParam[] =
        { { "DefaultDirExtract",guiItem.szDefaultDirExtract,MAX_PATH*sizeof(TCHAR),NULL },
          { "DefaultDirAddVersion",guiItem.szDefaultDirAddVersion,MAX_PATH*sizeof(TCHAR),NULL },
          { "DefaultDirPreviousVersion",guiItem.szDefaultDirPreviousVersion,MAX_PATH*sizeof(TCHAR),NULL } } ;

        SaveParam.DoReadTxtParam(tDirParam,3);
    }

    {
        TCHAR szNameCode[MAX_PATH+1]="";
        TCHAR szCrcCode[MAX_PATH+1]="";

        TXTSAVEPARAM tRegParam[2] =
        {
          { "RegistrationName",szNameCode,MAX_PATH*sizeof(TCHAR),NULL },
          { "RegistrationCode",szCrcCode,MAX_PATH*sizeof(TCHAR),NULL },
        } ;

        fRegistrationInfoRead = SaveParam.DoReadTxtParam(tRegParam,2);
        RegCode.SetRegCode(szNameCode,szCrcCode);
    }


    {

        LONGSAVEPARAM lsp[2] =
        {
            { LASTDAYOFUSEUNREGISTERED,(long*)&dwLastDayOfUse,0 },
            { NBDAYOFUSEUNREGISTERED,(long*)&dwNbUseUnregistered,0 }
        };
        SaveParam.DoReadLongParam(lsp,2);
    }

    SaveParam.CloseRegKey();

    if (!fRegistrationInfoRead)
    {
      SAVEPARAM SaveParamLocalMachine;
      if (SaveParamLocalMachine.OpenRegKey(GetSmartVersionKeyBase(),"Software\\SmartVersion",FALSE))
      {
            TCHAR szNameCode[MAX_PATH+1]="";
            TCHAR szCrcCode[MAX_PATH+1]="";

            TXTSAVEPARAM tRegParam[2] =
            {
                { "RegistrationName",szNameCode,MAX_PATH*sizeof(TCHAR),NULL },
                { "RegistrationCode",szCrcCode,MAX_PATH*sizeof(TCHAR),NULL },
            } ;

            SaveParamLocalMachine.DoReadTxtParam(tRegParam,2);
            RegCode.SetRegCode(szNameCode,szCrcCode);
            SaveParamLocalMachine.CloseRegKey();
      }
    }


    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  PARAMETERS:
//    hwnd     - window handle
//    uMessage - message number
//    wparam   - additional information (dependant on message number)
//    lparam   - additional information (dependant on message number)
//
//  RETURN VALUE:
//    The return value depends on the message number.  If the message
//    is implemented in the message dispatch table, the return value is
//    the value returned by the message handling function.  Otherwise,
//    the return value is the value returned by the default window procedure.
//
//  COMMENTS:
//    Call the DispMessage() function with the main window's message dispatch
//    information (msdiMain) and the message specific information.
//

BOOL SkipDumpCrashInCommandLine(LPTSTR &lpCmdLine)
{
  BOOL fRet=FALSE;
  LPCTSTR lpszOptCrash = "/dumpcrash";
    if (lpCmdLine != NULL)
      if (lstrlen(lpCmdLine)>=lstrlen(lpszOptCrash))
        if (memcmp(lpCmdLine,lpszOptCrash,lstrlen(lpszOptCrash)*sizeof(TCHAR))==0)
        {
            fRet=TRUE;
            lpCmdLine+=lstrlen(lpszOptCrash);
            if ((*lpCmdLine)==' ')
                lpCmdLine++;
        }
        return fRet;
}

LRESULT WNDMAIN::MsgCreate(HWND hwnd, UINT uMessage, WPARAM wparam, LPARAM lparam)
{
    // Create the TreeView control, with lines and buttons, even at
    // the root level.

    // Save this value for later
    LPTSTR lpCommandLine = GetCommandLine();

    guiItem.hwndMain = hwnd;
    guiItem.cxSplitter = GetSystemMetrics(SM_CXEDGE);
    if (guiItem.cxSplitter==0)
        guiItem.cxSplitter = 2;


    //DoLoadPref();
    /*
    if (!RegCode.IsRegistered())
        MessageBox(hwnd,"no reg","no reg",0);
        */

    guiItem.hwndTreeView = CreateTreeView(hwnd,
                                  IDD_TREEVIEW,
                                  TVS_HASLINES | TVS_HASBUTTONS |
                                  TVS_LINESATROOT | TVS_HASLINES |
                                  TVS_SHOWSELALWAYS |
                                  WS_VISIBLE | WS_CHILD /*| WS_BORDER*/);


    guiItem.hwndLV = CreateListView(hwnd);
    /*
    guiItem.hwndToolTip = CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL,
                            WS_POPUP |TTS_ALWAYSTIP ,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            guiItem.hwndTreeView, NULL, ghInst,
                            NULL);

    SetWindowPos(guiItem.hwndToolTip, HWND_TOPMOST,0, 0, 0, 0,
             SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    // GET COORDINATES OF THE MAIN CLIENT AREA
    RECT rect;
    GetClientRect (guiItem.hwndTreeView, &rect);

    // INITIALIZE MEMBERS OF THE TOOLINFO STRUCTURE
    TOOLINFO ti;
    ti.cbSize = sizeof(TOOLINFO);
    ti.uFlags = TTF_SUBCLASS|TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
    ti.hwnd = guiItem.hwndTreeView;
    ti.hinst = ghInst;
    ti.uId = 0;//uid;
    ti.lpszText = "too";// LPSTR_TEXTCALLBACK;//lptstr;
        // ToolTip control will cover the whole window
    ti.rect.left = rect.left;
    ti.rect.top = rect.top;
    ti.rect.right = rect.right;
    ti.rect.bottom = rect.bottom;

    // SEND AN ADDTOOL MESSAGE TO THE TOOLTIP CONTROL WINDOW
    SendMessage(guiItem.hwndToolTip, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
    //SendMessage(guiItem.hwndToolTip,TTM_TRACKACTIVATE,(WPARAM)TRUE,(LPARAM)&ti);
*/

    guiItem.hwndSB = CreateStatusBar(ghInst,hwnd);
    guiItem.hwndTB = CreateAToolBar(ghInst,hwnd);

    DoLoadPref();
    TryLoadLang(hwnd,guiItem.dwLangUISelect,FALSE);
    lrum.PlaceMenuLRUItem(GetSubMenu(GetMenu(guiItem.hwndMain),0),IDM_EXIT);

    if ((!fOnHideRegister) && (!fGlobalHideRegisterOrUninstall) && (!fOnUninstall))
        if (!RegCode.IsRegistered())
            DoSplash(guiItem,FALSE,dwNbUseUnregistered,30);
    guiItem.SetRegisteredMode(RegCode.IsRegistered());

    guiItem.InitListView(guiItem.hwndLV);

    // Post a message to fill the control, or nix app if
    // the creation failed.

    if (!guiItem.hwndTreeView || !guiItem.hwndLV)
        PostMessage(hwnd, WM_COMMAND, IDM_EXIT, 0L );
    else
        PostMessage(hwnd, WM_COMMAND, IDM_FILL, 0L );

    RefreshGrayingMenu(guiItem,DfsFileAndInfo);

    {
        BOOL fActualVersionSet, fPreviousVersionSet;
        IsSmartVersionExeRegistrySet(fActualVersionSet,fPreviousVersionSet);
        if (!fActualVersionSet)
        RegisterSmartVersionExe();
    }

    if (lpCommandLine != NULL)
        if (strlen(lpCommandLine)>0)
    {
        LPCSTR lpParcLine=lpCommandLine;
        TCHAR szPortionLine[MAX_PATH*2];
        szPortionLine[0] = 0;

        lpParcLine = CopyStrWord(lpParcLine,szPortionLine); // skip .Exe name

        lpParcLine = CopyStrWord(lpParcLine,szPortionLine);
        if (lstrcmpi(szPortionLine,"/dumpcrash")==0)
            lpParcLine = CopyStrWord(lpParcLine,szPortionLine);

        if (szPortionLine[0]!=0)
        {
            if (lstrcmpi(szPortionLine,"/HIDEREGISTER")==0)
            {
                fOnHideRegister=TRUE;
                RegisterSmartVersionExe();
                return -1;
            }
            else
            if (lstrcmpi(szPortionLine,"/REGISTER")==0)
                RegisterSmartVersionExe();
            else
            if (lstrcmpi(szPortionLine,"/UNINSTALL")==0)
            {
                fOnUninstall=TRUE;
                DoUninstalling();
                return -1;
            }
            else
            if (DoLoadDfs(guiItem,DfsFileAndInfo,lrum,szPortionLine,TRUE,NULL))
                RefreshGrayingMenu(guiItem,DfsFileAndInfo);
        }
    }

    DragAcceptFiles(hwnd,TRUE);
    guiItem.DisplayNewTitleBar();
    {
        HDC hdc= GetDC(hwnd);
        DoPaintWndItem(hwnd,hdc);
        ReleaseDC(hwnd,hdc);
    }
    return 0;
}



BOOL WNDMAIN::DoSavePref()
{
    SAVEPARAM SaveParam;

    SaveParam.OpenRegKey(GetSmartVersionKeyBase(),"Software\\SmartVersion",TRUE);
    {
        TXTSAVEPARAM tsp;
        tsp.lpszNameEntry = "SmartVersionUseRegistry";
        tsp.lpszValue = tsp.lpDefValue = "TRUE";
        tsp.uiSize = lstrlen(tsp.lpDefValue);

        SaveParam.DoWriteTxtParam(&tsp,1);
    }


    {
        TCHAR szColEntryName[NBLVCOLUMNFILELIST][MAX_PATH];
        LONG  lColSize[NBLVCOLUMNFILELIST];
        DWORD i;
        LONGSAVEPARAM lsp[NBLVCOLUMNFILELIST];
        for (i=0;i<NBLVCOLUMNFILELIST;i++)
        {
            lColSize[i]=guiItem.iColSizeFileList[i];//ListView_GetColumnWidth(guiItem.hwndLV,i);
            wsprintf(szColEntryName[i],"Column File List Width %u",i);
            lsp[i].lpszNameEntry=szColEntryName[i];
            lsp[i].plValue = &lColSize[i];
            lsp[i].lDefValue=0;
        }
        SaveParam.DoWriteLongParam(lsp,NBLVCOLUMNFILELIST);
    }
    {
        TCHAR szColEntryName[NBLVCOLUMNDIRLIST][MAX_PATH];
        LONG  lColSize[NBLVCOLUMNDIRLIST];
        DWORD i;
        LONGSAVEPARAM lsp[NBLVCOLUMNDIRLIST];
        for (i=0;i<NBLVCOLUMNDIRLIST;i++)
        {
            lColSize[i]=guiItem.iColSizeDirList[i];//ListView_GetColumnWidth(guiItem.hwndLV,i);
            wsprintf(szColEntryName[i],"Column Directory List Width %u",i);
            lsp[i].lpszNameEntry=szColEntryName[i];
            lsp[i].plValue = &lColSize[i];
            lsp[i].lDefValue=0;
        }
        SaveParam.DoWriteLongParam(lsp,NBLVCOLUMNDIRLIST);
    }


    {
        LONGSAVEPARAM lsp[2] =
            {
                { "ZlibCompressRatio",(long*)&guiItem.compressionParam.uZlibCompressRatio,1 },
                { "BlockCalcSizeSearch",(long*)&guiItem.compressionParam.dfBlockCalcSizeSearch,0 },
            };
        SaveParam.DoWriteLongParam(lsp,2);
    }


    {
        long lOverwrite = (long)guiItem.fOverwriteExtracting;
        long lSelectTempMemSize = (long)guiItem.fSelectTempMemSize;
        long lTempMemSize = (long)guiItem.dwTempMemSize;
        long lSelectTempPath = (long)guiItem.fSelectTempPath;
        long lStripIdentical = (long)guiItem.pfStripIdentical;
        long lMd5 = (long)guiItem.fMd5;
        long lSha1 = (long)guiItem.fSha1;
		long lSha256 = (long)guiItem.fSha256;
        //long lLzma = (long)guiItem.pfCompressLzma;

        LONGSAVEPARAM lsp[8] =
        {
            { "OverwriteExtracting",(long*)&lOverwrite,0 },
            { "SelectTempMemSize",(long*)&lSelectTempMemSize,0 },
            { "TempMemSize",(long*)&lTempMemSize,0 },
            { "SelectTempPath",(long*)&lSelectTempPath,0 },
            { "StripIdentical",(long*)&lStripIdentical,1 },
            { "MD5",(long*)&lMd5 ,1 },
            { "SHA1",(long*)&lSha1,0 },
            { "SHA256",(long*)&lSha256,0 } /*,
            { "LZMA",(long*)&lLzma,0 }*/

        };
        SaveParam.DoWriteLongParam(lsp,8);
    }


    {
        TCHAR szLangUI[MAX_PATH+1];
        szLangUI[0]=0;
        LPCTSTR lpszLang = GetLangInisLang((WORD)guiItem.dwLangUISelect);
        TXTSAVEPARAM tLangParam[] =
           { { "LangageUISelect", (LPTSTR)lpszLang ,MAX_PATH,NULL } } ;

        SaveParam.DoWriteTxtParam(tLangParam,1);
    }

    {
            TCHAR szTempPath[MAX_PATH+8]="";
            lstrcpy(szTempPath,guiItem.szTempPath);

            TXTSAVEPARAM tRegParam[1] =
            {
                { "TempPath",szTempPath,MAX_PATH*sizeof(TCHAR),NULL },
            } ;

            SaveParam.DoWriteTxtParam(tRegParam,1);

    }


    {
        WINSIZE gwsMain;
        gwsMain.fMax = IsZoomed(guiItem.hwndMain);
        if (!IsIconic(guiItem.hwndMain) && !IsZoomed(guiItem.hwndMain))
        {
            RECT rect;
            GetWindowRect(guiItem.hwndMain, &rect);
            gwsMain.x = rect.left;
            gwsMain.y = rect.top;
            gwsMain.cx = rect.right - rect.left;
            gwsMain.cy = rect.bottom - rect.top;
        }
        else
            gwsMain.x = gwsMain.y = gwsMain.cx = gwsMain.cy = 0;




        LONGSAVEPARAM lsp[5] =
            {
                { "gwsMain.x",(long*)&gwsMain.x,0 },
                { "gwsMain.y",(long*)&gwsMain.y,0 },
                { "gwsMain.cx",(long*)&gwsMain.cx,0 },
                { "gwsMain.cy",(long*)&gwsMain.cy,0 },
                { "gwsMain.fMax",(long*)&gwsMain.fMax,0 }
            };
        SaveParam.DoWriteLongParam(lsp,5);

    }



    {
        LONG lListViewStyle;
        UINT uiStyle;
        guiItem.GetListViewStyle(uiStyle);
        lListViewStyle = uiStyle;

        LONGSAVEPARAM lStyleParam[1] = { { "ListViewStyle",(long*)&lListViewStyle,LVS_REPORT } };


        SaveParam.DoWriteLongParam(lStyleParam,1);
        guiItem.SetListViewStyle(lListViewStyle);
    }


    {
        LONG lColumnSort,lInvert;
        lColumnSort = guiItem.GetColumnSort();
        lInvert = (guiItem.GetColumnSortInvert()) ? 1 : 0;

        LONGSAVEPARAM lColParam[2] = { { "ColumnSort",(long*)&lColumnSort,0 } ,
                                       { "ColomnSortInvert",(long*)&lInvert,0} };

        SaveParam.DoWriteLongParam(lColParam,2);
    }

    {
        long int i;

        for (i=0;i<=MAXLRU;i++)
        {
            TXTSAVEPARAM lst;
            TCHAR szEntryName[MAX_PATH];
            TCHAR szEntryValue[MAX_PATH+2]="";
            wsprintf(szEntryName,"LRU File %d",i);
            lst.lpszNameEntry = szEntryName;
            if (!lrum.GetMenuItem((short)i,FALSE,szEntryValue,MAX_PATH))
                szEntryValue[0]='\0';
            lst.lpszValue=szEntryValue;
            lst.uiSize=MAX_PATH;
            lst.lpDefValue=NULL;

            SaveParam.DoWriteTxtParam(&lst,1);
        }

        lrum.PlaceMenuLRUItem(GetSubMenu(GetMenu(guiItem.hwndMain),0),IDM_EXIT);
    }

    {
        TXTSAVEPARAM tDirParam[] =
        { { "DefaultDirExtract",guiItem.szDefaultDirExtract,MAX_PATH*sizeof(TCHAR),NULL },
          { "DefaultDirAddVersion",guiItem.szDefaultDirAddVersion,MAX_PATH*sizeof(TCHAR),NULL },
          { "DefaultDirPreviousVersion",guiItem.szDefaultDirPreviousVersion,MAX_PATH*sizeof(TCHAR),NULL } } ;

        SaveParam.DoWriteTxtParam(tDirParam,3);
    }

    {
        TCHAR szNameCode[MAX_PATH+1];
        TCHAR szCrcCode[MAX_PATH+1];
        lstrcpy(szNameCode,RegCode.GetNameCode());
        lstrcpy(szCrcCode,RegCode.GetCrcCode());
        TXTSAVEPARAM tRegParam[2] =
        { { "RegistrationName",szNameCode,MAX_PATH*sizeof(TCHAR),NULL },
          { "RegistrationCode",szCrcCode,MAX_PATH*sizeof(TCHAR),NULL },
        } ;

        SaveParam.DoWriteTxtParam(tRegParam,2);
    }


    if (!RegCode.IsRegistered())
    {
        long lCurDay = (long)GetDayDWord();
        if (((DWORD)lCurDay) != dwLastDayOfUse)
            dwNbUseUnregistered++;
        LONGSAVEPARAM lsp[2] =
        {
            { LASTDAYOFUSEUNREGISTERED,(long*)&lCurDay,0 },
            { NBDAYOFUSEUNREGISTERED,(long*)&dwNbUseUnregistered,0 }
        };
        SaveParam.DoWriteLongParam(lsp,2);
    }


    /*



*/
    SaveParam.CloseRegKey();
    return TRUE;
}


//
//  FUNCTION: MsgDestroy(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Calls PostQuitMessage(), cleans up image list.
//
//  PARAMETERS:
//
//    hwnd      - Window handle  (Unused)
//    uMessage  - Message number (Unused)
//    wparam    - Extra data     (Unused)
//    lparam    - Extra data     (Unused)
//
//  RETURN VALUE:
//
//    Always returns 0 - Message handled
//
//  COMMENTS:
//
//

LRESULT WNDMAIN::MsgDestroy(HWND hwnd, UINT uMessage, WPARAM wparam, LPARAM lparam)
{
    HMENU hMenuOld;
 //   bOutaHere = TRUE;

    if (!fOnUninstall)
    {
        guiItem.DoSaveColumnWidth();
        DoSavePref();
    }

    hMenuOld=GetMenu(hwnd);

    if (hMenuOld!=NULL)
    {
        SetMenu(hwnd,NULL);
        DestroyMenu(hMenuOld);
    }

    PostQuitMessage(0);
    return 0;
}

BOOL WNDMAIN::GetItemHigh(HWND hwnd,int &iTVLVHigh,int &iSBHigh, int &iTbHigh)
{
    RECT rc,rcSb,rcWnd;
    int cyClient ;

    GetClientRect(hwnd,&rc);
    cyClient = rc.bottom - rc.top;

    rc.bottom=rc.left=rc.right=rc.top=0;
    rcWnd=rcSb=rc;

    if (guiItem.hwndTB != NULL)
    {
        GetClientRect(guiItem.hwndTB,&rc);
        GetWindowRect(guiItem.hwndTB,&rcWnd);
        iTbHigh = rcWnd.bottom - rcWnd.top;
    }
    else
        iTbHigh = 0;

    if  (guiItem.hwndSB != NULL)
    {
        GetClientRect(guiItem.hwndSB,&rcSb);
        GetWindowRect(guiItem.hwndSB,&rcWnd);
        iSBHigh = rcSb.bottom - rcSb.top;
        iSBHigh = rcWnd.bottom - rcWnd.top;
    }
    else
    {
        iSBHigh = 0;
    }


    if (iTbHigh>cyClient)
    {
        iTVLVHigh = 0;
        iSBHigh = 0;
    }
    else
    {
        iTVLVHigh = cyClient - iTbHigh;

        if (iTVLVHigh > iSBHigh)
          iTVLVHigh -= iSBHigh;
        else
        {
          iSBHigh = iTVLVHigh;
          iTVLVHigh = 0;
        }
    }

    return TRUE;
}

BOOL WNDMAIN::DoResize(HWND hwnd,BOOL fSetNewSplit,int iNewSplit)
{
    RECT rc;    // Client area rect
    int cxClient,cyClient;
    INT iTVLVHigh;
    INT iTbHigh;
    INT iSBHigh;

    GetClientRect(hwnd,&rc);
    cxClient = rc.right - rc.left;
    cyClient = rc.bottom - rc.top;

    GetItemHigh(hwnd,iTVLVHigh,iSBHigh,iTbHigh);

//return 1;//1
    // Resize the TreeView control
    //SendMessage(guiItem.hwndTB,TB_AUTOSIZE,0,0);
    //SetWindowPos(guiItem.hwndTB, NULL,0,0,cxClient,cyClient,SWP_NOZORDER|SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);
    {
        //HWND hwndRB=GetParent(guiItem.hwndTB);
        //SendMessage(hwndRB, WM_SIZE, 0, MAKELONG(cxClient,cyClient));
    }
    //SendMessage(guiItem.hwndTB, WM_SIZE, 0, MAKELONG(cxClient,cyClient));

    {
        if (guiItem.hwndTB != NULL)
        {
          HWND hWndRB;

          hWndRB = GetParent(guiItem.hwndTB);
          if (hWndRB == hwnd)
              hWndRB = guiItem.hwndTB;


          if ((guiItem.sizeTB.cx != cxClient) ||
              (guiItem.sizeTB.cy != iTbHigh))
          {
              guiItem.sizeTB.cx = cxClient;
              guiItem.sizeTB.cy = iTbHigh;
              SetWindowPos(hWndRB, NULL,0,0,cxClient,iTbHigh,
                  SWP_NOZORDER|SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);
          }
        }

        if (guiItem.hwndSB != NULL)
          SetWindowPos(guiItem.hwndSB, NULL,
                       0,/*0*/ cyClient-iSBHigh,
                       cxClient,iSBHigh,
                       SWP_NOZORDER| /*SWP_NOMOVE|*/SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);
        //InvalidateRect(guiItem.hwndSB,NULL,FALSE);
    }
    //SendMessage(guiItem.hwndSB, WM_SIZE, 0, MAKELONG(cxClient,cyClient));
    if (guiItem.hwndProgress!=NULL)
        SendMessage(guiItem.hwndSB, WM_SIZE, 0, MAKELONG(cxClient,cyClient));
//return 1;//2

    InitializeStatusBar(guiItem.hwndSB,guiItem.hwndMain);
    //InvalidateRect(guiItem.hwndSB,NULL,FALSE);


//return 1;//3

    if (guiItem.hwndTreeView)
    {
        GetWindowRect(guiItem.hwndTreeView, &rc);
        ScreenToClient(hwnd, (LPPOINT)&rc.right);
        if (fSetNewSplit)
            rc.right=iNewSplit;
        rc.right = max(50, min(rc.right, cxClient - 50));

        SetWindowPos(guiItem.hwndTreeView,
                     NULL,
                     0, iTbHigh ,
                     rc.right,
                     iTVLVHigh,
                     SWP_NOZORDER);
    }

    // Resize the ListView control

    rc.right += guiItem.cxSplitter;

    if (guiItem.hwndLV)
        SetWindowPos(guiItem.hwndLV,
                     NULL,
                     rc.right, iTbHigh,
                     cxClient - rc.right,
                     iTVLVHigh,
                     SWP_NOZORDER);

    return TRUE;
}

LRESULT WNDMAIN::MsgSize(HWND hwnd, UINT uMessage, WPARAM wparam, LPARAM lparam)
{
    if (wparam != SIZE_MINIMIZED)
      DoResize(hwnd);
    return 0;
}


LRESULT WNDMAIN::MsgMenuSelect(HWND hwnd, UINT uMessage, WPARAM wparam, LPARAM lparam)
{
    TCHAR  szBuffer[MAX_PATH];
    UINT   nStringID = 0;
    UINT   fuFlags = GET_WM_MENUSELECT_FLAGS(wparam, lparam) & 0xffff;
    UINT   uCmd    = GET_WM_MENUSELECT_CMD(wparam, lparam);
    HMENU  hMenu   = GET_WM_MENUSELECT_HMENU(wparam, lparam);
    BOOL   fDone = FALSE;


    szBuffer[0] = 0;                            // First reset the buffer


    if (fuFlags == 0xffff && hMenu == NULL)     // Menu has been closed
        nStringID = IDS_DESCRIPTION;

    else if (fuFlags & MFT_SEPARATOR)           // Ignore separators
        nStringID = 0;

    else if (fuFlags & MF_POPUP)                // Popup menu
    {
        if (fuFlags & MF_SYSMENU)               // System menu
            nStringID = IDS_SYSMENU;
/*
        else
            // Get string ID for popup menu from idPopup array.
            nStringID = ((uCmd < sizeof(idPopup)/sizeof(idPopup[0])) ?
                            idPopup[uCmd] : 0);*/
    }  // for MF_POPUP

    else                                        // Must be a command item
        nStringID = uCmd;                       // String ID == Command ID

    // Load the string if we have an ID
    if (0 != nStringID)
        LoadString(ghInstRes, nStringID, szBuffer, sizeof(szBuffer));

    if ((nStringID == IDS_DESCRIPTION) && (DfsFileAndInfo.DfsFile != NULL))
    {
        fDone = guiItem.RefreshNormalStatusBar(DfsFileAndInfo);
    }

    // Finally... send the string to the status bar
    if (!fDone)
        guiItem.UpdateStatusBar(guiItem.hwndSB,szBuffer, 0, 0);

    return 0;
}

//
//  FUNCTION: MsgLButtonDown(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Initiates splitter bar stuff
//
//  PARAMETERS:
//
//    hwnd      - Window handle  (Unused)
//    uMessage  - Message number (Unused)
//    wparam    - Not used.
//    lparam    - Not used.
//
//  RETURN VALUE:
//
//    Always returns 0 - Message handled
//
//  COMMENTS:
//
//

LRESULT WNDMAIN::MsgLButtonDown(HWND hwnd, UINT uMessage, WPARAM wparam, LPARAM lparam)
{
    RECT rcTb,rcLV,rcTV;
    #define xPos ((int)(short)LOWORD(lparam))


    rcTb.bottom=rcTb.left=rcTb.right=rcTb.top=0;
    rcLV=rcTV=rcTb;

    SetCapture(hwnd);

    GetClientRect(hwnd, &guiItem.rcSplit);
    if (guiItem.hwndTB != NULL)
      GetClientRect(guiItem.hwndTB, &rcTb);
    GetClientRect(guiItem.hwndLV, &rcLV);
    //GetClientRect(guiItem.hwndTreeView, &rcTV);
    GetWindowRect(guiItem.hwndTreeView, &rcTV);

    guiItem.rcSplit.top=rcTb.bottom;
    if (guiItem.rcSplit.bottom<rcTb.bottom)
        guiItem.rcSplit.bottom=rcTb.bottom;

    {
        int iTVLVHigh,iTbHigh,iSBHigh;

        GetItemHigh(hwnd,iTVLVHigh,iSBHigh,iTbHigh);
        guiItem.rcSplit.bottom = guiItem.rcSplit.top + iTVLVHigh;
    }


    // Calculate initial position
    guiItem.rcSplit.left = min(max(50, (signed)(xPos - guiItem.cxSplitter/2)), (guiItem.rcSplit.right - 50)) + 1;

    // Get a DC (also used as a flag indicating we have capture)
    if (guiItem.hdcSplit)
        ReleaseDC(hwnd, guiItem.hdcSplit);
    guiItem.hdcSplit = GetDC(hwnd);

    // Draw splitter bar in initial position
    PatBlt(guiItem.hdcSplit, guiItem.rcSplit.left, guiItem.rcSplit.top+1, guiItem.cxSplitter, guiItem.rcSplit.bottom-guiItem.rcSplit.top, DSTINVERT);

    return 0;
}


//
//  FUNCTION: MsgMouseMove(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Moves splitter bar
//
//  PARAMETERS:
//
//    hwnd      - Window handle  (Unused)
//    uMessage  - Message number (Unused)
//    wparam    - Not used.
//    lparam    - Not used.
//
//  RETURN VALUE:
//
//    Always returns 0 - Message handled
//
//  COMMENTS:
//
//

LRESULT WNDMAIN::MsgMouseMove(HWND hwnd, UINT uMessage, WPARAM wparam, LPARAM lparam)
{
    #define xPos ((int)(short)LOWORD(lparam))

    if (guiItem.hdcSplit)
    {
        // Erase previous bar
        PatBlt(guiItem.hdcSplit, guiItem.rcSplit.left, guiItem.rcSplit.top+1, guiItem.cxSplitter, guiItem.rcSplit.bottom-guiItem.rcSplit.top, DSTINVERT);

        // Calculate new position
        guiItem.rcSplit.left = min(max(50, (signed)(xPos - guiItem.cxSplitter/2)), guiItem.rcSplit.right - 50) + 1;

        // Draw bar in new position
        PatBlt(guiItem.hdcSplit, guiItem.rcSplit.left, guiItem.rcSplit.top+1, guiItem.cxSplitter, guiItem.rcSplit.bottom-guiItem.rcSplit.top, DSTINVERT);
    }
    return 0;
}


//
//  FUNCTION: MsgLButtonUp(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Finishes splitter bar moving and resized child windows.
//
//  PARAMETERS:
//
//    hwnd      - Window handle  (Unused)
//    uMessage  - Message number (Unused)
//    wparam    - Not used.
//    lparam    - Not used.
//
//  RETURN VALUE:
//
//    Always returns 0 - Message handled
//
//  COMMENTS:
//
//

LRESULT WNDMAIN::MsgLButtonUp(HWND hwnd, UINT uMessage, WPARAM wparam, LPARAM lparam)
{
    #define xPos ((int)(short)LOWORD(lparam))

    if (guiItem.hdcSplit)
    {
        // Erase previous bar
        PatBlt(guiItem.hdcSplit, guiItem.rcSplit.left, guiItem.rcSplit.top+1, guiItem.cxSplitter, guiItem.rcSplit.bottom-guiItem.rcSplit.top, DSTINVERT);

        // Calculate new position
        guiItem.rcSplit.left = min(max(50, (signed)(xPos - guiItem.cxSplitter/2)), guiItem.rcSplit.right - 50) + 1;

        // Clean up
        ReleaseCapture();
        ReleaseDC(hwnd, guiItem.hdcSplit);
        guiItem.hdcSplit = NULL;

        guiItem.rcSplit.left -= min((int)guiItem.rcSplit.left,(int)guiItem.cxSplitter/2);

        DoResize(hwnd,TRUE,guiItem.rcSplit.left);
    }
    return 0;
}

LRESULT WNDMAIN::MsgDropFiles(HWND hwnd, UINT uMessage, WPARAM wparam, LPARAM lparam)
{
    HDROP hDrop = (HDROP) wparam;
    UINT uiNbFilesDropped ;
    BOOL fDfsFileDropped=FALSE;
    BOOL fZipFileDropped=FALSE;
    TCHAR szFirstFile[MAX_PATH+1];
    uiNbFilesDropped = DragQueryFile (hDrop,(UINT)-1,NULL,0);
    if (uiNbFilesDropped==1)
    {
        UINT uiRet;
        uiRet=DragQueryFile(hDrop,0,szFirstFile,MAX_PATH);

        if ((uiRet>0) && (uiRet<MAX_PATH))
        {
            DFSFILEINFOPARAM DfsFileParam;
            DFSFILE DfsFile = NULL;
            DfsFileParam.sizeStruct = sizeof(DfsFileParam);
            DfsFileParam.dfStatus = DFS_READABLE ;
            #ifdef UNICODE
            DfsFileParam.filename = szFirstFile;
            #else
            WCHAR wFileName[MAX_PATH];
            ConvertAnsiToUnicode(szFirstFile,(dfwcharp)wFileName,sizeof(wFileName)/sizeof(WCHAR));
            DfsFileParam.filename = (dfwcharp)wFileName;
            #endif

            if (DfsFileOpen(&DfsFileParam, &DfsFile, NULL) == DFS_SUCCESS)
            {
                // printf("Error in opening %ws\n", FileName);
                fDfsFileDropped=TRUE;
                DfsClose(DfsFile,NULL);
            }
            if (!fDfsFileDropped)
            {
                zlib_filefunc_def ffunc;
                fill_win32_filefunc(&ffunc);
                unzFile uf = unzOpen2(szFirstFile,&ffunc);
                if (uf != NULL)
                {
                    fZipFileDropped = TRUE;
                    unzClose(uf);
                }
            }
        }
    }


    if (fDfsFileDropped)
    {
            DoLoadDfs(guiItem,DfsFileAndInfo,lrum,szFirstFile,TRUE,NULL);
    }
    else
    if (fZipFileDropped)
    {
        if (DfsFileAndInfo.DfsFile == NULL)
            DoNewDfsZipFile(guiItem,DfsFileAndInfo,lrum,szFirstFile);
        else
            DoInsertVersionZipFile(guiItem,DfsFileAndInfo,lrum,szFirstFile);
    }
    else
    {
        dfuLong32 i;
        DWORD dwTotalSubDir=0;
        FILETOADDARRAY ftaArray;
        ClearFileToAddArrayWithDelete(&ftaArray,FALSE,FALSE);
        for (i=0;i<uiNbFilesDropped;i++)
        {
            TCHAR szFileName[MAX_PATH+1];
            UINT uiRet;

            uiRet=DragQueryFile(hDrop,i,szFileName,MAX_PATH);
            if ((uiRet>0) && (uiRet<MAX_PATH))
            {
                DWORD dwAttr = GetFileAttributes(szFileName);
                if ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) == 0)
                    AddFileFtaArray(&ftaArray,szFileName,NULL);
                else
                {
                    TCHAR szBuffer[MAX_PATH+1];
                    LPTSTR lpNameOnly=NULL;
                    GetFullPathName(szFileName,MAX_PATH,szBuffer,&lpNameOnly);
                    AddDirFtaArray(&ftaArray,szFileName,dwTotalSubDir,TRUE,lpNameOnly);
                }
            }
        }

        if (ftaArray.dfNbFileToAdd!=0)
        {
            if (DfsFileAndInfo.DfsFile == NULL)
                DoNewDfs(guiItem,DfsFileAndInfo,lrum,ftaArray.pFileToAdd,ftaArray.dfNbFileToAdd);
            else
                DoInsertVersion(guiItem,DfsFileAndInfo,lrum,ftaArray.pFileToAdd,ftaArray.dfNbFileToAdd);
        }

        ClearFileToAddArrayWithDelete(&ftaArray,TRUE,FALSE);
    }

    DragFinish(hDrop);

    return 0;
}

LRESULT WNDMAIN::MsgSetFocus(HWND hwnd,UINT,WPARAM,LPARAM)
{
    static BOOL fDone=FALSE;

    if (guiItem.GetIsListViewFocused())
        SetFocus(guiItem.hwndLV);
    else
        SetFocus(guiItem.hwndTreeView);

    return 0;
}


LRESULT WNDMAIN::MsgPaint(HWND hwnd, UINT uMessage, WPARAM wparam, LPARAM lparam)
{
  HDC         hdc;
  PAINTSTRUCT ps;
  BOOL        fRet;

  hdc = BeginPaint(hwnd, &ps);
  fRet = DoPaintWndItem(hwnd,hdc);
  EndPaint(hwnd, &ps);
  return fRet;
}

BOOL WNDMAIN::DoPaintWndItem(HWND hwnd,HDC hdc)
{
  COLORREF    clr;
  HBRUSH      hbr;
  RECT        rcTV;
  RECT        rcBelowTBDraw;
  RECT        rcSplitDraw;


  /*
  RECT        rc;
  clr = GetSysColor(COLOR_3DFACE);
  clr = RGB(255,0,0);

  hbr=CreateSolidBrush(clr);
  GetClientRect(hwnd,&rc);
  FillRect(hdc,&rc,hbr);
  DeleteBrush(hbr);
*/
  INT iTVLVHigh;
  INT iTbHigh;
  INT iSBHigh;


  GetItemHigh(hwnd,iTVLVHigh,iSBHigh,iTbHigh);
  GetWindowRect(guiItem.hwndTreeView, &rcTV);
  ScreenToClient(hwnd, (LPPOINT)&rcTV.left);
  ScreenToClient(hwnd, (LPPOINT)&rcTV.right);

  clr = GetSysColor(COLOR_3DFACE);
  //clr=RGB(0,255,0);
  hbr=CreateSolidBrush(clr);

  //PatBlt(guiItem.hdcSplit, guiItem.rcSplit.left, guiItem.rcSplit.top+1, guiItem.cxSplitter, guiItem.rcSplit.bottom-guiItem.rcSplit.top, DSTINVERT);
  rcSplitDraw.left = rcTV.right;
  rcSplitDraw.top = iTbHigh+1;
  rcSplitDraw.right = rcSplitDraw.left + guiItem.cxSplitter;
  rcSplitDraw.bottom = rcSplitDraw.top+iTVLVHigh;
//rcSplitDraw.top = 0;
  FillRect(hdc,&rcSplitDraw,hbr);
  DeleteBrush(hbr);

  if (guiItem.hwndTB != NULL)
  {
        HWND hWndRB;
        RECT rcTB;
        hWndRB = GetParent(guiItem.hwndTB);
        if (hWndRB == hwnd)
            hWndRB = guiItem.hwndTB;
        GetWindowRect(hWndRB,&rcBelowTBDraw);

        ScreenToClient(hwnd, (LPPOINT)&rcTB.left);
        ScreenToClient(hwnd, (LPPOINT)&rcTB.right);

        GetClientRect(hwnd,&rcBelowTBDraw);
        rcBelowTBDraw.top = iTbHigh-2;
        rcBelowTBDraw.bottom = iTbHigh;
        rcBelowTBDraw.left=0;

        //rcBelowTBDraw.top = 0;

        clr = GetSysColor(COLOR_3DFACE);
        //clr=RGB(0,255,255);
        hbr=CreateSolidBrush(clr);
        FillRect(hdc,&rcBelowTBDraw,hbr);
        DeleteBrush(hbr);
  }

  return TRUE;
}

LRESULT WNDMAIN::MsgCommand(HWND hwnd, UINT uMessage, WPARAM wparam, LPARAM lparam)
{
    UINT uiId = GET_WM_COMMAND_ID(wparam,lparam);

    if ((uiId>=IDM_FILE_LRU0) && (uiId<=IDM_FILE_LRU9))
    {
        TCHAR szFn[MAX_PATH+0x10];
        if (lrum.GetMenuItem(uiId,TRUE,szFn,(sizeof(szFn)/sizeof(TCHAR))-1))
        {
            if (!DoLoadDfs(guiItem,DfsFileAndInfo,lrum,szFn,TRUE,NULL))
            {
                  lrum.DelMenuItem(uiId,TRUE);
                  lrum.PlaceMenuLRUItem(GetSubMenu(GetMenu(guiItem.hwndMain),0),IDM_EXIT);
                  //MessageLoadStringTxtParam(hWnd,IDS_FILENOTEXIST,0,MB_OK,szFN);
            }
        }
    }

    if (uiId==IDM_KEYBOARD_MENUDELETE)
    {
        BOOL fIsDelVer;
        if (GetFocus()==guiItem.hwndLV)
            fIsDelVer = (((DWORD)DfsFileAndInfo.dfCurDir) == TVITEMPARAM_ROOT);
        else
            fIsDelVer = TRUE;
        uiId = fIsDelVer ? IDM_VERSION_DELETE:IDM_FILESDELETE;

    }

    switch (uiId)
    {
        case IDM_VIEW_LARGEICON:
        case IDM_VIEW_DETAILS:
        case IDM_VIEW_SMALLICONS:
        case IDM_VIEW_LIST:
            guiItem.SetListViewStyle((uiId-IDM_VIEW_LARGEICON)+LVS_ICON);
            break;

        case IDM_EXIT:
            SendMessage(hwnd,WM_CLOSE,0,0);
            break;

        case IDM_FILE_CLOSE:
            DoCloseDfs(guiItem,DfsFileAndInfo);
            break;

        case IDM_OPTION_SETTINGS:
            DoApplicationSettings(guiItem,DfsFileAndInfo,lrum);
            break;

        case IDM_OPTION_REGISTER:
            DoApplicationRegister(RegCode,guiItem,DfsFileAndInfo);
            break;

        case IDM_FILE_NEW:
            DoNewDfs(guiItem,DfsFileAndInfo,lrum,NULL,0);
            break;

        case IDM_VERSION_ADDNEWVERSION:
            DoInsertVersion(guiItem,DfsFileAndInfo,lrum,NULL,0);
            break;

        case IDM_VERSION_ADDNEWVERSIONFROMZIP:
            DoInsertVersionZipFile(guiItem,DfsFileAndInfo,lrum,NULL);
            break;

        case IDM_VERSION_GENERATESUBDFS:
            DoGenerateSubDfsUserSelected(guiItem,DfsFileAndInfo,lrum);
            break;

        case IDM_APPENDSVFTOSVF:
            DoAppendSvfToSvf(guiItem,DfsFileAndInfo,lrum);
            break;

        case IDM_VERSION_DELETE:
            DoDeleteVersion(guiItem,DfsFileAndInfo,lrum);
            break;

        case IDM_VERSION_ZIPFILE:
#ifdef _DEBUG
            {
                TCHAR sz[500];
                wsprintf(sz,"GET_WM_COMMAND_HWND=%08x, GET_WM_COMMAND_CMD=%08x\n",
                    GET_WM_COMMAND_HWND(wparam,lparam),GET_WM_COMMAND_CMD(wparam,lparam));
                OutputDebugString(sz);
            }
#endif
            DoGenerateZip(guiItem,DfsFileAndInfo);

            break;

        case IDM_FILE_NEW_FROM_ZIP:
            DoNewDfsZipFile(guiItem,DfsFileAndInfo,lrum,NULL);
            break;

        case IDM_FILE_OPEN:
            {
                TCHAR szDfs[MAX_PATH*2]="";
                if (DoOpeningDfs(guiItem,szDfs,sizeof(szDfs)/sizeof(TCHAR)))
                   DoLoadDfs(guiItem,DfsFileAndInfo,lrum,szDfs,TRUE,NULL);
            }
            break;

        case IDM_VERSION_SPECIFYBASEDIR:
            {
                DoSpecifyBaseDirVersion(guiItem,DfsFileAndInfo);
                break;
            }

        case IDM_VERSION_EXTRACT:
            DoExtract(guiItem,DfsFileAndInfo,FALSE);
            break;

        case IDM_FILEPROPERTIES:
            DoFileProperties(guiItem,DfsFileAndInfo,lrum);
            break;

        case IDM_FILESDELETE:
            DoFileDelete(guiItem,DfsFileAndInfo,lrum);
            break;

        case IDM_INSERTFILES:
            DoInsertFileInVersion(guiItem,DfsFileAndInfo,lrum);
            break;

        case IDM_VERSION_VERSIONPROPERTIES:
            DoVersionProperties(guiItem,DfsFileAndInfo);
            break;

        case IDM_HELP_ABOUT:
            DoAbout(guiItem);
            break;

        case IDM_HELP_CONTENTS:
            DoHelpContents(guiItem.GetHwndMain());
            break;

        case IDM_HELP_CONTENTSORDER:
            DoHelpContents(guiItem.GetHwndMain(),"::/Ordering_SmartVersion.htm");
            break;

        case IDM_HELP_WEBSITE:
            DoVisitWebSite(guiItem,IDS_HELP_WEBSITE_URL);
            break;

        case IDM_HELP_WEBSITEWINIMAGE:
            DoVisitWebSite(guiItem,IDS_HELP_WEBSITEWINIMAGE_URL);
            break;

        case IDM_HELP_WEBORDERSITE:
            DoVisitWebSite(guiItem,IDS_HELP_WEBORDERSITE_URL);
            break;

        case IDM_SWITCHPANE:
             {
              //SetFocus(guiItem.hwndTreeView);
                HWND hWndFocus=GetFocus();
                if (hWndFocus==guiItem.hwndTreeView)
                    SetFocus(guiItem.hwndLV);
                if (hWndFocus==guiItem.hwndLV)
                    SetFocus(guiItem.hwndTreeView);
            }
            break;
    }
    return 0;
}



LRESULT WNDMAIN::DoNotifyTips(HWND hwnd, UINT uMessage, WPARAM wparam, LPARAM lparam)
{

    LPTOOLTIPTEXT   lpToolTipText;
    TCHAR           szBuffer[MAX_PATH];
    LPNMHDR         lpnmhdr;


    lpToolTipText = (LPTOOLTIPTEXT)lparam;
    lpnmhdr = (LPNMHDR)lparam;

    if (lpToolTipText->hdr.code == TTN_POP)
    {
        guiItem.RefreshNormalStatusBar(DfsFileAndInfo);
    }

    if (lpToolTipText->hdr.code == TTN_NEEDTEXT)
    {
        int iln;


        iln=LoadString(ghInstRes,
                   (UINT)lpToolTipText->hdr.idFrom,   // string ID == command ID
                   szBuffer,
                   sizeof(szBuffer)/sizeof(TCHAR));

        guiItem.UpdateStatusBar(guiItem.hwndSB,szBuffer, 0, 0);

//        lpToolTipText->lpszText = szBuffer;
// Depending on what is entered into the hInst parameter of TOOLTIPTEXT
// structure, the lpszText member can be a buffer or an INTEGER VALUE
// obtained from MAKEINTRESOURCE()...

        lpToolTipText->hinst = ghInstRes;
        lpToolTipText->lpszText = MAKEINTRESOURCE(lpToolTipText->hdr.idFrom);


        return iln>0;
    }
    return 0;
}




LRESULT WNDMAIN::MsgNotify(HWND hwnd, UINT uMessage, WPARAM wparam, LPARAM lparam)
{
    NMHDR* pnmHdr = (NMHDR*)lparam;
    UINT uId = (UINT)wparam;

    if (DfsFileAndInfo.fNotifyAccepted)
    {
        if (pnmHdr->hwndFrom == guiItem.hwndTreeView)
            return DoNotifyTreeView(guiItem,DfsFileAndInfo,pnmHdr);
        if (pnmHdr->hwndFrom == guiItem.hwndLV)
            return DoNotifyListView(guiItem,DfsFileAndInfo,pnmHdr);
        if (pnmHdr->hwndFrom == guiItem.hwndToolTip)
            return DoNotifyToolTip(guiItem,DfsFileAndInfo,pnmHdr);
        #ifdef _DEBUG
        {
            char sz[MAX_PATH+256]="";
            if (guiItem.hwndLV!=NULL)
              wsprintf(sz,"\n Notif = %x, Tree=%x,LV=%x,LV Header=%x\n",pnmHdr->hwndFrom,guiItem.hwndTreeView,guiItem.hwndLV,ListView_GetHeader(guiItem.hwndLV));
            //OutputDebugString(sz);
        }
        #endif

        if (guiItem.hwndLV!=NULL)
          if (pnmHdr->hwndFrom == ListView_GetHeader(guiItem.hwndLV))
        {
            return DoNotifyHeaderOfListView(guiItem,DfsFileAndInfo,pnmHdr);
        }
    }

    if ((pnmHdr->code == TTN_NEEDTEXT) || (pnmHdr->code == TTN_POP))
        return DoNotifyTips(hwnd, uMessage, wparam, lparam);

    return 0;
}

LRESULT WNDMAIN::MsgContextMenu(HWND hwnd, UINT uMessage, WPARAM wparam, LPARAM lparam)
{
        POINT pt;

        pt.x = GET_X_LPARAM(lparam);  // horizontal position of cursor
        pt.y = GET_Y_LPARAM(lparam);  // vertical position of cursor


        if ((pt.x == -1) && (pt.y==-1) && (GetFocus()==guiItem.hwndLV))
        {
            DWORD i;
            DWORD dwCount = ListView_GetItemCount (guiItem.hwndLV);
            DWORD dwFocusItem=0;
            RECT rectLB,rectItem;
            for (i=0;i<dwCount;i++)
            {
                if (ListView_GetItemState(guiItem.hwndLV,i,LVIS_FOCUSED)!=0)
                {
                    dwFocusItem=i;
                    break;
                }
            }

            ListView_GetItemRect(guiItem.hwndLV,dwFocusItem,&rectItem, LVIR_LABEL  );

            GetWindowRect(guiItem.hwndLV,&rectLB);

            pt.x = rectLB.left + rectItem.left;
            pt.y = rectLB.top + ((rectItem.top + rectItem.bottom)/2);
        }

        if ((pt.x == -1) && (pt.y==-1) && (GetFocus()==guiItem.hwndTreeView))
        {
            DWORD dwFocusItem=0;
            RECT rectLB,rectItem;
            HTREEITEM hti =  TreeView_GetSelection(guiItem.hwndTreeView);

            if (hti != NULL)
            {
                TreeView_GetItemRect(guiItem.hwndTreeView,hti,&rectItem,TRUE);
                GetWindowRect(guiItem.hwndTreeView,&rectLB);
                pt.x = rectLB.left + rectItem.left;
                pt.y = rectLB.top + ((rectItem.top + rectItem.bottom)/2);
            }
        }


        if ((pt.x != -1) || (pt.y!=-1))
        {
            RECT rectLV,rectTV;
            POINT ptAbs,ptAbsTrue;
            GetWindowRect(guiItem.hwndLV,&rectLV);
            GetWindowRect(guiItem.hwndTreeView,&rectTV);
            ptAbs=pt;
            ClientToScreen(hwnd,&ptAbs);

            GetCursorPos(&ptAbsTrue);
            if (PtInRect(&rectTV,ptAbsTrue))
                guiItem.DoPopupMenu(&pt,TRUE,FALSE);
            if (PtInRect(&rectLV,ptAbsTrue))
                guiItem.DoPopupMenu(&pt,FALSE,guiItem.fCurrentColumnDirList);
        }
        return 0;
}

LRESULT WNDMAIN::MsgDrawItem(HWND hwnd, UINT uMessage, WPARAM wparam, LPARAM lparam)
{
    LPDRAWITEMSTRUCT lpdis=(LPDRAWITEMSTRUCT)lparam;
    UINT uId = (UINT)wparam;


    return TRUE;
}

/*
LRESULT MsgActivateApp(HWND hwnd, UINT uMessage, WPARAM wparam, LPARAM lparam)
{
    SetFocus(hwnd);
    return 0;
}
*/

LRESULT WNDMAIN::WndProc(HWND   hwnd,
                         UINT   uMessage,
                         WPARAM wparam,
                         LPARAM lparam)
{

    switch (uMessage)
    {
        case WM_CREATE:
            return MsgCreate(hwnd,uMessage,wparam,lparam);

        case WM_DESTROY:
            return MsgDestroy(hwnd,uMessage,wparam,lparam);

        case WM_SIZE:
            return MsgSize(hwnd,uMessage,wparam,lparam);

        case WM_MOUSEMOVE:
            return MsgMouseMove(hwnd,uMessage,wparam,lparam);

        case WM_LBUTTONDOWN:
            return MsgLButtonDown(hwnd,uMessage,wparam,lparam);

        case WM_LBUTTONUP:
            return MsgLButtonUp(hwnd,uMessage,wparam,lparam);

        case WM_SETFOCUS:
            return MsgSetFocus(hwnd,uMessage,wparam,lparam);

        case WM_PAINT:
            return MsgPaint(hwnd,uMessage,wparam,lparam);

        case WM_COMMAND:
            return MsgCommand(hwnd,uMessage,wparam,lparam);

        case WM_DROPFILES:
            return MsgDropFiles(hwnd,uMessage,wparam,lparam);

        case WM_NOTIFY:
            return MsgNotify(hwnd,uMessage,wparam,lparam);

        case WM_CONTEXTMENU:
            return MsgContextMenu(hwnd,uMessage,wparam,lparam);

        case WM_MENUSELECT:
            return MsgMenuSelect(hwnd,uMessage,wparam,lparam);

            /*
        case WM_ACTIVATEAPP:
            return MsgActivateApp(hwnd,uMessage,wparam,lparam);
            */
    }
//    return DispMessage(&msdiMain, hwnd, uMessage, wparam, lparam);
    return DefWindowProc(hwnd, uMessage, wparam, lparam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes=0;
    if (message==WM_CREATE)
      {
        LPCREATESTRUCT lpCreateStruct = (LPCREATESTRUCT)lParam;
        WNDMAIN* pWndMain = new WNDMAIN(hwnd,lpCreateStruct);
        if(!pWndMain)
          return FALSE;
        MySetWindowLongPtr(hwnd, 0, (LONG_PTR)pWndMain);
        lRes= TRUE;
      }
    WNDMAIN* pWndMain = (WNDMAIN*)::MyGetWindowLongPtr(hwnd, 0);
    if (pWndMain != NULL)
      lRes = pWndMain->WndProc(hwnd,message,wParam,lParam);

    if (message==WM_DESTROY)
    {
        ::MySetWindowLongPtr(hwnd, 0, (LONG_PTR)0);
        delete(pWndMain);

        {
            void* ptr=DfsMalloc(7);
            DfsFree(ptr);
        }
    }
    else if (pWndMain == NULL)
        return DefWindowProc(hwnd, message,wParam,lParam);
    return lRes;
}



//
//  FUNCTION: WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
//
//  PURPOSE: calls initialization function, processes message loop
//
//  PARAMETERS:
//
//    hInstance - The handle to the instance of this application that
//          is currently being executed.
//
//    hPrevInstance - This parameter is always NULL in Win32
//          applications.
//
//    lpCmdLine - A pointer to a null terminated string specifying the
//          command line of the application.
//
//    nCmdShow - Specifies how the main window is to be diplayed.
//
//  RETURN VALUE:
//    If the function terminates before entering the message loop,
//    return FALSE.
//    Otherwise, return the WPARAM value sent by the WM_QUIT message.
//
//
//  COMMENTS:
//
//    Windows recognizes this function by name as the initial entry point
//    for the program.  This function calls the initialization routine
//    It then executes a message retrieval and dispatch loop that is the
//    top-level control structure for the remainder of execution.  The
//    loop is terminated when a WM_QUIT  message is received, at which
//    time this function exits the application instance by returning the
//    value passed by PostQuitMessage().
//
//    If this function must abort before entering the message loop, it
//    returns the conventional value NULL.
//

//
//  FUNCTION: InitApplication(HINSTANCE, int)
//
//  PURPOSE: Initializes window data and registers window class.
//
//  PARAMETERS:
//    hInstance - The handle to the instance of this application that
//                is currently being executed.
//    nCmdShow  - Specifies how the main window is to be displayed.
//
//  RETURN VALUE:
//    TRUE  - Success
//    FALSE - Initialization failed
//
//  COMMENTS:
//
//    This function is called at application initialization time.  It
//    performs initialization tasks for the current application instance.
//    Unlike Win16, in Win32, each instance of an application must register
//    window classes.
//
//    In this function, we initialize a window class by filling out a data
//    structure of type WNDCLASS and calling the Windows RegisterClass()
//    function.  Then we create the main window and show it.
//
//


BOOL  InitApplicationData(HINSTANCE hInstance,BOOL fFirstLoad,BOOL fLangModified,BOOL fPrefUserModified)
{

    // Load the application name and description strings.
    crc32(0,(BYTE*)" ",1); // init CRC32

    LoadString(hInstance, IDS_APPNAME, szAppName, sizeof(szAppName));
    LoadString(hInstance, IDS_DESCRIPTION, szTitle, sizeof(szTitle));

    hBitmapDown= (HBITMAP)LoadImage(hInstance,
            MAKEINTRESOURCE(IDB_BITMAPDOWN), IMAGE_BITMAP, 0, 0,
            LR_LOADMAP3DCOLORS /*|LR_LOADTRANSPARENT*/);

    hBitmapUp= (HBITMAP)LoadImage(hInstance,
            MAKEINTRESOURCE(IDB_BITMAPUP), IMAGE_BITMAP, 0, 0,
            LR_LOADMAP3DCOLORS /*|LR_LOADTRANSPARENT*/);

    hBmpToolbar = NULL;
    return TRUE;
}

void UnInitApplication()
{
    IOConfig_dfsUnInitVirtualFileNameSpace((int)TRUE);
    if (hBitmapDown!=NULL)
        DeleteBitmap(hBitmapDown);
    if (hBitmapUp!=NULL)
        DeleteBitmap(hBitmapUp);
    if (hBmpToolbar!=NULL)
        DeleteBitmap(hBmpToolbar);
}

HWND InitApplication(HINSTANCE hInstance, int nCmdShow)
{
    WNDCLASSEX wc;
    HWND       hwnd; // Main window handle.

    InitCommonControls();

    {
     INITCOMMONCONTROLSEX icex;

       icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
       icex.dwICC   = ICC_COOL_CLASSES|ICC_BAR_CLASSES;
       InitCommonControlsEx(&icex);
    }
    /*
    // Ensure that the common control DLL is loaded.
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_BAR_CLASSES|ICC_LISTVIEW_CLASSES |ICC_PROGRESS_CLASS |ICC_TAB_CLASSES |ICC_TREEVIEW_CLASSES ;
    InitCommonControlsEx(&icex);
    */


    SetWhistlerThemeOk();
    //OleInitialize(NULL);
    InitLangInfo();

    if (!InitApplicationData(hInstance,TRUE,TRUE,TRUE))
        return NULL;

    // Save the instance handle in static variable, which will be used in
    // many subsequence calls from this application to Windows.

    ghInst = hInstance; // Store instance handle in our global variable
    ghInstRes = hInstance; // Store instance handle in our global variable

    // Fill in window class structure with parameters that describe the
    // main window.

    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = /*CS_HREDRAW | CS_VREDRAW*/0; // Class style(s).
    wc.lpfnWndProc   = (WNDPROC)WndProc;        // Window Procedure
    wc.cbClsExtra    = 0;                       // No per-class extra data.
    wc.cbWndExtra    = sizeof(WNDMAIN*);;                       // No per-window extra data.
    wc.hInstance     = hInstance;               // Owner of this class
    wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON)); // Icon name from .RC
    wc.hCursor       = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_SPLITTER)); // Cursor
    wc.hbrBackground = (HBRUSH)NULL;//(COLOR_3DFACE + 1); // Default color
    wc.lpszMenuName  = NULL;//= MAKEINTRESOURCE(IDM_MAIN);   // menu ID
    wc.lpszClassName = szAppName;               // Name to register as
    wc.hIconSm       = (HICON)LoadImage(hInstance,      // Load small icon image
                                 MAKEINTRESOURCE(IDI_APPICON),
                                 IMAGE_ICON,
                                 16, 16,
                                 0);

    // Register the window class and return FALSE if unsuccesful.

    if (!RegisterClassEx(&wc))
    {
        //Assume we are running on NT where RegisterClassEx() is
        //not implemented, so let's try calling RegisterClass().

        if (!RegisterClass((LPWNDCLASS)&wc.style))
            return NULL;
    }


    // Create a main window for this application instance.
    hwnd = CreateWindow(szAppName,           // See RegisterClass() call
                        szTitle,             // Text for window title bar
                        WS_OVERLAPPEDWINDOW, // Window style
                        90, 90,              // x, y
                        400, 600,            // cx, cy
                        NULL,                // Overlapped has no parent
                        NULL,                // Use the window class menu
                        hInstance,           // This instance owns this window
                        NULL                 // Don't need data in WM_CREATE
    );


    // If window could not be created, return "failure"
    if (!hwnd)
        return NULL;



    // Make the window visible; update its client area; and return "success"
    ShowWindow(hwnd, nCmdShow);  // Show the window
    UpdateWindow(hwnd);          // Sends WM_PAINT message
    SetFocus(hwnd);



    return hwnd;                 // We succeeded...
}



int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    MSG msg;
    HACCEL hAccel;
    HWND hWndMain;


//    HANDLE hAccelTable;

//    hInst = hInstance;


    SetErrorMode(SEM_NOALIGNMENTFAULTEXCEPT);   // FOR RISC and ia64
    //if (SkipDumpCrashInCommandLine(lpCmdLine))
    //    InstallDumperHandler("SmartVersionGui");

    if (lstrcmpi(lpCmdLine,"/HIDEREGISTER")==0)
        fGlobalHideRegisterOrUninstall=TRUE;
    if (lstrcmpi(lpCmdLine,"/UNINSTALL")==0)
        fGlobalHideRegisterOrUninstall=TRUE;

    {
        typedef struct {
            SHFILEINFO sfi;
            char dummy[1024*2];
        } DUM;
        DUM dum;
    memset(&dum.sfi, 0, sizeof(dum.sfi));
    //HIMAGELIST hil =  (HIMAGELIST)(
    DWORD_PTR dw=   SHGetFileInfo (
            "C:\\",
            FILE_ATTRIBUTE_DIRECTORY,
            &dum.sfi,
            sizeof(dum.sfi),
            SHGFI_SYSICONINDEX | SHGFI_SMALLICON  | SHGFI_USEFILEATTRIBUTES);
    }


	if (lpCmdLine != NULL)
	{
		if (lstrlen(lpCmdLine) > 8)
			if (memcmp(lpCmdLine, "DoRemix ", 8) == 0)
			{
				LPCSTR lpCommandLineAnsi = lpCmdLine + 8;
				DWORD dwSize = (lstrlenA(lpCommandLineAnsi) * 4) + 0x10;
				dfwcharp pCommandLine = (dfwcharp)DfsMalloc(dwSize + 0x10);
				ConvertAnsiToUnicode(lpCommandLineAnsi, pCommandLine, dwSize / sizeof(dfwchar));
				int iRet = (int)MixDfs(NULL, pCommandLine);
				DfsFree(pCommandLine);
				return iRet;
			}


		if (lstrlen(lpCmdLine) > 15)
			if (memcmp(lpCmdLine, "RunCommandLine ", 15) == 0)
			{
				LPCSTR lpCommandLineAnsi = lpCmdLine + 15;
				DWORD dwSize = (lstrlenA(lpCommandLineAnsi) * 4) + 0x10;
				dfwcharp pCommandLine = (dfwcharp)DfsMalloc(dwSize + 0x20);
				*(pCommandLine + 0) = 's';
				*(pCommandLine + 1) = 'm';
				*(pCommandLine + 2) = 'v';
				*(pCommandLine + 3) = ' ';
				ConvertAnsiToUnicode(lpCommandLineAnsi, pCommandLine + 4, dwSize / sizeof(dfwchar));
				int iRet = (int)PerformCommandLine(pCommandLine);
				DfsFree(pCommandLine);
				return iRet;
			}
	}
    // Initialize application by setting up the main window.
    hWndMain = InitApplication(hInstance, nCmdShow);
    if (hWndMain==NULL)
    {
        return FALSE;           // Exits if unable to initialize
    }

    //hAccelTable = LoadAccelerators(hInstance, szAppName);

    if (NOERROR != OleInitialize(NULL))
        return FALSE;

    MessageFilterHook=SetWindowsHookEx(WH_MSGFILTER, (HOOKPROC)HelpMessageFilterHook,
            hInstance, GetCurrentThreadId());
    // Acquire and dispatch messages until a WM_QUIT message is received.

    hAccel = LoadAccelerators(hInstance,"ACCEL");

    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(hWndMain, hAccel, &msg))
        {
            TranslateMessage(&msg);  // Translates virtual key codes
            DispatchMessage(&msg);   // Dispatches message to window
        }
    }

    OleUninitialize();
    UnhookWindowsHookEx(MessageFilterHook);

    UnInitApplication();

    return (int)msg.wParam;  // Returns the value from PostQuitMessage
}


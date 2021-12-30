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



////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef void (WINAPI* INITCOMMONCONTROLS) ();

typedef struct
{
char szShortExt[4];
LPSTR lpLongExt;
DWORD_PTR dwIcon;
HICON hIcon;
int   iIcon;
char  szTypeName[80];
BOOL  fRegisteredType;
} EXTINFO;
typedef EXTINFO FAR * LPEXTINFO;

class EXTINFOHEAP
{
private:
BOOL fExtInfoDirFilled;
LPEXTINFO lpExtInfoHeap;
DWORD dwNbExtInfo;
DWORD dwNbExtInfoAllocate;
BOOL AddExtInfo(LPEXTINFO lpExtInfo);
BOOL fApiLoadedOk;
BOOL fApiLoadedOkAndLook95;
HIMAGELIST hImageSmall,hImageLarge;
HIMAGELIST hUseSmall,hUseLarge;
DWORD dwShellVer;

HINSTANCE hLibCommCtl;
HINSTANCE hLibShell;
HBITMAP hBm;
EXTINFO ExtInfoDir,ExtInfoDirOpened;

BOOL CheckFillExtInfoDir();
public:
EXTINFOHEAP() ;
~EXTINFOHEAP() ;
BOOL GetExtInfo(LPSTR lpExt,BOOL fOnlyExt,LPEXTINFO lpExtInfo);
BOOL DrawExtIcon(HDC hDC,LPSTR lpExt,BOOL fOnlyExt,int x,int y,BOOL fRep,BOOL fLarge);
BOOL IsApiLoaded() { return fApiLoadedOk; } ;
BOOL IsApiLoadedAndLook95() { return fApiLoadedOkAndLook95; } ;
int GetItemIndexImage(LPSTR lpExt,BOOL fOnlyExt);
INITCOMMONCONTROLS lpInitCommonControls;
#ifndef WDIFSTRM
HWND CreateListView (HWND hWndParent,BOOL fTitleBar,LISTFILETYPE ListFileType);
#endif
EXTINFO GetExtInfoDir() { return ExtInfoDir ; } ;
int DoPropertySheet(LPVOID lp);
int DoPropertySheetW(LPVOID lp);
BOOL DoInitCommonControlsEx(LPINITCOMMONCONTROLSEX lpInitCtrls);
void doSHAddToRecentDocs(UINT  uFlags,LPCVOID  pv);
BOOL GetDirectoryExplorer(HWND hWnd,LPSTR lpDir,DWORD dwFlags,BOOL fAllowCreate,LPCSTR lpszTitle);
} ;

static BOOL IsWin95OrGreatherLook()
{
WORD wVer;
  wVer = LOWORD(GetVersion());
  wVer = (((WORD)LOBYTE(wVer)) << 8) | (WORD)HIBYTE(wVer);

  return (wVer >= 0x035F);
}


typedef struct _myDllVersionInfo
{
    DWORD cbSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformID;
}myDLLVERSIONINFO;

#define PACKVERSION(major,minor) MAKELONG(minor,major)
typedef HRESULT (CALLBACK* myDLLGETVERSIONPROC)(myDLLVERSIONINFO *);

DWORD GetDllVersion(HINSTANCE hinstDll)
{

    //HINSTANCE hinstDll;
    DWORD dwVersion = 0;

    //hinstDll = LoadLibrary(lpszDllName);

    if(hinstDll)
    {
        myDLLGETVERSIONPROC pDllGetVersion;

        pDllGetVersion = (myDLLGETVERSIONPROC) GetProcAddress(hinstDll, "DllGetVersion");

/*Because some DLLs might not implement this function, you
  must test for it explicitly. Depending on the particular
  DLL, the lack of a DllGetVersion function can be a useful
  indicator of the version.
*/
        if(pDllGetVersion)
        {
            myDLLVERSIONINFO dvi;
            HRESULT hr;

            ZeroMemory(&dvi, sizeof(dvi));
            dvi.cbSize = sizeof(dvi);

            hr = (*pDllGetVersion)(&dvi);

            if(SUCCEEDED(hr))
            {
                dwVersion = PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
            }
        }

        //FreeLibrary(hinstDll);
    }
    return dwVersion;
}


EXTINFOHEAP::EXTINFOHEAP()
{
  fExtInfoDirFilled=FALSE;
  dwNbExtInfoAllocate = dwNbExtInfo = 0 ;
  lpExtInfoHeap = NULL;


  hLibCommCtl = LoadLibrary("COMCTL32.DLL");
  hLibShell = LoadLibrary("SHELL32.DLL");
  dwShellVer = GetDllVersion(hLibShell);

  hImageSmall = hImageLarge = NULL;
  hUseSmall = hUseLarge = NULL;

  fApiLoadedOk = TRUE;

  fApiLoadedOkAndLook95 = fApiLoadedOk && IsWin95OrGreatherLook();
} ;

EXTINFOHEAP::~EXTINFOHEAP()
{
DWORD i;
  if (hImageSmall != NULL)
      ImageList_Destroy(hImageSmall);
  if (hImageLarge != NULL)
      ImageList_Destroy(hImageLarge);

  for (i=0;i<dwNbExtInfo;i++)
    if ((lpExtInfoHeap + i)->lpLongExt!=NULL)
      GlobalFreePtr((lpExtInfoHeap + i)->lpLongExt);
  if (lpExtInfoHeap!=NULL) GlobalFreePtr(lpExtInfoHeap) ;
  if (hLibShell!=NULL)
    FreeLibrary(hLibShell);
  if (hLibCommCtl!=NULL)
    FreeLibrary(hLibCommCtl);
} ;



#define GRANALLOCEXTINFOHEAP (0x10)
BOOL EXTINFOHEAP::AddExtInfo(LPEXTINFO lpExtInfoAdd)
{
  if (dwNbExtInfoAllocate <= dwNbExtInfo)
    {
      dwNbExtInfoAllocate += GRANALLOCEXTINFOHEAP;
      lpExtInfoHeap = (LPEXTINFO)ReallocOrAlloc(lpExtInfoHeap,dwNbExtInfoAllocate*sizeof(EXTINFO));
    }
  *(lpExtInfoHeap + dwNbExtInfo) = *lpExtInfoAdd;
  dwNbExtInfo++;
  return TRUE;
}

LPSTR GetExtensionFromName(LPSTR lpLine,BOOL fOnlyExt,LPSTR lpszExt)
{
DWORD i;
LPSTR lpExt;
  if (fOnlyExt)
    return lpLine;
  else
    {
        BOOL fCop=FALSE;
          lpExt=lpszExt;
          for (i=0;i<(DWORD)lstrlen(lpLine);i++)
            {
              if ((*(lpLine+i)=='\t') || (*(lpLine+i)=='\0'))
                break;
              if (fCop)
                *(lpExt++) = *(lpLine+i);
              if (*(lpLine+i)=='.')
                fCop=TRUE;
            }
           *lpExt = '\0';
           lpExt=lpszExt;
        }
  return lpExt;
}

BOOL EXTINFOHEAP::GetExtInfo(LPSTR lpLine,BOOL fOnlyExt,LPEXTINFO lpExtInfo)
{
int ilnExt;
LPSTR lpExt;
char szExt[MAX_PATH];
LPEXTINFO lpExtInfoParc=lpExtInfoHeap;
EXTINFO ExtInfoAdd;
DWORD i;
  lpExt=GetExtensionFromName(lpLine,fOnlyExt,szExt);
  ilnExt=lstrlen(lpExt);

  for (i=0;i<dwNbExtInfo;i++,lpExtInfoParc++)
    //if (lstrcmp((ilnExt<=3) ? lpExtInfoParc->szShortExt : lpExtInfoParc->lpLongExt,lpExt)==0)
    if (lstrcmp((lpExtInfoParc->lpLongExt==NULL) ? lpExtInfoParc->szShortExt : lpExtInfoParc->lpLongExt,lpExt)==0)
      {
        *lpExtInfo = *lpExtInfoParc;
        return TRUE;
      }

  memset(&ExtInfoAdd,0,sizeof(ExtInfoAdd));

  {
  char szPath[MAX_PATH];
  char szFnExt[MAX_PATH];
  //HANDLE hFile;
  SHFILEINFO shfi;
  DWORD_PTR dwIcon;
  UINT uiOldError;
    GetTempPath(sizeof(szPath)-1,szPath);

    //GetTempFileName(szPath,"WMI",(UINT)dwNbExtInfo,szFn);
    //lstrcpy(szFn+lstrlen(szFn)-3,lpExt);
    //wsprintf(szFn,"%sWMI__.%s",(LPSTR)szPath,lpExt);
    wsprintf(szFnExt,".%s",lpExt);

    if (ilnExt>3)
      ExtInfoAdd.lpLongExt = (LPSTR)GlobalAllocPtr(GHND,ilnExt+2);
    lstrcpy((ilnExt<=3) ? ExtInfoAdd.szShortExt : ExtInfoAdd.lpLongExt,lpExt);
/*
    hFile = CreateFile(szFnExt,GENERIC_WRITE | GENERIC_READ,FILE_SHARE_READ |
                                FILE_SHARE_WRITE,NULL,
                                CREATE_ALWAYS,FILE_FLAG_DELETE_ON_CLOSE|FILE_ATTRIBUTE_TEMPORARY,
                                NULL);
*/
    uiOldError = SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOALIGNMENTFAULTEXCEPT);
    dwIcon = SHGetFileInfo(szFnExt,FILE_ATTRIBUTE_NORMAL,&shfi,sizeof(shfi),
                 SHGFI_SMALLICON | SHGFI_TYPENAME | SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES
                 // SHGFI_SYSICONINDEX give error on .URL on NT 4 beta 1234
                 );
    SetErrorMode(uiOldError);

    ExtInfoAdd.dwIcon = dwIcon;
    ExtInfoAdd.hIcon = shfi.hIcon;
    ExtInfoAdd.iIcon = shfi.iIcon;
    ExtInfoAdd.fRegisteredType=TRUE;
    lstrcpy(ExtInfoAdd.szTypeName,shfi.szTypeName);
    //CloseHandle(hFile);
  }
  AddExtInfo(&ExtInfoAdd);
  *lpExtInfo = ExtInfoAdd;
  return TRUE;
}

BOOL EXTINFOHEAP::CheckFillExtInfoDir()
{
    if (!fExtInfoDirFilled)
    {
        TCHAR szWinDir[MAX_PATH];
        DWORD_PTR dwIcon;
        SHFILEINFO shfi;
          GetWindowsDirectory(szWinDir,sizeof(szWinDir)-1);
          dwIcon = SHGetFileInfo(szWinDir,FILE_ATTRIBUTE_DIRECTORY,&shfi,sizeof(shfi),
                 SHGFI_SMALLICON | SHGFI_TYPENAME| SHGFI_SYSICONINDEX  | SHGFI_USEFILEATTRIBUTES);
          //ListView_SetImageList(hWndList,hUseSmall=(HIMAGELIST)dwIcon,LVSIL_SMALL);
#ifdef _DEBUG
          //{char sz[500]; wsprintf(sz,"dwIcon=%08x%08x %x",(DWORD)(dwIcon>>32),(DWORD)(dwIcon),8); MessageBox(0,sz,sz,0);}
#endif
          ExtInfoDir.dwIcon = dwIcon;
          ExtInfoDir.hIcon = shfi.hIcon;
          ExtInfoDir.iIcon = shfi.iIcon;
          lstrcpy(ExtInfoDir.szTypeName,shfi.szTypeName);

          dwIcon = SHGetFileInfo(szWinDir,FILE_ATTRIBUTE_DIRECTORY,&shfi,sizeof(shfi),
                 SHGFI_SMALLICON | SHGFI_TYPENAME| SHGFI_SYSICONINDEX | SHGFI_OPENICON | SHGFI_USEFILEATTRIBUTES);
          ExtInfoDirOpened.dwIcon = dwIcon;
          ExtInfoDirOpened.hIcon = shfi.hIcon;
          ExtInfoDirOpened.iIcon = shfi.iIcon;
          lstrcpy(ExtInfoDirOpened.szTypeName,shfi.szTypeName);
    }
    fExtInfoDirFilled=TRUE;
    return fExtInfoDirFilled;
}

BOOL EXTINFOHEAP::DrawExtIcon(HDC hDC,LPSTR lpExt,BOOL fOnlyExt,int x,int y,BOOL fRep,BOOL fLarge)
{
EXTINFO ExtInfo;
  if (!fApiLoadedOkAndLook95)
    return FALSE;
  GetExtInfo(lpExt,fOnlyExt,&ExtInfo);
  CheckFillExtInfoDir();

  ImageList_Draw(
    fLarge ? (hUseLarge):(HIMAGELIST)ExtInfo.dwIcon,
    //fRep ? 4 :ExtInfo.iIcon,
    fRep ? (ExtInfoDir.iIcon) :ExtInfo.iIcon,
    hDC,
    x,
    y,
    ILD_TRANSPARENT
   );
  return TRUE;
}

EXTINFOHEAP ExtInfoHeap ;

BOOL RegisterExt(LPSTR lpExt,BOOL fOnlyExt,BOOL &fRegisteredType,LPSTR lpTypeName)
{
EXTINFO ExtInfo;
  fRegisteredType=FALSE;
  if (!ExtInfoHeap.IsApiLoadedAndLook95())
    return FALSE;

  if (lpExt != NULL)
      ExtInfoHeap.GetExtInfo(lpExt,fOnlyExt,&ExtInfo);
  else
      ExtInfo = ExtInfoHeap.GetExtInfoDir();
  if (lpTypeName != NULL)
      lstrcpy(lpTypeName,ExtInfo.szTypeName);
  fRegisteredType=ExtInfo.fRegisteredType;
  return TRUE;
}

BOOL DrawExtIcon(HDC hDC,LPSTR lpExt,BOOL fOnlyExt,int x,int y,BOOL fRep,BOOL fLarge)
{
  return ExtInfoHeap.DrawExtIcon(hDC,lpExt,fOnlyExt,x,y,fRep,fLarge);
}

int EXTINFOHEAP::GetItemIndexImage(LPSTR lpExt,BOOL fOnlyExt)
{
EXTINFO ExtInfo;

  if (!fApiLoadedOk)
    return FALSE;
  CheckFillExtInfoDir();
  if (lpExt==NULL)
      return fOnlyExt ? ExtInfoDirOpened.iIcon :ExtInfoDir.iIcon;
  GetExtInfo(lpExt,fOnlyExt,&ExtInfo);
  return ExtInfo.iIcon;
}

int GetItemIndexImage(LPSTR lpExt,BOOL fOnlyExt)
{
    return ExtInfoHeap.GetItemIndexImage(lpExt,fOnlyExt);
}

#define NUM_COLUMNS (5)






BOOL IsListViewPossible()
{
    return (ExtInfoHeap.IsApiLoaded());
}



BOOL IsMutlipleTypeIconAvaiable()
{
    return (ExtInfoHeap.IsApiLoadedAndLook95());
}

BOOL GetExtensionFromExtInRegistry(LPSTR lpExt,LPSTR lpDescExt)
{
TCHAR szKeyName[MAX_PATH]="";
TCHAR szExtName[MAX_PATH];
long lSize=sizeof(szKeyName)-1;
    wsprintf(szExtName,".%s",lpExt);
    if (RegQueryValue(HKEY_CLASSES_ROOT,szExtName,szKeyName,&lSize)
          !=ERROR_SUCCESS)
          return FALSE;

    lSize=sizeof(szExtName)-1;
    if (RegQueryValue(HKEY_CLASSES_ROOT,szKeyName,szExtName,&lSize)
          !=ERROR_SUCCESS)
          return FALSE;


    lstrcpy(lpDescExt,szExtName);
    return TRUE;
}


void EXTINFOHEAP::doSHAddToRecentDocs(UINT  uFlags,LPCVOID  pv)
{
    if (!ExtInfoHeap.IsApiLoaded())
        return ;
    SHAddToRecentDocs(uFlags,pv);
}

void doSHAddToRecentDocs(UINT  uFlags,LPCVOID  pv)
{
    ExtInfoHeap.doSHAddToRecentDocs(uFlags,pv);
}

//#ifdef WIN32
/*
long doSHGetMalloc(LPMALLOC * ppMalloc);
LPITEMIDLIST doSHBrowseForFolder(LPBROWSEINFO lpbi);
BOOL doSHGetPathFromIDList(LPCITEMIDLIST pidl,LPSTR pszPath);
//#endif
*/
/*
You want to install a callback, and on receipt of the
BFFM_INITIALIZED message, you send a BFFM_SETSELECTION message to
the window to change the initial selection.
*/
#ifndef BIF_RETURNONLYFSDIRS
#define BIF_RETURNONLYFSDIRS   0x0001
#endif

int CALLBACK GDE_BrowseCallbackProc(
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

#define newBIF_EDITBOX            0x0010   // Add an editbox to the dialog
#define newBIF_VALIDATE           0x0020   // insist on valid result (or CANCEL)

#define newBIF_NEWDIALOGSTYLE     0x0040   // Use the new dialog layout with the a
                                        // Caller needs to call OleInitialize()
#define newBIF_USENEWUI           (newBIF_NEWDIALOGSTYLE | newBIF_EDITBOX)

BOOL EXTINFOHEAP::GetDirectoryExplorer(HWND hWnd,LPSTR lpDir,DWORD dwFlags,BOOL fAllowCreate,LPCSTR lpszTitle)
{
  LPMALLOC pMalloc;
  BOOL fRet=FALSE;

  if(SUCCEEDED(SHGetMalloc(&pMalloc)))
  {
    LPITEMIDLIST pidlBrowse;
    CHAR         pchFolder[MAX_PATH + 1];
    BROWSEINFO   bi;
    //CString      sTitle;

    //sTitle.LoadString(uTitleID);
    lstrcpy(pchFolder,lpDir);

    bi.hwndOwner      = hWnd;
    bi.pidlRoot       = NULL;
    bi.pszDisplayName = pchFolder;
    bi.lpszTitle      = lpszTitle; // title
    bi.ulFlags        = BIF_RETURNONLYFSDIRS;
    bi.lpfn           = GDE_BrowseCallbackProc;//NULL;
    bi.lParam         = (LPARAM)lpDir;
    bi.iImage         = 0;

    //bi.ulFlags       |= newBIF_USENEWUI;
    //bi.ulFlags       |= newBIF_EDITBOX;
    if (dwShellVer>=0x00050000)
        bi.ulFlags       |= newBIF_NEWDIALOGSTYLE;

    pidlBrowse = SHBrowseForFolder(&bi);
    if(pidlBrowse)
    {
        if (SHGetPathFromIDList(pidlBrowse, lpDir))
            fRet = TRUE;
        pMalloc->Free(pidlBrowse);

    }
        // clean-up

    pMalloc->Release();
  }
  return fRet;
}

BOOL GetDirectoryExplorer(HWND hWnd,LPSTR lpDir,DWORD dwFlags,BOOL fAllowCreate,LPCSTR lpszTitle)
{
    if (!ExtInfoHeap.IsApiLoaded())
        return FALSE;
    return ExtInfoHeap.GetDirectoryExplorer(hWnd,lpDir,dwFlags,fAllowCreate,lpszTitle);
}


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
//#include "HFileCpt.h"
#include <dos.h>
#include <commdlg.h>
#include <commctrl.h>
#include <shellapi.h>
#include <string.h>
//#include "miscutil.h"


#include "resource.h"
#include "global.h"            // prototypes specific to this application

#include "ltoolsCPP.h"
#include "ExtInfo.h"
#include "GuiItem.h"
#include "../../lib/engine/svfile/common/DirSet.h"

#include "RegCode.h"
#include "DoExtracting.h"
#include "uiMain.h"
#include "DrpOleUt.h"

//#include "MiscUtilWDFS.h"
//#include "MiscUtil.h"

#include "LoadIcon.h"
#include "../../lib/engine/svfile/common/ArrayTl.h"
#include "../../lib/engine/patchstream/common/DfsOrigMemoryMap.h"
#include "../../lib/engine/patchstream/common/DfsIoHlp.h"

#include "Droputil.h"

static HWND hWndMain=NULL;
static HWND hDlgCurrent=NULL;
static HINSTANCE hInst=NULL;

//************ PROTOTYPES FOR DROP-FILE SERVER FUNCTIONS ********************/
HDROP FAR DragCreateFiles (LPPOINT lpptMousePos, BOOL fInNonClientArea);
HDROP FAR DragAppendFile (HDROP hDrop, LPCSTR szPathname);


/****************************************************************************/
/**********************                            **************************/
/********************** DROP-FILE SERVER FUNCTIONS **************************/
/**********************                            **************************/
/****************************************************************************/

//#ifdef WIN32
//#pragma pack (4)
//#endif
typedef struct {
        UINT  uiSize;                            // Size of data structure
        POINT ptMousePos;                       // Position of mouse cursor
        BOOL  fInNonClientArea;                 // Was the mouse in the
        long lDummy;
                                                // window's non-client area
} DROPFILESTRUCT, FAR *LPDROPFILESTRUCT;


HDROP FAR DragCreateFiles (LPPOINT lpptMousePos,
                           BOOL fInNonClientArea)
{

    HDROP hDrop;
    LPDROPFILESTRUCT lpDropFileStruct;
    UINT uiSizeStruct ;

    // GMEM_SHARE must be specified because the block will
    // be passed to another application
    uiSizeStruct =sizeof(DROPFILESTRUCT) ;
    hDrop = (HDROP)GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE | GMEM_ZEROINIT,
                        uiSizeStruct + 1);

    // If unsuccessful, return NULL
    if (hDrop == NULL) return(hDrop);

    // Lock block and initialize the data members
    lpDropFileStruct = (LPDROPFILESTRUCT) GlobalLock(hDrop);
    lpDropFileStruct->uiSize = uiSizeStruct;
    lpDropFileStruct->ptMousePos = *lpptMousePos;
    lpDropFileStruct->fInNonClientArea = fInNonClientArea;
    GlobalUnlock(hDrop);
    return(hDrop);
}


HDROP FAR DragAppendFile (HDROP hDrop, LPCSTR szPathname)
{
    LPDROPFILESTRUCT lpDropFileStruct;
    LPCSTR lpCrnt;
    WORD wSize;

    if (hDrop == NULL) return NULL;
    lpDropFileStruct = (LPDROPFILESTRUCT) GlobalLock(hDrop);

    // Point to first pathname in list
    lpCrnt = (LPSTR) lpDropFileStruct + lpDropFileStruct->uiSize;

    // Search for a pathname were first byte is a zero byte
    while (*lpCrnt)         // While the 1st char of path is non-zero
       {
       while (*lpCrnt) lpCrnt++;   // Skip to zero byte
       lpCrnt++;
       }

    // Calculate current size of block
    wSize = (WORD) (lpCrnt - (LPSTR) lpDropFileStruct + 1);
    GlobalUnlock(hDrop);

    // Increase block size to accommodate new pathname being appended
    hDrop = (HDROP)GlobalReAlloc(hDrop, wSize + lstrlen(szPathname) + 1,
                          GMEM_MOVEABLE | GMEM_ZEROINIT | GMEM_SHARE);

    // Return NULL if insufficient memory
    if (hDrop == NULL) return(hDrop);

    lpDropFileStruct = (LPDROPFILESTRUCT) GlobalLock(hDrop);
    // Append the pathname to the block
    lstrcpy((LPSTR) lpDropFileStruct + wSize - 1, szPathname);
    GlobalUnlock(hDrop);
    return(hDrop);  // Return the new handle to the block
}


HWND GetClientDropFormPoint(POINT pt)
{
HWND hWnd = WindowFromPoint(pt);
  while (IsWindow(hWnd))
    {
      if (hWnd == hWndMain) return NULL;
      if (GetWindowLong(hWnd,GWL_EXSTYLE) & WS_EX_ACCEPTFILES)
        return hWnd;
      hWnd = GetParent(hWnd);
    }
  return NULL;
}


void DoPeekWait(void)
{
MSG msg;
BOOL fCancel=FALSE;
  while (!fCancel && GetMessage(&msg,NULL,0,0/*,PM_REMOVE*/))
    {
    if ((!hDlgCurrent) || (!IsDialogMessage(hDlgCurrent, &msg)))
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if ((msg.message == WM_MOUSEMOVE) || (msg.message == WM_LBUTTONUP))
            fCancel=TRUE;
      }
    }
}


BOOL DoDraging(HWND hWnd,HWND &hWndSubject,POINT & ptMousePos,BOOL &fInNonClientArea,
                            BOOL fMultiple,BOOL &fCursorModified)
{
BOOL fOkToDrop;
HCURSOR hCrsrDrpNotAllow, hCrsrDrp;
HINSTANCE hCursorInstance = GetModuleHandle(NULL);
     fCursorModified=FALSE;
     hCrsrDrpNotAllow = LoadCursor(hInst,
                                   "DRPFIL_NOTALLOWED");

     hCrsrDrp         = LoadCursor(hInst,fMultiple ? "DRPFIL_MULTIPLE" :
                                   "DRPFIL_SINGLE");

     // *** Loop for determining the drop-file
     //     client window ***
     SetCapture(hWnd);
     do {
         // Get cursor pos. & window under the cursor
         GetCursorPos(&ptMousePos);

         if ((hWndSubject=GetClientDropFormPoint(ptMousePos)) == NULL)
             {
             fOkToDrop = FALSE;
             SetCursor(hCrsrDrpNotAllow);
             fCursorModified=TRUE;
             }
         else
             {
             fOkToDrop = TRUE;
             SetCursor(hCrsrDrp);
             fCursorModified=TRUE;
             }

         // Terminate loop when mouse button is released
         DoPeekWait();
     } while (GetAsyncKeyState(VK_LBUTTON) & 0x8000);
     ReleaseCapture();

     // Free the loaded cursors from memory
     DestroyCursor(hCrsrDrpNotAllow);
     DestroyCursor(hCrsrDrp);

     if (!fOkToDrop) return FALSE;

     // Is the cursor in the window's non-client area?
     fInNonClientArea = (HTCLIENT !=
             SendMessage(hWndSubject, WM_NCHITTEST, 0,
             (LONG) MAKELONG(ptMousePos.x, ptMousePos.y)));


     // Create drop-file memory block and initialize it
     ScreenToClient(hWndSubject, &ptMousePos);

     return TRUE;
}


LONG DoSrvDrop(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo)
{
    return DoSrvDropOle(guiItem,DfsFileAndInfo);
    /* #WIMA#
POINT ptMousePos;
HDROP hDrop;
BOOL fInNonClientArea;
WORD wNumFiles=0;
DWORD i;
LPDIRINFO lpdiParc ;
char szBuf[128];
HWND hWndSubject;
BOOL fCursorModified;

    #ifdef WIN32
    #ifndef IGNOREOLEDRAG
     if (fDragSrvOle)
         return DoSrvDropOle(hWnd);
    #endif
    #endif

     // Make sure that there are some selected files
     // to be dropped
     if ((lpdi==NULL) || (dwEntDirInfo==0)) return FALSE;

     // Get the handles to the cursors that
     // will shown to the user.

     for (i=0,lpdiParc = lpdi;i<dwEntDirInfo;i++,lpdiParc++)
        if ((lpdiParc->fSel) && (!lpdiParc->fIsSubDir))
          wNumFiles ++ ;

     if (!DoDraging(hWnd,hWndSubject,ptMousePos,
                        fInNonClientArea,(wNumFiles > 1),fCursorModified)) return 0;

     hDrop = DragCreateFiles(&ptMousePos, fInNonClientArea);
     if (hDrop == NULL)
       return 0;

     if (!DoExtract(hWndMain,hWndLB,FALSE,fAskExtrDrop))
               return 0;
     for (i=0,lpdiParc = lpdi;i<dwEntDirInfo;i++,lpdiParc++)
         if ((lpdiParc->fSel) && (!lpdiParc->fIsSubDir))
         {
             int iln;
             OFSTRUCTMYEX of;
             LPSTR lpEndBuf;
             lstrcpy(szBuf,szPathExtr);
             iln = lstrlen(szBuf);
             lpEndBuf=GetLatestChar(szBuf);
             if (lpEndBuf!=NULL)
                 if (((*lpEndBuf)!=':') &&
                     ((*lpEndBuf)!='\\'))
                     lstrcat(szBuf,"\\");
                 if (fUseLfnInExtr)
                     lstrcat(szBuf,lpdiParc->longname);
                 else
                     lstrcat(szBuf,lpdiParc->szCompactName);
                 if (OpenFileMyEx(szBuf,&of,OF_PARSE)!=HFILE_MYEX_ERROR)
                 {
                     LPCSTR lpCurInPath = GetNameWithoutPath(of.szPathName);
                     for (;;)
                     {
                         LPCSTR lpNext;
                         if ((*lpCurInPath)=='.')
                             break;
                         lpNext=CharNext(lpCurInPath);
                         if (lpNext==lpCurInPath)
                         {
                             lstrcat(of.szPathName,".");
                             break;
                         }
                         lpCurInPath=lpNext;
                     }
                     hDrop = DragAppendFile(hDrop,of.szPathName);
                 }
         }
     if (hDrop != NULL)
        {
        // All pathnames appended successfully,
        // post the message
        // to the drop-file client window


        PostMessage(hWndSubject, WM_DROPFILES, (WPARAM)hDrop, 0L);


        // Don't free the memory,
        // the Dropfile client will do it
        }*/
//     return 0;
}

BOOL DoDropDoc(HWND hWnd,BOOL fCursorModified)
{

POINT ptMousePos;
//HDROP hDrop;
BOOL fInNonClientArea;
HWND hWndSubject;
/*
  if (!pMemDisk->fIsDiskUsable)
       return FALSE;
*/
  if (!DoDraging(hWnd,hWndSubject,ptMousePos,fInNonClientArea,FALSE,fCursorModified))
                    return FALSE;
/* #WIMA#

  if ((fDirty) || (szFileName[0] == '\0'))
       if (!DoMenuSave(hWnd)) return FALSE;

  hDrop = DragCreateFiles(&ptMousePos, fInNonClientArea);
  hDrop = DragAppendFile(hDrop,szFileName);
  if (hDrop == NULL)
       return FALSE;

  PostMessage(hWndSubject, WM_DROPFILES, (WPARAM)hDrop, 0L);
  */
  return TRUE;
}

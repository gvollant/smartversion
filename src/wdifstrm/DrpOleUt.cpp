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

#ifndef IGNOREOLEDRAG
#ifndef STRICT
#define STRICT
#endif

#include <ole2.h>
#include <shlobj.h>
#include <tchar.h>

#define MYCOM


#include <dos.h>
#include <commdlg.h>
#include <shellapi.h>
#include <string.h>
#include "global.h"            // prototypes specific to this application

#include "ltoolsCPP.h"
#include "../../lib/engine/patchstream/common/DfsOrigMemoryMap.h"
#include "../../lib/engine/patchstream/common/DfsIoHlp.h"
#include "ExtInfo.h"
#include "GuiItem.h"
#include "../../lib/engine/svfile/common/DirSet.h"
#include "RegCode.h"
#include "DoExtracting.h"
#include "uiMain.h"

#include "DrpOleUt.h"

#include "enumfetc.h"

//#include "fsmgrex.h"
#include "fsmgr.h"
#include "DropHook.h"

#include "dataobj.h"
#include "util.h"
#include <stdio.h>
#include "DropHook.h"
#include "DropSrc.h"

HWND hDlgCurrent = NULL;
BOOL fCancel = FALSE;

#define WM_WAITFORREWORK (WM_USER + 0x205)

HCURSOR hCurNone;
HCURSOR hCurCopy;
BOOL fIsDragCopy;
void InitializeDragOle(HINSTANCE hInst)
{
    //OleInitialize(NULL);
    hCurNone = LoadCursor(hInst,"DRPFIL_NOTALLOWED");
    hCurCopy = LoadCursor(hInst,"DRPFIL_DROPCP");
}

void UninitializeDragOle()
{
    DestroyCursor(hCurCopy);
    DestroyCursor(hCurNone);
    OleUninitialize();
}

class CFileSetManagerWima : public CFileSetManager
{
public:
  // pass dragDropOperation = TRUE for drag drop, FALSE for clipboard
  CFileSetManagerWima( BOOL dragDropOperation, HWND hWndWimaS );
 ~CFileSetManagerWima();

  // called by the drop hook when the user releases the mouse
  void UserReleasedTheMouse() { m_readyToUnzip = TRUE; }
  void SetDragOperCopy(BOOL fSet)
    { fDragOperCopy = fSet ; } ;

  // CFileSetManager overrides
  UINT GetCount();
  BOOL ReadyToUnzip();
  void Unzip();
  const char* GetSourcePath( UINT zeroBasedIndex );


private:
  BOOL m_readyToUnzip;
  BOOL fDragOperCopy;
  HWND hWndWima;
  LPSTR lpBufName;
};


class CFileSetManagerSmartVersion : public CFileSetManager
{
public:
  // pass dragDropOperation = TRUE for drag drop, FALSE for clipboard
  CFileSetManagerSmartVersion( BOOL dragDropOperation, GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo );
 ~CFileSetManagerSmartVersion();

  // called by the drop hook when the user releases the mouse
  void UserReleasedTheMouse() { m_readyToUnzip = TRUE; }
  void SetDragOperCopy(BOOL fSet)
    { fDragOperCopy = fSet ; } ;

  // CFileSetManager overrides
  UINT GetCount();
  BOOL ReadyToUnzip();
  void Unzip();
  const char* GetSourcePath( UINT zeroBasedIndex );


private:
  BOOL m_readyToUnzip;
  BOOL fDragOperCopy;
  HWND hWndMain;
  LPTSTR lpBufName;
  PDIRINFO pCurDirInfo;


  EXTRACTINGMAPITEM *lpfExtractItemMap;
  dfuLong32*  lpfExtractItemIndex;
  dfuLong32 dfNbSelected;
  dfuLong32 dfBufNameStep;
  GUIITEM *pguiItem;
  DFSFILEANDINFO *pDfsFileAndInfo;
  dfuLong32 dfDirExtract;
  dfuLong32 dfNbTotalFile ;
};

class CDropHookExample : public CDropHook
{
public:
  CDropHookExample( CFileSetManagerSmartVersion* pFileSetManager )
    : m_pFileSetManager( pFileSetManager )
  {}

  // CDropHook overrides
  BOOL OkToDrop();
private:
  CFileSetManagerSmartVersion* m_pFileSetManager;
};

/***************************/
CFileSetManagerWima::CFileSetManagerWima( BOOL dragDropOperation,HWND hWndWimaS )
    // for the clipboard, we're always ready to unzip
  : m_readyToUnzip( !dragDropOperation ),
    hWndWima(hWndWimaS),
    lpBufName(NULL),
    fDragOperCopy(FALSE)
{}

CFileSetManagerWima::~CFileSetManagerWima()
{
  ODS( "CFileSetManagerWima dtor" );
  PostMessage(hWndWima,WM_WAITFORREWORK,0,0);
  if (lpBufName!=NULL)
      GlobalFreePtr(lpBufName);


  // in real life, you'll probably want to
  // delete any temporary files at this point.
}

UINT CFileSetManagerWima::GetCount()
{
DWORD dwNum=0;
/* #WIMA#
DWORD i;
LPDIRINFO lpdiParc ;
  //fDragOperCopy=fIsDragCopy;
    fDragOperCopy=FALSE;
  //return 5;
  for (i=0,lpdiParc = lpdi;i<dwEntDirInfo;i++,lpdiParc++)
      if ((lpdiParc->fSel))
        dwNum++;

  if (lpBufName!=NULL)
      GlobalFreePtr(lpBufName);

  lpBufName=(LPSTR)GlobalAllocPtr(GHND,(dwNum+1)*256);
  */
  return dwNum;
}

const TCHAR* CFileSetManagerWima::GetSourcePath( UINT inu )
{
    /*
  switch ( i )
  {
    case 0: return TEXT( "c:\\TestFile1.txt" );
    case 1: return TEXT( "c:\\TestFile2.txt" );
    case 2: return TEXT( "c:\\TestFile3.txt" );
    case 3: return TEXT( "c:\\TestFile4.txt" );
    case 4: return TEXT( "c:\\TestFile5.txt" );
  }   */

DWORD dwNum=0;
/* #WIMA#
DWORD i;
LPDIRINFO lpdiParc ;
//static char szFn[256];
  //LPSTR szFn=(LPSTR)malloc(258);

  //return 5;
  //if (lpBufName==
  //GetTempPath((256),szFn);
  LPSTR szFn = lpBufName + (inu*256);
  if (fDragOperCopy)
      lstrcpy(szFn,szPathExtr);
  else
      GetTempPath((256),szFn);
  for (i=0,lpdiParc = lpdi;i<dwEntDirInfo;i++,lpdiParc++)
      if ((lpdiParc->fSel))
  {

        if (dwNum==inu)
        {
            lstrcat(szFn,lpdiParc->longname);
            #ifdef DEBUG
            OutputDebugString(szFn);
            #endif
            return szFn;
        }
        dwNum++;
  }
  */

  return NULL;
}

/*
void create_test_file(const TCHAR* p )
    {
    HFILEMYEX hf;

    if ((hf = _lcreatMyEx(p, 0)) == HFILE_MYEX_ERROR)
        MessageBox(0, "file create error 2", 0, 0);
    else
        {
        if (_lwriteMyEx(hf, p, lstrlen(p)) == HFILE_ERROR)
            MessageBox(0, "write error", 0, 0);
        if (_lCloseMyEx(hf) == HFILE_ERROR)
            MessageBox(0, "close error", 0, 0);
        }
    }
*/

BOOL CFileSetManagerWima::ReadyToUnzip()
{


  return m_readyToUnzip;
}

void CFileSetManagerWima::Unzip()
{
char szPathTemp[256];
    /*
  MessageBox(0, "about to unzip", 0, 0);

  create_test_file( TEXT( "c:\\TestFile1.txt" ) );
  create_test_file( TEXT( "c:\\TestFile2.txt" ) );
  create_test_file( TEXT( "c:\\TestFile3.txt" ) );
  create_test_file( TEXT( "c:\\TestFile4.txt" ) );
  create_test_file( TEXT( "c:\\TestFile5.txt" ) );
  */
  //DoExtract(hWndWima,hWndLB,FALSE,fAskExtrDrop));
  GetTempPath(sizeof(szPathTemp),szPathTemp);
  /* #WIMA#
  if (fDragOperCopy)
      DoExtract(hWndWima,hWndLB,FALSE,fAskExtrDrop);
  else
      DoExtract(hWndWima,hWndLB,FALSE,FALSE,EXTRCREATE,szPathTemp);
      */
}


/****************************/


CFileSetManagerSmartVersion::CFileSetManagerSmartVersion( BOOL dragDropOperation, GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo )
{
    hWndMain = guiItem.hwndMain;


    //BOOL fRet;


    FILESET* pfsDest=NULL;

    dfuLong32 i,j;
    dfuLong32 dfNbFile=0;
    BOOL fUseListViewSelectInfo=TRUE;


    m_readyToUnzip= !dragDropOperation ;
    fDragOperCopy=FALSE;
    lpfExtractItemMap=NULL;
    lpfExtractItemIndex=NULL;
    dfNbSelected=0;



    pguiItem = &guiItem;
    pDfsFileAndInfo = &DfsFileAndInfo;
    if (DfsFileAndInfo.DfsFile == NULL)
        return /*FALSE */;
    if (((DWORD)DfsFileAndInfo.dfCurDir) == TVITEMPARAM_ROOT)
    {
       if (GetListViewSelectionStatus(guiItem.hwndLV,&dfDirExtract)!=1)
           return /*FALSE*/;
       fUseListViewSelectInfo=FALSE;
    }
    else
        dfDirExtract=DfsFileAndInfo.dfCurDir;


    pCurDirInfo = *(DfsFileAndInfo.pDirInfo+ dfDirExtract);


    lpfExtractItemMap = (EXTRACTINGMAPITEM*)DfsMalloc(sizeof(EXTRACTINGMAPITEM)*(pCurDirInfo ->dfNbFile + 1));
    lpfExtractItemIndex=(dfuLong32*)DfsMalloc(sizeof(dfuLong32)*(pCurDirInfo ->dfNbFile + 1));


    if (fUseListViewSelectInfo)
    {
        for (i=0;i<pCurDirInfo ->dfNbFile;i++)
            *(lpfExtractItemMap+i) = ExtractClassic;

        for (i=0;i<pCurDirInfo ->dfNbFile;i++)
        {
            dfuLong32 iItem = *(guiItem.pdfwListViewSortMap+i);
            *(lpfExtractItemMap+iItem) =
              (ListView_GetItemState(guiItem.hwndLV,i,LVIS_SELECTED) != 0) ? ExtractClassic: ExtractNone;
        }

        for (i=0;i<pCurDirInfo ->dfNbFile;i++)
            if (*(lpfExtractItemMap+i))
                dfNbSelected++;

    }

    if (dfNbSelected==0)
    {
        for (i=0;i<pCurDirInfo ->dfNbFile;i++)
            *(lpfExtractItemMap+i)=ExtractClassic;
        dfNbSelected=pCurDirInfo ->dfNbFile;
    }

    for (i=0,j=0;i<pCurDirInfo ->dfNbFile;i++)
    {
        if (*(lpfExtractItemMap+i))
        {
            *(lpfExtractItemIndex+j)=i;
            j++;
        }
    }

    dfNbTotalFile = pCurDirInfo ->dfNbFile;
    dfBufNameStep = ((MAX_PATH*2)*sizeof(TCHAR))+0x10;
    lpBufName = (LPTSTR)DfsMalloc(dfBufNameStep *dfNbSelected);

}

CFileSetManagerSmartVersion::~CFileSetManagerSmartVersion()
{
    if (lpfExtractItemMap!=NULL)
        DfsFree(lpfExtractItemMap);
    if (lpfExtractItemIndex!=NULL)
        DfsFree(lpfExtractItemIndex);
    if (lpBufName!=NULL)
        DfsFree(lpBufName);
    PostMessage(hWndMain,WM_WAITFORREWORK,0,0);
}


  // called by the drop hook when the user releases the mouse

  // CFileSetManager overrides
UINT CFileSetManagerSmartVersion::GetCount()
{
    return dfNbSelected;
}

BOOL CFileSetManagerSmartVersion::ReadyToUnzip()
{
    return TRUE;
}

void CFileSetManagerSmartVersion::Unzip()
{
//    dfuLong32 i;
    HCURSOR holdcursor;
    TCHAR szDirectory[MAX_PATH+1];/*
    for (i=0;i<dfNbSelected;i++)
    {
        DWORD dwr=0;
        HANDLE hf=CreateFile(GetSourcePath(i),GENERIC_WRITE | GENERIC_READ,0,NULL,CREATE_ALWAYS,0,NULL);
        WriteFile(hf,GetSourcePath(i),lstrlen(GetSourcePath(i)),&dwr,NULL);
        CloseHandle(hf);
    }*/

    GetTempPath((MAX_PATH),szDirectory);
    holdcursor = SetCursor(LoadCursor(NULL,IDC_WAIT));

    DoExtracting(pguiItem->hwndMain,*pguiItem,0,100,
       pDfsFileAndInfo->DfsFile,
       szDirectory,
       NULL,
       NULL, /* parameter : fTempDestExtr */
       dfDirExtract,pDfsFileAndInfo->pDirInfo,
       pDfsFileAndInfo->fBaseDirectorySelected,
       pDfsFileAndInfo->dfBaseDirNum,
       pDfsFileAndInfo->lpBaseDirectory,
       dfNbTotalFile,
       lpfExtractItemMap,
       TRUE,TRUE,NULL);
    SetCursor(holdcursor);

    return ;
}

const char* CFileSetManagerSmartVersion::GetSourcePath( UINT zeroBasedIndex )
{
    TCHAR szTempPath[MAX_PATH+1];
    TCHAR szFn[MAX_PATH+1];
    dfwchar szConvertedFileName[MAX_PATH+1];
    dfwcharp lpConvertedFileNameWithoutPath;
    dfuLong32 i=0;

    //int iln;
    GetTempPath((MAX_PATH),szTempPath);

    ConvertFileNameAndPath(((pCurDirInfo->pFileInDirInfo)+(*(lpfExtractItemIndex+zeroBasedIndex)))->FileName,
                             szConvertedFileName,MAX_PATH,TRUE);

    lpConvertedFileNameWithoutPath = ((dfwcharp)(szConvertedFileName));
    while (szConvertedFileName[i] != 0)
    {
        if ((szConvertedFileName[i]=='\\') || (szConvertedFileName[i]=='/'))
            lpConvertedFileNameWithoutPath = ((dfwcharp)(szConvertedFileName))+i+1;

        i++;
    }

    ConvertUnicodeToTChar(lpConvertedFileNameWithoutPath,szFn,MAX_PATH);

    char*lpBufNameCur = lpBufName+ (dfBufNameStep*zeroBasedIndex);
    lstrcpy(lpBufNameCur,szTempPath);
    //lstrcat(lpBufName,"\\");
    lstrcat(lpBufNameCur,szFn);
    return lpBufNameCur;
}

/*****************************/


BOOL CDropHookExample::OkToDrop()
{
  // the user has released the mouse.
  // set the flag that says it's ok to unzip now.
  m_pFileSetManager->UserReleasedTheMouse();

  return TRUE;
}
/**************************/

LONG DoSrvDropOle(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo)
{
          // allocate a FileSetManager who knows how to render the files
          CFileSetManagerSmartVersion* pFileSetManager = new CFileSetManagerSmartVersion( TRUE ,guiItem,DfsFileAndInfo);

          // allocate a FileContentsDataObject to implement IDataObject,
          // in concert with the FileSetManager.
          IDataObject* pDataObject = new CDataObject( pFileSetManager );

          // allocate a DropHook who will notify the FileSetManager
          // when the user releases the mouse
          CDropHook* pDropHook = new CDropHookExample( pFileSetManager );

          IDropSource* pDropSource = new CDropSource( pDropHook );

          // initiate drag drop
          DWORD dwEffect = 0;
          fIsDragCopy=FALSE;
          HRESULT hrdd=
          DoDragDrop(pDataObject,
                     pDropSource,
                     /*DROPEFFECT_COPY |*/ DROPEFFECT_MOVE, // allow the target to do "copy" or "move"
                                                        // change this to just DROPEFFECT_COPY
                                                        // if you don't want to allow a "move".
                     &dwEffect);

          // Nico: note that if you allow DROPEFFECT_MOVE, the shell will delete
          // the source file for you (but only if the user holds the shift key down)
          // The following code will tell you what happened:
          pFileSetManager ->SetDragOperCopy(dwEffect==DROPEFFECT_COPY);

          #ifdef DEBUG
          switch ( dwEffect )
          {
            case DROPEFFECT_COPY:
              OutputDebugString( TEXT( "Dropped with DROPEFFECT_COPY\r\n" ) );
              break;
            case DROPEFFECT_MOVE:
              OutputDebugString( TEXT( "Dropped with DROPEFFECT_MOVE\r\n" ) );
              break;
            case DROPEFFECT_NONE:
              OutputDebugString( TEXT( "Drop Canceled (DROPEFFECT_NONE)\r\n" ) );
              break;
            default:
              OutputDebugString( TEXT( "Dropped with ???\r\n" ) );
              break;
          }
          #endif

          pDataObject->Release();
          pDropSource->Release();
  {
  MSG msg;
  while (!fCancel && GetMessage(&msg,NULL,0,0/*,PM_REMOVE*/))
    {
    if ((!hDlgCurrent) || (!IsDialogMessage(hDlgCurrent, &msg)))
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if ((msg.message == WM_WAITFORREWORK))
            break;
      }
    }
  }

          //WM_WAITFORREWORK
  return 0;
}
#endif

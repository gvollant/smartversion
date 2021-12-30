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


#include "../../lib/engine/svfile/compress/DfsWrSet.h"
#include "../../lib/engine/svfile/compress/AddingTool.h"

BOOL SVFAPI ClearFileToAdd(FILETOADD* pfta);
//void ClearFileToAddArray(FILETOADDARRAY* pftaArray,BOOL fClearPrevious, BOOL fDeleteOnDisk);
BOOL BuildFileToAddFromRealFile(FILETOADD &fta,LPCTSTR lpszFileName,LPCSTR lpszAddPreviousDir);
BOOL AddFileFtaArray(FILETOADDARRAY* pftaArray,LPCTSTR lpszFileName,LPCSTR lpszAddPreviousDir);
BOOL AddDirFtaArray(FILETOADDARRAY* pftaArray,LPCTSTR lpszFileName,DWORD &dwTotal,BOOL fSubDir,LPCSTR lpszAddPreviousDir);

BOOL DisplayErrorMessage(HWND hWnd,UINT uId,H_ERROR_INFO & hei,BOOL fDisplayMessageIfNoInfo);

BOOL DoOpeningDfs(const GUIITEM &guiItem,TCHAR* pszFn,UINT uiSize);
BOOL DoLoadDfs(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,LRUMENU &lrum,
               LPCTSTR lpszDfs,BOOL fAskBaseDir,LPCTSTR lpPreselectedBaseDir,
               dfuLong32 dfSelectVersion=VERSION_NUMBER_DEFAULT);
LRESULT DoNotifyTreeView(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,NMHDR*);
LRESULT DoNotifyToolTip(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,NMHDR*);
LRESULT DoNotifyListView(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,NMHDR*);
LRESULT DoNotifyHeaderOfListView(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,NMHDR*);

BOOL DoDeleteVersion(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,LRUMENU &lrum);
BOOL DoGenerateSubDfsUserSelected(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,LRUMENU &lrum);
BOOL DoAppendSvfToSvf(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,LRUMENU &lrum);
BOOL DoGenerateZip(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo);

DWORD GetListViewSelectionStatus(HWND hWndLV,dfuLong32* pdwPosFirstSelected);



BOOL DoExtracting(HWND hwndMain,GUIITEM &guiItem,DWORD dwMinProgress,DWORD dwMaxProgress,
                  DFSFILE DfsFile,PTSTR pszBaseDirExtract,
                  FILESET** ppfsDest,BOOL fTempDestExtr,dfuLong32 dfDirExtr,PDIRINFO* pDirInfo,
                  BOOL fBaseDirectorySelected,dfuLong32 dfBaseDirNum,LPCTSTR lpBaseDirectory,
                  DWORD dwMapFileNumber,EXTRACTINGMAPITEM* lpExtractingMap,BOOL fOverwriteExtracting,
                  BOOL fFlatExtracting, H_ERROR_INFO * pei);

BOOL DoAbout(const GUIITEM &guiItem);
BOOL DoSplash(const GUIITEM &guiItem,BOOL fRegistered,DWORD dwCurDayUse,DWORD dwTotalDayUseAllowed=30);
BOOL DoHelpContents(HWND hWnd,LPTSTR lpszAddUrl=NULL);
BOOL DoVisitWebSite(const GUIITEM &guiItem,UINT uiResUrl);
BOOL DoCloseDfs(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo);
BOOL DoExtract(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,BOOL fZipFileToBuild);

BOOL DoFileProperties(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,LRUMENU &lrum);
BOOL DoFileDelete(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,LRUMENU &lrum);
BOOL DoInsertFileInVersion(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,LRUMENU &lrum);

BOOL DoVersionProperties(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,BOOL fSpecifyDir=FALSE,dfuLong32 dfDirNum=0);
BOOL DoSpecifyBaseDirVersion(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo);
BOOL DoNewDfs(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,LRUMENU &lrum,FILETOADD* pFileToAdd,dfuLong32 dfNbToInsert);
BOOL DoNewDfsZipFile(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,LRUMENU &lrum,LPTSTR lpszZipFile);
BOOL DoInsertVersion(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,LRUMENU &lrum,FILETOADD* pFileToAdd,dfuLong32 dfNbToInsert);
BOOL DoInsertVersionZipFile(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,LRUMENU &lrum,LPTSTR lpszZipFile);
BOOL DoApplicationSettings(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,LRUMENU &lrum);
BOOL DoApplicationRegister(REGCODE& RegCode,GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo);
BOOL DoAskAndSetDfsBaseDirectory(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,LPCTSTR lpActualCurDir,BOOL &fCancel);
BOOL TrySetDfsBaseDirectory(LPCSTR lpszDirectory,GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,dfuLong32 dfNumDir,BOOL fTryOtherDir,BOOL fVerboseAsk,BOOL &fCancel);

BOOL RefreshGrayingMenu(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo);

/*******************************/

#define LANGUI_SELECT_AUTO      ((DWORD)(0xffffffff))
WORD GetNbLang();
BOOL GetLangName(LPSTR lpLN,WORD i);
LPCTSTR GetLangInisLang(DWORD i);
WORD GetCurLang();
BOOL TryLoadLang(HWND hWndMain,DWORD wIndexLang,BOOL fReplace);
DWORD GetBetterAutoLang();


typedef struct
{
    UINT uiId;
    DWORD dwRatioShiftX;
    DWORD dwRatioShiftY;
    DWORD dwRatioShiftCX;
    DWORD dwRatioShiftCY;
    BOOL fInvalidateRect;
} ITEMCTLINFO;


typedef struct
{
    ITEMCTLINFO ItemCtlInfo;
    //RECT rcInitial;
    POINT ptInitial;
    SIZE sizeInitial;
} ITEMCTLINTERNAL;

#define NBITEM_UNSPECIFIED ((DWORD)0xffffffffUL)
class RESIZABLEDLGHELP
{
public:
    RESIZABLEDLGHELP();
    ~RESIZABLEDLGHELP();
    BOOL Init(HWND hParent);
    BOOL InitRatio(DWORD dwRatioShiftSet);
    DWORD GetRatio() { return dwRatioShift; };
    BOOL OnResize();

    BOOL InitCtlList(const ITEMCTLINFO* pItemCtlInit,DWORD dwNbItem=NBITEM_UNSPECIFIED);
    BOOL MoveAndResizeDlgItem();
    BOOL GetOriginalDialogSize(SIZE * ptSize);
private:
    BOOL RESIZABLEDLGHELP::InitGrip();
    HWND m_hParent;
    SIZE m_szInitialDlg;
    SIZE m_sizeGrip; // holds grip size
    HWND m_wndGrip; // grip control
    void ShowSizeGrip(BOOL bShow);
    void UpdateGripPos();

    ITEMCTLINTERNAL *pItemCtlInternal;
    DWORD dwNbCtlInternal;
    DWORD dwNbCtlInternalAllocated;
    DWORD dwNbCtlInternalStep;
    BOOL CheckAllocateNbCtlItem(DWORD dwNbNeeded);
    DWORD dwRatioShift;
    SIZE m_sizeOriginalDialog;
} ;

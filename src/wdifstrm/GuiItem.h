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


#define TVITEMPARAM_ROOT        ((DWORD)(0xffffffff))

int WinLongToStr(long lNum, LPSTR lpszStr, int cbStr);
int WinLongLongToStr(DWORD dwNumLow, long lNumHigh, LPSTR lpszStr, int cbStr);
int WinLong64ToStr(dfuLong64 dfl64Num, LPSTR lpszStr, int cbStr);

BOOL BuildStrDate(const DFSTM* pdfsTm,LPTSTR lpszString,UINT uiSize);
UINT GetUiResDescTypeDir(dfuLong32 dfTypeDir);



#define VERSION_NUMBER_DEFAULT  ((DWORD)(0xffffffff))


class GUIITEM
{
public:
  HWND hwndTreeView;
  HWND hwndLV;
  HWND hwndTB;
  HWND hwndSB;
  HWND hwndToolTip;

  HWND hwndProgress;

  HMENU hPopupMenu;
  DWORD dwLastPosProgress;
  DWORD dfListViewNbItem;

  DWORD dwLangUISelect;

  LPDWORD pdfwListViewSortMap ;
  BOOL fColumnInitialised;
  BOOL fCurrentColumnDirList;
  COMPRESSIONPARAM compressionParam;
  BOOL fOverwriteExtracting;


  BOOL fSelectTempMemSize;
  DWORD dwTempMemSize;
  BOOL fSelectTempPath;
  TCHAR szTempPath[MAX_PATH+1];
  BOOL  pfStripIdentical;
  BOOL  fSha1,fSha256,fMd5;

public:
  HWND hwndMain;
  DWORD cxSplitter;
  RECT rcSplit;
  SIZE sizeTB;
  HDC hdcSplit;
  UINT uiStyleLv;
  long lColumnSort;
  BOOL fInvert;
  EXTINFOCACHE ExtInfoCache;

  TCHAR szDefaultDirExtract[MAX_PATH*2];
  TCHAR szDefaultDirAddVersion[MAX_PATH*2];
  TCHAR szDefaultDirPreviousVersion[MAX_PATH*2];
  int iColSizeFileList[NBLVCOLUMNFILELIST];
  int iColSizeDirList[NBLVCOLUMNDIRLIST];

private:
  BOOL fDirty;
  TCHAR szFileName[MAX_PATH*2];
  BOOL fFileNameExist;
  DWORD dwMaxProgress;
  BOOL fRegistered;
  BOOL fListViewFocused;
public:
  GUIITEM();
  ~GUIITEM();
  long GetColumnSort() const { return lColumnSort; }  ;
  BOOL GetColumnSortInvert() const { return fInvert; }  ;
  BOOL SetColumnSort(long lNewCol,BOOL fInvertSet) { fInvert=fInvertSet; lColumnSort = lNewCol; return TRUE; } ;
  BOOL EraseListView() ;
  BOOL InitListView(HWND hwndLV) ;
  BOOL InitListViewColumn(HWND hwndLV,BOOL fDirList,BOOL fMainAppListView=TRUE);
  BOOL DoSaveColumnWidth();
  BOOL FillListView(DFSFILEANDINFO &DfsFileAndInfo,BOOL fAlwaysFullRebuild=FALSE);
  BOOL SetListViewStyle(UINT uiStyle);
  BOOL GetListViewStyle(UINT &uiStyle) const ;
  BOOL FillTreeView(DFSFILEANDINFO &DfsFileAndInfo,dfuLong32 dfSelectVersion=VERSION_NUMBER_DEFAULT);
  BOOL UpdateTreeViewItem(DFSFILEANDINFO &DfsFileAndInfo,dfuLong32 dfItemParam);

  BOOL DoSortColumn(DFSFILEANDINFO &DfsFileAndInfo,int iColumn,BOOL fInvert);

  BOOL InstallProgressBar(DWORD dwMaxSet);
  BOOL SetProgressPos(DWORD dwSet);
  BOOL RemoveProgressBar();

  void UpdateStatusBar(HWND hWndStatusbar,LPSTR lpszStatusString, WORD partNumber, WORD displayFlags);
  BOOL RefreshNormalStatusBar(DFSFILEANDINFO &DfsFileAndInfo);


  HWND GetHwndMain() const { return hwndMain; } ;
  BOOL GetDirtyFlag() const { return fDirty; } ;

  BOOL SetDirtyFlag(BOOL fFlag);
   /*  NULL : no file, strlen() = 0 : unnamed */
  BOOL SetszFileName(LPCTSTR lpszFileName,BOOL fChangeDirty=FALSE,BOOL fNewFlag=FALSE);
  LPCTSTR GetszFileName() { return szFileName; } ;

  BOOL RemovePopupMenu();
  BOOL DoPopupMenu(const POINT* pt,BOOL fTreeView,BOOL fListViewDir);
  void SetRegisteredMode(BOOL fRegisteredSet)
                 { fRegistered = fRegisteredSet ; } ;
  BOOL DisplayNewTitleBar();

  void SetListViewFocused(BOOL fListViewFocusedSet)
                 { fListViewFocused = fListViewFocusedSet ; };

  BOOL GetIsListViewFocused()
                 { return fListViewFocused ; };

private:
  void TreeViewFillItem(DFSFILEANDINFO &DfsFileAndInfo,TVINSERTSTRUCT& tv,dfuLong32 dfNumDir,
                        LPTSTR lpszTxtBuf, int sizeTxtBuf);
} ;

void DoGetDispInfoListViewDir(GUIITEM &guiItem,DFSFILEANDINFO &DfsFileAndInfo,NMHDR* pnmlv);
BOOL InitListViewImageLists(HWND hwndLV);
int CompareTypeOfFileName(EXTINFOCACHE* pExtInfoCache,dfwcharpc dfFileName1,dfwcharpc dfFileName2);
BOOL ShowSortColumnTitleBitmap(HWND hWndLV,int iColumn,BOOL fEraseOtherSort,BOOL fSortUp);

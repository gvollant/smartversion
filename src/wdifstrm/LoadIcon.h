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


// newintf.h
#ifndef WIN32
#ifndef LPCVOID
#define LPCVOID LPVOID
#endif
#endif

int GetItemIndexImage(LPSTR lpExt,BOOL fOnlyExt);
LPSTR GetExtensionFromName(LPSTR lpLine,BOOL fOnlyExt,LPSTR lpBufCop);
BOOL GetExtensionFromExtInRegistry(LPSTR lpExt,LPSTR lpDescExt);

/*
BOOL RegisterExt(LPSTR lpExt,BOOL fOnlyExt,BOOL &fRegisteredType,LPSTR lpTypeName=NULL);

BOOL DrawExtIcon(HDC hDC,LPSTR lpExt,BOOL fOnlyExt,int x,int y,BOOL fRep=FALSE,BOOL fLarge=FALSE);



BOOL IsListViewPossible();
BOOL IsMutlipleTypeIconAvaiable();



BOOL IsPropertySheetPossible();
int DoPropertySheet(LPVOID lp);
void doSHAddToRecentDocs(UINT uFlags,LPCVOID pv);
BOOL GetDirectoryExplorer(HWND hWnd,LPSTR lpDir,DWORD dwFlags,BOOL fAllowCreate,LPCSTR lpszTitle);

*/
////////////////////////////////////////////////////////////////////////////

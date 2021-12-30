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

#ifndef MISCUTL
#define MISCUTL

#ifndef Round4
#define Round4(x) ((((x)+3)/4)*4)
#endif

#if (!defined(LONG_PTR)) && (!defined(DWLP_USER))
#define LONG_PTR LONG
#define DWORD_PTR DWORD
#define DWLP_USER DWL_USER
#define GWLP_WNDPROC GWL_WNDPROC
#define DWLP_MSGRESULT DWL_MSGRESULT
#define GCLP_HCURSOR GCL_HCURSOR
#define INT_PTR LONG
typedef unsigned int DLGRETTYPE ;
#define DLGRETTYPE_DEFINED
#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER ((DWORD)0xFFFFFFFFL)
#endif
#ifndef LPARAM
#define LPARAM LONG
#endif


#define GetWindowLongPtr(a,b) GetWindowLong((a),(b))
#define SetWindowLongPtr(a,b,c) SetWindowLong((a),(b),(c))
#define SetClassLongPtr(a,b,c) SetClassLong((a),(b),(c))
#endif

#if !defined(DLGRETTYPE_DEFINED)
#define DLGRETTYPE INT_PTR
#endif

#define MakeuInt64(dwLow,dwHigh) (((DWORD)(dwLow)) | (((unsigned __int64)(((DWORD)dwHigh)))<<32))
#define GetLowDwordOfUint64(c)  ((DWORD)((c) & ((DWORD)0xffffffffUL)))
#define GetHighDwordOfUint64(c) ((DWORD)((c)>>32))


void  InitializeCRCTable(void);
DWORD ComputeBufferCRC(DWORD dwCRC, LPCVOID pvBuffer, DWORD cbBuffer);

BOOL TryFreePtr(LPVOID lp);
BOOL GetFloppyFromLine(LPSTR lpLine,BYTE &bNewFl);
LPCSTR CopyStrWord(LPCSTR lpSrc,LPSTR lpDest,DWORD dwMaxSize=0);
LPVOID ReallocOrAlloc(LPVOID lpOldPtr,DWORD dwNewSize);

BOOL CalcMulTo64(DWORD dwA,DWORD dwB,DWORD &dwLow,DWORD &dwHigh);
BOOL AddValueTo64(DWORD &dwLow,DWORD &dwHigh,DWORD dwAddLow=0,DWORD dwAddHigh=0);
BOOL Multiply64(DWORD &dwLow,DWORD &dwHigh,DWORD dwLow2,DWORD dwHigh2=0);
BOOL Multiply64LH(DWORD &dwLow,LONG &dwHigh,DWORD dwLow2,LONG dwHigh2=0);
DWORD GetKBSizeFrom64Size(DWORD dwLow,DWORD dwHigh);
int  CompareValue64(DWORD dwLow1,DWORD dwHigh1,DWORD dwLow2,DWORD dwHigh2);
BOOL SubValueFrom64(DWORD &dwLow,DWORD &dwHigh,DWORD dwSubLow=0,DWORD dwSubHigh=0);
DWORD CalculateRatio32(DWORD dfValue, DWORD dfTotal, DWORD dfWidth);
DWORD GivePctDivide(DWORD dwDone, DWORD dwAll);
DWORD GivePctDivide64(DWORD dwDoneLow,DWORD dwDoneHigh,DWORD dwAllLow,DWORD dwAllHigh);

void GetCurDateTime(WORD &wDosDate,WORD &wDosTime);
BOOL TimeBetweenFileTime(FILETIME ft1,FILETIME ft2,LPDWORD lpdwDay=NULL,LPDWORD lpdwSec=NULL);
LPTSTR Date_DOS2Text(WORD wDosDate,LPTSTR lpBuf,BOOL fLongYear);
LPTSTR Time_DOS2Text(WORD wDosTime,LPTSTR lpBuf);

LPTSTR GetLatestChar(LPTSTR lpText);
void GetDirectoryOfFileName(LPTSTR lpDest,LPTSTR lpFn);
LPTSTR AddNameInPathN(LPTSTR lpDest,DWORD dwLenDest,LPTSTR lpName,LPCTSTR lpOrgRep=NULL);
BOOL IsFileNameHasExtension(LPCTSTR lpFn) ;

BOOL MyGetDiskFreeSpaceEx(
  LPCTSTR lpDirectoryName,  // pointer to directory name on disk of
                            // interest
  PULARGE_INTEGER lpFreeBytesAvailableToCaller,
                            // pointer to variable to receive free
                            // bytes on disk available to the caller
  PULARGE_INTEGER lpTotalNumberOfBytes,
                            // pointer to variable to receive number
                            // of bytes on disk
  PULARGE_INTEGER lpTotalNumberOfFreeBytes);


BOOL CheckIfEmpty(BYTE bFloppy);

BOOL CheckExistFile(LPCTSTR lpFn);
BOOL PrepareForInc(LPTSTR lpFn);
BOOL IncNumberInName(LPSTR lpFn,BOOL fCheckExist=FALSE,
                     BOOL fNeedAbsolute=FALSE,DWORD dwStepIncr=1);

BOOL CreatePathIfNeeded(LPCTSTR lpPath);


void CenterChild(HWND hWnd);
void CenterWindow(HWND hwnd);
BOOL ResizeStaticCtl(HWND hCtl,BOOL fAlignRight=FALSE,BOOL fAlignBottom=FALSE);
BOOL ResizeStaticCtlForMaxUiString(HWND hCtl,LPUINT lpuiString,
                                   BOOL fAlignRight=FALSE,BOOL fAlignBottom=FALSE);

BOOL GetWindowPos(HWND hWnd, long* lpdwX, long* lpdwY, long* lpdwCx=NULL,long* lpdwCY=NULL);

BOOL HideDialogInInit(HWND hDlg);
BOOL IsParentOrGrandParent(HWND hWndWin,HWND hWndCanPar);

void CnvMaj(LPTSTR lpTxt);
void CnvMin(LPTSTR lpTxt);
void RemoveSpace(LPTSTR lpPtr);

BOOL OlderTime(DWORD & dwTime);
BOOL CheckExtension(LPCTSTR lpFn,LPSTR lpExt);

LPCTSTR GetNameWithoutPath(LPCTSTR lpFn);
BOOL ChangeFileNameExtension(LPTSTR lpFn,LPCTSTR lpExt);
HANDLE MyCreateFileWithModifiedExtension(LPCTSTR lpFn,LPCTSTR lpExt,
                                         DWORD dwDesiredAccess,
                                         DWORD dwShareMode,
                                         LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                         DWORD dwCreationDisposition,
                                         DWORD dwFlagsAndAttributes,
                                         HANDLE hTemplateFile);

#define FILE_POS_UNKNOWN ((DWORD)0xffffffff)
#define ALIGN_ISOFILEWHENWRITE ((DWORD)4*2048)
BOOL  FillAlignFile(HANDLE hFile,DWORD dwAlign,DWORD dwPos,LPDWORD lpdwWritten=NULL);

#ifdef WIN32
DWORD MyGetTempPath(DWORD nBufferLength,
                    LPTSTR lpBuffer);
#endif


#define AroundLower(dwValue,dwModulo) ((((DWORD)(dwValue)) / ((DWORD)(dwModulo))) * (dwModulo))
#define AroundUpper(dwValue,dwModulo) (((((DWORD)(dwValue)) + ((DWORD)(dwModulo)) -1) / ((DWORD)(dwModulo))) * (dwModulo))



/**************************************************************************/


typedef struct tagMYOFNA {
   DWORD        lStructSize;
   HWND         hwndOwner;
   HINSTANCE    hInstance;
   LPCSTR       lpstrFilter;
   LPSTR        lpstrCustomFilter;
   DWORD        nMaxCustFilter;
   DWORD        nFilterIndex;
   LPSTR        lpstrFile;
   DWORD        nMaxFile;
   LPSTR        lpstrFileTitle;
   DWORD        nMaxFileTitle;
   LPCSTR       lpstrInitialDir;
   LPCSTR       lpstrTitle;
   DWORD        Flags;
   WORD         nFileOffset;
   WORD         nFileExtension;
   LPCSTR       lpstrDefExt;
   LPARAM       lCustData;
   LPOFNHOOKPROC lpfnHook;
   LPCSTR       lpTemplateName;


    void *        pvReserved;
    DWORD         dwReserved;
    DWORD         FlagsEx;

} MYOPENFILENAMEA, *LPMYOPENFILENAMEA;

typedef struct tagMYOFNW {
   DWORD        lStructSize;
   HWND         hwndOwner;
   HINSTANCE    hInstance;
   LPCWSTR      lpstrFilter;
   LPWSTR       lpstrCustomFilter;
   DWORD        nMaxCustFilter;
   DWORD        nFilterIndex;
   LPWSTR       lpstrFile;
   DWORD        nMaxFile;
   LPWSTR       lpstrFileTitle;
   DWORD        nMaxFileTitle;
   LPCWSTR      lpstrInitialDir;
   LPCWSTR      lpstrTitle;
   DWORD        Flags;
   WORD         nFileOffset;
   WORD         nFileExtension;
   LPCWSTR      lpstrDefExt;
   LPARAM       lCustData;
   LPOFNHOOKPROC lpfnHook;
   LPCWSTR      lpTemplateName;

    void *        pvReserved;
    DWORD         dwReserved;
    DWORD         FlagsEx;


} MYOPENFILENAMEW, *LPMYOPENFILENAMEW;

#ifdef UNICODE
typedef MYOPENFILENAMEW MYOPENFILENAME;
#else
typedef MYOPENFILENAMEA MYOPENFILENAME;
#endif

MYOPENFILENAME* AllocMyOpenFileName();
void FreeMyOpenFileName(MYOPENFILENAME*);
BOOL GetMyOpenFileName(MYOPENFILENAME*);
BOOL GetMySaveFileName(MYOPENFILENAME*);
void InitOpenFileName(MYOPENFILENAME * pofn,HWND hWnd,
                    WORD wIdFilter,LPTSTR pFilter,int szFilter,
                    LPTSTR lpstrFile,DWORD dwSizeFile,
                    LPTSTR lpCapt=NULL,WORD wIdCapt=0,int szCapt=0);

/**************************************************************************/

void SetMUResourceBase(HINSTANCE hInstLangResNew,WORD wIndexStringResNew=0);
HINSTANCE GetMULingResHinst();
void  SetMUInternTemplate(LPTSTR);
LPTSTR GetMUInternTemplate(LPTSTR);


int LoadInternatString(UINT uId,LPTSTR lpBuf,int cch);

typedef DWORD (FAR * LPBEFOREMSG) ();
typedef void  (FAR * LPAFTERMSG) (DWORD);
void SetBeforeAfterMsgFunc(LPBEFOREMSG lpBeforeSet=NULL,
                           LPAFTERMSG lpAfterSet=NULL);



// #define ADD_SOURCE_INFO_BOX



int MessageBoxWithSourceLine(HWND hWnd,LPCTSTR lpText,LPCTSTR lpCaption,UINT uType,LPCSTR lpszSourceFileName,DWORD);

int MessageLoadStringWithSourceLine(HWND hWnd,UINT uiRes,UINT uiTitle,UINT uiSty,LPCSTR lpszSourceFileName,DWORD);
int MessageLoadStringTxtParamWithSourceLine(HWND hWnd,UINT uiRes,UINT uiTitle,UINT uiSty,LPCTSTR lpParam,LPCSTR lpszSourceFileName,DWORD);
int MessageLoadStringWin32ErrorParamWithSourceLine(HWND hWnd,UINT uiRes,UINT uiTitle,UINT uiSty,DWORD dwErr,LPCSTR lpszSourceFileName,DWORD);
int MessageLoadStringWin32ErrorAtNextLineWithSourceLine(HWND hWnd,UINT uiRes,UINT uiTitle,UINT uiSty,DWORD dwErr,LPCSTR lpszSourceFileName,DWORD);

int MessageBoxWin32ErrorWithSourceLine(HWND hWnd,LPCTSTR lpszCaption,DWORD dwErr,UINT dwFlag,LPCTSTR lpszFmt,LPCSTR lpszSourceFileName,DWORD);
int MessageBoxWithABFuncWithSourceLine(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType,LPCSTR lpszSourceFileName,DWORD );


#ifdef ADD_SOURCE_INFO_BOX

#define MessageBoxFullInfo(a,b,c,d) (MessageBoxWithSourceLine((a),(b),(c),(d),__FILE__,__LINE__))

#define MessageLoadString(a,b,c,d) (MessageLoadStringWithSourceLine((a),(b),(c),(d),__FILE__,__LINE__))
#define MessageLoadStringTxtParam(a,b,c,d,e) (MessageLoadStringTxtParamWithSourceLine((a),(b),(c),(d),(e),__FILE__,__LINE__))
#define MessageLoadStringWin32ErrorParam(a,b,c,d,e) (MessageLoadStringWin32ErrorParamWithSourceLine((a),(b),(c),(d),(e),__FILE__,__LINE__))
#define MessageLoadStringWin32ErrorAtNextLine(a,b,c,d,e) (MessageLoadStringWin32ErrorAtNextLineWithSourceLine((a),(b),(c),(d),(e),__FILE__,__LINE__))

#define MessageBoxWin32Error(a,b,c,d,e) (MessageBoxWin32ErrorWithSourceLine((a),(b),(c),(d),(e),__FILE__,__LINE__))
#define MessageBoxWithABFunc(a,b,c,d) (MessageBoxWithABFuncWithSourceLine((a),(b),(c),(d),__FILE__,__LINE__))


#else
#define MessageBoxFullInfo(a,b,c,d) (MessageBoxWithSourceLine((a),(b),(c),(d),NULL,0))

#define MessageLoadString(a,b,c,d) (MessageLoadStringWithSourceLine((a),(b),(c),(d),NULL,0))
#define MessageLoadStringTxtParam(a,b,c,d,e) (MessageLoadStringTxtParamWithSourceLine((a),(b),(c),(d),(e),NULL,0))
#define MessageLoadStringWin32ErrorParam(a,b,c,d,e) (MessageLoadStringWin32ErrorParamWithSourceLine((a),(b),(c),(d),(e),NULL,0))
#define MessageLoadStringWin32ErrorAtNextLine(a,b,c,d,e) (MessageLoadStringWin32ErrorAtNextLineWithSourceLine((a),(b),(c),(d),(e),NULL,0))

#define MessageBoxWin32Error(a,b,c,d,e) (MessageBoxWin32ErrorWithSourceLine((a),(b),(c),(d),(e),NULL,0))
#define MessageBoxWithABFunc(a,b,c,d) (MessageBoxWithABFuncWithSourceLine((a),(b),(c),(d),NULL,0))

#endif

INT_PTR DialogBoxParWithABFunc(HINSTANCE hInstance,LPCTSTR lpTemplateName,
                          HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam=0);
HWND CreateDialogParWithABFunc(HINSTANCE hInstance,LPCTSTR lpTemplateName,
                          HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam=0);



void SetWindowLoadString(HWND hWnd,UINT uiRes);
void SetDlgItemTextLoadString(HWND hWnd,int idctl,UINT uiRes);

BOOL TextOutLoadString(HDC hdc,int nXStart, int nYStart, UINT uiMsg);
int DrawTextLoadString(HDC hdc,UINT uiMsg,LPRECT lprc,UINT wFormat);

void TrackFileName(LPCTSTR lpOrigFn,LPTSTR lpDestFn,int iMaxSize);

BOOL FindLine(LPCTSTR lpBuf,LPCTSTR lpSearch,DWORD dwSizeBuf,DWORD &dwPosFind);




#ifdef WIN32
typedef enum { WINNT,WIN95ORGREATHER,WIN32S } WIN32KIND;
WIN32KIND GetWin32Kind();
WORD GetVersionReordered();

BOOL IsNT4ForConnectFAT32();
#define PACKVERSION(major,minor) MAKELONG(minor,major)
DWORD GetDllVersion(LPCTSTR lpszDllName);


void DeleteExecutableBF(void);
DWORD RegDeleteKeyNT(HKEY hStartKey , LPCTSTR pKeyName);

BOOL IsUnicodeSupported();
HWND MyCreateDialog(HINSTANCE,LPCTSTR,HWND,DLGPROC);
HWND MyCreateDialogParam(HINSTANCE,LPCTSTR,HWND,DLGPROC,LPARAM);
INT_PTR MyDialogBox(HINSTANCE,LPCTSTR,HWND,DLGPROC);
INT_PTR MyDialogBoxParam(HINSTANCE,LPCTSTR,HWND,DLGPROC,LPARAM);
int MyGetMenuString(HMENU hMenu,UINT uIDItem,LPTSTR lpString,int nMaxCount,UINT uFlag);
BOOL MyMultiByteToWideChar(LPCSTR, LPWSTR, int);
BOOL MyWideCharToMultiByte(LPCWSTR, LPSTR, int);
BOOL MyInsertMenu(HMENU hMenu,UINT uPosition,UINT uFlags,UINT_PTR uIDNewItem,LPCTSTR lpNewItem);

LONG_PTR MyGetWindowLongPtr(HWND hWnd,int nIndex) ;
LONG_PTR MySetWindowLongPtr(HWND hWnd,int nIndex, LONG_PTR dwNewLong) ;

#endif
BOOL IsWin95OrGreatherLook();
BOOL IsWin98orW2KOrGreatherLook();
BOOL IsWhistler();
BOOL IsVistaOrGreather();
BOOL SetWhistlerThemeOk();


typedef enum
{ NO_UAC_TOKEN,
  UAC_TOKEN_NOADMINUSER,
  UAC_TOKEN_ADMINNOTELEVATED,
  UAC_TOKEN_ADMINELEVATED
} UAC_SITUATION;

UAC_SITUATION GetUacSituation();

BOOL MyMkdir(LPCTSTR dirname);




BOOL UncompressGZMem(const BYTE* lpRes,DWORD dwSizeRes,LPSTR & lpBuf,DWORD & dwSizeExe);
BOOL UncompressGZRes(HMODULE hModule, LPCTSTR lpName, LPCTSTR lpType, LPSTR & lpBuf,DWORD & dwSizeExe);
BOOL FreeGzDecompressedMem(LPSTR lpBuf);

void TransparentBlt(HDC hDC, UINT x, UINT y, HBITMAP hBmp, COLORREF cr,UINT xs,UINT ys,UINT cx,UINT cy=0,BOOL fDoBlack=FALSE);
BOOL PaintBitmap(HDC hdc,int xd,int yd,int cx,int cy,HBITMAP hbm,int xs,int ys);
BOOL GetPremultipliedBitmap(HBITMAP hbmp,COLORREF clrBkgnd);

typedef enum
{
  ALPHA_NOALPHAPAINT = 0,
  ALPHA_MONOCOLOR,
  ALPHA_VERTICAL,
  ALPHA_HORIZONTAL
} ALPHA_STRATEGY;

typedef struct
{
  ALPHA_STRATEGY AlphaStrategy;
  DWORD dwNbClrBkGnd;
  const COLORREF* lpClrBkGnd;
  COLORREF clrMonoColor;
} TRANSPARENCY_PAINT;

HBITMAP LoadBitmapRes(IN HINSTANCE hInstance,IN LPCTSTR lpBitmapName,LPCTSTR lpType=RT_BITMAP,
                      HDC hDc=NULL,
                      ALPHA_STRATEGY AlphaStrategy=ALPHA_NOALPHAPAINT,
                      DWORD dwNbClrBkGnd=0,const COLORREF* lpClrBkGnd=NULL,
                      BOOL fMagicColorToReplace=FALSE,COLORREF clrToReplace=0);

HBITMAP LoadBitmapResAuto(IN HINSTANCE hInstance,IN LPCTSTR lpBitmapName,
                      HDC hDc=NULL,
                      ALPHA_STRATEGY AlphaStrategy=ALPHA_NOALPHAPAINT,
                      DWORD dwNbClrBkGnd=0,const COLORREF* lpClrBkGnd=NULL,
                      BOOL fMagicColorToReplace=FALSE,COLORREF clrToReplace=0);

HBITMAP LoadBitmapResMutltiTrans(IN HINSTANCE hInstance,IN LPCTSTR lpBitmapName,LPCTSTR lpType=RT_BITMAP,
                                 HDC hDc=NULL,
                                 DWORD dwNbTransparencyPaint=0,const TRANSPARENCY_PAINT* pTransPaint=NULL,
                                 BOOL fMagicColorToReplace=FALSE,COLORREF clrToReplace=0);

HBITMAP LoadBitmapResMutltiTransAuto(IN HINSTANCE hInstance,IN LPCTSTR lpBitmapName,
                                     HDC hDc=NULL,
                                     DWORD dwNbTransparencyPaint=0,const TRANSPARENCY_PAINT* pTransPaint=NULL,
                                     BOOL fMagicColorToReplace=FALSE,COLORREF clrToReplace=0);


BOOL GetWin32ErrorMessage(DWORD dwErr, LPTSTR lpBuffer, DWORD nSize,BOOL fAppend=FALSE);

#endif




#ifndef mymemsetdefined
#define mymemsetdefined
#if defined(_DEBUG) && defined(WIN32)
#define mymemset(a,b,c) \
    { if (IsBadWritePtr(a,c)) { char sz[256]; \
        wsprintf(sz,"error is writting %lx:%lx in line %u of file '%s' \n",a,c,__LINE__,__FILE__); \
        MessageBoxFullInfo(0,sz,NULL,MB_OK); \
        OutputDebugString(sz);} \
else (memset(a,b,c)); }
#else
#define mymemset(a,b,c) (memset(a,b,c))
#endif
#endif

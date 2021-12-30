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

// --------------------------------------------------------------------------
//  colorizer application. copyright Manutius.com, 2001. all rights reserved
// --------------------------------------------------------------------------
//  ml_hyperlink.cpp: implementation of the ml_hyperlink class.
// --------------------------------------------------------------------------

#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <commdlg.h>
#include <tchar.h>
#include "miscutil.h"
// --------------------------------------------------------------------------
//  ml_Hyperlink implementation
// --------------------------------------------------------------------------
#include "ml_hyperlink.h"

BOOL ml_Hyperlink::mRegistered = Register();

#define IDC_HAND_W2K            MAKEINTRESOURCE(32649)

HCURSOR GetHandCursor()
{
    BYTE bHighVer = LOBYTE(LOWORD(GetVersion()));

    if (bHighVer>=5)
        return ::LoadCursor(NULL,IDC_HAND_W2K);
    else
        return ::LoadCursor(GetModuleHandle(NULL), _T("IDC_HAND"));
}

// --------------------------------------------------------------------------

BOOL ml_Hyperlink::Register()
{
  WNDCLASSEX wc;

  // register the window class of the control
  wc.cbSize         = sizeof(WNDCLASSEX);
  wc.style          = CS_BYTEALIGNCLIENT | CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS;
  wc.lpfnWndProc    = WindowProc;
  wc.cbClsExtra     = 0;
  wc.cbWndExtra     = sizeof(ml_Hyperlink *);
  wc.hInstance      = NULL;
  wc.hIcon          = NULL;
  //wc.hCursor        = ::LoadCursor(ghinst, MAKEINTRESOURCE(IDC_HAND));
  wc.hCursor        = GetHandCursor();//::LoadCursor(GetModuleHandle(NULL), "IDC_HAND");
  wc.hbrBackground  = ::GetSysColorBrush(COLOR_3DFACE);
  wc.lpszMenuName   = NULL;
  wc.lpszClassName  = (_T("mHyperlink"));
  wc.hIconSm        = NULL;

  return ::RegisterClassEx(&wc);
}

// --------------------------------------------------------------------------
ml_Hyperlink* ml_Hyperlink::GetHyperLinkClass(HWND hwnd)
{
    ml_Hyperlink* pmlh = (ml_Hyperlink *)(LONG_PTR)(MyGetWindowLongPtr(hwnd, 0));
    return pmlh;
}

void ml_Hyperlink::SetText(LPCTSTR lpText)
{
	TCHAR *newText = new TCHAR[1 + lstrlen(lpText)];
	lstrcpy(newText, lpText);

	LPTSTR lpFoundSep = (_tcschr(newText, '$'));
	if (lpFoundSep == NULL)
	{
		SetAddress(newText);
		SetCaption(newText);
	}
	else
	{
		*lpFoundSep = 0;

		SetAddress(lpFoundSep+1);
		SetCaption(newText);
	}
	delete[] newText;
}

LRESULT CALLBACK ml_Hyperlink::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch(message)
  {
    case WM_CREATE:
      {
        ml_Hyperlink* pmlh = new ml_Hyperlink(hwnd);
        LPCREATESTRUCT lpCreateStruct = (LPCREATESTRUCT)lParam;
        if(!pmlh)
          return FALSE;

        #if defined(WIN64) || defined(_WIN64)
        ::SetWindowLongPtr(hwnd, 0, (LONG_PTR)pmlh);
        #else
        MySetWindowLongPtr(hwnd, 0, (LONG)((LONG_PTR)pmlh));
        #endif

        pmlh->SetText(lpCreateStruct->lpszName);
      }
      return TRUE;

    case WM_PAINT:
      {
        ml_Hyperlink* pmlh = GetHyperLinkClass(hwnd);
        pmlh->Paint(hwnd);
      }
      return 0;

    case WM_LBUTTONDOWN:
      {
        ml_Hyperlink* pmlh = GetHyperLinkClass(hwnd);
        pmlh->MouseDown(hwnd, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
      }
      return 0;

    case WM_MOUSEMOVE:
      {
        ml_Hyperlink* pmlh = GetHyperLinkClass(hwnd);
        pmlh->MouseMove(hwnd, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
      }
      return 0;

    case WM_TIMER:
      {
        ml_Hyperlink* pmlh = GetHyperLinkClass(hwnd);
        pmlh->Timer(hwnd);
      }
      return 0;

    case HM_SETLINKCOLOR:
      {
        ml_Hyperlink* pmlh = GetHyperLinkClass(hwnd);
        pmlh->SetLinkColor((COLORREF)wParam);
      }
      return 0;

    case HM_SETALINKCOLOR:
      {
        ml_Hyperlink* pmlh = GetHyperLinkClass(hwnd);
        pmlh->SetALinkColor((COLORREF)wParam);
      }
      return 0;

    case HM_SETVLINKCOLOR:
      {
        ml_Hyperlink* pmlh = GetHyperLinkClass(hwnd);
        pmlh->SetVLinkColor((COLORREF)wParam);
      }
      return 0;

    case WM_SETTEXT:
      {
        ml_Hyperlink* pmlh = GetHyperLinkClass(hwnd);
        pmlh->SetText((TCHAR *)lParam);
      }
      return 0;

    case HM_SETCAPTION:
      {
        ml_Hyperlink* pmlh = GetHyperLinkClass(hwnd);
        pmlh->SetCaption((TCHAR *)lParam);
      }
      return 0;

    case HM_SETADDRESS:
      {
        ml_Hyperlink* pmlh = GetHyperLinkClass(hwnd);
        pmlh->SetAddress((TCHAR *)lParam);
      }
      return 0;

    case WM_DESTROY:
      {
        ml_Hyperlink* pmlh = GetHyperLinkClass(hwnd);
        delete pmlh;
      }
      return 0;

    default:
      return DefWindowProc(hwnd, message, wParam, lParam);
  }
}

// --------------------------------------------------------------------------

ml_Hyperlink::ml_Hyperlink(HWND hwnd)
{
  mpchCaption = NULL;
  mpchAddress = NULL;
  mHover      = FALSE;
  mVisited    = FALSE;
  HCURSOR hCursor;

  hCursor = GetHandCursor();

  // set cursor

  #if defined(WIN64) || defined(_WIN64)
  ::SetClassLongPtr(hwnd, GCLP_HCURSOR, (LONG_PTR)hCursor);
  #else
  ::SetClassLongPtr(hwnd, GCLP_HCURSOR, (LONG)((LONG_PTR)hCursor));
  #endif


  // do the font stuff
  LOGFONT lf;
  mUnderlineFont = NULL;
  mNormalFont    = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
  if(mNormalFont)
  {
    ::GetObject(mNormalFont, sizeof(LOGFONT), &lf);
    lf.lfUnderline  = TRUE;
    mUnderlineFont = ::CreateFontIndirect(&lf);
  }

  mcLink  = RGB(0, 0, 255);
  mcALink = RGB(0, 0, 255);
  mcVLink = RGB(128, 0, 128);
}

// --------------------------------------------------------------------------

ml_Hyperlink::~ml_Hyperlink()
{
  if(mUnderlineFont)
    ::DeleteObject(mUnderlineFont);

  delete [] mpchAddress;
  delete [] mpchCaption;
}

// --------------------------------------------------------------------------

void ml_Hyperlink::Paint(HWND hwnd)
{
  PAINTSTRUCT ps;
  RECT  rc;
  HFONT hfontOld;

  // draw border
  HDC  hdc = ::BeginPaint(hwnd, &ps);
  ::GetClientRect(hwnd, &rc);
  ::SetBkMode(hdc, TRANSPARENT);

  // paint depending on state
  if(mHover)
  {
    hfontOld = (HFONT)::SelectObject(hdc, mUnderlineFont);
    ::SetTextColor(hdc, mcALink);
  }
  else
  {
    hfontOld = (HFONT)::SelectObject(hdc, mNormalFont);
    ::SetTextColor(hdc, mVisited ? mcVLink : mcLink);
  }

  ::DrawText(hdc, mpchCaption, -1, &rc, /*DT_CENTER | */DT_VCENTER | DT_SINGLELINE);

  ::SelectObject(hdc, hfontOld);
  ::EndPaint(hwnd, &ps);
}

// --------------------------------------------------------------------------

void ml_Hyperlink::MouseDown(HWND hwnd, int x, int y)
{
    /*
  IGNORE_PARAM(hwnd);
  IGNORE_PARAM(x);
  IGNORE_PARAM(y);
*/
  // set visited flag
  mHover   = FALSE;
  mVisited = TRUE;

  ::InvalidateRect(hwnd, NULL, TRUE);
  ::UpdateWindow(hwnd);

  // try to open the website through ShellExecute
  HINSTANCE result = ::ShellExecute(NULL, _T("open"), mpchAddress, NULL, NULL, SW_SHOWNORMAL);


  if((INT_PTR)result <= 32)
  {
    // error message goes here
    //ErrorMessage(hwnd, IDS_ERROR_BROWSER);
  }
}

// --------------------------------------------------------------------------

void ml_Hyperlink::MouseMove(HWND hwnd, int x, int y)
{
    /*
  IGNORE_PARAM(x);
  IGNORE_PARAM(y);
   */
  if(!mHover)
  {
    mHover = TRUE;
    ::InvalidateRect(hwnd, NULL, TRUE);
    ::UpdateWindow(hwnd);
    ::SetTimer(hwnd, ::GetDlgCtrlID(hwnd), 100, NULL);
  }
}

// --------------------------------------------------------------------------

void ml_Hyperlink::Timer(HWND hwnd)
{
  POINT pt;
  RECT  rc;

  DWORD dw = ::GetMessagePos();
  pt.x = LOWORD(dw);
  pt.y = HIWORD(dw);
  ::ScreenToClient(hwnd, &pt);
  ::GetClientRect(hwnd, &rc);
  if(!PtInRect(&rc, pt))
  {
    mHover = FALSE;
    ::KillTimer(hwnd, ::GetDlgCtrlID(hwnd));
  }
  ::InvalidateRect(hwnd, NULL, TRUE);
  ::UpdateWindow(hwnd);
}

// --------------------------------------------------------------------------

void ml_Hyperlink::SetCaption(const TCHAR* pch)
{
  if(mpchCaption)
    delete [] mpchCaption;

  mpchCaption = new TCHAR[1 + lstrlen(pch)];
  lstrcpy(mpchCaption, pch);
}

// --------------------------------------------------------------------------

void ml_Hyperlink::SetAddress(const TCHAR* pch)
{
  if(mpchAddress)
    delete [] mpchAddress;

  mpchAddress = new TCHAR[1 + lstrlen(pch)];
  lstrcpy(mpchAddress, pch);
}

// --------------------------------------------------------------------------

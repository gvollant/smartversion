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
//  ml_hyperlink.h: interface for the ml_hyperlink class
// --------------------------------------------------------------------------

#ifndef _ML_HYPERLINK
#define _ML_HYPERLINK


// --------------------------------------------------------------------------
//  hyperlink window class
// --------------------------------------------------------------------------

#define HM_SETLINKCOLOR   (WM_USER + 1)
#define HM_SETALINKCOLOR  (WM_USER + 2)
#define HM_SETVLINKCOLOR  (WM_USER + 3)
#define HM_SETCAPTION     (WM_USER + 4)
#define HM_SETADDRESS     (WM_USER + 5)

// --------------------------------------------------------------------------

class ml_Hyperlink
{
  public:
    ml_Hyperlink(HWND hwnd);
    virtual ~ml_Hyperlink();

    static BOOL Register();
    static ml_Hyperlink* GetHyperLinkClass(HWND hWnd) ;
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    void Paint(HWND hwnd);
    void MouseDown(HWND hwnd, int x, int y);
    void MouseMove(HWND hwnd, int x, int y);
    void Timer(HWND hwnd);

    void SetLinkColor(COLORREF cr)  { mcLink  = cr; }
    void SetALinkColor(COLORREF cr) { mcALink = cr; }
    void SetVLinkColor(COLORREF cr) { mcVLink = cr; }
    void SetCaption(const TCHAR* pch);
    void SetAddress(const TCHAR* pch);

  private:
	void SetText(LPCTSTR lpText);

  protected:
    static BOOL mRegistered;
    TCHAR*      mpchCaption;
    TCHAR*      mpchAddress;
    COLORREF    mcLink;
    COLORREF    mcALink;
    COLORREF    mcVLink;

    HFONT       mNormalFont;
    HFONT       mUnderlineFont;
    BOOL        mHover;
    BOOL        mVisited;

};

#endif // _ML_HYPERLINK

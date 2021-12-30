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

#ifndef IGNOREOLEDRAG
#ifndef STRICT
#define STRICT
#endif

#include <ole2.h>
#include <shlobj.h>
#include <tchar.h>
#include <stdio.h>

#include "util.h"


//----------------------------------------------------------------------------------------------//
BOOL FxAssert::doAssert( const TCHAR* str, const TCHAR* file, int line )
{
  TCHAR buf[512];
  const DWORD maxStrSize = 256;
  TCHAR* pStrTooBigMsg = TEXT( "??? assert msg too long ???" );
  if ( _tcslen( str ) > maxStrSize )
  {
    str = pStrTooBigMsg;
  }

  // format for trace output (single line)
  _stprintf( buf, TEXT( "%s(%i): %s\n" ), file, line, str );
  ODS2( TEXT( "ASSERTION FAILED: " ) );
  ODS2( buf );

  // format for msg box output
  _stprintf( buf, TEXT( "%s\n\nin %s\nline%i\n" ), str, file, line );

  // active popup window for the current thread
  HWND hWndParent = GetActiveWindow();
  if (hWndParent != NULL)
    hWndParent = GetLastActivePopup(hWndParent);

  int result = ::MessageBox( hWndParent, buf, "Assertion Failed",
                             MB_TASKMODAL | MB_ICONHAND | MB_ABORTRETRYIGNORE | MB_SETFOREGROUND );
  switch ( result )
  {
    case IDABORT:  abort();
    case IDRETRY:  return TRUE;
    case IDIGNORE: return FALSE;
  }
  return FALSE;  // never gets here
}
#endif

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

#ifndef _UTIL_H
#define _UTIL_H

// these are some generic utilities that you'll probably want
// to replace with your own stuff...

// rudimentary exception handling
// if you're using MFC you can replace this with CException stuff
class xmsg
{};

class xhr : public xmsg
{
public:
  xhr( HRESULT hr ) : m_hr( hr ) {}
  operator HRESULT() const { return m_hr; }
private:
  HRESULT m_hr;
};

struct FxAssert // namespace
{
  static BOOL doAssert( const char* str, const char* file, int line );
  static int debugBreak()
  {
    DebugBreak();
    return 0;
  }
};

#ifdef _DEBUG
#define ASSERT( exp ) \
  ((void)((exp) ? 0 : ( FxAssert::doAssert( #exp, __FILE__, __LINE__ ) ? FxAssert::debugBreak() : 0 ) ))
#else
#define ASSERT( exp ) \
  ((void)(0))
#endif // _DEBUG

#ifndef PRECONDITION
#define PRECONDITION ASSERT
#endif

#ifndef POSTCONDITION
#define POSTCONDITION ASSERT
#endif

// if you're using MFC, you can #define ODS( msg ) TRACE0( msg )
#ifdef _DEBUG
#define ODS( msg ) ::OutputDebugString( TEXT( msg "\r\n" ) )
#define ODS2( msg ) ::OutputDebugString( msg )
#else
#define ODS( msg ) ((void)(0))
#define ODS2( msg ) ((void)(0))
#endif

inline HGLOBAL Duplicate( HGLOBAL hSrc )
{
  if ( !hSrc )
    return NULL;

  DWORD dwSize = (DWORD)::GlobalSize( hSrc );
  HGLOBAL hDest = ::GlobalAlloc( GPTR, dwSize );
  if ( hDest )
    ::CopyMemory( hDest, hSrc, dwSize );

  return hDest;
}

class WideStr
{
public:
  WideStr( const char* mbStr );
  WideStr( const WCHAR* wstr );
 ~WideStr() { delete m_wstr; m_wstr = NULL; }
  operator const WCHAR*() { return m_wstr; }
  WCHAR* Detach() { WCHAR* p = m_wstr; m_wstr = NULL; return p; }
private:
  WCHAR* m_wstr;
};

#endif // _UTIL_H

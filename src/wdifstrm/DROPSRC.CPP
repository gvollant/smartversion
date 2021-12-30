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

#include <ole2.h>
#include <shlobj.h>
#include <tchar.h>
#include "util.h"

#include "DropSrc.h"
#include "DropHook.h"
#include "util.h"

CDropSource::CDropSource( CDropHook* pDropHook )
  : m_refs( 1 ),
    m_pDropHook( pDropHook )
{}

CDropSource::~CDropSource()
{
  delete m_pDropHook;
}

STDMETHODIMP CDropSource::QueryInterface(REFIID iid, void FAR* FAR* ppv)
{
//    OutputDebugString("CDropSource::QueryInterface\r\n");

    if(iid == IID_IUnknown || iid == IID_IDropSource)
    {
      *ppv = this;
      ++m_refs;
      return NOERROR;
    }
    *ppv = NULL;

    return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) CDropSource::AddRef(void)
{
//    OutputDebugString("CDropSource::AddRef\r\n");

    return ++m_refs;
}


STDMETHODIMP_(ULONG) CDropSource::Release(void)
{
//    OutputDebugString("CDropSource::Release\r\n");

    if(--m_refs == 0)
    {
      delete this;
      return 0;
    }
    return m_refs;
}

STDMETHODIMP CDropSource::QueryContinueDrag(BOOL fEscapePressed,
                                            DWORD grfKeyState)
{
    // if the user released the mouse and there's a drop hook,
    // give it a chance to cancel the operation.
    if ( !( grfKeyState & MK_LBUTTON ) && m_pDropHook )
    {
      ODS( "QueryContinueDrag( MK_LBUTTON ) - calling notify hook" );
      if ( m_pDropHook->OkToDrop() )
      {
        ODS( "notify hook said it was OK to drop" );
      }
      else
      {
        ODS( "notify hook said CANCEL the drop" );
        return DRAGDROP_S_CANCEL;
      }
    }

    if (fEscapePressed)
        return DRAGDROP_S_CANCEL;
    else if (!(grfKeyState & MK_LBUTTON))
        return DRAGDROP_S_DROP;
    else
        return NOERROR;
}


extern HCURSOR hCurNone;
extern HCURSOR hCurCopy;
extern BOOL fIsDragCopy;

STDMETHODIMP CDropSource::GiveFeedback(DWORD dwEffect)
{


    //return DRAGDROP_S_USEDEFAULTCURSORS;
    if (dwEffect & (DROPEFFECT_MOVE|DROPEFFECT_COPY))
    {
        SetCursor(hCurCopy);
        fIsDragCopy =(dwEffect & DROPEFFECT_COPY)!=0;
    }
    else
        SetCursor(hCurNone);

    //SetCursor
    return S_OK ;
}

#endif

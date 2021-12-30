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

#include "DataObj.h"
#include "enumfetc.h"
#include "fsmgr.h"
#include "util.h"
#include "stdio.h"
#include "idlist.h"

#ifndef _WIN32
#error this module designed for use with WIN32
#endif

// don't forget null terminator when considering size of idlists
inline UINT IDLISTSIZE( const ITEMIDLIST* pidl )
{ return pidl->mkid.cb + sizeof( USHORT ); }

//--------------------------
// CDataObject
//--------------------------
CDataObject::CDataObject( CFileSetManager* pFileSetManager )
  : m_refs( 1 ),
    m_cfShellIDListArray( RegisterClipboardFormat( TEXT( CFSTR_SHELLIDLIST ) ) ),
    m_pFileSetManager( pFileSetManager ),
    m_hDropfiles( NULL ),
    m_bUnzipped( FALSE )
{
  ASSERT( m_pFileSetManager );
}

CDataObject::~CDataObject()
{
  ODS( "CDataObject dtor" );
  delete m_pFileSetManager;
  if ( m_hDropfiles ) ::GlobalFree( m_hDropfiles );
}

//---------------------------------------------------------------------
//                    IUnknown Methods
//---------------------------------------------------------------------
STDMETHODIMP CDataObject::QueryInterface( const IID& iid, void** ppv )
{
  ODS( "CDataObject::QueryInterface()" );
  if ( !ppv )
    return E_POINTER;

  if ( IID_IUnknown == iid || IID_IDataObject == iid )
  {
    *ppv = (IDataObject*) this;
    AddRef();
    return NOERROR;
  }
  *ppv = NULL;
  return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) CDataObject::AddRef()
{
  ODS( "CDataObject::AddRef()" );
  return ++m_refs;
}


STDMETHODIMP_(ULONG) CDataObject::Release()
{
  ODS( "CDataObject::Release()" );
  if ( --m_refs == 0 )
  {
    delete this;
    return 0;
  }
  return m_refs;
}

//---------------------------------------------------------------------
//                    IDataObject Methods
//
// The following methods are NOT supported for data transfer using the
// clipboard or drag-drop:
//
//      IDataObject::SetData    -- return E_NOTIMPL
//      IDataObject::DAdvise    -- return OLE_E_ADVISENOTSUPPORTED
//                 ::DUnadvise
//                 ::EnumDAdvise
//      IDataObject::GetCanonicalFormatEtc -- return E_NOTIMPL
//                     (NOTE: must set pformatetcOut->ptd = NULL)
//---------------------------------------------------------------------

STDMETHODIMP CDataObject::GetData( FORMATETC* pformatetc, STGMEDIUM* pmedium )
{
  HRESULT hres = E_INVALIDARG;

  pmedium->hGlobal = NULL;
  pmedium->pUnkForRelease = NULL;

  if ( ( CF_HDROP == pformatetc->cfFormat ) && ( pformatetc->tymed & TYMED_HGLOBAL ) )
  {
    ODS( "GetData( CF_HDROP )" );
    hres = _renderDropfiles( pformatetc, pmedium );
  }
  else if ( ( m_cfShellIDListArray == pformatetc->cfFormat ) && ( pformatetc->tymed & TYMED_HGLOBAL ) )
  {
    ODS( "GetData( Shell IDList Array )" );

    // the shell is ready to accept the files, so we must render them now
    //try
    {
      if ( !m_bUnzipped && m_pFileSetManager->ReadyToUnzip() )
      {
        m_pFileSetManager->Unzip();
        m_bUnzipped = TRUE;
        hres = _renderShellIDListArray( pformatetc, pmedium );
      }
      else hres = E_FAIL;
    }
    /*
    catch( ... )
    {
      // catches any exceptions thrown from Unzip()
      hres = E_UNEXPECTED;
    }
    */
  }
  else
  {
    TCHAR formatName[200];
    GetClipboardFormatName( pformatetc->cfFormat, formatName, sizeof formatName / sizeof formatName[0] );
    TCHAR str2[256];
    _stprintf( str2, TEXT( "GetData( ? %s ? )\r\n" ), formatName );
    ODS2( str2 );
  }
  return hres;
}

STDMETHODIMP CDataObject::GetDataHere( FORMATETC* /* pformatetc */, STGMEDIUM* /* pmedium */ )
{
  ODS( "CDataObject::GetDataHere()" );
  return E_NOTIMPL;
}

STDMETHODIMP CDataObject::QueryGetData( FORMATETC* pformatetc )
{
  if ( ( CF_HDROP == pformatetc->cfFormat ) && ( pformatetc->tymed & TYMED_HGLOBAL ) )
  {
    ODS( "QueryGetData( CF_HDROP )" );
    return S_OK;
  }
  else if ( ( m_cfShellIDListArray == pformatetc->cfFormat ) && ( pformatetc->tymed & TYMED_HGLOBAL ) )
  {
    ODS( "QueryGetData( Shell IDList Array )" );
    return S_OK;
  }
  else
  {
    TCHAR formatName[200];
    GetClipboardFormatName( pformatetc->cfFormat, formatName, sizeof formatName / sizeof formatName[0] );
    TCHAR str2[256];
    _stprintf( str2, TEXT( "QueryGetData( ? %s ? )\r\n" ), formatName );
    ODS2( str2 );
  }
  return S_FALSE;
}

STDMETHODIMP CDataObject::GetCanonicalFormatEtc( FORMATETC* /* pformatetc */, FORMATETC* /* pformatetcOut */ )
{
  ODS( "CDataObject::GetCanonicalFormatEtc()" );
  return DATA_S_SAMEFORMATETC;
}

STDMETHODIMP CDataObject::SetData( FORMATETC* /* pformatetc */, STGMEDIUM* /* pmedium */, BOOL /* fRelease */ )
{
  ODS( "CDataObject::SetData()" );
  return E_NOTIMPL;
}

STDMETHODIMP CDataObject::EnumFormatEtc( DWORD /* dwDirection */, IEnumFORMATETC** ppenumFormatEtc )
{
  ODS( "CDataObject::EnumFormatEtc()" );
  FORMATETC fmtetc[] = { { CF_HDROP,             NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
                         { m_cfShellIDListArray, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL } };


  *ppenumFormatEtc = OleStdEnumFmtEtc_Create( sizeof fmtetc / sizeof fmtetc[0], fmtetc );

  if (*ppenumFormatEtc == NULL)
    return E_OUTOFMEMORY;

  return S_OK;
}

STDMETHODIMP CDataObject::DAdvise( FORMATETC* /* pFormatetc */, DWORD /* advf */, IAdviseSink* /* pAdvSink */, DWORD* /* pdwConnection */ )
{
  ODS( "CDataObject::DAdvise()" );
  return OLE_E_ADVISENOTSUPPORTED;
}


STDMETHODIMP CDataObject::DUnadvise( DWORD /* dwConnection */ )
{
  ODS( "CDataObject::DUnadvise()" );
  return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP CDataObject::EnumDAdvise( IEnumSTATDATA** /* ppenumAdvise */ )
{
  ODS( "CDataObject::EnumDAdvise()" );
  return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT CDataObject::_renderDropfiles( FORMATETC* pformatetc, STGMEDIUM* pmedium )
{
//  ODS( "CDataObject::_renderDropfiles()" );
  pmedium->hGlobal = NULL;
  HANDLE fileHandle = INVALID_HANDLE_VALUE;

  HRESULT hr = NOERROR;
  //try
  {
    if ( !m_hDropfiles )
    {
      // we only ask the FileSetManager to render it once,
      // then we hand out copies of the cached rendering.
      // we'll get hit many times for CF_HDROP,
      // so this makes us much more efficient.
      hr = _renderDropfilesOnHGlobal( &m_hDropfiles );
      if ( FAILED( hr ) )
        return hr;//throw xmsg();
    }
    pmedium->hGlobal = Duplicate( m_hDropfiles );
    if ( !pmedium->hGlobal )
    {
      hr = E_OUTOFMEMORY;
      return hr;//throw xmsg();
    }
    pmedium->tymed = TYMED_HGLOBAL;
  }
  /*
  catch ( ... )
  {}
  */
  return hr;
}

HRESULT CDataObject::_renderDropfilesOnHGlobal( HGLOBAL* phGlobal )
{
//  ODS( "CDataObject::_renderDropfilesOnHGlobal()" );
  ASSERT( phGlobal );
  HGLOBAL hGlobal = NULL;
  const char** sourceFiles = NULL;
  HRESULT hr = S_OK;
  UINT i;
  //try
  {
    const UINT itemCount = m_pFileSetManager->GetCount();

    sourceFiles = new const char*[itemCount];

    if ( !sourceFiles )
    {
      hr = E_OUTOFMEMORY;
      //throw xmsg();
    }

    if (hr==S_OK)
    {

    // collect the source paths from the FileSetManager and add up the size
    DWORD dwSize = sizeof( DROPFILES );
    for ( i = 0; i < itemCount; i++ )
    {
      const char* sourcePath = m_pFileSetManager->GetSourcePath( i );
      ASSERT( sourcePath );

      dwSize += lstrlen( sourcePath ) + sizeof( char );
      sourceFiles[i] = sourcePath;
    }
    dwSize += sizeof( char ); // add for final terminating null

    hGlobal = ::GlobalAlloc( GPTR, dwSize );

    if ( !hGlobal )
    {
      hr = E_OUTOFMEMORY;
      //throw xmsg();
    }
    }

    if (hr==S_OK)
    {
    // init the header
    DROPFILES* pDropFiles = (DROPFILES*) hGlobal;
    ::ZeroMemory( pDropFiles, sizeof( DROPFILES ) );
    pDropFiles->pFiles = sizeof( DROPFILES );

    // now copy the filenames into the global memory handle
    char* pNextItem = (char*) ( ( (char*) pDropFiles ) + sizeof( DROPFILES ) );
    for ( i = 0; i < itemCount; i++ )
    {
      lstrcpy( pNextItem, sourceFiles[i] );
      pNextItem += lstrlen( sourceFiles[i] ) + 1;
    }
    *pNextItem = '\0';

    delete sourceFiles;
    }
  }
  //catch ( ... )
  if (hr!=S_OK)
  {
    if ( hGlobal )
    {
      ::GlobalFree( hGlobal );
      hGlobal = NULL;
    }
    delete sourceFiles;
  }
  *phGlobal = hGlobal;
  return hr;
}

HRESULT CDataObject::_renderShellIDListArray( FORMATETC* pformatetc, STGMEDIUM* pmedium )
{
//  ODS( "CDataObject::_renderShellIDListArray()" );
xhr* phxr=NULL;
UINT i;

  ITEMIDLIST** apidl = NULL;
  IShellFolder* pDesktopFolder = NULL;

  UINT itemCount = 0;

  HRESULT hr = S_OK;
  //try
  {
    {
      HRESULT hr = SHGetDesktopFolder( &pDesktopFolder );
      ASSERT( SUCCEEDED( hr ) );
      ASSERT( pDesktopFolder );

      if ( !pDesktopFolder )
        phxr = new xhr( E_FAIL );
    }

    if (phxr==NULL)
    {
    itemCount = m_pFileSetManager->GetCount();

    // this array will hold all the IDLists
    // (the first is the absolute idlist of the folder)
    apidl = new ITEMIDLIST*[itemCount];
    ::ZeroMemory( apidl, sizeof( ITEMIDLIST* ) * ( itemCount ) );

    if ( !apidl )
      phxr = new xhr( E_OUTOFMEMORY );
    }

    // collect the filenames from the FileSetManager
    // and let the desktop parse them into idlists
    if (phxr==NULL)
     for ( i = 0; i < itemCount; i++ )
    {
      const char* sourcePath = m_pFileSetManager->GetSourcePath( i );
      ASSERT( sourcePath );

      ULONG chEaten = 0;  // note this is not currently set by the desktop
      ITEMIDLIST* pidlRelative = NULL;
      ULONG attr = 0;
      WideStr ws( sourcePath );
      WCHAR* wstr = ws.Detach();

      //HRESULT hr = pDesktopFolder->ParseDisplayName( NULL, NULL, wstr, &chEaten, &pidlRelative, &attr );
      HRESULT hr = pDesktopFolder->ParseDisplayName( NULL, NULL, wstr, &chEaten, (UNALIGNED struct _ITEMIDLIST **)&pidlRelative, &attr );
      delete wstr; wstr = NULL;
      ASSERT( SUCCEEDED( hr ) );
      ASSERT( pidlRelative );

      if ( !pidlRelative )
      {
        phxr = new xhr( E_FAIL );
        break;
      }

      apidl[i] = pidlRelative;
    }

    // now ask the source folder to render a CIDA on our behalf
    // (thank you very much, desktop!)
    if (phxr==NULL)
    {
      IDataObject* pDataObj = NULL;
      //HRESULT hr = pDesktopFolder->GetUIObjectOf( NULL, itemCount, (const ITEMIDLIST**) apidl, IID_IDataObject, NULL, (void**)&pDataObj );
      HRESULT hr = pDesktopFolder->GetUIObjectOf( NULL, itemCount, (UNALIGNED const struct _ITEMIDLIST **)(const ITEMIDLIST**) apidl, IID_IDataObject, NULL, (void**)&pDataObj );
      ASSERT( SUCCEEDED( hr ) );
      ASSERT( pDataObj );
      if ( FAILED( hr ) )
        phxr = new xhr( E_FAIL );

      if (phxr==NULL)
      {
      hr = pDataObj->GetData( pformatetc, pmedium );
      if ( FAILED( hr ) )
        phxr = new xhr( E_FAIL );//throw xhr( E_FAIL );
      }
      if (phxr==NULL)
        pDataObj->Release();

    }

    // finally, do some cleanup and we're done!
    if (phxr==NULL)
    {
    for ( i = 0; i < itemCount; i++ )
    {
      ::CoTaskMemFree( apidl[i] );
    }
    delete [] apidl;
    pDesktopFolder->Release();
    }
  }

  if (phxr!=NULL)
  {
    for ( UINT i = 0; i < itemCount; i++ )
    {
      if ( apidl[i] )
        ::CoTaskMemFree( apidl[i] );
    }
    delete [] apidl;
    if ( pDesktopFolder ) pDesktopFolder->Release();

    hr = *phxr;
    delete(phxr);
  }
  return hr;
}
#endif

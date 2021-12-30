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

#ifndef _DATAOBJ_H
#define _DATAOBJ_H

class CFileSetManager;

// CDataObject
//
// Implementation of IDataObject that exposes a collection of files to a
// consumer of FileContents and FileGroupDescriptor formats (namely the shell).
// A CFileSetManager must be provided to render the files.
// CDataObject will destruct the CFileSetManager when the CDataObject is released,
// (i.e. when the drop operation is finished, or the clipboard contents have been replaced
//  by something else.)
//
class CDataObject : public IDataObject
{
public:
  // ctor requires a FileSetManager
  CDataObject( CFileSetManager* pFileSetManager );

  // dtor destructs the FileSetManager
 ~CDataObject();

  //*** IUnknown ***
  STDMETHOD(QueryInterface)(REFIID riid, void** ppvObj);
  STDMETHOD_(ULONG, AddRef)(void);
  STDMETHOD_(ULONG, Release)(void);

  //*** IDataObject ***
  STDMETHOD(GetData)(FORMATETC* pformatetcIn,  STGMEDIUM* pmedium );
  STDMETHOD(GetDataHere)(FORMATETC* pformatetc, STGMEDIUM* pmedium );
  STDMETHOD(QueryGetData)(FORMATETC* pformatetc );
  STDMETHOD(GetCanonicalFormatEtc)(FORMATETC* pformatetc, FORMATETC* pformatetcOut);
  STDMETHOD(SetData)(FORMATETC* pformatetc, STGMEDIUM* pmedium, BOOL fRelease);
  STDMETHOD(EnumFormatEtc)(DWORD dwDirection, IEnumFORMATETC** ppenumFormatEtc);
  STDMETHOD(DAdvise)(FORMATETC* pFormatetc, DWORD advf, IAdviseSink* pAdvSink, DWORD* pdwConnection);
  STDMETHOD(DUnadvise)(DWORD dwConnection);
  STDMETHOD(EnumDAdvise)(IEnumSTATDATA** ppenumAdvise);

private:
  ULONG m_refs;                        // COM ref count
  const UINT m_cfShellIDListArray;
  CFileSetManager* m_pFileSetManager;  // the guy who manages & renders the files
  HGLOBAL m_hDropfiles;                // cached DROPFILES for efficiency
  BOOL m_bUnzipped;                    // TRUE if we've unzipped the files (only happens once)

  HRESULT _renderDropfiles( FORMATETC* pformatetc, STGMEDIUM* pmedium );
  HRESULT _renderDropfilesOnHGlobal( HGLOBAL* phGlobal );
  HRESULT _renderShellIDListArray( FORMATETC* pformatetc, STGMEDIUM* pmedium );
};

#endif // _DATAOBJ_H

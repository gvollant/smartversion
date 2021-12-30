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
#include "util.h"


#ifndef ASSERT
#define ASSERT( exp ) ((void)(0))
#endif

#ifndef PRECONDITION
#define PRECONDITION ASSERT
#endif

#ifndef POSTCONDITION
#define POSTCONDITION ASSERT
#endif

#ifndef FxTaskMemFree
#define FxTaskMemFree ::CoTaskMemFree
#endif

#ifndef FxTaskMemAlloc
#define FxTaskMemAlloc ::CoTaskMemAlloc
#endif

#include "IDList.h"

//-----------------------------------------
// CIDList
//-----------------------------------------
CIDList::CIDList( const ITEMIDLIST* pidl )
  : _pidl( (ITEMIDLIST*) pidl ), _owner( FALSE )
{}

//----------------------------------------------------------------------------------------------//
CIDList::CIDList( const SHITEMID& itemID )
  : _pidl( NULL ), _owner( FALSE )
{
  CopyFrom( itemID );
}

//----------------------------------------------------------------------------------------------//
CIDList::~CIDList()
{
  if ( _owner )
  {
    FxTaskMemFree( _pidl );
    _pidl = NULL;
  }
}

//----------------------------------------------------------------------------------------------//
ITEMIDLIST* CIDList::Detach()
{
  PRECONDITION( _pidl );
  ITEMIDLIST* retVal = _pidl;
  _pidl = NULL;
  _owner = FALSE;
  return retVal;
}

//----------------------------------------------------------------------------------------------//
int CIDList::GetSize() const
{
  int total = 0;
  ITEMIDLIST* pidl = _pidl;

  if ( pidl )
  {
    total += sizeof( pidl->mkid.cb ); // this is the null terminator
    while( pidl->mkid.cb )
    {
      total += pidl->mkid.cb;
      pidl = Tail( pidl );
    }
  }
  return total;
}

//----------------------------------------------------------------------------------------------//
void CIDList::CopyFrom( const ITEMIDLIST* pidlSource )
{
  PRECONDITION( pidlSource );

  int cbSource = GetSize( pidlSource );

  if( _owner )
  {
    FxTaskMemFree( _pidl );
  }

  _pidl = (ITEMIDLIST*) FxTaskMemAlloc( cbSource );
  _owner = TRUE;

  memcpy( _pidl, pidlSource, cbSource );

  POSTCONDITION( _pidl );
}

//----------------------------------------------------------------------------------------------//
void CIDList::CopyFrom( const SHITEMID& sourceItem )
{
  if( _owner )
  {
    FxTaskMemFree( _pidl );
  }

  _pidl = (ITEMIDLIST*) FxTaskMemAlloc( sourceItem.cb + sizeof( USHORT ) );
  _owner = TRUE;

  memcpy( _pidl, &sourceItem, sourceItem.cb );

  // null terminate
  *(USHORT*)(((BYTE*)_pidl)+sourceItem.cb) = 0;

  POSTCONDITION( _pidl );
}

//----------------------------------------------------------------------------------------------//
void CIDList::Append( const ITEMIDLIST* pidlToAppend )
{
  PRECONDITION( pidlToAppend );

  // If we are empty then just do a copy.
  if( !_pidl )
  {
    CopyFrom( pidlToAppend );
  }
  else
  {
    int originalSize = GetSize();
    int toAppendSize = GetSize( pidlToAppend ) + originalSize;

    // remember that each is null terminated, and we only need one terminator
    // (thus the sizeof( USHORT ) subtractions)
    ITEMIDLIST* pidlNew = (ITEMIDLIST*) FxTaskMemAlloc( originalSize + toAppendSize - sizeof( USHORT ) );

    memcpy( pidlNew, _pidl, originalSize );
    memcpy( (BYTE*)(((BYTE*)pidlNew) + ((DWORD)originalSize) - ((DWORD)sizeof(USHORT))), pidlToAppend, toAppendSize );

    // if I currently own my idlist, free it now.
    if ( _owner )
    {
      FxTaskMemFree( _pidl );
    }
    _pidl = pidlNew;
    _owner = TRUE;
  }
  POSTCONDITION( _pidl );
  POSTCONDITION( _owner );
}

//----------------------------------------------------------------------------------------------//
void CIDList::Append( const SHITEMID& itemToAppend )
{
  // If we are empty then just do a copy.
  if( !_pidl )
  {
    CopyFrom( itemToAppend );
  }
  else
  {
    int originalSize = GetSize();

    // note that originalSize already considers the null terminator
    ITEMIDLIST* pidlNew = (ITEMIDLIST*) FxTaskMemAlloc( originalSize + itemToAppend.cb );

    memcpy( pidlNew, _pidl, originalSize );
    memcpy( (BYTE*)(((BYTE*)pidlNew) + ((DWORD)originalSize) - ((DWORD)sizeof(USHORT))), &itemToAppend, itemToAppend.cb );

    // null terminate
    *(USHORT*)(((BYTE*)pidlNew) + originalSize + itemToAppend.cb - sizeof( USHORT ) ) = 0;

    if ( _owner )
    {
      FxTaskMemFree( _pidl );
    }
    _pidl = pidlNew;
    _owner = TRUE;
  }
  POSTCONDITION( _pidl );
  POSTCONDITION( _owner );
}

//----------------------------------------------------------------------------------------------//
int CIDList::GetNumElements() const
{
  int count = 0;
  if( _pidl )
  {
    for ( ITEMIDLIST* pNext = _pidl; pNext->mkid.cb; pNext = Tail( pNext ) )
    {
      count++;
    }
  }
  return count;
}
#endif

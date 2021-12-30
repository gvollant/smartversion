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

#ifndef _IDLIST_H
#define _IDLIST_H

//-----------------------------------------
// CIDList
//-----------------------------------------
class CIDList
{
public:
  CIDList( const ITEMIDLIST* pidl = NULL );
  CIDList( const SHITEMID& itemID );
 ~CIDList();

  const ITEMIDLIST* GetIDList() { return _pidl; }
//  operator const ITEMIDLIST*()  { return GetIDList(); }  may be ambiguous with ctor

  ITEMIDLIST* Detach();
  void CopyFrom(const ITEMIDLIST* pidl);
  void CopyFrom(const SHITEMID& itemID);
  void Append(const ITEMIDLIST* pidl);
  void Append(const SHITEMID& itemID);
  int GetSize() const;
  int GetNumElements() const;

  static ITEMIDLIST* Skip(const ITEMIDLIST* pList, int cb)
  { return((ITEMIDLIST*)(((BYTE*)(pList))+cb)); }

  static ITEMIDLIST* Tail(const ITEMIDLIST* pidl)
  { PRECONDITION( pidl ); return(Skip(pidl, pidl->mkid.cb)); }

  static int CIDList::GetSize( const ITEMIDLIST* pidl )
  { CIDList idlTemp( (ITEMIDLIST*) pidl ); return idlTemp.GetSize(); }

  static int CIDList::GetNumElements( const ITEMIDLIST* pidl )
  { CIDList idlTemp( (ITEMIDLIST*) pidl ); return idlTemp.GetNumElements(); }

  static BOOL IsSimple( const ITEMIDLIST* pidl )
  { PRECONDITION( pidl && pidl->mkid.cb ); return 0 == Tail( pidl )->mkid.cb; }

  static ITEMIDLIST* Dup( const ITEMIDLIST* pidl )
  { PRECONDITION( pidl ); CIDList idl; idl.CopyFrom( pidl ); return idl.Detach(); }

  const SHITEMID& Head() const
  { PRECONDITION( _pidl ); return _pidl->mkid; }

  const ITEMIDLIST* Tail() const
  { PRECONDITION( _pidl ); return Tail( _pidl ); }

  BOOL IsSimple() const  // answers TRUE if the idlist is only one deep
  { PRECONDITION( _pidl ); return IsSimple( _pidl ); }

private:
  ITEMIDLIST* _pidl;
  int _owner; // if this is true then the above pointed to idlist belongs
              // to this instance, otherwise it points to an ITEMIDLIST
              // owned by someone else
};

#endif // _IDLIST_H

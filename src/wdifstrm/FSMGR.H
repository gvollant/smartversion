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

#ifndef _FSMGR_H
#define _FSMGR_H

// CFileSetManager
//
// Abstract base class that defines the interface that a CDataObject
// needs to help it render files for the shell.
// Nico - derive from this class to provide your file management code for unzipping
//        and deleting temporary files. See fsmgrex.* for an example.
//
class CFileSetManager
{
public:
  // override the destructor to do any cleanup when the manager is released
  // (i.e. release memory, delete files, etc.)
  virtual ~CFileSetManager() {}

  // answers count of files (must be constant)
  virtual UINT GetCount() = 0;

  // override this to answer whether you're ready to unzip files
  // (you can use CDropHook to determine this for drag drop operation.)
  virtual BOOL ReadyToUnzip() = 0;

  // override this and unzip all the files
  virtual void Unzip() = 0;

  // answer the full path to the nth source file
  // the strings you provide should stay alive for the lifetime of
  // the CFileSetManager (so the caller doesn't have to dup them)
  // Also note these are ANSI strings, not UNICODE. I found out via
  // the WinDev archives that CF_HDROP must be rendered in ANSI.
  virtual const char* GetSourcePath( UINT zeroBasedIndex ) = 0;
};

#endif // _FSMGR_H

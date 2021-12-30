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

//
#include "difbasic.h"

#if defined(SMARTVERSION_USE_WINRT) || defined(DIFSTRM_USING_WINRT_API)
#include <Windows.h>
#include "dfsToolWinRT.h"



#if defined(WINAPI_FAMILY_PARTITION) && (!(defined(DIFSTRM_USING_WINRT_API))) && (!(defined(DIFSTRM_PREVENT_USING_WINRT_API)))
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
#define DIFSTRM_USING_WINRT_API 1
#endif
#endif

#ifdef DIFSTRM_USING_WINRT_API
#include <collection.h>
#include <ppltasks.h>


#include "difbasic.h"
#include "DfsTlTyp.h"
#include "difstool.h"

BOOL GetTempPathWinRT(DWORD nSize,LPWSTR szTempDirectoryWindows)
{
	Platform::String^ tmpPath= Windows::Storage::ApplicationData::Current->TemporaryFolder->Path;
	const wchar_t * tmpStr = tmpPath->Data();
	LPCWSTR tmpStr2 = tmpPath->Data();

	dfuLong32 len = (dfUnicodeStrlen((dfwcharp)tmpStr2)+1)*2;
	if (len>=nSize)
		return FALSE;
	DfsMemcpy(szTempDirectoryWindows,tmpStr2,len);
	return TRUE;
}

BOOL GetTempFileNameWinRT(DWORD nSize,LPWSTR szTempDirectoryWindows)
{

	//String^ createGUID(bool forFileName)

  if (nSize<0x80)
		return FALSE;
  if (!(GetTempPathWinRT(nSize-0x60,szTempDirectoryWindows)))
	  return FALSE;
  //wchar_t guidStr[120];
  GUID* g = 0x00;
  g = new GUID;
  CoCreateGuid(g);
  //wchar_t* maskForFN = L"%08x_%04x_%04x_%02x%02x_%02x%02x%02x%02x%02x%02x";
  wchar_t* mymaskForFN = L"\\%08x_%04x_%04x_%02x%02x_%02x%02x%02x%02x%02x%02x";
  //wchar_t* mask = L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x";

  swprintf( szTempDirectoryWindows+dfUnicodeStrlen((dfwcharpc)szTempDirectoryWindows),
	  nSize-dfUnicodeStrlen((dfwcharpc)szTempDirectoryWindows),
	  mymaskForFN,g->Data1,g->Data2,g->Data3,UINT(g->Data4[0]),UINT(g->Data4[1]),UINT(g->Data4[2]),UINT(g->Data4[3]),UINT(g->Data4[4]),UINT(g->Data4[5]),UINT(g->Data4[6]),UINT(g->Data4[7]));
  delete g;

  return TRUE;
}
#endif
#endif

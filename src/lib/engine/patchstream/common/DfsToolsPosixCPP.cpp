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


#include "difbasic.h"

#if defined(SMARTVERSION_USE_POSIX)

#include <stddef.h>
#include <string.h>

#include "difbasic.h"
#include "DfsTlTyp.h"
#include "difstool.h"

#ifndef ANDROID
#include <cwchar>
#include <clocale> //for setlocale
#include <cstdlib> //for SUCCESS
#endif
#define STD_BUF_CHARCNV (0x7ff)



void DispOutUnicodeString_CPP(dfwcharpc str)
{
    wchar_t szStdConvertBuffer[STD_BUF_CHARCNV+1];
    dfuLong32 dfUnicSize = dfUnicodeStrlen(str);
    dfuLong32 i;
    wchar_t* pUseBuf;

    if (dfUnicSize>=STD_BUF_CHARCNV)
        pUseBuf = (wchar_t*)DfsMalloc((dfUnicSize+2)*sizeof(wchar_t));
    else
        pUseBuf = szStdConvertBuffer;

    for (i=0;i<=dfUnicSize;i++)
    {
        DFWCHAR_BYTEACCESS dfwb;
        dfwb.u.wc = *(str+i);
        *(pUseBuf+i) = dfwb.u.b[0] | (((wchar_t)dfwb.u.b[1])<<8);
    }
	#ifndef ANDROID
    std::setlocale(LC_ALL,"French");
    std::wprintf(pUseBuf);
	#endif

    if (pUseBuf != szStdConvertBuffer)
        DfsFree(pUseBuf);
}

#endif

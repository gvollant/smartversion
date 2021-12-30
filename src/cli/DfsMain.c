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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <stddef.h>
#include <stdio.h>

#include "../misc/svfdll.h"

#include "DfsCdLin.h"

#if defined(DFS_MAIN_C)

#include <windows.h>



#define COMPRESS_RATIO (2)


static BOOL IsUnicodeSupported()
{
    return  ((GetVersion() & 0x80000000) == 0); // WINNT
}


int main(int argc, char *argv[])
{
int iRet=0;
dfwcharp pCommandLine;
    SetErrorMode(SEM_NOALIGNMENTFAULTEXCEPT);   // FOR RISC and ia64
    DfsCrc32(0,(BYTE*)" ",1); // init CRC32



    if (IsUnicodeSupported())
    {
        pCommandLine = GetCommandLineW();
        iRet=PerformCommandLine(pCommandLine);
    }
    else
    {
        LPCSTR lpCommandLineAnsi = GetCommandLineA();
        DWORD dwSize = (lstrlenA(lpCommandLineAnsi)*4) + 0x10;
        pCommandLine = DfsMalloc(dwSize+0x10);
        ConvertAnsiToUnicode(lpCommandLineAnsi,pCommandLine,dwSize/sizeof(dfwchar));
        iRet=PerformCommandLine(pCommandLine);
        DfsFree(pCommandLine);
    }

    return iRet;
}
#endif

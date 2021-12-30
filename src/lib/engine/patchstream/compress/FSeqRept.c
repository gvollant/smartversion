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

#include <stddef.h>
#include <stdio.h>

#include <memory.h>

#include "../common/difbasic.h"
#include "../common/difstrm.h"
#include "../common/DfsOrigMemoryMap.h"
#include "FindSeq.h"
#include <windows.h>



#ifndef FINDSEQ_REPORT
#define FINDSEQ_REPORT
#endif

#include "FSeqRept.h"

static HANDLE hfr=NULL;
static DWORD dwTimeBeginFind=0;

void BeginFindSeq()
{
    dwTimeBeginFind=GetTickCount();
}

void OpenFindSeq()
{
    if (hfr==NULL)
    {
        DWORD dwDesiredAccess = GENERIC_WRITE | GENERIC_READ;
        DWORD dwCreationDisposition = CREATE_ALWAYS;
        DWORD dwShareMode = 0;
        DWORD dwFlagsAndAttributes = 0;
        SYSTEMTIME st,stgmt;
        char szFn[MAX_PATH];


        GetSystemTime(&stgmt);
        SystemTimeToTzSpecificLocalTime(NULL,&stgmt,&st);

        wsprintf(szFn,"ReportFile_%02u_%02u_%02u___%02uh%02um%02us.txt",
                       st.wDay,st.wMonth,st.wYear%100,
                       st.wHour,st.wMinute,st.wSecond);

        hfr = CreateFile(szFn, dwDesiredAccess, dwShareMode, NULL,
                        dwCreationDisposition, dwFlagsAndAttributes, NULL);

    }
}

void ReportFindSeq(const char* dfProg,
                   dfuLong64 dwBeginSearch,dfuLong64 dwMaxSearchOrg,
                   dfuLong32 dwMinInter,
                   dfuLong32 dfLengthStopSearch,
                   dfuLong64 dwSizeOrg,dfuLong64 dwLatestOrg)
{
    char szLine[512];
    DWORD dwWritten=0;
    DWORD dwTimeFind = 0xffffffff;
    dfuLong64 dwSizeRech;


    if (dwTimeBeginFind != 0)
    {
        dwTimeFind = GetTickCount()-dwTimeBeginFind;
        dwTimeBeginFind=0;
    }


    if (dwMaxSearchOrg == 0)
      dwSizeRech = dwSizeOrg-dwBeginSearch;
    else
      dwSizeRech = dwMaxSearchOrg-dwBeginSearch;

    wsprintf(szLine,"%s,%u,%u,%u,%u,%u,%u,%u\n",dfProg,dwBeginSearch,dwMaxSearchOrg,dwMinInter,
                   dwSizeOrg,dwLatestOrg,(DWORD)dwSizeRech,dwTimeFind);
    if (hfr!=NULL)
      WriteFile(hfr,szLine,lstrlen(szLine),&dwWritten,NULL);
}


void CloseReportFindSeq()
{
    if (hfr!=NULL)
        CloseHandle(hfr);
    hfr=NULL;
}

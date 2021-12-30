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


#ifndef BYTE
typedef dfbyte BYTE;
#endif

#ifndef VOID
typedef void VOID;
#endif

#ifndef CONST
#define CONST               const
#endif

#ifndef LPBYTE
typedef BYTE *LPBYTE, *PBYTE;
#endif

#ifndef LPCBYTE
typedef CONST BYTE *LPCBYTE, *PCBYTE;
#endif

#ifndef LPCVOID
typedef CONST VOID *LPCVOID, *PCVOID;
#endif

#ifndef LPVOID
typedef VOID *LPVOID, *PVOID;
#endif

#if (!defined(DWORD)) && (!defined(WINAPI))
typedef dfuLong32 DWORD;
#endif

#ifndef LPDWORD
typedef DWORD *LPDWORD, *PDWORD;
#endif


#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#ifndef Round4
#define Round4(x) ((((x)+3)/4)*4)
#endif


typedef long (DFSCALLBACK *tCompareData) (LPCVOID lpElem1, LPCVOID lpElem2);
typedef BOOL(DFSCALLBACK *tDestructorData) (LPCVOID lpElem);

class STATIC_ARRAY
{
public:
  STATIC_ARRAY(dfuLong32 dwSizeElem = sizeof(LPVOID), dfuLong32 dwStepAlloc = 256, dfuLong32 dwFirstAlloc = 0);
  ~STATIC_ARRAY();
  dfuLong32 GetNbElemSA() const;
  BOOL InitStaticArray(dfuLong32 dwSizeElem = sizeof(LPVOID),
                       dfuLong32 dwStepAlloc = 256,
                       dfuLong32 dwFirstAlloc = 0);
  BOOL ReservAllocationSA(dfuLong32 dwNbReserv);
  BOOL DeleteElemSA(dfuLong32 dwPos, dfuLong32 dwNbElem = 1);
  BOOL InsertElemSA(dfuLong32 dwPos, LPCVOID lpData);
  BOOL SetElemSA(dfuLong32 dwPos, LPCVOID lpData);
  BOOL GetElemSA(dfuLong32 dwPos, LPVOID lpData) const;
  LPCVOID GetElemPtrSA(dfuLong32 dwPos) const;
  BOOL AddEndElemSA(LPCVOID lpData);
  BOOL SetFuncCompareDataSA(tCompareData fCompareDataSet, BOOL fForce=FALSE);
  BOOL SetFuncDestructorSA(tDestructorData fDestructorDataSet);
  BOOL InsertSortedSA(LPCVOID lpData);
  BOOL FindSameElemPosSA(LPCVOID lpDataElem, dfuLong32* lpdwPos) const;
private:
  BOOL SetNbAlloc(dfuLong32 dwNbAllocNew);
  BOOL CheckNbAlloc(dfuLong32 dwNbAllocNeeded);

  dfuLong32 dwNbUsed;
  dfuLong32 dwNbAlloc;
  dfuLong32 dwSizeElem;
  dfuLong32 dwStepAlloc;
  LPBYTE lpData;
  tCompareData fCompareData;
  tDestructorData fDestructorData;
};

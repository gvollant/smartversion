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

#ifdef __cplusplus
extern "C" {
#endif


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


#define DECLARE_LTHANDLE(name)    struct name##__ { int _name##_unused; }; \
                                  typedef const struct name##__ * name


DECLARE_LTHANDLE(STATIC_ARRAY_C);

typedef long (DFSCALLBACK *tCompareDataC) (const void* lpElem1, const void* lpElem2);
typedef BOOL(DFSCALLBACK *tDestructorDataC) (const void* lpElem);

  STATIC_ARRAY_C InitStaticArray_C(dfuLong32 dwSizeElem, dfuLong32 dwStepAllocNew);
  void DeleteStaticArray_C(STATIC_ARRAY_C sac);
  dfuLong32 GetNbElemSA(STATIC_ARRAY_C sac);
  BOOL InitStaticArray(STATIC_ARRAY_C sac,
                       dfuLong32 dwSizeElem,
                       dfuLong32 dwStepAlloc);
  BOOL ReservAllocationSA(STATIC_ARRAY_C sac,dfuLong32 dwNbReserv);
  BOOL DeleteElemSA(STATIC_ARRAY_C sac, dfuLong32 dwPos, dfuLong32 dwNbElem);
  BOOL InsertElemSA(STATIC_ARRAY_C sac, dfuLong32 dwPos, const void* lpData);
  BOOL SetElemSA(STATIC_ARRAY_C sac, dfuLong32 dwPos, const void* lpData);
  BOOL GetElemSA(STATIC_ARRAY_C sac, dfuLong32 dwPos, void* lpData) ;
  const void* GetElemPtrSA(STATIC_ARRAY_C sac, dfuLong32 dwPos) ;
  BOOL AddEndElemSA(STATIC_ARRAY_C sac, const void* lpData);
  BOOL SetFuncCompareDataSA(STATIC_ARRAY_C sac, tCompareDataC fCompareDataSet);
  BOOL SetFuncDestructorSA(STATIC_ARRAY_C sac, tDestructorDataC fDestructorDataSet);
  BOOL InsertSortedSA(STATIC_ARRAY_C sac, const void* lpData);
  BOOL FindSameElemPosSA(STATIC_ARRAY_C sac, const void* lpDataElem, dfuLong32* lpdwPos) ;


#ifdef __cplusplus
}
#endif


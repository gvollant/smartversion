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

/* DifsTool.h */
/* Basic tools for SVF engine, (disk, memory, date, charset, error management) */


/* 32 bits or more */
/* typedef unsigned long  dfuLong32Intel; */

/* DECLARE_HANDLE(dfuLong32Intel); */

#ifndef DIFSTOOL_H_INCLUDED
#define DIFSTOOL_H_INCLUDED


#if defined(__cplusplus) && (!defined(ALLINCPP))
extern "C" {
#endif


typedef void* hDifTimeElasped;

hDifTimeElasped SVFAPI SyncDifBuidTimeMarkerObject();
unsigned int SVFAPI SyncDifGetMSecElapsed(hDifTimeElasped ptr);
unsigned int SVFAPI SyncDifGetMSecElapsedNotDestructive(hDifTimeElasped ptr, int destructObject);

typedef void* SYNC_DIF_MUTEX_OBJECT;

SYNC_DIF_MUTEX_OBJECT SVFAPI SyncDifBuildMutex();
void SVFAPI SyncDifGetMutex(SYNC_DIF_MUTEX_OBJECT pMut);
void SVFAPI SyncDifReleaseMutex(SYNC_DIF_MUTEX_OBJECT pMut);
void SVFAPI SyncDifDeleteMutex(SYNC_DIF_MUTEX_OBJECT pMut);



void difstr_putValue_inmemory OF((void *dest, dfuLong32 x, dfuInt nbByte));
dfuLong32 difstr_getValue_inmemory OF((void *src, dfuInt nbByte));
dfuInt difstr_getNeededByte_forValue OF((dfuLong32 x));

dfuLong32 MyLogLong(dfuLong32 value);
dfuLong32 MyLogLongPtr(dfuIntPtr value);
dfuLong32 MyLogLong64(dfuLong64 value);

#if defined(_ZLIB_H) || defined (ZLIB_H) || defined (ZLIB_VERSION)
void ClearZStream OF((z_streamp strm));
#endif

dfuInt16Intel ConvertuInt16TouInt16Intel(dfuInt16 value);
dfuInt16 ConvertuInt16IntelTouInt16(dfuInt16Intel value);

dfuLong32Intel ConvertuLongToLongIntel OF((dfuLong32 value));
dfuLong32 ConvertuLongIntelToLong OF((dfuLong32Intel value));
dfuLong64Intel ConvertuLongToLongIntel64 OF((dfuLong64 value));
dfuLong64 ConvertuLongIntelToLong64 OF((dfuLong64Intel value));

dfuLong64 ConvertDualuLongIntelTouLong64(dfuLong32Intel valueLow,dfuLong32Intel valueHigh);
void ConvertuLong64ToDualuLongIntel(dfuLong64 value,dfuLong32Intel* pLow, dfuLong32Intel* pHigh);

dfuLong64 AdduLong64(dfuLong64 a,dfuLong64 b);
dfuLong64 SubuLong64(dfuLong64 a,dfuLong64 b);

#define dfmin(a,b) (((a) < (b)) ? (a) : (b))
#define dfmax(a,b) (((a) > (b)) ? (a) : (b))



dfuLong32 SVFAPI CalculateRatio(dfuLong64 dfValue, dfuLong64 dfTotal, dfuLong32 dfWidth);

dfwchar ConvertAnsiItemToUnicodeItem(char cItem);


void *DfsMalloc(size_t);
void DfsFree(void *);
void *DfsRealloc(void *, size_t);

BOOL CheckEmptyAlloc();

dfuLong32 DfsCrc32(dfuLong32 previousCrc,const void* ptr, dfuLong32 sizelen);
dfuLong32 DfsAdler32(dfuLong32 previousAdler,const void* ptr, dfuLong32 sizelen);

#ifdef REDEFINED_MEMFNC
void DfsMemcpy(void *ptrdest, const void *ptrsrc, dfuLong32 size);
void DfsMemmove(void *ptrdest, const void *ptrsrc, dfuLong32 size);
int DfsMemcmp(const void *ptrsrc1, const void *ptrsrc2, dfuLong32 size);
#else
#include <memory.h>
#include <string.h>

#define DfsMemcpy(dst,src,size) (memcpy((dst),(src),(size)))
#define DfsMemmove(dst,src,size) (memmove((dst),(src),(size)))
#define DfsMemcmp(ptr1,ptr2,size) ((int)(memcmp((ptr1),(ptr2),(size))))
#endif

void DfsClearStruct(void *ptrStruct, dfbyte fillbyte, size_t sizeStruct);
BOOL DfsCheckAllocatedMemory(dfvoidp * ptr, dfuLong32 * sizeAllocated,
                             dfuLong32 sizeNeeded, dfuLong32 step);

BOOL ConvertAnsiToUnicode(const char *lpszAnsi, dfwcharp lpszUnicode,
                          dfuLong32 dfUnicodeSize);
BOOL ConvertUnicodeToAnsi(dfwcharpc lpszUnicode, char *lpszAnsi,
                          dfuLong32 dfAnsiSize);

BOOL ConvertAnsiToUnicodeEx(const char *lpszAnsi, dfwcharp lpszUnicode,
                          dfuLong32 dfUnicodeSize,
                          BOOL fTransformSlashToOperatingSystemCompatibleSlash);
BOOL ConvertUnicodeToAnsiEx(dfwcharpc lpszUnicode, char *lpszAnsi,
                          dfuLong32 dfAnsiSize,
                          BOOL fTransformSlashToOperatingSystemCompatibleSlash);

BOOL IsUnicodeApiSupported();

BOOL ConvertNum64ToUnformattedStrAnsi(dfuLong64 dfNum,char* szAnsi, dfuLong32 dfBufSize,dfuLong32 dfMinimalTextSize);

BOOL ConvertNum64ToUnformattedStrUnicode(dfuLong64 dfNum,dfwchar* szUnicode, dfuLong32 dfBufSize,dfuLong32 dfMinimalTextSize);

dfuLong32 SVFAPI GetPhysicalMemoryKb();

#if defined(_TCHAR_DEFINED) && defined(UNICODE)
#define ConvertTCharToUnicode(tchar,unic,unicSize) \
             strncpy(unic,tchar,unicSize)
#define ConvertUnicodeToTChar(unic,tchar,tcharSize) \
             strncpy(tchar,unic,tcharSize)
#else
#define ConvertTCharToUnicode(tchar,unic,unicSize) \
             ConvertAnsiToUnicode(tchar,unic,unicSize)
#define ConvertUnicodeToTChar(unic,tchar,tcharSize) \
             ConvertUnicodeToAnsi(unic,tchar,tcharSize)
#endif

BOOL ConvertOemToUnicode(const char *lpszAnsi, dfwchar * lpszUnicode,
                          dfuLong32 dfUnicodeSize);

BOOL ConvertUnicodeToOem(const dfwchar * lpszUnicode, char *lpszAnsi,
                          dfuLong32 dfAnsiSize);

dfuLong32 SVFAPI dfUnicodeStrlen(dfwcharpc);

void SVFAPI DfUnicodeStrcpy(dfwcharp,dfwcharpc);
void SVFAPI DfUnicodeStrcat(dfwcharp,dfwcharpc);

int SVFAPI dfUnicodeStrcmpi(dfwcharpc str1,dfwcharpc str2);

int SVFAPI dfUnicodeStrcmp(dfwcharpc str1,dfwcharpc str2);

int SVFAPI CompareUnicodeAndAnsiString(dfwcharpc str1,const char* lpszAnsi,BOOL fCaseSensitive);

dfwcharpc SVFAPI GetUnicodeStringEmpty();
dfwcharpc SVFAPI GetUnicodeStringDirectorySeparator();
dfwcharpc SVFAPI GetUnicodeStringSlashSeparator();
dfwcharpc SVFAPI GetUnicodeStringAntiSlashSeparator();
dfwcharpc SVFAPI GetUnicodeStringDot();
dfwcharpc SVFAPI GetUnicodeStringStar();
dfwcharpc SVFAPI GetUnicodeStringDotDot();
dfwcharpc SVFAPI GetUnicodeSVFPrefix();
dfwcharpc SVFAPI GetUnicodeLowerCaseSvf();

void SVFAPI DispOutUnicodeString(dfwcharpc str);
long SVFAPI ConvertUnicodeStringToLong(dfwcharpc str);
BOOL SVFAPI CompareUnicodeWithSimpleChar(dfwchar wc,char c);

dfuLong32 SVFAPI CalculateRatio(dfuLong64 dfValue, dfuLong64 dfTotal, dfuLong32 dfPercent);

dfuLong32 SVFAPI dfUnicodeCopyConcat(dfwcharp lpszUnicodeDest,dfwcharpc lpszUnicodeSrc1,dfwcharpc lpszUnicodeSrc2);
dfwcharp SVFAPI dfUnicodeCopyConcatAlloc(dfwcharpc lpszUnicodeSrc1,dfwcharpc lpszUnicodeSrc2);
dfwcharp SVFAPI dfUnicodeCopyAlloc(dfwcharpc lpszUnicodeSrc);

dfuLong32 GetuLong64Low32(dfuLong64 a);
dfuLong32 GetuLong64High32(dfuLong64 a);

dfuLong64 MakeuLong64(dfuLong32 dfLow,dfuLong32 dfHigh);


void dfAdd64(dfuLong32 *dwLow,dfuLong32 *dwHigh,dfuLong32 dwAddLow,dfuLong32 dwAddHigh);
void dfMultiply64(dfuLong32 *dwLow,dfuLong32 *dwHigh,dfuLong32 dwLow2,dfuLong32 dwHigh2);
void dfSub64(dfuLong32 *dwLow,dfuLong32 *dwHigh,dfuLong32 dwSubLow,dfuLong32 dwSubHigh);
int  dfCompareValue64(dfuLong32 dwLow1,dfuLong32 dwHigh1,dfuLong32 dwLow2,dfuLong32 dwHigh2);


void dfDivide64(dfuLong32 *dwLow,dfuLong32 *dwHigh,dfuLong32 dwLow2,dfuLong32 dwHigh2);



void ConvertDfsInfoDateToDfsTm(const DFSINFODATE * dfsInfoDate,
                               DFSTM * dfsTm);

void ConvertDfsTmToDfsInfoDate(const DFSTM * dfsTm,
                               DFSINFODATE * dfsInfoDate);


void ConvertDfsCrcInfoParamToDfsCrcInfo(const DFSCRCINFOPARAM *
                                        pDfsCrcInfoParam,
                                        DFSCRCINFO_FULLSIZESTRUCTURE * pDfsCrcInfo);

BOOL ConvertDfsCrcInfoToDfsCrcInfoParam(const DFSCRCINFO_FULLSIZESTRUCTURE * pDfsCrcInfo,
                                        DFSCRCINFOPARAM * pDfsCrcInfoParam);

dfuLong32 GetNbCrcInfo(const DFSCRCINFO_FULLSIZESTRUCTURE *,dfuLong32 dfSizeBuf);
dfuLong32 GetCrcInfoSize(const DFSCRCINFO_FULLSIZESTRUCTURE *);
dfuLong32 GetCrcInfoSizeNeededToGetSize(const DFSCRCINFO_FULLSIZESTRUCTURE * pDfsCrcInfo);

int CompareDfsTm(const DFSTM * pDfsTm1,const DFSTM * pDfsTm2);

BOOL ConvertDfsFileProperties(/*DFSFILEPOSPROPERTIES6464* pDfsFileProperties,*/
                              DFSFILEPOSPROPERTIESINFO* pDfsFilePosPropertiesInfo,
                              dfvoidp TagBufProperties,dfuLong32 TagSizeProperties);

dfuLong32 ConvertDfsFilePropertiesToTag(const DFSFILEPOSPROPERTIESINFO* pDfsFilePosPropertiesInfo,
                                        dfvoidp TagBufProperties,dfuLong32 TagSizeProperties);


H_ERROR_INFO SVFAPI CreateErrorInfoBlock(dfuLong32 dfError,dfuLong32 dfSubError,dfuLong32 dfNbErrorItem,const ERROR_INFO_ITEM* pErrorInfoItem);
BOOL SVFAPI FreeErrorInfoBlock(H_ERROR_INFO hErrorInfo);

dfuLong32 SVFAPI GetErrorNumber(H_ERROR_INFO hErrorInfo);
dfuLong32 SVFAPI GetErrorSubNumber(H_ERROR_INFO hErrorInfo);
dfuLong32 SVFAPI GetNbErrorInfo(H_ERROR_INFO hErrorInfo);
BOOL SVFAPI GetErrorInfoItem(H_ERROR_INFO hErrorInfo,dfuLong32 dfNbItem,dfbytep* ptr,dfuLong32* pdfSize,dfuLong32* pdfInfoTag);
BOOL SVFAPI GetErrorInfoItemByTag(H_ERROR_INFO hErrorInfo,dfuLong32 dfTag,dfbytep* ptr,dfuLong32* pdfSize);

void DispMessageForDebugger(dfwcharpc lpszMessage);
void DispMessageForUser(dfwcharpc lpszMessageCaption,dfwcharpc lpszMessagePlain);


typedef struct
{
    union
    {
        dfbyte b[2];
        dfwchar wc;
    } u;
} DFWCHAR_BYTEACCESS;

#if defined(__cplusplus) && (!defined(ALLINCPP))
}
#endif

#endif

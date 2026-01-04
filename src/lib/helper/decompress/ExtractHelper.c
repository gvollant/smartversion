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


/*
*  ExtractHelper.c
*
*  Created by GillesVollant on 08/02/10.
*
*
*/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
//#include <conio.h>
#include <string.h>
//#include <windows.h>
#include "../../engine/patchstream/common/difbasic.h"
#include "../../../misc/svfxdll.h"
#include "../../../cli/DfsCdLin.h"

#include "../../engine/patchstream/common/ltoolsc.h"


#include "zlib.h"

#include "../../engine/patchstream/common/abstractDecompress.h"
#include "../../hash/svf_sha256.h"
#include "../../hash/svf_md5.h"
#include "../../hash/svf_sha.h"


#include "ExtractHelper.h"

#define mymin(a,b)            (((a) < (b)) ? (a) : (b))

#ifdef __cplusplus
extern "C" {
#endif

#if ((!defined(WIN32) && (!defined(_WIN32))))
	extern unsigned long dfPhysicalMemoryKBvalue;
	//extern char szTempDir[];
	BOOL SVFAPI SetTempDirectorySystemChar(const char* lpDirectory, dfuLong32 dfSizeBufferinwChar);
#endif

	void SVFAPI SetConst(const char* szTempDirSet, unsigned long dfPhysicalMemoryKBvalueSet)
	{
		//strcpy(szTempDir,szTempDirSet);
#if ((!defined(WIN32) && (!defined(_WIN32))))
		SetTempDirectorySystemChar(szTempDirSet, (dfuLong32)strlen(szTempDirSet) + 1);
		dfPhysicalMemoryKBvalue = dfPhysicalMemoryKBvalueSet;
#endif
	}

#ifdef __cplusplus
}
#endif





SYNC_DIF_MUTEX_OBJECT SyncDifMutexPreventConcurrentExtracting = NULL;



#define GET_MUTEX_PREVENT_CONCURRENT \
{ \
	if (SyncDifMutexPreventConcurrentExtracting!=NULL) \
	SyncDifGetMutex(SyncDifMutexPreventConcurrentExtracting); \
} ;

#define RELEASE_MUTEX_PREVENT_CONCURRENT \
{ \
	if (SyncDifMutexPreventConcurrentExtracting!=NULL) \
	SyncDifReleaseMutex(SyncDifMutexPreventConcurrentExtracting); \
} ;


static dfuLong32 DFSCALLBACK ConfirmBeforeCreatingFile(dfwcharpc dfFileName,
	dfvoidp dfpAdditionnalInfo,
	dfvoidp dfUserPtr)
{

	return CONFIRM_BEFORE_CREATING_FILE_OK;
}

/*
static dfuLong32 DFSCALLBACK fncExtractingFileWorkingEventConsole(dfuLong32 dfEvent,
const EXTR_WORK_EVENT_INFO* pEventPtr,
dfvoidp dfUserPtr)
{
if (dfEvent == EXTR_WORK_EVENT_BEFORE_EXTRACTING_FILE)
if (!pEventPtr->fTempFile)
{
printf("extracting : ");
DispOutUnicodeString(pEventPtr->dfFileNameOnDisk);
}

if (dfEvent == EXTR_WORK_EVENT_EXTRACTING_FILE_FINISHED)
if (!pEventPtr->fTempFile)
{
if (pEventPtr->dfSuccess==0)
printf(" Done\n");
else
printf(" Error %u\n",pEventPtr->dfSuccess);
}
return 0;
}
*/


typedef struct
{
	dfwcharpc FileNameOnDisk;
	dfwcharpc FileNameOnArchive;
	///dfuLong64 dfFileSizeUncompressed;
	BOOL      fTempFile;
	dfuLong32 dfPreviousVersion;
	BOOL      fIdenticalPreviousVersion;
	BOOL      fIgnore;
	BOOL      fForceDate;
	BOOL      fAddNewTag; /* to be used later */
	DFSINFODATE dfsInfoDate;
	dfvoidp   pReserved;
}
FILEUSERITEM;
typedef FILEUSERITEM *PFILEUSERITEM;

/*
static long DFSCALLBACK fncCompareFileUserItem(const void *lpElem1, const void *lpElem2)
{
const FILEUSERITEM *pfi1 = (const FILEUSERITEM *) lpElem1;
const FILEUSERITEM *pfi2 = (const FILEUSERITEM *) lpElem2;
return dfUnicodeStrcmpi(pfi1->FileNameOnArchive, pfi2->FileNameOnArchive);
}

static BOOL DFSCALLBACK fncDestructorFileUserItem(const void *lpElem)
{
FILEUSERITEM *pfi = (FILEUSERITEM *) lpElem;
if (pfi->FileNameOnArchive != NULL)
DfsFree((dfwcharp) pfi->FileNameOnArchive);
if (pfi->FileNameOnDisk != NULL)
DfsFree((dfwcharp) pfi->FileNameOnDisk);

return TRUE;
}


static BOOL ShowError(H_ERROR_INFO hei,BOOL fShowIsNull)
{
dfwcharp dfFileName=NULL;
dfwcharp dfErrorMsg=NULL;
dfuLong32 dfSizeFileName,dfSizeErrorMsg;

if ((!fShowIsNull) && (hei==NULL))
return FALSE;
printf("error detected :\n");

GetErrorInfoItemByTag(hei,DFS_ERRORTAG_FILENAME,(dfbytep*)(&dfFileName),&dfSizeFileName);
GetErrorInfoItemByTag(hei,DFS_ERRORTAG_ERRORMSG,(dfbytep*)(&dfErrorMsg),&dfSizeErrorMsg);

if (dfFileName!=NULL)
{
printf("filename : ");
DispOutUnicodeString(dfFileName);
printf("\n");
}


if (dfErrorMsg!=NULL)
{
printf("message : ");
DispOutUnicodeString(dfErrorMsg);
printf("\n");
}

return TRUE;
}
*/

static BOOL AddTextInBuf(char**szText, int* BufSize, const char* szTextAdd)
{
	int len = (int)strlen(szTextAdd);
	if ((len + 1) >= (*BufSize))
		return FALSE;
	strcpy(*szText, szTextAdd);
	(*szText) += len;
	(*BufSize) -= len;
	return TRUE;
}

static BOOL AddUnicTextInBuf(char**szText, int* BufSize, dfwcharp dfTextAdd)
{
	if (!ConvertUnicodeToAnsi(dfTextAdd, *szText, *BufSize))
		return FALSE;
	int len = (int)strlen(*szText);
	(*szText) += len;
	(*BufSize) -= len;
	return TRUE;
}


static BOOL GetErrorText(H_ERROR_INFO hei, char*szText, int BufSize)
{
	dfwcharp dfFileName = NULL;
	dfwcharp dfErrorMsg = NULL;
	dfuLong32 dfSizeFileName, dfSizeErrorMsg;
	BOOL fRet = TRUE;

	if (hei == NULL)
	{
		if ((szText != NULL) && (BufSize>0))
			*szText = 0;
		return TRUE;
	}
	if (BufSize<0x20)
		return FALSE;
	AddTextInBuf(&szText, &BufSize, "error detected : ");
	BufSize -= (int)strlen(szText);
	szText += strlen(szText);

	GetErrorInfoItemByTag(hei, DFS_ERRORTAG_FILENAME, (dfbytep*)(&dfFileName), &dfSizeFileName);
	GetErrorInfoItemByTag(hei, DFS_ERRORTAG_ERRORMSG, (dfbytep*)(&dfErrorMsg), &dfSizeErrorMsg);

	if (dfFileName != NULL)
	{
		AddTextInBuf(&szText, &BufSize, "filename : ");
		AddUnicTextInBuf(&szText, &BufSize, dfFileName);
		AddTextInBuf(&szText, &BufSize, "\n");
	}


	if (dfErrorMsg != NULL)
	{
		AddTextInBuf(&szText, &BufSize, "message : ");
		AddUnicTextInBuf(&szText, &BufSize, dfErrorMsg);
		AddTextInBuf(&szText, &BufSize, "\n");
	}

	return fRet;
}


COMPARE_RES SVFAPI ComputeHashFileReadingBufferEx2(dfwcharpc dfwFileNameToOpen, dfuLong64 dfExpectedSize, dfuLong64* p_dfExpectedSizeFill,
	BOOL fCompareCrc32, dfuLong32*p_dwCrc32,
	BOOL fCompareMD5, dfbytep bMD5, BOOL fCompareSHA1, dfbytep bSHA1, BOOL fCompareSHA256, dfbytep bSHA256,
	H_ERROR_INFO* pei)
{
	LOWLEVELFILE llFile;
	SVF_MD5_INTERNAL md5Ctx;
	SVF_SHA1_INTERNAL sha1Ctx;
	SVF_SHA256_INTERNAL sha256Ctx;
	COMPARE_RES CompareRes = COMPARE_RES_UNDEFINED;
	BOOL fGoodSize = FALSE;

	if (fCompareMD5)
		svf_md5_init(&md5Ctx);
	if (fCompareSHA1)
		svf_sha1_init(&sha1Ctx);
	if (fCompareSHA256)
		svf_sha256_init(&sha256Ctx);
	if (p_dwCrc32 != NULL)
		*p_dwCrc32 = 0;

	if (p_dfExpectedSizeFill != NULL)
		*p_dfExpectedSizeFill = 0;

	//pehaps use GetFileFullContentBuffer and GetMaxOrigDataSize
	llFile = OpenLowLevel(dfwFileNameToOpen, OPEN_READ, FALSE, FALSE, 0, pei);


	if (llFile != NULL)
	{
		dfuLong64 dfFileFoundSize;
		LowLevelGetSize64(llFile, &dfFileFoundSize);

		if (p_dfExpectedSizeFill != NULL)
		{
			*p_dfExpectedSizeFill = dfFileFoundSize;
			if (dfExpectedSize == 0)
				dfExpectedSize = dfFileFoundSize;
		}

		fGoodSize = (dfFileFoundSize == dfExpectedSize);
		if (!fGoodSize)
			CompareRes = COMPARE_RES_BADSIZE;
	}
	else CompareRes = COMPARE_RES_NOTFOUND;

	if (fGoodSize)
	{
		if (fCompareCrc32 || fCompareMD5 || fCompareSHA1)
		{
			dfuLong64 dwSizeToDo = dfExpectedSize;
			BOOL fProblemReading = FALSE;
			dfuLong32 dwSizeBuffer = (dfuLong32)(mymin(0x10000, dwSizeToDo + 1));

			void* ptr = DfsMalloc(dwSizeBuffer);

			if ((llFile != NULL) && (ptr != NULL))
			{
				while (dwSizeToDo>0)
				{
					dfuLong32 dwDone = 0;
					dfuLong32 dwToDoThis = (dfuLong32)(mymin(dwSizeToDo, dwSizeBuffer));
					//if (!ReadFile(hFile,ptr,mymin(dwSizeToDo,dwSizeBuffer),&dwDone,NULL))
					dwDone = LowLevelRead(llFile, ptr, dwToDoThis, pei);
					if (dwDone != dwToDoThis)
					{
						fProblemReading = TRUE;
						CompareRes = COMPARE_RES_PROBLEMREAD;
						break;
					}
					dwSizeToDo -= dwToDoThis;
					if (fCompareCrc32)
						if (p_dwCrc32 != NULL)
							*p_dwCrc32 = (dfuLong32)crc32(*p_dwCrc32, (const unsigned char*)ptr, dwDone);
					if (fCompareMD5)
						svf_md5_append(&md5Ctx, (const unsigned char*)ptr, dwDone);
					if (fCompareSHA1)
						svf_sha1_append(&sha1Ctx, (const unsigned char*)ptr, dwDone);
					if (fCompareSHA256)
						svf_sha256_append(&sha256Ctx, (const unsigned char*)ptr, dwDone);
				}
			}
			else
			{
				CompareRes = COMPARE_RES_PROBLEMREAD;
				fProblemReading = TRUE;
			}

			if (!fProblemReading)
				CompareRes = COMPARE_RES_GOOD;

			if (fCompareMD5)
				svf_md5_finish(&md5Ctx, bMD5);

			if (fCompareSHA1)
				svf_sha1_finish(&sha1Ctx, bSHA1);

			if (fCompareSHA256)
				svf_sha256_finish(&sha256Ctx, bSHA256);

			if (ptr != NULL)
				DfsFree(ptr);
		}
		else
		{
			CompareRes = COMPARE_RES_GOOD;
		}
	}

	if (llFile != NULL)
		LowLevelClose(llFile, pei);
	return CompareRes;
}

COMPARE_RES SVFAPI ComputeHashFileReadingBufferEx(dfwcharpc dfwFileNameToOpen, dfuLong64 dfExpectedSize, dfuLong64* p_dfExpectedSizeFill,
	BOOL fCompareCrc32, dfuLong32*p_dwCrc32,
	BOOL fCompareMD5, dfbytep bMD5, BOOL fCompareSHA1, dfbytep bSHA1,
	H_ERROR_INFO* pei)
{
	return ComputeHashFileReadingBufferEx2(dfwFileNameToOpen, dfExpectedSize, p_dfExpectedSizeFill,
		fCompareCrc32, p_dwCrc32,
		fCompareMD5, bMD5, fCompareSHA1, bSHA1, FALSE, NULL, pei);
}

COMPARE_RES SVFAPI ComputeHashFileReadingBuffer(dfwcharpc dfwFileNameToOpen, dfuLong64 dfExpectedSize,
	BOOL fCompareCrc32, dfuLong32*p_dwCrc32,
	BOOL fCompareMD5, dfbytep bMD5, BOOL fCompareSHA1, dfbytep bSHA1,
	H_ERROR_INFO* pei)
{
	return ComputeHashFileReadingBufferEx(dfwFileNameToOpen, dfExpectedSize, NULL,
		fCompareCrc32, p_dwCrc32,
		fCompareMD5, bMD5, fCompareSHA1, bSHA1,
		pei);
}

int SVFAPI DoComputeHashFile(const char*filename, dfuLong64 dfExpectedSize,
	BOOL fCompareCrc32, dfuLong32*p_dwCrc32,
	BOOL fCompareMD5, dfbytep bMD5, BOOL fCompareSHA1, dfbytep bSHA1,
	char* errBufTxt, int errBufSize)
{

	dfwchar dfwFileNameToOpen[MAX_PATH_LENGTH];
	H_ERROR_INFO hei = NULL;
	ConvertAnsiToUnicode(filename, dfwFileNameToOpen, MAX_PATH_LENGTH);

	if ((errBufTxt != NULL) && (errBufSize>0))
		*errBufTxt = 0;

	int ret = (int)ComputeHashFile(dfwFileNameToOpen, dfExpectedSize,
		fCompareCrc32, p_dwCrc32,
		fCompareMD5, bMD5, fCompareSHA1, bSHA1, &hei);


	if (hei != NULL)
	{
		GetErrorText(hei, errBufTxt, errBufSize);
	}
	FreeErrorInfoBlock(hei);
	hei = NULL;
	return ret;
}

int SVFAPI DoComputeHashFileEx(const char*filename, dfuLong64 *p_dfSizeFill,
	BOOL fCompareCrc32, dfuLong32*p_dwCrc32,
	BOOL fCompareMD5, dfbytep bMD5, BOOL fCompareSHA1, dfbytep bSHA1,
	char* errBufTxt, int errBufSize)
{
	dfwchar dfwFileNameToOpen[MAX_PATH_LENGTH];
	H_ERROR_INFO hei = NULL;
	ConvertAnsiToUnicode(filename, dfwFileNameToOpen, MAX_PATH_LENGTH);

	if ((errBufTxt != NULL) && (errBufSize>0))
		*errBufTxt = 0;

	int ret = (int)ComputeHashFileEx(dfwFileNameToOpen, 0, p_dfSizeFill,
		fCompareCrc32, p_dwCrc32,
		fCompareMD5, bMD5, fCompareSHA1, bSHA1, &hei);


	if (hei != NULL)
	{
		GetErrorText(hei, errBufTxt, errBufSize);
	}
	FreeErrorInfoBlock(hei);
	hei = NULL;
	return ret;
}

COMPARE_RES SVFAPI ComputeHashFileEx2(dfwcharpc dfwFileNameToOpen, dfuLong64 dfExpectedSize, dfuLong64* p_dfExpectedSizeFill,
	BOOL fCompareCrc32, dfuLong32*p_dwCrc32,
	BOOL fCompareMD5, dfbytep bMD5, BOOL fCompareSHA1, dfbytep bSHA1, BOOL fCompareSHA256, dfbytep bSHA256,
	H_ERROR_INFO* pei)
{
	//LOWLEVELFILE llFile;
	SVF_MD5_INTERNAL md5Ctx;
	SVF_SHA1_INTERNAL sha1Ctx;
	SVF_SHA256_INTERNAL sha256Ctx;
	COMPARE_RES CompareRes;
	ORIGDATA org;
	HFILECONTENTREADBUF hFileContentReadBuf = NULL;

	if (p_dwCrc32 != NULL)
		*p_dwCrc32 = 0;

	if (p_dfExpectedSizeFill != NULL)
		*p_dfExpectedSizeFill = 0;

	//pehaps use GetFileFullContentBuffer and GetMaxOrigDataSize
	//llFile = OpenLowLevel(dfwFileNameToOpen,OPEN_READ,FALSE,FALSE,0,pei);

	if (!GetFileFullContentBuffer(dfwFileNameToOpen, FILEFULLCONTENTBUFFER_READ, 0,
		&hFileContentReadBuf,
		&org, pei))
	{
		CompareRes = COMPARE_RES_NOTFOUND;
	}
	else
	{
		dfuLong64 dfFileFoundSize = GetMaxOrigDataSize(&org);
		if (p_dfExpectedSizeFill != NULL)
		{
			*p_dfExpectedSizeFill = dfFileFoundSize;
			if (dfExpectedSize == 0)
				dfExpectedSize = dfFileFoundSize;
		}

		if (dfFileFoundSize != dfExpectedSize)
			CompareRes = COMPARE_RES_BADSIZE;
		else
		{
			CompareRes = COMPARE_RES_GOOD;
			if (fCompareCrc32 || fCompareMD5 || fCompareSHA1)
			{
				dfuLong64 dwSizeToDo = dfExpectedSize;
				dfuLong64 dfCurPos = 0;
				dfuLong32 dwSizeBuffer = (dfuLong32)(mymin(0x1000, dwSizeToDo + 1));


				if (fCompareMD5)
					svf_md5_init(&md5Ctx);
				if (fCompareSHA1)
					svf_sha1_init(&sha1Ctx);
				if (fCompareSHA256)
					svf_sha256_init(&sha256Ctx);

				{
					while (dwSizeToDo>0)
					{
						dfuLong32 dwToDoThis = (dfuLong32)(mymin(dwSizeToDo, dwSizeBuffer));
						const unsigned char* ptr = ((const unsigned char*)GetOrigDataPtrpDataBySizeView(&org, dfCurPos, dwToDoThis)) + dfCurPos;

						dwSizeToDo -= dwToDoThis;
						dfCurPos += dwToDoThis;
						if (fCompareCrc32)
							if (p_dwCrc32 != NULL)
								*p_dwCrc32 = (dfuLong32)crc32(*p_dwCrc32, (const unsigned char*)ptr, dwToDoThis);
						if (fCompareMD5)
							svf_md5_append(&md5Ctx, (const unsigned char*)ptr, dwToDoThis);
						if (fCompareSHA1)
							svf_sha1_append(&sha1Ctx, (const unsigned char*)ptr, dwToDoThis);
						if (fCompareSHA256)
							svf_sha256_append(&sha256Ctx, (const unsigned char*)ptr, dwToDoThis);
					}
				}

				CompareRes = COMPARE_RES_GOOD;

				if (fCompareMD5)
					svf_md5_finish(&md5Ctx, bMD5);

				if (fCompareSHA1)
					svf_sha1_finish(&sha1Ctx, bSHA1);

				if (fCompareSHA256)
					svf_sha256_finish(&sha256Ctx, bSHA256);
			}
		}
		CloseFileFullContentBuffer(hFileContentReadBuf);
	}

	return CompareRes;
}


COMPARE_RES SVFAPI ComputeHashFileEx(dfwcharpc dfwFileNameToOpen, dfuLong64 dfExpectedSize, dfuLong64* p_dfExpectedSizeFill,
	BOOL fCompareCrc32, dfuLong32*p_dwCrc32,
	BOOL fCompareMD5, dfbytep bMD5, BOOL fCompareSHA1, dfbytep bSHA1,
	H_ERROR_INFO* pei)
{
	return ComputeHashFileEx2(dfwFileNameToOpen, dfExpectedSize, p_dfExpectedSizeFill,
		fCompareCrc32, p_dwCrc32,
		fCompareMD5, bMD5, fCompareSHA1, bSHA1, FALSE, NULL, pei);
}

COMPARE_RES SVFAPI CompareTwoFile(dfwcharpc dfwFileName1, dfwcharpc dfwFileName2,
	H_ERROR_INFO* pei)
{
	COMPARE_RES CompareRes;
	ORIGDATA org1,org2;
	HFILECONTENTREADBUF hFileContentReadBuf1 = NULL;
	HFILECONTENTREADBUF hFileContentReadBuf2 = NULL;


	if (!GetFileFullContentBuffer(dfwFileName1, FILEFULLCONTENTBUFFER_READ, 0,
		&hFileContentReadBuf1,
		&org1, pei))
	{
		CompareRes = COMPARE_RES_NOTFOUND;
	}
	else
		if (!GetFileFullContentBuffer(dfwFileName2, FILEFULLCONTENTBUFFER_READ, 0,
		&hFileContentReadBuf2,
		&org2, pei))
	{
		CompareRes = COMPARE_RES_NOTFOUND;
	}
	else
	{
		dfuLong64 dfFileFoundSize1 = GetMaxOrigDataSize(&org1);
		dfuLong64 dfFileFoundSize2 = GetMaxOrigDataSize(&org2);



		if (dfFileFoundSize1 != dfFileFoundSize2)
			CompareRes = COMPARE_RES_BADSIZE;
		else
		{
			CompareRes = COMPARE_RES_GOOD;

			{
				dfuLong64 dwSizeToDo = dfFileFoundSize1;
				dfuLong64 dfCurPos = 0;
				dfuLong32 dwSizeBuffer = (dfuLong32)(mymin(0x1000, dwSizeToDo + 1));

				{
					while (dwSizeToDo>0)
					{
						dfuLong32 dwToDoThis = (dfuLong32)(mymin(dwSizeToDo, dwSizeBuffer));
						const unsigned char* ptr1 = ((const unsigned char*)GetOrigDataPtrpDataBySizeView(&org1, dfCurPos, dwToDoThis)) + dfCurPos;
						const unsigned char* ptr2 = ((const unsigned char*)GetOrigDataPtrpDataBySizeView(&org2, dfCurPos, dwToDoThis)) + dfCurPos;

						dwSizeToDo -= dwToDoThis;
						dfCurPos += dwToDoThis;
						if (memcmp(ptr1, ptr2, dwToDoThis) != 0)
						{
							CompareRes = COMPARE_RES_BADCRC;
							break;
						}

					}
				}
			}
		}
		CloseFileFullContentBuffer(hFileContentReadBuf1);
		CloseFileFullContentBuffer(hFileContentReadBuf2);
	}

	return CompareRes;
}

COMPARE_RES SVFAPI ComputeHashFile(dfwcharpc dfwFileNameToOpen, dfuLong64 dfExpectedSize,
	BOOL fCompareCrc32, dfuLong32*p_dwCrc32,
	BOOL fCompareMD5, dfbytep bMD5, BOOL fCompareSHA1, dfbytep bSHA1,
	H_ERROR_INFO* pei)
{
	return ComputeHashFileEx(dfwFileNameToOpen, dfExpectedSize, NULL,
		fCompareCrc32, p_dwCrc32,
		fCompareMD5, bMD5, fCompareSHA1, bSHA1,
		pei);
}

const char* GetErrorMomentText(ERROR_MOMENT err_moment)
{
	switch (err_moment)
	{
	case ERROR_MOMENT_NO_ERROR: return "";
	case ERROR_MOMENT_OPEN_DFS: return "Error when opening .svf file";
	case ERROR_MOMENT_READ_DFS: return "Error on reading .svf file";
	case ERROR_MOMENT_INTERPRET_DFS: return "Error on .svf content file";
	case ERROR_MOMENT_READ_SOURCE: return "Error on reading source file";
	case ERROR_MOMENT_COMPARE_SOURCE: return "Mismatch source file error";
	case ERROR_MOMENT_EXTRACTING: return "Error on Extracting patch";
	}
	return "";
}

const char* GetErrorExplanation(dfuLong32 dfErr)
{
	switch (dfErr)
	{
	case DFS_ERROR_BAD_PARAMETER: return "Bad parameter";
	case DFS_ERROR_ERRORIO: return "IO Error";
	case DFS_ERROR_MEMORY_ERROR: return "Memory error";
	case DFS_ERROR_BAD_CHECKSUM: return "Bad checksum";
	case DFS_ERROR_FILE_NOT_FOUND: return "File not found";
	case DFS_SUCCESS: return "";
	case DFS_STOP_REQUESTED: return "Stop requested by user";
	}
	return "";
}

int SVFAPI ApplyMonofilePatchMemory(const char* base, int base_only_dir,
	const void*patch_buffer, size_t patch_size,
	const char*dest,
	ERROR_MOMENT *perr_moment,
	int* perrinfo, char* errBufTxt, int errBufSize)
{
	return ApplyMonofilePatchMemoryEx2(base, base_only_dir,
		patch_buffer, patch_size,
		dest,
		perr_moment,
		perrinfo, errBufTxt, errBufSize, FALSE,
		NULL, NULL, 0, 0);
}


int SVFAPI ApplyMonofilePatchMemoryEx(const char* base, int base_only_dir,
	const void*patch_buffer, size_t patch_size,
	const char*dest,
	ERROR_MOMENT *perr_moment,
	int* perrinfo, char* errBufTxt, int errBufSize, dfuLong32 dfExtractingMethod)
{
	return ApplyMonofilePatchMemoryEx2(base, base_only_dir,
		patch_buffer, patch_size,
		dest,
		perr_moment,
		perrinfo, errBufTxt, errBufSize, dfExtractingMethod,
		NULL, NULL, 0, 0);
}

static void BuildTempMemFileNameForMemoryPatch(
	char* szaPatchMemFileName, dfwcharp szwDfsFileName, dfuLong32 dfUnicodeBufferSize, const void*patch_buffer, size_t patch_size)
{
	sprintf(szaPatchMemFileName, "*memPatch_%08lx%08lx_%08lx",
		((unsigned long)((((size_t)patch_buffer) >> 16) >> 16)),
		((unsigned long)((((size_t)patch_buffer)))),
		((unsigned long)((((size_t)patch_size)))));
	ConvertAnsiToUnicode(szaPatchMemFileName, szwDfsFileName, dfUnicodeBufferSize);
}


static void FillErrorInfo(H_ERROR_INFO *hei, char* errBufTxt, int errBufSize)
{
	if ((*hei) != NULL)
	{
		ERROR_MOMENT err_moment = ERROR_MOMENT_OPEN_DFS;
		{
			AddTextInBuf(&errBufTxt, &errBufSize, GetErrorMomentText(err_moment));
			AddTextInBuf(&errBufTxt, &errBufSize, ", ");
		}
		GetErrorText((*hei), errBufTxt, errBufSize);
		FreeErrorInfoBlock((*hei));
		*hei = NULL;
	}
}

int SVFAPI ApplyMonofilePatchMemoryEx2(const char* base, int base_only_dir,
	const void*patch_buffer, size_t patch_size,
	const char*dest,
	ERROR_MOMENT *perr_moment,
	int* perrinfo, char* errBufTxt, int errBufSize, dfuLong32 dfExtractingMethod,
	tSetExtractPosCallBack pSetExtractPosCallBack, dfvoidp dfUserPtr,
	dfuLong32 dwMinProgress, dfuLong32 dwMaxProgress)
{
	LOWLEVELFILE llf;
	H_ERROR_INFO hei = NULL;
	int ret = 0;
	char szaPatchMemFileName[MAX_PATH_LENGTH + 1];
	dfwchar szwDfsFileName[MAX_PATH_LENGTH + 1];


	BuildTempMemFileNameForMemoryPatch(szaPatchMemFileName, szwDfsFileName, MAX_PATH_LENGTH, patch_buffer, patch_size);

	llf = memOpenOrBuildPermanentLowLevel(szwDfsFileName, OPEN_CREATE, FALSE, TRUE, (dfuLong64)patch_size, patch_buffer, &hei);
	if (llf != NULL)
	{
		LowLevelClose(llf, &hei);
		ret = ApplyMonofilePatchEx2(base, base_only_dir,
			szaPatchMemFileName, dest,
			perr_moment,
			perrinfo, errBufTxt, errBufSize, dfExtractingMethod,
			pSetExtractPosCallBack, dfUserPtr,
			dwMinProgress, dwMaxProgress);
		memDeleteFile(szwDfsFileName, &hei);
	}

	FillErrorInfo(&hei, errBufTxt, errBufSize);

	return ret;
}



int SVFAPI ApplyMultiFilePatchMemoryEx3(const char* base, int base_only_dir,
                       const void*patch_buffer, size_t patch_size, const char*dest, int onlyMonoFileBase,
                       ERROR_MOMENT *perr_moment,
                       int* perrinfo, char* errBufTxt, int errBufSize, dfuLong32 dfExtractingMethod,
                       dfuLong32 dfBaseVersionSpecified, dfuLong32 dfVersionSpecified,
                       tSetExtractPosCallBack pSetExtractPosCallBack, dfvoidp dfUserPtr,
                       dfuLong32 dwMinProgress, dfuLong32 dwMaxProgress)
{
	LOWLEVELFILE llf;
	H_ERROR_INFO hei = NULL;
	int ret = 0;
	char szaPatchMemFileName[MAX_PATH_LENGTH + 1];
	dfwchar szwDfsFileName[MAX_PATH_LENGTH + 1];


	BuildTempMemFileNameForMemoryPatch(szaPatchMemFileName, szwDfsFileName, MAX_PATH_LENGTH, patch_buffer, patch_size);

	llf = memOpenOrBuildPermanentLowLevel(szwDfsFileName, OPEN_CREATE, FALSE, TRUE, (dfuLong64)patch_size, patch_buffer, &hei);
	if (llf != NULL)
	{
		LowLevelClose(llf, &hei);
		ret = ApplyMultiFilePatchEx3(base, base_only_dir,
			szaPatchMemFileName, dest, onlyMonoFileBase,
			perr_moment,
			perrinfo, errBufTxt, errBufSize, dfExtractingMethod,
			dfBaseVersionSpecified, dfVersionSpecified,
			pSetExtractPosCallBack, dfUserPtr,
			dwMinProgress, dwMaxProgress);
		memDeleteFile(szwDfsFileName, &hei);
	}

	FillErrorInfo(&hei, errBufTxt, errBufSize);

	return ret;
}

int SVFAPI ApplyMonofilePatchMemoryToMemoryEx2(const char* base, int base_only_dir,
	const void*patch_buffer, size_t patch_size,
	const char*destfilename, void** destBuffer, size_t *dest_size,
	tCustomMallocCallBack pCustomMallocCallBack, dfvoidp dfMallocUserPtr,
	ERROR_MOMENT *perr_moment,
	int* perrinfo, char* errBufTxt, int errBufSize, dfuLong32 dfExtractingMethod,
	dfuLong32 dfBaseVersionSpecified, dfuLong32 dfVersionSpecified,
	tSetExtractPosCallBack pSetExtractPosCallBack, dfvoidp dfUserPtr,
	dfuLong32 dwMinProgress, dfuLong32 dwMaxProgress)
{
	LOWLEVELFILE llf;
	H_ERROR_INFO hei = NULL;
	int ret = 0;
	char szaPatchMemFileName[MAX_PATH_LENGTH + 1];
	char szaPatchMemDestDirectory[MAX_PATH_LENGTH + 0x100 + 1];

	dfwchar szwDfsFileName[MAX_PATH_LENGTH + 1];


	BuildTempMemFileNameForMemoryPatch(szaPatchMemFileName, szwDfsFileName, MAX_PATH_LENGTH, patch_buffer, patch_size);
	BuildTempMemFileNameForMemoryPatch(szaPatchMemDestDirectory, szwDfsFileName, MAX_PATH_LENGTH, patch_buffer, patch_size);
	strcat(szaPatchMemDestDirectory, "_dest/");

	llf = memOpenOrBuildPermanentLowLevel(szwDfsFileName, OPEN_CREATE, FALSE, TRUE, (dfuLong64)patch_size, patch_buffer, &hei);
	if (llf != NULL)
	{
		LowLevelClose(llf, &hei);/*
		ret = ApplyMonofilePatchEx2(base, base_only_dir,
			szaPatchMemFileName, szaPatchMemDestDirectory,
			perr_moment,
			perrinfo, errBufTxt, errBufSize, dfExtractingMethod,
			pSetExtractPosCallBack, dfUserPtr,
			dwMinProgress, dwMaxProgress);
			*/
		ret = ApplyMultiFilePatchEx3(base, base_only_dir,
			szaPatchMemFileName, szaPatchMemDestDirectory, 1,
			perr_moment,
			perrinfo, errBufTxt, errBufSize, dfExtractingMethod,
			dfBaseVersionSpecified, dfVersionSpecified,
			pSetExtractPosCallBack, dfUserPtr,
			dwMinProgress, dwMaxProgress);
		memDeleteFile(szwDfsFileName, &hei);
	}


	strcat(szaPatchMemDestDirectory, destfilename);
	dfwchar szwPatchMemDestDirectory[MAX_PATH_LENGTH + 0x100 + 1];
	ConvertAnsiToUnicode(szaPatchMemDestDirectory, szwPatchMemDestDirectory, MAX_PATH_LENGTH + 0x100);



	ORIGDATA org;
	HFILECONTENTREADBUF hFileContentReadBuf = NULL;


	if (GetFileFullContentBuffer(szwPatchMemDestDirectory, FILEFULLCONTENTBUFFER_READ, 0,
		&hFileContentReadBuf,
		&org, &hei))

	{
		dfuLong64 dfFileFoundSize = GetMaxOrigDataSize(&org);
		size_t return_size = (size_t)dfFileFoundSize;

		void* dataBuf = NULL;
		if ((return_size == dfFileFoundSize) && (destBuffer != NULL)) {
            if (pCustomMallocCallBack != NULL) {
				dataBuf = pCustomMallocCallBack(return_size, dfMallocUserPtr);
			}
			else
			{
			    dataBuf = (void*)malloc(return_size);
			}
		}
		if (dataBuf != NULL)
		{
			*destBuffer = dataBuf;
			if (dest_size != NULL)
			{
				*dest_size = return_size;
			}

			{
				dfuLong64 dwSizeToDo = dfFileFoundSize;
				dfuLong32 dwSizeBuffer = (dfuLong32)(mymin(0x1000, dwSizeToDo + 1));
				size_t curPos = 0;


				{
					while (dwSizeToDo>0)
					{
						dfuLong32 dwToDoThis = (dfuLong32)(mymin(dwSizeToDo, dwSizeBuffer));
						const unsigned char* ptr = ((const unsigned char*)GetOrigDataPtrpDataBySizeView(&org, curPos, dwToDoThis)) + curPos;
						memcpy((((char*)dataBuf) + curPos), ptr, dwToDoThis);
						curPos += dwToDoThis;
						dwSizeToDo -= dwToDoThis;
					}
				}

			}
		}
		CloseFileFullContentBuffer(hFileContentReadBuf);
	}

	memDeleteFile(szwPatchMemDestDirectory, &hei);
	if (hei != NULL)
	{
		ERROR_MOMENT err_moment = ERROR_MOMENT_OPEN_DFS;
		{
			AddTextInBuf(&errBufTxt, &errBufSize, GetErrorMomentText(err_moment));
			AddTextInBuf(&errBufTxt, &errBufSize, ", ");
		}
		GetErrorText(hei, errBufTxt, errBufSize);
		FreeErrorInfoBlock(hei);
		hei = NULL;
	}

	return ret;
}

int SVFAPI ApplyMonofilePatchMemoryToMemoryEx(const char* base, int base_only_dir,
	const void*patch_buffer, size_t patch_size,
	const char*destfilename, void** destBuffer, size_t *dest_size,
	ERROR_MOMENT *perr_moment,
	int* perrinfo, char* errBufTxt, int errBufSize, dfuLong32 dfExtractingMethod,
	dfuLong32 dfBaseVersionSpecified, dfuLong32 dfVersionSpecified,
	tSetExtractPosCallBack pSetExtractPosCallBack, dfvoidp dfUserPtr,
	dfuLong32 dwMinProgress, dfuLong32 dwMaxProgress)
{
    return ApplyMonofilePatchMemoryToMemoryEx2(base, base_only_dir,
		patch_buffer, patch_size,
		destfilename, destBuffer, dest_size,
		NULL, NULL,
		perr_moment,
		perrinfo, errBufTxt, errBufSize, dfExtractingMethod,
		dfBaseVersionSpecified, dfVersionSpecified,
		pSetExtractPosCallBack, dfUserPtr,
        dwMinProgress, dwMaxProgress);
}

int SVFAPI ApplyMonofilePatchMemoryToMemory(const char* base, int base_only_dir,
	const void*patch_buffer, size_t patch_size,
	const char*destfilename, void** destBuffer, size_t *dest_size,
	ERROR_MOMENT *perr_moment,
	int* perrinfo, char* errBufTxt, int errBufSize, dfuLong32 dfExtractingMethod,
	tSetExtractPosCallBack pSetExtractPosCallBack, dfvoidp dfUserPtr,
	dfuLong32 dwMinProgress, dfuLong32 dwMaxProgress)
{
	return ApplyMonofilePatchMemoryToMemoryEx(base, base_only_dir,
		patch_buffer, patch_size,
		destfilename, destBuffer, dest_size,
		perr_moment,
		perrinfo, errBufTxt, errBufSize, dfExtractingMethod,
		VALUE_UNKNOWN, VALUE_UNKNOWN,
		pSetExtractPosCallBack, dfUserPtr,
		dwMinProgress, dwMaxProgress);
}

int SVFAPI ApplyMonofilePatch(const char* base, int base_only_dir,
	const char*patch, const char*dest,
	ERROR_MOMENT *perr_moment,
	int* perrinfo, char* errBufTxt, int errBufSize)
{
	return ApplyMonofilePatchEx(base, base_only_dir,
		patch, dest,
		perr_moment,
		perrinfo, errBufTxt, errBufSize, FALSE);
}


int SVFAPI ApplyMultiFilePatchEx3(const char* base, int base_only_dir,
	const char*patch, const char*dest, int onlyMonoFileBase,
	ERROR_MOMENT *perr_moment,
	int* perrinfo, char* errBufTxt, int errBufSize, dfuLong32 dfExtractingMethod,
	dfuLong32 dfBaseVersionSpecified,dfuLong32 dfVersionSpecified,
	tSetExtractPosCallBack pSetExtractPosCallBack, dfvoidp dfUserPtr,
	dfuLong32 dwMinProgress, dfuLong32 dwMaxProgress)
{
	DFSFILE DfsFile = NULL;
	DFSFILEINFOPARAM DfsFileParam;
	H_ERROR_INFO hei = NULL;
	ERROR_MOMENT err_moment = ERROR_MOMENT_NO_ERROR;
	int ret = 1;
	dfuLong32 errinfo = DFS_SUCCESS;
	BOOL fReadHashContent = (dfExtractingMethod & EXTRACTING_METHOD_NO_CHECK_HASH_ORG_CONTENT) == 0;
	BOOL fFlushWrite = FALSE;
	DFSFEATUREPARAM DfsFeatureParam;

	GET_MUTEX_PREVENT_CONCURRENT


		if ((errBufTxt != NULL) && (errBufSize>0))
			*errBufTxt = 0;


	dfwchar szDfsFileName[MAX_PATH_LENGTH];
	dfwchar szDfsBaseDirectory[MAX_PATH_LENGTH];
	dfwchar szDfsBaseDirectoryExtr[MAX_PATH_LENGTH];


	STATIC_ARRAY_C saParam;
	saParam = InitStaticArray_C(sizeof(FILEUSERITEM), 0x80);
	SetFuncCompareDataSA(saParam,fncCompareFileItem);
	SetFuncDestructorSA(saParam,fncDestructorFileItem);


	DfsFeatureParam.fComputeMd5 = TRUE;
	DfsFeatureParam.fComputeSha1 = FALSE;
	DfsFeatureParam.fComputeSha256 = FALSE;
	DfsFeatureParam.fStripIdenticalBody = TRUE;

	DfsFileParam.sizeStruct = sizeof(DfsFileParam);
	DfsFileParam.dfStatus = DFS_READABLE;

	ConvertAnsiToUnicode(patch, szDfsFileName, MAX_PATH_LENGTH);
	ConvertAnsiToUnicode(dest, szDfsBaseDirectoryExtr, MAX_PATH_LENGTH);
	char basedir[MAX_PATH_LENGTH];
	strcpy(basedir, base);

	if (base_only_dir != 0)
	{
		int lastsep = 0;
		int i = 0;
		while ((*(basedir + i)) != 0)
		{
			if ((*(basedir + i) == '/') || (*(basedir + i) == '\\'))
				lastsep = i;
			i++;
		}
		if (lastsep != 0)
			(*(basedir + lastsep)) = 0;
	}
	ConvertAnsiToUnicode(basedir, szDfsBaseDirectory, MAX_PATH_LENGTH);

	DfsFileParam.filename = szDfsFileName;

	{
		dfuLong32 dfOpen = DfsFileOpen(&DfsFileParam, &DfsFile, &hei);
		if (dfOpen != DFS_SUCCESS)
			ret = 0;

		if (((errinfo == DFS_SUCCESS) || (DfsFile == NULL)) && (dfOpen != DFS_SUCCESS))
			errinfo = dfOpen;
		if ((err_moment == ERROR_MOMENT_NO_ERROR) && (((DfsFile == NULL)) && (dfOpen != DFS_SUCCESS)))
			err_moment = ERROR_MOMENT_OPEN_DFS;
	}
	/*
	if (DfsFile == NULL)
	{
	printf("Can't open ");
	DispOutUnicodeString(szDfsFileName);
	printf("\n");
	}
	else {
	printf("patch opened\n");
	}*/

	ConvertOldDirectoryCommentStorage(DfsFile, NULL);
	SetDfsFeatureParam(DfsFile, &DfsFeatureParam);

	//printf("'%s' + '%s' -> '%s'\n",base,patch,dest);

	if (DfsFile != NULL)
	{
		BOOL fRetDoMulti;
		PDIRINFO* pDirInfo;
		PCDIRINFO pCurDirInfo;
		EXTRACTINGMAPITEM* lpfExtractItemMap = NULL;
		dfuLong32 dfNbDir, dfDirExtract;
		dfwcharpc wchBaseDirExtract = NULL;
		BOOL fBaseDirectorySelected = FALSE;
		//dfuLong32 dfFirstParam = 0;
		//FILESET* pfsDest=NULL; // only for zip
		BOOL fOverwriteAll = TRUE;
		dfuLong32 dfError = 0;

//		dfuLong32 dfBaseVersionSpecified = VALUE_UNKNOWN;
//		dfuLong32 dfVersionSpecified = VALUE_UNKNOWN;

		wchBaseDirExtract = szDfsBaseDirectoryExtr;

		if (szDfsBaseDirectory[0] != 0)
			fBaseDirectorySelected = TRUE;

		if (dfBaseVersionSpecified == VALUE_UNKNOWN)
			dfBaseVersionSpecified = 0;

		pDirInfo = ReadAllDirInfo(DfsFile, &dfNbDir, READ_ALL_DIR, &dfError);

		if ((pDirInfo == NULL) || (dfError != DFS_SUCCESS))
			if (err_moment == ERROR_MOMENT_NO_ERROR)
				err_moment = ERROR_MOMENT_READ_DFS;

		if ((dfError != DFS_SUCCESS) && (errinfo == DFS_SUCCESS))
			errinfo = dfError;

		if (/*((*pDirInfo)->dfTypeDir != TYPEDIR_FILECRCONLY) ||*/ ((*pDirInfo)->dfNbFile != 1) && (onlyMonoFileBase != 0))
		{
			if ((err_moment) == ERROR_MOMENT_NO_ERROR)
				err_moment = ERROR_MOMENT_INTERPRET_DFS;
		}

		dfuLong32 countBaseFile;
		COMPARE_RES CompareRes = COMPARE_RES_GOOD;

		for (countBaseFile = 0; countBaseFile < (*pDirInfo)->dfNbFile; countBaseFile++)
		{
			// TODO : check a map, and if *(map+countBaseFile)==0 just do continue to skip
			const FILEINDIRINFO* pCurFileInDirInfo = ((*pDirInfo)->pFileInDirInfo + countBaseFile);
			char filename_base_file[MAX_PATH_LENGTH * 2];
			dfwchar dfwfilename_base_file[MAX_PATH_LENGTH * 2];

			strcpy(filename_base_file, basedir);
			int len_base_file = (int)strlen(filename_base_file);
			if (len_base_file != 0)
				if (!((((filename_base_file[len_base_file - 1])) == '\\') ||
					(((filename_base_file[len_base_file - 1])) == '/') ||
					(((filename_base_file[len_base_file - 1])) == ':')))
				{
					char szSep[0x20];
					ConvertUnicodeToAnsi(GetUnicodeStringDirectorySeparator(), szSep, 0x10);
					strcat(filename_base_file, szSep);
				}

			ConvertUnicodeToAnsi(((*pDirInfo)->pFileInDirInfo + countBaseFile)->FileName, filename_base_file + strlen(filename_base_file), MAX_PATH_LENGTH);
			ConvertAnsiToUnicode(filename_base_file, dfwfilename_base_file, MAX_PATH_LENGTH);
			dfuLong32 dfCrc32 = 0;
			dfbyte bMD5[16];
			dfbyte bSHA1[20];

			if ((*pDirInfo)->dfTypeDir == TYPEDIR_FILECRCONLY)
			{
				CompareRes =
					ComputeHashFile(dfwfilename_base_file,
					(pCurFileInDirInfo)->dfSize,
					(pCurFileInDirInfo)->fCrc32Filled && fReadHashContent, &dfCrc32,
					(pCurFileInDirInfo)->fMd5Filled && fReadHashContent, bMD5,
					(pCurFileInDirInfo)->fSha1Filled && fReadHashContent, bSHA1, &hei);

				if ((CompareRes != COMPARE_RES_GOOD))
				{
					if ((err_moment) == ERROR_MOMENT_NO_ERROR)
						err_moment = ERROR_MOMENT_READ_SOURCE;
					ret = 0;
				}



				if ((CompareRes == COMPARE_RES_GOOD) && (pCurFileInDirInfo->fCrc32Filled) && fReadHashContent)
					if ((dfCrc32 != pCurFileInDirInfo->dfCrc32))
						CompareRes = COMPARE_RES_BADCRC;

				if ((CompareRes == COMPARE_RES_GOOD) && (pCurFileInDirInfo->fMd5Filled) && fReadHashContent)
					if (DfsMemcmp(bMD5, pCurFileInDirInfo->bMd5, 16) != 0)
						CompareRes = COMPARE_RES_BADCRC;

				if ((CompareRes == COMPARE_RES_GOOD) && (pCurFileInDirInfo->fSha1Filled) && fReadHashContent)
					if (DfsMemcmp(bSHA1, pCurFileInDirInfo->bSha1, 20) != 0)
						CompareRes = COMPARE_RES_BADCRC;

				if (CompareRes == COMPARE_RES_BADCRC)
					if ((err_moment) == ERROR_MOMENT_NO_ERROR)
						err_moment = ERROR_MOMENT_COMPARE_SOURCE;

			}

		}

		if (dfVersionSpecified == VALUE_UNKNOWN)
			dfDirExtract = dfNbDir - 1;
		else
			dfDirExtract = dfVersionSpecified;


		pCurDirInfo = *(pDirInfo + dfDirExtract);

		if (pDirInfo != NULL)
			lpfExtractItemMap = (EXTRACTINGMAPITEM*)DfsMalloc(sizeof(EXTRACTINGMAPITEM)*(pCurDirInfo->dfNbFile + 1));

		if (lpfExtractItemMap != NULL)
		{
			dfuLong32 j /*,k*/;
			OVERWRITE_PARAM ovr;

			for (j = 0; j<pCurDirInfo->dfNbFile; j++)
			{
				//const dfwcharpc dfFileNameToTest = ((pCurDirInfo->pFileInDirInfo)+j)->FileName;

				//*(lpfExtractItemMap+j) = fInPlace ? ExtractInPlace : ExtractClassic;
				dfuLong32 dfExtractingMethodToFill = dfExtractingMethod & EXTRACTING_METHOD_MASK;
				*(lpfExtractItemMap + j) = ExtractClassic;
				if (dfExtractingMethodToFill == EXTRACTING_METHOD_INPLACE)
					*(lpfExtractItemMap + j) = ExtractInPlace;
				if (dfExtractingMethodToFill == EXTRACTING_METHOD_INPLACE_NOCHECKSUM)
					*(lpfExtractItemMap + j) = ExtractInPlaceNoChecksum;
				if (dfExtractingMethodToFill == EXTRACTING_METHOD_BY_MERGING)
					*(lpfExtractItemMap + j) = ExtractByMerging;
				if ((dfExtractingMethod & EXTRACTING_METHOD_FLUSH_FILE) != 0)
					fFlushWrite = TRUE;
			}

			ovr.fNoAll = FALSE;
			ovr.fYesAll = fOverwriteAll;

			fRetDoMulti = DoMultiExtracting(DfsFile, wchBaseDirExtract,
				NULL,
				FALSE, /* parameter : fTempDestExtr */
				dfDirExtract,
				pDirInfo,
				fBaseDirectorySelected, dfBaseVersionSpecified, szDfsBaseDirectory, NULL,
				pCurDirInfo->dfNbFile, lpfExtractItemMap,
				NULL,//EXTRACTINGMAPINFO* pExtractingMapInfo,
				pSetExtractPosCallBack, dfUserPtr,
				ConfirmBeforeCreatingFile, &ovr,//tConfirmBeforeCreatingFile pConfirmBeforeCreatingFile, dfvoidp dfUserPtrBeforeCreatingFile,
				//fncExtractingFileWorkingEventConsole,NULL,//tExtractingFileWorkingEvent pExtractingFileWorkingEvent, dfvoidp dfUserExtractingFileWorkingEvent,
				NULL, NULL,//tExtractingFileWorkingEvent pExtractingFileWorkingEvent, dfvoidp dfUserExtractingFileWorkingEvent,
				dwMinProgress, dwMaxProgress,
				FALSE, fFlushWrite, &hei);//BOOL fFlatExtracting, H_ERROR_INFO * pei);

			if (CompareRes == COMPARE_RES_BADCRC)
				if (err_moment == ERROR_MOMENT_NO_ERROR)
					err_moment = ERROR_MOMENT_EXTRACTING;



			DfsFree(lpfExtractItemMap);
		}

		FreeAllDirInfo(pDirInfo, dfNbDir);
	}




	{
		dfuLong32 dfClose = DfsClose(DfsFile, &hei);
		if (dfClose != DFS_SUCCESS)
			ret = 0;

		if ((errinfo == DFS_SUCCESS) && (dfClose != DFS_SUCCESS))
			errinfo = dfClose;
	}


	if (perr_moment != NULL)
		*perr_moment = err_moment;
	if (perrinfo != NULL)
		*perrinfo = (int)errinfo;

	if (hei != NULL)
	{
		if (err_moment != ERROR_MOMENT_NO_ERROR)
		{
			AddTextInBuf(&errBufTxt, &errBufSize, GetErrorMomentText(err_moment));
			AddTextInBuf(&errBufTxt, &errBufSize, ", ");
		}
		if (errinfo != DFS_SUCCESS)
		{
			AddTextInBuf(&errBufTxt, &errBufSize, GetErrorExplanation(errinfo));
			AddTextInBuf(&errBufTxt, &errBufSize, ", ");
		}
		GetErrorText(hei, errBufTxt, errBufSize);
	}
	FreeErrorInfoBlock(hei);
	hei = NULL;

	RELEASE_MUTEX_PREVENT_CONCURRENT

		return ret;
}


int SVFAPI ApplyMonofilePatchEx2(const char* base, int base_only_dir,
	const char*patch, const char*dest,
	ERROR_MOMENT *perr_moment,
	int* perrinfo, char* errBufTxt, int errBufSize, dfuLong32 dfExtractingMethod,
	tSetExtractPosCallBack pSetExtractPosCallBack, dfvoidp dfUserPtr,
	dfuLong32 dwMinProgress, dfuLong32 dwMaxProgress)
{
	return ApplyMultiFilePatchEx3(base, base_only_dir,
		patch, dest, 1,
		perr_moment,
		perrinfo, errBufTxt, errBufSize, dfExtractingMethod,
		VALUE_UNKNOWN, VALUE_UNKNOWN,
		pSetExtractPosCallBack, dfUserPtr,
		dwMinProgress, dwMaxProgress);
}


int SVFAPI ApplyMonofilePatchEx(const char* base, int base_only_dir,
	const char*patch, const char*dest,
	ERROR_MOMENT *perr_moment,
	int* perrinfo, char* errBufTxt, int errBufSize, dfuLong32 dfExtractingMethod)
{
	return ApplyMonofilePatchEx2(base, base_only_dir,
		patch, dest,
		perr_moment,
		perrinfo, errBufTxt, errBufSize, dfExtractingMethod, NULL, NULL, 0, 0);
}


void FillDirFileInfo(const FILEINDIRINFO*pCurFileInDirInfo,
	char* FileName, dfuLong32 FileNameBufferSize,
	dfuLong64* pSize,
	dfuLong32* pCrc32, BOOL *p_Crc32Found,
	unsigned char*bMd5, BOOL *p_Md5Found,
	unsigned char*bSha1, BOOL *p_Sha1Found,
	unsigned char*bSha256, BOOL *p_Sha256Found)
{
	if (FileName != NULL)
		ConvertUnicodeToAnsi(pCurFileInDirInfo->FileName, FileName, FileNameBufferSize);
	if (pSize != NULL)
		*pSize = pCurFileInDirInfo->dfSize;

	if (p_Crc32Found != NULL)
		*p_Crc32Found = pCurFileInDirInfo->fCrc32Filled;
	if ((pCrc32 != NULL) && (pCurFileInDirInfo->fCrc32Filled))
		*pCrc32 = pCurFileInDirInfo->dfCrc32;

	if (p_Md5Found != NULL)
		*p_Md5Found = pCurFileInDirInfo->fMd5Filled;
	if ((bMd5 != NULL) && (pCurFileInDirInfo->fMd5Filled))
		DfsMemcpy(bMd5, pCurFileInDirInfo->bMd5, 16);

	if (p_Sha1Found != NULL)
		*p_Sha1Found = pCurFileInDirInfo->fSha1Filled;
	if ((bSha1 != NULL) && (pCurFileInDirInfo->fSha1Filled))
		DfsMemcpy(bSha1, pCurFileInDirInfo->bSha1, 20);

	if (p_Sha256Found != NULL)
		*p_Sha256Found = pCurFileInDirInfo->fSha256Filled;
	if ((bSha256 != NULL) && (pCurFileInDirInfo->fSha256Filled))
		DfsMemcpy(bSha256, pCurFileInDirInfo->bSha256, 32);
}

int SVFAPI GetSvfMonoPatchInfo(const char*patch,
	KINDDFSFILE* pkdfsf,
	char* BaseFileName, dfuLong32 BaseFileNameBufferSize,
	dfuLong64* pSizeBase,
	dfuLong32* pCrc32Base, BOOL *p_Crc32BaseFound,
	unsigned char*bMd5Base, BOOL *p_Md5BaseFound,
	unsigned char*bSha1Base, BOOL *p_Sha1BaseFound,
	char* TargetFileName, dfuLong32 TargetFileNameBufferSize,
	dfuLong64* pSizeTarget,
	dfuLong32* pCrc32Target, BOOL *p_Crc32TargetFound,
	unsigned char*bMd5Target, BOOL *p_Md5TargetFound,
	unsigned char*bSha1Target, BOOL *p_Sha1TargetFound,
	dfuLong64* pDeplInPlaceSize, dfuLong64 * pDeplOutPlaceSize, BOOL *p_InsertSizeFound,
	ERROR_MOMENT *perr_moment,
	int* perrinfo, char* errBufTxt, int errBufSize
	)
{
	return GetSvfMonoPatchInfoEx(patch,
		pkdfsf,
		BaseFileName, BaseFileNameBufferSize,
		pSizeBase,
		pCrc32Base, p_Crc32BaseFound,
		bMd5Base, p_Md5BaseFound,
		bSha1Base, p_Sha1BaseFound,
		TargetFileName, TargetFileNameBufferSize,
		pSizeTarget,
		pCrc32Target, p_Crc32TargetFound,
		bMd5Target, p_Md5TargetFound,
		bSha1Target, p_Sha1TargetFound,
		pDeplInPlaceSize, pDeplOutPlaceSize, p_InsertSizeFound,
		perr_moment,
		perrinfo, errBufTxt, errBufSize,
		NULL, NULL, NULL, 0);
}

int SVFAPI GetSvfMonoPatchInfoEx(const char*patch,
	KINDDFSFILE* pkdfsf,
	char* BaseFileName, dfuLong32 BaseFileNameBufferSize,
	dfuLong64* pSizeBase,
	dfuLong32* pCrc32Base, BOOL *p_Crc32BaseFound,
	unsigned char*bMd5Base, BOOL *p_Md5BaseFound,
	unsigned char*bSha1Base, BOOL *p_Sha1BaseFound,
	char* TargetFileName, dfuLong32 TargetFileNameBufferSize,
	dfuLong64* pSizeTarget,
	dfuLong32* pCrc32Target, BOOL *p_Crc32TargetFound,
	unsigned char*bMd5Target, BOOL *p_Md5TargetFound,
	unsigned char*bSha1Target, BOOL *p_Sha1TargetFound,
	dfuLong64* pDeplInPlaceSize, dfuLong64 * pDeplOutPlaceSize, BOOL *p_InsertSizeFound,
	ERROR_MOMENT *perr_moment,
	int* perrinfo, char* errBufTxt, int errBufSize,
	dfuLong32* pNbVersion, dfuLong32* pSuggestedExtractMethod, dfvoidp pBufFullCompressInfo, dfuLong32 fullCompressInfoBufsize)
{
	return GetSvfMonoPatchInfoEx2(patch,
		pkdfsf,
		BaseFileName, BaseFileNameBufferSize,
		pSizeBase,
		pCrc32Base, p_Crc32BaseFound,
		bMd5Base, p_Md5BaseFound,
		bSha1Base, p_Sha1BaseFound, NULL, NULL,
		TargetFileName, TargetFileNameBufferSize,
		pSizeTarget,
		pCrc32Target, p_Crc32TargetFound,
		bMd5Target, p_Md5TargetFound,
		bSha1Target, p_Sha1TargetFound, NULL,NULL,
		pDeplInPlaceSize, pDeplOutPlaceSize, p_InsertSizeFound,
		perr_moment,
		perrinfo, errBufTxt, errBufSize,
		NULL, NULL, NULL, 0);
}
/*

typedef struct {
	dfuLong32  dfStructSize;
	dfuLong32  Crc32;
	dfuLong64  FileSize;
	DFSTM      FileTime;
	BOOL       fCrc32Found;
	BOOL       fMd5Found;
	BOOL       bSha1Found;
	BOOL       bSha256Found;
	unsigned char bMd5[16];
	unsigned char bSha1[20];
	unsigned char bSha256[32];
} FILE_IN_PATCH_INFO;
*/


int SVFAPI GetSvfMonoPatchInfoEx3(const char*patch,
	KINDDFSFILE* pkdfsf,
	char* BaseFileName, dfuLong32 BaseFileNameBufferSize,
	dfuLong64* pSizeBase, DFSTM* pDateBase,
	dfuLong32* pCrc32Base, BOOL *p_Crc32BaseFound,
	unsigned char*bMd5Base, BOOL *p_Md5BaseFound,
	unsigned char*bSha1Base, BOOL *p_Sha1BaseFound,
	unsigned char*bSha256Base, BOOL *p_Sha256BaseFound,
	char* TargetFileName, dfuLong32 TargetFileNameBufferSize,
	dfuLong64* pSizeTarget, DFSTM* pDateTarget,
	dfuLong32* pCrc32Target, BOOL *p_Crc32TargetFound,
	unsigned char*bMd5Target, BOOL *p_Md5TargetFound,
	unsigned char*bSha1Target, BOOL *p_Sha1TargetFound,
	unsigned char*bSha256Target, BOOL *p_Sha256TargetFound,
	dfuLong32* dfStorageStatus,
	dfuLong64* pDeplInPlaceSize, dfuLong64 * pDeplOutPlaceSize, BOOL *p_InsertSizeFound,
	ERROR_MOMENT *perr_moment,
	int* perrinfo, char* errBufTxt, int errBufSize,
	dfuLong32* pNbVersion, dfuLong32* pSuggestedExtractMethod,dfvoidp pBufFullCompressInfo, dfuLong32 fullCompressInfoBufsize)
{

	DFSFILE DfsFile = NULL;
	DFSFILEINFOPARAM DfsFileParam;
	H_ERROR_INFO hei = NULL;
	ERROR_MOMENT err_moment = ERROR_MOMENT_NO_ERROR;
	dfwchar szDfsFileName[MAX_PATH_LENGTH];
	int ret = 1;
	dfuLong32 errinfo = DFS_SUCCESS;

	PDIRINFO* pDirInfo = NULL;
	dfuLong32 dfNbDir = 0;



	if ((errBufTxt != NULL) && (errBufSize>0))
		*errBufTxt = 0;

	if (pkdfsf != NULL) *pkdfsf = KINDDFSFILE_INVALID;
	if (p_Crc32BaseFound != NULL) *p_Crc32BaseFound = FALSE;
	if (p_Md5BaseFound != NULL) *p_Md5BaseFound = FALSE;
	if (p_Sha1BaseFound != NULL) *p_Sha1BaseFound = FALSE;
	if (p_Sha256BaseFound != NULL) *p_Sha256BaseFound = FALSE;
	if (p_Crc32TargetFound != NULL) *p_Crc32TargetFound = FALSE;
	if (p_Md5TargetFound != NULL) *p_Md5TargetFound = FALSE;
	if (p_Sha1TargetFound != NULL) *p_Sha1TargetFound = FALSE;
	if (p_Sha256TargetFound != NULL) *p_Sha256TargetFound = FALSE;
	if (p_InsertSizeFound != NULL) *p_InsertSizeFound = FALSE;
	if (dfStorageStatus != NULL) *dfStorageStatus = 0;
	if (pDeplInPlaceSize != NULL) *pDeplInPlaceSize = 0;
	if (pDeplOutPlaceSize != NULL) *pDeplOutPlaceSize = 0;
	if (pNbVersion != NULL) *pNbVersion = 0;
	if (pSuggestedExtractMethod != NULL) *pSuggestedExtractMethod = 0;
	if (pBufFullCompressInfo != NULL) memset(pBufFullCompressInfo, 0, fullCompressInfoBufsize);

	if ((BaseFileName != NULL) && (BaseFileNameBufferSize>0))
		*BaseFileName = '\0';
	if ((TargetFileName != NULL) && (TargetFileNameBufferSize>0))
		*TargetFileName = '\0';


	ConvertAnsiToUnicode(patch, szDfsFileName, MAX_PATH_LENGTH);

	DfsFileParam.sizeStruct = sizeof(DfsFileParam);
	//   DfsFileParam.dfStatus = 0;  // remove to very specific ios debug
	DfsFileParam.dfStatus = DFS_READABLE;
	DfsFileParam.filename = szDfsFileName;

	{
		dfuLong32 dfOpen = DfsFileOpen(&DfsFileParam, &DfsFile, &hei);
		if (dfOpen != DFS_SUCCESS)
			ret = 0;

		if (((errinfo == DFS_SUCCESS) || (DfsFile == NULL)) && (dfOpen != DFS_SUCCESS))
			errinfo = dfOpen;
		if ((err_moment == ERROR_MOMENT_NO_ERROR) && (((DfsFile == NULL)) && (dfOpen != DFS_SUCCESS)))
			err_moment = ERROR_MOMENT_OPEN_DFS;
	}



	if (DfsFile != NULL)
	{
		DfsGetNbDir(DfsFile, &dfNbDir, NULL);
		if (pNbVersion != NULL)
			*pNbVersion = dfNbDir;
		pDirInfo = ReadAllDirInfo(DfsFile, &dfNbDir, dfNbDir, NULL);


		DFTAGBLOCKFLOAT TagBlockFloat;
		dfvoidp TagBufSuggestedMethod = NULL;
		dfuLong32 TagSizeSuggestedMethod = 0;

		TagBlockFloat = GetDfsTagBlockFloat(DfsFile, NULL);
		if (TagBlockFloat != NULL)
		{
			if (GetTagBlockFloat(TagBlockFloat, FLOATNUM_NOSPECIFIC, 0,
				DFSTAG_SUGGEST_METHOD_FLOAT, &TagBufSuggestedMethod, &TagSizeSuggestedMethod) && (pSuggestedExtractMethod != NULL))
			{
				if (TagSizeSuggestedMethod >= 8)
				{
					dfuLong32 suggestedMethod = ConvertuLongIntelToLong(*(((dfuLong32Intel*)TagBufSuggestedMethod) + 1));
					if ((suggestedMethod != EXTRACTING_METHOD_CLASSIC) && (suggestedMethod != EXTRACTING_METHOD_INPLACE) &&
						(suggestedMethod != EXTRACTING_METHOD_INPLACE_NOCHECKSUM) && (suggestedMethod != EXTRACTING_METHOD_BY_MERGING))
						suggestedMethod = 0;
#ifdef SVF_PREVENT_SUGGEST_INPLACE_METHOD
					if (suggestedMethod == EXTRACTING_METHOD_INPLACE)
						suggestedMethod = EXTRACTING_METHOD_CLASSIC;
#endif
					if (pSuggestedExtractMethod)
					  *pSuggestedExtractMethod = suggestedMethod;
				}
			}
		}
	}

	if (pDirInfo != NULL)
	{
		dfuLong32 curdir;
		if (dfNbDir < 1)
			ret = 0;
		if (dfNbDir > 1)
		{
			if (pkdfsf != NULL) *pkdfsf = KINDDFSFILE_PATCH;
			if ((((*(pDirInfo + 0))->dfTypeDir) != TYPEDIR_FILECRCONLY) || (((*(pDirInfo + 1))->dfTypeDir) != TYPEDIR_PATCHFROMPREVIOUS))
				ret = 0;
		}
		if (dfNbDir == 1)
		{
			if ((((*(pDirInfo + 0))->dfTypeDir) != TYPEDIR_FILEINSERTING_DEFLATE) && (((*(pDirInfo + 0))->dfTypeDir) != TYPEDIR_FILECRCONLY)
				&& (((*(pDirInfo + 0))->dfTypeDir) != TYPEDIR_FILEINSERTING_STORE))
				ret = 0;
			else
				if (pkdfsf != NULL)
				{
					if (((*(pDirInfo + 0))->dfTypeDir) == TYPEDIR_FILECRCONLY)
						*pkdfsf = KINDDFSFILE_REFERENCE;
					else
						*pkdfsf = KINDDFSFILE_FULL;
				}
		}

		BOOL allPatchHaveStorageInfo = TRUE;
		BOOL isFirstPatch = TRUE;
		dfuLong64 dfSizeDeplInPlace = 0;
		dfuLong64 dfPreviousStepSize = 0;
		for (curdir = 0; curdir<dfNbDir; curdir++)
		{
			int is_target, is_base;
			is_target = is_base = 0;
			PDIRINFO pCurDirInfo = *(pDirInfo + curdir);

			if ((pCurDirInfo)->dfNbFile != 1)
			{
				ret = 0;
				break;
			}
			const FILEINDIRINFO * pCurFileInDirInfo = ((pCurDirInfo)->pFileInDirInfo);
			if ((curdir == 0) && ((((pCurDirInfo)->dfTypeDir) == TYPEDIR_FILECRCONLY)))
				is_base = 1;
			if ((curdir == dfNbDir - 1) && ((((pCurDirInfo)->dfTypeDir) == TYPEDIR_FILEINSERTING_STORE) ||
				(((pCurDirInfo)->dfTypeDir) == TYPEDIR_FILEINSERTING_DEFLATE) ||
				(((pCurDirInfo)->dfTypeDir) == TYPEDIR_PATCHFROMPREVIOUS)))
				is_target = 1;


			BOOL isPatch = (((pCurDirInfo)->dfTypeDir) == TYPEDIR_PATCHFROMPREVIOUS);
			dfvoidp TagBufStorageStatus = NULL;
			dfvoidp TagBufStorageInfo = NULL;
			dfuLong32 TagSizeStorageStatus = 0;
			dfuLong32 TagSizeStorageInfo = 0;


			BOOL storageStatus = GetTag(*(pCurDirInfo->TagFile + 0), DFSTAG_STORAGESTATUS, &TagBufStorageStatus, &TagSizeStorageStatus);
			BOOL storageInfo = GetTag(*(pCurDirInfo->TagFile + 0), DFSTAG_STORAGEPATCHINFO, &TagBufStorageInfo, &TagSizeStorageInfo);

			if (storageStatus && (TagSizeStorageStatus == sizeof(dfuLong32Intel))) {
				dfuLong32 dfFileStatut = ConvertuLongIntelToLong(*(dfuLong32Intel*)TagBufStorageStatus);
				if (dfStorageStatus != NULL) *dfStorageStatus = dfFileStatut;
			}
			if ((storageInfo) && (TagSizeStorageInfo == sizeof(DFSSTORAGEPATCHINFOINTEL)))
			{
				if (TagSizeStorageInfo == sizeof(DFSSTORAGEPATCHINFOINTEL))
				{
					const DFSSTORAGEPATCHINFOINTEL* pDfsStoragePatchInfoIntel = (const DFSSTORAGEPATCHINFOINTEL*)TagBufStorageInfo;
					dfuLong64 dfSizeDeplInPlaceThis = ConvertuLongIntelToLong64(pDfsStoragePatchInfoIntel->dfSizeDeplInPlaceIntel);
					//dfuLong64 dfSizeDeplOutPlaceThis = ConvertuLongIntelToLong64(pDfsStoragePatchInfoIntel->dfSizeDeplOutPlaceIntel);

					if (isFirstPatch)
						dfSizeDeplInPlace = dfSizeDeplInPlaceThis;
					else
					{
						dfuLong64 dfCurrentStepSize = 0;
						FillDirFileInfo(pCurFileInDirInfo, NULL, 0, &dfCurrentStepSize, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

						dfuLong64 nbNotInPlaceInPrev =
							(dfPreviousStepSize > dfSizeDeplInPlace) ? (dfPreviousStepSize - dfSizeDeplInPlace) : 0;


						dfuLong64 nbNotInPlaceInCurrent =
							(dfCurrentStepSize > dfSizeDeplInPlaceThis) ? (dfCurrentStepSize - dfSizeDeplInPlaceThis) : 0;

						dfuLong64 nbNotInPlaceMax = nbNotInPlaceInPrev + nbNotInPlaceInCurrent;
						nbNotInPlaceMax = (nbNotInPlaceMax > dfCurrentStepSize) ? dfCurrentStepSize : nbNotInPlaceMax;

						dfSizeDeplInPlace = dfCurrentStepSize - nbNotInPlaceMax;
					}
				}
			}
			else if (isPatch)
				allPatchHaveStorageInfo = FALSE;

			if (isPatch)
				isFirstPatch = FALSE;

			FillDirFileInfo(pCurFileInDirInfo, NULL, 0, &dfPreviousStepSize, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

			if (is_base) {
				FillDirFileInfo(pCurFileInDirInfo, BaseFileName, BaseFileNameBufferSize,
				pSizeBase,
				pCrc32Base, p_Crc32BaseFound,
				bMd5Base, p_Md5BaseFound,
				bSha1Base, p_Sha1BaseFound,
				bSha256Base, p_Sha256BaseFound);
				if (pDateBase != NULL)
					*pDateBase = pCurFileInDirInfo->dfsTm;
		    }

			if (is_target)
			{
				FillDirFileInfo(pCurFileInDirInfo, TargetFileName, TargetFileNameBufferSize,
					pSizeTarget,
					pCrc32Target, p_Crc32TargetFound,
					bMd5Target, p_Md5TargetFound,
					bSha1Target, p_Sha1TargetFound,
					bSha256Target, p_Sha256TargetFound);

				if (pDateTarget != NULL)
					*pDateTarget = pCurFileInDirInfo->dfsTm;


				/*
				if (GetTag(*(pCurDirInfo->TagFile + 0), DFSTAG_STORAGESTATUS, &TagBuf, &TagSize))
				if (TagSize==sizeof(dfuLong32Intel))
				{
				dfuLong32 dfFileIdentical = ConvertuLongIntelToLong(*(dfuLong32Intel*)TagBuf);
				//dfuLong32 uiRes=0;
				switch (dfFileIdentical)
				{
				case DFS_STORAGESTATUS_IDENTICAL:
				//lpszTxt="Identical";
				break;

				case DFS_STORAGESTATUS_MODIFIED:
				//lpszTxt="Modified";
				break;

				case DFS_STORAGESTATUS_NEW:
				case DFS_STORAGESTATUS_NEWSTORED:
				//lpszTxt="New";
				break;

				case DFS_STORAGESTATUS_REFERENCE:
				//lpszTxt="Reference";
				break;
				}
				*/




				if ((storageInfo) && (TagSizeStorageInfo == sizeof(DFSSTORAGEPATCHINFOINTEL)))
				{
					if (TagSizeStorageInfo == sizeof(DFSSTORAGEPATCHINFOINTEL))
					{
						const DFSSTORAGEPATCHINFOINTEL* pDfsStoragePatchInfoIntel = (const DFSSTORAGEPATCHINFOINTEL*)TagBufStorageInfo;
						//dfuLong64 dfSizeDeplInPlace = ConvertuLongIntelToLong64(pDfsStoragePatchInfoIntel->dfSizeDeplInPlaceIntel);
						//dfSizeDeplInPlace = ConvertuLongIntelToLong64(pDfsStoragePatchInfoIntel->dfSizeDeplInPlaceIntel);
						dfuLong64 dfSizeDeplOutPlace = ConvertuLongIntelToLong64(pDfsStoragePatchInfoIntel->dfSizeDeplOutPlaceIntel);

						if (p_InsertSizeFound != NULL)
							*p_InsertSizeFound = (dfNbDir > 2) ? FALSE : TRUE;
						if (pDeplInPlaceSize != NULL)
							*pDeplInPlaceSize = dfSizeDeplInPlace;
						if (pDeplOutPlaceSize != NULL)
							*pDeplOutPlaceSize = dfSizeDeplOutPlace;
					}
				}
			}
		}
	}

	if (pDirInfo != NULL)
	{
		FreeAllDirInfo(pDirInfo, dfNbDir);
		pDirInfo = NULL;
	}


	if (DfsFile != NULL)
	{
		DfsClose(DfsFile, NULL);
		DfsFile = NULL;
	}


	return ret;
}



int SVFAPI GetSvfMonoPatchInfoStruct(const char*patch,
                        KINDDFSFILE* pkdfsf,
                        char* BaseFileName,dfuLong32 BaseFileNameBufferSize, FILE_IN_PATCH_INFO* pBaseFileInfo,
                        char* TargetFileName,dfuLong32 TargetFileNameBufferSize, FILE_IN_PATCH_INFO* pTargetFileInfo,
                        ERROR_MOMENT *perr_moment,
                        int* perrinfo,char* errBufTxt,int errBufSize,
                        dfuLong32* pNbVersion, dfuLong32* pSuggestedExtractMethod, dfvoidp pBufFullCompressInfo, dfuLong32 fullCompressInfoBufsize)
{
	if (pBaseFileInfo!=NULL) {
		pBaseFileInfo->insertSizeFound=0;
		pBaseFileInfo->dfSizeDeplInPlace=pBaseFileInfo->dfSizeDeplOutPlace=0;
		pBaseFileInfo->dfStorageStatus=0;
	}
	return GetSvfMonoPatchInfoEx3(patch, pkdfsf,
		BaseFileName, BaseFileNameBufferSize,
		(pBaseFileInfo != NULL) ? &pBaseFileInfo->FileSize : NULL,
		(pBaseFileInfo != NULL) ? &pBaseFileInfo->FileTime : NULL,
		(pBaseFileInfo != NULL) ? &pBaseFileInfo->Crc32 : NULL,
		(pBaseFileInfo != NULL) ? &pBaseFileInfo->fCrc32Found : NULL,
		(pBaseFileInfo != NULL) ? pBaseFileInfo->bMd5 : NULL,
		(pBaseFileInfo != NULL) ? &pBaseFileInfo->fMd5Found : NULL,
		(pBaseFileInfo != NULL) ? pBaseFileInfo->bSha1 : NULL,
		(pBaseFileInfo != NULL) ? &pBaseFileInfo->fSha1Found : NULL,
		(pBaseFileInfo != NULL) ? pBaseFileInfo->bSha256 : NULL,
		(pBaseFileInfo != NULL) ? &pBaseFileInfo->fSha256Found : NULL,

		TargetFileName, TargetFileNameBufferSize,
		(pTargetFileInfo != NULL) ? &pTargetFileInfo->FileSize : NULL,
		(pTargetFileInfo != NULL) ? &pTargetFileInfo->FileTime : NULL,
		(pTargetFileInfo != NULL) ? &pTargetFileInfo->Crc32 : NULL,
		(pTargetFileInfo != NULL) ? &pTargetFileInfo->fCrc32Found : NULL,
		(pTargetFileInfo != NULL) ? pTargetFileInfo->bMd5 : NULL,
		(pTargetFileInfo != NULL) ? &pTargetFileInfo->fMd5Found : NULL,
		(pTargetFileInfo != NULL) ? pTargetFileInfo->bSha1 : NULL,
		(pTargetFileInfo != NULL) ? &pTargetFileInfo->fSha1Found : NULL,
		(pTargetFileInfo != NULL) ? pTargetFileInfo->bSha256 : NULL,
		(pTargetFileInfo != NULL) ? &pTargetFileInfo->fSha256Found : NULL,
		(pTargetFileInfo != NULL) ? &pTargetFileInfo->dfStorageStatus : NULL,
		(pTargetFileInfo != NULL) ? &pTargetFileInfo->dfSizeDeplInPlace : NULL,
		(pTargetFileInfo != NULL) ? &pTargetFileInfo->dfSizeDeplOutPlace : NULL,
		(pTargetFileInfo != NULL) ? &pTargetFileInfo->insertSizeFound : NULL,
		perr_moment,
		perrinfo, errBufTxt, errBufSize,
		pNbVersion, pSuggestedExtractMethod, pBufFullCompressInfo, fullCompressInfoBufsize);
}

int SVFAPI GetSvfMonoPatchInfoEx2(const char*patch,
	KINDDFSFILE* pkdfsf,
	char* BaseFileName, dfuLong32 BaseFileNameBufferSize,
	dfuLong64* pSizeBase,
	dfuLong32* pCrc32Base, BOOL *p_Crc32BaseFound,
	unsigned char*bMd5Base, BOOL *p_Md5BaseFound,
	unsigned char*bSha1Base, BOOL *p_Sha1BaseFound,
	unsigned char*bSha256Base, BOOL *p_Sha256BaseFound,
	char* TargetFileName, dfuLong32 TargetFileNameBufferSize,
	dfuLong64* pSizeTarget,
	dfuLong32* pCrc32Target, BOOL *p_Crc32TargetFound,
	unsigned char*bMd5Target, BOOL *p_Md5TargetFound,
	unsigned char*bSha1Target, BOOL *p_Sha1TargetFound,
	unsigned char*bSha256Target, BOOL *p_Sha256TargetFound,
	dfuLong64* pDeplInPlaceSize, dfuLong64 * pDeplOutPlaceSize, BOOL *p_InsertSizeFound,
	ERROR_MOMENT *perr_moment,
	int* perrinfo, char* errBufTxt, int errBufSize,
	dfuLong32* pNbVersion, dfuLong32* pSuggestedExtractMethod,dfvoidp pBufFullCompressInfo, dfuLong32 fullCompressInfoBufsize)
{
	return GetSvfMonoPatchInfoEx3(patch,
		pkdfsf,
		BaseFileName, BaseFileNameBufferSize,
		pSizeBase, NULL,
		pCrc32Base, p_Crc32BaseFound,
		bMd5Base, p_Md5BaseFound,
		bSha1Base, p_Sha1BaseFound,
		bSha256Base, p_Sha256BaseFound,
		TargetFileName, TargetFileNameBufferSize,
		pSizeTarget, NULL,
		pCrc32Target, p_Crc32TargetFound,
		bMd5Target, p_Md5TargetFound,
		bSha1Target, p_Sha1TargetFound,
		bSha256Target, p_Sha256TargetFound,
		NULL,
		pDeplInPlaceSize, pDeplOutPlaceSize, p_InsertSizeFound,
		perr_moment,
		perrinfo, errBufTxt, errBufSize,
		pNbVersion, pSuggestedExtractMethod, pBufFullCompressInfo, fullCompressInfoBufsize);
}

int SVFAPI GetSvfMonoPatchInfoMemory(const void*patch_buffer, size_t patch_size,
	KINDDFSFILE* pkdfsf,
	char* BaseFileName, dfuLong32 BaseFileNameBufferSize,
	dfuLong64* pSizeBase,
	dfuLong32* pCrc32Base, BOOL *p_Crc32BaseFound,
	unsigned char*bMd5Base, BOOL *p_Md5BaseFound,
	unsigned char*bSha1Base, BOOL *p_Sha1BaseFound,
	char* TargetFileName, dfuLong32 TargetFileNameBufferSize,
	dfuLong64* pSizeTarget,
	dfuLong32* pCrc32Target, BOOL *p_Crc32TargetFound,
	unsigned char*bMd5Target, BOOL *p_Md5TargetFound,
	unsigned char*bSha1Target, BOOL *p_Sha1TargetFound,
	dfuLong64* pDeplInPlaceSize, dfuLong64 * pDeplOutPlaceSize, BOOL *p_InsertSizeFound,
	ERROR_MOMENT *perr_moment,
	int* perrinfo, char* errBufTxt, int errBufSize)
{
	return GetSvfMonoPatchInfoMemoryEx(patch_buffer, patch_size,
		pkdfsf,
		BaseFileName, BaseFileNameBufferSize,
		pSizeBase,
		pCrc32Base, p_Crc32BaseFound,
		bMd5Base, p_Md5BaseFound,
		bSha1Base, p_Sha1BaseFound,
		TargetFileName, TargetFileNameBufferSize,
		pSizeTarget,
		pCrc32Target, p_Crc32TargetFound,
		bMd5Target, p_Md5TargetFound,
		bSha1Target, p_Sha1TargetFound,
		pDeplInPlaceSize, pDeplOutPlaceSize, p_InsertSizeFound,
		perr_moment,
		perrinfo, errBufTxt, errBufSize,
		NULL, NULL, NULL, 0);
}



int SVFAPI GetSvfMonoPatchInfoMemoryEx2(const void*patch_buffer, size_t patch_size,
	KINDDFSFILE* pkdfsf,
	char* BaseFileName, dfuLong32 BaseFileNameBufferSize,
	dfuLong64* pSizeBase,
	dfuLong32* pCrc32Base, BOOL *p_Crc32BaseFound,
	unsigned char*bMd5Base, BOOL *p_Md5BaseFound,
	unsigned char*bSha1Base, BOOL *p_Sha1BaseFound,
	unsigned char*bSha256Base, BOOL *p_Sha256BaseFound,
	char* TargetFileName, dfuLong32 TargetFileNameBufferSize,
	dfuLong64* pSizeTarget,
	dfuLong32* pCrc32Target, BOOL *p_Crc32TargetFound,
	unsigned char*bMd5Target, BOOL *p_Md5TargetFound,
	unsigned char*bSha1Target, BOOL *p_Sha1TargetFound,
	unsigned char*bSha256Target, BOOL *p_Sha256TargetFound,
	dfuLong64* pDeplInPlaceSize, dfuLong64 * pDeplOutPlaceSize, BOOL *p_InsertSizeFound,
	ERROR_MOMENT *perr_moment,
	int* perrinfo, char* errBufTxt, int errBufSize,
	dfuLong32* pNbVersion, dfuLong32* pSuggestedExtractMethod, dfvoidp pBufFullCompressInfo, dfuLong32 fullCompressInfoBufsize
)
{
	LOWLEVELFILE llf;
	H_ERROR_INFO hei = NULL;
	int ret = 0;
	char szaPatchMemFileName[MAX_PATH_LENGTH + 1];
	dfwchar szwDfsFileName[MAX_PATH_LENGTH + 1];

	BuildTempMemFileNameForMemoryPatch(szaPatchMemFileName, szwDfsFileName, MAX_PATH_LENGTH, patch_buffer, patch_size);

	llf = memOpenOrBuildPermanentLowLevel(szwDfsFileName, OPEN_CREATE, FALSE, TRUE, (dfuLong64)patch_size, patch_buffer, &hei);
	if (llf != NULL)
	{
		LowLevelClose(llf, &hei);
		ret = GetSvfMonoPatchInfoEx2(szaPatchMemFileName,
			pkdfsf,
			BaseFileName, BaseFileNameBufferSize,
			pSizeBase,
			pCrc32Base, p_Crc32BaseFound,
			bMd5Base, p_Md5BaseFound,
			bSha1Base, p_Sha1BaseFound, bSha256Base, p_Sha256BaseFound,
			TargetFileName, TargetFileNameBufferSize,
			pSizeTarget,
			pCrc32Target, p_Crc32TargetFound,
			bMd5Target, p_Md5TargetFound,
			bSha1Target, p_Sha1TargetFound, bSha256Target, p_Sha256TargetFound,
			pDeplInPlaceSize, pDeplOutPlaceSize, p_InsertSizeFound,
			perr_moment,
			perrinfo, errBufTxt, errBufSize,
			pNbVersion, pSuggestedExtractMethod, pBufFullCompressInfo, fullCompressInfoBufsize
		);
		memDeleteFile(szwDfsFileName, &hei);
	}



	if (hei != NULL)
	{
		ERROR_MOMENT err_moment = ERROR_MOMENT_OPEN_DFS;
		{
			AddTextInBuf(&errBufTxt, &errBufSize, GetErrorMomentText(err_moment));
			AddTextInBuf(&errBufTxt, &errBufSize, ", ");
		}
		GetErrorText(hei, errBufTxt, errBufSize);
		FreeErrorInfoBlock(hei);
		hei = NULL;
	}
	return ret;
}


int SVFAPI GetSvfMonoPatchInfoMemoryStruct(const void*patch_buffer, size_t patch_size,
	KINDDFSFILE* pkdfsf,
	char* BaseFileName, dfuLong32 BaseFileNameBufferSize, FILE_IN_PATCH_INFO* pBaseFileInfo,
	char* TargetFileName, dfuLong32 TargetFileNameBufferSize, FILE_IN_PATCH_INFO* pTargetFileInfo,
	ERROR_MOMENT *perr_moment,
	int* perrinfo, char* errBufTxt, int errBufSize,
	dfuLong32* pNbVersion, dfuLong32* pSuggestedExtractMethod, dfvoidp pBufFullCompressInfo, dfuLong32 fullCompressInfoBufsize
)
{
	LOWLEVELFILE llf;
	H_ERROR_INFO hei = NULL;
	int ret = 0;
	char szaPatchMemFileName[MAX_PATH_LENGTH + 1];
	dfwchar szwDfsFileName[MAX_PATH_LENGTH + 1];

	BuildTempMemFileNameForMemoryPatch(szaPatchMemFileName, szwDfsFileName, MAX_PATH_LENGTH, patch_buffer, patch_size);

	llf = memOpenOrBuildPermanentLowLevel(szwDfsFileName, OPEN_CREATE, FALSE, TRUE, (dfuLong64)patch_size, patch_buffer, &hei);
	if (llf != NULL)
	{
		LowLevelClose(llf, &hei);
		ret = GetSvfMonoPatchInfoStruct(szaPatchMemFileName,
			pkdfsf,
			BaseFileName, BaseFileNameBufferSize, pBaseFileInfo,
			TargetFileName, TargetFileNameBufferSize, pTargetFileInfo,
			perr_moment,
			perrinfo, errBufTxt, errBufSize,
			pNbVersion, pSuggestedExtractMethod, pBufFullCompressInfo, fullCompressInfoBufsize
		);
		memDeleteFile(szwDfsFileName, &hei);
	}



	if (hei != NULL)
	{
		ERROR_MOMENT err_moment = ERROR_MOMENT_OPEN_DFS;
		{
			AddTextInBuf(&errBufTxt, &errBufSize, GetErrorMomentText(err_moment));
			AddTextInBuf(&errBufTxt, &errBufSize, ", ");
		}
		GetErrorText(hei, errBufTxt, errBufSize);
		FreeErrorInfoBlock(hei);
		hei = NULL;
	}
	return ret;
}



int SVFAPI GetSvfMonoPatchInfoMemoryEx(const void*patch_buffer, size_t patch_size,
	KINDDFSFILE* pkdfsf,
	char* BaseFileName, dfuLong32 BaseFileNameBufferSize,
	dfuLong64* pSizeBase,
	dfuLong32* pCrc32Base, BOOL *p_Crc32BaseFound,
	unsigned char*bMd5Base, BOOL *p_Md5BaseFound,
	unsigned char*bSha1Base, BOOL *p_Sha1BaseFound,
	char* TargetFileName, dfuLong32 TargetFileNameBufferSize,
	dfuLong64* pSizeTarget,
	dfuLong32* pCrc32Target, BOOL *p_Crc32TargetFound,
	unsigned char*bMd5Target, BOOL *p_Md5TargetFound,
	unsigned char*bSha1Target, BOOL *p_Sha1TargetFound,
	dfuLong64* pDeplInPlaceSize, dfuLong64 * pDeplOutPlaceSize, BOOL *p_InsertSizeFound,
	ERROR_MOMENT *perr_moment,
	int* perrinfo, char* errBufTxt, int errBufSize,
	dfuLong32* pNbVersion, dfuLong32* pSuggestedExtractMethod,dfvoidp pBufFullCompressInfo, dfuLong32 fullCompressInfoBufsize)
{
	LOWLEVELFILE llf;
	H_ERROR_INFO hei = NULL;
	int ret = 0;
	char szaPatchMemFileName[MAX_PATH_LENGTH + 1];
	dfwchar szwDfsFileName[MAX_PATH_LENGTH + 1];

	BuildTempMemFileNameForMemoryPatch(szaPatchMemFileName, szwDfsFileName, MAX_PATH_LENGTH, patch_buffer, patch_size);

	llf = memOpenOrBuildPermanentLowLevel(szwDfsFileName, OPEN_CREATE, FALSE, TRUE, (dfuLong64)patch_size, patch_buffer, &hei);
	if (llf != NULL)
	{
		LowLevelClose(llf, &hei);
		ret = GetSvfMonoPatchInfoEx(szaPatchMemFileName,
			pkdfsf,
			BaseFileName, BaseFileNameBufferSize,
			pSizeBase,
			pCrc32Base, p_Crc32BaseFound,
			bMd5Base, p_Md5BaseFound,
			bSha1Base, p_Sha1BaseFound,
			TargetFileName, TargetFileNameBufferSize,
			pSizeTarget,
			pCrc32Target, p_Crc32TargetFound,
			bMd5Target, p_Md5TargetFound,
			bSha1Target, p_Sha1TargetFound,
			pDeplInPlaceSize, pDeplOutPlaceSize, p_InsertSizeFound,
			perr_moment,
			perrinfo, errBufTxt, errBufSize,
			pNbVersion, pSuggestedExtractMethod, pBufFullCompressInfo, fullCompressInfoBufsize
			);
		memDeleteFile(szwDfsFileName, &hei);
	}



	if (hei != NULL)
	{
		ERROR_MOMENT err_moment = ERROR_MOMENT_OPEN_DFS;
		{
			AddTextInBuf(&errBufTxt, &errBufSize, GetErrorMomentText(err_moment));
			AddTextInBuf(&errBufTxt, &errBufSize, ", ");
		}
		GetErrorText(hei, errBufTxt, errBufSize);
		FreeErrorInfoBlock(hei);
		hei = NULL;
	}
	return ret;
}


SYNC_DIF_MUTEX_OBJECT SyncDifMutexVirtualFileNameSpaceMutexBuild = NULL;

int SVFAPI InitExtractSvfHelperEx(BOOL fPreventConcurrentExtracting)
{
	int ret = 1;
	if (fPreventConcurrentExtracting)
	{
		if (SyncDifMutexPreventConcurrentExtracting == NULL)
			SyncDifMutexPreventConcurrentExtracting = SyncDifBuildMutex();
		ret = (SyncDifMutexPreventConcurrentExtracting != NULL) ? 1 : 0;
	}
	else
	{
		if (SyncDifMutexPreventConcurrentExtracting != NULL)
		{
			SyncDifDeleteMutex(SyncDifMutexPreventConcurrentExtracting);
			SyncDifMutexPreventConcurrentExtracting = NULL;
		}
	}

	if (GetVirtualFileNameSpaceMutex() == NULL)
	{
		SYNC_DIF_MUTEX_OBJECT SyncDifMutex = SyncDifBuildMutex();
		if (SyncDifMutex == NULL)
			ret = 0;
		else
			if (SetVirtualFileNameSpaceMutex(SyncDifMutex, NULL, FALSE, FALSE))
			{
				SyncDifMutexVirtualFileNameSpaceMutexBuild = SyncDifMutex;
			}
			else
			{
				SyncDifDeleteMutex(SyncDifMutex);
				ret = 0;
			}
	}

	PerformInitAndMutexForMemFS();

	return ret;
}


#define SIZE_BUFFER_READBASE 0x8000
#define SIZE_BUFFER_WRITEBASE 0x8000

int SVFAPI WriteBaseFileUncompress(const char* srcfilename, const char* targetFileName, dfuLong64 expected_size, dfuLong32 expected_crc32_param)
{
  BOOL uncompress_possible = TRUE;


	dfuLong32 expected_crc32 = (dfuLong32)expected_crc32_param;
	dfuLong32 compute_crc = (dfuLong32)crc32(0, NULL, 0);
	dfuLong64 write_size = 0;


	unsigned char* byteArray = (unsigned char*)DfsMalloc(SIZE_BUFFER_READBASE);
	if (byteArray == NULL)
	{
		return 0;
	}

	abstract_decompress_stream stream;
	memset(&stream, 0, sizeof(stream));


	size_t size_uncompress_buffer = SIZE_BUFFER_WRITEBASE;
	void* out_buffer = (void*)DfsMalloc(size_uncompress_buffer);
    if (out_buffer == NULL)
	{
		DfsFree(byteArray);
		return 0;
	}
	FILE* fi = fopen(srcfilename, "rb");
	if (fi == NULL)
	{
		DfsFree(out_buffer);
		DfsFree(byteArray);
		return 0;
	}
	FILE* fo = fopen(targetFileName, "wb");
	if (fo == NULL)
	{
		DfsFree(out_buffer);
		DfsFree(byteArray);
		fclose(fi);
		return 0;
	}

	BOOL source_error = FALSE;
	BOOL write_error = FALSE;
	BOOL decompress_error = FALSE;
	BOOL stream_init_done = FALSE;
	int decompress_status = ABSTR_DECOMPRESS_Z_OK;

	while ((!source_error) && (!write_error) && (!decompress_error))
	{

		int nb_bytes = (int)fread(byteArray, 1, SIZE_BUFFER_READBASE, fi);

		if ((nb_bytes == 0) || (nb_bytes == -1))
			break;

		unsigned char* c_read_buffer = byteArray;
		if (c_read_buffer == NULL)
			break;

		if (!stream_init_done)
		{
			unsigned char prefix = *(const unsigned char*)c_read_buffer;

			if ((prefix != 'S') && uncompress_possible)
				decompress_status = abstract_init_prefix(&stream);
			else
				decompress_status = abstract_init_store(&stream);

			stream_init_done = TRUE;

			stream.avail_out = (uInt)size_uncompress_buffer;
			stream.next_out = (Bytef*)out_buffer;
		}

		stream.avail_in = (uInt)nb_bytes;
		stream.next_in = (Bytef*)c_read_buffer;

		while ((stream.avail_in != 0) && (decompress_status == ABSTR_DECOMPRESS_Z_OK))
		{
			decompress_status = abstract_decompress(&stream, ABSTR_DECOMPRESS_Z_SYNC_FLUSH);
			if ((decompress_status != ABSTR_DECOMPRESS_Z_OK) && (decompress_status != ABSTR_DECOMPRESS_Z_STREAM_END))
			{
				decompress_error = TRUE;
				break;
			}
			if (stream.avail_out != size_uncompress_buffer)
			{
				size_t pos_in_out_buffer = size_uncompress_buffer - stream.avail_out;
				if (fwrite(out_buffer, 1, (size_t)pos_in_out_buffer, fo) != (size_t)pos_in_out_buffer)
				{
					write_error = TRUE;
					break;
				}
				compute_crc = (dfuLong32)crc32(compute_crc, (const unsigned char*)out_buffer, (uInt)pos_in_out_buffer);
				write_size += pos_in_out_buffer;
			}
			stream.avail_out = (uInt)size_uncompress_buffer;
			stream.next_out = (Bytef*)out_buffer;
			if (decompress_status == ABSTR_DECOMPRESS_Z_STREAM_END)
				break;
		}
	}

	while ((!source_error) && (!write_error) && (!decompress_error) && (decompress_status != ABSTR_DECOMPRESS_Z_STREAM_END))
	{
		decompress_status = abstract_decompress(&stream, ABSTR_DECOMPRESS_Z_FINISH);
		if ((decompress_status != ABSTR_DECOMPRESS_Z_OK) && (decompress_status != ABSTR_DECOMPRESS_Z_STREAM_END))
		{

			decompress_error = TRUE;
			break;
		}
		if (stream.avail_out != size_uncompress_buffer)
		{
			size_t pos_in_out_buffer = size_uncompress_buffer - stream.avail_out;
			if (fwrite(out_buffer, 1, (size_t)pos_in_out_buffer, fo) != (size_t)pos_in_out_buffer)
			{
				write_error = TRUE;
				break;
			}
			compute_crc = (dfuLong32)crc32(compute_crc, (const unsigned char*)out_buffer, (uInt)pos_in_out_buffer);
			write_size += pos_in_out_buffer;
		}
		stream.avail_out = (uInt)size_uncompress_buffer;
		stream.next_out = (Bytef*)out_buffer;
	}
	fclose(fo);
	fclose(fi);
	abstract_decompress_end(&stream);
	DfsFree(out_buffer);
	DfsFree(byteArray);

  {
    BOOL success = ((write_size == expected_size) && (compute_crc == expected_crc32) && (!source_error) && (!write_error));

    return (success && (!source_error)) ? 1 : 0;
  }
}


int SVFAPI InitExtractSvfHelper()
{
	return InitExtractSvfHelperEx(FALSE);
}



int SVFAPI UninitExtractSvfHelper()
{
	int ret = 1;
	if (SyncDifMutexPreventConcurrentExtracting != NULL)
	{
		SyncDifDeleteMutex(SyncDifMutexPreventConcurrentExtracting);
		SyncDifMutexPreventConcurrentExtracting = NULL;
	}

	{
		SYNC_DIF_MUTEX_OBJECT SyncDifMutexPrev = NULL;
		if (SetVirtualFileNameSpaceMutex(NULL, &SyncDifMutexPrev, TRUE, TRUE))
		{
			if (SyncDifMutexVirtualFileNameSpaceMutexBuild != NULL)
				SyncDifDeleteMutex(SyncDifMutexVirtualFileNameSpaceMutexBuild);
		}
	}
	return ret;
}

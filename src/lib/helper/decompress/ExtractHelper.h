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



#ifndef EXTRACT_HELPHER_H_INCLUDED
#define EXTRACT_HELPHER_H_INCLUDED 1

#include "../../engine/patchstream/common/difbasic.h"



#ifdef __cplusplus
extern "C" {
#endif

    void SVFAPI SetConst(const char* szTempDirSet,unsigned long dfPhysicalMemoryKBvalueSet);


#ifndef T_SET_EXTRACT_POS_CALLBACK_DEFINED
#define T_SET_EXTRACT_POS_CALLBACK_DEFINED
typedef BOOL(DFSCALLBACK * tSetExtractPosCallBack) (dfuLong32 dwPos,
                                                    dfvoidp dfpAdditionnalInfo,
                                                    dfvoidp dfUserPtr);
#endif

#ifndef T_CUSTOM_MALLOC_CALLBACK_DEFINED
#define T_CUSTOM_MALLOC_CALLBACK_DEFINED
typedef void* (DFSCALLBACK * tCustomMallocCallBack) (dfuIntPtr dfSize,
                                                     dfvoidp dfUserPtr);
#endif

typedef enum
{
  COMPARE_RES_GOOD,
  COMPARE_RES_NOTFOUND,
  COMPARE_RES_BADSIZE,
  COMPARE_RES_PROBLEMREAD,
  COMPARE_RES_BADCRC,
  COMPARE_RES_UNDEFINED
} COMPARE_RES;

COMPARE_RES SVFAPI ComputeHashFile(dfwcharpc dfwFileNameToOpen,dfuLong64 dfExpectedSize,
                           BOOL fCompareCrc32,dfuLong32*p_dwCrc32,
                           BOOL fCompareMD5,unsigned char* bMD5,BOOL fCompareSHA1,unsigned char* bSHA1,
                           H_ERROR_INFO* pei);

typedef struct
{
    BOOL fYesAll;
    BOOL fNoAll;
} OVERWRITE_PARAM;


typedef enum
{
  ERROR_MOMENT_NO_ERROR=0,
  ERROR_MOMENT_OPEN_DFS,
  ERROR_MOMENT_READ_DFS,
  ERROR_MOMENT_INTERPRET_DFS,
  ERROR_MOMENT_READ_SOURCE,
  ERROR_MOMENT_COMPARE_SOURCE,
  ERROR_MOMENT_EXTRACTING
} ERROR_MOMENT;

typedef enum
{
  KINDDFSFILE_INVALID=0,
  KINDDFSFILE_REFERENCE,
  KINDDFSFILE_FULL,
  KINDDFSFILE_PATCH
} KINDDFSFILE;

#define EXTRACTING_METHOD_CLASSIC 0x00001
#define EXTRACTING_METHOD_INPLACE 0x00002
#define EXTRACTING_METHOD_INPLACE_NOCHECKSUM 0x00003
#define EXTRACTING_METHOD_BY_MERGING 0x00011
#define EXTRACTING_METHOD_MASK 0x00013

#define EXTRACTING_METHOD_NO_CHECK_HASH_ORG_CONTENT 0x00004
#define EXTRACTING_METHOD_FLUSH_FILE 0x0008

const char* GetErrorMomentText(ERROR_MOMENT err_moment);

const char* GetErrorExplanation(dfuLong32 dfErr);

typedef struct {
    dfuLong32  dfStructSize;
    dfuLong32  Crc32;
    dfuLong64  FileSize;
    DFSTM      FileTime;
    BOOL       fCrc32Found;
    BOOL       fMd5Found;
    BOOL       fSha1Found;
    BOOL       fSha256Found;

    dfuLong64  dfSizeDeplInPlace;
    dfuLong64  dfSizeDeplOutPlace;
    dfuLong32  dfStorageStatus;
    BOOL       insertSizeFound;
    unsigned char bMd5[16];
    unsigned char bSha1[20];
    unsigned char bSha256[32];
} FILE_IN_PATCH_INFO;

int SVFAPI DoComputeHashFile(const char*filename,dfuLong64 dfExpectedSize,
                           BOOL fCompareCrc32,dfuLong32*p_dwCrc32,
                           BOOL fCompareMD5,dfbytep bMD5,BOOL fCompareSHA1,dfbytep bSHA1,
                           char* errBufTxt,int errBufSize);

int SVFAPI DoComputeHashFileEx(const char*filename,dfuLong64* p_dfSizeFill,
                           BOOL fCompareCrc32,dfuLong32*p_dwCrc32,
                           BOOL fCompareMD5,dfbytep bMD5,BOOL fCompareSHA1,dfbytep bSHA1,
                           char* errBufTxt,int errBufSize);

COMPARE_RES SVFAPI ComputeHashFileReadingBuffer(dfwcharpc dfwFileNameToOpen,dfuLong64 dfExpectedSize,
                           BOOL fCompareCrc32,dfuLong32*p_dwCrc32,
                           BOOL fCompareMD5,dfbytep bMD5,BOOL fCompareSHA1,dfbytep bSHA1,
                           H_ERROR_INFO* pei);

COMPARE_RES SVFAPI ComputeHashFileReadingBufferEx(dfwcharpc dfwFileNameToOpen,dfuLong64 dfExpectedSize,dfuLong64* p_dfSizeFill,
                           BOOL fCompareCrc32,dfuLong32*p_dwCrc32,
                           BOOL fCompareMD5,dfbytep bMD5,BOOL fCompareSHA1,dfbytep bSHA1,
                           H_ERROR_INFO* pei);

COMPARE_RES SVFAPI ComputeHashFile(dfwcharpc dfwFileNameToOpen,dfuLong64 dfExpectedSize,
                           BOOL fCompareCrc32,dfuLong32*p_dwCrc32,
                           BOOL fCompareMD5,dfbytep bMD5,BOOL fCompareSHA1,dfbytep bSHA1,
                           H_ERROR_INFO* pei);

COMPARE_RES SVFAPI ComputeHashFileEx(dfwcharpc dfwFileNameToOpen,dfuLong64 dfExpectedSize,dfuLong64* p_dfSizeFill,
                           BOOL fCompareCrc32,dfuLong32*p_dwCrc32,
                           BOOL fCompareMD5,dfbytep bMD5,BOOL fCompareSHA1,dfbytep bSHA1,
                           H_ERROR_INFO* pei);


COMPARE_RES SVFAPI CompareTwoFile(dfwcharpc dfwFileName1, dfwcharpc dfwFileName2,
                       H_ERROR_INFO* pei);

int SVFAPI ApplyMonofilePatch(const char* base,int base_only_dir,
                       const char*patch,const char*dest,
                       ERROR_MOMENT *err_moment,
                       int* errinfo,char* errBufTxt,int errBufSize);

int SVFAPI ApplyMonofilePatchMemory(const char* base,int base_only_dir,
                       const void*patch_buffer,size_t patch_size,
                       const char*dest,
                       ERROR_MOMENT *perr_moment,
                       int* perrinfo,char* errBufTxt,int errBufSize);

int SVFAPI ApplyMonofilePatchMemoryEx(const char* base,int base_only_dir,
                       const void*patch_buffer,size_t patch_size,
                       const char*dest,
                       ERROR_MOMENT *perr_moment,
                       int* perrinfo,char* errBufTxt,int errBufSize,dfuLong32 dfExtractingMethod);

int SVFAPI ApplyMonofilePatchMemoryEx2(const char* base,int base_only_dir,
                       const void*patch_buffer,size_t patch_size,
                       const char*dest,
                       ERROR_MOMENT *perr_moment,
                       int* perrinfo,char* errBufTxt,int errBufSize,dfuLong32 dfExtractingMethod,
                       tSetExtractPosCallBack pSetExtractPosCallBack,dfvoidp dfUserPtr,
                       dfuLong32 dwMinProgress,dfuLong32 dwMaxProgress);

int SVFAPI ApplyMultiFilePatchMemoryEx3(const char* base, int base_only_dir,
                       const void*patch_buffer, size_t patch_size, const char*dest, int onlyMonoFileBase,
                       ERROR_MOMENT *perr_moment,
                       int* perrinfo, char* errBufTxt, int errBufSize, dfuLong32 dfExtractingMethod,
                       dfuLong32 dfBaseVersionSpecified, dfuLong32 dfVersionSpecified,
                       tSetExtractPosCallBack pSetExtractPosCallBack, dfvoidp dfUserPtr,
                       dfuLong32 dwMinProgress, dfuLong32 dwMaxProgress);

int SVFAPI ApplyMonofilePatchEx(const char* base,int base_only_dir,
                       const char*patch,const char*dest,
                       ERROR_MOMENT *perr_moment,
                       int* perrinfo,char* errBufTxt,int errBufSize,dfuLong32 dfExtractingMethod);

int SVFAPI ApplyMonofilePatchEx2(const char* base,int base_only_dir,
                       const char*patch,const char*dest,
                       ERROR_MOMENT *perr_moment,
                       int* perrinfo,char* errBufTxt,int errBufSize,dfuLong32 dfExtractingMethod,
                       tSetExtractPosCallBack pSetExtractPosCallBack,dfvoidp dfUserPtr,
                       dfuLong32 dwMinProgress,dfuLong32 dwMaxProgress);


int SVFAPI ApplyMultiFilePatchEx3(const char* base, int base_only_dir,
                       const char*patch, const char*dest, int onlyMonoFileBase,
                       ERROR_MOMENT *perr_moment,
                       int* perrinfo, char* errBufTxt, int errBufSize, dfuLong32 dfExtractingMethod,
                       dfuLong32 dfBaseVersionSpecified,dfuLong32 dfVersionSpecified,
                       tSetExtractPosCallBack pSetExtractPosCallBack, dfvoidp dfUserPtr,
                       dfuLong32 dwMinProgress, dfuLong32 dwMaxProgress);

int SVFAPI GetSvfMonoPatchInfo(const char*patch,
                        KINDDFSFILE* pkdfsf,
                        char* BaseFileName,dfuLong32 BaseFileNameBufferSize,
                        dfuLong64* pSizeBase,
                        dfuLong32* pCrc32Base,BOOL *p_Crc32BaseFound,
                        unsigned char*bMd5Base,BOOL *p_Md5BaseFound,
                        unsigned char*bSha1Base,BOOL *p_Sha1BaseFound,
                        char* TargetFileName,dfuLong32 TargetFileNameBufferSize,
                        dfuLong64* pSizeTarget,
                        dfuLong32* pCrc32Target,BOOL *p_Crc32TargetFound,
                        unsigned char*bMd5Target,BOOL *p_Md5TargetFound,
                        unsigned char*bSha1Target,BOOL *p_Sha1TargetFound,
                        dfuLong64* pDeplInPlaceSize,dfuLong64 * pDeplOutPlaceSize,BOOL *p_InsertSizeFound,
                        ERROR_MOMENT *perr_moment,
                        int* perrinfo,char* errBufTxt,int errBufSize
                        );


int SVFAPI GetSvfMonoPatchInfoEx(const char*patch,
                        KINDDFSFILE* pkdfsf,
                        char* BaseFileName,dfuLong32 BaseFileNameBufferSize,
                        dfuLong64* pSizeBase,
                        dfuLong32* pCrc32Base,BOOL *p_Crc32BaseFound,
                        unsigned char*bMd5Base,BOOL *p_Md5BaseFound,
                        unsigned char*bSha1Base,BOOL *p_Sha1BaseFound,
                        char* TargetFileName,dfuLong32 TargetFileNameBufferSize,
                        dfuLong64* pSizeTarget,
                        dfuLong32* pCrc32Target,BOOL *p_Crc32TargetFound,
                        unsigned char*bMd5Target,BOOL *p_Md5TargetFound,
                        unsigned char*bSha1Target,BOOL *p_Sha1TargetFound,
                        dfuLong64* pDeplInPlaceSize,dfuLong64 * pDeplOutPlaceSize,BOOL *p_InsertSizeFound,
                        ERROR_MOMENT *perr_moment,
                        int* perrinfo,char* errBufTxt,int errBufSize,
                        dfuLong32* pNbVersion, dfuLong32* pSuggestedExtractMethod, dfvoidp pBufFullCompressInfo, dfuLong32 fullCompressInfoBufsize
                        );


int SVFAPI GetSvfMonoPatchInfoEx2(const char*patch,
                        KINDDFSFILE* pkdfsf,
                        char* BaseFileName,dfuLong32 BaseFileNameBufferSize,
                        dfuLong64* pSizeBase,
                        dfuLong32* pCrc32Base,BOOL *p_Crc32BaseFound,
                        unsigned char*bMd5Base,BOOL *p_Md5BaseFound,
                        unsigned char*bSha1Base,BOOL *p_Sha1BaseFound,
                        unsigned char*bSha256Base,BOOL *p_Sha256BaseFound,
                        char* TargetFileName,dfuLong32 TargetFileNameBufferSize,
                        dfuLong64* pSizeTarget,
                        dfuLong32* pCrc32Target,BOOL *p_Crc32TargetFound,
                        unsigned char*bMd5Target,BOOL *p_Md5TargetFound,
                        unsigned char*bSha1Target,BOOL *p_Sha1TargetFound,
                        unsigned char*bSha256Target,BOOL *p_Sha256TargetFound,
                        dfuLong64* pDeplInPlaceSize,dfuLong64 * pDeplOutPlaceSize,BOOL *p_InsertSizeFound,
                        ERROR_MOMENT *perr_moment,
                        int* perrinfo,char* errBufTxt,int errBufSize,
                        dfuLong32* pNbVersion, dfuLong32* pSuggestedExtractMethod, dfvoidp pBufFullCompressInfo, dfuLong32 fullCompressInfoBufsize
                        );

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
	                    dfuLong32* pNbVersion, dfuLong32* pSuggestedExtractMethod,dfvoidp pBufFullCompressInfo, dfuLong32 fullCompressInfoBufsize);

int SVFAPI GetSvfMonoPatchInfoStruct(const char*patch,
                        KINDDFSFILE* pkdfsf,
                        char* BaseFileName,dfuLong32 BaseFileNameBufferSize, FILE_IN_PATCH_INFO* pBaseFileInfo,
                        char* TargetFileName,dfuLong32 TargetFileNameBufferSize, FILE_IN_PATCH_INFO* pTargetFileInfo,
                        ERROR_MOMENT *perr_moment,
                        int* perrinfo,char* errBufTxt,int errBufSize,
                        dfuLong32* pNbVersion, dfuLong32* pSuggestedExtractMethod, dfvoidp pBufFullCompressInfo, dfuLong32 fullCompressInfoBufsize
                        );

int SVFAPI GetSvfMonoPatchInfoMemory(const void*patch_buffer,size_t patch_size,
                        KINDDFSFILE* pkdfsf,
                        char* BaseFileName,dfuLong32 BaseFileNameBufferSize,
                        dfuLong64* pSizeBase,
                        dfuLong32* pCrc32Base,BOOL *p_Crc32BaseFound,
                        unsigned char*bMd5Base,BOOL *p_Md5BaseFound,
                        unsigned char*bSha1Base,BOOL *p_Sha1BaseFound,
                        char* TargetFileName,dfuLong32 TargetFileNameBufferSize,
                        dfuLong64* pSizeTarget,
                        dfuLong32* pCrc32Target,BOOL *p_Crc32TargetFound,
                        unsigned char*bMd5Target,BOOL *p_Md5TargetFound,
                        unsigned char*bSha1Target,BOOL *p_Sha1TargetFound,
                        dfuLong64* pDeplInPlaceSize,dfuLong64 * pDeplOutPlaceSize,BOOL *p_InsertSizeFound,
                        ERROR_MOMENT *perr_moment,
                        int* perrinfo,char* errBufTxt,int errBufSize
                        );

int SVFAPI GetSvfMonoPatchInfoMemoryEx(const void*patch_buffer,size_t patch_size,
                        KINDDFSFILE* pkdfsf,
                        char* BaseFileName,dfuLong32 BaseFileNameBufferSize,
                        dfuLong64* pSizeBase,
                        dfuLong32* pCrc32Base,BOOL *p_Crc32BaseFound,
                        unsigned char*bMd5Base,BOOL *p_Md5BaseFound,
                        unsigned char*bSha1Base,BOOL *p_Sha1BaseFound,
                        char* TargetFileName,dfuLong32 TargetFileNameBufferSize,
                        dfuLong64* pSizeTarget,
                        dfuLong32* pCrc32Target,BOOL *p_Crc32TargetFound,
                        unsigned char*bMd5Target,BOOL *p_Md5TargetFound,
                        unsigned char*bSha1Target,BOOL *p_Sha1TargetFound,
                        dfuLong64* pDeplInPlaceSize,dfuLong64 * pDeplOutPlaceSize,BOOL *p_InsertSizeFound,
                        ERROR_MOMENT *perr_moment,
                        int* perrinfo,char* errBufTxt,int errBufSize,
                        dfuLong32* pNbVersion, dfuLong32* pSuggestedExtractMethod, dfvoidp pBufFullCompressInfo, dfuLong32 fullCompressInfoBufsize
                        );

int SVFAPI GetSvfMonoPatchInfoMemoryEx2(const void*patch_buffer,size_t patch_size,
                        KINDDFSFILE* pkdfsf,
                        char* BaseFileName,dfuLong32 BaseFileNameBufferSize,
                        dfuLong64* pSizeBase,
                        dfuLong32* pCrc32Base,BOOL *p_Crc32BaseFound,
                        unsigned char*bMd5Base,BOOL *p_Md5BaseFound,
                        unsigned char*bSha1Base,BOOL *p_Sha1BaseFound,
                        unsigned char*bSha256Base,BOOL *p_Sha256BaseFound,
                        char* TargetFileName,dfuLong32 TargetFileNameBufferSize,
                        dfuLong64* pSizeTarget,
                        dfuLong32* pCrc32Target,BOOL *p_Crc32TargetFound,
                        unsigned char*bMd5Target,BOOL *p_Md5TargetFound,
                        unsigned char*bSha1Target,BOOL *p_Sha1TargetFound,
                        unsigned char*bSha256Target,BOOL *p_Sha256TargetFound,
                        dfuLong64* pDeplInPlaceSize,dfuLong64 * pDeplOutPlaceSize,BOOL *p_InsertSizeFound,
                        ERROR_MOMENT *perr_moment,
                        int* perrinfo,char* errBufTxt,int errBufSize,
                        dfuLong32* pNbVersion, dfuLong32* pSuggestedExtractMethod, dfvoidp pBufFullCompressInfo, dfuLong32 fullCompressInfoBufsize
                        );

int SVFAPI GetSvfMonoPatchInfoMemoryStruct(const void*patch_buffer, size_t patch_size,
	                    KINDDFSFILE* pkdfsf,
	                    char* BaseFileName, dfuLong32 BaseFileNameBufferSize, FILE_IN_PATCH_INFO* pBaseFileInfo,
	                    char* TargetFileName, dfuLong32 TargetFileNameBufferSize, FILE_IN_PATCH_INFO* pTargetFileInfo,
	                    ERROR_MOMENT *perr_moment,
	                    int* perrinfo, char* errBufTxt, int errBufSize,
	                    dfuLong32* pNbVersion, dfuLong32* pSuggestedExtractMethod, dfvoidp pBufFullCompressInfo, dfuLong32 fullCompressInfoBufsize);


int SVFAPI ApplyMonofilePatchMemoryToMemory(const char* base,int base_only_dir,
                       const void*patch_buffer,size_t patch_size,
                       const char*destfilename,void** destBuffer,size_t *dest_size,
                       ERROR_MOMENT *perr_moment,
                       int* perrinfo,char* errBufTxt,int errBufSize,dfuLong32 dfExtractingMethod,
                       tSetExtractPosCallBack pSetExtractPosCallBack,dfvoidp dfUserPtr,
                       dfuLong32 dwMinProgress,dfuLong32 dwMaxProgress);

int SVFAPI ApplyMonofilePatchMemoryToMemoryEx(const char* base, int base_only_dir,
                       const void*patch_buffer, size_t patch_size,
                       const char*destfilename, void** destBuffer, size_t *dest_size,
                       ERROR_MOMENT *perr_moment,
                       int* perrinfo, char* errBufTxt, int errBufSize, dfuLong32 dfExtractingMethod,
                       dfuLong32 dfBaseVersionSpecified, dfuLong32 dfVersionSpecified,
                       tSetExtractPosCallBack pSetExtractPosCallBack, dfvoidp dfUserPtr,
                       dfuLong32 dwMinProgress, dfuLong32 dwMaxProgress);

int SVFAPI ApplyMonofilePatchMemoryToMemoryEx2(const char* base, int base_only_dir,
                       const void*patch_buffer, size_t patch_size,
                       const char*destfilename, void** destBuffer, size_t *dest_size,
                       tCustomMallocCallBack pCustomMallocCallBack, dfvoidp dfMallocUserPtr,
                       ERROR_MOMENT *perr_moment,
                       int* perrinfo, char* errBufTxt, int errBufSize, dfuLong32 dfExtractingMethod,
                       dfuLong32 dfBaseVersionSpecified, dfuLong32 dfVersionSpecified,
                       tSetExtractPosCallBack pSetExtractPosCallBack, dfvoidp dfUserPtr,
                       dfuLong32 dwMinProgress, dfuLong32 dwMaxProgress);

int SVFAPI WriteBaseFileUncompress(const char* srcfilename, const char* targetFileName, dfuLong64 expected_size, dfuLong32 expected_crc32_param);

int SVFAPI InitExtractSvfHelper();

int SVFAPI InitExtractSvfHelperEx(BOOL fPreventConcurrentExtracting);

int SVFAPI UninitExtractSvfHelper();

#ifdef __cplusplus
}
#endif


#endif

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

#include "svfdll.h"
#include "../lib/helper/decompress/ExtractHelper.h"
#include "../cli/DfsCdLin.h"

#if defined(DFS_MAIN_TEST_DLL)

#include <windows.h>



#define COMPRESS_RATIO (2)



/*
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
                        );*/

#define SIZE_MD5_DATA 16
#define SIZE_SHA1_DATA 20
#define SIZE_SHA256_DATA 32


static int ShowSvfInfoOld(const char* svffile)
{/*
    KINDDFSFILE pkdfsf;
    char BaseFileName[0x400];
    dfuLong64 sizeBase;
    dfuLong32 crc32Base;
    BOOL crc32BaseFound;
    unsigned char  Md5Base[SIZE_MD5_DATA];
    BOOL md5BaseFound;
    unsigned char  sha1Base[SIZE_SHA1_DATA];
    BOOL p_Sha1BaseFound;
    unsigned char bSha256Base[SIZE_SHA256_DATA];
    BOOL p_Sha256BaseFound;
    char TargetFileName[0x400];
    dfuLong64 sizeTarget;

    BOOL crc32TargetFound;
    unsigned char  Md5Target[SIZE_MD5_DATA];
    BOOL md5TargetFound;
    unsigned char  sha1Target[SIZE_SHA1_DATA];
    BOOL p_Sha1TargetFound;
    unsigned char bSha256Target[SIZE_SHA256_DATA];
    BOOL p_Sha256TargetFound;
    dfuLong64 deplInPlaceSize, deplOutPlaceSize, fInsertSizeFound;
    ERROR_MOMENT perr_moment= ERROR_MOMENT_NO_ERROR;
    int perrinfo = 0;
    char errBufTxt[0x400] = "";
    dfuLong32 nbVersion;
    dfuLong32 suggestedExtractMethod;
    dfvoidp pBufFullCompressInfo;
    dfuLong32 fullCompressInfoBufsize;*/

    KINDDFSFILE kdfsf;
    char BaseFileName[0x400]="";
    dfuLong64 SizeBase = 0;
    dfuLong32 crc32Base = 0;
    dfuLong32 fCrc32PresentBase, fMd5PresentBase, fSha1PresentBase, fSha256PresentBase;
    unsigned char bMd5Base[SIZE_MD5_DATA];
    unsigned char bSha1Base[SIZE_SHA1_DATA];
    unsigned char bSha256Base[SIZE_SHA256_DATA];
    char TargetFileName[0x400]="";
    dfuLong64 SizeTarget = 0;
    dfuLong32 crc32Target = 0;
    dfuLong32 fCrc32PresentTarget, fMd5PresentTarget, fSha1PresentTarget, fSha256PresentTarget;
    unsigned char bMd5Target[SIZE_MD5_DATA];
    unsigned char bSha1Target[SIZE_SHA1_DATA];
    unsigned char bSha256Target[SIZE_SHA256_DATA];
    dfuLong64 dfDeplInPlaceSize = 0;
    dfuLong64 dfDeplOutPlaceSize = 0;
    dfuLong32 fInsertSizeFound = 0;

    dfuLong32 suggestedExtractMethod = 0;
    ERROR_MOMENT err_moment_get = ERROR_MOMENT_NO_ERROR;
    int errinfoget = 0;
    char errBufget[0x100] = "";
    fCrc32PresentBase = fMd5PresentBase = fSha1PresentBase = fCrc32PresentTarget = fMd5PresentTarget = fSha1PresentTarget = FALSE;
    int retGetPatchInfo = GetSvfMonoPatchInfoEx2(svffile,
        &kdfsf,
        BaseFileName, 0x400, &SizeBase,
        &crc32Base, (BOOL*)&fCrc32PresentBase,
        bMd5Base, (BOOL*)&fMd5PresentBase,
        bSha1Base, (BOOL*)&fSha1PresentBase,
        bSha256Base, (BOOL*)&fSha256PresentBase,

        TargetFileName, 0x400, &SizeTarget, &crc32Target, (BOOL*)&fCrc32PresentTarget,
        bMd5Target, (BOOL*)&fMd5PresentTarget,
        bSha1Target, (BOOL*)&fSha1PresentTarget,
        bSha256Target, (BOOL*)&fSha256PresentTarget,

        &dfDeplInPlaceSize, &dfDeplOutPlaceSize, (BOOL*)&fInsertSizeFound,
        &err_moment_get, &errinfoget, errBufget, sizeof(errBufget) - 1, NULL, &suggestedExtractMethod, NULL, 0);

    if ((retGetPatchInfo != 1) || ((kdfsf != KINDDFSFILE_FULL) && (kdfsf != KINDDFSFILE_PATCH)))
    {
        fprintf(stderr,"error: invalid SVF file %s\n", svffile);
        return 1;
    }
    fprintf(stderr, "SVF file %s kind %s ", svffile, (kdfsf == KINDDFSFILE_FULL) ? "full" : "patch");
    if (kdfsf == KINDDFSFILE_PATCH)
        fprintf(stderr, "from file %s size %i bytes ", BaseFileName, (int)SizeBase);
    fprintf(stderr, "to file %s size %i bytes ", TargetFileName, (int)SizeTarget);
    fprintf(stderr, "\n");
    return 0;
}

static int ShowSvfInfo(const char* svffile)
{
    FILE_IN_PATCH_INFO fipiBase, fipiTarget;
    KINDDFSFILE kdfsf;
    char BaseFileName[0x400]="";
    char TargetFileName[0x400]="";

    dfuLong32 suggestedExtractMethod = 0;
    ERROR_MOMENT err_moment_get = ERROR_MOMENT_NO_ERROR;
    int errinfoget = 0;
    char errBufget[0x100] = "";
    fipiBase.dfStructSize = fipiTarget.dfStructSize = sizeof(FILE_IN_PATCH_INFO);

    int retGetPatchInfo = GetSvfMonoPatchInfoStruct(svffile,
        &kdfsf,
        BaseFileName, 0x400, &fipiBase,
        TargetFileName, 0x400, &fipiTarget,
        &err_moment_get, &errinfoget, errBufget, sizeof(errBufget) - 1, NULL, &suggestedExtractMethod, NULL, 0);

    if ((retGetPatchInfo != 1) || ((kdfsf != KINDDFSFILE_FULL) && (kdfsf != KINDDFSFILE_PATCH)))
    {
        fprintf(stderr,"error: invalid SVF file %s\n", svffile);
        return 1;
    }
    fprintf(stderr, "SVF file %s kind %s ", svffile, (kdfsf == KINDDFSFILE_FULL) ? "full" : "patch");
    if (kdfsf == KINDDFSFILE_PATCH)
        fprintf(stderr, "from file %s size %i bytes ", BaseFileName, (int)fipiBase.FileSize);
    fprintf(stderr, "to file %s size %i bytes ", TargetFileName, (int)fipiTarget.FileSize);
    fprintf(stderr, "\n");
    return 0;
}

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

    if ((argc>=3) && (strcmp(argv[1],"monopatchinfo")==0))
        return ShowSvfInfo(argv[2]);

    if ((argc>=3) && (strcmp(argv[1],"monopatchinfoold")==0))
        return ShowSvfInfoOld(argv[2]);

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

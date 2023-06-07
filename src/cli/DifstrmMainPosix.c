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
#include "../lib/engine/patchstream/common/difbasic.h"

#if defined(SMARTVERSION_USE_POSIX)

#include <wchar.h>
#include <locale.h>
#include <langinfo.h>
#include <iconv.h>

#include "../lib/engine/patchstream/common/DfsTlTyp.h"
#include "../lib/engine/patchstream/common/difstool.h"
#include "../lib/engine/patchstream/common/DfsType.h"
#include "../lib/engine/svfile/common/DfsTagDf.h"
#include "../lib/engine/svfile/common/DfsTagMg.h"
#include "../lib/engine/svfile/common/DfsTagBlockFloatEnd.h"


#include "../lib/engine/patchstream/common/DfsOrigMemoryMap.h"
#include "../lib/engine/patchstream/common/DfsIoHlp.h"
#include "../lib/engine/svfile/common/DfsIntf.h"

#include "../lib/engine/patchstream/common/difstrm.h"
#include "../lib/engine/patchstream/decompress/apldifst.h"
#include "../lib/engine/patchstream/compress/makdifst.h"
#include "../lib/engine/patchstream/rebuild/RamDifWk.h"
#include "../lib/engine/patchstream/rebuild/RamDifTl.h"
#include "../lib/engine/patchstream/rebuild/RamDifWS.h"
#include "DfsCdLin.h"
#include "../lib/helper/decompress/ExtractHelper.h"
#include "RawCompress.h"

#if ((!defined(RAWCOMPRESSDIRECT)) && (!defined(SVF_EXTRACT_ONLY)))
#define RAWCOMPRESSDIRECT 1
#endif

#if (!defined(RAWUNCOMPRESSDIRECT))
#define RAWUNCOMPRESSDIRECT 1
#endif

#ifdef RAWCOMPRESSDIRECT
#include "../lib/engine/patchstream/compress/abstractCompress.h"
#endif

#ifdef RAWUNCOMPRESSDIRECT
#include "../lib/engine/patchstream/common/abstractDecompress.h"
#endif

int PerformTestIdentical(int nbarg,char* argvid[])
{
    int i;
    for (i=0;i<nbarg;i++)
    {
        unsigned long size = atol(argvid[i]);
        unsigned char buf[0x100];
        dfuLong32 dfSizeCode,j;
        if (*argvid[i]=='*')
        {
            size = atol((argvid[i])+1);
            for (j=0;j<10000;j++)
              dfSizeCode=MakeDifStFileIdentical((dfvoidp)buf, sizeof(buf), size);
        }

        printf("identical size %8lu: ",size);
        dfSizeCode=MakeDifStFileIdentical((dfvoidp)buf, sizeof(buf), size);
        printf("%3u bytes : ",dfSizeCode);
        for (j=0;j<dfSizeCode;j++)
            printf("%02x ",buf[j]);
        printf("\n");
    }
    return 0;
}

#define BLOCK_SIZE_RAWUNCOMPRESSDIRECT (65536*8)

BOOL DoRawUncompressMemTest(const char*file_in, int nbLoop)
{
	BOOL success = TRUE;
	int res;
	FILE* fin;

	abstract_decompress_stream strm;
	unsigned char* buf = (unsigned char*)DfsMalloc(2 * (BLOCK_SIZE_RAWUNCOMPRESSDIRECT));
	unsigned char* buf_in = buf;
	unsigned char* buf_out = buf + (BLOCK_SIZE_RAWUNCOMPRESSDIRECT);
	int loop;
	if (buf == NULL)
		return FALSE;
	fin = fopen(file_in, "rb");
	if (fin == NULL)
	{
		printf("cannot open %s\n", file_in);
		DfsFree(buf);
		return FALSE;
	}

	fseek(fin, 0, SEEK_END);
	size_t fileSize = (size_t)ftell(fin);

	unsigned char* fileBuff = DfsMalloc((size_t)fileSize + 1);

	fseek(fin, 0, SEEK_SET);
	if (fread(fileBuff, 1, (size_t)fileSize, fin) != (size_t)fileSize)
	{

		printf("cannot read file %s\n", file_in);
		DfsFree(buf);
		DfsFree(fileBuff);
		fclose(fin);
		return FALSE;
	}

	for (loop = 0; loop < nbLoop; loop++)
	{
		//fseek(fin, 0, SEEK_SET);
		size_t pos_read = 0;
		DfsClearStruct(&strm, 0, sizeof(abstract_decompress_stream));
		res = abstract_init_prefix(&strm);
		if (res != ABSTR_COMPRESS_Z_OK)
		{
			printf("cannot init compress\n");
			fclose(fin);
			DfsFree(buf);
			DfsFree(fileBuff);
			return FALSE;
		}
		int err = ABSTR_DECOMPRESS_Z_OK;
		while (success)
		{
			if (pos_read >= fileSize)
				break;
			size_t nb_read = (BLOCK_SIZE_RAWUNCOMPRESSDIRECT < (fileSize - pos_read)) ?
			BLOCK_SIZE_RAWUNCOMPRESSDIRECT : (fileSize - pos_read);
			memcpy(buf_in, fileBuff + pos_read, nb_read);
			pos_read += nb_read;

			strm.avail_in = (uInt)nb_read;
			strm.next_in = buf_in;

			while (success)
			{
				strm.avail_out = (uInt)(BLOCK_SIZE_RAWUNCOMPRESSDIRECT);
				strm.next_out = buf_out;
				err = abstract_decompress(&strm, ABSTR_DECOMPRESS_Z_SYNC_FLUSH);

				if ((err != ABSTR_DECOMPRESS_Z_OK) && (err != ABSTR_DECOMPRESS_Z_STREAM_END))
				{
					printf("ERROR\n");
					break;
				}

				uInt doneOut = (uInt)((BLOCK_SIZE_RAWUNCOMPRESSDIRECT)-strm.avail_out);
				if (doneOut > 0)
				{


				}
				if (doneOut == 0)
					break;
				if (strm.avail_in == 0)
					break;
			}
			if (err == ABSTR_DECOMPRESS_Z_STREAM_END)
				break;
		}

		while (success)
		{
			strm.avail_out = (uInt)(BLOCK_SIZE_RAWUNCOMPRESSDIRECT);
			strm.next_out = buf_out;
			err = abstract_decompress(&strm, ABSTR_DECOMPRESS_Z_FINISH);
			uInt done = (uInt)((BLOCK_SIZE_RAWUNCOMPRESSDIRECT)-strm.avail_out);
			if (done > 0)
			{


			}
			if (done == 0)
				break;
		}
		if (err != ABSTR_DECOMPRESS_Z_STREAM_END)
			printf("Error:\n");
		abstract_decompress_end(&strm);
	}
	fclose(fin);
	DfsFree(fileBuff);
	DfsFree(buf);
	return success;
}


int main(int argc, char *argv[])
{
int iRet=0;
int i;
dfwcharp pCommandLine;
//dfwcharp pCommandLineBrowse;
size_t size_buf_cdline=4;
int iCommandLinePos=0;
BOOL mustContinue;
int retTryCustomMain;
//    SetErrorMode(SEM_NOALIGNMENTFAULTEXCEPT);   // FOR RISC and ia64
    DfsCrc32(0,(void*)" ",1); // init CRC32

    setlocale(LC_CTYPE,"UTF-8");
	retTryCustomMain = TryCustomMain(argc, argv, &mustContinue);
	if (mustContinue == FALSE)
		return retTryCustomMain;

    /*
    if (argc>=2)
        if (strcmp(argv[1],"unictest")==0)
    {
        char sz[80];
        wchar_t szw[80];
        wchar_t szw2[80];
        const wchar_t* pbr;
        const char*pc;
        iconv_t icv;

        printf("%s\n",nl_langinfo(CODESET));
        printf("cur local %s\n",setlocale(LC_CTYPE,""));
        printf("cur local %s\n",setlocale(LC_CTYPE,""));
        setlocale(LC_CTYPE,"UTF-8");
        //setlocale(LC_CTYPE,"UTF-8");
        //setlocale(LC_CTYPE,"fr_FR.UTF-8");
        printf("%s\n",nl_langinfo(CODESET));

        printf("cur local %s\n",setlocale(LC_CTYPE,""));

        printf("cur local %s\n",setlocale(LC_CTYPE,"UTF-8"));

        strcpy(sz,"acc é êç  ä Ä");

        icv=iconv_open("UTF-16",nl_langinfo(CODESET));

        pbr=szw;
        //wcsrtombs(sz,&pbr,80,NULL);
        pc=sz;
        mbstowcs(szw,pc,80);
         printf("ansi '%s\n",sz);
        printf("unic '%ls'\n",szw);

        printf("unic: ");
        pbr=szw;
        while ((*pbr)!=0) printf("%x ",*(pbr++));
        printf("\n");

        printf("char: ");
        pc=sz;
        while ((*pc)!=0) printf("%x ",(unsigned char)(*(pc++)));
        printf("\n");


        {
             pc=sz;
             pbr=szw2;
             size_t pcsize=80;
             size_t pbrsize=80;
               icv=iconv_open("UTF-32",nl_langinfo(CODESET));
               iconv (icv,&pc,&pcsize,&pbr,&pbrsize);
               iconv_close(icv);
        pbr=szw2;
            printf("iconv\n");
        while ((*pbr)!=0) printf("%x ",*(pbr++));
        printf("\ndone iconv\n");
            printf("---\n");
            printf("\n");

        }

    printf("%ls '%lc'\n",L"Wide cara accentué \xe9",0xe9);
    printf("\n");
    printf("%s '%lc'\n","char cara accentué ansi \xe9",0xe9);
    printf("\n %u %c\n",'é','é');
    return 0;
    }
    */

    for (i=1;i<argc;i++)
        size_buf_cdline += (strlen(argv[i])*2) + 4;
    pCommandLine = (dfwcharp)DfsMalloc(size_buf_cdline * sizeof(dfwchar));
    if (pCommandLine == NULL)
        return 1;

    //pCommandLineBrowse=pCommandLine;
    ConvertAnsiToUnicode("smv ",pCommandLine+iCommandLinePos,size_buf_cdline - iCommandLinePos);
    iCommandLinePos += dfUnicodeStrlen(pCommandLine+iCommandLinePos);

#ifdef RAWCOMPRESSDIRECT
	if (argc>3)
		if (strcmp(argv[1], "/compressraw") == 0)
		{
			BOOL success;
			const char* srcFile = argv[2];
			const char* dstFile = argv[3];
			unsigned int ratio = 0;
			int strip_prefix = 0;
			int isStandardzLibWithNegMaxWBits = 0;
			if (argc > 4)
				ratio = (unsigned int)atol(argv[4]);
			if (argc > 5)
				isStandardzLibWithNegMaxWBits = (int)atoi(argv[5]);
			if (argc > 6)
				if ((strcmp(argv[6], "stripprefix") == 0) || (strcmp(argv[6], "strip") == 0))
					strip_prefix = 1;
			success = DoRawCompress(srcFile, dstFile, ratio, isStandardzLibWithNegMaxWBits, strip_prefix);
			if (success)
				printf("compress success\n");
			else
				printf("compress fail\n");
			return success ? 0 : 1;
		}
#endif

#ifdef RAWUNCOMPRESSDIRECT
	if (argc>3)
		if (strcmp(argv[1], "/uncompressraw") == 0)
		{
			BOOL success;
			const char* srcFile = argv[2];
			const char* dstFile = argv[3];
			success = DoRawUncompress(srcFile, dstFile);
			if (success)
				printf("uncompress success\n");
			else
				printf("uncompress fail\n");
			return success ? 0 : 1;
		}

	if (argc>2)
		if (strcmp(argv[1], "/uncompressrawtest") == 0)
		{
			BOOL success;
			const char* srcFile = argv[2];
			int nbLoop = 1;
			if (argc>3)
				nbLoop = atoi(argv[3]);
			success = DoRawUncompressMemTest(srcFile, nbLoop);
			if (success)
				printf("uncompress test success\n");
			else
				printf("uncompress test fail\n");
			return success ? 0 : 1;
		}
#endif

    if (argc>2)
    {
        if (((strcmp(argv[1],"DisplaySvfInfo")==0)) && (argc>2))
        {
            KINDDFSFILE kdfsf;
            char BaseFileName[0x200];
            dfuLong64 SizeBase;
            dfuLong32 crc32Base;
            BOOL fCrc32PresentBase,fMd5PresentBase,fSha1PresentBase;
            unsigned char bMd5Base[16];
            unsigned char bSha1Base[20];
            char TargetFileName[0x200];
            dfuLong64 SizeTarget;
            dfuLong32 crc32Target;
            BOOL fCrc32PresentTarget,fMd5PresentTarget,fSha1PresentTarget;
            unsigned char bMd5Target[16];
            unsigned char bSha1Target[20];
            dfuLong64 dfDeplInPlaceSize,dfDeplOutPlaceSize;
            BOOL fInsertSizeFound;

            ERROR_MOMENT err_moment=ERROR_MOMENT_NO_ERROR;
            int errinfo=0;
            char errBuf[0x100]="";

            int ret = GetSvfMonoPatchInfo(argv[2],
                        &kdfsf,
                        BaseFileName,0x200,&SizeBase,&crc32Base,&fCrc32PresentBase,
                        bMd5Base,&fMd5PresentBase,
                        bSha1Base,&fSha1PresentBase,


                        TargetFileName,0x200,&SizeTarget,&crc32Target,&fCrc32PresentTarget,
                        bMd5Target,&fMd5PresentTarget,
                        bSha1Target,&fSha1PresentTarget,

                        &dfDeplInPlaceSize,&dfDeplOutPlaceSize,&fInsertSizeFound,
                        &err_moment,&errinfo,errBuf,sizeof(errBuf)-1);


            printf("ret = %d , errinfo=%d, err_moment=%x errmsg='%s'\n",ret,errinfo,(int)err_moment,errBuf);
            if (BaseFileName[0]!='\0')
            {
                char szNumSize[40];
                ConvertNum64ToUnformattedStrAnsi(SizeBase,szNumSize,32,13);
                printf("Base : '%s' size=%s",BaseFileName,szNumSize);
                if (fCrc32PresentBase) printf(" crc32=0x%08x",crc32Base);

                printf("\n"/*,BaseFileName*/);
            }
            if (TargetFileName[0]!='\0')
            {
                char szNumSize[40];
                ConvertNum64ToUnformattedStrAnsi(SizeTarget,szNumSize,32,13);
                printf("Target : '%s' size=%s",TargetFileName,szNumSize);
                if (fCrc32PresentTarget) printf(" crc32=0x%08x",crc32Target);

                printf("\n"/*,TargetFileName*/);
            }
            if (fInsertSizeFound)
            {
                char szNumInPlace[40];
                char szNumOutPlace[40];
                char szNumInsertPlace[40];
                char szNumTotal[40];
                ConvertNum64ToUnformattedStrAnsi(dfDeplInPlaceSize,szNumInPlace,32,13);
                ConvertNum64ToUnformattedStrAnsi(dfDeplOutPlaceSize,szNumOutPlace,32,13);
                ConvertNum64ToUnformattedStrAnsi(SizeTarget-(dfDeplInPlaceSize+dfDeplOutPlaceSize),szNumInsertPlace,32,13);
                ConvertNum64ToUnformattedStrAnsi(SizeTarget,szNumTotal,32,13);
                printf("DeplInPlace=%s, DeplOutPlace=%s, Insert=%s for total %s\n",szNumInPlace,szNumOutPlace,szNumInsertPlace,szNumTotal);
            }
            return ret;
        }
    }

    if (argc>4)
    {
        if ((strcmp(argv[1],"ApplyMonofilePatch")==0) ||  (strcmp(argv[1],"ApplyMonofilePatchInPlace")==0) ||  (strcmp(argv[1],"ApplyMonofilePatchInPlaceNoChecksum")==0))
        {
            ERROR_MOMENT err_moment=ERROR_MOMENT_NO_ERROR;
            int errinfo=0;
            char errBuf[0x100]="";

            int ret= ApplyMonofilePatchEx(argv[2],1*0,argv[3],argv[4],&err_moment,&errinfo,errBuf,sizeof(errBuf)-1,
                (strcmp(argv[1],"ApplyMonofilePatchInPlace")==0) ? EXTRACTING_METHOD_INPLACE :
                (strcmp(argv[1],"ApplyMonofilePatchInPlaceNoChecksum")==0) ? EXTRACTING_METHOD_INPLACE_NOCHECKSUM : EXTRACTING_METHOD_CLASSIC);
            printf("ret = %d , errinfo=%d, err_moment=%x errmsg='%s'\n",ret,errinfo,(int)err_moment,errBuf);
            return ret;
        }
    }
    for (i=1;i<argc;i++)
    {
        BOOL fArgContainSpace=FALSE;
        {
            const char* pBrowseArg=argv[i];
            while ((*pBrowseArg)!=0)
            {
                char c=(*pBrowseArg);
                if (c==' ')
                    fArgContainSpace=TRUE;
                pBrowseArg++;
            }
        }
        if (fArgContainSpace)
        {
            ConvertAnsiToUnicode("\"",pCommandLine+iCommandLinePos,size_buf_cdline - iCommandLinePos);
            iCommandLinePos += dfUnicodeStrlen(pCommandLine+iCommandLinePos);
        }

        ConvertAnsiToUnicode(argv[i],pCommandLine+iCommandLinePos,size_buf_cdline - iCommandLinePos);
        iCommandLinePos += dfUnicodeStrlen(pCommandLine+iCommandLinePos);

        if (fArgContainSpace)
        {
            ConvertAnsiToUnicode("\"",pCommandLine+iCommandLinePos,size_buf_cdline - iCommandLinePos);
            iCommandLinePos += dfUnicodeStrlen(pCommandLine+iCommandLinePos);
        }

        ConvertAnsiToUnicode(" ",pCommandLine+iCommandLinePos,size_buf_cdline - iCommandLinePos);
        iCommandLinePos += dfUnicodeStrlen(pCommandLine+iCommandLinePos);
    }
    *(pCommandLine+iCommandLinePos) = 0;
    //DispOutUnicodeString(pCommandLine);
    iRet=PerformCommandLine(pCommandLine);
    DfsFree(pCommandLine);
/*
    {

        if (strcmp(argv[1],"DoTestIdentical")==0)
        {
            return PerformTestIdentical(argc-2,&argv[2]);
        }

        if (strcmp(argv[1],"DoTestRemix")==0)
        {
            dfuLong32 dfParam=0;
            dfwcharp dffnOrig=NULL;
            dfwcharp dffnDest=NULL;

            dfwchar szFnOrig[MAX_PATH+2];
            dfwchar szFnDest[MAX_PATH+2];
            dfuLong32 dfnbVer=0;
            dfuLong32 dfVerTab[200];

            if (argc>2)
              dfParam=(dfuLong32)atol(argv[2]);

            if (argc>3)
            {
                dffnOrig=szFnOrig;
                wsprintfW(dffnOrig,L"%S",argv[3]);
            }

            if (argc>4)
              wsprintfW(dffnDest=szFnDest,L"%S",argv[4]);

            if (argc>5)
            {
                dfuLong32 i;
                dfnbVer = argc-5;
                for (i=0;i<dfnbVer;i++)
                    dfVerTab[i]=(dfuLong32)(atol(argv[i+5]));
            }

            return (TestRemix(dfParam,dffnOrig,dffnDest,dfnbVer,dfVerTab) ? 0 : 1);
        }
    }

    if (argc>3)
    {
        if ((strcmp(argv[1],"DoTestPatchFileNoMapNewBufOld")==0) || (strcmp(argv[1],"DTPFNMNBO")==0))
        {
            dfuLong32 CompressRatio = COMPRESSIONPARAM_AUTOVALUE;
            if (argc>4)
                CompressRatio=atol(argv[4]);
            return (TestPatchFileNoMapNew(argv[2],argv[3],CompressRatio,FALSE,TRUE) ? 0 : 1);
        }

        if ((strcmp(argv[1],"DoTestPatchFileNoMapNew")==0) || (strcmp(argv[1],"DTPFNMN")==0))
        {
            dfuLong32 CompressRatio = COMPRESSIONPARAM_AUTOVALUE;
            if (argc>4)
                CompressRatio=atol(argv[4]);
            return (TestPatchFileNoMapNew(argv[2],argv[3],CompressRatio,TRUE,TRUE) ? 0 : 1);
        }

        if ((strcmp(argv[1],"DoTestPatchFileNoMapNewBufOldNoVirtualAlloc")==0) || (strcmp(argv[1],"DTPFNMNBONVA")==0))
        {
            dfuLong32 CompressRatio = COMPRESSIONPARAM_AUTOVALUE;
            if (argc>4)
                CompressRatio=atol(argv[4]);
            return (TestPatchFileNoMapNew(argv[2],argv[3],CompressRatio,FALSE,FALSE) ? 0 : 1);
        }

        if ((strcmp(argv[1],"DoTestPatchFileNoMapNewNoVirtualAlloc")==0) || (strcmp(argv[1],"DTPFNMNNVA")==0))
        {
            dfuLong32 CompressRatio = COMPRESSIONPARAM_AUTOVALUE;
            if (argc>4)
                CompressRatio=atol(argv[4]);
            return (TestPatchFileNoMapNew(argv[2],argv[3],CompressRatio,TRUE,FALSE) ? 0 : 1);
        }

        if (strcmp(argv[1],"DoTestPatchFile")==0)
        {
            dfuLong32 CompressRatio = COMPRESSIONPARAM_AUTOVALUE;
            if (argc>4)
                CompressRatio=atol(argv[4]);
            return (TestPatchFile(argv[2],argv[3],CompressRatio) ? 0 : 1);
        }

        if (strcmp(argv[1],"DoTestPatchFileRamDif")==0)
        {
            dfuLong32 CompressRatio = COMPRESSIONPARAM_AUTOVALUE;
            if (argc>4)
                CompressRatio=atol(argv[4]);
            return (TestPatchFileRamDif(argv[2],argv[3],CompressRatio) ? 0 : 1);
        }

        if (strcmp(argv[1],"DoTestPatchFileRamDif3")==0)
        {
            dfuLong32 CompressRatio = COMPRESSIONPARAM_AUTOVALUE;
            if (argc>4)
            {
                if (argc>5)
                CompressRatio=atol(argv[5]);
              return (TestPatchFileRamDif3(argv[2],argv[3],argv[4],CompressRatio) ? 0 : 1);
            }
        }
    }

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
*/
    return iRet;
}
#endif

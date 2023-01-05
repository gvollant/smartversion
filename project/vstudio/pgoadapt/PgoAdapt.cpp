// refmt.cpp : Defines the entry point for the console application.
//


#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <string.h>


#define STEPALLOC (0x800)

#define AroundUpper(dwValue,dwModulo) (((((DWORD)(dwValue)) + ((DWORD)(dwModulo)) -1) / ((DWORD)(dwModulo))) * (dwModulo))

BOOL DoLoading(LPTSTR& lpFileContent, DWORD &dwFileSize,LPCTSTR lpFileName,DWORD dwMaxAdd)
{
    BOOL fRet=FALSE;
    DWORD dwFileRead=0;
    HANDLE hFile = CreateFile(lpFileName,GENERIC_READ,FILE_SHARE_READ,
                              NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);
    if (hFile == NULL)
        return FALSE;
    dwFileSize = GetFileSize(hFile,NULL);
    lpFileContent = (LPTSTR)malloc(AroundUpper(dwFileSize+0x100+dwMaxAdd,STEPALLOC));
    if (lpFileContent!=NULL)
        fRet = ReadFile(hFile,lpFileContent,dwFileSize,&dwFileRead,NULL);
    if (fRet)
        fRet = dwFileRead == dwFileSize;
    CloseHandle(hFile);
    return fRet;
}


BOOL DoWriting(LPCTSTR lpFileContent, DWORD dwFileSize,LPCTSTR lpFileName)
{
    BOOL fRet=FALSE;
    DWORD dwFileWrite=0;
    HANDLE hFile = CreateFile(lpFileName,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,
                              NULL,CREATE_ALWAYS,FILE_FLAG_SEQUENTIAL_SCAN,NULL);
    if (hFile == NULL)
        return FALSE;
    if (lpFileContent!=NULL)
        fRet = WriteFile(hFile,lpFileContent,dwFileSize,&dwFileWrite,NULL);
    if (fRet)
        fRet = dwFileWrite == dwFileSize;
    CloseHandle(hFile);
    return fRet;
}

BOOL ReadjustPtr(LPTSTR& lpFileContent, DWORD dwFileSize)
{
    LPTSTR lpFileContentNew = (LPTSTR) realloc (lpFileContent,AroundUpper(dwFileSize+0x100,STEPALLOC));
    if (lpFileContentNew == NULL)
        return FALSE;
    lpFileContent=lpFileContentNew;
    return TRUE;
}

BOOL DoDelete(LPTSTR lpFileContent, DWORD &dwFileSize, DWORD dwPos, DWORD dwNbDelete)
{
    if ((dwPos + dwNbDelete)>=dwFileSize)
    {
        dwFileSize = dwPos;
        return TRUE;
    }
    if (dwNbDelete==0)
        return TRUE;
    dwFileSize -= dwNbDelete;
    memmove(lpFileContent+dwPos,lpFileContent+dwPos+dwNbDelete,dwFileSize-dwPos);
/*
    for (i=dwPos;i<dwFileSize;i++)
    {
        *(lpFileContent+i)=*(lpFileContent+i+dwNbDelete);
    }*/
    return TRUE;
}

BOOL DoAdding(LPTSTR& lpFileContent, DWORD &dwFileSize, DWORD dwPos, DWORD dwNbAdd)
{
    if (dwNbAdd==0)
        return TRUE;
    if (!(ReadjustPtr(lpFileContent,dwFileSize+dwNbAdd)))
        return FALSE;
    memmove(lpFileContent+dwPos+dwNbAdd,lpFileContent+dwPos,dwFileSize-dwPos);

    dwFileSize+=dwNbAdd;
    return TRUE;
}

BOOL DoWorking(LPTSTR& lpFileContent, DWORD& dwFileSize,int iVal)
{
    BOOL fRet=FALSE;
    DWORD dwPos=0;
    const char* szSearch= "WholeProgramOptimization=\"";
    const char* szSearch2="LinkTimeCodeGeneration=\"";
    int ilnSrch = (int)strlen(szSearch);
    int ilnSrch2 = (int)strlen(szSearch2);

    while (dwPos<dwFileSize-ilnSrch)
    {
        if (memcmp(lpFileContent + dwPos, szSearch, ilnSrch)==0)
        {
            if (*(lpFileContent+dwPos+ilnSrch+1)=='"')
            {
                fRet=TRUE;
                *(lpFileContent+dwPos+ilnSrch+0)='0' + (iVal & 0x0f);
            }
        }

        if (memcmp(lpFileContent + dwPos, szSearch2, ilnSrch2)==0)
        {
            if (*(lpFileContent+dwPos+ilnSrch2+1)=='"')
            {
                fRet=TRUE;
                *(lpFileContent+dwPos+ilnSrch2+0)='0' + (iVal & 0x0f);
            }
        }

        dwPos++;
    }
    return fRet;
}

BOOL DoWorking2010(LPTSTR& lpFileContent, DWORD& dwFileSize,const char* newstr)
{
    BOOL fRet=FALSE;
    DWORD dwPos=0;
    const char* szSearchBegin= "<WholeProgramOptimization>";
    const char* szSearchEnd="</WholeProgramOptimization>";
    int ilnSrchBegin = (int)strlen(szSearchBegin);
    int ilnSrchEnd = (int)strlen(szSearchEnd);
	int ilnNewStr = (int)strlen(newstr);
    while (dwPos<dwFileSize-ilnSrchBegin)
    {
        if (memcmp(lpFileContent + dwPos, szSearchBegin, ilnSrchBegin)==0)
        {
			DWORD dwPos2=dwPos;
			while (dwPos2<dwFileSize-ilnSrchEnd)
			{
				if (memcmp(lpFileContent + dwPos2, szSearchEnd, ilnSrchEnd)==0)
				{
					/* stuff */
					DWORD dwNewPos2 = dwPos + ilnSrchBegin + ilnNewStr;
					memmove(lpFileContent+dwNewPos2,lpFileContent+dwPos2,dwFileSize-dwPos2);
					memmove(lpFileContent+dwPos+ilnSrchBegin,newstr,ilnNewStr);
					dwFileSize += (dwNewPos2-(signed)dwPos2);
					fRet=TRUE;

					break;
				}
				dwPos2++;
			}
        }
		/*
        if (memcmp(lpFileContent + dwPos, szSearch2, ilnSrch2)==0)
        {
            if (*(lpFileContent+dwPos+ilnSrch2+1)=='"')
            {
                fRet=TRUE;
                *(lpFileContent+dwPos+ilnSrch2+0)='0' + (iVal & 0x0f);
            }
        }*/

        dwPos++;
    }
    return fRet;
}
int _tmain(int argc, _TCHAR* argv[])
{
    LPTSTR lpFileContent=NULL;
    DWORD dwFileSize=0;
    int iWpoVal = 1;

    printf("PgoAdapt 1.01 - Gilles Vollant 2007-2010 - http://www.winimage.com/misc/PgoAdapt\n");
    if (argc==1)
    {
        printf("\nUsage:\n"\
               "PgoAdapt [project.vcproj] #\n" \
               "  Where [project.vcproj] or [project.vcxproj] is filename of a \n"\
			   "        Visual Studio 2005/2008/2010 project\n" \
               "  # = 1 : for standard LTCG (without Profile-Guided Optimization)\n"
               "  # = 2 : for instrument PGO project\n"\
               "  # = 3 : for optimize PGO project\n" \
               "  # = 4 : for update PGO project\n\n");
    }

    if (argc>1)
    {
        BOOL fModifyDone = FALSE;
        if (argc>3)
        {
            iWpoVal = (int)atol(argv[3]);
        }

		if (argc==3)
		{
			iWpoVal = (int)atol(argv[2]);
		}

		if ((iWpoVal<1) || (iWpoVal>4))
			return 1;

        if (!DoLoading(lpFileContent,dwFileSize,argv[1],0x1000))
            return 1;
		BOOL fRet;
        fRet = DoWorking(lpFileContent,dwFileSize,iWpoVal);
		const char*lpString2010=NULL;

		if (iWpoVal==1)
			lpString2010="true";
		if (iWpoVal==2)
			lpString2010="PGInstrument";
		if (iWpoVal==3)
			lpString2010="PGOptimize";
		if (iWpoVal==3)
			lpString2010="PGUpdate";
		if (lpString2010 != NULL)
			if (DoWorking2010(lpFileContent,dwFileSize,lpString2010))
				fRet=TRUE;
		if (!fRet)
            return 2;
        //if (fModifyDone)
        {
            LPCTSTR lpszFileName = argv[1];
            if (argc > 2)
                if (*(argv[2]) != '*')
                    lpszFileName = argv[2];
            if (!DoWriting(lpFileContent,dwFileSize,lpszFileName))
                return 1;
        }
    }

    return 0;
}

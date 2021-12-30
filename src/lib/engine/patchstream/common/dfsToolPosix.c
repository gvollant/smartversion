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


#include "difbasic.h"

#if defined(SMARTVERSION_USE_POSIX)

#include <stddef.h>
#include <string.h>
#include <stdio.h>
#define noFORCE_USE_WCHAR
#ifdef FORCE_USE_WCHAR

#include <wchar.h>
#endif
#include <sys/mman.h>

#include "DfsTlTyp.h"
#include "difstool.h"


#include <sys/time.h>
#include <pthread.h>
#include <stdlib.h>

/*
hDifTimeElasped SVFAPI SyncDifBuidTimeMarkerObject();
unsigned int SVFAPI SyncDifGetMSecElapsed(hDifTimeElasped ptr);
unsigned int SVFAPI SyncDifGetMSecElapsedNotDestructive(hDifTimeElasped ptr, int destructObject);

typedef void* SYNC_DIF_MUTEX_OBJECT;

SYNC_DIF_MUTEX_OBJECT SVFAPI SyncDifBuildMutex();
void SVFAPI SyncDifGetMutex(SYNC_DIF_MUTEX_OBJECT pMut);
void SVFAPI SyncDifReleaseMutex(SYNC_DIF_MUTEX_OBJECT pMut);
void SVFAPI SyncDifDeleteMutex(SYNC_DIF_MUTEX_OBJECT pMut);
*/



typedef struct
{
    time_t tm_t;
    struct timeval tp;
} TIMEBEGIN;

hDifTimeElasped SVFAPI SyncDifBuidTimeMarkerObject()
{
    TIMEBEGIN* pBegin = (TIMEBEGIN*)malloc(sizeof(TIMEBEGIN));
    pBegin->tm_t = time(NULL);
    gettimeofday(&(pBegin->tp),NULL);
    return (hDifTimeElasped)pBegin;
}


unsigned int SVFAPI SyncDifGetMSecElapsedNotDestructive(hDifTimeElasped ptr, int destructObject)
{
    TIMEBEGIN tBegin;
    TIMEBEGIN tEnd;
    TIMEBEGIN* pBegin = (TIMEBEGIN*)ptr;
    unsigned int iRet;
    tBegin = *pBegin;
    tEnd.tm_t = time(NULL);
    gettimeofday(&(tEnd.tp),NULL);
    if (destructObject != 0)
      free(pBegin);

    iRet = (unsigned int)(((tEnd.tp.tv_sec*1000) + (tEnd.tp.tv_usec/1000)) -
                 ((tBegin.tp.tv_sec*1000) + (tBegin.tp.tv_usec/1000))) ;
    return iRet;
}


unsigned int SVFAPI SyncDifGetMSecElapsed(hDifTimeElasped ptr)
{
    return SyncDifGetMSecElapsedNotDestructive(ptr,1);
}





/*
Mutex implementation for Posix API
 (Linux, MacOS X, BSD...)
Documentation about posix thread API
http://manpages.ubuntu.com/manpages/dapper/fr/man3/pthread_mutex_init.3.html
http://developer.apple.com/documentation/Darwin/Reference/Manpages/man3/pthread_mutex_init.3.html
http://www.linux-kheops.com/doc/man/manfr/man-html-0.9/man3/pthread_mutex_init.3.html
*/

typedef struct
{
    pthread_mutex_t pmt;
} SYNC_MUTEX_OBJECT_INTERNAL;

SYNC_DIF_MUTEX_OBJECT SVFAPI SyncDifBuildMutex()
{
    int retCreate;
    SYNC_MUTEX_OBJECT_INTERNAL* pMoi = (SYNC_MUTEX_OBJECT_INTERNAL*)malloc(sizeof(SYNC_MUTEX_OBJECT_INTERNAL));
    if (pMoi == NULL)
        return NULL;
    retCreate = pthread_mutex_init(&pMoi->pmt,NULL);
    if (retCreate != 0)
    {
        free(pMoi);
        return NULL;
    }

    return (SYNC_DIF_MUTEX_OBJECT)pMoi;
}

void SVFAPI SyncDifGetMutex(SYNC_DIF_MUTEX_OBJECT pMut)
{
    SYNC_MUTEX_OBJECT_INTERNAL* pMoi = (SYNC_MUTEX_OBJECT_INTERNAL*)pMut;
    if (pMoi != NULL)
        pthread_mutex_lock(&pMoi->pmt);
}

void SVFAPI SyncDifReleaseMutex(SYNC_DIF_MUTEX_OBJECT pMut)
{
    SYNC_MUTEX_OBJECT_INTERNAL* pMoi = (SYNC_MUTEX_OBJECT_INTERNAL*)pMut;
    if (pMut != NULL)
        pthread_mutex_unlock(&pMoi->pmt);
}

void SVFAPI SyncDifDeleteMutex(SYNC_DIF_MUTEX_OBJECT pMut)
{
    SYNC_MUTEX_OBJECT_INTERNAL* pMoi = (SYNC_MUTEX_OBJECT_INTERNAL*)pMut;
    if (pMoi != NULL)
    {
        pthread_mutex_destroy(&pMoi->pmt);
        free(pMoi);
    }
}




#define STD_BUF_CHARCNV (0x3ff)
/* dfwcharp is a pointer to UTF16LE string */


#ifdef FORCE_USE_WCHAR
/* be better with mbstowcs see http://www.delorie.com/gnu/docs/glibc/libc_97.html
   using wchar_t type (32 bits) ;
   or http://www.delorie.com/gnu/docs/glibc/libc_93.html mbsrtowcs  */
BOOL ConvertAnsiToUnicodeEx(const char *lpszAnsi, dfwcharp lpszUnicode,
                            dfuLong32 dfUnicodeSize,
                            BOOL fTransformSlashToOperatingSystemCompatibleSlash)
{
    dfuLong32 i;
    mbstate_t state;
    wchar_t szBuf_wchar[STD_BUF_CHARCNV+1];
    wchar_t* pszBuf_wchar_Use=szBuf_wchar;
    int iWanted_wchar_buf_size=(strlen(lpszAnsi)*4)+0x100;

    if (iWanted_wchar_buf_size>=STD_BUF_CHARCNV)
        pszBuf_wchar_Use=(wchar_t*)DfsMalloc((iWanted_wchar_buf_size+1)*sizeof(wchar_t));



    memset (&state, '\0', sizeof (mbstate_t));
    mbsrtowcs(pszBuf_wchar_Use,&lpszAnsi,iWanted_wchar_buf_size,&state);
    *(pszBuf_wchar_Use+iWanted_wchar_buf_size)=0;

    for (i=0;i<dfUnicodeSize;i++)
    {
        DFWCHAR_BYTEACCESS dfwb;
        wchar_t c=*(pszBuf_wchar_Use+i);
        //dfwchar wc;
        if (fTransformSlashToOperatingSystemCompatibleSlash)
            if ((c=='\\'))
                c='/';

        dfwb.u.b[0]=(dfbyte)c;
        dfwb.u.b[1]=(dfbyte)(c>>8);

        *(lpszUnicode+i) = dfwb.u.wc;
        if (c==0)
            break;
    }
    if (i==dfUnicodeSize)
        *(lpszUnicode+i-1)=0;

    if (pszBuf_wchar_Use!=szBuf_wchar)
        DfsFree(pszBuf_wchar_Use);

    return TRUE;
}

BOOL ConvertUnicodeToAnsiEx(dfwcharpc str, char *lpszAnsi,
                            dfuLong32 dfAnsiSize,
                            BOOL fTransformSlashToOperatingSystemCompatibleSlash)
{
    dfuLong32 i;
    wchar_t szStdConvertBuffer[STD_BUF_CHARCNV+1];
    wchar_t* pUseBuf;
    dfuLong32 dfUnicSize = dfUnicodeStrlen(str);
    BOOL fRet=TRUE;

    if (dfUnicSize>=STD_BUF_CHARCNV)
        pUseBuf = (wchar_t*)DfsMalloc((dfUnicSize+1)*sizeof(wchar_t));
    else
        pUseBuf = szStdConvertBuffer;

    for (i=0;i<=dfUnicSize;i++)
    {
        DFWCHAR_BYTEACCESS dfwb;
        wchar_t w;
        dfwb.u.wc = *(str+i);
        w = dfwb.u.b[0] | (((wchar_t)dfwb.u.b[1])<<8);
        if (fTransformSlashToOperatingSystemCompatibleSlash)
            if ((w=='\\'))
                w='/';
        *(pUseBuf+i) = w;
    }


    {
        const wchar_t *srcBrowse;
        size_t ret;
        //pUseBufAnsi = szStdAnsiConvertBuffer;
        mbstate_t state;


        srcBrowse = pUseBuf;
        memset (&state, '\0', sizeof (mbstate_t));

        ret = wcsrtombs(lpszAnsi,&srcBrowse,dfAnsiSize,&state);
        if (ret<0)
        {
            fRet=FALSE;
            *lpszAnsi=0;
        }
        if ((ret>=dfAnsiSize) && (dfAnsiSize>0))
            *(lpszAnsi+dfAnsiSize-1)=0;
        /*
        if (ret==STD_BUF_CHARCNV)
        {
            pUseBufAnsi = (char*)DfsMalloc((dfUnicSize*16)+4);
            srcBrowse = pUseBuf;
            ret = wcsrtombs(pUseBufAnsi,&srcBrowse,(dfUnicSize*16),NULL);
            *(pUseBufAnsi+(dfUnicSize*16)+1)=0;
        }*/
    }

/*

    for (i=0;i<dfAnsiSize;i++)
    {
        DFWCHAR_BYTEACCESS dfwb;
        char c;
        dfwchar wc;

        wc = *(lpszUnicode+i);
        dfwb.u.wc=wc;
        c=dfwb.u.b[0];

        *(lpszAnsi+i)=c;
        if (c==0)
            break;
    }
*/
    return fRet;
}
#else
// must add utf8 and checdk low endian/big endian




#define GetUtf8Size(ch)  \
        (((((unsigned char)(ch)) & ((unsigned char)0x80))==((unsigned char)0x00)) ? 1 : \
        (((((unsigned char)(ch)) & ((unsigned char)0xe0))==((unsigned char)0xc0)) ? 2 : \
        (((((unsigned char)(ch)) & ((unsigned char)0xf0))==((unsigned char)0xe0)) ? 3 : \
        (((((unsigned char)(ch)) & ((unsigned char)0xf8))==((unsigned char)0xf0)) ? 4 : \
        (((((unsigned char)(ch)) & ((unsigned char)0xfc))==((unsigned char)0xf8)) ? 5 : \
        (((((unsigned char)(ch)) & ((unsigned char)0xfe))==((unsigned char)0xfc)) ? 6 : 001))))))


#define GetUtf8Mask(ch)  \
        (((((unsigned char)(ch)) & ((unsigned char)0x80))==((unsigned char)0x00)) ? ((unsigned char)0x7f) : \
        (((((unsigned char)(ch)) & ((unsigned char)0xe0))==((unsigned char)0xc0)) ? ((unsigned char)0x1f) : \
        (((((unsigned char)(ch)) & ((unsigned char)0xf0))==((unsigned char)0xe0)) ? ((unsigned char)0x0f) : \
        (((((unsigned char)(ch)) & ((unsigned char)0xf8))==((unsigned char)0xf0)) ? ((unsigned char)0x07) : \
        (((((unsigned char)(ch)) & ((unsigned char)0xfc))==((unsigned char)0xf8)) ? ((unsigned char)0x03) : \
        (((((unsigned char)(ch)) & ((unsigned char)0xfe))==((unsigned char)0xfc)) ? ((unsigned char)0x01) : 0))))))


BOOL ConvertAnsiToUnicodeEx(const char *lpszAnsi, dfwcharp lpszUnicode,
                            dfuLong32 dfUnicodeSize,
                            BOOL fTransformSlashToOperatingSystemCompatibleSlash)
{
    const unsigned char*src_walk=(const unsigned char*)lpszAnsi;
    size_t size_this_string_written=0;
    size_t nb_pack_read=0;
    if (src_walk==NULL)
        return FALSE;
    for (;;)
    {
        unsigned char ch = *(src_walk++);
        unsigned short c;

        nb_pack_read++;
        if ((ch&0x80) == 0)
        {
            if (fTransformSlashToOperatingSystemCompatibleSlash)
                if (ch=='\\')
                    ch='/';
            c=ch;
        }
        else
        {
            c=ch & GetUtf8Mask(ch);
            int nbbyte=GetUtf8Size(ch);

            for(;;)
            {
                nbbyte--;
                if (nbbyte==0)
                    break;

                c = (unsigned short)((c<<6) | ( (*(src_walk++)) & 0x3F));

                nb_pack_read++;
            }
        }

        if ((lpszUnicode!=NULL) && (size_this_string_written<dfUnicodeSize))
        {
            DFWCHAR_BYTEACCESS dfwb;

            dfwb.u.b[0]=(dfbyte)c;
            dfwb.u.b[1]=(dfbyte)(c>>8);

            *(lpszUnicode + size_this_string_written)=dfwb.u.wc;
        }
        size_this_string_written++;

        if (c==0)
        {
            return TRUE;
        }
    }
}

BOOL ConvertAnsiToUnicodeExSimple(const char *lpszAnsi, dfwcharp lpszUnicode,
                            dfuLong32 dfUnicodeSize,
                            BOOL fTransformSlashToOperatingSystemCompatibleSlash)
{
    dfuLong32 i;



    for (i=0;i<dfUnicodeSize;i++)
    {
        dfwchar c=*(lpszAnsi+i);
        //dfwchar wc;
        if (fTransformSlashToOperatingSystemCompatibleSlash)
            if (c=='\\')
                c='/';


        *(lpszUnicode+i) = c;
        if (c==0)
            break;
    }
    if (i==dfUnicodeSize)
        *(lpszUnicode+i-1)=0;


    return TRUE;
}


BOOL ConvertUnicodeToAnsiEx(dfwcharpc str, char *lpszAnsi,
                            dfuLong32 dfAnsiSize,
                            BOOL fTransformSlashToOperatingSystemCompatibleSlash)
{
    unsigned short c;
    unsigned char* dst_write=(unsigned char*)lpszAnsi;
    dfuLong32 size_buf_walk=dfAnsiSize;
    dfuLong32 size_needed=0;
    //const unichar empty_string [] = { 0 };
    if (str == NULL)
    {
        //str = empty_string ;
        if ((dst_write!=NULL) && (size_buf_walk>=1))
            *((unsigned char*)dst_write)=0x0;
        return TRUE;
    }

    do
    {
        DFWCHAR_BYTEACCESS dfwb;
        dfwb.u.wc = *(str++);
        c = (unsigned short)(dfwb.u.b[0] | (((unsigned short)dfwb.u.b[1])<<8));


        if (fTransformSlashToOperatingSystemCompatibleSlash)
            if (c=='\\')
                c='/';

        if (c<=0x7F) {
            if ((dst_write!=NULL) && (size_buf_walk>=1))
            {
                *(dst_write++)=(unsigned char)c;
                size_buf_walk--;
            }
            size_needed += 1;
        }
        else
        if (c<=0x7FF) {
            if ((dst_write!=NULL) && (size_buf_walk>=2))
            {
               *(dst_write++)=(unsigned char) (0xC0 | (c>>6));
               *(dst_write++)=(unsigned char) (0x80 | (c & 0x3F));
               size_buf_walk-=2;
            }
            size_needed += 2;
        }
        else
        {
            if ((dst_write!=NULL) && (size_buf_walk>=3))
            {
                *(dst_write++)=(unsigned char) (0xE0 | (c>>12));
                *(dst_write++)=(unsigned char) (0x80 | ((c>>6)&0x3F));
                *(dst_write++)=(unsigned char) (0x80 | (c&0x3F));
                size_buf_walk-=3;
            }
            size_needed += 3;
        }

    } while (c != 0);
    return TRUE;
}


BOOL ConvertUnicodeToAnsiExSml(dfwcharpc str, char *lpszAnsi,
                            dfuLong32 dfAnsiSize,
                            BOOL fTransformSlashToOperatingSystemCompatibleSlash)
{
    dfuLong32 i;
    BOOL fRet=TRUE;


    for (i=0;i<=dfAnsiSize;i++)
    {
        dfwchar w=(*(str++));
        if (fTransformSlashToOperatingSystemCompatibleSlash)
            if (w=='\\')
                w='/';
        *(lpszAnsi+i) = (char)w;
        if (w=='\0')
            break;
    }

    if (i==dfAnsiSize)
        *(lpszAnsi+i-1)=0;

    return fRet;
}

#endif


BOOL ConvertAnsiToUnicode(const char *lpszAnsi, dfwcharp lpszUnicode,
                          dfuLong32 dfUnicodeSize)
{
    return ConvertAnsiToUnicodeEx(lpszAnsi, lpszUnicode,
                          dfUnicodeSize,
                          FALSE);
}

BOOL ConvertUnicodeToAnsi(dfwcharpc lpszUnicode, char *lpszAnsi,
                          dfuLong32 dfAnsiSize)
{
    return ConvertUnicodeToAnsiEx(lpszUnicode,lpszAnsi,dfAnsiSize,FALSE);
}


int dfUnicodeStrcmpi(dfwcharpc str1, dfwcharpc str2)
{

  BOOL fRet=FALSE;

    char* lpszAnsiStr1;
    char* lpszAnsiStr2;
    dfuLong32 dwAlloc1 = (dfUnicodeStrlen(str1) * 4) + 0x10;
    dfuLong32 dwAlloc2 = (dfUnicodeStrlen(str2) * 4) + 0x10;
    lpszAnsiStr1 = (char* )DfsMalloc(dwAlloc1 + 0x10);
    lpszAnsiStr2 = (char* )DfsMalloc(dwAlloc2 + 0x10);

    ConvertUnicodeToAnsi(str1, lpszAnsiStr1, dwAlloc1);
    ConvertUnicodeToAnsi(str2, lpszAnsiStr2, dwAlloc2);
    fRet = strcasecmp(lpszAnsiStr1, lpszAnsiStr2);
    DfsFree(lpszAnsiStr1);
    DfsFree(lpszAnsiStr2);


  return fRet;
}

int dfUnicodeStrcmp(dfwcharpc str1, dfwcharpc str2)
{
  BOOL fRet=FALSE;

    char* lpszAnsiStr1;
    char* lpszAnsiStr2;
    dfuLong32 dwAlloc1 = (dfUnicodeStrlen(str1) * 4) + 0x10;
    dfuLong32 dwAlloc2 = (dfUnicodeStrlen(str2) * 4) + 0x10;
    lpszAnsiStr1 = (char* )DfsMalloc(dwAlloc1 + 0x10);
    lpszAnsiStr2 = (char* )DfsMalloc(dwAlloc2 + 0x10);

    ConvertUnicodeToAnsi(str1, lpszAnsiStr1, dwAlloc1);
    ConvertUnicodeToAnsi(str2, lpszAnsiStr2, dwAlloc2);
    fRet = strcmp(lpszAnsiStr1, lpszAnsiStr2);
    DfsFree(lpszAnsiStr1);
    DfsFree(lpszAnsiStr2);


  return fRet;
}


int CompareUnicodeAndAnsiString(dfwcharpc str1,const char* lpszAnsi,BOOL fCaseSensitive)
{
    int iRet;

    {
        dfwcharp pszUnic;
        dfuLong32 dfLenAnsi = (dfuLong32)strlen(lpszAnsi);

        pszUnic = (dfwcharp)DfsMalloc((sizeof(dfwchar)*dfLenAnsi*4)+0x110);


        ConvertAnsiToUnicode(lpszAnsi, pszUnic, (dfLenAnsi*4)+0x100);


        iRet = fCaseSensitive ? dfUnicodeStrcmp(str1,pszUnic) : dfUnicodeStrcmpi(str1,pszUnic);

        DfsFree(pszUnic);
    }

  return iRet;
}



    unsigned long dfPhysicalMemoryKBvalue=0;



#if (!defined(PREVENT_COMPILE_LZMASDK)) && defined(HAVE_LZMASDK_DECOMPRESS)
#include "tuklib_physmem.h"
dfuLong32 GetPhysicalMemoryKb()
{
    if (dfPhysicalMemoryKBvalue!=0)
        return (dfuLong32)dfPhysicalMemoryKBvalue;
    else
    {
        dfuLong32 phmem = (dfuLong32)(tuklib_physmem()/1024);
        if (sizeof(void*) == 4)
        {
          if (phmem>(3*1024*1024))
            phmem=(3*1024*1024);
        }
        return phmem;
    }
}
#else

dfuLong32 GetPhysicalMemoryKb()
{
    if (dfPhysicalMemoryKBvalue!=0)
        return (dfuLong32)dfPhysicalMemoryKBvalue;
    else
        return 256*1024;
}
#endif






char SVFAPI My_getch_console_char()
{
    return (char)getchar();
}


dfwchar SVFAPI My_getch_console()
{
    char cTab[8];
    dfwchar wTab[8];
    wTab[0] = wTab[1] = 0;
    cTab[0] = cTab[1] = 0;
    cTab[0] = My_getch_console_char();
    ConvertAnsiToUnicode(cTab,wTab,2);
    return wTab[0];
}

BOOL ConvertNum64ToUnformattedStrAnsi(dfuLong64 dfNum,char* szAnsi, dfuLong32 dfBufSize,dfuLong32 dfMinimalTextSize)
{
    int i,j;
    char szNumCompute[32];
    int iLastWritten=0;
    int nbSpaceAtBeginning=0;
    dfuLong64 dfuCurCompute = dfNum;

    szNumCompute[0]='0';
    i=0;

    for (;;)
    {
        int iCurNumber = (int)(dfuCurCompute % 10);
        if (iCurNumber != 0)
            iLastWritten=i;
        szNumCompute[i]=(char)('0'+iCurNumber);
        dfuCurCompute = (dfuCurCompute - iCurNumber)/10;
        if (dfuCurCompute==0)
            break;

        i++;
        if (i>=18)
            break;
    }


    if ((iLastWritten+1)<dfMinimalTextSize)
        nbSpaceAtBeginning = dfMinimalTextSize - (iLastWritten+1);

    for (j=0;j<nbSpaceAtBeginning;j++)
        szAnsi[j]=' ';

    for (j=0;j<=iLastWritten;j++)
        szAnsi[j+nbSpaceAtBeginning] = szNumCompute[iLastWritten-j];
    szAnsi[j+nbSpaceAtBeginning] = 0;

    return TRUE;
}


BOOL ConvertNum64ToUnformattedStrUnicode(dfuLong64 dfNum,dfwchar* szUnicode, dfuLong32 dfBufSize,dfuLong32 dfMinimalTextSize)
{
    char szAnsi[32];
    if (!ConvertNum64ToUnformattedStrAnsi(dfNum,szAnsi,(dfBufSize < 32) ? dfBufSize : 32,dfMinimalTextSize))
        return FALSE;

    ConvertAnsiToUnicode(szAnsi,szUnicode,dfBufSize);
    return TRUE;
}


dfwcharpc GetUnicodeStringDirectorySeparator()
{
    return GetUnicodeStringSlashSeparator();
}


#include <stdio.h>


void DispOutUnicodeString_useConvertUnicodeToAnsiEx(dfwcharpc str)
{
    char szStdConvertBuffer[STD_BUF_CHARCNV+1];
    dfuLong32 dfUnicSize = dfUnicodeStrlen(str);
    dfuLong32 dfAnsiBufNeedSize = (dfUnicSize*3)+8;
    char* pUseBuf;

    if (dfAnsiBufNeedSize>=STD_BUF_CHARCNV)
        pUseBuf = (char*)DfsMalloc(dfAnsiBufNeedSize*sizeof(char));
    else
        pUseBuf = szStdConvertBuffer;
    ConvertUnicodeToAnsiEx(str,pUseBuf,dfAnsiBufNeedSize,FALSE);
    printf("%s",pUseBuf);
    if (pUseBuf != szStdConvertBuffer)
        DfsFree(pUseBuf);
}

#ifdef FORCE_USE_WCHAR

void DispOutUnicodeString_UNICODE(dfwcharpc str)
{
    wchar_t szStdConvertBuffer[STD_BUF_CHARCNV+1];
    dfuLong32 dfUnicSize = dfUnicodeStrlen(str);
    dfuLong32 i;
    wchar_t* pUseBuf;

    if (dfUnicSize>=STD_BUF_CHARCNV)
        pUseBuf = (wchar_t*)DfsMalloc((dfUnicSize+2)*sizeof(wchar_t));
    else
        pUseBuf = szStdConvertBuffer;

    for (i=0;i<=dfUnicSize;i++)
    {
        DFWCHAR_BYTEACCESS dfwb;
        dfwb.u.wc = *(str+i);
        *(pUseBuf+i) = dfwb.u.b[0] | (((wchar_t)dfwb.u.b[1])<<8);
    }
    printf("%S",pUseBuf);
    if (pUseBuf != szStdConvertBuffer)
        DfsFree(pUseBuf);
}



void DispOutUnicodeString(dfwcharpc str)
{
    wchar_t szStdConvertBuffer[STD_BUF_CHARCNV+1];
    //char szStdAnsiConvertBuffer[STD_BUF_CHARCNV+1];
    dfuLong32 dfUnicSize = dfUnicodeStrlen(str);
    dfuLong32 i;
    wchar_t* pUseBuf;
    //char* pUseBufAnsi;
    if (str==NULL)
        return ;

    if (dfUnicSize>=STD_BUF_CHARCNV)
        pUseBuf = (wchar_t*)DfsMalloc((dfUnicSize+2)*sizeof(wchar_t));
    else
        pUseBuf = szStdConvertBuffer;

    for (i=0;i<=dfUnicSize;i++)
    {
        DFWCHAR_BYTEACCESS dfwb;
        dfwb.u.wc = *(str+i);
        *(pUseBuf+i) = dfwb.u.b[0] | (((wchar_t)dfwb.u.b[1])<<8);
    }

#ifndef NO_UNICODE_PRINTF
    printf("%S",pUseBuf);
#else
    {
        const wchar_t *srcBrowse;
        size_t ret;

        mbstate_t state;
        memset (&state, '\0', sizeof (mbstate_t));

        pUseBufAnsi = szStdAnsiConvertBuffer;


        srcBrowse = pUseBuf;
        ret = wcsrtombs(pUseBufAnsi,&srcBrowse,STD_BUF_CHARCNV,&state);
        if (ret==STD_BUF_CHARCNV)
        {
            pUseBufAnsi = (char*)DfsMalloc((dfUnicSize*16)+4);
            srcBrowse = pUseBuf;
            memset (&state, '\0', sizeof (mbstate_t));
            ret = wcsrtombs(pUseBufAnsi,&srcBrowse,(dfUnicSize*16),&state);
            *(pUseBufAnsi+(dfUnicSize*16)+1)=0;
        }
    }
    printf("%s",pUseBufAnsi);
    if (pUseBufAnsi != szStdAnsiConvertBuffer)
        DfsFree(pUseBufAnsi);
#endif

    //wcsnrtombs
    if (pUseBuf != szStdConvertBuffer)
        DfsFree(pUseBuf);
}
#else

void DispOutUnicodeString(dfwcharpc str)
{
    return DispOutUnicodeString_useConvertUnicodeToAnsiEx(str);
}
#endif

/*
#include <cwchar>
#include <clocale> //for setlocale
#include <cstdlib> //for SUCCESS

void DispOutUnicodeString_UNICODE(dfwcharpc str)
{
    wchar_t szStdConvertBuffer[STD_BUF_CHARCNV+1];
    dfuLong32 dfUnicSize = dfUnicodeStrlen(str);
    dfuLong32 i;
    wchar_t* pUseBuf;

    if (dfUnicSize>=STD_BUF_CHARCNV)
        pUseBuf = (wchar_t*)DfsMalloc((dfUnicSize+2)*sizeof(wchar_t));
    else
        pUseBuf = szStdConvertBuffer;

    for (i=0;i<=dfUnicSize;i++)
    {
        DFWCHAR_BYTEACCESS dfwb;
        dfwb.u.wc = *(str+i);
        *(pUseBuf+i) = dfwb.u.b[0] | (((wchar_t)dfwb.u.b[1])<<8);
    }
    std::wprintf(pUseBuf);
    if (pUseBuf != szStdConvertBuffer)
        DfsFree(pUseBuf);
}
*/

#endif

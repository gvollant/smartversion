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

#include <unistd.h>
/* DfsIOhlpPosix.c */
/* visit http://www.delorie.com/gnu/docs/glibc/libc_toc.html */
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>
#include <errno.h>
#include <wchar.h>
#include <dirent.h>

#include "DfsTlTyp.h"
#include "difstool.h"
#include "DfsOrigMemoryMap.h"
#include "DfsIoHlp.h"
#include "DfsIOHlpInternal.h"
#include "ltoolsc.h"


#ifndef DEV_MOBILE
#if defined(ANDROID)
#define DEV_MOBILE 1
#endif
#endif

#ifndef DEV_MOBILE
#ifdef TARGET_OS_IPHONE
#if TARGET_OS_IPHONE
#define DEV_MOBILE 1
#endif
#endif
#endif


#if (!defined(DIFSTRM_USE_FILE64_POSIX_FUNCTION)) && (!defined(PREVENT_DIFSTRM_USE_FILE64_POSIX_FUNCTION))

  # if defined(_LARGEFILE64_SOURCE) && !defined(__APPLE__) && \
  (!defined(_FILE_OFFSET_BITS) || _FILE_OFFSET_BITS != 64) || \
  defined(_AIX) && !defined(_LARGE_FILES) || \
  defined(BOOST_IOSTREAMS_HAS_LARGE_FILE_EXTENSIONS)
    #define DIFSTRM_USE_FILE64_POSIX_FUNCTION 1
  #endif

#endif
/*
boost does:


# if defined(_LARGEFILE64_SOURCE) && !defined(__APPLE__) && \
(!defined(_FILE_OFFSET_BITS) || _FILE_OFFSET_BITS != 64) || \
defined(_AIX) && !defined(_LARGE_FILES) || \
defined(BOOST_IOSTREAMS_HAS_LARGE_FILE_EXTENSIONS)


// Systems with transitional extensions for large file support

#  define BOOST_IOSTREAMS_FD_SEEK      lseek64
#  define BOOST_IOSTREAMS_FD_TRUNCATE  ftruncate64
#  define BOOST_IOSTREAMS_FD_MMAP      mmap64
#  define BOOST_IOSTREAMS_FD_STAT      stat64
#  define BOOST_IOSTREAMS_FD_FSTAT     fstat64
#  define BOOST_IOSTREAMS_FD_OFFSET    off64_t
# else
#  define BOOST_IOSTREAMS_FD_SEEK      lseek
#  define BOOST_IOSTREAMS_FD_TRUNCATE  ftruncate
#  define BOOST_IOSTREAMS_FD_MMAP      mmap
#  define BOOST_IOSTREAMS_FD_STAT      stat
#  define BOOST_IOSTREAMS_FD_FSTAT     fstat
#  define BOOST_IOSTREAMS_FD_OFFSET    off_t
# endif
#endif

*/

#ifndef DIFSTRM_OFF_T_TYPE
#define DIFSTRM_OFF_T_TYPE dfuLong64
#endif

#ifdef DIFSTRM_USE_FILE64_POSIX_FUNCTION

static inline void* my_mmap(void *addr, dfuIntPtr len, int prot, int flags, int fildes, dfuLong64 off)
{
	return mmap64(addr, len, prot, flags, fildes, off);
}


static inline int my_ftruncate(int fd, dfuLong64 length)
{
	return ftruncate64(fd, length);
}

static inline DIFSTRM_OFF_T_TYPE my_lseek(int fd, DIFSTRM_OFF_T_TYPE offset, int whence)
{
	return lseek64(fd, offset, whence);
}

#define my_stat_struct stat64
static inline int my_stat(const char *pathname, struct my_stat_struct *buf)
{
	return stat64(pathname, buf);
}

#else

static inline void* my_mmap(void *addr, dfuIntPtr len, int prot, int flags, int fildes, dfuLong64 off)
{
	return mmap(addr, len, prot, flags, fildes, off);
}


static inline int my_ftruncate(int fd, dfuLong64 length)
{
	return ftruncate(fd, length);
}

static inline DIFSTRM_OFF_T_TYPE my_lseek(int fd, DIFSTRM_OFF_T_TYPE offset, int whence)
{
	return lseek(fd, offset, whence);
}

#define my_stat_struct stat
static inline int my_stat(const char *pathname, struct my_stat_struct *buf)
{
	return stat(pathname, buf);
}
#endif
/*
#if defined(__off64_t_defined) || defined(LINUX)
#define DIFSTRM_OFF_T_TYPE __off64_t
#define LSEEK_FUNC(fildes, offset, whence) lseek((fildes), (offset), (whence))
#define FTRUNCATE_FUNC(fd, length) (ftruncate((fd), (length)))
#else
#define DIFSTRM_OFF_T_TYPE off_t
#define LSEEK_FUNC(fildes, offset, whence) (lseek((fildes), (offset), (whence)))
#define FTRUNCATE_FUNC(fd, length) (ftruncate((fd), (length)))
#endif
*/

BOOL BuildErrorInfoErrNo(int errno_code,ERROR_INFO_ITEM* peii)
{
    dfwcharp lpBuffer=NULL;

    if (errno_code != 0)
    {
        //dfwchar szErrTxt[(MAX_PATH*2)+1];
        dfuLong32 dwNbChar=0;
        char* strerror_res = strerror (errno_code);

        if (strerror_res!=NULL)
        {
          dwNbChar=(dfuLong32)strlen(strerror_res);
          lpBuffer=(dfwcharp)DfsMalloc((dwNbChar*4)+0x10);

          if (lpBuffer!=NULL)
          {
            ConvertAnsiToUnicode(strerror_res,(lpBuffer),(dwNbChar+1)*2);

            peii->dfInfoTag = DFS_ERRORTAG_ERRORMSG;
            peii->pInfo = (dfbytep)(lpBuffer);
            peii->dfSize = (dfUnicodeStrlen((dfwcharp)lpBuffer)+1)*2;
            return TRUE;
          }
        }
    }
    return FALSE;
}

BOOL SVFAPI FillErrNoFileName(dfwcharpc dfFileName,int errno_code,H_ERROR_INFO * pei)
{
    ERROR_INFO_ITEM eii[3];
    dfuLong32 dfNbErrorItem=0;
    BOOL fFirstWinError = FALSE;
    if (pei == NULL)
        return FALSE;
    if ((*pei) != NULL)
        return FALSE;
    // now create error

    if (errno_code != 0)
    {
        fFirstWinError = BuildErrorInfoErrNo(errno_code,&eii[dfNbErrorItem]);
        if (fFirstWinError)
                dfNbErrorItem++;
    }

    if (dfFileName!=NULL)
    {
        {
            eii[dfNbErrorItem].dfInfoTag = DFS_ERRORTAG_FILENAME;
            eii[dfNbErrorItem].pInfo = (dfbytep)dfFileName;
            eii[dfNbErrorItem].dfSize = (dfUnicodeStrlen(dfFileName)+1)*2;
            dfNbErrorItem++;
        }
    }
    {
        eii[dfNbErrorItem].dfInfoTag = DFS_ERRORTAG_ERRNONUMBER;
        eii[dfNbErrorItem].pInfo = (dfbytep)&errno_code;
        eii[dfNbErrorItem].dfSize = sizeof(int);
        dfNbErrorItem++;
    }


    *pei = CreateErrorInfoBlock(DFS_ERROR_ERRORIO,errno_code,dfNbErrorItem,eii);
    if (fFirstWinError)
        if (eii[0].pInfo != NULL)
            DfsFree(eii[0].pInfo);
    return ((*pei)!=NULL);
}


BOOL FillErrNoError(LOWLEVELFILE llf,int errno_code,H_ERROR_INFO * pei)
{
    dfwcharp dfFileName=NULL;
    if (pei == NULL)
        return FALSE;
    if ((*pei) != NULL)
        return FALSE;
    // now create error


    if (llf!=NULL)
    {
        LOWLEVELINTERNAL* pLowLevelIntern = (LOWLEVELINTERNAL*)llf;
        dfFileName = pLowLevelIntern->dfFileName;
    }
    return FillErrNoFileName(dfFileName,errno_code,pei);
}

dfuLong32 diskLowLevelWrite(LOWLEVELFILE llFile, void const *Buf, dfuLong32 size, H_ERROR_INFO * pei)
{
  LOWLEVELINTERNAL* pLowLevelIntern = (LOWLEVELINTERNAL*)llFile;
  int filedes;
  ssize_t ret;

  if (pLowLevelIntern == NULL)
      return 0;
  filedes = (int)(pLowLevelIntern->u.filedes);
  if (filedes == 0)
    return 0;

  ret = write(filedes, Buf, size);
  if (ret == -1)
  {
      FillErrNoError(llFile,errno,pei);
      return 0;
  }
  return (dfuLong32)ret;
}

dfuLong32 diskLowLevelRead(LOWLEVELFILE llFile, void *Buf, dfuLong32 size, H_ERROR_INFO* pei)
{
  LOWLEVELINTERNAL* pLowLevelIntern = (LOWLEVELINTERNAL*)llFile;
  int filedes;
  ssize_t ret;

  if (pLowLevelIntern == NULL)
      return 0;
  filedes = (int)(pLowLevelIntern->u.filedes);
  if (filedes == 0)
    return 0;

  ret = read(filedes, Buf, size);
  if (ret == -1)
  {
      FillErrNoError(llFile,errno,pei);
      return 0;
  }
  return (dfuLong32)ret;
}


BOOL GiveOffsetFromPosLowAndPosHigh(DIFSTRM_OFF_T_TYPE *poff,dfuLong32 PosLow, dfuLong32 PosHigh)
{
    DIFSTRM_OFF_T_TYPE ret;
    if (PosHigh > 0)
    {
        dfuLong64 ret64 = PosLow | (((dfuLong64)PosHigh)<<32);
        ret = (DIFSTRM_OFF_T_TYPE)ret64;
        *poff=ret;
        return ret != PosLow;
    }
    else
    {
        ret=PosLow;
        *poff=ret;
        return TRUE;
    }
}


BOOL diskLowLevelSetFileSize(LOWLEVELFILE llFile, dfuLong32 PosLow, dfuLong32 PosHigh, H_ERROR_INFO * pei)
{
  LOWLEVELINTERNAL* pLowLevelIntern = (LOWLEVELINTERNAL*)llFile;
  int filedes;
  DIFSTRM_OFF_T_TYPE offset;

  if (pLowLevelIntern == NULL)
      return 0;
  filedes = (int)(pLowLevelIntern->u.filedes);
  if (filedes == 0)
    return 0;

  if (GiveOffsetFromPosLowAndPosHigh(&offset,PosLow,PosHigh) == FALSE)
      return FALSE;

  if (my_ftruncate(filedes, offset) != -1)
      return TRUE;
  else
  {
      FillErrNoError(llFile,errno,pei);
      return FALSE;
  }
}

BOOL diskLowLevelSeek(LOWLEVELFILE llFile, dfuLong32 PosLow, dfuLong32 PosHigh, TYPESEEK TypeSeek, H_ERROR_INFO* pei)
{
  LOWLEVELINTERNAL* pLowLevelIntern = (LOWLEVELINTERNAL*)llFile;
  int filedes;
  DIFSTRM_OFF_T_TYPE offset;

  if (pLowLevelIntern == NULL)
      return 0;
  filedes = (int)(pLowLevelIntern->u.filedes);
  if (filedes == 0)
    return 0;

  if (GiveOffsetFromPosLowAndPosHigh(&offset,PosLow,PosHigh) == FALSE)
      return FALSE;

  if (my_lseek(filedes, offset, (TypeSeek == TYPESEEK_END) ? SEEK_END : SEEK_SET) != -1)
      return TRUE;
  else
  {
      FillErrNoError(llFile,errno,pei);
      return FALSE;
  }
}

void diskLowLevelTell(LOWLEVELFILE llFile, dfuLong32 *PosLow, dfuLong32 *PosHigh)
{
  LOWLEVELINTERNAL* pLowLevelIntern = (LOWLEVELINTERNAL*)llFile;
  int filedes;
  DIFSTRM_OFF_T_TYPE offset;

  if (pLowLevelIntern == NULL)
      return ;
  filedes = (int)(pLowLevelIntern->u.filedes);
  if (filedes == 0)
    return ;


  offset = my_lseek(filedes, 0, SEEK_CUR);

  if (offset == -1)
  {
      //FillErrNoError(llFile,errno,pei);
      return ;
  }

  if (PosLow!=NULL)
      *PosLow = GetuLong64Low32(offset);

  if (PosHigh!=NULL)
      *PosHigh = GetuLong64High32(offset);

  return ;
}

void diskLowLevelGetSize(LOWLEVELFILE llFile, dfuLong32 *PosLow, dfuLong32 *PosHigh)
{
  LOWLEVELINTERNAL* pLowLevelIntern = (LOWLEVELINTERNAL*)llFile;
  int filedes;
  DIFSTRM_OFF_T_TYPE return_offset;
  DIFSTRM_OFF_T_TYPE cur_offset;
  DIFSTRM_OFF_T_TYPE end_offset;

  if (pLowLevelIntern == NULL)
      return ;
  filedes = (int)(pLowLevelIntern->u.filedes);
  if (filedes == 0)
    return ;


  cur_offset = my_lseek(filedes, 0, SEEK_CUR);

  if (cur_offset == -1)
  {
      //FillErrNoError(llFile,errno,pei);
      return ;
  }

  end_offset = my_lseek(filedes, 0, SEEK_END);
  if (end_offset == -1)
  {
      //FillErrNoError(llFile,errno,pei);
      return ;
  }

  return_offset = my_lseek(filedes, cur_offset, SEEK_SET);
  if (return_offset == -1)
  {
      //FillErrNoError(llFile,errno,pei);
      return ;
  }


  if (PosLow!=NULL)
      *PosLow = GetuLong64Low32(end_offset);

  if (PosHigh!=NULL)
      *PosHigh = GetuLong64High32(end_offset);

  return ;
}

BOOL diskLowLevelClose(LOWLEVELFILE llFile, BOOL fFlushWrite, H_ERROR_INFO* pei)
{
  BOOL fRet;
  LOWLEVELINTERNAL* pLowLevelIntern = (LOWLEVELINTERNAL*)llFile;
  int filedes;

  if (pLowLevelIntern == NULL)
      return FALSE;

  filedes = (int)(pLowLevelIntern->u.filedes);
  if (filedes == 0)
    return 0;
  if (fFlushWrite)
    fsync(filedes);
  fRet = close(filedes) != -1;
  //printf("closed %u\n",filedes);

  if (!fRet)
  {
      FillErrNoError(llFile,errno,pei);
      return FALSE;
  }

  if (pLowLevelIntern->dfFileName!=NULL)
    DfsFree(pLowLevelIntern->dfFileName);
  DfsFree(pLowLevelIntern);
  return fRet;
}

BOOL diskLowLevelCloseChangeDateTime(LOWLEVELFILE llFile, BOOL fFlushWrite, const DFSTM* pDfsTm, BOOL* pDateTimeChanged, H_ERROR_INFO* pei)
{
    if (pDateTimeChanged!=NULL)
        *pDateTimeChanged=FALSE;
    return diskLowLevelClose(llFile, fFlushWrite, pei);
}

#define STD_BUF_CHARCNV (0x7ff)

static char* BuildWorkableAnsiBufferForUnicodeString(char* szPreallocatedBuffer,dfuLong32 dfPreallocatedBufferSize,
                                                     dfwcharpc strw,BOOL fTransformSlashToOperatingSystemCompatibleSlash)
{
    dfuLong32 dfLen = dfUnicodeStrlen(strw);
    dfuLong32 dfLenAnsiBufferWanted=(dfLen*4)+0x100;
    char*szAnsiFileName;
    if (dfPreallocatedBufferSize < dfLenAnsiBufferWanted)
        szAnsiFileName = (char*)DfsMalloc(dfLenAnsiBufferWanted+1);
    else
        szAnsiFileName = szPreallocatedBuffer;
    if (szAnsiFileName != NULL)
        ConvertUnicodeToAnsiEx(strw,szAnsiFileName,dfLenAnsiBufferWanted,TRUE);
    return szAnsiFileName;
}

void Convert_Time_T_TO_DfsTm(const time_t *pTime_T,DFSTM* pDfsTm)
{
    struct tm *ptm=localtime(pTime_T);
    pDfsTm -> df_msec = 0;

    pDfsTm -> df_sec = ptm -> tm_sec;
    pDfsTm -> df_min = ptm -> tm_min;
    pDfsTm -> df_hour = ptm -> tm_hour;
    pDfsTm -> df_mday = ptm -> tm_mday;
    pDfsTm -> df_mon = ptm -> tm_mon+1;
    pDfsTm -> df_year = ptm -> tm_year+1900;
      // df_timezone_bias;      bias [0..255] 8 bits. this is ((bias in minute/15)+128)
#if defined(_AIX43)
    pDfsTm -> df_timezone_bias = 0;
#else
    pDfsTm -> df_timezone_bias = (dfuLong32)(((ptm->tm_gmtoff/60)/15)+128);
#endif
}


/*
typedef struct
{
  dfuLong32 df_msec;
  dfuLong32 df_sec;
  dfuLong32 df_min;
  dfuLong32 df_hour;
  dfuLong32 df_mday;
  dfuLong32 df_mon;
  dfuLong32 df_year;
  dfuLong32 df_timezone_bias;
}
DFSTM;*/
dfuLong64 diskGetFileSizeByName(dfwcharpc FileName, DFSTM * pDfsTm, H_ERROR_INFO* pei)
{
    char *szAnsiFileName;
    dfwcharp szConvertedFileName;
    dfuLong32 dfLen = dfUnicodeStrlen(FileName);
    char szStdAnsiConvertBuffer[STD_BUF_CHARCNV+1];
    struct stat buf_stat;
    int iStat ;

    szConvertedFileName = (dfwcharp)DfsMalloc((dfLen+0x200)*2);
    //szAnsiFileName = (char*)DfsMalloc((dfLen+0x200)*2);
    ConvertFileNameAndPath(FileName,szConvertedFileName,dfLen+0x100,FALSE);

    szAnsiFileName = BuildWorkableAnsiBufferForUnicodeString(szStdAnsiConvertBuffer,STD_BUF_CHARCNV,szConvertedFileName,TRUE);


    iStat = my_stat(szAnsiFileName,&buf_stat);
    if (iStat != 0)
        buf_stat.st_size=0;

    if ((pDfsTm!=NULL) && (iStat == 0))
    {
#ifdef ANDROID
#else
        /*struct timespec *ptspc;*/
#ifdef LINUX
        /*ptspc = &(buf_stat.st_mtim);*/
        Convert_Time_T_TO_DfsTm(&buf_stat.st_mtim.tv_sec,pDfsTm);
#else
        /*ptspc = &(buf_stat.st_mtimespec);*/
        Convert_Time_T_TO_DfsTm(&buf_stat.st_mtimespec.tv_sec,pDfsTm);
#endif
#endif
    }

    if (szAnsiFileName != szStdAnsiConvertBuffer)
      DfsFree(szAnsiFileName);
    DfsFree(szConvertedFileName);

    return buf_stat.st_size;
}




int DoOpenW(dfwcharpc str,int flags,mode_t modeopen)
{
    char szStdAnsiConvertBuffer[STD_BUF_CHARCNV+1];
    int iRet = -1;
    char *szAnsiFileName = BuildWorkableAnsiBufferForUnicodeString(szStdAnsiConvertBuffer,STD_BUF_CHARCNV,str,TRUE);

    if (szAnsiFileName != NULL)
    {
        iRet = open(szAnsiFileName,flags,modeopen) ;
        //printf("open '%s' '-' %u %u %d %d\n",szAnsiFileName,strlen(szAnsiFileName),flags,iRet,errno);
        if (szAnsiFileName != szStdAnsiConvertBuffer)
          DfsFree(szAnsiFileName);
    }
    return iRet;
}



LOWLEVELFILE diskOpenLowLevel(dfwcharpc FileName, TYPEOPEN TypeOpen,
                          BOOL fTemporaryFile, BOOL fProjectedSize, dfuLong64 dfProjectedSize,
                          H_ERROR_INFO * pei)
{
    LOWLEVELFILE llfRet = NULL;

    //char *szAnsiFileName;
    dfwcharp szConvertedFileName;
    dfuLong32 dfLen = dfUnicodeStrlen(FileName);
    int iOpen ;
    int flags = O_RDONLY;

    szConvertedFileName = (dfwcharp)DfsMalloc((dfLen+0x200)*2);
    //szAnsiFileName = (char*)DfsMalloc((dfLen+0x200)*2);
    ConvertFileNameAndPath(FileName,szConvertedFileName,dfLen+0x100,FALSE);
    //ConvertUnicodeToAnsi(szConvertedFileName,szAnsiFileName,(dfLen+0x100)*2);

    switch (TypeOpen)
    {
      case OPEN_READ:
        flags = O_RDONLY;
        break;

      case OPEN_READWRITE:
        flags = O_RDWR;
        break;

      case OPEN_CREATE:
        flags = O_CREAT | O_RDWR;
        break;
    }
    //iOpen = open(szAnsiFileName,flags);
    iOpen = DoOpenW(szConvertedFileName,flags,S_IRUSR | S_IWUSR  |  S_IRGRP | S_IWGRP | S_IROTH/* |0100000| O_LARGEFILE*/);


    //DfsFree(szAnsiFileName);
    DfsFree(szConvertedFileName);

    if (iOpen==-1)
      {
          FillErrNoFileName(FileName,errno,pei);
          return NULL;
      }
    else
   {
      LOWLEVELINTERNAL* pLowLevelIntern=(LOWLEVELINTERNAL*)DfsMalloc(sizeof(LOWLEVELINTERNAL));
      if (pLowLevelIntern == NULL)
      {
          close(iOpen);
          iOpen=0;
      }
      else
      {
          pLowLevelIntern->u.filedes = iOpen;
          pLowLevelIntern->dfFileName = dfUnicodeCopyAlloc(FileName);

          pLowLevelIntern->pvfio=NULL;

          llfRet = (LOWLEVELFILE) pLowLevelIntern;
		  if (TypeOpen==OPEN_CREATE)
		  {
			  my_ftruncate(iOpen, 0);
		  }
      }
  }
    return llfRet;
}




dfvoidp fncDiskAdaptDataMapViewMmap(void* pOrgData_,dfuLong64 dfCurrentViewBegin,dfuLong64 dfCurrentViewEndOrSize,BOOL fBySize,BOOL *pfDoneOk);
dfuIntPtr CalculateMaxOrigDataExigibleSizeViewMask(dfuIntPtr dfMaxOrigDataExigibleSizeView);


BOOL diskGetFileFullContentBuffer(dfwcharpc FileName,dfuLong32 dfAccessFlag,dfuLong64 dfSizeRequested,//BOOL fReadOnly,BOOL fReadOnly,
                                  HFILECONTENTREADBUF* phFileContentReadBuf,
                                  ORIGDATA* pOrg,
                                  H_ERROR_INFO* pei)
{
  FILECONTENTREADBUFINTERNAL FileContentReadBufInternal;
  BOOL fReadOnly = (dfAccessFlag & FILEFULLCONTENTBUFFER_ACCESSMASK)==FILEFULLCONTENTBUFFER_READ;
  //dfuLong32 dwErr = 0;
  FILECONTENTREADBUFINTERNAL *pFileContentReadBufInternal = NULL;
  int iFileOpen;
  if (phFileContentReadBuf==NULL)
      return FALSE;
  *phFileContentReadBuf = NULL;

  if (pOrg == NULL)
      return FALSE;

  FillOrigDataForFullMemoryOrg(pOrg,NULL,FILE_SIZE_NOT_EXIST);

  if (FileName == NULL)
    return FALSE;

  FileContentReadBufInternal.lpBuf = NULL;
  FileContentReadBufInternal.fPartialMapping = FALSE;
  FileContentReadBufInternal.dfBeginMapPos = 0;
  FileContentReadBufInternal.dfMapSize = 0;
  FileContentReadBufInternal.dfGranularityMapping = 0;
  FileContentReadBufInternal.fWritePossible = !fReadOnly;
  FileContentReadBufInternal.fProgressiveFlush = (dfAccessFlag & FILEFULLCONTENTBUFFER_PROGRESSIVE_FLUSH) != 0;

  iFileOpen=DoOpenW(FileName,fReadOnly ? O_RDONLY:O_RDWR,0);
  if ((iFileOpen==0) || (iFileOpen==-1))
  {
      FillErrNoFileName(FileName,errno,pei);
      return FALSE;
  }

  if ((iFileOpen != 0) && ((dfAccessFlag & FILEFULLCONTENTBUFFER_RESIZE) != 0))
  {
      if (my_ftruncate(iFileOpen,dfSizeRequested) == -1)
      {
          close(iFileOpen);
          FillErrNoFileName(FileName,errno,pei);
          return FALSE;
      }
  }

  FileContentReadBufInternal.dwSize = my_lseek(iFileOpen, 0, SEEK_END);
  my_lseek(iFileOpen, 0, SEEK_SET);


  if (FileContentReadBufInternal.dwSize == 0)
  {
      (pOrg->size) = 0;
      return TRUE;
  }

  FileContentReadBufInternal.fVirtualMemFile=FALSE;

  FileContentReadBufInternal.u.filedes = iFileOpen;

  pFileContentReadBufInternal =
      (FILECONTENTREADBUFINTERNAL *)
      DfsMalloc(sizeof(FILECONTENTREADBUFINTERNAL));

  if (pFileContentReadBufInternal == NULL)
  {
    close(iFileOpen);
    return FALSE;
  }
  else
  {
    memset(pFileContentReadBufInternal, 0, sizeof(FILECONTENTREADBUFINTERNAL));
    FillOrigDataForFullMemoryOrg(pOrg,FileContentReadBufInternal.lpBuf,FileContentReadBufInternal.dwSize);

#ifndef MEMORY_MAP_FULLFILE

    if (FileContentReadBufInternal.lpBuf == NULL)
    {
        //SYSTEM_INFO si;
        //GetSystemInfo(&si);
        FileContentReadBufInternal.fPartialMapping = TRUE;
        FileContentReadBufInternal.dfBeginMapPos = 0;

        FileContentReadBufInternal.dfGranularityMapping = (dfuLong32)sysconf(_SC_PAGESIZE);//si.dwAllocationGranularity;

        FileContentReadBufInternal.dfMapSize = 1024*1024*128;//(si.dwAllocationGranularity*2);//
        if (FileContentReadBufInternal.dfMapSize>FileContentReadBufInternal.dwSize)
            FileContentReadBufInternal.dfMapSize = (dfuLong32)
              (((FileContentReadBufInternal.dwSize + FileContentReadBufInternal.dfGranularityMapping - 1) /
                 FileContentReadBufInternal.dfGranularityMapping) * FileContentReadBufInternal.dfGranularityMapping);

        /*
        pOrg->pCurrentView =
        FileContentReadBufInternal.lpBuf = MapViewOfFile(FileContentReadBufInternal.hFileMap, FILE_MAP_READ, 0, 0,
           (pOrg->size < FileContentReadBufInternal.dfMapSize) ? 0 : FileContentReadBufInternal.dfMapSize);
           */
        pOrg->pCurrentView = FileContentReadBufInternal.lpBuf =
            my_mmap(FileContentReadBufInternal.lpBuf, FileContentReadBufInternal.dfMapSize,
                FileContentReadBufInternal.fWritePossible ? (PROT_READ|PROT_WRITE) : (PROT_READ),
                FileContentReadBufInternal.fWritePossible ? (MAP_SHARED) : (MAP_PRIVATE),
                (int)FileContentReadBufInternal.u.filedes,
                FileContentReadBufInternal.dfBeginMapPos);

        if ((FileContentReadBufInternal.lpBuf == NULL) ||
            (FileContentReadBufInternal.lpBuf == (MAP_FAILED)) ||
            (FileContentReadBufInternal.lpBuf == ((void*)-1)))
        {
            close(iFileOpen);
            return FALSE;
        }


        pOrg->dfCurrentViewBegin = FileContentReadBufInternal.dfBeginMapPos;
        pOrg->dfCurrentViewLimitEnd = FileContentReadBufInternal.dfBeginMapPos+FileContentReadBufInternal.dfMapSize;
        pOrg->pInternalfncAdaptData = (dfvoidp)pFileContentReadBufInternal;

        if (FileContentReadBufInternal.dfMapSize>=FileContentReadBufInternal.dwSize)
            pOrg->dfMaxOrigDataExigibleSizeView = (FileContentReadBufInternal.dfMapSize) ;
        else
            pOrg->dfMaxOrigDataExigibleSizeView = (FileContentReadBufInternal.dfMapSize) -
                                           (1*(FileContentReadBufInternal.dfGranularityMapping));

        pOrg->dfMaxOrigDataExigibleSizeViewMask = CalculateMaxOrigDataExigibleSizeViewMask(pOrg->dfMaxOrigDataExigibleSizeView);
        pOrg->fncAdaptDataMapView = fncDiskAdaptDataMapViewMmap;
        pOrg->fncSetDirtyMapView = NULL;
    }

    if (((pOrg->pCurrentView) == NULL) && (pOrg->dfCurrentViewBegin ==0) && (pOrg->size!=0))
        printf("*");
#endif
    *pFileContentReadBufInternal = FileContentReadBufInternal;

  }
  *phFileContentReadBuf = (HFILECONTENTREADBUF) pFileContentReadBufInternal;
  return TRUE;
}



BOOL diskCloseFileFullContentBuffer(FILECONTENTREADBUFINTERNAL *pFileContentReadBufInternal, BOOL fFlushWrite, H_ERROR_INFO* pei)
{
  BOOL fRet=TRUE;
  if (pFileContentReadBufInternal==NULL)
      return FALSE;
  if (!pFileContentReadBufInternal->fVirtualMemFile)
  {
    if (pFileContentReadBufInternal->fProgressiveFlush)
        msync(pFileContentReadBufInternal->lpBuf, pFileContentReadBufInternal->dfMapSize, MS_SYNC);
    munmap(pFileContentReadBufInternal->lpBuf, pFileContentReadBufInternal->dfMapSize);
    if (fFlushWrite || pFileContentReadBufInternal->fProgressiveFlush)
        fsync(pFileContentReadBufInternal->u.filedes);

    fRet = close(pFileContentReadBufInternal->u.filedes) != -1;
    if (!fRet)
    {
        FillErrNoError(NULL,errno,pei);
    }

    pFileContentReadBufInternal->u.filedes=0;
    pFileContentReadBufInternal->lpBuf=NULL;
  }

  DfsFree(pFileContentReadBufInternal);
  return fRet;
}

#ifndef MEMORY_MAP_FULLFILE

dfuIntPtr CalculateMaxOrigDataExigibleSizeViewMask(dfuIntPtr dfMaxOrigDataExigibleSizeView)
{
    dfuIntPtr dfRes = 1;
    while ((dfRes << 1)<=dfMaxOrigDataExigibleSizeView)
        dfRes = dfRes << 1;
    return dfRes - 1;
}

dfvoidp fncDiskAdaptDataMapViewMmap(void* pOrgData_,dfuLong64 dfCurrentViewBegin,dfuLong64 dfCurrentViewEndOrSize,BOOL fBySize,BOOL *pfDoneOk)
{
    BOOL fDoneOk=TRUE;
    dfuLong64 dfCurrentViewEnd;
    dfuLong64 dfCurrentViewBeginAround = 0;
    dfuLong64 dfCurrentViewEndAround = 0;
    ORIGDATA* pOrgData = (ORIGDATA*)pOrgData_;
    FILECONTENTREADBUFINTERNAL* pFileContentReadBufInternal = (FILECONTENTREADBUFINTERNAL*)pOrgData->pInternalfncAdaptData;
    dfuLong32 dfGranularity = pFileContentReadBufInternal->dfGranularityMapping;

    if (fBySize)
        dfCurrentViewEnd = dfCurrentViewBegin + dfCurrentViewEndOrSize ;
    else
        dfCurrentViewEnd = dfCurrentViewEndOrSize ;

    if (dfCurrentViewEnd<dfCurrentViewBegin)
        fDoneOk = FALSE;
    else
    {
        dfCurrentViewBeginAround = (dfCurrentViewBegin / dfGranularity) * dfGranularity;
        dfCurrentViewEndAround = ((dfCurrentViewEnd + dfGranularity - 1) / dfGranularity) * dfGranularity;

        if ((dfCurrentViewEndAround - dfCurrentViewBeginAround) < pFileContentReadBufInternal->dfMapSize)
        {
            dfuLong32 dfNbMaxAddGranUnit = (dfuLong32)((pFileContentReadBufInternal->dfMapSize -
                                                (dfCurrentViewEndAround - dfCurrentViewBeginAround)) /
                                                dfGranularity);
            dfuLong32 dfNbMaxAddGranUnitBefore = (dfuLong32)((dfCurrentViewBeginAround) / dfGranularity);
            dfuLong32 dfNbMaxAddGranUnitAfter = (dfuLong32)((pFileContentReadBufInternal->dwSize +
                                                (dfGranularity - 1) - dfCurrentViewEndAround) / dfGranularity);
            dfuLong32 dfNbAddGranBefore,dfNbAddGranAfter;
            dfNbAddGranBefore = dfmin((dfNbMaxAddGranUnit)/2,dfNbMaxAddGranUnitBefore);
            dfNbAddGranAfter = dfmin(dfNbMaxAddGranUnit - dfNbAddGranBefore,dfNbMaxAddGranUnitAfter);
            dfCurrentViewBeginAround -= dfNbAddGranBefore * dfGranularity;
            dfCurrentViewEndAround += dfNbAddGranAfter * dfGranularity;
        }
    }

    if (fDoneOk)
    {
        if ((dfCurrentViewEndAround - dfCurrentViewBeginAround) > (pFileContentReadBufInternal->dfMapSize))
            printf("\n+++---overmap %lu %lu\n",(unsigned long)((dfuIntPtr)dfCurrentViewEndAround - dfCurrentViewBeginAround),(unsigned long)pFileContentReadBufInternal->dfMapSize);
    }
    if (fDoneOk)
    {
        /*
        if (pFileContentReadBufInternal->lpBuf)
            UnmapViewOfFile(pFileContentReadBufInternal->lpBuf);

        pFileContentReadBufInternal->lpBuf = MapViewOfFile(pFileContentReadBufInternal->hFileMap, FILE_MAP_READ,
                     (DWORD)(dfCurrentViewBeginAround>>32),
                     (DWORD)(dfCurrentViewBeginAround),
                     (dfCurrentViewEndAround >= pOrgData->size) ? 0 :
                       (dfuIntPtr)(dfCurrentViewEndAround - dfCurrentViewBeginAround));
*/
        if (pFileContentReadBufInternal->lpBuf!=NULL)
        {
            if (pFileContentReadBufInternal->fProgressiveFlush)
              msync(pFileContentReadBufInternal->lpBuf, pFileContentReadBufInternal->dfMapSize, MS_SYNC);
            munmap(pFileContentReadBufInternal->lpBuf,pFileContentReadBufInternal->dfMapSize);
            pFileContentReadBufInternal->lpBuf=NULL;
        }
        pFileContentReadBufInternal->lpBuf =
         my_mmap(pFileContentReadBufInternal->lpBuf,pFileContentReadBufInternal->dfMapSize,

                pFileContentReadBufInternal->fWritePossible ? (PROT_READ|PROT_WRITE) : (PROT_READ),
                pFileContentReadBufInternal->fWritePossible ? (MAP_SHARED) : (MAP_PRIVATE),

                (int)pFileContentReadBufInternal->u.filedes,
                dfCurrentViewBeginAround);


        if ((pFileContentReadBufInternal->lpBuf == NULL) ||
            (pFileContentReadBufInternal->lpBuf == (MAP_FAILED)) ||
            (pFileContentReadBufInternal->lpBuf == ((void*)-1)))
        {
            printf("error %u\n",errno);
            printf("map size=%lx,pos=%lx %lu , %lu, buf=%lx\n",
                (unsigned long)pFileContentReadBufInternal->dfMapSize,
                (unsigned long)dfCurrentViewBeginAround,
                (unsigned long)pOrgData->size,
                (unsigned long)pFileContentReadBufInternal->u.filedes,
                (unsigned long)pFileContentReadBufInternal->lpBuf);
        }

        if ((pFileContentReadBufInternal->lpBuf == NULL) ||
            (pFileContentReadBufInternal->lpBuf == (MAP_FAILED)) ||
            (pFileContentReadBufInternal->lpBuf == ((void*)-1)))
        {
            fDoneOk = FALSE;
        }
        else
        {
          pOrgData->pCurrentView = ((dfbytep)(pFileContentReadBufInternal->lpBuf)) - dfCurrentViewBeginAround;
          pOrgData->dfCurrentViewBegin = dfCurrentViewBeginAround;
          pOrgData->dfCurrentViewLimitEnd = dfCurrentViewEndAround;
        }
    }

    if (((pOrgData->pCurrentView) == NULL) && (pOrgData->dfCurrentViewBegin ==0) && (pOrgData->size!=0))
        printf("-");

    if (pfDoneOk!=NULL)
        *pfDoneOk = fDoneOk;

    if (!fDoneOk)
      return NULL;
    else
      return pOrgData->pCurrentView;
}
#endif


char szTempDir[MAX_PATH_LENGTH]="";

BOOL SVFAPI SetTempDirectorySystem(dfwcharp lpDirectory,dfuLong32 dfSizeBufferinwChar)
{
    if (dfUnicodeStrlen(lpDirectory)>=(MAX_PATH_LENGTH-0x18))
        return FALSE;
    ConvertUnicodeToAnsi(lpDirectory, szTempDir, MAX_PATH_LENGTH);
    return TRUE;
}

BOOL SVFAPI SetTempDirectorySystemChar(const char* lpDirectory,dfuLong32 dfSizeBufferinwChar)
{
    if (strlen(lpDirectory)>=(MAX_PATH_LENGTH-0x18))
        return FALSE;
    strcpy(szTempDir,lpDirectory);
    return TRUE;
}

BOOL SVFAPI GetTempDirectorySystem(dfwcharp lpDirectory,dfuLong32 dfSizeBufferinwChar)
{
	if (szTempDir[0]==0)
		DfUnicodeStrcpy(lpDirectory,GetUnicodeStringDot());
	else {
		ConvertAnsiToUnicode(szTempDir,lpDirectory,dfSizeBufferinwChar);
	}
    return TRUE;
}

void ConvertDfsTmToUtimeBuf(const DFSTM * pDfsTm,time_t *ptm)
{
    struct tm vtm;
    //struct tm *ptm=localtime(pTime_T);

    memset(&vtm,0,sizeof(vtm));
    vtm.tm_sec = pDfsTm -> df_sec  ;
    vtm.tm_min = pDfsTm -> df_min  ;
    vtm.tm_hour = pDfsTm -> df_hour  ;
    vtm.tm_mday = pDfsTm -> df_mday  ;
    vtm.tm_mon = pDfsTm -> df_mon-1  ;
    vtm.tm_year = pDfsTm -> df_year-1900  ;

      // df_timezone_bias;      bias [0..255] 8 bits. this is ((bias in minute/15)+128)
    //pDfsTm -> df_timezone_bias = ((ptm->tm_gmtoff/60)/15)+128;
    *ptm=mktime(&vtm);
}



BOOL diskChangeDateTimeFile(dfwcharpc FileName, const DFSTM * pDfsTm)
{
    BOOL fRet;
    char szStdAnsiConvertBuffer[STD_BUF_CHARCNV+1];
    struct utimbuf times;
    time_t tmt;
    ConvertDfsTmToUtimeBuf(pDfsTm,&tmt);
    times.actime=tmt;
    times.modtime=tmt;
    char *szAnsiFileName = BuildWorkableAnsiBufferForUnicodeString(szStdAnsiConvertBuffer,STD_BUF_CHARCNV,FileName,TRUE);

    fRet = utime (szAnsiFileName,&times) != -1;
    if (szAnsiFileName != szStdAnsiConvertBuffer)
      DfsFree(szAnsiFileName);
    return fRet;
}



BOOL MyMkdirStepByStep(const char*dirname)
{
  if ((*dirname)!='\0')
  {
      char* dupname=strdup(dirname);
      char* lpParc = dupname;

      if ((*lpParc)!=0)
          lpParc++;

      while ((*lpParc)!=0)
      {
          //char* lpNext;
          if ( ((*(lpParc))=='\\')|| ((*(lpParc))=='/') )
              {
                  char c=*(lpParc);
                  *(lpParc) = '\0';
                  mkdir(dupname,S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP|S_IXGRP|S_IXOTH);
                  *(lpParc) = c;
              }

          lpParc++;
      }
      free(dupname);
  }

  return mkdir(dirname,S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP|S_IXGRP|S_IXOTH) != -1;
}

BOOL SVFAPI MyMkdir(dfwcharpc DirectoryName, H_ERROR_INFO * pei)
{
    BOOL fRet ;
    char szStdAnsiConvertBuffer[STD_BUF_CHARCNV+1];
    char *szAnsiFileName = BuildWorkableAnsiBufferForUnicodeString(szStdAnsiConvertBuffer,STD_BUF_CHARCNV,DirectoryName,TRUE);

    //fRet = mkdir (szAnsiFileName,S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP|S_IXGRP|S_IXOTH) != -1;
    fRet= MyMkdirStepByStep(szAnsiFileName);
    if (szAnsiFileName != szStdAnsiConvertBuffer)
      DfsFree(szAnsiFileName);
    return fRet;
}


BOOL SVFAPI MyDeleteFile(dfwcharpc lpFileName, H_ERROR_INFO * pei)
{
    BOOL fRet ;
    char szStdAnsiConvertBuffer[STD_BUF_CHARCNV+1];
    char *szAnsiFileName;

    if (IsFileNameForMemorySpace(lpFileName))
	  return DeleteTempFile(lpFileName, pei);

    szAnsiFileName = BuildWorkableAnsiBufferForUnicodeString(szStdAnsiConvertBuffer,STD_BUF_CHARCNV,lpFileName,TRUE);

    fRet = unlink (szAnsiFileName) != -1;
    if (szAnsiFileName != szStdAnsiConvertBuffer)
      DfsFree(szAnsiFileName);
    return fRet;
}


BOOL SVFAPI MyCopyFile(dfwcharpc lpExistingFileName,dfwcharpc lpNewFileName,BOOL bFailIfExists, BOOL fFlushWrite, H_ERROR_INFO * pei)
{
    return ManualCopyFile(lpExistingFileName, lpNewFileName, bFailIfExists, fFlushWrite, pei);
}



typedef struct
{
    DIR* dp;
    FILESEARCHITEMFOUND FileSearchCurrentItem;
    BOOL fFirstReturned;
    dfwchar fnarray[STD_BUF_CHARCNV +1];
} FILESEARCHINGINTERNALPOSIX;

HFILESEARCHING SVFAPI InitFileSearching(dfwcharpc szWildcardSearch)
{
    FILESEARCHINGINTERNALPOSIX* pfsi;

    char szStdAnsiConvertBuffer[STD_BUF_CHARCNV+1];
    char *szAnsiFileName ;

    pfsi = (FILESEARCHINGINTERNALPOSIX*)DfsMalloc(sizeof(FILESEARCHINGINTERNALPOSIX));
    if (pfsi == NULL)
    {
        return NULL;
    }
    szAnsiFileName = BuildWorkableAnsiBufferForUnicodeString(szStdAnsiConvertBuffer,STD_BUF_CHARCNV,szWildcardSearch,TRUE);

    if (strcmp(szAnsiFileName,"*")==0)
        strcpy(szAnsiFileName,".");

    pfsi->dp= opendir(szAnsiFileName);
    if (pfsi->dp != NULL)
    {
        pfsi->fFirstReturned=TRUE;
    }
    else
    {
        struct stat statbuf;


        memset(&statbuf,0,sizeof(statbuf));
        if (my_stat(szAnsiFileName,&statbuf)!=-1)
        {
            pfsi->fFirstReturned=FALSE;

            ConvertAnsiToUnicode(szAnsiFileName,pfsi->fnarray,STD_BUF_CHARCNV);
            pfsi->FileSearchCurrentItem.FileName=pfsi->fnarray;
            pfsi->FileSearchCurrentItem.fIsDirectory=S_ISDIR(statbuf.st_mode);
            pfsi->FileSearchCurrentItem.FileSize=statbuf.st_size;
        }
        else
             pfsi->fFirstReturned=TRUE;
    }

    if ((pfsi->dp == NULL) && (pfsi->fFirstReturned))
    {
        DfsFree(pfsi);
        pfsi=NULL;
    }

    if (szAnsiFileName != szStdAnsiConvertBuffer)
      DfsFree(szAnsiFileName);
    return (HFILESEARCHING)pfsi;
}


FILESEARCHITEMFOUND* SVFAPI GetNextItemContent(HFILESEARCHING hfsi)
{
    struct dirent *ep;
    struct stat statbuf;

    FILESEARCHINGINTERNALPOSIX* pfsi = (FILESEARCHINGINTERNALPOSIX*)hfsi;

    if (!pfsi->fFirstReturned)
    {
        pfsi->fFirstReturned=TRUE;
        return &(pfsi->FileSearchCurrentItem);
    }

    if (pfsi->dp==NULL)
        return NULL;

    for (;;)
    {
        ep=readdir(pfsi->dp);
        if (ep==NULL)
            break;
        if ((strcmp(ep->d_name,".")!=0) && (strcmp(ep->d_name,"..")!=0))
            break;
    }

    if (ep!=NULL)
    {
        ConvertAnsiToUnicode(ep->d_name,pfsi->fnarray,STD_BUF_CHARCNV);
        pfsi->FileSearchCurrentItem.FileName=pfsi->fnarray;

        memset(&statbuf,0,sizeof(statbuf));
        if (my_stat(ep->d_name,&statbuf)==-1)
        {
            printf("stat bad for %s\n",ep->d_name);
            statbuf.st_size=0;
        }
        pfsi->FileSearchCurrentItem.fIsDirectory=S_ISDIR(statbuf.st_mode);

        pfsi->FileSearchCurrentItem.FileSize=statbuf.st_size;
        return &(pfsi->FileSearchCurrentItem);
    }
    else
    return NULL;
}

BOOL SVFAPI CloseFileSearching(HFILESEARCHING hfsi)
{
    FILESEARCHINGINTERNALPOSIX* pfsi = (FILESEARCHINGINTERNALPOSIX*)hfsi;
    if (pfsi == NULL)
        return FALSE;
    if (pfsi->dp!=NULL)
      closedir(pfsi->dp);
    DfsFree(pfsi);
    return TRUE;
}


BOOL SVFAPI GetTemporaryFilename(dfwcharpc lpTreeLetter,dfwcharp lpBuffer, dfuLong32 dfSizeBufferinwChar,BOOL fTempMemPossible,dfuLong64 dfFileSizeProjected)
{
    static dfuLong32 uUnique=0;
    return SvfGetTempFileName(lpTreeLetter,GetUnicodeSVFPrefix(),uUnique++,lpBuffer);
}

BOOL SVFAPI SvfGetTempFileName(dfwcharpc lpPathName,dfwcharpc lpPrefixString,dfuLong32 uUnique,dfwcharp lpTempFileName)
{
#ifdef DEV_MOBILE
	BOOL retValue = GetTempMemoryTmpFileName(lpTempFileName, 0x100, 0);
	return retValue;
#else
    char szTemp[256];
    char* result_temp;
    dfwchar szTempw[256];
    strcpy(szTemp,"svXXXXXX");
    result_temp=mktemp(szTemp);
    ConvertAnsiToUnicode(result_temp,szTempw,32);

    ConvertAnsiToUnicode(szTempDir,lpTempFileName,0x200);
    if (szTempDir[0]!=0)
    {
        dfuLong32 dfLen=dfUnicodeStrlen(lpTempFileName);
        dfwchar last=*(lpTempFileName+dfLen-1);
        if ((!CompareUnicodeWithSimpleChar(last,'/')) && (!CompareUnicodeWithSimpleChar(last,'\\')))
        {
            DfUnicodeStrcat(lpTempFileName,GetUnicodeStringSlashSeparator());
        }
        DfUnicodeStrcat(lpTempFileName,szTempw);
    }
    else
        DfUnicodeStrcpy(lpTempFileName,szTempw);

    return 0;
#endif
}

BOOL SVFAPI GetCanonicalFileName(dfwcharpc FileName,
                          dfwcharp szBuffer, dfuLong32 dfBufSizeInWChar,
                          dfuLong32* pdfBufSizeNeededInWChar)
{
    DfUnicodeStrcpy(szBuffer,FileName);
    return TRUE;
}


BOOL SVFAPI SvfMoveFile(dfwcharpc szOldName,dfwcharpc szNewName)
{

	if (IsFileNameForMemorySpace(szOldName) || (IsFileNameForMemorySpace(szNewName)))
	{
		return myRenameFile(szOldName, szNewName, NULL);
	}

	char szStdAnsiOldConvertBuffer[STD_BUF_CHARCNV + 1];
	char szStdAnsiNewConvertBuffer[STD_BUF_CHARCNV + 1];
	BOOL fRet = FALSE;
	char *szAnsiOldFileName = BuildWorkableAnsiBufferForUnicodeString(szStdAnsiOldConvertBuffer, STD_BUF_CHARCNV, szOldName, TRUE);
	char *szAnsiNewFileName = BuildWorkableAnsiBufferForUnicodeString(szStdAnsiNewConvertBuffer, STD_BUF_CHARCNV, szNewName, TRUE);

	if ((szAnsiOldFileName != NULL) && (szAnsiNewFileName != NULL))
	{
		fRet = (rename(szAnsiOldFileName,szAnsiNewFileName) == 0) ? TRUE : FALSE;
	}

	if ((szAnsiOldFileName != NULL) && (szAnsiOldFileName != szStdAnsiOldConvertBuffer))
		DfsFree(szAnsiOldFileName);

	if ((szAnsiNewFileName != NULL) && (szAnsiNewFileName != szStdAnsiNewConvertBuffer))
		DfsFree(szAnsiNewFileName);
    return fRet;
}

BOOL SVFAPI SvfGetFullPathName(dfwcharpc lpFileName,dfuLong32 nBufferLength,dfwcharp lpBuffer,dfwcharp *lpFilePart)
{
    DfUnicodeStrcpy(lpBuffer,lpFileName);
    *lpFilePart=lpBuffer;
    return TRUE;
}

#endif

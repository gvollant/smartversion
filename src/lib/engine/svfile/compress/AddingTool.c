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

/* AddingTool.c */
/* this file can be compiled as C file */

#include <stddef.h>
#include <stdio.h>

#include "../../patchstream/common/difbasic.h"
#include "../common/DfsMFile.h"
#include "../../patchstream/common/DfsTlTyp.h"
#include "../../patchstream/common/difstool.h"
#include "../common/DfsStruc.h"
#include "../common/DfsTagDf.h"
#include "../common/DfsTagMg.h"
#include "../common/DfsTagBlockFloatEnd.h"

#include "../common/DfsIntf.h"
#include "../../patchstream/common/DfsOrigMemoryMap.h"
#include "../../patchstream/rebuild/RamDifWk.h"
#include "../common/DfsIntfL.h"
#include "../../patchstream/common/DfsIoHlp.h"
#include "../common/DfsSet.h"
#include "../common/DfsSetTl.h"
#include "DfsWrSet.h"

#include "../../patchstream/common/difstrm.h"
#include "../../patchstream/compress/makdifst.h"

#include "zlib.h"
#include "../../patchstream/common/compress_store.h"

//#include "svf_md5.h"
//#include "svf_sha.h"


#include "../decompress/DfsRdSet.h"
#include "../common/DirSet.h"
#include "AddingTool.h"
#include "../common/ArrayTl.h"
#include "unzip.h"


#if (defined(_WIN32)) || (defined(WIN32))
#include "iowin32.h"
#endif
BOOL SVFAPI ClearFileToAdd(FILETOADD* pfta)
{
    if (pfta->filename_tostore != NULL)
        DfsFree((void*)pfta->filename_tostore);

    if (pfta->filename_ondisk != NULL)
        DfsFree((void*)pfta->filename_ondisk);

    if (pfta->filename_prevversionondisk != NULL)
        DfsFree((void*)pfta->filename_prevversionondisk);
    /*
    if (pfta->hRamDifToFlushPatch != NULL)
        {}
        */
    return TRUE;
}

void SVFAPI ClearFileToAddArrayWithDelete(FILETOADDARRAY* pftaArray,BOOL fClearPrevious,BOOL fDeleteOnDisk)
{
    dfuLong32 i;
    if (fClearPrevious)
    {
        for (i=0;i<pftaArray->dfNbFileToAdd;i++)
        {
            if (fDeleteOnDisk)
            {
                dfwcharpc dfOnDisk = ((pftaArray->pFileToAdd)+i)->filename_ondisk;
                DeleteTempFile(dfOnDisk,NULL);
            }
            ClearFileToAdd((pftaArray->pFileToAdd)+i);
        }
        if (pftaArray->pFileToAdd != NULL)
        DeleteArray(pftaArray->pFileToAdd);
    }

    pftaArray->pFileToAdd = 0;

    pftaArray->pFileToAdd = NULL;
    pftaArray->dfNbFileToAdd = 0;
    pftaArray->dfFileToAddStepAlloc = 0x10;
    pftaArray->dfNbFileToAddAllocated = 0;
}

BOOL SVFAPI AddFtaToFtaArray(FILETOADDARRAY* pftaArray,const FILETOADD* pfta,dfuLong32 dfNbAdd)
{
    FILETOADD* pFileToAddNew = (FILETOADD *)
      AddArrayElem(pftaArray->pFileToAdd,
                   &(pftaArray->dfNbFileToAdd),
                   &(pftaArray->dfNbFileToAddAllocated),
                   pftaArray->dfFileToAddStepAlloc,
                   sizeof(FILETOADD), pfta, dfNbAdd);
    if (pFileToAddNew == NULL)
        return FALSE;
    pftaArray->pFileToAdd = pFileToAddNew ;
    return TRUE;
}

#define MAX_PATH_IN_ZIP (0x200)

BOOL SVFAPI BuildFtaArrayFromZipfile(FILETOADDARRAY* pftaArray,const char * lpszZipFile)
{
    BOOL fRet=TRUE;
    dfuLong32 i;
    unzFile uf;
    unz_global_info gi;
    dfbytep dfBuffer;
    dfuLong32 dfBufferSize=0x8000;

    zlib_filefunc_def ffunc;
    BOOL fFuncFilled=FALSE;
#if (defined(_WIN32)) || (defined(WIN32))
    fill_win32_filefunc(&ffunc);
    fFuncFilled = TRUE;
#endif

    ClearFileToAddArrayWithDelete(pftaArray,FALSE,FALSE);

    uf = unzOpen2(lpszZipFile,fFuncFilled ? (&ffunc) : NULL);
    if (uf == NULL)
        return FALSE;

    int err;

    err = unzGetGlobalInfo (uf,&gi);
    if (err!=UNZ_OK)
    {
        unzClose(uf);
        return FALSE;
    }

    dfBuffer=(dfbytep)DfsMalloc(dfBufferSize);
    if (dfBuffer==NULL)
    {
        unzClose(uf);
        return FALSE;
    }

    for (i=0;i<gi.number_entry;i++)
    {
        unz_file_info file_info;
        char filename_inzip[MAX_PATH_IN_ZIP];
        LOWLEVELFILE llw;
        //LPTSTR lpParc;
        FILETOADD fta;


        err = unzGetCurrentFileInfo(uf,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);

        if (err==UNZ_OK)
          err = unzOpenCurrentFile(uf);
        if (err!=UNZ_OK)
        {
            fRet=FALSE;
            break;
        }
        /*
        lpParc = filename_inzip;
        while ((*lpParc)!='\0')
        {
            if ((*lpParc)=='/')
                (*lpParc)='\\';
        }*/

        DfsClearStruct(&fta,0,sizeof(FILETOADD));
        {
            dfwchar szTempFN[1024];

            GetTemporaryFilename((dfwcharpc)L"SVF",szTempFN,(sizeof(szTempFN) / sizeof(dfwchar))-1, TRUE,file_info.uncompressed_size);
            fta.filename_ondisk = dfUnicodeCopyConcatAlloc(szTempFN,NULL);
        }

        fta.fIgnore = FALSE;
        fta.fForceDate = TRUE;
        fta.hAddTags = NULL;
        fta.pReserved = NULL;
        fta.fForceRecopyPrevious = FALSE;
        fta.fWritingRaw=FALSE;
        fta.dfFileStatusForRaw=0;
        fta.hRamDifToFlushPatch = NULL;

        fta.filename_tostore = (dfwcharpc)DfsMalloc((strlen(filename_inzip)+0x10)*sizeof(dfwchar));
        //ConvertTCharToUnicode(filename_inzip,(dfwcharp)fta.filename_tostore,strlen(filename_inzip)+0x8);
        //ConvertOemToUnicode(filename_inzip,(dfwcharp)fta.filename_tostore,strlen(filename_inzip)+0x8);
        ConvertAnsiToUnicode(filename_inzip,(dfwcharp)fta.filename_tostore,(dfuLong32)(strlen(filename_inzip)+0x8));

        ConvertFileNameAndPath(fta.filename_tostore,NULL,0,FALSE);

        fta.filename_prevversionondisk = NULL;
        fta.dfPreviousVersionFilePosition = 0;

        {
            DFSTM DfsTm;
            DfsTm.df_msec=0;
            DfsTm.df_sec=file_info.tmu_date.tm_sec;
            DfsTm.df_min=file_info.tmu_date.tm_min;
            DfsTm.df_hour=file_info.tmu_date.tm_hour;
            DfsTm.df_mday=file_info.tmu_date.tm_mday;
            DfsTm.df_mon=file_info.tmu_date.tm_mon + 1;
            DfsTm.df_year=file_info.tmu_date.tm_year ;
            DfsTm.df_timezone_bias=0;
            ConvertDfsTmToDfsInfoDate(&DfsTm,&fta.dfsInfoDate);
        }

        if (fta.filename_ondisk!=NULL)
        {
            dfuLong32 dfLen = dfUnicodeStrlen(fta.filename_tostore);
            if (dfLen>0)
            {
                dfwchar wc= fta.filename_tostore[dfLen-1];
                if ((wc=='\\') || (wc=='/'))
                    fta.fIgnore = TRUE;
            }
        }
        fta.dfForceRecopyOrRawCopyCrc32 = (dfuLong32)file_info.crc;
        fta.dfForceRecopyOrRawCopySize = file_info.uncompressed_size;

        if (!AddFtaToFtaArray(pftaArray,&fta,1))
        {
            fRet=FALSE;
            break;
        }

        llw = OpenLowLevel(fta.filename_ondisk,OPEN_CREATE,
                            TRUE,TRUE,file_info.uncompressed_size,NULL); // To optimize temp WRITE, file_info.uncompressed_size


        do
        {
            err = unzReadCurrentFile(uf,dfBuffer,dfBufferSize);
            if (err<0)
            {
                fRet=FALSE;
                break;
            }
            if (err>0)
            {
                dfuLong32 dfWrite = LowLevelWrite(llw,dfBuffer,err,NULL);
                if (dfWrite != ((dfuLong32)err))
                {
                    fRet=FALSE;
                    LowLevelClose(llw,NULL);
                    break;
                }
            }
        }
        while (err>0);
        LowLevelClose(llw,NULL);


        /*
        if (do_extract_currentfile(uf,&opt_extract_without_path,
                                      &opt_overwrite) != UNZ_OK)
            break;
*/
        if ((i+1)<gi.number_entry)
        {
            err = unzGoToNextFile(uf);
            if (err!=UNZ_OK)
            {
                fRet=FALSE;
                break;
            }

        }
    }

    unzClose(uf);
    DfsFree(dfBuffer);
    if (!fRet)
        ClearFileToAddArrayWithDelete(pftaArray,FALSE,TRUE);
    return fRet;
}


BOOL SVFAPI BuildFileSetFromZipFile(const char * lpszZipFile,FILESET * pfs,BOOL fSetTempFlag,H_ERROR_INFO* pei)
{
    dfuLong32 i;
    FILETOADDARRAY ftaArray;
    FreeFileSet(pfs, FALSE);
    ClearFileToAddArrayWithDelete(&ftaArray,FALSE,FALSE);
    if (!BuildFtaArrayFromZipfile(&ftaArray,lpszZipFile))
    {
        ClearFileToAddArrayWithDelete(&ftaArray,TRUE,TRUE);
        return FALSE;
    }

    BOOL fRet=TRUE;

      for (i = 0; i < ftaArray.dfNbFileToAdd; i++)
      {
          FILETOADD *pftaCur = ftaArray.pFileToAdd+i;

          FILEITEM fi;
          fi.fTempFile = fSetTempFlag;
          fi.fIdenticalPreviousVersion = FALSE;
          fi.ExtAction=ExtActionExtractContent;
          fi.fForceDate=FALSE;
          fi.hAddTags = NULL;
          fi.pReserved = NULL;
          fi.FileNameOnArchive = dfUnicodeCopyAlloc(pftaCur->filename_tostore);
          fi.FileNameOnDisk = dfUnicodeCopyAlloc(pftaCur->filename_ondisk);
          fi.dfPreviousVersion = VALUE_UNKNOWN;
          fi.dfsInfoDate = pftaCur->dfsInfoDate;
          fi.hRamDifOfPatch = NULL;

          if (!AddItemToFileSet(pfs, &fi, 1))
          {
            //dfError = DFS_ERROR_MEMORY_ERROR;
            fRet=FALSE;
            break;
          }

      }



    if (fRet == FALSE)
    {
        ClearFileToAddArrayWithDelete(&ftaArray,TRUE,TRUE);
        FreeFileSet(pfs, FALSE);
        return FALSE;
    }
    else
    {
        ClearFileToAddArrayWithDelete(&ftaArray,TRUE,FALSE);
        return TRUE;
    }
}

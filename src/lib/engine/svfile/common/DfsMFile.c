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

#include <stdlib.h>
#include <stdio.h>
//#include <io.h>
#include <fcntl.h>


#include "../../patchstream/common/difbasic.h"
#include "../../patchstream/common/DfsTlTyp.h"
#include "../../patchstream/common/difstool.h"
#include "../../svfile/common/DfsMFile.h"
#include "../../svfile/common/DfsMtStr.h"
#include "../../patchstream/common/DfsOrigMemoryMap.h"
#include "../../patchstream/common/DfsIoHlp.h"

#ifndef SEEK_SET
#  define SEEK_SET        0     /* Seek from beginning of file.  */
#  define SEEK_CUR        1     /* Seek from current position.  */
#  define SEEK_END        2     /* Set file pointer to EOF plus "offset" */
#endif


/* next version will support callback */
/* for info, _chsize = SetEndOfFile */
typedef struct
{
  DFSFILEINFOPARAMINTERNAL dfsFileInfoParam;
  LOWLEVELFILE llfHandle;
  dfuLong32 dfsBlockAlignementSize;
  dfuLong32 dfsBlockAlignementLog2;
  BOOL fReadOnly;
  dfuLong32 dfuLong32erWrittenPosLow;
  dfuLong32 dfuLong32erWrittenPosHigh;
  dfuLong32 dfuCurPosLow;
  dfuLong32 dfuCurPosHigh;
  dfuLong32 dfSizePrefixSfx;
  BOOL fModified;
}
DFSFILEWRAPPARAM;

#include "stdio.h"
DFSFILEWRAP DfsOpenFile(DFSFILEINFOPARAMINTERNAL DfsFileParam,H_ERROR_INFO* pei)
{
  DFSFILEWRAPPARAM *dfsFileWrapParam;
  BOOL fGoodOperation = FALSE;
  BOOL fHandleOpened = FALSE;

  dfsFileWrapParam = (DFSFILEWRAPPARAM *) DfsMalloc(sizeof(DFSFILEWRAPPARAM));

  if (dfsFileWrapParam == NULL)
    return NULL;
  dfsFileWrapParam->dfsFileInfoParam = DfsFileParam;
  dfsFileWrapParam->fReadOnly = FALSE;
  dfsFileWrapParam->fModified = FALSE;
  dfsFileWrapParam->dfuLong32erWrittenPosLow = dfsFileWrapParam->dfuLong32erWrittenPosHigh = 0;
  dfsFileWrapParam->dfuCurPosLow = dfsFileWrapParam->dfuCurPosHigh = 0;
  dfsFileWrapParam->dfSizePrefixSfx = 0;
  dfsFileWrapParam->llfHandle = NULL;

  if ((DfsFileParam.dfStatus & DFS_STREAM) == 0)
  {
    if ((DfsFileParam.dfStatus & DFS_WRITABLE) != 0)
    {
      dfsFileWrapParam->llfHandle =
        OpenLowLevel((dfwcharp) DfsFileParam.filename,
                     ((DfsFileParam.dfStatus & DFS_NEWFILE) !=
                      0) ? OPEN_CREATE : OPEN_READWRITE,FALSE,FALSE,0, pei);
      fHandleOpened = fGoodOperation = (dfsFileWrapParam->llfHandle != NULL);
    }

    if (((DfsFileParam.dfStatus & DFS_READABLE) != 0) && (!fGoodOperation))
    {
      dfsFileWrapParam->llfHandle =
        OpenLowLevel((dfwcharp) DfsFileParam.filename, OPEN_READ, FALSE,FALSE,0,pei);
      fHandleOpened = fGoodOperation = (dfsFileWrapParam->llfHandle != NULL);
    }
  }

  if ((fGoodOperation)
      && ((DfsFileParam.dfStatus & (DFS_NEWFILE | DFS_WRITABLE)) ==
              (DFS_NEWFILE | DFS_WRITABLE)))
  {
    DFSINFOBEGIN DfsInfoBegin;
    //dfsFileWrapParam->dfsBlockAlignement = 0x04;
    //dfsFileWrapParam->dfsBlockAlignement = 0x3;
    dfsFileWrapParam->dfsBlockAlignementSize = 0x04;
    dfsFileWrapParam->dfsBlockAlignementLog2 = 2;
    InitDfsInfoBegin(&DfsInfoBegin, TRUE, dfsFileWrapParam->dfsBlockAlignementSize);
    if (LowLevelWrite
        (dfsFileWrapParam->llfHandle, &DfsInfoBegin,
         sizeof(DFSINFOBEGIN),pei) != sizeof(DFSINFOBEGIN))
      fGoodOperation = FALSE;
  }

  if ((fGoodOperation)
      && (((DfsFileParam.dfStatus & (DFS_NEWFILE | DFS_WRITABLE)) ==
           (DFS_WRITABLE))
          ||
          ((DfsFileParam.
            dfStatus & (DFS_NEWFILE | DFS_WRITABLE | DFS_READABLE)) ==
           (DFS_READABLE))))
  {
    DFSINFOBEGIN DfsInfoBegin;
    DFSINFOEND DfsInfoEnd;
    dfuLong32 dfPosInfoEndLow,dfPosInfoEndHigh;

    dfsFileWrapParam->fReadOnly =
      ((DfsFileParam.dfStatus & (DFS_NEWFILE | DFS_WRITABLE)) == 0);

    LowLevelSeek(dfsFileWrapParam->llfHandle, 0, 0, TYPESEEK_END, pei);
    LowLevelTell(dfsFileWrapParam->llfHandle,&dfsFileWrapParam->dfuLong32erWrittenPosLow,&dfsFileWrapParam->dfuLong32erWrittenPosHigh);

    /* pehaps we can look at     printf("open %s res %u %d\n",szAnsiFileName,iOpen,errno); struct and if dfsTotalSize != 0, store to dfuLong32erWrittenPos */
    dfPosInfoEndLow = dfsFileWrapParam->dfuLong32erWrittenPosLow;
    dfPosInfoEndHigh= dfsFileWrapParam->dfuLong32erWrittenPosHigh;
    dfSub64(&dfPosInfoEndLow,&dfPosInfoEndHigh,sizeof(DFSINFOEND),0);

    LowLevelSeek(dfsFileWrapParam->llfHandle, dfPosInfoEndLow, dfPosInfoEndHigh, TYPESEEK_BEGIN, pei);
    if (LowLevelRead
        (dfsFileWrapParam->llfHandle, &DfsInfoEnd,
         sizeof(DFSINFOEND), pei) != sizeof(DFSINFOEND))
      fGoodOperation = FALSE;

    {
        dfuLong32 dfsThisFileSizeLow = ConvertuLongIntelToLong(DfsInfoEnd.dfsThisFileSizeLow);
        dfuLong32 dfsThisFileSizeHigh= ConvertuLongIntelToLong(DfsInfoEnd.dfsThisFileSizeHigh);
        if ((dfsThisFileSizeLow>0) || (dfsThisFileSizeHigh>0))
        {
            int iCmp = dfCompareValue64(dfsThisFileSizeLow,dfsThisFileSizeHigh,
                                   dfsFileWrapParam->dfuLong32erWrittenPosLow,dfsFileWrapParam->dfuLong32erWrittenPosHigh);
            if (iCmp == 1)
                fGoodOperation = FALSE;


            if (iCmp == -1)
            {
                dfuLong32 dfSizePrefixSfxLow = dfsFileWrapParam->dfuLong32erWrittenPosLow;
                dfuLong32 dfSizePrefixSfxHigh = dfsFileWrapParam->dfuLong32erWrittenPosHigh;
                dfSub64(&dfSizePrefixSfxLow,&dfSizePrefixSfxHigh,dfsThisFileSizeLow,dfsThisFileSizeHigh);
                if (dfSizePrefixSfxHigh>0)
                    fGoodOperation = FALSE;
                else
                    dfsFileWrapParam->dfSizePrefixSfx = dfSizePrefixSfxLow;
            }
        }
    }


    dfSub64(&dfsFileWrapParam->dfuLong32erWrittenPosLow,&dfsFileWrapParam->dfuLong32erWrittenPosHigh,
            dfsFileWrapParam->dfSizePrefixSfx+sizeof(DFSINFOBEGIN)+sizeof(DFSINFOEND),0);
    LowLevelSeek(dfsFileWrapParam->llfHandle, dfsFileWrapParam->dfSizePrefixSfx, 0, TYPESEEK_BEGIN, pei);

    if (LowLevelRead
        (dfsFileWrapParam->llfHandle, &DfsInfoBegin,
         sizeof(DFSINFOBEGIN),pei) != sizeof(DFSINFOBEGIN))
      fGoodOperation = FALSE;
    if (fGoodOperation)
    {
      fGoodOperation =
        CheckDfsInfoBegin(&DfsInfoBegin,
                          &dfsFileWrapParam->dfsBlockAlignementSize);
      if (fGoodOperation)
      {
          dfuLong32 i;
          dfuLong32 dfLog2=0;
          dfuLong32 dfBlockSizePossible=1;
          fGoodOperation=FALSE;
          for (i=0;i<32;i++)
          {
              if (dfBlockSizePossible==dfsFileWrapParam->dfsBlockAlignementSize)
              {
                  dfsFileWrapParam->dfsBlockAlignementLog2 = dfLog2;
                  fGoodOperation=TRUE;
                  break;
              }
              dfLog2++;
              dfBlockSizePossible *= 2;
          }
      }
    }
  }

  if (!fGoodOperation)
  {
    if (fHandleOpened)
    {
        LowLevelClose(dfsFileWrapParam->llfHandle, pei);
    }
    DfsFree(dfsFileWrapParam);
    dfsFileWrapParam = NULL;
  }

  return (DFSFILEWRAP) dfsFileWrapParam;
}



/*
Flush les donnÈe du fichier DFS.
ParamËtres :
DfsFileWrap :   Handle du fichier DFS
Valeur de retour : 0
*/
dfuLong32 DfsFlushWriteFile(DFSFILEWRAP dfsFileWrap,H_ERROR_INFO* pei)
{
  DFSFILEWRAPPARAM *dfsFileWrapParam = (DFSFILEWRAPPARAM *) dfsFileWrap;
  dfuLong32 dfError = DFS_SUCCESS;

  if (dfsFileWrap == NULL)
    return DFS_ERROR_BAD_PARAMETER;

  if ((!dfsFileWrapParam->fReadOnly) && (dfsFileWrapParam->fModified))
  {
    DFSINFOEND DfsInfoEnd;
    DFSINFOBEGIN DfsInfoBegin;
    dfuLong32 dfFileSizeLow,dfFileSizeHigh;
    dfuLong32 dfPreviousLow=0;
    dfuLong32 dfPreviousHigh=0;
    dfuLong32 dfSeekLow,dfSeekHigh;

    InitDfsInfoEnd(&DfsInfoEnd);

    dfFileSizeLow = dfsFileWrapParam->dfuLong32erWrittenPosLow;
    dfFileSizeHigh = dfsFileWrapParam->dfuLong32erWrittenPosHigh;
    dfAdd64(&dfFileSizeLow,&dfFileSizeHigh,dfsFileWrapParam->dfSizePrefixSfx + sizeof(DFSINFOBEGIN) + sizeof(DFSINFOEND),0);

    DfsInfoEnd.dfsThisFileSizeLow =
      DfsInfoEnd.dfsTotalSizeLow = ConvertuLongToLongIntel(dfFileSizeLow);
    DfsInfoEnd.dfsThisFileSizeHigh =
      DfsInfoEnd.dfsTotalSizeHigh = ConvertuLongToLongIntel(dfFileSizeHigh);
    DfsInfoEnd.dfVersionNeeded = ConvertuLongToLongIntel(MAKEVERSION(1,0));


    LowLevelTell(dfsFileWrapParam->llfHandle, &dfPreviousLow, &dfPreviousHigh);

    dfSeekLow = dfsFileWrapParam->dfuLong32erWrittenPosLow;
    dfSeekHigh = dfsFileWrapParam->dfuLong32erWrittenPosHigh;
    dfAdd64(&dfSeekLow,&dfSeekHigh,sizeof(DFSINFOBEGIN) + dfsFileWrapParam->dfSizePrefixSfx,0);


    if (!LowLevelSeek(dfsFileWrapParam->llfHandle,
                 dfsFileWrapParam->dfSizePrefixSfx,0, TYPESEEK_BEGIN, pei))
                 dfError = DFS_ERROR_ERRORIO;
    if (dfError == DFS_SUCCESS)
        if (LowLevelRead(dfsFileWrapParam->llfHandle, &DfsInfoBegin,
                    sizeof(DFSINFOBEGIN), pei) != sizeof(DFSINFOBEGIN))
                    dfError  = DFS_ERROR_ERRORIO;
    InitDfsInfoBegin(&DfsInfoBegin, FALSE, ConvertuLongIntelToLong(DfsInfoBegin.dfsBlockAlignement));

    DfsInfoBegin.dfMinimalVersionForOpen = ConvertuLongToLongIntel(MAKEVERSION(1,0));
    DfsInfoBegin.dfsThisFileSizeHigh = DfsInfoBegin.dfsTotalSizeHigh =
        ConvertuLongToLongIntel(dfFileSizeHigh);
    DfsInfoBegin.dfsThisFileSizeLow = DfsInfoBegin.dfsTotalSizeLow =
        ConvertuLongToLongIntel(dfFileSizeLow);

    if (!LowLevelSeek(dfsFileWrapParam->llfHandle,
                 dfsFileWrapParam->dfSizePrefixSfx,0, TYPESEEK_BEGIN, pei))
                 dfError = DFS_ERROR_ERRORIO;
    if (dfError == DFS_SUCCESS)
        if (LowLevelWrite(dfsFileWrapParam->llfHandle, &DfsInfoBegin,
                    sizeof(DFSINFOBEGIN),pei) != sizeof(DFSINFOBEGIN))
                    dfError  = DFS_ERROR_ERRORIO;


    if (!LowLevelSeek(dfsFileWrapParam->llfHandle,
                 dfSeekLow,dfSeekHigh, TYPESEEK_BEGIN,pei))
                 dfError = DFS_ERROR_ERRORIO;
    if (dfError == DFS_SUCCESS)
        if (LowLevelWrite(dfsFileWrapParam->llfHandle, &DfsInfoEnd,
                    sizeof(DFSINFOEND),pei) != sizeof(DFSINFOEND))
                    dfError  = DFS_ERROR_ERRORIO;

    if (dfError == DFS_SUCCESS)
     if (!LowLevelSetFileSize(dfsFileWrapParam->llfHandle,dfFileSizeLow,dfFileSizeHigh,pei))
         dfError  = DFS_ERROR_ERRORIO;

    if (!LowLevelSeek(dfsFileWrapParam->llfHandle,dfPreviousLow,dfPreviousHigh,TYPESEEK_BEGIN,pei))
        dfError  = DFS_ERROR_ERRORIO;

    dfsFileWrapParam->fModified = FALSE;
  }

  return dfError;
}


/*
Ferme un fichier DFS.
ParamËtres :
DfsFileWrap :   Handle du fichier DFS
Valeur de retour : 0
*/
dfuLong32 DfsCloseFile(DFSFILEWRAP dfsFileWrap,H_ERROR_INFO* pei)
{
  DFSFILEWRAPPARAM *dfsFileWrapParam = (DFSFILEWRAPPARAM *) dfsFileWrap;
  dfuLong32 dfError = DFS_SUCCESS;

  if (dfsFileWrap == NULL)
    return DFS_ERROR_BAD_PARAMETER;

  DfsFlushWriteFile(dfsFileWrap,pei);

  LowLevelClose(dfsFileWrapParam->llfHandle,pei);
  DfsFree(dfsFileWrapParam);
  return dfError;
}

/*
Retourne les flag permettant de savoir ce qui est possible avec le fichier.
ParamËtres :
DfsFileWrap :   Handle du fichier DFS
Valeur de retour : Combinaison des valeurs suivantes
DFS_STREAM :    implique qu'un seek est impossible.
DFS_READABLE :  Fichier permettant de lire
DFS_WRITABLE :  Fichier permettant d'Ècrire
DFS_NEWFILE :   Nouveau fichier. Ne peut Ítre prÈsent que si DFS_WRITABLE est prÈsent
*/
// Un fichier DFS_STREAM est soit DFS_READABLE soit DFS_WRITABLE | DFS_NEWFILE
dfuLong32 DfsGetStatus(DFSFILEWRAP dfsFileWrap,H_ERROR_INFO* pei)
{
  DFSFILEWRAPPARAM *dfsFileWrapParam = (DFSFILEWRAPPARAM *) dfsFileWrap;

  if (dfsFileWrap == NULL)
    return DFS_ERROR_BAD_PARAMETER;
  return dfsFileWrapParam->dfsFileInfoParam.dfStatus;
}



/*
Retourne la taille d'un fichier DFS.
ParamËtres :
DfsFileWrap :   Handle du fichier DFS
Valeur de retour : Taille du fichier DFS en octets

Un fichier avec le flag DFS_STREAM retournera DFS_ERROR_BADPARAMETER
*/
void DfsGetSize(DFSFILEWRAP dfsFileWrap, dfuLong32 *posLow, dfuLong32 *posHigh,H_ERROR_INFO* pei)
{
  DFSFILEWRAPPARAM *dfsFileWrapParam = (DFSFILEWRAPPARAM *) dfsFileWrap;
  dfuLong32 dfFileSizeLow,dfFileSizeHigh;

  if (dfsFileWrap == NULL)
  {
    *posLow = *posHigh = 0;
    return ;
  }

  LowLevelGetSize(dfsFileWrapParam->llfHandle,&dfFileSizeLow,&dfFileSizeHigh);
  dfSub64(&dfFileSizeLow,&dfFileSizeHigh,dfsFileWrapParam->dfSizePrefixSfx + sizeof(DFSINFOBEGIN) + sizeof(DFSINFOEND),0);
  *posLow = dfFileSizeLow;
  *posHigh = dfFileSizeHigh;
}

/*
Positionne le pointeur courant dans le fichier DFS (‡ partir du dÈbut du fichier).
ParamËtres :
DfsFileWrap :   Handle du fichier DFS
Valeur de retour : 0 en cas de succËs (DFS_SUCCESS)

Un fichier avec le flag DFS_STREAM retournera DFS_ERROR_BADPARAMETER
*/
dfuLong32 DfsSeek(DFSFILEWRAP dfsFileWrap, dfuLong32 posLow, dfuLong32 posHigh,H_ERROR_INFO* pei)
{
  DFSFILEWRAPPARAM *dfsFileWrapParam = (DFSFILEWRAPPARAM *) dfsFileWrap;

  if (dfsFileWrap == NULL)
    return DFS_ERROR_BAD_PARAMETER;
  dfAdd64(&posLow,&posHigh,sizeof(DFSINFOBEGIN)+dfsFileWrapParam->dfSizePrefixSfx,0);
  if (!LowLevelSeek
      (dfsFileWrapParam->llfHandle, posLow, posHigh, TYPESEEK_BEGIN, pei))
    return DFS_ERROR_BAD_PARAMETER;
  dfsFileWrapParam->dfuCurPosLow = posLow;
  dfsFileWrapParam->dfuCurPosHigh = posHigh;
  return DFS_SUCCESS;
}

dfuLong32 DfsSeekMulAlign(DFSFILEWRAP dfsFileWrap, dfuLong32 posLow, dfuLong32 posHigh,H_ERROR_INFO* pei)
{
  DFSFILEWRAPPARAM *dfsFileWrapParam = (DFSFILEWRAPPARAM *) dfsFileWrap;

  dfuLong32 i;

  if (dfsFileWrap == NULL)
    return DFS_ERROR_BAD_PARAMETER;

  for (i=0;i<dfsFileWrapParam->dfsBlockAlignementLog2;i++)
  {
      posHigh = (posHigh<<1) | (posLow>>31);
      posLow = (posLow<<1);
  }

  return DfsSeek(dfsFileWrap,posLow,posHigh,pei);
}

/*
Effectue une lecture dans le fichier .DFS
ParamËtres :
DfsFileWrap :   Handle du fichier DFS
Buf :   Pointeur vers le buffer recevant les donnÈes
Size :  Nombre d'octet ‡ lire
errorCode : Pointeur vers une variable recevant le code d'erreur (peut Ítre NULL)
Valeur de retour : Nombre d'octets lus.

Un fichier sans le flag DFS_READABLE retournera 0 et *errorCode ‡ DFS_ERROR_BADPARAMETER
Si le fichier est terminÈ, *errorcode est ‡ DFS_ERROR_EOF, sinon 0Ö
Avec Size = 0, on peut utiliser cette fonction pour savoir si la fin du fichier est atteinte.
*/
dfuLong32 DfsRead(DFSFILEWRAP dfsFileWrap, void *Buf, dfuLong32 Size,
                dfuLong32 * errorCode,H_ERROR_INFO* pei)
{
  int iread;
  DFSFILEWRAPPARAM *dfsFileWrapParam = (DFSFILEWRAPPARAM *) dfsFileWrap;
  if (dfsFileWrap == NULL)
  {
    if (errorCode != NULL)
      *errorCode = DFS_ERROR_BAD_PARAMETER;
    return 0;
  }

  if (errorCode != NULL)
    *errorCode = 0;
  iread = LowLevelRead(dfsFileWrapParam->llfHandle, Buf, Size, pei);
  if (iread == -1)
  {
    iread = 0;
    if (errorCode != NULL)
      *errorCode = DFS_ERROR_ERRORIO;
  }
  else
  {
    dfAdd64(&dfsFileWrapParam->dfuCurPosLow,&dfsFileWrapParam->dfuCurPosHigh, iread,0);
  }

  return iread;
}

/*
Effectue une Ècriture dans le fichier .DFS
ParamËtres :
DfsFileWrap :   Handle du fichier DFS
Buf :   Pointeur vers le buffer contenant les donnÈes
Size :  Nombre d'octet ‡ lire
errorCode : Pointeur vers une variable recevant le code d'erreur (peut Ítre NULL)
Valeur de retour : Nombre d'octets Ècrits.

Un fichier sans le flag DFS_WRITABLE retournera 0 et *errorCode ‡ DFS_ERROR_BADPARAMETER
*/
dfuLong32 DfsWrite(DFSFILEWRAP dfsFileWrap, const void *Buf,
                 dfuLong32 Size, dfuLong32 * errorCode,H_ERROR_INFO* pei)
{
  dfuLong32 iwrite;
  DFSFILEWRAPPARAM *dfsFileWrapParam = (DFSFILEWRAPPARAM *) dfsFileWrap;
  if (dfsFileWrap == NULL)
  {
    if (errorCode != NULL)
      *errorCode = DFS_ERROR_BAD_PARAMETER;
    return 0;
  }

  if (errorCode != NULL)
    *errorCode = 0;
  if (Size > 0)
      dfsFileWrapParam->fModified = TRUE;
  iwrite = LowLevelWrite(dfsFileWrapParam->llfHandle, Buf, Size, pei);
  if (iwrite != Size)
  {
    iwrite = 0;
    if (errorCode != NULL)
      *errorCode = DFS_ERROR_ERRORIO;
  }
  else
  {
    dfAdd64(&dfsFileWrapParam->dfuCurPosLow,&dfsFileWrapParam->dfuCurPosHigh, iwrite,0);
    if (dfCompareValue64(dfsFileWrapParam->dfuCurPosLow,dfsFileWrapParam->dfuCurPosHigh,
                         dfsFileWrapParam->dfuLong32erWrittenPosLow,dfsFileWrapParam->dfuLong32erWrittenPosHigh) == 1)
    {
        dfsFileWrapParam->dfuLong32erWrittenPosLow = dfsFileWrapParam->dfuCurPosLow;
        dfsFileWrapParam->dfuLong32erWrittenPosHigh = dfsFileWrapParam->dfuCurPosHigh;
    }
  }
  return iwrite;
}


/* Donne la position courante dans le fichier */
void DfsTell(DFSFILEWRAP dfsFileWrap, dfuLong32 *posLow, dfuLong32 *posHigh,
             dfuLong32 * errorCode,H_ERROR_INFO* pei)
{
  dfuLong32 dfTellRetLow,dfTellRetHigh;
  DFSFILEWRAPPARAM *dfsFileWrapParam = (DFSFILEWRAPPARAM *) dfsFileWrap;
  if (dfsFileWrap == NULL)
  {
    if (errorCode != NULL)
      *errorCode = DFS_ERROR_BAD_PARAMETER;
    return ;
  }

  if (errorCode != NULL)
    *errorCode = 0;
  LowLevelTell(dfsFileWrapParam->llfHandle,&dfTellRetLow,&dfTellRetHigh) ;
  dfSub64(&dfTellRetLow,&dfTellRetHigh,sizeof(DFSINFOBEGIN)+dfsFileWrapParam->dfSizePrefixSfx,0);
  *posLow=dfTellRetLow;
  *posHigh=dfTellRetHigh;
}

void DfsMarkEndNow(DFSFILEWRAP dfsFileWrap, dfuLong32 * errorCode,H_ERROR_INFO* pei)
{
  dfuLong32 dfTellLow,dfTellHigh;
  DFSFILEWRAPPARAM *dfsFileWrapParam = (DFSFILEWRAPPARAM *) dfsFileWrap;
  if (dfsFileWrap == NULL)
  {
    if (errorCode != NULL)
      *errorCode = DFS_ERROR_BAD_PARAMETER;
    return ;
  }

  if (errorCode != NULL)
    *errorCode = 0;

  LowLevelTell(dfsFileWrapParam->llfHandle,&dfTellLow,&dfTellHigh) ;
  dfSub64(&dfTellLow,&dfTellHigh,sizeof(DFSINFOBEGIN)+dfsFileWrapParam->dfSizePrefixSfx,0);
  dfsFileWrapParam->dfuLong32erWrittenPosLow = dfTellLow;
  dfsFileWrapParam->dfuLong32erWrittenPosHigh = dfTellHigh;
}

void DfsTellDivAlign(DFSFILEWRAP dfsFileWrap,dfuLong32 *posLow, dfuLong32* posHigh,
                     dfuLong32 * perrorCode,H_ERROR_INFO* pei)
{
  DFSFILEWRAPPARAM *dfsFileWrapParam = (DFSFILEWRAPPARAM *) dfsFileWrap;
  dfuLong32 dfsBlockAlignement ;
  dfuLong32 i ;
  dfuLong32 dfTellRetLow,dfTellRetHigh;
  dfuLong32 errorCode=0;

  if (dfsFileWrap == NULL)
  {
      if (perrorCode != NULL)
        *perrorCode = DFS_ERROR_BAD_PARAMETER;
      *posLow = *posHigh = 0;
      return ;
  }

  dfsBlockAlignement = dfsFileWrapParam->dfsBlockAlignementSize;
  if (dfsBlockAlignement == 0)
      dfsBlockAlignement = 1;

  DfsTell(dfsFileWrap,&dfTellRetLow,&dfTellRetHigh,&errorCode,pei);
  if (perrorCode!=NULL)
  {
      *perrorCode = errorCode;
  }

  if (((dfTellRetLow % dfsBlockAlignement) != 0) && (errorCode==0))
  {
      if (perrorCode != NULL)
        *perrorCode = DFS_ERROR_BAD_PARAMETER;
      return ;
  }

  for (i=0;i<dfsFileWrapParam->dfsBlockAlignementLog2;i++)
  {
      dfTellRetLow = (dfTellRetLow>>1) | (dfTellRetHigh<<31);
      dfTellRetHigh = (dfTellRetHigh>>1) ;
  }

  *posLow=dfTellRetLow;
  *posHigh=dfTellRetHigh;
}

#define AroundLower(dwValue,dwModulo) \
   ((((dfuLong32)dwValue) / ((dfuLong32)dwModulo)) * dwModulo)
#define AroundUpper(dwValue,dwModulo) \
   (((((dfuLong32)dwValue) + ((dfuLong32)dwModulo) -1) / ((dfuLong32)dwModulo)) * dwModulo)

dfuLong32 DfsReadOrWriteFillAlign(DFSFILEWRAP dfsFileWrap, BOOL fWrite,
                                  dfuLong32 * errorCode,H_ERROR_INFO* pei)
{
  dfuLong32 dfPosLow,dfPosHigh;
  dfuLong32 dfNbToAlign = 0;
  dfuLong32 error = 0;

  DFSFILEWRAPPARAM *dfsFileWrapParam = (DFSFILEWRAPPARAM *) dfsFileWrap;

  DfsTell(dfsFileWrap, &dfPosLow, &dfPosHigh,&error,pei);
  if (error != 0)
  {
    if (errorCode != NULL)
      *errorCode = error;
  }
  else
  {
    dfuLong32 dfsBlockAlignement = dfsFileWrapParam->dfsBlockAlignementSize;
    if (dfsBlockAlignement == 0)
      dfsBlockAlignement = 1;
    if (dfPosLow + dfsBlockAlignement > dfPosLow) // normal
      dfNbToAlign = AroundUpper(dfPosLow, dfsBlockAlignement) - dfPosLow;
    else
      dfNbToAlign = AroundUpper((dfPosLow-dfsBlockAlignement), dfsBlockAlignement) - (dfPosLow-dfsBlockAlignement);
    if (dfNbToAlign > 0)
    {
      dfbytep Buf;
      dfuLong32 i;
      Buf = (dfbytep) DfsMalloc(dfNbToAlign);
      if (Buf == NULL)
        error = DFS_ERROR_MEMORY_ERROR;
      else
      {

        for (i = 0; i < dfNbToAlign; i++)
          *(Buf + i) = 0;
        if (fWrite)
        {
          DfsWrite(dfsFileWrap, Buf, dfNbToAlign, &error, pei);
          dfsFileWrapParam->fModified = TRUE;
        }
        else
          DfsRead(dfsFileWrap, Buf, dfNbToAlign, &error, pei);
        DfsFree(Buf);
      }
    }
  }
  if (errorCode != NULL)
    *errorCode = error;
  return dfNbToAlign;
}

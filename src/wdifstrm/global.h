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




#include "../../lib/engine/patchstream/common/difbasic.h"
#include "../../lib/engine/svfile/common/DfsMFile.h"
#include "../../lib/engine/patchstream/common/DfsTlTyp.h"
#include "../../lib/engine/patchstream/common/difstool.h"
#include "../../lib/engine/svfile/common/DfsStruc.h"
#include "../../lib/engine/svfile/common/DfsTagDf.h"
#include "../../lib/engine/svfile/common/DfsTagMg.h"
#include "../../lib/engine/svfile/common/DfsTagBlockFloatEnd.h"

#include "../../lib/engine/svfile/common/DfsIntf.h"
#include "../../lib/engine/patchstream/common/DfsOrigMemoryMap.h"
#include "../../lib/engine/patchstream/rebuild/RamDifWk.h"
#include "../../lib/engine/svfile/common/DfsIntfL.h"
#include "../../lib/engine/patchstream/common/DfsIoHlp.h"
#include "../../lib/engine/svfile/common/DfsSet.h"
#include "../../lib/engine/svfile/common/DfsSetTl.h"
#include "../../lib/engine/svfile/decompress/DfsRdSet.h"


#include "LruMenu.h"

#ifndef S_EXTERN
#define S_EXTERN extern
#endif

S_EXTERN HINSTANCE ghInst;
S_EXTERN HINSTANCE ghInstRes;
S_EXTERN HBITMAP hBitmapDown,hBitmapUp;

typedef struct
{
  DFSFILE DfsFile;
  LPCTSTR lpszDfsFileName;
  dfuLong32 dfNbDir;
  PDIRINFO* pDirInfo ;
  dfuLong32 dfCurDir;
  dfuLong32 dfNbFileSelectedCurDir;
  dfuLong64 dfSizeSelectedCurDir;
  dfuLong64 dfSizeTotalCurDir;
  dfuLong32 dfNbFileTotalCurDir;

  BOOL fBaseDirectorySelected;
  BOOL fBaseDirectoryNeeded;
  BOOL fNotifyAccepted;
  LPCSTR lpBaseDirectory;
  dfuLong32 dfBaseDirNum;
} DFSFILEANDINFO;

typedef struct _WINSIZE
    {
    BOOL fMax;
    int x;
    int y;
    int cx;
    int cy;
    } WINSIZE, FAR *PWINSIZE;


#define IDD_LISTVIEW        31000
#define IDD_TREEVIEW        31001
//#define GetMULingResHinst() (hInst)
#define NBLVCOLUMNFILELIST (9)
#define NBLVCOLUMNDIRLIST (8)
#define MAXLRU (9)

#define COLUMNWIDTH_UNKNOWN (0xffff)

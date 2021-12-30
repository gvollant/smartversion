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

/* DfsWrSet.c */
#include <stddef.h>
#include <stdio.h>

#include "../../patchstream/common/difbasic.h"
#include "DfsMFile.h"
#include "../../patchstream/common/DfsTlTyp.h"
#include "../../patchstream/common/difstool.h"
#include "DfsStruc.h"
#include "DfsTagDf.h"
#include "DfsTagMg.h"
#include "DfsTagBlockFloatEnd.h"

#include "DfsIntf.h"
#include "../../patchstream/common/DfsOrigMemoryMap.h"
#include "../../patchstream/common/DfsIoHlp.h"
#include "DfsSet.h"
#include "DfsSetTl.h"

void InitProgressCallBackInfo(PROGRESSCALLBACKINFO * pProgressCallBackInfo,
                              DFSFILE DfsFile, dfvoidp dfUserPtr)
{
  DfsClearStruct(pProgressCallBackInfo, 0, sizeof(PROGRESSCALLBACKINFO));
  pProgressCallBackInfo->DfsFile = DfsFile;
  pProgressCallBackInfo->dfUserPtr = dfUserPtr;
}


BOOL CallCallBack(tProgressCallBack pProgressCallBack,
                  PROGRESSCALLBACKINFO * pProgressCallBackInfo,
                  dfuLong32 dfEvent, dfvoidp dfDataEventPtr)
{
  if (pProgressCallBackInfo == NULL)
    return FALSE;
  if (pProgressCallBack == NULL)
    return TRUE;
  pProgressCallBackInfo->dfEvent = dfEvent;
  pProgressCallBackInfo->dfDataEventPtr = dfDataEventPtr;
  return (*pProgressCallBack) (pProgressCallBackInfo);
}

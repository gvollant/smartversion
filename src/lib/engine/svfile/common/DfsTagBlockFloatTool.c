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

//#include <windows. h>
#include <stdlib.h>
#include "../../patchstream/common/difbasic.h"

#include "../../patchstream/common/DfsTlTyp.h"
#include "../../patchstream/common/difstool.h"
#include "DfsStruc.h"
#include "DfsTagDf.h"
#include "DfsTagMg.h"
#include "DfsTagBlockFloatEnd.h"

#include "DfsIntf.h"

#include "DfsSet.h"


#include "DfsTagBlockFloatTool.h"

BOOL DuplicateDirectoryFloatBlock(DFSFILE DfsFileRead,DFSFILE DfsFileWrite,dfuLong32 i,dfuLong32 dfNbVersionWritten, H_ERROR_INFO * pei)
{
//    dfvoidp TagBuf;
//    dfuLong32 TagSize;
    DFTAGBLOCKFLOAT TagBlockFloat;
    DFTAGBLOCKFLOAT TagBlockFloatWriting;
    BOOL fRet=TRUE;

    TagBlockFloat = GetDfsTagBlockFloat(DfsFileRead, pei);
    TagBlockFloatWriting = GetDfsTagBlockFloat(DfsFileWrite, pei);

    if (TagBlockFloat == NULL)
        fRet=FALSE;

    if (fRet)
    {
        fRet=DuplicateTagBlockFloatRange(TagBlockFloat,i,FLOATNUM_NOSPECIFIC,
                     TagBlockFloatWriting,dfNbVersionWritten,FLOATNUM_NOSPECIFIC,
                     DFSTAG_FIRST_FLOAT_DIR_RECOPY,DFSTAG_LAST_FLOAT_DIR_RECOPY);
        if (fRet)
            SetDfsTagBlockFloatDirty(DfsFileWrite, pei);
    }
        /*
    {
        if (GetTagBlockFloat(TagBlockFloat,i,FLOATNUM_NOSPECIFIC,DFSTAG_DIR_NAME_FLOAT,&TagBuf, &TagSize))
        {
            DFTAGBLOCKFLOAT TagBlockFloatWriting;
            TagBlockFloatWriting = GetDfsTagBlockFloat(DfsFileWrite, pei);
            if (TagBlockFloatWriting != NULL)
                AddTagBlockFloat(TagBlockFloatWriting,dfNbVersionWritten,
                                    FLOATNUM_NOSPECIFIC,DFSTAG_DIR_NAME_FLOAT,
                                    TagBuf, TagSize);
            SetDfsTagBlockFloatDirty(DfsFileWrite, pei);
            //DfsFlushWriteDfsFile(DfsFileWrite);
        }

        if (GetTagBlockFloat(TagBlockFloat,i,FLOATNUM_NOSPECIFIC,DFSTAG_DIR_COMMENT_FLOAT,&TagBuf, &TagSize))
        {
            DFTAGBLOCKFLOAT TagBlockFloatWriting;
            TagBlockFloatWriting = GetDfsTagBlockFloat(DfsFileWrite, pei);
            if (TagBlockFloatWriting != NULL)
                AddTagBlockFloat(TagBlockFloatWriting,dfNbVersionWritten,
                                    FLOATNUM_NOSPECIFIC,DFSTAG_DIR_COMMENT_FLOAT,
                                    TagBuf, TagSize);
            SetDfsTagBlockFloatDirty(DfsFileWrite, pei);
            //DfsFlushWriteDfsFile(DfsFileWrite, pei);
        }

    }*/
    return fRet;
}

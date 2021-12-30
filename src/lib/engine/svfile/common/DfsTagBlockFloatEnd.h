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

/* DfsTagBlockFloatEnd.h */
/* Managenet of tag at the end of DFS File (tag which are modified) */

#ifndef DFS_TAG_BLOCK_FLOAT_H_INCLUDED
#define DFS_TAG_BLOCK_FLOAT_H_INCLUDED


#if defined(__cplusplus) && (!defined(ALLINCPP))
extern "C" {
#endif

DECLARE_DFHANDLE(DFTAGBLOCKFLOAT);

#define FLOATNUM_NOSPECIFIC (0xffffffff)

DFTAGBLOCKFLOAT SVFAPI BuildEmptyBlockFloat();

DFTAGBLOCKFLOAT SVFAPI ReadDfTagBlockFloat(dfvoidp ptr, dfuLong32 * pTagListPackedSize);

DFTAGBLOCKFLOAT SVFAPI ReadDfTagBlockFloatSizeLimited(dfvoidp ptr, dfuLong32 * pTagListPackedSize, dfuLong32 dfLimit);

BOOL SVFAPI CloseDfTagBlockFloat(DFTAGBLOCKFLOAT);


BOOL SVFAPI AddTagBlockFloat(DFTAGBLOCKFLOAT TagBlockFloat, dfuLong32 dfDirNum,
                             dfuLong32 dfFileNum,
                             dfuLong32 TagNumber, dfvoidpc TagBuf, dfuLong32 TagSize);

BOOL SVFAPI AddTaguLongBlockFloat(DFTAGBLOCKFLOAT TagBlockFloat,
                                  dfuLong32 dfDirNum, dfuLong32 dfFileNum, dfuLong32 TagNumber, dfuLong32 value);


BOOL SVFAPI GetTagBlockFloat(DFTAGBLOCKFLOAT TagBlockFloat,
                             dfuLong32 dfDirNum, dfuLong32 dfFileNum, dfuLong32 TagNumber, dfvoidp * pTagBuf,
                             dfuLong32 * pTagSize);

BOOL SVFAPI GetTaguLongBlockFloat(DFTAGBLOCKFLOAT TagBlockFloat, dfuLong32 dfDirNum, dfuLong32 dfFileNum, dfuLong32 TagNumber, dfuLong32 * value);

BOOL SVFAPI RemoveTagBlockFloat(DFTAGBLOCKFLOAT TagBlockFloat, dfuLong32 dfDirNum, dfuLong32 dfFileNum, dfuLong32 TagNumber);

BOOL SVFAPI DuplicateTagBlockFloatRange(DFTAGBLOCKFLOAT TagBlockFloatSrc, dfuLong32 dfDirNumSrc, dfuLong32 dfFileNumSrc,
                                 DFTAGBLOCKFLOAT TagBlockFloatDst, dfuLong32 dfDirNumDst, dfuLong32 dfFileNumDst,
                                 dfuLong32 TagNumberFirst,
                                 dfuLong32 TagNumberLast);

#if defined(__cplusplus) && (!defined(ALLINCPP))
}
#endif

#endif

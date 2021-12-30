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




typedef struct
{
    dfuLong32 dfDirNum;
    dfuLong32 dfFileNum;
    DFTAGLIST dfTagList;
} DFTAGBLOCK;

typedef struct
{
    dfuLong32 dfNbTagBlockUsed;
    dfuLong32 dfNbTagBlockAllocated;
    dfuLong32 dfNbTagAllocStep;
    DFTAGBLOCK* pdfTabBlock;
} DFTAGBLOCKFLOATINTERNAL;

typedef struct
{
    dfuLong32Intel dfDirNum;
    dfuLong32Intel dfFileNum;
} DFTAGBLOCKSTOREDLOCALHEADER;

typedef struct
{
  dfuLong32Intel dfStoreMethod;
  dfuLong32Intel dfSizeFloatStreamCompressed;
  dfuLong32Intel dfSizeFloatStreamUncompressed;
  dfuLong32Intel dfCrc32Tags;
} DFTAGBLOCKSTOREDGENERALHEADER;



#define TAGBLOCKFLOATSTOREMETHOD_NONE     (0)
#define TAGBLOCKFLOATSTOREMETHOD_DEFLATE  (2)


BOOL SVFAPI CheckPtrBufSize(dfvoidp* ptr,dfuLong32* pdfSizeAllocated, dfuLong32 dfSizeNeeded, dfuLong32 dfStepAlloc);



BOOL
SVFAPI BuildTagListPacking(DFTAGLIST TagList, dfvoidp * ptr, dfuLong32 * size,
                    BOOL compressed);


BOOL SVFAPI FreeTagPtr(dfvoidp ptr);

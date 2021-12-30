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

/* SvfDll.h
    Main header of SmartVersion SDK
    Please include this header in your file */


/* difbasic.h define basic type and structure for SmartVersion engine */
#include "../lib/engine/patchstream/common/difbasic.h"

/* DfsTlTyp.h define complex structure for SmartVersion engine and SVF file */
#include "../lib/engine/patchstream/common/DfsTlTyp.h"

/* DifsTool.h Basic tools (disk, memory, date, charset, error management) */
#include "../lib/engine/patchstream/common/difstool.h"

/* ArrayTl.h basic function to manage an array of fixed item in memory */
#include "../lib/engine/svfile/common/ArrayTl.h"

/* DfsOrigMemoryMap.h define macro to get access on the original version of a file */
#include "../lib/engine/patchstream/common/DfsOrigMemoryMap.h"

/* DfsIOhlp.h Basic low level disk function, including memory mapped */
#include "../lib/engine/patchstream/common/DfsIoHlp.h"

/* DfsStruc.h Structure of meta header on SVF file */
#include "../lib/engine/svfile/common/DfsStruc.h"

/* DfsTagDf.h define constant used to store tag */
#include "../lib/engine/svfile/common/DfsTagDf.h"

/* DfsTagMg.h define function used to store tag */
#include "../lib/engine/svfile/common/DfsTagMg.h"

/* DfsTagBlockFloatEnd.h
   Managenet of tag at the end of DFS File (tag which are modified) */
#include "../lib/engine/svfile/common/DfsTagBlockFloatEnd.h"

/* difstrm.h contain a function to create a "dummy" stream with identical patch */
#include "../lib/engine/patchstream/common/difstrm.h"

/* apldifst.h function and structure to execute a stream patch from two file */
#include "../lib/engine/patchstream/decompress/apldifst.h"

/* DfsIntf.h basic function and structure to work on SVF file */
#include "../lib/engine/svfile/common/DfsIntf.h"

/* dfsSet.h define callback used during work on SVF file */
#include "../lib/engine/svfile/common/DfsSet.h"

/* RamDifWk.h */
/* function to deal merging patch in ramdif */
#include "../lib/engine/patchstream/rebuild/RamDifWk.h"

/* DfsRdSet.h */
/* function to extract a version into SVF file, based on previous version */
#include "../lib/engine/svfile/decompress/DfsRdSet.h"

/* DirSet.h basic structure and tools to extract several version at a time */
#include "../lib/engine/svfile/common/DirSet.h"

/* DoExtracting.h Extract several version in one time */
#include "../lib/engine/svfile/decompress/DoExtracting.h"

/* RemixDfs.h Modify version in a Dfs */
#include "../lib/engine/svfile/rebuild/ReMixDfs.h"

/* ReMixHelper.h function to be used with Remix */
#include "../lib/helper/rebuild/ReMixHelper.h"

/* AppendDfs.h append one SVF file to another */
#include "../lib/engine/svfile/compress/AppendDfs.h"

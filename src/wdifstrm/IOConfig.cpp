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

//


#include "IOConfig.h"
#include <stddef.h>
#include <stdio.h>

#include "..\..\lib\engine\patchstream\common\difbasic.h"
#include "..\..\lib\engine\svfile\common\DfsMFile.h"
#include "..\..\lib\engine\patchstream\common\DfsTlTyp.h"
#include "..\..\lib\engine\patchstream\common\difstool.h"
#include "..\..\lib\engine\svfile\common\DfsStruc.h"
#include "..\..\lib\engine\svfile\common\DfsTagDf.h"
#include "..\..\lib\engine\svfile\common\DfsTagMg.h"
#include "..\..\lib\engine\svfile\common\DfsTagBlockFloatEnd.h"

#include "..\..\lib\engine\svfile\common\DfsIntf.h"
#include "..\..\lib\engine\patchstream\common\DfsOrigMemoryMap.h"
#include "..\..\lib\engine\patchstream\rebuild\RamDifWk.h"
#include "..\..\lib\engine\svfile\common\DfsIntfL.h"
#include "..\..\lib\engine\patchstream\common\DfsIoHlp.h"

#include "IOConfig.h"

int IOConfig_SetVirtualFileNameMaximumMemory(int fLimitSize,unsigned long dfMaxSize)
{
	return (int)dfsSetVirtualFileNameMaximumMemory((BOOL) fLimitSize,(dfuLong64) dfMaxSize);
}

int IOConfig_dfsUnInitVirtualFileNameSpace(int fClearMaximumMemoryValue)
{
	return (int)dfsUnInitVirtualFileNameSpace((BOOL)fClearMaximumMemoryValue);
}

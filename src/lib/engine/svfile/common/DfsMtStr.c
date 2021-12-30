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

/* */
#include <stdlib.h>

#include "../../patchstream/common/difbasic.h"
#include "../../patchstream/common/DfsTlTyp.h"
#include "../../patchstream/common/difstool.h"

#include "DfsMtStr.h"
#include "zlib.h"

#define MAGICTEXT ("DFS File\x0d\x0ahttp://www.difstream.com\x0d\x0a")
#define MAGICTEXT2 ("SmartVersion File (.SVF)\x0d\x0asee http://www.smartversion.com\x0d\x0a")

#define MAGICTEXTSIZE (36)
#define MAGICTEXTSIZE2 (59)

BOOL InitDfsInfoBegin(DFSINFOBEGIN * pDfsInfoBegin, BOOL fNewFile, dfuLong32 dfBlockAlign)
{
  if (fNewFile)
    DfsClearStruct(pDfsInfoBegin, 0, sizeof(DFSINFOBEGIN));
  else
    DfsClearStruct(pDfsInfoBegin->szMagicText, 0, sizeof(pDfsInfoBegin->szMagicText));
  DfsMemcpy(&(pDfsInfoBegin->szMagicText), MAGICTEXT, MAGICTEXTSIZE);
  pDfsInfoBegin->dfMagic = ConvertuLongToLongIntel(0x46445647);
  pDfsInfoBegin->dfsBlockAlignement = ConvertuLongToLongIntel(dfBlockAlign);

  pDfsInfoBegin->dfMinimalVersionForOpen = ConvertuLongToLongIntel(MAKEVERSION(1,0));
  pDfsInfoBegin->dfCrc32MagicText = ConvertuLongToLongIntel((dfuLong32)crc32(0,
                         (dfbytep)pDfsInfoBegin->szMagicText, sizeof(pDfsInfoBegin->szMagicText)));

  return TRUE;
}

BOOL CheckDfsInfoBegin(const DFSINFOBEGIN * pDfsInfoBegin,
                       dfuLong32 * pdfBlockAlign)
{
  dfuLong32 dfMinimalVersionForOpen;
  if ((DfsMemcmp(&(pDfsInfoBegin->szMagicText), MAGICTEXT, MAGICTEXTSIZE) != 0) &&
      (DfsMemcmp(&(pDfsInfoBegin->szMagicText), MAGICTEXT2, MAGICTEXTSIZE2) != 0))
    return FALSE;
  if (ConvertuLongIntelToLong(pDfsInfoBegin->dfMagic) != 0x46445647)
    return FALSE;
  if (pdfBlockAlign != NULL)
    *pdfBlockAlign =
      ConvertuLongIntelToLong(pDfsInfoBegin->dfsBlockAlignement);

  dfMinimalVersionForOpen = ConvertuLongIntelToLong(pDfsInfoBegin->dfMinimalVersionForOpen);
  if (dfMinimalVersionForOpen > MAKEVERSION(1,0))
      return FALSE;
  return TRUE;
}

BOOL InitDfsInfoEnd(DFSINFOEND * pDfsInfoEnd)
{
  DfsClearStruct(pDfsInfoEnd, 0, sizeof(DFSINFOEND));
  pDfsInfoEnd->dfEndMagic = ConvertuLongToLongIntel(0x46445647);

  return TRUE;
}

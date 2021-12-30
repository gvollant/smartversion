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

#include <stddef.h>
#include <string.h>

#include "difbasic.h"
#include "DfsTlTyp.h"
#include "difstool.h"



dfuLong32 SVFAPI dfUnicodeStrlen(const dfwchar * lpszUnicode)
{
  dfuLong32 i = 0;
  if (lpszUnicode == NULL)
    return 0;
  while ((*(lpszUnicode + i)) != 0)
    i++;
  return i;
}

dfuLong32 SVFAPI dfUnicodeCopyConcat(dfwcharp lpszUnicodeDest,
                                     dfwcharpc lpszUnicodeSrc1,
                                     dfwcharpc lpszUnicodeSrc2)
{
  dfuLong32 dfSize1, dfSize2;
  if (lpszUnicodeSrc1 != NULL)
    dfSize1 = dfUnicodeStrlen(lpszUnicodeSrc1);
  else
    dfSize1 = 0;

  if (lpszUnicodeSrc2 != NULL)
    dfSize2 = dfUnicodeStrlen(lpszUnicodeSrc2);
  else
    dfSize2 = 0;

  if ((dfSize1 > 0) && (lpszUnicodeDest != lpszUnicodeSrc1))
    DfsMemcpy(lpszUnicodeDest, lpszUnicodeSrc1, dfSize1 * sizeof(dfwchar));
  if (dfSize2 > 0)
    DfsMemcpy(lpszUnicodeDest + dfSize1, lpszUnicodeSrc2,
              dfSize2 * sizeof(dfwchar));
  *(lpszUnicodeDest + dfSize1 + dfSize2) = 0;

  return dfSize1 + dfSize2;
}


void SVFAPI DfUnicodeStrcpy(dfwcharp dest,dfwcharpc src)
{
    dfUnicodeCopyConcat(dest,src,NULL);
}

void SVFAPI DfUnicodeStrcat(dfwcharp dest,dfwcharpc src)
{
    dfUnicodeCopyConcat(dest,dest,src);
}

void SVFAPI DfUnicodeStrcat(dfwcharp,dfwcharpc);

dfwcharp SVFAPI dfUnicodeCopyConcatAlloc(dfwcharpc lpszUnicodeSrc1,
                                  dfwcharpc lpszUnicodeSrc2)
{
  dfuLong32 dfSize1, dfSize2;
  dfwcharp lpszUnicodeDest;

  if (lpszUnicodeSrc1 != NULL)
    dfSize1 = dfUnicodeStrlen(lpszUnicodeSrc1);
  else
    dfSize1 = 0;

  if (lpszUnicodeSrc2 != NULL)
    dfSize2 = dfUnicodeStrlen(lpszUnicodeSrc2);
  else
    dfSize2 = 0;

  lpszUnicodeDest =
    (dfwcharp) DfsMalloc((dfSize1 + dfSize2 + 1) * sizeof(dfwchar));
  if (dfSize1 > 0)
    DfsMemcpy(lpszUnicodeDest, lpszUnicodeSrc1, dfSize1 * sizeof(dfwchar));
  if (dfSize2 > 0)
    DfsMemcpy(lpszUnicodeDest + dfSize1, lpszUnicodeSrc2,
              dfSize2 * sizeof(dfwchar));
  *(lpszUnicodeDest + dfSize1 + dfSize2) = 0;
  return lpszUnicodeDest;
}

dfwcharp SVFAPI dfUnicodeCopyAlloc(dfwcharpc lpszUnicodeSrc)
{
    return dfUnicodeCopyConcatAlloc(lpszUnicodeSrc,NULL);
}



dfuLong32 SVFAPI CalculateRatio(dfuLong64 dfValue, dfuLong64 dfTotal, dfuLong32 dfWidth)
{
  if (dfValue==0)
      return 0;

  if (dfTotal==0)
      return 0;

  if ((((dfValue * dfWidth) / dfValue) != dfWidth) &&
      (dfTotal > dfWidth))
  {
    return (dfuLong32)((dfValue) / (dfTotal / dfWidth));
  }
  else
  {
    return (dfuLong32)((dfValue * dfWidth) / dfTotal);
  }
}


BOOL SVFAPI CompareUnicodeWithSimpleChar(dfwchar wc,char c)
{
    DFWCHAR_BYTEACCESS dfwb;
    dfwb.u.wc = wc;
    if (dfwb.u.b[1] != 0)
        return FALSE;
    return (dfwb.u.b[0] == c);
}

dfwchar ConvertAnsiItemToUnicodeItem(char cItem)
{
    char cTab[8];
    dfwchar wTab[8];
    wTab[0] = wTab[1] = 0;
    cTab[0] = cTab[1] = 0;
    cTab[0] = cItem;
    ConvertAnsiToUnicode(cTab,wTab,2);
    return wTab[0];
}

char ConvertUnicodeItemToAnsiItem(dfwchar wItem)
{
    char cTab[8];
    dfwchar wTab[8];
    wTab[0] = wTab[1] = 0;
    cTab[0] = cTab[1] = 0;
    wTab[0] = wItem;
    ConvertUnicodeToAnsi(wTab,cTab,2);
    return cTab[0];
}

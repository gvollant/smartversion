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

/* DfsTagMg.h */
/* define function used to store tag */

#if defined(__cplusplus) && (!defined(ALLINCPP))
extern "C" {
#endif


DFTAGLIST SVFAPI AllocNewTagList();

BOOL SVFAPI IsTagListEmpty(DFTAGLIST TagList);

BOOL SVFAPI AddTag(DFTAGLIST TagList, dfuLong32 TagNumber, dfvoidpc TagBuf,
                   dfuLong32 TagSize);

BOOL SVFAPI DuplicateTagRange(DFTAGLIST TagListSrc,
                              DFTAGLIST TagListDst,
                              dfuLong32 TagNumberFirst,
                              dfuLong32 TagNumberLast);

BOOL SVFAPI RemoveTag(DFTAGLIST TagList, dfuLong32 TagNumber);

BOOL SVFAPI AddTaguLong(DFTAGLIST TagList, dfuLong32 TagNumber, dfuLong32 value);

dfuLong32 SVFAPI GetCountOfTags(DFTAGLIST TagList);

dfuLong32 SVFAPI GetTagNumberAtPos(DFTAGLIST TagList, dfuLong32 pos);


/****************************************************************************/

#define READTAGSIZE_NOSIZELIMIT (0xffffffff)
DFTAGLIST SVFAPI ReadTagListSizeLimited(dfvoidp ptr, dfuLong32 * pTagListPackedSize,
                                        dfuLong32 dfLimit);

DFTAGLIST SVFAPI ReadTagList(dfvoidp ptr, dfuLong32 * pTagListPackedSize);

BOOL SVFAPI CloseTagList(DFTAGLIST TagList);

BOOL SVFAPI GetTag(DFTAGLIST TagList, dfuLong32 TagNumber, dfvoidp * pTagBuf,
                   dfuLong32 * pTagSize);

BOOL SVFAPI GetTaguLong(DFTAGLIST TagList, dfuLong32 TagNumber, dfuLong32 * value);


/*

  Tag relative to file
- DFSTAG_FILENAME : nom du fichier en unicode sans le zéro terminal.
- DFSTAG_DATE : Date du fichier au format DFSINFODATE
- DFSTAG_BASEPATCH

  // only in final dir
- DFSTAG_FILEPOSPROPERTIES : localisation du fichier, en octets (sur 4 ou 8 octets).
- DFSTAG_CRCINFO : tableau de structures DFSCRCINFO (on connaît le nombre de CRC avec la taille du TAG).



  Tag relative to the directory
- DFSTAG_NUMBEROFFILEINDIR
*/



#if defined(__cplusplus) && (!defined(ALLINCPP))
}
#endif

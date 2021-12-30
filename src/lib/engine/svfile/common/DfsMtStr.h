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


/* description of general structure of DFS File
   Init on DfsMtStr.c */
typedef struct
{
  char szMagicText[60];
  dfuLong32Intel dfCrc32MagicText;
  dfuLong32Intel dfMagic;
  dfuLong32Intel dfMinimalVersionForOpen;
  dfuLong32Intel dfsVolumeSizeHigh;
  dfuLong32Intel dfsVolumeSizeLow;
  dfuLong32Intel dfsTotalSizeHigh;
  dfuLong32Intel dfsTotalSizeLow;
  dfuLong32Intel dfsThisFileSizeHigh;
  dfuLong32Intel dfsThisFileSizeLow;
  dfuLong32Intel dfsBlockAlignement;
  dfuLong32Intel dfsVolumeNumber;
}
DFSINFOBEGIN;


typedef struct
{
  dfuLong32Intel dfVersionNeeded;
  dfuLong32Intel dfsThisVolumeNumber;
  dfuLong32Intel dfsTotalNumberOfVolume;

  dfuLong32Intel dfsTotalSizeHigh;
  dfuLong32Intel dfsTotalSizeLow;
  dfuLong32Intel dfsThisFileSizeLow;
  dfuLong32Intel dfsThisFileSizeHigh;
  dfuLong32Intel dfsFlags;
  dfuLong32Intel dfEndMagic;
}
DFSINFOEND;

BOOL InitDfsInfoBegin(DFSINFOBEGIN * pDfsInfoBegin, BOOL fNewFile, dfuLong32 dfBlockAlign);
BOOL CheckDfsInfoBegin(const DFSINFOBEGIN * pDfsInfoBegin,
                       dfuLong32 * pdfBlockAlign);
BOOL InitDfsInfoEnd(DFSINFOEND * pDfsInfoEnd);

#define MAKEVERSION(major,minor) (((major) << 16)| (minor))
#define FILEDESC_LOCALFILEINFO 0x00000001

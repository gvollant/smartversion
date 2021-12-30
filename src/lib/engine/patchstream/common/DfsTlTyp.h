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

/* DfsTlTyp.h */
/* Define complex structure for SmartVersion engine and SVF file */

#ifndef __DFS_TL_TYPE_INLCUDED
#define __DFS_TL_TYPE_INLCUDED

typedef struct
{
  unsigned char tab[2];
}
dfuInt16Intel;

typedef struct
{
  unsigned char tab[4];
}
dfuLong32Intel;

typedef struct
{
  unsigned char tab[8];
}
dfuLong64Intel;

typedef struct
{
  dfbyte dfsInfoDateByte[8];
}
DFSINFODATE;

typedef struct
{
    dfuLong64Intel dfSizeDeplInPlaceIntel;
    dfuLong64Intel dfSizeDeplOutPlaceIntel;
}
DFSSTORAGEPATCHINFOINTEL;


typedef struct
{
  dfuLong32Intel dfTypeCrcInfo;
  dfuLong32Intel dfBeginPos;
  dfuLong32Intel dfEndPos;
  dfuLong32Intel dfCrcValue;
}
DFSCRCINFOONLYCRC32_32;

typedef struct
{
  dfuLong32Intel dfTypeCrcInfo;
  dfuLong32Intel dfBeginPos;
  dfuLong32Intel dfEndPos;
  dfuLong32Intel dfCrcValue;
  dfuLong32Intel dfBeginPosHigh;
  dfuLong32Intel dfEndPosHigh;
}
DFSCRCINFOONLYCRC32_64;


typedef struct
{
  dfuLong32Intel dfTypeCrcAndMd5Info;
  dfuLong32Intel dfBeginPos;
  dfuLong32Intel dfEndPos;
  dfuLong32Intel dfCrcValue;
  dfbyte         bMd5[16];
} DFSCRCINFOWITHMD5_32;

typedef struct
{
  dfuLong32Intel dfTypeCrcAndMd5Info;
  dfuLong32Intel dfBeginPos;
  dfuLong32Intel dfEndPos;
  dfuLong32Intel dfCrcValue;
  dfbyte         bMd5[16];
  dfuLong32Intel dfBeginPosHigh;
  dfuLong32Intel dfEndPosHigh;
} DFSCRCINFOWITHMD5_64;

typedef struct
{
  dfuLong32Intel dfTypeCrcAndMd5Info;
  dfuLong32Intel dfBeginPos;
  dfuLong32Intel dfEndPos;
  dfuLong32Intel dfCrcValue;
  dfbyte         bBinDataPart1[16];
  dfuLong32Intel dfBeginPosHigh;
  dfuLong32Intel dfEndPosHigh;
  dfuLong32Intel dfStructSize;
  dfbyte         bBinDataPart2[20];
} DFSCRCINFOWITHMD5SHA1_64;


typedef struct
{
  dfuLong32Intel dfTypeCrcAndMd5Info;
  dfuLong32Intel dfBeginPos;
  dfuLong32Intel dfEndPos;
  dfuLong32Intel dfCrcValue;
  dfbyte         bBinDataPart1[16];
  dfuLong32Intel dfBeginPosHigh;
  dfuLong32Intel dfEndPosHigh;
  dfuLong32Intel dfStructSize;
  dfbyte         bBinDataPart2[20+32];
} DFSCRCINFOWITHMD5SHA1SHA256_64;

#define DFSCRCINFO_SIZE_MINIMAL (sizeof(DFSCRCINFOONLYCRC32_32))
#define DFSCRCINFO_SIZE_MAXIMAL (sizeof(DFSCRCINFOWITHMD5SHA1SHA256_64))

typedef struct
{
    dfbyte data[DFSCRCINFO_SIZE_MAXIMAL];
} DFSCRCINFO_FULLSIZESTRUCTURE;

typedef struct
{
  dfuLong32 dfTypeCrcAndMd5Info;
  dfuLong64 dfBeginPos;
  dfuLong64 dfEndPos;
  dfuLong32 dfCrc32Value;
  BOOL      fMd5;
  dfbyte    bMd5[16];
  BOOL      fSha1;
  dfbyte    bSha1[20];
  BOOL      fSha256;
  dfbyte    bSha256[32];
  //++SHA256
} DFSCRCINFOPARAM;

#define TYPECRCINFO_CRC32               (0x00000001)
#define TYPECRCINFO_MD5                 (0x00000002)
#define TYPECRCINFO_CRC32MD5            (0x00000003)
#define TYPECRCINFO_MD5EMPTY            (0x00000004)
#define TYPECRCINFO_STRUCTSIZEPRESENT   (0x00000008)
#define TYPECRCINFO_SHA1                (0x00000010)
#define TYPECRCINFO_SHA1EMPTY           (0x00000020)
#define TYPECRCINFO_64BITSPOS           (0x00000080)
#define TYPECRCINFO_SHA256              (0x00000040)
#define TYPECRCINFO_SHA256EMPTY         (0x00000100)
//#define TYPECRCINFO_TYPECHECKSUM_MASK   (0x000001ff)



typedef struct
{
  dfuLong64 dfFilePos;
  dfuLong64 dfFileEncodedSizeWithoutPreAndPostHeader;
  dfuLong64 dfFileEncodedSizeWithPreAndPostHeader;
  dfuLong64 dfFileSizeContentUncompressed;
  dfuLong32 dfContentStoreMethod;
} DFSFILEPOSPROPERTIESINFO;

typedef struct
{
  dfuLong64 total_size_insert;
  dfuLong64 total_size_depl_in_place;
  dfuLong64 total_size_depl_out_place;
} DFSPATCHANALYSEINFO_MEMORY;

#define DFS_STREAM              (0x00000001)
#define DFS_READABLE            (0x00000002)
#define DFS_WRITABLE            (0x00000004)
#define DFS_NEWFILE             (0x00000008)

#define DFS_ERROR_BAD_PARAMETER  (0xffffffff)
#define DFS_ERROR_ERRORIO        (0xfffffffe)
#define DFS_ERROR_MEMORY_ERROR   (0xfffffffd)
#define DFS_ERROR_BAD_CHECKSUM   (0xfffffffc)
#define DFS_ERROR_FILE_NOT_FOUND (0xfffffffb)

#define DFS_SUCCESS              (0x00000000)
#define DFS_STOP_REQUESTED       (0xfffffff0)

#define DFS_ERRORTAG_FILENAME    (0x00000001)
#define DFS_ERRORTAG_ERRORMSG    (0x00000002)
#define DFS_ERRORTAG_WIN32NUMBER (0x00000003)
#define DFS_ERRORTAG_ERRNONUMBER (0x00000004)

#endif

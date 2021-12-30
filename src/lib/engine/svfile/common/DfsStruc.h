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

/* DfsStruc.h */
/* Structure of meta header on SVF file */

#ifndef DFSSTRUCT_H_INCLUDED
#define DFSSTRUCT_H_INCLUDED


#ifdef __cplusplus
extern "C" {
#endif

/* the header just before a file */
typedef struct
{
  dfuLong32Intel dfBlocType;
  dfuLong32Intel dfsFileHeaderSize;

  dfuLong32Intel dfContentStoreMethod;
  dfuLong32Intel dfContentEncodedSize;

  dfuLong32Intel dfContentUncompressedSize;
  dfuLong32Intel dfTagListSize;
} DFSFILEHEADER32;

typedef struct
{
  dfuLong32Intel dfBlocType;
  dfuLong32Intel dfsFileHeaderSize;

  dfuLong32Intel dfContentStoreMethod;
  dfuLong32Intel dfContentEncodedSizeLow;

  dfuLong32Intel dfContentUncompressedSizeLow;
  dfuLong32Intel dfTagListSize;

  dfuLong32Intel dfContentEncodedSizeHigh;
  dfuLong32Intel dfContentUncompressedSizeHigh;
} DFSFILEHEADER64;

/* the just after a file */
typedef struct
{
  dfuLong32Intel dfFileContentUncompressedSizeLow;
  dfuLong32Intel dfFileContentCompressedCrc32;
  dfuLong32Intel dfNumberCrc32Info;
  dfuLong32Intel dfFileContentUncompressedSizeHigh;
} DFSPOSTFILEINFO;

/* and just after */
/* after the file content, and in tag DFSTAG_CRCINFO in dir */


/* in tag DFSTAG_FILEPOSPROPERTIES , in dir */
typedef struct
{
  dfuLong32Intel dfFilePosLow;
  dfuLong32Intel dfFileEncodedSizeWithoutPreAndPostHeaderLow;
  dfuLong32Intel dfFileEncodedSizeWithPreAndPostHeaderLow;
  dfuLong32Intel dfFileSizeContentUncompressedLow;
  dfuLong32Intel dfContentStoreMethod;
  dfuLong32Intel dfFilePosHigh;
  dfuLong32Intel dfFileEncodedSizeWithoutPreAndPostHeaderHigh;
  dfuLong32Intel dfFileEncodedSizeWithPreAndPostHeaderHigh;
  dfuLong32Intel dfFileSizeContentUncompressedHigh;
} DFSFILEPOSPROPERTIES6464;

typedef struct
{
  dfuLong32Intel dfFilePosLow;
  dfuLong32Intel dfFileEncodedSizeWithoutPreAndPostHeader;
  dfuLong32Intel dfFileEncodedSizeWithPreAndPostHeader;
  dfuLong32Intel dfFileSizeContentUncompressed;
  dfuLong32Intel dfContentStoreMethod;
  dfuLong32Intel dfFilePosHigh;
} DFSFILEPOSPROPERTIES3264;

typedef struct
{
  dfuLong32Intel dfFilePos;
  dfuLong32Intel dfFileEncodedSizeWithoutPreAndPostHeader;
  dfuLong32Intel dfFileEncodedSizeWithPreAndPostHeader;
  dfuLong32Intel dfFileSizeContentUncompressed;
  dfuLong32Intel dfContentStoreMethod;
} DFSFILEPOSPROPERTIES32;

typedef struct
{
  dfuLong32Intel dfPreviousVersionFileNumber; /* actually 1 or 0 */
  dfuLong32Intel dfPreviousVersionFilePosition;
} DFSPREVIOUSVERSIONINFO;

/* the header of a dir */
typedef struct
{
  dfuLong32Intel dfBlocType;
  dfuLong32Intel dfTypeDir;

  dfuLong32Intel dfNumberOfnDir;
  dfuLong32Intel dfTagSize;
}
DFSDIRINTROHEADER;

typedef struct
{
  dfuLong32Intel dfBlocType;
  dfuLong32Intel dfTypeDir;
  dfuLong32Intel dfStoreMethod;

  dfuLong32Intel dfSizeDirTag;
  dfuLong32Intel dfSizeFilesTag;

  dfuLong32Intel dfSizeCompressed;
  dfuLong32Intel dfCrc32Uncompressed;

  dfuLong32Intel dfNumberFilesInDir;
  dfuLong32Intel dfNumberOfnDir;
} DFSDIRHEADER;

typedef struct
{
  dfuLong32Intel dfTypeDir;
  dfuLong32Intel dfOffsetDirIntro;
  dfuLong32Intel dfOffsetDir;
  dfuLong32Intel dfBlockDirSize;
}
DFSDIRINFOOLD;

/* dfOffsetPostFileTag can be 0 (for old files) */

typedef struct
{
  dfuLong32Intel dfBlocType;
  dfuLong32Intel dfNbDir;
  dfuLong32Intel dfTypeDfsFile;
  dfuLong32Intel dfOffsetPostFileTag;
} DFSLISTDIRHEADHEADEROLD;

typedef struct
{
  DFSLISTDIRHEADHEADEROLD DfsListDirHeadHeader;
  DFSDIRINFOOLD dfsDirInfo[1];
}
DFSLISTDIRHEADOLD;


typedef struct
{
  dfuLong32Intel dfBlocType;
  dfuLong32Intel dfNbDir;
  dfuLong32Intel dfTypeDfsFile;
  dfuLong32Intel dfOffsetPostFileTagHigh;
  dfuLong32Intel dfOffsetPostFileTagLow;
} DFSLISTDIRHEADHEADER64;

typedef struct
{
  dfuLong32Intel dfTypeDir;
  dfuLong32Intel dfOffsetDirIntroLow;
  dfuLong32Intel dfOffsetDirIntroHigh;
  dfuLong32Intel dfOffsetDirLow;
  dfuLong32Intel dfOffsetDirHigh;
  dfuLong32Intel dfBlockDirSize;
} DFSDIRINFO64;

typedef struct
{
  DFSLISTDIRHEADHEADER64 DfsListDirHeadHeader;
  DFSDIRINFO64 dfsDirInfo[1];
}
DFSLISTDIRHEAD64;

typedef struct
{
  dfuLong32Intel dfBlockCrc32;
  dfuLong32Intel dfBlockSize;
}
DFSLISTDIREND;

typedef struct
{
  dfuLong64Intel total_size_insert;
  dfuLong64Intel total_size_depl_in_place;
  dfuLong64Intel total_size_depl_out_place;
} DFSPATCHANALYSEINFO_INTEL;

/**************************************/
/* if dfOffsetPostFileTag != 0 in DFSLISTDIRHEADHEADER,
   go to a DFSENDFILETAGHEADER structure */

typedef struct
{
  dfuLong32Intel dfBlocType;
  dfuLong32Intel dfTotalSizeEndFileInfo;
  dfuLong32Intel dfTotalSizeCompressedFloatTag;
  dfuLong32Intel dfReserved;
}
DFSENDFILETAGHEADER;

/*************************************************************************/

typedef struct
{
  dfuLong32Intel dfDirectoryNumber;
  dfuLong32Intel dfTypeDir;
  dfuLong32Intel dfDirPos;
  dfuLong32Intel dfReserved;
}
DFSDIRTABLEITEM;

/**************************************************************************/
#define BLOCKTYPE_FILE      (1)
#define BLOCKTYPE_DIRINTRO  (2)
#define BLOCKTYPE_DIR       (3)
#define BLOCKTYPE_LISTDIR   (4)
#define BLOCKTYPE_LISTDIR64 (5)
#define BLOCKTYPE_ENDFILETAGANDINFO  (8)

#define DFSMETHOD_STORE   (0)
#define DFSMETHOD_DEFLATE (2)
#define DFSMETHOD_PATCH   (0x010)

#define VALUE_UNKNOWN     ((dfuLong32)(0xffffffff))

#ifdef __cplusplus
}
#endif

#endif

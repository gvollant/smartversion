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

/* DfsIntlInternal.h */


#ifndef DEF_MEM_LEVEL
#define DEF_MEM_LEVEL (8)
#endif


//#define crc32 adler32
// #define COMPRESS_RATIO (2)

//    DFSFILEWRAP DfsFileWrapf;





/* internal structure */
/*******************************************/

typedef struct
{
  dfuLong32 dfTypeDir;
  dfuLong32 dfOffsetDir;
  dfuLong32 dfBlockDirSize;
  dfuLong32 dfDirNumber;
}
DFSDIRINFOINTERNAL;

/*******************************************/


typedef struct
{
  DFSFILEWRAP DfsFileWrap;

  /* to manage the list of directories in the DFS */
  //dfuLong32 dfNbDir_unused;
  dfuLong32 dfNbDirAllocated;
  DFSDIRINFOINTERNAL *dfDirInfoInternal;
  dfuLong32 dfDirStepAlloc;


  /* Information about current directory, Initialized on DfsWCreateNewDir,
     released after DfsWCloseFlushCurrentDir */
  dfuLong32 dfNumberOfDir;
  dfuLong32 dfNumberFilesInDir;
  dfuLong32 dfTypeCurDir;
  dfuLong32 dfDirPosLow;
  dfuLong32 dfDirPosHigh;
  dfuLong32 dfDirIntroPosLow;
  dfuLong32 dfDirIntroPosHigh;
  dfuLong32 dfFilePosLow;
  dfuLong32 dfFilePosHigh;

  DFTAGBLOCKFLOAT dfTagBlockFloat;

  // Tag of the Dir (general property for all the directory)
  DFTAGLIST dfTagListCurDirInfo;
  DFTAGLIST dfTagListCurDirInfoIntro;

  /* The tags about each file of current dir, one after one.
     Prepared by DfsWCreateNewFileInDir and DfsWAddTagInNewFileInDir,
     modified by DfsWCloseFileInDir */

  void *CurDirFileTagInBuildingPtr;
  dfuLong32 CurDirFileTagInBuildingSize;
  dfuLong32 CurDirFileTagInBuildingAllocated;
  dfuLong32 CurDirStepAlloc;


  /* information about file currently written */
  DFSFILEHEADER64 dfsFileHeader;
  dfuLong32 DfsFileHeaderPosLow;
  dfuLong32 DfsFileHeaderPosHigh;

  DFSFILEPOSPROPERTIESINFO dfsFilePosProperties;

  dfuLong64 dfFileEncodedSizeWithPreAndPostHeader;
  dfuLong64 dfFileEncodedSizeWithoutPreAndPostHeader;
  /* the tag written about the file in directory */
  DFTAGLIST dfTagListCurFileInFinalDir;
  /* the tag written about file just before file content */
  DFTAGLIST dfTagListCurFileInPreFileInfoDir;

  dfuLong64 dfContentUncompressedSizeDeclared;
  dfuLong64 dfEncodedSizeExecuted;
  dfuLong32 dfEncodedCrc32;

    /***********************/


  /* general information */

  dfuLong32 dfCurState;
  dfuLong32 dfBlockTypeJustRead;


  dfuLong32 dfStatus;
  dfuLong32 dfPosBlockDataEndLow;
  dfuLong32 dfPosBlockDataEndHigh;

  dfuLong32 dfPosOfLastUnfloatDataWhenAtEndOfDataLow;
  dfuLong32 dfPosOfLastUnfloatDataWhenAtEndOfDataHigh;

    /******************************/


    /******************/
  DFSLISTDIRHEAD64 *DfsDirListHead;
  dfuLong32 CurDfsDirListNbDir;
  dfuLong32 CurDfsDirListHeadSize;
  dfuLong32 CurDfsDirListHeadAllocated;
  dfuLong32 CurDfsDirListHeadStepAlloc;

  //BOOL fStripIdenticalBody;
  DFSFEATUREPARAM DfsFeatureParam;

  BOOL fModifyingDone;
  BOOL fModifyingDoneNoFlushed;
}
DFSFILEINTERNAL;

/* No dir opened, no file opened */
#define CURSTATE_NODIROPENED_ATENDOFDATA                (0x00000001)

/* a dir opened, no file opened */
#define CURSTATE_WR_AFTERCREATEDIR_BEFOREWRITEINTRO     (0x00000012)
#define CURSTATE_WR_AFTERCREATEDIR_AFTERWRITEINTRO      (0x00000013)

/* a dir opened, a file opened */
#define CURSTATE_WR_AFTERCREATEFILE                     (0x00000014)

/* a dir opened, a file opened, but tag can be added only in final */
#define CURSTATE_WRITINGFILECONTENT                     (0x00000015)

/********************************************************************/


#define CURSTATE_RD_BEFOREREAD_BLOCKTYPE                (0x00000020)
#define CURSTATE_RD_BEFOREREAD_BLOCK_FILE               (0x00000021)
#define CURSTATE_RD_BEFOREREAD_BLOCK_POSTFILE           (0x00000022)
#define CURSTATE_RD_BEFOREREAD_BLOCK_DIRINTRO           (0x00000023)
#define CURSTATE_RD_BEFOREREAD_BLOCK_DIR                (0x00000024)
#define CURSTATE_RD_BEFOREREAD_BLOCK_LISTDIR            (0x00000025)

#define CURSTATE_RD_READINGFILECONTENT                  (0x0000002f)

#define INVALID_POSITION                                (0xffffffff)

/********************************************************************/


void ClearDirInWorkMember(DFSFILEINTERNAL * DfsFileInternal,
                          BOOL fClearMemBlock);


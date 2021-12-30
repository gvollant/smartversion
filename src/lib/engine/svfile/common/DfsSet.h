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

/* dfsSet.h */
/* define callback used during work on SVF file */

#ifndef DFS_SET_H_INCLUDED
#define DFS_SET_H_INCLUDED

#define TYPEDIR_FILECRCONLY             (1)
#define TYPEDIR_FILEINSERTING_STORE     (2)
#define TYPEDIR_FILEINSERTING_DEFLATE   (3)
#define TYPEDIR_PATCHFROMPREVIOUS       (4)


typedef struct
{
  dfuLong32 dfSizeStruct;
  DFSFILE DfsFile;

  dfuLong32 dfEvent;
  dfvoidp dfDataEventPtr;
  dfvoidp dfUserPtr;
  dfvoidp dfReserved;

  dfuLong32 dfDirType;
  BOOL fWillIgnoreFile;         // for DFCBM_BEFOREOPENPREVIOUSFILE
  dfwcharpc filename_onwork;
  dfwcharpc filename_previousversion;

  dfwcharpc filename_stored;
  BOOL fTemporaryFilename ;

  dfuLong64 dfDirOrigDone;
  dfuLong64 dfDirEncodedDone;
  dfuLong64 dfDirOrigSize;    // not alway filles
  dfuLong64 dfDirEncodedSize; // not alway filles
  dfuLong32 dfFileNumber;
  dfuLong64 dfFileOrigDone;
  dfuLong64 dfFileEncodedDone;
  dfuLong64 dfFileOrigSize;
  dfuLong64 dfFileEncodedSize;
}
PROGRESSCALLBACKINFO;

typedef BOOL(DFSCALLBACK * tProgressCallBack) (PROGRESSCALLBACKINFO *
                                               pProgressCallBackInfo);




#define DFCBM_BEFOREOPENDIRECTORY           (0x00000001)
#define DFCBM_AFTEROPENDIRECTORY            (0x00000011)
#define DFCBM_BEFOREOPENPREVIOUSFILE        (0x00000002)
#define DFCBM_BEFOREOPENWORKINGFILE         (0x00000003)
#define DFCBM_PROGRESSWORKINGFILE           (0x00000004)
#define DFCBM_BEFOREOPENWORKINGFILEPREPARSE (0x00000005)
#define DFCBM_AFTERCLOSINGWORKINGFILE       (0x00000015)
#define DFCBM_AFTERCLOSINGPREVIOUSFILE      (0x00000016)
#define DFCBM_AFTERCLOSINGDIRECTORY         (0x00000017)

#endif

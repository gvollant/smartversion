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

/* DfsTagDf.h */
/* define constant used to store tag */


#ifndef DFS_TAG_DF_H_INCLUDED
#define DFS_TAG_DF_H_INCLUDED


#ifdef __cplusplus
extern "C" {
#endif

DECLARE_DFHANDLE(DFTAGLIST);


#define DFSTAG_FILENAME             (0x00000002)
#define DFSTAG_DATE                 (0x00000003)
#define DFSTAG_BASEPATCH            (0x00000004)

#define DFSTAG_FILEPOSPROPERTIES    (0x00000005)
#define DFSTAG_CRCINFO              (0x00000006)
#define DFSTAG_PREVIOUSVERSIONINFO  (0x00000007)
#define DFSTAG_STORAGESTATUS        (0x00000008)

#define DFSTAG_STORAGEPATCHINFO     (0x00000009)

#define DFSTAG_DIR_NAME             (0x00000801)
#define DFSTAG_DIR_NAME_FLOAT       (0x00000802)
#define DFSTAG_DIR_COMMENT_FLOAT    (0x00000803)


#define DFSTAG_SUGGEST_METHOD_FLOAT (0x00001001)

#define DFSTAG_FIRST_FLOAT_DIR_RECOPY   (0x00000802)
#define DFSTAG_LAST_FLOAT_DIR_RECOPY    (0x000009ff)

#define DFSTAG_NUMBEROFFILEINDIR    (0x10000001)

#define DFSTAG_NOFLOATFILLED        (0x80000001)


#ifdef __cplusplus
}
#endif

#endif

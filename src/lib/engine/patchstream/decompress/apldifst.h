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

/* apldifst.h */
/* function and structure to execute a stream patch from two file */

#ifndef APL_DIFST_H_INCLUDED
#define APL_DIFST_H_INCLUDED

DECLARE_DFHANDLE(HAPPLYDIF_INTERNALSTATE);

typedef struct
{
  dfuLong32 nbOrig;               /* must be initialised before call init */
  ORIGDATAPTR OrigDataPtr;      /* must be initialised before call init */
  dfuLong32 uDoChecksum;          /* 1 if we compute checksum in OutData */


  IN_DATA_STREAM in_data_stream;
  OUT_DATA_STREAM out_data_stream;

  HAPPLYDIF_INTERNALSTATE state;        /* not visible by applications */
}
APPLYDIF_STREAM;

int SVFAPI ApplyDifInit OF((APPLYDIF_STREAM * read_stream));
int SVFAPI ApplyDifInitEx OF((APPLYDIF_STREAM * read_stream, BOOL fComputeChecksum));
int SVFAPI DoApplyDifWork OF((APPLYDIF_STREAM * read_stream));
int SVFAPI CloseApplyDif OF((APPLYDIF_STREAM * read_stream, dfuLong32 * uChecksum));

/****************************************************************************/
#endif

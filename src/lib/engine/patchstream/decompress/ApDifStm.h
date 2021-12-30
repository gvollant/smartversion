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

#include "../common/abstractDecompress.h"
#include "../common/difstrmi.h"

typedef struct
{
  abstract_decompress_stream zstream;
  dfbytep streamBuf;
  dfuLong32 posWriteBuf;
  dfuLong32 posReadBuf;
  dfuLong32 sizeBuf;
  int end_reached;
  BOOL inflateInitDone;
  BOOL inflateInitNegMaxWBits;
  BOOL abstractDecompressor;
}
RD_UNIT_STREAM;

typedef struct
{
    RD_UNIT_STREAM ReadStream[NUMBER_STREAM];
    dfuLong32 inBufferSize;
    dfuLong32 inBufferPos;
    dfbytep inBuffer;             /* data in reading buffering */

    dfuLong32 bufferSizeUnit;
    dfuLong32 nullDummy;

    dfuLong32 sizeBufStream;
    dfuLong32 nextStreamToFlush;

    //int nbStreamInReading;
    //dfuLong32 CurrentStreamBytesToRead;
} READ_MULTI_BUFFER;

#define DSERR_NEED_READMORE (2)

int InitReadMultiBuffer(READ_MULTI_BUFFER* pReadMultiBuffer);

int
GetDataFromStream(READ_MULTI_BUFFER* pReadMultiBuffer, IN_DATA_STREAM * pIn_data_stream,
                  int nbStream, dfvoidp buf,
                  dfuLong32 size, dfuLong32 * written_size);

int
ReadUntilDataFilledFromStream(READ_MULTI_BUFFER* pReadMultiBuffer, IN_DATA_STREAM * pIn_data_stream,
                              int nbStream,dfuLong32 size);

int VerifyEachStreamEnded(READ_MULTI_BUFFER* pReadMultiBuffer);

void CloseReadMultiBuffer(READ_MULTI_BUFFER* pReadMultiBuffer);


/**************************************************************************/


dfuLong32 DecodeSeqInfo(dfuLong64* pCurPos_depl, dfbytep ptr,
                        dfuLong32 size, dfuLong32 * is_insert_bytes,
                        dfuLong64 * size_block, dfuLong64 * pos_seq);

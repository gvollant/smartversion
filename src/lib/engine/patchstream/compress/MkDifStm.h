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

typedef struct
{
  abstract_compress_stream zstream;
  dfbytep streamBuf;
  dfuLong32 posBufStream;
  dfuLong64 totalDataWritten;
  dfuLong64 total_compressed;
  dfuLong64 total_uncompressed;
  dfuLong64 uncompressed_all_stream_pos_last_flush;
}
WR_UNIT_STREAM;

typedef struct
{
  dfuLong64 total_out_internal;
  dfuLong64 uncompressed_stream_pos;
  dfuLong32 outBufferSize;
  dfuLong32 outBufferPos;
  dfbytep outBuffer;

  WR_UNIT_STREAM WriteStream[NUMBER_STREAM];
  dfuLong32 max_uncompressed_all_stream_to_be_buffered;
  dfuLong32 sizeBufStream;
  dfuLong32 nextStreamToFlush;
  dfuLong32 in_flush;             /* say if we will have no more in data */
  BOOL      fLongHeaderStreamFormat;
  BOOL      fCompressBzip2;
  BOOL      fCompressXZUtils;
  BOOL      fCompressPrefix;

  dfuLong32 latest_written_seq_is_ins;
  dfuLong32 latest_size_of_written_seq;
  dfuLong64 latest_written_seq_size;
  dfuLong64 latest_written_seq_pos;
  dfuLong64 latest_written_seq_posCurOrg;


  dfuLong64 latest_write_out_pos;
  dfuLong64 write_out_insert_size;
  dfuLong64 write_out_size_depl_in_place;
  dfuLong64 write_out_size_depl_out_place;

} OUT_MULTI_BUFFER ;

int InitOutPatchStream(OUT_MULTI_BUFFER *pOutPatchStream,dfuLong32 CompressRatio, dfuLong32 dfDefaultSizeBufStreamKB);
void ReleaseOutPatchStream(OUT_MULTI_BUFFER *pOutPatchStream);

void FlushOutInStream(OUT_MULTI_BUFFER *pOutPatchStream,OUT_DATA_STREAM* pOutDataStream);
int NeedMoreOutSpaceForFlushing(OUT_MULTI_BUFFER *pOutPatchStream,OUT_DATA_STREAM* pOutDataStream);

int WriteOutBuffer(OUT_MULTI_BUFFER *pOutPatchStream, dfvoidp data, dfuLong32 size);

/* FlushWriteStreamData : flush a stream
*/

int WriteStreamData(OUT_MULTI_BUFFER *pOutPatchStream, OUT_DATA_STREAM* pOutDataStream, int nbStream,
                dfvoidp data, dfuLong64 size);


void SetInFinalFlush(OUT_MULTI_BUFFER *pOutPatchStream);
dfuLong32 IsFinalFlush(OUT_MULTI_BUFFER *pOutPatchStream);

int FinalFlushOutData(OUT_MULTI_BUFFER *pOutPatchStream, OUT_DATA_STREAM* pOutDataStream);

int GetSteamAnalysis(const OUT_MULTI_BUFFER *pOutPatchStream, DFSPATCHANALYSEINFO_MEMORY* pDfsPatchAnalyseInfo);

int WriteInsertBytesInStream(OUT_MULTI_BUFFER *pOutPatchStream, OUT_DATA_STREAM* pOutDataStream, dfvoidp data, dfuLong64 size);
int WriteInsertDeplInStream(OUT_MULTI_BUFFER *pOutPatchStream, OUT_DATA_STREAM* pOutDataStream,
                            dfuLong64 posCurOrg,
                            dfuLong64 posIntPtr, dfuLong64 size);

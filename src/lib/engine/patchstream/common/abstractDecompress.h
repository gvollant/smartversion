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

#ifndef ABSTRACT_DECOMPRESS_H
#define ABSTRACT_DECOMPRESS_H 1

#include "zlib.h"
#include "difbasic.h"


#define INTERNALDFSCALLBACK

#ifdef __cplusplus
extern "C" {
#endif

struct abs_stream_s;

typedef int(INTERNALDFSCALLBACK * t_abstract_decompress) (struct abs_stream_s* strm,int flush);
typedef int(INTERNALDFSCALLBACK * t_abstract_decompress_end) (struct abs_stream_s* strm);


typedef struct abs_stream_s {
    Bytef    *next_in;  /* next input byte */
    uInt     avail_in;  /* number of bytes available at next_in */
    dfuLong64 total_in;  /* total nb of input bytes read so far */

    Bytef    *next_out; /* next output byte should be put there */
    uInt     avail_out; /* remaining free space at next_out */
    dfuLong64 total_out; /* total nb of bytes output so far */

	t_abstract_decompress fnc_decompress;
	t_abstract_decompress_end fnc_decompress_end;

	void* p_native_stream;

	int contain_prefix;
	int is_prefix_read;
	int end_reached;
	uInt size_prefix;

} abstract_decompress_stream;



#define ABSTR_DECOMPRESS_Z_OK            0
#define ABSTR_DECOMPRESS_Z_STREAM_END    1
#define ABSTR_DECOMPRESS_Z_ERRNO        (-1)
#define ABSTR_DECOMPRESS_Z_MEM_ERROR    (-4)


#define ABSTR_DECOMPRESS_Z_NO_FLUSH      0
#define ABSTR_DECOMPRESS_Z_SYNC_FLUSH    2
#define ABSTR_DECOMPRESS_Z_FINISH        4

void clear_abstract_decompress_stream(
     abstract_decompress_stream* strm);

int abstract_decompress_end (abstract_decompress_stream* strm);
int abstract_decompress (abstract_decompress_stream* strm, int flush);


int abstract_init_prefix(abstract_decompress_stream* strm);
// inflateInit2(&pReadMultiBuffer->ReadStream[stream_number].zstream, -MAX_WBITS);
int abstract_init_inflate_withNegMaxWBits(abstract_decompress_stream* strm);

// inflateInit(&pReadMultiBuffer->ReadStream[stream_number].zstream);
int abstract_init_inflate_withoutNegMaxWBits(abstract_decompress_stream* strm);
int abstract_init_store(abstract_decompress_stream* strm);
int XflateStore_decompress(abstract_decompress_stream* strm, int flush);

#ifdef __cplusplus
}
#endif

#endif

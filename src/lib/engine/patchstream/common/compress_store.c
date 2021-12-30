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

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "zlib.h"
#include "difbasic.h"
#include "DfsTlTyp.h"

#ifndef NO_SMARTVERSION_ENGINE
#include "difstool.h"
#else
#define DfsMemcpy(a,b,c) (memcpy((a),(b),(c)))
#endif
#include "compress_store.h"


int XflateStore(z_streamp strm, int flush)
{
      int retErr = Z_OK;
      uLong dwDoThis;
      if ((strm->avail_in) < (strm->avail_out))
          dwDoThis = strm->avail_in;
      else
          dwDoThis = strm->avail_out;

      //memcpy(strm->next_out,strm->next_in,dwDoThis);
      {
          const unsigned char* src;
          unsigned char* dst;
          uLong dwToCopy = dwDoThis;
          dst = strm->next_out;
          src = strm->next_in;
          /*
          while (dwToCopy>0)
          {
              *dst = *src;
              dst++;
              src++;
              dwToCopy--;
          }*/
          DfsMemcpy(dst, src, dwToCopy);
      }
      strm->avail_in  -= dwDoThis;
      strm->avail_out -= dwDoThis;
      strm->total_in  += dwDoThis;
      strm->total_out += dwDoThis;

      strm->next_out += dwDoThis;
      strm->next_in += dwDoThis;
      if ((flush==Z_FINISH) && (strm->avail_in==0))
          retErr = Z_STREAM_END;
      return retErr;
}

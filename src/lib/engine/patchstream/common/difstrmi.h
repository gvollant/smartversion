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

#define ENDOFSTREAM_MASK    (0x89A00000)
#define NUMBERSTREAM_MASK   (0x000FFFFF)

#define SMALLHEADER_SIGN_VALUE   (0x80)
#define SMALLHEADER_SIGN_MASK    (0xc0) /* 0x80 | 0x40 */

#define SMALLHEADER_SIZE_MASK    (0x30) /* 0x20 | 0x10 */
#define SMALLHEADER_SIZE_VALUE4  (0x30)
#define SMALLHEADER_SIZE_VALUE2  (0x20)
#define SMALLHEADER_SIZE_VALUE1  (0x10)
#define SMALLHEADER_SIZE_VALUE0  (0x00)

#define SMALLHEADER_ENDSTREAM_MASK (0x08)
#define SMALLHEADER_NUMBERSTREAM_MASK (0x03)
#define SMALLHEADER_ASTRACT_DECOMPRESSOR (0x04)
#define SMALLHEADER_ASTRACT_DECOMPRESSOR_MASK (0x04)

#if (defined (DEBUG) || defined (_DEBUG)) && defined (STDC)
#define DFASSERT(cod) \
   { if (!(cod)) printf("\n\n**:error with condition (" #cod ") at line %d, file (%s)\n",__LINE__, __FILE__ ); }
#else
#define DFASSERT(cod)
#endif

#define STREAM_INS (0)
#define STREAM_SEQ (1)

#define FIRST_STREAM (0)
#define LAST_STREAM  (1)
#define NUMBER_STREAM (LAST_STREAM + 1)

#define SEQINFO_STREAM (0)
#define INSBYTE_STREAM (1)

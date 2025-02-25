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

// xz_embedded_crc32_and_crc64_init.cpp
// written by Gilles Vollant
// this file build the CRC table at init, before starting app

#if (!defined(PREVENT_COMPILE_XZEMBEDDED)) && (!defined(HAVE_XZEMBEDDED)) && (!defined(UNCOMPRESS_LZMASDK))
#define HAVE_XZEMBEDDED 1
#endif

#if defined(HAVE_XZEMBEDDED)
#include "xz.h"

static int calc_xzcrc32_init()
{
    xz_crc32_init();
    return 0;
}
int res_calc_xz_embedded_crc32_init = calc_xzcrc32_init();

#if defined(XZ_USE_CRC64)
static int calc_xzcrc64_init()
{
    xz_crc64_init();
    return 0;
}
int res_calc_xz_embedded_crc64_init = calc_xzcrc64_init();
#endif

#endif

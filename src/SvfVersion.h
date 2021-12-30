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


#define SVF_VERSION_MAJOR 3
#define SVF_MINOR_DIGIT_1 8
#define SVF_MINOR_DIGIT_2 4

#define SVF_VERSION_YEAR 2022

// for windows resource
#define FULL_STRING_SMARTVERSION_VERSION "SmartVersion 3.84"
#define FULL_STRING_SMARTVERSION_COPYRIGHT "Copyright © 2002-2022 Gilles Vollant."
#define FULL_STRING_SMARTVERSION_COPYRIGHT_2 "Portion Copyright Gilles Vollant (collaborate with Amabis and Hubbi), Francois Liger (of Lemma Factory), Facebook and Yann Colet (Zstd, xxhash and lz4), OpenSSL (hash), Rich Geldreich (lzham)"
#define FULL_STRING_SMARTVERSION_COPYRIGHT_3 "Portion Copyright Igor Pavlov (Lzma sdk), Lasse Collin (xz utils), Xavier Roche (fastlzlib), Ariya Hidayat (FastLZ), Julian Seward (bzip), Apple (lzfse), Mark Adler and Jean-Loup Gailly (zlib)"




////////////////////

#define SVF_STRINGIFY(s) #s
#define XSVF_STRINGIFY(s) SVF_STRINGIFY(s)


#if SVF_MINOR_DIGIT_2 == 0
#define SVF_VERSION_STRING  XSVF_STRINGIFY(SVF_VERSION_MAJOR) "."  XSVF_STRINGIFY(SVF_MINOR_DIGIT_1)
#else
#define SVF_VERSION_STRING  XSVF_STRINGIFY(SVF_VERSION_MAJOR) "."  XSVF_STRINGIFY(SVF_MINOR_DIGIT_1) XSVF_STRINGIFY(SVF_MINOR_DIGIT_2)
#endif

#define SVF_VERSION_YEAR_STRING XSVF_STRINGIFY(SVF_VERSION_YEAR)


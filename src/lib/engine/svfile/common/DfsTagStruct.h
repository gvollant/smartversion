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


/* external structure */
typedef struct
{
  dfuLong32Intel dfStoreMethod;
  dfuLong32Intel dfSizeCompressed;
  dfuLong32Intel dfSizeUncompressed;
  dfuLong32Intel dfCrc32Tags;
}
DFSTAGLISTHEADER;

#define TAGSTOREMETHOD_NONE     (0)
#define TAGSTOREMETHOD_DEFLATE  (2)

typedef struct
{
  dfuLong32Intel dfTagNumber;
  dfuLong32Intel dfTagSize;
}
DFSTAGHEADER;

typedef struct
{
  dfuLong32Intel TagNumber;
  dfuLong32Intel TagSize;
}
DFSTAGDATAHEADER;

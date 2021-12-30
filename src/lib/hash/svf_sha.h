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


#ifndef svf_sha_INCLUDED
#  define svf_sha_INCLUDED

#ifdef __cplusplus
extern "C"
{
#endif


/*
under MS-Windows, see https://nagareshwar.securityxploded.com/2010/10/22/cryptocode-generate-sha1sha256-hash-using-windows-cryptography-library/
but slower than openssl + assembly code
*/

#if (defined(COCOA_COMMON_CRYPTO_HASH))
#include <CommonCrypto/CommonDigest.h>


    typedef struct
    {
        CC_SHA1_CTX ctx;
    } SVF_SHA1_INTERNAL;

    static void svf_sha1_init(SVF_SHA1_INTERNAL* pgv_mds)
    { CC_SHA1_Init(&pgv_mds->ctx) ; }

    static void svf_sha1_append(SVF_SHA1_INTERNAL*pgv_mds, const unsigned char* buf, int nbytes)
    { CC_SHA1_Update(&pgv_mds->ctx,(const void *)buf,(CC_LONG)nbytes); }

    static void svf_sha1_finish(SVF_SHA1_INTERNAL*pgv_mds, unsigned char digest[20])
    { CC_SHA1_CTX ctx_prov = pgv_mds->ctx;
        CC_SHA1_Final((unsigned char*)digest,&ctx_prov); }

#else

#include "sha.h"
typedef struct
{
    SHA_CTX ctx;
} SVF_SHA1_INTERNAL;

static void svf_sha1_init(SVF_SHA1_INTERNAL* pgv_mds)
 { SHA1_Init(&pgv_mds->ctx) ; }

static void svf_sha1_append(SVF_SHA1_INTERNAL*pgv_mds, const unsigned char* buf, int nbytes)
 { SHA1_Update(&pgv_mds->ctx,(const void *)buf,(unsigned long)nbytes); }

static void svf_sha1_finish(SVF_SHA1_INTERNAL*pgv_mds, unsigned char digest[20])
 { SHA_CTX ctx_prov = pgv_mds->ctx;
   SHA1_Final((unsigned char*)digest,&ctx_prov); }

#endif

#ifdef __cplusplus
}  /* end extern "C" */
#endif

#endif
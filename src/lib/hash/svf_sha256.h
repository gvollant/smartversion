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


#ifndef svf_sha256_INCLUDED
#  define svf_sha256_INCLUDED

#ifdef __cplusplus
extern "C"
{
#endif


#if (defined(COCOA_COMMON_CRYPTO_HASH))
#include <CommonCrypto/CommonDigest.h>


    typedef struct
    {
        CC_SHA256_CTX ctx;
    } SVF_SHA256_INTERNAL;

    static void svf_sha256_init(SVF_SHA256_INTERNAL* pgv_mds)
    { CC_SHA256_Init(&pgv_mds->ctx) ; }

    static void svf_sha256_append(SVF_SHA256_INTERNAL*pgv_mds, const unsigned char* buf, int nbytes)
    { CC_SHA256_Update(&pgv_mds->ctx,(const void *)buf,(CC_LONG)nbytes); }

    static void svf_sha256_finish(SVF_SHA256_INTERNAL*pgv_mds, unsigned char digest[32])
    { CC_SHA256_CTX ctx_prov = pgv_mds->ctx;
        CC_SHA256_Final((unsigned char*)digest,&ctx_prov); }

#else

#define SHA256FROMLZMASDK

#ifdef SHA256FROMLZMASDK
//#define OPENSSL_NO_SHA256
#include "lzma_sdk/C/Sha256.h"
typedef struct
{
    CSha256 ctx;
} SVF_SHA256_INTERNAL;

static void svf_sha256_init(SVF_SHA256_INTERNAL* pgv_mds)
 { Sha256_Init(&pgv_mds->ctx) ; }

static void svf_sha256_append(SVF_SHA256_INTERNAL*pgv_mds, const unsigned char* buf, int nbytes)
 { Sha256_Update(&pgv_mds->ctx,(const Byte *)buf,(unsigned long)nbytes); }

static void svf_sha256_finish(SVF_SHA256_INTERNAL*pgv_mds, unsigned char digest[32])
 { CSha256 ctx_prov = pgv_mds->ctx;
   Sha256_Final(&ctx_prov,(unsigned char*)digest); }
#else

typedef struct
{
	SHA256_CTX ctx;
} SVF_SHA256_INTERNAL;

static void svf_sha256_init(SVF_SHA256_INTERNAL* pgv_mds)
 { SHA256_Init(&pgv_mds->ctx) ; }

static void svf_sha256_append(SVF_SHA256_INTERNAL*pgv_mds, const unsigned char* buf, int nbytes)
 { SHA256_Update(&pgv_mds->ctx,(const void *)buf,(unsigned long)nbytes); }

static void svf_sha256_finish(SVF_SHA256_INTERNAL*pgv_mds, unsigned char digest[32])
 { SHA256_CTX ctx_prov = pgv_mds->ctx;
   SHA256_Final((unsigned char*)digest,&ctx_prov); }
#endif
#endif

#ifdef __cplusplus
}  /* end extern "C" */
#endif

#endif
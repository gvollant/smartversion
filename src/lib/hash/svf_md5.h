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


#ifndef svf_md5_INCLUDED
#  define svf_md5_INCLUDED

#ifdef __cplusplus
extern "C"
{
#endif

#if (defined(COCOA_COMMON_CRYPTO_HASH))

#include <CommonCrypto/CommonDigest.h>


    typedef struct
    {
        CC_MD5_CTX ctx;
    } SVF_MD5_INTERNAL;

    static void svf_md5_init(SVF_MD5_INTERNAL* pgv_mds)
    { CC_MD5_Init(&pgv_mds->ctx) ; }

    static void svf_md5_append(SVF_MD5_INTERNAL*pgv_mds, const unsigned char* buf, int nbytes)
    { CC_MD5_Update(&pgv_mds->ctx,(const void *)buf,(CC_LONG)nbytes); }

    /*
     static void svf_md5_finish(SVF_MD5_INTERNAL*pgv_mds, unsigned char digest[16])
     { MD5_Final((unsigned char*)digest,&pgv_mds->ctx); }
     */
    static void svf_md5_finish(SVF_MD5_INTERNAL*pgv_mds, unsigned char digest[16])
    { CC_MD5_CTX ctx_prov = pgv_mds->ctx;
        CC_MD5_Final((unsigned char*)digest,&ctx_prov); }



#else

#if (!defined(MD5_ONLY_OPENSSL)) && (!defined(NO_MD5_ONLY_OPENSSL))
#define MD5_ONLY_OPENSSL
#endif

#if ((!(defined(WIMA_SFX))) && (!(defined(md5_openssl))) && (defined(PFRFEATURE))) || defined(MD5_ONLY_OPENSSL)
#define md5_openssl
#endif


#ifdef md5_openssl
#include "md5ossl.h"
typedef struct
{
    MD5_CTX ctx;
} SVF_MD5_INTERNAL;

static void svf_md5_init(SVF_MD5_INTERNAL* pgv_mds)
 { MD5_Init(&pgv_mds->ctx) ; }

static void svf_md5_append(SVF_MD5_INTERNAL*pgv_mds, const unsigned char* buf, int nbytes)
 { MD5_Update(&pgv_mds->ctx,(const void *)buf,(unsigned long)nbytes); }

 /*
static void svf_md5_finish(SVF_MD5_INTERNAL*pgv_mds, unsigned char digest[16])
 { MD5_Final((unsigned char*)digest,&pgv_mds->ctx); }
 */
static void svf_md5_finish(SVF_MD5_INTERNAL*pgv_mds, unsigned char digest[16])
 { MD5_CTX ctx_prov = pgv_mds->ctx;
   MD5_Final((unsigned char*)digest,&ctx_prov); }

#else
#include "md5.h"
typedef struct
{
  md5_state_t mds;
} SVF_MD5_INTERNAL;

static void svf_md5_init(SVF_MD5_INTERNAL* pgv_mds)
  { md5_init(&pgv_mds->mds) ; }

static void svf_md5_append(SVF_MD5_INTERNAL*pgv_mds, const unsigned char* buf, int nbytes)
  { md5_append(&pgv_mds->mds,buf,nbytes) ; }

  /*
static void svf_md5_finish(SVF_MD5_INTERNAL*pgv_mds, unsigned char digest[16])
  { md5_finish(&pgv_mds->mds,digest) ; }
*/
static void svf_md5_finish(SVF_MD5_INTERNAL*pgv_mds, unsigned char digest[16])
  { md5_state_t mds_prov = pgv_mds->mds;
    md5_finish(&mds_prov,digest) ; }

#endif
#endif

/*
void svf_md5_init(SVF_MD5_INTERNAL*);

void svf_md5_append(SVF_MD5_INTERNAL*, const unsigned char*, int nbytes);

void svf_md5_finish(SVF_MD5_INTERNAL*, unsigned char digest[16]);
*/

#ifdef __cplusplus
}  /* end extern "C" */
#endif

#endif
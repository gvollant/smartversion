#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef uint64_t IA32CAP;

extern "C"
{
 extern unsigned int OPENSSL_ia32cap_P[4];
 unsigned int *OPENSSL_ia32cap_loc(void) ;
 void OPENSSL_cpuid_setup(void);
 IA32CAP OPENSSL_ia32_cpuid(unsigned int *);
}



/* ====================================================================
 * Copyright (c) 1998-2006 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.openssl.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    openssl-core@openssl.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.openssl.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This product includes cryptographic software written by Eric Young
 * (eay@cryptsoft.com).  This product includes software written by Tim
 * Hudson (tjh@cryptsoft.com).
 *
 */
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 *
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 *
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 *
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */
/* ====================================================================
 * Copyright 2002 Sun Microsystems, Inc. ALL RIGHTS RESERVED.
 * ECDH support in OpenSSL originally developed by
 * SUN MICROSYSTEMS, INC., and contributed to the OpenSSL project.
 */



#if     defined(__i386)   || defined(__i386__)   || defined(_M_IX86) || \
        defined(__INTEL__) || \
        defined(__x86_64) || defined(__x86_64__) || \
        defined(_M_AMD64) || defined(_M_X64)

extern unsigned int OPENSSL_ia32cap_P[4];
unsigned int *OPENSSL_ia32cap_loc(void)
{
    return OPENSSL_ia32cap_P;
}

#ifndef OPENSSL_CPUID_OBJ
#define OPENSSL_CPUID_OBJ
#endif

# if defined(OPENSSL_CPUID_OBJ) && !defined(OPENSSL_NO_ASM) && !defined(I386_ONLY)
#include <stdio.h>
#include <stdint.h>
#  define OPENSSL_CPUID_SETUP
//typedef uint64_t IA32CAP;
void OPENSSL_cpuid_setup(void)
{
    static int trigger = 0;

    IA32CAP vec;
    char *env;

    if (trigger)
        return;

    trigger = 1;
    if ((env = getenv("OPENSSL_ia32cap"))) {
        int off = (env[0] == '~') ? 1 : 0;
#  if defined(_WIN32)
        if (!sscanf(env + off, "%I64i", &vec))
            vec = strtoul(env + off, NULL, 0);
#  else
        if (!sscanf(env + off, "%lli", (long long *)&vec))
            vec = strtoul(env + off, NULL, 0);
#  endif
        if (off)
            vec = OPENSSL_ia32_cpuid(OPENSSL_ia32cap_P) & ~vec;
        else if (env[0] == ':')
            vec = OPENSSL_ia32_cpuid(OPENSSL_ia32cap_P);

        OPENSSL_ia32cap_P[2] = 0;
        if ((env = strchr(env, ':'))) {
            unsigned int vecx;
            env++;
            off = (env[0] == '~') ? 1 : 0;
            vecx = strtoul(env + off, NULL, 0);
            if (off)
                OPENSSL_ia32cap_P[2] &= ~vecx;
            else
                OPENSSL_ia32cap_P[2] = vecx;
        }
    } else
        vec = OPENSSL_ia32_cpuid(OPENSSL_ia32cap_P);

    /*
     * |(1<<10) sets a reserved bit to signal that variable
     * was initialized already... This is to avoid interference
     * with cpuid snippets in ELF .init segment.
     */
    OPENSSL_ia32cap_P[0] = (unsigned int)vec | (1 << 10);
    OPENSSL_ia32cap_P[1] = (unsigned int)(vec >> 32);
}
# else
unsigned int OPENSSL_ia32cap_P[4];
# endif

#else
unsigned int *OPENSSL_ia32cap_loc(void)
{
    return NULL;
}
#endif
int OPENSSL_NONPIC_relocated = 0;
#if !defined(OPENSSL_CPUID_SETUP) && !defined(OPENSSL_CPUID_OBJ)
void OPENSSL_cpuid_setup(void)
{
}
#endif


#ifdef _MSC_VER
// _cpuid since vs2005
#if _MSC_VER>=1400

#include <intrin.h>
#endif
#endif

# if defined(__i386) || defined(__i386__) || defined(_M_IX86) || \
     defined( __i486 ) || defined( __i586 ) || defined( __i686 ) || \
     defined( __i486__ ) || defined( __i586__ ) || defined( __i686__ ) || \
     defined(__x86_64) || defined(__x86_64__) || defined(_M_AMD64) || defined(_M_X64)

int OPENSSL_cpuid_auto_setup(void)
{
	OPENSSL_ia32cap_P[0] = OPENSSL_ia32cap_P[1] = OPENSSL_ia32cap_P[2] = OPENSSL_ia32cap_P[3] = 0;

#ifdef _MSC_VER
// _cpuid since vs2005
#if _MSC_VER>=1400

// https://msdn.microsoft.com/en-us/library/hskdteyh(v=vs.140).aspx
// https://msdn.microsoft.com/en-us/library/hskdteyh(v=vs.100).aspx

	int CPUInfo[4] = { -1 };
	int nIds;
	__cpuid(CPUInfo, 0);
	nIds = CPUInfo[0];

	if (nIds >= 7)
	{
		__cpuid(CPUInfo, 7);
		int f_7_EBX_ = CPUInfo[1];
		OPENSSL_ia32cap_P[2] = (f_7_EBX_ & 0x20000000);
	}
#endif
#endif

	// with OPENSSL_ia32cap_P[2] = 0x20000000; we uses shaext
	//OPENSSL_ia32cap_P[2] = 0x20000000;


	//IA32CAP OPENSSL_ia32_cpuid(void);
	IA32CAP vec;




	vec = OPENSSL_ia32_cpuid(OPENSSL_ia32cap_P);

	/*
	* |(1<<10) sets a reserved bit to signal that variable
	* was initialized already... This is to avoid interference
	* with cpuid snippets in ELF .init segment.
	*/
	OPENSSL_ia32cap_P[0] = (unsigned int)vec | (1 << 10);
	OPENSSL_ia32cap_P[1] = (unsigned int)(vec >> 32);

	OPENSSL_cpuid_setup();

	return (int)vec;
}

int perform_setup = OPENSSL_cpuid_auto_setup();
#endif

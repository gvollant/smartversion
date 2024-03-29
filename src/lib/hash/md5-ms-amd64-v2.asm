
; MD5 optimized for AMD64.
;
; Author: Marc Bevand <bevand_m (at) epita.fr>
; Licence: I hereby disclaim the copyright on this code and place it
; in the public domain.
;
; Gilles Vollant <info (at) winimage.com > made the port to Intel/Amd
; mnemonic for Microsoft ML64 and Microsoft C++ for Windows x64
;
; to compile this file, I use option
;   ml64.exe /Flm5n64 /c /Zi m5n64.asm
;   with Microsoft Macro Assembler (x64) for AMD64
;
;   ml64.exe is given with Visual Studio 2005, Windows 2003 server DDK
;
;   (you can get Windows 2003 server DDK with ml64 and cl for AMD64 from
;      http://www.microsoft.com/whdc/devtools/ddk/default.mspx for low price)
;
;  http://etud.epita.fr/~bevand_m/papers/md5-amd64.html
;  http://www.winimage.com/md5-amd64-ms.htm
;
;  Charles Liu made optimisation on
;  http://article.gmane.org/gmane.comp.encryption.openssl.devel/9835
;
.code
md5_block_asm_host_order PROC
    push    rbp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15
    push    rsi
    push    rdi
; parameter 1 in rcx, param 2 in rdx , param 3 in r8

; see http://weblogs.asp.net/oldnewthing/archive/2004/01/14/58579.aspx and
; http://msdn.microsoft.com/library/en-us/kmarch/hh/kmarch/64bitAMD_8e951dd2-ee77-4728-8702-55ce4b5dd24a.xml.asp
;
; All registers must be preserved across the call, except for
;   rax, rcx, rdx, r8, r-9, r10, and r11, which are scratch.

    ;# rdi = arg #1 (ctx, MD5_CTX pointer)
    ;# rsi = arg #2 (ptr, data pointer)
    ;# rdx = arg #3 (nbr, number of 16-word blocks to process)

    mov rsi,rdx
    mov edx,r8d

    mov r12,rcx ;# rbp = ctx
    shl rdx,6   ;# rdx = nbr in bytes
    push r12
    lea rdi,[rsi+rdx];  # rdi = end

    mov eax,DWORD PTR 0[r12]    ;# eax = ctx->A
    mov ebx,DWORD PTR 4[r12]    ;# ebx = ctx->B
    mov ecx,DWORD PTR 8[r12]    ;# ecx = ctx->C
    mov edx,DWORD PTR 12[r12]   ;# edx = ctx->D
    ;push   rbp     ;# save ctx
    ;# end is 'rdi'
    ;# ptr is 'rsi'
    ;# A is 'eax'
    ;# B is 'ebx'
    ;# C is 'ecx'
    ;# D is 'edx'

; it is better with align 16 here, I don't known why
align 16
    cmp rsi,rdi     ;# cmp end with ptr
    mov r13d,0ffffffffh
    je  lab1        ;# jmp if ptr == end

    ;# BEGIN of loop over 16-word blocks
lab2:   ;# save old values of A, B, C, D
    mov r8d,eax
    mov r9d,ebx
    mov r14d,ecx
    mov r15d,edx
; BEGIN of the round serie
  mov r10 , QWORD PTR (0*4)[rsi]      ;/* (NEXT STEP) X[0] */
  mov r11d , edx          ;/* (NEXT STEP) z' = %edx */
    xor r11d,ecx     ;/* y ^ ... */
    lea eax,DWORD PTR 0d76aa478h [ eax * 1 +r10d ]      ;/* Const + dst + ... */
    and r11d,ebx             ;/* x & ... */
    xor r11d,edx             ;/* z ^ ... */
    ;mov    r10d,DWORD PTR (1*4)[rsi]     ;/* (NEXT STEP) X[1] */
    shr r10,32
    add eax,r11d           ;/* dst += ... */
    rol eax, 7            ;/* dst <<< s */
    mov r11d , ecx           ;/* (NEXT STEP) z' = ecx */
    add eax , ebx           ;/* dst += x */
    xor r11d,ebx     ;/* y ^ ... */
    lea edx,DWORD PTR 0e8c7b756h [ edx * 1 +r10d ]      ;/* Const + dst + ... */
    and r11d,eax             ;/* x & ... */
    xor r11d,ecx             ;/* z ^ ... */
    mov r10,QWORD PTR (2*4)[rsi]      ;/* (NEXT STEP) X[2] */
    add edx,r11d           ;/* dst += ... */
    rol edx, 12            ;/* dst <<< s */
    mov r11d , ebx           ;/* (NEXT STEP) z' = ebx */
    add edx , eax           ;/* dst += x */
    xor r11d,eax     ;/* y ^ ... */
    lea ecx,DWORD PTR 0242070dbh [ ecx * 1 +r10d ]      ;/* Const + dst + ... */
    and r11d,edx             ;/* x & ... */
    xor r11d,ebx             ;/* z ^ ... */
    ;mov    r10d,DWORD PTR (3*4)[rsi]     ;/* (NEXT STEP) X[3] */
    shr r10,32
    add ecx,r11d           ;/* dst += ... */
    rol ecx, 17            ;/* dst <<< s */
    mov r11d , eax           ;/* (NEXT STEP) z' = eax */
    add ecx , edx           ;/* dst += x */
    xor r11d,edx     ;/* y ^ ... */
    lea ebx,DWORD PTR 0c1bdceeeh [ ebx * 1 +r10d ]      ;/* Const + dst + ... */
    and r11d,ecx             ;/* x & ... */
    xor r11d,eax             ;/* z ^ ... */
    mov r10,QWORD PTR (4*4)[rsi]      ;/* (NEXT STEP) X[4] */
    add ebx,r11d           ;/* dst += ... */
    rol ebx, 22            ;/* dst <<< s */
    mov r11d , edx           ;/* (NEXT STEP) z' = edx */
    add ebx , ecx           ;/* dst += x */
    xor r11d,ecx     ;/* y ^ ... */
    lea eax,DWORD PTR 0f57c0fafh [ eax * 1 +r10d ]      ;/* Const + dst + ... */
    and r11d,ebx             ;/* x & ... */
    xor r11d,edx             ;/* z ^ ... */
    ;mov    r10d,DWORD PTR (5*4)[rsi]     ;/* (NEXT STEP) X[5] */
    shr r10,32
    add eax,r11d           ;/* dst += ... */
    rol eax, 7            ;/* dst <<< s */
    mov r11d , ecx           ;/* (NEXT STEP) z' = ecx */
    add eax , ebx           ;/* dst += x */
    xor r11d,ebx     ;/* y ^ ... */
    lea edx,DWORD PTR 04787c62ah [ edx * 1 +r10d ]      ;/* Const + dst + ... */
    and r11d,eax             ;/* x & ... */
    xor r11d,ecx             ;/* z ^ ... */
    mov r10,QWORD PTR (6*4)[rsi]      ;/* (NEXT STEP) X[6] */
    add edx,r11d           ;/* dst += ... */
    rol edx, 12            ;/* dst <<< s */
    mov r11d , ebx           ;/* (NEXT STEP) z' = ebx */
    add edx , eax           ;/* dst += x */
    xor r11d,eax     ;/* y ^ ... */
    lea ecx,DWORD PTR 0a8304613h [ ecx * 1 +r10d ]      ;/* Const + dst + ... */
    and r11d,edx             ;/* x & ... */
    xor r11d,ebx             ;/* z ^ ... */
    ;mov    r10d,DWORD PTR (7*4)[rsi]     ;/* (NEXT STEP) X[7] */
    shr r10,32
    add ecx,r11d           ;/* dst += ... */
    rol ecx, 17            ;/* dst <<< s */
    mov r11d , eax           ;/* (NEXT STEP) z' = eax */
    add ecx , edx           ;/* dst += x */
    xor r11d,edx     ;/* y ^ ... */
    lea ebx,DWORD PTR 0fd469501h [ ebx * 1 +r10d ]      ;/* Const + dst + ... */
    and r11d,ecx             ;/* x & ... */
    xor r11d,eax             ;/* z ^ ... */
    mov r10,QWORD PTR (8*4)[rsi]      ;/* (NEXT STEP) X[8] */
    add ebx,r11d           ;/* dst += ... */
    rol ebx, 22            ;/* dst <<< s */
    mov r11d , edx           ;/* (NEXT STEP) z' = edx */
    add ebx , ecx           ;/* dst += x */
    xor r11d,ecx     ;/* y ^ ... */
    lea eax,DWORD PTR 0698098d8h [ eax * 1 +r10d ]      ;/* Const + dst + ... */
    and r11d,ebx             ;/* x & ... */
    xor r11d,edx             ;/* z ^ ... */
    ;mov    r10d,DWORD PTR (9*4)[rsi]     ;/* (NEXT STEP) X[9] */
    shr r10,32
    add eax,r11d           ;/* dst += ... */
    rol eax, 7            ;/* dst <<< s */
    mov r11d , ecx           ;/* (NEXT STEP) z' = ecx */
    add eax , ebx           ;/* dst += x */
    xor r11d,ebx     ;/* y ^ ... */
    lea edx,DWORD PTR 08b44f7afh [ edx * 1 +r10d ]      ;/* Const + dst + ... */
    and r11d,eax             ;/* x & ... */
    xor r11d,ecx             ;/* z ^ ... */
    mov r10,QWORD PTR (10*4)[rsi]      ;/* (NEXT STEP) X[10] */
    add edx,r11d           ;/* dst += ... */
    rol edx, 12            ;/* dst <<< s */
    mov r11d , ebx           ;/* (NEXT STEP) z' = ebx */
    add edx , eax           ;/* dst += x */
    xor r11d,eax     ;/* y ^ ... */
    lea ecx,DWORD PTR 0ffff5bb1h [ ecx * 1 +r10d ]      ;/* Const + dst + ... */
    and r11d,edx             ;/* x & ... */
    xor r11d,ebx             ;/* z ^ ... */
    ;mov    r10d,DWORD PTR (11*4)[rsi]     ;/* (NEXT STEP) X[11] */
    shr r10,32
    add ecx,r11d           ;/* dst += ... */
    rol ecx, 17            ;/* dst <<< s */
    mov r11d , eax           ;/* (NEXT STEP) z' = eax */
    add ecx , edx           ;/* dst += x */
    xor r11d,edx     ;/* y ^ ... */
    lea ebx,DWORD PTR 0895cd7beh [ ebx * 1 +r10d ]      ;/* Const + dst + ... */
    and r11d,ecx             ;/* x & ... */
    xor r11d,eax             ;/* z ^ ... */
    mov r10,QWORD PTR (12*4)[rsi]      ;/* (NEXT STEP) X[12] */
    add ebx,r11d           ;/* dst += ... */
    rol ebx, 22            ;/* dst <<< s */
    mov r11d , edx           ;/* (NEXT STEP) z' = edx */
    add ebx , ecx           ;/* dst += x */
    xor r11d,ecx     ;/* y ^ ... */
    lea eax,DWORD PTR 06b901122h [ eax * 1 +r10d ]      ;/* Const + dst + ... */
    and r11d,ebx             ;/* x & ... */
    xor r11d,edx             ;/* z ^ ... */
    ;mov    r10d,DWORD PTR (13*4)[rsi]     ;/* (NEXT STEP) X[13] */
    shr r10,32
    add eax,r11d           ;/* dst += ... */
    rol eax, 7            ;/* dst <<< s */
    mov r11d , ecx           ;/* (NEXT STEP) z' = ecx */
    add eax , ebx           ;/* dst += x */
    xor r11d,ebx     ;/* y ^ ... */
    lea edx,DWORD PTR 0fd987193h [ edx * 1 +r10d ]      ;/* Const + dst + ... */
    and r11d,eax             ;/* x & ... */
    xor r11d,ecx             ;/* z ^ ... */
    mov r10,QWORD PTR (14*4)[rsi]      ;/* (NEXT STEP) X[14] */
    add edx,r11d           ;/* dst += ... */
    rol edx, 12            ;/* dst <<< s */
    mov r11d , ebx           ;/* (NEXT STEP) z' = ebx */
    add edx , eax           ;/* dst += x */
    xor r11d,eax     ;/* y ^ ... */
    lea ecx,DWORD PTR 0a679438eh [ ecx * 1 +r10d ]      ;/* Const + dst + ... */
    and r11d,edx             ;/* x & ... */
    xor r11d,ebx             ;/* z ^ ... */
    ;mov    r10d,DWORD PTR (15*4)[rsi]     ;/* (NEXT STEP) X[15] */
    shr r10,32
    add ecx,r11d           ;/* dst += ... */
    rol ecx, 17            ;/* dst <<< s */
    mov r11d , eax           ;/* (NEXT STEP) z' = eax */
    add ecx , edx           ;/* dst += x */
    xor r11d,edx     ;/* y ^ ... */
    lea ebx,DWORD PTR 049b40821h [ ebx * 1 +r10d ]      ;/* Const + dst + ... */
    and r11d,ecx             ;/* x & ... */
    xor r11d,eax             ;/* z ^ ... */
    mov r10,QWORD PTR (0*4)[rsi]      ;/* (NEXT STEP) X[0] */
    add ebx,r11d           ;/* dst += ... */
    rol ebx, 22            ;/* dst <<< s */
    mov r11d , edx           ;/* (NEXT STEP) z' = edx */
    add ebx , ecx           ;/* dst += x */
 mov  r10d , 4 [rsi]      ;/* (NEXT STEP) X[1] */
 mov  r11d, edx       ;/* (NEXT STEP) z' = %edx */
 mov  r12d, edx       ;/* (NEXT STEP) z' = %edx */
    not r11d
    lea eax,DWORD PTR 0f61e2562h [ eax * 1 +r10d ]      ;/* Const + dst + ... */

    and r12d,ebx     ;/* x & z */
    and r11d,ecx     ;/* y & (not z) */

    mov r10d , (6*4) [rsi]        ;/* (NEXT STEP) X[6] */



    or  r12d,r11d   ;/* (y & (not z)) | (x & z) */
    mov r11d,ecx     ;/* (NEXT STEP) z' = ecx */
    add eax, r12d  ;   /* dst += ... */
    mov r12d,ecx     ;/* (NEXT STEP) z' = ecx */


    rol eax , 5           ;/* dst <<< s */
    add eax , ebx           ;/* dst += x */
    not r11d
    lea edx,DWORD PTR 0c040b340h [ edx * 1 +r10d ]      ;/* Const + dst + ... */

    and r12d,eax     ;/* x & z */
    and r11d,ebx     ;/* y & (not z) */

    mov r10d , (11*4) [rsi]        ;/* (NEXT STEP) X[11] */



    or  r12d,r11d   ;/* (y & (not z)) | (x & z) */
    mov r11d,ebx     ;/* (NEXT STEP) z' = ebx */
    add edx, r12d  ;   /* dst += ... */
    mov r12d,ebx     ;/* (NEXT STEP) z' = ebx */


    rol edx , 9           ;/* dst <<< s */
    add edx , eax           ;/* dst += x */
    not r11d
    lea ecx,DWORD PTR 0265e5a51h [ ecx * 1 +r10d ]      ;/* Const + dst + ... */

    and r12d,edx     ;/* x & z */
    and r11d,eax     ;/* y & (not z) */

    mov r10d , (0*4) [rsi]        ;/* (NEXT STEP) X[0] */



    or  r12d,r11d   ;/* (y & (not z)) | (x & z) */
    mov r11d,eax     ;/* (NEXT STEP) z' = eax */
    add ecx, r12d  ;   /* dst += ... */
    mov r12d,eax     ;/* (NEXT STEP) z' = eax */


    rol ecx , 14           ;/* dst <<< s */
    add ecx , edx           ;/* dst += x */
    not r11d
    lea ebx,DWORD PTR 0e9b6c7aah [ ebx * 1 +r10d ]      ;/* Const + dst + ... */

    and r12d,ecx     ;/* x & z */
    and r11d,edx     ;/* y & (not z) */

    mov r10d , (5*4) [rsi]        ;/* (NEXT STEP) X[5] */



    or  r12d,r11d   ;/* (y & (not z)) | (x & z) */
    mov r11d,edx     ;/* (NEXT STEP) z' = edx */
    add ebx, r12d  ;   /* dst += ... */
    mov r12d,edx     ;/* (NEXT STEP) z' = edx */


    rol ebx , 20           ;/* dst <<< s */
    add ebx , ecx           ;/* dst += x */
    not r11d
    lea eax,DWORD PTR 0d62f105dh [ eax * 1 +r10d ]      ;/* Const + dst + ... */

    and r12d,ebx     ;/* x & z */
    and r11d,ecx     ;/* y & (not z) */

    mov r10d , (10*4) [rsi]        ;/* (NEXT STEP) X[10] */



    or  r12d,r11d   ;/* (y & (not z)) | (x & z) */
    mov r11d,ecx     ;/* (NEXT STEP) z' = ecx */
    add eax, r12d  ;   /* dst += ... */
    mov r12d,ecx     ;/* (NEXT STEP) z' = ecx */


    rol eax , 5           ;/* dst <<< s */
    add eax , ebx           ;/* dst += x */
    not r11d
    lea edx,DWORD PTR 02441453h [ edx * 1 +r10d ]      ;/* Const + dst + ... */

    and r12d,eax     ;/* x & z */
    and r11d,ebx     ;/* y & (not z) */

    mov r10d , (15*4) [rsi]        ;/* (NEXT STEP) X[15] */



    or  r12d,r11d   ;/* (y & (not z)) | (x & z) */
    mov r11d,ebx     ;/* (NEXT STEP) z' = ebx */
    add edx, r12d  ;   /* dst += ... */
    mov r12d,ebx     ;/* (NEXT STEP) z' = ebx */


    rol edx , 9           ;/* dst <<< s */
    add edx , eax           ;/* dst += x */
    not r11d
    lea ecx,DWORD PTR 0d8a1e681h [ ecx * 1 +r10d ]      ;/* Const + dst + ... */

    and r12d,edx     ;/* x & z */
    and r11d,eax     ;/* y & (not z) */

    mov r10d , (4*4) [rsi]        ;/* (NEXT STEP) X[4] */



    or  r12d,r11d   ;/* (y & (not z)) | (x & z) */
    mov r11d,eax     ;/* (NEXT STEP) z' = eax */
    add ecx, r12d  ;   /* dst += ... */
    mov r12d,eax     ;/* (NEXT STEP) z' = eax */


    rol ecx , 14           ;/* dst <<< s */
    add ecx , edx           ;/* dst += x */
    not r11d
    lea ebx,DWORD PTR 0e7d3fbc8h [ ebx * 1 +r10d ]      ;/* Const + dst + ... */

    and r12d,ecx     ;/* x & z */
    and r11d,edx     ;/* y & (not z) */

    mov r10d , (9*4) [rsi]        ;/* (NEXT STEP) X[9] */



    or  r12d,r11d   ;/* (y & (not z)) | (x & z) */
    mov r11d,edx     ;/* (NEXT STEP) z' = edx */
    add ebx, r12d  ;   /* dst += ... */
    mov r12d,edx     ;/* (NEXT STEP) z' = edx */


    rol ebx , 20           ;/* dst <<< s */
    add ebx , ecx           ;/* dst += x */
    not r11d
    lea eax,DWORD PTR 021e1cde6h [ eax * 1 +r10d ]      ;/* Const + dst + ... */

    and r12d,ebx     ;/* x & z */
    and r11d,ecx     ;/* y & (not z) */

    mov r10d , (14*4) [rsi]        ;/* (NEXT STEP) X[14] */



    or  r12d,r11d   ;/* (y & (not z)) | (x & z) */
    mov r11d,ecx     ;/* (NEXT STEP) z' = ecx */
    add eax, r12d  ;   /* dst += ... */
    mov r12d,ecx     ;/* (NEXT STEP) z' = ecx */


    rol eax , 5           ;/* dst <<< s */
    add eax , ebx           ;/* dst += x */
    not r11d
    lea edx,DWORD PTR 0c33707d6h [ edx * 1 +r10d ]      ;/* Const + dst + ... */

    and r12d,eax     ;/* x & z */
    and r11d,ebx     ;/* y & (not z) */

    mov r10d , (3*4) [rsi]        ;/* (NEXT STEP) X[3] */



    or  r12d,r11d   ;/* (y & (not z)) | (x & z) */
    mov r11d,ebx     ;/* (NEXT STEP) z' = ebx */
    add edx, r12d  ;   /* dst += ... */
    mov r12d,ebx     ;/* (NEXT STEP) z' = ebx */


    rol edx , 9           ;/* dst <<< s */
    add edx , eax           ;/* dst += x */
    not r11d
    lea ecx,DWORD PTR 0f4d50d87h [ ecx * 1 +r10d ]      ;/* Const + dst + ... */

    and r12d,edx     ;/* x & z */
    and r11d,eax     ;/* y & (not z) */

    mov r10d , (8*4) [rsi]        ;/* (NEXT STEP) X[8] */



    or  r12d,r11d   ;/* (y & (not z)) | (x & z) */
    mov r11d,eax     ;/* (NEXT STEP) z' = eax */
    add ecx, r12d  ;   /* dst += ... */
    mov r12d,eax     ;/* (NEXT STEP) z' = eax */


    rol ecx , 14           ;/* dst <<< s */
    add ecx , edx           ;/* dst += x */
    not r11d
    lea ebx,DWORD PTR 0455a14edh [ ebx * 1 +r10d ]      ;/* Const + dst + ... */

    and r12d,ecx     ;/* x & z */
    and r11d,edx     ;/* y & (not z) */

    mov r10d , (13*4) [rsi]        ;/* (NEXT STEP) X[13] */



    or  r12d,r11d   ;/* (y & (not z)) | (x & z) */
    mov r11d,edx     ;/* (NEXT STEP) z' = edx */
    add ebx, r12d  ;   /* dst += ... */
    mov r12d,edx     ;/* (NEXT STEP) z' = edx */


    rol ebx , 20           ;/* dst <<< s */
    add ebx , ecx           ;/* dst += x */
    not r11d
    lea eax,DWORD PTR 0a9e3e905h [ eax * 1 +r10d ]      ;/* Const + dst + ... */

    and r12d,ebx     ;/* x & z */
    and r11d,ecx     ;/* y & (not z) */

    mov r10d , (2*4) [rsi]        ;/* (NEXT STEP) X[2] */



    or  r12d,r11d   ;/* (y & (not z)) | (x & z) */
    mov r11d,ecx     ;/* (NEXT STEP) z' = ecx */
    add eax, r12d  ;   /* dst += ... */
    mov r12d,ecx     ;/* (NEXT STEP) z' = ecx */


    rol eax , 5           ;/* dst <<< s */
    add eax , ebx           ;/* dst += x */
    not r11d
    lea edx,DWORD PTR 0fcefa3f8h [ edx * 1 +r10d ]      ;/* Const + dst + ... */

    and r12d,eax     ;/* x & z */
    and r11d,ebx     ;/* y & (not z) */

    mov r10d , (7*4) [rsi]        ;/* (NEXT STEP) X[7] */



    or  r12d,r11d   ;/* (y & (not z)) | (x & z) */
    mov r11d,ebx     ;/* (NEXT STEP) z' = ebx */
    add edx, r12d  ;   /* dst += ... */
    mov r12d,ebx     ;/* (NEXT STEP) z' = ebx */


    rol edx , 9           ;/* dst <<< s */
    add edx , eax           ;/* dst += x */
    not r11d
    lea ecx,DWORD PTR 0676f02d9h [ ecx * 1 +r10d ]      ;/* Const + dst + ... */

    and r12d,edx     ;/* x & z */
    and r11d,eax     ;/* y & (not z) */

    mov r10d , (12*4) [rsi]        ;/* (NEXT STEP) X[12] */



    or  r12d,r11d   ;/* (y & (not z)) | (x & z) */
    mov r11d,eax     ;/* (NEXT STEP) z' = eax */
    add ecx, r12d  ;   /* dst += ... */
    mov r12d,eax     ;/* (NEXT STEP) z' = eax */


    rol ecx , 14           ;/* dst <<< s */
    add ecx , edx           ;/* dst += x */
    not r11d
    lea ebx,DWORD PTR 08d2a4c8ah [ ebx * 1 +r10d ]      ;/* Const + dst + ... */

    and r12d,ecx     ;/* x & z */
    and r11d,edx     ;/* y & (not z) */

    mov r10d , (0*4) [rsi]        ;/* (NEXT STEP) X[0] */



    or  r12d,r11d   ;/* (y & (not z)) | (x & z) */
    mov r11d,edx     ;/* (NEXT STEP) z' = edx */
    add ebx, r12d  ;   /* dst += ... */
    mov r12d,edx     ;/* (NEXT STEP) z' = edx */


    rol ebx , 20           ;/* dst <<< s */
    add ebx , ecx           ;/* dst += x */
 mov  r10d , (5*4)[rsi]       ;/* (NEXT STEP) X[5] */
 mov  r11d , ecx      ;/* (NEXT STEP) y' = %ecx */
    lea eax,DWORD PTR 0fffa3942h [ eax * 1 +r10d ]      ;/* Const + dst + ... */
    mov r10d,DWORD PTR (8*4)[rsi]     ;/* (NEXT STEP) X[8] */
    xor r11d,edx         ;/* z ^ ... */
    xor r11d,ebx             ;/* x ^ ... */
    add eax , r11d         ;/* dst += ... */
    rol eax , 4           ;/* dst <<< s */
    mov r11d , ebx           ;/* (NEXT STEP) y' = ebx */
    add eax , ebx           ;/* dst += x */
    lea edx,DWORD PTR 08771f681h [ edx * 1 +r10d ]      ;/* Const + dst + ... */
    mov r10d,DWORD PTR (11*4)[rsi]     ;/* (NEXT STEP) X[11] */
    xor r11d,ecx         ;/* z ^ ... */
    xor r11d,eax             ;/* x ^ ... */
    add edx , r11d         ;/* dst += ... */
    rol edx , 11           ;/* dst <<< s */
    mov r11d , eax           ;/* (NEXT STEP) y' = eax */
    add edx , eax           ;/* dst += x */
    lea ecx,DWORD PTR 06d9d6122h [ ecx * 1 +r10d ]      ;/* Const + dst + ... */
    mov r10d,DWORD PTR (14*4)[rsi]     ;/* (NEXT STEP) X[14] */
    xor r11d,ebx         ;/* z ^ ... */
    xor r11d,edx             ;/* x ^ ... */
    add ecx , r11d         ;/* dst += ... */
    rol ecx , 16           ;/* dst <<< s */
    mov r11d , edx           ;/* (NEXT STEP) y' = edx */
    add ecx , edx           ;/* dst += x */
    lea ebx,DWORD PTR 0fde5380ch [ ebx * 1 +r10d ]      ;/* Const + dst + ... */
    mov r10d,DWORD PTR (1*4)[rsi]     ;/* (NEXT STEP) X[1] */
    xor r11d,eax         ;/* z ^ ... */
    xor r11d,ecx             ;/* x ^ ... */
    add ebx , r11d         ;/* dst += ... */
    rol ebx , 23           ;/* dst <<< s */
    mov r11d , ecx           ;/* (NEXT STEP) y' = ecx */
    add ebx , ecx           ;/* dst += x */
    lea eax,DWORD PTR 0a4beea44h [ eax * 1 +r10d ]      ;/* Const + dst + ... */
    mov r10d,DWORD PTR (4*4)[rsi]     ;/* (NEXT STEP) X[4] */
    xor r11d,edx         ;/* z ^ ... */
    xor r11d,ebx             ;/* x ^ ... */
    add eax , r11d         ;/* dst += ... */
    rol eax , 4           ;/* dst <<< s */
    mov r11d , ebx           ;/* (NEXT STEP) y' = ebx */
    add eax , ebx           ;/* dst += x */
    lea edx,DWORD PTR 04bdecfa9h [ edx * 1 +r10d ]      ;/* Const + dst + ... */
    mov r10d,DWORD PTR (7*4)[rsi]     ;/* (NEXT STEP) X[7] */
    xor r11d,ecx         ;/* z ^ ... */
    xor r11d,eax             ;/* x ^ ... */
    add edx , r11d         ;/* dst += ... */
    rol edx , 11           ;/* dst <<< s */
    mov r11d , eax           ;/* (NEXT STEP) y' = eax */
    add edx , eax           ;/* dst += x */
    lea ecx,DWORD PTR 0f6bb4b60h [ ecx * 1 +r10d ]      ;/* Const + dst + ... */
    mov r10d,DWORD PTR (10*4)[rsi]     ;/* (NEXT STEP) X[10] */
    xor r11d,ebx         ;/* z ^ ... */
    xor r11d,edx             ;/* x ^ ... */
    add ecx , r11d         ;/* dst += ... */
    rol ecx , 16           ;/* dst <<< s */
    mov r11d , edx           ;/* (NEXT STEP) y' = edx */
    add ecx , edx           ;/* dst += x */
    lea ebx,DWORD PTR 0bebfbc70h [ ebx * 1 +r10d ]      ;/* Const + dst + ... */
    mov r10d,DWORD PTR (13*4)[rsi]     ;/* (NEXT STEP) X[13] */
    xor r11d,eax         ;/* z ^ ... */
    xor r11d,ecx             ;/* x ^ ... */
    add ebx , r11d         ;/* dst += ... */
    rol ebx , 23           ;/* dst <<< s */
    mov r11d , ecx           ;/* (NEXT STEP) y' = ecx */
    add ebx , ecx           ;/* dst += x */
    lea eax,DWORD PTR 0289b7ec6h [ eax * 1 +r10d ]      ;/* Const + dst + ... */
    mov r10d,DWORD PTR (0*4)[rsi]     ;/* (NEXT STEP) X[0] */
    xor r11d,edx         ;/* z ^ ... */
    xor r11d,ebx             ;/* x ^ ... */
    add eax , r11d         ;/* dst += ... */
    rol eax , 4           ;/* dst <<< s */
    mov r11d , ebx           ;/* (NEXT STEP) y' = ebx */
    add eax , ebx           ;/* dst += x */
    lea edx,DWORD PTR 0eaa127fah [ edx * 1 +r10d ]      ;/* Const + dst + ... */
    mov r10d,DWORD PTR (3*4)[rsi]     ;/* (NEXT STEP) X[3] */
    xor r11d,ecx         ;/* z ^ ... */
    xor r11d,eax             ;/* x ^ ... */
    add edx , r11d         ;/* dst += ... */
    rol edx , 11           ;/* dst <<< s */
    mov r11d , eax           ;/* (NEXT STEP) y' = eax */
    add edx , eax           ;/* dst += x */
    lea ecx,DWORD PTR 0d4ef3085h [ ecx * 1 +r10d ]      ;/* Const + dst + ... */
    mov r10d,DWORD PTR (6*4)[rsi]     ;/* (NEXT STEP) X[6] */
    xor r11d,ebx         ;/* z ^ ... */
    xor r11d,edx             ;/* x ^ ... */
    add ecx , r11d         ;/* dst += ... */
    rol ecx , 16           ;/* dst <<< s */
    mov r11d , edx           ;/* (NEXT STEP) y' = edx */
    add ecx , edx           ;/* dst += x */
    lea ebx,DWORD PTR 04881d05h [ ebx * 1 +r10d ]      ;/* Const + dst + ... */
    mov r10d,DWORD PTR (9*4)[rsi]     ;/* (NEXT STEP) X[9] */
    xor r11d,eax         ;/* z ^ ... */
    xor r11d,ecx             ;/* x ^ ... */
    add ebx , r11d         ;/* dst += ... */
    rol ebx , 23           ;/* dst <<< s */
    mov r11d , ecx           ;/* (NEXT STEP) y' = ecx */
    add ebx , ecx           ;/* dst += x */
    lea eax,DWORD PTR 0d9d4d039h [ eax * 1 +r10d ]      ;/* Const + dst + ... */
    mov r10d,DWORD PTR (12*4)[rsi]     ;/* (NEXT STEP) X[12] */
    xor r11d,edx         ;/* z ^ ... */
    xor r11d,ebx             ;/* x ^ ... */
    add eax , r11d         ;/* dst += ... */
    rol eax , 4           ;/* dst <<< s */
    mov r11d , ebx           ;/* (NEXT STEP) y' = ebx */
    add eax , ebx           ;/* dst += x */
    lea edx,DWORD PTR 0e6db99e5h [ edx * 1 +r10d ]      ;/* Const + dst + ... */
    mov r10d,DWORD PTR (15*4)[rsi]     ;/* (NEXT STEP) X[15] */
    xor r11d,ecx         ;/* z ^ ... */
    xor r11d,eax             ;/* x ^ ... */
    add edx , r11d         ;/* dst += ... */
    rol edx , 11           ;/* dst <<< s */
    mov r11d , eax           ;/* (NEXT STEP) y' = eax */
    add edx , eax           ;/* dst += x */
    lea ecx,DWORD PTR 01fa27cf8h [ ecx * 1 +r10d ]      ;/* Const + dst + ... */
    mov r10d,DWORD PTR (2*4)[rsi]     ;/* (NEXT STEP) X[2] */
    xor r11d,ebx         ;/* z ^ ... */
    xor r11d,edx             ;/* x ^ ... */
    add ecx , r11d         ;/* dst += ... */
    rol ecx , 16           ;/* dst <<< s */
    mov r11d , edx           ;/* (NEXT STEP) y' = edx */
    add ecx , edx           ;/* dst += x */
    lea ebx,DWORD PTR 0c4ac5665h [ ebx * 1 +r10d ]      ;/* Const + dst + ... */
    mov r10d,DWORD PTR (0*4)[rsi]     ;/* (NEXT STEP) X[0] */
    xor r11d,eax         ;/* z ^ ... */
    xor r11d,ecx             ;/* x ^ ... */
    add ebx , r11d         ;/* dst += ... */
    rol ebx , 23           ;/* dst <<< s */
    mov r11d , ecx           ;/* (NEXT STEP) y' = ecx */
    add ebx , ecx           ;/* dst += x */
 mov  r10d , (0*4)[rsi]       ;/* (NEXT STEP) X[0] */
 mov  r11d , r13d ;0ffffffffh ;%r11d
 xor  r11d , edx      ;/* (NEXT STEP) not z' = not %edx*/
    lea eax,DWORD PTR 0f4292244h [ eax * 1 +r10d ]      ;/* Const + dst + ... */
    or  r11d , ebx           ;/* x | ... */
    xor r11d , ecx           ;/* y ^ ... */
    add eax , r11d         ;/* dst += ... */
    mov r10d , DWORD PTR (7*4)[rsi]       ;/* (NEXT STEP) X[7] */
    mov r11d , r13d ; 0ffffffffh
    rol eax , 6           ;/* dst <<< s */
    xor r11d , ecx           ;/* (NEXT STEP) not z' = not ecx */
    add eax , ebx           ;/* dst += x */
    lea edx,DWORD PTR 0432aff97h [ edx * 1 +r10d ]      ;/* Const + dst + ... */
    or  r11d , eax           ;/* x | ... */
    xor r11d , ebx           ;/* y ^ ... */
    add edx , r11d         ;/* dst += ... */
    mov r10d , DWORD PTR (14*4)[rsi]       ;/* (NEXT STEP) X[14] */
    mov r11d , r13d ; 0ffffffffh
    rol edx , 10           ;/* dst <<< s */
    xor r11d , ebx           ;/* (NEXT STEP) not z' = not ebx */
    add edx , eax           ;/* dst += x */
    lea ecx,DWORD PTR 0ab9423a7h [ ecx * 1 +r10d ]      ;/* Const + dst + ... */
    or  r11d , edx           ;/* x | ... */
    xor r11d , eax           ;/* y ^ ... */
    add ecx , r11d         ;/* dst += ... */
    mov r10d , DWORD PTR (5*4)[rsi]       ;/* (NEXT STEP) X[5] */
    mov r11d , r13d ; 0ffffffffh
    rol ecx , 15           ;/* dst <<< s */
    xor r11d , eax           ;/* (NEXT STEP) not z' = not eax */
    add ecx , edx           ;/* dst += x */
    lea ebx,DWORD PTR 0fc93a039h [ ebx * 1 +r10d ]      ;/* Const + dst + ... */
    or  r11d , ecx           ;/* x | ... */
    xor r11d , edx           ;/* y ^ ... */
    add ebx , r11d         ;/* dst += ... */
    mov r10d , DWORD PTR (12*4)[rsi]       ;/* (NEXT STEP) X[12] */
    mov r11d , r13d ; 0ffffffffh
    rol ebx , 21           ;/* dst <<< s */
    xor r11d , edx           ;/* (NEXT STEP) not z' = not edx */
    add ebx , ecx           ;/* dst += x */
    lea eax,DWORD PTR 0655b59c3h [ eax * 1 +r10d ]      ;/* Const + dst + ... */
    or  r11d , ebx           ;/* x | ... */
    xor r11d , ecx           ;/* y ^ ... */
    add eax , r11d         ;/* dst += ... */
    mov r10d , DWORD PTR (3*4)[rsi]       ;/* (NEXT STEP) X[3] */
    mov r11d , r13d ; 0ffffffffh
    rol eax , 6           ;/* dst <<< s */
    xor r11d , ecx           ;/* (NEXT STEP) not z' = not ecx */
    add eax , ebx           ;/* dst += x */
    lea edx,DWORD PTR 08f0ccc92h [ edx * 1 +r10d ]      ;/* Const + dst + ... */
    or  r11d , eax           ;/* x | ... */
    xor r11d , ebx           ;/* y ^ ... */
    add edx , r11d         ;/* dst += ... */
    mov r10d , DWORD PTR (10*4)[rsi]       ;/* (NEXT STEP) X[10] */
    mov r11d , r13d ; 0ffffffffh
    rol edx , 10           ;/* dst <<< s */
    xor r11d , ebx           ;/* (NEXT STEP) not z' = not ebx */
    add edx , eax           ;/* dst += x */
    lea ecx,DWORD PTR 0ffeff47dh [ ecx * 1 +r10d ]      ;/* Const + dst + ... */
    or  r11d , edx           ;/* x | ... */
    xor r11d , eax           ;/* y ^ ... */
    add ecx , r11d         ;/* dst += ... */
    mov r10d , DWORD PTR (1*4)[rsi]       ;/* (NEXT STEP) X[1] */
    mov r11d , r13d ; 0ffffffffh
    rol ecx , 15           ;/* dst <<< s */
    xor r11d , eax           ;/* (NEXT STEP) not z' = not eax */
    add ecx , edx           ;/* dst += x */
    lea ebx,DWORD PTR 085845dd1h [ ebx * 1 +r10d ]      ;/* Const + dst + ... */
    or  r11d , ecx           ;/* x | ... */
    xor r11d , edx           ;/* y ^ ... */
    add ebx , r11d         ;/* dst += ... */
    mov r10d , DWORD PTR (8*4)[rsi]       ;/* (NEXT STEP) X[8] */
    mov r11d , r13d ; 0ffffffffh
    rol ebx , 21           ;/* dst <<< s */
    xor r11d , edx           ;/* (NEXT STEP) not z' = not edx */
    add ebx , ecx           ;/* dst += x */
    lea eax,DWORD PTR 06fa87e4fh [ eax * 1 +r10d ]      ;/* Const + dst + ... */
    or  r11d , ebx           ;/* x | ... */
    xor r11d , ecx           ;/* y ^ ... */
    add eax , r11d         ;/* dst += ... */
    mov r10d , DWORD PTR (15*4)[rsi]       ;/* (NEXT STEP) X[15] */
    mov r11d , r13d ; 0ffffffffh
    rol eax , 6           ;/* dst <<< s */
    xor r11d , ecx           ;/* (NEXT STEP) not z' = not ecx */
    add eax , ebx           ;/* dst += x */
    lea edx,DWORD PTR 0fe2ce6e0h [ edx * 1 +r10d ]      ;/* Const + dst + ... */
    or  r11d , eax           ;/* x | ... */
    xor r11d , ebx           ;/* y ^ ... */
    add edx , r11d         ;/* dst += ... */
    mov r10d , DWORD PTR (6*4)[rsi]       ;/* (NEXT STEP) X[6] */
    mov r11d , r13d ; 0ffffffffh
    rol edx , 10           ;/* dst <<< s */
    xor r11d , ebx           ;/* (NEXT STEP) not z' = not ebx */
    add edx , eax           ;/* dst += x */
    lea ecx,DWORD PTR 0a3014314h [ ecx * 1 +r10d ]      ;/* Const + dst + ... */
    or  r11d , edx           ;/* x | ... */
    xor r11d , eax           ;/* y ^ ... */
    add ecx , r11d         ;/* dst += ... */
    mov r10d , DWORD PTR (13*4)[rsi]       ;/* (NEXT STEP) X[13] */
    mov r11d , r13d ; 0ffffffffh
    rol ecx , 15           ;/* dst <<< s */
    xor r11d , eax           ;/* (NEXT STEP) not z' = not eax */
    add ecx , edx           ;/* dst += x */
    lea ebx,DWORD PTR 04e0811a1h [ ebx * 1 +r10d ]      ;/* Const + dst + ... */
    or  r11d , ecx           ;/* x | ... */
    xor r11d , edx           ;/* y ^ ... */
    add ebx , r11d         ;/* dst += ... */
    mov r10d , DWORD PTR (4*4)[rsi]       ;/* (NEXT STEP) X[4] */
    mov r11d , r13d ; 0ffffffffh
    rol ebx , 21           ;/* dst <<< s */
    xor r11d , edx           ;/* (NEXT STEP) not z' = not edx */
    add ebx , ecx           ;/* dst += x */
    lea eax,DWORD PTR 0f7537e82h [ eax * 1 +r10d ]      ;/* Const + dst + ... */
    or  r11d , ebx           ;/* x | ... */
    xor r11d , ecx           ;/* y ^ ... */
    add eax , r11d         ;/* dst += ... */
    mov r10d , DWORD PTR (11*4)[rsi]       ;/* (NEXT STEP) X[11] */
    mov r11d , r13d ; 0ffffffffh
    rol eax , 6           ;/* dst <<< s */
    xor r11d , ecx           ;/* (NEXT STEP) not z' = not ecx */
    add eax , ebx           ;/* dst += x */
    lea edx,DWORD PTR 0bd3af235h [ edx * 1 +r10d ]      ;/* Const + dst + ... */
    or  r11d , eax           ;/* x | ... */
    xor r11d , ebx           ;/* y ^ ... */
    add edx , r11d         ;/* dst += ... */
    mov r10d , DWORD PTR (2*4)[rsi]       ;/* (NEXT STEP) X[2] */
    mov r11d , r13d ; 0ffffffffh
    rol edx , 10           ;/* dst <<< s */
    xor r11d , ebx           ;/* (NEXT STEP) not z' = not ebx */
    add edx , eax           ;/* dst += x */
    lea ecx,DWORD PTR 02ad7d2bbh [ ecx * 1 +r10d ]      ;/* Const + dst + ... */
    or  r11d , edx           ;/* x | ... */
    xor r11d , eax           ;/* y ^ ... */
    add ecx , r11d         ;/* dst += ... */
    mov r10d , DWORD PTR (9*4)[rsi]       ;/* (NEXT STEP) X[9] */
    mov r11d , r13d ; 0ffffffffh
    rol ecx , 15           ;/* dst <<< s */
    xor r11d , eax           ;/* (NEXT STEP) not z' = not eax */
    add ecx , edx           ;/* dst += x */
    lea ebx,DWORD PTR 0eb86d391h [ ebx * 1 +r10d ]      ;/* Const + dst + ... */
    or  r11d , ecx           ;/* x | ... */
    xor r11d , edx           ;/* y ^ ... */
    add ebx , r11d         ;/* dst += ... */
    mov r10d , DWORD PTR (0*4)[rsi]       ;/* (NEXT STEP) X[0] */
    mov r11d , r13d ; 0ffffffffh
    rol ebx , 21           ;/* dst <<< s */
    xor r11d , edx           ;/* (NEXT STEP) not z' = not edx */
    add ebx , ecx           ;/* dst += x */
;   # add old values of A, B, C, D
    add eax,r8d
    add ebx,r9d
    add ecx,r14d
    add edx,r15d

;   # loop control
    add rsi,64      ;# ptr += 64
    cmp rsi,rdi     ;# cmp end with ptr
    jb  lab2                ;# jmp if ptr < end
;   # END of loop over 16-word blocks

lab1:   ;pop    rbp             ;# restore ctx
pop r12
    mov DWORD PTR 0[r12],eax            ;# ctx->A = A
    mov DWORD PTR 4[r12],ebx            ;# ctx->B = B
    mov DWORD PTR 8[r12],ecx            ;# ctx->C = C
    mov DWORD PTR 12[r12],edx           ;# ctx->D = D

    pop rdi
    pop rsi
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret
md5_block_asm_host_order ENDP
END

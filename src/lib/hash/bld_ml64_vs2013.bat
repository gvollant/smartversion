set ADDIT_PARAM_ML64=

ml64 %ADDIT_PARAM_ML64% /Flm5_win64  /c /Zi m5_win64.asm
ml64 %ADDIT_PARAM_ML64% /Flmd5-amd64  /c /Zi md5-amd64.asm
ml64 %ADDIT_PARAM_ML64% /Flmd5-ms-amd64-v2  /c /Zi md5-ms-amd64-v2.asm


ml64 %ADDIT_PARAM_ML64% /Flx86_64cpuid  /c /Zi x86_64cpuid.asm


ml64 %ADDIT_PARAM_ML64% /Flsha1-x86_64  /c /Zi sha1-x86_64.asm
ml64 %ADDIT_PARAM_ML64% /Flsha256-x86_64  /c /Zi sha256-x86_64.asm
ml64 %ADDIT_PARAM_ML64% /Flmd5-x86_64  /c /Zi md5-x86_64.asm

set ADDIT_PARAM_ML32=/safeseh
rem ml /coff /Zi /c %ADDIT_PARAM_ML32% /Fl m5_win32.lst m5_win32.asm  
ml /coff /Zi /c %ADDIT_PARAM_ML32% /Flmd5-586.lst  md5-586.asm 

ml /coff /Zi /c %ADDIT_PARAM_ML32% /Flsha1-386.lst sha1-386.asm
ml /coff /Zi /c %ADDIT_PARAM_ML32% /Flsha1-586.lst sha1-586.asm
ml /coff /Zi /c %ADDIT_PARAM_ML32% /Flx86cpuid.lst x86cpuid.asm

rem ml /coff /Zi /c %ADDIT_PARAM_ML32% /Flvs2010xp_rtm_compat.lst vs2010xp_rtm_compat.asm

rem ml /coff /Zi /c %ADDIT_PARAM_ML32% /Flvs2010_oldxp.lst vs2010_oldxp.asm
rem ml /coff /Zi /c %ADDIT_PARAM_ML32% /Flvs2012_oldxp.lst vs2012_oldxp.asm

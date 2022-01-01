# smartversion
Smartversion create archive with several versions of file or delta compression between versions

Written by Gilles Vollant 2002-2022

https://www.smartversion.com/

For building:
run ./external_rebuild.sh under Linux or Windows + WSL to generate the external folder

open smv.cbp under CodeBlocks for Linux
open smartversion\project\vstudio\smv_vs.sln under Visual Studio 2019 or 2022 under MS Windows

under linux, you can also run
make -f smv.cbp.mak linux32staticrelease -j8
make -f smv.cbp.mak linux64staticrelease -j8

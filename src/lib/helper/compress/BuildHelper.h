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



#ifndef BUILD_HELPHER_H_INCLUDED
#define BUILD_HELPHER_H_INCLUDED 1

#include "../../engine/patchstream/common/difbasic.h"

#define FLAG_NO_COMPUTE_MD5      0x0010
#define FLAG_NO_COMPUTE_SHA1     0x0020
#define FLAG_NO_COMPUTE_SHA256   0x0040


#define BUILDERR_ERR_SOURCE_FILE_ERROR     (-2)
#define BUILDERR_ERR_DEST_FILE_ERROR       (-3)
#define BUILDERR_ERR_RECOMPRESS            (-4)
#ifdef __cplusplus
extern "C" {
#endif

int SVFAPI BuildPatchFromTwoFile(const char*DfsToBuild,const char*FirstFilename,const char*SecondFileName);
int SVFAPI BuildPatchFromTwoFileEx(const char*DfsToBuild, const char*FirstFilename, const char*SecondFileName, char*errText, dfuLong32 dwErrTextSize);
int SVFAPI BuildPatchFromTwoFileEx2(const char*DfsToBuild, const char*FirstFilename, const char*SecondFileName, char*errText, dfuLong32 dwErrTextSize,
	signed long CompressRatio,signed long HashBits,signed long BlockSize);
int SVFAPI BuildPatchFromTwoFileEx3(const char*DfsToBuild, const char*FirstFilename, const char*SecondFileName, char*errText, dfuLong32 dwErrTextSize,
	signed long CompressRatio, signed long HashBits, signed long BlockSize, signed long supp_flags);

#if defined (COMPRESSIONPARAM_DEFINED)
int SVFAPI BuildPatchFromTwoFileExCprParam(const char*DfsToBuild, const char*FirstFilename, const char*SecondFileName, char*errText, dfuLong32 dwErrTextSize,
    COMPRESSIONPARAM* pCprParamToBuildFnc, signed long supp_flags);
#endif
int SVFAPI RecompressPatch(const char* DfsOrg,const char*DfsToBuild, char*errText, dfuLong32 dwErrTextSize,
	signed long CompressRatio, signed long HashBits, signed long BlockSize, signed long supp_flags);
#ifdef __cplusplus
}
#endif


#endif

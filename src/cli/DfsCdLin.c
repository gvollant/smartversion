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

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../misc/svfdll.h"

#include "DfsCdLin.h"

#include "../lib/engine/patchstream/common/ltoolsc.h"
#include "../lib/engine/svfile/compress/AddingTool.h"



#include "zlib.h"
#include "../lib/helper/decompress/ExtractHelper.h"


#ifndef SVF_EXTRACT_ONLY
#include "../lib/helper/compress/BuildHelper.h"
#endif

#include "../SvfVersion.h"


typedef struct
{
  dfwcharpc FileNameOnDisk;
  dfwcharpc FileNameOnArchive;
  ///dfuLong64 dfFileSizeUncompressed;
  BOOL      fTempFile;
  dfuLong32 dfPreviousVersion;
  BOOL      fIdenticalPreviousVersion;
  BOOL      fIgnore;
  BOOL      fForceDate;
  DFTAGLIST hAddTags; /* to be used later */
  DFSINFODATE dfsInfoDate;
  dfvoidp   pReserved;
}
FILEUSERITEM;
typedef FILEUSERITEM *PFILEUSERITEM;


long DFSCALLBACK fncCompareFileUserItem(const void *lpElem1, const void *lpElem2)
{
  const FILEUSERITEM *pfi1 = (const FILEUSERITEM *) lpElem1;
  const FILEUSERITEM *pfi2 = (const FILEUSERITEM *) lpElem2;
  return dfUnicodeStrcmpi(pfi1->FileNameOnArchive, pfi2->FileNameOnArchive);
}

BOOL DFSCALLBACK fncDestructorFileUserItem(const void *lpElem)
{
  FILEUSERITEM *pfi = (FILEUSERITEM *) lpElem;
  if (pfi->FileNameOnArchive != NULL)
    DfsFree((dfwcharp) pfi->FileNameOnArchive);
  if (pfi->FileNameOnDisk != NULL)
    DfsFree((dfwcharp) pfi->FileNameOnDisk);

  return TRUE;
}


typedef struct
{
  dfwcharpc FileNameOriginal;
  dfwcharpc FileNameRenamed;
}
FILERENAMEDITEM;
typedef FILERENAMEDITEM *PFILERENAMEDITEM;


long DFSCALLBACK fncCompareFileRenamed(const void *lpElem1, const void *lpElem2)
{
  const FILERENAMEDITEM *pri1 = (const FILERENAMEDITEM *) lpElem1;
  const FILERENAMEDITEM *pri2 = (const FILERENAMEDITEM *) lpElem2;

  return dfUnicodeStrcmpi(pri1->FileNameRenamed, pri2->FileNameRenamed);
}

BOOL DFSCALLBACK fncDestructorFileRenamed(const void *lpElem)
{
  const FILERENAMEDITEM *pri = (const FILERENAMEDITEM *) lpElem;
  if (pri->FileNameOriginal != NULL)
    DfsFree((dfwcharp) pri->FileNameOriginal);
  if (pri->FileNameRenamed != NULL)
    DfsFree((dfwcharp) pri->FileNameRenamed);

  return TRUE;
}



int dfCompareUnicodeArgumentWithAnsiString(dfwcharpc str1, const char*str2)
{
    return CompareUnicodeAndAnsiString(str1,str2,FALSE);
}

void SVFAPI help()
{
  printf("%s\n%s\n%s\n\n", FULL_STRING_SMARTVERSION_COPYRIGHT, FULL_STRING_SMARTVERSION_COPYRIGHT_2, FULL_STRING_SMARTVERSION_COPYRIGHT_3);
  printf(
         ""
         "Usage:\n"
         "smv l [SvfFile]\n"
         "   List content of a SvfFile\n"
         "   lv instead l display a all checksum in file\n"
         "   lc/lvc instead l/lv display and patch composition\n"
#ifndef SVF_EXTRACT_ONLY
         "smv cr [SvfFile] [List of file, directory or wilcard]\n"
         "   Create a new SvfFile, create the first directory as reference with List\n"
         "smv crz [SvfFile] [zipfile]\n"
         "   Create a new SvfFile, create the first directory as reference from zipfile\n"
         "smv cz [SvfFile] [List of file, directory or wilcard]\n"
         "   Create a new SvfFile, create the first directory as compressed with List\n"
         "smv czz [SvfFile] [List of file, directory or wilcard]\n"
         "   Create a new SvfFile, create the first directory as compressed from zipfile\n"
         "\n"
         "smv i [SvfFile] [-br base_directory] [List of file, directory or wilcard]\n"
         "   Add a set of file as new version\n"
         "\n"
         "smv iz SvfFile zipfile [-br base_directory] [List of file, directory or wilcard]\n"
         "   Add content of a zipfile as new version\n"
         "\n"
         "smv a [SvfFile] [-br base_directory] [List of file, dir or wilcard] -v #[-#2]\n"
         "   Add a set of file in existing version\n"
         "\n"
         "smv d [SvfFile] [-br base_directory] [List of file or wilcard] -v #[-#2]\n"
         "   Delete file into existing version, from version # to version #2\n"
#endif
         "\n"
         "smv x [SvfFile] [-br base_directory] [List of file or wilcard] [-v #] [-o]\n"
         "   Extract a version (number # version, or latest by default, -o overwrite all)\n"
         "\n"
         "smv xz SvfFile zipfile [-br base_directory] [List of file or wilcard] [-v #]\n"
         "   Extract a version to zip (# version, or latest by default)\n"
         "\n"
         "smv t [SvfFile] [-br base_directory] [-v #]\n"
         "   Test the base directory,  (number # version, or first by default)\n"
         "\n"
#ifndef SVF_EXTRACT_ONLY
         "smv av [SvfFile] [SvfFileToBeAppend]\n"
         "   Append somes version from file SvfFileToBeAppend to file SvfFile. A version\n" \
         "   of SvfFileToBeAppend must match extactly the latest version of SvfFile\n"
         "\n"
#endif
         "for smv i,a,d,x,t, [-br base_directory] needed if the first version is reference\n"\
         "   SvfFile created using cz contain the first version compressed (like .zip)\n"
         "   SvfFile created using cr contain only name, size and checksum of file of the\n"
         "     first version. So when you use a SvfFile created with cr, you need specify\n"
         "     a directory which contain files referenced in first version with -br\n"
         "\n" \
         "\n" \
         "Options for smv i,a before [List of file, dir, wilcard]:\n" \
         "    [-onlyexistlatest] (or -oel) use only file which exist on latest version\n" \
         "    [-onlyexistversion #] (or -oev #) use only file which exist on version #\n" \
         "\n" \
         "Options for smv cr,cz,i,a before [List of file, dir, wilcard]:\n" \
         "    [-r]   to recurse into directories\n" \
         "    [-n #] where # is the version name\n" \
         "    [-c #] where # is the version comment\n" \
         "    [-nf #, -cf #, -nfu #, -cfu #] for reading name/comment from file #\n" \
         "    [-nf #, -cf #] for ansi text file, [-nfu #, -cfu #] for unicode file\n" \
         "\n"\
         "Options for smv m,cr,cz,i,a before [List of file, dir, wilcard]:\n" \
         "    [-compressratio #] : select the block compress ratio\n" \
         "      if # is between 1 (faster) and 9 (smaller) : uses zlib engine\n" \
         "      if # is between 110 (faster) and 119 (smaller) : uses lz4 engine with fastlzlib\n" \
         "      if # is between 171 (faster) and 192 (smaller) : uses zstd engine\n" \
         "      if # is between 531 (faster) and 552 (smaller) : uses zstd engine with long\n"
         "                          distance (smaller, need more memory)\n" \
         "      if # is between 501 (faster) and 522 (smaller) : uses zstd engine with very long\n"
         "                          distance (need smartversion 3.8.2 to decompress)\n" \
         "      if # is between 171 (faster) and 192 (smaller) : uses zstd engine\n" \
/* lzham support is alive but deprecated, no documented now */
/*
         "      if # is between 251 (faster) and 259 (smaller again) : uses lzham engine\n" \
*/
         "      if # is between 41 (faster) and 49 (smallest) : uses lzma engine\n" \
         "      if # is between 51 (faster) and 59  : uses mix zlib+lzma engine\n" \
         "      if # is between 308 (faster) and 332 (smallest) : use lz4frame with 64 Kb blocks\n" \
         "      if # is between 338 (faster) and 362 (smallest) : use lz4frame with 1 Mb blocks\n" \
         "      if # is between 368 (faster) and 392 (smallest) : use lz4frame with 4 Mb blocks\n" \
         "      if # is between 408 and 492 : lz4 like minus 100, buth with faster decompression\n" \
         "          svf file with lzma are not compatible with smartversion 1 or 2\n"
         "          svf file with zstd or lzham are not compatible with smartversion 3.6 and older\n"
         "          svf file with lz4framce are not compatible with smartversion 3.7 and older\n"
         "          from faster to smaller : lz4, zstd, lzham, lzma\n"
         "" \
         "    [-v1compatibility] (or -v1compat) for compatibile file with SmartVersion 1\n" \
         "    [-blocksize #] : select the internal blocksize (8 to 8192, power of 2)\n" \
         "    [-minalign #] : select the minimal alignement of data. Default 1\n" \
         "    [-nbhashbits #] : select the internal number of hash bits\n" \
         "    [-sha1] : for cz,czz, cr or crz : compute sha1 checksum\n" \
         "\n"\
         "Options for smv x,t,m,cr,cz,i,a before [List of file, dir, wilcard]:\n" \
         "    [-tmpdir #] : select the directory used to write temporary file\n" \
         "    [-memtmpsize #] : select the maximum memory size (KB) for temporary data\n" \
         "\n" \
         "Alternative to [List of file, dir, wilcard]\n" \
         "    [-lf #, -lfu #] read file list from text file\n" \
         "\n" \


         "Option for extracing (smv x):\n" \
         " -extractbystep : classic extracting, write version after version. (default)\n" \
         " -inplace : modify existing file(can be dangerous : if computer crashes during modification, file is lost)\n" \
         " -inplacenochecksum : like - inplace, but faster : do not verify checksum of file. Only if you are sure!\n" \
         " -bymerging : merge successive patch in memory before apply them\n" \
         "the -inplace extracting is fast when lvc list show a very high proportion of same position data\n" \
         "the -bymerging extracting is fast when your svf contain small successives patch against big files\n" \

         "\n" \

         "smv m [OriginalSvfFile] [SvfFileToBeRebuild] [options...]\n" \
         "   Rebuild/Recompress svf file to create a new svf file.\n" \
         "" \
         "Possible options:\n" \
         "   [-firstinsert] : store the first version as compressed content\n" \
         "   [-firstref] : store the first version as checksum reference\n" \
         "   [-br base_directory]: location of content of first version (if needed)\n" \
         "   [-compressratio #] : select the block compress ratio (see above)\n" \
         "   [-nbhashbits #] : select the internal number of hash bits\n" \
         "   [-blocksize #] : select the internal blocksize (8 to 8192, power of 2)\n" \
         "   [-sizestreamkb #] : size of atomic compression stream in KB. Default 16\n" \
         "     Bigger value need more memory to compress and decompress, but can be smaller and faster\n" \
         "   [-minalign #] : select the minimal alignement of data. Default 1\n" \
         "   [-recompress] : recompress patch\n" \
         "   [-rebuildpatch] : rebuild patch (then recompress them)\n" \
         "   [-nomd5] : do not store md5 checksum (enabled by default)\n" \
         "   [-sha1] : store sha1 checksum\n" \
         "   [-sha256] : store sha256 checksum\n" \
         "   [-all] : include all version (default)\n" \
         "   [#] : include version number #\n" \
         "     If's often better select also -recompress if you select -rebuildpatch\n"
         "\n"

#ifndef SVF_EXTRACT_ONLY
         "smv BuildPatch [SvfFile] [OldFile] [NewFile] [options...]\n" \
         "   A one command to create patch from one file to another\n" \
         "Possible options:\n"
         "  -blocksize #,-minalign #,-compressratio #,-nbhashbits #,-nomd5,-sha1,-sha256\n"
         "\n"
         "     -blocksize # specify the size of block search in original file.\n"
         "     -minalign # specify the search alignement.\n"
         " by example, to create quickly a patch between two virtual machine with 2048 bytes/cluster\n"
         "  -blocksize 2048 -minalign 512\n"
         /*
         "DifStrm c [SvfFile] [-br base_directory] [-v #]\n"
         "   Compare version with the base directory,  (number # version, or first by default)\n"
         "\n"
         */
         "\n"
#endif
         "Example:\n"
#ifndef SVF_EXTRACT_ONLY
         "smv cz archives.svf -r c:\\directory\\*\n" \
         "   create a new archives with files from c:\\directory recursely\n"\
         "smv i archives.svf -r c:\\directory\\*\n" \
         "   add new version on archives.svf with files from c:\\directory recursely\n"\
         "smv a archives.svf -r c:\\directory\\file*.txt -v 1-3\n" \
         "   add file*.txt on existing version from 1 to 3\n"\
         "smv d archives.svf file*.txt -v 1-3\n" \
         "   delete file*.txt on existing version from 1 to 3\n"\
         "smv d archives.svf * -v 2\n" \
         "   fully delete existing version 2\n"
#endif
         "smv x archives.svf *.doc -v 3 -o\n" \
         "   extract *.doc file from archives.svf version 3, overwrite all existing file\n"
#ifndef SVF_EXTRACT_ONLY
         "smv i archives.svf -lf listfile.txt -nf namever.txt -cf commentv.txt\n" \
         "   create new version on archives.svf, by reading list list of files from\n" \
         "   listfile.txt, get name of version on namever.txt and comment on commentv.txt\n"
#endif
         "smv | more\n" \
         "   display help on several screen\n"\
         "\n"\
         ""
         "");
}


BOOL TestWildcardAgaintFileInArchive(dfwcharpc dfFileNameToTest,dfwcharpc dfWilcard)
{
    dfuLong32 dfLenFileNameToTest;
    dfuLong32 dfLenWilcard;
    dfuLong32 i;
    dfuLong32 dfSkipFileNameToTest=0;
    if (dfUnicodeStrcmpi(dfFileNameToTest, dfWilcard)==0)
        return TRUE;
    dfLenFileNameToTest=dfUnicodeStrlen(dfFileNameToTest);
    dfLenWilcard=dfUnicodeStrlen(dfWilcard);

    for (i=0;((i+dfSkipFileNameToTest<dfLenFileNameToTest) && (i<dfLenWilcard));i++)
    {
        dfwchar cFileName = *(dfFileNameToTest+dfSkipFileNameToTest+i);
        dfwchar cWilcard = *(dfWilcard+i);
        dfwchar szWilcardFileName[2];
        dfwchar szWilcardCharText[2];

        if ((CompareUnicodeWithSimpleChar(cWilcard,'*')) && ((*(dfWilcard+i+1))==0))
            return TRUE;

        if (CompareUnicodeWithSimpleChar(cWilcard,'*'))
        {
            if (dfLenFileNameToTest>=dfLenWilcard)
            {
                dfSkipFileNameToTest = dfLenFileNameToTest-dfLenWilcard;
                i++;
                cFileName = *(dfFileNameToTest+dfSkipFileNameToTest+i);
                cWilcard = *(dfWilcard+i);
            }
            else return FALSE;
        }

        szWilcardFileName[1] = szWilcardCharText[1] = 0;
        szWilcardFileName[0] = cFileName;
        szWilcardCharText[0] = cWilcard;
        if ((dfUnicodeStrcmpi(szWilcardFileName,szWilcardCharText)!=0) && (!CompareUnicodeWithSimpleChar(cWilcard,'?')))
            return FALSE;
    }

    return ((dfLenWilcard+dfSkipFileNameToTest)==dfLenFileNameToTest);
}



const char* GetStringDirType(dfuLong32 dfTypeDir)
{
  switch (dfTypeDir)
    {
    case TYPEDIR_FILECRCONLY:
      return ("Reference and Checksum of files");
    case TYPEDIR_FILEINSERTING_STORE:
      return ("Files stored uncompressed");
    case TYPEDIR_FILEINSERTING_DEFLATE:
      return ("Files stored compressed");
    case TYPEDIR_PATCHFROMPREVIOUS:
      return ("Patch from previous version");
    }
  return ("Unknown type");
}


BOOL PrintDfsListArraySelect(dfwcharpc FileName,dfuLong32 dfVersionSpecified,dfuLong32 dfLastVersionSpecified,STATIC_ARRAY_C psa,BOOL fVerbose,BOOL fComposition)
{
  dfuLong32 dfError = DFS_SUCCESS;
  //dfuLong32 dfBlockType = 0;
  dfuLong32 dfNbDir = 0;
  dfuLong32 dfNumDir;
  dfuLong32 dfFirstVersion,dfFirstVersionLoop;
  DFSFILE DfsFile = NULL;
  DFSFILEINFOPARAM DfsFileParam;
  PDIRINFO* pDirInfo = NULL;

  DfsFileParam.sizeStruct = sizeof(DfsFileParam);
  DfsFileParam.dfStatus = DFS_READABLE;
  DfsFileParam.filename = FileName;
  if (DfsFileOpen(&DfsFileParam, &DfsFile,NULL) != DFS_SUCCESS)
    {
      printf("Error in opening ");
      DispOutUnicodeString(FileName);
      printf("\n");
      return FALSE;
    }
  ConvertOldDirectoryCommentStorage(DfsFile,NULL);
  DfsGetNbDir(DfsFile, &dfNbDir,NULL);

  if ((dfLastVersionSpecified == VALUE_UNKNOWN) && (dfVersionSpecified == VALUE_UNKNOWN))
      dfLastVersionSpecified = dfNbDir ;

  if (dfVersionSpecified == VALUE_UNKNOWN)
      dfFirstVersion = 0;
  else
      dfFirstVersion = dfVersionSpecified;

  if (dfLastVersionSpecified == VALUE_UNKNOWN)
      dfLastVersionSpecified = dfFirstVersion+1 ;
  else
      dfLastVersionSpecified ++ ;

  if (dfFirstVersion >= dfNbDir)
      dfFirstVersion = dfNbDir;
  if (dfLastVersionSpecified > dfNbDir)
      dfLastVersionSpecified = dfNbDir;
  pDirInfo = ReadAllDirInfo(DfsFile,&dfNbDir,dfLastVersionSpecified,NULL);
/*
  if (dfFirstVersion>0)
  {
      dfError = ReadDirectoryInfo(DfsFile, dfFirstVersion-1, &pDirInfoPrev, NULL, NULL, NULL);
  }*/

  dfFirstVersionLoop = dfFirstVersion;
  if (dfFirstVersionLoop>0)
      dfFirstVersionLoop--;
  if (fVerbose)
      dfFirstVersionLoop = 0;


  for (dfNumDir = dfFirstVersionLoop; dfNumDir < dfLastVersionSpecified; dfNumDir++)
    {
      dfuLong32 i = 0;

      PDIRINFO pCurDirInfo = *(pDirInfo+dfNumDir);


/*
      dfError = ReadDirectoryInfo(DfsFile, dfNumDir, &pDirInfo, NULL, NULL, NULL);
      if (pDirInfoPrev!=NULL)
      {
          FixIndenticalDifferentSizeInReadDirectoryInfo(pDirInfoPrev,pDirInfo);
          FreeDirectoryInfo(&pDirInfoPrev,NULL);
      }
*/
      if ((pCurDirInfo != NULL) && (dfNumDir >= dfFirstVersion))
      {
          BOOL fShowSHA1=FALSE;
          BOOL fShowSHA256=FALSE;
          BOOL fShowMD5=FALSE;
          dfuLong32 dfGetNbFile = pCurDirInfo->dfNbFile;
          printf("\nVersion number %u, type %s", dfNumDir + 0,
                 GetStringDirType(pCurDirInfo->dfTypeDir));


            i=0;
            while ((i < dfGetNbFile) && (dfError == DFS_SUCCESS))
            {
                BOOL fShowThis=TRUE;

                if (psa!=NULL)
                {
                    dfuLong32 k;
                    fShowThis=FALSE;
                    if (GetNbElemSA(psa) == 0)
                        fShowThis=TRUE;
                    else
                        for (k=0;k<GetNbElemSA(psa);k++)
                        {
                            const FILEUSERITEM *pfi = (const FILEUSERITEM *) GetElemPtrSA(psa,k);
                            if (TestWildcardAgaintFileInArchive((pCurDirInfo->pFileInDirInfo + i)->FileName,
                                                                pfi->FileNameOnArchive))
                            {
                                fShowThis=TRUE;
                                break;
                            }
                        }
                }
                if (fShowThis)
                {
                    if ((pCurDirInfo->pFileInDirInfo + i)->fMd5Filled)
                        {
                            fShowMD5=TRUE;
                        }
                    if ((pCurDirInfo->pFileInDirInfo + i)->fSha1Filled)
                        {
                            fShowSHA1=TRUE;
                        }
                    if ((pCurDirInfo->pFileInDirInfo + i)->fSha256Filled)
                        {
                            fShowSHA256=TRUE;
                        }
                }
                i++;
            }



          {
            dfvoidp TagBuf;
            dfuLong32 TagSize;
            DFTAGBLOCKFLOAT TagBlockFloat;
            TagBlockFloat = GetDfsTagBlockFloat(DfsFile, NULL);
            if (TagBlockFloat != NULL)
            {
                if (GetTagBlockFloat(TagBlockFloat,dfNumDir,FLOATNUM_NOSPECIFIC,DFSTAG_DIR_NAME_FLOAT,&TagBuf, &TagSize))
                {
                    printf(": ");
                    DispOutUnicodeString((dfwcharpc)TagBuf);
                }

                if (fVerbose)
                if (GetTagBlockFloat(TagBlockFloat,dfNumDir,FLOATNUM_NOSPECIFIC,DFSTAG_DIR_COMMENT_FLOAT,&TagBuf, &TagSize))
                {
                    printf(": ");
                    DispOutUnicodeString((dfwcharpc)TagBuf);
                }

            }
          }

          printf("\n");


          printf
            ("Name                                ");
          if (fVerbose)
              printf("                              ");
          printf(                                "Date   Time       ");
          if (fVerbose)
              printf("  ");
          printf(                                                   "Size    ");
          if (fVerbose)
              printf("    ");
          printf(                                                            "Packed");
          if (fVerbose)
              printf("  ");
          printf(                                                                   "  CRC-32");
          if (fVerbose)
              printf("    Status ");
          if (fVerbose && fShowMD5)
              printf("               MD5               ");
          if (fVerbose && fShowSHA1)
              printf("                   SHA1                  ");
          if (fVerbose && fShowSHA256)
              printf("                              SHA256                       ");
          /*
          if (fSha1)
              printf("                 SHA1");*/
          printf( "\n");
          if (fVerbose)
              printf("------------------------------");
          printf("--------------------------------- -------- -----  --------");
          if (fVerbose)
              printf("----");
          printf(                                                           "  --------");
          if (fVerbose)
              printf("----");
          printf(                                                                      " --------");
          if (fVerbose)
               printf(" ---------");
          if (fVerbose && fShowMD5)
               printf(" --------------------------------");
          if (fVerbose && fShowSHA1)
               printf(" ----------------------------------------");
          if (fVerbose && fShowSHA256)
              printf(" ----------------------------------------------------------------");
          printf("\n");
          i=0;
          while ((i < dfGetNbFile) && (dfError == DFS_SUCCESS))
          {
            BOOL fShowThis=TRUE;

            if (psa!=NULL)
            {
                dfuLong32 k;
                fShowThis=FALSE;
                if (GetNbElemSA(psa) == 0)
                    fShowThis=TRUE;
                else
                    for (k=0;k<GetNbElemSA(psa);k++)
                    {
                        const FILEUSERITEM *pfi = (const FILEUSERITEM *) GetElemPtrSA(psa,k);
                        if (TestWildcardAgaintFileInArchive((pCurDirInfo->pFileInDirInfo + i)->FileName,
                                                            pfi->FileNameOnArchive))
                        {
                            fShowThis=TRUE;
                            break;
                        }
                    }
            }
            if (fShowThis)
            {
              dfvoidp TagBuf;
              dfuLong32 TagSize;
              int iCountSpace;

              //printf(fVerbose ? "%-62ws  " : "%-32ws  ", (pCurDirInfo->pFileInDirInfo + i)->FileName);
              DispOutUnicodeString((pCurDirInfo->pFileInDirInfo + i)->FileName);
              for (iCountSpace=dfUnicodeStrlen((pCurDirInfo->pFileInDirInfo + i)->FileName);iCountSpace<(fVerbose ? 62:32);iCountSpace++)
                  printf(" ");
              printf("  ");

              if ((GetTag
                  (*(pCurDirInfo->TagFile + i), DFSTAG_DATE, &TagBuf, &TagSize)) && (TagSize>=sizeof(DFSINFODATE)))
                {
                  DFSTM dfsTm;
                  ConvertDfsInfoDateToDfsTm((DFSINFODATE *) TagBuf, &dfsTm);
                  printf("%02u/%02u/%02u %02u:%02u ", dfsTm.df_mday,
                         dfsTm.df_mon, dfsTm.df_year % 100, dfsTm.df_hour,
                         dfsTm.df_min/*, dfsTm.df_sec*/);
                }
                /*
              printf("%9u %9u ", (pCurDirInfo->pFileInDirInfo + i)->dfSize,
                     (pCurDirInfo->pFileInDirInfo + i)->dfFileEncodedSize);
                     */
              {
                  char szNumSize[32];
                  char szNumEncodedSize[32];
                  ConvertNum64ToUnformattedStrAnsi((pCurDirInfo->pFileInDirInfo + i)->dfSize,szNumSize,32,fVerbose ? 13 :9);
                  ConvertNum64ToUnformattedStrAnsi((pCurDirInfo->pFileInDirInfo + i)->dfFileEncodedSize,szNumEncodedSize,32,fVerbose ? 13 :9);
                  printf("%s %s ",szNumSize,szNumEncodedSize);
              }

              if ((pCurDirInfo->pFileInDirInfo + i)->fCrc32Filled)
                printf("%08lx", (unsigned long)((pCurDirInfo->pFileInDirInfo + i)->dfCrc32));

              if (fVerbose)
              {
                  const char* lpszTxt = NULL;
                  size_t iln;
                  printf(" ");
                  if (GetTag(*(pCurDirInfo->TagFile + i), DFSTAG_STORAGESTATUS, &TagBuf, &TagSize))
                      if (TagSize == sizeof(dfuLong32Intel))
                      {
                          dfuLong32 dfFileIdentical = ConvertuLongIntelToLong(*(dfuLong32Intel*)TagBuf);
                          //dfuLong32 uiRes=0;
                          switch (dfFileIdentical)
                          {
                          case DFS_STORAGESTATUS_IDENTICAL:
                              lpszTxt = "Identical";
                              break;

                          case DFS_STORAGESTATUS_MODIFIED:
                              lpszTxt = "Modified";
                              break;

                          case DFS_STORAGESTATUS_NEW:
                          case DFS_STORAGESTATUS_NEWSTORED:
                              lpszTxt = "New";
                              break;

                          case DFS_STORAGESTATUS_REFERENCE:
                              lpszTxt = "Reference";
                              break;
                          }
                      }
                  iln = 0;
                  if (lpszTxt != NULL)
                  {
                      iln = strlen(lpszTxt);
                      printf("%s", lpszTxt);
                  }
                  while (iln < 9)
                  {
                      iln++;
                      printf(" ");
                  }

                  if (fShowMD5)
                  {
                      if ((pCurDirInfo->pFileInDirInfo + i)->fMd5Filled)
                      {
                          int k;
                          printf(" ");
                          for (k = 0; k < 16; k++)
                              printf("%02x", (pCurDirInfo->pFileInDirInfo + i)->bMd5[k]);
                      }
                      else if (fShowSHA1 || fShowSHA256)
                      {
                          int k;
                          for (k = 0; k < 33; k++)
                              printf(" ");
                      }
                  }

                  if (fShowSHA1)
                  {
                      if ((pCurDirInfo->pFileInDirInfo + i)->fSha1Filled)
                      {
                          int k;
                          printf(" ");
                          for (k = 0; k < 20; k++)
                              printf("%02x", (pCurDirInfo->pFileInDirInfo + i)->bSha1[k]);
                      }
                      else if (fShowSHA256)
                      {
                          int k;
                          for (k = 0; k < 41; k++)
                              printf(" ");
                      }
                  }
                  if (fShowSHA256)
                      if ((pCurDirInfo->pFileInDirInfo + i)->fSha256Filled)
                      {
                          int k;
                          printf(" ");
                          for (k = 0; k < 32; k++)
                              printf("%02x", (pCurDirInfo->pFileInDirInfo + i)->bSha256[k]);
                      }

              }

                  if (fComposition &&
                      (((pCurDirInfo->pFileInDirInfo + i)->dfSize) > 0) &&
                      GetTag(*(pCurDirInfo->TagFile + i), DFSTAG_STORAGEPATCHINFO, &TagBuf, &TagSize))
                      if (TagSize == sizeof(DFSSTORAGEPATCHINFOINTEL))
                      {
                          const DFSSTORAGEPATCHINFOINTEL* pDfsStoragePatchInfoIntel = (const DFSSTORAGEPATCHINFOINTEL*)TagBuf;
                          dfuLong64 dfFileSize = (pCurDirInfo->pFileInDirInfo + i)->dfSize;
                          dfuLong64 dfSizeDeplInPlace = ConvertuLongIntelToLong64(pDfsStoragePatchInfoIntel->dfSizeDeplInPlaceIntel);
                          dfuLong64 dfSizeDeplOutPlace = ConvertuLongIntelToLong64(pDfsStoragePatchInfoIntel->dfSizeDeplOutPlaceIntel);
                          dfuLong64 dfSizeInserted = (dfFileSize - (dfSizeDeplInPlace + dfSizeDeplOutPlace));
                          char szSizeDeplInPlace[32];
                          char szSizeDeplOutPlace[32];
                          char szSizeInserted[32];

                          ConvertNum64ToUnformattedStrAnsi(dfSizeDeplInPlace, szSizeDeplInPlace, 32, 1);
                          ConvertNum64ToUnformattedStrAnsi(dfSizeDeplOutPlace, szSizeDeplOutPlace, 32, 1);
                          ConvertNum64ToUnformattedStrAnsi(dfSizeInserted, szSizeInserted, 32, 1);

                          printf("\n   (copied at same position = %s (%d %%), copied at different position = %s (%d %%), new data = %s (%d %%))",
                              szSizeDeplInPlace, (int)((dfSizeDeplInPlace * 100) / dfFileSize),
                              szSizeDeplOutPlace, (int)((dfSizeDeplOutPlace * 100) / dfFileSize),
                              szSizeInserted, (int)((dfSizeInserted * 100) / dfFileSize));
                      }

              printf("\n");
            }
            i++;
          }
      }
    }
  FreeAllDirInfo(pDirInfo,dfNbDir);
  DfsClose(DfsFile,NULL);
  return dfError;
}

BOOL SVFAPI PrintDfsList(dfwcharpc FileName,dfuLong32 dfVersionSpecified,dfuLong32 dfLastVersionSpecified)
{
    return PrintDfsListArraySelect(FileName,dfVersionSpecified,dfLastVersionSpecified,NULL,FALSE,FALSE);
}

dfwcharpc CopyStrWord(dfwcharpc lpSrc, dfwcharp lpDest, dfuLong32 dwMaxSize)
{
  BOOL fInQuote = FALSE;
  dfuLong32 dwDestPos = 0;

  *lpDest = '\0';

  if (lpSrc == NULL)
    return NULL;

  while (CompareUnicodeWithSimpleChar(*lpSrc,' '))
    lpSrc++;

  while (((*lpSrc) != '\0') && ((dwMaxSize == 0) || (dwDestPos < dwMaxSize)))
    {
      while (CompareUnicodeWithSimpleChar(*lpSrc,'"'))
        {
          fInQuote = !fInQuote;
          lpSrc++;
        }

      if ((CompareUnicodeWithSimpleChar(*lpSrc,' ')) && (!fInQuote))
        {
          while (CompareUnicodeWithSimpleChar(*lpSrc,' '))
            lpSrc++;
          return lpSrc;
        }

      *lpDest = *lpSrc;
      lpDest++;
      dwDestPos++;
      *lpDest = '\0';
      lpSrc++;
    }
  return lpSrc;
}

BOOL ShowError(H_ERROR_INFO hei,BOOL fShowIsNull)
{
    dfwcharp dfFileName=NULL;
    dfwcharp dfErrorMsg=NULL;
    dfuLong32 dfSizeFileName,dfSizeErrorMsg;

    if ((!fShowIsNull) && (hei==NULL))
        return FALSE;
    printf("error detected: %s\n", GetErrorExplanation(GetErrorNumber(hei)));

    GetErrorInfoItemByTag(hei,DFS_ERRORTAG_FILENAME,(dfbytep*)(&dfFileName),&dfSizeFileName);
    GetErrorInfoItemByTag(hei,DFS_ERRORTAG_ERRORMSG,(dfbytep*)(&dfErrorMsg),&dfSizeErrorMsg);

    if (dfFileName!=NULL)
    {
        printf("filename : ");
        DispOutUnicodeString(dfFileName);
        printf("\n");
    }


    if (dfErrorMsg!=NULL)
    {
        printf("message : ");
        DispOutUnicodeString(dfErrorMsg);
        printf("\n");
    }

    return TRUE;
}


BOOL AppendExtSvfIfNeeded(dfwcharp lpszDfsFileName)
{
    dfuLong32 i;
    dfuLong32 dfLen = dfUnicodeStrlen(lpszDfsFileName);
    for (i=0;i<dfLen;i++)
    {
        //if (*(lpszDfsFileName+i)=='.')
        if (CompareUnicodeWithSimpleChar(*(lpszDfsFileName+i),'.'))
            break;
    }
    if (i==dfLen)
    {
        DfUnicodeStrcpy(lpszDfsFileName+i,GetUnicodeStringDot());
        DfUnicodeStrcpy(lpszDfsFileName+i+1,GetUnicodeLowerCaseSvf());
        /*
        *(lpszDfsFileName+i)='.';
        *(lpszDfsFileName+i+1)='s';
        *(lpszDfsFileName+i+2)='v';
        *(lpszDfsFileName+i+3)='f';
        *(lpszDfsFileName+i+4)=0;*/
        return TRUE;
    }
    else
        return FALSE;
}

// duplicated in BuildHelper.cpp and DfsCdLin.c
static dfuLong32 GetPhysSizeKbUsable()
{
  dfuLong32 sizeReported = GetPhysicalMemoryKb();
  dfuLong32 totalLess2GB = (sizeReported > (2048*1024)) ? (GetPhysicalMemoryKb() - (2048*1024)) : 0;
  dfuLong32 totalDivide4 = sizeReported / 4;
  dfuLong32 sizeSelected = (totalLess2GB > totalDivide4) ? totalLess2GB : totalDivide4;
  return sizeSelected;
}

#ifndef SVF_EXTRACT_ONLY
BOOL SVFAPI MixDfs(dfwcharpc lpszDfsFileName, dfwcharpc pCommandLine)
{
    dfuLong32 dfError = DFS_SUCCESS;
    //dfuLong32 dfBlockType = 0;
    dfuLong32 dfNbDir = 0;
    dfuLong32 dfNumDir;
    dfwchar szDestDfs[MAX_PATH_LENGTH+1024];
    dfwchar szDfsBaseDirectory[MAX_PATH_LENGTH+1024];
    BOOL fBaseDirectorySelected = FALSE;
    DFSFILE DfsFile = NULL;
    DFSFILE DfsFileOut = NULL;
    DFSFILEINFOPARAM DfsFileParam;
    PDIRINFO *pDirInfo;
    BOOL *lpVersionMap = NULL;
    BOOL fReuseOldPatch = TRUE;
    BOOL fRawAccepted = TRUE;
    BOOL fFirstVersionAsReferenceNewDfs = FALSE;
    BOOL fFirstVersionStatusUserSelected = FALSE;
    BOOL fStripIdentical = TRUE;
    BOOL fSha1=FALSE;
    BOOL fSha256=FALSE;
    BOOL fMd5=TRUE;

    BOOL fRet = TRUE;
    H_ERROR_INFO hei=NULL;
    COMPRESSIONPARAM cprParam;

#ifndef SVF_EXTRACT_ONLY
    InitDefaultCompressionParam(&cprParam);
#endif
    cprParam.dfPhysicalMemoryKB = GetPhysSizeKbUsable();

    szDestDfs[0]=szDfsBaseDirectory[0]=0;


//    printf("-org: %ws \n-dest: %ws \n-command: %ws \n", szDfsFileName,szDestDfs,pCommandLine);

    DfsFileParam.sizeStruct = sizeof(DfsFileParam);
    DfsFileParam.dfStatus = DFS_READABLE;

    if (lpszDfsFileName != NULL)
    {
        DfsFileParam.filename = lpszDfsFileName;
        if (DfsFileOpen(&DfsFileParam, &DfsFile, NULL) != DFS_SUCCESS)
        {
            printf("Error in opening ");
            DispOutUnicodeString(lpszDfsFileName);
            printf("\n");
            return FALSE;
        }
    }
    else
    {
        dfwchar szDfsFileName[MAX_PATH_LENGTH+1024];
        pCommandLine = CopyStrWord(pCommandLine, szDfsFileName, (sizeof(szDfsFileName) / sizeof(dfwchar)) - 1);

        DfsFileParam.filename = szDfsFileName;
        if (DfsFileOpen(&DfsFileParam, &DfsFile, NULL) != DFS_SUCCESS)
        {
            dfuLong32 dfLen = dfUnicodeStrlen(szDfsFileName);
            if (dfLen < (sizeof(szDfsFileName) / sizeof(dfwchar)) - 5)
                AppendExtSvfIfNeeded(szDfsFileName);

            if (DfsFileOpen(&DfsFileParam, &DfsFile, NULL) != DFS_SUCCESS)
            {
                printf("Error in opening ");
                DispOutUnicodeString(lpszDfsFileName);
                printf("\n");
                return FALSE;
            }
        }
    }

    pCommandLine = CopyStrWord(pCommandLine, szDestDfs, (sizeof(szDestDfs) / sizeof(dfwchar)) - 1);
    {
        dfuLong32 dfLen = dfUnicodeStrlen(szDestDfs);
        if (dfLen < (sizeof(szDestDfs) / sizeof(dfwchar)) - 5)
            AppendExtSvfIfNeeded(szDestDfs);
    }
    ConvertOldDirectoryCommentStorage(DfsFile, NULL);
    DfsGetNbDir(DfsFile, &dfNbDir, NULL);

    lpVersionMap = (BOOL *) DfsMalloc(sizeof(BOOL) * (dfNbDir + 1));


    for (dfNumDir = 0; dfNumDir < dfNbDir; dfNumDir++)
        *(lpVersionMap + dfNumDir)=FALSE;

    while (pCommandLine!=NULL)
    {
        dfwchar szPortionLine[MAX_PATH_LENGTH+1024];

        pCommandLine = CopyStrWord(pCommandLine, szPortionLine, (sizeof(szPortionLine) / sizeof(dfwchar)) - 1);
        if (szPortionLine[0]==0)
            break;


        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/firstinsert") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-firstinsert") == 0)))
        {
            fFirstVersionAsReferenceNewDfs = FALSE;
            fFirstVersionStatusUserSelected = TRUE;
        }
        else
        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/firstref") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-firstref") == 0)))
        {
            fFirstVersionAsReferenceNewDfs = TRUE;
            fFirstVersionStatusUserSelected = TRUE;
        }
        else
        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/br") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-br") == 0)))
        {
            pCommandLine =
                CopyStrWord(pCommandLine, szDfsBaseDirectory,
                            (sizeof(szDfsBaseDirectory) / sizeof(dfwchar)) - 1);
            fBaseDirectorySelected = TRUE;
        }
        else
        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/blocksize") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-blocksize") == 0)))
        {
            dfwchar szBlockSize[MAX_PATH_LENGTH];
            unsigned long lValue;

            szBlockSize[0]=0;
            pCommandLine =
                CopyStrWord(pCommandLine, szBlockSize,
                            (sizeof(szBlockSize) / sizeof(dfwchar)) - 2);

            lValue=ConvertUnicodeStringToLong(szBlockSize);
            cprParam.dfBlockCalcSizeSearch=lValue;
        }
        else
        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/maxalignedsearch") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-maxalignedsearch") == 0)))
        {
            dfwchar szTextNumber[MAX_PATH_LENGTH];
            unsigned long lValue;

            szTextNumber[0]=0;
            pCommandLine =
                CopyStrWord(pCommandLine, szTextNumber,
                            (sizeof(szTextNumber) / sizeof(dfwchar)) - 2);

            lValue=ConvertUnicodeStringToLong(szTextNumber);
            cprParam.dfMaxAlignedSearchNumber=lValue;
        }
        else
        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/minalign") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-minalign") == 0)))
        {
            dfwchar szTextNumber[MAX_PATH_LENGTH];
            unsigned long lValue;

            szTextNumber[0]=0;
            pCommandLine =
                CopyStrWord(pCommandLine, szTextNumber,
                            (sizeof(szTextNumber) / sizeof(dfwchar)) - 2);

            lValue=ConvertUnicodeStringToLong(szTextNumber);
            cprParam.dfMinimalSearchAlignement=lValue;
        }
        else
        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/compressratio") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-compressratio") == 0)))
        {
            dfwchar szValWChar[MAX_PATH_LENGTH];
            unsigned long lValue;

            szValWChar[0] = 0;
            pCommandLine =
                CopyStrWord(pCommandLine, szValWChar,
                (sizeof(szValWChar) / sizeof(dfwchar)) - 1);

            lValue = ConvertUnicodeStringToLong(szValWChar);
            cprParam.uZlibCompressRatio = (dfuLong32)lValue;
        }
        else
        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/sizestreamkb") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-sizestreamkb") == 0)))
            {
            dfwchar szValWChar[MAX_PATH_LENGTH];
            unsigned long lValue;

            szValWChar[0]=0;
            pCommandLine =
                CopyStrWord(pCommandLine, szValWChar,
                            (sizeof(szValWChar) / sizeof(dfwchar)) - 2);

            lValue=ConvertUnicodeStringToLong(szValWChar);
            cprParam.dfSizeButStreamKB=lValue;
            }
        else
        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/nbhashbits") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-nbhashbits") == 0)))
            {
            dfwchar szValWChar[MAX_PATH_LENGTH];
            unsigned long lValue;

            szValWChar[0]=0;
            pCommandLine =
                CopyStrWord(pCommandLine, szValWChar,
                            (sizeof(szValWChar) / sizeof(dfwchar)) - 2);

            lValue=ConvertUnicodeStringToLong(szValWChar);
            cprParam.dfNbHashBit=lValue;
            }
        else
        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/recpr") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-recpr") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/recompress") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-recompress") == 0)))
         {
             fRawAccepted = FALSE;
             printf("recompress chunk\n");
         }
         else
         if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/bldpatch") == 0)) ||
             ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-bldpatch") == 0)) ||
             ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/rebuildpatch") == 0)) ||
             ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-rebuildpatch") == 0)))
         {
            fReuseOldPatch = FALSE;
            printf("rebuild patch\n");
         }
         else
         if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/bld") == 0)) ||
             ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-bld") == 0)) ||
             ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/rebuild") == 0)) ||
             ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-rebuild") == 0)))
        {
            fReuseOldPatch = fRawAccepted = FALSE;
            printf("rebuild patch and recompress chunk\n");
        }
        else
        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/strip") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-strip") == 0)))
        {
            fStripIdentical=TRUE;
            //printf("strip identical files\n");
        }
        else
        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/nomd5") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-nomd5") == 0)))
        {
            fMd5=FALSE;
        }
        else
        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/sha1") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-sha1") == 0)))
        {
            fSha1=TRUE;
        }
        else
        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/sha256") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-sha256") == 0)))
        {
            fSha256=TRUE;
        }
        else
        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/v1compat") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-v1compat") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/v1compatibility") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-v1compatibility") == 0)))
        {
            fStripIdentical=FALSE;
            //printf("strip identical files\n");
        }
        else
        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/tempdir") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-tempdir") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/tmpdir") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-tmpdir") == 0)))
            {
            dfwchar szDfsTmp[1024];
            szDfsTmp[0]=0;
            pCommandLine =
                CopyStrWord(pCommandLine, szDfsTmp,
                            (sizeof(szDfsTmp) / sizeof(dfwchar)) - 1);
            printf("new dir tmp : '");
            DispOutUnicodeString(szDfsTmp);
            printf("'\n");
            if (szDfsTmp[0]!=0)
                SetTempDirectory(szDfsTmp);
            }
        else
        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/memtempsize") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-memtempsize") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/memtmpsize") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-memtmpsize") == 0)))
            {
                dfwchar szMemTmpSize[MAX_PATH_LENGTH];
                unsigned long lValue;

                szMemTmpSize[0]=0;
                pCommandLine =
                    CopyStrWord(pCommandLine, szMemTmpSize,
                                (sizeof(szMemTmpSize) / sizeof(dfwchar)) - 1);

                lValue=ConvertUnicodeStringToLong(szMemTmpSize);
                dfsSetVirtualFileNameMaximumMemory(TRUE,((dfuLong64)lValue));
            }
        else
        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/all") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-all") == 0)))
        {
            for (dfNumDir = 0; dfNumDir < dfNbDir; dfNumDir++)
                *(lpVersionMap + dfNumDir)=TRUE;
            printf("include all version\n");
        }
        else
        {
            unsigned long lVer;
            lVer=ConvertUnicodeStringToLong(szPortionLine);
            if (lVer<dfNbDir)
                *(lpVersionMap + lVer)=TRUE;
            printf("include version %lu\n",lVer);
        }
    }


    for (dfNumDir = 0; dfNumDir < dfNbDir; dfNumDir++)
        if (*(lpVersionMap + dfNumDir) == TRUE)
            break;
    if (dfNumDir == dfNbDir)
    {
        for (dfNumDir = 0; dfNumDir < dfNbDir; dfNumDir++)
            *(lpVersionMap + dfNumDir) = TRUE;
        printf("no version selected, include all version by default\n");
    }

    pDirInfo = ReadAllDirInfo(DfsFile,&dfNbDir,READ_ALL_DIR,NULL);

    if ((!fFirstVersionStatusUserSelected) && (dfNbDir>0))
        fFirstVersionAsReferenceNewDfs = ((*pDirInfo)->dfTypeDir == TYPEDIR_FILECRCONLY);

    AdaptDfsFileFeature(DfsFile,(const PCDIRINFO*)pDirInfo,dfNbDir);
    if (pDirInfo == NULL)
        fRet = FALSE;

    if (fRet)
    {
        VERSIONTOADD_REMIX *pVersionRemix;
        dfuLong32 dwNbMapVersionMap = dfNbDir;
        dfuLong32 i, j, dfNbVersionRemix;
        DFSFEATUREPARAM DfsFeatureParam;
        DfsFeatureParam.fComputeMd5 = fMd5;
        DfsFeatureParam.fComputeSha1 = fSha1;
        DfsFeatureParam.fComputeSha256 = fSha256;
        DfsFeatureParam.fStripIdenticalBody = fStripIdentical;

        pVersionRemix =
            (VERSIONTOADD_REMIX *) DfsMalloc(sizeof(VERSIONTOADD_REMIX) * (dwNbMapVersionMap + 1));
        dfNbVersionRemix = 0;
        for (i = 0; i < dwNbMapVersionMap; i++)
            if (*(lpVersionMap + i))
            {
                VERSIONTOADD_REMIX *pFileCopyInfoCur = pVersionRemix + dfNbVersionRemix;
                PDIRINFO pDirOrg = *(pDirInfo + i);
                pFileCopyInfoCur->dfNumVersionPreviousSvf = i;
                pFileCopyInfoCur->dfNbPreviousFileInMask = pDirOrg->dfNbFile;
                pFileCopyInfoCur->dfNbFileToAdd = 0;
                pFileCopyInfoCur->pFileCopyInfo = (FILETOCOPYINFO_REMIX *)
                    DfsMalloc(sizeof(FILETOCOPYINFO_REMIX) * (pDirOrg->dfNbFile + 1));
                pFileCopyInfoCur->pfta = NULL;
                for (j = 0; j < pDirOrg->dfNbFile; j++)
                {
                    FILETOCOPYINFO_REMIX *pftci = (pFileCopyInfoCur->pFileCopyInfo) + j;
                    pftci->dfReferenceItem = FTCI_REFERENCE_UNMODIFIED;
                    pftci->fIsReferenceInAddedFile = FALSE;
                }
                dfNbVersionRemix++;
            }


        dfError = DoReMixDfsEx(DfsFile, dfNbDir, (const PCDIRINFO*)pDirInfo, &DfsFileOut,
                                szDestDfs,
                                &DfsFeatureParam,

                                //fBaseDirectorySelected, dfBaseDirNum, wchBaseDirectory,
                                fBaseDirectorySelected, 0, fBaseDirectorySelected ? szDfsBaseDirectory : NULL, NULL,

                                dfNbVersionRemix, pVersionRemix,

                                //fFirstVersionAsReference, fReuseOldPatch,        // future
                                fFirstVersionAsReferenceNewDfs, fReuseOldPatch, fRawAccepted,

                                &cprParam,NULL,NULL,0,100,&hei);
                                //pCprParam, pSetExtractPosCallBack, dfUserPtr, dwMinProgress, dwMaxProgress, NULL);
        if (dfError != DFS_SUCCESS)
            fRet = FALSE;

        {
            dfuLong32 i;
            for (i = 0; i < dfNbVersionRemix; i++)
            {
                FILETOCOPYINFO_REMIX *pFileCopyInfo = (pVersionRemix + i)->pFileCopyInfo;
                DfsFree(pFileCopyInfo);
            }
        }
        DfsFree(pVersionRemix);
    }

    FreeAllDirInfo(pDirInfo,dfNbDir);
    pDirInfo = NULL;

    DfsFree(lpVersionMap);
    DfsClose(DfsFile,NULL);
    DfsClose(DfsFileOut,NULL);
    if (hei!=NULL)
    {
        ShowError(hei,TRUE);
    }
    FreeErrorInfoBlock(hei);

    return fRet;
}









BOOL SVFAPI BuildPatchCmdLine(dfwcharpc lpszDfsFileName, dfwcharpc pCommandLine)
{
    dfwchar szFirstFile[MAX_PATH_LENGTH + 1024];
    dfwchar szSecondFile[MAX_PATH_LENGTH + 1024];
    BOOL fStripIdentical = TRUE;
    /*
    dfuLong32 dfError = DFS_SUCCESS;
    BOOL fBaseDirectorySelected = FALSE;
    dfuLong32 dfBlockType = 0;
    dfuLong32 dfNbDir = 0;
    H_ERROR_INFO hei = NULL;
    */
    BOOL fSha1 = FALSE;
    BOOL fSha256 = FALSE;
    BOOL fMd5 = TRUE;

    BOOL fRet = TRUE;
    COMPRESSIONPARAM cprParam;
    int retValue;

    InitDefaultCompressionParam(&cprParam);

    cprParam.dfPhysicalMemoryKB = GetPhysSizeKbUsable();

    szFirstFile[0] = szSecondFile[0] = 0;


    pCommandLine = CopyStrWord(pCommandLine, szFirstFile, (sizeof(szFirstFile) / sizeof(dfwchar)) - 1);
    pCommandLine = CopyStrWord(pCommandLine, szSecondFile, (sizeof(szSecondFile) / sizeof(dfwchar)) - 1);

    if ((szFirstFile[0] == 0) || (szSecondFile[0] == 0) || (lpszDfsFileName[0] == 0))
    {
        printf("Error in command line\n");
        return FALSE;
    }

    printf("Creating SmartVersion file ");
    DispOutUnicodeString(lpszDfsFileName);
    printf(" with patch from ");
    DispOutUnicodeString(szFirstFile);
    printf(" to ");
    DispOutUnicodeString(szSecondFile);
    printf("\n");

    while (pCommandLine != NULL)
    {
        dfwchar szPortionLine[MAX_PATH_LENGTH + 1024];

        if ((*pCommandLine) == 0)
            break;

        pCommandLine = CopyStrWord(pCommandLine, szPortionLine, (sizeof(szPortionLine) / sizeof(dfwchar)) - 1);

        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/blocksize") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-blocksize") == 0)))
        {
            dfwchar szBlockSize[MAX_PATH_LENGTH];
            unsigned long lValue;

            szBlockSize[0] = 0;
            pCommandLine =
                CopyStrWord(pCommandLine, szBlockSize,
                (sizeof(szBlockSize) / sizeof(dfwchar)) - 2);

            lValue = ConvertUnicodeStringToLong(szBlockSize);
            cprParam.dfBlockCalcSizeSearch = lValue;
        }
        else
        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/maxalignedsearch") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-maxalignedsearch") == 0)))
        {
            dfwchar szTextNumber[MAX_PATH_LENGTH];
            unsigned long lValue;

            szTextNumber[0]=0;
            pCommandLine =
                CopyStrWord(pCommandLine, szTextNumber,
                            (sizeof(szTextNumber) / sizeof(dfwchar)) - 2);

            lValue=ConvertUnicodeStringToLong(szTextNumber);
            cprParam.dfMaxAlignedSearchNumber=lValue;
        }
        else
        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/minalign") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-minalign") == 0)))
        {
            dfwchar szTextNumber[MAX_PATH_LENGTH];
            unsigned long lValue;

            szTextNumber[0]=0;
            pCommandLine =
                CopyStrWord(pCommandLine, szTextNumber,
                            (sizeof(szTextNumber) / sizeof(dfwchar)) - 2);

            lValue=ConvertUnicodeStringToLong(szTextNumber);
            cprParam.dfMinimalSearchAlignement=lValue;
        }
        else
        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/maxalignedsearch") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-maxalignedsearch") == 0)))
        {
            dfwchar szTextNumber[MAX_PATH_LENGTH];
            unsigned long lValue;

            szTextNumber[0]=0;
            pCommandLine =
                CopyStrWord(pCommandLine, szTextNumber,
                            (sizeof(szTextNumber) / sizeof(dfwchar)) - 2);

            lValue=ConvertUnicodeStringToLong(szTextNumber);
            cprParam.dfMaxAlignedSearchNumber=lValue;
        }
        else
        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/minalign") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-minalign") == 0)))
        {
            dfwchar szTextNumber[MAX_PATH_LENGTH];
            unsigned long lValue;

            szTextNumber[0]=0;
            pCommandLine =
                CopyStrWord(pCommandLine, szTextNumber,
                            (sizeof(szTextNumber) / sizeof(dfwchar)) - 2);

            lValue=ConvertUnicodeStringToLong(szTextNumber);
            cprParam.dfMinimalSearchAlignement=lValue;
        }
        else
            if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/compressratio") == 0)) ||
                ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-compressratio") == 0)))
            {
                dfwchar szValWChar[MAX_PATH_LENGTH];
                unsigned long lValue;

                szValWChar[0] = 0;
                pCommandLine =
                    CopyStrWord(pCommandLine, szValWChar,
                    (sizeof(szValWChar) / sizeof(dfwchar)) - 1);

                lValue = ConvertUnicodeStringToLong(szValWChar);
                cprParam.uZlibCompressRatio = (dfuLong32)lValue;
            }
            else
                if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/nbhashbits") == 0)) ||
                    ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-nbhashbits") == 0)))
                {
                    dfwchar szValWChar[MAX_PATH_LENGTH];
                    unsigned long lValue;

                    szValWChar[0] = 0;
                    pCommandLine =
                        CopyStrWord(pCommandLine, szValWChar,
                        (sizeof(szValWChar) / sizeof(dfwchar)) - 2);

                    lValue = ConvertUnicodeStringToLong(szValWChar);
                    cprParam.dfNbHashBit = lValue;
                }
                else
                    if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/nomd5") == 0)) ||
                        ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-nomd5") == 0)))
                    {
                        fMd5 = FALSE;
                    }
                    else
                        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/sha1") == 0)) ||
                            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-sha1") == 0)))
                        {
                            fSha1 = TRUE;
                        }
                    else
                        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/sha256") == 0)) ||
                            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-sha256") == 0)))
                        {
                            fSha256 = TRUE;
                        }
                        else
                            if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/v1compat") == 0)) ||
                                ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-v1compat") == 0)) ||
                                ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/v1compatibility") == 0)) ||
                                ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-v1compatibility") == 0)))
                            {
                                fStripIdentical = FALSE;
                                //printf("strip identical files\n");
                            }
                            else
                                if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/tempdir") == 0)) ||
                                    ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-tempdir") == 0)) ||
                                    ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/tmpdir") == 0)) ||
                                    ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-tmpdir") == 0)))
                                {
                                    dfwchar szDfsTmp[1024];
                                    szDfsTmp[0] = 0;
                                    pCommandLine =
                                        CopyStrWord(pCommandLine, szDfsTmp,
                                        (sizeof(szDfsTmp) / sizeof(dfwchar)) - 1);
                                    printf("new dir tmp : '");
                                    DispOutUnicodeString(szDfsTmp);
                                    printf("'\n");
                                    if (szDfsTmp[0] != 0)
                                        SetTempDirectory(szDfsTmp);
                                }
                                else
                                    if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/blocksize") == 0)) ||
                                        ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-blocksize") == 0)))
                                    {
                                        dfwchar szBlockSize[MAX_PATH_LENGTH];
                                        unsigned long lValue;

                                        szBlockSize[0] = 0;
                                        pCommandLine =
                                            CopyStrWord(pCommandLine, szBlockSize,
                                            (sizeof(szBlockSize) / sizeof(dfwchar)) - 1);

                                        lValue = ConvertUnicodeStringToLong(szBlockSize);
                                        cprParam.dfBlockCalcSizeSearch = (dfuLong32)lValue;
                                    }
                                else
                                      if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/maxalignedsearch") == 0)) ||
                                          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-maxalignedsearch") == 0)))
                                      {
                                        dfwchar szTextNumber[MAX_PATH_LENGTH];
                                        unsigned long lValue;

                                        szTextNumber[0] = 0;
                                        pCommandLine =
                                          CopyStrWord(pCommandLine, szTextNumber,
                                          (sizeof(szTextNumber) / sizeof(dfwchar)) - 2);

                                        lValue = ConvertUnicodeStringToLong(szTextNumber);
                                        cprParam.dfMaxAlignedSearchNumber = lValue;
                                      }
                                else
                                        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/minalign") == 0)) ||
                                            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-minalign") == 0)))
                                        {
                                          dfwchar szTextNumber[MAX_PATH_LENGTH];
                                          unsigned long lValue;

                                          szTextNumber[0] = 0;
                                          pCommandLine =
                                            CopyStrWord(pCommandLine, szTextNumber,
                                            (sizeof(szTextNumber) / sizeof(dfwchar)) - 2);

                                          lValue = ConvertUnicodeStringToLong(szTextNumber);
                                          cprParam.dfMinimalSearchAlignement = lValue;
                                        }
                                else
                                    if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/memtempsize") == 0)) ||
                                        ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-memtempsize") == 0)) ||
                                        ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/memtmpsize") == 0)) ||
                                        ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-memtmpsize") == 0)))
                                    {
                                        dfwchar szMemTmpSize[MAX_PATH_LENGTH];
                                        unsigned long lValue;

                                        szMemTmpSize[0] = 0;
                                        pCommandLine =
                                            CopyStrWord(pCommandLine, szMemTmpSize,
                                            (sizeof(szMemTmpSize) / sizeof(dfwchar)) - 1);

                                        lValue = ConvertUnicodeStringToLong(szMemTmpSize);
                                        dfsSetVirtualFileNameMaximumMemory(TRUE, ((dfuLong64)lValue));
                                    }

    }

    char szaDfsFileName[MAX_PATH_LENGTH + 1024];
    char szaFirstFile[MAX_PATH_LENGTH + 1024];
    char szaSecondFile[MAX_PATH_LENGTH + 1024];


    ConvertUnicodeToAnsi(szFirstFile, szaFirstFile, sizeof(szaFirstFile) - 1);
    ConvertUnicodeToAnsi(szSecondFile, szaSecondFile, sizeof(szaSecondFile) - 1);
    ConvertUnicodeToAnsi(lpszDfsFileName, szaDfsFileName, sizeof(szaDfsFileName) - 1);


    char errText[2048] = "";
    signed long supp_flags = 0;
    if (!fMd5)
        supp_flags |= FLAG_NO_COMPUTE_MD5;

    if (!fSha1)
        supp_flags |= FLAG_NO_COMPUTE_SHA1;

    if (!fSha256)
        supp_flags |= FLAG_NO_COMPUTE_SHA256;

    retValue = BuildPatchFromTwoFileExCprParam(szaDfsFileName, szaFirstFile, szaSecondFile, errText, sizeof(errText-1),
        &cprParam, supp_flags);


    if (retValue != 0)
    {
        printf("Error %d : %s\n", retValue, errText);
        fRet = FALSE;
    }

    return fRet;
}

#endif

/*
{
    pVersionRemix = (VERSIONTOADD_REMIX*)DfsMalloc(sizeof(VERSIONTOADD_REMIX) * (dwNbMapVersionMap + 1));
    dfNbVersionRemix = 0;
    for (i = 0; i < dwNbMapVersionMap; i++)
        if (*(lpVersionMap + i))
        {
            VERSIONTOADD_REMIX *pFileCopyInfoCur = pVersionRemix + dfNbVersionRemix;
            PDIRINFO pDirOrg = *(pDirInfo + i);
            pFileCopyInfoCur->dfNumVersionPreviousSvf = i;
            pFileCopyInfoCur->dfNbPreviousFileInMask = pDirOrg->dfNbFile;
            pFileCopyInfoCur->dfNbFileToAdd = 0;
            pFileCopyInfoCur->pFileCopyInfo = (FILETOCOPYINFO_REMIX*)
                DfsMalloc(sizeof(FILETOCOPYINFO_REMIX) * (pDirOrg->dfNbFile + 1));
            pFileCopyInfoCur->pfta = NULL;
            for (j = 0; j < pDirOrg->dfNbFile; j++)
            {
                FILETOCOPYINFO_REMIX *pftci = (pFileCopyInfoCur->pFileCopyInfo) + j;
                pftci->dfReferenceItem = FTCI_REFERENCE_UNMODIFIED;
                pftci->fIsReferenceInAddedFile = FALSE;
            }
            dfNbVersionRemix++;
        }
}

*/

typedef struct
{
    dfuLong32 dfSizeAllocatedInWChar;
    dfwcharp pBuf;
} EXPANDABLE_BUFFERED_STRING;

void FreeExpandableBufferedString(EXPANDABLE_BUFFERED_STRING* pEbs)
{
    if (pEbs != NULL)
    {
        if (pEbs->pBuf != NULL)
            DfsFree(pEbs->pBuf);
        pEbs->pBuf = NULL;
        pEbs->dfSizeAllocatedInWChar = 0;
    }
}

BOOL ExpandBufferedString(EXPANDABLE_BUFFERED_STRING* pEbs,dfuLong32 dfSize)
{
    dfwcharp pNewBuf;
    if (pEbs == NULL)
        return FALSE;
    if (pEbs->dfSizeAllocatedInWChar >= (dfSize*2)+4)
        return TRUE;

    if (pEbs->pBuf == NULL)
        pNewBuf = (dfwcharp)DfsMalloc((dfSize*4)+0x20);
    else
        pNewBuf = (dfwcharp)DfsRealloc(pEbs->pBuf,(dfSize*4)+0x20);
    if (pNewBuf == NULL)
        return FALSE;

    pEbs->pBuf = pNewBuf;
    pEbs->dfSizeAllocatedInWChar = (dfSize*2)+4;
    return TRUE;
}

BOOL StoreCanonicalFileName(dfwcharpc szFileName, EXPANDABLE_BUFFERED_STRING* pEbs)
{
    dfuLong32 dfSize;
    if (!(ExpandBufferedString(pEbs,MAX_PATH_LENGTH+4)))
        return FALSE;
    dfSize = MAX_PATH_LENGTH+2;

    if (!GetCanonicalFileName(szFileName,
                          pEbs->pBuf,pEbs->dfSizeAllocatedInWChar,
                          &dfSize))
    {
        if (!(ExpandBufferedString(pEbs,(dfSize*4)+0x20)))
            return FALSE;

        if (!GetCanonicalFileName(szFileName,
                            pEbs->pBuf,pEbs->dfSizeAllocatedInWChar,
                            &dfSize))
                            return FALSE;
    }
    return TRUE;
}

BOOL AddWilcardSortedStaticW(dfwcharpc szWildcard, STATIC_ARRAY_C psa,BOOL fRecurse,dfwcharpc szwPreviousDir,
                             dfwcharpc szCanonicalFileNameToNotAdd)
{
  HFILESEARCHING hfsi;
  //WIN32_FIND_DATAW FileData;
  BOOL fContinue=TRUE;
  BOOL fAddOk = TRUE;
  dfwchar szPrePath[1024];
  dfwcharp pszFileNameAfterPath;
  dfuLong32 dfPathPortionSize;
  FILESEARCHITEMFOUND* pFsItem;
  EXPANDABLE_BUFFERED_STRING EbsCanonicalFileNameTry;

  EbsCanonicalFileNameTry.dfSizeAllocatedInWChar = 0;
  EbsCanonicalFileNameTry.pBuf = NULL;

  /* need Win95/98/Me/32s check */
  //hFind = FindFirstFileW((LPCWSTR)szWildcard, &FileData);
  //printf("wilcard: '%ws'\n",szWildcard);

  hfsi=InitFileSearching(szWildcard);


  if (hfsi == NULL)
    return FALSE;
//printf("+++1\n");
  /* need Win95/98/Me/32s check */

  SvfGetFullPathName((dfwcharpc)szWildcard, sizeof(szPrePath) / sizeof(dfwchar),(dfwcharp)szPrePath,
                              (dfwcharp*)&pszFileNameAfterPath);

  *pszFileNameAfterPath = 0;
  dfPathPortionSize = dfUnicodeStrlen(szPrePath);

  //FILESEARCHITEMFOUND* GetNextItemContent(HFILESEARCHING);
  while (((pFsItem = GetNextItemContent(hfsi)) != NULL) && (fContinue && fAddOk))
    {
//printf("+++2\n");
        if ((pFsItem->fIsDirectory) == FALSE)
        {
          FILEUSERITEM fi;
          dfuLong32 dfSize;
          dfwcharp pszStr;
          BOOL fInsert=TRUE;
          if (szwPreviousDir==NULL)
          {
              dfSize = dfUnicodeStrlen(pFsItem->FileName) + 0x1;
              pszStr = (dfwcharp) DfsMalloc(dfSize * sizeof(dfwchar));
              DfsMemcpy(pszStr, pFsItem->FileName, dfSize * sizeof(dfwchar));
          }
          else
          {
              dfSize = dfUnicodeStrlen(pFsItem->FileName) + dfUnicodeStrlen(szwPreviousDir) + 0x2;
              pszStr = (dfwcharp) DfsMalloc(dfSize * sizeof(dfwchar));
              DfUnicodeStrcpy(pszStr,szwPreviousDir);
              DfUnicodeStrcat(pszStr,GetUnicodeStringDirectorySeparator());
              DfUnicodeStrcat(pszStr,pFsItem->FileName);
          }

          fi.FileNameOnArchive = pszStr;
          fi.fForceDate=FALSE;
          fi.hAddTags = NULL;
          fi.pReserved = NULL;
          ConvertFileNameAndPath(fi.FileNameOnArchive,NULL,0,FALSE);
          fi.fTempFile = FALSE;
          fi.fIdenticalPreviousVersion=FALSE;
          fi.fIgnore=FALSE;
//DispOutUnicodeString(fi.FileNameOnArchive);printf("\n");

          pszStr =
            (dfwcharp) DfsMalloc((dfPathPortionSize + dfSize) *
                                 sizeof(dfwchar));
          DfsMemcpy(pszStr, szPrePath, dfPathPortionSize * sizeof(dfwchar));
          DfsMemcpy(pszStr + dfPathPortionSize, pFsItem->FileName,
                    dfSize * sizeof(dfwchar));
          fi.FileNameOnDisk = pszStr;
          ///fi.dfFileSizeUncompressed = 0;

          if (fInsert && (szCanonicalFileNameToNotAdd!=NULL))
          {
              StoreCanonicalFileName(fi.FileNameOnDisk,&EbsCanonicalFileNameTry);
              if (EbsCanonicalFileNameTry.pBuf != NULL)
                if (dfUnicodeStrcmpi(EbsCanonicalFileNameTry.pBuf,szCanonicalFileNameToNotAdd)==0)
                {
                    fInsert=FALSE;
                }
          }



/*
          if (fInsert)
          {
              printf("insert item ");
              DispOutUnicodeString(pFsItem->FileName);
              printf(" size=%u\n",pFsItem->FileSize);
          }
*/

          if (fInsert)
            if (!FindSameElemPosSA(psa,&fi, NULL))
                InsertSortedSA(psa,&fi);
        }
        else
          if ((dfUnicodeStrcmpi(pFsItem->FileName, GetUnicodeStringDotDot()) != 0) &&
              (dfUnicodeStrcmpi(pFsItem->FileName, GetUnicodeStringDot()) != 0) && fRecurse)
        {
            dfwchar szNewDirPrevious[MAX_PATH_LENGTH];
            dfwchar szNewDirSrch[MAX_PATH_LENGTH];
            int iln;
            if (szwPreviousDir!=NULL)
            {
                DfUnicodeStrcpy(szNewDirPrevious,szwPreviousDir);
                DfUnicodeStrcat(szNewDirPrevious,GetUnicodeStringDirectorySeparator());
            }
            else szNewDirPrevious[0]=0;
            DfUnicodeStrcat(szNewDirPrevious,pFsItem->FileName);

            DfUnicodeStrcpy(szNewDirSrch,szPrePath);
            iln = dfUnicodeStrlen(szPrePath);
            if (iln>0)
            {
                dfwchar wc=szNewDirSrch[iln-1];
                if ((!CompareUnicodeWithSimpleChar(wc,'\\')) && (!CompareUnicodeWithSimpleChar(wc,'/')))
                //if ((szNewDirSrch[iln-1] != '\\') && (szNewDirSrch[iln-1] != '/'))
                {
                    DfUnicodeStrcat(szNewDirSrch,GetUnicodeStringDirectorySeparator());
                }
            }

            DfUnicodeStrcat(szNewDirSrch,pFsItem->FileName);
            DfUnicodeStrcat(szNewDirSrch,GetUnicodeStringDirectorySeparator());
            DfUnicodeStrcat(szNewDirSrch,GetUnicodeStringStar());
//printf("before add\n");
            fContinue = AddWilcardSortedStaticW(szNewDirSrch,psa,fRecurse,szNewDirPrevious,szCanonicalFileNameToNotAdd);
//printf("after add\n");
        }
//printf("+++3\n");
      //fContinue = FindNextFileW(hFind, &FileData);
    }
  //while (fContinue && fAddOk);

  //FindClose(hFind);
//printf("+++3a\n");
  CloseFileSearching(hfsi);
//printf("+++3b\n");
  FreeExpandableBufferedString(&EbsCanonicalFileNameTry);
//printf("after add2\n");
//printf("+++4\n");
  return fAddOk;
}


/*
BOOL AddWilcardSortedStaticTChar(LPTSTR szWildcard, STATIC_ARRAY_C psa,BOOL fRecurse,LPCTSTR szwPreviousDir,
                                 dfwcharpc szCanonicalFileNameToNotAdd)
{
  HANDLE hFind;
  WIN32_FIND_DATA FileData;
  BOOL fContinue;
  BOOL fAddOk = TRUE;
  TCHAR szPrePath[1024];
  LPTSTR pszFileNameAfterPath;
  dfuLong32 dfPathPortionSize;
  EXPANDABLE_BUFFERED_STRING EbsCanonicalFileNameTry;
  EbsCanonicalFileNameTry.dfSizeAllocatedInWChar = 0;
  EbsCanonicalFileNameTry.pBuf = NULL;

  //  need Win95/98/Me/32s check
  hFind = FindFirstFile(szWildcard, &FileData);
  //printf("wilcard: '%ws'\n",szWildcard);


  if ((hFind == NULL) || (hFind == INVALID_HANDLE_VALUE))
    return FALSE;

  // need Win95/98/Me/32s check
  GetFullPathName(szWildcard, sizeof(szPrePath) / sizeof(dfwchar), szPrePath,
                   &pszFileNameAfterPath);
  *pszFileNameAfterPath = 0;
  dfPathPortionSize = lstrlen(szPrePath);

  do
    {
        if ((FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0)
        {
          FILEUSERITEM fi;
          dfuLong32 dfSize;
          LPTSTR pszStr;
          dfuLong32 dfSizeAlloc;
          BOOL fInsert=TRUE;
          if (szwPreviousDir==NULL)
          {
              dfSize = lstrlen(FileData.cFileName) + 0x10;
              pszStr = (LPTSTR) DfsMalloc(dfSize * sizeof(TCHAR));
              lstrcpy(pszStr, FileData.cFileName);
          }
          else
          {
              dfSize = lstrlen(FileData.cFileName) + lstrlen(szwPreviousDir) + 0x10;
              pszStr = (LPTSTR) DfsMalloc(dfSize * sizeof(TCHAR));
              lstrcpy(pszStr, szwPreviousDir);
              lstrcat(pszStr,"\\");
              lstrcat(pszStr,FileData.cFileName);
          }

          dfSizeAlloc = (lstrlen(pszStr) + 0x10);
          fi.FileNameOnArchive = (dfwcharp)DfsMalloc(dfSizeAlloc*sizeof(dfwchar));
          fi.fForceDate=FALSE;
          fi.fAddNewTag = FALSE;
          fi.pReserved = NULL;
          ConvertTCharToUnicode(pszStr, (dfwcharp)fi.FileNameOnArchive, dfSizeAlloc);
          DfsFree(pszStr);

          ConvertFileNameAndPath(fi.FileNameOnArchive,NULL,0,FALSE);
          fi.fTempFile = FALSE;
          fi.fIdenticalPreviousVersion=FALSE;
          fi.fIgnore=FALSE;

          pszStr =
            (LPTSTR) DfsMalloc((dfPathPortionSize + dfSize) *
                                 sizeof(TCHAR));
          DfsMemcpy(pszStr, szPrePath, dfPathPortionSize * sizeof(TCHAR));
          DfsMemcpy(pszStr + dfPathPortionSize, FileData.cFileName,
                    dfSize * sizeof(TCHAR));


          dfSizeAlloc = (lstrlen(pszStr) + 0x10);
          fi.FileNameOnDisk = (dfwcharp)DfsMalloc(dfSizeAlloc*sizeof(dfwchar));
          ///fi.dfFileSizeUncompressed = 0;
          ConvertTCharToUnicode(pszStr, (dfwcharp)fi.FileNameOnDisk, dfSizeAlloc);
          DfsFree(pszStr);

          if (fInsert && (szCanonicalFileNameToNotAdd!=NULL))
          {
              StoreCanonicalFileName(fi.FileNameOnDisk,&EbsCanonicalFileNameTry);
              if (EbsCanonicalFileNameTry.pBuf != NULL)
                if (dfUnicodeStrcmpi(EbsCanonicalFileNameTry.pBuf,szCanonicalFileNameToNotAdd)==0)
                {
                    fInsert=FALSE;
                }
          }

          if (fInsert)
            if (!psa->FindSameElemPosSA(&fi, NULL))
                psa->InsertSortedSA(&fi);
        }
        else
          if ((lstrcmpi(FileData.cFileName, "..") != 0) &&
              (lstrcmpi(FileData.cFileName, ".") != 0) && fRecurse)
        {
            TCHAR szNewDirPrevious[MAX_PATH_LENGTH];
            TCHAR szNewDirSrch[MAX_PATH_LENGTH];
            int iln;
            if (szwPreviousDir!=NULL)
            {
                lstrcpy(szNewDirPrevious,szwPreviousDir);
                lstrcat(szNewDirPrevious,"\\");
            }
            else szNewDirPrevious[0]=0;
            lstrcat(szNewDirPrevious,FileData.cFileName);

            lstrcpy(szNewDirSrch,szPrePath);
            iln = lstrlen(szPrePath);
            if (iln>0)
                if (szNewDirSrch[iln-1] != '\\')
                {
                    lstrcat(szNewDirSrch,"\\");
                }

            lstrcat(szNewDirSrch,FileData.cFileName);
            lstrcat(szNewDirSrch,"\\*");
            fContinue = AddWilcardSortedStaticTChar(szNewDirSrch,psa,fRecurse,szNewDirPrevious,szCanonicalFileNameToNotAdd);
        }

      fContinue = FindNextFile(hFind, &FileData);

    }
  while (fContinue && fAddOk);

  FindClose(hFind);
  FreeExpandableBufferedString(&EbsCanonicalFileNameTry);

  return fAddOk;
}
*/


BOOL AddWilcardSortedStatic(dfwcharpc szwWilcard, STATIC_ARRAY_C psa,BOOL fAddExistingFile,BOOL fRecurse,
                            dfwcharpc szCanonicalFileNameToNotAdd)
{
    if (fAddExistingFile)
    {
        BOOL fRet;
        /*
        BOOL fUnicodeSupported = IsUnicodeApiSupported();

        if (fUnicodeSupported)
        {
            return AddWilcardSortedStaticW(szwWilcard,psa,fRecurse,NULL,szCanonicalFileNameToNotAdd);
        }
        else
        {
            TCHAR szWilcard[(MAX_PATH_LENGTH*2)+0x10];
                ConvertUnicodeToTChar(szwWilcard, szWilcard, MAX_PATH_LENGTH);

            return AddWilcardSortedStaticTChar(szWilcard,psa,fRecurse,NULL,szCanonicalFileNameToNotAdd);
        }*/
        //printf("avn+++\n");

        fRet= AddWilcardSortedStaticW(szwWilcard,psa,fRecurse,NULL,szCanonicalFileNameToNotAdd);
        //printf("ap---\n");
        return fRet;
    }
    else
    {
          FILEUSERITEM fi;
          dfuLong32 dfSize;
          dfwcharp pszStr;
          BOOL fRet=FALSE;

          DfsClearStruct(&fi, 0, sizeof(fi));

          dfSize = dfUnicodeStrlen((dfwcharpc)szwWilcard) + 0x1;
          pszStr = (dfwcharp) DfsMalloc(dfSize * sizeof(dfwchar));
          DfsMemcpy(pszStr, szwWilcard, dfSize * sizeof(dfwchar));

          fi.FileNameOnDisk = NULL;
          fi.FileNameOnArchive = pszStr;
          fi.fForceDate=FALSE;
          fi.hAddTags = NULL;
          fi.pReserved = NULL;

          fi.fTempFile = FALSE;
          fi.fIdenticalPreviousVersion=FALSE;
          fi.fIgnore=FALSE;

          if (!FindSameElemPosSA(psa,&fi, NULL))
                fRet = InsertSortedSA(psa,&fi);
          return fRet;
    }
}


BOOL DFSCALLBACK ProgressCallBackCreatePatchConsole(PROGRESSCALLBACKINFO * pProgressCallBackInfo)
{
    if (pProgressCallBackInfo->dfEvent==DFCBM_BEFOREOPENWORKINGFILE)
    {
        printf("processing ");
        DispOutUnicodeString(pProgressCallBackInfo->filename_stored);
    }

    if (pProgressCallBackInfo->dfEvent==DFCBM_AFTERCLOSINGWORKINGFILE)
    {
        printf("\n");
    }
    return TRUE;
}

BOOL AddWilcardSortedStaticFromList(dfwcharp lpszwWilcard, STATIC_ARRAY_C psa,BOOL fAddExistingFile,BOOL fRecurse,
                                    dfwcharpc szCanonicalFileNameToNotAdd)
{
    BOOL fRet=TRUE;
    for (;;)
    {
        dfwcharp lpszwWilcardEnd;
        dfwchar cSave;
        BOOL fRetThis;
        /*
        while (((*lpszwWilcard)==0x0d) || ((*lpszwWilcard)==0x0a) || ((*lpszwWilcard)==' ') || ((*lpszwWilcard)=='\t'))
            lpszwWilcard++;
            */
        while (CompareUnicodeWithSimpleChar(*lpszwWilcard,0x0d) || CompareUnicodeWithSimpleChar(*lpszwWilcard,0x0a) ||
               CompareUnicodeWithSimpleChar(*lpszwWilcard,' ' ) || CompareUnicodeWithSimpleChar(*lpszwWilcard,'\t'))
               lpszwWilcard++;

        if ((*lpszwWilcard)==0)
            break;
        lpszwWilcardEnd=lpszwWilcard;
        /*
        while (((*lpszwWilcardEnd) != 0x0d) && ((*lpszwWilcardEnd) != 0x0a) && ((*lpszwWilcardEnd) != 0x00))
            lpszwWilcardEnd++;
            */
        while ((!CompareUnicodeWithSimpleChar(*lpszwWilcardEnd,0x0d)) && (!CompareUnicodeWithSimpleChar(*lpszwWilcardEnd,0x0a)) &&
               (!CompareUnicodeWithSimpleChar(*lpszwWilcardEnd,0x00)))
               lpszwWilcardEnd++;
/*
        while (((*(lpszwWilcardEnd-1) == '\t') || ((*(lpszwWilcardEnd-1)) == ' ')) && (lpszwWilcardEnd>lpszwWilcard+1))
            lpszwWilcardEnd--;
            */
        while (CompareUnicodeWithSimpleChar(*(lpszwWilcardEnd-1),'\t') || CompareUnicodeWithSimpleChar(*(lpszwWilcardEnd-1),' '))
               lpszwWilcardEnd--;

        cSave = *lpszwWilcardEnd;
        *lpszwWilcardEnd = 0;
        //printf("adding '%ws' (len=%u)\n",lpszwWilcard,dfUnicodeStrlen(lpszwWilcard));
        fRetThis=AddWilcardSortedStatic(lpszwWilcard,psa,fAddExistingFile,fRecurse,szCanonicalFileNameToNotAdd);
        if (!fRetThis)
            fRet=FALSE;
        if (!fRet)
          break;
        *lpszwWilcardEnd = cSave;
        lpszwWilcard=lpszwWilcardEnd;
    }
    return fRet;
}

typedef struct
{
    dfwcharpc lpszName;
} FILENAMEITEM;

long DFSCALLBACK fncCompareDfwChar(const void *lpElem1, const void *lpElem2)
{
  const FILENAMEITEM *pfni1 = (const FILENAMEITEM *) lpElem1;
  const FILENAMEITEM *pfni2 = (const FILENAMEITEM *) lpElem2;

  return dfUnicodeStrcmpi(pfni1->lpszName, pfni2->lpszName);
}

BOOL DFSCALLBACK fncDestructorNothing(const void *lpElem)
{
  return TRUE;
}

BOOL RemoveFileNotInDirInfoFromList(PDIRINFO pDirInfo,STATIC_ARRAY_C psa)
{
    dfuLong32 i;
    BOOL fRet=TRUE;
    STATIC_ARRAY_C saInfoFileNameList;
    saInfoFileNameList = InitStaticArray_C(sizeof(dfwcharpc),0x80);
    SetFuncCompareDataSA(saInfoFileNameList,fncCompareDfwChar);
    SetFuncDestructorSA(saInfoFileNameList,fncDestructorNothing);

    for (i=0;i< pDirInfo->dfNbFile;i++)
    {
        FILENAMEITEM fni;
        fni.lpszName = ((pDirInfo->pFileInDirInfo)+i)->FileName;
        InsertSortedSA(saInfoFileNameList,&fni);
    }

    i=0;
    while (i < GetNbElemSA(psa))
    {
        BOOL fRemove = FALSE;
        dfuLong32 dwPos=0;
        const FILEUSERITEM *pfi = (const FILEUSERITEM *) GetElemPtrSA(psa,i);
        FILENAMEITEM fniSearch;
        fniSearch.lpszName = pfi->FileNameOnArchive;
        fRemove = !FindSameElemPosSA(saInfoFileNameList,&fniSearch,&dwPos);
        //test (pfi->FileNameOnArchive)
        if (fRemove)
            DeleteElemSA(psa,i,1);
        else
            i++;
    }

    DeleteStaticArray_C(saInfoFileNameList);
    return fRet;
}

#ifndef SVF_EXTRACT_ONLY



const FILEUSERITEM * GetCorrespondingOldFile(dfwcharpc dfSearchFileName,
                                             const STATIC_ARRAY_C saParam,const STATIC_ARRAY_C saParamRenamed,
                                             const STATIC_ARRAY_C saOldFile)
{
    const FILEUSERITEM *pftOldFile = NULL;

    {
        dfuLong32 dwPos=0;
        FILEUSERITEM fuSearch;
        fuSearch.FileNameOnArchive = dfSearchFileName;
        if (FindSameElemPosSA(saOldFile,&fuSearch, &dwPos))
        {
            pftOldFile = (const FILEUSERITEM *) GetElemPtrSA(saOldFile,dwPos);
        }
    }

    if (pftOldFile == NULL)
    {
        FILERENAMEDITEM frSearch;
        frSearch.FileNameRenamed = dfSearchFileName;
        dfuLong32 dwPosRenamed=0;
        if (FindSameElemPosSA(saParamRenamed,&frSearch, &dwPosRenamed))
        {
            const FILERENAMEDITEM *pfRenamed = (const FILERENAMEDITEM *) GetElemPtrSA(saParamRenamed,dwPosRenamed);
            FILEUSERITEM fuSearchRen;
            fuSearchRen.FileNameOnArchive = pfRenamed->FileNameOriginal;
            dfuLong32 dwPosRenItem=0;
            if (FindSameElemPosSA(saOldFile, &fuSearchRen, &dwPosRenItem))
            {
                pftOldFile = (const FILEUSERITEM *) GetElemPtrSA(saOldFile,dwPosRenItem);
            }
        }
    }

    return pftOldFile;
}

dfuLong32 InsertNewPatch(DFSFILE DfsFile, FILESET * pfsFileSetPrevious,
                       const STATIC_ARRAY_C saParam, const STATIC_ARRAY_C saParamRenamed,
                       dfwcharpc szVersionName,dfwcharpc szVersionComment,
                       const COMPRESSIONPARAM* pCprParam)
{
  dfuLong32 dfError = DFS_SUCCESS;
  STATIC_ARRAY_C saOldFile;
  FILETOADD *pfta = NULL;
  dfuLong32 i;

  saOldFile = InitStaticArray_C(sizeof(FILEUSERITEM), 0x80);
  SetFuncCompareDataSA(saOldFile, fncCompareFileUserItem);
  SetFuncDestructorSA(saOldFile, fncDestructorFileUserItem);

  for (i = 0; i < pfsFileSetPrevious->dfNbFileItem; i++)
    {
      FILEUSERITEM fi;
      fi.fTempFile = FALSE;
      fi.fIdenticalPreviousVersion=FALSE;
      fi.fIgnore=FALSE;
      fi.fForceDate=FALSE;
      fi.hAddTags = NULL;
      fi.pReserved = NULL;

      fi.FileNameOnArchive =
        dfUnicodeCopyConcatAlloc(((pfsFileSetPrevious->pFileItem) +
                                  i)->FileNameOnArchive, NULL);
      fi.FileNameOnDisk =
        dfUnicodeCopyConcatAlloc(((pfsFileSetPrevious->pFileItem) + i)->FileNameOnDisk,
                                 NULL);
      ///fi.dfFileSizeUncompressed = 0;
      fi.dfPreviousVersion = i; // very important !!
      if (!FindSameElemPosSA(saOldFile,&fi, NULL))
        InsertSortedSA(saOldFile,&fi);
    }
  pfta =
    (FILETOADD *) DfsMalloc(sizeof(FILETOADD) * (GetNbElemSA(saParam) + 1));

  for (i = 0; i < GetNbElemSA(saParam); i++)
    {
      const FILEUSERITEM *pftParam;
      pftParam = (const FILEUSERITEM *) GetElemPtrSA(saParam,i);


      const FILEUSERITEM *pfi = (const FILEUSERITEM *) GetElemPtrSA(saParam,i);
      dfwcharpc FileNameToSearch = pfi->FileNameOnArchive;
      const FILEUSERITEM *pftOldFile = GetCorrespondingOldFile(FileNameToSearch,saParam,saParamRenamed,saOldFile);
      if (pftOldFile != NULL)
        {
          (pfta + i)->dfPreviousVersionFilePosition = pftOldFile->dfPreviousVersion; // ++--###
          (pfta + i)->filename_prevversionondisk = pftOldFile->FileNameOnDisk;
        }
      else
        {
          (pfta + i)->filename_prevversionondisk = NULL;
          (pfta + i)->dfPreviousVersionFilePosition = 0;
        }
      (pfta + i)->filename_ondisk = pftParam->FileNameOnDisk;
      (pfta + i)->filename_tostore = pftParam->FileNameOnArchive;
      (pfta + i)->fForceRecopyPrevious = FALSE;
      (pfta + i)->fWritingRaw = FALSE;
      (pfta + i)->dfFileStatusForRaw = 0;
      (pfta + i)->hRamDifToFlushPatch = NULL;

      (pfta + i)->dfForceRecopyOrRawCopySize = 0;
      (pfta + i)->dfForceRecopyOrRawCopyCrc32 = 0;
      (pfta + i)->fForceRecopyOrRawCopyMd5Present = FALSE;
      (pfta + i)->fForceRecopyOrRawCopySha1Present = FALSE;
      (pfta + i)->fForceRecopyOrRawCopySha256Present = FALSE;
      (pfta + i)->fIgnore = FALSE;
      (pfta + i)->fForceDate = FALSE;
      (pfta + i)->hAddTags = FALSE;
      (pfta + i)->pReserved = NULL;
    }
  SetFuncCompareDataSA(saOldFile,fncCompareFileUserItem);

  dfError =
    InsertDirectoryinDfsFile(DfsFile, TYPEDIR_PATCHFROMPREVIOUS,
                             GetNbElemSA(saParam), pfta, FALSE,
                             szVersionName, szVersionComment,
                             pCprParam,
                             ProgressCallBackCreatePatchConsole, NULL,
                             NULL);
  DfsFree(pfta);

  DeleteStaticArray_C(saOldFile);
  return dfError;
}


dfuLong32 InsertNewPatchZip(DFSFILE DfsFile, FILESET * pfsFileSetPrevious,
                       const STATIC_ARRAY_C saParam,const STATIC_ARRAY_C saParamRenamed,
                       dfwcharpc szVersionName,dfwcharpc szVersionComment,
                       const COMPRESSIONPARAM* pCprParam)
{
  dfuLong32 dfError = DFS_SUCCESS;
  STATIC_ARRAY_C saOldFile;
  //FILETOADD *pfta = NULL;
  dfuLong32 i;

  if (GetNbElemSA(saParam) != 1)
  {
      printf("Error in parameter : just one zipfile for iz");
      return 0;
  }
  const FILEUSERITEM * pfuizip=(const FILEUSERITEM *) GetElemPtrSA(saParam,0);


    char szZipFileName[0x200];
    FILETOADDARRAY ftaArray;
    ConvertUnicodeToAnsi(pfuizip->FileNameOnDisk, szZipFileName,
                       sizeof(szZipFileName) - 1);

    ClearFileToAddArrayWithDelete(&ftaArray,FALSE,FALSE);
    if (!BuildFtaArrayFromZipfile(&ftaArray,szZipFileName))
    {
        ClearFileToAddArrayWithDelete(&ftaArray,TRUE,TRUE);
        printf("error in processing zipfile\n");
        return 0;
    }


  saOldFile=InitStaticArray_C(sizeof(FILEUSERITEM), 0x80);
  SetFuncCompareDataSA(saOldFile, fncCompareFileUserItem);
  SetFuncDestructorSA(saOldFile, fncDestructorFileUserItem);

  for (i = 0; i < pfsFileSetPrevious->dfNbFileItem; i++)
    {
      FILEUSERITEM fi;
      fi.fTempFile = FALSE;
      fi.fIdenticalPreviousVersion=FALSE;
      fi.fIgnore=FALSE;
      fi.fForceDate=FALSE;
      fi.hAddTags = NULL;
      fi.pReserved = NULL;

      fi.FileNameOnArchive =
        dfUnicodeCopyConcatAlloc(((pfsFileSetPrevious->pFileItem) +
                                  i)->FileNameOnArchive, NULL);
      fi.FileNameOnDisk =
        dfUnicodeCopyConcatAlloc(((pfsFileSetPrevious->pFileItem) + i)->FileNameOnDisk,
                                 NULL);
      ///fi.dfFileSizeUncompressed = 0;
      fi.dfPreviousVersion = i; // very important !!
      if (!FindSameElemPosSA(saOldFile,&fi, NULL))
        InsertSortedSA(saOldFile, &fi);
    }
  /*
  pfta =
    (FILETOADD *) DfsMalloc(sizeof(FILETOADD) * (saParam.GetNbElemSA() + 1));
    */
  for (i = 0; i < ftaArray.dfNbFileToAdd; i++)
  {
      FILETOADD *pftaCur = ftaArray.pFileToAdd+i;
      const FILEUSERITEM *pftOldFile = GetCorrespondingOldFile(pftaCur->filename_tostore,saParam,saParamRenamed,saOldFile);
      if (pftOldFile != NULL)
      {
          pftaCur->dfPreviousVersionFilePosition = pftOldFile->dfPreviousVersion; // ++--###
          pftaCur->filename_prevversionondisk = pftOldFile->FileNameOnDisk;
      }
  }
  SetFuncCompareDataSA(saOldFile, fncCompareFileUserItem);
/*
  for (i = 0; i < saParam.GetNbElemSA(); i++)
    {
      DWORD dwPos;
      const FILEUSERITEM *pftParam;
      pftParam = (const FILEUSERITEM *) saParam.GetElemPtrSA(i);


      if (saOldFile.FindSameElemPosSA(saParam.GetElemPtrSA(i), &dwPos))
        {
          const FILEUSERITEM *pftOldFile =
            (const FILEUSERITEM *) saOldFile.GetElemPtrSA(dwPos);
          (pfta + i)->dfPreviousVersionFilePosition = dwPos;
          (pfta + i)->filename_prevversionondisk = pftOldFile->FileNameOnDisk;
        }
      else
        {
          (pfta + i)->filename_prevversionondisk = NULL;
          (pfta + i)->dfPreviousVersionFilePosition = 0;
        }
      (pfta + i)->filename_ondisk = pftParam->FileNameOnDisk;
      (pfta + i)->filename_tostore = pftParam->FileNameOnArchive;
      (pfta + i)->fForceRecopyPrevious = FALSE;
      (pfta + i)->fWritingRaw = FALSE;
      (pfta + i)->dfFileStatusForRaw = 0;

      (pfta + i)->dfForceRecopyOrRawCopySize = 0;
      (pfta + i)->dfForceRecopyOrRawCopyCrc32 = 0;
      (pfta + i)->fForceRecopyOrRawCopyMd5Present = FALSE;
      (pfta + i)->fForceRecopyOrRawCopySha1Present = FALSE;
      (pfta + i)->fForceRecopyOrRawCopySha256Present = FALSE;
      (pfta + i)->fIgnore = FALSE;
      (pfta + i)->fForceDate = FALSE;
      (pfta + i)->fAddNewTag = FALSE;
      (pfta + i)->pReserved = NULL;
    }
*/
  dfError =
    InsertDirectoryinDfsFile(DfsFile, TYPEDIR_PATCHFROMPREVIOUS,
                             //saParam.GetNbElemSA(), pfta,
                             ftaArray.dfNbFileToAdd,ftaArray.pFileToAdd,
                             FALSE,
                             szVersionName, szVersionComment,
                             pCprParam,
                             ProgressCallBackCreatePatchConsole, NULL,
                             NULL);
  //DfsFree(pfta);

  for (i = 0; i < ftaArray.dfNbFileToAdd; i++)
  {
      FILETOADD *pftaCur = ftaArray.pFileToAdd+i;
      pftaCur->filename_prevversionondisk = NULL;
  }
  ClearFileToAddArrayWithDelete(&ftaArray,TRUE,TRUE);

  DeleteStaticArray_C(saOldFile);

  return dfError;
}
#endif


BOOL DFSCALLBACK ProgressCallBackCheckDir(PROGRESSCALLBACKINFO *
                                          pProgressCallBackInfo)
{
  FILETOCHECK *pFileToCheck =
    (FILETOCHECK *) pProgressCallBackInfo->dfUserPtr;


  if (pProgressCallBackInfo->dfEvent == DFCBM_BEFOREOPENWORKINGFILE)
    {
      if (pProgressCallBackInfo->fWillIgnoreFile)
        printf("ignore file %u\n", pProgressCallBackInfo->dfFileNumber);
      else
      {
        printf("Testing file # %u : ",
               pProgressCallBackInfo->dfFileNumber);
        DispOutUnicodeString(pProgressCallBackInfo->filename_stored);
        //printf("\n");
      }
    }

  if ((pProgressCallBackInfo->dfEvent == DFCBM_AFTERCLOSINGWORKINGFILE)
      && (!pProgressCallBackInfo->fWillIgnoreFile))
  {
    //printf("  End test file nbr %u :",pProgressCallBackInfo->dfFileNumber);
    //DispOutUnicodeString(pProgressCallBackInfo->filename_onwork);
    if ((pFileToCheck + pProgressCallBackInfo->dfFileNumber)->fIsIdentical)
        printf(" : OK\n");
    else
        printf(" Mismatch !\n");
  }
  return TRUE;
}




dfuLong32 DFSCALLBACK CallBackAskReplacing(const ASKREPL_CBINFO* pAskRepl_CbInfo,
                                         dfvoidp dfUserPtr)
{
    char rep;
    dfuLong32 dfRet = 0;
    printf("Replacing file ");
    DispOutUnicodeString(pAskRepl_CbInfo->dfFileNameToStore),
    printf(" in version number %u ? : ",pAskRepl_CbInfo->dfVersion);

    printf("\n    [r]eplace, [R]eplace alwyas, [n]o replace, [N]ever replace, [C]ancel : ");
    do
    {
      rep = My_getch_console_char();
      switch (rep)
      {
      case 'r' : dfRet = CBASKREPLACING_ANSWER_REPLACE;
                 break;

      case 'R' : dfRet = CBASKREPLACING_ANSWER_REPLACEALL;
                 break;

      case 'n' : dfRet = CBASKREPLACING_ANSWER_NOREPLACE;
                 break;

      case 'N' : dfRet = CBASKREPLACING_ANSWER_NOREPLACEALL;
                 break;

      case 'C' : dfRet = CBASKREPLACING_ANSWER_CANCEL;
                 break;
      }
    } while (dfRet == 0);
    printf("%c\n",rep);

    return dfRet;
}



dfwcharp ReadTextFileContent(dfwcharpc dfFileName,BOOL fUnicode,BOOL fCutFirstLine)
{
  LOWLEVELFILE llr;
  void* lpBufRead;
  dfuLong32 dfRead;
  dfuLong32 dfSizeText=0;
  dfuLong32 dfSizeTextHigh=0;
  dfwcharp ret;

  llr=OpenLowLevel(dfFileName,OPEN_READ,FALSE,FALSE,0,NULL);
  if (llr==NULL)
      return NULL;
  LowLevelGetSize(llr,&dfSizeText,&dfSizeTextHigh);
  lpBufRead = (void*)DfsMalloc(dfSizeText+2);
  if (lpBufRead == NULL)
  {
      LowLevelClose(llr,NULL);
      return NULL;
  }
  DfsClearStruct(lpBufRead,0,dfSizeText+2);
  dfRead = LowLevelRead(llr,lpBufRead,dfSizeText,NULL);
  LowLevelClose(llr,NULL);

  if (dfRead != dfSizeText)
  {
      DfsFree((void*)lpBufRead);
      lpBufRead=NULL;
  }

  if ((lpBufRead==NULL) || fUnicode)
  {
      ret = (dfwcharp)lpBufRead;
      if (ret!=NULL)
      {
          dfbyte c1=*((dfbytep)ret);
          dfbyte c2=*(((dfbytep)ret)+1);
          dfuLong32 i;
          if ((c1==0xff) && (c2==0xfe))
              for (i=0;i<(dfSizeText/2);i++)
                  *(ret+i) = *(ret+i+1);
          else
          if ((c1==0xfe) && (c2==0xff))
          {
              for (i=0;i<(dfSizeText/2);i++)
              {
                  dfwchar cBig=*(ret+i+1);
                  cBig = (dfwchar)(((cBig & 0xff00) >> 8) | ((cBig & 0x00ff) << 8));
                  *(ret+i) = cBig;
              }
          }
      }
  }
  else
  {
      dfwcharp lpszTextUnicode = (dfwcharp)DfsMalloc((dfSizeText+2)*sizeof(dfwchar));

      DfsClearStruct(lpszTextUnicode,0,(dfSizeText+2)*sizeof(dfwchar));
      ConvertAnsiToUnicode((char*)lpBufRead,lpszTextUnicode,dfSizeText+1);
      DfsFree((void*)lpBufRead);
      ret = lpszTextUnicode;
  }

  if ((ret != NULL) && (fCutFirstLine))
  {
      dfwcharp pBrowse=ret;
      while ((*pBrowse)!=0)
      {
          dfwchar c=*pBrowse;
          if ((c==0x0a)||(c==0x0d))
              *pBrowse=0;
          pBrowse++;
      }
  }
  return ret;
}

#ifndef SVF_EXTRACT_ONLY
BOOL CreateTemporaryDfsFileNameInSameDirectory(dfwcharpc lpszOriginalFile,dfwcharp lpszTempFileName,dfuLong32 uiSizeBuffer)
{
    dfwchar szDirectory[MAX_PATH_LENGTH+8];
    dfwchar szTempFileName[MAX_PATH_LENGTH+8];
    dfwcharp lpszFilePart=NULL;
    SvfGetFullPathName(lpszOriginalFile,MAX_PATH_LENGTH,szDirectory,&lpszFilePart);
    *lpszFilePart=0;
    SvfGetTempFileName(szDirectory,GetUnicodeSVFPrefix(),0,szTempFileName);
    if (((unsigned)dfUnicodeStrlen(szTempFileName))>=uiSizeBuffer)
        return FALSE;
    DfUnicodeStrcpy(lpszTempFileName,szTempFileName);
    return TRUE;
}



BOOL ExecuteRemix(dfuLong32 dfNbDirRemix,const VERSIONTOADD_REMIX *pVersionRemix,
                  DFSFILE* pDfsFile,
                  dfwcharpc lpszDfsFileName,dfwcharpc lpszDfsFileNameNewDest,dfwcharpc szDfsBaseDirectory,FILESET*pfsUnzippedBase,
                  dfuLong32 dfNbDir,const PCDIRINFO* pDirInfo,dfuLong32 dfBaseVersionSpecified,
                  BOOL fReuseOldPatch, BOOL fRawAccepted,
                  const COMPRESSIONPARAM* pCprParam,const DFSFEATUREPARAM* pDfsFeatureParam,
                  BOOL fChangeFirstVersionStorage,BOOL fFirstVersionAsReferenceRequested)
{
    dfwchar szwTempFileName[MAX_PATH_LENGTH+8];
    BOOL fBaseDirectorySelected ;
    BOOL fBaseDirectoryNeeded = FALSE;
    dfuLong32 dfError ;
    BOOL fRet=TRUE;
    DFSFEATUREPARAM DfsFeatureParamUse;
    DfsFeatureParamUse = *pDfsFeatureParam;

    if (lpszDfsFileNameNewDest == NULL)
      CreateTemporaryDfsFileNameInSameDirectory(lpszDfsFileName,szwTempFileName,MAX_PATH_LENGTH);
    fBaseDirectorySelected = (szDfsBaseDirectory[0] != '\0');
    if (pfsUnzippedBase!=NULL)
        if (pfsUnzippedBase->dfNbFileItem > 0)
            fBaseDirectorySelected = TRUE;

    {
        dfuLong32 dfNbDir=0;

        DfsGetNbDir(*pDfsFile, &dfNbDir,NULL);
        if (dfNbDir>0) {
            PDIRINFO pDirInfo=NULL;
            if (ReadDirectoryInfo(*pDfsFile, 0, &pDirInfo, NULL, NULL,NULL) == DFS_SUCCESS)
            {
                dfuLong32 dfTypeFirstDir = ((((pDirInfo))))->dfTypeDir;
                fBaseDirectoryNeeded = dfTypeFirstDir == TYPEDIR_FILECRCONLY;
            }
            if (pDirInfo != NULL)
              FreeDirectoryInfo((&pDirInfo), NULL);
        }
    }

    BOOL fFirstVersionAsReferenceNewDfs = fBaseDirectoryNeeded;

    if (fChangeFirstVersionStorage)
        fFirstVersionAsReferenceNewDfs = fFirstVersionAsReferenceRequested;

    dfError = DoReMixDfsEx(*pDfsFile,dfNbDir,pDirInfo,
                NULL,
                (lpszDfsFileNameNewDest == NULL) ? szwTempFileName: lpszDfsFileNameNewDest, // BOOL fZipFile,
                &DfsFeatureParamUse,
                fBaseDirectorySelected,
                fBaseDirectorySelected ? dfBaseVersionSpecified : 0,
                fBaseDirectorySelected ? szDfsBaseDirectory : NULL,
                fBaseDirectorySelected ? pfsUnzippedBase : NULL,
                dfNbDirRemix, pVersionRemix,
                fFirstVersionAsReferenceNewDfs,fReuseOldPatch,fRawAccepted,
                pCprParam, //const COMPRESSIONPARAM* pCprParam,
                NULL,NULL, //tSetExtractPosCallBack pSetExtractPosCallBack,dfvoidp dfUserPtr,
                0,0, //dfuLong32 dwMinProgress,dfuLong32 dwMaxProgress);
                NULL);




    if (dfError == DFS_SUCCESS)
    {
        dfwcharp lpszCopyFileName = (dfwcharp)DfsMalloc((dfUnicodeStrlen(lpszDfsFileName)+4)*sizeof(dfwchar));
        DfUnicodeStrcpy(lpszCopyFileName,lpszDfsFileName);

        DfsClose(*pDfsFile,NULL);
        *pDfsFile=NULL;

        //dfuLong32 dwErr=0;
        /* warning : MoveFileEx is not Win95/98 compatible */
        if (lpszDfsFileNameNewDest == NULL)
        {
            BOOL fRenamed = SvfMoveFile(szwTempFileName,lpszCopyFileName);
            if (!fRenamed)
                fRet=FALSE;
        }
        DfsFree(lpszCopyFileName);
    }
    else
    {
        MyDeleteFile(szwTempFileName,NULL);
        fRet=FALSE;
    }

  return fRet;
}
#endif

typedef struct
{
    BOOL fYesAll;
    BOOL fNoAll;
} OVERWRITEPARAM;

dfuLong32 DFSCALLBACK ConfirmConsoleBeforeCreatingFile(dfwcharpc dfFileName,
                                                       dfvoidp dfpAdditionnalInfo,
                                                       dfvoidp dfUserPtr)
{
    dfuLong32 ret = 0;
    OVERWRITEPARAM* powp = (OVERWRITEPARAM*)dfUserPtr;
    LOWLEVELFILE llr;
    dfwchar rep;
    BOOL fGoodAnswer;

    if (powp ->fYesAll)
        return CONFIRM_BEFORE_CREATING_FILE_OK;

    llr=OpenLowLevel(dfFileName,OPEN_READ,FALSE,FALSE,0,NULL);
    if (llr == NULL)
        return CONFIRM_BEFORE_CREATING_FILE_OK;

    LowLevelClose(llr,NULL);
    if (powp ->fNoAll)
        return CONFIRM_BEFORE_CREATING_FILE_SKIP;

    printf("Replace ");
    DispOutUnicodeString(dfFileName);
    printf(" [y]es, [n]o, [A]lways, [N]ever: ");


    do
    {
      rep = My_getch_console_char();
      fGoodAnswer=FALSE;
      switch (rep)
      {
        case 'A' :  powp->fYesAll=TRUE;
        case 'Y' :
        case 'y' :  ret = CONFIRM_BEFORE_CREATING_FILE_OK;
                    fGoodAnswer=TRUE;
                    break;

        case 'N' :  powp->fNoAll=TRUE;
        case 'n' :  ret = CONFIRM_BEFORE_CREATING_FILE_SKIP;
                    fGoodAnswer=TRUE;
                    break;


        case 'S' :
        case 's' :  ret = CONFIRM_BEFORE_CREATING_FILE_STOP;
                    fGoodAnswer=TRUE;
                    break;
      }
    } while (!fGoodAnswer);
    printf("%c\n",rep);

    return ret;
}

dfuLong32 DFSCALLBACK fncExtractingFileWorkingEventConsole(dfuLong32 dfEvent,
                                               const EXTR_WORK_EVENT_INFO* pEventPtr,
                                               dfvoidp dfUserPtr)
{
    if (dfEvent == EXTR_WORK_EVENT_BEFORE_EXTRACTING_FILE)
        if (!pEventPtr->fTempFile)
        {
            printf("extracting : ");
            DispOutUnicodeString(pEventPtr->dfFileNameOnDisk);
        }

    if (dfEvent == EXTR_WORK_EVENT_EXTRACTING_FILE_FINISHED)
        if (!pEventPtr->fTempFile)
        {
            if (pEventPtr->dfSuccess==0)
                printf(" Done\n");
            else
                printf(" Error %u\n",pEventPtr->dfSuccess);
        }

    return 0;
}

BOOL AdaptDfsFileFeatureForOpenedDfs(DFSFILE DfsFile)
{
  dfuLong32 dfNbDir = 0;
  if (DfsGetNbDir(DfsFile, &dfNbDir,NULL) != DFS_SUCCESS)
      return FALSE;
  PDIRINFO* pDirInfo = (PDIRINFO*)DfsMalloc(sizeof(PDIRINFO)*(dfNbDir+1));
  if (pDirInfo == NULL)
      return FALSE;

  BOOL fRet = TRUE;
  dfuLong32 dfNumDir;
  for (dfNumDir = 0; dfNumDir <= dfNbDir; dfNumDir++)
  {
      (*(pDirInfo + dfNumDir)) = NULL;
  }

  for (dfNumDir = 0; dfNumDir < dfNbDir; dfNumDir++)
  {
      dfuLong32 dfError = DFS_SUCCESS;
      dfError = ReadDirectoryInfo(DfsFile, dfNumDir, (pDirInfo)+dfNumDir, NULL, NULL,NULL);
      if (dfError != DFS_SUCCESS)
      {
          fRet = FALSE;
          break;
      }

      if (dfNumDir > 0)
      {
          PDIRINFO pDirInfoPrev=*((pDirInfo)+dfNumDir-1);
          PDIRINFO pDirInfoCur=*((pDirInfo)+dfNumDir);

          FixIndenticalDifferentSizeInReadDirectoryInfo(pDirInfoPrev,pDirInfoCur);
      }
  }


  if (fRet)
    AdaptDfsFileFeature(DfsFile,(const PCDIRINFO*)pDirInfo,dfNbDir);

  for (dfNumDir = 0; dfNumDir < dfNbDir; dfNumDir++)
    FreeDirectoryInfo(pDirInfo + dfNumDir, NULL);

  DfsFree(pDirInfo);
  return fRet;
}


static BOOL PerformCommandLineInternal(dfwcharpc pCommandLine)
{
  //dfwcharpc pBrowseCommandLine = pCommandLine;
  dfwchar szPortionLine[MAX_PATH_LENGTH+1024];
  dfwchar szDfsFileName[MAX_PATH_LENGTH+1024];
  dfwchar szDfsBaseDirectory[MAX_PATH_LENGTH+1024];
  dfwchar szTargetDfs[MAX_PATH_LENGTH+1024];
  //dfwchar szVersionName[MAX_PATH_LENGTH+1024];
  //dfwchar szVersionComment[MAX_PATH_LENGTH+1024];
  dfwcharpc lpszVersionName=NULL;
  dfwcharpc lpszVersionComment=NULL;


  BOOL fGoodPrefix = FALSE;
  BOOL fCmdL, fCmdLV, fCmdCR, fCmdCZ, fCmdCRz, fCmdCZz, fCmdI, fCmdIz, fCmdA, fCmdD, fCmdDV, fCmdRS, fCmdX, fCmdXz, fCmdT, fCmdC, fCmdM, fCmdBld, fCmdAppVer;
  BOOL fCmdIXNew=TRUE;
  BOOL fRecurse;
  BOOL fOverwriteAll;
  BOOL fAddExistingFile;
  BOOL fOnlyExistLatest=FALSE;
  BOOL fOnlyExistVersionSpecified=FALSE;
  BOOL fChangeFirstVersionStorage = FALSE;
  BOOL fFirstVersionAsReferenceRequested = FALSE;
  BOOL fVerbose = FALSE;
  BOOL fReuseOldPatch = TRUE;
  BOOL fRawAccepted = TRUE;
  dfuLong32 dfVersionOnlyExist = VALUE_UNKNOWN;
  //FILESET fsParam;
  dfuLong32 dfVersionSpecified = VALUE_UNKNOWN;
  dfuLong32 dfLastVersionSpecified = VALUE_UNKNOWN;
  dfuLong32 dfBaseVersionSpecified = VALUE_UNKNOWN;
  EXTRACTINGMAPITEM ExtractingPreferedMethod = ExtractClassic;
  STATIC_ARRAY_C saParam=NULL;
  STATIC_ARRAY_C saParamRenamed=NULL;
  BOOL fRet=TRUE;
  COMPRESSIONPARAM cprParam;
  EXPANDABLE_BUFFERED_STRING EbsCanonicalFileNameDfs;
  FILESET fsUnzippedBase;
  BOOL fComposition = FALSE;
  /*
  BOOL fSha1=FALSE;
  BOOL fSha256=FALSE;
  BOOL fMd5=TRUE;
  BOOL fV1Compat=FALSE;
*/
   DFSFEATUREPARAM DfsFeatureParam;
   DfsFeatureParam.fComputeMd5 = TRUE;
   DfsFeatureParam.fComputeSha1 = FALSE;
   DfsFeatureParam.fComputeSha256 = FALSE;
   DfsFeatureParam.fStripIdenticalBody = TRUE;

   EbsCanonicalFileNameDfs.pBuf = NULL;
   EbsCanonicalFileNameDfs.dfSizeAllocatedInWChar = 0;


  fCmdLV = fCmdL = fCmdCR = fCmdCZ = fCmdA = fCmdD = fCmdDV = fCmdRS = fCmdI = fCmdIz =
           fCmdX = fCmdXz = fCmdT = fCmdC = fCmdM = fCmdBld = fCmdCRz = fCmdCZz = FALSE;
  fCmdAppVer = FALSE;
  fRecurse = fOverwriteAll = FALSE;

  szPortionLine[0] = szDfsFileName[0] = szDfsBaseDirectory[0] = szTargetDfs[0] = 0;
  saParam=InitStaticArray_C(sizeof(FILEUSERITEM), 0x80);
  SetFuncCompareDataSA(saParam, fncCompareFileItem);
  SetFuncDestructorSA(saParam, fncDestructorFileItem);

  saParamRenamed=InitStaticArray_C(sizeof(FILERENAMEDITEM), 0x80);
  SetFuncCompareDataSA(saParamRenamed, fncCompareFileRenamed);
  SetFuncDestructorSA(saParamRenamed, fncDestructorFileRenamed);

#ifndef SVF_EXTRACT_ONLY
  InitDefaultCompressionParam(&cprParam);
#endif
  cprParam.dfPhysicalMemoryKB = GetPhysSizeKbUsable();

  printf("Smartversion " SVF_VERSION_STRING " (c) 2002-" SVF_VERSION_YEAR_STRING " G. Vollant - http://www.smartversion.com/");
  if (sizeof(void*)==4)
      printf(" - 32 bits");
    if (sizeof(void*)==8)
        printf(" - 64 bits");
  printf("\n");

  if ((sizeof(dfuLong32)!=4) || (sizeof(dfsLong32)!=4))
  {
      printf("error internal size 32\n");
      DeleteStaticArray_C(saParam);
      DeleteStaticArray_C(saParamRenamed);
      return FALSE;
  }

  if ((sizeof(dfuLong64)!=8) || (sizeof(dfsLong64)!=8))
  {
      printf("error internal size 64\n");
      DeleteStaticArray_C(saParam);
      DeleteStaticArray_C(saParamRenamed);
      return FALSE;
  }

  if ((sizeof(dfuInt16)!=2) || (sizeof(dfwchar)!=2))
  {
      printf("error internal size 16\n");
      DeleteStaticArray_C(saParam);
      DeleteStaticArray_C(saParamRenamed);
      return FALSE;
  }

  if ((sizeof(dfuIntPtr)!=sizeof(void*)))
  {
      printf("error internal size ptr\n");
      DeleteStaticArray_C(saParam);
      DeleteStaticArray_C(saParamRenamed);
      return FALSE;
  }
  //InitFileSet(&fsParam);

  if (pCommandLine == NULL)
    {
      help();
      DeleteStaticArray_C(saParam);
      DeleteStaticArray_C(saParamRenamed);
      return TRUE;
    }

  if (*pCommandLine == 0)
    {
      help();
      return TRUE;
    }


  pCommandLine =
    CopyStrWord(pCommandLine, szPortionLine,
                (sizeof(szPortionLine) / sizeof(dfwchar)) - 1);

  if (*pCommandLine == 0)
    {
      help();
      DeleteStaticArray_C(saParam);
      DeleteStaticArray_C(saParamRenamed);
      return TRUE;
    }

  InitFileSet(&fsUnzippedBase);

  pCommandLine =
    CopyStrWord(pCommandLine, szPortionLine,
                (sizeof(szPortionLine) / sizeof(dfwchar)) - 1);

  if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/dumpcrash") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-dumpcrash") == 0)))
  {
    pCommandLine =
        CopyStrWord(pCommandLine, szPortionLine,
                    (sizeof(szPortionLine) / sizeof(dfwchar)) - 1);
  }

  if (*pCommandLine == 0)
    {
      FreeFileSet(&fsUnzippedBase,TRUE);
      help();
      DeleteStaticArray_C(saParam);
      DeleteStaticArray_C(saParamRenamed);
      return TRUE;
    }

  if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/r") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-r") == 0)))
    {
      fRecurse=TRUE;

      pCommandLine =
        CopyStrWord(pCommandLine, szPortionLine,
                    (sizeof(szPortionLine) / sizeof(dfwchar)) - 1);

      if (*pCommandLine == 0)
        {
          FreeFileSet(&fsUnzippedBase,TRUE);
          help();
          DeleteStaticArray_C(saParam);
          DeleteStaticArray_C(saParamRenamed);
          return TRUE;
        }
    }

  if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/l") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-l") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "l") == 0)))
    {
      fCmdL = fGoodPrefix = TRUE;
    }

  if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/lc") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-lc") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "lc") == 0)))
    {
      fCmdL = fGoodPrefix = fComposition = TRUE;
    }

  if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/lv") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-lv") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "lv") == 0)))
    {
      fCmdLV = fGoodPrefix = TRUE;
    }

  if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/lvc") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-lvc") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "lvc") == 0)))
  {
      fCmdLV = fGoodPrefix = fComposition = TRUE;
  }

#ifndef SVF_EXTRACT_ONLY
  if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/cr") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-cr") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "CR") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "cr") == 0)))
    {
      fCmdCR = fGoodPrefix = TRUE;
    }
  if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/crz") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-crz") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "crz") == 0)))
    {
      fCmdCRz = fGoodPrefix = TRUE;
    }

  if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/cz") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-cz") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "cz") == 0)))
    {
      fCmdCZ = fGoodPrefix = TRUE;
    }
  if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/czz") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-czz") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "czz") == 0)))
    {
      fCmdCZz = fGoodPrefix = TRUE;
    }

  if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/i") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-i") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "i") == 0)))
    {
      fCmdI = fGoodPrefix = TRUE;
    }
  if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/iz") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-iz") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "iz") == 0)))
    {
      fCmdIz = fGoodPrefix = TRUE;
    }

  if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/av") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-av") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "av") == 0)))
    {
      fCmdAppVer = fGoodPrefix = TRUE;
    }

  if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/a") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-a") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "a") == 0)))
    {
      fCmdA = fGoodPrefix = TRUE;
    }

  if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/d") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-d") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "d") == 0)))
    {
      fCmdD = fGoodPrefix = TRUE;
    }

  if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/dv") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-dv") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "dv") == 0)))
    {
      fCmdDV = fGoodPrefix = TRUE;
    }
  if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/rs") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-rs") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "rs") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/restructure") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-restructure") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "restructure") == 0)))
    {
      fCmdRS = fGoodPrefix = TRUE;
    }
#endif

  if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/x") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-x") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "x") == 0)))
    {
      fCmdX = fGoodPrefix = TRUE;
    }
  if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/xz") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-xz") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "xz") == 0)))
    {
      fCmdXz = fGoodPrefix = TRUE;
    }

#ifndef SVF_EXTRACT_ONLY
  if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/m") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-m") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "m") == 0)))
    {
      fCmdM = fGoodPrefix = TRUE;
    }

  if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/BuildPatch") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-BuildPatch") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "BuildPatch") == 0)))
  {
      fCmdBld = fGoodPrefix = TRUE;
  }
#endif

  if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/t") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-t") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "t") == 0)))
    {
      fCmdT = fGoodPrefix = TRUE;
    }

  if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/c") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-c") == 0)) ||
      ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "c") == 0)))
    {
      fCmdC = fGoodPrefix = TRUE;
    }



  if (fGoodPrefix)
    {
      pCommandLine =
        CopyStrWord(pCommandLine, szDfsFileName,
                    (sizeof(szDfsFileName) / sizeof(dfwchar)) - 1);

      if (((dfCompareUnicodeArgumentWithAnsiString(szDfsFileName, "/r") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szDfsFileName, "-r") == 0)))
        {
          fRecurse=TRUE;

          pCommandLine =
            CopyStrWord(pCommandLine, szDfsFileName,
                        (sizeof(szPortionLine) / sizeof(dfwchar)) - 1);

          if (*pCommandLine == 0)
            {
              FreeFileSet(&fsUnzippedBase,TRUE);
              help();
              DeleteStaticArray_C(saParam);
              DeleteStaticArray_C(saParamRenamed);
              return TRUE;
            }
        }


      if (szDfsFileName[0] == 0)
        fGoodPrefix = FALSE;
    }

  {
        dfuLong32 dfLen = dfUnicodeStrlen(szDfsFileName);
        if (dfLen < (sizeof(szDfsFileName) / sizeof(dfwchar)) - 5)
            AppendExtSvfIfNeeded(szDfsFileName);
  }

  if (!fGoodPrefix)
    {
      FreeFileSet(&fsUnzippedBase,TRUE);
      printf("bad command\n");
      help();
      DeleteStaticArray_C(saParam);
      DeleteStaticArray_C(saParamRenamed);
      return FALSE;
    }

#ifndef SVF_EXTRACT_ONLY
  if (fCmdM)
  {
    FreeFileSet(&fsUnzippedBase,TRUE);
    DeleteStaticArray_C(saParam);
    DeleteStaticArray_C(saParamRenamed);
    return MixDfs(szDfsFileName,pCommandLine);
  }
  if (fCmdBld)
  {
    FreeFileSet(&fsUnzippedBase,TRUE);
    DeleteStaticArray_C(saParam);
    DeleteStaticArray_C(saParamRenamed);
    return BuildPatchCmdLine(szDfsFileName,pCommandLine);
  }
#endif
  fAddExistingFile = !(fCmdX || fCmdXz || fCmdD || fCmdDV || fCmdRS || fCmdL || fCmdLV);
/* latest return until end */

  StoreCanonicalFileName(szDfsFileName,&EbsCanonicalFileNameDfs);

  if ((*pCommandLine) == 0)
    {
      //AddWilcardToFileSet(L"*", &fsParam,fRecurse);
      AddWilcardSortedStatic(GetUnicodeStringStar(), saParam,fAddExistingFile,fRecurse,
          fAddExistingFile ? EbsCanonicalFileNameDfs.pBuf : NULL);
      //printf("no param\n");
    }


  /* fCmdX - fCmdXz must be different */
  while ((*pCommandLine) != 0)
    {
      pCommandLine =
        CopyStrWord(pCommandLine, szPortionLine,
        (sizeof(szPortionLine) / sizeof(dfwchar)) - 1);

      if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/inplace") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-inplace") == 0)))
      {
          ExtractingPreferedMethod = ExtractInPlace;
      }
      else
      if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/inplacenochecksum") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-inplacenochecksum") == 0)))
      {
          ExtractingPreferedMethod = ExtractInPlaceNoChecksum;
      }
      else
      if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/inplace") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-inplace") == 0)))
      {
          ExtractingPreferedMethod = ExtractInPlace;
      }
      else
      if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/bymerging") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-bymerging") == 0)))
      {
          ExtractingPreferedMethod = ExtractByMerging;
      }
      else
      if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/extractbystep") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-extractbystep") == 0)))
      {
          ExtractingPreferedMethod = ExtractClassic;
      }
      else
      if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/r") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-r") == 0)))
        {
          fRecurse=TRUE;
        }
      else
      if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/ref") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-ref") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/reference") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-reference") == 0)))
        {
          fChangeFirstVersionStorage=TRUE;
          fFirstVersionAsReferenceRequested=TRUE;
        }
      else
      if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/cpr") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-cpr") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/compressed") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-compressed") == 0)))
        {
          fChangeFirstVersionStorage=TRUE;
          fFirstVersionAsReferenceRequested=FALSE;
        }
      else
      if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/out") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-out") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/target") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-target") == 0)))
        {
           pCommandLine =
             CopyStrWord(pCommandLine, szTargetDfs,
                         (sizeof(szTargetDfs) / sizeof(dfwchar)) - 1);
        }
      else
      if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/verbose") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-verbose") == 0)))
        {
          fVerbose=TRUE;
        }
      else
      if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/oel") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-oel") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/onlyexistlatest") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-onlyexistlatest") == 0)))
        {
          fOnlyExistLatest=TRUE;
        }
      else
      if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/recpr") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-recpr") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/recompress") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-recompress") == 0)))
      {
          fRawAccepted=FALSE;
      }
      else
      if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/bldpatch") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-bldpatch") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/rebuildpatch") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-rebuildpatch") == 0)))
      {
          fReuseOldPatch = FALSE;
      }
      else
      if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/bld") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-bld") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/rebuild") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-rebuild") == 0)))
      {
          fReuseOldPatch = fRawAccepted = FALSE;
      }
      else
      if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/oev") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-oev") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/onlyexistversion") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-onlyexistversion") == 0)))
            {
            dfwchar szOEVerNum[MAX_PATH_LENGTH];
            unsigned long lValue;
            fOnlyExistVersionSpecified=TRUE;

            szOEVerNum[0]=0;
            pCommandLine =
                CopyStrWord(pCommandLine, szOEVerNum,
                            (sizeof(szOEVerNum) / sizeof(dfwchar)) - 1);

            lValue=ConvertUnicodeStringToLong(szOEVerNum);
            dfVersionOnlyExist=(dfuLong32)lValue;
            }
        else
        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/v1compat") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-v1compat") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/v1compatibility") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-v1compatibility") == 0)))
        {
          DfsFeatureParam.fStripIdenticalBody=FALSE;
        }
        else
        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/nomd5") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-nomd5") == 0)))
        {
            DfsFeatureParam.fComputeMd5=FALSE;
        }
        else
        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/sha1") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-sha1") == 0)))
        {
            DfsFeatureParam.fComputeSha1=TRUE;
        }
        else
        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/sha256") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-sha256") == 0)))
        {
            DfsFeatureParam.fComputeSha256=TRUE;
        }

      else
        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/blocksize") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-blocksize") == 0)))
            {
            dfwchar szBlockSize[MAX_PATH_LENGTH];
            unsigned long lValue;

            szBlockSize[0]=0;
            pCommandLine =
                CopyStrWord(pCommandLine, szBlockSize,
                            (sizeof(szBlockSize) / sizeof(dfwchar)) - 1);

            lValue=ConvertUnicodeStringToLong(szBlockSize);
            cprParam.dfBlockCalcSizeSearch = (dfuLong32)lValue;
            }
        else
        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/compressratio") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-compressratio") == 0)))
            {
            dfwchar szValWChar[MAX_PATH_LENGTH];
            unsigned long lValue;

            szValWChar[0]=0;
            pCommandLine =
                CopyStrWord(pCommandLine, szValWChar,
                            (sizeof(szValWChar) / sizeof(dfwchar)) - 1);

            lValue=ConvertUnicodeStringToLong(szValWChar);
            cprParam.uZlibCompressRatio = (dfuLong32)lValue;
            }
        else
        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/nbhashbits") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-nbhashbits") == 0)))
            {
            dfwchar szValWChar[MAX_PATH_LENGTH];
            unsigned long lValue;

            szValWChar[0]=0;
            pCommandLine =
                CopyStrWord(pCommandLine, szValWChar,
                            (sizeof(szValWChar) / sizeof(dfwchar)) - 1);

            lValue=ConvertUnicodeStringToLong(szValWChar);
            cprParam.dfNbHashBit = (dfuLong32)lValue;
            }
        else
        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/tempdir") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-tempdir") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/tmpdir") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-tmpdir") == 0)))
            {
            dfwchar szDfsTmp[1024];
            szDfsTmp[0]=0;
            pCommandLine =
                CopyStrWord(pCommandLine, szDfsTmp,
                            (sizeof(szDfsTmp) / sizeof(dfwchar)) - 1);
            //printf("new dir tmp : '%ws'\n",szDfsTmp);
            if (szDfsTmp[0]!=0)
                SetTempDirectory(szDfsTmp);
            }
        else
        if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/memtempsize") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-memtempsize") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/memtmpsize") == 0)) ||
            ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-memtmpsize") == 0)))
            {
                dfwchar szMemTmpSize[MAX_PATH_LENGTH];
                unsigned long lValue;

                szMemTmpSize[0]=0;
                pCommandLine =
                    CopyStrWord(pCommandLine, szMemTmpSize,
                                (sizeof(szMemTmpSize) / sizeof(dfwchar)) - 1);

                lValue=ConvertUnicodeStringToLong(szMemTmpSize);
                dfsSetVirtualFileNameMaximumMemory(TRUE,((dfuLong64)lValue));
            }
        else
      if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/o") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-o") == 0)))
        {
          fOverwriteAll=TRUE;
        }
      else
      if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/br") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-br") == 0)))
        {
          pCommandLine =
            CopyStrWord(pCommandLine, szDfsBaseDirectory,
                        (sizeof(szDfsBaseDirectory) / sizeof(dfwchar)) - 1);
        }
      else
      if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/bz") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-bz") == 0)))
        {
          H_ERROR_INFO hei=NULL;
          pCommandLine =
            CopyStrWord(pCommandLine, szPortionLine,
                        (sizeof(szPortionLine) / sizeof(dfwchar)) - 1);

          char szZipFileName[0x200];
          ConvertUnicodeToAnsi(szPortionLine, szZipFileName,
                           sizeof(szZipFileName) - 1);

          BOOL fBuildFSDone = BuildFileSetFromZipFile(szZipFileName,&fsUnzippedBase,TRUE,&hei);
          if (hei!=NULL)
          {
            ShowError(hei,TRUE);
          }
          FreeErrorInfoBlock(hei);
          if (!fBuildFSDone)
          {
              printf("error in processing base zipfile\n");
              FreeFileSet(&fsUnzippedBase,TRUE);
              DeleteStaticArray_C(saParam);
              DeleteStaticArray_C(saParamRenamed);
              return FALSE;
          }
        }
      else
      if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/c") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-c") == 0)))
        {
          dfuLong32 dfSizeBuf;
          dfwcharp lpszNewText;
          if (lpszVersionComment != NULL)
              DfsFree((void*)lpszVersionName);
          dfSizeBuf = (dfUnicodeStrlen(pCommandLine)+1);
          lpszNewText = (dfwcharp)DfsMalloc((dfSizeBuf+1)*sizeof(dfwchar));
          if (lpszNewText != NULL)
          {
            pCommandLine =
                CopyStrWord(pCommandLine, lpszNewText,
                            dfSizeBuf);
            if ((*lpszNewText)==0)
            {
                DfsFree(lpszNewText);
                lpszNewText = NULL;
            }
          }
          lpszVersionComment = lpszNewText;
        }
      else
      if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/n") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-n") == 0)))
        {
          dfuLong32 dfSizeBuf;
          dfwcharp lpszNewText;
          if (lpszVersionName != NULL)
              DfsFree((void*)lpszVersionName);
          dfSizeBuf = (dfUnicodeStrlen(pCommandLine)+1);
          lpszNewText = (dfwcharp)DfsMalloc((dfSizeBuf+1)*sizeof(dfwchar));
          if (lpszNewText != NULL)
          {
            pCommandLine =
                CopyStrWord(pCommandLine, lpszNewText,
                            dfSizeBuf);
            if ((*lpszNewText)==0)
            {
                DfsFree(lpszNewText);
                lpszNewText = NULL;
            }
          }
          lpszVersionName = lpszNewText ;
        }
      else
      if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/nf") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-nf") == 0)))
        {
            pCommandLine =
                CopyStrWord(pCommandLine, szPortionLine,
                            (sizeof(szPortionLine) / sizeof(dfwchar)) - 1);

            if (lpszVersionName != NULL)
                DfsFree((void*)lpszVersionName);
            lpszVersionName = ReadTextFileContent(szPortionLine,FALSE,TRUE);
        }
      else
      if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/nfu") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-nfu") == 0)))
        {
            pCommandLine =
                CopyStrWord(pCommandLine, szPortionLine,
                            (sizeof(szPortionLine) / sizeof(dfwchar)) - 1);

            if (lpszVersionName != NULL)
                DfsFree((void*)lpszVersionName);
            lpszVersionName = ReadTextFileContent(szPortionLine,TRUE,TRUE);
        }
      else
      if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/cf") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-cf") == 0)))
        {
            pCommandLine =
                CopyStrWord(pCommandLine, szPortionLine,
                            (sizeof(szPortionLine) / sizeof(dfwchar)) - 1);

            if (lpszVersionComment != NULL)
                DfsFree((void*)lpszVersionComment);
            lpszVersionComment = ReadTextFileContent(szPortionLine,FALSE,FALSE);
        }
      else
      if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/cfu") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-cfu") == 0)))
        {
            pCommandLine =
                CopyStrWord(pCommandLine, szPortionLine,
                            (sizeof(szPortionLine) / sizeof(dfwchar)) - 1);

            if (lpszVersionComment != NULL)
                DfsFree((void*)lpszVersionComment);
            lpszVersionComment = ReadTextFileContent(szPortionLine,TRUE,FALSE);
        }
      else
      if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/v") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-v") == 0)))
        {
          dfwchar szVersionNbr[256];
          char szVersionNbrAnsi[256];
          dfuLong32 i;
          szVersionNbr[0] = 0;

          pCommandLine =
            CopyStrWord(pCommandLine, szVersionNbr,
                        (sizeof(szVersionNbr) / sizeof(dfwchar)) - 1);
          ConvertUnicodeToAnsi(szVersionNbr, szVersionNbrAnsi,
                               sizeof(szVersionNbrAnsi) - 1);
          dfVersionSpecified = (dfuLong32)atol(szVersionNbrAnsi);
          for (i=0;i<strlen(szVersionNbrAnsi);i++)
          {
              if (szVersionNbrAnsi[i]=='-')
              {
                  szVersionNbrAnsi[i]='\0';
                  dfVersionSpecified = (dfuLong32)atol(szVersionNbrAnsi);
                  dfLastVersionSpecified = (dfuLong32)atol(((char*)szVersionNbrAnsi) + i + 1);
                  break;
              }
          }
        }
      else
      if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/bv") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-bv") == 0)))
        {
          dfwchar szVersionNbr[256];
          char szVersionNbrAnsi[256];
          szVersionNbr[0] = 0;

          pCommandLine =
            CopyStrWord(pCommandLine, szVersionNbr,
                        (sizeof(szVersionNbr) / sizeof(dfwchar)) - 1);
          ConvertUnicodeToAnsi(szVersionNbr, szVersionNbrAnsi,
                               sizeof(szVersionNbrAnsi) - 1);
          dfBaseVersionSpecified = (dfuLong32)atol(szVersionNbrAnsi);
        }
      else
      if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/lf") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-lf") == 0)))
        {
            dfwcharp lpszListFile;
            pCommandLine =
                CopyStrWord(pCommandLine, szPortionLine,
                            (sizeof(szPortionLine) / sizeof(dfwchar)) - 1);

            lpszListFile = ReadTextFileContent(szPortionLine,FALSE,FALSE);
            if (lpszListFile != NULL)
                AddWilcardSortedStaticFromList(lpszListFile, saParam,fAddExistingFile,fRecurse,
                                 fAddExistingFile ? EbsCanonicalFileNameDfs.pBuf : NULL);
            if (lpszListFile != NULL)
                DfsFree((void*)lpszListFile);
        }
      else
      if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/lfu") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-lfu") == 0)))
        {
            dfwcharp lpszListFile;
            pCommandLine =
                CopyStrWord(pCommandLine, szPortionLine,
                            (sizeof(szPortionLine) / sizeof(dfwchar)) - 1);

            lpszListFile = ReadTextFileContent(szPortionLine,TRUE,FALSE);
            if (lpszListFile != NULL)
                AddWilcardSortedStaticFromList(lpszListFile, saParam,fAddExistingFile,fRecurse,
                             fAddExistingFile ? EbsCanonicalFileNameDfs.pBuf : NULL);
            if (lpszListFile != NULL)
                DfsFree((void*)lpszListFile);
        }
      else
      if (((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "/rf") == 0)) ||
          ((dfCompareUnicodeArgumentWithAnsiString(szPortionLine, "-rf") == 0)))
        {
            FILERENAMEDITEM fri;

            pCommandLine =
                CopyStrWord(pCommandLine, szPortionLine,
                            (sizeof(szPortionLine) / sizeof(dfwchar)) - 1);
            fri.FileNameOriginal = dfUnicodeCopyAlloc(szPortionLine);

            pCommandLine =
                CopyStrWord(pCommandLine, szPortionLine,
                            (sizeof(szPortionLine) / sizeof(dfwchar)) - 1);
            fri.FileNameRenamed = dfUnicodeCopyAlloc(szPortionLine);
            BOOL fInserted = FALSE;

            if ((fri.FileNameOriginal != NULL) && (fri.FileNameRenamed != NULL))
                if (((*fri.FileNameOriginal) != 0) && ((*fri.FileNameRenamed) != 0))
                {
                    fInserted = InsertSortedSA(saParamRenamed, &fri);
                }

            if (!fInserted)
            {
                if (fri.FileNameOriginal != NULL)
                    DfsFree((void*)fri.FileNameOriginal);
                if (fri.FileNameRenamed != NULL)
                    DfsFree((void*)fri.FileNameRenamed);
            }



            // todo : error
        }
      else
        {
          //AddWilcardToFileSet(szPortionLine, &fsParam,fRecurse);
            //printf("+++\n");
          AddWilcardSortedStatic(szPortionLine, saParam,fAddExistingFile,fRecurse,
                              fAddExistingFile ? EbsCanonicalFileNameDfs.pBuf : NULL);
            //printf("---\n");
        }

      //printf("next word: '%ws', rest='%ws'\n", szPortionLine, pCommandLine);
    }

  if (fCmdL || fCmdLV)
  {
    //printf("version = %u-%u\n",dfVersionSpecified,dfLastVersionSpecified);
    fRet = PrintDfsListArraySelect(szDfsFileName,dfVersionSpecified,dfLastVersionSpecified,saParam,fCmdLV,fComposition);
  }
  else
  if ((fCmdCR) || (fCmdCZ) || (fCmdCRz) || (fCmdCZz) || (fCmdT) || (fCmdC) ||
      (fCmdX) || (fCmdXz) || (fCmdI) || (fCmdIz) || (fCmdAppVer) || (fCmdA) || (fCmdD) || (fCmdDV) || (fCmdRS))
    {
      DFSFILE DfsFile = NULL;
      DFSFILEINFOPARAM DfsFileParam;
      DfsFileParam.sizeStruct = sizeof(DfsFileParam);
      DfsFileParam.dfStatus = 0;
      if ((fCmdCR) || (fCmdCZ) || (fCmdCRz) || (fCmdCZz))
        DfsFileParam.dfStatus = DFS_NEWFILE | DFS_WRITABLE;
      else if ((fCmdT) || (fCmdX) || (fCmdXz) || (fCmdC))
        DfsFileParam.dfStatus = DFS_READABLE;
      if ((fCmdI) || (fCmdIz) || (fCmdA) || (fCmdD) || (fCmdDV) || (fCmdRS) || (fCmdAppVer))
        DfsFileParam.dfStatus = DFS_WRITABLE;
      DfsFileParam.filename = szDfsFileName;

      DfsFileOpen(&DfsFileParam, &DfsFile,NULL);

      ConvertOldDirectoryCommentStorage(DfsFile,NULL);
      /*
      if (!DfsFeatureParam.fStripIdenticalBody)
        SetDfsExtendedMode(DfsFile,TRUE);
        */
      SetDfsFeatureParam(DfsFile,&DfsFeatureParam);

      if (DfsFile == NULL)
      {
        printf("Can't open ");
        DispOutUnicodeString(szDfsFileName);
        printf("\n");
      }


#ifndef SVF_EXTRACT_ONLY
      if (fCmdAppVer && (DfsFile != NULL))
      {
        DFSFILE DfsFileWhereRead = NULL;
        DFSFILEINFOPARAM DfsFileParamWhereRead;
        DfsFileParamWhereRead.sizeStruct = sizeof(DfsFileParamWhereRead);
        DfsFileParamWhereRead.dfStatus = DFS_READABLE;

        if (GetNbElemSA(saParam) >= 1)
        {
            dfuLong32 dfNbDirWhereAdd=0;
            dfuLong32 dfNbDirWhereRead=0;
            PDIRINFO *pDirInfoWhereAdd = NULL;
            PDIRINFO *pDirInfoDfsFileWhereRead = NULL;
            const FILEUSERITEM *pfi = (const FILEUSERITEM *) GetElemPtrSA(saParam,0);
            DfsFileParamWhereRead.filename = pfi ->FileNameOnDisk;
            DfsFileOpen(&DfsFileParamWhereRead, &DfsFileWhereRead,NULL);

            if (DfsFileWhereRead == NULL)
            {
                dfwcharp pFileName = (dfwcharp)DfsMalloc((dfUnicodeStrlen(DfsFileParamWhereRead.filename)+0x10)*sizeof(dfwchar));
                if (AppendExtSvfIfNeeded(pFileName))
                  DfsFileOpen(&DfsFileParamWhereRead, &DfsFileWhereRead,NULL);
                DfsFree(pFileName);
            }

            if (DfsFileWhereRead != NULL)
            {
              ConvertOldDirectoryCommentStorage(DfsFileWhereRead,NULL);
              pDirInfoWhereAdd = ReadAllDirInfo(DfsFile,&dfNbDirWhereAdd,READ_ALL_DIR,NULL);
              AdaptDfsFileFeature(DfsFile,(const PCDIRINFO*)pDirInfoWhereAdd,dfNbDirWhereAdd);
            }

            if (pDirInfoWhereAdd != NULL)
              pDirInfoDfsFileWhereRead = ReadAllDirInfo(DfsFileWhereRead,&dfNbDirWhereRead,READ_ALL_DIR,NULL);


            if (DfsFileWhereRead == NULL)
            {
                printf("Can't open ");
                DispOutUnicodeString(DfsFileParamWhereRead.filename);
                printf("\n");
            }
            else
            if ((pDirInfoWhereAdd == NULL) || (dfNbDirWhereAdd == 0))
            {
                printf("error reading ");
                DispOutUnicodeString(DfsFileParam.filename);
                printf("\n");
            }
            else
            if ((pDirInfoDfsFileWhereRead == NULL) || (dfNbDirWhereRead == 0))
            {
                printf("error reading ");
                DispOutUnicodeString(DfsFileParamWhereRead.filename);
                printf("\n");
            }
            else
            {
                dfuLong32 i;

                dfuLong32 dfError;

                printf("Appending version from ");
                DispOutUnicodeString(DfsFileParamWhereRead.filename);
                printf(" on ");
                DispOutUnicodeString(DfsFileParam.filename);
                printf("\n");

                for (i=0;(i+1<dfNbDirWhereRead);i++)
                {
                    //PDIRINFO pDirInfoWhereReadTry = NULL;
                    dfuLong32 dfNbItemConversionMapList;
                    dfuLong32* pPositionConversionMapList;


                    dfNbItemConversionMapList = 0;

                    pPositionConversionMapList=GetPositionConversionMapList(*(pDirInfoDfsFileWhereRead+i),
                                                        *(pDirInfoWhereAdd+dfNbDirWhereAdd-1),&dfNbItemConversionMapList);


                    if (pPositionConversionMapList != NULL)
                    {
                      printf("version number %u of ",i);
                      DispOutUnicodeString(DfsFileParamWhereRead.filename);
                      printf(" is compatible with latest version of ");
                      DispOutUnicodeString(DfsFileParam.filename);
                      printf("\n");
                        // DoAppendDfs
                      dfError = DoAppendDfs(DfsFile,
                                                dfNbDirWhereAdd,/*
                                                pDirInfoWhereAdd,*/
                                                DfsFileWhereRead,
                                                /*dfNbDirWhereRead,*/
                                                (const PCDIRINFO*)pDirInfoDfsFileWhereRead,
                                                i+1,
                                                dfNbDirWhereRead-(i+1),
                                                pPositionConversionMapList,dfNbItemConversionMapList,
                                                NULL,NULL,
                                                0,0,
                                                NULL);


                        if (dfError != DFS_SUCCESS)
                        {
                            printf("error in writing/appending\n");
                        }

                        DfsFree(pPositionConversionMapList);
                        break;
                    }
                }
                if ((i+1) == dfNbDirWhereRead)
                    printf("no compatible version found\n");
            }

            if (pDirInfoWhereAdd != NULL)
                FreeAllDirInfo(pDirInfoWhereAdd,dfNbDirWhereAdd);


            if (pDirInfoDfsFileWhereRead != NULL)
                FreeAllDirInfo(pDirInfoDfsFileWhereRead,dfNbDirWhereRead);


            if (DfsFileWhereRead != NULL)
            {
                DfsClose(DfsFileWhereRead, NULL);
                DfsFileWhereRead = NULL;
            }
        }
      }

      if (((fCmdCR) || (fCmdCZ)) && (DfsFile != NULL))
        {
          FILETOADD *pFileToAdd = NULL;
          dfuLong32 dfNbFileToAdd = 0;
          dfuLong32 dfFileToAddStepAlloc = 0x10;
          dfuLong32 dfNbFileToAddAllocated = 0;
          dfuLong32 i, dfErr;

          for (i = 0; i < GetNbElemSA(saParam); i++)
            {
              FILETOADD fta;
              const FILEUSERITEM *pfi = (const FILEUSERITEM *) GetElemPtrSA(saParam,i);
              fta.filename_ondisk = pfi->FileNameOnDisk;
              fta.filename_tostore = pfi->FileNameOnArchive;
              fta.filename_prevversionondisk = NULL;
              fta.hRamDifToFlushPatch = NULL;
              fta.dfPreviousVersionFilePosition = 0;
              fta.fForceRecopyPrevious = FALSE;
              fta.fIgnore = FALSE;
              fta.fForceDate = FALSE;
              fta.hAddTags = FALSE;
              fta.pReserved = NULL;
              fta.dfForceRecopyOrRawCopySize = 0;
              fta.dfForceRecopyOrRawCopyCrc32 = 0;
              fta.fForceRecopyOrRawCopyMd5Present = FALSE;
              fta.fForceRecopyOrRawCopySha1Present = FALSE;
              fta.fForceRecopyOrRawCopySha256Present = FALSE;
              fta.fWritingRaw = FALSE;


              pFileToAdd = (FILETOADD *)
                AddArrayElem(pFileToAdd, &dfNbFileToAdd,
                             &dfNbFileToAddAllocated, dfFileToAddStepAlloc,
                             sizeof(FILETOADD), &fta, 1);
            }

          printf("creating ");
          DispOutUnicodeString(szDfsFileName);
          printf(" with %u files\n",GetNbElemSA(saParam));
          //blabla
          dfErr = InsertDirectoryinDfsFile(DfsFile,
                                           fCmdCR ? TYPEDIR_FILECRCONLY :
                                           TYPEDIR_FILEINSERTING_DEFLATE,
                                           dfNbFileToAdd, pFileToAdd, FALSE,
                                           lpszVersionName,
                                           lpszVersionComment,
                                           &cprParam,
                                           ProgressCallBackCreatePatchConsole, NULL,
                                           NULL);
          if (dfErr != DFS_SUCCESS)
          {
            printf("error in writing ");
            DispOutUnicodeString(szDfsFileName);
            printf("\n");
          }
          DeleteArray(pFileToAdd);
        }

      if (((fCmdCRz) || (fCmdCZz)) && (DfsFile != NULL))
        {
          FILETOADDARRAY ftaArray;
          dfuLong32 dfErr=0;
          if (GetNbElemSA(saParam) != 1)
          {
              FreeFileSet(&fsUnzippedBase,TRUE);
              DeleteStaticArray_C(saParam);
              DeleteStaticArray_C(saParamRenamed);
              return FALSE;
          }
          const FILEUSERITEM * pfuizip=(const FILEUSERITEM *) GetElemPtrSA(saParam,0);



            char szZipFileName[0x200];
            ConvertUnicodeToAnsi(pfuizip->FileNameOnDisk, szZipFileName,
                               sizeof(szZipFileName) - 1);

            ClearFileToAddArrayWithDelete(&ftaArray,FALSE,FALSE);
            if (!BuildFtaArrayFromZipfile(&ftaArray,szZipFileName))
            {
                FreeFileSet(&fsUnzippedBase,TRUE);
                ClearFileToAddArrayWithDelete(&ftaArray,TRUE,TRUE);
                printf("error in processing zipfile\n");
                DeleteStaticArray_C(saParam);
                DeleteStaticArray_C(saParamRenamed);
                return FALSE;
            }


          printf("creating ");
          DispOutUnicodeString(szDfsFileName);
          printf(" with %u files from zip\n",ftaArray.dfNbFileToAdd);
          //blabla
          dfErr = InsertDirectoryinDfsFile(DfsFile,
                                           fCmdCRz ? TYPEDIR_FILECRCONLY :
                                           TYPEDIR_FILEINSERTING_DEFLATE,
                                           ftaArray.dfNbFileToAdd,ftaArray.pFileToAdd,
                                           /*dfNbFileToAdd, pFileToAdd, */FALSE,
                                           lpszVersionName,
                                           lpszVersionComment,
                                           &cprParam,
                                           ProgressCallBackCreatePatchConsole, NULL,
                                           NULL);
          if (dfErr != DFS_SUCCESS)
          {
            printf("error in writing ");
            DispOutUnicodeString(szDfsFileName);
            printf("\n");
          }
          ClearFileToAddArrayWithDelete(&ftaArray,TRUE,TRUE);
        }
#endif

      if ((fCmdC) && (DfsFile != NULL))
        {
          FILETOCHECK *pFileToCheck;
          dfuLong32 i;
          FILESET fs;
          dfuLong32 dfNumDir = 0;
          dfuLong32 dfNbDir = 0;
          BOOL fComplete = FALSE;
          DfsGetNbDir(DfsFile, &dfNbDir,NULL);

          fRet=FALSE;

          if (dfVersionSpecified != VALUE_UNKNOWN)
            dfNumDir = dfVersionSpecified;
          else if (dfNbDir > 0)
            dfNumDir = 0;//dfNbDir - 1;
          InitFileSet(&fs);

          CreateFileSetForVersionInDirectory(DfsFile, &fs, dfNumDir,
                                             szDfsBaseDirectory,&fsUnzippedBase,
                                             NULL,
                                             &fComplete,
                                             TRUE, TRUE, TRUE, TRUE, NULL,FALSE,FALSE,NULL);

          pFileToCheck =
            (FILETOCHECK *) DfsMalloc(sizeof(FILETOCHECK) *
                                      (fs.dfNbFileItem + 1));

          for (i = 0; i < fs.dfNbFileItem; i++)
            {
              (pFileToCheck + i)->filename_ondisk_tocompare =
                (fs.pFileItem + i)->FileNameOnDisk;
              (pFileToCheck + i)->filename_archive = (fs.pFileItem + i)->FileNameOnArchive;
              (pFileToCheck + i)->fIgnore =
                ((pFileToCheck + i)->filename_ondisk_tocompare) == NULL;
            }

          if (CheckDirectoryCrcWithRealFileSet(DfsFile, dfNumDir, fs.dfNbFileItem,
                                           pFileToCheck,
                                           fVerbose ? ProgressCallBackCheckDir : NULL,
                                           pFileToCheck,NULL) == 0)
          {
              fRet=TRUE;
              for (i = 0; i < fs.dfNbFileItem; i++)
                  if (!((pFileToCheck + i)->fIsIdentical))
                      fRet=FALSE;
          }

          if (fRet) printf("version base comparing ok\n");
          if (!fRet) printf("version base comparing mismatch\n");
          DfsFree(pFileToCheck);
          FreeFileSet(&fs,FALSE);
        }

      if ((fCmdX || fCmdXz) && fCmdIXNew && (DfsFile != NULL))
      {
          BOOL fRetDoMulti;
          PDIRINFO* pDirInfo;
          PCDIRINFO pCurDirInfo;
          EXTRACTINGMAPITEM* lpfExtractItemMap=NULL;
          dfuLong32 dfNbDir,dfDirExtract;
          dfwcharpc wchBaseDirExtract=NULL;
          BOOL fBaseDirectorySelected=FALSE;
          dfuLong32 dfFirstParam = (fCmdXz ? 1 : 0);
          FILESET* pfsDest=NULL; // only for zip


          if ((fCmdXz) && ((ExtractingPreferedMethod == ExtractInPlace) || (ExtractingPreferedMethod == ExtractInPlaceNoChecksum)))
              ExtractingPreferedMethod = ExtractByMerging;
          if ((fsUnzippedBase.dfNbFileItem != 0) && ((ExtractingPreferedMethod == ExtractInPlace) || (ExtractingPreferedMethod == ExtractInPlaceNoChecksum)))
              ExtractingPreferedMethod = ExtractByMerging;
          //char szZipFileName[0x200];
          dfwcharpc szZipFileNameUnicode=NULL;

          if ((fCmdXz) && (GetNbElemSA(saParam)==0))
          {
              FreeFileSet(&fsUnzippedBase,TRUE);
              printf("error : a zipfile must be specified\n");
              DeleteStaticArray_C(saParam);
              DeleteStaticArray_C(saParamRenamed);
              return FALSE;
          }


          if (fCmdXz)
          {
              const FILEUSERITEM * pfuizip=(const FILEUSERITEM *) GetElemPtrSA(saParam,0);
              szZipFileNameUnicode = pfuizip->FileNameOnArchive;
              //ConvertUnicodeToAnsi(szZipFileNameUnicode, szZipFileName,
              //                     sizeof(szZipFileName) - 1);
          }

          if (szDfsBaseDirectory[0]!=0)
                fBaseDirectorySelected=TRUE;

          //if (fsUnzippedBase!=NULL)
              if (fsUnzippedBase.dfNbFileItem!=0)
                  fBaseDirectorySelected=TRUE;

          if (dfBaseVersionSpecified == VALUE_UNKNOWN)
            dfBaseVersionSpecified = 0;

          pDirInfo=ReadAllDirInfo(DfsFile,&dfNbDir,READ_ALL_DIR,NULL);
/*
          if ((fOnlyExistLatest) && (dfNbDir>0))
            RemoveFileNotInDirInfoFromList(*(pDirInfo + dfNbDir-1),&saParam);
*/
          if (dfVersionSpecified == VALUE_UNKNOWN)
            dfDirExtract = dfNbDir - 1;
          else
            dfDirExtract = dfVersionSpecified;

          if (dfDirExtract>=dfNbDir)
          {
              FreeFileSet(&fsUnzippedBase,TRUE);
              printf("invalid version number\n");
              DeleteStaticArray_C(saParam);
              DeleteStaticArray_C(saParamRenamed);
              return 1;
          }

          pCurDirInfo = *(pDirInfo + dfDirExtract);

          if (pDirInfo!=NULL)
            lpfExtractItemMap = (EXTRACTINGMAPITEM*)DfsMalloc(sizeof(EXTRACTINGMAPITEM)*(pCurDirInfo ->dfNbFile + 1));

          if (lpfExtractItemMap!=NULL)
          {
              dfuLong32 j,k;
              OVERWRITEPARAM ovr;
              ovr.fYesAll = FALSE;
              ovr.fNoAll = FALSE;

              H_ERROR_INFO hei=NULL;
              for (j=0;j<pCurDirInfo -> dfNbFile;j++)
              {
                  const dfwcharpc dfFileNameToTest = ((pCurDirInfo->pFileInDirInfo)+j)->FileName;

                  *(lpfExtractItemMap+j)=ExtractNone;
                  if (GetNbElemSA(saParam) == dfFirstParam)
                      *(lpfExtractItemMap + j) = ExtractingPreferedMethod;
                  else
                    for (k=dfFirstParam;k<GetNbElemSA(saParam);k++)
                    {
                        BOOL fTestWilcard = TRUE;
                        const FILEUSERITEM *pfi = (const FILEUSERITEM *) GetElemPtrSA(saParam, k);
                        //printf("modern request extract %ws, test %ws : ",pfi->FileNameOnArchive,dfFileNameToTest);
                        fTestWilcard=TestWildcardAgaintFileInArchive(dfFileNameToTest,pfi->FileNameOnArchive);
                        //printf(fTestWilcard ? "Y\n":"N\n");
                        if (fTestWilcard)
                        {
                            *(lpfExtractItemMap + j) = ExtractingPreferedMethod;
                            break;
                        }
                    }
              }

              ovr.fNoAll = FALSE;
              ovr.fYesAll = fOverwriteAll;

              fRetDoMulti=DoMultiExtracting(DfsFile,wchBaseDirExtract,
                       fCmdXz ? &pfsDest : NULL,
                       fCmdXz, /* parameter : fTempDestExtr */
                       dfDirExtract,
                       pDirInfo,
                       fBaseDirectorySelected,dfBaseVersionSpecified,szDfsBaseDirectory,&fsUnzippedBase,
                       pCurDirInfo -> dfNbFile,lpfExtractItemMap,
                       NULL,//EXTRACTINGMAPINFO* pExtractingMapInfo,
                       NULL,NULL,//tSetExtractPosCallBack pSetExtractPosCallBack,dfvoidp dfUserPtr,
                       ConfirmConsoleBeforeCreatingFile,&ovr,//tConfirmBeforeCreatingFile pConfirmBeforeCreatingFile, dfvoidp dfUserPtrBeforeCreatingFile,
                       fncExtractingFileWorkingEventConsole,NULL,//tExtractingFileWorkingEvent pExtractingFileWorkingEvent, dfvoidp dfUserExtractingFileWorkingEvent,
                       0,0,//dfuLong32 dwMinProgress,dfuLong32 dwMaxProgress,
                       FALSE,FALSE,&hei);//BOOL fFlatExtracting, H_ERROR_INFO * pei);

              DfsFree(lpfExtractItemMap);
              if (hei!=NULL)
              {
                ShowError(hei,TRUE);
              }
              FreeErrorInfoBlock(hei);


            if (fRetDoMulti && fCmdXz)
            {
                dfwcharp pZipComment=NULL;
                dfuLong32 i;

                {
                    DFTAGBLOCKFLOAT TagBlockFloat;
                    dfvoidp TagBufDirName = NULL;
                    dfvoidp TagBufDirComment = NULL;
                    dfuLong32 TagSizeDirName,TagSizeDirComment;
                    TagSizeDirName=TagSizeDirComment=0;

                    TagBlockFloat = GetDfsTagBlockFloat(DfsFile,NULL);
                    if (TagBlockFloat != NULL)
                    {
                        GetTagBlockFloat(TagBlockFloat,dfDirExtract,FLOATNUM_NOSPECIFIC,
                                             DFSTAG_DIR_NAME_FLOAT,&TagBufDirName, &TagSizeDirName);
                        GetTagBlockFloat(TagBlockFloat,dfDirExtract,FLOATNUM_NOSPECIFIC,
                                             DFSTAG_DIR_COMMENT_FLOAT,&TagBufDirComment, &TagSizeDirComment);
                    }
                    if (TagSizeDirName + TagSizeDirComment>0)
                    {
                        pZipComment = (dfwcharp)DfsMalloc(TagSizeDirName + TagSizeDirComment + 0x10);
                        dfuLong32 uln=0;
                        if (TagSizeDirName>0)
                        {
                          DfsMemcpy(pZipComment,TagBufDirName,TagSizeDirName);
                          uln = dfUnicodeStrlen(pZipComment);
                        }
                        if (TagSizeDirComment>0)
                        {
                          dfwchar cr,lf;

                          cr = L'\x0d';
                          lf = L'\x0a';

                          *(pZipComment+uln) = cr;
                          *(pZipComment+uln+1) = lf;
                          DfsMemcpy(pZipComment+uln+2,TagBufDirComment,TagSizeDirComment);
                        }
                    }
                }

                for (i=0;(i<pfsDest->dfNbFileItem) && (fRet);i++)
                    if (((pfsDest->pFileItem + i)->ExtAction != ExtActionIgnore))
                    //if (!((pfsDest->pFileItem + i)->fIgnore))
                        (pfsDest->pFileItem + i)->fForceDate=TRUE;

                fRet = BuildZipFileFromFileSet(pfsDest,szZipFileNameUnicode,Z_DEFAULT_COMPRESSION,
                                                 pZipComment,
                                                 NULL,NULL,
                                                 //SetPosCallBack,&guiItem,
                                                 100,200,&hei);

                FreeFileSet(pfsDest,TRUE);
                DfsFree(pfsDest);

                if (pZipComment!=NULL)
                    DfsFree(pZipComment);
            }
          }

          FreeAllDirInfo(pDirInfo,dfNbDir);
      }

      if (((fCmdX && (!fCmdIXNew)) || fCmdT || ((fCmdI || (fCmdIz) )&& (dfVersionSpecified == VALUE_UNKNOWN))) && (DfsFile != NULL))
        {
          AdaptDfsFileFeatureForOpenedDfs(DfsFile);

          dfuLong32 dfNbDir = 0;
          FILESET *pfsOrg = NULL;
          FILESET *pfsDest = NULL;
          dfuLong32 i;
          dfuLong32 dfStartVersionExtract;
          BOOL fNewFileSetTemp;
          dfuLong32 dfError = DFS_SUCCESS;
          dfuLong32 dfTypeDir;
          PDIRINFO* pDirInfo;

          if (dfBaseVersionSpecified == VALUE_UNKNOWN)
            dfBaseVersionSpecified = 0;

          pDirInfo = ReadAllDirInfo(DfsFile,&dfNbDir,READ_ALL_DIR,NULL);

          if ((fOnlyExistLatest) && (dfNbDir>0))
              RemoveFileNotInDirInfoFromList(*(pDirInfo + dfNbDir-1),saParam);
          else
          if ((fOnlyExistVersionSpecified) && (dfVersionOnlyExist != VALUE_UNKNOWN) && (dfVersionOnlyExist<dfNbDir))
              RemoveFileNotInDirInfoFromList(*(pDirInfo + dfVersionOnlyExist),saParam);

          if ((dfVersionSpecified == VALUE_UNKNOWN) || fCmdI)
            dfVersionSpecified = dfNbDir - 1;
/*
        fsOrg = (FILESET*)DfsMalloc(sizeof(FILESET));
        InitFileSet(fsOrg);
        */
          dfStartVersionExtract = dfBaseVersionSpecified;

          fNewFileSetTemp = (dfBaseVersionSpecified != dfVersionSpecified)
            || fCmdT || fCmdI;

          dfError =
            DfsGetDirType(DfsFile, dfBaseVersionSpecified, &dfTypeDir,NULL);



          if (dfError == DFS_SUCCESS)
            if ((dfTypeDir != TYPEDIR_FILEINSERTING_DEFLATE) &&
                (dfTypeDir != TYPEDIR_FILEINSERTING_STORE))
              {
                BOOL fAllFound = TRUE;
                pfsDest = (FILESET *) DfsMalloc(sizeof(FILESET));
                InitFileSet(pfsDest);
                dfError =
                  CreateFileSetForVersionInDirectory(DfsFile, pfsDest,
                                                     dfBaseVersionSpecified,
                                                     szDfsBaseDirectory,&fsUnzippedBase,
                                                     *(pDirInfo+dfBaseVersionSpecified),
                                                     &fAllFound,
                                                     TRUE, !fNewFileSetTemp,
                                                     TRUE, FALSE, &dfTypeDir,FALSE,FALSE,NULL);
                dfStartVersionExtract++;
                if ((!fAllFound) && (dfError == DFS_SUCCESS))
                    dfError=DFS_ERROR_BAD_PARAMETER;
              }

          for (i = dfStartVersionExtract;
               (i <= dfVersionSpecified) && (dfError == DFS_SUCCESS); i++)
            {
              if (pfsOrg != NULL)
                {
                  FreeFileSet(pfsOrg, TRUE);
                  DfsFree(pfsOrg);
                }
              pfsOrg = pfsDest;

              pfsDest = (FILESET *) DfsMalloc(sizeof(FILESET));
              InitFileSet(pfsDest);

              fNewFileSetTemp = (i != dfVersionSpecified) || fCmdT || fCmdI || fCmdIz;

              dfError =
                CreateFileSetForVersionInDirectory(DfsFile, pfsDest, i,GetUnicodeStringEmpty(),NULL,
                                                   *(pDirInfo+i),
                                                   NULL, TRUE,
                                                   !fNewFileSetTemp, FALSE,
                                                   FALSE, &dfTypeDir,(!pfsDest->fTempFile),FALSE,NULL);
              pfsDest->fTempFile = fNewFileSetTemp;

              if (!fNewFileSetTemp)
              {
                  // need select file to extract
                  dfuLong32 j,k;
                  for (j=0;j<pfsDest->dfNbFileItem;j++)
                  {
                    const dfwcharpc dfFileNameToTest = ((pfsDest->pFileItem) + j)->FileNameOnArchive;
                    ((pfsDest->pFileItem) + j)->ExtAction=ExtActionIgnore;
                    if (GetNbElemSA(saParam)==0)
                        ((pfsDest->pFileItem) + j)->ExtAction=ExtActionExtractContent;
                    else
                        for (k=0;k<GetNbElemSA(saParam);k++)
                        {
                            BOOL fTestWilcard;
                            const FILEUSERITEM *pfi = (const FILEUSERITEM *) GetElemPtrSA(saParam,k);

                            fTestWilcard=TestWildcardAgaintFileInArchive(dfFileNameToTest,
                                                                         pfi->FileNameOnArchive);

                            if (fTestWilcard)
                            {
                                ((pfsDest->pFileItem) + j)->ExtAction=ExtActionExtractContent;
                                break;
                            }
                        }
                  }
              }

              if (dfError != DFS_SUCCESS)
                printf("error create fileset %u\n", i);
              if (dfError == DFS_SUCCESS)
                {
                  //PDIRINFO pDirInfo=NULL;
                  //dfError = ReadDirectoryInfo(DfsFile, i, (&pDirInfo) , NULL, NULL, NULL);
                  if (dfError == DFS_SUCCESS)
                  dfError =
                    ExtractPatch(DfsFile, pfsOrg, pfsDest, i, *(pDirInfo+i),
                                 fNewFileSetTemp, fNewFileSetTemp,FALSE,
                                 GetUnicodeStringDot(),NULL,NULL,
                                 NULL, NULL,
                                 NULL);

                  //if (pDirInfo!=NULL)
                  //    FreeDirectoryInfo((&pDirInfo), NULL);

                  if (!pfsDest->fTempFile)
                  {
                  }

                  if (dfError != DFS_SUCCESS)
                    printf("error ExtractPatch %u\n", i);
                }
            }

          FreeAllDirInfo(pDirInfo,dfNbDir);
          if (pfsOrg != NULL)
            {
              FreeFileSet(pfsOrg, TRUE);
              DfsFree(pfsOrg);
            }
#ifndef SVF_EXTRACT_ONLY
          if ((fCmdI) && (dfError == DFS_SUCCESS))
            {
              dfError = InsertNewPatch(DfsFile, pfsDest, saParam, saParamRenamed,
                                       lpszVersionName,
                                       lpszVersionComment,&cprParam);
            }
          if ((fCmdIz) && (dfError == DFS_SUCCESS))
            {
              dfError = InsertNewPatchZip(DfsFile, pfsDest, saParam, saParamRenamed,
                                       lpszVersionName,
                                       lpszVersionComment,&cprParam);
            }
#endif
          if (pfsDest != NULL)
            {
              FreeFileSet(pfsDest, pfsDest->fTempFile);
              DfsFree(pfsDest);
            }
        }

#ifndef SVF_EXTRACT_ONLY
      if (fCmdA)
        {
            INSERTFILEINVERSION_DATA InsertFileInVersionData;
            dfuLong32 i,dfNbDir;
            PDIRINFO* pDirInfo;
            VERSIONTOADD_REMIX *pVersionRemix=NULL;
            dfuLong32 dfError = DFS_SUCCESS;


            DfsClearStruct(&InsertFileInVersionData, 0, sizeof(InsertFileInVersionData));

            pDirInfo = ReadAllDirInfo(DfsFile,&dfNbDir,READ_ALL_DIR,NULL);
            AdaptDfsFileFeature(DfsFile,(const PCDIRINFO*)pDirInfo,dfNbDir);

            if ((fOnlyExistLatest) && (dfNbDir>0))
                RemoveFileNotInDirInfoFromList(*(pDirInfo + dfNbDir-1),saParam);
            else
            if ((fOnlyExistVersionSpecified) && (dfVersionOnlyExist != VALUE_UNKNOWN) && (dfVersionOnlyExist<dfNbDir))
                RemoveFileNotInDirInfoFromList(*(pDirInfo + dfVersionOnlyExist),saParam);

            if ((dfVersionSpecified == VALUE_UNKNOWN) && (dfNbDir>0))
            {
                //printf("use latest version %u\n",dfNbDir-1);
                dfVersionSpecified = dfNbDir-1;
            }

            if (pDirInfo != NULL)
            {
                InsertFileInVersionData.pFileToAddInVersions = (FILETOADDINVERSIONS*)DfsMalloc(sizeof(FILETOADDINVERSIONS)*(GetNbElemSA(saParam)+1));
                InsertFileInVersionData.dfNbFileToAddInVersion = GetNbElemSA(saParam)*1;
                InsertFileInVersionData.dfNbVersion = dfNbDir;
                InsertFileInVersionData.pVersionMap = (BOOL*)DfsMalloc(sizeof(BOOL)*(dfNbDir+1));
                InsertFileInVersionData.ReplacementFileOption=REPLACEMENTFILEOPTION_UIASKING;//REPLACEMENTFILEOPTION_REPLACE;
                InsertFileInVersionData.fAskLinkRenamedVersionBefore=FALSE;
                InsertFileInVersionData.fAskLinkRenamedVersionAfter=FALSE;
            }

            if ((pDirInfo == NULL) || (InsertFileInVersionData.pVersionMap == NULL) ||
                                  (InsertFileInVersionData.pFileToAddInVersions == NULL))
            {
                fRet=FALSE;
            }
            else
            {
                dfuLong32 dfLastVersionUse = dfLastVersionSpecified;
                if (dfLastVersionUse == VALUE_UNKNOWN)
                    dfLastVersionUse = dfVersionSpecified;
                if ((dfLastVersionUse >= dfNbDir))
                    dfLastVersionUse = dfNbDir-1;

                if (dfBaseVersionSpecified == VALUE_UNKNOWN)
                    dfBaseVersionSpecified = 0;
                printf("Adding files on existing version number %u",dfVersionSpecified);
                if (dfLastVersionUse>dfVersionSpecified)
                    printf(" to %u",dfLastVersionUse);
                printf("\n");
                for (i = 0;i<InsertFileInVersionData.dfNbFileToAddInVersion;i++)
                {
                    FILETOADDINVERSIONS* pFileToAddInVersionsCur = (InsertFileInVersionData.pFileToAddInVersions)+i;
                    const FILEUSERITEM *pfi = (const FILEUSERITEM *) GetElemPtrSA(saParam,i);
                    pFileToAddInVersionsCur->dfFileNameOnDisk = pfi->FileNameOnDisk;
                    pFileToAddInVersionsCur->dfFileNameToStore = pfi->FileNameOnArchive;
                    DispOutUnicodeString(pFileToAddInVersionsCur->dfFileNameToStore);
                    printf("\n");
                }

                for (i = 0; i < dfNbDir; i++)
                {
                    *((InsertFileInVersionData.pVersionMap)+i)=FALSE;

                    if ((i>=dfVersionSpecified) && (i<=dfLastVersionUse))
                            *((InsertFileInVersionData.pVersionMap)+i)=TRUE;
                }

                if (fRet)
                    pVersionRemix = CreateInsertingForReplaceCurrentDfs(dfNbDir,  pDirInfo,
                                                                        &InsertFileInVersionData,
                                                                        dfVersionSpecified,&dfError,
                                                                        CallBackAskReplacing,NULL);
            }

            if (pVersionRemix!=NULL)
            {
                fRet =
                    ExecuteRemix(dfNbDir,pVersionRemix,&DfsFile,
                       szDfsFileName,(szTargetDfs[0]==0) ? NULL : szTargetDfs,szDfsBaseDirectory,&fsUnzippedBase,
                       dfNbDir, (const PCDIRINFO*)pDirInfo, dfBaseVersionSpecified, fReuseOldPatch, fRawAccepted,&cprParam, &DfsFeatureParam,
                       fChangeFirstVersionStorage,fFirstVersionAsReferenceRequested);

                for (i = 0; i < dfNbDir; i++)
                {
                    FILETOCOPYINFO_REMIX *pFileCopyInfo = (pVersionRemix + i)->pFileCopyInfo;
                    DfsFree(pFileCopyInfo);
                    DfsFree((pVersionRemix + i)->pfta);
                }
                DfsFree(pVersionRemix);
            }

            FreeAllDirInfo(pDirInfo,dfNbDir);
            pDirInfo = NULL;

            if (InsertFileInVersionData.pFileToAddInVersions != NULL)
              DfsFree(InsertFileInVersionData.pFileToAddInVersions);

            if (InsertFileInVersionData.pVersionMap != NULL)
              DfsFree(InsertFileInVersionData.pVersionMap);
        }


      if (fCmdD && (dfVersionSpecified != VALUE_UNKNOWN))
        {
            dfuLong32 i,j,dfNbDir;
            PDIRINFO* pDirInfo;
            VERSIONTOADD_REMIX *pVersionRemix=NULL;
            dfuLong32 dfNbVersionRemix = 0;
            //dfuLong32 dfError = DFS_SUCCESS;

            if (dfBaseVersionSpecified == VALUE_UNKNOWN)
                dfBaseVersionSpecified = 0;

            pDirInfo = ReadAllDirInfo(DfsFile,&dfNbDir,READ_ALL_DIR,NULL);
            AdaptDfsFileFeature(DfsFile,(const PCDIRINFO*)pDirInfo,dfNbDir);

            if (pDirInfo != NULL)
            {
                pVersionRemix =
                    (VERSIONTOADD_REMIX *) DfsMalloc(sizeof(VERSIONTOADD_REMIX) * (dfNbDir + 1));
            }

            if (pVersionRemix != NULL)
            {
                for (i = 0; i < dfNbDir; i++)
                    //if (*(lpVersionMap + i))
                    {
                        VERSIONTOADD_REMIX *pFileCopyInfoCur = pVersionRemix + dfNbVersionRemix;
                        PCDIRINFO pDirOrg = *(pDirInfo + i);
                        dfuLong32 dfNbFileDeleted=0;

                        pFileCopyInfoCur->dfNumVersionPreviousSvf = i;
                        pFileCopyInfoCur->dfNbPreviousFileInMask = pDirOrg->dfNbFile;
                        pFileCopyInfoCur->dfNbFileToAdd = 0;
                        pFileCopyInfoCur->pFileCopyInfo = (FILETOCOPYINFO_REMIX *)
                            DfsMalloc(sizeof(FILETOCOPYINFO_REMIX) * (pDirOrg->dfNbFile + 1));
                        pFileCopyInfoCur->pfta = NULL;
                        for (j = 0; j < pDirOrg->dfNbFile; j++)
                        {
                            dfuLong32 dfLastVersionSpecifiedToUse;
                            FILETOCOPYINFO_REMIX *pftci = (pFileCopyInfoCur->pFileCopyInfo) + j;
                            pftci->dfReferenceItem = FTCI_REFERENCE_UNMODIFIED;
                            pftci->fIsReferenceInAddedFile = FALSE;

                            dfLastVersionSpecifiedToUse = (dfLastVersionSpecified == VALUE_UNKNOWN) ?
                                    dfVersionSpecified : dfLastVersionSpecified;

                            if ((i>=dfVersionSpecified) && (i<=dfLastVersionSpecifiedToUse))
                            {
                                dfuLong32 k;
                                const dfwcharpc dfFileNameToTest = ((pDirOrg->pFileInDirInfo)+j)->FileName;
                                if (GetNbElemSA(saParam) == 0)
                                    pftci->dfReferenceItem = FTCI_REFERENCE_DELETE;
                                else
                                    for (k=0;k<GetNbElemSA(saParam);k++)
                                    {
                                        BOOL fTestWilcard;
                                        const FILEUSERITEM *pfi = (const FILEUSERITEM *) GetElemPtrSA(saParam, k);

                                        fTestWilcard=TestWildcardAgaintFileInArchive(dfFileNameToTest,
                                                                                    pfi->FileNameOnArchive);

                                        if (fTestWilcard)
                                        {
                                            pftci->dfReferenceItem = FTCI_REFERENCE_DELETE;
                                            dfNbFileDeleted++;
                                            break;
                                        }
                                    }
                            }
                        }

                        if (dfNbFileDeleted != pDirOrg->dfNbFile)
                        {
                          dfNbVersionRemix++;
                          if (dfNbFileDeleted != 0)
                          {
                              printf("\nFile deleted in version %u\n",i);
                              for (j = 0; j < pDirOrg->dfNbFile; j++)
                              {
                                  FILETOCOPYINFO_REMIX *pftci = (pFileCopyInfoCur->pFileCopyInfo) + j;
                                  const dfwcharpc dfFileNameToShow = ((pDirOrg->pFileInDirInfo)+j)->FileName;
                                  if (pftci->dfReferenceItem == FTCI_REFERENCE_DELETE)
                                  {
                                    printf("  ");
                                    DispOutUnicodeString(dfFileNameToShow);
                                    printf("\n");
                                  }
                              }
                              printf("\n");
                          }
                        }
                        else
                        {
                            DfsFree(pFileCopyInfoCur->pFileCopyInfo);
                            printf("Version %u fully supressed\n",i);
                        }
                    }
            }

            if (pVersionRemix!=NULL)
            {
                fRet =
                    ExecuteRemix(dfNbVersionRemix,pVersionRemix,&DfsFile,
                       szDfsFileName,(szTargetDfs[0]==0) ? NULL : szTargetDfs,szDfsBaseDirectory,&fsUnzippedBase,
                       dfNbDir, (const PCDIRINFO*)pDirInfo, dfBaseVersionSpecified, fReuseOldPatch, fRawAccepted,&cprParam, &DfsFeatureParam,
                       fChangeFirstVersionStorage,fFirstVersionAsReferenceRequested);

                for (i = 0; i < dfNbVersionRemix; i++)
                {
                    FILETOCOPYINFO_REMIX *pFileCopyInfo = (pVersionRemix + i)->pFileCopyInfo;
                    DfsFree(pFileCopyInfo);
                    DfsFree((pVersionRemix + i)->pfta);
                }
                DfsFree(pVersionRemix);
            }

            FreeAllDirInfo(pDirInfo,dfNbDir);
            pDirInfo = NULL;
        }

      if (fCmdDV || fCmdRS)
        {
            dfuLong32 i,j,dfNbDir;
            PDIRINFO* pDirInfo;
            VERSIONTOADD_REMIX *pVersionRemix = NULL;
            dfuLong32 dfNbVersionRemix = 0;
            dfuLong32 dfLastVersionSpecifiedToUse = dfLastVersionSpecified;

            if (dfBaseVersionSpecified == VALUE_UNKNOWN)
                dfBaseVersionSpecified = 0;

            pDirInfo = ReadAllDirInfo(DfsFile,&dfNbDir,READ_ALL_DIR,NULL);
            AdaptDfsFileFeature(DfsFile,(const PCDIRINFO*)pDirInfo,dfNbDir);

            if (pDirInfo != NULL)
            {
                pVersionRemix =
                    (VERSIONTOADD_REMIX *) DfsMalloc(sizeof(VERSIONTOADD_REMIX) * (dfNbDir + 1));
            }

            if ((fCmdRS) && (dfVersionSpecified != VALUE_UNKNOWN))
            {
                if (dfLastVersionSpecifiedToUse == VALUE_UNKNOWN)
                {
                    if (dfVersionSpecified==0)
                    {
                        dfLastVersionSpecifiedToUse = 1;
                    }
                    else
                    {
                        dfLastVersionSpecifiedToUse = dfVersionSpecified;
                        dfVersionSpecified--;
                    }
                    fChangeFirstVersionStorage=TRUE;
                    fFirstVersionAsReferenceRequested=TRUE;
                }
            }

            if (pVersionRemix != NULL)
            {
                for (i = 0; i < dfNbDir; i++)
                    //if (*(lpVersionMap + i))
                    {
                        if (dfVersionSpecified != VALUE_UNKNOWN)
                        {
                            if (fCmdDV)
                            {
                                if ((i==dfVersionSpecified) && (dfLastVersionSpecifiedToUse == VALUE_UNKNOWN))
                                    continue;
                                if ((i>=dfVersionSpecified) && (i<=dfLastVersionSpecifiedToUse) && (dfLastVersionSpecifiedToUse != VALUE_UNKNOWN))
                                    continue;
                            }

                            if ((fCmdRS) && (dfVersionSpecified != VALUE_UNKNOWN))
                            {
                                if ((i<dfVersionSpecified) || (i>dfLastVersionSpecifiedToUse))
                                    continue;
                            }
                        }


                        VERSIONTOADD_REMIX *pFileCopyInfoCur = pVersionRemix + dfNbVersionRemix;
                        PCDIRINFO pDirOrg = *(pDirInfo + i);

                        pFileCopyInfoCur->dfNumVersionPreviousSvf = i;
                        pFileCopyInfoCur->dfNbPreviousFileInMask = pDirOrg->dfNbFile;
                        pFileCopyInfoCur->dfNbFileToAdd = 0;
                        pFileCopyInfoCur->pFileCopyInfo = (FILETOCOPYINFO_REMIX *)
                            DfsMalloc(sizeof(FILETOCOPYINFO_REMIX) * (pDirOrg->dfNbFile + 1));
                        pFileCopyInfoCur->pfta = NULL;
                        for (j = 0; j < pDirOrg->dfNbFile; j++)
                        {
                            FILETOCOPYINFO_REMIX *pftci = (pFileCopyInfoCur->pFileCopyInfo) + j;
                            pftci->dfReferenceItem = FTCI_REFERENCE_UNMODIFIED;
                            pftci->fIsReferenceInAddedFile = FALSE;
                        }
                        dfNbVersionRemix++;
                    }
            }

            if (pVersionRemix!=NULL)
            {
                fRet =
                    ExecuteRemix(dfNbVersionRemix,pVersionRemix,&DfsFile,
                       szDfsFileName,(szTargetDfs[0]==0) ? NULL : szTargetDfs,szDfsBaseDirectory,&fsUnzippedBase,
                       dfNbDir, (const PCDIRINFO*)pDirInfo, dfBaseVersionSpecified, fReuseOldPatch, fRawAccepted, &cprParam, &DfsFeatureParam,
                       fChangeFirstVersionStorage,fFirstVersionAsReferenceRequested);

                for (i = 0; i < dfNbVersionRemix; i++)
                {
                    FILETOCOPYINFO_REMIX *pFileCopyInfo = (pVersionRemix + i)->pFileCopyInfo;
                    DfsFree(pFileCopyInfo);
                    DfsFree((pVersionRemix + i)->pfta);
                }
                DfsFree(pVersionRemix);
            }

            FreeAllDirInfo(pDirInfo,dfNbDir);
            pDirInfo = NULL;
        }
#endif
      if (DfsFile != NULL)
        DfsClose(DfsFile,NULL);
    }


  //FreeFileSet(&fsParam);
  DeleteElemSA(saParam, 0, GetNbElemSA(saParam));
  DeleteElemSA(saParamRenamed, 0, GetNbElemSA(saParamRenamed));
  if (lpszVersionName!=NULL)
      DfsFree((void*)lpszVersionName);
  if (lpszVersionComment!=NULL)
      DfsFree((void*)lpszVersionComment);
  FreeExpandableBufferedString(&EbsCanonicalFileNameDfs);
  FreeFileSet(&fsUnzippedBase,TRUE);
  DeleteStaticArray_C(saParam);
  DeleteStaticArray_C(saParamRenamed);
  return fRet;
}

static tCustomMainFunction fncCustomMainFunction = NULL;

tCustomMainFunction SVFAPI GetCustomMainFunction()
{
    return fncCustomMainFunction;
}

BOOL SVFAPI SetCustomMainFunction(tCustomMainFunction fncCustomMainFunction_)
{
    fncCustomMainFunction = fncCustomMainFunction_;
    return TRUE;
}

int SVFAPI TryCustomMain(int argc, char *argv[], BOOL* mustContinue)
{
    *mustContinue = TRUE;
    if (fncCustomMainFunction!=NULL)
    {
        return (*fncCustomMainFunction)(argc, argv, mustContinue);
    }
    else
    {
        return 0;
    }
}

int SVFAPI PerformCommandLine(dfwcharpc pCommandLine)
{
    int retValue;

#ifdef _DEBUG
    SYNC_DIF_MUTEX_OBJECT SyncDifMutex = GetVirtualFileNameSpaceMutex();
    if (SyncDifMutex != NULL)
        SyncDifDeleteMutex(SyncDifMutex);
    SetVirtualFileNameSpaceMutex(NULL, NULL, TRUE, TRUE);
    CheckEmptyAlloc();
#endif

    retValue = PerformCommandLineInternal(pCommandLine) ? 0 : 1;

#ifdef _DEBUG
    SyncDifMutex = GetVirtualFileNameSpaceMutex();
    if (SyncDifMutex != NULL)
        SyncDifDeleteMutex(SyncDifMutex);
    SetVirtualFileNameSpaceMutex(NULL, NULL, TRUE, TRUE);
    CheckEmptyAlloc();
#endif

    return retValue;
}

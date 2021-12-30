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


#ifndef _SMART_VERSION_EXTRACTER_H
#define _SMART_VERSION_EXTRACTER_H 1

#include "../SmartVersionBase.h"


smv_file* smv_read_open(const char* file_name, long options);

smv_file* smv_read_open_memory(const void*, size_t, long options);

unsigned int get_smv_nb_versions(smv_file*);
unsigned int get_smv_nb_files_in_version(smv_file*, unsigned int version);

// return 0 : OK , -1 : error in param, >0 : filename overflow, max buffer size is retValue
int get_files_info(smv_file*, unsigned int version, unsigned int first, unsigned int nb_files, smv_file_info*, size_t);

int select_smv_file(smv_file*, unsigned int version, unsigned int first);

smv_uint_32 get_smv_file_crc32(smv_file*);
smv_uint_64 get_file_size(smv_file*);
smv_uint_64 get_file_size_encoded(smv_file*);
smv_uint_64 get_file_size_inplace(smv_file*);
smv_uint_64 get_file_size_inserted(smv_file*);
smv_uint_32 get_flags(smv_file*); // have crc, md5, sha1, date filled, is patch/ref/compress
smv_uint_32 get_flags2(smv_file*);
smv_file_date get_file_date(smv_file*);
unsigned char* get_md5(smv_file*);
unsigned char* get_sha1(smv_file*);
unsigned char* get_sha256(smv_file*);
smv_uint_32 get_filename_size(smv_file*);
const char* get_filename(smv_file*);
smv_uint_32 get_filename_in_buffer(smv_file*, char* buffer, size_t buffer_size);

unsigned int first_version_is_reference(smv_file*);

unsigned int last_operation_error(smv_file*);
size_t last_operation_error_text(smv_file*, char*, size_t);



smv_uint_32 get_comment_size(smv_file*, int version, int file_number);
const char* get_comment_text(smv_file*, int version, int file_number);
smv_uint_32 get_comment_text_in_buffer(smv_file*, int version, int file_number, char* buffer, size_t buffer_size);

/*


int SVFAPI ApplyMultiFilePatchEx2(const char* base, int base_only_dir,
const char*patch, const char*dest, int onlyMonoFileBase,
ERROR_MOMENT *perr_moment,
int* perrinfo, char* errBufTxt, int errBufSize, dfuLong32 dfExtractingMethod,
tSetExtractPosCallBack pSetExtractPosCallBack, dfvoidp dfUserPtr,
dfuLong32 dwMinProgress, dfuLong32 dwMaxProgress)
*/
#define SMVSCALLBACK
#define SMVSCALLBACKRETURN int

typedef SMVSCALLBACKRETURN(SMVSCALLBACK * smvExtractCallBack) (smv_uint_32 dwPos,
	const void* dfpAdditionnalInfo,
	const void* dfUserPtr);


int select_extracting_base(smv_file*, int base_version_number, const char* base_version_location);
int select_extracting_base_zipfile(smv_file*, int base_version_number, const char* zipbase);
int select_extracting_version(smv_file*);

int select_extracting_file_range(smv_file*, unsigned int first_file_pos, unsigned int number_files, int extract_flag);

// default to 0 - 1000000
void smv_callback_set_min_max_progress(smv_uint_32 min_progress, smv_uint_32 max_progress);
void smv_callback_set_callback(smvExtractCallBack*, const void* userPtr);


//////////

//extracting

int smv_extract_in_place(smv_file*, const char* directory, smv_uint_32 flags);
int smv_extract_copy(smv_file*, const char* destination_directory, smv_uint_32 flags);
int smv_extract_to_zip(smv_file*, const char* zipname, smv_uint_32 flags);

int smv_close(smv_file*);

#endif

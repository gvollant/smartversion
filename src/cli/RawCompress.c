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
#include <stdio.h>

#include <memory.h>

#include "../lib/engine/patchstream/common/difbasic.h"

#include "../lib/engine/patchstream/compress/abstractCompress.h"
#include "zlib.h"



#include "../lib/engine/svfile/common/DfsMFile.h"
#include "../lib/engine/patchstream/common/DfsTlTyp.h"
#include "../lib/engine/patchstream/common/difstool.h"
#include "RawCompress.h"

#include "../lib/engine/patchstream/compress/abstractCompress.h"
#include "../lib/engine/patchstream/common/abstractDecompress.h"

#define BLOCK_SIZE_RAWCOMPRESSDIRECT (65536)

BOOL DoRawCompress(const char* file_in, const char* file_out, unsigned int ratio, int isStandardzLibWithNegMaxWBits, int strip_prefix)
{
    BOOL success = TRUE;
    int res;
    FILE* fin;
    FILE* fout;
    abstract_compress_stream strm;
    uInt sizeBlockRead = BLOCK_SIZE_RAWCOMPRESSDIRECT;
    uInt sizeBlockWrite = BLOCK_SIZE_RAWCOMPRESSDIRECT;
    unsigned char* buf = (unsigned char*)DfsMalloc(((size_t)sizeBlockRead) + ((size_t)(sizeBlockWrite)));
    unsigned char* buf_in = buf;
    unsigned char* buf_out = buf + sizeBlockRead;
    int size_prefix = 0;
    int size_strip_prefix;
    if (((ratio >= 21) && (ratio <= 69)) || (isStandardzLibWithNegMaxWBits))
        size_prefix = 1;

    size_strip_prefix = ((strip_prefix != 0) ? size_prefix : 0);

    if (buf == NULL)
        return FALSE;
    fin = (strcmp(file_in, "stdin") == 0) ? stdin: fopen(file_in, "rb");
    if (fin == NULL)
    {
        fprintf(stderr, "cannot open %s\n", file_in);
        DfsFree(buf);
        return FALSE;
    }
    fout = (strcmp(file_out, "stdout") == 0) ? stdout: fopen(file_out, "wb");
    if (fout == NULL)
    {
        fprintf(stderr, "cannot open %s\n", file_out);
        if (fin!=stdin) fclose(fin);
        DfsFree(buf);
        return FALSE;
    }

    DfsClearStruct(&strm, 0, sizeof(abstract_compress_stream));
    res = abstract_init_compress_autoselect(&strm, ratio, isStandardzLibWithNegMaxWBits);
    if (res != ABSTR_COMPRESS_Z_OK)
    {
        fprintf(stderr, "cannot init compress\n");
        if (fin!=stdin) fclose(fin);
        if (fout!=stdout) fclose(fout);
        DfsFree(buf);
        return FALSE;
    }

    while (success)
    {
        size_t nb_read = fread(buf_in, 1, (size_t)sizeBlockRead, fin);
        if (nb_read <= 0)
            break;
        strm.avail_in = (uInt)nb_read;
        strm.next_in = buf_in;

        while (success)
        {
            strm.avail_out = (uInt)sizeBlockWrite;
            strm.next_out = buf_out;
            abstract_compress(&strm, ABSTR_COMPRESS_Z_NO_FLUSH);
            uInt done = (uInt)(sizeBlockWrite - strm.avail_out);
            if (done > 0)
            {
                if ((fwrite(buf_out + size_strip_prefix, 1, ((size_t)(done)) - (size_t)size_strip_prefix, fout) != ((size_t)(done)) - (size_t)size_strip_prefix))
                {
                    fprintf(stderr, "error writing file\n");
                    success = FALSE;
                    break;
                }
                size_strip_prefix = 0;
            }
            if (done == 0)
                break;
        }
    }

    while (success)
    {
        int res_compress;
        strm.avail_out = (uInt)sizeBlockWrite;
        strm.next_out = buf_out;
        res_compress = abstract_compress(&strm, ABSTR_COMPRESS_Z_FINISH);

        uInt done = (uInt)(sizeBlockWrite - strm.avail_out);
        if (done > 0)
        {
            if ((fwrite(buf_out, 1, (size_t)done, fout) != (size_t)done))
            {
                fprintf(stderr, "error writing file\n");
                success = FALSE;
                break;
            }
        }
        if ((done == 0) || (res_compress == ABSTR_COMPRESS_Z_STREAM_END))
            break;
    }
    abstract_compress_end(&strm);
    if (fin!=stdin) fclose(fin);
    if (fout!=stdout) fclose(fout);
    DfsFree(buf);
    return success;
}




#define BLOCK_SIZE_RAWUNCOMPRESSDIRECT 65536*4*32
BOOL DoRawUncompress(const char* file_in, const char* file_out)
{
    BOOL success = TRUE;
    int res;
    FILE* fin;
    FILE* fout;
    abstract_decompress_stream strm;

    uInt sizeBlockRead = BLOCK_SIZE_RAWUNCOMPRESSDIRECT;
    uInt sizeBlockWrite = BLOCK_SIZE_RAWUNCOMPRESSDIRECT;
    unsigned char* buf = (unsigned char*)DfsMalloc(((size_t)sizeBlockRead) + ((size_t)(sizeBlockWrite)));
    unsigned char* buf_in = buf;
    unsigned char* buf_out = buf + sizeBlockRead;
    if (buf == NULL)
        return FALSE;





    fin = (strcmp(file_in, "stdin") == 0) ? stdin: fopen(file_in, "rb");
    if (fin == NULL)
    {
        fprintf(stderr, "cannot open %s\n", file_in);
        DfsFree(buf);
        return FALSE;
    }
    fout = (strcmp(file_out, "stdout") == 0) ? stdout: fopen(file_out, "wb");
    if (fout == NULL)
    {
        fprintf(stderr, "cannot open %s\n", file_out);
        if (fin!=stdin) fclose(fin);
        DfsFree(buf);
        return FALSE;
    }

    DfsClearStruct(&strm, 0, sizeof(abstract_decompress_stream));
    res = abstract_init_prefix(&strm);
    if (res != ABSTR_COMPRESS_Z_OK)
    {
        fprintf(stderr, "cannot init compress\n");
        if (fin!=stdin) fclose(fin);
        if (fout!=stdout) fclose(fout);
        DfsFree(buf);
        return FALSE;
    }
    int err = ABSTR_DECOMPRESS_Z_OK;
    while (success)
    {
        size_t nb_read = fread(buf_in, 1, sizeBlockRead, fin);
        if (nb_read <= 0)
            break;
        strm.avail_in = (uInt)nb_read;
        strm.next_in = buf_in;

        while (success)
        {
            uInt done_out;
            strm.avail_out = (uInt)(sizeBlockWrite);
            strm.next_out = buf_out;
            err = abstract_decompress(&strm, ABSTR_DECOMPRESS_Z_SYNC_FLUSH);

            if ((err != ABSTR_DECOMPRESS_Z_OK) && (err != ABSTR_DECOMPRESS_Z_STREAM_END))
            {
                fprintf(stderr, "ERROR\n");
                break;
            }


            done_out = (uInt)(sizeBlockWrite - strm.avail_out);
            if (done_out > 0)
            {
                if ((fwrite(buf_out, 1, (size_t)done_out, fout) != (size_t)done_out))
                {
                    fprintf(stderr, "error writing file\n");
                    success = FALSE;
                    break;
                }
            }
            if (done_out == 0)
                break;

            if (strm.avail_in == 0)
                break;
        }
        if (err == ABSTR_DECOMPRESS_Z_STREAM_END)
            break;
    }

    while ((success) && (err != ABSTR_DECOMPRESS_Z_STREAM_END))
    {
        uInt done;
        strm.avail_out = (uInt)sizeBlockWrite;
        strm.next_out = buf_out;
        err = abstract_decompress(&strm, ABSTR_DECOMPRESS_Z_FINISH);

        done = (uInt)(sizeBlockWrite - strm.avail_out);
        if (done > 0)
        {
            if ((fwrite(buf_out, 1, (size_t)done, fout) != (size_t)done))
            {
                fprintf(stderr, "error writing file\n");
                success = FALSE;
                break;
            }
        }
        if (done == 0)
            break;
    }
    if (err != ABSTR_DECOMPRESS_Z_STREAM_END)
        fprintf(stderr, "Error:\n");
    abstract_decompress_end(&strm);
    if (fin!=stdin) fclose(fin);
    if (fout!=stdout) fclose(fout);
    DfsFree(buf);
    return success;
}
//




#define TstComprAroundUpper(dwValue,dwModulo) (((((dwValue)) + ((dwModulo)) -1) / ((dwModulo))) * (dwModulo))

BOOL DoTestCompressThenUncompressBufMemory(void*buf, size_t fileSize, int compressratio)
{
    size_t fileSizeMarginAround = TstComprAroundUpper((fileSize + (fileSize / 8) + 4096), 0x1000);
    abstract_compress_stream strm;
    hDifTimeElasped startCompress = SyncDifBuidTimeMarkerObject();

    unsigned char* bufOrg = (unsigned char*)buf;
    unsigned char* bufCpr = (unsigned char*)DfsMalloc(2 * fileSizeMarginAround);
    unsigned char* bufUnCpr = ((unsigned char*)bufCpr) + (fileSizeMarginAround);
    dfuLong64 size_compressed;
    unsigned int timeCompress;
    int retc;
    hDifTimeElasped startUncompress;
    int retUncompress = ABSTR_DECOMPRESS_Z_OK;
    int nbCallCompress = 0;
    int nbCallDecompress = 0;
    abstract_decompress_stream ustrm;
    dfuLong64 size_uncompressed;
    unsigned int timeUncompress;


    memset(&strm, 0, sizeof(abstract_compress_stream));
    abstract_init_compress_autoselect(&strm, compressratio, 0);
    strm.avail_in = (uInt)fileSize;
    strm.next_in = bufOrg;


    strm.avail_out = (uInt)fileSizeMarginAround;
    strm.next_out = bufCpr;

    do
    {
        retc = abstract_compress(&strm, ABSTR_COMPRESS_Z_FINISH);
        nbCallCompress++;
    } while (retc == 0);

    fprintf(stderr, "-- ratio %i : ret compress %d size=%u, crc32=%08lx--\n",
          compressratio, retc, (unsigned int)strm.total_out, crc32(0, bufCpr, (uInt)strm.total_out));

    if (retc != ABSTR_COMPRESS_Z_STREAM_END)
    {
        printf("error in compress\n");
        abstract_compress_end(&strm);
        SyncDifGetMSecElapsed(startCompress);
        DfsFree(bufCpr);
        return FALSE;
    }
    size_compressed = strm.total_out;
    abstract_compress_end(&strm);
    timeCompress = SyncDifGetMSecElapsed(startCompress);


    startUncompress = SyncDifBuidTimeMarkerObject();
    memset(&ustrm, 0, sizeof(abstract_decompress_stream));
    abstract_init_prefix(&ustrm);

    ustrm.avail_in = (uInt)size_compressed + 0;
    ustrm.next_in = bufCpr;
    ustrm.avail_out = (uInt)fileSizeMarginAround;
    ustrm.next_out = bufUnCpr;

    while (retUncompress == ABSTR_DECOMPRESS_Z_OK)
    {
        retUncompress = abstract_decompress(&ustrm,/* (ustrm.avail_in!=0) ? ABSTR_DECOMPRESS_Z_SYNC_FLUSH : */ABSTR_DECOMPRESS_Z_FINISH);

        if ((retUncompress == ABSTR_DECOMPRESS_Z_OK) && (ustrm.total_out == fileSize))
            retUncompress = ABSTR_DECOMPRESS_Z_STREAM_END;

        nbCallDecompress++;
    }


    if (retUncompress != ABSTR_DECOMPRESS_Z_STREAM_END)
    {
        fprintf(stderr, "error in uncompress\n");

        abstract_decompress_end(&ustrm);
        SyncDifGetMSecElapsed(startUncompress);
        DfsFree(bufCpr);
        return FALSE;
    }

    size_uncompressed = ustrm.total_out;
    abstract_decompress_end(&ustrm);
    timeUncompress = SyncDifGetMSecElapsed(startUncompress);


    BOOL isSame = (size_uncompressed == fileSize) && (memcmp(buf, bufUnCpr, fileSize) == 0);
    if (!isSame)
    {
        fprintf(stderr, "error in compare\n");
    }

    fprintf(stderr, "size uncompressed = %lu, compressed=%lu for param=%d, ", (unsigned long)fileSize, (unsigned long)size_compressed, (int)compressratio);
    fprintf(stderr, "time compress=%.3f sec, uncompress=%.3f sec (%d,%d)\n", timeCompress / 1000., timeUncompress / 1000., nbCallCompress, nbCallDecompress);

    DfsFree(bufCpr);
    return isSame;
}



BOOL DoTestCompressThenUncompressBufMemoryByBlock(void* buf, size_t fileSize, int compressratio)
{
    size_t size_max_out=65536;
    size_t size_max_in=65536;
    size_t fileSizeMarginAround = TstComprAroundUpper((fileSize + (fileSize / 8) + 4096), 0x1000);
    abstract_compress_stream strm;
    hDifTimeElasped startCompress = SyncDifBuidTimeMarkerObject();
    hDifTimeElasped startUncompress;
    unsigned int timeCompress;
    unsigned char* bufOrg = (unsigned char*)buf;
    unsigned char* bufCpr = (unsigned char*) DfsMalloc(2 * fileSizeMarginAround);
    unsigned char* bufUnCpr = ((unsigned char*)bufCpr) + (fileSizeMarginAround);
    dfuLong64 size_compressed ;
    abstract_decompress_stream ustrm;

    memset(&strm, 0, sizeof(abstract_compress_stream));
    abstract_init_compress_autoselect(&strm, compressratio, 0);

    strm.avail_in = (uInt)fileSize;
    strm.next_in = bufOrg;


    strm.avail_out = (uInt)fileSizeMarginAround;
    strm.next_out = bufCpr;

    int retc;
    do
    {
        int type_flush;
        strm.avail_in = (uInt)(fileSize - strm.total_in);
        type_flush = (strm.avail_in>size_max_in) ? ABSTR_COMPRESS_Z_NO_FLUSH : ABSTR_COMPRESS_Z_FINISH;
        if (strm.avail_in>(uInt)size_max_in)
            strm.avail_in=(uInt)size_max_in;
        strm.avail_out = (uInt)(fileSizeMarginAround - strm.total_out);
        if (strm.avail_out>(uInt)size_max_out)
            strm.avail_out=(uInt)size_max_out;
        retc = abstract_compress(&strm, type_flush);
    } while (retc == 0);
    timeCompress = SyncDifGetMSecElapsed(startCompress);
    size_compressed = strm.total_out;
    fprintf(stderr,"-- compressratio %i : ret compress by block %d size=%u, crc32=%08lx--\n",
        compressratio, retc,(unsigned int)size_compressed,crc32(0,bufCpr,(uInt)size_compressed));
    if (retc != ABSTR_COMPRESS_Z_STREAM_END)
    {
        fprintf(stderr, "error in compress\n");
        abstract_compress_end(&strm);
        SyncDifGetMSecElapsed(startCompress);
        DfsFree(bufCpr);
        return FALSE;
    }

    abstract_compress_end(&strm);




    startUncompress = SyncDifBuidTimeMarkerObject();
    memset(&ustrm, 0, sizeof(abstract_decompress_stream));
    abstract_init_prefix(&ustrm);

    ustrm.avail_in = (uInt)size_compressed + 0;
    ustrm.next_in = bufCpr;
    ustrm.avail_out = (uInt)fileSizeMarginAround;
    ustrm.next_out = bufUnCpr;

    int retUncompress = ABSTR_DECOMPRESS_Z_OK;
    int nbCallDecompress = 0;
    while (retUncompress == ABSTR_DECOMPRESS_Z_OK)
    {
        retUncompress = abstract_decompress(&ustrm,/* (ustrm.avail_in!=0) ? ABSTR_DECOMPRESS_Z_SYNC_FLUSH : */ABSTR_DECOMPRESS_Z_FINISH);

        if ((retUncompress == ABSTR_DECOMPRESS_Z_OK) && (ustrm.total_out == fileSize))
            retUncompress = ABSTR_DECOMPRESS_Z_STREAM_END;

        nbCallDecompress++;
    }


    if (retUncompress != ABSTR_DECOMPRESS_Z_STREAM_END)
    {
        fprintf(stderr, "error in uncompress\n");

        abstract_decompress_end(&ustrm);
        SyncDifGetMSecElapsed(startUncompress);
        DfsFree(bufCpr);
        return FALSE;
    }

    dfuLong64 size_uncompressed = ustrm.total_out;
    abstract_decompress_end(&ustrm);
    unsigned int timeUncompress = SyncDifGetMSecElapsed(startUncompress);


    BOOL isSame = (size_uncompressed == fileSize) && (memcmp(buf, bufUnCpr, fileSize) == 0);
    if (!isSame)
    {
        fprintf(stderr, "error in compare\n");
    }

    fprintf(stderr, "size uncompressed = %lu, compressed=%lu for param=%d, ", (unsigned long)fileSize, (unsigned long)size_compressed, (int)compressratio);
    fprintf(stderr, "time compress=%.3f sec, uncompress=%.3f sec (%d)\n", timeCompress / 1000., timeUncompress / 1000., nbCallDecompress);
    DfsFree(bufCpr);
    return isSame;
}

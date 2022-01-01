
do_download()
{
  DOWNLOAD_URL=$1
  DOWNLOAD_FILENAME=$2
  #wget $1 -O $2
  curl -L $1 -o $2
}

rm -rf  rbldext
mkdir rbldext
cd rbldext
do_download https://github.com/gvollant/lzham_codec/archive/refs/tags/v1_0_stable3_compat.zip lzham.zip
do_download https://github.com/gvollant/lzham_codec/archive/refs/tags/v1_0_stable3_compat.tar.gz lzham.tar.gz

do_download https://github.com/gvollant/fastlzlib/archive/refs/tags/fastlzlib_with_lzfse.zip fastlzlib.zip
do_download https://github.com/gvollant/fastlzlib/archive/refs/tags/fastlzlib_with_lzfse.tar.gz fastlzlib.tar.gz

do_download https://github.com/lz4/lz4/archive/refs/tags/v1.9.3.zip lz4.zip
do_download https://github.com/lz4/lz4/archive/refs/tags/v1.9.3.tar.gz lz4.tar.gz

do_download https://github.com/ariya/FastLZ/archive/refs/tags/0.5.0.zip fastlz.zip
do_download https://github.com/ariya/FastLZ/archive/refs/tags/0.5.0.tar.gz fastlz.tar.gz

do_download https://sourceware.org/pub/bzip2/bzip2-1.0.5.tar.gz oldbzip2.tar.gz
do_download https://sourceware.org/pub/bzip2/bzip2-1.0.8.tar.gz bzip2.tar.gz


do_download https://www.7-zip.org/a/lzma2107.7z lzma.7z

do_download https://tukaani.org/xz/xz-5.2.5.tar.gz xz.tar.gz

do_download https://tukaani.org/xz/xz-embedded-20210201.tar.gz xz-embedded.tar.gz

do_download http://zlib.net/zlib-1.2.11.tar.gz zlib.tar.gz
do_download http://zlib.net/zlib1211.zip zlib.zip


do_download https://github.com/facebook/zstd/archive/refs/tags/v1.5.1.tar.gz zstd.tar.gz
do_download https://github.com/facebook/zstd/archive/refs/tags/v1.5.1.zip zstd.zip

do_download https://github.com/lzfse/lzfse/archive/refs/tags/lzfse-1.0.zip lzfse.zip
do_download https://github.com/lzfse/lzfse/archive/refs/tags/lzfse-1.0.tar.gz lzfse.tar.gz

cd ..

./decomp_ext.sh

rm -rf  rbldext
mkdir rbldext
cd rbldext
wget https://github.com/gvollant/lzham_codec/archive/refs/tags/v1_0_stable2_compat.zip   -O lzham.zip
wget https://github.com/gvollant/lzham_codec/archive/refs/tags/v1_0_stable2_compat.tar.gz -O lzham.tar.gz

wget https://github.com/gvollant/fastlzlib/archive/refs/tags/fastlzlib_with_lzfse.zip -O fastlzlib.zip
wget https://github.com/gvollant/fastlzlib/archive/refs/tags/fastlzlib_with_lzfse.tar.gz -O fastlzlib.tar.gz

wget https://github.com/lz4/lz4/archive/refs/tags/v1.9.3.zip -O lz4.zip
wget https://github.com/lz4/lz4/archive/refs/tags/v1.9.3.tar.gz -O lz4.tar.gz

wget https://github.com/ariya/FastLZ/archive/refs/tags/0.5.0.zip -O fastlz.zip
wget https://github.com/ariya/FastLZ/archive/refs/tags/0.5.0.tar.gz -O fastlz.tar.gz

wget https://sourceware.org/pub/bzip2/bzip2-1.0.5.tar.gz -O oldbzip2.tar.gz
wget https://sourceware.org/pub/bzip2/bzip2-1.0.8.tar.gz -O bzip2.tar.gz


wget https://www.7-zip.org/a/lzma2107.7z -O lzma.7z

wget https://tukaani.org/xz/xz-5.2.5.tar.gz -O xz.tar.gz

wget https://tukaani.org/xz/xz-embedded-20210201.tar.gz -O xz-embedded.tar.gz

wget http://zlib.net/zlib-1.2.11.tar.gz -O zlib.tar.gz
wget http://zlib.net/zlib1211.zip -O zlib.zip


wget https://github.com/facebook/zstd/archive/refs/tags/v1.5.1.tar.gz -O zstd.tar.gz
wget https://github.com/facebook/zstd/archive/refs/tags/v1.5.1.zip -O zstd.zip

wget https://github.com/lzfse/lzfse/archive/refs/tags/lzfse-1.0.zip -O lzfse.zip
wget https://github.com/lzfse/lzfse/archive/refs/tags/lzfse-1.0.tar.gz -O lzfse.tar.gz

./decomp_ext.sh

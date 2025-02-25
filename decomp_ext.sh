rm -rf newext
mkdir newext
cd newext

mkdir fastlzlib
cd fastlzlib
tar xvf ../../rbldext/fastlzlib.tar.gz  --strip-components=1
mv lz4 lz4_fromfastlzlib
mv fastlz fastlz_fromfastlzlib

mkdir lz4
cd lz4
tar xvf ../../../rbldext/lz4.tar.gz  --strip-components=2
cd ..

mkdir fastlz
cd fastlz
tar xvf ../../../rbldext/fastlz.tar.gz  --strip-components=1
cd ..


mkdir lzfse
cd lzfse
tar xvf ../../../rbldext/lzfse.tar.gz  --strip-components=1
cd ..

cd ..

mkdir bzip
cd bzip
tar xvf ../../rbldext/bzip2.tar.gz  --strip-components=1
cd ..


mkdir lzham
cd lzham
tar xvf ../../rbldext/lzham.tar.gz  --strip-components=1
cd ..


mkdir xz-embedded
cd xz-embedded
tar xvf ../../rbldext/xz-embedded.tar.gz  --strip-components=1
cd ..



mkdir xz
cd xz
tar xvf ../../rbldext/xz.tar.gz  --strip-components=1
mkdir -p msvc
cp ../../config.h.xz config.h
cp ../../config.h.xz msvc/config.h
sed -i 's/HAVE_WCWIDTH/HAVE_no_WCWIDTH/g' msvc/config.h
cd ..


mkdir zlib
cd zlib
tar xvf ../../rbldext/zlib.tar.gz  --strip-components=1
mv compress.c zlibcompress.c
cd ..



mkdir zstd
cd zstd
tar xvf ../../rbldext/zstd.tar.gz  --strip-components=1
cd ..

mkdir lzma_sdk
cd lzma_sdk
7za x ../../rbldext/lzma.7z
cd C
mv Sha256.c LZMASDK_Sha256.c
cd ..
cd ..
cd ..

pwd

mv newext external

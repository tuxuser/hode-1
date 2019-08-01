set -x

ZLIB=$HOME/Install/zlib-1.2.11

OUT=hod_decoder.exe
i686-w64-mingw32-g++ -o $OUT decode_mst.cpp fileio.cpp lzw.cpp main.cpp screenshot.cpp wav.cpp -L $ZLIB/ -I $ZLIB/ -lz -static-libgcc -static-libstdc++
i686-w64-mingw32-strip $OUT
upx -9 $OUT
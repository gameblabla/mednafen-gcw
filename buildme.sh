export PATH="/opt/gcw0-toolchain/usr/bin:${PATH}"

make clean
./configure --host=mipsel-linux --without-libsndfile --enable-ngp --enable-nes --enable-gba --enable-gb --enable-lynx --disable-psx --disable-pce --disable-pce-fast --disable-vb --disable-pcfx --disable-snes --disable-wswan --disable-debugger --disable-snes-faust --disable-fancy-scalers CFLAGS="-O3 -fomit-frame-pointer -mips32r2" CXXFLAGS="-O3 -fomit-frame-pointer -mips32r2" LDFLAGS="-Wl,--as-needed -flto"
make -j6
mv ./src/mednafen ./opk/mednafen

make clean
./configure --host=mipsel-linux --without-libsndfile --enable-psx --disable-pce --disable-pce-fast --disable-lynx --disable-ngp --disable-vb --disable-pcfx --disable-nes --disable-snes --disable-wswan --disable-gba --disable-gb --disable-debugger --disable-snes-faust --disable-fancy-scalers CFLAGS="-O3 -fomit-frame-pointer -mips32r2" CXXFLAGS="-O3 -fomit-frame-pointer -mips32r2" LDFLAGS="-Wl,--as-needed -flto"
make -j6
mv ./src/mednafen ./opk_psx/mednafen_psx

make clean
./configure --host=mipsel-linux --without-libsndfile --enable-pce-fast --disable-pce --disable-ngp --disable-lynx --disable-vb --disable-pcfx --disable-nes --disable-snes --disable-wswan --disable-gba --disable-gb --disable-debugger --disable-snes-faust --disable-fancy-scalers CFLAGS="-O3 -fomit-frame-pointer -mips32r2" CXXFLAGS="-O3 -fomit-frame-pointer -mips32r2" LDFLAGS="-Wl,--as-needed -flto"
make -j6
mv ./src/mednafen ./opk_pce/mednafen_pce

make clean
./configure --host=mipsel-linux --without-libsndfile --enable-pcfx --disable-pce --disable-pce-fast --disable-lynx --disable-ngp --disable-vb --disable-nes --disable-snes --disable-wswan --disable-gba --disable-gb --disable-debugger --disable-snes-faust --disable-fancy-scalers CFLAGS="-O3 -fomit-frame-pointer -mips32r2" CXXFLAGS="-O3 -fomit-frame-pointer -mips32r2" LDFLAGS="-Wl,--as-needed -flto"
make -j6

mv ./src/mednafen ./opk_pcfx/mednafen_pcfx

mksquashfs ./opk mednafen.opk -all-root -noappend -no-exports -no-xattrs
mksquashfs ./opk_psx mednafen_psx.opk -all-root -noappend -no-exports -no-xattrs
mksquashfs ./opk_pcfx mednafen_pcfx.opk -all-root -noappend -no-exports -no-xattrs
mksquashfs ./opk_pce mednafen_pce.opk -all-root -noappend -no-exports -no-xattrs
mv mednafen.opk ./release/mednafen.opk
mv mednafen_pcfx.opk ./release/mednafen_pcfx.opk
mv mednafen_psx.opk ./release/mednafen_psx.opk
mv mednafen_pce.opk ./release/mednafen_pce.opk


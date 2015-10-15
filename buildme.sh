export PATH="/opt/gcw0-toolchain/usr/bin:${PATH}"

make clean
./configure --host=mipsel-linux --enable-ngp --enable-nes --enable-gba --enable-gb --enable-lynx --disable-psx --disable-pce --disable-pce-fast --disable-vb --disable-pcfx --disable-snes --disable-wswan --disable-debugger --disable-snes-faust --disable-fancy-scalers CFLAGS="-O3 -fomit-frame-pointer -mips32r2" CXXFLAGS="-O3 -fomit-frame-pointer -mips32r2" LDFLAGS="-Wl,--as-needed -flto"
make
mv ./src/mednafen ./opk/mednafen

make clean
./configure --host=mipsel-linux --enable-psx --disable-pce --disable-pce-fast --disable-lynx --disable-ngp --disable-vb --disable-pcfx --disable-nes --disable-snes --disable-wswan --disable-gba --disable-gb --disable-debugger --disable-snes-faust --disable-fancy-scalers CFLAGS="-O3 -fomit-frame-pointer -mips32r2" CXXFLAGS="-O3 -fomit-frame-pointer -mips32r2" LDFLAGS="-Wl,--as-needed -flto"
make
mv ./src/mednafen ./opk/mednafen_psx

make clean
./configure --host=mipsel-linux --enable-pce-fast --disable-pce --disable-ngp --disable-lynx --disable-vb --disable-pcfx --disable-nes --disable-snes --disable-wswan --disable-gba --disable-gb --disable-debugger --disable-snes-faust --disable-fancy-scalers CFLAGS="-O3 -fomit-frame-pointer -mips32r2" CXXFLAGS="-O3 -fomit-frame-pointer -mips32r2" LDFLAGS="-Wl,--as-needed -flto"
make
mv ./src/mednafen ./opk/mednafen_pce

make clean
./configure --host=mipsel-linux --enable-pcfx --disable-pce --disable-pce-fast --disable-lynx --disable-ngp --disable-vb --disable-nes --disable-snes --disable-wswan --disable-gba --disable-gb --disable-debugger --disable-snes-faust --disable-fancy-scalers CFLAGS="-O3 -fomit-frame-pointer -mips32r2" CXXFLAGS="-O3 -fomit-frame-pointer -mips32r2" LDFLAGS="-Wl,--as-needed -flto"
make
mv ./src/mednafen ./opk_pcfx/mednafen_pcfx

mksquashfs ./opk mednafen.opk -all-root -noappend -no-exports -no-xattrs
mv mednafen.opk ./release/mednafen.opk


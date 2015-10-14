export PATH="/opt/gcw0-toolchain/usr/bin:${PATH}"


make clean
./configure --host=mipsel-linux --enable-nes --enable-gba --enable-gb --enable-lynx --enable-ngp --disable-psx --disable-pce --disable-pce-fast --disable-ngp --disable-vb --disable-pcfx --disable-snes --disable-wswan --disable-debugger --disable-snes-faust --disable-fancy-scalers CFLAGS="-O3 -fomit-frame-pointer -mips32r2" CXXFLAGS="-O3 -fomit-frame-pointer -mips32r2" LDFLAGS="-Wl,--as-needed -flto"
make
mv ./src/mednafen ./opk/mednafen
sh pack_generic.sh
mv mednafen_generic.opk ./release/mednafen_generic.opk

make clean
./configure --host=mipsel-linux --enable-psx --disable-pce --disable-pce-fast --disable-lynx --disable-ngp --disable-vb --disable-pcfx --disable-nes --disable-snes --disable-wswan --disable-gba --disable-gb --disable-debugger --disable-snes-faust --disable-fancy-scalers CFLAGS="-O3 -fomit-frame-pointer -mips32r2" CXXFLAGS="-O3 -fomit-frame-pointer -mips32r2" LDFLAGS="-Wl,--as-needed -flto"
make
mv ./src/mednafen ./opk_psx/mednafen
sh pack_psx.sh
mv mednafen_psx.opk ./release/mednafen_psx.opk

make clean
./configure --host=mipsel-linux --enable-pce-fast --disable-pce --disable-ngp --disable-lynx --disable-vb --disable-pcfx --disable-nes --disable-snes --disable-wswan --disable-gba --disable-gb --disable-debugger --disable-snes-faust --disable-fancy-scalers CFLAGS="-O3 -fomit-frame-pointer -mips32r2" CXXFLAGS="-O3 -fomit-frame-pointer -mips32r2" LDFLAGS="-Wl,--as-needed -flto"
make
mv ./src/mednafen ./opk_pce/mednafen
sh pack_pce.sh
mv mednafen_pce.opk ./release/mednafen_pce.opk

make clean
./configure --host=mipsel-linux --enable-pcfx --disable-pce --disable-pce-fast --disable-lynx --disable-ngp --disable-vb --disable-nes --disable-snes --disable-wswan --disable-gba --disable-gb --disable-debugger --disable-snes-faust --disable-fancy-scalers CFLAGS="-O3 -fomit-frame-pointer -mips32r2" CXXFLAGS="-O3 -fomit-frame-pointer -mips32r2" LDFLAGS="-Wl,--as-needed -flto"
make
mv ./src/mednafen ./opk_pcfx/mednafen
sh pack_pcfx.sh
mv mednafen_pcfx.opk ./release/mednafen_pcfx.opk

make clean
./configure --host=mipsel-linux --enable-vb --disable-pce --disable-pce-fast --disable-lynx --disable-ngp --disable-pcfx --disable-nes --disable-snes --disable-wswan --disable-gba --disable-gb --disable-debugger --disable-snes-faust --disable-fancy-scalers CFLAGS="-O3 -fomit-frame-pointer -mips32r2" CXXFLAGS="-O3 -fomit-frame-pointer -mips32r2" LDFLAGS="-Wl,--as-needed -flto"
make
mv ./src/mednafen ./opk_vb/mednafen
sh pack_vb.sh
mv mednafen_vb.opk ./release/mednafen_vb.opk


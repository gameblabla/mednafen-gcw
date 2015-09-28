export PATH="$HOME/Documents/DEV/gcw0-toolchain/usr/bin:${PATH}"
make clean
./configure --host=mipsel-linux --disable-pce --disable-debugger --disable-snes-faust --disable-fancy-scalers --disable-psx CFLAGS="-O3 -fomit-frame-pointer -mips32r2 -fdata-sections -ffunction-sections" CXXFLAGS="-O3 -fomit-frame-pointer -mips32r2 -fdata-sections -ffunction-sections" LDFLAGS="-Wl,--as-needed -Wl,--gc-sections -flto"
make
mv ./src/mednafen ./opk/mednafen
sh pack.sh
mv mednafen.opk ./release/mednafen.opk
make clean
./configure --host=mipsel-linux --enable-psx --disable-pce --disable-pce-fast --disable-ngp --disable-vb --disable-pcfx --disable-nes --disable-snes --disable-wswan --disable-gba --disable-gb --disable-debugger --disable-snes-faust --disable-fancy-scalers CFLAGS="-O3 -fomit-frame-pointer -mips32r2 -fdata-sections -ffunction-sections" CXXFLAGS="-O3 -fomit-frame-pointer -mips32r2 -fdata-sections -ffunction-sections" LDFLAGS="-Wl,--as-needed -Wl,--gc-sections -flto"
make
mv ./src/mednafen ./opk_psx/mednafen
sh pack_psx.sh
mv mednafen_psx.opk ./release/mednafen_psx.opk
make clean

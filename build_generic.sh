export PATH="/opt/gcw0-toolchain/usr/bin:${PATH}"

make clean
./configure --host=mipsel-linux --enable-nes --enable-gba --enable-gb --enable-lynx --enable-ngp --disable-psx --disable-pce --disable-pce-fast --disable-ngp --disable-vb --disable-pcfx --disable-snes --disable-wswan --disable-debugger --disable-snes-faust --disable-fancy-scalers CFLAGS="-O2 -mips32r2" CXXFLAGS="-O2 -mips32r2" LDFLAGS="-Wl,--as-needed -flto"
make
mv ./src/mednafen ./opk/mednafen
sh pack_generic.sh
mv mednafen_generic.opk ./release/mednafen_generic.opk

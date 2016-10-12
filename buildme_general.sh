export PATH="/opt/gcw0-toolchain/usr/bin:${PATH}"

./configure --host=mipsel-linux --without-libsndfile --enable-gba --enable-gb --enable-lynx --enable-ngp --enable-nes --disable-pce-fast --disable-pcfx --disable-psx --disable-sms --disable-debugger --disable-threads --disable-cjk-fonts --disable-ss --disable-ssfplay --disable-pce --disable-md --disable-vb --disable-snes --disable-wswan --disable-debugger --disable-snes-faust --disable-fancy-scalers --disable-alsatest CFLAGS="-Ofast -fdata-sections -ffunction-sections -march=mips32r2 -mips32r2 -fno-PIE" CXXFLAGS="-Ofast -fdata-sections -ffunction-sections -march=mips32r2 -mips32r2 -fno-rtti -fno-PIE" LDFLAGS="-fno-PIE -Wl,--as-needed -Wl,--gc-sections -flto -s"
make -j7
mv ./src/mednafen ./opk/mednafen


mksquashfs ./opk mednafen.opk -all-root -noappend -no-exports -no-xattrs
mv mednafen.opk ./release/mednafen.opk


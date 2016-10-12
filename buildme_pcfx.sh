export PATH="/opt/gcw0-toolchain/usr/bin:${PATH}"

./configure --host=mipsel-linux --without-libsndfile --enable-pcfx \
--disable-psx --disable-sms --disable-debugger --disable-cdplay --disable-threads --disable-cjk-fonts  --disable-jack \
--disable-ss --disable-ssfplay --disable-pce --disable-md --disable-pce-fast --disable-lynx --disable-ngp --disable-vb --disable-nes --disable-snes  \
--disable-wswan --disable-gba --disable-gb --disable-snes-faust --disable-fancy-scalers --disable-alsatest  \
CFLAGS="-Ofast -fdata-sections -ffunction-sections  -march=mips32r2 -mips32r2 -mmxu -fno-PIE"  \
CXXFLAGS="-Ofast -fdata-sections -ffunction-sections -march=mips32r2 -mips32r2 -mmxu -fno-rtti -fno-PIE"  \
LDFLAGS="-fno-PIE -Wl,--as-needed -Wl,--gc-sections -flto -s"
make -j7
mipsel-linux-strip ./src/mednafen --strip-all
mv ./src/mednafen ./opk_pcfx/mednafen
mksquashfs ./opk_pcfx mednafen_pcfx.opk -all-root -noappend -no-exports -no-xattrs
mv mednafen_pcfx.opk ./release/mednafen_pcfx.opk


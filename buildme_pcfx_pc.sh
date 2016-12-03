./configure CFLAGS="-pg" LDFLAGS="-pg" --without-libsndfile --enable-pcfx \
--disable-psx --disable-sms --disable-debugger --disable-cdplay --disable-threads --disable-cjk-fonts  --disable-jack \
--disable-ss --disable-ssfplay --disable-pce --disable-md --disable-pce-fast --disable-lynx --disable-ngp --disable-vb --disable-nes --disable-snes  \
--disable-wswan --disable-gba --disable-gb --disable-snes-faust --disable-fancy-scalers --disable-alsatest
make -j7
mv ./src/mednafen ./mednafen

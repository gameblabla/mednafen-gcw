===========================
Mednafen GCW0
===========================
Version 1.33
Port by gameblabla

Here is a port of Mednafen for GCW0.
Only cores for PC Engine (CD), NEC PC-FX, PSX and GB/GBA/NES/LYNX are available.
For other cores, you should use the other emulators available on the platform instead.
(Mednafen's cores are accurate and way too slow)

================
CONTROLS
================

Exit 				: Select + X
Configure Controls 	: Start  + A

================
INSTALLATION
================

First of all,
put the file mednafen-09x.cfg in /media/home/.mednafen.
If the folder ".mednafen" does not exist, create it and place the file there.

Then, put all opk files in /media/data/apps.
You should then find them in the "Emulators" section.

PCFX requires a BIOS called pcfx.rom to be placed in /media/home/.mednafen.
Careful, as there is one revision that does not work with it !
The BIOS annoted with a [a] is the only one known to work with Mednafen.

PC Engine CD games requires a syscard called syscard3.pce. (same directory)

PSX requires three bioses in the home directory.(/media/home/.mednafen.) 
These are : scph5500.bin, scph5501.bin and scph5502.bin.

Lynx requires a BIOS called lynxboot.img in the home directory. (/media/home/.mednafen.)

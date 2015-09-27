<?php require("docgen.inc"); ?>

<?php BeginPage('psx', 'Sony PlayStation'); ?>

<?php BeginSection('Introduction', "", FALSE, FALSE, "Section_intro"); ?>
<p>
<font color="red" size="+2">WARNING:</font> Save states(and by extension, state rewinding and netplay) and saved games on emulated memory cards do not mix well unless you are
extremely careful.  For any given game, either use save states or the game's own save system, but not both, lest you may destroy your saved games.
</p>
<p>
PlayStation 1 emulation is currently in a state of development, so there may occasionally be horrifying SPINE-TINGLING regressions in new versions.
</p>
<p>
A dual-core Phenom II or Athlon II at 3GHz or higher, or rough equivalent(in terms of single-core IPC), is recommended for
running Mednafen's PlayStation 1 emulation on.  For better performance, the binary should be compiled for a 64-bit
target(for example, x86_64) rather than 32-bit, if available.<br>
</p>

<p>
Enabling CD image preloading into memory via the <a href="mednafen.html#cd.image_memcache">cd.image_memcache</a> setting is recommended, to
avoid short emulator pauses and audio pops due to waiting for disk accesses to complete when the emulated CD is accessed.
</p>

<p>
<a href="http://www.neillcorlett.com/psf/">PSF1</a> playback is supported.
</p>

<?php EndSection(); ?>


<?php BeginSection("Firmware/BIOS", "", FALSE, FALSE, "Section_firmware_bios"); ?>
<p>
Place the correct BIOS image files in the <a href="mednafen.html#Section_firmware_bios">correct location</a>.  Do not rename other revisions/regions of the BIOS to match the expected filenames, or you'll likely
cause emulation glitches(and cause confusion if you ever distribute or otherwise refer to these misnamed files).
</p>

<p>
The filenames listed below are per default psx.bios_* settings.
</p>
<table border>
 <tr><th>Filename:</th><th>Purpose:</th><th>Hash:</tr>
 <tr><td>scph5500.bin</td><td><b>SCPH-5500</b>/v<b>3.0J</b> BIOS image.<br>Required for Japan-region games.</td><td><b><u>MD5:</u></b> 8dd7d5296a650fac7319bce665a6a53c<br><b><u>SHA256:</u></b> 9c0421858e217805f4abe18698afea8d5aa36ff0727eb8484944e00eb5e7eadb</td></tr>
 <tr><td>scph5501.bin</td><td><b>SCPH-5501</b>/v<b>3.0A</b> BIOS image.<br>Required for North America/US-region games.<br>Reportedly the same as the SCPH-7003 BIOS image.</td><td><b><u>MD5:</u></b> 490f666e1afb15b7362b406ed1cea246<br><b><u>SHA256:</u></b> 11052b6499e466bbf0a709b1f9cb6834a9418e66680387912451e971cf8a1fef</td></tr>
 <tr><td>scph5502.bin</td><td><b>SCPH-5502</b>/v<b>3.0E</b> BIOS image.<br>Required for Europe-region games.</td><td><b><u>MD5:</b></u> 32736f17079d0b2b7024407c39bd3050<br><b><u>SHA256:</u></b> 1faaa18fa820a0225e488d9f086296b8e6c46df739666093987ff7d8fd352c09</td></tr>
</table>
<?php EndSection(); ?>

<?php BeginSection("Analog Sticks Range Issues"); ?>
The DualShock and Dual Analog controllers' analog sticks have a circular physical range of movement, but a much more squareish(corners are a bit rounded for
DualShock) logical range, likely due to conservative calibration in the gamepads' hardware and firmware design.  Modern PC(compatible) gamepads with a circular
physical range of motion for their analog sticks(e.g. XBox 360 type controllers) tend to be more tightly-calibrated in hardware, and thus their logical range
of motion will be closer to circles than squares.  This can cause problems with movement in some PS1 games(e.g. "Mega Man Legends 2") that care not for proper trigonometry, as they are expecting larger
values at ordinal angles of the sticks than the aforementioned type of PC gamepad can provide due to its design.

<p>
An "axis_scale" setting(named like "<a href="#psx.input.port1.dualshock.axis_scale">psx.input.port1.dualshock.axis_scale</a>") is provided for each possible
emulated DualShock and Dual Analog controller on each port.  To work around this range issue with DualShock emulation, an "axis_scale" setting of "1.33" is
recommended as a starting point.  Smaller values(such as "1.20") may be sufficient and provide for more precise control, so try experimenting to find the ideal for your combination of gamepad and games.
</p>

<?php EndSection(); ?>

<?php BeginSection("Multitap Usage", "", FALSE, FALSE, "Section_multitap"); ?>
<p>
By default, no multitap is enabled.  Be aware that if you enable multitap on PSX port 1, game view mapping will be
inconsistent between games that support multitap and those that do not.
</p>

<p>
Enabling multitap on either PSX port <b>may</b> cause slight game slowdown.  Some 1-and-2-player-only games half-support the
multitap, but are apparently not coded with the multitap in mind, and <b>may</b> suffer from graphical glitches like screen tearing if multitap
is enabled when running them.
</p>

<table border="1">
<tr><th colspan="4">(Virtual) Port Index to Game View Mappings for Multitap only on PSX Port 1</th></tr>
<tr><th>Port Index:</th><th>Multitap-Compatible-Game:</th><th>Multitap-Incompatible-Game:</th><th>Physical Port Name:</th></tr>
<tr><td>1</td><td>1</td><td>1</td><td>1A</td></tr>
<tr><td>2</td><td>2</td><td>-</td><td>1B</td></tr>
<tr><td>3</td><td>3</td><td>-</td><td>1C</td></tr>
<tr><td>4</td><td>4</td><td>-</td><td>1D</td></tr>
<tr><td>5</td><td>5</td><td>2</td><td>2</td></tr>
<tr><td>6</td><td>-</td><td>-</td><td>-</td></tr>
<tr><td>7</td><td>-</td><td>-</td><td>-</td></tr>
<tr><td>8</td><td>-</td><td>-</td><td>-</td></tr>
</table>

<br>

<table border="1">
<tr><th colspan="4">(Virtual) Port Index to Game View Mappings for Multitap only on PSX Port 2</th></tr>
<tr><th>Port Index:</th><th>Multitap-Compatible-Game:</th><th>Multitap-Incompatible-Game:</th><th>Physical Port Name:</th></tr>
<tr><td>1</td><td>1</td><td>1</td><td>1</td></tr>
<tr><td>2</td><td>2</td><td>2</td><td>2A</td></tr>
<tr><td>3</td><td>3</td><td>-</td><td>2B</td></tr>
<tr><td>4</td><td>4</td><td>-</td><td>2C</td></tr>
<tr><td>5</td><td>5</td><td>-</td><td>2D</td></tr>
<tr><td>6</td><td>-</td><td>-</td><td>-</td></tr>
<tr><td>7</td><td>-</td><td>-</td><td>-</td></tr>
<tr><td>8</td><td>-</td><td>-</td><td>-</td></tr>
</table>
<br>
<table border="1">
<tr><th colspan="4">(Virtual) Port Index to Game View Mappings for Multitap on both PSX Ports</th></tr>
<tr><th>Port Index:</th><th>Multitap-Compatible-Game:</th><th>Multitap-Incompatible-Game:</th><th>Physical Port Name:</th></tr>
<tr><td>1</td><td>1</td><td>1</td><td>1A</td></tr>
<tr><td>2</td><td>2</td><td>-</td><td>1B</td></tr>
<tr><td>3</td><td>3</td><td>-</td><td>1C</td></tr>
<tr><td>4</td><td>4</td><td>-</td><td>1D</td></tr>
<tr><td>5</td><td>5</td><td>2</td><td>2A</td></tr>
<tr><td>6</td><td>6</td><td>-</td><td>2B</td></tr>
<tr><td>7</td><td>7</td><td>-</td><td>2C</td></tr>
<tr><td>8</td><td>8</td><td>-</td><td>2D</td></tr>
</table>


</p>
<?php EndSection(); ?>


<?php PrintSettings(); ?>

<?php EndPage(); ?>


/* empty file for compile 
sdcard  is on local bus.
On some platform there is no sdcard on local bus,pmon/fs/devfs.c will compile error.
So we add this empty file.
If target has sdcard on local bus,will first include sdcard.h on Target dir,not this now.
 */


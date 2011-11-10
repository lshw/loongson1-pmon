#ifndef __PARTE_H
#define __PARTE_H

/* Interface types */
#define IF_TYPE_UNKNOWN		0
#define IF_TYPE_IDE		1
#define IF_TYPE_SCSI		2
#define IF_TYPE_ATAPI		3
#define IF_TYPE_USB		4
#define IF_TYPE_DOC		5
#define IF_TYPE_MMC		6

/* Part types */
#define PART_TYPE_UNKNOWN	0x00
#define PART_TYPE_MAC		0x01
#define PART_TYPE_DOS		0x02
#define PART_TYPE_ISO		0x03
#define PART_TYPE_AMIGA		0x04

/* device types */
#define DEV_TYPE_UNKNOWN    0xff    /* not connected */
#define DEV_TYPE_HARDDISK   0x00    /* harddisk */
#define DEV_TYPE_TAPE       0x01    /* Tape */
#define DEV_TYPE_CDROM      0x05    /* CD-ROM */
#define DEV_TYPE_OPDISK     0x07    /* optical disk */

#endif //__PARTE_H

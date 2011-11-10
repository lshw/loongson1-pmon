/*
 * Read flash partition table from Compaq Bootloader
 *
 * Copyright 2001 Compaq Computer Corporation.
 *
 * $Id: bootldr.h,v 1.1.1.1 2006/05/08 03:32:49 cpu Exp $
 *
 * Use consistent with the GNU GPL is permitted,
 * provided that this copyright notice is
 * preserved in its entirety in all copies and derived works.
 *
 * COMPAQ COMPUTER CORPORATION MAKES NO WARRANTIES, EXPRESSED OR IMPLIED,
 * AS TO THE USEFULNESS OR CORRECTNESS OF THIS CODE OR ITS
 * FITNESS FOR ANY PARTICULAR PURPOSE.
 *
 */

/*
 * Maintainer: Jamey Hicks (jamey.hicks@compaq.com)
 */

#ifndef _MTD_BOOTLDR_H_
#define _MTD_BOOTLDR_H_

#define BOOTLDR_PARTITION_NAMELEN 32

struct bootldr_flash_region {
    char name[BOOTLDR_PARTITION_NAMELEN];
    u32 base;
    u32 size;
    u32 flags;
};

#define BOOTLDR_MAGIC      0x646c7462        /* btld: marks a valid bootldr image */
#define BOOTLDR_PARTITION_MAGIC  0x646c7470  /* btlp: marks a valid bootldr partition table in params sector */

#define BOOTLDR_MAGIC_OFFSET 0x20 /* offset 0x20 into the bootldr */
#define BOOTCAP_OFFSET 0X30 /* offset 0x30 into the bootldr */

#define BOOTCAP_WAKEUP	(1<<0)
#define BOOTCAP_PARTITIONS (1<<1) /* partition table stored in params sector */
#define BOOTCAP_PARAMS_AFTER_BOOTLDR (1<<2) /* params sector right after bootldr sector(s), else in last sector */

// the tags are parsed too early to malloc or alloc_bootmem so we'll fix it
// for now
#define MAX_NUM_PARTITIONS 8
struct tag_ptable
{
    u32 magic; /* should be filled with 0x646c7470 (btlp) BOOTLDR_PARTITION_MAGIC */
    u32 npartitions;
    struct bootldr_flash_region partition[MAX_NUM_PARTITIONS];
};

#endif

/* keep the unofficial ptag entry until we get bootldr's updated */ 
#define ATAG_PTABLE_COMPAT 0x5441000A /* unofficial value we were using -- will delete */

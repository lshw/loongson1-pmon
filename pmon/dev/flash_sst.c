/*	$Id: flash_sst.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2001 ipUnplugged AB   (www.ipunplugged.com)
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by
 *	ipUnplugged AB, Sweden.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <machine/pio.h>

#include <pmon.h>

#include <pflash.h>
#include <dev/pflash_tgt.h>



extern void delay __P((int));

int fl_erase_chip_sst __P((struct fl_map *, struct fl_device *));
int fl_erase_sector_sst __P((struct fl_map *, struct fl_device *, int ));
int fl_isbusy_sst __P((struct fl_map *, struct fl_device *, int, int, int));
int fl_reset_sst __P((struct fl_map *, struct fl_device *));
int fl_erase_suspend_sst __P((struct fl_map *, struct fl_device *));
int fl_erase_resume_sst __P((struct fl_map *, struct fl_device *));
int fl_program_sst __P((struct fl_map *, struct fl_device *, int , unsigned char *));

struct fl_functions fl_func_sst = {fl_erase_chip_sst,
                                   fl_erase_sector_sst,
                                   fl_isbusy_sst,
                                   fl_reset_sst,
                                   fl_erase_suspend_sst,
                                   fl_program_sst};

/*
 *   FLASH Programming functions.
 *
 *   Handle flash programing aspects such as different types
 *   organizations and actual HW design organization/interleaving.
 *   Combinations can be endless but an attempt to make something
 *   more MI may be done later when the variations become clear.
 */

static quad_t widedata;

#define	SETWIDE(x) do {						\
			u_int32_t __a = x;			\
			__a |= __a << 8;				\
			__a |= __a << 16;				\
			widedata = (quad_t)__a << 32 | __a;		\
		} while(0)




/*
 *  Function to erase sector
 */
int
fl_erase_sector_sst(map, dev, offset)
	struct fl_map *map;
	struct fl_device *dev;
	int offset;
{
int stat;

	switch(map->fl_map_bus) {
	case FL_BUS_8:
		outb((map->fl_map_base + SST_CMDOFFS1), 0xAA);
		outb((map->fl_map_base + SST_CMDOFFS2), 0x55);
		outb((map->fl_map_base + SST_CMDOFFS1), FL_ERASE);
		outb((map->fl_map_base + SST_CMDOFFS1), 0xAA);
		outb((map->fl_map_base + SST_CMDOFFS2), 0x55);
		outb((map->fl_map_base + offset), FL_SECT);
		break;
	case FL_BUS_64:
		break;	/* Leave this for now */
	case FL_BUS_8_ON_64:
		SETWIDE(0xaa);
		movequad((void *)map->fl_map_base + (0x5555 << 3), &widedata);
		SETWIDE(0x55);
		movequad((void *)map->fl_map_base + (0x2aaa << 3), &widedata);
		break;
	}

	return(0);
}

/*
 *  Function to Program
 */
int
fl_program_sst(map, dev, pa, pd)
	struct fl_map *map;
	struct fl_device *dev;
	int pa;
	unsigned char *pd;
{
	int stat;

	switch(map->fl_map_bus) {
	case FL_BUS_8:
		outb((map->fl_map_base + SST_CMDOFFS1), 0xAA);
		outb((map->fl_map_base + SST_CMDOFFS2), 0x55);
		outb((map->fl_map_base + SST_CMDOFFS1), 0xA0);
		outb((map->fl_map_base + pa), *pd);
		break;
	case FL_BUS_8_ON_64:
		SETWIDE(0xaa);
		movequad((void *)map->fl_map_base + (0x5555 << 3), &widedata);
		SETWIDE(0x55);
		movequad((void *)map->fl_map_base + (0x2aaa << 3), &widedata);
		SETWIDE(*pd);
		movequad((void *)map->fl_map_base + (pa << 3), &widedata);
		break;
	case FL_BUS_64:
		break;	/* Leave this for now */
	}
        do {
                stat = fl_isbusy_sst(map, dev, *pd, pa, FALSE);
        } while(stat == 1);  
	return(0);
}

/*
 *  Function to erase chip
 */
int
fl_erase_chip_sst(map, dev)
	struct fl_map *map;
	struct fl_device *dev;
{

	switch(map->fl_map_bus) {
	case FL_BUS_8:
		outb((map->fl_map_base + SST_CMDOFFS1), 0xaa);
		outb((map->fl_map_base + SST_CMDOFFS2), 0x55);
		outb((map->fl_map_base + SST_CMDOFFS1), FL_ERASE);
		outb((map->fl_map_base + SST_CMDOFFS1), 0xaa);
		outb((map->fl_map_base + SST_CMDOFFS2), 0x55);
		outb((map->fl_map_base + SST_CMDOFFS1), FL_ERASE_CHIP);
		break;
	case FL_BUS_64:
		break;	/* Leave this for now */
	case FL_BUS_8_ON_64:
		SETWIDE(0xaa);
		movequad((void *)map->fl_map_base + (0x5555 << 3), &widedata);
		SETWIDE(0x55);
		movequad((void *)map->fl_map_base + (0x2aaa << 3), &widedata);
		break;
	}
	return(0);
}

/*
 *  Function to "reset" flash, eg return to read mode.
 */
int
fl_reset_sst(map, dev)
	struct fl_map *map;
	struct fl_device *dev;
{
	switch(map->fl_map_bus) {
	case FL_BUS_8:
		outb((map->fl_map_base), FL_RESET);
		break;
	case FL_BUS_8_ON_64:
		SETWIDE(FL_RESET);
		movequad((void *)map->fl_map_base, &widedata);
		break;
	case FL_BUS_64:
		break;
	}
        return(0);
}

/*
 *  Function to poll flash BUSY if available.
 *  returns 1 if busy, 0 if OK, -1 if error.
 */
int
fl_isbusy_sst(map, dev, what, offset, erase)
	struct fl_device *dev;
	struct fl_map *map;
	int what;
        int offset;
	int erase;
{
	int busy;

	switch(map->fl_map_bus) {
	case FL_BUS_8_ON_64:
		offset = offset << 3;
		/* Fallthrough */
	case FL_BUS_8:
		/* Data polling 
		 *  algorithm is in Figure 6
		 */

		while (1) {
			u_int8_t poll1, poll2;
			poll1 = inb(map->fl_map_base + offset);
			poll2 = inb(map->fl_map_base + offset);
			if ((poll1 ^ poll2) & 0x40) {   /* toggle */ 
				busy=1;
				break;
			}
			else { 
				poll1 = inb(map->fl_map_base + offset);
				poll2 = inb(map->fl_map_base + offset);
				if ((poll1 ^ poll2) & 0x40) busy=1;
				else busy = 0;
				break;   /* program completed */
			}
		}
		break;
	default:;
		/* Not supported but sorted out much earlier */
	}
	return(busy);
}

/*
 *  Function to suspend erase sector
 */

int
fl_erase_suspend_sst(map, dev)
	struct fl_map *map;
	struct fl_device *dev;
{

	return(0);
}

/*
 *  Function to Disable the LPC write protection of SST49LF040B/Disable the FIRMWARE HUB write protection of SST49LF008A
 *  For Loongson3 by wanghuandong(AdonWang, whd)
 */

	int
fl_write_protect_unlock(map, dev, offset)
	struct fl_map *map;
	struct fl_device *dev;
	int offset;
{
	unsigned int trans_unlock_value;
	unsigned int value;

	if (!((((dev->fl_mfg & 0xff) == 0xbf) && ((dev->fl_id & 0xff) == 0x50))   //NOT SST49LF040B
				||(((dev->fl_mfg & 0xff) == 0xbf) && ((dev->fl_id & 0xff) == 0x5a)))) /* NOT SST49LF008A */
		return(0);

	if (((dev->fl_mfg & 0xff) == 0xbf) && ((dev->fl_id & 0xff) == 0x50)) /* SST49LF040B */
	{
		printf("Disable all space write protection of 49LF040B. \r\n");
		/* Open translation of 0xbc000000 - 0xbd00000 */
		trans_unlock_value = inl(0xbff10200);
		outl(0xbff10200, (0x00ff0000 | trans_unlock_value));

		/* Disable all space write protection */
		outb(0xbdbf0002, 0x0);
		outb(0xbdbe0002, 0x0);
		outb(0xbdbd0002, 0x0);
		outb(0xbdbc0002, 0x0);
		outb(0xbdbb0002, 0x0);
		outb(0xbdba0002, 0x0);
		outb(0xbdb90002, 0x0);
		outb(0xbdb80002, 0x0);

		outl(0xbff10200, trans_unlock_value);
	}
	else if (((dev->fl_mfg & 0xff) == 0xbf) && ((dev->fl_id & 0xff) == 0x5a))   /* SST49LF008A */
	{
		printf("Disable all space write protection of 49LF008A. \r\n");
		/* Open translation of 0xbc000000 - 0xbd00000 */
		trans_unlock_value = inl(0xbff10200);
		outl(0xbff10200, (0x00870000 | trans_unlock_value));
		/* Enable firmware memory access */
		value = inl(0xbff10204);
		value |= 1 << 31;
		outl(0xbff10204, value);

		/* Disable all space write protection */
		outb(0xbdbf0002, 0x0);
		outb(0xbdbe0002, 0x0);
		outb(0xbdbd0002, 0x0);
		outb(0xbdbc0002, 0x0);
		outb(0xbdbb0002, 0x0);
		outb(0xbdba0002, 0x0);
		outb(0xbdb90002, 0x0);
		outb(0xbdb80002, 0x0);
		outb(0xbdb70002, 0x0);
		outb(0xbdb60002, 0x0);
		outb(0xbdb50002, 0x0);
		outb(0xbdb40002, 0x0);
		outb(0xbdb30002, 0x0);
		outb(0xbdb20002, 0x0);
		outb(0xbdb10002, 0x0);
		outb(0xbdb00002, 0x0);

		outl(0xbff10200, trans_unlock_value);
	}
	return(1);
}

/*
 *  Function to enable the LPC write protection of SST49LF040B/enable the FIRMWARE HUB write protection of SST49LF008A
 *  For Loongson3 by wanghuandong(AdonWang, whd)
 */

	int
fl_write_protect_lock(map, dev, offset)
	struct fl_map *map;
	struct fl_device *dev;
	int offset;
{
	unsigned int trans_unlock_value;
	unsigned int value;

	if (!((((dev->fl_mfg & 0xff) == 0xbf) && ((dev->fl_id & 0xff) == 0x50))   /* NOT SST49LF040B */
				||(((dev->fl_mfg & 0xff) == 0xbf) && ((dev->fl_id & 0xff) == 0x5a)))) /* NOT SST49LF008A */
		return(0);

	if (((dev->fl_mfg & 0xff) == 0xbf) && ((dev->fl_id & 0xff) == 0x50)) /* SST49LF040B */
	{
		printf("Enable all space write protection of 49LF040B. \r\n");
		/* Open translation of 0xbc000000 - 0xbd00000 */
		trans_unlock_value = inl(0xbff10200);
		outl(0xbff10200, (0x00ff0000 | trans_unlock_value));

		/* Enable all space write protection */
		outb(0xbdbf0002, 0x1);
		outb(0xbdbe0002, 0x1);
		outb(0xbdbd0002, 0x1);
		outb(0xbdbc0002, 0x1);
		outb(0xbdbb0002, 0x1);
		outb(0xbdba0002, 0x1);
		outb(0xbdb90002, 0x1);
		outb(0xbdb80002, 0x1);

		outl(0xbff10200, trans_unlock_value);
	}
	else if (((dev->fl_mfg & 0xff) == 0xbf) && ((dev->fl_id & 0xff) == 0x5a))   /* SST49LF008A */
	{
		printf("Enable all space write protection of 49LF008A. \r\n");
		/* Open translation of 0xbc000000 - 0xbd00000 */
		trans_unlock_value = inl(0xbff10200);
		outl(0xbff10200, (0x00870000 | trans_unlock_value));
		/* Enable firmware memory access */
		value = inl(0xbff10204);
		value |= 1 << 31;
		outl(0xbff10204, value);

		/* Enable all space write protection */
		outb(0xbdbf0002, 0x1);
		outb(0xbdbe0002, 0x1);
		outb(0xbdbd0002, 0x1);
		outb(0xbdbc0002, 0x1);
		outb(0xbdbb0002, 0x1);
		outb(0xbdba0002, 0x1);
		outb(0xbdb90002, 0x1);
		outb(0xbdb80002, 0x1);
		outb(0xbdb70002, 0x1);
		outb(0xbdb60002, 0x1);
		outb(0xbdb50002, 0x1);
		outb(0xbdb40002, 0x1);
		outb(0xbdb30002, 0x1);
		outb(0xbdb20002, 0x1);
		outb(0xbdb10002, 0x1);
		outb(0xbdb00002, 0x1);

		outl(0xbff10200, trans_unlock_value);
	}
	return(1);
}

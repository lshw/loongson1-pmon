/*	$Id: flash_winbond.c,v 1.1.1.1 2006/06/29 06:43:25 cpu Exp $ */

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

int fl_erase_chip_winbond __P((struct fl_map *, struct fl_device *));
int fl_erase_sector_winbond __P((struct fl_map *, struct fl_device *, int ));
int fl_isbusy_winbond __P((struct fl_map *, struct fl_device *, int, int, int));
int fl_reset_winbond __P((struct fl_map *, struct fl_device *));
int fl_erase_suspend_winbond __P((struct fl_map *, struct fl_device *));
int fl_erase_resume_winbond __P((struct fl_map *, struct fl_device *));
int fl_program_winbond __P((struct fl_map *, struct fl_device *, int , unsigned char *));

struct fl_functions fl_func_winbond = {fl_erase_chip_winbond,
                                   fl_erase_sector_winbond,
                                   fl_isbusy_winbond,
                                   fl_reset_winbond,
                                   fl_erase_suspend_winbond,
                                   fl_program_winbond};

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
fl_erase_sector_winbond(map, dev, offset)
	struct fl_map *map;
	struct fl_device *dev;
	int offset;
{
	int stat;
	int count = 0xfffff;

	switch(map->fl_map_bus) {
	case FL_BUS_8:
		outb((map->fl_map_base + WINBOND_CMDOFFS1), 0xAA);
		delay(10);
		outb((map->fl_map_base + WINBOND_CMDOFFS2), 0x55);
		delay(10);
		outb((map->fl_map_base + WINBOND_CMDOFFS1), FL_ERASE);
		delay(10);
		outb((map->fl_map_base + WINBOND_CMDOFFS1), 0xAA);
		delay(10);
		outb((map->fl_map_base + WINBOND_CMDOFFS2), 0x55);
		delay(10);
		outb((map->fl_map_base + offset), FL_SECT);
		delay(10);
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
        do {
                stat = fl_isbusy_winbond(map, dev, 0, 0, FALSE);
        } while(stat == 1 && count-- > 0);  
	return(0);
}

/*
 *  Function to Program
 */
int
fl_program_winbond(map, dev, pa, pd)
	struct fl_map *map;
	struct fl_device *dev;
	int pa;
	unsigned char *pd;
{
	int stat;
	int count = 0xfffff;

	switch(map->fl_map_bus) {
	case FL_BUS_8:
		outb((map->fl_map_base + WINBOND_CMDOFFS1), 0xAA);
		outb((map->fl_map_base + WINBOND_CMDOFFS2), 0x55);
		outb((map->fl_map_base + WINBOND_CMDOFFS1), 0xA0);
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
                stat = fl_isbusy_winbond(map, dev, *pd, pa, FALSE);
        } while(stat == 1 && count-- > 0);  
	return(0);
}

/*
 *  Function to erase chip
 */
int
fl_erase_chip_winbond(map, dev)
	struct fl_map *map;
	struct fl_device *dev;
{

	switch(map->fl_map_bus) {
	case FL_BUS_8:
		outb((map->fl_map_base + WINBOND_CMDOFFS1), 0xaa);
		delay(10);
		outb((map->fl_map_base + WINBOND_CMDOFFS2), 0x55);
		delay(10);
		outb((map->fl_map_base + WINBOND_CMDOFFS1), FL_ERASE);
		delay(10);
		outb((map->fl_map_base + WINBOND_CMDOFFS1), 0xaa);
		delay(10);
		outb((map->fl_map_base + WINBOND_CMDOFFS2), 0x55);
		delay(10);
		outb((map->fl_map_base + WINBOND_CMDOFFS1), FL_ERASE_CHIP);
		delay(10);
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
fl_reset_winbond(map, dev)
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
fl_isbusy_winbond(map, dev, what, offset, erase)
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
fl_erase_suspend_winbond(map, dev)
	struct fl_map *map;
	struct fl_device *dev;
{

	return(0);
}


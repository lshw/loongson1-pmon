/*	$Id: flash_int.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

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

int fl_erase_chip_int __P((struct fl_map *, struct fl_device *));
int fl_erase_sector_int __P((struct fl_map *, struct fl_device *, int ));
int fl_isbusy_int __P((struct fl_map *, struct fl_device *, int, int, int));
int fl_reset_int __P((struct fl_map *, struct fl_device *));
int fl_erase_suspend_int __P((struct fl_map *, struct fl_device *));
int fl_erase_resume_int __P((struct fl_map *, struct fl_device *));
int fl_program_int __P((struct fl_map *, struct fl_device *, int , unsigned char *));

struct fl_functions fl_func_int = {fl_erase_chip_int,
                                   fl_erase_sector_int,
                                   fl_isbusy_int,
                                   fl_reset_int,
                                   fl_erase_suspend_int,
                                   fl_program_int};

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
 *  Inlineable function to send INT command to flash.
 */
static __inline void fl_command_int __P((struct fl_map *, int, int));
static __inline void
fl_command_int(map, command, offset)
	struct fl_map *map;
	int command;
	int offset;
{
	delay(5);
	switch(map->fl_map_bus) {
	case FL_BUS_8:
		outb(map->fl_map_base + offset, command);
		break;

	case FL_BUS_16:
		SETWIDE(command);
		outw(map->fl_map_base + offset, widedata);
		break;

	case FL_BUS_32:
		SETWIDE(command);
		outl(map->fl_map_base + offset, widedata);
		break;

	case FL_BUS_64:
		SETWIDE(command);
		movequad((void *)(map->fl_map_base + offset), &widedata);
		break;
	}
}

/*
 *  Function to erase sector
 */
int
fl_erase_sector_int(map, dev, offset)
	struct fl_map *map;
	struct fl_device *dev;
	int offset;
{
	fl_command_int(map, FL_INT_BE, 0);
	fl_command_int(map, FL_INT_CNF, (int)offset);
	return(0);
}

/*
 *  Function to Program
 */
int
fl_program_int(map, dev, pa, pd)
	struct fl_map *map;
	struct fl_device *dev;
	int pa;
	unsigned char *pd;
{
	int stat;

	fl_command_int(map, FL_INT_PGM, 0);

	switch(map->fl_map_bus) {
	case FL_BUS_8:
		*(u_char *)(map->fl_map_base + pa) = *pd;
		break;

	case FL_BUS_16:
		*(u_short *)(map->fl_map_base + pa) = *(u_short *)pd;
		break;

	case FL_BUS_32:
		*(u_int *)(map->fl_map_base + pa) = *(u_int *)pd;
		break;

	case FL_BUS_64:
		bcopy(pd, (void *)&widedata, 8);
		movequad((void *)(map->fl_map_base + pa), &widedata);
		break; 
	}
	do {
		stat = fl_isbusy_int(map, dev, 0, pa, FALSE);
	} while(stat == 1);

	return(stat);
}

/*
 *  Function to erase chip
 */
int
fl_erase_chip_int(map, dev)
	struct fl_map *map;
	struct fl_device *dev;
{
	if(dev->fl_cap & FL_CAP_A7) {
		fl_command_int(map, FL_INT_FCE_A7, 0);
	}
	else {
		fl_command_int(map, FL_INT_FCE, 0);
	}
	fl_command_int(map, FL_INT_CNF, 0);
	return(0);
}

/*
 *  Inlineable function to suspend erase sector
 */
int
fl_erase_suspend_int(map, dev)
	struct fl_map *map;
	struct fl_device *dev;
{
	return(0);
}

/*
 *  Inlineable function to resume erase sector
 */
int
fl_erase_resume_int(map, dev)
	struct fl_map *map;
	struct fl_device *dev;
{

	switch(map->fl_map_bus) {
	case FL_BUS_8:
		outb(map->fl_map_base, FL_RESUME);
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
 *  Inlineable function to "reset" flash, eg return to read mode.
 */
int
fl_reset_int(map, dev)
	struct fl_map *map;
	struct fl_device *dev;
{
#if 1
	fl_command_int(map, FL_INT_CLR, 0);
	fl_command_int(map, FL_INT_READ, 0);
#else
	switch(map->fl_map_bus) {
	case FL_BUS_8:
		outb((map->fl_map_base), FL_RESET);
		break;

	case FL_BUS_16:
		SETWIDE(FL_RESET);
		outw(map->fl_map_base + offset, widedata);
		break;

	case FL_BUS_32:
		SETWIDE(FL_RESET);
		outl(map->fl_map_base + offset, widedata);
		break;

	case FL_BUS_8_ON_64:
		SETWIDE(FL_RESET);
		movequad((void *)map->fl_map_base, &widedata);
		break;
	case FL_BUS_64:
		fl_command_int(map, FL_INT_READ, 0);
		break;
	}
#endif
        return(0);
}

/*
 *  Generic function to poll flash BUSY if available.
 *  returns 1 if busy, 0 if OK, -1 if error.
 */
int
fl_isbusy_int(map, dev, what, offset, erase)
	struct fl_device *dev;
	struct fl_map *map;
	int what;
        int offset;
	int erase;
{
	u_char *p;
	int d;

	switch(map->fl_map_bus) {
	case FL_BUS_8:
		d = !(inb(map->fl_map_base) & FL_INT_BUSY);
		break;

	case FL_BUS_16:
		d = !(inw(map->fl_map_base) & FL_INT_BUSY);
		break;

	case FL_BUS_32:
		d = inl(map->fl_map_base);
		d &= (FL_INT_BUSY << 16) | FL_INT_BUSY;
		d = d != ((FL_INT_BUSY << 16) | FL_INT_BUSY);
		break;

	case FL_BUS_64:
		movequad(&widedata, (void *)map->fl_map_base);
		p = (u_char *)&widedata;
		d = !((p[1] & p[3] & p[5] & p[7]) & FL_INT_BUSY);
		break;
	}
	return(d);
}


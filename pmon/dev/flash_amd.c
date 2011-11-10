/*	$Id: flash_amd.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

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

int fl_erase_chip_amd __P((struct fl_map *, struct fl_device *));
int fl_erase_sector_amd __P((struct fl_map *, struct fl_device *, int ));
int fl_isbusy_amd __P((struct fl_map *, struct fl_device *, int, int, int));
int fl_reset_amd __P((struct fl_map *, struct fl_device *));
int fl_erase_suspend_amd __P((struct fl_map *, struct fl_device *));
int fl_erase_resume_amd __P((struct fl_map *, struct fl_device *));
int fl_program_amd __P((struct fl_map *, struct fl_device *, int , unsigned char *));

struct fl_functions fl_func_amd = {fl_erase_chip_amd,
                                   fl_erase_sector_amd,
                                   fl_isbusy_amd,
                                   fl_reset_amd,
                                   fl_erase_suspend_amd,
                                   fl_program_amd};

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

#define ConvAddr2(A) (A*2)

/*
 *  Inlineable function to Unlock Bypass 
 */
static __inline void fl_unlock_bypass_amd __P((struct fl_map *));
static __inline void
fl_unlock_bypass_amd(map)
	struct fl_map *map;
{

	switch(map->fl_map_bus) {
	case FL_BUS_8:
		outb((map->fl_map_base + AMD_CMDOFFS1), 0xaa);
		outb((map->fl_map_base + AMD_CMDOFFS2), 0x55);
		outb((map->fl_map_base + AMD_CMDOFFS1), 0x20);
		break;
	case FL_BUS_16:
		outw((map->fl_map_base + ConvAddr2(AMD_CMDOFFS1)), 0xaa);
		outw((map->fl_map_base + ConvAddr2(AMD_CMDOFFS2)), 0x55);
		outw((map->fl_map_base + ConvAddr2(AMD_CMDOFFS1)), 0x20);
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
}

/*
 *  Inlineable function to Unlock Bypass 
 */
static __inline void fl_unlock_bypass_reset_amd __P((struct fl_map *));
static __inline void
fl_unlock_bypass_reset_amd(map)
	struct fl_map *map;
{

	switch(map->fl_map_bus) {
	case FL_BUS_8:
	case FL_BUS_16:
		outb((map->fl_map_base), 0x90);
		outb((map->fl_map_base), 0x00);
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
}

/*
 *  Inlineable function to Unlock Bypass 
 */
static __inline void fl_unlock_bypass_program_amd __P((struct fl_map *, int , int ));
static __inline void
fl_unlock_bypass_program_amd(map, pa, pd)
	struct fl_map *map;
	int pa;
	int pd;
{

	switch(map->fl_map_bus) {
	case FL_BUS_8:
		outb((map->fl_map_base + AMD_CMDOFFS1), 0xaa);
		outb((map->fl_map_base + AMD_CMDOFFS2), 0x55);
		break;
	case FL_BUS_16:
		outw((map->fl_map_base + ConvAddr2(AMD_CMDOFFS1)), 0xaa);
		outw((map->fl_map_base + ConvAddr2(AMD_CMDOFFS2)), 0x55);
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
}

/*
 *  Function to erase sector
 */
int
fl_erase_sector_amd(map, dev, offset)
	struct fl_map *map;
	struct fl_device *dev;
	int offset;
{

	switch(map->fl_map_bus) {
	case FL_BUS_8:
		outb((map->fl_map_base + AMD_CMDOFFS1), 0xAA);
		outb((map->fl_map_base + AMD_CMDOFFS2), 0x55);
		outb((map->fl_map_base + AMD_CMDOFFS1), FL_ERASE);
		outb((map->fl_map_base + AMD_CMDOFFS1), 0xAA);
		outb((map->fl_map_base + AMD_CMDOFFS2), 0x55);
		outb((map->fl_map_base + offset), FL_SECT);
		break;
	case FL_BUS_16:
		outw((map->fl_map_base + ConvAddr2(AMD_CMDOFFS1)), 0xAA);
		outw((map->fl_map_base + ConvAddr2(AMD_CMDOFFS2)), 0x55);
		outw((map->fl_map_base + ConvAddr2(AMD_CMDOFFS1)), FL_ERASE);
		outw((map->fl_map_base + ConvAddr2(AMD_CMDOFFS1)), 0xAA);
		outw((map->fl_map_base + ConvAddr2(AMD_CMDOFFS2)), 0x55);
		outw((map->fl_map_base + offset), FL_SECT);
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
fl_program_amd(map, dev, pa, pd)
	struct fl_map *map;
	struct fl_device *dev;
	int pa;
	unsigned char *pd;
{
	int stat;

	switch(map->fl_map_bus) {
	case FL_BUS_8:
		outb((map->fl_map_base + AMD_CMDOFFS1), 0xAA);
		outb((map->fl_map_base + AMD_CMDOFFS2), 0x55);
		outb((map->fl_map_base + AMD_CMDOFFS1), 0xA0);
		outb((map->fl_map_base + pa), *pd);
		break;
	case FL_BUS_16:
		outw((map->fl_map_base + ConvAddr2(AMD_CMDOFFS1)), 0xAA);
		outw((map->fl_map_base + ConvAddr2(AMD_CMDOFFS2)), 0x55);
		outw((map->fl_map_base + ConvAddr2(AMD_CMDOFFS1)), 0xA0);
		outw((map->fl_map_base + pa), ((int)pd[1]<<8)|pd[0]);
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
                stat = fl_isbusy_amd(map, dev, *pd, pa, FALSE);
        } while(stat == 1);  
	return(0);
}

/*
 *  Function to erase chip
 */
int
fl_erase_chip_amd(map, dev)
	struct fl_map *map;
	struct fl_device *dev;
{

	switch(map->fl_map_bus) {
	case FL_BUS_8:
		outb((map->fl_map_base + AMD_CMDOFFS1), 0xaa);
		outb((map->fl_map_base + AMD_CMDOFFS2), 0x55);
		outb((map->fl_map_base + AMD_CMDOFFS1), FL_ERASE);
		outb((map->fl_map_base + AMD_CMDOFFS1), 0xaa);
		outb((map->fl_map_base + AMD_CMDOFFS2), 0x55);
		outb((map->fl_map_base + AMD_CMDOFFS1), FL_ERASE_CHIP);
		break;
	case FL_BUS_16:
		outw((map->fl_map_base + ConvAddr2(AMD_CMDOFFS1)), 0xaa);
		outw((map->fl_map_base + ConvAddr2(AMD_CMDOFFS2)), 0x55);
		outw((map->fl_map_base + ConvAddr2(AMD_CMDOFFS1)), FL_ERASE);
		outw((map->fl_map_base + ConvAddr2(AMD_CMDOFFS1)), 0xaa);
		outw((map->fl_map_base + ConvAddr2(AMD_CMDOFFS2)), 0x55);
		outw((map->fl_map_base + ConvAddr2(AMD_CMDOFFS1)), FL_ERASE_CHIP);
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
 *  Function to suspend erase sector
 */
int
fl_erase_suspend_amd(map, dev)
	struct fl_map *map;
	struct fl_device *dev;
{

	switch(map->fl_map_bus) {
	case FL_BUS_16:
	case FL_BUS_8:
		outb(map->fl_map_base, FL_SUSPEND);
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
 *  Function to resume erase sector
 */
int
fl_erase_resume_amd(map, dev)
	struct fl_map *map;
	struct fl_device *dev;
{

	switch(map->fl_map_bus) {
	case FL_BUS_16:
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
 *  Function to "reset" flash, eg return to read mode.
 */
int
fl_reset_amd(map, dev)
	struct fl_map *map;
	struct fl_device *dev;
{
	switch(map->fl_map_bus) {
	case FL_BUS_16:
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
fl_isbusy_amd(map, dev, what, offset, erase)
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
#if defined(CPC700) || defined(OCELOT_G)
		if(erase) {
			int d;
			d = inb(map->fl_map_base + offset);
			d ^= inb(map->fl_map_base + offset + 1);
			if((d & 0x40) == 0) { /* Not toggling */
				busy = 0;
			}
			else {
				if(!(inb(map->fl_map_base + offset) & 0x20)) {
					busy = 1;  /* Check again */
				}
				else {
					d = inb(map->fl_map_base + offset);
					d ^= inb(map->fl_map_base + offset + 1);
					if((d & 0x40) == 1) {
						busy = 1;
					}
					else {
						busy = -1;
					}
				}
			}
		}
		else {
			int d;
			d = inb(map->fl_map_base + offset);
			if(((d ^ what) & 0x80) == 0) {
				busy = 0;       /* Done */
			}
			else if((d & 0x20) == 0) {
				busy = 1;       /* Busy */
			}
			else {
				d = inb(map->fl_map_base + offset);
				if(((d ^ what) & 0x80) == 0) {
					busy = 0;       /* Done */
				}
				else {
					busy = -1;      /* Poll error */
				}
			}
		} 
#else

		while (1) {
			u_int8_t poll1, poll2;
			poll1 = inb(map->fl_map_base + offset);
			poll2 = inb(map->fl_map_base + offset);
			if ((poll1 ^ poll2) & 0x40) {   /* toggle */ 
				if (poll2 & 0x20) {     /* DQ5 = 1 */
					/* read twice */
					poll1 = inb(map->fl_map_base + offset);
					poll2 = inb(map->fl_map_base + offset);
					if ((poll1 ^ poll2) & 0x40) {   /* toggle */
    	 					busy = -1;
						break;
					}
					else {
						busy = 0;
						break;   /* program completed */
					}
				}
				else {
					poll1 = poll2;
					continue;
				}
			}
			else { 
				busy = 0;
				break;   /* program completed */
			}
		}
#endif
		break;
	case FL_BUS_16:
		while (1) {
			unsigned short poll1, poll2;
			poll2 = inw(map->fl_map_base + offset);
			poll1 = inw(map->fl_map_base + offset);

			if((poll1&0x0040)==(poll2&0x0040))return 0;
			if((poll2&0x20)!=0x20)return 1;
			poll1 = inw(map->fl_map_base + offset);
			poll2 = inw(map->fl_map_base + offset);
			if((poll1&0x0040)==(poll2&0x0040))return 0;
			else return -1;
		}
		break;
	default:;
		/* Not supported but sorted out much earlier */
	}
	return(busy);
}


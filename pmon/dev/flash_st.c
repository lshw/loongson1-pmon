/*	$Id: flash_st.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

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

int fl_erase_chip_st __P((struct fl_map *, struct fl_device *));
int fl_erase_sector_st __P((struct fl_map *, struct fl_device *, int ));
int fl_isbusy_st __P((struct fl_map *, struct fl_device *, int, int, int));
int fl_reset_st __P((struct fl_map *, struct fl_device *));
int fl_erase_suspend_st __P((struct fl_map *, struct fl_device *));
int fl_erase_resume_st __P((struct fl_map *, struct fl_device *));
int fl_program_st __P((struct fl_map *, struct fl_device *, int , unsigned char *));

struct fl_functions fl_func_st = {fl_erase_chip_st,
                                   fl_erase_sector_st,
                                   fl_isbusy_st,
                                   fl_reset_st,
                                   fl_erase_suspend_st,
                                   fl_program_st};

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


#ifdef USE_M29W640FB_16 /* The M29W640FB device 16 bit */
   #define MANUFACTURER_ST (0x0020)  /* ST Manufacturer Code is 0x20 */
   #define EXPECTED_DEVICE (0x22FD)  /* Device code for the M29W640FB */
   #define FLASH_SIZE (0x400000)       /* Total device size in Words */
#endif /* USE_M29W640FB_16 */

#define ConvAddr1(A) (2*A+!(A&0x1))  /* Convert a word mode command to byte mode command :
                                           Word Mode Command    Byte Mode Command
                                                0x555      ->     0xAAA
                                                0x2AA      ->     0x555
                                                0x55       ->     0xAA            */

#define ConvAddr2(A) (A*2)
/*
 *  Function to erase sector
 */
int
fl_erase_sector_st(map, dev, offset)
	struct fl_map *map;
	struct fl_device *dev;
	int offset;
{
int stat;

	switch(map->fl_map_bus) {
	case FL_BUS_8:

		outb((map->fl_map_base + ConvAddr1(0x555)), 0xAA);
		outb((map->fl_map_base + ConvAddr1(0x2AA)), 0x55);
		outb((map->fl_map_base + ConvAddr1(0x555)), FL_ERASE);
		outb((map->fl_map_base + ConvAddr1(0x555)), 0xAA);
		outb((map->fl_map_base + ConvAddr1(0x2aa)), 0x55);
		outb((map->fl_map_base + offset), FL_SECT);
		break;
	case FL_BUS_16:
   		outw((map->fl_map_base + ConvAddr2(0x00555)), 0x00AA );
   		outw((map->fl_map_base + ConvAddr2(0x002AA)), 0x0055 );
   		outw((map->fl_map_base + ConvAddr2(0x00555)), FL_ERASE );
   		outw((map->fl_map_base + ConvAddr2(0x00555)), 0x00AA );
   		outw((map->fl_map_base + ConvAddr2(0x002AA)), 0x0055 );
   		outw((map->fl_map_base + offset), FL_SECT);
		break;
	case FL_BUS_64:
		break;	/* Leave this for now */
	case FL_BUS_8_ON_64:
		break;
	}

	return(0);
}

/*
 *  Function to Program
 */
int
fl_program_st(map, dev, pa, pd)
	struct fl_map *map;
	struct fl_device *dev;
	int pa;
	unsigned char *pd;
{
	int stat;

	switch(map->fl_map_bus) {
	case FL_BUS_8:
   outb( map->fl_map_base + ConvAddr1(0x00555), 0x00AA ); /* 1st cycle */
   outb( map->fl_map_base + ConvAddr1(0x002AA), 0x0055 ); /* 2nd cycle */
   outb(map->fl_map_base + ConvAddr1(0x0555), 0x00A0 ); /* 1st cycle */
   outb((map->fl_map_base +pa) , *pd ); /* 2nd Cycle */  
		break;
	case FL_BUS_16:
   /* Step 4: Issue the Unlock Bypass command */
   outw( (map->fl_map_base + ConvAddr2(0x00555)), 0x00AA ); /* 1st cycle */
   outw( (map->fl_map_base + ConvAddr2(0x002AA)), 0x0055 ); /* 2nd cycle */
   outw( map->fl_map_base + ConvAddr2(0x0555),0xA0 ); /* 1st cycle */
   outw( (map->fl_map_base + pa),((int)pd[1]<<8)|pd[0] ); /* 2nd Cycle */  
   break;

	case FL_BUS_8_ON_64:
		break;
	case FL_BUS_64:
		break;	/* Leave this for now */
	}
        do {
                stat = fl_isbusy_st(map, dev, *pd, pa, FALSE);
        } while(stat == 1);  
	return(0);
}

/*
 *  Function to erase chip
 */
int
fl_erase_chip_st(map, dev)
	struct fl_map *map;
	struct fl_device *dev;
{

	switch(map->fl_map_bus) {
	case FL_BUS_8:
		outb((map->fl_map_base + ConvAddr1(0x00555)), 0xaa);
		outb((map->fl_map_base + ConvAddr1(0x002AA)), 0x55);
		outb((map->fl_map_base + ConvAddr1(0x00555)), FL_ERASE);
		outb((map->fl_map_base + ConvAddr1(0x00555)), 0xaa);
		outb((map->fl_map_base + ConvAddr1(0x002AA)), 0x55);
		outb((map->fl_map_base + ConvAddr1(0x00555)), FL_ERASE_CHIP);
		break;
	case FL_BUS_16:
		outw((map->fl_map_base + ConvAddr2(0x00555)), 0xaa);
		outw((map->fl_map_base + ConvAddr2(0x002AA)), 0x55);
		outw((map->fl_map_base + ConvAddr2(0x00555)), FL_ERASE);
		outw((map->fl_map_base + ConvAddr2(0x00555)), 0xaa);
		outw((map->fl_map_base + ConvAddr2(0x002AA)), 0x55);
		outw((map->fl_map_base + ConvAddr2(0x00555)), FL_ERASE_CHIP);
		break;
	case FL_BUS_64:
		break;	/* Leave this for now */
	case FL_BUS_8_ON_64:
		break;
	}
	return(0);
}

/*
 *  Function to "reset" flash, eg return to read mode.
 */
int
fl_reset_st(map, dev)
	struct fl_map *map;
	struct fl_device *dev;
{
	switch(map->fl_map_bus) {
	case FL_BUS_8:
   outb((map->fl_map_base + ConvAddr1(0x00555)), 0x00AA ); /* 1st Cycle */
   outb((map->fl_map_base + ConvAddr1(0x002AA)), 0x0055 ); /* 2nd Cycle */
   outb(map->fl_map_base , FL_RESET ); /* 3rd Cycle: write 0x00F0 to ANY address */
		break;
	case FL_BUS_16:
   outw( (map->fl_map_base + ConvAddr2(0x00555)), 0x00AA ); /* 1st Cycle */
   outw( (map->fl_map_base + ConvAddr2(0x002AA)), 0x0055 ); /* 2nd Cycle */
   outw( map->fl_map_base, FL_RESET ); /* 3rd Cycle: write 0x00F0 to ANY address */
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
fl_isbusy_st(map, dev, what, offset, erase)
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
			poll2 = inb(map->fl_map_base + offset);
			poll1 = inb(map->fl_map_base + offset);

			if((poll1&0x0040)==(poll2&0x0040))return 0;
			if((poll2&0x20)!=0x20)return 1;
			poll1 = inb(map->fl_map_base + offset);
			poll2 = inb(map->fl_map_base + offset);
			if((poll1&0x0040)==(poll2&0x0040))return 0;
			else return -1;
		}
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

/*
 *  Function to suspend erase sector
 */

int
fl_erase_suspend_st(map, dev)
	struct fl_map *map;
	struct fl_device *dev;
{
	switch(map->fl_map_bus) {
	case FL_BUS_8:
   outb(map->fl_map_base,0x00B0);   
	case FL_BUS_16:
   outw(map->fl_map_base,0x00B0);   
   break;
	default:break;
	}

	return(0);
}


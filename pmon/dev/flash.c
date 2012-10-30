/*	$Id: flash.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2000 Opsycon AB  (www.opsycon.se)
 * Copyright (c) 2000 Rtmx, Inc   (www.rtmx.com)
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
 *	This product includes software developed for Rtmx, Inc by
 *	Opsycon Open System Consulting AB, Sweden.
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
#include <flash.h>
#include <dev/pflash_tgt.h>

extern void delay __P((int));

#ifndef PFLASH_MAX_TIMEOUT
#define PFLASH_MAX_TIMEOUT 16000
#endif

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
			__a |= __a << 8;			\
			__a |= __a << 16;			\
			widedata = (quad_t)__a << 32 | __a;	\
		} while(0)


/*
 *  Inlineable function to Auto Select Device
 */
#define TYPE_AMD 0x5a5a0001
#define TYPE_SST 0x5a5a0002
#define TYPE_ST 0x5a5a0003
#define ConvAddr1(A) (2*A+!(A&0x1))  /* Convert a word mode command to byte mode command :
                                           Word Mode Command    Byte Mode Command
                                                0x555      ->     0xAAA
                                                0x2AA      ->     0x555
                                                0x55       ->     0xAA            */

#define ConvAddr2(A) (A*2)
static __inline void fl_mydetect(struct fl_map *map)
{
		char oldc=inb(map->fl_map_base);	
        outb((map->fl_map_base + SST_CMDOFFS1), 0xAA);
        outb((map->fl_map_base + SST_CMDOFFS2), 0x55);
        outb((map->fl_map_base + SST_CMDOFFS1), FL_AUTOSEL);
        if(inb(map->fl_map_base)!=oldc){map->fl_type=TYPE_SST;return;}
        outb((map->fl_map_base + ConvAddr1(0x555)), 0xAA);
        outb((map->fl_map_base + ConvAddr1(0x2aa)), 0x55);
        outb((map->fl_map_base + ConvAddr1(0x555)), FL_AUTOSEL);
        if(inb(map->fl_map_base)!=oldc){map->fl_type=TYPE_ST;return;}
		outb((map->fl_map_base + AMD_CMDOFFS1), 0xAA);
		outb((map->fl_map_base + AMD_CMDOFFS2), 0x55);
		outb((map->fl_map_base + AMD_CMDOFFS1), FL_AUTOSEL);
        if(inb(map->fl_map_base)!=oldc){map->fl_type=TYPE_AMD;return;}
	    else {map->fl_type=0;printf("unknow flash type\n");	}
}

static __inline void
fl_autoselect(struct fl_map *map)
{

	switch(map->fl_map_bus) {
	case FL_BUS_8:
		if((map->fl_type>>16)!=0x5a5a)fl_mydetect(map);
#if NMOD_FLASH_SST || NMOD_FLASH_WINBOND	
		if(map->fl_type==TYPE_SST) //SST or WINBOND
		{
		outb((map->fl_map_base + SST_CMDOFFS1), 0xAA);
		outb((map->fl_map_base + SST_CMDOFFS2), 0x55);
		outb((map->fl_map_base + SST_CMDOFFS1), FL_AUTOSEL);
		break;
		}
#endif
#if NMOD_FLASH_AMD
		if(map->fl_type==TYPE_AMD)
		{
		outb((map->fl_map_base + AMD_CMDOFFS1), 0xAA);
		outb((map->fl_map_base + AMD_CMDOFFS2), 0x55);
		outb((map->fl_map_base + AMD_CMDOFFS1), FL_AUTOSEL);
		break;
		}
#endif
#if NMOD_FLASH_ST
		if(map->fl_type==TYPE_ST)
		{
		outb((map->fl_map_base + ConvAddr1(0x555)), 0xAA);
		outb((map->fl_map_base + ConvAddr1(0x2aa)), 0x55);
		outb((map->fl_map_base + ConvAddr1(0x555)), FL_AUTOSEL);
		break;
		}
#endif

		break;

	case FL_BUS_16:
	{
		short oldc=inw(map->fl_map_base);	
        outw((map->fl_map_base + ConvAddr2(SST_CMDOFFS1)), 0xAA);
        outw((map->fl_map_base + ConvAddr2(SST_CMDOFFS2)), 0x55);
        outw((map->fl_map_base + ConvAddr2(SST_CMDOFFS1)), FL_AUTOSEL);
        if(inw(map->fl_map_base)!=oldc){map->fl_type=TYPE_SST;return;}
		outw((map->fl_map_base + ConvAddr2(AMD_CMDOFFS1)), 0xAA);
		outw((map->fl_map_base + ConvAddr2(AMD_CMDOFFS2)), 0x55);
		outw((map->fl_map_base + ConvAddr2(AMD_CMDOFFS1)), FL_AUTOSEL);
        if(inw(map->fl_map_base)!=oldc){map->fl_type=TYPE_AMD;return;}
	    else {map->fl_type=0;printf("unknow flash type\n");	}
	}
		break;

	case FL_BUS_32:
		SETWIDE(0xaa);
		outl(map->fl_map_base + (0x5555 << 4), widedata);
		SETWIDE(0x55);
		outl(map->fl_map_base + (0xaaaa << 4), widedata);
		SETWIDE(FL_AUTOSEL);
		outl(map->fl_map_base + (0x5555 << 4), widedata);
		break;

	case FL_BUS_64:
	case FL_BUS_8_ON_64:
		SETWIDE(0xAA);
		movequad((void *)map->fl_map_base + (0x5555 << 3), &widedata);
		SETWIDE(0x55);
		movequad((void *)map->fl_map_base + (0x2AAA << 3), &widedata);
		SETWIDE(FL_AUTOSEL);
		movequad((void *)map->fl_map_base + (0x5555 << 3), &widedata);
		break;
	}
}

/*
 *  Inlineable function to Generic Reset the Chip
 */
static __inline void
fl_reset(struct fl_map *map)
{

	switch(map->fl_map_bus) {
	case FL_BUS_8:
		if(!map->fl_type)fl_mydetect(map);
#if NMOD_FLASH_SST
		if(map->fl_type==TYPE_SST)
		{
		outb((map->fl_map_base), 0xf0);
		break;
		}
#endif
#if NMOD_FLASH_AMD
        if(map->fl_type==TYPE_AMD)
        {
		outb((map->fl_map_base), 0x90);
		outb((map->fl_map_base), 0x00);
		break;
		}
#endif
#if NMOD_FLASH_ST
		if(map->fl_type==TYPE_ST)
		{
		outb(map->fl_map_base, 0xf0);
		break;
		}
#endif
		break;

        case FL_BUS_16:
                SETWIDE(FL_RESET);
                outw(map->fl_map_base, widedata);
                break;

        case FL_BUS_32:
                SETWIDE(FL_RESET);
                outl(map->fl_map_base, widedata);
                break;
       
	case FL_BUS_64:
	case FL_BUS_8_ON_64:
                SETWIDE(FL_RESET);
		movequad((void *)map->fl_map_base, &widedata);
		break;
	}
}

/*
 *  Find what flash map address nelongs to.
 */
struct fl_map *
fl_find_map(void *base)
{
	struct fl_map *map;
	for(map = tgt_flashmap(); map->fl_map_size != 0; map++) {
		if(map->fl_map_base > (u_int32_t)base ||
		   (map->fl_map_base + map->fl_map_size - 1) < (u_int32_t)base) {
			continue;	/* Not this one */
		}
		else {
			return(map);
		}
	}
	return((struct fl_map *)NULL);
}
		
/*
 *  Try to figure out what kind of flash there is on given address.
 */
struct fl_device *
  __attribute__((weak))fl_devident(void *base, struct fl_map **m)
{
	struct fl_device *dev;
	struct fl_map *map;
	char mfgid, chipid;

	/* If we can't write to flash, we can't identify */
	if(!tgt_flashwrite_enable()) {
		return((struct fl_device *)NULL);
	}
	map = fl_find_map(base);

	if(map != NULL) {
		fl_autoselect(map);

		switch(map->fl_map_bus) {
		case FL_BUS_8:
			mfgid = inb(map->fl_map_base);
			chipid = inb(map->fl_map_base+1);

			if(chipid == mfgid) { /* intel 16 bit flash mem */
				chipid = inb(map->fl_map_base+3);
			}
			break;

		case FL_BUS_8_ON_64:
			mfgid = inb(map->fl_map_base);
			chipid = inb(map->fl_map_base+8);
			break;

		case FL_BUS_16:
			mfgid = inw(map->fl_map_base);
			chipid = inw(map->fl_map_base+2);
			break;

		case FL_BUS_32:
			mfgid = inl(map->fl_map_base);
			chipid = inl(map->fl_map_base+4);
			break;

		case FL_BUS_64:
			mfgid = inw(map->fl_map_base);
			chipid = inw(map->fl_map_base+8);
			break;
		}
		fl_reset(map);

		/* Lookup device type using manufacturer and device id */
		for(dev = &fl_known_dev[0]; dev->fl_name != 0; dev++) {
			if(dev->fl_mfg == mfgid && dev->fl_id == chipid) {
				tgt_flashwrite_disable();
				if(m) {
					*m = map;
				}
				return(dev);	/* GOT IT! */
			}
		}
                printf("Mfg %2x, Id %2x\n", mfgid, chipid);
	}
	
	tgt_flashwrite_disable();
	outb((map->fl_map_base), 0xf0);
	outb((map->fl_map_base), 0x90);
	outb((map->fl_map_base), 0x00);
	return((struct fl_device *)NULL);
}



/*
 *  Erase the flash device(s) addressed.
 */
int
  __attribute__((weak))fl_erase_device(void *base, int size, int verbose)
{
	struct fl_map *map;
	struct fl_device *dev;
	int mask, ok, block;
	int timeout;

	if(tgt_flashwrite_enable() == 0) {
		printf("Flash can't be write enabled\n");
		return(-2);	/* Flash can't be write enabled */
	}

	dev = fl_devident(base, &map);
	if(dev == NULL) {
		printf("No flash found at %x\n",(u_int32_t)base);
		return(-3);	/* No flash device found at address */
	}

	/*
	 * Sanity checks!
	 */
	if(size == -1 && (int)base == map->fl_map_base) {
		size = map->fl_map_size;	/* Entire flash */
	}

        if(dev->fl_varsecsize != NULL) {
                int offset = (int)(base - map->fl_map_base) + map->fl_map_offset;
                int totalsize;
		printf("offset=%x,base=%x\n",offset,map->fl_map_base);

                for(block=0, totalsize=0; totalsize < offset; block++) {
                        totalsize += dev->fl_varsecsize[block] * map->fl_map_chips;
                }

                mask = ((dev->fl_varsecsize[block] * map->fl_map_width / map->fl_map_chips) - 1);
                if((int)base & mask) {
                        size += (int)base & mask;
                        base = (void *)((int)base & ~mask);
                } else if((size + ((int)base - map->fl_map_base)) > map->fl_map_size) {
                        return(-4);	/* End beyound end of flash */
                }
                base -= map->fl_map_base;

        } else {
                mask = ((dev->fl_secsize * map->fl_map_width / map->fl_map_chips) - 1);
                if((int)base & mask) {
                        size += (int)base & mask;
                        base = (void *)((int)base & ~mask);
                } else if((size + ((int)base - map->fl_map_base)) > map->fl_map_size) {
                        return(-4);	/* End beyound end of flash */
                }

                base -= map->fl_map_base;
                block = (int)base / map->fl_map_chips / dev->fl_secsize;
                size = (size + mask) & ~mask; /* Round up to catch entire flash */
        }
        
        tgt_flashwrite_enable();
        
	while(size > 0) {
		int boffs = (int)base;
		if(size == map->fl_map_size &&
			dev->fl_cap & (FL_CAP_DE|FL_CAP_A7)) {
			/*
			 * Erase entire devices using the BULK erase feature
			 */
			if(verbose) {
				printf("Erasing all FLASH blocks. ");
			}

			(*dev->functions->erase_chip)(map, dev);

			size = 0;
		}
		else {
			/*
			 * Not entire flash or no BULK erase feature. We
			 * use sector/block erase.
			 */
			if(verbose) {
				printf("\rErasing FLASH block %3d      \b\b\b\b\b", block);
			}

			if((*dev->functions->erase_sector)(map, dev, boffs) != 0) {
				printf("\nError: Failed to enter erase mode\n");
				(*dev->functions->erase_suspend)(map, dev);
				(*dev->functions->reset)(map, dev);
				return(-4);
			}

                        if(dev->fl_varsecsize != NULL) {
                                base += dev->fl_varsecsize[block] * map->fl_map_chips;
                                size -= dev->fl_varsecsize[block] * map->fl_map_chips;
                                block++;
                        } else {
                                base += dev->fl_secsize * map->fl_map_chips;
                                size -= dev->fl_secsize * map->fl_map_chips;
                                block++;
                        }
		}

		delay(1000);
		for(timeout = 0 ;
		    ((ok = (*dev->functions->isbusy)(map, dev, 0xffffffff, boffs, TRUE)) == 1)
				&& (timeout < PFLASH_MAX_TIMEOUT); timeout++) {
				delay(1000);
			if(verbose) {
				dotik(256, 0);
			}
		}
		delay(1000);

		if(!(timeout < PFLASH_MAX_TIMEOUT)) {
			(*dev->functions->erase_suspend)(map, dev);
		}
		(*dev->functions->reset)(map, dev);

		if(verbose) {
			if(!(timeout < PFLASH_MAX_TIMEOUT)) {
/* XXX if timed out what should really happen here? This doesn't look right. */
				printf("\b\b, command timed out!\n");
			} else {
				printf("\b Done.\n");
			}
		}
	}

	tgt_flashwrite_disable();
	return(ok);
}



int fl_program(void *fl_base, void *data_base, int data_size, int verbose)
{
        char *nvrambuf;
        char *nvramsecbuf;
	    char *nvram;
		int offs,count,left;
		struct fl_device *dev=fl_devident(fl_base,0);
		int nvram_size;

		if (!dev) return -1;
		nvram_size=dev->fl_secsize;

	nvramsecbuf = (char *)malloc(nvram_size);
	if(nvramsecbuf == 0) {
		printf("Warning! Unable to malloc nvrambuffer!\n");
		return(-1);
	}
      nvram = fl_base;
	  left = data_size;
	  while(left)
	  {

		offs = (int)nvram &(nvram_size - 1);
        nvram  = (int)nvram & ~(nvram_size - 1);
		count = min(nvram_size-offs,left);
		 

        memcpy(nvramsecbuf, nvram, nvram_size);
        if(fl_erase_device(nvram, nvram_size, verbose)) {
		printf("Error! Nvram erase failed!\n");
		free(nvramsecbuf);
                return(0);
        }
	    
		nvrambuf = nvramsecbuf + offs;

		memcpy(nvrambuf,data_base,count);
        
		if(fl_program_device(nvram, nvramsecbuf, nvram_size, verbose)) {
		printf("Error! Nvram program failed!\n");
		free(nvramsecbuf);
                return(0);
        }

		data_base += count;
		nvram += nvram_size;
		left -= count;
		}
	free(nvramsecbuf);
        return 0;
}





/*
 *  Program a flash device. Assumed that the area is erased already.
 */
int
  __attribute__((weak))fl_program_device(void *fl_base, void *data_base, int data_size, int verbose)
{
	struct fl_map *map;
	struct fl_device *dev;
	int ok;
	int i, off;
	
	if(tgt_flashwrite_enable() == 0) {
		return(-2);	/* Flash can't be write enabled */
	}
	dev = fl_devident(fl_base, &map);
	if(dev == NULL) {
		tgt_flashwrite_disable();
		return(-3);	/* No flash device found at address */
	}

	if(data_size == -1 || (int)data_base == -1) {
		return(-4);		/* Bad parameters */
	}
	if((data_size + ((int)fl_base - map->fl_map_base)) > map->fl_map_size) {
		return(-4);	/* Size larger than device array */
	}

	off = (int)(fl_base - map->fl_map_base) + map->fl_map_offset;
	if(verbose) {
		printf("Programming FLASH. ");
	}

	tgt_flashwrite_enable();

	for(i = 0; i < data_size; i += map->fl_map_width) {

		ok = (*dev->functions->program)(map, dev, (int)off, data_base);

		switch(map->fl_map_bus) {
		case FL_BUS_8:
		case FL_BUS_8_ON_64:
			off++;
			data_base++;
			break;

		case FL_BUS_16:
			data_base += 2;
			off += 2;
			break;

		case FL_BUS_32:
			data_base += 4;
			off += 4;
			break;

		case FL_BUS_64:
			data_base += 8;
			off += 8;
			break; 
		}

		if(verbose) {
			dotik(256, 0);
		}
	}

	(*dev->functions->reset)(map, dev);

	if(verbose) {
		printf("\b Done.\n");
	}
	
	tgt_flashwrite_disable();
	return(ok);
}

/*
 *  Verify flash contents to ram contents.
 */
int
fl_verify_device(void *fl_base, void *data_base, int data_size, int verbose)
{
	struct fl_map *map;
	struct fl_device *dev;
	void *fl_last;
	int ok;
	int i;


	dev = fl_devident(fl_base, &map);
	if(dev == NULL) {
		return(-3);	/* No flash device found at address */
	}

	if(data_size == -1 || (int)data_base == -1) {
		return(-4);		/* Bad parameters */
	}
	if((data_size + ((int)fl_base - map->fl_map_base)) > map->fl_map_size) {
		return(-4);	/* Size larger than device array */
	}

	if(verbose) {
		printf("Verifying FLASH. ");
	}

	for(i = 0; i < data_size; i += map->fl_map_width) {
		fl_last = fl_base;
		switch(map->fl_map_bus) {
		case FL_BUS_8:
			ok = (*((u_char *)fl_base) == *((u_char *)data_base)); fl_base += 1; data_base += 1;
			break;

		case FL_BUS_16:
			ok = (*(u_short *)fl_base == *((u_short *)data_base)); fl_base += 2; data_base += 2;
			break;

		case FL_BUS_32:
			ok = (*(u_int *)fl_base == *(u_int *)data_base); fl_base += 4; data_base += 4;
			break;

		case FL_BUS_64:
			movequad(&widedata, fl_base);
			ok = (bcmp(data_base, (void *)&widedata, 8) == 0);
			data_base += 8;
			fl_base += 8;
			break; 

		case FL_BUS_8_ON_64:
			ok = (*((u_char *)map->fl_map_base +
				   (((int)fl_base - map->fl_map_base) << 3)) ==
				*(u_char *)data_base++);
			fl_base++;
			break;
		}

		if(verbose & !ok) {
			printf(" error offset %p\n", fl_last);
			{
			char str[100];
			int timeout;
			printf("erase all chip(y/N)?");
			gets(str);
			if(str[0]=='y'||str[0]=='Y')
			{
				tgt_flashwrite_enable();
				printf("Erasing all FLASH blocks. ");
				fl_erase_device(map->fl_map_base,map->fl_map_size,FALSE);
				tgt_flashwrite_disable();
			}
			}
			break;
		}
		else if(verbose) {
			dotik(32, 0);
		}
	}

	if(verbose && ok) {
		printf("\b No Errors found.\n");
	}
	
	return(ok);
}
/*
 *  List the various FLASH devices found.
 */
void
fl_query_info()
{
	struct fl_map *map;
	struct fl_device *dev;

	printf("Available FLASH memory\n");
	printf("   Start      Size    Width  Sectorsize  Type\n");
	/*	0x0000ffff 0x0000ffff  4*8   0x00010000  Am29f040   */
	for(map = tgt_flashmap(); map->fl_map_size != 0; map++) {
		dev = fl_devident((void *)map->fl_map_base, NULL);
		if(dev == NULL) {
			continue;	/* Empty socket ? */
		}
		printf("0x%08x 0x%08x %2d*8   0x%08x   %-10s\n",
			map->fl_map_base, map->fl_map_size, map->fl_map_width,
			dev->fl_secsize * map->fl_map_chips, dev->fl_name);
	}
	printf("\n");
}


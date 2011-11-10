/*	$Id: cachecmds.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2001-2002 Opsycon AB  (www.opsycon.se)
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
 *	This product includes software developed by Opsycon AB, Sweden.
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
#include <termio.h>
#include <string.h>
#include <sys/endian.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#ifdef _KERNEL
#undef _KERNEL
#include <sys/ioctl.h>
#define _KERNEL
#else
#include <sys/ioctl.h>
#endif

#include <machine/cpu.h>
#include <machine/bus.h>

#include <pmon.h>

extern int md_l1_enable(int);
extern int md_l1_disable(int);
extern int md_l2_enable(int);
extern int md_l2_disable(int);
extern void md_cache_disable(void);
extern void md_set_l2cr(int);
extern int md_get_l2cr(void);
extern int md_size_cache(void);

static void md_config_hid0(void);
static void md_config_l2_cache(void);
static void l2_cache_speed(int);
static void l2_cache_size(int);

int cachecmds(int, char **);
int hid0_conf;
int l2cr_value;
int l2cachesize;

/*
 *  Configure setting for branch target cache and history.
 */
void
md_config_hid0()
{
	int cputype;

	cputype = md_cputype();

	switch((cputype >> 16) & 0xffff) {
	case CPU_TYPE_750:
		switch((cputype >> 8) & 0xff) {
		case 0x00:	/* 750 */
		case 0x01:	/* 750 */
		case 0x02:	/* 750 */
		case 0x03:	/* 750 */
		case 0x83:	/* 750L */
		case 0x31:	/* 755 */
		case 0x32:	/* 755 */
			hid0_conf = CPU_HID0_BTIC | CPU_HID0_BHT;
			break;
		case 0x22:
		case 0x33:
			hid0_conf = CPU_HID0_BTIC | CPU_HID0_BHT;
			break;
		default:
			break;
		}
		break;

	case CPU_TYPE_750FX:
		hid0_conf = CPU_HID0_BTIC | CPU_HID0_BHT;
		break;


	case CPU_TYPE_7400:
	case CPU_TYPE_7410:
		hid0_conf = CPU_HID0_BTIC | CPU_HID0_BHT;
		break;

	default:
		break;
	}
}

/*
 *  Configure L2 cache. We need to calculate parameters for the
 *  L2 CR register. Some of them depend on CPU type and frequency.
 *  Speed of L2 cache is assumed to be 150Mhz.
 */
void
md_config_l2_cache()
{
	int cputype;

	/* Common bits in all variants */
	l2cr_value = 0;
	l2cr_value |= PPC_CACHE_L2DO  << 22;
	l2cr_value |= PPC_CACHE_L2WT  << 19;

	cputype = md_cputype();

	switch((cputype >> 16) & 0xffff) {
	case CPU_TYPE_750:
		switch((cputype >> 8) & 0xff) {
		case 0x00:	/* 750 */
		case 0x01:	/* 750 */
		case 0x02:	/* 750 */
		case 0x03:	/* 750 */
		case 0x83:	/* 750L */
		case 0x31:	/* 755 */
		case 0x32:	/* 755 */
			l2_cache_speed(0);
			l2_cache_size(0x100000);	/* 1MB */
			break;
		case 0x22:	/* 750CX 256kb onchip */
		case 0x33:	/* 750CXe 256kb onchip */
			l2cr_value |= PPC_CACHE_L2CE << 30;
			l2cachesize = 256 * 1024;
			break;
		default:
			return;
		}
		break;

	case CPU_TYPE_750FX:
		l2cr_value |= PPC_CACHE_L2CE << 30;
		l2cachesize = 512 * 1024;
		break;


	case CPU_TYPE_7400:
	case CPU_TYPE_7410:
		l2_cache_speed(0);
		l2_cache_size(0);	/* Probe */
		break;

	default:
		break;
	}

	/* Finally invalidate and clean L2 cache */
	md_set_l2cr(l2cr_value);
	md_set_l2cr(l2cr_value | 0x00200000);	/* clear */
	while(md_get_l2cr() & 0x00000001);
	md_set_l2cr(l2cr_value);
	l2cr_value |= 0x80000000;
}

/*
 *  Decide L2 cache frequency setup
 */	
static void
l2_cache_speed(int speed)
{
static int cldiv[] = {
	0x02000000, 0x04000000, 0x08000000, 0x0a000000,
	0x0c000000, 0x06000000, 0x0e000000, 0x00000000
};
	
	int i;
	int cacheclock;

	if(speed != 0) {
		cacheclock = speed / 10;
	}
	else {
		cacheclock = PPC_CACHE_SPEED / 10;
	}

	for(i = 8; i > 0; i--) {
		if(tgt_pipefreq() > (cacheclock * 5 * i)) {
			l2cr_value |= cldiv[i-1];
			return;
		}
	}
}

/*
 *  Decide L2 cache size setup
 */	
static void
l2_cache_size(int size)
{
	int cachesize;

	/* Set various parameters */
	l2cr_value |= PPC_CACHE_L2PE  << 30;
	l2cr_value |= PPC_CACHE_L2RAM << 23;
	l2cr_value |= 0x00020000;	/* 'third' output hold */

	if(size == 0) {
		/* Now try to figure out the size of the L2 */
		md_set_l2cr(l2cr_value);
		md_set_l2cr(l2cr_value | 0x00200000);	/* clear */
		while(md_get_l2cr() & 0x00000001);
		md_set_l2cr(l2cr_value);
		md_set_l2cr(l2cr_value | 0x80440000);	/* test mode */
		l2cachesize = cachesize = md_size_cache();
		md_set_l2cr(l2cr_value);
	}
	else {
		l2cachesize = cachesize = size;
	}

	switch(cachesize) {
	case 0x200000:
		l2cr_value |= 0x00000000;
		break;
	case 0x100000:
		l2cr_value |= 0x30000000;
		break;
	case 0x080000:
		l2cr_value |= 0x20000000;
		break;
	case 0x040000:
		l2cr_value |= 0x10000000;
		break;
	default:
		break;
	}
}

/*
 *  Cache on/off command
 */

int
cachecmds(int ac, char *av[])
{
	int ccr;

	if(ac > 1) {
		if(strcmp(av[1], "on") == 0) {
			md_cacheon();
		}
		else if(strcmp(av[1], "l1on") == 0) {
			md_l1_enable(0x0000c000 | hid0_conf);
		}
        	else if(strcmp(av[1], "l1ion") == 0) {
                	md_l1_enable(0x00008000 | hid0_conf);
        	}
        	else if(strcmp(av[1], "l1don") == 0) {
                	flush_cache (DCACHE, NULL);
                	md_l1_enable(0x00004000);
        	}
		else if(strcmp(av[1], "off") == 0) {
			ccr = md_l1_enable(0x0);
			if(ccr & 0x0000c000) {
				flushdcache(0, memorysize);
			}
			ccr = md_l2_enable(0x0);
			if(ccr & 0x80000000) {
				md_l2_disable(0x80000000);
			}
			ccr = md_l1_enable(0x0);
			if(ccr & 0x0000c000) {
				md_l1_disable(0x0000c000);
			}
		}
		else {
			return(-1);
		}
	}
	else {
		ccr = md_l1_enable(0x0);
		printf("L1 cache: instruction %s, data %s.\n", 
			((ccr & 0x8000) ? "on" : "off"),
			((ccr & 0x4000) ? "on" : "off"));
		ccr = md_l2_enable(0x0);
		if(ccr & 0x80000000) {
			printf("L2 cache: on. Size: ");
			if (l2cachesize >= 1048576) {
				printf("%d Mbyte.\n", l2cachesize / 1048576);
			}
			else {
				printf("%d Kbyte.\n", l2cachesize / 1024);
			}
		}
		else {
			printf("L2 cache: off.\n");
		}
	}
	return(0);
}

/*
 *  Turn on full caches.
 */
void
md_cacheon()
{
	md_config_hid0();
	if(md_l1_enable(0) & 0x0000c000) {
		md_l1_enable(0x0000c000 | hid0_conf);
	}
	else {
		md_l1_enable(0x0000cc00);	/* invalidate */
		md_l1_disable(0x00000c00);	/* XXX PPC604 */
	}
	if(!(l2cr_value & 0x80000000)) {
		md_config_l2_cache();
		md_set_l2cr(l2cr_value);
	}
	else {
		if(!(md_l2_enable(0) & 0x80000000)) {
			md_set_l2cr(l2cr_value & 0x7fffffff);
			md_l2_enable(0x00200000);	/* Invalidate */
			while(md_get_l2cr() & 0x00000001);
			md_set_l2cr(l2cr_value);
		}
	}
}

/*
 *  Return status of cache on or off.
 */
int
md_cachestat()
{
	int	ccr;

	ccr = md_l1_enable(0x0);
	if (ccr & 0xc000) {
		return(1);
	}
	else {
		return(0);
	}
}


static const Cmd Cmds[] =
{
	{"Memory"},
	{"cache",	"[on | l1on | l2on | l1ion | l1don | off]",
			0,
			"cache control",
			cachecmds, 1, 2, 0},
	{0, 0}
};


static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}

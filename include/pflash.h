/*	$Id: pflash.h,v 1.1.1.1 2006/09/14 01:59:06 root Exp $ */

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
#ifndef __PFLASH_H
#define __PFLASH_H

struct fl_functions;

/*
 *  Structure describing each FLASH device the FLASH code can handle.
 */
struct fl_device {
	char	*fl_name;
	char	fl_mfg;		/* Flash manufacturer code */
	char	fl_id;		/* Device id code */
	char	fl_proto;	/* Command protocol */
	char	fl_cap;		/* Flash capabilites */
	int	fl_size;	/* Device size in k-bytes */
	int	fl_secsize;	/* Device sector-size in k-bytes */
        int	*fl_varsecsize;	/* Not NULL, if device has flexable sector size */
	struct fl_functions *functions;
};

#define	FL_PROTO_INT	1	/* Scalable Command Set */
#define	FL_PROTO_AMD	2	/* AMD in lack of better name... */
#define FL_PROTO_SST	2
#define FL_PROTO_ST		2
#define FL_PROTO_WINBOND	2

#define	FL_CAP_DE	0x01	/* Device have entire device Erase */
#define	FL_CAP_A7	0x02	/* Device uses a7 for Bulk Erase */

/*
 *  Structure describing targets FLASH memory map.
 */
struct fl_map {
	u_int32_t fl_map_base;	/* Start of flash area in physical map */
	u_int32_t fl_map_size;	/* Size of flash area in physical map */
	int	fl_map_width;	/* Number of bytes to program in one cycle */
	int	fl_map_chips;	/* Number of chips to operate in one cycle */
	int	fl_map_bus;	/* Bus width type, se below */
	int	fl_map_offset;	/* Flash Offset mapped in memory */
	int fl_type;
};

#define	FL_BUS_8	0x01	/* Byte wide bus */
#define	FL_BUS_16	0x02	/* Short wide bus */
#define	FL_BUS_32	0x03	/* Word wide bus */
#define	FL_BUS_64	0x04	/* Quad wide bus */
#define	FL_BUS_8_ON_64	0x05	/* Byte wide on quad wide bus */
#define	FL_BUS_16_ON_64	0x06	/* 16-bit wide flash on quad wide bus */


/*
 * Structure of functions for Flash devices
 */

struct fl_functions {
	int	(*erase_chip)		/* Erase Chip */
		__P((struct fl_map *, struct fl_device *));
	int	(*erase_sector)		/* Erase Sector */
		__P((struct fl_map *, struct fl_device *, int));
	int	(*isbusy)		/* Check if device is Busy */
		__P((struct fl_map *, struct fl_device *, int, int, int));
	int	(*reset)		/* Reset Chip */
		__P((struct fl_map *, struct fl_device *));
	int	(*erase_suspend)		/* Erase Suspend */
		__P((struct fl_map *, struct fl_device *));
	int	(*program)		/* Program Device */
		__P((struct fl_map *, struct fl_device *, int, unsigned char *));

};


/* 
 *  Flash Commands
 */
#define FL_AUTOSEL 	0x90	/* Device identification */
#define FL_RESET	0xf0	/* Return to DATA mode */
#define FL_ERASE	0x80
#define FL_ERASE_CHIP	0x10
#define FL_CHIP		0x10
#define FL_SECT		0x30
#define FL_PGM		0xa0
#define FL_SUSPEND	0xb0	/* Erase Suspend */
#define FL_RESUME	0x30	/* Erase Resume */
#define	AMD_CMDOFFS	0x555
#define	AMD_CMDOFFS1	0x555
#define	AMD_CMDOFFS2	0x2AA

#define	SST_CMDOFFS	0x5555
#define SST_CMDOFFS1 0x5555
#define SST_CMDOFFS2 0x2aaa

#define	WINBOND_CMDOFFS	0x5555
#define WINBOND_CMDOFFS1 0x5555
#define WINBOND_CMDOFFS2 0x2aaa

#define	FL_INT_FCE	0x30	/* Full Chip Erase */
#define	FL_INT_FCE_A7	0xa7	/* Full Chip Erase */
#define	FL_INT_BE	0x20	/* Block Erase */
#define	FL_INT_CNF	0xd0	/* Command confirm */
#define FL_INT_BUSY	0x80	/* Device busy status */ 
#define	FL_INT_CLR	0x50	/* Clear status register */
#define	FL_INT_READ	0xff	/* Set to read mode */
#define	FL_INT_PGM	0x40	/* FLASH program */

/* Prototypes */
struct fl_map *fl_find_map __P((void *));
struct fl_device *fl_devident __P((void *, struct fl_map **));
void fl_query_info __P((void));
int fl_erase_device __P((void *, int, int));
int fl_program_device __P((void *, void *, int, int));
int fl_verify_device __P((void *, void *, int, int));

extern struct fl_device fl_known_dev[];
extern struct fl_functions fl_func_amd;
extern struct fl_functions fl_func_int;
extern struct fl_functions fl_func_sst;
extern struct fl_functions fl_func_st;
extern struct fl_functions fl_func_winbond;
#endif

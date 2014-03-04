/*	$Id: flashdev.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2000 Opsycon AB  (www.opsycon.se)
 * Copyright (c) 2000 Rtmx, Inc   (www.rtmx.com)
 * Copyright (c) 2001 ipUnplugged AB  (www.ipunplugged.com)
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
#include <dev/pflash_tgt.h>

#include "flash.h"

#define __KB 1024

#if NMOD_FLASH_AMD > 0
int secsize_am29LV008bb[] = {16*__KB,  8*__KB,  8*__KB, 32*__KB, 64*__KB,
                             64*__KB, 64*__KB, 64*__KB, 64*__KB, 64*__KB,
                             64*__KB, 64*__KB, 64*__KB, 64*__KB, 64*__KB,
                             64*__KB, 64*__KB, 64*__KB, 64*__KB, 0};

int secsize_am29LV008bt[] = {64*__KB, 64*__KB, 64*__KB, 64*__KB, 64*__KB,
                             64*__KB, 64*__KB, 64*__KB, 64*__KB, 64*__KB,
                             64*__KB, 64*__KB, 64*__KB, 64*__KB, 64*__KB,
                             32*__KB,  8*__KB,  8*__KB, 16*__KB, 0};
#endif /* NMOD_FLASH_AMD */

#if NMOD_FLASH_ST > 0
   static const int secsize_M29W640F16bt[] = {
4*__KB,4*__KB,4*__KB,4*__KB,4*__KB,4*__KB,4*__KB,4*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB,32*__KB};
#endif

/*
 *  Flash devices known by this code.
 */
struct fl_device fl_known_dev[] = {
	{ "sst25vf080",	0xbf, 0x8E, FL_PROTO_AMD, FL_CAP_DE,		//lxy
	1024*__KB, 64*__KB,  NULL, NULL},	//&fl_func_sst8 },
	{ "winb25x128",	0xef, 0x18, FL_PROTO_AMD, FL_CAP_DE,
	16384*__KB, 64*__KB,  NULL, NULL},	//&fl_func_winb },
	{ "winb25x64",	0xef, 0x17, FL_PROTO_AMD, FL_CAP_DE,
	8192*__KB, 64*__KB,  NULL, NULL},	//&fl_func_winb },
	{ "winb25x80",	0xef, 0x14, FL_PROTO_AMD, FL_CAP_DE,
	1024*__KB, 64*__KB,  NULL, NULL},	//&fl_func_winb },
	{ "winb25x40",	0xef, 0x13, FL_PROTO_AMD, FL_CAP_DE,
	512*__KB, 64*__KB,  NULL, NULL},	//&fl_func_winb },
#if NMOD_FLASH_AMD > 0
	{ "Am29F040",	0x01, 0xa4, FL_PROTO_AMD, FL_CAP_DE,
	512*__KB, 64*__KB,  NULL, &fl_func_amd },
	{ "Am29F016",	0x01, 0xad, FL_PROTO_AMD, FL_CAP_DE,
	2048*__KB, 64*__KB,  NULL, &fl_func_amd },
	{ "Am29F032",	0x01, 0x41, FL_PROTO_AMD, FL_CAP_DE,
	4096*__KB, 64*__KB,  NULL, &fl_func_amd },
	{ "Am29LV040",	0x01, 0x4f, FL_PROTO_AMD, FL_CAP_DE,
	512*__KB, 64*__KB,  NULL, &fl_func_amd },
	{ "S29GL128",	0x01, 0x7e, FL_PROTO_AMD, FL_CAP_DE,
	16384*__KB, 128*__KB,  NULL, &fl_func_amd },
	{ "Am29LV160",	0x01, 0xc4, FL_PROTO_AMD, FL_CAP_DE,
	2048*__KB, 64*__KB,  NULL, &fl_func_amd },
	{ "Am29LV017",	0x01, 0xc8, FL_PROTO_AMD, FL_CAP_DE,
	2048*__KB, 64*__KB,  NULL, &fl_func_amd },
	{ "Am29LV065",	0x01, 0xc8, FL_PROTO_AMD, FL_CAP_DE,
	8192*__KB, 64*__KB,  NULL, &fl_func_amd },
	{ "Am29LV065D",	0x01, 0x93, FL_PROTO_AMD, FL_CAP_DE,
	8192*__KB, 64*__KB,  NULL, &fl_func_amd },
	{ "Am29LV008BB", 0x01, 0x37, FL_PROTO_AMD, FL_CAP_DE,
	1024*__KB, 64*__KB,  secsize_am29LV008bb, &fl_func_amd},
	{ "Am29LV008BT", 0x01, 0x3E, FL_PROTO_AMD, FL_CAP_DE,
	1024*__KB, 64*__KB,  secsize_am29LV008bt, &fl_func_amd},
	{ "Am29L040L", 0x37, 0x92, FL_PROTO_AMD, FL_CAP_DE,
	512*__KB, 64*__KB,  NULL, &fl_func_amd},
	{ "ST29F040",	0x20, 0xe2, FL_PROTO_AMD, FL_CAP_DE,
	512*__KB, 64*__KB,  NULL, &fl_func_amd },
	{ "EN29F040",	0x7f, 0x4f, FL_PROTO_AMD, FL_CAP_DE,
	512*__KB, 64*__KB,  NULL, &fl_func_amd },
	{ "EN29LV040",	0x1c, 0x4f, FL_PROTO_AMD, FL_CAP_DE,
	512*__KB, 64*__KB,  NULL, &fl_func_amd },
	/* zfx, should be compatible */
	{"MX29LV040", 0xC2, 0x4F, FL_PROTO_SST, FL_CAP_DE,
	 512 * __KB, 64 * __KB, NULL, &fl_func_sst},
    { "Am29LV160",  0x01, 0x49, FL_PROTO_AMD, FL_CAP_DE,  
      2048*__KB, 8*__KB,  NULL, &fl_func_amd },  //hwm     
#endif /* NMOD_FLASH_AMD */
#if NMOD_FLASH_INTEL > 0
	{ "i28F016SA",	0x89, 0xa0, FL_PROTO_INT, FL_CAP_A7,
	  2048*__KB, 64*__KB,  NULL, &fl_func_int },
	{ "i28F016",	0x89, 0xaa, FL_PROTO_INT,         0,
	  2048*__KB, 64*__KB,  NULL, &fl_func_int },
	{ "i28F320x",	0x89, 0x14, FL_PROTO_INT, FL_CAP_DE,
	  4096*__KB, 64*__KB,  NULL, &fl_func_int },
	{ "i28F640x",	0x89, 0x15, FL_PROTO_INT, FL_CAP_DE,
	  8192*__KB, 64*__KB,  NULL, &fl_func_int },
	{ "i28F320J3A",	0x89, 0x16, FL_PROTO_INT, FL_CAP_DE,
	  32*128*__KB, 128*__KB, NULL, &fl_func_int },
	{ "i28F640J3A",	0x89, 0x17, FL_PROTO_INT, FL_CAP_DE,
	  64*128*__KB, 128*__KB, NULL, &fl_func_int },
	{ "i28F128J3A",	0x89, 0x18, FL_PROTO_INT, FL_CAP_DE,
	  128*128*__KB, 128*__KB, NULL, &fl_func_int },
	{ "i28F160",	0xb0, 0xd0, FL_PROTO_INT, FL_CAP_DE,
	  2048*__KB, 64*__KB,  NULL, &fl_func_int },
	{ "i28F320",	0xb0, 0xd4, FL_PROTO_INT, FL_CAP_DE,
	  4096*__KB, 64*__KB,  NULL, &fl_func_int },
#endif /* NMOD_FLASH_INTEL */
#if NMOD_FLASH_SST > 0
	{ "Am29F040",	0x01, 0xa4, FL_PROTO_SST, FL_CAP_DE,
	512*__KB, 64*__KB,  NULL, &fl_func_sst },
	{ "Am29F016",	0x01, 0xad, FL_PROTO_SST, FL_CAP_DE,
	2048*__KB, 64*__KB,  NULL, &fl_func_sst },
	{ "Am29F032",	0x01, 0x41, FL_PROTO_SST, FL_CAP_DE,
	4096*__KB, 64*__KB,  NULL, &fl_func_sst },
	{ "SST39F040",	0x0bf, 0xd7, FL_PROTO_SST, FL_CAP_DE,
	512*__KB, 4*__KB,  NULL, &fl_func_sst }, /*sector size must be correct*/
    { "SST39VF6401B",	0xbf, 0x236d, FL_PROTO_SST, FL_CAP_DE,
      8192*__KB, 64*__KB,  NULL, &fl_func_sst },   //hwm
    { "SST39VF1601B",	0xbf, 0x234b, FL_PROTO_SST, FL_CAP_DE,
      2048*__KB, 4*__KB,  NULL, &fl_func_sst },   //hwm
	{ "SST39SF040",	0x0bf, 0xb7, FL_PROTO_SST, FL_CAP_DE,
	512*__KB, 4*__KB,  NULL, &fl_func_sst }, /*sector size must be correct*/
	{ "SST49LF040",	0xbf, 0x51, FL_PROTO_SST, FL_CAP_DE,
	512*__KB, 4*__KB,  NULL, &fl_func_sst },//whd
	{ "SST49LF040B",	0xbf, 0x50, FL_PROTO_SST, FL_CAP_DE,
	512*__KB, 4*__KB,  NULL, &fl_func_sst },//whd
	{ "SST49LF080A",	0xbf, 0x5b, FL_PROTO_SST, FL_CAP_DE,
	1024*__KB, 4*__KB,  NULL, &fl_func_sst },//whd
	{ "SST49LF008A",	0xbf, 0x5a, FL_PROTO_SST, FL_CAP_DE,
	1024*__KB, 4*__KB,  NULL, &fl_func_sst },
	{ "Am29LV017",	0x01, 0xc8, FL_PROTO_SST, FL_CAP_DE,
	2048*__KB, 64*__KB,  NULL, &fl_func_sst },
	{ "Am29LV065",	0x01, 0xc8, FL_PROTO_SST, FL_CAP_DE,
	8192*__KB, 64*__KB,  NULL, &fl_func_sst },
	{ "Am29LV065D",	0x01, 0x93, FL_PROTO_SST, FL_CAP_DE,
	8192*__KB, 64*__KB,  NULL, &fl_func_sst },
	{ "ST29F040",	0x20, 0xe2, FL_PROTO_SST, FL_CAP_DE,
	512*__KB, 64*__KB,  NULL, &fl_func_sst },
#endif
#if NMOD_FLASH_WINBOND > 0
	{ "W29C040",	0xda, 0x46, FL_PROTO_WINBOND, FL_CAP_DE,
	512*__KB, 256,  NULL, &fl_func_winbond }, /*sector size must be correct*/
	{ "WINBOND39LV040A",0xda, 0xd6, FL_PROTO_WINBOND, FL_CAP_DE,
	512*__KB, 64*__KB,  NULL, &fl_func_winbond },
	{ "WINBOND39LV040", 0xda, 0xb6, FL_PROTO_WINBOND, FL_CAP_DE,
	512*__KB, 64*__KB,  NULL, &fl_func_winbond },
#endif
#if NMOD_FLASH_ST > 0
	{ "M29W640F",	0x20, 0x22ED, FL_PROTO_ST, FL_CAP_DE,
	8192*__KB, 4*__KB,secsize_M29W640F16bt, &fl_func_st }, /*sector size must be correct*/
#endif
    { 0 },
};
#undef __KB

/*	$Id: pflash_tgt.h,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2000 Opsycon AB  (www.opsycon.se)
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

/*
 *  Define a 'struct fl_map' init set for each flash area on the target.
 */

/* Note! Not requiered but prefered, sort in ascending address order */

/*
 *  NOTE about 'width' codes. Usually the 'width' is the number of
 *  bytes that are accessed at the same time while the next value
 *  is the number of chips. Bytes per chips can be found by dividing
 *  'width' with 'chips'. The code 9 for width is special to handle
 *  how the cp7000 addresses the AMD single chip when not mapped as
 *  boot device.
 */
#if 0
#define	TARGET_FLASH_DEVICES_16 \
    { PHYS_TO_UNCACHED(BONITO_FLASH_BASE), 0x01000000, 1, 1, FL_BUS_8  },	\
    { 0x00000000, 0x00000000 }
#else
#define	TARGET_FLASH_DEVICES_16 \
    { PHYS_TO_UNCACHED(0x1fc00000), 0x00080000, 1, 1, FL_BUS_8  },	\
    { PHYS_TO_UNCACHED(0x1f800000), 0x00080000, 1, 1, FL_BUS_8  },	\
    { PHYS_TO_UNCACHED(0x1c000000), 0x01000000, 2, 1, FL_BUS_16  },	\
    { 0x00000000, 0x00000000 }
#endif

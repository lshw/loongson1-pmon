/*	$OpenBSD$ */

/*
 * Copyright (c) 2001 Opsycon AB  (www.opsycon.se)
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

#ifndef _EV64240_H_
#define _EV64240_H_

#include <machine/endian.h>

/*
 *  Mapping
 */
#define	IO_OFFSET		0x00000000
#define	PHYS_TO_IOSPACE(x)	(PHYS_TO_UNCACHED(x) + IO_OFFSET)

/*
 *  Define top of PMON-land. PMON will not use memory above this
 *  address but leave it alone for applications.
 */
//#define	PMON_TOP		0x00200000	/* 2MB */

/*
 *  CS space mapping.
 */

#define	SRAM_BASE		PHYS_TO_IOSPACE(0x1c000000)
#define	SRAM_SIZE		0x00800000
#define	RTC_BASE		PHYS_TO_IOSPACE(0x1c800000)
#define	RTC_SIZE		0x00800000
#define	UART_BASE		PHYS_TO_IOSPACE(0x1d000000)
#define	UART_SIZE		0x01000000
#define	FLASH_BASE		PHYS_TO_IOSPACE(0x1f000000)
#define	FLASH_SIZE		0x00c00000
#define	BOOT_BASE		PHYS_TO_IOSPACE(0x1fc00000)
#define	BOOT_SIZE		0x00400000
#define	BOOT_LOW_BASE	0x1fc00000

/*
 *  PCI Bus allocation seen from CPU side. PCI Physical starts at 0x0
 */
 /*
#define PCI0_MEM_SPACE_BASE	0xc0000000
#define PCI0P_MEM_SPACE_BASE	0xc0000000
#define PCI0_MEM_SPACE_SIZE	0x10000000
#define PCI0_IO_SPACE_BASE	0xb0200000
#define PCI0P_IO_SPACE_BASE	0x00100000
#define PCI0_IO_SPACE_SIZE	0x00100000

#define PCI1_MEM_SPACE_BASE	0xd0000000
#define PCI1P_MEM_SPACE_BASE	0xd0000000
#define PCI1_MEM_SPACE_SIZE	0x10000000
#define PCI1_IO_SPACE_BASE	0xb0100000
#define PCI1P_IO_SPACE_BASE	0x00000000
#define PCI1_IO_SPACE_SIZE	0x00100000

#define PCI1_MEM_SPACELOW_BASE	0xb0000000
#define PCI1P_MEM_SPACELOW_BASE	0x00000000
#define PCI1_MEM_SPACELOW_SIZE	0x00100000
*/

#ifndef CONFIG_PCI0_LARGE_MEM 
#define PCI0_MEM_SPACE_BASE	0xb2000000
#define PCI0P_MEM_SPACE_BASE	0x12000000
#define PCI0_MEM_SPACE_SIZE	0x01800000
#define PCI0_IO_SPACE_BASE	0xb0200000
#define PCI0P_IO_SPACE_BASE	0x00100000
#define PCI0_IO_SPACE_SIZE	0x00100000

#define PCI1_MEM_SPACE_BASE	0xb0800000
#define PCI1P_MEM_SPACE_BASE	0x10800000
#define PCI1_MEM_SPACE_SIZE	0x01800000
#define PCI1_IO_SPACE_BASE	0xb0100000
#define PCI1P_IO_SPACE_BASE	0x00000000
#define PCI1_IO_SPACE_SIZE	0x00100000

#define PCI1_MEM_SPACELOW_BASE	0xb0000000
#define PCI1P_MEM_SPACELOW_BASE	0x00000000
#define PCI1_MEM_SPACELOW_SIZE	0x00100000

#else

#define PCI0_MEM_SPACE_BASE	0xb0800000
#define PCI0P_MEM_SPACE_BASE	0x10800000
#define PCI0_MEM_SPACE_SIZE	0x03800000
#define PCI0_IO_SPACE_BASE	0xb0200000
#define PCI0P_IO_SPACE_BASE	0x00100000
#define PCI0_IO_SPACE_SIZE	0x00100000

#define PCI1_MEM_SPACE_BASE	0xb5800000
#define PCI1P_MEM_SPACE_BASE	0x15800000
#define PCI1_MEM_SPACE_SIZE	0x06800000
#define PCI1_IO_SPACE_BASE	0xb0100000
#define PCI1P_IO_SPACE_BASE	0x00000000
#define PCI1_IO_SPACE_SIZE	0x00100000

#define PCI1_MEM_SPACELOW_BASE	0xb5000000
#define PCI1P_MEM_SPACELOW_BASE	0x00000000
#define PCI1_MEM_SPACELOW_SIZE	0x00100000

#endif

#define	ISA_IO_BASE		PCI_IO_BASE

#define	PCI0_CPU_MEM_BASE	0x00000000	/* Where CPU mem is in PCI0 */
#define	PCI1_CPU_MEM_BASE	0x00000000	/* Where CPU mem is in PCI1 */

/*
 *  NVRAM mapping
 */
#ifdef NVRAM_IN_FLASH

#define	NVRAM_SIZE		494
#define	NVRAM_SECSIZE		500
#define	NVRAM_OFFS		0x00000000
#define ETHER_OFFS		494 	/* Ethernet address base */
#ifdef not_very_likely
#define NVRAM_VXWORKS		(NVRAM_OFFS + NVRAM_SIZE)
#define NVRAM_VXWORKS_DEFAULT \
"dc(0,0)host:/usr/vw/config/ev64240/vxWorks h=90.0.0.3 e=90.0.0.50 u=target"
#endif

#else	/* Use clock ram, 256 bytes only */
#define NVRAM_SIZE		108
#define NVRAM_SECSIZE		NVRAM_SIZE	/* Helper */
#define NVRAM_OFFS		0
#define ETHER_OFFS		108 	/* Ethernet address base */
#endif

/*
 *  Device module flash memory.
 */
#define	GT_DM_FLASH	FLASH_BASE

/*
 *  Device module duart I/O ports.
 */
#if !defined(USE_SUPERIO_UART)  
#define COM1_BASE_ADDR	PHYS_TO_IOSPACE(0x1d000000 + 0x20)	/* Com 1 */
#define COM2_BASE_ADDR	PHYS_TO_IOSPACE(0x1d000000 + 0x00)	/* Com 2 */
#define	NS16550HZ	3686400
#else

#define COM1_BASE_ADDR	0xb01003f8
#define COM2_BASE_ADDR	0xb01002f8
#define	NS16550HZ	1843200
#endif

#define GT_COM1   	1 
#define GT_COM2   	2 

#if !defined(USE_SUPERIO_UART)
#if BYTE_ORDER == LITTLE_ENDIAN
#define nsreg(x) unsigned char CAT(pad_,x)[3]; unsigned char x;
#define NSREG(x) ((x) * 4 + 3)
#endif
#if BYTE_ORDER == BIG_ENDIAN
#define nsreg(x) unsigned char x;unsigned char CAT(pad_,x)[3];
#define NSREG(x) ((x) * 4)
#endif
#else
#define nsreg(x)	unsigned char x;
#define NSREG(x)	(x)
#endif

#ifdef USE_PIIX_RTC
#define RTC_INDEX_REG 0x70
#define RTC_DATA_REG 0x71
#define RTC_NVRAM_BASE		0x0e
#endif

#define	DBGLED0_ON	(RTC_BASE + 0x08000)
#define	DBGLED1_ON	(RTC_BASE + 0x0c000)
#define	DBGLED2_ON	(RTC_BASE + 0x10000)
#define	DBGLED0_OFF	(RTC_BASE + 0x14000)
#define	DBGLED1_OFF	(RTC_BASE + 0x18000)
#define	DBGLED2_OFF	(RTC_BASE + 0x1c000)

/*
 * PLD Address
 */
#define PLD_BASE_ADDR	PHYS_TO_IOSPACE(0x1c000000)
#define PLD_BAR		(PLD_BASE_ADDR + 0x00)
#define PLD_ID1		(PLD_BASE_ADDR + 0x01)
#define PLD_ID2		(PLD_BASE_ADDR + 0x02)
#define PLD_RSTAT	(PLD_BASE_ADDR + 0x03)
#define PLD_BSTAT	(PLD_BASE_ADDR + 0x04)
#define PLD_CPCI	(PLD_BASE_ADDR + 0x05)

#endif /* _EV64240_H_ */

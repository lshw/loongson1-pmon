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

#ifndef _EV64420_H_
#define _EV64420_H_

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
#if  defined(CONFIG_PCI0_HUGE_MEM)||defined(CONFIG_PCI0_GAINT_MEM)

#define	SRAM_BASE		PHYS_TO_IOSPACE(0x1e800000)
#define	SRAM_SIZE		0x00080000
#define	RTC_BASE		PHYS_TO_IOSPACE(0x1e880000)
#define	RTC_SIZE		0x00080000
#define	UART_BASE		PHYS_TO_IOSPACE(0x1e900000)
#define	UART_SIZE		0x00080000
#define	FLASH_BASE		PHYS_TO_IOSPACE(0x1f000000)
#define	FLASH_SIZE		0x00c00000
#define	BOOT_BASE		PHYS_TO_IOSPACE(0x1fc00000)
#define	BOOT_SIZE		0x00400000
#define	BOOT_LOW_BASE	0x1fc00000

#else

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

#endif

/*
 *  PCI Bus allocation seen from CPU side. PCI Physical starts at 0x0
 */

#ifdef CONFIG_PCI0_GAINT_MEM

#define PCI_MEM_SPACE_BASE	0xb0000000
#define PCIP_MEM_SPACE_BASE	0x10000000
#define PCI_MEM_SPACE_SIZE	0x0e000000
#define PCI_IO_SPACE_BASE	0xbea00000
#define PCIP_IO_SPACE_BASE	0x00000000
#define PCI_IO_SPACE_SIZE	0x00100000

#define PCI_MEM_SPACELOW_BASE	0xbee00000
#define PCIP_MEM_SPACELOW_BASE	0x00000000
#define PCI_MEM_SPACELOW_SIZE	0x00100000

#endif

#ifdef CONFIG_PCI0_HUGE_MEM

#define PCI_MEM_SPACE_BASE	0xb5000000
#define PCIP_MEM_SPACE_BASE	0x15000000
#define PCI_MEM_SPACE_SIZE	0x09800000
#define PCI_IO_SPACE_BASE	0xb0100000
#define PCIP_IO_SPACE_BASE	0x00000000
#define PCI_IO_SPACE_SIZE	0x00100000

#define PCI_MEM_SPACELOW_BASE	0xb4800000
#define PCIP_MEM_SPACELOW_BASE	0x00000000
#define PCI_MEM_SPACELOW_SIZE	0x00100000

#endif

#ifdef CONFIG_PCI0_LARGE_MEM

#define PCI_MEM_SPACE_BASE	0xb5800000
#define PCIP_MEM_SPACE_BASE	0x15800000
#define PCI_MEM_SPACE_SIZE	0x06800000
#define PCI_IO_SPACE_BASE	0xb0100000
#define PCIP_IO_SPACE_BASE	0x00000000
#define PCI_IO_SPACE_SIZE	0x00100000

#define PCI_MEM_SPACELOW_BASE	0xb5000000
#define PCIP_MEM_SPACELOW_BASE	0x00000000
#define PCI_MEM_SPACELOW_SIZE	0x00100000

#endif

#ifndef PCI_MEM_SPACE_BASE

#define PCI_MEM_SPACE_BASE	0xb0800000
#define PCIP_MEM_SPACE_BASE	0x10800000
#define PCI_MEM_SPACE_SIZE	0x01800000
#define PCI_IO_SPACE_BASE	0xb0100000
#define PCIP_IO_SPACE_BASE	0x00000000
#define PCI_IO_SPACE_SIZE	0x00100000

#define PCI_MEM_SPACELOW_BASE	0xb0000000
#define PCIP_MEM_SPACELOW_BASE	0x00000000
#define PCI_MEM_SPACELOW_SIZE	0x00100000

#endif


#define	ISA_IO_BASE		PCI_IO_BASE

#define	PCI_CPU_MEM_BASE	0x00000000	/* Where CPU mem is in PCI */

/*
 *  NVRAM mapping
 */
#ifdef NVRAM_IN_FLASH

#define	NVRAM_SIZE		494

#define	NVRAM_OFFS		0x0007f000
#define ETHER_OFFS		494 	/* Ethernet address base */
#ifdef not_very_likely
#define NVRAM_VXWORKS		(NVRAM_OFFS + NVRAM_SIZE)
#define NVRAM_VXWORKS_DEFAULT \
"dc(0,0)host:/usr/vw/config/ev64420/vxWorks h=90.0.0.3 e=90.0.0.50 u=target"
#endif

#else	/* Use clock ram, 256 bytes only */
#define NVRAM_SIZE		108
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

#define COM1_BASE_ADDR	(PCI_IO_SPACE_BASE+0x3f8)
#define COM2_BASE_ADDR	(PCI_IO_SPACE_BASE+0x2f8)
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

#ifdef USE_LEGACY_RTC
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


#define VTSB_BUS 0
#define VTSB_DEV 10
#define VTSB_ISA_FUNC 0
#define VTSB_IDE_FUNC 1

#endif /* _EV64420_H_ */

/*	$Id: start.S,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

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

#ifndef _KERNEL
#define _KERNEL
#endif

#include <asm.h>
#include <regnum.h>
#include <cpu.h>
#include <pte.h>

#include "target/ev64240.h"

#include "pmon/dev/ns16550.h"
#include "pmon/dev/gt64240reg.h"

#ifdef GODSONEV2A
#	define SKIPDIMM
#	define SDRAM_MODE_FIX
#else
#	undef SKIPDIMM
#	undef SDRAM_MODE_FIX
#endif

#undef RUN_SDRAM_FAST

#define DEBUG_LOCORE 1
#ifdef DEBUG_LOCORE
#define	TTYDBG(x) \
	.rdata;98: .asciz x; .text; la a0, 98b; bal stringserial; nop
#else
#define TTYDBG(x)
#endif

#define	PRINTSTR(x) \
	.rdata;98: .asciz x; .text; la a0, 98b; bal stringserial; nop

#define GTINIT(offset, value) \
	.word	GT_BASE_ADDR+(offset), HTOLE32(value)

/*
 *   Register usage:
 *
 *	s0	link versus load offset, used to relocate absolute adresses.
 *	s1	free
 *	s2	base address to Galileo chip.
 *	s3	L1 I cache size.
 *	s4	L1 I cache line size.
 *	s5	L1 D cache size.
 *	s6	L1 D cache line size.
 *	s7	L2 Cache size.
 *	s8	L3 Cache size.
 */


	.set	noreorder

	.globl	_start
	.globl	start
	.globl	__main
_start:
start:
	.globl	stack
stack = start - 0x4000		/* Place PMON stack below PMON start in RAM */

/* NOTE!! Not more that 16 instructions here!!! Right now it's FULL! */
	mtc0	zero, COP_0_STATUS_REG

	li	t0, SR_BOOT_EXC_VEC	/* Exception to Boostrap Location */
	mtc0	t0, COP_0_STATUS_REG

	lui     a0,0x3000
	li      a1,SR_BOOT_EXC_VEC
	or      a0,a0,a1
	mtc0    a0, COP_0_STATUS_REG

	mtc0	zero, COP_0_CAUSE_REG
	la	sp, stack
	la	gp, _gp

	bal	uncached		/* Switch to uncached address space */
	nop

	bal	locate			/* Get current execute address */
	nop

uncached:
	or	ra, UNCACHED_MEMORY_ADDR
	j	ra
	nop

/*
 *  Reboot vector usable from outside pmon.
 */
	.align	8
ext_map_and_reboot:
	bal	CPU_TLBClear
	nop

	li	a0, 0xc0000000
	li	a1, 0x40000000
	bal	CPU_TLBInit
	nop
	la	v0, tgt_reboot
	la	v1, start
	subu	v0, v1
	lui	v1, 0xffc0
	addu	v0, v1
	jr	v0
	nop

/*
 *  Exception vectors here for rom, before we are up and running. Catch
 *  whatever comes up before we have a fully fledged exception handler.
 */
	.align	9			/* bfc00200 */
	la	a0, v200_msg
	bal	stringserial
	nop
	b	exc_common

	.align	7			/* bfc00280 */
	la	a0, v280_msg
	bal	stringserial
	nop
	b	exc_common

/* Cache error */
	.align	8			/* bfc00300 */
	PRINTSTR("\r\nPANIC! Unexpected Cache Error exception! ")
	mfc0	a0, COP_0_CACHE_ERR
	bal	hexserial
	nop
	b	exc_common

/* General exception */
	.align	7			/* bfc00380 */

/* make keg0 uncached to make sure prints works! */
#if 0
        mfc0    a0,COP_0_CONFIG
        and    a0,a0,0xfffffff8
        or     a0,a0,0x2
	mtc0   a0,COP_0_CONFIG
#endif
#if 1
	lui a1, 0xa040
        nop
	mfc0 a0, COP_0_EXC_PC
        nop
	sw a0, (a1)
        nop
	mfc0 a0, COP_0_CAUSE_REG
        nop
	sw a0, (a1)
        nop
	dmfc0 a0, $8
        nop
	dsrl a0,a0,32
        nop
	sw a0, (a1)
        nop
	mfc0 a0, $8
        nop
	sw a0, (a1)
        nop

	mfc0 a0, COP_0_EXC_PC
	nop
	lw a0, (a0)
	nop
	sw a0, (a1)
	nop

#endif

	la	a0, v380_msg
	bal	stringserial
	nop
	b	exc_common

	.align	8			/* bfc00400 */
	la	a0, v400_msg
	bal	stringserial
	nop

exc_common:
	PRINTSTR("\r\nERRORPC=")
	mfc0	a0, COP_0_ERROR_PC
	bal	hexserial
	nop
	PRINTSTR("\r\nEPC=")
	mfc0	a0, COP_0_EXC_PC
	bal	hexserial
	nop
	PRINTSTR("\r\n(EPC)=")
	mfc0	a0, COP_0_EXC_PC
	lw	a0, (a0)
	bal	hexserial
	nop

	PRINTSTR("\r\nSTATUS=")
	mfc0	a0, COP_0_STATUS_REG
	bal	hexserial
	nop
	PRINTSTR("\r\nCAUSE=")
	mfc0	a0, COP_0_CAUSE_REG
	bal	hexserial
	nop
	PRINTSTR("\r\nBADVADDR=")
	dmfc0	a0, $8
        dsrl  a0,a0,32
	bal	hexserial
        nop
	dmfc0	a0, $8
	bal	hexserial
	nop

	PRINTSTR("\r\nDERR0=")
	cfc0	a0, COP_0_DERR_0
	bal	hexserial
	nop
	PRINTSTR("\r\nDERR1=")
	cfc0	a0, COP_0_DERR_1
	bal	hexserial
	nop
	
	1:	 b  1b
        nop
	 
//	b	ext_map_and_reboot
	nop


/*
 *  We get here from executing a bal to get the PC value of the current execute
 *  location into ra. Check to see if we run from ROM or if this is ramloaded.
 */
	
locate:
	bal init_regs
	nop


	la	s0, start		/* RA set from BAL above! */
	subu	s0, ra, s0		/* s0 is now load vs. link offset */
	and	s0, 0xffff0000		/* Mask off lower bits */

/*
 *  Clean out and initialize the TLB
 */
	bal	CPU_TLBClear
	nop

	li	a0, 0xc0000000
	li	a1, 0x40000000
	bal	CPU_TLBInit
	nop

/*
 *  Turn off all high decoders to avoid address conflicts.
 */
	la	s2, GT_BASE_ADDR_DEFAULT
	li	t0, HTOLE32(0x00000fff)
	sw	t0, PCI_0I_O_LOW_DECODE_ADDRESS(s2)
	sw	t0, PCI_0MEMORY0_LOW_DECODE_ADDRESS(s2)
	sw	t0, PCI_0MEMORY1_LOW_DECODE_ADDRESS(s2)
	sw	t0, PCI_0MEMORY2_LOW_DECODE_ADDRESS(s2)
	sw	t0, PCI_0MEMORY3_LOW_DECODE_ADDRESS(s2)
	sw	zero, PCI_0I_O_HIGH_DECODE_ADDRESS(s2)
	sw	zero, PCI_0MEMORY0_HIGH_DECODE_ADDRESS(s2)
	sw	zero, PCI_0MEMORY1_HIGH_DECODE_ADDRESS(s2)
	sw	zero, PCI_0MEMORY2_HIGH_DECODE_ADDRESS(s2)
	sw	zero, PCI_0MEMORY3_HIGH_DECODE_ADDRESS(s2)
	sw	t0, PCI_1I_O_LOW_DECODE_ADDRESS(s2)
	sw	t0, PCI_1MEMORY0_LOW_DECODE_ADDRESS(s2)
	sw	t0, PCI_1MEMORY1_LOW_DECODE_ADDRESS(s2)
	sw	t0, PCI_1MEMORY2_LOW_DECODE_ADDRESS(s2)
	sw	t0, PCI_1MEMORY3_LOW_DECODE_ADDRESS(s2)
	sw	zero, PCI_1I_O_HIGH_DECODE_ADDRESS(s2)
	sw	zero, PCI_1MEMORY0_HIGH_DECODE_ADDRESS(s2)
	sw	zero, PCI_1MEMORY1_HIGH_DECODE_ADDRESS(s2)
	sw	zero, PCI_1MEMORY2_HIGH_DECODE_ADDRESS(s2)
	sw	zero, PCI_1MEMORY3_HIGH_DECODE_ADDRESS(s2)
#if 1
	li	t0, HTOLE32(0x00000101)
	sw	t0, PCI_1I_O_LOW_DECODE_ADDRESS(s2)
	li	t0, HTOLE32(0x00000101)
	sw	t0, PCI_1I_O_HIGH_DECODE_ADDRESS(s2)
	sw	zero, PCI_1I_O_ADDRESS_REMAP(s2)

	li	t0, HTOLE32(0x80000004)
	sw	t0, PCI_1CONFIGURATION_ADDRESS(s2)
	lw	t1, PCI_1CONFIGURATION_DATA_VIRTUAL_REGISTER(s2)
	ori	t1, 0x02|0x04
	sw	t0, PCI_1CONFIGURATION_ADDRESS(s2)
	sw	t1, PCI_1CONFIGURATION_DATA_VIRTUAL_REGISTER(s2)

	lw	t0, PCI_0ARBITER_CONTROL(s2)
	or	t0, t0, HTOLE32(0x80000000)
	sw	t0, PCI_0ARBITER_CONTROL(s2)
	lw	t0, PCI_1ARBITER_CONTROL(s2)
	or	t0, t0, HTOLE32(0x80000000)
	sw	t0, PCI_1ARBITER_CONTROL(s2)
#endif
/*
 *  Relocate the Galileo to HIGH memory.
 */
	li	t0, HTOLE32((UNCACHED_TO_PHYS(GT_BASE_ADDR) >> 20) | 0x01000000)
	sw	t0, INTERNAL_SPACE_DECODE(s2)

	li	t0, 0x100		/* Need a small delay here */
1:
	bnez	t0, 1b
	addiu	t0, -1

	la	s2, GT_BASE_ADDR	/* From now this is the GT base */
//	bal	tgt_setpar125mhz	/* Set bus timing for 125MHz */
	bal	tgt_setpar100mhz	/* Set bus timing for 125MHz */
	nop				/* Changed later if 100Mhz bus */

	lw	t0, CPU_CONFIG(s2)	/* Turn off  automatic retries */
	li	t1, HTOLE32(0x00020000)	
	or	t0, t0, t1
	sw	t0, CPU_CONFIG(s2)

/*
 *  Set up I/O decoders to point correctly.
 */
	bal	2f	/* Load address to init table */
	nop

	GTINIT(CS_0_LOW_DECODE_ADDRESS, UNCACHED_TO_PHYS(SRAM_BASE) >> 20)
	GTINIT(CS_0_HIGH_DECODE_ADDRESS, (UNCACHED_TO_PHYS(SRAM_BASE) + SRAM_SIZE - 1) >> 20)
	GTINIT(CS_1_LOW_DECODE_ADDRESS, UNCACHED_TO_PHYS(RTC_BASE) >> 20)
	GTINIT(CS_1_HIGH_DECODE_ADDRESS, (UNCACHED_TO_PHYS(RTC_BASE) + RTC_SIZE - 1) >> 20)
	GTINIT(CS_2_LOW_DECODE_ADDRESS, UNCACHED_TO_PHYS(UART_BASE) >> 20)
	GTINIT(CS_2_HIGH_DECODE_ADDRESS, (UNCACHED_TO_PHYS(UART_BASE) + UART_SIZE - 1) >> 20)

	GTINIT(CS_3_LOW_DECODE_ADDRESS, UNCACHED_TO_PHYS(FLASH_BASE) >> 20)
	GTINIT(CS_3_HIGH_DECODE_ADDRESS, (UNCACHED_TO_PHYS(FLASH_BASE) + FLASH_SIZE - 1) >> 20)

	/* end mark */
	.word	0, 0

1:
	sw	v1, 0(v0)
2:
	lw	v0, 0(ra)		/* Address */
	lw	v1, 4(ra)		/* Data */
	bnez	v0, 1b
	addiu	ra, 8


/*
 * BOOT rom. Read width bits to check boot width size to set up flash CS
 * "correctly". Bootwidth sticks with straps but for clarity we retain it
 * Also it should be correct to use the same timing for both flashes since
 * they can be swapped, although they may have different characteristics.
 */
	lw	t0, DEVICE_BOOT_BANK_PARAMETERS(s2)
	li	t1, HTOLE32(GT_DEVPAR_TurnOff(2) | GT_DEVPAR_AccToFirst(15) | \
			    GT_DEVPAR_AccToNext(15) | GT_DEVPAR_ALEtoWr(5) |  \
			    GT_DEVPAR_WrActive(7)   | GT_DEVPAR_WrHigh(5) |   \
			    GT_DEVPAR_Reserved)
	and	t0, HTOLE32(GT_DEVPAR_DevWidthMASK)
	or	t1, t0				/* DevWidth value */
	li	t2, HTOLE32(GT_DEVPAR_DevWidth32)
	beqz	t0, bootis32
	nop
//	sw	t1, DEVICE_BOOT_BANK_PARAMETERS(s2)

	li	t2, HTOLE32(GT_DEVPAR_DevWidth8)
bootis32:
	lw	t0, DEVICE_BANK3PARAMETERS(s2)		/* Flash Disk */
	and	t0, HTOLE32(GT_DEVPAR_ReservedMASK)
	or	t0, t2
	li	t1, HTOLE32(GT_DEVPAR_TurnOff(2) | GT_DEVPAR_AccToFirst(17) | \
			    GT_DEVPAR_AccToNext(17) | GT_DEVPAR_ALEtoWr(5) |  \
			    GT_DEVPAR_WrActive(7)   | GT_DEVPAR_WrHigh(7))
	or	t0, t1
	sw	t0, DEVICE_BANK3PARAMETERS(s2)

/*
 *  Init serial I/O for diagnostic output.
 */
	bal	initserial
	nop

	PRINTSTR("\r\nPMON2000 MIPS Initializing. Standby...\r\n")

	PRINTSTR("ERRORPC=")
	mfc0	a0, COP_0_ERROR_PC
	bal	hexserial
	nop

	PRINTSTR(" CONFIG=")
	mfc0	a0, COP_0_CONFIG
	bal	hexserial
	nop
	PRINTSTR("\r\n")

	la	s0, start		/* RA set from BAL above! */
	subu	s0, ra, s0		/* s0 is now load vs. link offset */
	and	s0, 0xffff0000		/* Mask off lower bits */


	TTYDBG("Setting up SDRAM controller\r\n");

 	/* Read the SPD info and get DRAM Configuration */	

	bal	doinit
	nop

	GTINIT(SCS_0_LOW_DECODE_ADDRESS,  0xfff)
	GTINIT(SCS_0_HIGH_DECODE_ADDRESS, 0x000)
	GTINIT(SCS_1_LOW_DECODE_ADDRESS,  0xfff)
	GTINIT(SCS_1_HIGH_DECODE_ADDRESS, 0x000)
	GTINIT(SCS_2_LOW_DECODE_ADDRESS,  0xfff)
	GTINIT(SCS_2_HIGH_DECODE_ADDRESS, 0x000)
	GTINIT(SCS_3_LOW_DECODE_ADDRESS,  0xfff)
	GTINIT(SCS_3_HIGH_DECODE_ADDRESS, 0x000)
#ifdef RUN_SDRAM_FAST
	GTINIT(SDRAM_TIMING_PARAMETERS, 0x0000072a)
#else
	GTINIT(SDRAM_TIMING_PARAMETERS, 0x0000072a)
#endif
	.word 0, 0

doinit:
	li	v0, 0xb0000000		/* ra is set from previous bal */
	bgtu	v0, ra, in_ram		/* if pc is lower than rom space.. */
	nop
	b	2f
	nop

1:
	sw	v1, 0(v0)
2:
	lw	v0, 0(ra)		/* Address */
	lw	v1, 4(ra)		/* Data */
	bnez	v0, 1b
	addiu	ra, 8

/*
 *  We are executing in ROM space so start do the setup work.
 *
 *  Initialize SDRAM. 
 *
 *  NOTE!!! We can't complete address mapping at this point
 *  because we cant't move the bootrom mapping until we are
 *  executing out of SDRAM. We start by setting both banks
 *  to 128MB and then once running from SDRAM we change the
 *  decoding to map the actual memory.
 */
#ifdef SKIPDIMM
	b	skipdimm
	nop
#endif
	bal	boot_i2c_init
	nop
	TTYDBG("Probing DIMM...\r\nDIMM0=");
	bal	probe_sdram_size
	li	a0, 0x600

	move	t2, v0
	bal	hexserial
	move	a0,v0

	TTYDBG("\r\nDIMM1=");
	
	bal	probe_sdram_size
	li	a0, 0x400
	
	move	t3, v0
	bal	hexserial
	move	a0, v0

	TTYDBG("\r\n");

	srl	s1, t2, 20

	add	t1, t2, t3
	beqz	t1, panic_no_memory
	nop

	bgt	t2, 256*1024*1024, panic_dimm0_too_big
	nop
	
	bnez		t2,	1f
	nop
	bgt	t3, 256*1024*1024, panic_dimm1_too_big
	nop
	b	sdram_slot1_config
	nop
1:
	bal	boot_i2c_read
	li	a0, 0x61f
	move t4, v0

	bal	boot_i2c_read
	li	a0, 0x604
	move	t5, v0

	bal	boot_i2c_read
	li	a0, 0x605
	beq	v0, 2, sdram_slot0_2bank
	nop
	
	TTYDBG("DIMM0: 1 side\r\n");
	
	li	t0, 0
	sw	t0, SCS_0_LOW_DECODE_ADDRESS(s2)
	srl	t0, t2, 20
	sub	t0, 1
	b sdram_slot1_config
	sw	t0, SCS_0_HIGH_DECODE_ADDRESS(s2)
	
sdram_slot0_2bank:
	TTYDBG("DIMM0: 2 sides\r\n");
	li	t0, 0
	sw	t0, SCS_0_LOW_DECODE_ADDRESS(s2)
	srl	t0, t2, 21
	sw	t0, SCS_1_LOW_DECODE_ADDRESS(s2)
	sub	t0, 1
	sw	t0, SCS_0_HIGH_DECODE_ADDRESS(s2)
	srl	t0, t2, 20
	sub	t0, 1
	sw	t0, SCS_1_HIGH_DECODE_ADDRESS(s2)
	
sdram_slot1_config:
	beqz	t3, sdram_set_param
	nop
	
	bal	boot_i2c_read
	li	a0, 0x41f
	move t6, v0

	bal	boot_i2c_read
	li	a0, 0x404
	move	t7, v0

	bal	boot_i2c_read
	li	a0, 0x405
	beq	v0, 2, sdram_slot1_2bank
	nop
	TTYDBG("DIMM1: 1 side\r\n");
	add	t1, t2, t3
	bgt	t1, 256*1024*1024, dimm1_set_high_1bank
	nop
	srl	t0, t2, 20
	sw	t0, SCS_2_LOW_DECODE_ADDRESS(s2)
	srl	t0, t1, 20
	sub	t0, 1
	sw	t0, SCS_2_HIGH_DECODE_ADDRESS(s2)
	srl	t0, t3, 20
	b sdram_set_param
	add	s1, s1, t0
dimm1_set_high_1bank:
	or	s1, s1, 0x80000000
	li	t0, 0x0200
	sw	t0, SCS_2_LOW_DECODE_ADDRESS(s2)
	addu	t0, t3, 0x20000000
	srl	t0, t0, 20
	sub	t0, 1
	sw	t0, SCS_2_HIGH_DECODE_ADDRESS(s2)
	srl	t0, t3, 4
	b	sdram_set_param
	or	s1, s1, t0
sdram_slot1_2bank:
	TTYDBG("DIMM1: 2 sides\r\n");

	bgt	t1, 256*1024*1024, dimm1_set_high_2bank
	nop

	srl	t0, t2, 20
	sw	t0, SCS_2_LOW_DECODE_ADDRESS(s2)
	srl	t0, t3, 1
	addu	t0, t0, t2
	srl	t0, t0, 20
	sw	t0, SCS_3_LOW_DECODE_ADDRESS(s2)
	sub	t0, 1
	sw	t0, SCS_2_HIGH_DECODE_ADDRESS(s2)
	addu	t0, t2, t3
	srl	t0, t0, 20
	sub	t0, 1
	sw	t0, SCS_3_HIGH_DECODE_ADDRESS(s2)
	srl	t0, t3, 20
	b	sdram_set_param
	add	s1, s1, t0
dimm1_set_high_2bank:
	or	s1, s1, 0x80000000
	li	t0, 0x0200
	sw	t0, SCS_2_LOW_DECODE_ADDRESS(s2)
	srl	t0, t3, 1
	addu	t0, t0, 0x20000000
	srl	t0, t0, 20
	sw	t0, SCS_3_LOW_DECODE_ADDRESS(s2)
	sub	t0, 1
	sw	t0, SCS_2_HIGH_DECODE_ADDRESS(s2)
	addu	t0, t3, 0x20000000
	srl	t0, t0, 20
	sub	t0, 1
	sw	t0, SCS_3_HIGH_DECODE_ADDRESS(s2)
	srl	t0, t3, 4
	or	s1, s1, t0
	
sdram_set_param:
	beqz	t2, skip_dimm0
	nop
	li	t0, HTOLE32(0x8000)
	li	v1, 0x10
	beq	t4, v1, set_dens0		/* 64MB */
	li	v1, 0x20
	beq	t4, v1, set_dens0		/* 128MB */
	li	v1, 0x40
	li	t0, HTOLE32(0xc000)
	beq	t4, v1, set_dens0		/* 256MB */
	li	v1, 0x80
	beq	t4, v1, set_dens0		/* 512MB */
	nop
	li	t0, HTOLE32(0x4000)
	li	v1, 10
	beq	t5, v1, set_dens0
	nop
	li	t0, HTOLE32(0x8000)
	b	set_dens0
	nop
skip_dimm0:
	beqz	t3, done_dens
	nop
	li	t0, HTOLE32(0x8000)
	li	v1, 0x10
	beq	t6, v1, set_dens1		/* 64MB */
	li	v1, 0x20
	beq	t6, v1, set_dens1		/* 128MB */
	li	v1, 0x40
	li	t0, HTOLE32(0xc000)
	beq	t6, v1, set_dens1		/* 256MB */
	li	v1, 0x80
	beq	t6, v1, set_dens1		/* 512MB */
	nop
	li	t0, HTOLE32(0x4000)
	li	v1, 10
	beq	t7, v1, set_dens1
	nop
	li	t0, HTOLE32(0x8000)
	b	set_dens1
	nop
set_dens0:
#ifdef RUN_SDRAM_FAST
	or	t0, 0x00f0000
#endif
	sw	t0, SDRAM_BANK0PARAMETERS(s2)
	sw	t0, SDRAM_BANK1PARAMETERS(s2)
	b	skip_dimm0
	nop
set_dens1:
#ifdef RUN_SDRAM_FAST
	or	t0, 0x00f0000
#endif
	sw	t0, SDRAM_BANK2PARAMETERS(s2)
	sw	t0, SDRAM_BANK3PARAMETERS(s2)
done_dens:
#ifdef RUN_SDRAM_FAST
	li	t0, HTOLE32(0x00e0c080)		/* non-registred */
#else
	li	t0, HTOLE32(0x04e0c080)		/* non-registred */
#endif
set_conf:
	sw	t0, SDRAM_CONFIGURATION(s2)

	li	t0, HTOLE32(2)
	sw	t0, SDRAM_ADDRESS_DECODE(s2)
	sw	zero, SDRAM_OPERATION_MODE(s2)
	b	2f
	nop
#ifdef SKIPDIMM
skipdimm:
	li	t0, 0x0000007f
	sw	t0, SCS_0_HIGH_DECODE_ADDRESS(s2)
	sw	zero, SCS_0_LOW_DECODE_ADDRESS(s2)
	li	t0, 0x000000ff
	sw	t0, SCS_1_HIGH_DECODE_ADDRESS(s2)
	li	t0, 0x00000080
	sw	t0, SCS_1_LOW_DECODE_ADDRESS(s2)

	li	t0, 0x00008000
	sw	t0, SDRAM_BANK0PARAMETERS(s2)
	sw	t0, SDRAM_BANK1PARAMETERS(s2)
	li	t0, HTOLE32(0x04e0c080)		/* non-registred */
	sw	t0, SDRAM_CONFIGURATION(s2)

#ifdef	SDRAM_MODE_FIX
	li	t0, HTOLE32(2)
	sw	t0, SDRAM_ADDRESS_DECODE(s2)
	/* MRS command issue */
	li	t0, HTOLE32(3)
	sw	t0, SDRAM_OPERATION_MODE(s2)
	lw	t0, SDRAM_OPERATION_MODE(s2)
	/* dummy write */
	li	t0, 0xa0000000
	sw	zero, 0(t0)
1:
	lw	t0, SDRAM_OPERATION_MODE(s2)
	and	t0, 0x80000000
	beqz	t0, 1b
	nop

	/* MRS command issue */
	li	t0, HTOLE32(3)
	sw	t0, SDRAM_OPERATION_MODE(s2)
	lw	t0, SDRAM_OPERATION_MODE(s2)
	/* dummy write */
	li	t0, 0xa0000000 + 1024*1024*128
	sw	zero, 0(t0)
1:
	lw	t0, SDRAM_OPERATION_MODE(s2)
	and	t0, 0x80000000
	beqz	t0, 1b
	nop
#endif
	sw	zero, SDRAM_OPERATION_MODE(s2)
	lw	t0, SDRAM_OPERATION_MODE(s2)
	
	b	2f
	nop
#endif	
panic_dimm0_too_big:
	PRINTSTR("\r\nPANIC! DIMM0 cannot exceed 256MB\r\n")
	b	1f
panic_dimm1_too_big:
	PRINTSTR("\r\nPANIC! DIMM1 cannot exceed 256MB if DIMM0 is absent\r\n")
	b	1f
panic_no_memory:
	PRINTSTR("\r\nPANIC! No SDRAM installed\r\n")
1:
	b	1b
	nop

2:

	TTYDBG("Dumping SDRAM registers...\r\n");
	
	
	TTYDBG("SDRAM_CONFIGURATION=");
	bal	hexserial
	lw	a0, SDRAM_CONFIGURATION(s2)

	TTYDBG("\r\nSDRAM_TIMING_PARAMETERS=");
	bal	hexserial
	lw	a0, SDRAM_TIMING_PARAMETERS(s2)

	TTYDBG("\r\nSDRAM_BANK0PARAMETERS=");
	bal	hexserial
	lw	a0, SDRAM_BANK0PARAMETERS(s2)

	TTYDBG("\r\nSDRAM_BANK1PARAMETERS=");
	bal	hexserial
	lw	a0, SDRAM_BANK1PARAMETERS(s2)

	TTYDBG("\r\nSDRAM_BANK2PARAMETERS=");
	bal	hexserial
	lw	a0, SDRAM_BANK2PARAMETERS(s2)
	
	TTYDBG("\r\nSDRAM_BANK3PARAMETERS=");
	bal	hexserial
	lw	a0, SDRAM_BANK3PARAMETERS(s2)
	
	TTYDBG("\r\nSCS_0=");
	bal	hexserial
	lw	a0, SCS_0_LOW_DECODE_ADDRESS(s2)
	TTYDBG("~");
	bal	hexserial
	lw	a0, SCS_0_HIGH_DECODE_ADDRESS(s2)

	TTYDBG("\r\nSCS_1=");
	bal	hexserial
	lw	a0, SCS_1_LOW_DECODE_ADDRESS(s2)
	TTYDBG("~");
	bal	hexserial
	lw	a0, SCS_1_HIGH_DECODE_ADDRESS(s2)

	TTYDBG("\r\nSCS_2=");
	bal	hexserial
	lw	a0, SCS_2_LOW_DECODE_ADDRESS(s2)
	TTYDBG("~");
	bal	hexserial
	lw	a0, SCS_2_HIGH_DECODE_ADDRESS(s2)

	TTYDBG("\r\nSCS_3=");
	bal	hexserial
	lw	a0, SCS_3_LOW_DECODE_ADDRESS(s2)
	TTYDBG("~");
	bal	hexserial
	lw	a0, SCS_3_HIGH_DECODE_ADDRESS(s2)
	
	TTYDBG("\r\n");

/*
 *  Clear out 2Mb of memory (maximum cache size)
 */

	TTYDBG("Clearing cache size memory...\r\n");

	la	t0, UNCACHED_MEMORY_ADDR
	addu	t1, t0, 2*1024*1024
1:
	addu	t0, 8
	bne	t1, t0, 1b
	sd	zero, -8(t0)

	TTYDBG("Init SDRAM Done!\r\n");
	b	do_caches
	nop

in_ram:
	PRINTSTR("RAM loaded\r\n");
/*
 *  Reset and initialize caches to a known state.
 */
#define IndexStoreTagI	0x08
#define IndexStoreTagD	0x09
#define IndexStoreTagS	0x0b
#define IndexStoreTagT	0x0a
#define FillI		0x14

/*
 *  RM7000 config register bits.
 */
#define CF_7_SE         (1 << 3)        /* Secondary cache enable */
#define CF_7_SC         (1 << 31)       /* Secondary cache not present */
#define CF_7_TE         (1 << 12)       /* Tertiary cache enable */
#define CF_7_TC         (1 << 17)       /* Tertiary cache not present */
#define CF_7_TS         (3 << 20)       /* Tertiary cache size */
#define CF_7_TS_AL      20              /* Shift to align */
#define NOP8 nop;nop;nop;nop;nop;nop;nop;nop
do_caches:
	TTYDBG("Sizing caches...\r\n");

	mfc0	t3, COP_0_CONFIG	/* t3 = original config */
	and	t3, 0xffffeff0		/* Make sure coherency is OK */

	and	t3, ~(CF_7_TE|CF_7_SE|CF_7_TC|CF_7_SC)  /* disable L2/L3 cache */
	mtc0    t3, COP_0_CONFIG

	li	t2, 4096

	srl	t1, t3, 9
	and	t1, 3
	sllv	s3, t2, t1		/* s3 = I cache size */

	and	t1, t3, 0x20
	srl	t1, t1, 1
	addu	s4, t1, 16		/* s4 = I cache line size */

	srl	t1, t3, 6
	and	t1, 3
	sllv	s5, t2, t1		/* s5 = D cache size */

	and	t1, t3, 0x10
	addu	s6, t1, 16		/* s6 = D cache line size */
	TTYDBG("Init caches...\r\n")

	li	s7, 0                   /* no L2 cache */
	li	s8, 0                   /* no L3 cache */

	mfc0    a0, COP_0_PRID
	li      a1, 0x6300
	bne     a0,a1,1f
	nop
	TTYDBG("godson2 caches found\r\n")
        bal     godson2_cache_init
        nop
	b	cache_done
	nop
1:

#if 0

	and	t1, t3, CF_7_TC
	bnez	t1, Conf7KL2		/* Any L3 disabled if set */
	li	s8, 0

	li	s8, 1024 * 1024 * 2
#if 0
	li	t0, CF_7_TS		/* Use when cache size is in cfg reg */
	and	t1, t3, t0
	beq	t1, t0, Conf7KL2
	srl	t1, CF_7_TS_AL

	li	s8, 5024288		/* 512k */
	sll	s8, t1
#endif

Conf7KL2:
	and	t1, t3, CF_7_SC
	bnez	t1, Conf7KEnd
	li	s7, 0

	li	s7, 262144		/* Size of L2 cache */

#endif
	li      s7,0
	li      s8,0

Conf7KEnd:

	TTYDBG("Disable cache exceptions...\r\n");

	mfc0	t0, COP_0_STATUS_REG
	and	t1, t0, SR_BOOT_EXC_VEC
	or	t1, SR_DIAG_DE
	mtc0	t1, COP_0_STATUS_REG

	mtc0	zero, COP_0_TAG_LO
	mtc0	zero, COP_0_TAG_HI
	mtc0	zero, COP_0_ECC

	and	t2, t3, ~(CF_7_SE|CF_7_TE)
	mtc0	t2, COP_0_CONFIG		/* Disable L2 and L3 */
	NOP8

/*
 *  Do L1 instruction cache.
 */

	TTYDBG("Init L1 instruction cache...\r\n")

	la	a0, CACHED_MEMORY_ADDR
	addu	a1, a0, s3			/* End = size of I cache */

1:
	addu	a0, s4				/* Step by line size */
	cache	IndexStoreTagI, -4(a0)
	nop
	cache	FillI, -4(a0)
	nop
	bne	a0, a1, 1b
	cache	IndexStoreTagI, -4(a0)

/*
 *  Do L1 data cache.
 */

	TTYDBG("Init L1 data cache...\r\n")

	la	a0, CACHED_MEMORY_ADDR
	add	a1, a0, s5			/* End = size of D cache */

1:
	addu	a0, s6				/* Step by line size */
	bne	a0, a1, 1b
	cache	IndexStoreTagD, -4(a0)

	la	a0, CACHED_MEMORY_ADDR
	add	a1, a0, s5			/* End = size of D cache */

1:
	addu	a0, s6				/* Step by line size */
	bne	a0, a1, 1b
	lw	zero, -4(a0)

	la	a0, CACHED_MEMORY_ADDR
	add	a1, a0, s5			/* End = size of D cache */

1:
	addu	a0, s6				/* Step by line size */
	bne	a0, a1, 1b
	cache	IndexStoreTagD, -4(a0)

	beqz	s7, no_L2_cache
	nop

/*
 *  Do L2 cache
 */

	TTYDBG("Init L2 unified cache...\r\n")

	or	t3, CF_7_SE			/* Enable secondary cache */
	mtc0	t3, COP_0_CONFIG
	NOP8

	la	a0, CACHED_MEMORY_ADDR
	add	a1, a0, s7			/* End = size of L2 cache */

1:
	addu	a0, 32				/* Step by line size */
	bne	a0, a1, 1b
	cache	IndexStoreTagS, -4(a0)

	sync

	la	a0, CACHED_MEMORY_ADDR
	add	a1, a0, s7			/* End = size of L2 cache */

1:
	addu	a0, 32				/* Step by line size */
	bne	a0, a1, 1b
	lw	zero, -4(a0)

	sync

	la	a0, CACHED_MEMORY_ADDR
	add	a1, a0, s7			/* End = size of L2 cache */

1:
	addu	a0, 32				/* Step by line size */
	bne	a0, a1, 1b
	cache	IndexStoreTagS, -4(a0)

	sync

no_L2_cache:

/*
 *  Do any L3 cache
 */

	beqz	s8, no_L3_cache			/* Any L3 size? */
	nop

	TTYDBG("Init L3 unified cache...\r\n")

	or	t3, CF_7_TE			/* Enable tertiary cache */
	mtc0	t3, COP_0_CONFIG
	NOP8

	mtc0	zero, COP_0_TAG_HI
	mtc0	zero, COP_0_TAG_LO

	la	a0, CACHED_MEMORY_ADDR
	addu	a1, a0, s8			/* Compute size of L3 */

1:
	cache	IndexStoreTagT, 0(a0)
	addu	a0, 32
	bne	a0, a1, 1b
	nop

	lw	a0, CPU_CONFIG(s2)		/* Set GT64240 for L3 cache */
	li	a1, HTOLE32(0x00004000)	
	or	a0, a0, a1
	sw	a0, CPU_CONFIG(s2)

no_L3_cache:

	mtc0	t0, COP_0_STATUS_REG		/* Restore status reg */
	mtc0	t3, COP_0_CONFIG		/* Restore cache config */
	NOP8

cache_done:

#ifdef DEBUG_LOCORE
	TTYDBG("Init caches done, cfg = ")
	mfc0	a0, COP_0_CONFIG
	bal	hexserial
	nop
	TTYDBG("\r\n")
#endif
#if 0 
	TTYDBG("Testing memory...\r\n")

	li	t7, 10
tmem:
	li	t0, 0xa0000000+1*1024*1024
	li	t1, 0xa0000000
	li	t2, 0xffffffff
1:
	sw	t2, 0(t1)
	lw	t3, 0(t1)
	bne	t3, t2, 1f
	nop
	not	t2
	sw	t2, 0(t1)
	lw	t3, 0(t1)
	bne	t3, t2, 1f
	nop
	not t2
	addu	t2, 1
	addu	t1, 4
	beq	t1, t0, 2f
	nop
	and	t4, t1, 0x000fffff
	bnez	t4, skipdot
	li	a0, '.'
	bal	tgt_putchar
	nop
skipdot:
	b	1b
	nop
1:
	TTYDBG("Memory test failed at ");
	move	a0,	t1
	bal	hexserial
	nop
	TTYDBG("\r\nWrite=");
	move	a0, t2
	bal	hexserial
	nop
	TTYDBG("\r\nRead=");
	move	a0, t3
	bal	hexserial
	nop
1:
	b	1b
	nop
2:
	TTYDBG("Testing ok...\r\n");
	sub	t7,1
	beqz	t7, 1f
	nop
	b	tmem
	nop
1:	
	b	1b
	nop
#endif


#if 1 //Ê¹ÄÜcache
        mfc0   a0,COP_0_CONFIG
        and    a0,a0,0xfffffff8
        or     a0,a0,0x3
        mtc0   a0,COP_0_CONFIG
#endif

/*
 *  At this point all memory controller setup should have been done
 *  and we should be able to function 'normally' and C code can be
 *  used freely from this point.
 */
	TTYDBG("Copy PMON to execute location...\r\n")
#ifdef DEBUG_LOCORE
	TTYDBG("start = ")
	la	a0, start
	bal	hexserial
	nop
	TTYDBG("\r\ns0 = ")
	move	a0, s0
	bal	hexserial
	nop
	TTYDBG("\r\n")
#endif
	la	a0, start
	li	a1, 0xbfc00000
	la	a2, _edata
	subu	t1, a2, a0
	srl t1, t1, 2

	/* copy text section */
	li	t0, 0
1:	lw	v0, 0(a1)
	nop
	sw	v0, 0(a0)
	addu	a0, 4
	bne a2, a0, 1b
	addu	a1, 4
	
	/* Clear BSS */
	la	a0, _edata
	la	a2, _end
2:	sw	zero, 0(a0)
	bne a2, a0, 2b
	addu	a0, 4
#if 0
	la	a1, start	/* RAM start address */
	la	v0, copytoram
	addu	v0, s0		/* Compute ROM address of 'copytoram' */
	jal	v0
	add	a0, a1, s0	/* ROM start address */

	beqz	v0, 1f
	nop

	move	s3, v0
	PRINTSTR("\r\nPANIC! Copy to memory failed at 0x")
	move	a0, s3
	bal	hexserial
	nop
	PRINTSTR(".\r\n")
	b	stuck
	nop
#endif

1:
	TTYDBG("Copy PMON to execute location done.\r\n")

	move	a3,s1	
	
#ifdef SKIPDIMM
	li	a3,	256
#endif

	la	a0, start
	la	a2, _edata
	subu	a2, a2, a0
	srl	a1, a1, 2
	la	v0, decompress_pmon
	jalr	v0
	nop

stuck:
#ifdef DEBUG_LOCORE
	TTYDBG("Dumping GT64240 setup.\r\n")
	TTYDBG("offset----data------------------------.\r\n")
	li	s3, 0
1:
	move	a0, s3
	bal	hexserial
	nop
	TTYDBG(": ")
2:
	add	a0, s3, s2
	lw	a0, 0(a0)
	bal	hexserial
	addiu	s3, 4
	TTYDBG(" ")
	li	a0, 0xfff
	and	a0, s3
	beqz	a0, 3f
	li	a0, 0x01f
	and	a0, s3
	bnez	a0, 2b
	TTYDBG("\r\n")
	b	1b
	nop
3:
	b	3b
	nop

#else
	b	stuck
	nop
#endif

/*
 *  Clear the TLB. Normally called from start.S.
 */
LEAF(CPU_TLBClear)
	li	a3, 0			# First TLB index.

	li	a2, PG_SIZE_4K
	dmtc0   a2, COP_0_TLB_PG_MASK   # Whatever...

1:
	dmtc0   zero, COP_0_TLB_HI	# Clear entry high.
	dmtc0   zero, COP_0_TLB_LO0	# Clear entry low0.
	dmtc0   zero, COP_0_TLB_LO1	# Clear entry low1.

	mtc0    a3, COP_0_TLB_INDEX	# Set the index.
	addiu	a3, 1
	li	a2, 64
	nop
	nop
	tlbwi				# Write the TLB

	bne	a3, a2, 1b
	nop

	jr	ra
	nop
END(CPU_TLBClear)

/*
 *  Set up the TLB. Normally called from start.S.
 */
LEAF(CPU_TLBInit)
	li	a3, 0			# First TLB index.

	li	a2, PG_SIZE_16M
	dmtc0   a2, COP_0_TLB_PG_MASK   # All pages are 16Mb.

1:
	and	a2, a0, PG_SVPN
	dmtc0   a2, COP_0_TLB_HI	# Set up entry high.

	move	a2, a0
	srl	a2, a0, PG_SHIFT 
	and	a2, a2, PG_FRAME
	ori	a2, PG_IOPAGE
	dmtc0   a2, COP_0_TLB_LO0	# Set up entry low0.
	addu	a2, (0x01000000 >> PG_SHIFT)
	dmtc0   a2, COP_0_TLB_LO1	# Set up entry low1.

	mtc0    a3, COP_0_TLB_INDEX	# Set the index.
	addiu	a3, 1
	li	a2, 0x02000000
	subu	a1, a2
	nop
	tlbwi				# Write the TLB

	bgtz	a1, 1b
	addu	a0, a2			# Step address 32Mb.

	jr	ra
	nop
END(CPU_TLBInit)

/*
 *  Set DEVPAR for device bus timing.
 */

	.globl	tgt_setpar125mhz
tgt_setpar125mhz:
	move	a0, ra		/* Don't put in delay slot! */
	bal	do_table	/* Load address to init table */
	nop

	/* Device CS0 - PLD */
        GTINIT(DEVICE_BANK0PARAMETERS, \
				GT_DEVPAR_TurnOff(2) |		\
				GT_DEVPAR_AccToFirst(8) |	\
				GT_DEVPAR_AccToNext(8)	|	\
				GT_DEVPAR_ALEtoWr(3) |		\
				GT_DEVPAR_WrActive(3) |		\
				GT_DEVPAR_WrHigh(5) |		\
				GT_DEVPAR_DevWidth8 |		\
				GT_DEVPAR_Reserved)

	/* Device CS1 - RTC */
        GTINIT(DEVICE_BANK1PARAMETERS, \
				GT_DEVPAR_TurnOff(2) |		\
				GT_DEVPAR_AccToFirst(13) |	\
				GT_DEVPAR_AccToNext(13)	|	\
				GT_DEVPAR_ALEtoWr(5) |		\
				GT_DEVPAR_WrActive(7) |		\
				GT_DEVPAR_WrHigh(5) |		\
				GT_DEVPAR_DevWidth8 |		\
				GT_DEVPAR_Reserved)

	/* Device CS2 - UART */
        GTINIT(DEVICE_BANK2PARAMETERS, \
				GT_DEVPAR_TurnOff(3) |		\
				GT_DEVPAR_AccToFirst(15) |	\
				GT_DEVPAR_AccToNext(15)	|	\
				GT_DEVPAR_ALEtoWr(5) |		\
				GT_DEVPAR_WrActive(8) |		\
				GT_DEVPAR_WrHigh(5) |		\
				GT_DEVPAR_DevWidth8 |		\
				GT_DEVPAR_Reserved)
				
	/* end mark */
	.word	0, 0

	.globl	tgt_setpar100mhz
tgt_setpar100mhz:
	move	a0, ra		/* Don't put in delay slot! */
	bal	do_table	/* Load address to init table */
	nop

	/* Device CS0 - PLD */
        GTINIT(DEVICE_BANK0PARAMETERS, \
				GT_DEVPAR_TurnOff(3) |		\
				GT_DEVPAR_AccToFirst(6) |	\
				GT_DEVPAR_AccToNext(6)	|	\
				GT_DEVPAR_ALEtoWr(3) |		\
				GT_DEVPAR_WrActive(3) |		\
				GT_DEVPAR_WrHigh(5) |		\
				GT_DEVPAR_DevWidth8 |		\
				GT_DEVPAR_Reserved)

	/* Device CS1 - NVRAM */
        GTINIT(DEVICE_BANK1PARAMETERS, \
				GT_DEVPAR_TurnOff(3) |		\
				GT_DEVPAR_AccToFirst(10) |	\
				GT_DEVPAR_AccToNext(10)	|	\
				GT_DEVPAR_ALEtoWr(5) |		\
				GT_DEVPAR_WrActive(6) |		\
				GT_DEVPAR_WrHigh(5) |		\
				GT_DEVPAR_DevWidth8 |		\
				GT_DEVPAR_Reserved)

	/* Device CS2 - UART */
        GTINIT(DEVICE_BANK2PARAMETERS, \
				GT_DEVPAR_TurnOff(4) |		\
				GT_DEVPAR_AccToFirst(11) |	\
				GT_DEVPAR_AccToNext(11)	|	\
				GT_DEVPAR_ALEtoWr(5) |		\
				GT_DEVPAR_WrActive(6) |		\
				GT_DEVPAR_WrHigh(5) |		\
				GT_DEVPAR_DevWidth8 |		\
				GT_DEVPAR_Reserved)
	/* end mark */
	.word	0, 0

1:
	sw	v1, 0(v0)
do_table:
	lw	v0, 0(ra)		/* Address */
	lw	v1, 4(ra)		/* Data */
	bnez	v0, 1b
	addiu	ra, 8

	jr	a0
	nop


/*
 * Simple character printing routine used before full initialization
 */

LEAF(stringserial)
	move	a2, ra
	addu	a1, a0, s0
	lbu	a0, 0(a1)
1:
	beqz	a0, 2f
	nop
	bal	tgt_putchar
	addiu	a1, 1
	b	1b
	lbu	a0, 0(a1)

2:
	j	a2
	nop
END(stringserial)

LEAF(stringserial_zsh)
	move	a2, ra
	lbu	a0, 0(a1)
1:
	beqz	a0, 2f
	nop
	bal	tgt_putchar
	addiu	a1, 1
	b	1b
	lbu	a0, 0(a1)

2:
	j	a2
	nop
END(stringserial_zsh)

LEAF(hexserial_zfx)
	move	a2, ra
	move	a1, a0
	li	a3, 7
1:
	rol	a0, a1, 4
	move	a1, a0
	and	a0, 0xf
	la	v0, hexchar
	addu	v0, a0
#	bal	tgt_putchar
#	lbu	a0, 0(v0)

	bnez	a3, 1b
	addu	a3, -1
#	bal	tgt_putchar
#	li	a0, 0xd
#	bal	tgt_putchar
	li	a0, 0xa

	j	a2
	nop
END(hexserial_zfx)

LEAF(hexserial)
	move	a2, ra
	move	a1, a0
	li	a3, 7
1:
	rol	a0, a1, 4
	move	a1, a0
	and	a0, 0xf
	la	v0, hexchar
	addu	v0, s0
	addu	v0, a0
	bal	tgt_putchar
	lbu	a0, 0(v0)

	bnez	a3, 1b
	addu	a3, -1

	j	a2
	nop
END(hexserial)
	

#if 0
LEAF(tgt_putchar)
	move   t8,ra
	bal     1f
	nop
1:
	addu   ra,ra,16
	or     ra,ra,0x20000000
	jr     ra
        nop

	la	v0, COM1_BASE_ADDR
1:
	lbu	v1, NSREG(NS16550_LSR)(v0)
	and	v1, LSR_TXRDY
	beqz	v1, 1b
	nop

	sb	a0, NSREG(NS16550_DATA)(v0)

        move    ra,t8
  
	j	ra
	nop	
END(tgt_putchar)
#endif

LEAF(tgt_putchar)
	la	v0, COM1_BASE_ADDR
1:
	lbu	v1, NSREG(NS16550_LSR)(v0)
	and	v1, LSR_TXRDY
	beqz	v1, 1b
	nop

	sb	a0, NSREG(NS16550_DATA)(v0)

	j	ra
	nop	
END(tgt_putchar)

/* baud rate definitions, matching include/termios.h */
#define B0      0
#define B50     50      
#define B75     75
#define B110    110
#define B134    134
#define B150    150
#define B200    200
#define B300    300
#define B600    600
#define B1200   1200
#define B1800   1800
#define B2400   2400
#define B4800   4800
#define B9600   9600
#define B19200  19200
#define B38400  38400
#define B57600  57600
#define B115200 115200

LEAF(initserial)
	la	v0, COM1_BASE_ADDR
1:
	li	v1, FIFO_ENABLE|FIFO_RCV_RST|FIFO_XMT_RST|FIFO_TRIGGER_4
	sb	v1, NSREG(NS16550_FIFO)(v0)
	li	v1, CFCR_DLAB
	sb	v1, NSREG(NS16550_CFCR)(v0)
 	li	v1, NS16550HZ/(16*CONS_BAUD)
	sb	v1, NSREG(NS16550_DATA)(v0)
	srl	v1, 8
	sb	v1, NSREG(NS16550_IER)(v0)
	li	v1, CFCR_8BITS
	sb	v1, NSREG(NS16550_CFCR)(v0)
#if 0
	li	v1, MCR_DTR|MCR_RTS
#endif
	sb	v1, NSREG(NS16550_MCR)(v0)
	li	v1, 0x0
	sb	v1, NSREG(NS16550_IER)(v0)
#if 0
	move	v1, v0
	la	v0, COM2_BASE_ADDR
	bne	v0, v1, 1b
	nop
#endif
	j	ra
	nop
END(initserial)

__main:
	j	ra
	nop


	.rdata
transmit_pat_msg:
	.asciz	"\r\nInvalid transmit pattern.  Must be DDDD or DDxDDx\r\n"
v200_msg:
	.asciz	"\r\nPANIC! Unexpected TLB refill exception!\r\n"
v280_msg:
	.asciz	"\r\nPANIC! Unexpected XTLB refill exception!\r\n"
v380_msg:
	.asciz	"\r\nPANIC! Unexpected General exception!\r\n"
v400_msg:
	.asciz	"\r\nPANIC! Unexpected Interrupt exception!\r\n"
hexchar:
	.ascii	"0123456789abcdef"

	.text
	.align	2


/*
 *   I2C Functions used in early startup code to get SPD info from
 *   SDRAM modules. This code must be entirely PIC and RAM independent.
 */

/* Delay macro */
#define	DELAY(count)	\
	li v0, count;	\
99:			\
	bnz	v0, 99b;\
	addiu	v0, -1

#define GT_REGAD(offs)			\
	la	v1, GT_BASE_ADDR+(offs)

#define GT_REGRD(offs)			\
	lw	v0, GT_BASE_ADDR+(offs)

#define GT_REGWR(offs, value)		\
	li	v0, HTOLE32(value);	\
	sw	v0, GT_BASE_ADDR+(offs)

#define GT_REGSET(offs, value)		\
	lw	v0, GT_BASE_ADDR+(offs);\
	li	v1, HTOLE32(value);	\
	or	v0, v1;			\
	sw	v0, GT_BASE_ADDR+(offs)

#define GT_REGCLR(offs, value)		\
	lw	v0, GT_BASE_ADDR+(offs);\
	li	v1, HTOLE32(~(value));	\
	and	v0, v1;			\
	sw	v0, GT_BASE_ADDR+(offs)

#define I2C_INT_ENABLE	0x80
#define I2C_ENABLE	0x40
#define I2C_ACK		0x04
#define I2C_INT_FLAG	0x08
#define I2C_STOP_BIT	0x10
#define I2C_START_BIT	0x20

#define	I2C_AMOD_RD	0x01

#define	BUS_ERROR				0x00
#define	START_CONDITION_TRA			0x08
#define	RSTART_CONDITION_TRA			0x10
#define	ADDR_AND_WRITE_BIT_TRA_ACK_REC		0x18
#define	ADDR_AND_READ_BIT_TRA_ACK_REC		0x40
#define	SLAVE_REC_WRITE_DATA_ACK_TRA		0x28
#define	MAS_REC_READ_DATA_ACK_NOT_TRA		0x58

/*
 *  Wait for interrupt, return status byte
 */
wait_int:
	GT_REGRD(I2C_CONTROL)
	li	v1, HTOLE32(I2C_INT_FLAG)
	and	v0, v1
	beqz	v0, wait_int
	nop

	GT_REGRD(I2C_STATUS_BAUDE_RATE)
	jr	ra
	nop

/*
 *  I2C Master init.
 */

	.globl	boot_i2c_init
boot_i2c_init:

	GT_REGWR(I2C_SOFT_RESET, 0x0)
	GT_REGWR(I2C_STATUS_BAUDE_RATE, 0x24);
	GT_REGWR(I2C_CONTROL, I2C_ENABLE)

	jr	ra
	nop

/*
 * I2C Read byte from device. Use RANDOM READ protocol.
 */

	.globl	boot_i2c_read
boot_i2c_read:

	move	t0, ra				/* Save return address */

	GT_REGSET(I2C_CONTROL, I2C_START_BIT)

	bal	wait_int
	nop

	li	v1, HTOLE32(START_CONDITION_TRA)
	bne	v0, v1, boot_i2c_read_bad		/* Bad start, exit */
	nop
/**/
	andi	v0, a0, 0x700			/* Get device part of addr */
	srl	v0, v0, 7
	ori	v0, 0xa0			/* Device type + write(addr) */
#if BYTE_ORDER == BIG_ENDIAN
	sll	v0, v0, 24
#endif
	GT_REGAD(I2C_DATA)			/* Send device address */
	sw	v0, 0(v1)

	GT_REGCLR(I2C_CONTROL, I2C_INT_FLAG)	/* Send it */

	bal	wait_int
	nop

	li	v1, HTOLE32(ADDR_AND_WRITE_BIT_TRA_ACK_REC)
	bne	v0, v1, boot_i2c_read_bad
	nop
/**/
	andi	v0, a0, 0xff
#if BYTE_ORDER == BIG_ENDIAN
	sll	v0, v0, 24
#endif
	GT_REGAD(I2C_DATA)			/* Send address */
	sw	v0, 0(v1)
	
	GT_REGCLR(I2C_CONTROL, I2C_INT_FLAG)	/* Send it */

	bal	wait_int
	nop

	li	v1, HTOLE32(SLAVE_REC_WRITE_DATA_ACK_TRA)
	bne	v0, v1, boot_i2c_read_bad
	nop
/**/
	GT_REGSET(I2C_CONTROL, I2C_START_BIT)	/* Restart! */
	GT_REGCLR(I2C_CONTROL, I2C_INT_FLAG)	/* Send it */

	bal	wait_int
	nop

	li	v1, HTOLE32(RSTART_CONDITION_TRA)
	bne	v0, v1, boot_i2c_read_bad		/* Bad start, exit */
	nop
/**/
	andi	v0, a0, 0x700			/* Get device part of addr */
	srl	v0, v0, 7
	ori	v0, 0xa1			/* Device type + read */
#if BYTE_ORDER == BIG_ENDIAN
	sll	v0, v0, 24
#endif
	GT_REGAD(I2C_DATA)			/* Send device address */
	sw	v0, 0(v1)

	GT_REGCLR(I2C_CONTROL, I2C_INT_FLAG)	/* Send it */

	bal	wait_int
	nop

	li	v1, HTOLE32(ADDR_AND_READ_BIT_TRA_ACK_REC)
	bne	v0, v1, boot_i2c_read_bad
	nop
/**/
	GT_REGCLR(I2C_CONTROL, I2C_INT_FLAG | I2C_ACK)	/* Get data */
/**/
	bal	wait_int
	nop

	li	v1, HTOLE32(MAS_REC_READ_DATA_ACK_NOT_TRA)
	bne	v1, v0, boot_i2c_read_bad
	nop

	GT_REGRD(I2C_DATA)
#if BYTE_ORDER == BIG_ENDIAN
	srl	v0, v0, 24
#endif

	b	boot_i2c_read_end
	nop
/**/
boot_i2c_read_bad:
	li	v0, -1

boot_i2c_read_end:
	move	a0, v0
	GT_REGSET(I2C_CONTROL, I2C_STOP_BIT)
	GT_REGCLR(I2C_CONTROL, I2C_INT_FLAG)
	move	v0, a0

	jr	t0
	nop

	.globl	probe_sdram_size
probe_sdram_size:
	move	a2, ra
	andi	a0, 0x700
	move	a3, a0

	bal	boot_i2c_read
	nop

	li	t1, -1
	beq	t1, v0, 1f
	nop
	
	or	a0, a3, 5
	bal	boot_i2c_read
	nop
	move t1, v0

	or	a0, a3, 31
	bal	boot_i2c_read
	nop

	mult	t1, v0
	mflo	t1

	or	a0, a3, 17
	bal	boot_i2c_read
	nop

	mult	t1, v0
	mflo	t1

	sll	v0, t1, 20
	b	2f
	nop
1:
	li	v0, 0
2:
	jr	a2
	nop
#define Index_Invalidate_I      0x00
#define Index_Writeback_Inv_D   0x01
#define Index_Invalidate_SI     0x02
#define Index_Writeback_Inv_SD  0x03
#define Index_Load_Tag_I	0x04
#define Index_Load_Tag_D	0x05
#define Index_Load_Tag_SI	0x06
#define Index_Load_Tag_SD	0x07
#define Index_Store_Tag_I	0x08
#define Index_Store_Tag_D	0x09
#define Index_Store_Tag_SI	0x0A
#define Index_Store_Tag_SD	0x0B
#define Create_Dirty_Excl_D	0x0d
#define Create_Dirty_Excl_SD	0x0f
#define Hit_Invalidate_I	0x10
#define Hit_Invalidate_D	0x11
#define Hit_Invalidate_SI	0x12
#define Hit_Invalidate_SD	0x13
#define Fill			0x14
#define Hit_Writeback_Inv_D	0x15
					/* 0x16 is unused */
#define Hit_Writeback_Inv_SD	0x17
#define Hit_Writeback_I		0x18
#define Hit_Writeback_D		0x19
					/* 0x1a is unused */
#define Hit_Writeback_SD	0x1b
					/* 0x1c is unused */
					/* 0x1e is unused */
#define Hit_Set_Virtual_SI	0x1e
#define Hit_Set_Virtual_SD	0x1f

#define CP0_CONFIG $16
#define CP0_TAGLO  $28
#define CP0_TAGHI  $29

LEAF(godson2_cache_init)
####part 2####
cache_detect_2way:
	mfc0	t4, CP0_CONFIG
	andi	t5, t4, 0x0e00
	srl	t5, t5, 9
	andi	t6, t4, 0x01c0
	srl	t6, t6, 6
	addiu	t6, t6, 11
	addiu	t5, t5, 11
	addiu	t4, $0, 1
	sllv	t6, t4, t6
	sllv	t5, t4, t5
	addiu	t7, $0, 2
####part 3####
	lui	a0, 0x8000
	addu	a1, $0, t5
	addu	a2, $0, t6
cache_init_d2way:
#a0=0x80000000, a1=icache_size, a2=dcache_size
#a3, v0 and v1 used as local registers
	mtc0	$0, CP0_TAGHI
	addu	v0, $0, a0
	addu	v1, a0, a2
1:	slt	a3, v0, v1
	beq	a3, $0, 1f
	nop
	mtc0	$0, CP0_TAGLO
	cache	Index_Store_Tag_D, 0x0(v0)
	mtc0	$0, CP0_TAGLO
	cache	Index_Store_Tag_D, 0x1(v0)
	beq	$0, $0, 1b
	addiu	v0, v0, 0x20
1:
cache_flush_i2way:
	addu	v0, $0, a0
	addu	v1, a0, a1
1:	slt	a3, v0, v1
	beq	a3, $0, 1f
	nop
	cache	Index_Invalidate_I, 0x0(v0)
#	cache	Index_Invalidate_I, 0x1(v0)
	beq	$0, $0, 1b
	addiu	v0, v0, 0x20
1:
cache_flush_d2way:
	addu	v0, $0, a0
	addu	v1, a0, a2
1:	slt	a3, v0, v1
	beq	a3, $0, 1f
	nop
	cache	Index_Writeback_Inv_D, 0x0(v0)
	cache	Index_Writeback_Inv_D, 0x1(v0)
	beq	$0, $0, 1b
	addiu	v0, v0, 0x20
1:
cache_init_finish:
	nop
	jr	ra
	nop

cache_init_panic:
	TTYDBG("cache init panic\r\n");
1:	b	1b
	nop
	.end	godson2_cache_init

LEAF(test_icache_1)
#define TEST_ILINE \
.align 5 ; \
1: lw v0, (a0) ; \
.align 0 ; \
addiu v0, 1 ; \
sw v0, (a0) ; \
b 1f ; \
nop

.set noreorder
//.set mips3
.align 5
b 1f
sw zero, (a0)
TEST_ILINE
TEST_ILINE
TEST_ILINE
TEST_ILINE
TEST_ILINE
TEST_ILINE
TEST_ILINE
TEST_ILINE

.align 5
1:
jr ra
nop
END(test_icache_1)

LEAF(test_icache_2)
#undef  TEST_ILINE
#define TEST_ILINE \
.align 5 ; \
1: move v0, a0 ; \
.align 0 ; \
addiu v0, 1 ; \
move a0, v0 ; \
b 1f ; \
nop

.set noreorder
//.set mips3
.align 5
b 1f
move zero, a0
TEST_ILINE
TEST_ILINE
TEST_ILINE
TEST_ILINE
TEST_ILINE
TEST_ILINE
TEST_ILINE
TEST_ILINE

.align 5
1:
jr ra
nop
END(test_icache_2)

LEAF(test_icache_3)
#undef  TEST_ILINE
#define TEST_ILINE \
.align 5 ; \
nop ;\
nop ;\
nop ;\
nop ;\
nop 

.set noreorder
//.set mips3
.align 5
TEST_ILINE
TEST_ILINE
TEST_ILINE
TEST_ILINE
TEST_ILINE
TEST_ILINE
TEST_ILINE
TEST_ILINE

.align 5
1:
jr ra
nop
END(test_icache_3)

LEAF(test_ld)
lui a0,0xa040
ori a0,a0,0
li   t0,0xaa55aa55
dsll t0,t0,32
ori  t0,t0,0xaa55

sd  t0,0(a0)
ld  t1,0(a0)

beq t0,t1,1f
nop

TTYDBG("Write=");
dsrl  a0,t0,32
bal   hexserial
nop
move  a0,t0
bal   hexserial
nop

TTYDBG("Load back=");
dsrl  a0,t1,32
bal   hexserial
nop
move  a0,t1
bal   hexserial
nop

TTYDBG("LD/SD test failure!!")

1: 
// TTYDBG("OK");
 j ra
 nop
END(test_ld)

LEAF(fill_icache_1)
.align 5
/*
	addiu a0,a0,1
	addiu a0,a0,1
        lui v0,0x8020
        sw a0,0(v0)
	lw a0,0(v0)
        sw a0,36(v0)
*/
/*
	lui  v0,0xbd00
	ori  v0,v0,0x20
	lbu  v1,23(v0)
	andi v1,v1,0x20
	beqz v1,1f
	nop
*/
/*
	.set noreorder
	move t8,ra
	bal tgt_putchar
	li  a0,'='
	bal tgt_putchar
	li  a0,'@'
*/
	move t9,ra
	li  a0,'@'
	jal tgt_putchar
	nop
	move ra,t9
	jr ra
	nop
END(fill_icache_1)

LEAF(godson2_cache_flush)
        li    a0,0x80000000
        addu  a1,a0,16384
1:
        cache  1,0(a0)
        cache  1,1(a0)
        cache  0,(a0)
        add    a0,a0,32
        beq    a0,a1,1b
        nop
        TTYDBG("cache flushed");
END(godson2_cache_flush)

	.rdata;
.align 2
argv_array: 
	.word   0
	.word   0
	.word   0
.align  2
env_array:
	.word   0
	.word   0
	.word   0
.align  2
argv0:
	.asciz "g"
	.word   0
.align  2
argv1:
        .asciz "root=/dev/hda1" 
	.word   0
.align  2
env0:
        .asciz "cpuclock=3000000" 
	.word   0
.align  2
env1:
        .asciz "gtbase=b4000000" 
	.word   0
.align  2
arg_end:
	.text
	
	TTYDBG("Copy linux kernel to execute location...\r\n")

#if 0
	li     a0, 0 
	bnezl  a0,1f
	sw     zero,0(a0)
1:	
#endif

#if 0
LEAF(load_linux)
	move   s6,a0 // entry
	move   s7,a1 // flash addr


        mfc0    a0,COP_0_CONFIG
        and    a0,a0,0xfffffff8
        or     a0,a0,0x2
	mtc0   a0,COP_0_CONFIG

        move    s0, zero // we are called from cached space,hexserial need it??
        
	TTYDBG("Copying linux kernel...\r\n");

#if 0
	la	a0, 0xa0100000
	li	a1, 0xbf100000
	la	a2, 0xa0400000
#endif
	la	a0, 0xa0100000
	move	a1, s7
	la	a2, 0xa0400000

	/* copy text section */
	li	t0, 0
1:	ld	v0, 0(a1)
	nop
	sd	v0, 0(a0)
	addu	a0, 8
	and     t6, a0, 0xffff
	bnez    t6, 2f
	nop
	move    t7,a0
	li      a0,'.'
	bal     tgt_putchar
	nop
	move    a0,t7
2:	bne a2, a0, 1b
	addu	a1, 8

	move s0,zero
	TTYDBG("Copy done\r\n");

#if 1
do_copy_param:
	move s0,zero
	TTYDBG("Copy params...\r\n")
	la	a0, argv_array
	addu    a0,0xbfc00000 - 0x80020000

	la	a1, argv_array

	la      a2, arg_end
	addu    a2,0xbfc00000 - 0x80020000

1:	lw	v0, 0(a0)
	nop
	sw	v0, 0(a1)
	addu	a0, 4
	bne     a2, a0, 1b
	addu	a1, 4

	move s0,zero
	TTYDBG("Copy param done\r\n")
#endif

	li	a0,2
	
	la	a1,argv_array

        la      t0,argv0
	sw      t0,0(a1); 

        la      t0,argv1
	sw      t0,4(a1); 

	sw      zero,8(a1); 

	la	a2,env_array

        la      t0,env0
	sw      t0,0(a2); 

        la      t0,env1
	sw      t0,4(a2); 

	sw      zero,8(a2); 

	move s0,zero
	move s2,a0
	move s3,a1
	move s4,a2
	TTYDBG("jumping\r\n");
	move a0,s2
	move a1,s3
	move a2,s4

	//li      t9,0x802b4040
	//li      t9,0x8026a040
	//jalr    t9
	jalr   s6
	nop
END(load_linux)
#endif

LEAF(init_regs)
/* all initial registers to zero */
	.set push
	.set mips3
	.set noat
	.set noreorder
#if 0
        mfc0   $1,$12
        lui    $2,0x400 /* FR = 1 */
         or    $1,$1,$2
        mtc0   $1,$12
         
        dmtc1  $0,$0
        dmtc1  $0,$1
        dmtc1  $0,$2
        dmtc1  $0,$3
        dmtc1  $0,$4
        dmtc1  $0,$5
        dmtc1  $0,$6
        dmtc1  $0,$7
        dmtc1  $0,$8
        dmtc1  $0,$9

	c.lt.d $f0,$f0

        dmtc1  $0,$10
        dmtc1  $0,$11
        dmtc1  $0,$12
        dmtc1  $0,$13
        dmtc1  $0,$14
        dmtc1  $0,$15
        dmtc1  $0,$16
        dmtc1  $0,$17
        dmtc1  $0,$18
        dmtc1  $0,$19
        dmtc1  $0,$20
        dmtc1  $0,$21
        dmtc1  $0,$22
        dmtc1  $0,$23
        dmtc1  $0,$24
        dmtc1  $0,$25
        dmtc1  $0,$26
        dmtc1  $0,$27
        dmtc1  $0,$28
        dmtc1  $0,$29
        dmtc1  $0,$30
        dmtc1  $0,$31


/* fix registers */
	daddu  $0,$0,$0
	daddu  $1,$0,$0
	daddu  $2,$0,$0
	daddu  $3,$0,$0
	daddu  $4,$0,$0
	daddu  $5,$0,$0
	daddu  $6,$0,$0
	daddu  $7,$0,$0
	daddu  $8,$0,$0
	daddu  $9,$0,$0

	daddu  $10,$0,$0
	daddu  $11,$0,$0
	daddu  $12,$0,$0
	daddu  $13,$0,$0
	daddu  $14,$0,$0
	daddu  $15,$0,$0
	daddu  $16,$0,$0
	daddu  $17,$0,$0
	daddu  $18,$0,$0
	daddu  $19,$0,$0

	daddu  $20,$0,$0
	daddu  $21,$0,$0
	daddu  $22,$0,$0
	daddu  $23,$0,$0
	daddu  $24,$0,$0
	daddu  $25,$0,$0
	daddu  $26,$0,$0
	daddu  $27,$0,$0
	daddu  $28,$0,$0
	daddu  $29,$0,$0

	daddu  $30,$0,$0
	//daddu  $31,$0,$0

	mthi   $0
	mtlo   $0

	mtc0	zero, COP_0_STATUS_REG
	mtc0	zero, COP_0_CAUSE_REG
	li	t0, SR_BOOT_EXC_VEC	/* Exception to Boostrap Location */
	mtc0	t0, COP_0_STATUS_REG
	la	sp, stack
	la	gp, _gp

#endif
	jr	ra
	nop
	.set pop
END(init_regs)

LEAF(test_regs)
	addiu   sp,sp,-80
	sd     ra,0(sp) 

	mfc0	t0, $16
	sd	t0, 8(sp)
	ori	t0, t0, 3
	mtc0	t0, $16

#	.align	5
	
	li     t7,1
	li     t0,-1
	li     t1,0
1:
	bal	test_reg_2
	dsrl32	a0, t7, 0

	lui	t6, 0xa141
	sd	t7, 0x0(t6)
	sd	t0, 0x8(t6)

#	bne	t7, $0, 1b
	bne	t7, t1, 1b
#	daddiu	t7, t7, -1
	daddu	t7, t7, t0

	ld	t0, 0x8(sp)
	mtc0	t0, $16

	
	ld   ra,0(sp)
	jr   ra
	addiu	sp, sp, 80
#####
test_reg_2:
	move	a2, ra
	move	a1, a0
	li	a3, 2 #7
11:
	rol	a0, a1, 4
	move	a1, a0
	and	a0, 0xf
	la	v0, hexchar
	addu	v0, a0

	bnez	a3, 11b
	addu	a3, -1
	li	a0, 0xa

	j	a2
	nop
#####
END(test_regs)
LEAF(nullfunction)
	jr ra
	nop
END(nullfunction)

/*	$Id: starto.S,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

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

#include "target/ev64420.h"

#include "pmon/dev/ns16550.h"
#include "pmon/dev/gt64420reg.h"
#include "test/macro.S"

#if __mips < 3
#define DMTC0  mtc0
#define DMFC0  mfc0
#else
#define DMTC0  dmtc0
#define DMFC0  dmfc0
#endif

/* Delay macro */
#define	DELAY(count)	\
	li v0, count;	\
99:			\
	bnez	v0, 99b;\
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

#define CS0_EN	(1<<0)
#define CS1_EN	(1<<1)
#define CS2_EN	(1<<2)
#define CS3_EN	(1<<3)
#define DEVCS0_EN	(1<<4)
#define DEVCS1_EN	(1<<5)
#define DEVCS2_EN	(1<<6)
#define DEVCS3_EN	(1<<7)
#define BOOTCS_EN (1<<8)
#define PCI_IO_EN (1<<9)
#define PCI_MEM0_EN (1<<10)
#define PCI_MEM1_EN (1<<11)
#define PCI_MEM2_EN (1<<12)
#define PCI_MEM3_EN (1<<13)

#define INIT_CPU_WINDOW \
	li	t0,(~(BOOTCS_EN))&0x3fff;	\
	sw	t0,BASE_ADDRESS_ENABLE_REG(s2);	
#define ENABLE_CPU_WINDOW(dev) \
	lw t0,BASE_ADDRESS_ENABLE_REG(s2);	\
	and t0,~dev;	\
	sw t0,BASE_ADDRESS_ENABLE_REG(s2);

#define DBG_LED0	(1<<4)     /*MPP4 as led0*/
#define DBG_LED1	(1<<5)     /*MPP5 as led1*/
#define DBG_LED2	(1<<6)	   /*MPP6 as led2*/
#define MPP_UART_TX	(1<<0)
#define MP_INTA	(1<<24)
#define MP_INTB	(1<<25)
#define MP_INTC	(1<<26)
#define MP_INTD	(1<<27)



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
#define HIGHMEM_BASE 0x20000000
/*
 *   Register usage:
 *
 *	s0	link versus load offset, used to relocate absolute adresses.
 *	s1	memsize
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
#if 1
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

#if __mips >= 3
	DMFC0 a0, $8
        nop
	dsrl a0,a0,32
        nop
#endif
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
    /* for exception in C :tmp hack*/
	li  s0, 0
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
#if __mips >= 3
	DMFC0	a0, $8
        dsrl  a0,a0,32
	bal	hexserial
        nop
#endif
	DMFC0	a0, $8
	bal	hexserial
	nop

#if 0
	PRINTSTR("\r\nDERR0=")
	cfc0	a0, COP_0_DERR_0
	bal	hexserial
	nop
	PRINTSTR("\r\nDERR1=")
	cfc0	a0, COP_0_DERR_1
	bal	hexserial
	nop
#endif
	
	1:	 b  1b
        nop
	 
//	b	ext_map_and_reboot
	nop


/*
 *  We get here from executing a bal to get the PC value of the current execute
 *  location into ra. Check to see if we run from ROM or if this is ramloaded.
 */
	.align 8    /*bfc00500*/
	.word read
	.word write
	.word open
	.word close
	.word nullfunction
	.word printf
	.word vsprintf
	.word nullfunction
	.word nullfunction
	.word getenv
	.word nullfunction
	.word nullfunction
	.word nullfunction
	.word nullfunction

	
locate:

/**/


	la	s0, start		/* RA set from BAL above! */
	subu	s0, ra, s0		/* s0 is now load vs. link offset */
	and	s0, 0xffff0000		/* Mask off lower bits */

	bal init_regs
	nop

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
/*在初始化完成前,禁掉不用的设备*/
	INIT_CPU_WINDOW	
#if 1
/*
 *  Relocate the Galileo to HIGH memory.
 */
	li	t0, HTOLE32((UNCACHED_TO_PHYS(GT_BASE_ADDR) >> 16) | 0x01000000)
	sw	t0, INTERNAL_SPACE_DECODE(s2)

	li	t0, 0x100		/* Need a small delay here */
1:
	bnez	t0, 1b
	addiu	t0, -1
#endif
	la	s2, GT_BASE_ADDR	/* From now this is the GT base */

#if 0
	GT_REGWR(0xf25c,GT_BASE_ADDR);
#endif


	GT_REGWR(GPP_IO_CONTROL,MPP_UART_TX|DBG_LED0|DBG_LED1|DBG_LED2);
 	GT_REGWR(GPP_LEVEL_CONTROL,MP_INTA|MP_INTB|MP_INTC|MP_INTD);
 	GT_REGWR(GPP_INTERRUPT_MASK,MP_INTA|MP_INTB|MP_INTC|MP_INTD);

	/* Route Multi Purpose Pins */
	GT_REGWR(MPP_CONTROL0, 0x00000322);	//MPP0,MPP1 as S0_TXD,S0_RXD,MPP2 clk24M,others set to gpio pin
	GT_REGWR(MPP_CONTROL1, 0x00000000);	
	GT_REGWR(MPP_CONTROL2, 0x11111111);	//MPP16~23 as PCI REQ or GNT.
	GT_REGWR(MPP_CONTROL3, 0x00400000); //MPP24~27 as PCI INTA-INTD,MPP29 as clk14M if nessary,others gpio.


/**/
#if 1
/*映射南桥1M IO到CPU空间0x10100000 or 0x1ea00000(for CONFIG_PCI0_GAINT_MEM)*/
	li	t0, HTOLE32(UNCACHED_TO_PHYS(PCI_IO_SPACE_BASE)>>16)
	sw	t0, PCI_I_O_BASE_ADDRESS(s2)
	li	t0, HTOLE32(0xf)
	sw	t0, PCI_I_O_SIZE(s2)
	sw	zero, PCI_I_O_ADDRESS_REMAP(s2)

	ENABLE_CPU_WINDOW(PCI_IO_EN);
	ENABLE_CPU_WINDOW(PCI_MEM0_EN);
	ENABLE_CPU_WINDOW(PCI_MEM1_EN);

	li	t0, HTOLE32(0x80000004)
	sw	t0, PCI_CONFIGURATION_ADDRESS(s2)
	lw	t1, PCI_CONFIGURATION_DATA_VIRTUAL_REGISTER(s2)
	ori	t1, 0x02|0x04
	sw	t0, PCI_CONFIGURATION_ADDRESS(s2)
	sw	t1, PCI_CONFIGURATION_DATA_VIRTUAL_REGISTER(s2)

	lw	t0, PCI_ARBITER_CONTROL(s2)
	or	t0, t0, HTOLE32(0x80000000)
	sw	t0, PCI_ARBITER_CONTROL(s2)
#endif

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

	GTINIT(DEVCS_0_BASE_ADDRESS, UNCACHED_TO_PHYS(SRAM_BASE) >>16)
	GTINIT(DEVCS_0_SIZE, (SRAM_SIZE - 1) >> 16)
	GTINIT(DEVCS_1_BASE_ADDRESS, UNCACHED_TO_PHYS(RTC_BASE) >> 16)
	GTINIT(DEVCS_1_SIZE, (RTC_SIZE - 1) >> 16)
	GTINIT(DEVCS_2_BASE_ADDRESS, UNCACHED_TO_PHYS(UART_BASE) >> 16)
	GTINIT(DEVCS_2_SIZE, (UART_SIZE - 1) >> 16)

	GTINIT(DEVCS_3_BASE_ADDRESS, UNCACHED_TO_PHYS(FLASH_BASE) >> 16)
	GTINIT(DEVCS_3_SIZE, (FLASH_SIZE - 1) >> 16)

	/* end mark */
	.word	0, 0

1:
	sw	v1, 0(v0)
2:
	lw	v0, 0(ra)		/* Address */
	lw	v1, 4(ra)		/* Data */
	bnez	v0, 1b
	addiu	ra, 8

//	ENABLE_CPU_WINDOW(/*DEVCS0_EN|DEVCS1_EN|DEVCS2_EN|*/DEVCS3_EN);

/*
 * BOOT rom. Read width bits to check boot width size to set up flash CS
 * "correctly". Bootwidth sticks with straps but for clarity we retain it
 * Also it should be correct to use the same timing for both flashes since
 * they can be swapped, although they may have different characteristics.
 */
	lw	t0, DEVICE_BOOT_BANK_PARAMETERS(s2)
	and	t2,t0, HTOLE32(GT_DEVPAR_DevWidthMASK)
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
//SET_LED(1);
	bal	superio_init
	nop
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



	TTYDBG("Setting up SDRAM controller\r\n");

 	/* Read the SPD info and get DRAM Configuration */	

	bal	doinit
	nop

doinit:
	li	v0, 0xb0000000		/* ra is set from previous bal */
	bgtu	v0, ra, in_ram		/* if pc is lower than rom space.. */
	nop
	b	2f
	nop

2:

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
//SET_LED(2);

	bal	boot_i2c_init
	nop
//SET_LED(3);


#include "sdram_init.S"

//#include "test/testmem6.S"

done_dens:
	TTYDBG("\r\n");

/*
 *  Clear out 2Mb of memory (maximum cache size)
 */

	TTYDBG("Clearing cache size memory...\r\n");

	la	t0, UNCACHED_MEMORY_ADDR
	addu	t1, t0, 1*1024*1024
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
/* godson2 */
	mfc0    a0, COP_0_PRID
	andi    a0, a0, 0xff00
	li      a1, 0x6300
	bne     a0,a1,1f
	nop
	TTYDBG("godson2 caches found\r\n")
    bal     godson2_cache_init
    nop
    bal     godson2_cache_flush
    nop
	b	cache_done
	nop
1:
/* non-godson */
	TTYDBG("Non-godson cache not supported!\n");
	b 1b;

cache_done:

#ifdef DEBUG_LOCORE
	TTYDBG("Init caches done, cfg = ")
	mfc0	a0, COP_0_CONFIG
	bal	hexserial
	nop
	TTYDBG("\r\n")
#endif

/*
 *  At this point all memory controller setup should have been done
 *  and we should be able to function 'normally' and C code can be
 *  used freely from this point.
 */
	TTYDBG("Copy PMON to execute location...")

#if 0
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
	TTYDBG("done.\r\n")

	sw	s8, CpuTertiaryCacheSize /* Set L3 cache size */

#if 1
    mfc0   a0,COP_0_CONFIG
    and    a0,a0,0xfffffff8
    or     a0,a0,0x3
    mtc0   a0,COP_0_CONFIG
#endif

//#include "test/test_after_copy.S"

	TTYDBG("memsize=");
	bal	hexserial
	move	a0,s1
	TTYDBG("\r\n");
#if 0
	TTYDBG("jumping to=");
	la	a0, initmips
	bal	hexserial
	nop
	TTYDBG("(initmips)=");
	la	a0, initmips
	lw  a0,0(a0)
	bal	hexserial
	nop
	TTYDBG("(initmips+4)=");
	la	a0, initmips
	lw  a0,4(a0)
	bal	hexserial
	nop

	TTYDBG("sp=");
	move a0, sp
	bal	hexserial
	nop
#endif
#if __mips >= 3
	TTYDBG("sp high=");
	dsrl a0, sp,32
	bal	hexserial
	nop
#endif

	move	a0,s1
	srl		a0,s1,20
#ifdef SKIPDIMM
	li	a0,	256
#endif
    /* pass pointer to kseg1 tgt_putchar */
    la  a1, tgt_putchar
	addu a1,a1,s0

    la  a2, stringserial
	addu a2,a2,s0

	la	v0, initmips
	jalr	v0
	nop

stuck:
#ifdef DEBUG_LOCORE
	TTYDBG("Dumping GT64420 setup.\r\n")
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
	DMTC0   a2, COP_0_TLB_PG_MASK   # Whatever...

1:
	DMTC0   zero, COP_0_TLB_HI	# Clear entry high.
	DMTC0   zero, COP_0_TLB_LO0	# Clear entry low0.
	DMTC0   zero, COP_0_TLB_LO1	# Clear entry low1.

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
	DMTC0   a2, COP_0_TLB_PG_MASK   # All pages are 16Mb.

1:
	and	a2, a0, PG_SVPN
	DMTC0   a2, COP_0_TLB_HI	# Set up entry high.

	move	a2, a0
	srl	a2, a0, PG_SHIFT 
	and	a2, a2, PG_FRAME
	ori	a2, PG_IOPAGE
	DMTC0   a2, COP_0_TLB_LO0	# Set up entry low0.
	addu	a2, (0x01000000 >> PG_SHIFT)
	DMTC0   a2, COP_0_TLB_LO1	# Set up entry low1.

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
	move	k0, ra
	addu	k1, a0, s0
	lbu	    a0, 0(k1)
1:
	beqz	a0, 2f
	nop
	//tgt_putchar must preserve k0/k1
	bal	tgt_putchar
	nop
	addiu	k1, 1
	b	1b
	lbu	a0, 0(k1)
2:
	j	k0
	nop
END(stringserial)

LEAF(hexserial)
	move	k0, ra
	move	k1, a0
	li	a3, 7
1:
	rol	a0, k1, 4
	move	k1, a0
	and	a0, 0xf
	la	v0, hexchar
	addu	v0, s0
	addu	v0, a0
	//tgt_putchar must preserve k0/k1,a3
	bal	tgt_putchar
	lbu	a0, 0(v0)

	bnez	a3, 1b
	addu	a3, -1

	j	k0
	nop
END(hexserial)
	



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

#ifdef USE_SUPERIO_UART
#include "ttyS_16550.S"
#else
#define __ASM__
	.set noreorder
#include "../dev/mpsc/mpsc.h"
#include "../dev/mpsc/ttyS_mpsc.S"
#endif

#define HIGH_SPEED_SERIAL

LEAF(superio_init)

	PCICONF_WRITEW(VTSB_DEV,0,4,7);
#if 0
	/*positive decode*/
	PCICONF_ORB(VTSB_DEV,0,0x81,0x80);
	PCICONF_WRITEB(VTSB_DEV,0,0x83,0x80|0x1| 0x8);
#endif
	PCICONF_WRITEB(VTSB_DEV,0,0x85,3);
	/* enable RTC/PS2/KBC */
	PCICONF_WRITEB(VTSB_DEV,0,0x5A,7);

SUPERIO_WR(0xe2,E2_S2|E2_S1|E2_EPP|E2_FLOPPY) /*enable serial and floppy */
SUPERIO_WR(0xe3,0x3f0>>2) /*floppy base address*/
SUPERIO_WR(0xe6,0x378>>2) /*parallel port*/
SUPERIO_WR(0xe7,0x3f8>>2) /*set serial port1 base addr 0x3f8*/
SUPERIO_WR(0xe8,0x2f8>>2) /*set serial port2 base addr 0x2f8*/
SUPERIO_WR(0xee,0xc0) /* both ports on high speed*/
#SUPERIO_WR(0xe2,E2_S2|E2_S1|E2_EPP|E2_FLOPPY) /*enable serial and floppy */

PCICONF_WRITEB(VTSB_DEV,0,0x85,1)
	jr ra
	nop
END(superio_init)

LEAF(test_superio)
move s1,ra
PCICONF_WRITEW(VTSB_DEV,0,0x4,0x7);
PCICONF_WRITEB(VTSB_DEV,0,0x85,3);
li t6,0
1:
move a0,t6
bal hexserial
nop
li a0,':';
bal tgt_putchar
nop
SUPERIO_RD(t6,t7)
move a0,t7
bal hexserial
nop
PRINTSTR("\r\n")
addu t6,1
sgt t0,t6,0x100
beqz t0,1b
nop
PCICONF_WRITEB(VTSB_DEV,0,0x85,1);
jr s1
nop
END(test_superio)


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
.set mips3
/*####part 2####*/
cache_detect_size_way:
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

	mfc0    t7, COP_0_PRID
	andi    t7, t7, 1
	srlv    t6, t6, t7      /* 4way */
	srlv    t5, t5, t7

/*####part 3####*/
	lui	a0, 0x8000
	addu	a1, $0, t5
	addu	a2, $0, t6
cache_init_dway:
/*
a0=0x80000000, a1=icache_size, a2=dcache_size
a3, v0 and v1 used as local registers
*/
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
    beqzl   t7, 1b
    addiu   v0, v0, 0x20
/* godson2c 4way */
    cache   Index_Store_Tag_D, 0x2(v0)
    cache   Index_Store_Tag_D, 0x3(v0)
/* godson2c 4way */
    beq $0, $0, 1b
    addiu   v0, v0, 0x20
1:
cache_flush_iway:
    addu    v0, $0, a0
    addu    v1, a0, a1
1:  slt a3, v0, v1
    beq a3, $0, 1f
    nop
    cache   Index_Invalidate_I, 0x0(v0)
    beq $0, $0, 1b
    addiu   v0, v0, 0x20
1:
cache_flush_dway:
    addu    v0, $0, a0
    addu    v1, a0, a2
1:  slt a3, v0, v1
    beq a3, $0, 1f
    nop
    cache   Index_Writeback_Inv_D, 0x0(v0)
    cache   Index_Writeback_Inv_D, 0x1(v0)
    beqzl   t7, 1b
    addiu   v0, v0, 0x20
/* godson2c 4way */
    cache   Index_Writeback_Inv_D, 0x2(v0)
    cache   Index_Writeback_Inv_D, 0x3(v0)
/* godson2c 4way */
    beq $0, $0, 1b
    addiu   v0, v0, 0x20
1:
cache_init_finish:
	nop
	jr	ra
	nop

cache_init_panic:
	TTYDBG("cache init panic\r\n");
1:	b	1b
	nop
.set mips0
	.end	godson2_cache_init

LEAF(godson2_cache_flush)
.set mips3
		move  t9,ra
        li    a0,0x80000000
        addu  a1,a0,16384
1:
        cache  1,0(a0)
        cache  1,1(a0)
        cache  1,2(a0)
        cache  1,3(a0)
        cache  0,(a0)
        add    a0,a0,32
        beq    a0,a1,1b
        nop
        TTYDBG("cache flushed");
		jr   t9
		.set mips0
END(godson2_cache_flush)

LEAF(init_regs)
	jr	ra
	nop
END(init_regs)

LEAF(nullfunction)
	jr ra
	nop
END(nullfunction)

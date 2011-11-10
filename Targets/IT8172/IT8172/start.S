/*	$Id: start.S,v 1.1.1.1 2006/09/14 01:59:09 root Exp $ */

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

#include "target/it8172.h"

#include "pmon/dev/ns16550.h"

#ifdef GODSONEV2A
#	define SKIPDIMM
#	define SDRAM_MODE_FIX
#else
#	undef SKIPDIMM
#	undef SDRAM_MODE_FIX
#endif


#define DEBUG_LOCORE
#ifdef DEBUG_LOCORE
#define	TTYDBG(x) \
	.rdata;98: .asciz x; .text; la a0, 98b; bal stringserial; nop
#else
#define TTYDBG(x)
#endif

#define	PRINTSTR(x) \
	.rdata;98: .asciz x; .text; la a0, 98b; bal stringserial; nop



/*
 *   Register usage:
 *
 *	s0	link versus load offset, used to relocate absolute adresses.
 *	s1	free
 *	s2	base address to ITE chip.
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
#if 0
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
 * gpio function define         
 */
#define GPIO_LOOP       10000

#define MK_LAT(v)\
        li              t0, (v);        \
9:      subu    t0, 1;          \
        bne             t0, 0, 9b;      \
        nop;

#if 1
#define FLASH_GPIO(a)   \
        li      a0, (a)         ;\
        bal     __flash_gpio    ;\
        nop

#else
#define FLASH_GPIO(a)   \
        li      t1, 0xb4013802;\
        li      t0, 0x550       ;\
        sh      t0, (t1)        ;\
        subu    t1, t1, 2;\
        li      t0, (a)         ;\
        sll     t0, 2           ;\
        sb      t0, (t1);       \
        MK_LAT(GPIO_LOOP);
#endif

/* 
 * LED      
 */    
#define FLASH_LED(a)    \
        li      a0, (a)         ;\
        bal     __led_display   ;\
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
	bal init_regs
	nop

	la	s0, start		/* RA set from BAL above! */
	subu	s0, ra, s0		/* s0 is now load vs. link offset */
	and	s0, 0xffff0000		/* Mask off lower bits */

#define CFG_IB		0x00000020
#define CFG_DB		0x00000010
#define	CFG_C_WBACK	2
#define CFG_BE		0x00008000
#define	FG_BE 		0x00008000
#define	CFG_EPMASK	0x0f000000
#define	CFG_EPD		0x00000000
#define	CFG_AD		0x00800000
/*
 * Initialize Configuration for board
 */
       	mfc0	   t1, COP_0_CONFIG                  # Get Config Reg

	and             t1, ~0x3f               # Set Config Reg bits 5..0 only
        or              t1, CFG_IB \
                          | CFG_DB \
                          | CFG_C_WBACK         # Set Cache Mode related bits in Config Reg
        and             t1, ~CFG_BE             # Clean Big Endian bit in Config Reg

        /* set DDDD rate for CPUs that aren't hardware configured */
        and             t1, ~CFG_EPMASK         # Set EP 4 bits to 0 in Config Reg
        or              t1, CFG_EPD             # Set EP DataPattern 0
        and             t1, ~CFG_AD                             # ???
        mtc0    t1, COP_0_CONFIG           # Re-Set Config Reg bits

        mfc0    t1, COP_0_STATUS_REG           
        mtc0    zero, COP_0_CAUSE_REG          
        and             t1, SR_SOFT_RESET       #
        or              t1, SR_BOOT_EXC_VEC      # Should be omited because it has been done before
        mtc0    t1, COP_0_STATUS_REG           # Set STATUS Reg
	li	t1, 0xa000038c
	mtc0	t1, COP_0_ERROR_PC
        nop


/*
 *  Init serial I/O for diagnostic output.
 */
        bal     initserial
        nop

        PRINTSTR("\r\nPMON2000 MIPS Initializing. Standby...\r\n")

        PRINTSTR("ERRORPC=")
        mfc0    a0, COP_0_ERROR_PC
        bal     hexserial
        nop

        FLASH_GPIO(0x04)                # GPD 2..4 => 0001
        PRINTSTR(" CONFIG=")
        mfc0    a0, COP_0_CONFIG
        bal     hexserial
        nop
        PRINTSTR("\r\n")

        la      s0, start               /* RA set from BAL above! */
        subu    s0, ra, s0              /* s0 is now load vs. link offset */
        and     s0, 0xffff0000          /* Mask off lower bits */


        TTYDBG("Setting up SDRAM controller\r\n");


/*
 *  Reset and initialize caches to a known state.
 */
#define IndexStoreTagI	0x08
#define IndexStoreTagD	0x09
#define IndexStoreTagS	0x0b
#define IndexStoreTagT	0x0a
#define FillI		0x14


#if 0
/*
 *  Clean out and initialize the TLB
 */
	bal	CPU_TLBClear
	nop

	li	a0, 0xc0000000
	li	a1, 0x40000000
	bal	CPU_TLBInit
	nop
#endif 


/* Initialize Cache */
#	bal godson2_cache_flush
#	nop
#	bal godson2_cache_init
#	nop

/* Memory test */
#	bal mem_test
#	nop
	li	v0, 0x12345678
	li	a1, 0xa2000000
	sw	v0, 0(a1)
	lw	a0, 0(a1)
	bal	hexserial
	nop

/*
 *  At this point all memory controller setup should have been done
 *  and we should be able to function 'normally' and C code can be
 *  used freely from this point.
 */
/* If use compression all the datas have been already loaded into memory */
#if 0
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

/* plj */
	bal	PC_REG
	nop
	/* copy text section */
	li	t0, 0
1:	lw	v0, 0(a1)
	nop
	sw	v0, 0(a0)
	move 	t1, a0
	lw	a0, 0(t1)
	bal	hexserial
	nop
	move	a0, t1
	addu	a0, 4
	bne a2, a0, 1b
	addu	a1, 4
	
	/* Clear BSS */
	la	a0, _edata
	la	a2, _end
	addu	a0, KSEG1
	addu	a2, KSEG1
	
2:	sw	zero, 0(a0)
	bne 	a2, a0, 2b
	addu	a0, 4
#endif

	TTYDBG("Copy PMON to execute location done.\r\n")


	la 	v0, initmips
	move	a0, v0
	bal	hexserial
	nop

	move 	v0, a0

	li	a0,0x04000000 /* 64Mb */
	nop
	la	v0, initmips
	nop
	jalr	v0
	nop

stuck:
#ifdef DEBUG_LOCORE
	TTYDBG("Dumping IT8172 setup.\r\n")
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
 * Common functions are defined below
 */

/*
 * flash_gpio entry 
 */

LEAF(__flash_gpio)
        li      t1, 0xb4013802
        li      t0, 0x550
        sh      t0, (t1)
        subu    t1, t1, 2
        move    t0, a0
        sll     t0, 2
        sb      t0, (t1)
        MK_LAT(GPIO_LOOP)
        j       ra
END(__flash_gpio)


/*
 * led display
 */
LEAF(__led_display)
        li              t0, 0xb80000f0
        sb              a0, (t0)
        li              t0,     2000
9:      subu    t0, 1
        bne             t0, 0, 9b
        nop
        j               ra
END(__led_display)




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

LEAF(tgt_putchar)
	la	v0, IT8172_PCI_IO_BASE + IT_UART_BASE
	addu	v0, UNCACHED_MEMORY_ADDR
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
	li	v0, KSEG1
	addu	v0, IT8172_PCI_IO_BASE
	addu	v0, IT_PM_DSR
	lh	v1, (v0)
	and	v1, ~0x0100
	sh	v1, (v0)

	la	v0, IT8172_PCI_IO_BASE + IT_UART_BASE
	addu	v0, UNCACHED_MEMORY_ADDR
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
	sb	v1, NSREG(NS16550_MCR)(v0)
	li	v1, 0x0
	sb	v1, NSREG(NS16550_IER)(v0)
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

#ifdef CONFIG_CACHE_64K_4WAY
	sll	t6,2
#endif

	sllv	t5, t4, t5

#ifdef CONFIG_CACHE_64K_4WAY
	sll	t5,2
#endif
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
/*#ifdef CONFIG_CACHE_64K_4WAY*/
	mtc0	$0, CP0_TAGLO
	cache   Index_Store_Tag_D, 0x2(v0)

	mtc0	$0, CP0_TAGLO
	cache   Index_Store_Tag_D, 0x3(v0)
/*#endif*/
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
/*#ifdef  CONFIG_CACHE_64K_4WAY*/
#	cache	Index_Invalidate_I, 0x2(v0)
#	cache	Index_Invalidate_I, 0x3(v0)
/*#endif*/
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

/*#ifdef CONFIG_CACHE_64K_4WAY*/
	cache	Index_Writeback_Inv_D, 0x2(v0)
	cache	Index_Writeback_Inv_D, 0x3(v0)
/*#endif*/
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

LEAF(invalidate_external_cache_page)
	.set push
	.set mips3
	li	t0, 0x90000010
	dsll32	t0, t0, 0
	dsubu	t0, t0, 8
	sd	a0, 0(t0)
	jr	ra
	nop	
	.set mips0
	.set pop
	.end invalidate_external_cache_page	

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


LEAF(init_regs)
/* all initial registers to zero */
	.set push
	.set mips3
	.set noat
	.set noreorder
	mfc0    t0, COP_0_PRID
	mfc0    v0, COP_0_STATUS_REG 
	mtc0    zero, COP_0_WATCH_LO
	mtc0    zero, COP_0_WATCH_HI

	.set reorder
	and             v0, SR_SOFT_RESET
	or              v0, SR_BOOT_EXC_VEC

	.set noreorder
	mtc0    v0, COP_0_STATUS_REG
	mtc0    zero, COP_0_CAUSE_REG

	jr	ra
	nop
	.set pop
END(init_regs)

LEAF(nullfunction)
	jr ra
	nop
END(nullfunction)

LEAF(PC_REG)
	TTYDBG("PC=")
	move t1, a0
	move a0, ra
	bal hexserial
	nop 
	TTYDBG("\n")
	move a0, t1
	jr ra
	nop
END(PC_REG)


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

#include "pmon/dev/ns16550.h"
#include "target/prid.h"
#include "target/sbd.h"
#include "target/ls232.h"
#include "target/via686b.h"
#include "target/i8254.h"
#include "target/isapnpreg.h"
#define DEBUG_LOCORE


#ifdef DEBUG_LOCORE
#define	TTYDBG(x) \
	.rdata;98: .asciz x; .text; la a0, 98b; bal stringserial; nop
#else
#define TTYDBG(x)
#endif

#define	PRINTSTR(x) \
	.rdata;98: .asciz x; .text; la a0, 98b; bal stringserial; nop

#ifdef DEVBD_LS232_SM502
#define GPIOLED_DIR    0xff
#define GPIO_OE_7_0    0x0
#define GPIO_OE_15_8   0x1
#define GPIO_OE_23_16  0x2
#define GPIO_OE_28_24  0x3
#define GPIO_OE	       GPIO_OE_7_0
#define GPIO_O_7_0    0x20
#define GPIO_O_15_8   0x21
#define GPIO_O_23_16  0x22
#define GPIO_O_28_24  0x2c
#define GPIO_O	       GPIO_O_7_0
#endif

#define GPIOLED_SET(x) \
li v0,0xbf004100; \
lb v1,0x0(v0); \
or v1,0xff; \
sb v1,0(v0); \
li v1,(((~x)<<3)&0x78);\
sb v1,0x20(v0);\
li v1,0x10000;\
78: \
subu v1,1;\
bnez v1,78b;\
nop;


#define tmpsize		s1
#define msize		s2
#define bonito		s4
#define dbg			s5
#define sdCfg		s6

#define CP0_CONFIG $16
#define CP0_TAGLO  $28
#define CP0_TAGHI  $29


/*
 *   Register usage:
 *
 *	s0	link versus load offset, used to relocate absolute adresses.
 *	s1	free
 *	s2	memory size.
 *	s3	free.
 *	s4	Bonito base address.
 *	s5	dbg.
 *	s6	sdCfg.
 *	s7	rasave.
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
	mtc0	zero, COP_0_CAUSE_REG
	li	t0, SR_BOOT_EXC_VEC	/* Exception to Boostrap Location */
	mtc0	t0, COP_0_STATUS_REG
	la	sp, stack
	la	gp, _gp

//	bal	uncached		/* Switch to uncached address space */
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
	.align	9
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

 /* */
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
	la	a0, v380_msg
	bal	stringserial
	nop
	b	exc_common
	
	.align	9			/* bfc00400 */
	la	a0, v400_msg
	bal	stringserial
	nop
	b	exc_common

	.align	7			/* bfc00480 */
	la	a0, v480_msg
	bal	stringserial
	nop
	b	exc_common

	
	.align	8			/* bfc00500 */
exc_common:
	PRINTSTR("\r\nCAUSE=")
	mfc0	a0, COP_0_CAUSE_REG
	bal	hexserial
	nop
	PRINTSTR("\r\nSTATUS=")
	mfc0	a0, COP_0_STATUS_REG
	bal	hexserial
	nop
	PRINTSTR("\r\nERRORPC=")
	mfc0	a0, COP_0_ERROR_PC
	bal	hexserial
	nop
	PRINTSTR("\r\nEPC=")
	mfc0	a0, COP_0_EXC_PC
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

	b	ext_map_and_reboot
	nop

	.align 9
		nop
	.align 8
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


/*
 *  We get here from executing a bal to get the PC value of the current execute
 *  location into ra. Check to see if we run from ROM or if this is ramloaded.
 */
locate:
	lb	t0,0xbf004140
	and	t0,0xbf
	sb	t0,0xbf004140
	GPIOLED_SET(0)	

	la	s0,uncached
	subu	s0,ra,s0
	and	s0,0xffffff00

	GPIOLED_SET(1)	
//	bal	CPU_TLBClear
//	nop
	li	t0,SR_BOOT_EXC_VEC
	mtc0	t0,COP_0_STATUS_REG
        mtc0    zero,COP_0_CAUSE_REG
	.set noreorder

	GPIOLED_SET(2)	
	bal	initserial
	nop
	
	GPIOLED_SET(4)	
	
	PRINTSTR("\r\nPMON2000 MIPS Initializing. Standby...\r\n")
	PRINTSTR("ERRORPC=")
	GPIOLED_SET(5)	
	mfc0	a0, COP_0_ERROR_PC
	bal	hexserial
	nop
	GPIOLED_SET(6)	

	PRINTSTR(" CONFIG=")
	mfc0	a0, COP_0_CONFIG
	bal	hexserial
	nop
	PRINTSTR("\r\n")
	
	PRINTSTR(" PRID=")
	mfc0	a0, COP_0_PRID
	bal	hexserial
	nop
	PRINTSTR("\r\n")

	PRINTSTR(" s0=")
	move	a0, s0
	bal	hexserial
	nop
	PRINTSTR("\r\n")

	GPIOLED_SET(7)	


	/* 
	 * Now determine DRAM configuration and size by
	 * reading the I2C EEROM on the DIMMS
	 */

#if 1
################################
#   Initailize SDRAM Interface #
################################
        li      a2, 0xbf000000
        #li      a1, 0x0c4007b8
        li	a1, 0x0c4002d4
#zgj-20090205        sw      a1, 0x0(a2)
        sw      a1, 0x0(a2) ; #zgj-20090227
       # li      a1, 0x209800a7
        li      a1, 0x209800cd  #for 256MB sdram 4 x 8k x 2k x 8 b  ##zgj-20090713 for ls232 Demo#
        #zgj-20090713  li      a1, 0x209800ca  #for 128MB sdram 4 x 8k x 1k x 16 b
	#zgj        li      a1, 0x209800c6 #for 64MB sdram 4 x 8k x 512 x 16 b
        #li      a1, 0x209000c6
        #li      a1, 0x208800c6
#zgj-20090205        sw      a1, 0x4(a2) ; #zgj-20090227
        sw      a1, 0x4(a2) ; #zgj-20090227
	GPIOLED_SET(11)	

	#li  msize,0x02000000     
	#zgj li  msize,0x04000000 #for 64MB
 	#zgj-20090713 li  msize,0x08000000  #for 128MB
 	li  msize,0x10000000  #for 256MB
	PRINTSTR("Init SDRAM Done!\r\n");


#define CF_7_SE         (1 << 3)        /* Secondary cache enable */
#define CF_7_SC         (1 << 31)       /* Secondary cache not present */
#define CF_7_TE         (1 << 12)       /* Tertiary cache enable */
#define CF_7_TC         (1 << 17)       /* Tertiary cache not present */
#define CF_7_TS         (3 << 20)       /* Tertiary cache size */
#define CF_7_TS_AL      20              /* Shift to align */
#define NOP8 nop;nop;nop;nop;nop;nop;nop;nop
do_caches:
	TTYDBG("Init caches...\r\n")

	li	s7, 0                   /* no L2 cache */
	li	s8, 0                   /* no L3 cache */

	GPIOLED_SET(12)	
	TTYDBG("LS232 caches found\r\n")
    bal     cache_init
    nop
#endif

	GPIOLED_SET(13)	
	TTYDBG("Init caches done, cfg = ")
	mfc0	a0, COP_0_CONFIG
	bal	hexserial
	nop
	TTYDBG("\r\n\r\n")


	GPIOLED_SET(14)	


#include "newtest.32/mydebug.S"
bootnow:

	bnez s0,1f
	nop
	b copydone
	nop
1:

#define	FCALL_PRINTSTR(x) \
	.rdata;98: .asciz x; .text; la a0, 98b; la v0, stringserial; addu v0,s0;jalr v0; nop	

#undef BAL
#define BAL(x) \
	la v0,x; \
	addu v0,s0; \
	jalr v0; \
	nop;

        mfc0   a0,COP_0_CONFIG
        and    a0,a0,0xfffffff8
        or     a0,a0,0x2
        mtc0   a0,COP_0_CONFIG

/*copy copy program to sdram to make copy fast*/

	la t0,121f
	addu t0,s0
	la t1,122f
	addu t1,s0
	
	li t2,0x80000000
1:
	lw  v0,(t0)
	sw v0,(t2)
	addu t0,4
	addu t2,4
	ble t0,t1,1b
	nop

	li t0,0x80000000
   	jr t0	
	nop		
.align 3
121: 
        mfc0   a0,COP_0_CONFIG
        and    a0,a0,0xfffffff8
        or     a0,a0,0x3
        mtc0   a0,COP_0_CONFIG
	FCALL_PRINTSTR("Copy PMON to execute location...\r\n")
	la	a0, start
	addu a1,a0,s0
	la	a2, _edata
	subu	t1, a2, a0
	srl	t1, t1, 2

	move	t0, a0
	move	t1, a1
	move	t2, a2

	/* copy text section */
	
1:	and	t3,t0,0x0000ffff
	bnez	t3,2f
	nop
	move	a0,t0
	BAL(hexserial)
	nop
	li	a0,'\r'
	BAL(tgt_putchar)
	nop
2:	lw	t3, 0(t1)
	nop
	sw	t3, 0(t0)
	addu	t0, 4
	addu	t1, 4
	bne	t2, t0, 1b
	nop

	FCALL_PRINTSTR("\ncopy text section done.\r\n")
	
	/* Clear BSS */
	la	a0, _edata
	la	a2, _end
2:	sw	zero, 0(a0)
	bne	a2, a0, 2b
	addu	a0, 4


	FCALL_PRINTSTR("Copy PMON to execute location done.\r\n")
	FCALL_PRINTSTR("sp=");
	move a0, sp
	BAL(hexserial)
	nop

	.set mips3
#ifndef WAYBIT
#define WAYBIT 12
#endif
#define WAY__(x) x
#define WAY_(x,y) WAY__((x<<y))
#define WAY(x) WAY_(x,WAYBIT)
        li    $5,0x80000000;
        addu  $6,$5,16384;
1:
        cache  1, WAY(0)($5);
        cache  1, WAY(1)($5);
        cache  1, WAY(2)($5);
        cache  1, WAY(3)($5);
        cache  0, WAY(0)($5);
        cache  0, WAY(1)($5);
        cache  0, WAY(2)($5);
        cache  0, WAY(3)($5);
        add    $5,$5,32;
        bne    $5,$6,1b;
        nop;
	.set mips0

copydone:

	li	a0, 0
	sw	a0, CpuTertiaryCacheSize /* Set L3 cache size */


	move	a0,msize
	srl	a0,20


	la	v0, initmips
	jalr	v0
	nop

.align 3
122:
stuck:
	b	stuck
	nop

/*
 *  Clear the TLB. Normally called from start.S.
 */
#if __mips64
#define MTC0 dmtc0
#else 
#define MTC0 mtc0
#endif
LEAF(CPU_TLBClear)
	li	a3, 0			# First TLB index.

	li	a2, PG_SIZE_4K
	MTC0   a2, COP_0_TLB_PG_MASK   # Whatever...

1:
	MTC0   zero, COP_0_TLB_HI	# Clear entry high.
	MTC0   zero, COP_0_TLB_LO0	# Clear entry low0.
	MTC0   zero, COP_0_TLB_LO1	# Clear entry low1.

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
	MTC0   a2, COP_0_TLB_PG_MASK   # All pages are 16Mb.

1:
	and	a2, a0, PG_SVPN
	MTC0   a2, COP_0_TLB_HI	# Set up entry high.

	move	a2, a0
	srl	a2, a0, PG_SHIFT 
	and	a2, a2, PG_FRAME
	ori	a2, PG_IOPAGE
	MTC0   a2, COP_0_TLB_LO0	# Set up entry low0.
	addu	a2, (0x01000000 >> PG_SHIFT)
	MTC0   a2, COP_0_TLB_LO1	# Set up entry low1.

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

LEAF(outstring)
	move	a2, ra
	move	a1, a0
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
END(outstring)

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

LEAF(tgt_testchar)
	la	v0, COM1_BASE_ADDR
1:
	lbu	v1, NSREG(NS16550_LSR)(v0)
	and	v0,v1, LSR_RXRDY
	jr ra
	nop
END(tgt_testchar)

LEAF(tgt_getchar)
	la	v0, COM1_BASE_ADDR
1:
	lbu	v1, NSREG(NS16550_LSR)(v0)
	and	v1, LSR_RXRDY
	beqz	v1, 1b
	nop
	lb	v0, NSREG(NS16550_DATA)(v0)
	jr ra
	nop
END(tgt_getchar)


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
// 	li	v1, NS16550HZ/(16*CONS_BAUD)
#zgj-20090713-FPGA 	li	v1, (NS16550HZ*4)/(16*CONS_BAUD) 
 	li	v1, NS16550HZ/(16*CONS_BAUD)  #zgj-20090713
// 	li	v1, 16666666/(16*57600)
// 	li	v1, 25000000/(16*57600)
	sb	v1, NSREG(NS16550_DATA)(v0)
	srl	v1, 8
	sb	v1, NSREG(NS16550_IER)(v0)
	li	v1, CFCR_8BITS
	sb	v1, NSREG(NS16550_CFCR)(v0)
	li	v1, MCR_DTR|MCR_RTS
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
v480_msg:
	.asciz	"\r\nPANIC! You have been in the Ejtag Debug MOde Trap is 0!\r\n"
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
	bnz	vo, 99b;\
	addiu	v0, -1



#define Index_Store_Tag_D			0x09
#define Index_Invalidate_I			0x00
#define Index_Writeback_Inv_D			0x01

LEAF(nullfunction)
	jr ra
	nop
END(nullfunction)

	.ent	cache_init
    .global cache_init
    .set noreorder
cache_init:
        move t1,ra
####part 2####
cache_detect_4way:
    .set mips32
        mfc0    t4, CP0_CONFIG,1
	lui	v0, 0x7
        and    v0, t4, v0    
	srl	t3, v0, 16    #ic

	li	t5,0x800 #32*64
	srl	v1,t4,22
	andi	v1,7
	sll	t5,v1   #InstCacheSetSize
	sll	t5,t3	#t6 InstCacheSize


        andi    v0, t4, 0x0380
        srl     t7, v0, 7     #dc

	li	t6,0x800 #32*64
	srl	v1,t4,13
	andi	v1,7
	sll	t6,v1  #DataCacheSetSize
	sll	t6,t7  #t5 DataCacheSize

####part 3####
#	.set	mips3
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
	cache	Index_Store_Tag_D, 0x0(v0)	# 1 way
4:	beq	$0, $0, 1b
	addiu	v0, v0, 0x20
1:
cache_flush_i2way:
	addu	v0, $0, a0
	addu	v1, a0, a1
1:	slt	a3, v0, v1
	beq	a3, $0, 1f
	nop
	cache	Index_Invalidate_I, 0x0(v0)	# 1 way
4:	beq	$0, $0, 1b
	addiu	v0, v0, 0x20
1:
cache_flush_d2way:
	addu	v0, $0, a0
	addu	v1, a0, a2
1:	slt	a3, v0, v1
	beq	a3, $0, 1f
	nop
	cache	Index_Writeback_Inv_D, 0x0(v0) 	#1 way
4:	beq	$0, $0, 1b
	addiu	v0, v0, 0x20
	.set	mips0

1:
cache_init_finish:
    jr	t1
    nop
    .set reorder
	.end 	cache_init

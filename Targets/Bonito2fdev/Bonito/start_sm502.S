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
#include "target/bonito.h"
#include "target/via686b.h"
#include "target/i8254.h"
#include "target/isapnpreg.h"
#define DEBUG_LOCORE
#define PCI_IDSEL_SM502 14
#define MMIO		0x06000000
#define MMIO_BASE  	0xb6000000


#ifdef DEBUG_LOCORE
#define	TTYDBG(x) \
	.rdata;98: .asciz x; .text; la a0, 98b; bal stringserial; nop
#else
#define TTYDBG(x)
#endif

#define	PRINTSTR(x) \
	.rdata;98: .asciz x; .text; la a0, 98b; bal stringserial; nop

#ifdef DEVBD2F_SM502
#define GPIOLED_DIR  0xe
#else
#define GPIOLED_DIR  0xf
#endif

#define GPIOLED_SET(x) \
li v0,0xbfe0011c; \
lw v1,4(v0); \
or v1,0xf; \
xor v1,GPIOLED_DIR; \
sw v1,4(v0); \
li v1,(~x)&0xf;\
sw v1,0(v0);\
li v1,0x1000;\
78: \
subu v1,1;\
bnez v1,78b;\
nop;

#define CONFIG_CACHE_64K_4WAY 1

#define tmpsize		s1
#define msize		s2
#define bonito		s4
#define dbg			s5
#define sdCfg		s6

#define CP0_CONFIG $16
#define CP0_TAGLO  $28
#define CP0_TAGHI  $29

#define	DDR100 0x1d441091 
/* 
#define	DDR100 0x0c011091*/
#define	DDR266 0x0410435e
#define DDR300 0x041453df

#define DDR_DQ 0xbff00030             /*  zgj  */

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
	GPIOLED_SET(1)

//	bal	uncached		/* Switch to uncached address space */
	nop
	GPIOLED_SET(2)

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
	la	a0, v380_msg
	bal	stringserial
	nop
	b	exc_common
	
	.align	8			/* bfc00400 */
	la	a0, v400_msg
	bal	stringserial
	nop


	
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

//	b	ext_map_and_reboot
	nop

	.align 8
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
	la	s0,start
	subu	s0,ra,s0
	and	s0,0xffff0000
	GPIOLED_SET(3)

	li	t0,SR_BOOT_EXC_VEC
	mtc0	t0,COP_0_STATUS_REG
        mtc0    zero,COP_0_CAUSE_REG
	.set noreorder

 	li	bonito,PHYS_TO_UNCACHED(BONITO_REG_BASE)

#define MOD_MASK	0x00000003
#define MOD_B		0x00000000 /* byte "modifier" */
#define MOD_H		0x00000001 /* halfword "modifier" */
#define MOD_W		0x00000002 /* word "modifier" */
#if __mips64
#	define MOD_D		0x00000003 /* doubleword "modifier" */
#endif

#define OP_MASK		0x000000fc
#define	OP_EXIT		0x00000000 /* exit (status) */
#define OP_DELAY	0x00000008 /* delay (cycles) */
#define OP_RD		0x00000010 /* read (addr) */
#define OP_WR		0x00000014 /* write (addr, val) */
#define OP_RMW		0x00000018 /* read-modify-write (addr, and, or) */
#define OP_WAIT		0x00000020 /* wait (addr, mask, value) */

#define WR_INIT(mod,addr,val) \
	.word	OP_WR|mod,PHYS_TO_UNCACHED(addr);\
	.word	(val),0
	
#define RD_INIT(mod,addr) \
	.word	OP_RD|mod,PHYS_TO_UNCACHED(addr);\
	.word	0,0
	
#define RMW_INIT(mod,addr,and,or) \
	.word	OP_RMW|mod,PHYS_TO_UNCACHED(addr);\
	.word	(and),(or)
	
#define WAIT_INIT(mod,addr,and,or) \
	.word	OP_WAIT|mod,PHYS_TO_UNCACHED(addr);\
	.word	(mask),(val)

#define DELAY_INIT(cycles) \
	.word	OP_DELAY,(cycles);\
	.word	0,0
	
#define EXIT_INIT(status) \
	.word	OP_EXIT,(status);\
	.word	0,0

#define BONITO_INIT(r,v) WR_INIT(MOD_W,BONITO_BASE+/**/r,v)
#define BONITO_BIS(r,b) RMW_INIT(MOD_W,BONITO_BASE+(r),~0,b)
#define BONITO_BIC(r,b) RMW_INIT(MOD_W,BONITO_BASE+(r),~(b),0)
#define BONITO_RMW(r,c,s) RMW_INIT(MOD_W,BONITO_BASE+(r),~(c),s)
	
#define CFGADDR(idsel,function,reg) ((1<<(11+(idsel)))+((function)<<8)+(reg))
#define _ISABWR_INIT(mod,function,isabreg,val) \
	WR_INIT(MOD_W,BONITO_BASE+BONITO_PCIMAP_CFG,CFGADDR(PCI_IDSEL_VIA686B,function,isabreg)>>16) ; \
        RD_INIT(MOD_W,BONITO_BASE+BONITO_PCIMAP_CFG) ; \
	WR_INIT(mod,PCI_CFG_SPACE+(CFGADDR(PCI_IDSEL_VIA686B,function,isabreg)&0xffff),val)
	
#define _ISABRD_INIT(mod,function,isabreg) \
	WR_INIT(MOD_W,BONITO_BASE+BONITO_PCIMAP_CFG,CFGADDR(PCI_IDSEL_VIA686B,function,isabreg)>>16) ; \
        RD_INIT(MOD_W,BONITO_BASE+BONITO_PCIMAP_CFG) ; \
	RD_INIT(mod,PCI_CFG_SPACE+(CFGADDR(PCI_IDSEL_VIA686B,function,isabreg)&0xffff))


#define _ISAWR_INIT(isareg,val) \
	WR_INIT(MOD_B,PCI_IO_SPACE+(isareg),val)
	
#define _ISARD_INIT(isareg) \
	RD_INIT(MOD_B,PCI_IO_SPACE+(isareg))
	

#define ISABBWR_INIT(function,isabreg,val) \
	_ISABWR_INIT(MOD_B,function,(isabreg),val)
#define ISABHWR_INIT(function,isabreg,val) \
	_ISABWR_INIT(MOD_H,function,(isabreg),val)
#define ISABWWR_INIT(function,isabreg,val) \
	_ISABWR_INIT(MOD_W,function,isabreg,val)
#define ISAWR_INIT(isareg,val) \
	_ISAWR_INIT(isareg,val)
#define ISARD_INIT(isareg) \
	_ISARD_INIT(isareg)
	GPIOLED_SET(4)

	bal	1f	
	nop

	/* bonito endianess */
	BONITO_BIC(BONITO_BONPONCFG,BONITO_BONPONCFG_CPUBIGEND)
	BONITO_BIC(BONITO_BONGENCFG,BONITO_BONGENCFG_BYTESWAP|BONITO_BONGENCFG_MSTRBYTESWAP)
	BONITO_BIS(BONITO_BONPONCFG, BONITO_BONPONCFG_IS_ARBITER)
	
        /*
         * In certain situations it is possible for the Bonito ASIC
         * to come up with the PCI registers uninitialised, so do them here
         */
#define PCI_CLASS_BRIDGE		0x06
#define PCI_CLASS_SHIFT			24
#define PCI_SUBCLASS_BRIDGE_HOST	0x00
#define PCI_SUBCLASS_SHIFT		16
#define PCI_COMMAND_IO_ENABLE		0x00000001
#define PCI_COMMAND_MEM_ENABLE		0x00000002
#define PCI_COMMAND_MASTER_ENABLE	0x00000004
#define PCI_COMMAND_STATUS_REG		0x04
#define PCI_MAP_IO			0X00000001
#define PCI_CFG_SPACE			BONITO_PCICFG_BASE

        BONITO_INIT(BONITO_PCICLASS,(PCI_CLASS_BRIDGE << PCI_CLASS_SHIFT) | (PCI_SUBCLASS_BRIDGE_HOST << PCI_SUBCLASS_SHIFT))
	BONITO_INIT(BONITO_PCICMD, BONITO_PCICMD_PERR_CLR|BONITO_PCICMD_SERR_CLR|BONITO_PCICMD_MABORT_CLR|BONITO_PCICMD_MTABORT_CLR|BONITO_PCICMD_TABORT_CLR|BONITO_PCICMD_MPERR_CLR)
	//BONITO_INIT(BONITO_PCILTIMER, 0)
	BONITO_INIT(BONITO_PCILTIMER, 255)
	BONITO_INIT(BONITO_PCIBASE0, 0)
	BONITO_INIT(BONITO_PCIBASE1, 0)
	BONITO_INIT(BONITO_PCIBASE2, 0)
	BONITO_INIT(BONITO_PCIEXPRBASE, 0)
	BONITO_INIT(BONITO_PCIINT, 0)

        BONITO_INIT(0x150,0x8000000c)
	BONITO_INIT(0x154,0xffffffff)

//////////////

	BONITO_INIT(0x10,0x80000000)
	BONITO_INIT(0x18,0x00800000)
	BONITO_INIT(0x1c,0x0)
	BONITO_INIT(0x14,0x0)
	BONITO_INIT(0x150,0x8000000c)
	BONITO_INIT(0x154,0xffffffff)
	BONITO_INIT(0x158,0xff80000c)
	BONITO_INIT(0x15c,0xffffffff)
	BONITO_INIT(0x110,0x2040)
	


///////////////
	BONITO_BIS(BONITO_PCICMD, BONITO_PCICMD_PERRRESPEN)
	
	BONITO_BIS(BONITO_PCICMD, PCI_COMMAND_IO_ENABLE|PCI_COMMAND_MEM_ENABLE|PCI_COMMAND_MASTER_ENABLE)
	
    BONITO_BIC(BONITO_BONGENCFG, 0x80)  #陆没?鹿iobc

	#BONITO_BIS(BONITO_BONGENCFG, BONITO_BONGENCFG_BUSERREN)

	/* Set debug mode */
	BONITO_BIS(BONITO_BONGENCFG, BONITO_BONGENCFG_DEBUGMODE)

    /******** added to init southbridge*/
	
#if  (PCI_IDSEL_VIA686B != 0)
	/* Set the SMB base address */
	ISABWWR_INIT(4, SMBUS_IO_BASE_ADDR, SMBUS_IO_BASE_VALUE | 0x1)
	/* enable the host controller */
	ISABHWR_INIT(4, SMBUS_HOST_CONFIG_ADDR, SMBUS_HOST_CONFIG_ENABLE_BIT)
	/* enable the SMB IO ports */
	ISABBWR_INIT(4, PCI_COMMAND_STATUS_REG, PCI_COMMAND_IO_ENABLE)
	
/* 15us ISA bus refresh clock */
#define ISAREFRESH (PT_CRYSTAL/(1000000/15))
	ISARD_INIT(CTC_PORT+PT_CONTROL)
	
	/* program i8254 ISA refresh counter */
	ISAWR_INIT(CTC_PORT+PT_CONTROL,PTCW_SC(PT_REFRESH)|PTCW_16B|PTCW_MODE(MODE_RG))
	ISAWR_INIT(CTC_PORT+PT_REFRESH, ISAREFRESH & 0xff)
	ISAWR_INIT(CTC_PORT+PT_REFRESH, ISAREFRESH >> 8)
#endif
                                                                        
	EXIT_INIT(0)


#define	Init_Op	0
#define	Init_A0	4
#define	Init_A1	8
#define	Init_A2	12
#define	Init_Size	16

1:	move a0,ra
reginit:			/* local name */
	lw	t3, Init_Op(a0)
	lw	t0, Init_A0(a0)
	and	t4,t3,OP_MASK
	
	
	/* 
	 * EXIT(STATUS) 
	 */
	bne	t4, OP_EXIT, 8f
	nop
	move	v0,t0
	b	.done
	nop
	
	/* 
	 * DELAY(CYCLES) 
	 */
8:	bne	t4, OP_DELAY, 8f
	nop
1:	bnez	t0,1b
	subu	t0,1
	b	.next
	nop	
	/* 
	 * READ(ADDR) 
	 */
8:	bne	t4,OP_RD,8f
	nop
	and	t4,t3,MOD_MASK
	
	bne	t4,MOD_B,1f
	nop
	lbu	t5,0(t0)
	b	.next
	nop
1:	bne	t4,MOD_H,1f
	nop
	lhu	t5,0(t0)
	b	.next
	nop
1:	bne	t4,MOD_W,1f
	nop
#if __mips64
	lwu	t5,0(t0)
#else 
	lw	t5,0(t0)
#endif
	b	.next
	nop
1:	
#if __mips64
	lw	t5,0(t0)
	b	.next
	nop
#else
	b	.fatal
	nop
#endif
	
	/* 
	 * WRITE(ADDR,VAL) 
	 */
8:	bne	t4,OP_WR,8f
	nop
	lw	t1,Init_A1(a0)
	and	t4,t3,MOD_MASK
	
	bne	t4,MOD_B,1f
	nop
	sb	t1,0(t0)
	b	.next
	nop
1:	bne	t4,MOD_H,1f
	nop
	sh	t1,0(t0)
	b	.next
	nop
1:	bne	t4,MOD_W,1f
	nop
	sw	t1,0(t0)
	b	.next
	nop
	
1:	
#if __mips64
	sd	t1,0(t0)
	b	.next
	nop
#else
	b	.fatal
	nop
#endif
		
	
	/* 
	 * RMW(ADDR,AND,OR) 
	 */
8:	bne	t4,OP_RMW,8f
	nop
	lw	t1,Init_A1(a0)
	lw	t2,Init_A2(a0)
	and	t4,t3,MOD_MASK
	
	bne	t4,MOD_B,1f
	nop
	lbu	t4,0(t0)
	and	t4,t1
	or	t4,t2
	sb	t4,0(t0)
	b	.next
	nop
1:	bne	t4,MOD_H,1f
	nop
	lhu	t4,0(t0)
	and	t4,t1
	or	t4,t2
	sh	t4,0(t0)
	b	.next
	nop
1:	bne	t4,MOD_W,1f
	nop
	lw	t4,0(t0)
	and	t4,t1
	or	t4,t2
	sw	t4,0(t0)
	b	.next
	nop
	
1:		
#if __mips64
	ld	t4,0(t0)
	and	t4,t1
	or	t4,t2
	sd	t4,0(t0)
	b	.next
	nop
#else	
	b	.fatal
	nop
#endif
		
	
	/* 
	 * WAIT(ADDR,MASK,VAL) 
	 */
8:	bne	t4,OP_WAIT,8f
	nop
	lw	t1,Init_A1(a0)
	lw	t2,Init_A2(a0)
	and	t4,t3,MOD_MASK
	
	bne	t4,MOD_B,1f
	nop
3:	lbu	t4,0(t0)
	and	t4,t1
	bne	t4,t2,3b
	nop
	b	.next
	nop
1:	bne	t4,MOD_H,1f
	nop
3:	lhu	t4,0(t0)
	and	t4,t1
	bne	t4,t2,3b
	nop
	b	.next
	nop
1:	bne	t4,MOD_W,1f
	nop
3:	lw	t4,0(t0)
	and	t4,t1
	bne	t4,t2,3b
	nop
	b	.next
	nop
1:		
#if __mips64
3:	ld	t4,0(t0)
	and	t4,t1
	bne	t4,t2,3b
	nop
	b	.next
	nop
#else	
	b	.fatal	
	nop
#endif
	
	
.next:	addu	a0,Init_Size
	b	reginit	
	nop	
	
8:
.fatal:	b .done
	nop
	bal 	stuck
	nop
.done:	

	GPIOLED_SET(5)
	bal 	superio_init
	nop
	
	GPIOLED_SET(6)
	bal	initserial
	nop
	GPIOLED_SET(7)
	
#ifdef TEST_SM502_UART0
1:
//	li	a0,0x55
	li	a0,0x11
	bal	tgt_putchar
	nop
	b	1b
	nop
		
#endif
	PRINTSTR("\r\nPMON2000 MIPS Initializing. Standby...\r\n")
	PRINTSTR("ERRORPC=")
	mfc0	a0, COP_0_ERROR_PC
	bal	hexserial
	nop
	GPIOLED_SET(8)
//*******************************
	
//	bal mydebug_main
  //  	nop	
	
	li	a1,(MMIO_BASE+0x40)
	lw	a0,0(a1)
	nop
	bal	hexserial
	nop
	PRINTSTR("\r\ns0 =\r\n")
	move	a0,s0
	bal	hexserial
	nop
//***********************************/

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

	bnez s0,1f
	nop
	li a0,128
	la v0,initmips
	jr v0
	nop
1:

gs_2f_v3_ddr2_cfg:
	GPIOLED_SET(9)

    ###enable the reg space###
#if 1
	TTYDBG	("\r\nenable register space of MEMORY\r\n")
        li  t2,0xbfe00180
        ld  a1,0x0(t2)
	and a1,a1,0x6ff 
        #or  a1,a1,0x100
        sd  a1,0x0(t2)
#endif    
	PRINTSTR("DDR2 config begin_whd\r\n")
	b	ddr2_config
	nop
ddr2_config_done:
	PRINTSTR("DDR2 config end\r\n")
	


#if 1 /* read ddr2 registers */
        li	t0, 0xaffffe00
        
not_locked:
        ld	t1, 0x10(t0)
        andi    t1, 0x01
        beqz    t1, not_locked
        nop

        PRINTSTR("DDR2 DLL locked\r\n")
        
        ld	t1, 0xf0(t0)
        move    a0, t1
        bal     hexserial
        nop

#endif
        
    ###disable the reg space###
#if 1
	TTYDBG	("\r\ndisable register space of MEMORY\r\n")
        li  t2,0xbfe00180
        lw  a1,0x0(t2)
        or  a1,a1,0x700 //disable ddr config space and no 写结束时是否允许数据未到内存 , no 写结束时是否允许数据未到内存
		xor a1,0x600
        sw  a1,0x0(t2)
#endif    


##########################################

	
skipdimm:
	GPIOLED_SET(10)
	PRINTSTR("Init SDRAM Done!\r\n");

		bal CPU_TLBClear
		nop



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

#ifdef CONFIG_CACHE_64K_4WAY 
        sll     s3,2
#endif

	and	t1, t3, 0x20
	srl	t1, t1, 1
	addu	s4, t1, 16		/* s4 = I cache line size */

	srl	t1, t3, 6
	and	t1, 3
	sllv	s5, t2, t1		/* s5 = D cache size */

#ifdef CONFIG_CACHE_64K_4WAY
        sll     s5,2
#endif

	and	t1, t3, 0x10
	addu	s6, t1, 16		/* s6 = D cache line size */
	TTYDBG("Init caches...\r\n")

	li	s7, 0                   /* no L2 cache */
	li	s8, 0                   /* no L3 cache */

#if 0
        mfc0    a0, COP_0_PRID
        li      a1, 0x6301
        bne     a0,a1,1f
        nop
#endif
	TTYDBG("godson2 caches found\r\n")
        bal     godson2_cache_init
        nop

	/* close L2 cache */
	li      a0, 0xbfe00164
        sw      zero, 0(a0);

        mfc0   a0,COP_0_CONFIG
        and    a0,a0,~((1<<12) | 3)
	or     a0,a0,2
        mtc0   a0,COP_0_CONFIG

#ifdef DEBUG_LOCORE
	TTYDBG("Init caches done, cfg = ")
	mfc0	a0, COP_0_CONFIG
	bal	hexserial
	nop
	TTYDBG("\r\n\r\n")
#endif

/* zhb */
#if 0
/*	li	t0, 0		*/
/*	li	t1, 0x90000	*/
/*1:	addiu	t0, 1		*/
/*	bne	t0, t1, 1b	*/
	nop

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
	not	t2
	subu	t2, 1
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
#ifdef USE_SM502_UART0
#define HAVE_TARGET_GETCHAR
#endif
#include "machine/newtest/mydebug.S"
bootnow:
	TTYDBG("Copy PMON to execute location...\r\n")
#ifdef DEBUG_LOCORE
	TTYDBG("  start = 0x")
	la	a0, start
	bal	hexserial
	nop
	TTYDBG("\r\n  s0 = 0x")
	move	a0, s0
	bal	hexserial
	nop
	TTYDBG("\r\n")
#endif
	la	a0, start
	li	a1, 0xbfc00000
	la	a2, _edata
        or      a0, 0xa0000000
        or      a2, 0xa0000000
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
	bal	hexserial
	nop
	li	a0,'\r'
	bal 	tgt_putchar
	nop
2:	lw	t3, 0(t1)
	nop
	sw	t3, 0(t0)
	addu	t0, 4
	addu	t1, 4
	bne	t2, t0, 1b
	nop

	PRINTSTR("\ncopy text section done.\r\n")
	
	/* Clear BSS */
	la	a0, _edata
	la	a2, _end
2:	sw	zero, 0(a0)
	bne	a2, a0, 2b
	addu	a0, 4


	TTYDBG("Copy PMON to execute location done.\r\n")
/* zhb */
#if 0
zhb:
	TTYDBG("Testing...\r\n")
	la	a0, start
	li	a1, 0xbfc00000
	la	a2, _edata
        or      a0, 0xa0000000
        or      a2, 0xa0000000
/*	subu	s6, a2, a0*/
/*	srl	s6, s6, 2*/

	move	t0, a0
	move	t1, a1
	move	t2, a2
	/* copy text section */
	
1:	lw	t4, 0(t1)
	nop
	lw	t5, 0(t0)
	addu	t0, 4
	addu	t1, 4
	beq	t4, t5, 2f
	nop
	move	a0, t0
	subu	a0, 4
	bal	hexserial
	nop
	TTYDBG	(" ")
	move	a0, t4
	bal	hexserial
	nop
	TTYDBG	(" ")
	move	a0, t5
	bal	hexserial
	nop
	TTYDBG	("\r\n")
2:	bne	t2, t0, 1b
	nop
	TTYDBG	("test ok!\r\n")
/*
3:	beqz	zero, 3b
	nop
*/
#endif

	

	TTYDBG("sp=");
	move a0, sp
	bal	hexserial
	nop

	li	a0, 4096*1024
	sw	a0, CpuTertiaryCacheSize /* Set L3 cache size */

#if 0
        mfc0   a0,COP_0_CONFIG
        and    a0,a0,0xfffffff8
        or     a0,a0,0x3
        mtc0   a0,COP_0_CONFIG
#endif


    /* pass pointer to kseg1 tgt_putchar */
    la  a1, tgt_putchar
	addu a1,a1,s0

    la  a2, stringserial
	addu a2,a2,s0

	srl	msize,20
	move	a0,msize
	
	la	v0, initmips
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
	add	a0, s3, bonito
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
	.set noat
	move AT,ra


#ifdef HAVE_NB_SERIAL
	la	v0, COM3_BASE_ADDR
	bal 1f
	nop
#endif
#ifdef USE_SM502_UART0
#define	NSREG1(x)    (x*4)
#define	SM502_ADDR  0xb6030000
	la	v0,SM502_ADDR
11:
	lbu	v1, NSREG1(NS16550_LSR)(v0)
	and	v1, LSR_TXRDY
	beqz	v1, 11b
	nop

	sb	a0, NSREG1(NS16550_DATA)(v0)
#endif
	jr AT
	nop
1:
	lbu	v1, NSREG(NS16550_LSR)(v0)
	and	v1, LSR_TXRDY
	beqz	v1, 1b
	nop

	sb	a0, NSREG(NS16550_DATA)(v0)
	j	ra
	nop	
	.set at
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
	.set noat
	move AT,ra

	la	v0, COM3_BASE_ADDR
 	li	a0, NS16550HZ/(16*CONS_BAUD)
	bal 1f
	nop
#ifdef USE_SM502_UART0
#define	SM502_ADDR  0xb6030000
#define	NSREG1(x)    (x*4)
#define	SM502_UART_FRE	96000000
	la	v0,SM502_ADDR
//	li	a0,SM502_UART_FRE/(12*CONS_BAUD)
//	li	a0,SM502_UART_FRE/(16*19450)
	li	a0,0x34
	li	v1, FIFO_ENABLE|FIFO_RCV_RST|FIFO_XMT_RST|FIFO_TRIGGER_4
	sb	v1, NSREG1(NS16550_FIFO)(v0)
	li	v1, CFCR_DLAB
	sb	v1, NSREG1(NS16550_CFCR)(v0)
	sb	a0, NSREG1(NS16550_DATA)(v0)
	srl	a0, 8
	sb	a0, NSREG1(NS16550_IER)(v0)
	li	v1, CFCR_8BITS
	sb	v1, NSREG1(NS16550_CFCR)(v0)
//	li	v1, MCR_DTR|MCR_RTS
	li	v1, 0x0
	sb	v1, NSREG1(NS16550_MCR)(v0)
	li	v1, 0x0
	sb	v1, NSREG1(NS16550_IER)(v0)
#endif
	jr AT
	nop
1:
	li	v1, FIFO_ENABLE|FIFO_RCV_RST|FIFO_XMT_RST|FIFO_TRIGGER_4
	sb	v1, NSREG(NS16550_FIFO)(v0)
	li	v1, CFCR_DLAB
	sb	v1, NSREG(NS16550_CFCR)(v0)
	sb	a0, NSREG(NS16550_DATA)(v0)
	srl	a0, 8
	sb	a0, NSREG(NS16550_IER)(v0)
	li	v1, CFCR_8BITS
	sb	v1, NSREG(NS16550_CFCR)(v0)
//	li	v1, MCR_DTR|MCR_RTS
	li	v1, 0x0
	sb	v1, NSREG(NS16550_MCR)(v0)
	li	v1, 0x0
	sb	v1, NSREG(NS16550_IER)(v0)
	j	ra
	nop
	.set at
END(initserial)


#include "i2c_sm502.S"
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
	bnz	vo, 99b;\
	addiu	v0, -1


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

#define Index_Store_Tag_D			0x05
#define Index_Invalidate_I			0x00
#define Index_Writeback_Inv_D			0x01
#define Index_Store_Tag_S			0x0b
#define Index_Writeback_Inv_S			0x03

LEAF(nullfunction)
	jr ra
	nop
END(nullfunction)

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
	srl	t6,1
	sllv	t5, t4, t5
	srl	t5,1
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
	mtc0	$0, CP0_TAGLO
	cache   Index_Store_Tag_D, 0x2(v0)
	mtc0	$0, CP0_TAGLO
	cache   Index_Store_Tag_D, 0x3(v0)
	beq	$0, $0, 1b
	addiu	v0, v0, 0x20

#if 1
1:
cache_init_l24way:
        mtc0    $0, CP0_TAGHI
        addu    v0, $0, a0
        addu    v1, a0, 128*1024
1:      slt     a3, v0, v1
        beq     a3, $0, 1f
        nop
        mtc0    $0, CP0_TAGLO
        cache   Index_Store_Tag_S, 0x0(v0)
        mtc0    $0, CP0_TAGLO
        cache   Index_Store_Tag_S, 0x1(v0)
        mtc0    $0, CP0_TAGLO
        cache   Index_Store_Tag_S, 0x2(v0)
        mtc0    $0, CP0_TAGLO
        cache   Index_Store_Tag_S, 0x3(v0)
        beq     $0, $0, 1b
        addiu   v0, v0, 0x20


1:
cache_flush_4way:
	addu	v0, $0, a0
	addu	v1, a0, 128*1024
1:	slt	a3, v0, v1
	beq	a3, $0, 1f
	nop
	cache	Index_Writeback_Inv_S, 0x0(v0)
	cache	Index_Writeback_Inv_S, 0x1(v0)
	cache	Index_Writeback_Inv_S, 0x2(v0)
	cache	Index_Writeback_Inv_S, 0x3(v0)
	beq	$0, $0, 1b
	addiu	v0, v0, 0x20
# endif

1:
cache_flush_i2way:
	addu	v0, $0, a0
	addu	v1, a0, a1
1:	slt	a3, v0, v1
	beq	a3, $0, 1f
	nop
	cache	Index_Invalidate_I, 0x0(v0)
#	cache	Index_Invalidate_I, 0x1(v0)
#	cache	Index_Invalidate_I, 0x2(v0)
#	cache	Index_Invalidate_I, 0x3(v0)
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
	cache	Index_Writeback_Inv_D, 0x2(v0)
	cache	Index_Writeback_Inv_D, 0x3(v0)
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

#define PCICONF_WRITEB(dev,func,reg,data) \
	li      a0,CFGADDR(dev,func,reg); \
	li      a1,PHYS_TO_UNCACHED(PCI_CFG_SPACE); \
        and     a2,a0,0xffff; \
	or      a1,a2; \
	srl     a0,16; \
	li      a2,BONITO_BASE+BONITO_PCIMAP_CFG; \
    	sw      a0,BONITO_PCIMAP_CFG(bonito); \
    	lw      zero,BONITO_PCIMAP_CFG(bonito); \
    	or	a0,zero,data; \
	sb      a0,(a1);

#define PCICONF_WRITEW(dev,func,reg,data) \
	li      a0,CFGADDR(dev,func,reg); \
	li      a1,PHYS_TO_UNCACHED(PCI_CFG_SPACE); \
        and     a2,a0,0xffff; \
	or      a1,a2; \
	srl     a0,16; \
	li      a2,BONITO_BASE+BONITO_PCIMAP_CFG; \
    	sw      a0,BONITO_PCIMAP_CFG(bonito); \
    	lw      zero,BONITO_PCIMAP_CFG(bonito); \
    	or	a0,zero,data; \
	sw      a0,(a1);
#define PCICONF_ORB(dev,func,reg,data) \
	li      a0,CFGADDR(dev,func,reg); \
        li      a1,PHYS_TO_UNCACHED(PCI_CFG_SPACE); \
        and     a2,a0,0xffff; \
        or      a1,a2; \
        srl     a0,16; \
        li      a2,BONITO_BASE+BONITO_PCIMAP_CFG; \
        sw      a0,BONITO_PCIMAP_CFG(bonito); \
        lw      zero,BONITO_PCIMAP_CFG(bonito); \
    	lbu	a2,(a1); \
    	ori	a2,data; \
        sw      a0,BONITO_PCIMAP_CFG(bonito); \
        lw      zero,BONITO_PCIMAP_CFG(bonito); \
    	sb	a2,(a1);
#define SUPERIO_WR(idx,data) \
        li 	v0,BONITO_PCIIO_BASE_VA+0x3f0; \
        or      v1,zero,idx; \
        sb      v1,(v0); \
        or 	v1,zero,data; \
        sb      v1,1(v0);

#define E2_EPP 2
#define E2_S1 (1<<2)
#define E2_S2 (1<<3)
#define E2_FLOPPY (1<<4)
    
LEAF(superio_init)

	
	li	a0,0x2040
	li	a1,0xbfe00110
	sw	a0,0(a1)
	nop
	PCICONF_WRITEW(PCI_IDSEL_SM502,0,4,0);
	PCICONF_WRITEW(PCI_IDSEL_SM502,0,0x14,MMIO);
	PCICONF_WRITEW(PCI_IDSEL_SM502,0,4,7);
//	PCICONF_WRITEW(PCI_IDSEL_SM502,0,0x40,0x2184f);
	li 	a0,0x218cf
	li	a1,(MMIO_BASE+0x40)
	sw	a0,0(a1)
	nop
	li 	a0,0x1e0
	li	a1,0xb600000c
	sw	a0,0(a1)
	nop
	
	jr ra
	nop
END(superio_init)

#######################################
#define REG_ADDRESS 0x0
#define CONFIG_BASE 0xaffffe00
    

        .global ddr2_config
        .set    noreorder
        .set    mips3
ddr2_config:
    la      t0, ddr2_reg_data
#ifdef __MYDEBUG__
	bal tgt_testchar
	nop
	beqz v0,1f
	nop
	bal tgt_getchar
	nop
	bal tgt_getchar
	nop
	li v1,'X'
	bne v0,v1,1f
	nop

	PRINTSTR("use backup data for ddr cfg\r\n");
    la      t0, ddr2_reg_data1
1:
#endif
    addu    t0, t0, s0
    li      t1, 0x1d
    li      t2, CONFIG_BASE

reg_write:
    ld      a1, 0x0(t0)
    sd      a1, REG_ADDRESS(t2)
    subu    t1, t1, 0x1
    addiu     t0, t0, 0x8
    bne     t1, $0, reg_write
    addiu     t2, t2, 0x10

	/* 
	 * Now determine DRAM configuration and size by
	 * reading the I2C EEROM on the DIMMS
	 */
i2c_test:
	li	k1,0
#include "i2ccfgddr.S"
	beqz msize,i2c_test
	nop
2:	
/////////////////

#ifdef __MYDEBUG__
	bal tgt_testchar
	nop
	beqz v0,2f
	nop
	bal tgt_getchar
	nop
	bal tgt_getchar
	nop
	li v1,'Y'
	bne v0,v1,2f
	nop
1:
	PRINTSTR("press x to begin debug,then Q to quit\r\n");
	nop
	bal tgt_getchar
	nop
	li v1,'x'
	bne v0,v1,1b
	nop

	bal mydebug_main
    nop	
2:
#endif
    
    ############start##########
	//PRINTSTR("start\n\r");
    li      t2, CONFIG_BASE
    la		t0,DDR2_CTL_start_DATA_LO
    #la		t0,ddr2_start_reg
    addu    	t0, t0, s0
    ld      a1, 0x0(t0)
    sd      a1, 0x30(t2)

	b ddr2_config_done
	nop

    jr      ra
    nop




	.text
	.align 5
	.global ddr2_reg_data;
ddr2_reg_data:
//0000000_0 arefresh 0000000_1 ap 0000000_1 addr_cmp_en 0000000_1 active_aging
DDR2_CTL_00_DATA_LO:	.word 0x00000101
// 0000000_1 ddrii_sdram_mode 0000000_1 concurrentap 0000000_1 bank_split_en 0000000_0 auto_refresh_mode 
DDR2_CTL_00_DATA_HI:	.word 0x01000100 #no_concurrentap
//DDR2_CTL_00_DATA_HI:	.word 0x01010100
//0000000_0 ecc_disable_w_uc_err 0000000_1 dqs_n_en 0000000_0 dll_bypass_mode 0000000_0 dlllockreg
//DDR2_CTL_01_DATA_LO:	.word 0x00010100 #dll_by_pass
DDR2_CTL_01_DATA_LO:	.word 0x00010000
//0000000_0 fwc 0000000_0 fast_write 0000000_0 enable_quick_srefresh 0000000_0 eight_bank_mode 
DDR2_CTL_01_DATA_HI:	.word 0x00010000
//0000000_0 no_cmd_init 0000000_0 intrptwritea 0000000_0 intrptreada 0000000_0 intrptapburst
DDR2_CTL_02_DATA_LO:	.word 0x00000000
//0000000_1 priority_en 0000000_0 power_down 0000000_1 placement_en 0000000_1 odt_add_turn_clk_en 
DDR2_CTL_02_DATA_HI:	.word 0x01000101
//0000000_1 rw_same_en 0000000_0 reg_dimm_enable 0000000_0 reduc 0000000_0 pwrup_srefresh_exit
DDR2_CTL_03_DATA_LO:	.word 0x01000000
//0000000_1 swap_port_rw_same_en 0000000_1 swap_en 0000000_0 start 0000000_0 srefresh 
DDR2_CTL_03_DATA_HI:	.word 0x01010000
//0000000_0 write_modereg 0000000_1 writeinterp 0000000_1 tref_enable 0000000_1 tras_lockout
DDR2_CTL_04_DATA_LO:	.word 0x00010101
/*
.单个CS的情况
控制器设成 75ohm (rtt_pad_termination)
DIMM设成   150 ohm (rtt_0), 并且写的odt_wr_map_cs0 设为 1
                            读的 odt_rd_map_cs0 设为 0
 
2.多个CS的情况
控制器设成 150ohm
DIMM 设成  75 ohm, 写的 odt_wr_map_cs0 设为 0x02, odt_wr_map_cs1 设为 0x01
                   读的 odt_rd_map_cs0 设为 0x02, odt_rd_map_cs1 设为 0x01
*/
#ifdef DEVBD2F_SM502
//000000_01 rtt_0 000000_00 ctrl_raw 000000_10 axi0_w_priority 000000_10 axi0_r_priority 
DDR2_CTL_04_DATA_HI:	.word 0x01000202
//00000_100 column_size 00000_101 caslat 00000_010 addr_pins 000000_10 rtt_pad_termination
DDR2_CTL_05_DATA_LO:	.word 0x04050202 #CL =4
#else
//000000_01 rtt_0 000000_00 ctrl_raw 000000_10 axi0_w_priority 000000_10 axi0_r_priority 
DDR2_CTL_04_DATA_HI:	.word 0x01000202
//00000_100 column_size 00000_101 caslat 00000_010 addr_pins 000000_10 rtt_pad_termination
DDR2_CTL_05_DATA_LO:	.word 0x04050102 #CL =4
#endif
//00000_000 q_fullness 00000_000 port_data_error_type 00000_000 out_of_range_type 00000_000 max_cs_reg 
DDR2_CTL_05_DATA_HI:	.word 0x00000000
//00000_010 trtp 00000_010 trrd 00000_010 temrs 00000_011 tcke
//DDR2_CTL_06_DATA_LO:	.word 0x01020203 #125 M
//DDR2_CTL_06_DATA_LO:	.word 0x02020203 #400
//DDR2_CTL_06_DATA_LO:	.word 0x02020203 #250M
DDR2_CTL_06_DATA_LO:	.word 0x03050203 #800
//0000_1010 aprebit 00000_100 wrlat 00000_010 twtr 00000_100 twr_int 
//DDR2_CTL_06_DATA_HI:	.word 0x0a040203 #125 M
//DDR2_CTL_06_DATA_HI:	.word 0x0a040204 #400
//DDR2_CTL_06_DATA_HI:	.word 0x0a030203 #250M CL=4
//DDR2_CTL_06_DATA_HI:	.word 0x0a020203 #CL=3
DDR2_CTL_06_DATA_HI:	.word 0x0a040306 #800
//0000_0000 ecc_c_id 0000_1111 cs_map 0000_0111 caslat_lin_gate 0000_1010 caslat_lin
//DDR2_CTL_07_DATA_LO:	.word 0x000f0808 #CL=4//cs_map to cs0-cs3
//DDR2_CTL_07_DATA_LO:	.word 0x000f0606 #CL=3///cs_map to cs0-cs3
#ifdef DEVBD2F_SM502
DDR2_CTL_07_DATA_LO:	.word 0x00030a0b #CL=5//cs_map to cs0-cs3
#else
DDR2_CTL_07_DATA_LO:	.word 0x000f0a0b #CL=5//cs_map to cs0-cs3
#endif
//DDR2_CTL_07_DATA_LO:	.word 0x000c0708 #CL=4//cs_map to cs2-cs3
//0000_0000 max_row_reg 0000_0000 max_col_reg 0000_0010 initaref 0000_0000 ecc_u_id 
//DDR2_CTL_07_DATA_HI:	.word 0x00000200
DDR2_CTL_07_DATA_HI:	.word 0x00000400 #800
#ifdef DEVBD2F_SM502
//0000_0001 odt_rd_map_cs3 0000_0010 odt_rd_map_cs2 0000_0100 odt_rd_map_cs1 0000_1000 odt_rd_map_cs0
DDR2_CTL_08_DATA_LO:	.word 0x00000102
//0000_0001 odt_wr_map_cs3 0000_0010 odt_wr_map_cs2 0000_0100 odt_wr_map_cs1 0000_1000 odt_wr_map_cs0 
DDR2_CTL_08_DATA_HI:	.word 0x00000000
#else
//0000_0001 odt_rd_map_cs3 0000_0010 odt_rd_map_cs2 0000_0100 odt_rd_map_cs1 0000_1000 odt_rd_map_cs0
DDR2_CTL_08_DATA_LO:	.word 0x0102040f
//0000_0001 odt_wr_map_cs3 0000_0010 odt_wr_map_cs2 0000_0100 odt_wr_map_cs1 0000_1000 odt_wr_map_cs0 
DDR2_CTL_08_DATA_HI:	.word 0x01020400
#endif

//0000_0000 port_data_error_id 0000_0000 port_cmd_error_type 0000_0000 port_cmd_error_id 0000_0000 out_of_range_source_id
DDR2_CTL_09_DATA_LO:	.word 0x00000000
//000_00000 ocd_adjust_pup_cs_0 000_00000 ocd_adjust_pdn_cs_0 0000_0100 trp 0000_1000 tdal 
//DDR2_CTL_09_DATA_HI:	.word 0x00000204 #125 M
//DDR2_CTL_09_DATA_HI:	.word 0x00000408 #400
//DDR2_CTL_09_DATA_HI:	.word 0x0000030c #250M
DDR2_CTL_09_DATA_HI:	.word 0x0000060c #800
//00_111111 age_count 000_01111 trc 000_00010 tmrd 000_00000 tfaw
//DDR2_CTL_10_DATA_LO:	.word 0x3f070200 #125 M
//DDR2_CTL_10_DATA_LO:	.word 0x3f0f0200 #400
DDR2_CTL_10_DATA_LO:	.word 0x3f1a021b #250M
//DDR2_CTL_10_DATA_LO:	.word 0x3f1f0200 #800
//0_0011101 dll_dqs_delay_2 0_0011101 dll_dqs_delay_1 0_0011101 dll_dqs_delay_0 00_111111 command_age_count 
DDR2_CTL_10_DATA_HI:	.word 0x1515153f
//0_0011101 dll_dqs_delay_6 0_0011101 dll_dqs_delay_5 0_0011101 dll_dqs_delay_4 0_0011101 dll_dqs_delay_3
DDR2_CTL_11_DATA_LO:	.word 0x15151515
//0_1011111 wr_dqs_shift 0_1111111 dqs_out_shift 0_0011101 dll_dqs_delay_8 0_0011101 dll_dqs_delay_7 
DDR2_CTL_11_DATA_HI:	.word 0x5f7f1515
//00001011 tras_min 00000000 out_of_range_length 00000000 ecc_u_synd 00000000 ecc_c_synd
//DDR2_CTL_12_DATA_LO:	.word 0x05000000 #125 M
//DDR2_CTL_12_DATA_LO:	.word 0x0b000000 #400
//DDR2_CTL_12_DATA_LO:	.word 0x05000000 #250M
DDR2_CTL_12_DATA_LO:	.word 0x15000000 #800
//0000000_000101010 dll_dqs_delay_bypass_0 00011100 trfc 00000100 trcd_int 
//DDR2_CTL_12_DATA_HI:	.word 0x01ff0302 #125/2 M,read_dqs_delay_max
//DDR2_CTL_12_DATA_HI:	.word 0x002a0302 #125/2 M
//DDR2_CTL_12_DATA_HI:	.word 0x002a0602 #125 M
//DDR2_CTL_12_DATA_HI:	.word 0x002a3c04 #400
DDR2_CTL_12_DATA_HI:	.word 0x002a3c05 #250M
//DDR2_CTL_12_DATA_HI:	.word 0x002a3c06 #800
//0000000_000101010 dll_dqs_delay_bypass_2 0000000_000101010 dll_dqs_delay_bypass_1
//DDR2_CTL_13_DATA_LO:	.word 0x01ff01ff #read_dqs_delay_max
DDR2_CTL_13_DATA_LO:	.word 0x002a002a 
//0000000_000101010 dll_dqs_delay_bypass_4 0000000_000101010 dll_dqs_delay_bypass_3 
//DDR2_CTL_13_DATA_HI:	.word 0x01ff01ff #read_dqs_delay_max
DDR2_CTL_13_DATA_HI:	.word 0x002a002a
//0000000_000101010 dll_dqs_delay_bypass_6 0000000_000101010 dll_dqs_delay_bypass_5
//DDR2_CTL_14_DATA_LO:	.word 0x01ff01ff #read_dqs_delay_max
DDR2_CTL_14_DATA_LO:	.word 0x002a002a
//0000000_000101010 dll_dqs_delay_bypass_8 0000000_000101010 dll_dqs_delay_bypass_7 
//DDR2_CTL_14_DATA_HI:	.word 0x01ff01ff #read_dqs_delay_max
DDR2_CTL_14_DATA_HI:	.word 0x002a002a
//0000000_000000000 dll_lock 0000000_000100100 dll_increment
DDR2_CTL_15_DATA_LO:	.word 0x00000004
//0000000_010110100 dqs_out_shift_bypass 0000000_010000111 dll_start_point 
DDR2_CTL_15_DATA_HI:	.word 0x00b40020
//000000_0000000000 int_ack 0000000_010000111 wr_dqs_shift_bypass
DDR2_CTL_16_DATA_LO:	.word 0x00000087
//00000_00000000000 int_status 00000_00000000000 int_mask 
DDR2_CTL_16_DATA_HI:	.word 0x000007ff #no_interrupt
//DDR2_CTL_16_DATA_HI:	.word 0x00000000 #no_masked
//0_000000000000000 emrs1_data 00_00100000011011 tref
//DDR2_CTL_17_DATA_LO:	.word 0x0000004b #125/16 M
//DDR2_CTL_17_DATA_LO:	.word 0x0000009c #20 M
//DDR2_CTL_17_DATA_LO:	.word 0x000004b0 #125 M
//DDR2_CTL_17_DATA_LO:	.word 0x0000081b #400
DDR2_CTL_17_DATA_LO:	.word 0x0004101b #800
//0_000000000000000 emrs2_data_1 0_000000000000000 emrs2_data_0 
DDR2_CTL_17_DATA_HI:	.word 0x00000000
//0_000000000000000 emrs2_data_3 0_000000000000000 emrs2_data_2
DDR2_CTL_18_DATA_LO:	.word 0x00000000
//0000000000011100 axi0_en_size_lt_width_instr 0_000000000000000 emrs3_data 
DDR2_CTL_18_DATA_HI:	.word 0x001c0000
//0000000011001000 tdll 0000000001101011 tcpd
DDR2_CTL_19_DATA_LO:	.word 0x00c8006b
//0100100011100001 tras_max 0000000000000010 tpdex 
//DDR2_CTL_19_DATA_HI:	.word 0x04b00002 #125 M
//DDR2_CTL_19_DATA_HI:	.word 0x48e10002 #400
DDR2_CTL_19_DATA_HI:	.word 0x68e10002 #800
//0000000011001000 txsr 0000000000011111 txsnr
//DDR2_CTL_20_DATA_LO:	.word 0x00c8000f #125 M
//DDR2_CTL_20_DATA_LO:	.word 0x00c8001f #400
DDR2_CTL_20_DATA_LO:	.word 0x00c8002f #800
//0000000000000000 xor_check_bits 0000000000000000 version 
DDR2_CTL_20_DATA_HI:	.word 0x00000000
 //000000000000000000110110 tinit
DDR2_CTL_21_DATA_LO:	.word 0x00030d40 #real
//DDR2_CTL_21_DATA_LO:	.word 0x00000036 #simulation
 //000_0000000000000000000000000000000000000 ecc_c_addr 
DDR2_CTL_21_DATA_HI:	.word 0x00000000
//000000000000000000000000000_0000000000000000000000000000000000000 ecc_u_addr
DDR2_CTL_22_DATA_LO:	.word 0x00000000
DDR2_CTL_22_DATA_HI:	.word 0x00000000
//000000000000000000000000000_0000000000000000000000000000000000000 out_of_range_addr
DDR2_CTL_23_DATA_LO:	.word 0x00000000
DDR2_CTL_23_DATA_HI:	.word 0x00000000
//000000000000000000000000000_0000000000000000000000000000000000000 port_cmd_error_addr
DDR2_CTL_24_DATA_LO:	.word 0x00000000
DDR2_CTL_24_DATA_HI:	.word 0x00000000
//0000000000000000000000000000000000000000000000000000000000000000 ecc_c_data
DDR2_CTL_25_DATA_LO:	.word 0x00000000
DDR2_CTL_25_DATA_HI:	.word 0x00000000
//0000000000000000000000000000000000000000000000000000000000000000 ecc_u_data
DDR2_CTL_26_DATA_LO:	.word 0x00000000
DDR2_CTL_26_DATA_HI:	.word 0x00000000
//0000000000000000000000000000000000000000000000000000000000000000 
DDR2_CTL_27_DATA_LO:	.word 0x00000000
DDR2_CTL_27_DATA_HI:	.word 0x00000000
//0000000000000000000000000000000000000000000000000000000000000000
#ifdef DEVBD2F_SM502
DDR2_CTL_28_DATA_LO:	.word 0x00000000
#else
DDR2_CTL_28_DATA_LO:	.word 0x00000001
#endif
DDR2_CTL_28_DATA_HI:	.word 0x00000000
//0000000_1 rw_same_en 0000000_0 reg_dimm_enable 0000000_0 reduc 0000000_0 pwrup_srefresh_exit
DDR2_CTL_start_DATA_LO: .word 0x01000000
//0000000_1 swap_port_rw_same_en 0000000_1 swap_en 0000000_0 start 0000000_0 srefresh
DDR2_CTL_start_DATA_HI: .word 0x01010100

	.rdata
	.align 5
ddr2_reg_data1:
//0000000_0 arefresh 0000000_1 ap 0000000_1 addr_cmp_en 0000000_1 active_aging
DDR2_CTL_00_DATA_LO_1:	.word 0x00000101
// 0000000_1 ddrii_sdram_mode 0000000_1 concurrentap 0000000_1 bank_split_en 0000000_0 auto_refresh_mode 
DDR2_CTL_00_DATA_HI_1:	.word 0x01000100 #no_concurrentap
//DDR2_CTL_00_DATA_HI_1:	.word 0x01010100
//0000000_0 ecc_disable_w_uc_err 0000000_1 dqs_n_en 0000000_0 dll_bypass_mode 0000000_0 dlllockreg
//DDR2_CTL_01_DATA_LO_1:	.word 0x00010100 #dll_by_pass
DDR2_CTL_01_DATA_LO_1:	.word 0x00010000
//0000000_0 fwc 0000000_0 fast_write 0000000_0 enable_quick_srefresh 0000000_0 eight_bank_mode 
DDR2_CTL_01_DATA_HI_1:	.word 0x00010000
//0000000_0 no_cmd_init 0000000_0 intrptwritea 0000000_0 intrptreada 0000000_0 intrptapburst
DDR2_CTL_02_DATA_LO_1:	.word 0x00000000
//0000000_1 priority_en 0000000_0 power_down 0000000_1 placement_en 0000000_1 odt_add_turn_clk_en 
DDR2_CTL_02_DATA_HI_1:	.word 0x01000101
//0000000_1 rw_same_en 0000000_0 reg_dimm_enable 0000000_0 reduc 0000000_0 pwrup_srefresh_exit
DDR2_CTL_03_DATA_LO_1:	.word 0x01000000
//0000000_1 swap_port_rw_same_en 0000000_1 swap_en 0000000_0 start 0000000_0 srefresh 
DDR2_CTL_03_DATA_HI_1:	.word 0x01010000
//0000000_0 write_modereg 0000000_1 writeinterp 0000000_1 tref_enable 0000000_1 tras_lockout
DDR2_CTL_04_DATA_LO_1:	.word 0x00010101
#ifdef DEVBD2F_SM502
//000000_01 rtt_0 000000_00 ctrl_raw 000000_10 axi0_w_priority 000000_10 axi0_r_priority 
DDR2_CTL_04_DATA_HI_1:	.word 0x01000202
//00000_100 column_size 00000_101 caslat 00000_010 addr_pins 000000_10 rtt_pad_termination
DDR2_CTL_05_DATA_LO_1:	.word 0x04050202 #CL =4
#else
//000000_01 rtt_0 000000_00 ctrl_raw 000000_10 axi0_w_priority 000000_10 axi0_r_priority 
DDR2_CTL_04_DATA_HI_1:	.word 0x02000202
//00000_100 column_size 00000_101 caslat 00000_010 addr_pins 000000_10 rtt_pad_termination
DDR2_CTL_05_DATA_LO_1:	.word 0x04050101 #CL =4
#endif
//00000_000 q_fullness 00000_000 port_data_error_type 00000_000 out_of_range_type 00000_000 max_cs_reg 
DDR2_CTL_05_DATA_HI_1:	.word 0x00000000
//00000_010 trtp 00000_010 trrd 00000_010 temrs 00000_011 tcke
//DDR2_CTL_06_DATA_LO_1:	.word 0x01020203 #125 M
//DDR2_CTL_06_DATA_LO_1:	.word 0x02020203 #400
//DDR2_CTL_06_DATA_LO_1:	.word 0x02020203 #250M
DDR2_CTL_06_DATA_LO_1:	.word 0x03050203 #800
//0000_1010 aprebit 00000_100 wrlat 00000_010 twtr 00000_100 twr_int 
//DDR2_CTL_06_DATA_HI_1:	.word 0x0a040203 #125 M
//DDR2_CTL_06_DATA_HI_1:	.word 0x0a040204 #400
//DDR2_CTL_06_DATA_HI_1:	.word 0x0a030203 #250M CL=4
//DDR2_CTL_06_DATA_HI_1:	.word 0x0a020203 #CL=3
DDR2_CTL_06_DATA_HI_1:	.word 0x0a040306 #800
//0000_0000 ecc_c_id 0000_1111 cs_map 0000_0111 caslat_lin_gate 0000_1010 caslat_lin
//DDR2_CTL_07_DATA_LO_1:	.word 0x000f0808 #CL=4//cs_map to cs0-cs3
//DDR2_CTL_07_DATA_LO_1:	.word 0x000f0606 #CL=3///cs_map to cs0-cs3
#ifdef DEVBD2F_SM502
DDR2_CTL_07_DATA_LO_1:	.word 0x00030a0b #CL=5//cs_map to cs0-cs3
#else
DDR2_CTL_07_DATA_LO_1:	.word 0x000f0a0b #CL=5//cs_map to cs0-cs3
#endif
//DDR2_CTL_07_DATA_LO_1:	.word 0x000c0708 #CL=4//cs_map to cs2-cs3
//0000_0000 max_row_reg 0000_0000 max_col_reg 0000_0010 initaref 0000_0000 ecc_u_id 
//DDR2_CTL_07_DATA_HI_1:	.word 0x00000200
DDR2_CTL_07_DATA_HI_1:	.word 0x00000400 #800
//0000_0001 odt_rd_map_cs3 0000_0010 odt_rd_map_cs2 0000_0100 odt_rd_map_cs1 0000_1000 odt_rd_map_cs0
#ifdef DEVBD2F_SM502
DDR2_CTL_08_DATA_LO_1:	.word 0x00000102
//0000_0001 odt_wr_map_cs3 0000_0010 odt_wr_map_cs2 0000_0100 odt_wr_map_cs1 0000_1000 odt_wr_map_cs0 
DDR2_CTL_08_DATA_HI_1:	.word 0x00000000
#else
DDR2_CTL_08_DATA_LO_1:	.word 0x0102040f
//0000_0001 odt_wr_map_cs3 0000_0010 odt_wr_map_cs2 0000_0100 odt_wr_map_cs1 0000_1000 odt_wr_map_cs0 
DDR2_CTL_08_DATA_HI_1:	.word 0x01020400
#endif
//0000_0000 port_data_error_id 0000_0000 port_cmd_error_type 0000_0000 port_cmd_error_id 0000_0000 out_of_range_source_id
DDR2_CTL_09_DATA_LO_1:	.word 0x00000000
//000_00000 ocd_adjust_pup_cs_0 000_00000 ocd_adjust_pdn_cs_0 0000_0100 trp 0000_1000 tdal 
//DDR2_CTL_09_DATA_HI_1:	.word 0x00000204 #125 M
//DDR2_CTL_09_DATA_HI_1:	.word 0x00000408 #400
//DDR2_CTL_09_DATA_HI_1:	.word 0x0000030c #250M
DDR2_CTL_09_DATA_HI_1:	.word 0x0000060c #800
//00_111111 age_count 000_01111 trc 000_00010 tmrd 000_00000 tfaw
//DDR2_CTL_10_DATA_LO_1:	.word 0x3f070200 #125 M
//DDR2_CTL_10_DATA_LO_1:	.word 0x3f0f0200 #400
DDR2_CTL_10_DATA_LO_1:	.word 0x3f1a021b #250M
//DDR2_CTL_10_DATA_LO_1:	.word 0x3f1f0200 #800
//0_0011101 dll_dqs_delay_2 0_0011101 dll_dqs_delay_1 0_0011101 dll_dqs_delay_0 00_111111 command_age_count 
DDR2_CTL_10_DATA_HI_1:	.word 0x1515153f
//0_0011101 dll_dqs_delay_6 0_0011101 dll_dqs_delay_5 0_0011101 dll_dqs_delay_4 0_0011101 dll_dqs_delay_3
DDR2_CTL_11_DATA_LO_1:	.word 0x15151515
//0_1011111 wr_dqs_shift 0_1111111 dqs_out_shift 0_0011101 dll_dqs_delay_8 0_0011101 dll_dqs_delay_7 
DDR2_CTL_11_DATA_HI_1:	.word 0x5f7f1515
//00001011 tras_min 00000000 out_of_range_length 00000000 ecc_u_synd 00000000 ecc_c_synd
//DDR2_CTL_12_DATA_LO_1:	.word 0x05000000 #125 M
//DDR2_CTL_12_DATA_LO_1:	.word 0x0b000000 #400
//DDR2_CTL_12_DATA_LO_1:	.word 0x05000000 #250M
DDR2_CTL_12_DATA_LO_1:	.word 0x15000000 #800
//0000000_000101010 dll_dqs_delay_bypass_0 00011100 trfc 00000100 trcd_int 
//DDR2_CTL_12_DATA_HI_1:	.word 0x01ff0302 #125/2 M,read_dqs_delay_max
//DDR2_CTL_12_DATA_HI_1:	.word 0x002a0302 #125/2 M
//DDR2_CTL_12_DATA_HI_1:	.word 0x002a0602 #125 M
//DDR2_CTL_12_DATA_HI_1:	.word 0x002a3c04 #400
DDR2_CTL_12_DATA_HI_1:	.word 0x002a3c05 #250M
//DDR2_CTL_12_DATA_HI_1:	.word 0x002a3c06 #800
//0000000_000101010 dll_dqs_delay_bypass_2 0000000_000101010 dll_dqs_delay_bypass_1
//DDR2_CTL_13_DATA_LO_1:	.word 0x01ff01ff #read_dqs_delay_max
DDR2_CTL_13_DATA_LO_1:	.word 0x002a002a 
//0000000_000101010 dll_dqs_delay_bypass_4 0000000_000101010 dll_dqs_delay_bypass_3 
//DDR2_CTL_13_DATA_HI_1:	.word 0x01ff01ff #read_dqs_delay_max
DDR2_CTL_13_DATA_HI_1:	.word 0x002a002a
//0000000_000101010 dll_dqs_delay_bypass_6 0000000_000101010 dll_dqs_delay_bypass_5
//DDR2_CTL_14_DATA_LO_1:	.word 0x01ff01ff #read_dqs_delay_max
DDR2_CTL_14_DATA_LO_1:	.word 0x002a002a
//0000000_000101010 dll_dqs_delay_bypass_8 0000000_000101010 dll_dqs_delay_bypass_7 
//DDR2_CTL_14_DATA_HI_1:	.word 0x01ff01ff #read_dqs_delay_max
DDR2_CTL_14_DATA_HI_1:	.word 0x002a002a
//0000000_000000000 dll_lock 0000000_000100100 dll_increment
DDR2_CTL_15_DATA_LO_1:	.word 0x00000004
//0000000_010110100 dqs_out_shift_bypass 0000000_010000111 dll_start_point 
DDR2_CTL_15_DATA_HI_1:	.word 0x00b40020
//000000_0000000000 int_ack 0000000_010000111 wr_dqs_shift_bypass
DDR2_CTL_16_DATA_LO_1:	.word 0x00000087
//00000_00000000000 int_status 00000_00000000000 int_mask 
DDR2_CTL_16_DATA_HI_1:	.word 0x000007ff #no_interrupt
//DDR2_CTL_16_DATA_HI_1:	.word 0x00000000 #no_masked
//0_000000000000000 emrs1_data 00_00100000011011 tref
//DDR2_CTL_17_DATA_LO_1:	.word 0x0000004b #125/16 M
//DDR2_CTL_17_DATA_LO_1:	.word 0x0000009c #20 M
//DDR2_CTL_17_DATA_LO_1:	.word 0x000004b0 #125 M
//DDR2_CTL_17_DATA_LO_1:	.word 0x0000081b #400
DDR2_CTL_17_DATA_LO_1:	.word 0x0004101b #800
//0_000000000000000 emrs2_data_1 0_000000000000000 emrs2_data_0 
DDR2_CTL_17_DATA_HI_1:	.word 0x00000000
//0_000000000000000 emrs2_data_3 0_000000000000000 emrs2_data_2
DDR2_CTL_18_DATA_LO_1:	.word 0x00000000
//0000000000011100 axi0_en_size_lt_width_instr 0_000000000000000 emrs3_data 
DDR2_CTL_18_DATA_HI_1:	.word 0x001c0000
//0000000011001000 tdll 0000000001101011 tcpd
DDR2_CTL_19_DATA_LO_1:	.word 0x00c8006b
//0100100011100001 tras_max 0000000000000010 tpdex 
//DDR2_CTL_19_DATA_HI_1:	.word 0x04b00002 #125 M
//DDR2_CTL_19_DATA_HI_1:	.word 0x48e10002 #400
DDR2_CTL_19_DATA_HI_1:	.word 0x68e10002 #800
//0000000011001000 txsr 0000000000011111 txsnr
//DDR2_CTL_20_DATA_LO_1:	.word 0x00c8000f #125 M
//DDR2_CTL_20_DATA_LO_1:	.word 0x00c8001f #400
DDR2_CTL_20_DATA_LO_1:	.word 0x00c8002f #800
//0000000000000000 xor_check_bits 0000000000000000 version 
DDR2_CTL_20_DATA_HI_1:	.word 0x00000000
 //000000000000000000110110 tinit
DDR2_CTL_21_DATA_LO_1:	.word 0x00030d40 #real
//DDR2_CTL_21_DATA_LO_1:	.word 0x00000036 #simulation
 //000_0000000000000000000000000000000000000 ecc_c_addr 
DDR2_CTL_21_DATA_HI_1:	.word 0x00000000
//000000000000000000000000000_0000000000000000000000000000000000000 ecc_u_addr
DDR2_CTL_22_DATA_LO_1:	.word 0x00000000
DDR2_CTL_22_DATA_HI_1:	.word 0x00000000
//000000000000000000000000000_0000000000000000000000000000000000000 out_of_range_addr
DDR2_CTL_23_DATA_LO_1:	.word 0x00000000
DDR2_CTL_23_DATA_HI_1:	.word 0x00000000
//000000000000000000000000000_0000000000000000000000000000000000000 port_cmd_error_addr
DDR2_CTL_24_DATA_LO_1:	.word 0x00000000
DDR2_CTL_24_DATA_HI_1:	.word 0x00000000
//0000000000000000000000000000000000000000000000000000000000000000 ecc_c_data
DDR2_CTL_25_DATA_LO_1:	.word 0x00000000
DDR2_CTL_25_DATA_HI_1:	.word 0x00000000
//0000000000000000000000000000000000000000000000000000000000000000 ecc_u_data
DDR2_CTL_26_DATA_LO_1:	.word 0x00000000
DDR2_CTL_26_DATA_HI_1:	.word 0x00000000
//0000000000000000000000000000000000000000000000000000000000000000 
DDR2_CTL_27_DATA_LO_1:	.word 0x00000000
DDR2_CTL_27_DATA_HI_1:	.word 0x00000000
//0000000000000000000000000000000000000000000000000000000000000000
#ifdef DEVBD2F_SM502
DDR2_CTL_28_DATA_LO_1:	.word 0x00000000
#else
DDR2_CTL_28_DATA_LO_1:	.word 0x00000001
#endif
DDR2_CTL_28_DATA_HI_1:	.word 0x00000000
//0000000_1 rw_same_en 0000000_0 reg_dimm_enable 0000000_0 reduc 0000000_0 pwrup_srefresh_exit
DDR2_CTL_start_DATA_LO_1: .word 0x01000000
//0000000_1 swap_port_rw_same_en 0000000_1 swap_en 0000000_0 start 0000000_0 srefresh
DDR2_CTL_start_DATA_HI_1: .word 0x01010100

.text
#ifdef USE_SM502_UART0
LEAF(tgt_testchar)
#define	NSREG1(x)	(x*4)
	la	v0,0xb6030000
	lbu	v1, NSREG1(NS16550_LSR)(v0)
	and	v0,v1, LSR_RXRDY
	jr ra
	nop
END(tgt_testchar)

LEAF(tgt_getchar)
#define	NSREG1(x)	(x*4)
	la	v0,0xb6030000
11:
	lbu	v1, NSREG1(NS16550_LSR)(v0)
	and	v1, LSR_RXRDY
	beqz	v1, 11b
	nop
	lb	v0, NSREG1(NS16550_DATA)(v0)
	jr ra
	nop
END(tgt_getchar)
#endif


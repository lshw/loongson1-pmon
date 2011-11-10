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
#include "target/i82371eb.h"
#include "target/prid.h"
#include "target/sbd.h"
#include "target/bonito.h"
#include "target/i8254.h"
#include "target/pc97307.h"
#include "target/isapnpreg.h"
#define DEBUG_LOCORE
#undef	DEBUG_DIMM_SPD
#ifdef DEBUG_LOCORE
#define	TTYDBG(x) \
	.rdata;98: .asciz x; .text; la a0, 98b; bal stringserial; nop
#else
#define TTYDBG(x)
#endif
#define	PRINTSTR(x) \
	.rdata;98: .asciz x; .text; la a0, 98b; bal stringserial; nop

#define CONFIG_CACHE_64K_4WAY 1

#define tmpsize		s1
#define msize		s2
#define sdShape		s3
#define bonito		s4
#define dbg		s5
#define sdCfg		s6

#define	CFG_IB 0x00000020
#define CFG_DB 0x00000010
#define CFG_C_WBACK 3
#define CFG_BE 0x00008000
#define CFG_EPMASK 0x0f000000
#define CFG_EPD 0x00000000
#define CFG_EM_R4K 0x00000000
#define CFG_EMMASK 0x00c00000
#define CFG_AD 0x00800000

#define CP0_CONFIG $16
#define CP0_TAGLO  $28
#define CP0_TAGHI  $29

#define DDR100 0x04041091
#define	DDR266 0x0410435e
#define DDR300 0x041453df

#if 0
#define DEBUG_WR(num) \
li v0,0xbfe00170; \
li v1,num; \
sw v1,(v0);
#else
#define DEBUG_WR(num) 
#endif

/*
 *   Register usage:
 *
 *	s0	link versus load offset, used to relocate absolute adresses.
 *	s1	free
 *	s2	memory size.
 *	s3	sdShape.
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
#if 1
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
#endif

/*
 *  We get here from executing a bal to get the PC value of the current execute
 *  location into ra. Check to see if we run from ROM or if this is ramloaded.
 */
locate:
	la	s0,start
	subu	s0,ra,s0
	and	s0,0xffff0000

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
	WR_INIT(MOD_W,BONITO_BASE+BONITO_PCIMAP_CFG,CFGADDR(PCI_IDSEL_I82371,function,isabreg)>>16) ; \
        RD_INIT(MOD_W,BONITO_BASE+BONITO_PCIMAP_CFG) ; \
	WR_INIT(mod,PCI_CFG_SPACE+(CFGADDR(PCI_IDSEL_I82371,function,isabreg)&0xffff),val)
	
#define _ISABRD_INIT(mod,function,isabreg) \
	WR_INIT(MOD_W,BONITO_BASE+BONITO_PCIMAP_CFG,CFGADDR(PCI_IDSEL_I82371,function,isabreg)>>16) ; \
        RD_INIT(MOD_W,BONITO_BASE+BONITO_PCIMAP_CFG) ; \
	RD_INIT(mod,PCI_CFG_SPACE+(CFGADDR(PCI_IDSEL_I82371,function,isabreg)&0xffff))


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
#define	PCI_DEV_I82371			17
#define PCI_CFG_SPACE			BONITO_PCICFG_BASE

        BONITO_INIT(BONITO_PCICLASS,(PCI_CLASS_BRIDGE << PCI_CLASS_SHIFT) | (PCI_SUBCLASS_BRIDGE_HOST << PCI_SUBCLASS_SHIFT))
	BONITO_INIT(BONITO_PCICMD, BONITO_PCICMD_PERR_CLR|BONITO_PCICMD_SERR_CLR|BONITO_PCICMD_MABORT_CLR|BONITO_PCICMD_MTABORT_CLR|BONITO_PCICMD_TABORT_CLR|BONITO_PCICMD_MPERR_CLR)
	BONITO_INIT(BONITO_PCILTIMER, 0)
	BONITO_INIT(BONITO_PCIBASE0, 0)
	BONITO_INIT(BONITO_PCIBASE1, 0)
	BONITO_INIT(BONITO_PCIBASE2, 0)
	BONITO_INIT(BONITO_PCIEXPRBASE, 0)
	BONITO_INIT(BONITO_PCIINT, 0)

	BONITO_BIS(BONITO_PCICMD, BONITO_PCICMD_PERRRESPEN)
	
	BONITO_BIS(BONITO_PCICMD, PCI_COMMAND_IO_ENABLE|PCI_COMMAND_MEM_ENABLE|PCI_COMMAND_MASTER_ENABLE)
	
	/* enable i/o buffer cache and other go faster bits */
	BONITO_BIS(BONITO_BONGENCFG, \
			BONITO_BONGENCFG_BUSERREN| \
			BONITO_BONGENCFG_PREFETCHEN| \
			BONITO_BONGENCFG_WBEHINDEN| \
			BONITO_BONGENCFG_PCIQUEUE| \
			BONITO_BONGENCFG_SNOOPEN)
	
       BONITO_BIC(BONITO_BONGENCFG, 0x80)  #陆没脰鹿iobc

#      BONITO_BIS(BONITO_BONGENCFG, BONITO_BONGENCFG_BUSERREN)

	/* Set debug mode */
	BONITO_BIS(BONITO_BONGENCFG, BONITO_BONGENCFG_DEBUGMODE)

        /******** added to void init southbridge*/
#if 1
#if 0
	/* zhb init USB */
	ISABWWR_INIT(2, 0x20, 0x8040|PCI_MAP_IO)
	ISABWWR_INIT(2, PCI_COMMAND_STATUS_REG, 5)
#endif
#if 1
	/* zhb init floppy-disk */
	ISABWWR_INIT(3, I82371_PCI3_DEVRESD, 0x1800)
	ISABWWR_INIT(3, I82371_PCI3_DEVRESB, 0x20000000)
#endif
#if 1
	/* Turn most special purpose pins into GPIO; set ISA mode */
	ISABWWR_INIT(0, I82371_GENCFG, I82371_GENCFG_CFG)
	
	/* Set the SMB base address */
	ISABWWR_INIT(3, I82371_PCI3_SMBBA, SMB_PORT|PCI_MAP_IO)
	/* enable the host controller */
	ISABBWR_INIT(3, I82371_PCI3_SMBHSTCFG, I82371_PCI3_SMB_HST_EN)
	/* enable the SMB IO ports */
	ISABBWR_INIT(3, PCI_COMMAND_STATUS_REG, PCI_COMMAND_IO_ENABLE)
	
	ISABWWR_INIT(3, I82371_PCI3_PMBA, PM_PORT|PCI_MAP_IO) /*notice*/

	ISABBWR_INIT(3, I82371_PCI3_PMREGMISC, 0x01)
#endif
#if 1
	/* disable RTC & KBD chip selects */
	ISABHWR_INIT(0, I82371_XBCS, 0)
	
	/* Enable PCI 2.1 timing support */
	ISABBWR_INIT(0, I82371_DLC, I82371_DLC_DT /* | I82371_DLC_PR */ | I82371_DLC_USBPR | I82371_DLC_DTTE)

	/* Set top of memory to 16MB, so all ISA bus master & DMA
	   accesses are forwarded to PCI mem space
	 */
	ISABBWR_INIT(0, I82371_TOM, I82371_TOM_TOM(16) | I82371_TOM_FWD_LBIOS | I82371_TOM_FWD_AB | I82371_TOM_FWD_89)
	
	
/* 15us ISA bus refresh clock */
#define ISAREFRESH (PT_CRYSTAL/(1000000/15))
	ISARD_INIT(CTC_PORT+PT_CONTROL)
	
	/* program i8254 ISA refresh counter */
	ISAWR_INIT(CTC_PORT+PT_CONTROL,PTCW_SC(PT_REFRESH)|PTCW_16B|PTCW_MODE(MODE_RG))
	ISAWR_INIT(CTC_PORT+PT_REFRESH, ISAREFRESH & 0xff)
	ISAWR_INIT(CTC_PORT+PT_REFRESH, ISAREFRESH >> 8)

	/* program ISA ICU */
	ISAWR_INIT(ICU1_PORT,  0x11) /* ICW1 */
	ISAWR_INIT(ICU1_PORT+1,0x00) /* ICW2: vector */
	ISAWR_INIT(ICU1_PORT+1,0x04) /* ICW3: cascade on IRQ2 */
	ISAWR_INIT(ICU1_PORT+1,0x01) /* ICW4: 8086 mode */
	ISAWR_INIT(ICU1_PORT+1,0xff) /* OCW1: mask all */

	ISAWR_INIT(ICU2_PORT,  0x11) /* ICW1 */
	ISAWR_INIT(ICU2_PORT+1,0x08) /* ICW2: vector */
	ISAWR_INIT(ICU2_PORT+1,0x02) /* ICW3:  */
	ISAWR_INIT(ICU2_PORT+1,0x01) /* ICW4: 8086 mode */
	ISAWR_INIT(ICU2_PORT+1,0xff) /* OCW1: mask all */
		
	ISAWR_INIT(ICU1_PORT+1,~(1<<2)) /* enable IRQ2 */
        /* set up ISA devices */
        
        /* select logical device 1 (mouse) */
        ISAWR_INIT(ISAPNP_MBADDR,ISAPNP_LOGICAL_DEV_NUM)
        ISAWR_INIT(ISAPNP_MBDATA,1)
        ISAWR_INIT(ISAPNP_MBADDR,ISAPNP_ACTIVATE)
        ISAWR_INIT(ISAPNP_MBDATA,1)
                                                                               
        /* select logical device 4 (parallel) */
        ISAWR_INIT(ISAPNP_MBADDR,ISAPNP_LOGICAL_DEV_NUM)
        ISAWR_INIT(ISAPNP_MBDATA,4)
        ISAWR_INIT(ISAPNP_MBADDR,ISAPNP_IO_DESC0+ISAPNP_IO_BASE_15_8)
        ISAWR_INIT(ISAPNP_MBDATA,(ECP_PORT>>8) & 0xff)
        ISAWR_INIT(ISAPNP_MBADDR,ISAPNP_IO_DESC0+ISAPNP_IO_BASE_7_0)
        ISAWR_INIT(ISAPNP_MBDATA,ECP_PORT & 0xff)
        ISAWR_INIT(ISAPNP_MBADDR,ISAPNP_IRQ_DESC0+ISAPNP_IRQ_CONTROL)
        ISAWR_INIT(ISAPNP_MBDATA,ISAPNP_IRQ_HIGH)
        ISAWR_INIT(ISAPNP_MBADDR,ISAPNP_ACTIVATE)
        ISAWR_INIT(ISAPNP_MBDATA,1)
                                                                               
        /* select logical device 5 (COM2) */
        ISAWR_INIT(ISAPNP_MBADDR,ISAPNP_LOGICAL_DEV_NUM)
        ISAWR_INIT(ISAPNP_MBDATA,5)
        ISAWR_INIT(ISAPNP_MBADDR,ISAPNP_ACTIVATE)
        ISAWR_INIT(ISAPNP_MBDATA,1)

        /* select logical device 6 (COM1) */
        ISAWR_INIT(ISAPNP_MBADDR,ISAPNP_LOGICAL_DEV_NUM)
        ISAWR_INIT(ISAPNP_MBDATA,6)
        ISAWR_INIT(ISAPNP_MBADDR,ISAPNP_ACTIVATE)
        ISAWR_INIT(ISAPNP_MBDATA,1)
#endif
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
	/* Initialise other low-level I/O devices */

	bal initgpio
	nop

	move k0,v0

	bal nb_pcirst
	nop

	li a0,ACTIVECOM_OFFS
	bal get_userenv
	nop
	move k1,v0

	bal	initserial
	nop
	
	move a0,k0
	bal	hexserial
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
	
	PRINTSTR(" PRID=")
	mfc0	a0, COP_0_PRID
	bal	hexserial
	nop
	PRINTSTR("\r\n")

	PRINTSTR("Raw word read of SMB base address: ");
	li	a0,CFGADDR(PCI_DEV_I82371,3,I82371_PCI3_SMBBA)
	li	a1,PHYS_TO_UNCACHED(PCI_CFG_SPACE)
	and	a2,a0,0xffff
	or	a1,a2
	srl	a0,16
	li	a2,BONITO_BASE+BONITO_PCIMAP_CFG
	sw	a0,BONITO_PCIMAP_CFG(bonito)
	lw	zero,BONITO_PCIMAP_CFG(bonito)
	lw	a0,(a1)
	
	li	a1,2
	bal	hexserial
	nop
	PRINTSTR("\r\n")	
	
	
/***(qiaochong) memeory initialization use macro,not smbbus ***/
					  		 /*bit 31: DDR配置结束标志，1表示结束，只读*/
#define DDR_DQS_SELECT	(0<<30) 		 /*bit 30 选择数据来源，0: 双沿采样；1：DQS采样*/
#define DDR_DIMM_DIC (1<<29)           /*bit 29 标识DIMM_slot0是否插有内存条。
                                0：无；1：有；*/
#define DDR_DIMM_MODULE_NUM (3<<27)    /*bit 28:27 DIMM0/DIMM1上MOUDLE的数目：
								2’b00：DIMM1: 1; DIMM0: 1
								2’b01：DIMM1: 1; DIMM0: 2
								2’b10：DIMM1: 2; DIMM0: 1
								2’b11：DIMM1: 2; DIMM0: 2
								*/
#define DDR_IS_SEQ  (1<<26)            /*bit 26 义突发式读写时的块内顺序，
								1’b0：顺序；1’b1：交替，现在只支持交替方式*/
#define DDR_TYPE (5<<22) /*bit 25:22 表2：DDR 控制器所支持的DDR SDRAM 计芯片类型
								BITS Density Org.  Row Addr.  Col Addr.
								0000 64Mb 16Mb X 4 DA[11:0] DA[9:0]
									128Mb 16Mb X 8
								0001 64Mb 8Mb X 8 DA[11:0] DA[8:0]
									128Mb 8Mb X 16 
								0010 64Mb 4Mb X 16 DA[11:0] DA[7:0]
								0011 128Mb 32Mb X 4 DA[11:0] DA[11],DA[9:0]
								0100 256Mb 64Mb X 4 DA[12:0] DA[11],DA[9:0]
									 512Mb 64Mb X 8 
								0101 256Mb 32Mb X 8 DA[12:0] DA[9:0]
									 512Mb 32Mb X 16
								0110 256Mb 16Mb X 16 DA[12:0] DA[8:0]
								0111 512Mb 128Mb X 4 DA[12:0] DA[12:11],DA[9:0]
								1000 1Gb 256Mb X 4 DA[13:0] DA[12:11],DA[9:0]
								1001 1Gb 128Mb X 8 DA[13:0] DA[11],DA[9:0]
								1010 1Gb 64Mb X 16 DA[13:0] DA[9:0]
								*/
#define DDR_tREF  (100<<10) /*bit 21:10 SDRAM刷新操作之间计数（主频100MHz）：
								780      7.8us
								1560     15.6us
								SDRAM刷新操作之间计数（主频133MHz）：
								1040      7.8us
								2080      15.6us
								SDRAM刷新操作之间计数（主频166MHz）：
								1300      7.8us
								2600      15.6us
								*/
#define DDR_TRCD (0<<9) /*bit 9 行地址有效到列地址有效之间需经过的计数
								1’b0   2 cycles（DDR100）
								1’b1   3 cycles（DDR266、DDR333）*/
#define DDR_TRPC (1<<7) /*bit 8:7 AUTO_REFRESH到ACTIVE之间需经过的计数
								2’b00  Null 
								2’b01  8 cylces （DDR100）
								2’b10  10 cycles（DDR266）
								2’b11  12 cycles（DDR333）
								*/
#define DDR_TRAS (0<<6) /*bit 6 ACTIVE到PRECHARGE之间需经过的计数
								1’b0   5 cycles（DDR100）
								1’b1   7 cycles（DDR266、DDR333）  
								*/
#define DDR_TCAS (1<<4) /*bit 5:4 从读命令到第一个数据到来需经过的计数
								2’b00  1.5 cycles
								2’b01  2 cycles
								2’b10  2.5 cycles
								2’b11  3 cycles
								*/
#define DDR_TWR (0<<3) /*bit 3 写操作最后一个数据到PRECHARGE之间需经过的计数
								1’b0   2 cycles（DDR100）
								1’b1   3 cycles（DDR266、DDR333）
								*/
#define DDR_TRP (0<<2) /*bit 2 PRECHARGE命令执行时间计数
								1’b0   2 cycles（DDR100）
								1’b1   3 cycles（DDR266、DDR333）
								*/
#define DDR_TRC (1<<0) /*bit 1:0 ACTIVE与ACTIVE/AUTO_REFRESH命令之间计数
								2’b00  Null
								2’b01  7 cycles（DDR100）
								2’b10  9 cycles（DDR266）
								2’b11  10cycles（DDR333）
								注（由于precharge和ras cas的延时加起来正好满足这个延时，所以在DDR控制器里没有具体考虑这个参数）
								*/
#define sdcfg_DATA DDR_DQS_SELECT|DDR_DIMM_DIC|DDR_DIMM_MODULE_NUM|DDR_IS_SEQ|DDR_TYPE|DDR_tREF|DDR_TRCD|DDR_TRPC|DDR_TRAS|DDR_TCAS|DDR_TWR|DDR_TRP|DDR_TRC
	li  msize,512*(1<<20)
	li  sdCfg,sdcfg_DATA
#	li sdCfg,0x05441091
	sw	sdCfg,BONITO_SDCFG(bonito)
#if 1//def DBGSBD
	PRINTSTR("sdcfg=");
	move	a0,sdCfg
	bal	hexserial
	nop
	PRINTSTR("\r\n");
	PRINTSTR("msize=");
	move	a0,msize
	sw	a0,BONITO_MEMSIZE(bonito)
	bal	hexserial
	nop
	PRINTSTR("\r\n")
#endif
	li	t1,0		# accumulate pcimembasecfg settings
		
	/* set bar0 mask and translation to point to SDRAM */
	sub	t0,msize,1
	not	t0
	srl	t0,BONITO_PCIMEMBASECFG_ASHIFT-BONITO_PCIMEMBASECFG_MEMBASE0_MASK_SHIFT
	and	t0,BONITO_PCIMEMBASECFG_MEMBASE0_MASK
	or	t1,t0
	
	li	t0,0x00000000
	srl	t0,BONITO_PCIMEMBASECFG_ASHIFT-BONITO_PCIMEMBASECFG_MEMBASE0_TRANS_SHIFT
	and	t0,BONITO_PCIMEMBASECFG_MEMBASE0_TRANS
	or	t1,t0
	or	t1,BONITO_PCIMEMBASECFG_MEMBASE0_CACHED

	/* set bar1 to minimum size to conserve PCI space */
	li	t0, ~0
	srl	t0,BONITO_PCIMEMBASECFG_ASHIFT-BONITO_PCIMEMBASECFG_MEMBASE1_MASK_SHIFT
	and	t0,BONITO_PCIMEMBASECFG_MEMBASE1_MASK
	or	t1,t0
	
	li	t0,0x00000000
	srl	t0,BONITO_PCIMEMBASECFG_ASHIFT-BONITO_PCIMEMBASECFG_MEMBASE1_TRANS_SHIFT
	and	t0,BONITO_PCIMEMBASECFG_MEMBASE1_TRANS
	or	t1,t0
	or	t1,BONITO_PCIMEMBASECFG_MEMBASE1_CACHED

	sw	t1,BONITO_PCIMEMBASECFG(bonito)

	/* enable configuration cycles now */
	lw	t0,BONITO_BONPONCFG(bonito)
	and	t0,~BONITO_BONPONCFG_CONFIG_DIS
	sw	t0,BONITO_BONPONCFG(bonito)

	PRINTSTR("Init SDRAM Done!\r\n");

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
	andi	a0, a0, 0xff00
	li      a1, 0x6300
	bne     a0, a1, cache_done
	nop
	TTYDBG("godson2 caches found\r\n")
        bal     godson2_cache_init
        nop
cache_done:

#ifdef DEBUG_LOCORE
	TTYDBG("Init caches done, cfg = ")
	mfc0	a0, COP_0_CONFIG
	bal	hexserial
	nop
	TTYDBG("\r\n")
#endif
#include "machine/newtest/mydebug.S"
bootnow:

	TTYDBG("Copy PMON to execute location...\r\n")
#include "copypmon.S"
	TTYDBG("Copy PMON to execute location done.\r\n")
#include "test_after_copy1.S"


	li	a0, 0
	sw	a0, CpuTertiaryCacheSize /* Set L3 cache size */
	move	a0,msize
	srl	a0,20
 
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

LEAF(add_tlb_entry)

	and	a2, a0, PG_SVPN
	MTC0   a2, COP_0_TLB_HI	# Set up entry high.
	tlbp
	and	a2, a0, PG_SVPN
	MTC0   a2, COP_0_TLB_HI	# Set up entry high.

	srl	a2, a1, PG_SHIFT 
	and	a2, a1, PG_FRAME
	ori	a2, PG_IOPAGE
	MTC0   a2, COP_0_TLB_LO0	# Set up entry low0.
	addu	a2, (0x01000000 >> PG_SHIFT)
	MTC0   a2, COP_0_TLB_LO1	# Set up entry low1.
	MFC0	a3,COP_0_TLB_INDEX
	blez a3,1f
	nop
	tlbwi				# Write the TLB
	bal 2f
	nop
1:
	tlbwr
2:
	jr	ra
	nop
END(add_tlb_entry)


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
	and v0,k1,1
	beqz v0,11f
	nop
	la	v0, COM1_BASE_ADDR
	bal 1f
	nop
11:
	and v0,k1,2
	beqz v0,12f
	la	v0, COM2_BASE_ADDR
	bal 1f
	nop
12:
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
#ifdef HAVE_NB_SERIAL
	la	v0, COM3_BASE_ADDR
 	li	a0, NS16550HZ/(16*CONS_BAUD)
	bal 1f
	nop
#endif
 	li	a0, NS16550HZ/2/(16*CONS_BAUD)
	and v0,k1,1
	beqz v0,11f
	nop
	la	v0, COM1_BASE_ADDR
	bal 1f
	nop
11:
	and v0,k1,2
	beqz v0,12f
	nop
	la	v0, COM2_BASE_ADDR
	bal 1f
	nop
12:
	jr AT
	nop
1:
	li	v1, FIFO_ENABLE|FIFO_RCV_RST|FIFO_XMT_RST|FIFO_TRIGGER_4
	sb	v1, NSREG(NS16550_FIFO)(v0)
	li	v1, CFCR_DLAB
	sb	v1, NSREG(NS16550_CFCR)(v0)
 	//li	v1, NS16550HZ/(16*CONS_BAUD)
	move v1,a0
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
	.set at
END(initserial)

#define get1counts \
li v0,0; \
li v1,32; \
991:\
and a1,a0,1; \
add v0,a1; \
srl a0,1; \
bnez v1,991b; \
addiu v1,-1; 

LEAF(get_userenv)
#ifdef NVRAM_IN_FLASH
	li a1,(0xbfc00000+NVRAM_OFFS+3)
	add a1,a0
	addu a0,a1,-3
	move v1,zero
1:
	lbu v0,(a1)
	sll v1,8
	or v1,v0
	bne a0,a1,1b
	addu a1,-1
#else
	li a1,RTC_NVRAM_BASE+3
	add a1,a0
	addu a0,a1,-3
	move v1,zero
	1:
	sb a1,(PHYS_TO_UNCACHED(PCI_IO_SPACE+RTC_INDEX_REG))
	lbu v0,(PHYS_TO_UNCACHED(PCI_IO_SPACE+RTC_DATA_REG))
	sll v1,v1,8
	or v1,v0
	bne a1,a0,1b
	addiu a1,-1
#endif
	move v0,v1
	jr ra
	nop
END(get_userenv)

LEAF(initgpio)
	move a3,ra
	li a0,GPIOCFG_OFFS
	bal get_userenv
	nop
	move ra,a3
	move a2,v0
	and a0,a2,UART1_232|UART1_422|UART1_485
	get1counts
	addiu v0,-1
	bgtz v0,1f
	nop
	and a0,a2,UART2_232|UART2_422|UART2_485
	get1counts
	addiu v0,-1
	bgtz v0,1f
	nop
	or a2,GPIO_SETS
	sw a2,(PHYS_TO_UNCACHED(PCI_IO_SPACE+GPO_REG))
	move v0,a2
	jr ra
	nop
1:
	move v0,a2
	li a0,(UART1_232|UART2_232|LAN1_EN|LAN2A_EN|LAN2B_EN|GPIO_SETS)
	sw a0,(PHYS_TO_UNCACHED(PCI_IO_SPACE+GPO_REG))
	jr ra
	nop
END(initgpio)

LEAF(nb_pcirst)
/* enable configuration cycles now */
    lw  t0,BONITO_BONPONCFG(bonito)
    and t1,t0,~(1<<3)
    sw  t1,BONITO_BONPONCFG(bonito)
	li v0, 0x100;	
1:			
	bnez	v0, 1b;
	addiu	v0, -1
    sw  t0,BONITO_BONPONCFG(bonito)
	jr ra
	nop
END(nb_pcirst)

#define I82371_RST_REG 0xcf9
LEAF(sb_pcirst)
	li v0,2
	sb v0,(PHYS_TO_UNCACHED(PCI_IO_SPACE+I82371_RST_REG))
	li v0,6
	sb v0,(PHYS_TO_UNCACHED(PCI_IO_SPACE+I82371_RST_REG))
	li v0, 0x8000;	
1:			
	bnez	v0, 1b;
	addiu	v0, -1
	jr ra
	nop
END(sb_pcirst)

#define SMBOFFS(reg) I82371_SMB_SMB##reg

LEAF(i2cdump)
	move	t4,ra
	
	li	a0,PHYS_TO_UNCACHED(ISAPORT_BASE(SMB_PORT))
	lbu	a0,SMBOFFS(HSTSTS)(a0)
	li	a1,1
	bal	hexserial
	nop
	PRINTSTR("\r\n")
		
	li	a0,PHYS_TO_UNCACHED(ISAPORT_BASE(SMB_PORT))
	lbu	a0,SMBOFFS(SLVSTS)(a0)
	li	a1,1
	bal	hexserial
	nop
	PRINTSTR("\r\n")
	
	li	a0,PHYS_TO_UNCACHED(ISAPORT_BASE(SMB_PORT))
	lbu	a0,SMBOFFS(HSTCNT)(a0)
	li	a1,1
	bal	hexserial
	nop
	PRINTSTR("\r\n")
	
	li	a0,PHYS_TO_UNCACHED(ISAPORT_BASE(SMB_PORT))
	lbu	a0,SMBOFFS(HSTCMD)(a0)
	li	a1,1
	bal	hexserial
	nop
	PRINTSTR("\r\n")
	
	li	a0,PHYS_TO_UNCACHED(ISAPORT_BASE(SMB_PORT))
	lbu	a0,SMBOFFS(HSTADD)(a0)
	li	a1,1
	bal	hexserial
	nop
	PRINTSTR("\r\n")
	
	li	a0,PHYS_TO_UNCACHED(ISAPORT_BASE(SMB_PORT))
	lbu	a0,SMBOFFS(HSTDAT0)(a0)
	li	a1,1
	bal	hexserial
	nop
	PRINTSTR("\r\n")
	
	li	a0,PHYS_TO_UNCACHED(ISAPORT_BASE(SMB_PORT))
	lbu	a0,SMBOFFS(HSTDAT1)(a0)
	li	a1,1
	bal	hexserial
	nop
	PRINTSTR("\r\n")
	
	li	a0,PHYS_TO_UNCACHED(ISAPORT_BASE(SMB_PORT))
	lbu	a0,SMBOFFS(BLKDAT)(a0)
	li	a1,1
	bal	hexserial
	nop
	PRINTSTR("\r\n")
	
	li	a0,PHYS_TO_UNCACHED(ISAPORT_BASE(SMB_PORT))
	lbu	a0,SMBOFFS(SLVCNT)(a0)
	li	a1,1
	bal	hexserial
	nop
	PRINTSTR("\r\n")
	
	li	a0,PHYS_TO_UNCACHED(ISAPORT_BASE(SMB_PORT))
	lbu	a0,SMBOFFS(SHDWCMD)(a0)
	li	a1,1
	bal	hexserial
	nop
	PRINTSTR("\r\n")
	
	li	a0,PHYS_TO_UNCACHED(ISAPORT_BASE(SMB_PORT))
	lbu	a0,SMBOFFS(SLVEVT)(a0)
	li	a1,1
	bal	hexserial
	nop
	PRINTSTR("\r\n")
	
	li	a0,PHYS_TO_UNCACHED(ISAPORT_BASE(SMB_PORT))
	lbu	a0,SMBOFFS(SLVDAT)(a0)
	li	a1,1
	bal	hexserial
	nop
	PRINTSTR("\r\n")
	
	jr	t4
	nop
END(i2cdump)



LEAF(i2cread)
	li	t0,PHYS_TO_UNCACHED(ISAPORT_BASE(SMB_PORT))

	lbu	t1,SMBOFFS(HSTSTS)(t0)
	and	t1,~(I82371_SMB_FAILED|I82371_SMB_BUS_ERR|I82371_SMB_DEV_ERR|I82371_SMB_INTER)
	sb	t1,SMBOFFS(HSTSTS)(t0)
	
	sll	t1,a0,1
	or	t1,0xa1		# DIMM base address and read bit
	sb	t1,SMBOFFS(HSTADD)(t0)
	sb	a1,SMBOFFS(HSTCMD)(t0)
	
	
	li	t1,I82371_SMB_START|I82371_SMB_BDRW
	sb	t1,SMBOFFS(HSTCNT)(t0)

	li	t3,10000	
1:	lbu	t1,SMBOFFS(HSTSTS)(t0)
	and	t2,t1,I82371_SMB_FAILED|I82371_SMB_BUS_ERR|I82371_SMB_DEV_ERR|I82371_SMB_INTER
	bnez	t2,1f
	nop
	sub	t3,1
	bnez	t3,1b
	nop
	b	9f		
	nop
1:	
	# clear pending errors/interrupts
	sb	t1,SMBOFFS(HSTSTS)(t0)
	
	and	t2,t1,I82371_SMB_FAILED|I82371_SMB_BUS_ERR|I82371_SMB_DEV_ERR
	bnez	t2,9f
	nop
	lbu	v0,SMBOFFS(HSTDAT0)(t0)
	j	ra
	nop
	
9:	li	v0,-1
	j	ra
	nop
END(i2cread)	

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

LEAF(nullfunction)
	jr ra
	nop
END(nullfunction)

LEAF(godson2_cache_init)
####part 2####
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

	mfc0 	t7, COP_0_PRID
	andi	t7, t7, 1
	srlv	t6, t6, t7		/* 4way */
	srlv	t5, t5, t7

####part 3####
	lui	a0, 0x8000
	addu	a1, $0, t5
	addu	a2, $0, t6
cache_init_dway:
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
	cache	Index_Store_Tag_D, 0x1(v0)
	beqzl	t7, 1b
	addiu	v0, v0, 0x20
/* godson2c 4way */
	cache   Index_Store_Tag_D, 0x2(v0)
	cache   Index_Store_Tag_D, 0x3(v0)
/* godson2c 4way */
	beq	$0, $0, 1b
	addiu	v0, v0, 0x20
1:
cache_flush_iway:
	addu	v0, $0, a0
	addu	v1, a0, a1
1:	slt	a3, v0, v1
	beq	a3, $0, 1f
	nop
	cache	Index_Invalidate_I, 0x0(v0)
	beq	$0, $0, 1b
	addiu	v0, v0, 0x20
1:
cache_flush_dway:
	addu	v0, $0, a0
	addu	v1, a0, a2
1:	slt	a3, v0, v1
	beq	a3, $0, 1f
	nop
	cache	Index_Writeback_Inv_D, 0x0(v0)
	cache	Index_Writeback_Inv_D, 0x1(v0)
	beqzl	t7, 1b
	addiu	v0, v0, 0x20
/* godson2c 4way */
	cache	Index_Writeback_Inv_D, 0x2(v0)
	cache	Index_Writeback_Inv_D, 0x3(v0)
/* godson2c 4way */
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

#define WATCHDOG_REG BONITO(0x0160)
rebootnow1:
	sw v1,(a0)
	li a0,0
	sw	a0,WATCHDOG_REG(bonito)
	or a0,3<<24
	sw	a0,WATCHDOG_REG(bonito)
1:
	b 1b
	nop
rebootnow:
	sw v1,(a0)
	li a0,0xbfe00104
	lw a1,(a0)	
	and a1,~(1<<2)	
	sw a1,(a0)
	or a1,(1<<2)
	sw a1,(a0)
1:
	b 1b
	nop


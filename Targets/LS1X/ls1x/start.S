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
#include "target/fcr.h"
#include <target/regs-clk.h>

#define DEBUG_LOCORE

/***********************************MAIN FLOW****************************************/

#ifndef BOOT_FROM_EJTAG      
#define BOOT_FROM_EJTAG
#endif
#undef BOOT_FROM_EJTAG

#ifdef DEBUG_LOCORE
#define	TTYDBG(x) \
	.rdata;98: .asciz x; .text; la a0, 98b; bal stringserial; nop
#else
#define TTYDBG(x)
#endif

#ifdef	FAST_STARTUP
#define	PRINTSTR(x) 
#else
#define	PRINTSTR(x) \
	.rdata;98: .asciz x; .text; la a0, 98b; bal stringserial; nop
#endif

#ifdef	FAST_STARTUP
#define	FCALL_PRINTSTR(x) 
#else
#define	FCALL_PRINTSTR(x) \
	.rdata;98: .asciz x; .text; la a0, 98b; la v0, stringserial; addu v0,s0;jalr v0; nop	
#endif

#undef BAL
#define BAL(x) \
	la v0,x; \
	addu v0,s0; \
	jalr v0; \
	nop;

/* Delay macro */
#define	DELAY(count)	\
	li v0, count;	\
99:			\
	bnez	v0, 99b;\
	addiu	v0, -1

#define tmpsize		s1
#define msize		s2
#define	output_en	s3
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

#if defined(NAND_BOOT) && defined(LS1CSOC)
#include "nand_boot.S"
	la	t0, start
	la	t1, real_bin
	sub	t1, t0	/* t1 = start - real_bin */
	li	t0, DATA_BUFF
	add	t1, t0	/* t1 = (start - real_bin) + DATA_BUFF */
	jr	t1
	nop
real_bin:
#endif

	bal	uncached		/* Switch to uncached address space */
	nop

	bal	locate			/* Get current execute address */
	nop

uncached:
	or	ra, UNCACHED_MEMORY_ADDR
	j	ra
	nop

/***********************************EXC VECTOR***************************************/

/*
 *  Reboot vector usable from outside pmon.
 */
	.align	9
ext_map_and_reboot:
	li	a0,0x10000000 /*test from 0xbfcxxxxx or 0xff20xxxx */
	and	a0,ra
	bnez	a0,1f
	la	a0,_start
	li	s0,0xbfc00000
	subu	s0,a0
1:
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
	li	a0,0x10000000 /*test from 0xbfcxxxxx or 0xff20xxxx */
	and	a0,ra
	bnez	a0,1f
	la	a0,_start
	li	s0,0xbfc00000
	subu	s0,a0
1:
	la	a0, v380_msg
	bal	stringserial
	nop
	b	exc_common
	
	.align	9			/* bfc00400 */
	la	a0, v400_msg
	bal	stringserial
	nop
	b	exc_common

/*acpi: set ddr autorefresh and suspend */
	.align	7			/* bfc00480 */
	li	t0, 0xaffffe30
	lw	t1, 0x4(t0)
	li	t2, 0x1
	or	t1, t1, t2
	sw	t1, 0x4(t0)

	li	t0, 0xbfe7c008
	lw	t1, 0x0(t0)
	ori t1, t1, 0x2000
	sw	t1, 0x0(t0)
	
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

	PRINTSTR("\r\nBADADDR=")
	mfc0	a0, COP_0_BAD_VADDR
	bal	hexserial
	nop

//	bal mydebug_main
	nop
1:
	b 1b
	nop

	.align 9
	nop
	.align 8
	.word read
	.word write
	.word open
	.word close
	.word nullfunction
//	.word printf
//	.word vsprintf
	.word nullfunction
	.word nullfunction
	.word getenv
	.word nullfunction
	.word nullfunction
	.word nullfunction
	.word nullfunction

/****************************************LOCATE*********************************/

/*
 *  We get here from executing a bal to get the PC value of the current execute
 *  location into ra. Check to see if we run from ROM or if this is ramloaded.
 */
locate:
	la		s0, uncached
	subu	s0, ra, s0

	li		t0, SR_BOOT_EXC_VEC
	mtc0	t0, COP_0_STATUS_REG
	mtc0	zero, COP_0_CAUSE_REG
	.set	noreorder
	
	/*spi speed*/
	li  t0, 0xbfe80000
	li  t1, 0x17	// div 4, fast_read + burst_en + memory_en double I/O 模式 部分SPI flash可能不支持
	sb  t1, 0x4(t0)	// sfc_param
	li  t1, 0x05
	sb  t1, 0x6(t0)

#if defined(LS1ASOC)/* to adjust the DDR frequency */
	li v0, 0xbfc00000+(NVRAM_POS+PLL_OFFS)
//	li v0, 0xbfc80000+(NVRAM_POS+PLL_OFFS)	/* 注意：1A LPC Flash启动地址映射为0xbfc80000 */
	lw a1, (v0)

	li v0, 0xffff0000
	and v0, a1
	bnez v0, 1f
	nop

	li  v0, 0x8888
	or  a1, a1, v0

	b 2f
	nop
1:
#define DDRCFG_DATA (0x8888|(CPU_MULT-4)|((DDR_MULT-3)<<8))
    li  a1, DDRCFG_DATA
2:
	li  a2, 0xa
3:
    li  a0, 0xbfe78030
    sw  a1, 0x0(a0)
    nop
    sub  a2, 0x1
    bnez a2, 3b
    nop
//	lw	a2, 0x0(a0)
//	xor a2,a1
//	andi a2,0xf0f
//	bnez a2,2b
//	nop
#elif defined(LS1BSOC)
	li a0, 0xbfe78030
	/* 31:dc_div_en,30-26:dc_div,25:cpu_div_en,24-20:cpu_div,19:ddr_div_en,18-14:ddr_div */
	li v0, 0xbfc00000+(NVRAM_POS+PLL_OFFS)
	lw v1, 4(v0)
	lw a1, (v0)

	li v0, 0xfffc0000
	and v0, a1
	bnez v0, 1f
	nop

	andi v0, v1, 0x3f
	bnez v0, 1f
	nop

	li v0, (1<<31)|(1<<25)|(1<<19)
	and a2, v0, v1
	bne a2, v0, 1f
	nop

	beqz v1, 1f
	nop
	nop
	b 2f
	nop
1:
#ifdef CONFIG_VGA_MODEM
	/* ddr的分频在运行时切换会有出问题,需要重新锁定ddr的pll,
	   在使用vga显示时为了得到合适的刷新率，ddr的分频可能会被改变，所以这里设置成与改变的值一致，
	   避免出现问题 */
	li v1, (1<<31)|(4<<26)|(1<<25)|(2<<20)|(1<<19)|(3<<14)|0x2a00
	li a1, 0x14
#else
    li v1, (1<<31)|(4<<26)|(1<<25)|(3<<20)|(1<<19)|(4<<14)|0x2a00
    li a1, CPU_MULT
#endif
2:
	or v1, 0x2a00
	sw v1, 4(a0)
	sw a1, (a0);

#elif defined(LS1CSOC) && !defined(NAND_BOOT)
	/* config pll div for cpu and sdram */
	#define SDRAM_DIV_2	0x0
	#define SDRAM_DIV_3	0x2
	#define SDRAM_DIV_4	0x1
	#define SDRAM_PARAM_DIV_NUM		((1 << (SDRAM_DIV+1)) % 5)

	li	t0, 0xbfe78030
	
	li	v0, 0xbfc00000+(NVRAM_POS+PLL_OFFS)
	lw	t2, 0(v0)
	lw	t3, 4(v0)

	li v0, 0x00000000
	beq t2, v0, 1f
	nop
	li v0, 0xffffffff
	beq t2, v0, 1f
	nop

	li v0, 0x00000000
	beq t3, v0, 1f
	nop
	li v0, 0xffffffff
	beq t3, v0, 1f
	nop
	nop
	b 2f
	nop
1:
	/* 设置PLL倍频 及SDRAM分频 */
	li	t2, (0x80000008 | (PLL_MULT << 8) | (0x3 << 2) | SDRAM_DIV)
	/* 设置CPU分频 */
	li	t3, (0x00008003 | (CPU_DIV << 8))
2:
	/* 注意：首先需要把分频使能位清零 */
	li	t1, 0x2
	sw	t1, 0x4(t0)
	sw	t2, 0x0(t0)
	sw	t3, 0x4(t0)
	DELAY(2000)
#endif //#ifdef LS1ASOC

	/* initialize UART */
	li a0, 0
	bal	initserial
	nop

	/* 芯片上电默认使用gpio(输入模式）但大多时候是使用模块的功能，如lcd i2c spi ac97等
	   所以这里把gpio都关闭，方便使用模块功能。如果上电后需要gpio输出一个确定电平，
	   如继电器、LDE等，可以修改这里的代码。*/
	/* disable all gpio */
	li a0,0xbfd00000
	sw zero,0x10c0(a0)	/* disable gpio 0-31 */
	sw zero,0x10c4(a0)	/* disable gpio 32-63 */
	sw zero,0x10c8(a0)	/* disable gpio 64-95 */

#if defined(LS1ASOC)
	/* 启动时默认关闭SATA GPU USB GMAC0 GMAC1 */
	li a0,0xbfd00000
	lw a2,0x420(a0);
	xor a2,0x00500000	//关闭SATA GPU
	xor a2,0x01a00000	//关闭USB GMAC0 GMAC1
	sw a2,0x420(a0);
#elif defined(LS1BSOC)
	/* 启动时默认关闭USB GMAC0 GMAC1 */
	li a0,0xbfd00000
	lw a2,0x424(a0);
//	ori a2,0x3800
	ori a2,0x0800
	sw a2,0x424(a0);
#endif

#ifdef BUZZER
	/* disable buzzer gpio40*/
	li	v0, 0xbfd010c4
	lw  t0, 0x00(v0)
	or	t0, 0x00000100
	sw	t0, 0x00(v0)

	lw  t0, 0x10(v0)
	and	t0, 0xfffffeff
	sw	t0, 0x10(v0)

	lw  t0, 0x30(v0)
	and	t0, 0xfffffeff
	sw	t0, 0x30(v0)
#endif

	li output_en, 0x1
#ifdef FAST_STARTUP
	li a1, 0x03000000
	sw a1, 0x10c4(a0)
	sw a1, 0x10d4(a0)
	lw a2, 0x10e4(a0)
	and a2, a1
	beq a2, a1, get_pin_val_finish
	nop
	li output_en, 0x1

get_pin_val_finish:

#endif

	/* Initializing. Standby... */
	bnez s0, 1f
	nop
	li a0, 128
	jal initmips
	nop
1:

/* use only 8wins */
#define CPU_WIN_BASE 0xbfd00000
#define CPU_WIN_MASK 0xbfd00040
#define CPU_WIN_MMAP 0xbfd00080

#define set_cpu_window(id, base, mask, mmap) \
        li      t0, CPU_WIN_BASE          ;  \
        sw      $0, 0x80+id*8(t0)         ;  \
        li      t1, base                  ;  \
        sw      t1, 0x00+id*8(t0)         ;  \
        sw      $0, 0x04+id*8(t0)         ;  \
        li      t1, mask                  ;  \
        sw      t1, 0x40+id*8(t0)         ;  \
        sw      $0, 0x44+id*8(t0)         ;  \
        li      t1, mmap                  ;  \
        sw      t1, 0x80+id*8(t0)         ;  \
        sw      $0, 0x84+id*8(t0)

/* fixup cpu window */
cpu_win_fixup:
	//
	// hit         = (paddr & mask) == (mmap & mask)
	// mapped_addr =  paddr &~mask | mmap & mask
	//
	// mmap[7] -> enable
	// mmap[5] -> block trans enable
	// mmap[4] -> cachable
	// mmap[1:0] -> destination
	//
	// NOTE: the address windows has priority, win0 > win1 > ... > win7
#if defined(LS1ASOC)
//	set_cpu_window(0, 0x1fc00000, 0xfff00000, 0x1fc000f3) // boot rom
	set_cpu_window(0, 0x10000000, 0xf8000000, 0x100000d1) // PCI mem0, mem1
	set_cpu_window(1, 0x18000000, 0xfc000000, 0x180000d1) // PCI mem2
	set_cpu_window(2, 0x1c000000, 0xffe00000, 0x1c0000d1) // PCI cfg/IO/header
	set_cpu_window(3, 0x1c200000, 0xffe00000, 0x1c2000d2) // gpu 1c2 /dc 1c3
	set_cpu_window(4, 0x1f000000, 0xff000000, 0x1f0000d3) // AXIMUX
//	set_cpu_window(5, 0x40000000, 0xc0000000, 0x000000f0) // DDR 1GB
	set_cpu_window(5, 0x00000000, 0x00000000, 0x000000f0) // everything else
	set_cpu_window(6, 0x00000000, 0x00000000, 0x000000f0) // everything else
	set_cpu_window(7, 0x00000000, 0x00000000, 0x000000f0) // everything else
#elif defined(LS1BSOC)
	set_cpu_window(0, 0x1c300000, 0xfff00000, 0x1c3000d2) // dc       1M must cachable
	set_cpu_window(1, 0x1fe10000, 0xffffe000, 0x1fe100d3) // gmac0	8K
	set_cpu_window(2, 0x1fe20000, 0xffffe000, 0x1fe200d3) // gmac1	8K
	set_cpu_window(3, 0x1fe10000, 0xffff0000, 0x1fe100d0) // gmac0	64K
	set_cpu_window(4, 0x1fe20000, 0xffff0000, 0x1fe200d0) // gmac1	64K
	set_cpu_window(5, 0x1ff00000, 0xfff00000, 0x1ff000d0) // reserved 1M
	set_cpu_window(6, 0x1f000000, 0xff000000, 0x1f0000d3) // AXIMUX   16M
	set_cpu_window(7, 0x00000000, 0x00000000, 0x000000f0) // ddr 0
	li	t0, 0xbfd000e0
	lw	t1, 0x0(t0)	//0xbfd000e0
	and t1, t1, 0xffffff00
	ori	t1, 0xd0
	sw	t1, 0x0(t0)
	lw	t1, 0x8(t0)	//0xbfd000e8
	and t1, t1, 0xffffff00
	ori	t1, 0xd0
	sw	t1, 0x8(t0)
#elif defined(LS1CSOC)
/*	set_cpu_window(0, 0x1c280000, 0xfff80000, 0x1c280083) // camera 512K
	set_cpu_window(1, 0x1c300000, 0xfff00000, 0x1c300081) // dc 1M
	set_cpu_window(2, 0x1fe10000, 0xffffe000, 0x1fe10082) // gmac0	8K
	set_cpu_window(3, 0x1fe10000, 0xffff0000, 0x1fe100d0) // gmac0	64K
	set_cpu_window(4, 0x1f000000, 0xff000000, 0x1f000082) // AXIMUX   16M
	set_cpu_window(5, 0x00000000, 0x00000000, 0x000000f0) // ddr 0
	set_cpu_window(6, 0x00000000, 0x00000000, 0x000000f0) // ddr 0
	set_cpu_window(7, 0x00000000, 0x00000000, 0x000000f0) // ddr 0*/

	set_cpu_window(0, 0x1c280000, 0xfff80000, 0x1c2800d3) // camera
//	set_cpu_window(1, 0x1fc00000, 0xfff00000, 0x1fc000f2) //
	set_cpu_window(2, 0x1c300000, 0xfff00000, 0x1c3000d1) // dc 1M
//	set_cpu_window(3, 0x1f000000, 0xff000000, 0x1f0000d2) //
	set_cpu_window(4, 0x00000000, 0x00000000, 0x000000f0)
	set_cpu_window(5, 0x00000000, 0x00000000, 0x000000f0)
	set_cpu_window(6, 0x00000000, 0x00000000, 0x000000f0) // ddr 0
	set_cpu_window(7, 0x00000000, 0x00000000, 0x000000f0) // ddr 0
#endif
	// after this fixup, the kernel code should be compiled with
	// uncached instruction fetch patch

	/* 配置内存 */
#if defined(LS1ASOC) || defined(LS1BSOC)
	/*
	 * set *_ssel and *_tsel
	 * *_ssel参数用于配置DDR IO的驱动强度 01: 弱驱动 11: 强驱动
	 * *_tsel参数用于配置DDR IO的ODT输入匹配阻抗 00: disable 01: 75ohm 10: 150ohm 11: 50ohm
	 * pad_st不用于SSTL18模式，应保持为0
	 */
	li	t0, 0xbfd010c8
	li	t1, 0xfc000000
#ifdef LS1ASOC
	li	t1, 0x00000000
#endif
	sw	t1, (t0)
	li	t0, 0xbfd010f8
	li	t1, 0x14000000
	sw	t1, (t0)

	/* DDR2 config begin */
	bal     ddr2_config
	nop

	/* memory size defined in conf */
	li msize, MEM_SIZE

#ifdef CONFIG_DDR16BIT
	/*16bit ddr and disable conf*/ 
	#ifdef LS1ASOC
		li a1, 0x3
	#elif LS1BSOC
		li a1, 0x110000
	#endif
#else
	/*disable conf*/
	#ifdef LS1ASOC
		li a1, 0x2
	#elif LS1BSOC
		li a1, 0x100000
	#endif
#endif //#ifdef CONFIG_DDR16BIT

#ifdef LS1ASOC
	li a0, 0xbfd00420
#elif LS1BSOC
	li a0, 0xbfd00424
#endif
	lw a2, 0x0(a0)
	or a2, a1
	sw a2, 0x0(a0)

#elif defined(LS1CSOC)
	li msize, MEM_SIZE
#include "sdram_cfg.S"

	li  t1, 0xbfd00410
	li  a1, SD_PARA0
	sw  a1, 0x0(t1)
	li  a1, SD_PARA1
	sw  a1, 0x4(t1)
#endif
/**************************************CACHE*****************************/

#define CF_7_SE         (1 << 3)        /* Secondary cache enable */
#define CF_7_SC         (1 << 31)       /* Secondary cache not present */
#define CF_7_TE         (1 << 12)       /* Tertiary cache enable */
#define CF_7_TC         (1 << 17)       /* Tertiary cache not present */
#define CF_7_TS         (3 << 20)       /* Tertiary cache size */
#define CF_7_TS_AL      20              /* Shift to align */
#define NOP8 nop;nop;nop;nop;nop;nop;nop;nop

do_caches:
	/* Init caches... */
	li	s7, 0                   /* no L2 cache */
	li	s8, 0                   /* no L3 cache */

    bal     cache_init
    nop

	mfc0   a0, COP_0_CONFIG
	and    a0, a0, ~((1<<12) | 7)
	or     a0, a0, 2
	mtc0   a0, COP_0_CONFIG



#ifdef LS1ASOC
acpi_begin:

/* Access ACPI controller to check out if the machine is resuming from suspend 
   	   Zeng Lu <zenglu@loongson.cn> */
	li	t0,	0xbfe7c000
	lw	t1,(t0)
	and t1,t1,	(1<<15)//WAK_STS
	beqz t1,acpi_end
	nop
	/*clear wakeup events */
	li	t0, 0xbfe7c000
	li	t1, -1
	sw	t1, (t0)
	li	t0, 0xbfe7c020
	li	t1, -1
	sw	t1, (t0)

	li	t0,	0xbfe7c008
	lw	t1,	(t0)
#if 0 //clear slp_type if needed,this bit will be cleared in os
	li	t2, 0
	sw	t2, (t0)
#endif

	srl	t1,	10
	and	t1,	t1,	7//SLP_TYP
	sub	t1,	t1,	5	
	bnez	t1,	acpi_end	/* Resuming from suspend */
	nop

	li	t0,	0xa01ffc00
	li	t1,	0xa01ffc48
1:
	addiu t0,t0,4
	bne t0,t1,1b
	nop

	mfc0   a0,COP_0_CONFIG
	and    a0,a0,~((1<<12) | 3)
	or     a0,a0,2
	mtc0   a0,COP_0_CONFIG

	bal	CPU_TLBClear
	nop
	bal	suspend_resume
	nop
	/* Resume will never get here */
1:
	b	1b
	nop
acpi_end:/* Startup as usual */
#endif

/***********************MEMORY DEBUGGING AND COPY SELF TO RAM***********************/
//#include "newtest.32/mydebug.S"
bootnow:

/* copy copy program to sdram to make copy fast */
	la		t0, 121f
	addu	t0, s0
	la		t1, 122f
	addu	t1, s0
	
	li		t2, 0xa0000000
1:
	lw		v0, (t0)
	sw		v0, (t2)
	addu	t0, 4
	addu	t2, 4
	ble		t0, t1, 1b
	nop

	li		t0, 0xa0000000
   	jr		t0	
	nop		

.align 3
121: 
	/* Copy PMON to execute location... */
	la		a0, start
	addu	a1, a0, s0
	la		a2, _edata
	or		a0, 0xa0000000
	or		a2, 0xa0000000
	subu	t1, a2, a0
	srl		t1, t1, 2

	move	t0, a0
	move	t1, a1
	move	t2, a2

	/* copy text section */
1:	and		t3, t0,0x0000ffff
	bnez	t3, 2f
	nop
2:	lw		t3, 0(t1)
	nop
	sw		t3, 0(t0)
	addu	t0, 4
	addu	t1, 4
	bne		t2, t0, 1b
	nop
	/* copy text section done. */
	
	/* Clear BSS */
	la		a0, _edata
	la		a2, _end
2:	sw		zero, 0(a0)
	bne		a2, a0, 2b
	addu	a0, 4
	/* Copy PMON to execute location done */

	li		a0, 0
	sw		a0, CpuTertiaryCacheSize /* Set L3 cache size */

	move	a0,msize
	srl		a0,20

	la		v0, initmips
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

/*
 * Resume the CPU state, jump to the kernel
 */
LEAF(suspend_resume)
	li	t0,	0xa01ffc00
	lw	ra,	(t0)
	lw	sp,	4(t0)
	lw	s8,	8(t0)
	lw	gp,	12(t0)
	lw	s0,	16(t0)
	lw	s1,	20(t0)
	lw	s2,	24(t0)
	lw	s3,	28(t0)
	lw	s4,	32(t0)
	lw	s5,	36(t0)
	lw	s6,	40(t0)
	lw	s7,	44(t0)

	lw	k0,	48(t0)
	lw	k1,	52(t0)

	lw	v0,	56(t0)
	lw	v1,	60(t0)

	lw	t1,	64(t0)
	mtc0	t1,	$12
	lw	t1,	68(t0)
	mtc0	t1,	$4
	lw	t1,	72(t0)
	mtc0	t1,	$5

	jr	ra
	nop
END(suspend_resume)

LEAF(suspend_save)
	li	t0,	0xa01ffc00
#	sw	ra,	(t0)
	sw	sp,	4(t0)
	sw	s8,	8(t0)
	sw	gp,	12(t0)
	sw	s0,	16(t0)
	sw	s1,	20(t0)
	sw	s2,	24(t0)
	sw	s3,	28(t0)
	sw	s4,	32(t0)
	sw	s5,	36(t0)
	sw	s6,	40(t0)
	sw	s7,	44(t0)

	sw	k0,	48(t0)
	sw	k1,	52(t0)

	sw	v0,	56(t0)
	sw	v1,	60(t0)
                                
	mfc0	t1,	$12
	sw	t1,	64(t0)
	mfc0	t1,	$4
	sw	t1,	68(t0)
	mfc0	t1,	$5
	sw	t1,	72(t0)

	jr	ra
	nop
END(suspend_save)

/***********************************FUNCTIONS**********************************/

LEAF(stringserial)
	nop
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
	nop
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
	move	AT, ra
	beqz	output_en, 2f
	nop
	
	la	v0, COM1_BASE_ADDR
#ifdef	HAVE_MUT_COM
	bal	1f
	nop
	
	la	v0, COM3_BASE_ADDR
	bal	1f
	nop
	
	jr	AT
	nop
#endif
1:
	lbu	v1, NSREG(NS16550_LSR)(v0)
	and	v1, LSR_TXRDY
	beqz	v1, 1b
	nop

	sb	a0, NSREG(NS16550_DATA)(v0)
2:
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
	move	AT, ra

	beqz	output_en, 2f
	nop
	
	la	v0, COM1_BASE_ADDR
#ifdef	HAVE_MUT_COM
	bal	1f
	nop
	
	la	v0, COM3_BASE_ADDR
	bal	1f
	nop
	
	jr	AT
	nop
#endif
1:
	lbu	v1, NSREG(NS16550_LSR)(v0)
	and	v1, LSR_RXRDY
	beqz	v1, 1b
	nop
	lb	v0, NSREG(NS16550_DATA)(v0)
2:
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
#define B230400	230400
#define B380400	380400
#define B460800 460800
#define B921600	921600

LEAF(initserial)
	move AT,ra
	
	la	v0, COM1_BASE_ADDR
#ifdef	HAVE_MUT_COM
	bal	1f
	nop

	li	a0, 0
	la	v0, COM3_BASE_ADDR
	bal	1f
	nop

	jr	AT
	nop
#endif
1:
	li	v1, FIFO_ENABLE|FIFO_RCV_RST|FIFO_XMT_RST|FIFO_TRIGGER_4
	sb	v1, NSREG(NS16550_FIFO)(v0)
	li	v1, CFCR_DLAB
	sb	v1, NSREG(NS16550_CFCR)(v0)
#if defined(LS1ASOC)
//	li	v1, ((APB_CLK*DDR_MULT)/(2*16*CONS_BAUD))     // 8MHz crystal,   M[7:3]=6     1fboard
//	li	a0, 0xbfe78030
//	lw	a0, (a0)
	move	a0, a1
	and		a0, 0x700
	srl		a0, 8
	addiu	a0, 3
	li		v1, APB_CLK
	multu	a0, v1
	mflo	v1
	li		a0, 2*16*CONS_BAUD
	divu	v1, a0
	mflo	v1
#elif defined(LS1BSOC)
	move	v1, a0
	bnez	v1, 2f
	nop
	li		v1, APB_CLK
	li		a0, 0xbfe78030
	lw		a1, 4(a0)
	li		a2, 0xc00
	and		a1, a2
	beq		a1, a2,2f
	nop
	lw		a1, (a0)
	andi	a2, a1, 0x3f
	addiu	a2, 12
	sll		a2, 10
	srl		a1, 8
	andi	a1, 0x3ff
	addu	a1, a2
	li		a2, (APB_CLK>>11)
	multu	a1, a2
	mflo	v1
	lw		a1, 4(a0)
	srl		a1, 14
	andi	a2, a1, 0x20
	beqz	a2, 1f
	nop
	andi	a1, 0x1f
	divu	v1, a1
	mflo	v1
	b 2f
	nop
1:
	srl		v1, 1	
2:
	li		a1, 2*16*CONS_BAUD
	divu	v1, v1, a1
#elif defined(LS1CSOC)
	/* uart3 config mux 默认第一复用 */
	li		a0, 0xbfd011c4
//	lw		a1, 0x00(a0)
//	and		a1, 0xfffffff9
//	sw		a1, 0x00(a0)
	lw		a1, 0x10(a0)
	ori		a1, 0x06
	sw		a1, 0x10(a0)
//	lw		a1, 0x20(a0)
//	and		a1, 0xfffffff9
//	sw		a1, 0x20(a0)
//	lw		a1, 0x30(a0)
//	and		a1, 0xfffffff9
//	sw		a1, 0x30(a0)

/*	li		a0, 0xbfd011f0
	lw		a1, 0x00(a0)
	ori		a1, 0x03
	sw		a1, 0x00(a0)*/

	/* UART2 */
/*	li		a0, 0xbfd011c4
	lw		a1, 0x10(a0)
	ori		a1, 0x30
	sw		a1, 0x10(a0)*/

/*	li		a0, 0xbfd011f0
	lw		a1, 0x00(a0)
	ori		a1, 0x03
	sw		a1, 0x00(a0)*/

	li		a0, 0xbfe78030
	lw		a1, 0(a0)
	andi	a2, a1, 0x3
	addiu	a2, 1			//sdram_div+1
	li		t1, 1
	sll		t1, a2			//1<<(sdram_div+1)
	li		t2, 5
	remu	t1, t2			//(1<<(sdram_div+1))%5
	srl		a1, 8
	andi	a1, 0xff
	li		a2, APB_CLK
	srl		a2, 2			//APB_CLK/4
	multu	a1, a2
	mflo	v1				//APB_CLK/4 * PLL_MULT
	divu	v1, t1
	mflo	v1				//APB_CLK/4 * PLL_MULT / (1<<(sdram_div+1))%5

	lw		a1, 4(a0)
	andi	a2, a1, DIV_CPU_SEL
	bnez	a2, 1f
	nop
	li		v1, APB_CLK
	b		3f
	nop
1:
	andi	a2, a1, DIV_CPU_EN
	bnez	a2, 2f
	nop
	srl		v1, 1			//APB_CLK/4 * PLL_MULT / (1<<(sdram_div+1))%5 / 2
	b		3f
	nop
2:
	andi	a1, DIV_CPU
	srl		a1, DIV_CPU_SHIFT
	divu	v1, a1
	mflo	v1				//APB_CLK/4 * PLL_MULT / (1<<(sdram_div+1))%5 / CPU_DIV
3:
//	li	v1, ((APB_CLK / 4) * (PLL_MULT / CPU_DIV)) / SDRAM_PARAM_DIV_NUM / (16*CONS_BAUD)
	li		a1, 16*CONS_BAUD
	divu	v1, v1, a1
#endif
	sb	v1, NSREG(NS16550_DATA)(v0)
	srl	v1, 8
	sb	v1, NSREG(NS16550_IER)(v0)
	li	v1, CFCR_8BITS
	sb	v1, NSREG(NS16550_CFCR)(v0)
	li	v1, MCR_DTR|MCR_RTS
	sb	v1, NSREG(NS16550_MCR)(v0)
	li	v1, 0x0
	sb	v1, NSREG(NS16550_IER)(v0)
        
	#disable all interrupt
	li  v1, 0x0
	sb  v1, NSREG(NS16550_IER)(v0)
	j   ra
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

#define Index_Store_Tag_D			0x09
#define Index_Invalidate_I			0x00
#define Index_Writeback_Inv_D		0x01


/*************lxy add this *************/
	.ent	CPU_DisableCache
	.global CPU_DisableCache
	.set noreorder
	.set mips32
CPU_DisableCache:
	mfc0   	t0, CP0_CONFIG, 0
	and   	t0, 0xfffffff8
	or	t0, 0x2
	mtc0   	t0, CP0_CONFIG, 0
	jr	ra
   	nop
	.set reorder
	.end 	CPU_DisableCache
/***************lxy*******************/

LEAF(nullfunction)
	jr ra
	nop
END(nullfunction)

	.ent		cache_init
	.global	cache_init
	.set		noreorder
cache_init:
	move t1, ra
####part 2####
cache_detect_4way:
	.set	mips32
	mfc0	t4, CP0_CONFIG,1
	lui		v0, 0x7		//v0 = 0x7 << 16
	and		v0, t4, v0    
	srl		t3, v0, 16    //t3 = v0 >> 16  Icache组相联数 IA

	li		t5, 0x800 		//32*64
	srl		v1, t4,22		//v1 = t4 >> 22
	andi	v1, 7			//Icache每路的组数 64x2^S IS
	sll		t5, v1			//InstCacheSetSize
	sll		t5, t3			//t5 InstCacheSize


	andi	v0, t4, 0x0380
	srl		t7, v0, 7		//DA

	li		t6, 0x800 #32*64
	srl		v1, t4,13
	andi	v1, 7			//DS
	sll		t6, v1  #DataCacheSetSize
	sll		t6, t7  #t5 DataCacheSize

####part 3####
#	.set	mips3
	lui		a0, 0x8000			//a0 = 0x8000 << 16
	addu	a1, $0, t5
	addu	a2, $0, t6
cache_init_d2way:
/******************************/	//lxy
//	addiu	t3, t3, 1
//	li	t4, 0
//5:
/******************************/
#a0=0x80000000, a1=icache_size, a2=dcache_size
#a3, v0 and v1 used as local registers
	mtc0	$0, CP0_TAGHI
	addu	v0, $0, a0		//v0 = 0 + a0
	addu	v1, a0, a2		//v1 = a0 + a2
1:	slt		a3, v0, v1		//a3 = v0 < v1 ? 1 : 0
	beq		a3, $0, 1f		//if (a3 == 0) goto 1f
	nop
	mtc0	$0, CP0_TAGLO
	cache	Index_Store_Tag_D, 0x0(v0)	# 1 way
4:	beq		$0, $0, 1b
	addiu	v0, v0, 0x20
1:
cache_flush_i2way:
	addu	v0, $0, a0
	addu	v1, a0, a1
1:	slt		a3, v0, v1
	beq		a3, $0, 1f
	nop
	cache	Index_Invalidate_I, 0x0(v0)	# 1 way
4:	beq		$0, $0, 1b
	addiu	v0, v0, 0x20
1:
cache_flush_d2way:
	addu	v0, $0, a0
	addu	v1, a0, a2
1:	slt		a3, v0, v1
	beq		a3, $0, 1f
	nop
	cache	Index_Writeback_Inv_D, 0x0(v0) 	#1 way
4:	beq		$0, $0, 1b
	addiu	v0, v0, 0x20
/******************************/	//lxy
//	addiu	t4, t4, 1
//	addiu	a0, a0, 1
//	slt		t5, t4, t3
//	bne		t5, $0, 5b
//	nop
/******************************/
//	.set	mips0

1:
cache_init_finish:
	jr	t1
	nop
	.set	reorder
	.end	cache_init

#ifdef LS1ASOC
#include "ddr2fconfig-1a-samsung-CL3.S"
#elif LS1BSOC
//#include "ddr2fconfig-samsung.S"
#include "ddr2fconfig-samsung-CL3.S"
//#include "ddr2fconfig.S"
#endif


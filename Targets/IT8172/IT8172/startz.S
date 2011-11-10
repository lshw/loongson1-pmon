/*	$Id: startz.S,v 1.1.1.1 2006/09/14 01:59:09 root Exp $ */

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
#include <target/it8172.h>



#define IndexInvalidate_I       0x00
#define IndexWriteBack_D        0x01

	.set	noreorder

	.globl	_start
	.globl	start
_start:
start:
	bal	INIT_GODSON_MEM
	nop
	bal	locate
	nop
locate:
	subu	s8, ra, 0x10	/* Where we were loaded */
	la	sp, (.stack + 8192)

	move	s0, a0		/* Save boot rom start args */

	la	a0, start	/* Where we were linked to run */

	move	a1, s8
	la	a2, _edata
	subu	t1, a2, a0
	srl	t1, t1, 2

	/* copy text section */
	li	t0, 0
1:	lw	v0, 0(a1)
	nop
	sw	v0, 0(a0)
	xor	t0, t0, v0
	addu	a0, 4
	bne	a2, a0, 1b
	addu	a1, 4

	
	/* Clear BSS */
	la	a0, _edata
	la	a2, _end
2:	sw	zero, 0(a0)
	bne a2, a0, 2b
	addu	a0, 4

	move	a0, s8		     /* load address */
	move	a1, t1               /* length in words */
	move	a2, t0               /* checksum */
	move	a3, sp

	la	ra, 1f
	la	k0, decompress_pmon
	jr	k0
	nop
1:

	move	a0, s0
	li	k0, PMON_ENTRY
	jr	k0
	nop
3:
	b 3b

	.comm .stack,4096*2,4

LEAF(INIT_GODSON_MEM)
       	mfc0	   t1, COP_0_CONFIG                  # Get Config Reg
	nop
	and        t1, t1, ~0x3
	or         t1, t1, 2
       	mtc0	   t1, COP_0_CONFIG                  # kseg0 uncached

        mfc0    t1, COP_0_STATUS_REG           
        mtc0    zero, COP_0_CAUSE_REG          
        and             t1, SR_SOFT_RESET       #
        or              t1, SR_BOOT_EXC_VEC      # Should be omited because it has been done before
        mtc0    t1, COP_0_STATUS_REG           # Set STATUS Reg
	li	t1, 0xa000038c
	mtc0	t1, COP_0_ERROR_PC
        nop

#       li      t9, 0x03        /* 256Mb x 2 on cs0*/ /* 64M*/
        li      t9, 0x1B        # NC-it8172G on-board DIMM
#       li      t9, 0x12        /* 128Mb x 2 on cs0 cs1 */ /*32M*/

        sw      t9, ( IT_MC_SDTYPE + IT8172_BASE + KSEG1 )


	li      t9, 0x0300              # 4mA
        sh      t9, (UNCACHED_MEMORY_ADDR + IT8172_BASE + IT_PCDCR)
        nop

        li      t9, 0x803B              # cas=3, burst length=8
        sh      t9, (UNCACHED_MEMORY_ADDR + IT8172_BASE + IT_MC_SDRMR)
        nop


        li      t9, 0x01ff
        sh      t9, (UNCACHED_MEMORY_ADDR + IT8172_BASE + IT_MC_SDRTR)
        nop


        li      t9, 0x1                 # open board
        sh      t9, (UNCACHED_MEMORY_ADDR + IT8172_BASE + IT_CMFPCR)    # change IT_CMFPCR
        nop

	jr	ra
	nop
END(INIT_GODSON_MEM)

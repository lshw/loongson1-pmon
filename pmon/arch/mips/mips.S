/*	$Id: mips.S,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2000 Opsycon AB  (www.opsycon.se)
 * Copyright (c) 2000 Rtmx, Inc   (www.rtmx.com)
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
 *	This product includes software developed for Rtmx, Inc by
 *	Opsycon Open System Consulting AB, Sweden.
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
#define DEBUG

#ifndef _KERNEL
#define _KERNEL
#endif

#include <asm.h>
#include <regnum.h>
#include <cpu.h>

#if __mips < 3
#define STORE   sw      /* 32 bit mode regsave instruction */
#define LOAD    lw      /* 32 bit mode regload instruction */
#define	FSTORE	swc1	/* 32 bit mode float register store */
#define	FLOAD	lwc1	/* 32 bit mode float register load */
#define RSIZE   4       /* 32 bit mode register size */
#define MTC0	mtc0
#define MFC0	mfc0
#else
#define STORE   sd      /* 64 bit mode regsave instruction */
#define LOAD    ld      /* 64 bit mode regload instruction */
#define	FSTORE	sdc1	/* 32 bit mode float register store */
#define	FLOAD	ldc1	/* 32 bit mode float register load */
#define RSIZE   8       /* 64 bit mode register size */
#define MTC0	dmtc0
#define MFC0	dmfc0

#endif
#define	NOP8	nop;nop;nop;nop;nop;nop;nop;nop

	.set	noreorder

	.data
/*  Register area outgoing */
	.global DBGREG
	.common DBGREG, 128*8

/*  return value and jump buffer used when exiting program returns. */
	.global	retvalue
retvalue:
	.word	0
	.globl	go_return_jump
	.common go_return_jump, 12*8

/*
 *  Exception trampoline copied down to RAM after initialization.
 */
	.text
	.globl	MipsException
	.globl	MipsExceptionEnd
MipsException:
	.set	noat
	la	k0, exception_handler
	jr	k0
	nop
	.set	at
MipsExceptionEnd:

/*
 *  Restore register state and transfer to new PC value.
 */
LEAF(_go)
	li	v0, SR_EXL		/* Precaution */
	mtc0	v0, COP_0_STATUS_REG

	la	k0, DBGREG

	LOAD	t0, WATCHLO * RSIZE(k0)
	LOAD	t1, WATCHHI * RSIZE(k0)
	MTC0	t0, COP_0_WATCH_LO
	mfc0	t0, COP_0_PRID
	MTC0	t1, COP_0_WATCH_HI
	srl	t1, t0, 8
	bne	t1, MIPS_RM7000, 1f
	LOAD	t0, WATCH1 * RSIZE(k0)
	
	/* RM7000 specific */
	LOAD	t1, WATCH2 * RSIZE(k0)
	mtc0	t0, COP_0_WATCH_1
	LOAD	t0, WATCHM * RSIZE(k0)
	mtc0	t1, COP_0_WATCH_2
	LOAD	t1, PCOUNT * RSIZE(k0)
	mtc0	t0, COP_0_WATCH_M
	LOAD	t0, PCTRL * RSIZE(k0)
	mtc0	t1, COP_0_PC_COUNT
	LOAD	t1, ICR * RSIZE(k0)
	mtc0	t0, COP_0_PC_CTRL
	mtc0	t1, COP_0_ICR

1:
	LOAD	v0, PC * RSIZE(k0)
	LOAD	v1, CAUSE * RSIZE(k0)
	MTC0	v0, COP_0_EXC_PC
	LOAD	v0, SR * RSIZE(k0)
	mtc0	v1, COP_0_CAUSE_REG
	or	v0, SR_EXL		/* Keep Exeption level status */
	mtc0	v0, COP_0_STATUS_REG

	LOAD	v0, MULHI * RSIZE(k0)
	LOAD	v1, MULLO * RSIZE(k0)
	mthi	v0
	mtlo	v1

	.set	noat

	LOAD	AT, AST * RSIZE(k0)
	LOAD	v0, V0 * RSIZE(k0)
	LOAD	v1, V1 * RSIZE(k0)
	LOAD	a0, A0 * RSIZE(k0)
	LOAD	a1, A1 * RSIZE(k0)
	LOAD	a2, A2 * RSIZE(k0)
	LOAD	a3, A3 * RSIZE(k0)
	LOAD	t0, T0 * RSIZE(k0)
	LOAD	t1, T1 * RSIZE(k0)
	LOAD	t2, T2 * RSIZE(k0)
	LOAD	t3, T3 * RSIZE(k0)
	LOAD	t4, T4 * RSIZE(k0)
	LOAD	t5, T5 * RSIZE(k0)
	LOAD	t6, T6 * RSIZE(k0)
	LOAD	t7, T7 * RSIZE(k0)
	LOAD	s0, S0 * RSIZE(k0)
	LOAD	s1, S1 * RSIZE(k0)
	LOAD	s2, S2 * RSIZE(k0)
	LOAD	s3, S3 * RSIZE(k0)
	LOAD	s4, S4 * RSIZE(k0)
	LOAD	s5, S5 * RSIZE(k0)
	LOAD	s6, S6 * RSIZE(k0)
	LOAD	s7, S7 * RSIZE(k0)
	LOAD	t8, T8 * RSIZE(k0)
	LOAD	t9, T9 * RSIZE(k0)
	LOAD	k1, K1 * RSIZE(k0)
	LOAD	gp, GP * RSIZE(k0)
	LOAD	sp, SP * RSIZE(k0)
	LOAD	s8, S8 * RSIZE(k0)
	LOAD	ra, RA * RSIZE(k0)
	LOAD	k0, K0 * RSIZE(k0)

	.set	at
	.set push
	.set mips3
	eret
	.set pop
END(_go)

/*
 *  Top return address set to this func so a program returning
 *  is catched and control gracefully passed to PMON2000.
 */
LEAF(_exit)
	sw	v0, retvalue
	la	a0, go_return_jump
	jal	longjmp
	nop
END(_exit)


/*
 *  Main exception handler. Not really a leaf routine but not a normal
 *  function either. Save away the entire cpu state end enter exception mode.
 */
LEAF(exception_handler)
	.set	noat
	la	k0, start - 1024

	STORE	AT, AST * RSIZE(k0)
	STORE	v0, V0 * RSIZE(k0)
	STORE	v1, V1 * RSIZE(k0)
	STORE	a0, A0 * RSIZE(k0)
	STORE	a1, A1 * RSIZE(k0)
	STORE	a2, A2 * RSIZE(k0)
	STORE	a3, A3 * RSIZE(k0)
	STORE	t0, T0 * RSIZE(k0)
	STORE	t1, T1 * RSIZE(k0)
	STORE	t2, T2 * RSIZE(k0)
	STORE	t3, T3 * RSIZE(k0)
	STORE	t4, T4 * RSIZE(k0)
	STORE	t5, T5 * RSIZE(k0)
	STORE	t6, T6 * RSIZE(k0)
	STORE	t7, T7 * RSIZE(k0)
	STORE	s0, S0 * RSIZE(k0)
	STORE	s1, S1 * RSIZE(k0)
	STORE	s2, S2 * RSIZE(k0)
	STORE	s3, S3 * RSIZE(k0)
	STORE	s4, S4 * RSIZE(k0)
	STORE	s5, S5 * RSIZE(k0)
	STORE	s6, S6 * RSIZE(k0)
	STORE	s7, S7 * RSIZE(k0)
	STORE	t8, T8 * RSIZE(k0)
	STORE	t9, T9 * RSIZE(k0)
	STORE	k1, K1 * RSIZE(k0)
	STORE	gp, GP * RSIZE(k0)
	STORE	sp, SP * RSIZE(k0)
	STORE	s8, S8 * RSIZE(k0)
	STORE	ra, RA * RSIZE(k0)

	.set	at

	mfhi	v0
	mflo	v1
	STORE	v0, MULHI * RSIZE(k0)
	STORE	v1, MULLO * RSIZE(k0)

	mfc0	a0, COP_0_STATUS_REG
	mfc0	v1, COP_0_CAUSE_REG
	STORE	a0, SR * RSIZE(k0)
	MFC0	v0, COP_0_BAD_VADDR
	STORE	v1, CAUSE * RSIZE(k0)
	MFC0	v1, COP_0_EXC_PC
	STORE	v0, BADVADDR * RSIZE(k0)
	STORE	v1, PC * RSIZE(k0)

	MFC0	v0, COP_0_TLB_CONTEXT
	MFC0	v1, COP_0_TLB_XCONTEXT
	STORE	v0, CONTX * RSIZE(k0)
	STORE	v1, XCONTX * RSIZE(k0)
	MFC0	v0, COP_0_TLB_HI
	MFC0	v1, COP_0_TLB_LO0
	STORE	v0, ENTHI * RSIZE(k0)
	MFC0	v0, COP_0_TLB_LO1
	STORE	v1, ENTLO0 * RSIZE(k0)
	STORE	v0, ENTLO1 * RSIZE(k0)

	MFC0	t0, COP_0_WATCH_LO
	MFC0	t1, COP_0_WATCH_HI
	STORE	t0, WATCHLO * RSIZE(k0)
	MFC0	t0, COP_0_PRID
	STORE	t1, WATCHHI * RSIZE(k0)
	MTC0	zero, COP_0_WATCH_LO
	MTC0	zero, COP_0_WATCH_HI
	srl	t1, t0, 8
	bne	t1, MIPS_RM7000, 1f
	STORE	t0, PRID * RSIZE(k0)

	mfc0	t0, COP_0_WATCH_1
	mfc0	t1, COP_0_WATCH_2
	STORE	t0, WATCH1 * RSIZE(k0)
	mfc0	t0, COP_0_WATCH_M
	STORE	t1, WATCH2 * RSIZE(k0)
	mfc0	t1, COP_0_PC_COUNT
	STORE	t0, WATCHM * RSIZE(k0)
	mfc0	t0, COP_0_PC_CTRL
	STORE	t1, PCOUNT * RSIZE(k0)
	mfc0	t1, COP_0_ICR
	STORE	t0, PCTRL * RSIZE(k0)
	STORE	t1, ICR * RSIZE(k0)

1:
	addu	sp, k0, -64		/* Get a new stack */

	and     t0, a0, ~(SR_EXL | SR_KSU_MASK | SR_INT_ENAB)
	mtc0	t0, COP_0_STATUS_REG
	NOP8
	la	gp, _gp

	jal	exception
	move	a0, k0
	/*NORETURN*/
END(exception_handler)

/*
 *  Save/restore floating point registers.
 *  Call with a0 pointing at frame.
 */
LEAF(md_fpsave)
	mfc0	t1, COP_0_STATUS_REG
	or	v0, t1, SR_COP_1_BIT|SR_FR_32
	mtc0	v0, COP_0_STATUS_REG
	NOP8

	cfc1	t0, FPC_CSR		/* drain FP pipeline */
	cfc1	t0, FPC_CSR		/* get updated status... */

	FSTORE	$f0, F0 * RSIZE(a0)
	FSTORE	$f1, F1 * RSIZE(a0)
	FSTORE	$f2, F2 * RSIZE(a0)
	FSTORE	$f3, F3 * RSIZE(a0)
	FSTORE	$f4, F4 * RSIZE(a0)
	FSTORE	$f5, F5 * RSIZE(a0)
	FSTORE	$f6, F6 * RSIZE(a0)
	FSTORE	$f7, F7 * RSIZE(a0)
	FSTORE	$f8, F8 * RSIZE(a0)
	FSTORE	$f9, F9 * RSIZE(a0)
	FSTORE	$f10, F10 * RSIZE(a0)
	FSTORE	$f11, F11 * RSIZE(a0)
	FSTORE	$f12, F12 * RSIZE(a0)
	FSTORE	$f13, F13 * RSIZE(a0)
	FSTORE	$f14, F14 * RSIZE(a0)
	FSTORE	$f15, F15 * RSIZE(a0)
	FSTORE	$f16, F16 * RSIZE(a0)
	FSTORE	$f17, F17 * RSIZE(a0)
	FSTORE	$f18, F18 * RSIZE(a0)
	FSTORE	$f19, F19 * RSIZE(a0)
	FSTORE	$f20, F20 * RSIZE(a0)
	FSTORE	$f21, F21 * RSIZE(a0)
	FSTORE	$f22, F22 * RSIZE(a0)
	FSTORE	$f23, F23 * RSIZE(a0)
	FSTORE	$f24, F24 * RSIZE(a0)
	FSTORE	$f25, F25 * RSIZE(a0)
	FSTORE	$f26, F26 * RSIZE(a0)
	FSTORE	$f27, F27 * RSIZE(a0)
	FSTORE	$f28, F28 * RSIZE(a0)
	FSTORE	$f29, F29 * RSIZE(a0)
	FSTORE	$f30, F30 * RSIZE(a0)
	FSTORE	$f31, F31 * RSIZE(a0)
	STORE	t0, FSR * RSIZE(a0)

	mtc0	t1, COP_0_STATUS_REG

	mfc0	t0, COP_0_TLB_RANDOM
	mfc0	t1, COP_0_TLB_INDEX
	STORE	t0, RANDOM * RSIZE(a0)
	mfc0	t0, COP_0_TLB_LO0
	STORE	t1, INDEX * RSIZE(a0)
	mfc0	t1, COP_0_TLB_LO1
	STORE	t0, ENTLO0 * RSIZE(a0)
	mfc0	t0, COP_0_TLB_CONTEXT
	STORE	t1, ENTLO1 * RSIZE(a0)
	mfc0	t1, COP_0_TLB_PG_MASK
	STORE	t0, CONTX * RSIZE(a0)
	mfc0	t0, COP_0_TLB_WIRED
	STORE	t1, PGMSK * RSIZE(a0)
	mfc0	t1, COP_0_COUNT
	STORE	t0, WIRED * RSIZE(a0)
	mfc0	t0, COP_0_TLB_HI
	STORE	t1, COUNT * RSIZE(a0)
	mfc0	t1, COP_0_COMPARE
	STORE	t0, ENTHI * RSIZE(a0)
	mfc0	t0, COP_0_CONFIG
	STORE	t1, COMPARE * RSIZE(a0)
	mfc0	t1, COP_0_LLADDR
	STORE	t0, CONFIG * RSIZE(a0)
	mfc0	t0, COP_0_TLB_XCONTEXT
	STORE	t1, LLADR * RSIZE(a0)
	mfc0	t1, COP_0_ECC
	STORE	t0, XCONTX * RSIZE(a0)
	mfc0	t0, COP_0_CACHE_ERR
	STORE	t1, ECC * RSIZE(a0)
	mfc0	t1, COP_0_TAG_LO
	STORE	t0, CACHER * RSIZE(a0)
	mfc0	t0, COP_0_TAG_HI
	STORE	t1, TAGLO * RSIZE(a0)
	mfc0	t1, COP_0_ERROR_PC
	STORE	t0, TAGHI * RSIZE(a0)
	STORE	t1, ERRPC * RSIZE(a0)

	j	ra
	nop
END(md_fpsave)

LEAF(md_fprestore)
	mfc0	t1, COP_0_STATUS_REG
	or	v0, t1, SR_COP_1_BIT|SR_FR_32
	mtc0	v0, COP_0_STATUS_REG
	NOP8

	cfc1	t0, FPC_CSR		/* drain FP pipeline */

	FLOAD	$f0, F0 * RSIZE(a0)
	FLOAD	$f1, F1 * RSIZE(a0)
	FLOAD	$f2, F2 * RSIZE(a0)
	FLOAD	$f3, F3 * RSIZE(a0)
	FLOAD	$f4, F4 * RSIZE(a0)
	FLOAD	$f5, F5 * RSIZE(a0)
	FLOAD	$f6, F6 * RSIZE(a0)
	FLOAD	$f7, F7 * RSIZE(a0)
	FLOAD	$f8, F8 * RSIZE(a0)
	FLOAD	$f9, F9 * RSIZE(a0)
	FLOAD	$f10, F10 * RSIZE(a0)
	FLOAD	$f11, F11 * RSIZE(a0)
	FLOAD	$f12, F12 * RSIZE(a0)
	FLOAD	$f13, F13 * RSIZE(a0)
	FLOAD	$f14, F14 * RSIZE(a0)
	FLOAD	$f15, F15 * RSIZE(a0)
	FLOAD	$f16, F16 * RSIZE(a0)
	FLOAD	$f17, F17 * RSIZE(a0)
	FLOAD	$f18, F18 * RSIZE(a0)
	FLOAD	$f19, F19 * RSIZE(a0)
	FLOAD	$f20, F20 * RSIZE(a0)
	FLOAD	$f21, F21 * RSIZE(a0)
	FLOAD	$f22, F22 * RSIZE(a0)
	FLOAD	$f23, F23 * RSIZE(a0)
	FLOAD	$f24, F24 * RSIZE(a0)
	FLOAD	$f25, F25 * RSIZE(a0)
	FLOAD	$f26, F26 * RSIZE(a0)
	FLOAD	$f27, F27 * RSIZE(a0)
	FLOAD	$f28, F28 * RSIZE(a0)
	FLOAD	$f29, F29 * RSIZE(a0)
	FLOAD	$f30, F30 * RSIZE(a0)
	FLOAD	$f31, F31 * RSIZE(a0)
	LOAD	t0, FSR * RSIZE(a0)
	ctc1	t0, FPC_CSR

	mtc0	t1, COP_0_STATUS_REG
	NOP8

	j	ra
	nop
END(md_fprestore)

/*
 *  Get CP0 count register value
 */
LEAF(CPU_GetCOUNT)
	mfc0	v0, COP_0_COUNT
	nop
	j	ra
	nop
END(CPU_GetCOUNT)

LEAF(CPU_SetCOUNT)
	mtc0	a0, COP_0_COUNT
	nop
	j	ra
	nop
END(CPU_GetCOUNT)
/*
 *  Modify SR value, arg 1 = set bits, arg 2 = clear bits.
 */
LEAF(CPU_SetSR)
	mfc0	v0, COP_0_STATUS_REG
	not	v1, a1
	and	v1, v0
	or	v1, a0
	mtc0	v1, COP_0_STATUS_REG
	NOP8
	j	ra
	nop
END(CPU_GetCOUNT)

/*
 *  Get configuration register contents.
 */
LEAF(CPU_GetCONFIG)
	mfc0	v0, COP_0_CONFIG
	j	ra
	nop
END(CPU_GetCONFIG)
/*
 *  Helper routine to move a quad word in one operation.
 */
LEAF(movequad)
	LOAD	v0, 0(a1)
	STORE	v0, 0(a0)
	j	ra
	nop
END(movequad)

/*
 *  Execute 1000000 loops.
 */
LEAF(loopforameg)
	li	v0, 1000000
1:
	bnez	v0, 1b
	addiu	v0, -1

	j	ra
	nop
END(loopforameg)

/*
 *  Return CPU type.
 */
LEAF(md_cputype)
	mfc0	v0, COP_0_PRID
	j	ra
	nop
END(md_cputype)


/*
 *  execute a break instruction.
 */
LEAF(_pmon_break)
	break	0

	j	ra
	nop
END(_pmon_break)

/*
 *      Block I/O routines mainly used by I/O drivers.
 *
 *      Args as:        a0 = port
 *                      a1 = memory address
 *                      a2 = count
 */
LEAF(insb)
        beq     a2, zero, 2f
        addu	a2, a1
1:
        lbu     v0, 0(a0)
        addu	a1, 1
        bne     a1, a2, 1b
        sb      v0, -1(a1)
2:
        jr      ra
        nop
END(insb)

LEAF(insw)
        beq     a2, zero, 2f
        addu	a2, a2
        addu	a2, a1
1:
        lhu     v0, 0(a0)
        addu	a1, 2
        bne     a1, a2, 1b
        sh      v0, -2(a1)
2:
        jr      ra
        nop
END(insw)

LEAF(insl)
        beq     a2, zero, 2f
        sll	a2, 2
        addu	a2, a1
1:
        lw      v0, 0(a0)
        addu	a1, 4
        bne     a1, a2, 1b
        sw      v0, -4(a1)
2:
        jr      ra
        nop
END(insl)

LEAF(outsb)
        beq     a2, zero, 2f
        addu	a2, a1
1:
        lbu     v0, 0(a1)
        addu	a1, 1
        bne     a1, a2, 1b
        sb      v0, 0(a0)
2:
        jr      ra
        nop
END(outsb)

LEAF(outsw)
        beq     a2, zero, 2f
        addu	a2, a2
        li      v0, 1
        and     v0, a1
        bne     v0, zero, 3f            # arghh, unaligned.
        addu	a2, a1
1:
        lhu     v0, 0(a1)
        addu	a1, 2
        bne     a1, a2, 1b
        sh      v0, 0(a0)
2:
        jr      ra
        nop
3:
        LWHI    v0, 0(a1)
        LWLO    v0, 3(a1)
        addu	a1, 2
        bne     a1, a2, 3b
        sh      v0, 0(a0)

        jr      ra
        nop
END(outsw)

LEAF(outsl)
        beq     a2, zero, 2f
        sll	a2, 2
        li      v0, 3
        and     v0, a1
        bne     v0, zero, 3f            # arghh, unaligned.
        addu	a2, a1
1:
        lw      v0, 0(a1)
        addu	a1, 4
        bne     a1, a2, 1b
        sw      v0, 0(a0)
2:
        jr      ra
        nop
3:
        LWHI    v0, 0(a1)
        LWLO    v0, 3(a1)
        addu	a1, 4
        bne     a1, a2, 3b
        sw      v0, 0(a0)

        jr      ra
        nop
END(outsl)


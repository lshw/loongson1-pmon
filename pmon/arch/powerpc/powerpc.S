/*	$Id: powerpc.S,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2000 Opsycon AB  (http://www.opsycon.se)
 * Copyright (c) 2000 RTMX, Inc   (http://www.rtmx.com)
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

#include <machine/psl.h>
#include <machine/mpc_regs.h>
#include <machine/frame.h>

#include <target/pmon_target.h>


#define HIADJ(x)        (x)@ha
#define HI(x)    	(x)@h
#define LO(x)           (x)@l 

#define	HID0	1008
#define	L2CR	1017

	.data

/* BAT loadup when trap is handled */
	.globl	trapbat
	.common	trapbat, 8*8

/* retvalue and longjump buffer used by exiting programs. */
	.globl	retvalue
retvalue:
	.word	0
	.globl	go_return_jump
	.common	go_return_jump, 24*4


/*
 *  Restore CPU state to DBGREG and GO! Note that this has to be done
 *  with address translation disabled since we are changing the mapping
 *  registers. Mapping state is restored when the MSR is restored.
 */
	.text
	.globl	_go
_go:
	mfmsr	r2			/* Disable mapping and interrupts */
	lis	r3, HI(PSL_DR|PSL_IR|PSL_ME|PSL_RI|PSL_EE)
	ori	r3, r3, LO(PSL_DR|PSL_IR|PSL_ME|PSL_RI|PSL_EE)
	andc	r2, r2, r3
	mtmsr	r2
	isync

	CPUINFOADDR(r1)

	or	r3, r1, r1
	bl	md_fprestore

	lwz	r2, (36*4)(r1)		/* srr0 */
	lwz	r3, (37*4)(r1)		/* srr1 */
	lwz	r4, (35*4)(r1)		/* ctr */
	lwz	r5, (34*4)(r1)		/* xer */
	lwz	r6, (32*4)(r1)		/* lr */
	stw	r2, TEMPSAVE(r1)
	stw	r3, TEMPSAVE+4(r1)
	mtctr	r4
	mtxer	r5
	mtlr	r6
	lwz	r0, (0*4)(r1)
	lmw	r4, (4*4)(r1)		/* Restore regs R4-R31 */
	lwz	r2, (1*4)(r1)
	mtsprg	1, r2			/* R1 and R2 restored from sprg */
	lwz	r2, (2*4)(r1)
	mtsprg	2, r2

/* Restore BAT mapping. Do inline to preserve lr */

	addi	r3, r1, (128*4)
	lwz	r2, 0(r3)
	mtibatl 0, r2
	lwz	r2, 4(r3)
	mtibatu 0, 2
	lwz	r2, 8(r3)
	mtibatl 1, r2
	lwz	r2, 12(r3)
	mtibatu 1, r2
	lwz	r2, 16(r3)
	mtibatl 2, r2
	lwz	r2, 20(r3)
	mtibatu 2, r2
	lwz	r2, 24(r3)
	mtibatl 3, r2
	lwz	r2, 28(r3)
	mtibatu 3, r2

	lwz	r2, 32(r3)
	mtdbatl 0, r2
	lwz	r2, 36(r3)
	mtdbatu 0, r2
	lwz	r2, 40(r3)
	mtdbatl 1, r2
	lwz	r2, 44(r3)
	mtdbatu 1, r2
	lwz	r2, 48(r3)
	mtdbatl 2, r2
	lwz	r2, 52(r3)
	mtdbatu 2, r2
	lwz	r2, 56(r3)
	mtdbatl 3, r2
	lwz	r2, 60(r3)
	mtdbatu 3, r2

	lwz	r2, (33*4)(r1)		/* cr */
	mtcr	r2
	lwz	r3, (3*4)(r1)		/* r3 */
	lwz	r2, TEMPSAVE+4(r1)
	lwz	r1, TEMPSAVE(r1)
	mtsrr0	r1
	mtsrr1	r2
	mfsprg	r1, 1
	mfsprg	r2, 2

	rfi				/* HERE WE GO! */

/*
 *  Address put on top of stack to catch a program returning
 *  so it gracefully exits to PMON2000 instead of crashing.
 *  In SMP mode if this is not CPU 1 throw it into smpidle.
 */
	.global	_exit
_exit:
	mr	r12, r3
#if defined(SMP)
	bl	tgt_smpwhoami
	cmpwi	r3, 0
	beq	1f
	tweq	r1, r1	
	/*noreturn*/
1:
#endif
	lis	r2, HIADJ(retvalue)
	stw	r12, LO(retvalue)(r2)
	lis	r3, HIADJ(go_return_jump)
	addi	r3, r3, LO(go_return_jump)
	li	r4, 1
	bl	longjmp
	/*noreturn*/

/*
 *  Generate a break to trap into PMON via exception.
 */
	.globl	_pmon_break
_pmon_break:
	tweq	r1, r1	
	blr

#if defined(SMP)
/*
 *  Take a snapshot of the CPU for smp forking.
 */
	.globl	_pmon_snap
	.globl	_pmon_snap_trap
_pmon_snap:
	li	r3, 0		/* This will be the return value */
_pmon_snap_trap:
	tweq	r1, r1	
	blr
#endif


/*
 *  External interrupt vector code copied to vector area at startup.
 *  This code is supposed to handle nested interrupts to a limit.
 */

	.text
	.globl	eintrcode
eintrcode:
	mtsprg	1, r1	 		/* Save SP */
	stmw	r28, TEMPSAVE(r0)	/* Save registers R28-R31 */
	mflr	r28	 		/* Save LR */
	mfcr	r29	 		/* Save CR */
	mfxer	r30			/* Save XER */

#if 0
	lis	r31, HIADJ(pmon_intstack)/* Setup interrupt stack */
	addi	r31, r31, LO(pmon_intstack)
#endif
	cmpw	r31, r1			/* If sp is less than intstack */
	bgt	1f			/* nested int, get current stack */
	mr	r1, r31			
1:
	stwu	r1, -88(r1)
	stw	r0, 84(r1)
	mfsprg	r0, 1
	stw	r0, 0(r1)
	stw	r3, 80(r1)
	stw	r4, 76(r1)
	stw	r5, 72(r1)
	stw	r6, 68(r1)
	stw	r7, 64(r1)
	stw	r8, 60(r1)
	stw	r9, 56(r1)
	stw	r10, 52(r1)
	stw	r11, 48(r1)
	stw	r12, 44(r1)
	stw	r28, 40(r1)
	stw	r29, 36(r1)
	stw	r30, 32(r1)
	lmw	r28, TEMPSAVE(r0)
	mfctr	r6
	mfsrr0	r4
	mfsrr1	r3
	stw	r6, 28(r1)
	stw	r5, 20(r1)
	stw	r4, 12(r1)
	stw	r3, 8(r1)

	mfmsr	r5
	ori	r5, r5, (PSL_IR|PSL_DR|PSL_RI)
	mtmsr	r5
	isync
	bla	extint_handler
/**/
	mfmsr	r5
	andi.	r5, r5, LO(PSL_EE|PSL_ME|PSL_RI|PSL_DR|PSL_IR)
	mtmsr	r5
	isync
	lwz	r12, 44(r1)
	lwz	r11, 48(r1)
	lwz	r10, 52(r1)
	lwz	r9, 56(r1)
	lwz	r8, 60(r1)
	lwz	r7, 64(r1)

	lwz	r6, 8(r1)
	lwz	r5, 12(r1)
	lwz	r4, 28(r1)
	lwz	r3, 32(r1)
	mtsrr1	r6
	mtsrr0	r5
	mtctr	r4
	mtxer	r3
	
	lwz	r5, 36(r1)
	lwz	r6, 40(r1)
	mtcr	r5
	mtlr	r6
	lwz	r6, 68(r1)
	lwz	r5, 72(r1)
	lwz	r4, 76(r1)
	lwz	r3, 80(r1)
	lwz	r0, 84(r1)
	lwz	r1, 0(r1)
	rfi
	.globl	eintrsize
eintrsize = .-eintrcode



/*
 *  Common trap catcher. Non special handling is dispatched here.
 *  Save CPU state on stack, NOT CPU state area if this is PMON2000
 *  trapping. Otherwise we may destroy debugging info.
 */
trap_catch:
	stmw	30, TEMPSAVE+24(r1)

	addi	r1, r1, STACKSIZE-1024
	mfsprg	r31, 1			/* Saved stack pointer */
	stwu	r31, -8(r1)		/* Frame linkage */
	stw	r0,  (0*4)+8(r1)	/* R0 */
	stw	r31, (1*4)+8(r1)	/* R1 (sp) */	
	stw	r28, (32*4)+8(r1)	/* LR */
	stw	r29, (33*4)+8(r1)	/* CR */
	lmw	r28, TEMPSAVE-(STACKSIZE-1032)(r1)	/* Load up R28-R31 */
	stmw	r2,  (2*4)+8(r1)	/* And save away R2-R31 */
	lmw	r28, TEMPSAVE+16-(STACKSIZE-1032)(r1)	/* Get DAR, DSISR, SRR0 and SRR1 */
	mfxer	r3
	mfctr	r4
	mflr	r5			/* Get vector offset from LR */
	andi.	r5, 5, 0xff00		/* Make exception code */
	stw	r3,  (34*4)+8(r1)	/* XER */
	stw	r4,  (35*4)+8(r1)	/* CTR */
	stw	r5,  (40*4)+8(r1)	/* EXC */
	stw	r30, (36*4)+8(r1)	/* SRR0 */
	stw	r31, (37*4)+8(r1)	/* SRR1 */
	stw	r28, (38*4)+8(r1)	/* DAR */
	stw	r29, (39*4)+8(r1)	/* DSISR */
/* Save BAT mapping and set up PMON mapping */
	addi	r3, r1, (128*4)+8
	bl	savebat

	lis	r3, HIADJ(trapbat)
	addi	r3, r3, LO(trapbat)
	bl	loadbat

/* Enable machine check, address translation and tag exceptions recoverable */
	mfmsr	r7
	ori	r7, r7, (PSL_FP|PSL_ME|PSL_IR|PSL_DR|PSL_RI)
	mtmsr	r7
	isync

	addi	r3, r1, 0x8		/* trap(frame) */
	bl	exception
	nop				/* so backtrace finds right function */
	/* NOTE: We never return here! */

/* Load up the bat registers */
	.globl	loadbat
loadbat:
	lwz	r2, 0(r3)
	mtibatl 0, r2
	lwz	r2, 4(r3)
	mtibatu 0, r2
	lwz	r2, 8(r3)
	mtibatl 1, r2
	lwz	r2, 12(r3)
	mtibatu 1, r2
	lwz	r2, 16(r3)
	mtibatl 2, r2
	lwz	r2, 20(r3)
	mtibatu 2, r2
	lwz	r2, 24(r3)
	mtibatl 3, r2
	lwz	r2, 28(r3)
	mtibatu 3, r2

	lwz	r2, 32(r3)
	mtdbatl 0, r2
	lwz	r2, 36(r3)
	mtdbatu 0, r2
	lwz	r2, 40(r3)
	mtdbatl 1, r2
	lwz	r2, 44(r3)
	mtdbatu 1, r2
	lwz	r2, 48(r3)
	mtdbatl 2, r2
	lwz	r2, 52(r3)
	mtdbatu 2, r2
	lwz	r2, 56(r3)
	mtdbatl 3, r2
	lwz	r2, 60(r3)
	mtdbatu 3, r2
	blr

/* Save the bat registers */
	.globl	savebat
savebat:
	mfibatl r2, 0
	stw	r2, 0(r3)
	mfibatu r2, 0
	stw	r2, 4(r3)
	mfibatl r2, 1
	stw	r2, 8(r3)
	mfibatu r2, 1
	stw	r2, 12(r3)
	mfibatl r2, 2
	stw	r2, 16(r3)
	mfibatu r2, 2
	stw	r2, 20(r3)
	mfibatl r2, 3
	stw	r2, 24(r3)
	mfibatu r2, 3
	stw	r2, 28(r3)

	mfdbatl r2, 0
	stw	r2, 32(r3)
	mfdbatu r2, 0
	stw	r2, 36(r3)
	mfdbatl r2, 1
	stw	r2, 40(r3)
	mfdbatu r2, 1
	stw	r2, 44(r3)
	mfdbatl r2, 2
	stw	r2, 48(r3)
	mfdbatu r2, 2
	stw	r2, 52(r3)
	mfdbatl r2, 3
	stw	r2, 56(r3)
	mfdbatu r2, 3
	stw	r2, 60(r3)
	blr



/*
 *  Default Exception Vector Code copied to vector area at startup.
 */

	.text
	.globl	trapcode
trapcode:
	mtsprg	1, r1	 		/* Save SP */
	CPUINFOADDR(r1)
	stmw	r28, TEMPSAVE(r1)	/* Save registers R28-R31 */
	mflr	r28	 		/* Save LR */
	mfcr	r29	 		/* Save CR */

	mfsrr0	r30	 		/* PC */
	mfsrr1	r31	 		/* SR */
	bla	trap_catch 		/* Go do trap processing */

	.globl	trapsize
trapsize = .-trapcode

/*
 *  DSI and ISI trap handlers for excercising the BAT maping regs.
 *  This code is moved down to respecive vector when in PMON mode.
 */
	.globl	isitrap, isisize
isitrap:
	mtsprg	1, r1			/* Save SP */
	CPUINFOADDR(r1)
	stmw	r28, DISISAVE(r1)	/* free up a couple of regs */
	mflr	r28
	mfcr	r29
	mfsrr1	r31
	mtcr	r31
	bc	12, 17, 1f		/* PSL_PR set (should not) */

	mfsrr0	r31			/* Fault address */
	rlwinm	r31, r31, 7, 25, 28	/* Get segment nr * 8 */
	addis	r31, r31, HIADJ(battable)
	lwz	r30, LO(battable)(r31)
	mtcr	r30
	bc	4, 30, 1f		/* Not a valid sement */

	addi	r31, r31, 4		/* Yes, this is correct, don't change */
	lwz	r31, LO(battable)(r31)
	mtibatu	3, r30
	mtibatl 3, r31

	mtcr	r29
	lmw	r28, DISISAVE(r1)
	mfsprg	r1, 1			/* Restore SP */
	rfi

1:
	mtcr	r29
	lmw	r28, DISISAVE(r1)
	stmw	r28, TEMPSAVE(r1)	/* Save registers R28-R31 */
	mflr	r28			/* Save LR */
	mfcr	r29			/* Save CR */

	mfsrr0	r30			/* PC */
	mfsrr1	r31			/* SR */
	bla	trap_catch		/* Go do trap processing */
isisize = .-isitrap

	.globl	dsitrap, dsisize
dsitrap:
	mtsprg	1, r1			/* Save SP */
	CPUINFOADDR(r1)
	stmw	r28, DISISAVE(r1)	/* free up a couple of regs */
	mfcr	r29
	mfxer	r30
	mtsprg	2, r30
	mfsrr1	r31
	mtcr	r31
	bc	12, 17, 1f		/* PSL_PR set (should not) */

	mfdar	r31			/* Fault address */
	rlwinm	r31, r31, 7, 25, 28	/* Get segment nr * 8 */
	addis	r31, r31, HIADJ(battable)
	lwz	r30, LO(battable)(r31)
	mtcr	r30
	bc	4, 30, 1f		/* Not a valid sement */

	addi	r31, r31, 4		/* Yes, this is correct, don't change */
	lwz	r31, LO(battable)(r31)
	mtdbatu	3, r30
	mtdbatl 3, r31

	mfsprg	r30, 2
	mtxer	r30
	mtcr	r29
	lmw	r28, DISISAVE(r1)
	mfsprg	r1, 1			/* Restore SP */
	rfi

1:
	mtcr	r29
	mfsprg	r30, 2
	mtxer	r30
	lmw	r28, DISISAVE(r1)
	stmw	r28, TEMPSAVE(r1)	/* Save registers R28-R31 */
	mflr	r28			/* Save LR */
	mfcr	r29

	mfsrr0	r30			/* PC */
	mfsrr1	r31			/* SR */
	bla	trap_catch		/* Go do trap processing */
dsisize = .-dsitrap


/*
 *  Floating point save/restore.
 *
 *  fprestore(&fpsavearea);
 *  fpsave(&fpsavearea);
 */
	.text
	.globl	md_fprestore
md_fprestore:
	mfmsr	r4
	ori	r5, r4, PSL_FP
	mtmsr	r5
	isync
	addi	r3, r3, 64*4		/* Offset in frame to float registers */
	lfd	f0, -8(r3)
	mtfsf	0xff, f0
	lfd	f0, 0(r3)
	lfd	f1, 8(r3)
	lfd	f2, 16(r3)
	lfd	f3, 24(r3)
	lfd	f4, 32(r3)
	lfd	f5, 40(r3)
	lfd	f6, 48(r3)
	lfd	f7, 56(r3)
	lfd	f8, 64(r3)
	lfd	f9, 72(r3)
	lfd	f10, 80(r3)
	lfd	f11, 88(r3)
	lfd	f12, 96(r3)
	lfd	f13, 104(r3)
	lfd	f14, 112(r3)
	lfd	f15, 120(r3)
	lfd	f16, 128(r3)
	lfd	f17, 136(r3)
	lfd	f18, 144(r3)
	lfd	f19, 152(r3)
	lfd	f20, 160(r3)
	lfd	f21, 168(r3)
	lfd	f22, 176(r3)
	lfd	f23, 184(r3)
	lfd	f24, 192(r3)
	lfd	f25, 200(r3)
	lfd	f26, 208(r3)
	lfd	f27, 216(r3)
	lfd	f28, 224(r3)
	lfd	f29, 232(r3)
	lfd	f30, 240(r3)
	lfd	f31, 248(r3)
	mtmsr	r4
	isync
	blr

	.text
	.globl	md_fpsave
md_fpsave:
	mfmsr	r4
	ori	r5, r4, PSL_FP
	mtmsr	r5
	isync
	addi	r3, r3, 64*4		/* Offset in frame to float registers */
	stfd	f0, 0(r3)
	stfd	f1, 8(r3)
	stfd	f2, 16(r3)
	stfd	f3, 24(r3)
	stfd	f4, 32(r3)
	stfd	f5, 40(r3)
	stfd	f6, 48(r3)
	stfd	f7, 56(r3)
	stfd	f8, 64(r3)
	stfd	f9, 72(r3)
	stfd	f10, 80(r3)
	stfd	f11, 88(r3)
	stfd	f12, 96(r3)
	stfd	f13, 104(r3)
	stfd	f14, 112(r3)
	stfd	f15, 120(r3)
	stfd	f16, 128(r3)
	stfd	f17, 136(r3)
	stfd	f18, 144(r3)
	stfd	f19, 152(r3)
	stfd	f20, 160(r3)
	stfd	f21, 168(r3)
	stfd	f22, 176(r3)
	stfd	f23, 184(r3)
	stfd	f24, 192(r3)
	stfd	f25, 200(r3)
	stfd	f26, 208(r3)
	stfd	f27, 216(r3)
	stfd	f28, 224(r3)
	stfd	f29, 232(r3)
	stfd	f30, 240(r3)
	stfd	f31, 248(r3)
	mffs	f0
	stfd	f0, -8(r3)
	mtmsr	r4
	isync
	blr

/*
 *  Function to do 64 bit accesses using FP regs.
 *  Needed by Flash programming code.
 *
 *  movequad(u_int64_t *where, u_int64_t *what);
 */
	.text
	.globl	movequad
movequad:
	mfmsr	r5
	ori	r6, r5, PSL_FP
	mtmsr	r6
	isync

	lfd	f0, 0(r4)
	stfd	f0, 0(r3)

	mtmsr	r5
	isync
	blr

/*
 *  Execute 1000000 instructions.
 */
	.text
	.globl	loopforameg
loopforameg:
	lis	r0, 0x000f
	ori	r0, r0, 0x4240-4
	mtctr	r0
1:
	bdnz	1b
	blr

/*
 *  Loop for N instructions
 */
	.text
	.globl	loopNinstr
loopNinstr:
	mtctr	r3
1:
	bdnz	1b
	blr

/*
 *  Returns address to this CPUs cpuinfo area.
 */
	.globl	md_getcpuinfoptr
md_getcpuinfoptr:
	CPUINFOADDR(r3)
	blr


/*
 *  Get value from cputype register
 */
	.text
	.globl	md_cputype
md_cputype:
	mfspr	r3, 287
	blr

/*
 * get/set the L2CR register.
 */

	.globl	md_get_l2cr
md_get_l2cr:
	sync
	isync
	mfspr	r3, L2CR
	sync
	isync
	blr

	.globl	md_set_l2cr
md_set_l2cr:
	sync
	isync
	mtspr	L2CR, r3
	sync
	isync
	blr

/*
 *   cache stuff
 */
        .globl  md_l1_enable
md_l1_enable:
        isync
        or      r4, r3, r3
        mfspr   r3, HID0
        isync
        or      r3, r3, r4
        mtspr   HID0, 3
        isync
        blr

        .globl  md_l1_disable
md_l1_disable:
	isync
	or	r4, r3, r3
	mfspr	r3, HID0
	isync
	andc.	r4, r3, r4
	mtspr	HID0, r4
	isync
	blr

        .globl  md_l2_enable
md_l2_enable:
	isync
	mfspr	r4, L2CR
	or	r3, r4, r3
	isync
	mtspr	L2CR, r3
	isync
	blr

        .globl  md_l2_disable
md_l2_disable:
        isync
        mfspr   r4, L2CR
        andc.	r4, r4, r3
        isync
        mtspr   L2CR, r4
        isync
        blr

/*
 *  Size 750 L2 backside cache. The way to do the sizing is 
 *  to run the L2 cache in test mode, filling it with the line
 *  index, one for each line. The total number of lines with
 *  correct index read back will give the size of the L2 cache.
 */

	.globl	md_size_cache
md_size_cache:

	lis	r3, 0x0020		/* Use memadr @ 2MB */
	li	r4, 0
	lis	r5, 0x0020		/* Max size to scan */

_size_cache_init:
	dcbz	r4, r3			/* Clear cacheline */
	stwx	r4, r4, r3
	dcbf	r4, r3
	addi	r4, r4, 32		/* XXX cache line size */
	cmpw	r4, r5
	bne	_size_cache_init

	li	r4, 0
	li	r7, 0

_size_cache_count:
	lwzx	r6, r4, r3
	cmpw	r6, r4
	bne	_dont_count
	addi	r7, r7, 32
	dcbi	r4, r3			/* invalidate line */
_dont_count:
	addi	r4, r4, 32		/* XXX cacheline */
	cmpw	r4, r5
	bne	_size_cache_count

	or	r3, r7, r7

	blr

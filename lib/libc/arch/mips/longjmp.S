/*	$OpenBSD$ */

/*
 * Copyright (c) 2000 Opsycon AB  (www.opsycon.se)
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

#define	ABICALLS			# NO ABICALLS
#define _KERNEL
#include <machine/asm.h>

	.set	noreorder		# Noreorder is default style!

/*
 * Amount to take off of the stack for the benefit of the debugger.
 */
#define START_FRAME	((4 * 4) + 4 + 4)

#if __mips < 3
#define	STORE	sw	/* 32 bit mode regsave instruction */
#define	LOAD	lw	/* 32 bit mode regload instruction */
#define	RSIZE	4	/* 32 bit mode register size */
#else
#define	STORE	sd	/* 64 bit mode regsave instruction */
#define	LOAD	ld	/* 64 bit mode regload instruction */
#define	RSIZE	8	/* 64 bit mode register size */
#endif

/*  WARNING! If size of jmpbuf changes change go_return_jump as well! */

LEAF(setjmp)
	STORE	s0, RSIZE * 0(a0)
	STORE	s1, RSIZE * 1(a0)
	STORE	s2, RSIZE * 2(a0)
	STORE	s3, RSIZE * 3(a0)
	STORE	s4, RSIZE * 4(a0)
	STORE	s5, RSIZE * 5(a0)
	STORE	s6, RSIZE * 6(a0)
	STORE	s7, RSIZE * 7(a0)
	STORE	s8, RSIZE * 8(a0)
	STORE	sp, RSIZE * 9(a0)
	STORE	gp, RSIZE * 10(a0)
	STORE	ra, RSIZE * 11(a0)
	jr	ra
	li	v0, 0			# setjmp return
END(setjmp)

LEAF(longjmp)
	LOAD	ra, RSIZE * 11(a0)
	LOAD	s0, RSIZE * 0(a0)
	LOAD	s1, RSIZE * 1(a0)
	LOAD	s2, RSIZE * 2(a0)
	LOAD	s3, RSIZE * 3(a0)
	LOAD	s4, RSIZE * 4(a0)
	LOAD	s5, RSIZE * 5(a0)
	LOAD	s6, RSIZE * 6(a0)
	LOAD	s7, RSIZE * 7(a0)
	LOAD	s8, RSIZE * 8(a0)
	LOAD	sp, RSIZE * 9(a0)
	LOAD	gp, RSIZE * 10(a0)
	jr	ra
	move v0,a1 #li	v0, 1			# longjmp return
END(longjmp)


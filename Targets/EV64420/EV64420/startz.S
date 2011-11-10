/*	$Id: startz.S,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

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

#define GTINIT(offset, value) \
		.word	GT_BASE_ADDR+(offset), HTOLE32(value)

	.set	noreorder

	.globl	_start
	.globl	start
_start:
start:
	la	gp, _gp
	move	a3,	a0
	li	a0, 'S'
	bal	tgt_putchar
	nop
	li	a0, 'T'
	bal	tgt_putchar
	nop
	li	a0, 'G'
	bal tgt_putchar
	nop
	li	a0, '2'
	bal tgt_putchar
	nop
	li	a0, '\r'
	bal tgt_putchar
	nop
	li	a0, '\n'
	bal tgt_putchar
	nop
	
	/* Clear BSS */
	la	a0, _edata
	la	a2, _end
2:	sw	zero, 0(a0)
	bne a2, a0, 2b
	addu	a0, 4

	sw	zero,CpuTertiaryCacheSize
	move	a0,	a3
	la	v0, initmips
	jr	v0
	nop

	/*
	 *	Set DEVPAR for device bus timing.
	 */
	
		.globl	tgt_setpar125mhz
	tgt_setpar125mhz:
		move	a0, ra		/* Don't put in delay slot! */
		bal do_table	/* Load address to init table */
		nop
	
		/* Device CS0 - PLD */
			GTINIT(DEVICE_BANK0PARAMETERS, \
					GT_DEVPAR_TurnOff(2) |		\
					GT_DEVPAR_AccToFirst(8) |	\
					GT_DEVPAR_AccToNext(8)	|	\
					GT_DEVPAR_ALEtoWr(3) |		\
					GT_DEVPAR_WrActive(3) | 	\
					GT_DEVPAR_WrHigh(5) |		\
					GT_DEVPAR_DevWidth8 |		\
					GT_DEVPAR_Reserved)
	
		/* Device CS1 - RTC */
			GTINIT(DEVICE_BANK1PARAMETERS, \
					GT_DEVPAR_TurnOff(2) |		\
					GT_DEVPAR_AccToFirst(13) |	\
					GT_DEVPAR_AccToNext(13) |	\
					GT_DEVPAR_ALEtoWr(5) |		\
					GT_DEVPAR_WrActive(7) | 	\
					GT_DEVPAR_WrHigh(5) |		\
					GT_DEVPAR_DevWidth8 |		\
					GT_DEVPAR_Reserved)
	
		/* Device CS2 - UART */
			GTINIT(DEVICE_BANK2PARAMETERS, \
					GT_DEVPAR_TurnOff(3) |		\
					GT_DEVPAR_AccToFirst(15) |	\
					GT_DEVPAR_AccToNext(15) |	\
					GT_DEVPAR_ALEtoWr(5) |		\
					GT_DEVPAR_WrActive(8) | 	\
					GT_DEVPAR_WrHigh(5) |		\
					GT_DEVPAR_DevWidth8 |		\
					GT_DEVPAR_Reserved)
					
		/* end mark */
		.word	0, 0
	
		.globl	tgt_setpar100mhz
	tgt_setpar100mhz:
		move	a0, ra		/* Don't put in delay slot! */
		bal do_table	/* Load address to init table */
		nop
	
		/* Device CS0 - PLD */
			GTINIT(DEVICE_BANK0PARAMETERS, \
					GT_DEVPAR_TurnOff(3) |		\
					GT_DEVPAR_AccToFirst(6) |	\
					GT_DEVPAR_AccToNext(6)	|	\
					GT_DEVPAR_ALEtoWr(3) |		\
					GT_DEVPAR_WrActive(3) | 	\
					GT_DEVPAR_WrHigh(5) |		\
					GT_DEVPAR_DevWidth8 |		\
					GT_DEVPAR_Reserved)
	
		/* Device CS1 - NVRAM */
			GTINIT(DEVICE_BANK1PARAMETERS, \
					GT_DEVPAR_TurnOff(3) |		\
					GT_DEVPAR_AccToFirst(10) |	\
					GT_DEVPAR_AccToNext(10) |	\
					GT_DEVPAR_ALEtoWr(5) |		\
					GT_DEVPAR_WrActive(6) | 	\
					GT_DEVPAR_WrHigh(5) |		\
					GT_DEVPAR_DevWidth8 |		\
					GT_DEVPAR_Reserved)
	
		/* Device CS2 - UART */
			GTINIT(DEVICE_BANK2PARAMETERS, \
					GT_DEVPAR_TurnOff(4) |		\
					GT_DEVPAR_AccToFirst(11) |	\
					GT_DEVPAR_AccToNext(11) |	\
					GT_DEVPAR_ALEtoWr(5) |		\
					GT_DEVPAR_WrActive(6) | 	\
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

